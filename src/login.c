/*
** Copyright (c) 2007 D. Richard Hipp
**
** This program is free software; you can redistribute it and/or
** modify it under the terms of the GNU General Public
** License as published by the Free Software Foundation; either
** version 2 of the License, or (at your option) any later version.
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
** This file contains code for generating the login and logout screens.
**
** Notes:
**
** There are two special-case user-ids: "anonymous" and "nobody".
** The capabilities of the nobody user are available to anyone,
** regardless of whether or not they are logged in.  The capabilities
** of anonymous are only available after logging in, but the login
** screen displays the password for the anonymous login, so this
** should not prevent a human user from doing so.
**
** The nobody user has capabilities that you want spiders to have.
** The anonymous user has capabilities that you want people without
** logins to have.
**
** Of course, a sophisticated spider could easily circumvent the
** anonymous login requirement and walk the website.  But that is
** not really the point.  The anonymous login keeps search-engine
** crawlers and site download tools like wget from walking change
** logs and downloading diffs of very version of the archive that
** has ever existed, and things like that.
*/
#include "config.h"
#include "login.h"
#ifdef __MINGW32__
#  include <windows.h>           /* for Sleep */
#  define sleep Sleep            /* windows does not have sleep, but Sleep */
#endif
#include <time.h>

/*
** Return the name of the login cookie
*/
static char *login_cookie_name(void){
  static char *zCookieName = 0;
  if( zCookieName==0 ){
    int n = strlen(g.zTop);
    zCookieName = malloc( n*2+16 );
                      /* 0123456789 12345 */
    strcpy(zCookieName, "fossil_login_");
    encode16((unsigned char*)g.zTop, (unsigned char*)&zCookieName[13], n);
  }
  return zCookieName;
}

/*
** Redirect to the page specified by the "g" query parameter.
** Or if there is no "g" query parameter, redirect to the homepage.
*/
static void redirect_to_g(void){
  const char *zGoto = P("g");
  if( zGoto ){
    cgi_redirect(zGoto);
  }else{
    fossil_redirect_home();
  }
}

/*
** WEBPAGE: /login
** WEBPAGE: /logout
**
** Generate the login page
*/
void login_page(void){
  const char *zUsername, *zPasswd;
  const char *zNew1, *zNew2;
  const char *zAnonPw = 0;
  int anonFlag;
  char *zErrMsg = "";

  login_check_credentials();
  zUsername = P("u");
  zPasswd = P("p");
  anonFlag = P("anon")!=0;
  if( P("out")!=0 ){
    const char *zCookieName = login_cookie_name();
    cgi_set_cookie(zCookieName, "", 0, -86400);
    redirect_to_g();
  }
  if( g.okPassword && zPasswd && (zNew1 = P("n1"))!=0 && (zNew2 = P("n2"))!=0 ){
    if( db_int(1, "SELECT 0 FROM user"
                  " WHERE uid=%d AND pw=%Q", g.userUid, zPasswd) ){
      sleep(1);
      zErrMsg = 
         @ <p><font color="red">
         @ You entered an incorrect old password while attempting to change
         @ your password.  Your password is unchanged.
         @ </font></p>
      ;
    }else if( strcmp(zNew1,zNew2)!=0 ){
      zErrMsg = 
         @ <p><font color="red">
         @ The two copies of your new passwords do not match.
         @ Your password is unchanged.
         @ </font></p>
      ;
    }else{
      db_multi_exec(
         "UPDATE user SET pw=%Q WHERE uid=%d", zNew1, g.userUid
      );
      redirect_to_g();
      return;
    }
  }
  if( zUsername!=0 && zPasswd!=0 && zPasswd[0]!=0 ){
    int uid = db_int(0,
        "SELECT uid FROM user"
        " WHERE login=%Q AND pw=%Q", zUsername, zPasswd);
    if( uid<=0 || strcmp(zUsername,"nobody")==0 ){
      sleep(1);
      zErrMsg = 
         @ <p><font color="red">
         @ You entered an unknown user or an incorrect password.
         @ </font></p>
      ;
    }else{
      char *zCookie;
      const char *zCookieName = login_cookie_name();
      const char *zExpire = db_get("cookie-expire","8766");
      int expires = atoi(zExpire)*3600;
      const char *zIpAddr = PD("REMOTE_ADDR","nil");
 
      if( strcmp(zUsername, "anonymous")==0 ){
        cgi_set_cookie(zCookieName, "anonymous", 0, expires);
      }else{
        zCookie = db_text(0, "SELECT '%d/' || hex(randomblob(25))", uid);
        cgi_set_cookie(zCookieName, zCookie, 0, expires);
        db_multi_exec(
          "UPDATE user SET cookie=%Q, ipaddr=%Q, "
          "  cexpire=julianday('now')+%d/86400.0 WHERE uid=%d",
          zCookie, zIpAddr, expires, uid
        );
      }
      redirect_to_g();
    }
  }
  style_header("Login/Logout");
  @ %s(zErrMsg)
  @ <form action="login" method="POST">
  if( P("g") ){
    @ <input type="hidden" name="g" value="%h(P("g"))">
  }
  @ <table align="left" hspace="10">
  @ <tr>
  @   <td align="right">User ID:</td>
  if( anonFlag ){
    @   <td><input type="text" name="u" value="anonymous" size=30></td>
  }else{
    @   <td><input type="text" name="u" value="" size=30></td>
  }
  @ </tr>
  @ <tr>
  @  <td align="right">Password:</td>
  @   <td><input type="password" name="p" value="" size=30></td>
  @ </tr>
  if( g.zLogin==0 ){
    zAnonPw = db_text(0, "SELECT pw FROM user"
                         " WHERE login='anonymous'"
                         "   AND cap!=''");
    if( zAnonPw && anonFlag ){
      @ <tr><td></td>
      @ <td>The anonymous password is "<b>%h(zAnonPw)</b>".</td>
      @ </tr>
    }
  }
  @ <tr>
  @   <td></td>
  @   <td><input type="submit" name="in" value="Login"></td>
  @ </tr>
  @ </table>
  if( g.zLogin==0 ){
    @ <p>To login
  }else{
    @ <p>You are currently logged in as <b>%h(g.zLogin)</b></p>
    @ <p>To change your login to a different user
  }
  @ enter the user-id and password at the left and press the
  @ "Login" button.  Your user name will be stored in a browser cookie.
  @ You must configure your web browser to accept cookies in order for
  @ the login to take.</p>
  if( g.zLogin==0 ){
    if( zAnonPw && !anonFlag ){
      @ <p>The password for user "anonymous" is "<b>%h(zAnonPw)</b>".</p>
      @ <p>&nbsp;</p>
    }else{
      @ <p>&nbsp;</p><p>&nbsp;</p>
    }
  }
  if( g.zLogin ){
    @ <br clear="both"><hr>
    @ <p>To log off the system (and delete your login cookie)
    @  press the following button:<br>
    @ <input type="submit" name="out" value="Logout"></p>
  }
  @ </form>
  if( g.okPassword ){
    @ <br clear="both"><hr>
    @ <p>To change your password, enter your old password and your
    @ new password twice below then press the "Change Password"
    @ button.</p>
    @ <form action="login" method="POST">
    @ <table>
    @ <tr><td align="right">Old Password:</td>
    @ <td><input type="password" name="p" size=30></td></tr>
    @ <tr><td align="right">New Password:</td>
    @ <td><input type="password" name="n1" size=30></td></tr>
    @ <tr><td align="right">Repeat New Password:</td>
    @ <td><input type="password" name="n2" size=30></td></tr>
    @ <tr><td></td>
    @ <td><input type="submit" value="Change Password"></td></tr>
    @ </table>
    @ </form>
  }
  style_footer();
}



/*
** This routine examines the login cookie to see if it exists and
** and is valid.  If the login cookie checks out, it then sets 
** g.zUserUuid appropriately.
**
*/
void login_check_credentials(void){
  int uid = 0;                  /* User id */
  const char *zCookie;          /* Text of the login cookie */
  const char *zRemoteAddr;      /* IP address of the requestor */
  const char *zCap = 0;         /* Capability string */
  const char *zNcap;            /* Capabilities of user "nobody" */
  const char *zAcap;            /* Capabllities of user "anonymous" */

  /* Only run this check once.  */
  if( g.userUid!=0 ) return;


  /* If the HTTP connection is coming over 127.0.0.1 and if
  ** local login is disabled and if we are using HTTP and not HTTPS, 
  ** then there is no need to check user credentials.
  **
  */
  zRemoteAddr = PD("REMOTE_ADDR","nil");
  if( strcmp(zRemoteAddr, "127.0.0.1")==0
   && db_get_int("localauth",0)==0
   && P("HTTPS")==0
  ){
    uid = db_int(0, "SELECT uid FROM user WHERE cap LIKE '%%s%%'");
    g.zLogin = db_text("?", "SELECT login FROM user WHERE uid=%d", uid);
    zCap = "s";
    g.noPswd = 1;
    strcpy(g.zCsrfToken, "localhost");
  }

  /* Check the login cookie to see if it matches a known valid user.
  */
  if( uid==0 && (zCookie = P(login_cookie_name()))!=0 ){
    if( isdigit(zCookie[0]) ){
      uid = db_int(0, 
            "SELECT uid FROM user"
            " WHERE uid=%d"
            "   AND cookie=%Q"
            "   AND ipaddr=%Q"
            "   AND cexpire>julianday('now')",
            atoi(zCookie), zCookie, zRemoteAddr
         );
    }else if( zCookie[0]=='a' ){
      uid = db_int(0, "SELECT uid FROM user WHERE login='anonymous'");
    }
    sqlite3_snprintf(sizeof(g.zCsrfToken), g.zCsrfToken, "%.10s", zCookie);
  }

  if( uid==0 ){
    uid = db_int(0, "SELECT uid FROM user WHERE login='nobody'");
    if( uid==0 ){
      uid = -1;
      zCap = "";
    }
    strcpy(g.zCsrfToken, "none");
  }
  if( zCap==0 ){
    if( uid ){
      Stmt s;
      db_prepare(&s, "SELECT login, cap FROM user WHERE uid=%d", uid);
      if( db_step(&s)==SQLITE_ROW ){
        g.zLogin = db_column_malloc(&s, 0);
        zCap = db_column_malloc(&s, 1);
      }
      db_finalize(&s);
    }
    if( zCap==0 ){
      zCap = "";
    }
  }
  g.userUid = uid;
  if( g.zLogin && strcmp(g.zLogin,"nobody")==0 ){
    g.zLogin = 0;
  }
  if( uid && g.zLogin ){
    /* All logged-in users inherit privileges from "nobody" */
    zNcap = db_text("", "SELECT cap FROM user WHERE login = 'nobody'");
    login_set_capabilities(zNcap);
    if( strcmp(g.zLogin, "anonymous")!=0 ){
      /* All logged-in users inherit privileges from "anonymous" */
      zAcap = db_text("", "SELECT cap FROM user WHERE login = 'anonymous'");
      login_set_capabilities(zAcap);
    }
  }
  login_set_capabilities(zCap);
}

/*
** Set the global capability flags based on a capability string.
*/
void login_set_capabilities(const char *zCap){
  static char *zDev = 0;
  int i;
  for(i=0; zCap[i]; i++){
    switch( zCap[i] ){
      case 's':   g.okSetup = 1;  /* Fall thru into Admin */
      case 'a':   g.okAdmin = g.okRdTkt = g.okWrTkt = 
                              g.okRdWiki = g.okWrWiki = g.okNewWiki =
                              g.okApndWiki = g.okHistory = g.okClone = 
                              g.okNewTkt = g.okPassword = g.okRdAddr =
                              g.okTktFmt = 1;  /* Fall thru into Read/Write */
      case 'i':   g.okRead = g.okWrite = 1;                     break;
      case 'o':   g.okRead = 1;                                 break;
      case 'z':   g.okZip = 1;                                  break;

      case 'd':   g.okDelete = 1;                               break;
      case 'h':   g.okHistory = 1;                              break;
      case 'g':   g.okClone = 1;                                break;
      case 'p':   g.okPassword = 1;                             break;

      case 'j':   g.okRdWiki = 1;                               break;
      case 'k':   g.okWrWiki = g.okRdWiki = g.okApndWiki =1;    break;
      case 'm':   g.okApndWiki = 1;                             break;
      case 'f':   g.okNewWiki = 1;                              break;

      case 'e':   g.okRdAddr = 1;                               break;
      case 'r':   g.okRdTkt = 1;                                break;
      case 'n':   g.okNewTkt = 1;                               break;
      case 'w':   g.okWrTkt = g.okRdTkt = g.okNewTkt = 
                  g.okApndTkt = 1;                              break;
      case 'c':   g.okApndTkt = 1;                              break;
      case 't':   g.okTktFmt = 1;                               break;

      /* The "v" privileges is a little different.  It recursively 
      ** inherits all privileges of the user named "developer" */
      case 'v': {
        if( zDev==0 ){
          zDev = db_text("", "SELECT cap FROM user WHERE login='developer'");
          login_set_capabilities(zDev);
        }
        break;
      }
    }
  }
}

/*
** If the current login lacks any of the capabilities listed in
** the input, then return 0.  If all capabilities are present, then
** return 1.
*/
int login_has_capability(const char *zCap, int nCap){
  int i;
  int rc = 1;
  if( nCap<0 ) nCap = strlen(zCap);
  for(i=0; i<nCap && rc && zCap[i]; i++){
    switch( zCap[i] ){
      case 'a':  rc = g.okAdmin;     break;
      /* case 'b': */
      case 'c':  rc = g.okApndTkt;   break;
      case 'd':  rc = g.okDelete;    break;
      case 'e':  rc = g.okRdAddr;    break;
      case 'f':  rc = g.okNewWiki;   break;
      case 'g':  rc = g.okClone;     break;
      case 'h':  rc = g.okHistory;   break;
      case 'i':  rc = g.okWrite;     break;
      case 'j':  rc = g.okRdWiki;    break;
      case 'k':  rc = g.okWrWiki;    break;
      /* case 'l': */
      case 'm':  rc = g.okApndWiki;  break;
      case 'n':  rc = g.okNewTkt;    break;
      case 'o':  rc = g.okRead;      break;
      case 'p':  rc = g.okPassword;  break;
      /* case 'q': */
      case 'r':  rc = g.okRdTkt;     break;
      case 's':  rc = g.okSetup;     break;
      case 't':  rc = g.okTktFmt;    break;
      /* case 'u': */
      /* case 'v': */
      case 'w':  rc = g.okWrTkt;     break;
      /* case 'x': */
      /* case 'y': */
      case 'z':  rc = g.okZip;       break;
      default:   rc = 0;             break;
    }
  }
  return rc;
}

/*
** Call this routine when the credential check fails.  It causes
** a redirect to the "login" page.
*/
void login_needed(void){
  const char *zUrl = PD("REQUEST_URI", "index");
  cgi_redirect(mprintf("login?g=%T", zUrl));
  /* NOTREACHED */
  assert(0);
}

/*
** Call this routine if the user lacks okHistory permission.  If
** the anonymous user has okHistory permission, then paint a mesage
** to inform the user that much more information is available by
** logging in as anonymous.
*/
void login_anonymous_available(void){
  if( !g.okHistory &&
      db_exists("SELECT 1 FROM user"
                " WHERE login='anonymous'"
                "   AND cap LIKE '%%h%%'") ){
    const char *zUrl = PD("REQUEST_URI", "index");
    @ <p>Many <font color="red">hyperlinks are disabled.</font><br />
    @ Use <a href="%s(g.zTop)/login?anon=1&g=%T(zUrl)">anonymous login</a>
    @ to enable hyperlinks.</p>
  }
}

/*
** While rendering a form, call this routine to add the Anti-CSRF token
** as a hidden element of the form.
*/
void login_insert_csrf_secret(void){
  @ <input type="hidden" name="csrf" value="%s(g.zCsrfToken)">
}

/*
** Before using the results of a form, first call this routine to verify
** that ths Anti-CSRF token is present and is valid.  If the Anti-CSRF token
** is missing or is incorrect, then this emits and error message and never
** returns.
*/
void login_verify_csrf_secret(void){
  const char *zCsrf;            /* The CSRF secret */
  if( g.okCsrf ) return;
  if( (zCsrf = P("csrf"))!=0 && strcmp(zCsrf, g.zCsrfToken)==0 ){
    g.okCsrf = 1;
    return;
  }
  fossil_fatal("Cross-site request forgery attempt");
}
