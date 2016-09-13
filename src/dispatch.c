/*
** Copyright (c) 2016 D. Richard Hipp
**
** This program is free software; you can redistribute it and/or
** modify it under the terms of the Simplified BSD License (also
** known as the "2-Clause License" or "FreeBSD License".)
**
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
** This file contains code used to map command names (ex: "help", "commit",
** "diff") or webpage names (ex: "/timeline", "/search") into the functions
** that implement those commands and web pages and their associated help
** text.
*/
#include "config.h"
#include <assert.h>
#include "dispatch.h"


#if INTERFACE
/*
** An instance of this object defines everything we need to know about an
** individual command or webpage.
*/
struct CmdOrPage {
  const char *zName;       /* Name.  Webpages start with "/". Commands do not */
  void (*xFunc)(void);     /* Function that implements the command or webpage */
  const char *zHelp;       /* Raw help text */
  unsigned eCmdFlags;      /* Flags */
};

/***************************************************************************
** These macros must match similar macros in mkindex.c
** Allowed values for CmdOrPage.eCmdFlags.
*/
#define CMDFLAG_1ST_TIER  0x0001      /* Most important commands */
#define CMDFLAG_2ND_TIER  0x0002      /* Obscure and seldom used commands */
#define CMDFLAG_TEST      0x0004      /* Commands for testing only */
#define CMDFLAG_WEBPAGE   0x0008      /* Web pages */
#define CMDFLAG_COMMAND   0x0010      /* A command */
/**************************************************************************/

/* Only the bits above that are part of CMDFLAG_TH_MASK are passed into
** the TH1 hook procedures. */
#define CMDFLAG_TH_MASK   0x000f      /* Legacy flags only */

/* Values for the 2nd parameter to dispatch_name_search() */
#define CMDFLAG_ANY       0x0018      /* Match anything */
#define CMDFLAG_PREFIX    0x0020      /* Prefix match is ok */

#endif /* INTERFACE */

/*
** The page_index.h file contains the definition for aCommand[] - an array
** of CmdOrPage objects that defines all available commands and webpages
** known to Fossil.
**
** The entries in aCommand[] are in sorted order by name.  Since webpage names
** always begin with "/", all webpage names occur first.  The page_index.h file
** also sets the FOSSIL_FIRST_CMD macro to be the *approximate* index 
** in aCommand[] of the first command entry.  FOSSIL_FIRST_CMD might be
** slightly too low, and so the range FOSSIL_FIRST_CMD...MX_COMMAND might
** contain a few webpage entries at the beginning.
**
** The page_index.h file is generated by the mkindex program which scans all
** source code files looking for header comments on the functions that
** implement command and webpages.
*/
#include "page_index.h"
#define MX_COMMAND (sizeof(aCommand)/sizeof(aCommand[0]))

/*
** Given a command or webpage name in zName, find the corresponding CmdOrPage
** object and return a pointer to that object in *ppCmd.
**
** The eType field is CMDFLAG_COMMAND to lookup commands or CMDFLAG_WEBPAGE
** to look up webpages or CMDFLAG_ANY to look for either.  If the CMDFLAG_PREFIX
** flag is set, then a prefix match is allowed.
**
** Return values:
**    0:     Success.  *ppCmd is set to the appropriate CmdOrPage
**    1:     Not found.
**    2:     Ambiguous.  Two or more entries match.
*/
int dispatch_name_search(
  const char *zName,           /* Look for this name */
  unsigned eType,              /* CMDFLAGS_* bits */
  const CmdOrPage **ppCmd      /* Write the matching CmdOrPage object here */
){
  int upr, lwr, mid;
  int nName = strlen(zName);
  lwr = 0;
  upr = MX_COMMAND - 1;
  while( lwr<=upr ){
    int c;
    mid = (upr+lwr)/2;
    c = strcmp(zName, aCommand[mid].zName);
    if( c==0 ){
      *ppCmd = &aCommand[mid];
      return 0;  /* An exact match */
    }else if( c<0 ){
      upr = mid - 1;
    }else{
      lwr = mid + 1;
    }
  }
  if( (eType & CMDFLAG_PREFIX)!=0
   && lwr<MX_COMMAND
   && strncmp(zName, aCommand[lwr].zName, nName)==0
  ){
    if( lwr<MX_COMMAND-1 && strncmp(zName, aCommand[lwr+1].zName, nName)==0 ){
      return 2;  /* Ambiguous prefix */
    }else{
      *ppCmd = &aCommand[lwr];
      return 0;  /* Prefix match */
    }
  }
  return 1;  /* Not found */
}

/*
** Fill Blob with a space-separated list of all command names that
** match the prefix zPrefix.
*/
void dispatch_matching_names(const char *zPrefix, Blob *pList){
  int i;
  int nPrefix = (int)strlen(zPrefix);
  for(i=FOSSIL_FIRST_CMD; i<MX_COMMAND; i++){
    if( strncmp(zPrefix, aCommand[i].zName, nPrefix)==0 ){
      blob_appendf(pList, " %s", aCommand[i].zName);
    }
  }
}


/*
** COMMAND: test-all-help
**
** Usage: %fossil test-all-help ?OPTIONS?
**
** Show help text for commands and pages.  Useful for proof-reading.
** Defaults to just the CLI commands.  Specify --www to see only the
** web pages, or --everything to see both commands and pages.
**
** Options:
**    -e|--everything   Show all commands and pages.
**    -t|--test         Include test- commands
**    -w|--www          Show WWW pages.
*/
void test_all_help_cmd(void){
  int i;
  int mask = CMDFLAG_1ST_TIER | CMDFLAG_2ND_TIER;

  if( find_option("www","w",0) ){
    mask = CMDFLAG_WEBPAGE;
  }
  if( find_option("everything","e",0) ){
    mask = CMDFLAG_1ST_TIER | CMDFLAG_2ND_TIER | CMDFLAG_WEBPAGE;
  }
  if( find_option("test","t",0) ){
    mask |= CMDFLAG_TEST;
  }
  fossil_print("Help text for:\n");
  if( mask & CMDFLAG_1ST_TIER ) fossil_print(" * Commands\n");
  if( mask & CMDFLAG_2ND_TIER ) fossil_print(" * Auxiliary commands\n");
  if( mask & CMDFLAG_TEST )     fossil_print(" * Test commands\n");
  if( mask & CMDFLAG_WEBPAGE )  fossil_print(" * Web pages\n");
  fossil_print("---\n");
  for(i=0; i<MX_COMMAND; i++){
    if( (aCommand[i].eCmdFlags & mask)==0 ) continue;
    fossil_print("# %s\n", aCommand[i].zName);
    fossil_print("%s\n\n", aCommand[i].zHelp);
  }
  fossil_print("---\n");
  version_cmd();
}

/*
** WEBPAGE: help
** URL: /help?name=CMD
**
** Show the built-in help text for CMD.  CMD can be a command-line interface
** command or a page name from the web interface.
*/
void help_page(void){
  const char *zCmd = P("cmd");

  if( zCmd==0 ) zCmd = P("name");
  style_header("Command-line Help");
  if( zCmd ){
    int rc;
    char *z, *s, *d;
    const CmdOrPage *pCmd = 0;

    style_submenu_element("Command-List", "Command-List", "%s/help", g.zTop);
    if( *zCmd=='/' ){
      /* Some of the webpages require query parameters in order to work.
      ** @ <h1>The "<a href='%R%s(zCmd)'>%s(zCmd)</a>" page:</h1> */
      @ <h1>The "%s(zCmd)" page:</h1>
    }else{
      @ <h1>The "%s(zCmd)" command:</h1>
    }
    rc = dispatch_name_search(zCmd, CMDFLAG_ANY, &pCmd);
    if( rc==1 ){
      @ unknown command: %s(zCmd)
    }else if( rc==2 ){
      @ ambiguous command prefix: %s(zCmd)
    }else{
      z = (char*)pCmd->zHelp;
      if( z[0]==0 ){
        @ no help available for the %s(pCmd->zName) command
      }else{
        z=s=d=mprintf("%s",z);
        while( *s ){
          if( *s=='%' && strncmp(s, "%fossil", 7)==0 ){
            s++;
          }else{
            *d++ = *s++;
          }
        }
        *d = 0;
        @ <blockquote><pre>
        @ %h(z)
        @ </pre></blockquote>
        fossil_free(z);
      }
    }
  }else{
    int i, j, n;

    @ <h1>Available commands:</h1>
    @ <table border="0"><tr>
    for(i=j=0; i<MX_COMMAND; i++){
      const char *z = aCommand[i].zName;
      if( '/'==*z || strncmp(z,"test",4)==0 ) continue;
      j++;
    }
    n = (j+6)/7;
    for(i=j=0; i<MX_COMMAND; i++){
      const char *z = aCommand[i].zName;
      if( '/'==*z || strncmp(z,"test",4)==0 ) continue;
      if( j==0 ){
        @ <td valign="top"><ul>
      }
      @ <li><a href="%R/help?cmd=%s(z)">%s(z)</a></li>
      j++;
      if( j>=n ){
        @ </ul></td>
        j = 0;
      }
    }
    if( j>0 ){
      @ </ul></td>
    }
    @ </tr></table>

    @ <h1>Available web UI pages:</h1>
    @ <table border="0"><tr>
    for(i=j=0; i<MX_COMMAND; i++){
      const char *z = aCommand[i].zName;
      if( '/'!=*z ) continue;
      j++;
    }
    n = (j+4)/5;
    for(i=j=0; i<MX_COMMAND; i++){
      const char *z = aCommand[i].zName;
      if( '/'!=*z ) continue;
      if( j==0 ){
        @ <td valign="top"><ul>
      }
      if( aCommand[i].zHelp[0] ){
        @ <li><a href="%R/help?cmd=%s(z)">%s(z+1)</a></li>
      }else{
        @ <li>%s(z+1)</li>
      }
      j++;
      if( j>=n ){
        @ </ul></td>
        j = 0;
      }
    }
    if( j>0 ){
      @ </ul></td>
    }
    @ </tr></table>

    @ <h1>Unsupported commands:</h1>
    @ <table border="0"><tr>
    for(i=j=0; i<MX_COMMAND; i++){
      const char *z = aCommand[i].zName;
      if( strncmp(z,"test",4)!=0 ) continue;
      j++;
    }
    n = (j+3)/4;
    for(i=j=0; i<MX_COMMAND; i++){
      const char *z = aCommand[i].zName;
      if( strncmp(z,"test",4)!=0 ) continue;
      if( j==0 ){
        @ <td valign="top"><ul>
      }
      if( aCommand[i].zHelp[0] ){
        @ <li><a href="%R/help?cmd=%s(z)">%s(z)</a></li>
      }else{
        @ <li>%s(z)</li>
      }
      j++;
      if( j>=n ){
        @ </ul></td>
        j = 0;
      }
    }
    if( j>0 ){
      @ </ul></td>
    }
    @ </tr></table>

  }
  style_footer();
}

/*
** WEBPAGE: test-all-help
**
** Show all help text on a single page.  Useful for proof-reading.
*/
void test_all_help_page(void){
  int i;
  style_header("Testpage: All Help Text");
  for(i=0; i<MX_COMMAND; i++){
    if( memcmp(aCommand[i].zName, "test", 4)==0 ) continue;
    @ <h2>%s(aCommand[i].zName):</h2>
    @ <blockquote><pre>
    @ %h(aCommand[i].zHelp)
    @ </pre></blockquote>
  }
  style_footer();
}

static void multi_column_list(const char **azWord, int nWord){
  int i, j, len;
  int mxLen = 0;
  int nCol;
  int nRow;
  for(i=0; i<nWord; i++){
    len = strlen(azWord[i]);
    if( len>mxLen ) mxLen = len;
  }
  nCol = 80/(mxLen+2);
  if( nCol==0 ) nCol = 1;
  nRow = (nWord + nCol - 1)/nCol;
  for(i=0; i<nRow; i++){
    const char *zSpacer = "";
    for(j=i; j<nWord; j+=nRow){
      fossil_print("%s%-*s", zSpacer, mxLen, azWord[j]);
      zSpacer = "  ";
    }
    fossil_print("\n");
  }
}

/*
** COMMAND: test-list-webpage
**
** List all web pages.
*/
void cmd_test_webpage_list(void){
  int i, nCmd;
  const char *aCmd[MX_COMMAND];
  for(i=nCmd=0; i<MX_COMMAND; i++){
    if(0x08 & aCommand[i].eCmdFlags){
      aCmd[nCmd++] = aCommand[i].zName;
    }
  }
  assert(nCmd && "page list is empty?");
  multi_column_list(aCmd, nCmd);
}


/*
** List of commands starting with zPrefix, or all commands if zPrefix is NULL.
*/
static void command_list(const char *zPrefix, int cmdMask){
  int i, nCmd;
  int nPrefix = zPrefix ? strlen(zPrefix) : 0;
  const char *aCmd[MX_COMMAND];
  for(i=nCmd=0; i<MX_COMMAND; i++){
    const char *z = aCommand[i].zName;
    if( (aCommand[i].eCmdFlags & cmdMask)==0 ) continue;
    if( zPrefix && memcmp(zPrefix, z, nPrefix)!=0 ) continue;
    aCmd[nCmd++] = aCommand[i].zName;
  }
  multi_column_list(aCmd, nCmd);
}

/*
** COMMAND: help
**
** Usage: %fossil help COMMAND
**    or: %fossil COMMAND --help
**
** Display information on how to use COMMAND.  To display a list of
** available commands use one of:
**
**    %fossil help              Show common commands
**    %fossil help -a|--all     Show both common and auxiliary commands
**    %fossil help -t|--test    Show test commands only
**    %fossil help -x|--aux     Show auxiliary commands only
**    %fossil help -w|--www     Show list of WWW pages
*/
void help_cmd(void){
  int rc;
  int isPage = 0;
  const char *z;
  const char *zCmdOrPage;
  const char *zCmdOrPagePlural;
  const CmdOrPage *pCmd = 0;
  if( g.argc<3 ){
    z = g.argv[0];
    fossil_print(
      "Usage: %s help COMMAND\n"
      "Common COMMANDs:  (use \"%s help -a|--all\" for a complete list)\n",
      z, z);
    command_list(0, CMDFLAG_1ST_TIER);
    version_cmd();
    return;
  }
  if( find_option("all","a",0) ){
    command_list(0, CMDFLAG_1ST_TIER | CMDFLAG_2ND_TIER);
    return;
  }
  else if( find_option("www","w",0) ){
    command_list(0, CMDFLAG_WEBPAGE);
    return;
  }
  else if( find_option("aux","x",0) ){
    command_list(0, CMDFLAG_2ND_TIER);
    return;
  }
  else if( find_option("test","t",0) ){
    command_list(0, CMDFLAG_TEST);
    return;
  }
  isPage = ('/' == *g.argv[2]) ? 1 : 0;
  if(isPage){
    zCmdOrPage = "page";
    zCmdOrPagePlural = "pages";
  }else{
    zCmdOrPage = "command";
    zCmdOrPagePlural = "commands";
  }
  rc = dispatch_name_search(g.argv[2], CMDFLAG_ANY|CMDFLAG_PREFIX, &pCmd);
  if( rc==1 ){
    fossil_print("unknown %s: %s\nAvailable %s:\n",
                 zCmdOrPage, g.argv[2], zCmdOrPagePlural);
    command_list(0, isPage ? CMDFLAG_WEBPAGE : (0xff & ~CMDFLAG_WEBPAGE));
    fossil_exit(1);
  }else if( rc==2 ){
    fossil_print("ambiguous %s prefix: %s\nMatching %s:\n",
                 zCmdOrPage, g.argv[2], zCmdOrPagePlural);
    command_list(g.argv[2], 0xff);
    fossil_exit(1);
  }
  z = pCmd->zHelp;
  if( z==0 ){
    fossil_fatal("no help available for the %s %s",
                 pCmd->zName, zCmdOrPage);
  }
  while( *z ){
    if( *z=='%' && strncmp(z, "%fossil", 7)==0 ){
      fossil_print("%s", g.argv[0]);
      z += 7;
    }else{
      putchar(*z);
      z++;
    }
  }
  putchar('\n');
}