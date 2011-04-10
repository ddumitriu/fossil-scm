/*
** Copyright (c) 2009 D. Richard Hipp
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
** This file manages low-level SSL communications.
**
** This file implements a singleton.  A single SSL connection may be active
** at a time.  State information is stored in static variables.  The identity
** of the server is held in global variables that are set by url_parse().
**
** SSL support is abstracted out into this module because Fossil can
** be compiled without SSL support (which requires OpenSSL library)
*/


#ifdef FOSSIL_ENABLE_SSL
#include <openssl/bio.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <assert.h>
#include <sys/types.h>
#endif

#include "config.h"
#include "http_ssl.h"

/*
** Make sure the CERT table exists in the ~/.fossil database.
**
** This routine must be called in between two calls to db_swap_databases().
*/
static void create_cert_table_if_not_exist(void){
  static const char zSql[] = 
     @ CREATE TABLE IF NOT EXISTS certs(
     @   name TEXT NOT NULL,
     @   type TEXT NOT NULL,
     @   filepath TEXT NOT NULL,
     @   PRIMARY KEY(name, type)
     @ );
     ;
  db_multi_exec(zSql);    
}

#ifdef FOSSIL_ENABLE_SSL

/*
** There can only be a single OpenSSL IO connection open at a time.
** State information about that IO is stored in the following
** local variables:
*/
static int sslIsInit = 0;    /* True after global initialization */
static BIO *iBio;            /* OpenSSL I/O abstraction */
static char *sslErrMsg = 0;  /* Text of most recent OpenSSL error */
static SSL_CTX *sslCtx;      /* SSL context */
static SSL *ssl;
static char *pempasswd = 0;  /* Passphrase used to unlock key */


/*
** Clear the SSL error message
*/
static void ssl_clear_errmsg(void){
  free(sslErrMsg);
  sslErrMsg = 0;
}

/*
** Set the SSL error message.
*/
void ssl_set_errmsg(char *zFormat, ...){
  va_list ap;
  ssl_clear_errmsg();
  va_start(ap, zFormat);
  sslErrMsg = vmprintf(zFormat, ap);
  va_end(ap);
}

/*
** Return the current SSL error message
*/
const char *ssl_errmsg(void){
  return sslErrMsg;
}

/*
** Called by SSL when a passphrase protected file needs to be unlocked.
** We cache the passphrase so the user doesn't have to re-enter it for each new
** connection.
*/
static int ssl_passwd_cb(char *buf, int size, int rwflag, void *userdata){
  if( userdata==0 ){
    Blob passwd;
    prompt_for_password("\nPEM unlock passphrase: ", &passwd, 0);
    strncpy(buf, (char *)blob_str(&passwd), size);
    buf[size-1] = '\0';
    blob_reset(&passwd);
    pempasswd = strdup(buf);
    if( !pempasswd ){
      fossil_panic("Unable to allocate memory for PEM passphrase.");
    }
    SSL_CTX_set_default_passwd_cb_userdata(sslCtx, pempasswd);
  }else{
    strncpy(buf, (char *)userdata, size);
  }

  return strlen(buf);
}

/*
** Call this routine once before any other use of the SSL interface.
** This routine does initial configuration of the SSL module.
*/
void ssl_global_init(void){
  if( sslIsInit==0 ){
    SSL_library_init();
    SSL_load_error_strings();
    ERR_load_BIO_strings();
    OpenSSL_add_all_algorithms();    
    sslCtx = SSL_CTX_new(SSLv23_client_method());
    X509_STORE_set_default_paths(SSL_CTX_get_cert_store(sslCtx));
    SSL_CTX_set_default_passwd_cb(sslCtx, ssl_passwd_cb);
    SSL_CTX_set_default_passwd_cb_userdata(sslCtx, NULL);
    sslIsInit = 1;
  }
}

/*
** Call this routine to shutdown the SSL module prior to program exit.
*/
void ssl_global_shutdown(void){
  if( sslIsInit ){
    SSL_CTX_free(sslCtx);
    ssl_clear_errmsg();
    sslIsInit = 0;
  }
}

/*
** Close the currently open SSL connection.  If no connection is open, 
** this routine is a no-op.
*/
void ssl_close(void){
  if( iBio!=NULL ){
    (void)BIO_reset(iBio);
    BIO_free_all(iBio);
  }
}

/*
** Open an SSL connection.  The identify of the server is determined
** by global varibles that are set using url_parse():
**
**    g.urlName       Name of the server.  Ex: www.fossil-scm.org
**    g.urlPort       TCP/IP port to use.  Ex: 80
**
** Return the number of errors.
*/
int ssl_open(void){
  X509 *cert;
  int hasSavedCertificate = 0;
  char *connStr;
  ssl_global_init();

  /* If client certificate/key has been set, load them into the SSL context. */
  ssl_load_client_authfiles();

  /* Get certificate for current server from global config and
  ** (if we have it in config) add it to certificate store.
  */
  cert = ssl_get_certificate();
  if ( cert!=NULL ){
    X509_STORE_add_cert(SSL_CTX_get_cert_store(sslCtx), cert);
    X509_free(cert);
    hasSavedCertificate = 1;
  }

  iBio = BIO_new_ssl_connect(sslCtx);
  BIO_get_ssl(iBio, &ssl);
  SSL_set_mode(ssl, SSL_MODE_AUTO_RETRY);
  if( iBio==NULL ){
    ssl_set_errmsg("SSL: cannot open SSL (%s)", 
                    ERR_reason_error_string(ERR_get_error()));
    return 1;
  }
  
  connStr = mprintf("%s:%d", g.urlName, g.urlPort);
  BIO_set_conn_hostname(iBio, connStr);
  free(connStr);
  
  if( BIO_do_connect(iBio)<=0 ){
    ssl_set_errmsg("SSL: cannot connect to host %s:%d (%s)", 
        g.urlName, g.urlPort, ERR_reason_error_string(ERR_get_error()));
    ssl_close();
    return 1;
  }
  
  if( BIO_do_handshake(iBio)<=0 ) {
    ssl_set_errmsg("Error establishing SSL connection %s:%d (%s)", 
        g.urlName, g.urlPort, ERR_reason_error_string(ERR_get_error()));
    ssl_close();
    return 1;
  }
  /* Check if certificate is valid */
  cert = SSL_get_peer_certificate(ssl);

  if ( cert==NULL ){
    ssl_set_errmsg("No SSL certificate was presented by the peer");
    ssl_close();
    return 1;
  }

  if( SSL_get_verify_result(ssl) != X509_V_OK ){
    char *desc, *prompt;
    char *warning = "";
    Blob ans;
    BIO *mem;
    
    mem = BIO_new(BIO_s_mem());
    X509_NAME_print_ex(mem, X509_get_subject_name(cert), 2, XN_FLAG_MULTILINE);
    BIO_puts(mem, "\n\nIssued By:\n\n");
    X509_NAME_print_ex(mem, X509_get_issuer_name(cert), 2, XN_FLAG_MULTILINE);
    BIO_write(mem, "", 1); // null-terminate mem buffer
    BIO_get_mem_data(mem, &desc);
    
    if( hasSavedCertificate ){
      warning = "WARNING: Certificate doesn't match the "
                "saved certificate for this host!";
    }
    prompt = mprintf("\nUnknown SSL certificate:\n\n%s\n\n%s\n"
                     "Accept certificate [a=always/y/N]? ", desc, warning);
    BIO_free(mem);

    prompt_user(prompt, &ans);
    free(prompt);
    if( blob_str(&ans)[0]!='y' && blob_str(&ans)[0]!='a' ) {
      X509_free(cert);
      ssl_set_errmsg("SSL certificate declined");
      ssl_close();
      return 1;
    }
    if( blob_str(&ans)[0]=='a' ) {
      ssl_save_certificate(cert);
    }
    blob_reset(&ans);
  }
  X509_free(cert);
  return 0;
}

/*
** Save certificate to global certificate/key store.
*/
void ssl_save_certificate(X509 *cert){
  BIO *mem;
  char *zCert;

  mem = BIO_new(BIO_s_mem());
  PEM_write_bio_X509(mem, cert);
  BIO_write(mem, "", 1); // null-terminate mem buffer
  BIO_get_mem_data(mem, &zCert);
  db_swap_connections();
  create_cert_table_if_not_exist();
  db_begin_transaction();
  db_multi_exec("REPLACE INTO certs(name,type,filepath) "
      "VALUES(%Q,'scert',%Q)", g.urlName, zCert);
  db_end_transaction(0);
  db_swap_connections();
  BIO_free(mem);  
}

/*
** Get certificate for g.urlName from global certificate/key store.
** Return NULL if no certificate found.
*/
X509 *ssl_get_certificate(void){
  char *zCert;
  BIO *mem;
  X509 *cert;

  db_swap_connections();
  create_cert_table_if_not_exist();
  zCert = db_text(0, "SELECT filepath FROM certs WHERE name=%Q"
                     " AND type='scert'", g.urlName);
  db_swap_connections();
  if( zCert==NULL )
    return NULL;
  mem = BIO_new(BIO_s_mem());
  BIO_puts(mem, zCert);
  cert = PEM_read_bio_X509(mem, NULL, 0, NULL);
  free(zCert);
  BIO_free(mem);  
  return cert;
}

/*
** Send content out over the SSL connection.
*/
size_t ssl_send(void *NotUsed, void *pContent, size_t N){
  size_t sent;
  size_t total = 0;
  while( N>0 ){
    sent = BIO_write(iBio, pContent, N);
    if( sent<=0 ) break;
    total += sent;
    N -= sent;
    pContent = (void*)&((char*)pContent)[sent];
  }
  return total;
}

/*
** Receive content back from the SSL connection.
*/
size_t ssl_receive(void *NotUsed, void *pContent, size_t N){
  size_t got;
  size_t total = 0;
  while( N>0 ){
    got = BIO_read(iBio, pContent, N);
    if( got<=0 ) break;
    total += got;
    N -= got;
    pContent = (void*)&((char*)pContent)[got];
  }
  return total;
}

/*
** If a certbundle has been specified on the command line, then use it to look
** up certificates and keys, and then store the URL-certbundle association in
** the global database. If no certbundle has been specified on the command
** line, see if there's an entry for the url in global_config, and use it if
** applicable.
*/
void ssl_load_client_authfiles(void){
  char *zBundleName = NULL;
  char *cafile;
  char *capath;
  char *certfile;
  char *keyfile;

  if( g.urlCertBundle ){
    char *zName;
    zName = mprintf("certbundle:%s", g.urlName);
    db_set(zName, g.urlCertBundle, 1);
    free(zName);
    zBundleName = strdup(g.urlCertBundle);
  }else{
    db_swap_connections();
    zBundleName = db_text(0, "SELECT value FROM global_config"
                             " WHERE name='certbundle:%q'", g.urlName);
    db_swap_connections();
  }
  if( !zBundleName ){
    /* No cert bundle specified on command line or found cached for URL */
    return;
  }

  db_swap_connections();
  create_cert_table_if_not_exist();
  cafile = db_text(0, "SELECT filepath FROM certs WHERE name=%Q"
                      " AND type='cafile'", zBundleName);
  capath = db_text(0, "SELECT filepath FROM certs WHERE name=%Q"
                      " AND type='capath'", zBundleName);
  db_swap_connections();

  if( cafile || capath ){
    /* The OpenSSL documentation warns that if several CA certificates match
    ** the same name, key identifier and serial number conditions, only the
    ** first will be examined. The caveat situation occurs when one stores an
    ** expired CA certificate among the valid ones.
    ** Simply put: Do not mix expired and valid certificates.
    */
    if( SSL_CTX_load_verify_locations(sslCtx, cafile, capath)==0 ){
      fossil_fatal("SSL: Unable to load CA verification file/path");
    }
  }

  db_swap_connections();
  keyfile = db_text(0, "SELECT filepath FROM certs WHERE name=%Q"
                       " AND type='ckey'", zBundleName);
  certfile = db_text(0, "SELECT filepath FROM certs WHERE name=%Q"
                        " AND type='ccert'", zBundleName);
  db_swap_connections();

  if( certfile ){
    /* If a client certificate is explicitly specified, but a key is not, then
    ** assume the key is in the same file as the certificate.
    */
    if( !keyfile ){
      keyfile = certfile;
    }
    if( SSL_CTX_use_certificate_file(sslCtx, certfile, SSL_FILETYPE_PEM)<=0 ){
      fossil_fatal("SSL: Unable to open client certificate in %s.", certfile);
    }
    if( SSL_CTX_use_PrivateKey_file(sslCtx, keyfile, SSL_FILETYPE_PEM)<=0 ){
      fossil_fatal("SSL: Unable to open client key in %s.", keyfile);
    }
    if( certfile && keyfile && !SSL_CTX_check_private_key(sslCtx) ){
      fossil_fatal("SSL: Private key does not match the certificate public "
          "key.");
    }
  }

  if( keyfile != certfile ){
    free(keyfile);
  }
  free(certfile);
  free(capath);
  free(cafile);
}
#endif /* FOSSIL_ENABLE_SSL */


/*
** COMMAND: cert
**
** Usage: %fossil cert SUBCOMMAND ...
**
** Manage/bundle PKI client keys/certificates and CA certificates for SSL
** certificate chain verifications.
**
**    %fossil cert add NAME ?--key KEYFILE? ?--cert CERTFILE?
**           ?--cafile CAFILE? ?--capath CAPATH?
**
**        Create a certificate bundle NAME with the associated
**        certificates/keys. If a client certificate is specified but no
**        key, it is assumed that the key is located in the client
**        certificate file.
**        The file formats must be PEM.
**
**    %fossil cert list
**
**        List all certificate bundles, their values and their URL
**        associations.
**
**    %fossil cert disassociate URL
**
**        Disassociate URL from any certificate bundle.
**
**    %fossil cert delete NAME
**
**        Remove the certificate bundle NAME and all its URL associations.
**
*/
void cert_cmd(void){
  int n;
  const char *zCmd = "list";	/* Default sub-command */
  if( g.argc>=3 ){
    zCmd = g.argv[2];
  }
  n = strlen(zCmd);
  if( strncmp(zCmd, "add", n)==0 ){
    const char *zContainer;
    const char *zCKey;
    const char *zCCert;
    const char *zCAFile;
    const char *zCAPath;
    if( g.argc<5 ){
      usage("add NAME ?--key KEYFILE? ?--cert CERTFILE? ?--cafile CAFILE? "
          "?--capath CAPATH?");
    }
    zContainer = g.argv[3];
    zCKey = find_option("key",0,1);
    zCCert = find_option("cert",0,1);
    zCAFile = find_option("cafile",0,1);
    zCAPath = find_option("capath",0,1);

    /* If a client certificate was specified, but a key was not, assume the
    ** key is stored in the same file as the certificate.
    */
    if( !zCKey && zCCert ){
      zCKey = zCCert;
    }

    db_open_config(0);
    db_swap_connections();
    create_cert_table_if_not_exist();
    db_begin_transaction();
    if( db_exists("SELECT 1 FROM certs WHERE name='%q'", zContainer)!=0 ){
      db_end_transaction(0);
      fossil_fatal("certificate bundle \"%s\" already exists", zContainer);
    }
    if( zCKey ){
      db_multi_exec("INSERT INTO certs (name,type,filepath) "
          "VALUES(%Q,'ckey',%Q)",
          zContainer, zCKey);
    }
    if( zCCert ){
      db_multi_exec("INSERT INTO certs (name,type,filepath) "
          "VALUES(%Q,'ccert',%Q)",
          zContainer, zCCert);
    }
    if( zCAFile ){
      db_multi_exec("INSERT INTO certs (name,type,filepath) "
          "VALUES(%Q,'cafile',%Q)",
          zContainer, zCAFile);
    }
    if( zCAPath ){
      db_multi_exec("INSERT INTO certs (name,type,filepath) "
          "VALUES(%Q,'capath',%Q)",
          zContainer, zCAPath);
    }
    db_end_transaction(0);
    db_swap_connections();
  }else if(strncmp(zCmd, "list", n)==0){
    Stmt q;
    char *bndl = NULL;

    db_open_config(0);
    db_swap_connections();
    create_cert_table_if_not_exist();

    db_prepare(&q, "SELECT name,type,filepath FROM certs"
                   " WHERE type NOT IN ('server')"
                   " ORDER BY name,type");
    while( db_step(&q)==SQLITE_ROW ){
      const char *zCont = db_column_text(&q, 0);
      const char *zType = db_column_text(&q, 1);
      const char *zFilePath = db_column_text(&q, 2);
      if( fossil_strcmp(zCont, bndl)!=0 ){
        free(bndl);
        bndl = strdup(zCont);
        puts(zCont);
      }
      printf("\t%s=%s\n", zType, zFilePath);
    }
    db_finalize(&q);

    /* List the URL associations. */
    db_prepare(&q, "SELECT name FROM global_config"
                   " WHERE name LIKE 'certbundle:%%' AND value=%Q"
                   " ORDER BY name", bndl);
    free(bndl);

    while( db_step(&q)==SQLITE_ROW ){
      const char *zName = db_column_text(&q, 0);
      static int first = 1;
      if( first ) {
        puts("\tAssociations");
        first = 0;
      }
      printf("\t\t%s\n", zName+11);
    }

    db_swap_connections();
  }else if(strncmp(zCmd, "disassociate", n)==0){
    const char *zURL;
    if( g.argc<4 ){
      usage("disassociate URL");
    }
    zURL = g.argv[3];

    db_open_config(0);
    db_swap_connections();
    db_begin_transaction();
    db_multi_exec("DELETE FROM global_config WHERE name='certbundle:%q'",
        zURL);
    if( db_changes() == 0 ){
      fossil_warning("No certificate bundle associated with URL \"%s\".",
          zURL);
    }else{
      printf("%s disassociated from its certificate bundle.\n", zURL);
    }
    db_end_transaction(0);
    db_swap_connections();

  }else if(strncmp(zCmd, "delete", n)==0){
    const char *zContainer;
    if( g.argc<4 ){
      usage("delete NAME");
    }
    zContainer = g.argv[3];

    db_open_config(0);
    db_swap_connections();
    create_cert_table_if_not_exist();
    db_begin_transaction();
    db_multi_exec("DELETE FROM certs WHERE name=%Q", zContainer);
    if( db_changes() == 0 ){
      fossil_warning("No certificate bundle named \"%s\" found",
          zContainer);
    }else{
      printf("%d entries removed\n", db_changes());
    }
    db_multi_exec("DELETE FROM global_config WHERE name LIKE 'certbundle:%%'"
        " AND value=%Q", zContainer);
    if( db_changes() > 0 ){
      printf("%d associations removed\n", db_changes());
    }
    db_end_transaction(0);
    db_swap_connections();
  }else{
    fossil_panic("cert subcommand should be one of: "
                 "add list disassociate delete");
  }
}
