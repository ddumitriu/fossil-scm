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
**   drh@hwaci.com
**   http://www.hwaci.com/drh/
**
*******************************************************************************
**
** This module contains the code that initializes the "sqlite4" command-line
** shell against the repository database.  The command-line shell itself
** is a copy of the "shell.c" code from SQLite.  This file contains logic
** to initialize the code in shell.c.
*/
#include "config.h"
#include "sqlcmd.h"
#include <zlib.h>

/*
** Implementation of the "content(X)" SQL function.  Return the complete
** content of artifact identified by X as a blob.
*/
static void sqlcmd_content(
  sqlite4_context *context,
  int argc,
  sqlite4_value **argv
){
  int rid;
  Blob cx;
  const char *zName;
  assert( argc==1 );
  zName = (const char*)sqlite4_value_text(argv[0]);
  if( zName==0 ) return;
  g.db = sqlite4_context_db_handle(context);
  g.repositoryOpen = 1;
  rid = name_to_rid(zName);
  if( rid==0 ) return;
  if( content_get(rid, &cx) ){
    sqlite4_result_blob(context, blob_buffer(&cx), blob_size(&cx), 
                                 SQLITE4_TRANSIENT);
    blob_reset(&cx);
  }
}

/*
** Implementation of the "compress(X)" SQL function.  The input X is
** compressed using zLib and the output is returned.
*/
static void sqlcmd_compress(
  sqlite4_context *context,
  int argc,
  sqlite4_value **argv
){
  const unsigned char *pIn;
  unsigned char *pOut;
  unsigned int nIn;
  unsigned long int nOut;

  pIn = sqlite4_value_blob(argv[0]);
  nIn = sqlite4_value_bytes(argv[0]);
  nOut = 13 + nIn + (nIn+999)/1000;
  pOut = sqlite4_malloc(0, nOut+4);
  pOut[0] = nIn>>24 & 0xff;
  pOut[1] = nIn>>16 & 0xff;
  pOut[2] = nIn>>8 & 0xff;
  pOut[3] = nIn & 0xff;
  compress(&pOut[4], &nOut, pIn, nIn);
  sqlite4_result_blob(context, pOut, nOut+4, SQLITE4_DYNAMIC);
}

/*
** Implementation of the "decompress(X)" SQL function.  The argument X
** is a blob which was obtained from compress(Y).  The output will be
** the value Y.
*/
static void sqlcmd_decompress(
  sqlite4_context *context,
  int argc,
  sqlite4_value **argv
){
  const unsigned char *pIn;
  unsigned char *pOut;
  unsigned int nIn;
  unsigned long int nOut;
  int rc;

  pIn = sqlite4_value_blob(argv[0]);
  nIn = sqlite4_value_bytes(argv[0]);
  nOut = (pIn[0]<<24) + (pIn[1]<<16) + (pIn[2]<<8) + pIn[3];
  pOut = sqlite4_malloc(0, nOut+1);
  rc = uncompress(pOut, &nOut, &pIn[4], nIn-4);
  if( rc==Z_OK ){
    sqlite4_result_blob(context, pOut, nOut, SQLITE4_DYNAMIC);
  }else{
    sqlite4_result_error(context, "input is not zlib compressed", -1);
  }
}

/*
** This is the "automatic extensionn" initializer that runs right after
** the connection to the repository database is opened.  Set up the
** database connection to be more useful to the human operator.
*/
static int sqlcmd_autoinit(
  sqlite4 *db,
  const char **pzErrMsg,
  const void *notUsed
){
  sqlite4_create_function(db, "content", 1, SQLITE4_ANY, 0,
                          sqlcmd_content, 0, 0);
  sqlite4_create_function(db, "compress", 1, SQLITE4_ANY, 0,
                          sqlcmd_compress, 0, 0);
  sqlite4_create_function(db, "decompress", 1, SQLITE4_ANY, 0,
                          sqlcmd_decompress, 0, 0);
  return SQLITE4_OK;
}


/*
** COMMAND: sqlite4
**
** Usage: %fossil sqlite4 ?DATABASE? ?OPTIONS?
**
** Run the standalone sqlite4 command-line shell on DATABASE with OPTIONS.
** If DATABASE is omitted, then the repository that serves the working
** directory is opened.
**
** WARNING:  Careless use of this command can corrupt a Fossil repository
** in ways that are unrecoverable.  Be sure you know what you are doing before
** running any SQL commands that modifies the repository database.
*/
void sqlite4_cmd(void){
  extern int sqlite4_shell(int, char**);
  db_find_and_open_repository(OPEN_ANY_SCHEMA, 0);
  db_close(1);
  sqlite4_shutdown(0);
  sqlite4_shell(g.argc-1, g.argv+1);
}

/*
** This routine is called by the patched sqlite4 command-line shell in order
** to load the name and database connection for the open Fossil database.
*/
void fossil_open(const char **pzRepoName){
  /*sqlite4_auto_extension((void(*)(void))sqlcmd_autoinit);*/
  *pzRepoName = g.zRepositoryName;
}
