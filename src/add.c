/*
** Copyright (c) 2007 D. Richard Hipp
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
** This file contains code used to check-out versions of the project
** from the local repository.
*/
#include "config.h"
#include "add.h"
#include <assert.h>
#include <dirent.h>

/*
** Set to true if files whose names begin with "." should be
** included when processing a recursive "add" command.
*/
static int includeDotFiles = 0;

/*
** This routine returns the names of files in a working checkout that
** are created by Fossil itself, and hence should not be added, deleted,
** or merge, and should be omitted from "clean" and "extra" lists.
**
** Return the N-th name.  The first name has N==0.  When all names have
** been used, return 0.
*/
const char *fossil_reserved_name(int N){
  /* Possible names of the local per-checkout database file and
  ** its associated journals
  */
  static const char *azName[] = {
     "_FOSSIL_",
     "_FOSSIL_-journal",
     "_FOSSIL_-wal",
     "_FOSSIL_-shm",
     ".fos",
     ".fos-journal",
     ".fos-wal",
     ".fos-shm",
  };

  /* Names of auxiliary files generated by SQLite when the "manifest"
  ** properity is enabled
  */
  static const char *azManifest[] = {
     "manifest",
     "manifest.uuid",
  };

  if( N>=0 && N<count(azName) ) return azName[N];
  if( N>=count(azName) && N<count(azName)+count(azManifest)
      && db_get_boolean("manifest",0) ){
    return azManifest[N-count(azName)];
  }
  return 0;
}

/*
** Return a list of all reserved filenames as an SQL list.
*/
const char *fossil_all_reserved_names(void){
  static char *zAll = 0;
  if( zAll==0 ){
    Blob x;
    int i;
    const char *z;
    blob_zero(&x);
    for(i=0; (z = fossil_reserved_name(i))!=0; i++){
      if( i>0 ) blob_append(&x, ",", 1);
      blob_appendf(&x, "'%s'", z);
    }
    zAll = blob_str(&x);
  }
  return zAll;
}

/*
** Add a single file named zName to the VFILE table with vid.
**
** Omit any file whose name is pOmit.
*/
static int add_one_file(
  const char *zPath,   /* Tree-name of file to add. */
  int vid,             /* Add to this VFILE */
  int caseSensitive    /* True if filenames are case sensitive */
){
  const char *zCollate = caseSensitive ? "binary" : "nocase";
  if( !file_is_simple_pathname(zPath) ){
    fossil_fatal("filename contains illegal characters: %s", zPath);
  }
  if( db_exists("SELECT 1 FROM vfile"
                " WHERE pathname=%Q COLLATE %s", zPath, zCollate) ){
    db_multi_exec("UPDATE vfile SET deleted=0"
                  " WHERE pathname=%Q COLLATE %s", zPath, zCollate);
  }else{
    char *zFullname = mprintf("%s%s", g.zLocalRoot, zPath);
    db_multi_exec(
      "INSERT INTO vfile(vid,deleted,rid,mrid,pathname,isexe,islink)"
      "VALUES(%d,0,0,0,%Q,%d,%d)",
      vid, zPath, file_wd_isexe(zFullname), file_wd_islink(zFullname));
    fossil_free(zFullname);
  }
  if( db_changes() ){
    fossil_print("ADDED  %s\n", zPath);
    return 1;
  }else{
    fossil_print("SKIP   %s\n", zPath);
    return 0;
  }
}

/*
** Add all files in the sfile temp table.
**
** Automatically exclude the repository file.
*/
static int add_files_in_sfile(int vid, int caseSensitive){
  const char *zRepo;        /* Name of the repository database file */
  int nAdd = 0;             /* Number of files added */
  int i;                    /* Loop counter */
  const char *zReserved;    /* Name of a reserved file */
  Blob repoName;            /* Treename of the repository */
  Stmt loop;                /* SQL to loop over all files to add */
  int (*xCmp)(const char*,const char*);
 
  if( !file_tree_name(g.zRepositoryName, &repoName, 0) ){
    blob_zero(&repoName);
    zRepo = "";
  }else{
    zRepo = blob_str(&repoName);
  }
  if( caseSensitive ){
    xCmp = fossil_strcmp;
  }else{
    xCmp = fossil_stricmp;
    db_multi_exec(
      "CREATE INDEX IF NOT EXISTS vfile_nocase"
      "    ON vfile(pathname COLLATE nocase)"
    );
  }
  db_prepare(&loop, "SELECT x FROM sfile ORDER BY x");
  while( db_step(&loop)==SQLITE_ROW ){
    const char *zToAdd = db_column_text(&loop, 0);
    if( fossil_strcmp(zToAdd, zRepo)==0 ) continue;
    for(i=0; (zReserved = fossil_reserved_name(i))!=0; i++){
      if( xCmp(zToAdd, zReserved)==0 ) break;
    }
    if( zReserved ) continue;
    nAdd += add_one_file(zToAdd, vid, caseSensitive);
  }
  db_finalize(&loop);
  blob_reset(&repoName);
  return nAdd;
}

/*
** COMMAND: add
**
** Usage: %fossil add ?OPTIONS? FILE1 ?FILE2 ...?
**
** Make arrangements to add one or more files or directories to the
** current checkout at the next commit.
**
** When adding files or directories recursively, filenames that begin
** with "." are excluded by default.  To include such files, add
** the "--dotfiles" option to the command-line.
**
** The --ignore option is a comma-separate list of glob patterns for files
** to be excluded.  Example:  '*.o,*.obj,*.exe'  If the --ignore option
** does not appear on the command line then the "ignore-glob" setting is
** used.
**
** Options:
**
**    --dotfiles       include files beginning with a dot (".")   
**    --ignore <CSG>   ignore files matching patterns from the 
**                     comma separated list of glob patterns.
** 
** See also: addremove, rm
*/
void add_cmd(void){
  int i;                     /* Loop counter */
  int vid;                   /* Currently checked out version */
  int nRoot;                 /* Full path characters in g.zLocalRoot */
  const char *zIgnoreFlag;   /* The --ignore option or ignore-glob setting */
  Glob *pIgnore;             /* Ignore everything matching this glob pattern */
  int caseSensitive;         /* True if filenames are case sensitive */

  zIgnoreFlag = find_option("ignore",0,1);
  includeDotFiles = find_option("dotfiles",0,0)!=0;
  capture_case_sensitive_option();
  db_must_be_within_tree();
  caseSensitive = filenames_are_case_sensitive();
  if( zIgnoreFlag==0 ){
    zIgnoreFlag = db_get("ignore-glob", 0);
  }
  vid = db_lget_int("checkout",0);
  if( vid==0 ){
    fossil_panic("no checkout to add to");
  }
  db_begin_transaction();
  db_multi_exec("CREATE TEMP TABLE sfile(x TEXT PRIMARY KEY)");
#if defined(_WIN32)
  db_multi_exec(
     "CREATE INDEX IF NOT EXISTS vfile_pathname "
     "  ON vfile(pathname COLLATE nocase)"
  );
#endif
  pIgnore = glob_create(zIgnoreFlag);
  nRoot = strlen(g.zLocalRoot);
  
  /* Load the names of all files that are to be added into sfile temp table */
  for(i=2; i<g.argc; i++){
    char *zName;
    int isDir;
    Blob fullName;

    file_canonical_name(g.argv[i], &fullName);
    zName = blob_str(&fullName);
    isDir = file_isdir(zName);
    if( isDir==1 ){
      vfile_scan(&fullName, nRoot-1, includeDotFiles, pIgnore);
    }else if( isDir==0 ){
      fossil_fatal("not found: %s", zName);
    }else if( file_access(zName, R_OK) ){
      fossil_fatal("cannot open %s", zName);
    }else{
      char *zTreeName = &zName[nRoot];
      db_multi_exec(
         "INSERT OR IGNORE INTO sfile(x)"
         "  SELECT %Q WHERE NOT EXISTS(SELECT 1 FROM vfile WHERE pathname=%Q)",
         zTreeName, zTreeName
      );
    }
    blob_reset(&fullName);
  }
  glob_free(pIgnore);

  add_files_in_sfile(vid, caseSensitive);
  db_end_transaction(0);
}

/*
** COMMAND: rm
** COMMAND: delete
**
** Usage: %fossil rm FILE1 ?FILE2 ...?
**    or: %fossil delete FILE1 ?FILE2 ...?
**
** Remove one or more files or directories from the repository.
**
** This command does NOT remove the files from disk.  It just marks the
** files as no longer being part of the project.  In other words, future
** changes to the named files will not be versioned.
**
** See also: addremove, add
*/
void delete_cmd(void){
  int i;
  int vid;
  Stmt loop;

  db_must_be_within_tree();
  vid = db_lget_int("checkout", 0);
  if( vid==0 ){
    fossil_panic("no checkout to remove from");
  }
  db_begin_transaction();
  db_multi_exec("CREATE TEMP TABLE sfile(x TEXT PRIMARY KEY)");
  for(i=2; i<g.argc; i++){
    Blob treeName;
    char *zTreeName;

    file_tree_name(g.argv[i], &treeName, 1);
    zTreeName = blob_str(&treeName);
    db_multi_exec(
       "INSERT OR IGNORE INTO sfile"
       " SELECT pathname FROM vfile"
       "  WHERE (pathname=%Q"
       "     OR (pathname>'%q/' AND pathname<'%q0'))"
       "    AND NOT deleted",
       zTreeName, zTreeName, zTreeName
    );
    blob_reset(&treeName);
  }
  
  db_prepare(&loop, "SELECT x FROM sfile");
  while( db_step(&loop)==SQLITE_ROW ){
    fossil_print("DELETED %s\n", db_column_text(&loop, 0));
  }
  db_finalize(&loop);
  db_multi_exec(
    "UPDATE vfile SET deleted=1 WHERE pathname IN sfile;"
    "DELETE FROM vfile WHERE rid=0 AND deleted;"
  );
  db_end_transaction(0);
}

/*
** Capture the command-line --case-sensitive option.
*/
static const char *zCaseSensitive = 0;
void capture_case_sensitive_option(void){
  if( zCaseSensitive==0 ){
    zCaseSensitive = find_option("case-sensitive",0,1);
  }
}

/*
** This routine determines if files should be case-sensitive or not.
** In other words, this routine determines if two filenames that
** differ only in case should be considered the same name or not.
**
** The case-sensitive setting determines the default value.  If
** the case-sensitive setting is undefined, then case sensitivity
** defaults on for Mac and Windows and off for all other unix.
**
** The --case-sensitive BOOLEAN command-line option overrides any
** setting.
*/
int filenames_are_case_sensitive(void){
  int caseSensitive;

  if( zCaseSensitive ){
    caseSensitive = is_truth(zCaseSensitive);
  }else{
#if !defined(_WIN32) && !defined(__DARWIN__) && !defined(__APPLE__)
    caseSensitive = 1;
#else
    caseSensitive = 0;
#endif
    caseSensitive = db_get_boolean("case-sensitive",caseSensitive);
  }
  return caseSensitive;
}

/*
** COMMAND: addremove
**
** Usage: %fossil addremove ?OPTIONS?
**
** Do all necessary "add" and "rm" commands to synchronize the repository
** with the content of the working checkout:
**
**  *  All files in the checkout but not in the repository (that is,
**     all files displayed using the "extra" command) are added as
**     if by the "add" command.
**
**  *  All files in the repository but missing from the checkout (that is,
**     all files that show as MISSING with the "status" command) are
**     removed as if by the "rm" command.
**
** The command does not "commit".  You must run the "commit" separately
** as a separate step.
**
** Files and directories whose names begin with "." are ignored unless
** the --dotfiles option is used.
**
** The --ignore option overrides the "ignore-glob" setting.  See
** documentation on the "settings" command for further information.
**
** The --test option shows what would happen without actually doing anything.
**
** This command can be used to track third party software.
** 
** Options: 
**   --dotfiles       include files beginning with a dot (".")   
**   --ignore <CSG>   ignore files matching patterns from the 
**                    comma separated list of glob patterns.
**   --test           If given, show what would be done without doing so.
**
** See also: add, rm
*/
void addremove_cmd(void){
  Blob path;
  const char *zIgnoreFlag = find_option("ignore",0,1);
  int allFlag = find_option("dotfiles",0,0)!=0;
  int isTest = find_option("test",0,0)!=0;
  int caseSensitive;
  int n;
  Stmt q;
  int vid;
  int nAdd = 0;
  int nDelete = 0;
  Glob *pIgnore;

  capture_case_sensitive_option();
  db_must_be_within_tree();
  caseSensitive = filenames_are_case_sensitive();
  if( zIgnoreFlag==0 ){
    zIgnoreFlag = db_get("ignore-glob", 0);
  }
  vid = db_lget_int("checkout",0);
  if( vid==0 ){
    fossil_panic("no checkout to add to");
  }
  db_begin_transaction();

  /* step 1:  
  ** Populate the temp table "sfile" with the names of all unmanged
  ** files currently in the check-out, except for files that match the
  ** --ignore or ignore-glob patterns and dot-files.  Then add all of
  ** the files in the sfile temp table to the set of managed files.
  */
  db_multi_exec("CREATE TEMP TABLE sfile(x TEXT PRIMARY KEY)");
  n = strlen(g.zLocalRoot);
  blob_init(&path, g.zLocalRoot, n-1);
  /* now we read the complete file structure into a temp table */
  pIgnore = glob_create(zIgnoreFlag);
  vfile_scan(&path, blob_size(&path), allFlag, pIgnore);
  glob_free(pIgnore);
  nAdd = add_files_in_sfile(vid, caseSensitive);

  /* step 2: search for missing files */
  db_prepare(&q,
      "SELECT pathname, %Q || pathname, deleted FROM vfile"
      " WHERE NOT deleted"
      " ORDER BY 1",
      g.zLocalRoot
  );
  while( db_step(&q)==SQLITE_ROW ){
    const char * zFile;
    const char * zPath;

    zFile = db_column_text(&q, 0);
    zPath = db_column_text(&q, 1);
    if( !file_wd_isfile_or_link(zPath) ){
      if( !isTest ){
        db_multi_exec("UPDATE vfile SET deleted=1 WHERE pathname=%Q", zFile);
      }
      fossil_print("DELETED  %s\n", zFile);
      nDelete++;
    }
  }
  db_finalize(&q);
  /* show cmmand summary */
  fossil_print("added %d files, deleted %d files\n", nAdd, nDelete);

  db_end_transaction(isTest);
}


/*
** Rename a single file.
**
** The original name of the file is zOrig.  The new filename is zNew.
*/
static void mv_one_file(int vid, const char *zOrig, const char *zNew){
  fossil_print("RENAME %s %s\n", zOrig, zNew);
  db_multi_exec(
    "UPDATE vfile SET pathname='%s' WHERE pathname='%s' AND vid=%d",
    zNew, zOrig, vid
  );
}

/*
** COMMAND: mv
** COMMAND: rename
**
** Usage: %fossil mv|rename OLDNAME NEWNAME
**    or: %fossil mv|rename OLDNAME... DIR
**
** Move or rename one or more files or directories within the repository tree.
** You can either rename a file or directory or move it to another subdirectory.
**
** This command does NOT rename or move the files on disk.  This command merely
** records the fact that filenames have changed so that appropriate notations
** can be made at the next commit/checkin.
**
** See also: changes, status
*/
void mv_cmd(void){
  int i;
  int vid;
  char *zDest;
  Blob dest;
  Stmt q;

  db_must_be_within_tree();
  vid = db_lget_int("checkout", 0);
  if( vid==0 ){
    fossil_panic("no checkout rename files in");
  }
  if( g.argc<4 ){
    usage("OLDNAME NEWNAME");
  }
  zDest = g.argv[g.argc-1];
  db_begin_transaction();
  file_tree_name(zDest, &dest, 1);
  db_multi_exec(
    "UPDATE vfile SET origname=pathname WHERE origname IS NULL;"
  );
  db_multi_exec(
    "CREATE TEMP TABLE mv(f TEXT UNIQUE ON CONFLICT IGNORE, t TEXT);"
  );
  if( file_isdir(zDest)!=1 ){
    Blob orig;
    if( g.argc!=4 ){
      usage("OLDNAME NEWNAME");
    }
    file_tree_name(g.argv[2], &orig, 1);
    db_multi_exec(
      "INSERT INTO mv VALUES(%B,%B)", &orig, &dest
    );
  }else{
    if( blob_eq(&dest, ".") ){
      blob_reset(&dest);
    }else{
      blob_append(&dest, "/", 1);
    }
    for(i=2; i<g.argc-1; i++){
      Blob orig;
      char *zOrig;
      int nOrig;
      file_tree_name(g.argv[i], &orig, 1);
      zOrig = blob_str(&orig);
      nOrig = blob_size(&orig);
      db_prepare(&q,
         "SELECT pathname FROM vfile"
         " WHERE vid=%d"
         "   AND (pathname='%q' OR (pathname>'%q/' AND pathname<'%q0'))"
         " ORDER BY 1",
         vid, zOrig, zOrig, zOrig
      );
      while( db_step(&q)==SQLITE_ROW ){
        const char *zPath = db_column_text(&q, 0);
        int nPath = db_column_bytes(&q, 0);
        const char *zTail;
        if( nPath==nOrig ){
          zTail = file_tail(zPath);
        }else{
          zTail = &zPath[nOrig+1];
        }
        db_multi_exec(
          "INSERT INTO mv VALUES('%s','%s%s')",
          zPath, blob_str(&dest), zTail
        );
      }
      db_finalize(&q);
    }
  }
  db_prepare(&q, "SELECT f, t FROM mv ORDER BY f");
  while( db_step(&q)==SQLITE_ROW ){
    const char *zFrom = db_column_text(&q, 0);
    const char *zTo = db_column_text(&q, 1);
    mv_one_file(vid, zFrom, zTo);
  }
  db_finalize(&q);
  db_end_transaction(0);
}
