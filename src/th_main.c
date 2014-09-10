/*
** Copyright (c) 2008 D. Richard Hipp
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
** This file contains an interface between the TH scripting language
** (an independent project) and fossil.
*/
#include "config.h"
#include "th_main.h"
#include "sqlite3.h"

#if INTERFACE
/*
** Flag parameters to the Th_FossilInit() routine used to control the
** interpreter creation and initialization process.
*/
#define TH_INIT_NONE        ((u32)0x00000000) /* No flags. */
#define TH_INIT_NEED_CONFIG ((u32)0x00000001) /* Open configuration first? */
#define TH_INIT_FORCE_TCL   ((u32)0x00000002) /* Force Tcl to be enabled? */
#define TH_INIT_FORCE_RESET ((u32)0x00000004) /* Force TH1 commands re-added? */
#define TH_INIT_FORCE_SETUP ((u32)0x00000008) /* Force eval of setup script? */
#define TH_INIT_MASK        ((u32)0x0000000F) /* All possible init flags. */

/*
** Useful and/or "well-known" combinations of flag values.
*/
#define TH_INIT_DEFAULT     (TH_INIT_NONE)      /* Default flags. */
#define TH_INIT_HOOK        (TH_INIT_NEED_CONFIG | TH_INIT_FORCE_SETUP)
#define TH_INIT_FORBID_MASK (TH_INIT_FORCE_TCL) /* Illegal from a script. */
#endif

/*
** Flags set by functions in this file to keep track of integration state
** information.  These flags should not be used outside of this file.
*/
#define TH_STATE_CONFIG     ((u32)0x00000010) /* We opened the config. */
#define TH_STATE_REPOSITORY ((u32)0x00000020) /* We opened the repository. */
#define TH_STATE_MASK       ((u32)0x00000030) /* All possible state flags. */

#ifdef FOSSIL_ENABLE_TH1_HOOKS
/*
** These are the "well-known" TH1 error messages that occur when no hook is
** registered to be called prior to executing a command or processing a web
** page, respectively.  If one of these errors is seen, it will not be sent
** or displayed to the remote user or local interactive user, respectively.
*/
#define NO_COMMAND_HOOK_ERROR "no such command:  command_hook"
#define NO_WEBPAGE_HOOK_ERROR "no such command:  webpage_hook"
#endif

/*
** These macros are used within this file to detect if the repository and
** configuration ("user") database are currently open.
*/
#define Th_IsRepositoryOpen()     (g.repositoryOpen)
#define Th_IsConfigOpen()         (g.zConfigDbName!=0)

/*
** Global variable counting the number of outstanding calls to malloc()
** made by the th1 implementation. This is used to catch memory leaks
** in the interpreter. Obviously, it also means th1 is not threadsafe.
*/
static int nOutstandingMalloc = 0;

/*
** Implementations of malloc() and free() to pass to the interpreter.
*/
static void *xMalloc(unsigned int n){
  void *p = fossil_malloc(n);
  if( p ){
    nOutstandingMalloc++;
  }
  return p;
}
static void xFree(void *p){
  if( p ){
    nOutstandingMalloc--;
  }
  free(p);
}
static Th_Vtab vtab = { xMalloc, xFree };

/*
** Returns the number of outstanding TH1 memory allocations.
*/
int Th_GetOutstandingMalloc(){
  return nOutstandingMalloc;
}

/*
** Generate a TH1 trace message if debugging is enabled.
*/
void Th_Trace(const char *zFormat, ...){
  va_list ap;
  va_start(ap, zFormat);
  blob_vappendf(&g.thLog, zFormat, ap);
  va_end(ap);
}

/*
** Forces input and output to be done via the CGI subsystem.
*/
void Th_ForceCgi(int fullHttpReply){
  g.httpOut = stdout;
  g.httpIn = stdin;
  fossil_binary_mode(g.httpOut);
  fossil_binary_mode(g.httpIn);
  g.cgiOutput = 1;
  g.fullHttpReply = fullHttpReply;
}

/*
** Checks if the TH1 trace log needs to be enabled.  If so, prepares
** it for use.
*/
void Th_InitTraceLog(){
  g.thTrace = find_option("th-trace", 0, 0)!=0;
  if( g.thTrace ){
    blob_zero(&g.thLog);
  }
}

/*
** Prints the entire contents of the TH1 trace log to the standard
** output channel.
*/
void Th_PrintTraceLog(){
  if( g.thTrace ){
    fossil_print("\n------------------ BEGIN TRACE LOG ------------------\n");
    fossil_print("%s", blob_str(&g.thLog));
    fossil_print("\n------------------- END TRACE LOG -------------------\n");
  }
}

/*
** TH1 command: httpize STRING
**
** Escape all characters of STRING which have special meaning in URI
** components. Return a new string result.
*/
static int httpizeCmd(
  Th_Interp *interp,
  void *p,
  int argc,
  const char **argv,
  int *argl
){
  char *zOut;
  if( argc!=2 ){
    return Th_WrongNumArgs(interp, "httpize STRING");
  }
  zOut = httpize((char*)argv[1], argl[1]);
  Th_SetResult(interp, zOut, -1);
  free(zOut);
  return TH_OK;
}

/*
** True if output is enabled.  False if disabled.
*/
static int enableOutput = 1;

/*
** TH1 command: enable_output BOOLEAN
**
** Enable or disable the puts and hputs commands.
*/
static int enableOutputCmd(
  Th_Interp *interp,
  void *p,
  int argc,
  const char **argv,
  int *argl
){
  int rc;
  if( argc<2 || argc>3 ){
    return Th_WrongNumArgs(interp, "enable_output [LABEL] BOOLEAN");
  }
  rc = Th_ToInt(interp, argv[argc-1], argl[argc-1], &enableOutput);
  if( g.thTrace ){
    Th_Trace("enable_output {%.*s} -> %d<br>\n", argl[1],argv[1],enableOutput);
  }
  return rc;
}

/*
** Returns a name for a TH1 return code.
*/
const char *Th_ReturnCodeName(int rc, int nullIfOk){
  static char zRc[32];

  switch( rc ){
    case TH_OK:       return nullIfOk ? 0 : "TH_OK";
    case TH_ERROR:    return "TH_ERROR";
    case TH_BREAK:    return "TH_BREAK";
    case TH_RETURN:   return "TH_RETURN";
    case TH_CONTINUE: return "TH_CONTINUE";
    default: {
      sqlite3_snprintf(sizeof(zRc), zRc, "TH1 return code %d", rc);
    }
  }
  return zRc;
}

/*
** Send text to the appropriate output:  Either to the console
** or to the CGI reply buffer.  Escape all characters with special
** meaning to HTML if the encode parameter is true.
*/
static void sendText(const char *z, int n, int encode){
  if( enableOutput && n ){
    if( n<0 ) n = strlen(z);
    if( encode ){
      z = htmlize(z, n);
      n = strlen(z);
    }
    if( g.cgiOutput ){
      cgi_append_content(z, n);
    }else{
      fwrite(z, 1, n, stdout);
      fflush(stdout);
    }
    if( encode ) free((char*)z);
  }
}

static void sendError(const char *z, int n, int forceCgi){
  int savedEnable = enableOutput;
  enableOutput = 1;
  if( forceCgi || g.cgiOutput ){
    sendText("<hr><p class=\"thmainError\">", -1, 0);
  }
  sendText("ERROR: ", -1, 0);
  sendText((char*)z, n, 1);
  sendText(forceCgi || g.cgiOutput ? "</p>" : "\n", -1, 0);
  enableOutput = savedEnable;
}

/*
** TH1 command: puts STRING
** TH1 command: html STRING
**
** Output STRING escaped for HTML (html) or unchanged (puts).
*/
static int putsCmd(
  Th_Interp *interp,
  void *pConvert,
  int argc,
  const char **argv,
  int *argl
){
  if( argc!=2 ){
    return Th_WrongNumArgs(interp, "puts STRING");
  }
  sendText((char*)argv[1], argl[1], *(unsigned int*)pConvert);
  return TH_OK;
}

/*
** TH1 command: wiki STRING
**
** Render the input string as wiki.
*/
static int wikiCmd(
  Th_Interp *interp,
  void *p,
  int argc,
  const char **argv,
  int *argl
){
  int flags = WIKI_INLINE | WIKI_NOBADLINKS | *(unsigned int*)p;
  if( argc!=2 ){
    return Th_WrongNumArgs(interp, "wiki STRING");
  }
  if( enableOutput ){
    Blob src;
    blob_init(&src, (char*)argv[1], argl[1]);
    wiki_convert(&src, 0, flags);
    blob_reset(&src);
  }
  return TH_OK;
}

/*
** TH1 command: htmlize STRING
**
** Escape all characters of STRING which have special meaning in HTML.
** Return a new string result.
*/
static int htmlizeCmd(
  Th_Interp *interp,
  void *p,
  int argc,
  const char **argv,
  int *argl
){
  char *zOut;
  if( argc!=2 ){
    return Th_WrongNumArgs(interp, "htmlize STRING");
  }
  zOut = htmlize((char*)argv[1], argl[1]);
  Th_SetResult(interp, zOut, -1);
  free(zOut);
  return TH_OK;
}

/*
** TH1 command: date
**
** Return a string which is the current time and date.  If the
** -local option is used, the date appears using localtime instead
** of UTC.
*/
static int dateCmd(
  Th_Interp *interp,
  void *p,
  int argc,
  const char **argv,
  int *argl
){
  char *zOut;
  if( argc>=2 && argl[1]==6 && memcmp(argv[1],"-local",6)==0 ){
    zOut = db_text("??", "SELECT datetime('now'%s)", timeline_utc());
  }else{
    zOut = db_text("??", "SELECT datetime('now')");
  }
  Th_SetResult(interp, zOut, -1);
  free(zOut);
  return TH_OK;
}

/*
** TH1 command: hascap STRING...
**
** Return true if the user has all of the capabilities listed in STRING.
*/
static int hascapCmd(
  Th_Interp *interp,
  void *p,
  int argc,
  const char **argv,
  int *argl
){
  int rc = 0, i;
  if( argc<2 ){
    return Th_WrongNumArgs(interp, "hascap STRING ...");
  }
  for(i=1; i<argc && rc==0; i++){
    rc = login_has_capability((char*)argv[i],argl[i]);
  }
  if( g.thTrace ){
    Th_Trace("[hascap %#h] => %d<br />\n", argl[1], argv[1], rc);
  }
  Th_SetResultInt(interp, rc);
  return TH_OK;
}

/*
** TH1 command: hasfeature STRING
**
** Return true if the fossil binary has the given compile-time feature
** enabled. The set of features includes:
**
** "ssl"             = FOSSIL_ENABLE_SSL
** "th1Hooks"        = FOSSIL_ENABLE_TH1_HOOKS
** "tcl"             = FOSSIL_ENABLE_TCL
** "useTclStubs"     = USE_TCL_STUBS
** "tclStubs"        = FOSSIL_ENABLE_TCL_STUBS
** "tclPrivateStubs" = FOSSIL_ENABLE_TCL_PRIVATE_STUBS
** "json"            = FOSSIL_ENABLE_JSON
** "markdown"        = FOSSIL_ENABLE_MARKDOWN
**
*/
static int hasfeatureCmd(
  Th_Interp *interp,
  void *p,
  int argc,
  const char **argv,
  int *argl
){
  int rc = 0;
  char const * zArg;
  if( argc!=2 ){
    return Th_WrongNumArgs(interp, "hasfeature STRING");
  }
  zArg = (char const*)argv[1];
  if(NULL==zArg){
    /* placeholder for following ifdefs... */
  }
#if defined(FOSSIL_ENABLE_SSL)
  else if( 0 == fossil_strnicmp( zArg, "ssl\0", 4 ) ){
    rc = 1;
  }
#endif
#if defined(FOSSIL_ENABLE_TH1_HOOKS)
  else if( 0 == fossil_strnicmp( zArg, "th1Hooks\0", 9 ) ){
    rc = 1;
  }
#endif
#if defined(FOSSIL_ENABLE_TCL)
  else if( 0 == fossil_strnicmp( zArg, "tcl\0", 4 ) ){
    rc = 1;
  }
#endif
#if defined(USE_TCL_STUBS)
  else if( 0 == fossil_strnicmp( zArg, "useTclStubs\0", 12 ) ){
    rc = 1;
  }
#endif
#if defined(FOSSIL_ENABLE_TCL_STUBS)
  else if( 0 == fossil_strnicmp( zArg, "tclStubs\0", 9 ) ){
    rc = 1;
  }
#endif
#if defined(FOSSIL_ENABLE_TCL_PRIVATE_STUBS)
  else if( 0 == fossil_strnicmp( zArg, "tclPrivateStubs\0", 16 ) ){
    rc = 1;
  }
#endif
#if defined(FOSSIL_ENABLE_JSON)
  else if( 0 == fossil_strnicmp( zArg, "json\0", 5 ) ){
    rc = 1;
  }
#endif
  else if( 0 == fossil_strnicmp( zArg, "markdown\0", 9 ) ){
    rc = 1;
  }
  if( g.thTrace ){
    Th_Trace("[hasfeature %#h] => %d<br />\n", argl[1], zArg, rc);
  }
  Th_SetResultInt(interp, rc);
  return TH_OK;
}


/*
** TH1 command: tclReady
**
** Return true if the fossil binary has the Tcl integration feature
** enabled and it is currently available for use by TH1 scripts.
**
*/
static int tclReadyCmd(
  Th_Interp *interp,
  void *p,
  int argc,
  const char **argv,
  int *argl
){
  int rc = 0;
  if( argc!=1 ){
    return Th_WrongNumArgs(interp, "tclReady");
  }
#if defined(FOSSIL_ENABLE_TCL)
  if( g.tcl.interp ){
    rc = 1;
  }
#endif
  if( g.thTrace ){
    Th_Trace("[tclReady] => %d<br />\n", rc);
  }
  Th_SetResultInt(interp, rc);
  return TH_OK;
}


/*
** TH1 command: anycap STRING
**
** Return true if the user has any one of the capabilities listed in STRING.
*/
static int anycapCmd(
  Th_Interp *interp,
  void *p,
  int argc,
  const char **argv,
  int *argl
){
  int rc = 0;
  int i;
  if( argc!=2 ){
    return Th_WrongNumArgs(interp, "anycap STRING");
  }
  for(i=0; rc==0 && i<argl[1]; i++){
    rc = login_has_capability((char*)&argv[1][i],1);
  }
  if( g.thTrace ){
    Th_Trace("[hascap %#h] => %d<br />\n", argl[1], argv[1], rc);
  }
  Th_SetResultInt(interp, rc);
  return TH_OK;
}

/*
** TH1 command: combobox NAME TEXT-LIST NUMLINES
**
** Generate an HTML combobox.  NAME is both the name of the
** CGI parameter and the name of a variable that contains the
** currently selected value.  TEXT-LIST is a list of possible
** values for the combobox.  NUMLINES is 1 for a true combobox.
** If NUMLINES is greater than one then the display is a listbox
** with the number of lines given.
*/
static int comboboxCmd(
  Th_Interp *interp,
  void *p,
  int argc,
  const char **argv,
  int *argl
){
  if( argc!=4 ){
    return Th_WrongNumArgs(interp, "combobox NAME TEXT-LIST NUMLINES");
  }
  if( enableOutput ){
    int height;
    Blob name;
    int nValue;
    const char *zValue;
    char *z, *zH;
    int nElem;
    int *aszElem;
    char **azElem;
    int i;

    if( Th_ToInt(interp, argv[3], argl[3], &height) ) return TH_ERROR;
    Th_SplitList(interp, argv[2], argl[2], &azElem, &aszElem, &nElem);
    blob_init(&name, (char*)argv[1], argl[1]);
    zValue = Th_Fetch(blob_str(&name), &nValue);
    zH = htmlize(blob_buffer(&name), blob_size(&name));
    z = mprintf("<select id=\"%s\" name=\"%s\" size=\"%d\">", zH, zH, height);
    free(zH);
    sendText(z, -1, 0);
    free(z);
    blob_reset(&name);
    for(i=0; i<nElem; i++){
      zH = htmlize((char*)azElem[i], aszElem[i]);
      if( zValue && aszElem[i]==nValue
             && memcmp(zValue, azElem[i], nValue)==0 ){
        z = mprintf("<option value=\"%s\" selected=\"selected\">%s</option>",
                     zH, zH);
      }else{
        z = mprintf("<option value=\"%s\">%s</option>", zH, zH);
      }
      free(zH);
      sendText(z, -1, 0);
      free(z);
    }
    sendText("</select>", -1, 0);
    Th_Free(interp, azElem);
  }
  return TH_OK;
}

/*
** TH1 command: linecount STRING MAX MIN
**
** Return one more than the number of \n characters in STRING.  But
** never return less than MIN or more than MAX.
*/
static int linecntCmd(
  Th_Interp *interp,
  void *p,
  int argc,
  const char **argv,
  int *argl
){
  const char *z;
  int size, n, i;
  int iMin, iMax;
  if( argc!=4 ){
    return Th_WrongNumArgs(interp, "linecount STRING MAX MIN");
  }
  if( Th_ToInt(interp, argv[2], argl[2], &iMax) ) return TH_ERROR;
  if( Th_ToInt(interp, argv[3], argl[3], &iMin) ) return TH_ERROR;
  z = argv[1];
  size = argl[1];
  for(n=1, i=0; i<size; i++){
    if( z[i]=='\n' ){
      n++;
      if( n>=iMax ) break;
    }
  }
  if( n<iMin ) n = iMin;
  if( n>iMax ) n = iMax;
  Th_SetResultInt(interp, n);
  return TH_OK;
}

/*
** TH1 command: repository ?BOOLEAN?
**
** Return the fully qualified file name of the open repository or an empty
** string if one is not currently open.  Optionally, it will attempt to open
** the repository if the boolean argument is non-zero.
*/
static int repositoryCmd(
  Th_Interp *interp,
  void *p,
  int argc,
  const char **argv,
  int *argl
){
  if( argc!=1 && argc!=2 ){
    return Th_WrongNumArgs(interp, "repository ?BOOLEAN?");
  }
  if( argc==2 ){
    int openRepository = 0;
    if( Th_ToInt(interp, argv[1], argl[1], &openRepository) ){
      return TH_ERROR;
    }
    if( openRepository ) db_find_and_open_repository(OPEN_OK_NOT_FOUND, 0);
  }
  Th_SetResult(interp, g.zRepositoryName, -1);
  return TH_OK;
}

/*
** TH1 command: checkout ?BOOLEAN?
**
** Return the fully qualified directory name of the current checkout or an
** empty string if it is not available.  Optionally, it will attempt to find
** the current checkout, opening the configuration ("user") database and the
** repository as necessary, if the boolean argument is non-zero.
*/
static int checkoutCmd(
  Th_Interp *interp,
  void *p,
  int argc,
  const char **argv,
  int *argl
){
  if( argc!=1 && argc!=2 ){
    return Th_WrongNumArgs(interp, "checkout ?BOOLEAN?");
  }
  if( argc==2 ){
    int openCheckout = 0;
    if( Th_ToInt(interp, argv[1], argl[1], &openCheckout) ){
      return TH_ERROR;
    }
    if( openCheckout ) db_open_local(0);
  }
  Th_SetResult(interp, g.zLocalRoot, -1);
  return TH_OK;
}

/*
** TH1 command: trace STRING
**
** Generate a TH1 trace message if debugging is enabled.
*/
static int traceCmd(
  Th_Interp *interp,
  void *p,
  int argc,
  const char **argv,
  int *argl
){
  if( argc!=2 ){
    return Th_WrongNumArgs(interp, "trace STRING");
  }
  if( g.thTrace ){
    Th_Trace("%s", argv[1]);
  }
  Th_SetResult(interp, 0, 0);
  return TH_OK;
}

/*
** TH1 command: globalState NAME ?DEFAULT?
**
** Returns a string containing the value of the specified global state
** variable -OR- the specified default value.  Currently, the supported
** items are:
**
** "checkout"        = The active local checkout directory, if any.
** "configuration"   = The active configuration database file name,
**                     if any.
** "executable"      = The fully qualified executable file name.
** "flags"           = The TH1 initialization flags.
** "log"             = The error log file name, if any.
** "repository"      = The active local repository file name, if
**                     any.
** "top"             = The base path for the active server instance,
**                     if applicable.
** "user"            = The active user name, if any.
** "vfs"             = The SQLite VFS in use, if overridden.
**
** Attempts to query for unsupported global state variables will result
** in a script error.  Additional global state variables may be exposed
** in the future.
**
** See also: checkout, repository, setting
*/
static int globalStateCmd(
  Th_Interp *interp,
  void *p,
  int argc,
  const char **argv,
  int *argl
){
  const char *zDefault = 0;
  if( argc!=2 && argc!=3 ){
    return Th_WrongNumArgs(interp, "globalState NAME ?DEFAULT?");
  }
  if( argc==3 ){
    zDefault = argv[2];
  }
  if( fossil_strnicmp(argv[1], "checkout\0", 9)==0 ){
    Th_SetResult(interp, g.zLocalRoot ? g.zLocalRoot : zDefault, -1);
    return TH_OK;
  }else if( fossil_strnicmp(argv[1], "configuration\0", 14)==0 ){
    Th_SetResult(interp, g.zConfigDbName ? g.zConfigDbName : zDefault, -1);
    return TH_OK;
  }else if( fossil_strnicmp(argv[1], "executable\0", 11)==0 ){
    Th_SetResult(interp, g.nameOfExe ? g.nameOfExe : zDefault, -1);
    return TH_OK;
  }else if( fossil_strnicmp(argv[1], "flags\0", 6)==0 ){
    Th_SetResultInt(interp, g.th1Flags);
    return TH_OK;
  }else if( fossil_strnicmp(argv[1], "log\0", 4)==0 ){
    Th_SetResult(interp, g.zErrlog ? g.zErrlog : zDefault, -1);
    return TH_OK;
  }else if( fossil_strnicmp(argv[1], "repository\0", 11)==0 ){
    Th_SetResult(interp, g.zRepositoryName ? g.zRepositoryName : zDefault, -1);
    return TH_OK;
  }else if( fossil_strnicmp(argv[1], "top\0", 4)==0 ){
    Th_SetResult(interp, g.zTop ? g.zTop : zDefault, -1);
    return TH_OK;
  }else if( fossil_strnicmp(argv[1], "user\0", 5)==0 ){
    Th_SetResult(interp, g.zLogin ? g.zLogin : zDefault, -1);
    return TH_OK;
  }else if( fossil_strnicmp(argv[1], "vfs\0", 4)==0 ){
    Th_SetResult(interp, g.zVfsName ? g.zVfsName : zDefault, -1);
    return TH_OK;
  }else{
    Th_ErrorMessage(interp, "unsupported global state:", argv[1], argl[1]);
    return TH_ERROR;
  }
}

/*
** TH1 command: getParameter NAME ?DEFAULT?
**
** Return the value of the specified query parameter or the specified default
** value when there is no matching query parameter.
*/
static int getParameterCmd(
  Th_Interp *interp,
  void *p,
  int argc,
  const char **argv,
  int *argl
){
  const char *zDefault = 0;
  if( argc!=2 && argc!=3 ){
    return Th_WrongNumArgs(interp, "getParameter NAME ?DEFAULT?");
  }
  if( argc==3 ){
    zDefault = argv[2];
  }
  Th_SetResult(interp, cgi_parameter(argv[1], zDefault), -1);
  return TH_OK;
}

/*
** TH1 command: setParameter NAME VALUE
**
** Sets the value of the specified query parameter.
*/
static int setParameterCmd(
  Th_Interp *interp,
  void *p,
  int argc,
  const char **argv,
  int *argl
){
  if( argc!=3 ){
    return Th_WrongNumArgs(interp, "setParameter NAME VALUE");
  }
  cgi_replace_parameter(mprintf("%s", argv[1]), mprintf("%s", argv[2]));
  return TH_OK;
}

/*
** TH1 command: reinitialize ?FLAGS?
**
** Reinitializes the TH1 interpreter using the specified flags.
*/
static int reinitializeCmd(
  Th_Interp *interp,
  void *p,
  int argc,
  const char **argv,
  int *argl
){
  u32 flags = TH_INIT_DEFAULT;
  if( argc!=1 && argc!=2 ){
    return Th_WrongNumArgs(interp, "reinitialize ?FLAGS?");
  }
  if( argc==2 ){
    int iFlags;
    if( Th_ToInt(interp, argv[1], argl[1], &iFlags) ){
      return TH_ERROR;
    }else{
      flags = (u32)iFlags;
    }
  }
  Th_FossilInit(flags & ~TH_INIT_FORBID_MASK);
  Th_SetResult(interp, 0, 0);
  return TH_OK;
}

/*
** TH1 command: render STRING
**
** Renders the template and writes the results.
*/
static int renderCmd(
  Th_Interp *interp,
  void *p,
  int argc,
  const char **argv,
  int *argl
){
  int rc;
  if( argc!=2 ){
    return Th_WrongNumArgs(interp, "render STRING");
  }
  rc = Th_Render(argv[1]);
  Th_SetResult(interp, 0, 0);
  return rc;
}

/*
** TH1 command: styleHeader TITLE
**
** Render the configured style header.
*/
static int styleHeaderCmd(
  Th_Interp *interp,
  void *p,
  int argc,
  const char **argv,
  int *argl
){
  if( argc!=2 ){
    return Th_WrongNumArgs(interp, "styleHeader TITLE");
  }
  if( Th_IsRepositoryOpen() ){
    style_header("%s", argv[1]);
    Th_SetResult(interp, 0, 0);
    return TH_OK;
  }else{
    Th_SetResult(interp, "repository unavailable", -1);
    return TH_ERROR;
  }
}

/*
** TH1 command: styleFooter
**
** Render the configured style footer.
*/
static int styleFooterCmd(
  Th_Interp *interp,
  void *p,
  int argc,
  const char **argv,
  int *argl
){
  if( argc!=1 ){
    return Th_WrongNumArgs(interp, "styleFooter");
  }
  if( Th_IsRepositoryOpen() ){
    style_footer();
    Th_SetResult(interp, 0, 0);
    return TH_OK;
  }else{
    Th_SetResult(interp, "repository unavailable", -1);
    return TH_ERROR;
  }
}

/*
** TH1 command: artifact ID ?FILENAME?
**
** Attempts to locate the specified artifact and return its contents.  An
** error is generated if the repository is not open or the artifact cannot
** be found.
*/
static int artifactCmd(
  Th_Interp *interp,
  void *p,
  int argc,
  const char **argv,
  int *argl
){
  if( argc!=2 && argc!=3 ){
    return Th_WrongNumArgs(interp, "artifact ID ?FILENAME?");
  }
  if( Th_IsRepositoryOpen() ){
    int rid;
    Blob content;
    if( argc==3 ){
      rid = artifact_from_ci_and_filename(argv[1], argv[2]);
    }else{
      rid = name_to_rid(argv[1]);
    }
    if( rid!=0 && content_get(rid, &content) ){
      Th_SetResult(interp, blob_str(&content), blob_size(&content));
      blob_reset(&content);
      return TH_OK;
    }else{
      Th_SetResult(interp, "artifact not found", -1);
      return TH_ERROR;
    }
  }else{
    Th_SetResult(interp, "repository unavailable", -1);
    return TH_ERROR;
  }
}

#ifdef _WIN32
# include <windows.h>
#else
# include <sys/time.h>
# include <sys/resource.h>
#endif

/*
** Get user and kernel times in microseconds.
*/
static void getCpuTimes(sqlite3_uint64 *piUser, sqlite3_uint64 *piKernel){
#ifdef _WIN32
  FILETIME not_used;
  FILETIME kernel_time;
  FILETIME user_time;
  GetProcessTimes(GetCurrentProcess(), &not_used, &not_used,
                  &kernel_time, &user_time);
  if( piUser ){
     *piUser = ((((sqlite3_uint64)user_time.dwHighDateTime)<<32) +
                         (sqlite3_uint64)user_time.dwLowDateTime + 5)/10;
  }
  if( piKernel ){
     *piKernel = ((((sqlite3_uint64)kernel_time.dwHighDateTime)<<32) +
                         (sqlite3_uint64)kernel_time.dwLowDateTime + 5)/10;
  }
#else
  struct rusage s;
  getrusage(RUSAGE_SELF, &s);
  if( piUser ){
    *piUser = ((sqlite3_uint64)s.ru_utime.tv_sec)*1000000 + s.ru_utime.tv_usec;
  }
  if( piKernel ){
    *piKernel =
              ((sqlite3_uint64)s.ru_stime.tv_sec)*1000000 + s.ru_stime.tv_usec;
  }
#endif
}

/*
** TH1 command: utime
**
** Return the number of microseconds of CPU time consumed by the current
** process in user space.
*/
static int utimeCmd(
  Th_Interp *interp,
  void *p,
  int argc,
  const char **argv,
  int *argl
){
  sqlite3_uint64 x;
  char zUTime[50];
  getCpuTimes(&x, 0);
  sqlite3_snprintf(sizeof(zUTime), zUTime, "%llu", x);
  Th_SetResult(interp, zUTime, -1);
  return TH_OK;
}

/*
** TH1 command: stime
**
** Return the number of microseconds of CPU time consumed by the current
** process in system space.
*/
static int stimeCmd(
  Th_Interp *interp,
  void *p,
  int argc,
  const char **argv,
  int *argl
){
  sqlite3_uint64 x;
  char zUTime[50];
  getCpuTimes(0, &x);
  sqlite3_snprintf(sizeof(zUTime), zUTime, "%llu", x);
  Th_SetResult(interp, zUTime, -1);
  return TH_OK;
}


/*
** TH1 command: randhex  N
**
** Return N*2 random hexadecimal digits with N<50.  If N is omitted,
** use a value of 10.
*/
static int randhexCmd(
  Th_Interp *interp,
  void *p,
  int argc,
  const char **argv,
  int *argl
){
  int n;
  unsigned char aRand[50];
  unsigned char zOut[100];
  if( argc!=1 && argc!=2 ){
    return Th_WrongNumArgs(interp, "repository ?BOOLEAN?");
  }
  if( argc==2 ){
    if( Th_ToInt(interp, argv[1], argl[1], &n) ){
      return TH_ERROR;
    }
    if( n<1 ) n = 1;
    if( n>sizeof(aRand) ) n = sizeof(aRand);
  }else{
    n = 10;
  }
  sqlite3_randomness(n, aRand);
  encode16(aRand, zOut, n);
  Th_SetResult(interp, (const char *)zOut, -1);
  return TH_OK;
}

/*
** TH1 command: query SQL CODE
**
** Run the SQL query given by the SQL argument.  For each row in the result
** set, run CODE.
**
** In SQL, parameters such as $var are filled in using the value of variable
** "var".  Result values are stored in variables with the column name prior
** to each invocation of CODE.
*/
static int queryCmd(
  Th_Interp *interp,
  void *p,
  int argc,
  const char **argv,
  int *argl
){
  sqlite3_stmt *pStmt;
  int rc;
  const char *zSql;
  int nSql;
  const char *zTail;
  int n, i;
  int res = TH_OK;
  int nVar;
  char *zErr = 0;

  if( argc!=3 ){
    return Th_WrongNumArgs(interp, "query SQL CODE");
  }
  if( g.db==0 ){
    Th_ErrorMessage(interp, "database is not open", 0, 0);
    return TH_ERROR;
  }
  zSql = argv[1];
  nSql = argl[1];
  while( res==TH_OK && nSql>0 ){
    zErr = 0;
    sqlite3_set_authorizer(g.db, report_query_authorizer, (void*)&zErr);
    rc = sqlite3_prepare_v2(g.db, argv[1], argl[1], &pStmt, &zTail);
    sqlite3_set_authorizer(g.db, 0, 0);
    if( rc!=0 || zErr!=0 ){
      Th_ErrorMessage(interp, "SQL error: ",
                      zErr ? zErr : sqlite3_errmsg(g.db), -1);
      return TH_ERROR;
    }
    n = (int)(zTail - zSql);
    zSql += n;
    nSql -= n;
    if( pStmt==0 ) continue;
    nVar = sqlite3_bind_parameter_count(pStmt);
    for(i=1; i<=nVar; i++){
      const char *zVar = sqlite3_bind_parameter_name(pStmt, i);
      int szVar = zVar ? th_strlen(zVar) : 0;
      if( szVar>1 && zVar[0]=='$'
       && Th_GetVar(interp, zVar+1, szVar-1)==TH_OK ){
        int nVal;
        const char *zVal = Th_GetResult(interp, &nVal);
        sqlite3_bind_text(pStmt, i, zVal, nVal, SQLITE_TRANSIENT);
      }
    }
    while( res==TH_OK && sqlite3_step(pStmt)==SQLITE_ROW ){
      int nCol = sqlite3_column_count(pStmt);
      for(i=0; i<nCol; i++){
        const char *zCol = sqlite3_column_name(pStmt, i);
        int szCol = th_strlen(zCol);
        const char *zVal = (const char*)sqlite3_column_text(pStmt, i);
        int szVal = sqlite3_column_bytes(pStmt, i);
        Th_SetVar(interp, zCol, szCol, zVal, szVal);
      }
      res = Th_Eval(interp, 0, argv[2], argl[2]);
      if( res==TH_BREAK || res==TH_CONTINUE ) res = TH_OK;
    }
    rc = sqlite3_finalize(pStmt);
    if( rc!=SQLITE_OK ){
      Th_ErrorMessage(interp, "SQL error: ", sqlite3_errmsg(g.db), -1);
      return TH_ERROR;
    }
  }
  return res;
}

/*
** TH1 command: setting name
**
** Gets and returns the value of the specified Fossil setting.
*/
#define SETTING_WRONGNUMARGS "setting ?-strict? ?--? name"
static int settingCmd(
  Th_Interp *interp,
  void *p,
  int argc,
  const char **argv,
  int *argl
){
  int rc;
  int strict = 0;
  int nArg = 1;
  char *zValue;
  if( argc<2 || argc>4 ){
    return Th_WrongNumArgs(interp, SETTING_WRONGNUMARGS);
  }
  if( fossil_strcmp(argv[nArg], "-strict")==0 ){
    strict = 1; nArg++;
  }
  if( fossil_strcmp(argv[nArg], "--")==0 ) nArg++;
  if( nArg+1!=argc ){
    return Th_WrongNumArgs(interp, SETTING_WRONGNUMARGS);
  }
  zValue = db_get(argv[nArg], 0);
  if( zValue!=0 ){
    Th_SetResult(interp, zValue, -1);
    rc = TH_OK;
  }else if( strict ){
    Th_ErrorMessage(interp, "no value for setting \"", argv[nArg], -1);
    rc = TH_ERROR;
  }else{
    Th_SetResult(interp, 0, 0);
    rc = TH_OK;
  }
  if( g.thTrace ){
    Th_Trace("[setting %s%#h] => %d<br />\n", strict ? "strict " : "",
             argl[nArg], argv[nArg], rc);
  }
  return rc;
}

/*
** TH1 command: regexp ?-nocase? ?--? exp string
**
** Checks the string against the specified regular expression and returns
** non-zero if it matches.  If the regular expression is invalid or cannot
** be compiled, an error will be generated.
*/
#define REGEXP_WRONGNUMARGS "regexp ?-nocase? ?--? exp string"
static int regexpCmd(
  Th_Interp *interp,
  void *p,
  int argc,
  const char **argv,
  int *argl
){
  int rc;
  int noCase = 0;
  int nArg = 1;
  ReCompiled *pRe = 0;
  const char *zErr;
  if( argc<3 || argc>5 ){
    return Th_WrongNumArgs(interp, REGEXP_WRONGNUMARGS);
  }
  if( fossil_strcmp(argv[nArg], "-nocase")==0 ){
    noCase = 1; nArg++;
  }
  if( fossil_strcmp(argv[nArg], "--")==0 ) nArg++;
  if( nArg+2!=argc ){
    return Th_WrongNumArgs(interp, REGEXP_WRONGNUMARGS);
  }
  zErr = re_compile(&pRe, argv[nArg], noCase);
  if( !zErr ){
    Th_SetResultInt(interp, re_match(pRe,
        (const unsigned char *)argv[nArg+1], argl[nArg+1]));
    rc = TH_OK;
  }else{
    Th_SetResult(interp, zErr, -1);
    rc = TH_ERROR;
  }
  re_free(pRe);
  return rc;
}

/*
** TH1 command: http ?-asynchronous? ?--? url ?payload?
**
** Perform an HTTP or HTTPS request for the specified URL.  If a
** payload is present, it will be interpreted as text/plain and
** the POST method will be used; otherwise, the GET method will
** be used.  Upon success, if the -asynchronous option is used, an
** empty string is returned as the result; otherwise, the response
** from the server is returned as the result.  Synchronous requests
** are not currently implemented.
*/
#define HTTP_WRONGNUMARGS "http ?-asynchronous? ?--? url ?payload?"
static int httpCmd(
  Th_Interp *interp,
  void *p,
  int argc,
  const char **argv,
  int *argl
){
  int nArg = 1;
  int fAsynchronous = 0;
  const char *zType, *zRegexp;
  Blob payload;
  ReCompiled *pRe = 0;
  UrlData urlData;

  if( argc<2 || argc>5 ){
    return Th_WrongNumArgs(interp, HTTP_WRONGNUMARGS);
  }
  if( fossil_strnicmp(argv[nArg], "-asynchronous", argl[nArg])==0 ){
    fAsynchronous = 1; nArg++;
  }
  if( fossil_strcmp(argv[nArg], "--")==0 ) nArg++;
  if( nArg+1!=argc && nArg+2!=argc ){
    return Th_WrongNumArgs(interp, REGEXP_WRONGNUMARGS);
  }
  memset(&urlData, '\0', sizeof(urlData));
  url_parse_local(argv[nArg], 0, &urlData);
  if( urlData.isSsh || urlData.isFile ){
    Th_ErrorMessage(interp, "url must be http:// or https://", 0, 0);
    return TH_ERROR;
  }
  zRegexp = db_get("th1-uri-regexp", 0);
  if( zRegexp && zRegexp[0] ){
    const char *zErr = re_compile(&pRe, zRegexp, 0);
    if( zErr ){
      Th_SetResult(interp, zErr, -1);
      return TH_ERROR;
    }
  }
  if( !pRe || !re_match(pRe, (const unsigned char *)urlData.canonical, -1) ){
    Th_SetResult(interp, "url not allowed", -1);
    re_free(pRe);
    return TH_ERROR;
  }
  re_free(pRe);
  blob_zero(&payload);
  if( nArg+2==argc ){
    blob_append(&payload, argv[nArg+1], argl[nArg+1]);
    zType = "POST";
  }else{
    zType = "GET";
  }
  if( fAsynchronous ){
    const char *zSep, *zParams;
    Blob hdr;
    zParams = strrchr(argv[nArg], '?');
    if( strlen(urlData.path)>0 && zParams!=argv[nArg] ){
      zSep = "";
    }else{
      zSep = "/";
    }
    blob_zero(&hdr);
    blob_appendf(&hdr, "%s %s%s%s HTTP/1.0\r\n",
                 zType, zSep, urlData.path, zParams ? zParams : "");
    if( urlData.proxyAuth ){
      blob_appendf(&hdr, "Proxy-Authorization: %s\r\n", urlData.proxyAuth);
    }
    if( urlData.passwd && urlData.user && urlData.passwd[0]=='#' ){
      char *zCredentials = mprintf("%s:%s", urlData.user, &urlData.passwd[1]);
      char *zEncoded = encode64(zCredentials, -1);
      blob_appendf(&hdr, "Authorization: Basic %s\r\n", zEncoded);
      fossil_free(zEncoded);
      fossil_free(zCredentials);
    }
    blob_appendf(&hdr, "Host: %s\r\n"
        "User-Agent: %s\r\n", urlData.hostname, get_user_agent());
    if( zType[0]=='P' ){
      blob_appendf(&hdr, "Content-Type: application/x-www-form-urlencoded\r\n"
          "Content-Length: %d\r\n\r\n", blob_size(&payload));
    }else{
      blob_appendf(&hdr, "\r\n");
    }
    if( transport_open(&urlData) ){
      Th_ErrorMessage(interp, transport_errmsg(&urlData), 0, 0);
      blob_reset(&hdr);
      blob_reset(&payload);
      return TH_ERROR;
    }
    transport_send(&urlData, &hdr);
    transport_send(&urlData, &payload);
    blob_reset(&hdr);
    blob_reset(&payload);
    transport_close(&urlData);
    Th_SetResult(interp, 0, 0); /* NOTE: Asynchronous, no results. */
    return TH_OK;
  }else{
    Th_ErrorMessage(interp,
        "synchronous requests are not yet implemented", 0, 0);
    blob_reset(&payload);
    return TH_ERROR;
  }
}

/*
** Attempts to open the configuration ("user") database.  Optionally, also
** attempts to try to find the repository and open it.
*/
void Th_OpenConfig(
  int openRepository
){
  if( openRepository && !Th_IsRepositoryOpen() ){
    db_find_and_open_repository(OPEN_ANY_SCHEMA | OPEN_OK_NOT_FOUND, 0);
    if( Th_IsRepositoryOpen() ){
      g.th1Flags |= TH_STATE_REPOSITORY;
    }else{
      g.th1Flags &= ~TH_STATE_REPOSITORY;
    }
  }
  if( !Th_IsConfigOpen() ){
    db_open_config(0);
    if( Th_IsConfigOpen() ){
      g.th1Flags |= TH_STATE_CONFIG;
    }else{
      g.th1Flags &= ~TH_STATE_CONFIG;
    }
  }
}

/*
** Attempts to close the configuration ("user") database.  Optionally, also
** attempts to close the repository.
*/
void Th_CloseConfig(
  int closeRepository
){
  if( g.th1Flags & TH_STATE_CONFIG ){
    db_close_config();
    g.th1Flags &= ~TH_STATE_CONFIG;
  }
  if( closeRepository && (g.th1Flags & TH_STATE_REPOSITORY) ){
    db_close(1);
    g.th1Flags &= ~TH_STATE_REPOSITORY;
  }
}

/*
** Make sure the interpreter has been initialized.  Initialize it if
** it has not been already.
**
** The interpreter is stored in the g.interp global variable.
*/
void Th_FossilInit(u32 flags){
  int wasInit = 0;
  int needConfig = flags & TH_INIT_NEED_CONFIG;
  int forceReset = flags & TH_INIT_FORCE_RESET;
  int forceTcl = flags & TH_INIT_FORCE_TCL;
  int forceSetup = flags & TH_INIT_FORCE_SETUP;
  static unsigned int aFlags[] = { 0, 1, WIKI_LINKSONLY };
  static struct _Command {
    const char *zName;
    Th_CommandProc xProc;
    void *pContext;
  } aCommand[] = {
    {"anycap",        anycapCmd,            0},
    {"artifact",      artifactCmd,          0},
    {"checkout",      checkoutCmd,          0},
    {"combobox",      comboboxCmd,          0},
    {"date",          dateCmd,              0},
    {"decorate",      wikiCmd,              (void*)&aFlags[2]},
    {"enable_output", enableOutputCmd,      0},
    {"getParameter",  getParameterCmd,      0},
    {"globalState",   globalStateCmd,       0},
    {"httpize",       httpizeCmd,           0},
    {"hascap",        hascapCmd,            0},
    {"hasfeature",    hasfeatureCmd,        0},
    {"html",          putsCmd,              (void*)&aFlags[0]},
    {"htmlize",       htmlizeCmd,           0},
    {"http",          httpCmd,              0},
    {"linecount",     linecntCmd,           0},
    {"puts",          putsCmd,              (void*)&aFlags[1]},
    {"query",         queryCmd,             0},
    {"randhex",       randhexCmd,           0},
    {"regexp",        regexpCmd,            0},
    {"reinitialize",  reinitializeCmd,      0},
    {"render",        renderCmd,            0},
    {"repository",    repositoryCmd,        0},
    {"setParameter",  setParameterCmd,      0},
    {"setting",       settingCmd,           0},
    {"styleHeader",   styleHeaderCmd,       0},
    {"styleFooter",   styleFooterCmd,       0},
    {"tclReady",      tclReadyCmd,          0},
    {"trace",         traceCmd,             0},
    {"stime",         stimeCmd,             0},
    {"utime",         utimeCmd,             0},
    {"wiki",          wikiCmd,              (void*)&aFlags[0]},
    {0, 0, 0}
  };
  if( g.thTrace ){
    Th_Trace("th1-init 0x%x => 0x%x<br />\n", g.th1Flags, flags);
  }
  if( needConfig ){
    /*
    ** This function uses several settings which may be defined in the
    ** repository and/or the global configuration.  Since the caller
    ** passed a non-zero value for the needConfig parameter, make sure
    ** the necessary database connections are open prior to continuing.
    */
    Th_OpenConfig(1);
  }
  if( forceReset || forceTcl || g.interp==0 ){
    int created = 0;
    int i;
    if( g.interp==0 ){
      g.interp = Th_CreateInterp(&vtab);
      created = 1;
    }
    if( forceReset || created ){
      th_register_language(g.interp);     /* Basic scripting commands. */
    }
#ifdef FOSSIL_ENABLE_TCL
    if( forceTcl || fossil_getenv("TH1_ENABLE_TCL")!=0 ||
        db_get_boolean("tcl", 0) ){
      if( !g.tcl.setup ){
        g.tcl.setup = db_get("tcl-setup", 0); /* Grab Tcl setup script. */
      }
      th_register_tcl(g.interp, &g.tcl);  /* Tcl integration commands. */
    }
#endif
    for(i=0; i<sizeof(aCommand)/sizeof(aCommand[0]); i++){
      if ( !aCommand[i].zName || !aCommand[i].xProc ) continue;
      Th_CreateCommand(g.interp, aCommand[i].zName, aCommand[i].xProc,
                       aCommand[i].pContext, 0);
    }
  }else{
    wasInit = 1;
  }
  if( forceSetup || !wasInit ){
    int rc = TH_OK;
    if( !g.th1Setup ){
      g.th1Setup = db_get("th1-setup", 0); /* Grab TH1 setup script. */
    }
    if( g.th1Setup ){
      rc = Th_Eval(g.interp, 0, g.th1Setup, -1);
      if( rc==TH_ERROR ){
        int nResult = 0;
        char *zResult = (char*)Th_GetResult(g.interp, &nResult);
        sendError(zResult, nResult, 0);
      }
    }
    if( g.thTrace ){
      Th_Trace("th1-setup {%h} => %h<br />\n", g.th1Setup,
               Th_ReturnCodeName(rc, 0));
    }
  }
  g.th1Flags &= ~TH_INIT_MASK;
  g.th1Flags |= (flags & TH_INIT_MASK);
}

/*
** Store a string value in a variable in the interpreter.
*/
void Th_Store(const char *zName, const char *zValue){
  Th_FossilInit(TH_INIT_DEFAULT);
  if( zValue ){
    if( g.thTrace ){
      Th_Trace("set %h {%h}<br />\n", zName, zValue);
    }
    Th_SetVar(g.interp, zName, -1, zValue, strlen(zValue));
  }
}

/*
** Appends an element to a TH1 list value.  This function is called by the
** transfer subsystem; therefore, it must be very careful to avoid doing
** any unnecessary work.  To that end, the TH1 subsystem will not be called
** or initialized if the list pointer is zero (i.e. which will be the case
** when TH1 transfer hooks are disabled).
*/
void Th_AppendToList(
  char **pzList,
  int *pnList,
  const char *zElem,
  int nElem
){
  if( pzList && zElem ){
    Th_FossilInit(TH_INIT_DEFAULT);
    Th_ListAppend(g.interp, pzList, pnList, zElem, nElem);
  }
}

/*
** Stores a list value in the specified TH1 variable using the specified
** array of strings as the source of the element values.
*/
void Th_StoreList(
  const char *zName,
  char **pzList,
  int nList
){
  Th_FossilInit(TH_INIT_DEFAULT);
  if( pzList ){
    char *zValue = 0;
    int nValue = 0;
    int i;
    for(i=0; i<nList; i++){
      Th_ListAppend(g.interp, &zValue, &nValue, pzList[i], -1);
    }
    if( g.thTrace ){
      Th_Trace("set %h {%h}<br />\n", zName, zValue);
    }
    Th_SetVar(g.interp, zName, -1, zValue, nValue);
    Th_Free(g.interp, zValue);
  }
}

/*
** Store an integer value in a variable in the interpreter.
*/
void Th_StoreInt(const char *zName, int iValue){
  Blob value;
  char *zValue;
  Th_FossilInit(TH_INIT_DEFAULT);
  blob_zero(&value);
  blob_appendf(&value, "%d", iValue);
  zValue = blob_str(&value);
  if( g.thTrace ){
    Th_Trace("set %h {%h}<br />\n", zName, zValue);
  }
  Th_SetVar(g.interp, zName, -1, zValue, strlen(zValue));
  blob_reset(&value);
}

/*
** Unset a variable.
*/
void Th_Unstore(const char *zName){
  if( g.interp ){
    Th_UnsetVar(g.interp, (char*)zName, -1);
  }
}

/*
** Retrieve a string value from the interpreter.  If no such
** variable exists, return NULL.
*/
char *Th_Fetch(const char *zName, int *pSize){
  int rc;
  Th_FossilInit(TH_INIT_DEFAULT);
  rc = Th_GetVar(g.interp, (char*)zName, -1);
  if( rc==TH_OK ){
    return (char*)Th_GetResult(g.interp, pSize);
  }else{
    return 0;
  }
}

/*
** Return true if the string begins with the TH1 begin-script
** tag:  <th1>.
*/
static int isBeginScriptTag(const char *z){
  return z[0]=='<'
      && (z[1]=='t' || z[1]=='T')
      && (z[2]=='h' || z[2]=='H')
      && z[3]=='1'
      && z[4]=='>';
}

/*
** Return true if the string begins with the TH1 end-script
** tag:  </th1>.
*/
static int isEndScriptTag(const char *z){
  return z[0]=='<'
      && z[1]=='/'
      && (z[2]=='t' || z[2]=='T')
      && (z[3]=='h' || z[3]=='H')
      && z[4]=='1'
      && z[5]=='>';
}

/*
** If string z[0...] contains a valid variable name, return
** the number of characters in that name.  Otherwise, return 0.
*/
static int validVarName(const char *z){
  int i = 0;
  int inBracket = 0;
  if( z[0]=='<' ){
    inBracket = 1;
    z++;
  }
  if( z[0]==':' && z[1]==':' && fossil_isalpha(z[2]) ){
    z += 3;
    i += 3;
  }else if( fossil_isalpha(z[0]) ){
    z ++;
    i += 1;
  }else{
    return 0;
  }
  while( fossil_isalnum(z[0]) || z[0]=='_' ){
    z++;
    i++;
  }
  if( inBracket ){
    if( z[0]!='>' ) return 0;
    i += 2;
  }
  return i;
}

#ifdef FOSSIL_ENABLE_TH1_HOOKS
/*
** This function determines if TH1 hooks are enabled for the repository.  It
** may be necessary to open the repository and/or the configuration ("user")
** database from within this function.  Before this function returns, any
** database opened will be closed again.  This is very important because some
** commands do not expect the repository and/or the configuration ("user")
** database to be open prior to their own code doing so.
*/
int Th_AreHooksEnabled(void){
  int rc;
  if( fossil_getenv("TH1_ENABLE_HOOKS")!=0 ){
    return 1;
  }
  Th_OpenConfig(1);
  rc = db_get_boolean("th1-hooks", 0);
  Th_CloseConfig(1);
  return rc;
}

/*
** This function is called by Fossil just prior to dispatching a command.
** Returning a value other than TH_OK from this function (i.e. via an
** evaluated script raising an error or calling [break]/[continue]) will
** cause the actual command execution to be skipped.
*/
int Th_CommandHook(
  const char *zName,
  char cmdFlags
){
  int rc = TH_OK;
  if( !Th_AreHooksEnabled() ) return rc;
  Th_FossilInit(TH_INIT_HOOK);
  Th_Store("cmd_name", zName);
  Th_StoreList("cmd_args", g.argv, g.argc);
  Th_StoreInt("cmd_flags", cmdFlags);
  rc = Th_Eval(g.interp, 0, "command_hook", -1);
  if( rc==TH_ERROR ){
    int nResult = 0;
    char *zResult = (char*)Th_GetResult(g.interp, &nResult);
    /*
    ** Make sure that the TH1 script error was not caused by a "missing"
    ** command hook handler as that is not actually an error condition.
    */
    if( memcmp(zResult, NO_COMMAND_HOOK_ERROR, nResult)!=0 ){
      sendError(zResult, nResult, 0);
    }
  }
  /*
  ** If the script returned TH_ERROR (e.g. the "command_hook" TH1 command does
  ** not exist because commands are not being hooked), return TH_OK because we
  ** do not want to skip executing essential commands unless the called command
  ** (i.e. "command_hook") explicitly forbids this by successfully returning
  ** TH_BREAK or TH_CONTINUE.
  */
  if( g.thTrace ){
    Th_Trace("[command_hook {%h}] => %h<br />\n", zName,
             Th_ReturnCodeName(rc, 0));
  }
  /*
  ** Does our call to Th_FossilInit() result in opening a database?  If so,
  ** clean it up now.  This is very important because some commands do not
  ** expect the repository and/or the configuration ("user") database to be
  ** open prior to their own code doing so.
  */
  if( TH_INIT_HOOK & TH_INIT_NEED_CONFIG ) Th_CloseConfig(1);
  return (rc != TH_ERROR) ? rc : TH_OK;
}

/*
** This function is called by Fossil just after dispatching a command.
** Returning a value other than TH_OK from this function (i.e. via an
** evaluated script raising an error or calling [break]/[continue]) may
** cause an error message to be displayed to the local interactive user.
** Currently, TH1 error messages generated by this function are ignored.
*/
int Th_CommandNotify(
  const char *zName,
  char cmdFlags
){
  int rc = TH_OK;
  if( !Th_AreHooksEnabled() ) return rc;
  Th_FossilInit(TH_INIT_HOOK);
  Th_Store("cmd_name", zName);
  Th_StoreList("cmd_args", g.argv, g.argc);
  Th_StoreInt("cmd_flags", cmdFlags);
  rc = Th_Eval(g.interp, 0, "command_notify", -1);
  if( g.thTrace ){
    Th_Trace("[command_notify {%h}] => %h<br />\n", zName,
             Th_ReturnCodeName(rc, 0));
  }
  /*
  ** Does our call to Th_FossilInit() result in opening a database?  If so,
  ** clean it up now.  This is very important because some commands do not
  ** expect the repository and/or the configuration ("user") database to be
  ** open prior to their own code doing so.
  */
  if( TH_INIT_HOOK & TH_INIT_NEED_CONFIG ) Th_CloseConfig(1);
  return rc;
}

/*
** This function is called by Fossil just prior to processing a web page.
** Returning a value other than TH_OK from this function (i.e. via an
** evaluated script raising an error or calling [break]/[continue]) will
** cause the actual web page processing to be skipped.
*/
int Th_WebpageHook(
  const char *zName,
  char cmdFlags
){
  int rc = TH_OK;
  if( !Th_AreHooksEnabled() ) return rc;
  Th_FossilInit(TH_INIT_HOOK);
  Th_Store("web_name", zName);
  Th_StoreList("web_args", g.argv, g.argc);
  Th_StoreInt("web_flags", cmdFlags);
  rc = Th_Eval(g.interp, 0, "webpage_hook", -1);
  if( rc==TH_ERROR ){
    int nResult = 0;
    char *zResult = (char*)Th_GetResult(g.interp, &nResult);
    /*
    ** Make sure that the TH1 script error was not caused by a "missing"
    ** webpage hook handler as that is not actually an error condition.
    */
    if( memcmp(zResult, NO_WEBPAGE_HOOK_ERROR, nResult)!=0 ){
      sendError(zResult, nResult, 1);
    }
  }
  /*
  ** If the script returned TH_ERROR (e.g. the "webpage_hook" TH1 command does
  ** not exist because commands are not being hooked), return TH_OK because we
  ** do not want to skip processing essential web pages unless the called
  ** command (i.e. "webpage_hook") explicitly forbids this by successfully
  ** returning TH_BREAK or TH_CONTINUE.
  */
  if( g.thTrace ){
    Th_Trace("[webpage_hook {%h}] => %h<br />\n", zName,
             Th_ReturnCodeName(rc, 0));
  }
  /*
  ** Does our call to Th_FossilInit() result in opening a database?  If so,
  ** clean it up now.  This is very important because some commands do not
  ** expect the repository and/or the configuration ("user") database to be
  ** open prior to their own code doing so.
  */
  if( TH_INIT_HOOK & TH_INIT_NEED_CONFIG ) Th_CloseConfig(1);
  return (rc != TH_ERROR) ? rc : TH_OK;
}

/*
** This function is called by Fossil just after processing a web page.
** Returning a value other than TH_OK from this function (i.e. via an
** evaluated script raising an error or calling [break]/[continue]) may
** cause an error message to be displayed to the remote user.
** Currently, TH1 error messages generated by this function are ignored.
*/
int Th_WebpageNotify(
  const char *zName,
  char cmdFlags
){
  int rc = TH_OK;
  if( !Th_AreHooksEnabled() ) return rc;
  Th_FossilInit(TH_INIT_HOOK);
  Th_Store("web_name", zName);
  Th_StoreList("web_args", g.argv, g.argc);
  Th_StoreInt("web_flags", cmdFlags);
  rc = Th_Eval(g.interp, 0, "webpage_notify", -1);
  if( g.thTrace ){
    Th_Trace("[webpage_notify {%h}] => %h<br />\n", zName,
             Th_ReturnCodeName(rc, 0));
  }
  /*
  ** Does our call to Th_FossilInit() result in opening a database?  If so,
  ** clean it up now.  This is very important because some commands do not
  ** expect the repository and/or the configuration ("user") database to be
  ** open prior to their own code doing so.
  */
  if( TH_INIT_HOOK & TH_INIT_NEED_CONFIG ) Th_CloseConfig(1);
  return rc;
}
#endif

/*
** The z[] input contains text mixed with TH1 scripts.
** The TH1 scripts are contained within <th1>...</th1>.
** TH1 variables are $aaa or $<aaa>.  The first form of
** variable is literal.  The second is run through htmlize
** before being inserted.
**
** This routine processes the template and writes the results
** on either stdout or into CGI.
*/
int Th_Render(const char *z){
  int i = 0;
  int n;
  int rc = TH_OK;
  char *zResult;
  Th_FossilInit(TH_INIT_DEFAULT);
  while( z[i] ){
    if( z[i]=='$' && (n = validVarName(&z[i+1]))>0 ){
      const char *zVar;
      int nVar;
      int encode = 1;
      sendText(z, i, 0);
      if( z[i+1]=='<' ){
        /* Variables of the form $<aaa> are html escaped */
        zVar = &z[i+2];
        nVar = n-2;
      }else{
        /* Variables of the form $aaa are output raw */
        zVar = &z[i+1];
        nVar = n;
        encode = 0;
      }
      rc = Th_GetVar(g.interp, (char*)zVar, nVar);
      z += i+1+n;
      i = 0;
      zResult = (char*)Th_GetResult(g.interp, &n);
      sendText((char*)zResult, n, encode);
    }else if( z[i]=='<' && isBeginScriptTag(&z[i]) ){
      sendText(z, i, 0);
      z += i+5;
      for(i=0; z[i] && (z[i]!='<' || !isEndScriptTag(&z[i])); i++){}
      if( g.thTrace ){
        Th_Trace("eval {<pre>%#h</pre>}<br>", i, z);
      }
      rc = Th_Eval(g.interp, 0, (const char*)z, i);
      if( rc!=TH_OK ) break;
      z += i;
      if( z[0] ){ z += 6; }
      i = 0;
    }else{
      i++;
    }
  }
  if( rc==TH_ERROR ){
    zResult = (char*)Th_GetResult(g.interp, &n);
    sendError(zResult, n, 1);
  }else{
    sendText(z, i, 0);
  }
  return rc;
}

/*
** COMMAND: test-th-render
*/
void test_th_render(void){
  int forceCgi, fullHttpReply;
  Blob in;
  Th_InitTraceLog();
  forceCgi = find_option("th-force-cgi", 0, 0)!=0;
  fullHttpReply = find_option("th-full-http", 0, 0)!=0;
  if( forceCgi ) Th_ForceCgi(fullHttpReply);
  if( find_option("th-open-config", 0, 0)!=0 ){
    Th_OpenConfig(1);
  }
  if( g.argc<3 ){
    usage("FILE");
  }
  blob_zero(&in);
  blob_read_from_file(&in, g.argv[2]);
  Th_Render(blob_str(&in));
  Th_PrintTraceLog();
  if( forceCgi ) cgi_reply();
}

/*
** COMMAND: test-th-eval
*/
void test_th_eval(void){
  int rc;
  const char *zRc;
  int forceCgi, fullHttpReply;
  Th_InitTraceLog();
  forceCgi = find_option("th-force-cgi", 0, 0)!=0;
  fullHttpReply = find_option("th-full-http", 0, 0)!=0;
  if( forceCgi ) Th_ForceCgi(fullHttpReply);
  if( find_option("th-open-config", 0, 0)!=0 ){
    Th_OpenConfig(1);
  }
  if( g.argc!=3 ){
    usage("script");
  }
  Th_FossilInit(TH_INIT_DEFAULT);
  rc = Th_Eval(g.interp, 0, g.argv[2], -1);
  zRc = Th_ReturnCodeName(rc, 1);
  fossil_print("%s%s%s\n", zRc, zRc ? ": " : "", Th_GetResult(g.interp, 0));
  Th_PrintTraceLog();
  if( forceCgi ) cgi_reply();
}

#ifdef FOSSIL_ENABLE_TH1_HOOKS
/*
** COMMAND: test-th-hook
*/
void test_th_hook(void){
  int rc = TH_OK;
  int nResult = 0;
  char *zResult;
  int forceCgi, fullHttpReply;
  Th_InitTraceLog();
  forceCgi = find_option("th-force-cgi", 0, 0)!=0;
  fullHttpReply = find_option("th-full-http", 0, 0)!=0;
  if( forceCgi ) Th_ForceCgi(fullHttpReply);
  if( g.argc<5 ){
    usage("TYPE NAME FLAGS");
  }
  if( fossil_stricmp(g.argv[2], "cmdhook")==0 ){
    rc = Th_CommandHook(g.argv[3], (char)atoi(g.argv[4]));
  }else if( fossil_stricmp(g.argv[2], "cmdnotify")==0 ){
    rc = Th_CommandNotify(g.argv[3], (char)atoi(g.argv[4]));
  }else if( fossil_stricmp(g.argv[2], "webhook")==0 ){
    rc = Th_WebpageHook(g.argv[3], (char)atoi(g.argv[4]));
  }else if( fossil_stricmp(g.argv[2], "webnotify")==0 ){
    rc = Th_WebpageNotify(g.argv[3], (char)atoi(g.argv[4]));
  }else{
    fossil_fatal("Unknown TH1 hook %s\n", g.argv[2]);
  }
  zResult = (char*)Th_GetResult(g.interp, &nResult);
  sendText("RESULT (", -1, 0);
  sendText(Th_ReturnCodeName(rc, 0), -1, 0);
  sendText("): ", -1, 0);
  sendText(zResult, nResult, 0);
  sendText("\n", -1, 0);
  Th_PrintTraceLog();
  if( forceCgi ) cgi_reply();
}
#endif
