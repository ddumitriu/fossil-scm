/*
** Copyright (c) 2009 D. Richard Hipp
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
** This file contains code to implement a very simple search function
** against timeline comments, checkin content, wiki pages, and/or tickets.
**
** The search is full-text like in that it is looking for words and ignores
** punctuation and capitalization.  But it is more akin to "grep" in that
** it scans the entire corpus for the search, and it does not support the
** full functionality of FTS4.
*/
#include "config.h"
#include "search.h"
#include <assert.h>

#if INTERFACE

/* Maximum number of search terms */
#define SEARCH_MAX_TERM   8

/*
** A compiled search pattern
*/
struct Search {
  int nTerm;            /* Number of search terms */
  struct srchTerm {     /* For each search term */
    char *z;               /* Text */
    int n;                 /* length */
  } a[SEARCH_MAX_TERM];
  /* Snippet controls */
  char *zPattern;       /* The search pattern */
  char *zMarkBegin;     /* Start of a match */   
  char *zMarkEnd;       /* End of a match */
  char *zMarkGap;       /* A gap between two matches */
  unsigned fSrchFlg;    /* Flags */
};

#define SRCHFLG_HTML    0x01   /* Escape snippet text for HTML */
#define SRCHFLG_SCORE   0x02   /* Prepend the score to each snippet */
#define SRCHFLG_STATIC  0x04   /* The static gSearch object */

#endif

/*
** There is a single global Search object:
*/
static Search gSearch;


/*
** Theses characters constitute a word boundary
*/
static const char isBoundary[] = {
  1, 1, 1, 1, 1, 1, 1, 1,     1, 1, 1, 1, 1, 1, 1, 1,
  1, 1, 1, 1, 1, 1, 1, 1,     1, 1, 1, 1, 1, 1, 1, 1,
  1, 1, 1, 1, 1, 1, 1, 1,     1, 1, 1, 1, 1, 1, 1, 1,
  0, 0, 0, 0, 0, 0, 0, 0,     0, 0, 1, 1, 1, 1, 1, 1,
  1, 0, 0, 0, 0, 0, 0, 0,     0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0,     0, 0, 0, 1, 1, 1, 1, 0,
  1, 0, 0, 0, 0, 0, 0, 0,     0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0,     0, 0, 0, 1, 1, 1, 1, 1,
  0, 0, 0, 0, 0, 0, 0, 0,     0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0,     0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0,     0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0,     0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0,     0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0,     0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0,     0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0,     0, 0, 0, 0, 0, 0, 0, 0,
};
#define ISALNUM(x)  (!isBoundary[(x)&0xff])


/*
** Destroy a search context.
*/
void search_end(Search *p){
  if( p ){
    fossil_free(p->zPattern);
    fossil_free(p->zMarkBegin);
    fossil_free(p->zMarkEnd);
    fossil_free(p->zMarkGap);
    memset(p, 0, sizeof(*p));
    if( p!=&gSearch ) fossil_free(p);
  }
}

/*
** Compile a search pattern
*/
Search *search_init(
  const char *zPattern,       /* The search pattern */
  const char *zMarkBegin,     /* Start of a match */   
  const char *zMarkEnd,       /* End of a match */
  const char *zMarkGap,       /* A gap between two matches */
  unsigned fSrchFlg           /* Flags */
){
  Search *p;
  char *z;
  int i;

  if( fSrchFlg & SRCHFLG_STATIC ){
    p = &gSearch;
    search_end(p);
  }else{
    p = fossil_malloc(sizeof(*p));
    memset(p, 0, sizeof(*p));
  }
  p->zPattern = z = mprintf("%s", zPattern);
  p->zMarkBegin = mprintf("%s", zMarkBegin);
  p->zMarkEnd = mprintf("%s", zMarkEnd);
  p->zMarkGap = mprintf("%s", zMarkGap);
  p->fSrchFlg = fSrchFlg;
  while( *z && p->nTerm<SEARCH_MAX_TERM ){
    while( *z && !ISALNUM(*z) ){ z++; }
    if( *z==0 ) break;
    p->a[p->nTerm].z = z;
    for(i=1; ISALNUM(z[i]); i++){}
    p->a[p->nTerm].n = i;
    z += i;
    p->nTerm++;
  }
  return p;
}


/*
** Append n bytes of text to snippet zTxt.  Encode the text appropriately.
*/
static void snippet_text_append(
  Search *p,             /* The search context */
  Blob *pSnip,           /* Append to this snippet */
  const char *zTxt,      /* Text to append */
  int n                  /* How many bytes to append */
){
  if( n>0 ){
    if( p->fSrchFlg & SRCHFLG_HTML ){
      blob_appendf(pSnip, "%#h", n, zTxt);
    }else{
      blob_append(pSnip, zTxt, n);
    }
  }
}

/*
** Compare a search pattern against one or more input strings which
** collectively comprise a document.  Return a match score.  Optionally
** also return a "snippet".
**
** Scoring:
**   *  All terms must match at least once or the score is zero
**   *  One point for each matching term
**   *  Extra points if consecutive words of the pattern are consecutive
**      in the document
*/
static int search_score(
  Search *p,              /* Search pattern and flags */
  int nDoc,               /* Number of strings in this document */
  const char **azDoc,     /* Text of each string */
  Blob *pSnip             /* If not NULL: Write a snippet here */
){
  int score;                         /* Final score */
  int i;                             /* Offset into current document */
  int ii;                            /* Loop counter */
  int j;                             /* Loop over search terms */
  int k;                             /* Loop over prior terms */
  int iWord = 0;                     /* Current word number */
  int iDoc;                          /* Current document number */
  int wantGap = 0;                   /* True if a zMarkGap is wanted */
  const char *zDoc;                  /* Current document text */
  const int CTX = 50;                /* Amount of snippet context */
  int anMatch[SEARCH_MAX_TERM];      /* Number of terms in best match */
  int aiBestDoc[SEARCH_MAX_TERM];    /* Document containing best match */
  int aiBestOfst[SEARCH_MAX_TERM];   /* Byte offset to start of best match */
  int aiLastDoc[SEARCH_MAX_TERM];    /* Document containing most recent match */
  int aiLastOfst[SEARCH_MAX_TERM];   /* Byte offset to the most recent match */
  int aiWordIdx[SEARCH_MAX_TERM];    /* Word index of most recent match */

  memset(anMatch, 0, sizeof(anMatch));
  memset(aiWordIdx, 0xff, sizeof(aiWordIdx));
  for(iDoc=0; iDoc<nDoc; iDoc++){
    zDoc = azDoc[iDoc];
    if( zDoc==0 ) continue;
    iWord++;
    for(i=0; zDoc[i]; i++){
      if( !ISALNUM(zDoc[i]) ) continue;
      iWord++;
      for(j=0; j<p->nTerm; j++){
        int n = p->a[j].n;
        if( sqlite3_strnicmp(p->a[j].z, &zDoc[i], n)==0
         && (!ISALNUM(zDoc[i+n]) || p->a[j].z[n]=='*')
        ){
          aiWordIdx[j] = iWord;
          aiLastDoc[j] = iDoc;
          aiLastOfst[j] = i;
          for(k=1; j-k>=0 && anMatch[j-k] && aiWordIdx[j-k]==iWord-k; k++){}
          for(ii=0; ii<k; ii++){
            if( anMatch[j-ii]<k ){
              anMatch[j-ii] = k;
              aiBestDoc[j-ii] = aiLastDoc[j-ii];
              aiBestOfst[j-ii] = aiLastOfst[j-ii];
            }
          }
          break;
        }
      }
      while( ISALNUM(zDoc[i]) ){ i++; }
    }
  }

  /* Finished search all documents.
  ** Every term must be seen or else the score is zero 
  */
  score = 1;
  for(j=0; j<p->nTerm; j++) score *= anMatch[j];
  if( score==0 || pSnip==0 ) return score;


  /* Prepare a snippet that describes the matching text.
  */
  blob_init(pSnip, 0, 0);
  if( p->fSrchFlg & SRCHFLG_SCORE ) blob_appendf(pSnip, "%08x", score);

  while(1){
    int iOfst;
    int iTail;
    int iBest;
    for(ii=0; ii<p->nTerm && anMatch[ii]==0; ii++){}
    if( ii>=p->nTerm ) break;  /* This is where the loop exits */
    iBest = ii;
    iDoc = aiBestDoc[ii];
    iOfst = aiBestOfst[ii];
    for(; ii<p->nTerm; ii++){
      if( anMatch[ii]==0 ) continue;
      if( aiBestDoc[ii]>iDoc ) continue;
      if( aiBestOfst[ii]>iOfst ) continue;
      iDoc = aiBestDoc[ii];
      iOfst = aiBestOfst[ii];
      iBest = ii;
    }
    iTail = iOfst + p->a[iBest].n;
    anMatch[iBest] = 0;
    for(ii=0; ii<p->nTerm; ii++){
      if( anMatch[ii]==0 ) continue;
      if( aiBestDoc[ii]!=iDoc ) continue;
      if( aiBestOfst[ii]<=iTail+CTX*2 ){
        if( iTail<aiBestOfst[ii]+p->a[ii].n ){
          iTail = aiBestOfst[ii]+p->a[ii].n;
        }
        anMatch[ii] = 0;
        ii = -1;
        continue;
      }
    }
    zDoc = azDoc[iDoc];
    iOfst -= CTX;
    if( iOfst<0 ) iOfst = 0;
    while( iOfst>0 && ISALNUM(zDoc[iOfst-1]) ) iOfst--;
    while( zDoc[iOfst] && !ISALNUM(zDoc[iOfst]) ) iOfst++;
    for(ii=0; ii<CTX && zDoc[iTail]; ii++, iTail++){}
    while( ISALNUM(zDoc[iTail]) ) iTail++;
    if( iOfst>0 || wantGap ) blob_append(pSnip, p->zMarkGap, -1);
    wantGap = zDoc[iTail]!=0;
    zDoc += iOfst;
    iTail -= iOfst;

    /* Add a snippet segment using characters iOfst..iOfst+iTail from zDoc */
    for(i=0; i<iTail; i++){
      if( !ISALNUM(zDoc[i]) ) continue;
      for(j=0; j<p->nTerm; j++){
        int n = p->a[j].n;
        if( sqlite3_strnicmp(p->a[j].z, &zDoc[i], n)==0
         && (!ISALNUM(zDoc[i+n]) || p->a[j].z[n]=='*')
        ){
          snippet_text_append(p, pSnip, zDoc, i);
          zDoc += i;
          iTail -= i;
          blob_append(pSnip, p->zMarkBegin, -1);
          if( p->a[j].z[n]=='*' ){
            while( ISALNUM(zDoc[n]) ) n++;
          }
          snippet_text_append(p, pSnip, zDoc, n);
          zDoc += n;
          iTail -= n;
          blob_append(pSnip, p->zMarkEnd, -1);
          i = -1;
          break;
        } /* end-if */
      } /* end for(j) */
      if( j<p->nTerm ){
        while( ISALNUM(zDoc[i]) && i<iTail ){ i++; }
      }
    } /* end for(i) */
    snippet_text_append(p, pSnip, zDoc, iTail);
  }
  if( wantGap ) blob_append(pSnip, p->zMarkGap, -1);
  return score;
}

/*
** COMMAND: test-snippet
**
** Usage: fossil test-snippet SEARCHSTRING FILE1 FILE2 ... 
*/
void test_snippet_cmd(void){
  Search *p;
  int i;
  Blob x;
  Blob snip;
  int score;
  char *zDoc;
  int flg = 0;
  char *zBegin = (char*)find_option("begin",0,1);
  char *zEnd = (char*)find_option("end",0,1);
  char *zGap = (char*)find_option("gap",0,1);
  if( find_option("html",0,0)!=0 ) flg |= SRCHFLG_HTML;
  if( find_option("score",0,0)!=0 ) flg |= SRCHFLG_SCORE;
  if( find_option("static",0,0)!=0 ) flg |= SRCHFLG_STATIC;
  verify_all_options();
  if( g.argc<4 ) usage("SEARCHSTRING FILE1...");
  if( zBegin==0 ) zBegin = "[[";
  if( zEnd==0 ) zEnd = "]]";
  if( zGap==0 ) zGap = " ... ";
  p = search_init(g.argv[2], zBegin, zEnd, zGap, flg);
  for(i=3; i<g.argc; i++){
    blob_read_from_file(&x, g.argv[i]);
    zDoc = blob_str(&x);
    score = search_score(p, 1, (const char**)&zDoc, &snip);
    fossil_print("%s: %d\n", g.argv[i], score);
    blob_reset(&x);
    if( score ){
      fossil_print("%.78c\n%s\n%.78c\n\n", '=', blob_str(&snip), '=');
      blob_reset(&snip);
    }
  }
}

/*
** An SQL function to initialize the global search pattern:
**
**     search_init(PATTERN,BEGIN,END,GAP,FLAGS)
**
** All arguments are optional.
*/
static void search_init_sqlfunc(
  sqlite3_context *context,
  int argc,
  sqlite3_value **argv
){
  const char *zPattern = 0;
  const char *zBegin = "<b>";
  const char *zEnd = "</b>";
  const char *zGap = " ... ";
  unsigned int flg = SRCHFLG_HTML;
  switch( argc ){
    default:
      flg = (unsigned int)sqlite3_value_int(argv[4]);
    case 4:
      zGap = (const char*)sqlite3_value_text(argv[3]);
    case 3:
      zEnd = (const char*)sqlite3_value_text(argv[2]);
    case 2:
      zBegin = (const char*)sqlite3_value_text(argv[1]);
    case 1:
      zPattern = (const char*)sqlite3_value_text(argv[0]);
  }
  if( zPattern && zPattern[0] ){
    search_init(zPattern, zBegin, zEnd, zGap, flg | SRCHFLG_STATIC);
  }else{
    search_end(&gSearch);
  }
}

/*
** This is an SQLite function that scores its input using
** the pattern from the previous call to search_init().
*/
static void search_score_sqlfunc(
  sqlite3_context *context,
  int argc,
  sqlite3_value **argv
){
  int isSnippet = sqlite3_user_data(context)!=0;
  const char **azDoc;
  int score;
  int i;
  Blob snip;

  if( gSearch.nTerm==0 ) return;
  azDoc = fossil_malloc( sizeof(const char*)*(argc+1) );
  for(i=0; i<argc; i++) azDoc[i] = (const char*)sqlite3_value_text(argv[i]);
  score = search_score(&gSearch, argc, azDoc, isSnippet ? &snip : 0);
  fossil_free((void *)azDoc);
  if( isSnippet ){
    if( score ){
      sqlite3_result_text(context, blob_materialize(&snip), -1, fossil_free);
    }
  }else{
    sqlite3_result_int(context, score);
  }
}

/*
** Register the "score()" SQL function to score its input text
** using the given Search object.  Once this function is registered,
** do not delete the Search object.
*/
void search_sql_setup(sqlite3 *db){
  sqlite3_create_function(db, "score", -1, SQLITE_UTF8, 0,
     search_score_sqlfunc, 0, 0);
  sqlite3_create_function(db, "snippet", -1, SQLITE_UTF8, &gSearch,
     search_score_sqlfunc, 0, 0);
  sqlite3_create_function(db, "search_init", -1, SQLITE_UTF8, 0,
     search_init_sqlfunc, 0, 0);
}

/*
** Testing the search function.
**
** COMMAND: search*
** %fossil search [-all|-a] [-limit|-n #] [-width|-W #] pattern...
**
** Search for timeline entries matching all words
** provided on the command line. Whole-word matches
** scope more highly than partial matches.
**
** Outputs, by default, some top-N fraction of the
** results. The -all option can be used to output
** all matches, regardless of their search score.
** The -limit option can be used to limit the number
** of entries returned.  The -width option can be
** used to set the output width used when printing
** matches.
*/
void search_cmd(void){
  Blob pattern;
  int i;
  Blob sql = empty_blob;
  Stmt q;
  int iBest;
  char fAll = NULL != find_option("all", "a", 0); /* If set, do not lop
                                                     off the end of the
                                                     results. */
  const char *zLimit = find_option("limit","n",1);
  const char *zWidth = find_option("width","W",1);
  int nLimit = zLimit ? atoi(zLimit) : -1000;   /* Max number of matching
                                                   lines/entries to list */
  int width;
  if( zWidth ){
    width = atoi(zWidth);
    if( (width!=0) && (width<=20) ){
      fossil_fatal("-W|--width value must be >20 or 0");
    }
  }else{
    width = -1;
  }

  db_must_be_within_tree();
  if( g.argc<2 ) return;
  blob_init(&pattern, g.argv[2], -1);
  for(i=3; i<g.argc; i++){
    blob_appendf(&pattern, " %s", g.argv[i]);
  }
  (void)search_init(blob_str(&pattern),"*","*","...",SRCHFLG_STATIC);
  blob_reset(&pattern);
  search_sql_setup(g.db);

  db_multi_exec(
     "CREATE TEMP TABLE srch(rid,uuid,date,comment,x);"
     "CREATE INDEX srch_idx1 ON srch(x);"
     "INSERT INTO srch(rid,uuid,date,comment,x)"
     "   SELECT blob.rid, uuid, datetime(event.mtime%s),"
     "          coalesce(ecomment,comment),"
     "          score(coalesce(ecomment,comment)) AS y"
     "     FROM event, blob"
     "    WHERE blob.rid=event.objid AND y>0;",
     timeline_utc()
  );
  iBest = db_int(0, "SELECT max(x) FROM srch");
  blob_append(&sql,
              "SELECT rid, uuid, date, comment, 0, 0 FROM srch "
              "WHERE 1 ", -1);
  if(!fAll){
    blob_append_sql(&sql,"AND x>%d ", iBest/3);
  }
  blob_append(&sql, "ORDER BY x DESC, date DESC ", -1);
  db_prepare(&q, "%s", blob_sql_text(&sql));
  blob_reset(&sql);
  print_timeline(&q, nLimit, width, 0);
  db_finalize(&q);
}

/*
** WEBPAGE: /search
**
** This is an EXPERIMENTAL page for doing search across a repository.
**
** The current implementation does a full text search over embedded
** documentation files on the tip of the "trunk" branch.  Only files
** ending in ".wiki", ".md", ".html", and ".txt" are searched.
**
** The entire text is scanned.  There is no full-text index.  This is
** experimental.  We may change to using a full-text index depending
** on performance.
**
** Other pending enhancements:
**    *   Search tickets
**    *   Search wiki
*/
void search_page(void){
  const char *zPattern = PD("s","");
  Stmt q;

  login_check_credentials();
  if( !g.perm.Read ){ login_needed(); return; }
  style_header("Search");
  @ <form method="GET" action="search"><center>
  @ <input type="text" name="s" size="40" value="%h(zPattern)">
  @ <input type="submit" value="Search">
  @ </center></form>
  while( fossil_isspace(zPattern[0]) ) zPattern++;
  if( zPattern[0] ){
    search_sql_setup(g.db);
    add_content_sql_commands(g.db);
    search_init(zPattern, "<b>", "</b>", " ... ",
            SRCHFLG_STATIC|SRCHFLG_HTML|SRCHFLG_SCORE);
    db_multi_exec(
      "CREATE VIRTUAL TABLE temp.foci USING files_of_checkin;"
      "CREATE TEMP TABLE x(fn TEXT,url TEXT,snip TEXT);"
      "INSERT INTO x(fn,url,snip)"
      "  SELECT filename, printf('%R/doc/trunk/%%s',filename),"
      "         snippet(content(uuid))"
      "    FROM foci"
      "   WHERE checkinID=symbolic_name_to_rid('trunk')"
      "     AND (filename GLOB '*.wiki' OR"
      "          filename GLOB '*.md' OR"
      "          filename GLOB '*.txt' OR"
      "          filename GLOB '*.html');"
    );
    db_prepare(&q, "SELECT url, substr(snip,8)"
                   "   FROM x WHERE snip IS NOT NULL"
                   " ORDER BY substr(snip,1,8) DESC, fn;");
    @ <ol>
    while( db_step(&q)==SQLITE_ROW ){
      const char *zUrl = db_column_text(&q, 0);
      const char *zSnippet = db_column_text(&q, 1);
      @ <li><p>%s(href("%s",zUrl))%h(zUrl)</a><br>%s(zSnippet)</li>
    }
    db_finalize(&q);
    @ </ol>
  }
  style_footer();
}
