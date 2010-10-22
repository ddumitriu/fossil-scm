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

#include "config.h"

#ifdef FOSSIL_ENABLE_SSL

#include <openssl/bio.h>
#include <openssl/ssl.h>
#include <openssl/err.h>

#include "http_ssl.h"
#include <assert.h>
#include <sys/types.h>

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
  int vresult = 0;
  ssl_global_init();

  /* Get certificate for current server from global config and
   * (if we have it in config) add it to certificate store.
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
  if( iBio==NULL ) {
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

  if( (vresult = SSL_get_verify_result(ssl)) != X509_V_OK ){
    char *desc, *prompt;
    char *warning = "";
    char *ssl_verify_error = "";
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
    switch(vresult) {
      case X509_V_ERR_UNABLE_TO_GET_ISSUER_CERT:
        ssl_verify_error = "SSL: unable to get issuer certificate.";
        break;

      case X509_V_ERR_UNABLE_TO_GET_CRL:
        ssl_verify_error = "SSL: unable to get certificate CRL.";
        break;

      case X509_V_ERR_UNABLE_TO_DECRYPT_CERT_SIGNATURE:
        ssl_verify_error = "SSL: unable to decrypt certificate’s signature.";
        break;

      case X509_V_ERR_UNABLE_TO_DECRYPT_CRL_SIGNATURE:
        ssl_verify_error = "SSL: unable to decrypt CRL’s signature.";
        break;

      case X509_V_ERR_UNABLE_TO_DECODE_ISSUER_PUBLIC_KEY:
        ssl_verify_error = "SSL: unable to decode issuer public key.";
        break;

      case X509_V_ERR_CERT_SIGNATURE_FAILURE:
        ssl_verify_error = "SSL: certificate signature failure.";
        break;

      case X509_V_ERR_CRL_SIGNATURE_FAILURE:
        ssl_verify_error = "SSL: CRL signature failure.";
        break;

      case X509_V_ERR_CERT_NOT_YET_VALID:
        ssl_verify_error = "SSL: certificate is not yet valid.";
        break;

      case X509_V_ERR_CERT_HAS_EXPIRED:
        ssl_verify_error = "SSL: certificate has expired.";
        break;

      case X509_V_ERR_CRL_NOT_YET_VALID:
        ssl_verify_error = "SSL: CRL is not yet valid.";
        break;

      case X509_V_ERR_CRL_HAS_EXPIRED:
        ssl_verify_error = "SSL: CRL has expired.";
        break;

      case X509_V_ERR_ERROR_IN_CERT_NOT_BEFORE_FIELD:
        ssl_verify_error = "SSL: format error in certificate’s notBefore field.";
        break;

      case X509_V_ERR_ERROR_IN_CERT_NOT_AFTER_FIELD:
        ssl_verify_error = "SSL: format error in certificate’s notAfter field.";
        break;

      case X509_V_ERR_ERROR_IN_CRL_LAST_UPDATE_FIELD:
        ssl_verify_error = "SSL: format error in CRL’s lastUpdate field.";
        break;

      case X509_V_ERR_ERROR_IN_CRL_NEXT_UPDATE_FIELD:
        ssl_verify_error = "SSL: format error in CRL’s nextUpdate field.";
        break;

      case X509_V_ERR_OUT_OF_MEM:
        ssl_verify_error = "SSL: out of memory.";
        break;

      case X509_V_ERR_DEPTH_ZERO_SELF_SIGNED_CERT:
        ssl_verify_error = "SSL: self signed certificate.";
        break;

      case X509_V_ERR_SELF_SIGNED_CERT_IN_CHAIN:
        ssl_verify_error = "SSL: self signed certificate in certificate chain.";
        break;

      case X509_V_ERR_UNABLE_TO_GET_ISSUER_CERT_LOCALLY:
        ssl_verify_error = "SSL: unable to get local issuer certificate.";
        break;

      case X509_V_ERR_UNABLE_TO_VERIFY_LEAF_SIGNATURE:
        ssl_verify_error = "SSL: unable to verify the first certificate.";
        break;

      case X509_V_ERR_CERT_CHAIN_TOO_LONG:
        ssl_verify_error = "SSL: certificate chain too long.";
        break;

      case X509_V_ERR_CERT_REVOKED:
        ssl_verify_error = "SSL: certificate revoked.";
        break;

      case X509_V_ERR_INVALID_CA:
        ssl_verify_error = "SSL: invalid CA certificate.";
        break;

      case X509_V_ERR_PATH_LENGTH_EXCEEDED:
        ssl_verify_error = "SSL: path length constraint exceeded.";
        break;

      case X509_V_ERR_INVALID_PURPOSE:
        ssl_verify_error = "SSL: unsupported certificate purpose.";
        break;

      case X509_V_ERR_CERT_UNTRUSTED:
        ssl_verify_error = "SSL: certificate not trusted.";
        break;

      case X509_V_ERR_CERT_REJECTED:
        ssl_verify_error = "SSL: certificate rejected.";
        break;

      case X509_V_ERR_SUBJECT_ISSUER_MISMATCH:
        ssl_verify_error = "SSL: subject issuer mismatch.";
        break;

      case X509_V_ERR_AKID_SKID_MISMATCH:
        ssl_verify_error = "SSL: authority and subject key identifier mismatch.";
        break;

      case X509_V_ERR_AKID_ISSUER_SERIAL_MISMATCH:
        ssl_verify_error = "SSL: authority and issuer serial number mismatch.";
        break;

      case X509_V_ERR_KEYUSAGE_NO_CERTSIGN:
        ssl_verify_error = "SSL: key usage does not include certificate signing.";
        break;
      default:
        ssl_verify_error = "SSL: Unknown error.";
    };
    prompt = mprintf("\nUnknown SSL certificate:\n\n%s\n\n%s\n%s Code: %d\n"
                     "Accept certificate [a=always/y/N]? ", desc, warning, ssl_verify_error, vresult);
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
** Save certificate to global config.
*/
void ssl_save_certificate(X509 *cert){
  BIO *mem;
  char *zCert, *zHost;

  mem = BIO_new(BIO_s_mem());
  PEM_write_bio_X509(mem, cert);
  BIO_write(mem, "", 1); // null-terminate mem buffer
  BIO_get_mem_data(mem, &zCert);
  zHost = mprintf("cert:%s", g.urlName);
  db_set(zHost, zCert, 1);
  free(zHost);
  BIO_free(mem);  
}

/*
** Get certificate for g.urlName from global config.
** Return NULL if no certificate found.
*/
X509 *ssl_get_certificate(void){
  char *zHost, *zCert;
  BIO *mem;
  X509 *cert;

  zHost = mprintf("cert:%s", g.urlName);
  zCert = db_get(zHost, NULL);
  free(zHost);
  if ( zCert==NULL )
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

#endif /* FOSSIL_ENABLE_SSL */
