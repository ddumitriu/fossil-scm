/*
** Copyright (c) 2007 D. Richard Hipp
**
** This program is free software; you can redistribute it and/or
** modify it under the terms of the GNU General Public
** License version 2 as published by the Free Software Foundation.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
** General Public License for more details.
** 
** You should have received a copy of the GNU General Public
** License along with this library; if not, write to the
** Free Software Foundation, Inc., 59 Temple Place - Suite 330,
** Boston, MA  02111-1307, USA.
**
** Author contact information:
**   drh@hwaci.com
**   http://www.hwaci.com/drh/
**
*******************************************************************************
**
** This file contains code for parsing URLs that appear on the command-line
*/
#include "config.h"
#include "url.h"

/*
** Parse the given URL.  Populate variables in the global "g" structure.
**
**      g.urlIsFile      True if this is a file URL
**      g.urlName        Hostname for HTTP:.  Filename for FILE:
**      g.urlPort        Port name for HTTP.
**      g.urlPath        Path name for HTTP.
**      g.urlUser        Userid.
**      g.urlPasswd      Password.
**      g.urlCanonical   The URL in canonical form
**
** HTTP url format is:
**
**     http://userid:password@host:port/path?query#fragment
**
*/
void url_parse(const char *zUrl){
  int i, j, c;
  char *zFile = 0;
  if( strncmp(zUrl, "http:", 5)==0 ){
    g.urlIsFile = 0;
    for(i=7; (c=zUrl[i])!=0 && c!='/' && c!='@'; i++){}
    if( c=='@' ){
      for(j=7; j<i && zUrl[j]!=':'; j++){}
      g.urlUser = mprintf("%.*s", j-7, &zUrl[7]);
      if( j<i ){
        g.urlPasswd = mprintf("%.*s", i-j-1, &zUrl[j+1]);
      }
      for(j=i+1; (c=zUrl[j])!=0 && c!='/' && c!=':'; j++){}
      g.urlName = mprintf("%.*s", j-i-1, &zUrl[i+1]);
      i = j;
    }else{
      for(i=7; (c=zUrl[i])!=0 && c!='/' && c!=':'; i++){}
      g.urlName = mprintf("%.*s", i-7, &zUrl[7]);
    }
    for(j=0; g.urlName[j]; j++){ g.urlName[j] = tolower(g.urlName[j]); }
    if( c==':' ){
      g.urlPort = 0;
      i++;
      while( (c = zUrl[i])!=0 && isdigit(c) ){
        g.urlPort = g.urlPort*10 + c - '0';
        i++;
      }
    }else{
      g.urlPort = 80;
    }
    g.urlPath = mprintf(&zUrl[i]);
    dehttpize(g.urlName);
    dehttpize(g.urlPath);
    g.urlCanonical = mprintf("http://%T:%d%T", g.urlName, g.urlPort, g.urlPath);
  }else if( strncmp(zUrl, "file:", 5)==0 ){
    g.urlIsFile = 1;
    if( zUrl[5]=='/' && zUrl[6]=='/' ){
      i = 7;
    }else{
      i = 5;
    }
    zFile = mprintf("%s", &zUrl[i]);
  }else if( file_isfile(zUrl) ){
    g.urlIsFile = 1;
    zFile = mprintf("%s", zUrl);
  }else if( file_isdir(zUrl)==1 ){
    zFile = mprintf("%s/FOSSIL", zUrl);
    if( file_isfile(zFile) ){
      g.urlIsFile = 1;
    }else{
      free(zFile);
      fossil_panic("unknown repository: %s", zUrl);
    }
  }else{
    fossil_panic("unknown repository: %s", zUrl);
  }
  if( g.urlIsFile ){
    Blob cfile;
    dehttpize(zFile);  
    file_canonical_name(zFile, &cfile);
    free(zFile);
    g.urlName = mprintf("%b", &cfile);
    g.urlCanonical = mprintf("file://%T", g.urlName);
    blob_reset(&cfile);
  }
}

/*
** COMMAND: test-urlparser
*/
void cmd_test_urlparser(void){
  if( g.argc!=3 && g.argc!=4 ){
    usage("URL");
  }
  url_parse(g.argv[2]);
  printf("g.urlIsFile    = %d\n", g.urlIsFile);
  printf("g.urlName      = %s\n", g.urlName);
  printf("g.urlPort      = %d\n", g.urlPort);
  printf("g.urlPath      = %s\n", g.urlPath);
  printf("g.urlUser      = %s\n", g.urlUser);
  printf("g.urlPasswd    = %s\n", g.urlPasswd);
  printf("g.urlCanonical = %s\n", g.urlCanonical);
}

/*
** If the "proxy" setting is defined, then change the URL to refer
** to the proxy server.
*/
void url_enable_proxy(const char *zMsg){
  const char *zProxy = db_get("proxy", 0);
  if( zProxy && zProxy[0] && !is_false(zProxy) ){
    char *zOriginalUrl = g.urlCanonical;
    if( zMsg ) printf("%s%s\n", zMsg, zProxy);
    url_parse(zProxy);
    g.urlPath = zOriginalUrl;
  }
}
