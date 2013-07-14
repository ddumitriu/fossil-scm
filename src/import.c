/*
** Copyright (c) 2010 D. Richard Hipp
**
** This program is free software; you can redistribute it and/or
** modify it under the terms of the Simplified BSD License (also
** known as the "2-Clause License" or "FreeBSD License".)

** This program is distributed in the hope that it will be useful,
** but without any warranty; without even the implied warranty of
** merchantability or fitness for a particular purpose.
**
** Author contact information:
**   drh@sqlite.org
**
*******************************************************************************
**
** This file contains code used to import the content of a Git
** repository in the git-fast-import format as a new Fossil
** repository.
*/
#include "config.h"
#include "import.h"
#include <assert.h>

#if INTERFACE
/*
** A single file change record.
*/
struct ImportFile {
  char *zName;           /* Name of a file */
  char *zUuid;           /* UUID of the file */
  char *zPrior;          /* Prior name if the name was changed */
  char isExe;            /* True if executable */
  char isLink;           /* True if symlink */
  char hasChanged;       /* True when different than baseline */
};
#endif


/*
** State information about an on-going fast-import parse.
*/
static struct {
  void (*xFinish)(void);      /* Function to finish a prior record */
  int nData;                  /* Bytes of data */
  char *zBaseline;            /* Baseline manifest.  The B card. */
  char *zTag;                 /* Name of a tag */
  char *zBranch;              /* Name of a branch for a commit */
  char *zPrevBranch;          /* The branch of the previous check-in */
  char *aData;                /* Data content */
  char *zMark;                /* The current mark */
  char *zDate;                /* Date/time stamp */
  char *zUser;                /* User name */
  char *zComment;             /* Comment of a commit */
  char *zFrom;                /* from value as a UUID */
  char *zPrevCheckin;         /* Name of the previous check-in */
  char *zFromMark;            /* The mark of the "from" field */
  int nMerge;                 /* Number of merge values */
  int nMergeAlloc;            /* Number of slots in azMerge[] */
  char **azMerge;             /* Merge values */
  int nFile;                  /* Number of aFile values */
  int nFileAlloc;             /* Number of slots in aFile[] */
  int nFileEffective;         /* Number of aFile items with zUuid != 0 */
  int nChanged;               /* Number of aFile that differ from baseline */
  ImportFile *aFile;          /* Information about files in a commit */
  int fromLoaded;             /* True zFrom content loaded into aFile[] */
  int hasLinks;               /* True if git repository contains symlinks */
  int tagCommit;              /* True if the commit adds a tag */
  int tryDelta;               /* Attempt to generate delta manifests */
} gg;

/*
** Duplicate a string.
*/
char *fossil_strdup(const char *zOrig){
  char *z = 0;
  if( zOrig ){
    int n = strlen(zOrig);
    z = fossil_malloc( n+1 );
    memcpy(z, zOrig, n+1);
  }
  return z;
}

/*
** A no-op "xFinish" method
*/
static void finish_noop(void){}

/*
** Deallocate the state information.
**
** The azMerge[] and aFile[] arrays are zeroed by allocated space is
** retained unless the freeAll flag is set.
*/
static void import_reset(int freeAll){
  int i;
  gg.xFinish = 0;
  fossil_free(gg.zBaseline); gg.zBaseline = 0;
  fossil_free(gg.zTag); gg.zTag = 0;
  fossil_free(gg.zBranch); gg.zBranch = 0;
  fossil_free(gg.aData); gg.aData = 0;
  fossil_free(gg.zMark); gg.zMark = 0;
  fossil_free(gg.zDate); gg.zDate = 0;
  fossil_free(gg.zUser); gg.zUser = 0;
  fossil_free(gg.zComment); gg.zComment = 0;
  fossil_free(gg.zFrom); gg.zFrom = 0;
  fossil_free(gg.zFromMark); gg.zFromMark = 0;
  for(i=0; i<gg.nMerge; i++){
    fossil_free(gg.azMerge[i]); gg.azMerge[i] = 0;
  }
  gg.nMerge = 0;
  for(i=0; i<gg.nFile; i++){
    fossil_free(gg.aFile[i].zName);
    fossil_free(gg.aFile[i].zUuid);
    fossil_free(gg.aFile[i].zPrior);
    gg.aFile[i].zPrior = 0;
  }
  memset(gg.aFile, 0, gg.nFile*sizeof(gg.aFile[0]));
  gg.nChanged = 0;
  gg.nFileEffective = 0;
  gg.nFile = 0;
  if( freeAll ){
    fossil_free(gg.zPrevBranch);
    fossil_free(gg.zPrevCheckin);
    fossil_free(gg.azMerge);
    fossil_free(gg.aFile);
    memset(&gg, 0, sizeof(gg));
  }
  gg.xFinish = finish_noop;
}

/*
** Insert an artifact into the BLOB table if it isn't there already.
** If zMark is not zero, create a cross-reference from that mark back
** to the newly inserted artifact.
**
** If saveUuid is true, then pContent is a commit record.  Record its
** UUID in gg.zPrevCheckin.
*/
static int fast_insert_content(Blob *pContent, const char *zMark, int saveUuid){
  Blob hash;
  Blob cmpr;
  int rid;

  sha1sum_blob(pContent, &hash);
  rid = db_int(0, "SELECT rid FROM blob WHERE uuid=%B", &hash);
  if( rid==0 ){
    static Stmt ins;
    db_static_prepare(&ins,
        "INSERT INTO blob(uuid, size, content) VALUES(:uuid, :size, :content)"
    );
    db_bind_text(&ins, ":uuid", blob_str(&hash));
    db_bind_int(&ins, ":size", gg.nData);
    blob_compress(pContent, &cmpr);
    db_bind_blob(&ins, ":content", &cmpr);
    db_step(&ins);
    db_reset(&ins);
    blob_reset(&cmpr);
    rid = db_last_insert_rowid();
  }
  if( zMark ){
    db_multi_exec(
        "INSERT OR IGNORE INTO xmark(tname, trid, tuuid)"
        "VALUES(%Q,%d,%B)",
        zMark, rid, &hash
    );
    db_multi_exec(
        "INSERT OR IGNORE INTO xmark(tname, trid, tuuid)"
        "VALUES(%B,%d,%B)",
        &hash, rid, &hash
    );
  }
  if( saveUuid ){
    fossil_free(gg.zPrevCheckin);
    gg.zPrevCheckin = fossil_strdup(blob_str(&hash));
  }
  blob_reset(&hash);
  return rid;
}

/*
** Use data accumulated in gg from a "blob" record to add a new file
** to the BLOB table.
*/
static void finish_blob(void){
  Blob content;
  blob_init(&content, gg.aData, gg.nData);
  fast_insert_content(&content, gg.zMark, 0);
  blob_reset(&content);
  import_reset(0);
}

/*
** Use data accumulated in gg from a "tag" record to add a new
** control artifact to the BLOB table.
*/
static void finish_tag(void){
  Blob record, cksum;
  if( gg.zDate && gg.zTag && gg.zFrom && gg.zUser ){
    blob_zero(&record);
    blob_appendf(&record, "D %s\n", gg.zDate);
    blob_appendf(&record, "T +%F %s\n", gg.zTag, gg.zFrom);
    blob_appendf(&record, "U %F\n", gg.zUser);
    md5sum_blob(&record, &cksum);
    blob_appendf(&record, "Z %b\n", &cksum);
    fast_insert_content(&record, 0, 0);
    blob_reset(&record);
    blob_reset(&cksum);
  }
  import_reset(0);
}

/*
** Compare two ImportFile objects for sorting
*/
static int mfile_cmp(const void *pLeft, const void *pRight){
  const ImportFile *pA = (const ImportFile*)pLeft;
  const ImportFile *pB = (const ImportFile*)pRight;
  return fossil_strcmp(pA->zName, pB->zName);
}

/*
** Compare two strings for sorting.
*/
static int string_cmp(const void *pLeft, const void *pRight){
  const char *zLeft = *(char const **)pLeft;
  const char *zRight = *(char const **)pRight;
  return fossil_strcmp(zLeft, zRight);
}

/* Forward reference */
static void import_prior_files(void);

/*
** Use data accumulated in gg from a "commit" record to add a new
** manifest artifact to the BLOB table.
*/
static void finish_commit(void){
  int i;
  int delta;
  char *zFromBranch;
  char *aTCard[4];                /* Array of T cards for manifest */
  int nTCard = 0;                 /* Entries used in aTCard[] */
  Blob record, cksum;

  import_prior_files();
  qsort(gg.aFile, gg.nFile, sizeof(gg.aFile[0]), mfile_cmp);
  /*
  ** This is the same mechanism used by the commit command to decide whether to
  ** generate a delta manifest or not.  It is evaluated and saved for later
  ** uses.
  */
  delta = gg.tryDelta && (gg.zBaseline!=0 || gg.zFrom!=0) &&
                         (gg.nChanged*gg.nChanged)<(gg.nFileEffective*3-9);
  blob_zero(&record);
  if( delta ){
    blob_appendf(&record, "B %s\n", gg.zBaseline!=0 ? gg.zBaseline : gg.zFrom);
  }
  blob_appendf(&record, "C %F\n", gg.zComment);
  blob_appendf(&record, "D %s\n", gg.zDate);
  for(i=0; i<gg.nFile; i++){
    const struct ImportFile *pFile = &gg.aFile[i];
    if( (!delta && pFile->zUuid==0) || (delta && !pFile->hasChanged) ) continue;
    blob_appendf(&record, "F %F", pFile->zName);
    if( pFile->zUuid!=0 ) {
      blob_appendf(&record, " %s", pFile->zUuid);
      if( pFile->isExe ){
        blob_append(&record, " x", 2);
      }else if( pFile->isLink ){
        blob_append(&record, " l", 2);
      }
      if( pFile->zPrior!=0 ){
        if( !pFile->isExe && !pFile->isLink ){
          blob_append(&record, " w", 2);
        }
        blob_appendf(&record, " %F", pFile->zPrior);
      }
    }
    blob_append(&record, "\n", 1);
  }
  if( gg.zFrom ){
    blob_appendf(&record, "P %s", gg.zFrom);
    for(i=0; i<gg.nMerge; i++){
      blob_appendf(&record, " %s", gg.azMerge[i]);
    }
    blob_append(&record, "\n", 1);
    zFromBranch = db_text(0, "SELECT brnm FROM xbranch WHERE tname=%Q",
                              gg.zFromMark);
  }else{
    zFromBranch = 0;
  }

  /* Add the required "T" cards to the manifest. Make sure they are added
  ** in sorted order and without any duplicates. Otherwise, fossil will not
  ** recognize the document as a valid manifest. */
  if( !gg.tagCommit && fossil_strcmp(zFromBranch, gg.zBranch)!=0 ){
    aTCard[nTCard++] = mprintf("T *branch * %F\n", gg.zBranch);
    aTCard[nTCard++] = mprintf("T *sym-%F *\n", gg.zBranch);
    if( zFromBranch ){
      aTCard[nTCard++] = mprintf("T -sym-%F *\n", zFromBranch);
    }
  }
  if( gg.zFrom==0 ){
    aTCard[nTCard++] = mprintf("T *sym-trunk *\n");
  }
  qsort(aTCard, nTCard, sizeof(char *), string_cmp);
  for(i=0; i<nTCard; i++){
    if( i==0 || fossil_strcmp(aTCard[i-1], aTCard[i]) ){
      blob_appendf(&record, "%s", aTCard[i]);
    }
  }
  for(i=0; i<nTCard; i++) free(aTCard[i]);

  free(zFromBranch);
  db_multi_exec("INSERT INTO xbranch(tname, brnm) VALUES(%Q,%Q)",
                gg.zMark, gg.zBranch);
  blob_appendf(&record, "U %F\n", gg.zUser);
  md5sum_blob(&record, &cksum);
  blob_appendf(&record, "Z %b\n", &cksum);
  fast_insert_content(&record, gg.zMark, 1);
  blob_reset(&record);
  blob_reset(&cksum);

  /* The "git fast-export" command might output multiple "commit" lines
  ** that reference a tag using "refs/tags/TAGNAME".  The tag should only
  ** be applied to the last commit that is output.  The problem is we do not
  ** know at this time if the current commit is the last one to hold this
  ** tag or not.  So make an entry in the XTAG table to record this tag
  ** but overwrite that entry if a later instance of the same tag appears.
  **
  ** This behavior seems like a bug in git-fast-export, but it is easier
  ** to work around the problem than to fix git-fast-export.
  */
  if( gg.tagCommit && gg.zDate && gg.zUser && gg.zFrom ){
    blob_appendf(&record, "D %s\n", gg.zDate);
    blob_appendf(&record, "T +sym-%F %s\n", gg.zBranch, gg.zPrevCheckin);
    blob_appendf(&record, "U %F\n", gg.zUser);
    md5sum_blob(&record, &cksum);
    blob_appendf(&record, "Z %b\n", &cksum);
    db_multi_exec(
       "INSERT OR REPLACE INTO xtag(tname, tcontent)"
       " VALUES(%Q,%Q)", gg.zBranch, blob_str(&record)
    );
    blob_reset(&record);
    blob_reset(&cksum);
  }

  fossil_free(gg.zPrevBranch);
  gg.zPrevBranch = gg.zBranch;
  gg.zBranch = 0;
  import_reset(0);
}

/*
** Turn the first \n in the input string into a \000
*/
static void trim_newline(char *z){
  while( z[0] && z[0]!='\n' ){ z++; }
  z[0] = 0;
}

/*
** Get a token from a line of text.  Return a pointer to the first
** character of the token and zero-terminate the token.  Make
** *pzIn point to the first character past the end of the zero
** terminator, or at the zero-terminator at EOL.
*/
static char *next_token(char **pzIn){
  char *z = *pzIn;
  int i, j;
  if( z[0]==0 ) return z;
  if( z[0]=='"' ){
    /* Quoted path name */
    z++;
    for(i=0, j=0; z[i] && z[i]!='"' && z[i]!='\n'; i++, j++){
      if( z[i]=='\\' && z[i+1] ){
        char v, c = z[++i];
        switch( c ){
          case 0:
          case '"':  c = '"';  break;
          case '\\': c = '\\'; break;
          case 'a':  c = '\a'; break;
          case 'b':  c = '\b'; break;
          case 'f':  c = '\f'; break;
          case 'n':  c = '\n'; break;
          case 'r':  c = '\r'; break;
          case 't':  c = '\t'; break;
          case 'v':  c = '\v'; break;
          case '0': case '1': case '2': case '3':
            v = (c - '0') << 6;
            c = z[++i];
            if( c < '0' || c > '7' )
              fossil_fatal("Invalid octal digit '%c' in sequence", c);
            v |= (c - '0') << 3;
            c = z[++i];
            if( c < '0' || c > '7' )
              fossil_fatal("Invalid octal digit '%c' in sequence", c);
            v |= (c - '0');
            c = v;
            break;
          default:
            fossil_fatal("Unrecognized escape sequence \"\\%c\"", c);
        }
        z[j] = c;
      }
    }
    if( z[i]=='"' ) z[i++] = 0;
  }else{
    /* Unquoted path name or generic token */
    for(i=0; z[i] && z[i]!=' ' && z[i]!='\n'; i++){}
  }
  if( z[i] ){
    z[i] = 0;
    *pzIn = &z[i+1];
  }else{
    *pzIn = &z[i];
  }
  return z;
}

/*
** Convert a "mark" or "committish" into the UUID.
*/
static char *resolve_committish(const char *zCommittish){
  char *zRes;

  zRes = db_text(0, "SELECT tuuid FROM xmark WHERE tname=%Q", zCommittish);
  return zRes;
}

/*
** Create a new entry in the gg.aFile[] array
*/
static ImportFile *import_add_file(void){
  ImportFile *pFile;
  if( gg.nFile>=gg.nFileAlloc ){
    gg.nFileAlloc = gg.nFileAlloc*2 + 100;
    gg.aFile = fossil_realloc(gg.aFile, gg.nFileAlloc*sizeof(gg.aFile[0]));
  }
  pFile = &gg.aFile[gg.nFile++];
  memset(pFile, 0, sizeof(*pFile));
  return pFile;
}


/*
** Load all file information out of the gg.zFrom check-in
*/
static void import_prior_files(void){
  Manifest *p;
  int rid;
  ManifestFile *pOld;
  ImportFile *pNew;
  if( gg.fromLoaded ) return;
  gg.fromLoaded = 1;
  if( gg.zFrom==0 && gg.zPrevCheckin!=0
   && fossil_strcmp(gg.zBranch, gg.zPrevBranch)==0
  ){
     gg.zFrom = gg.zPrevCheckin;
     gg.zPrevCheckin = 0;
  }
  if( gg.zFrom==0 ) return;
  rid = fast_uuid_to_rid(gg.zFrom);
  if( rid==0 ) return;
  p = manifest_get(rid, CFTYPE_MANIFEST);
  if( p==0 ) return;
  manifest_file_rewind(p);
  if( gg.tryDelta && p->pBaseline ){
    /*
    ** The manifest_file_next() iterator skips deletion "F" cards in delta
    ** manifests.  But, in order to build more delta manifests, this information
    ** is necessary because it propagates.  Therefore, in this case, the
    ** manifest has to be traversed "manually".
    */
    Manifest *pB = p->pBaseline;
    gg.zBaseline = fossil_strdup(p->zBaseline);
    while( p->iFile<p->nFile || pB->iFile<pB->nFile ){
      pNew = import_add_file();
      if( p->iFile>=p->nFile ){
        /* No more "F" cards in delta manifest, finish the baseline */
        pOld = &pB->aFile[pB->iFile++];
      }else if( pB->iFile>=pB->nFile ){
        /* No more "F" cards in baseline, finish the delta manifest */
        pOld = &p->aFile[p->iFile++];
        pNew->hasChanged = 1;
      }else{
        int cmp = fossil_strcmp(pB->aFile[pB->iFile].zName,
                                p->aFile[p->iFile].zName);
        if( cmp < 0 ){
          pOld = &pB->aFile[pB->iFile++];
        }else if( cmp >= 0 ){
          pOld = &p->aFile[p->iFile++];
          pNew->hasChanged = 1;
          if( cmp==0 ) pB->iFile++;
        }
      }
      pNew->zName = fossil_strdup(pOld->zName);
      pNew->isExe = pOld->zPerm && strstr(pOld->zPerm, "x")!=0;
      pNew->isLink = pOld->zPerm && strstr(pOld->zPerm, "l")!=0;
      pNew->zUuid = fossil_strdup(pOld->zUuid);
      gg.nChanged += pNew->hasChanged;
      if( pNew->zUuid!=0 ) gg.nFileEffective++;
    }
  }else{
    while( (pOld = manifest_file_next(p, 0))!=0 ){
      pNew = import_add_file();
      pNew->zName = fossil_strdup(pOld->zName);
      pNew->isExe = pOld->zPerm && strstr(pOld->zPerm, "x")!=0;
      pNew->isLink = pOld->zPerm && strstr(pOld->zPerm, "l")!=0;
      pNew->zUuid = fossil_strdup(pOld->zUuid);
      if( pNew->zUuid!=0 ) gg.nFileEffective++;
    }
  }
  manifest_destroy(p);
}

/*
** Locate a file in the gg.aFile[] array by its name.  Begin the search
** with the *pI-th file.  Update *pI to be one past the file found.
** Do not search past the mx-th file.
*/
static ImportFile *import_find_file(const char *zName, int *pI, int mx){
  int i = *pI;
  int nName = strlen(zName);
  while( i<mx ){
    const char *z = gg.aFile[i].zName;
    if( memcmp(zName, z, nName)==0 && (z[nName]==0 || z[nName]=='/') ){
      *pI = i+1;
      return &gg.aFile[i];
    }
    i++;
  }
  return 0;
}


/*
** Read the git-fast-import format from pIn and insert the corresponding
** content into the database.
*/
static void git_fast_import(FILE *pIn){
  ImportFile *pFile, *pNew;
  int i;
  char *z;
  char *zUuid;
  char *zName;
  char *zPerm;
  char *zFrom;
  char *zTo;
  char zLine[1000];

  gg.xFinish = finish_noop;
  while( fgets(zLine, sizeof(zLine), pIn) ){
    if( zLine[0]=='\n' || zLine[0]=='#' ) continue;
    if( memcmp(zLine, "blob", 4)==0 ){
      gg.xFinish();
      gg.xFinish = finish_blob;
    }else
    if( memcmp(zLine, "commit ", 7)==0 ){
      gg.xFinish();
      gg.xFinish = finish_commit;
      trim_newline(&zLine[7]);
      z = &zLine[7];

      /* The argument to the "commit" line might match either of these
      ** patterns:
      **
      **   (A)  refs/heads/BRANCHNAME
      **   (B)  refs/tags/TAGNAME
      **
      ** If pattern A is used, then the branchname used is as shown.
      ** Except, the "master" branch which is the default branch name in
      ** Git is changed to "trunk" which is the default name in Fossil.
      ** If the pattern is B, then the new commit should be on the same
      ** branch as its parent.  And, we might need to add the TAGNAME
      ** tag to the new commit.  However, if there are multiple instances
      ** of pattern B with the same TAGNAME, then only put the tag on the
      ** last commit that holds that tag.
      **
      ** None of the above is explained in the git-fast-export
      ** documentation.  We had to figure it out via trial and error.
      */
      for(i=strlen(z)-1; i>=0 && z[i]!='/'; i--){}
      gg.tagCommit = memcmp(&z[i-4], "tags", 4)==0;  /* True for pattern B */
      if( z[i+1]!=0 ) z += i+1;
      if( fossil_strcmp(z, "master")==0 ) z = "trunk";
      gg.zBranch = fossil_strdup(z);
      gg.fromLoaded = 0;
    }else
    if( memcmp(zLine, "tag ", 4)==0 ){
      gg.xFinish();
      gg.xFinish = finish_tag;
      trim_newline(&zLine[4]);
      gg.zTag = fossil_strdup(&zLine[4]);
    }else
    if( memcmp(zLine, "reset ", 4)==0 ){
      gg.xFinish();
    }else
    if( memcmp(zLine, "checkpoint", 10)==0 ){
      gg.xFinish();
    }else
    if( memcmp(zLine, "feature", 7)==0 ){
      gg.xFinish();
    }else
    if( memcmp(zLine, "option", 6)==0 ){
      gg.xFinish();
    }else
    if( memcmp(zLine, "progress ", 9)==0 ){
      gg.xFinish();
      trim_newline(&zLine[9]);
      fossil_print("%s\n", &zLine[9]);
      fflush(stdout);
    }else
    if( memcmp(zLine, "data ", 5)==0 ){
      fossil_free(gg.aData); gg.aData = 0;
      gg.nData = atoi(&zLine[5]);
      if( gg.nData ){
        int got;
        gg.aData = fossil_malloc( gg.nData+1 );
        got = fread(gg.aData, 1, gg.nData, pIn);
        if( got!=gg.nData ){
          fossil_fatal("short read: got %d of %d bytes", got, gg.nData);
        }
        gg.aData[got] = 0;
        if( gg.zComment==0 && gg.xFinish==finish_commit ){
          gg.zComment = gg.aData;
          gg.aData = 0;
          gg.nData = 0;
        }
      }
    }else
    if( memcmp(zLine, "author ", 7)==0 ){
      /* No-op */
    }else
    if( memcmp(zLine, "mark ", 5)==0 ){
      trim_newline(&zLine[5]);
      fossil_free(gg.zMark);
      gg.zMark = fossil_strdup(&zLine[5]);
    }else
    if( memcmp(zLine, "tagger ", 7)==0 || memcmp(zLine, "committer ",10)==0 ){
      sqlite3_int64 secSince1970;
      for(i=0; zLine[i] && zLine[i]!='<'; i++){}
      if( zLine[i]==0 ) goto malformed_line;
      z = &zLine[i+1];
      for(i=i+1; zLine[i] && zLine[i]!='>'; i++){}
      if( zLine[i]==0 ) goto malformed_line;
      zLine[i] = 0;
      fossil_free(gg.zUser);
      gg.zUser = fossil_strdup(z);
      secSince1970 = 0;
      for(i=i+2; fossil_isdigit(zLine[i]); i++){
        secSince1970 = secSince1970*10 + zLine[i] - '0';
      }
      fossil_free(gg.zDate);
      gg.zDate = db_text(0, "SELECT datetime(%lld, 'unixepoch')", secSince1970);
      gg.zDate[10] = 'T';
    }else
    if( memcmp(zLine, "from ", 5)==0 ){
      trim_newline(&zLine[5]);
      fossil_free(gg.zFromMark);
      gg.zFromMark = fossil_strdup(&zLine[5]);
      fossil_free(gg.zFrom);
      gg.zFrom = resolve_committish(&zLine[5]);
    }else
    if( memcmp(zLine, "merge ", 6)==0 ){
      trim_newline(&zLine[6]);
      if( gg.nMerge>=gg.nMergeAlloc ){
        gg.nMergeAlloc = gg.nMergeAlloc*2 + 10;
        gg.azMerge = fossil_realloc(gg.azMerge, gg.nMergeAlloc*sizeof(char*));
      }
      gg.azMerge[gg.nMerge] = resolve_committish(&zLine[6]);
      if( gg.azMerge[gg.nMerge] ) gg.nMerge++;
    }else
    if( memcmp(zLine, "M ", 2)==0 ){
      import_prior_files();
      z = &zLine[2];
      zPerm = next_token(&z);
      zUuid = next_token(&z);
      zName = next_token(&z);
      i = 0;
      pFile = import_find_file(zName, &i, gg.nFile);
      if( pFile==0 ){
        pFile = import_add_file();
        pFile->zName = fossil_strdup(zName);
        gg.nFileEffective++;
      }
      pFile->isExe = (fossil_strcmp(zPerm, "100755")==0);
      pFile->isLink = (fossil_strcmp(zPerm, "120000")==0);
      fossil_free(pFile->zUuid);
      pFile->zUuid = resolve_committish(zUuid);
      /*
      ** This trick avoids counting multiple changes on the same filename as
      ** changes to multiple filenames.  When an entry in gg.aFile is already
      ** different from its baseline, it is not counted again.
      */
      gg.nChanged += 1 - pFile->hasChanged;
      pFile->hasChanged = 1;
    }else
    if( memcmp(zLine, "D ", 2)==0 ){
      import_prior_files();
      z = &zLine[2];
      zName = next_token(&z);
      i = 0;
      pFile = import_find_file(zName, &i, gg.nFile);
      if( pFile!=0 ){
        /* Do not remove the item from gg.aFile, just mark as deleted */
        fossil_free(pFile->zUuid);
        pFile->zUuid = 0;
        gg.nChanged += 1 - pFile->hasChanged;
        pFile->hasChanged = 1;
        gg.nFileEffective--;
      }
    }else
    if( memcmp(zLine, "C ", 2)==0 ){
      import_prior_files();
      z = &zLine[2];
      zFrom = next_token(&z);
      zTo = next_token(&z);
      i = 0;
      pFile = import_find_file(zFrom, &i, gg.nFile);
      if( pFile!=0 ){
        int j = 0;
        pNew = import_find_file(zTo, &j, gg.nFile);
        if( pNew==0 ){
          pNew = import_add_file();
          pFile = &gg.aFile[i-1];  /* gg.aFile may have been realloc()-ed */
          pNew->zName = fossil_strdup(zTo);
          gg.nFileEffective++;
        }else{
          fossil_free(pNew->zUuid);
        }
        pNew->isExe = pFile->isExe;
        pNew->isLink = pFile->isLink;
        pNew->zUuid = fossil_strdup(pFile->zUuid);
        gg.nChanged += 1 - pNew->hasChanged;
        pNew->hasChanged = 1;
      }
    }else
    if( memcmp(zLine, "R ", 2)==0 ){
      import_prior_files();
      z = &zLine[2];
      zFrom = next_token(&z);
      zTo = next_token(&z);
      i = 0;
      pFile = import_find_file(zFrom, &i, gg.nFile);
      if( pFile!=0 ){
        /*
        ** File renames in delta manifests require two "F" cards: one to
        ** delete the old file (without UUID) and another with the rename
        ** (with prior name equals to the name in the other card).
        **
        ** This forces us to also lookup by the destination name, as it may
        ** already exist in the form of a delta manifest deletion "F" card.
        */
        int j = 0;
        pNew = import_find_file(zTo, &j, gg.nFile);
        if( pNew==0 ){
          pNew = import_add_file();
          pFile = &gg.aFile[i-1];
          pNew->zName = fossil_strdup(zTo);
        }else{
          fossil_free(pNew->zUuid);  /* Just in case */
        }
        pNew->isExe = pFile->isExe;
        pNew->isLink = pFile->isLink;
        pNew->zPrior = fossil_strdup(zFrom);
        pNew->zUuid = pFile->zUuid;
        pFile->zUuid = 0;
        gg.nChanged += 2 - (pNew->hasChanged + pFile->hasChanged);
        pNew->hasChanged = 1;
        pFile->hasChanged = 1;
      }
    }else
    if( memcmp(zLine, "deleteall", 9)==0 ){
      gg.fromLoaded = 1;
    }else
    if( memcmp(zLine, "N ", 2)==0 ){
      /* No-op */
    }else

    {
      goto malformed_line;
    }
  }
  gg.xFinish();
  if( gg.hasLinks ){
    db_set_int("allow-symlinks", 1, 0);
  }
  import_reset(1);
  return;

malformed_line:
  trim_newline(zLine);
  fossil_fatal("bad fast-import line: [%s]", zLine);
  return;
}

/*
** COMMAND: import
**
** Usage: %fossil import --git ?OPTIONS? NEW-REPOSITORY ?FILE?
**
** Read text generated by the git-fast-export command and use it to
** construct a new Fossil repository named by the NEW-REPOSITORY
** argument.  If given, the git-fast-export text is read from the FILE argument,
** otherwise text is read from standard input.
**
** The git-fast-export file format is currently the only VCS interchange
** format that is understood, though other interchange formats may be added
** in the future.
**
** The --incremental option allows an existing repository to be extended
** with new content.  Otherwise, if a file with the same name as NEW-REPOSITORY
** is found, the command fails unless the --force option is used.
**
** When the --delta option is used, delta manifests will be generated when they
** are smaller than the equivalent baseline manifest.  Please beware that delta
** manifests are not understood by older versions of Fossil.  Therefore, only
** use this option when it can be assured that only newer clients will pull or
** read from it.
**
** Options:
**   -d|--delta        enable delta manifest generation
**   -f|--force        remove existing file
**   -i|--incremental  allow importing into an existing repository
**
** See also: export
*/
void git_import_cmd(void){
  char *zPassword;
  FILE *pIn;
  Stmt q;
  int forceFlag = find_option("force", "f", 0)!=0;
  int incrFlag = find_option("incremental", "i", 0)!=0;

  find_option("git",0,0);  /* Skip the --git option for now */
  gg.tryDelta = find_option("delta", "d", 0)!=0;
  verify_all_options();
  if( g.argc!=3  && g.argc!=4 ){
    usage("REPOSITORY-NAME");
  }
  if( g.argc==4 ){
    pIn = fossil_fopen(g.argv[3], "rb");
  }else{
    pIn = stdin;
    fossil_binary_mode(pIn);
  }
  if( !incrFlag ){
    if( forceFlag ) file_delete(g.argv[2]);
    db_create_repository(g.argv[2]);
  }
  db_open_repository(g.argv[2]);
  db_open_config(0);

  /* The following temp-tables are used to hold information needed for
  ** the import.
  **
  ** The XMARK table provides a mapping from fast-import "marks" and symbols
  ** into artifact ids (UUIDs - the 40-byte hex SHA1 hash of artifacts).
  ** Given any valid fast-import symbol, the corresponding fossil rid and
  ** uuid can found by searching against the xmark.tname field.
  **
  ** The XBRANCH table maps commit marks and symbols into the branch those
  ** commits belong to.  If xbranch.tname is a fast-import symbol for a
  ** checkin then xbranch.brnm is the branch that checkin is part of.
  **
  ** The XTAG table records information about tags that need to be applied
  ** to various branches after the import finishes.  The xtag.tcontent field
  ** contains the text of an artifact that will add a tag to a check-in.
  ** The git-fast-export file format might specify the same tag multiple
  ** times but only the last tag should be used.  And we do not know which
  ** occurrence of the tag is the last until the import finishes.
  */
  db_multi_exec(
     "CREATE TEMP TABLE xmark(tname TEXT UNIQUE, trid INT, tuuid TEXT);"
     "CREATE TEMP TABLE xbranch(tname TEXT UNIQUE, brnm TEXT);"
     "CREATE TEMP TABLE xtag(tname TEXT UNIQUE, tcontent TEXT);"
  );


  db_begin_transaction();
  if( !incrFlag ) db_initial_setup(0, 0, 0, 1);
  git_fast_import(pIn);
  db_prepare(&q, "SELECT tcontent FROM xtag");
  while( db_step(&q)==SQLITE_ROW ){
    Blob record;
    db_ephemeral_blob(&q, 0, &record);
    fast_insert_content(&record, 0, 0);
    import_reset(0);
  }
  db_finalize(&q);
  db_end_transaction(0);
  db_begin_transaction();
  fossil_print("Rebuilding repository meta-data...\n");
  rebuild_db(0, 1, !incrFlag);
  verify_cancel();
  db_end_transaction(0);
  fossil_print("Vacuuming..."); fflush(stdout);
  db_multi_exec("VACUUM");
  fossil_print(" ok\n");
  if( !incrFlag ){
    fossil_print("project-id: %s\n", db_get("project-code", 0));
    fossil_print("server-id:  %s\n", db_get("server-code", 0));
    zPassword = db_text(0, "SELECT pw FROM user WHERE login=%Q", g.zLogin);
    fossil_print("admin-user: %s (password is \"%s\")\n", g.zLogin, zPassword);
  }
}
