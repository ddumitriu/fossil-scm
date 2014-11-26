/*
** Copyright (c) 2014 D. Richard Hipp
**
** This program is free software; you can redistribute it and/or
** modify it under the terms of the Simplified BSD License (also
** known as the "2-Clause License" or "FreeBSD License".)

** This program is distributed in the hope that it will be useful,
** but without any warranty; without even the implied warranty of
** merchantability or fitness for a particular purpose.
**
** Author contact information:
**   drh@hwaci.com
**   http://www.hwaci.com/drh/
**
*******************************************************************************
**
** This file contains code used to implement and manage a "bundle" file.
*/
#include "config.h"
#include "bundle.h"
#include <assert.h>

/*
** SQL code used to initialize the schema of a bundle.
**
** The bblob.delta field can be an integer, a text string, or NULL.
** If an integer, then the corresponding blobid is the delta basis.
** If a text string, then that string is a SHA1 hash for the delta
** basis, which is presumably in the master repository.  If NULL, then
** data contains contain without delta compression.
*/
static const char zBundleInit[] = 
@ CREATE TABLE IF NOT EXISTS "%w".bconfig(
@   bcname TEXT,
@   bcvalue ANY
@ );
@ CREATE TABLE IF NOT EXISTS "%w".bblob(
@   blobid INTEGER PRIMARY KEY,      -- Blob ID
@   uuid TEXT NOT NULL,              -- SHA1 hash of expanded blob
@   sz INT NOT NULL,                 -- Size of blob after expansion
@   delta ANY,                       -- Delta compression basis, or NULL
@   data BLOB                        -- compressed content
@ );
;

/*
** Attach a bundle file to the current database connection using the
** attachment name zBName.
*/
static void bundle_attach_file(
  const char *zFile,       /* Name of the file that contains the bundle */
  const char *zBName,      /* Attachment name */
  int doInit               /* Initialize a new bundle, if true */
){
  db_multi_exec("ATTACH %Q AS %Q;", zFile, zBName);
  db_multi_exec(zBundleInit /*works-like:"%w%w"*/, zBName, zBName);
}

/*
**  fossil bundle ls BUNDLE ?OPTIONS?
**
** Display the content of a bundle in human-readable form.
*/
static void bundle_ls_cmd(void){
  Stmt q;
  sqlite3_int64 sumSz = 0;
  sqlite3_int64 sumLen = 0;
  bundle_attach_file(g.argv[3], "b1", 0);
  db_prepare(&q,
    "SELECT bcname, bcvalue FROM bconfig"
    " WHERE typeof(bcvalue)='text'"
    "   AND bcvalue NOT GLOB char(0x2a,0x0a,0x2a);"
  );
  while( db_step(&q)==SQLITE_ROW ){
    fossil_print("%s: %s\n", db_column_text(&q,0), db_column_text(&q,1));
  }
  db_finalize(&q);
  db_prepare(&q,
    "SELECT blobid, substr(uuid,1,16), coalesce(substr(delta,1,16),''),"
    "       sz, length(data)"
    "  FROM bblob"
  );
  while( db_step(&q)==SQLITE_ROW ){
    fossil_print("%4d %16s %16s %10d %10d\n",
      db_column_int(&q,0),
      db_column_text(&q,1),
      db_column_text(&q,2),
      db_column_int(&q,3),
      db_column_int(&q,4));
    sumSz += db_column_int(&q,3);
    sumLen += db_column_int(&q,4);
  }
  db_finalize(&q);
  fossil_print("%39s %10lld %10lld\n", "Total:", sumSz, sumLen);
}

/*
** Implement the "fossil bundle append BUNDLE FILE..." command.  Add
** the named files into the BUNDLE.  Create the BUNDLE if it does not
** alraedy exist.
*/
static void bundle_append_cmd(void){
  char *zFilename;
  Blob content, hash;
  int i;
  Stmt q;

  verify_all_options();
  bundle_attach_file(g.argv[3], "b1", 1);
  db_prepare(&q, 
    "INSERT INTO bblob(blobid, uuid, sz, delta, data) "
    "VALUES(NULL, $uuid, $sz, NULL, $data)");
  db_begin_transaction();
  for(i=4; i<g.argc; i++){
    int sz;
    blob_read_from_file(&content, g.argv[i]);
    sz = blob_size(&content);
    sha1sum_blob(&content, &hash);
    blob_compress(&content, &content);
    db_bind_text(&q, "$uuid", blob_str(&hash));
    db_bind_int(&q, "$sz", sz);
    db_bind_blob(&q, "$data", &content);
    db_step(&q);
    db_reset(&q);
    blob_reset(&content);
    blob_reset(&hash);
  }
  db_end_transaction(0);
  db_finalize(&q);
}

/*
** Identify a subsection of the checkin tree using command-line switches.
** There must be one of the following switch available:
**
**     --branch BRANCHNAME          All checkins on the most recent
**                                  instance of BRANCHNAME
**     --from TAG1 [--to TAG2]      Checkin TAG1 and all primary descendants
**                                  up to and including TAG2
**     --checkin TAG                Checkin TAG only
**
** Store the RIDs for all applicable checkins in the zTab table that 
** should already exist.  Invoke fossil_fatal() if any kind of error is
** seen.
*/
void subtree_from_arguments(const char *zTab){
  const char *zBr;
  const char *zFrom;
  const char *zTo;
  const char *zCkin;
  int rid, endRid;

  zBr = find_option("branch",0,1);
  zFrom = find_option("from",0,1);
  zTo = find_option("to",0,1);
  zCkin = find_option("checkin",0,1);
  if( zCkin ){
    if( zFrom ) fossil_fatal("cannot use both --checkin and --from");
    if( zBr ) fossil_fatal("cannot use both --checkin and --branch");
    rid = symbolic_name_to_rid(zCkin, "ci");
    endRid = rid;
  }else{
    endRid = zTo ? name_to_typed_rid(zTo, "ci") : 0;
  }
  if( zFrom ){
    rid = name_to_typed_rid(zFrom, "ci");
  }else if( zBr ){
    rid = name_to_typed_rid(zBr, "br");
  }else if( zCkin==0 ){
    fossil_fatal("need on of: --branch, --from, --checkin");
  }
  db_multi_exec("INSERT OR IGNORE INTO \"%w\" VALUES(%d)", zTab, rid);
  if( rid!=endRid ){
    Blob sql;
    blob_zero(&sql);
    blob_appendf(&sql,
       "WITH RECURSIVE child(rid) AS (VALUES(%d) UNION ALL "
       "  SELECT cid FROM plink, child"
       "   WHERE plink.pid=child.rid"
       "     AND plink.isPrim", rid);
    if( endRid>0 ){
      double endTime = db_double(0.0, "SELECT mtime FROM event WHERE objid=%d",
                                 endRid);
      blob_appendf(&sql,
        "    AND child.rid!=%d"
        "    AND (SELECT mtime FROM event WHERE objid=plink.cid)<=%.17g",
        endRid, endTime
      );
    }
    if( zBr ){
      blob_appendf(&sql,
         "     AND EXISTS(SELECT 1 FROM tagxref"
                        "  WHERE tagid=%d AND tagtype>0"
                        "    AND value=%Q and rid=plink.cid)",
         TAG_BRANCH, zBr);
    }
    blob_appendf(&sql, ") INSERT OR IGNORE INTO \"%w\" SELECT rid FROM child;",
                 zTab);
    db_multi_exec("%s", blob_str(&sql)/*safe-for-%s*/);
  }
}

/*
** COMMAND: test-subtree
**
** Usage: %fossil test-subtree ?OPTIONS?
**
** Show the subset of checkins that match the supplied options.  This
** command is used to test the subtree_from_options() subroutine in the
** implementation and does not really have any other practical use that
** we know of.
**
** Options:
**    --branch BRANCH           Include only checkins on BRANCH
**    --from TAG                Start the subtree at TAG
**    --to TAG                  End the subtree at TAG
**    --checkin TAG             The subtree is the single checkin TAG
*/
void test_subtree_cmd(void){
  Stmt q;
  db_find_and_open_repository(0,0);
  db_begin_transaction();
  db_multi_exec("CREATE TEMP TABLE tobundle(rid INTEGER PRIMARY KEY);");
  subtree_from_arguments("tobundle");
  db_prepare(&q,
    "SELECT "
    "  (SELECT substr(uuid,1,10) FROM blob WHERE rid=tobundle.rid),"
    "  (SELECT substr(comment,1,30) FROM event WHERE objid=tobundle.rid),"
    "  tobundle.rid"
    " FROM tobundle;"
  );
  while( db_step(&q)==SQLITE_ROW ){
     fossil_print("%5d %s %s\n", 
        db_column_int(&q, 2),
        db_column_text(&q, 0),
        db_column_text(&q, 1));
  }
  db_finalize(&q);
  db_end_transaction(1);
}

/* fossil bundle export BUNDLE ?OPTIONS?
**
** OPTIONS:
**   --branch BRANCH
**   --from TAG
**   --to TAG
**   --checkin TAG
*/
static void bundle_export_cmd(void){
  db_multi_exec("CREATE TEMP TABLE tobundle(rid INTEGER PRIMARY KEY);");
  subtree_from_arguments("tobundle");
  verify_all_options();
  bundle_attach_file(g.argv[3], "b1", 1);
  find_checkin_associates("tobundle");
  db_begin_transaction();
  db_multi_exec(
    "REPLACE INTO bblob(blobid,uuid,sz,delta,data) "
    " SELECT"
    "   tobundle.rid,"
    "   b1.uuid,"
    "   b1.size,"
    "   CASE WHEN delta.srcid NOT IN tobundle"
    "        THEN (SELECT uuid FROM blob WHERE rid=delta.srcid)"
    "        ELSE delta.srcid END,"
    "   b1.content"
    " FROM tobundle"
    "      JOIN blob AS b1 ON b1.rid=tobundle.rid"
    "      LEFT JOIN delta ON delta.rid=tobundle.rid"
  );
  db_multi_exec(
    "INSERT INTO bconfig(bcname,bcvalue)"
    " VALUES('mtime',datetime('now'));"
  );
  db_multi_exec(
    "INSERT INTO bconfig(bcname,bcvalue)"
    " SELECT name, value FROM config"
    "  WHERE name IN ('project-code');"
  );
  db_end_transaction(0);
}


/*
** There is a TEMP table bix(blobid,delta) containing a set of purgeitems
** that need to be transferred to the BLOB table.  This routine does
** all items that have srcid=iSrc.  The pBasis blob holds the content
** of the source document if iSrc>0.
*/
static void bundle_import_elements(int iSrc, Blob *pBasis, int isPriv){
  Stmt q;
  static Bag busy;
  assert( pBasis!=0 || iSrc==0 );
  if( iSrc>0 ){
    if( bag_find(&busy, iSrc) ){
      fossil_fatal("delta loop while uncompressing bundle artifacts");
    }
    bag_insert(&busy, iSrc);
  }
  db_prepare(&q, 
     "SELECT uuid, data, bblob.delta, bix.blobid"
     "  FROM bix, bblob"
     " WHERE bix.delta=%d"
     "   AND bix.blobid=bblob.blobid;",
     iSrc
  );
  while( db_step(&q)==SQLITE_ROW ){
    Blob h1, h2, c1, c2;
    int rid;
    blob_zero(&h1);
    db_column_blob(&q, 0, &h1);
    blob_zero(&c1);
    db_column_blob(&q, 1, &c1);
    blob_uncompress(&c1, &c1);
    blob_zero(&c2);
    if( db_column_type(&q,2)==SQLITE_TEXT && db_column_bytes(&q,2)==40 ){
      Blob basis;
      rid = db_int(0,"SELECT rid FROM blob WHERE uuid=%Q",
                   db_column_text(&q,2));
      content_get(rid, &basis);
      blob_delta_apply(&basis, &c1, &c2);
      blob_reset(&basis);
      blob_reset(&c1);
    }else if( pBasis ){
      blob_delta_apply(pBasis, &c1, &c2);
      blob_reset(&c1);
    }else{
      c2 = c1;
    }
    sha1sum_blob(&c2, &h2);
    if( blob_compare(&h1, &h2)!=0 ){
      fossil_fatal("SHA1 hash mismatch - wanted %s, got %s",
                   blob_str(&h1), blob_str(&h2));
    }
    blob_reset(&h2);
    rid = content_put_ex(&c2, blob_str(&h1), 0, 0, isPriv);
    if( rid==0 ){
      fossil_fatal("%s", g.zErrMsg);
    }else{
      if( !isPriv ) content_make_public(rid);
      content_get(rid, &c1);
      manifest_crosslink(rid, &c1, MC_NO_ERRORS);
    }
    bundle_import_elements(db_column_int(&q,3), &c2, isPriv);
    blob_reset(&c2);
  }
  db_finalize(&q);
  if( iSrc>0 ) bag_remove(&busy, iSrc);
}


/* fossil bundle import BUNDLE ?OPTIONS?
**
** Attempt to import the changes contained in BUNDLE.  Make the change
** private so that they do not sync.
**
** OPTIONS:
**    --force           Import even if the project-code does not match
**    --publish         Imported changes are not private
*/
static void bundle_import_cmd(void){
  int forceFlag = find_option("force","f",0)!=0;
  int isPriv = find_option("publish",0,0)==0;
  char *zMissingDeltas;
  Stmt q;
  verify_all_options();
  bundle_attach_file(g.argv[3], "b1", 1);

  /* Only import a bundle that was generated from a repo with the same
  ** project code, unless the --force flag is true */
  if( !forceFlag ){
    if( !db_exists("SELECT 1 FROM config, bconfig"
                  " WHERE config.name='project-code'"
                  "   AND bconfig.bcname='project-code'"
                  "   AND config.value=bconfig.bcvalue;")
    ){
      fossil_fatal("project-code in the bundle does not match the "
                   "repository project code.  (override with --force).");
    }
  }

  /* If the bundle contains deltas with a basis that is external to the
  ** bundle and those external basis files are missing from the local
  ** repo, then the delta encodings cannot be decoded and the bundle cannot
  ** be extracted. */
  zMissingDeltas = db_text(0,
      "SELECT group_concat(substr(delta,1,10),' ')"
      "  FROM bblob"
      " WHERE typeof(delta)='text' AND length(delta)=40"
      "   AND NOT EXISTS(SELECT 1 FROM blob WHERE uuid=bblob.delta)");
  if( zMissingDeltas && zMissingDeltas[0] ){
    fossil_fatal("delta basis artifacts not found in repository: %s",
                 zMissingDeltas);
  }

  db_begin_transaction();
  db_multi_exec(
    "CREATE TEMP TABLE bix("
    "  blobid INTEGER PRIMARY KEY,"
    "  delta INTEGER"
    ");"
    "CREATE INDEX bixdelta ON bix(delta);"
    "INSERT INTO bix(blobid,delta)"
    "  SELECT blobid,"
    "         CASE WHEN typeof(delta)=='integer'"
    "              THEN delta ELSE 0 END"
    "    FROM bblob"
    "   WHERE NOT EXISTS(SELECT 1 FROM blob WHERE uuid=bblob.uuid);"
  );
  manifest_crosslink_begin();
  bundle_import_elements(0, 0, isPriv);
  manifest_crosslink_end(0);
  db_end_transaction(0);    
}

/*
** COMMAND: bundle
**
** Usage: %fossil bundle SUBCOMMAND ARGS...
**
**   fossil bundle export BUNDLE ?OPTIONS?
**
**      Generate a new bundle, in the file named BUNDLE, that constains a
**      subset of the check-ins in the repository (usually a single branch)
**      as determined by OPTIONS.  OPTIONS include:
**
**         --branch BRANCH            Package all check-ins on BRANCH.
**         --from TAG1 --to TAG2      Package check-ins between TAG1 and TAG2.
**         --m COMMENT                Add the comment to the bundle.
**         --explain                  Just explain what would have happened.
**
**   fossil bundle import BUNDLE ?--publish?
**
**      Import the bundle in file BUNDLE into the repository.  The --publish
**      option makes the import public.  The --explain option makes no changes
**      to the repository but rather explains what would have happened.
**
**   fossil bundle ls BUNDLE
**
**      List the contents of BUNDLE on standard output
**
**   fossil bundle append BUNDLE FILE...
**
**      Add files named on the command line to BUNDLE.  This subcommand has
**      little practical use and is mostly intended for testing.
**
**   fossil bundle cat BUNDLE UUID ?FILE?
**
**      Extract an artifact from the bundle.  Write it into FILE, or onto
**      standard output if FILE is omitted.
**
** SUMMARY:
**   fossil bundle export BUNDLEFILE ?OPTIONS?
**          --branch BRANCH
**          --from TAG1 --to TAG2
**          --explain
**   fossil bundle import BUNDLEFILE ?OPTIONS?
**          --publish
**          --explain
**   fossil bundle ls BUNDLEFILE
*/
void bundle_cmd(void){
  const char *zSubcmd;
  const char *zBundleFile;
  int n;
  if( g.argc<4 ) usage("SUBCOMMAND BUNDLE ?ARGUMENTS?");
  zSubcmd = g.argv[2];
  db_find_and_open_repository(0,0);
  n = (int)strlen(zSubcmd);
  if( strncmp(zSubcmd, "export", n)==0 ){
    bundle_export_cmd();
  }else if( strncmp(zSubcmd, "import", n)==0 ){
    bundle_import_cmd();
  }else if( strncmp(zSubcmd, "ls", n)==0 ){
    bundle_ls_cmd();
  }else if( strncmp(zSubcmd, "append", n)==0 ){
    bundle_append_cmd();
  }else if( strncmp(zSubcmd, "extract", n)==0 ){
    fossil_print("Not yet implemented...\n");
  }else{
    fossil_fatal("unknown subcommand for bundle: %s", zSubcmd);
  }
}
