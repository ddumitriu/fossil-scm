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
** This file contains code used to cross link control files and
** manifests.  The file is named "manifest.c" because it was
** original only used to parse manifests.  Then later clusters
** and control files and wiki pages and tickets were added.
*/
#include "config.h"
#include "manifest.h"
#include <assert.h>

#if INTERFACE
/*
** Types of control files
*/
#define CFTYPE_MANIFEST   1
#define CFTYPE_CLUSTER    2
#define CFTYPE_CONTROL    3
#define CFTYPE_WIKI       4
#define CFTYPE_TICKET     5

/*
** Mode parameter values
*/
#define CFMODE_READ       1
#define CFMODE_APPEND     2
#define CFMODE_WRITE      3

/*
** A parsed manifest or cluster.
*/
struct Manifest {
  Blob content;         /* The original content blob */
  int type;             /* Type of file */
  int mode;             /* Access mode */
  char *zComment;       /* Decoded comment */
  double rDate;         /* Time in the "D" line */
  char *zUser;          /* Name of the user */
  char *zRepoCksum;     /* MD5 checksum of the baseline content */
  char *zWiki;          /* Text of the wiki page */
  char *zWikiTitle;     /* Name of the wiki page */
  char *zTicketUuid;    /* UUID for a ticket */
  int nFile;            /* Number of F lines */
  int nFileAlloc;       /* Slots allocated in aFile[] */
  struct { 
    char *zName;           /* Name of a file */
    char *zUuid;           /* UUID of the file */
    char *zPerm;           /* File permissions */
    char *zPrior;          /* Prior name if the name was changed */
    int iRename;           /* index of renamed name in prior/next manifest */
  } *aFile;
  int nParent;          /* Number of parents */
  int nParentAlloc;     /* Slots allocated in azParent[] */
  char **azParent;      /* UUIDs of parents */
  int nCChild;          /* Number of cluster children */
  int nCChildAlloc;     /* Number of closts allocated in azCChild[] */
  char **azCChild;      /* UUIDs of referenced objects in a cluster */
  int nTag;             /* Number of T lines */
  int nTagAlloc;        /* Slots allocated in aTag[] */
  struct { 
    char *zName;           /* Name of the tag */
    char *zUuid;           /* UUID that the tag is applied to */
    char *zValue;          /* Value if the tag is really a property */
  } *aTag;
  int nField;           /* Number of J lines */
  int nFieldAlloc;      /* Slots allocated in aField[] */
  struct { 
    char *zName;           /* Key or field name */
    char *zValue;          /* Value of the field */
  } *aField;
  int nAttach;          /* Number of A lines */
  int nAttachAlloc;     /* Slots allocated in aAttach[] */
  struct { 
    char *zUuid;           /* UUID of the attachment */
    char *zName;           /* Name of the attachment */
    char *zDesc;           /* Description of the attachment */
  } *aAttach;
};
#endif


/*
** Clear the memory allocated in a manifest object
*/
void manifest_clear(Manifest *p){
  blob_reset(&p->content);
  free(p->aFile);
  free(p->azParent);
  free(p->azCChild);
  free(p->aTag);
  free(p->aField);
  free(p->aAttach);
  memset(p, 0, sizeof(*p));
}

/*
** Parse a blob into a Manifest object.  The Manifest object
** takes over the input blob and will free it when the
** Manifest object is freed.  Zeros are inserted into the blob
** as string terminators so that blob should not be used again.
**
** Return TRUE if the content really is a control file of some
** kind.  Return FALSE if there are syntax errors.
**
** This routine is strict about the format of a control file.
** The format must match exactly or else it is rejected.  This
** rule minimizes the risk that a content file will be mistaken
** for a control file simply because they look the same.
**
** The pContent is reset.  If TRUE is returned, then pContent will
** be reset when the Manifest object is cleared.  If FALSE is
** returned then the Manifest object is cleared automatically
** and pContent is reset before the return.
**
** The entire file can be PGP clear-signed.  The signature is ignored.
** The file consists of zero or more cards, one card per line.
** (Except: the content of the W card can extend of multiple lines.)
** Each card is divided into tokens by a single space character.
** The first token is a single upper-case letter which is the card type.
** The card type determines the other parameters to the card.
** Cards must occur in lexicographical order.
*/
int manifest_parse(Manifest *p, Blob *pContent){
  int seenHeader = 0;
  int seenZ = 0;
  int i, lineNo=0;
  Blob line, token, a1, a2, a3, a4;
  char cPrevType = 0;

  memset(p, 0, sizeof(*p));
  memcpy(&p->content, pContent, sizeof(p->content));
  blob_zero(pContent);
  pContent = &p->content;

  blob_zero(&a1);
  blob_zero(&a2);
  blob_zero(&a3);
  md5sum_init();
  while( blob_line(pContent, &line) ){
    char *z = blob_buffer(&line);
    lineNo++;
    if( z[0]=='-' ){
      if( strncmp(z, "-----BEGIN PGP ", 15)!=0 ){
        goto manifest_syntax_error;
      }
      if( seenHeader ){
        break;
      }
      while( blob_line(pContent, &line)>2 ){}
      if( blob_line(pContent, &line)==0 ) break;
      z = blob_buffer(&line);
    }
    if( z[0]<cPrevType ){
      /* Lines of a manifest must occur in lexicographical order */
      goto manifest_syntax_error;
    }
    cPrevType = z[0];
    seenHeader = 1;
    if( blob_token(&line, &token)!=1 ) goto manifest_syntax_error;
    switch( z[0] ){
      /*
      **     A <uuid> <filename> <description>
      **
      ** Identifies an attachment to either a wiki page or a ticket.
      ** <uuid> is the artifact that is the attachment.
      */
      case 'A': {
        char *zName, *zUuid, *zDesc;
        md5sum_step_text(blob_buffer(&line), blob_size(&line));
        if( blob_token(&line, &a1)==0 ) goto manifest_syntax_error;
        if( blob_token(&line, &a2)==0 ) goto manifest_syntax_error;
        if( blob_token(&line, &a3)==0 ) goto manifest_syntax_error;
        zUuid = blob_terminate(&a1);
        zName = blob_terminate(&a2);
        zDesc = blob_terminate(&a3);
        if( blob_size(&a1)!=UUID_SIZE ) goto manifest_syntax_error;
        if( !validate16(zUuid, UUID_SIZE) ) goto manifest_syntax_error;
        defossilize(zName);
        if( !file_is_simple_pathname(zName) ){
          goto manifest_syntax_error;
        }
        defossilize(zDesc);
        if( p->nAttach>=p->nAttachAlloc ){
          p->nAttachAlloc = p->nAttachAlloc*2 + 10;
          p->aAttach = realloc(p->aAttach,
                               p->nAttachAlloc*sizeof(p->aAttach[0]) );
          if( p->aAttach==0 ) fossil_panic("out of memory");
        }
        i = p->nAttach++;
        p->aAttach[i].zUuid = zUuid;
        p->aAttach[i].zName = zName;
        p->aAttach[i].zDesc = zDesc;
        if( i>0 && strcmp(p->aAttach[i-1].zUuid, zUuid)>=0 ){
          goto manifest_syntax_error;
        }
        break;
      }

      /*
      **     C <comment>
      **
      ** Comment text is fossil-encoded.  There may be no more than
      ** one C line.  C lines are required for manifests and are
      ** disallowed on all other control files.
      */
      case 'C': {
        md5sum_step_text(blob_buffer(&line), blob_size(&line));
        if( p->zComment!=0 ) goto manifest_syntax_error;
        if( blob_token(&line, &a1)==0 ) goto manifest_syntax_error;
        if( blob_token(&line, &a2)!=0 ) goto manifest_syntax_error;
        p->zComment = blob_terminate(&a1);
        defossilize(p->zComment);
        break;
      }

      /*
      **     D <timestamp>
      **
      ** The timestamp should be ISO 8601.   YYYY-MM-DDtHH:MM:SS
      ** There can be no more than 1 D line.  D lines are required
      ** for all control files except for clusters.
      */
      case 'D': {
        char *zDate;
        md5sum_step_text(blob_buffer(&line), blob_size(&line));
        if( p->rDate!=0.0 ) goto manifest_syntax_error;
        if( blob_token(&line, &a1)==0 ) goto manifest_syntax_error;
        if( blob_token(&line, &a2)!=0 ) goto manifest_syntax_error;
        zDate = blob_terminate(&a1);
        p->rDate = db_double(0.0, "SELECT julianday(%Q)", zDate);
        break;
      }

      /*
      **     E <mode>
      **
      ** Access mode.  <mode> can be one of "read", "append",
      ** or "write".
      */
      case 'E': {
        md5sum_step_text(blob_buffer(&line), blob_size(&line));
        if( p->mode!=0 ) goto manifest_syntax_error;
        if( blob_token(&line, &a1)==0 ) goto manifest_syntax_error;
        if( blob_token(&line, &a2)!=0 ) goto manifest_syntax_error;
        if( blob_eq(&a1, "write") ){
          p->mode = CFMODE_WRITE;
        }else if( blob_eq(&a1, "append") ){
          p->mode = CFMODE_APPEND;
        }else if( blob_eq(&a1, "read") ){
          p->mode = CFMODE_READ;
        }else{
          goto manifest_syntax_error;
        }
        break;
      }

      /*
      **     F <filename> <uuid> ?<permissions>? ?<old-name>?
      **
      ** Identifies a file in a manifest.  Multiple F lines are
      ** allowed in a manifest.  F lines are not allowed in any
      ** other control file.  The filename and old-name are fossil-encoded.
      */
      case 'F': {
        char *zName, *zUuid, *zPerm, *zPriorName;
        md5sum_step_text(blob_buffer(&line), blob_size(&line));
        if( blob_token(&line, &a1)==0 ) goto manifest_syntax_error;
        if( blob_token(&line, &a2)==0 ) goto manifest_syntax_error;
        zName = blob_terminate(&a1);
        zUuid = blob_terminate(&a2);
        blob_token(&line, &a3);
        zPerm = blob_terminate(&a3);
        if( blob_size(&a2)!=UUID_SIZE ) goto manifest_syntax_error;
        if( !validate16(zUuid, UUID_SIZE) ) goto manifest_syntax_error;
        defossilize(zName);
        if( !file_is_simple_pathname(zName) ){
          goto manifest_syntax_error;
        }
        blob_token(&line, &a4);
        zPriorName = blob_terminate(&a4);
        if( zPriorName[0] ){
          defossilize(zPriorName);
          if( !file_is_simple_pathname(zPriorName) ){
            goto manifest_syntax_error;
          }
        }else{
          zPriorName = 0;
        }
        if( p->nFile>=p->nFileAlloc ){
          p->nFileAlloc = p->nFileAlloc*2 + 10;
          p->aFile = realloc(p->aFile, p->nFileAlloc*sizeof(p->aFile[0]) );
          if( p->aFile==0 ) fossil_panic("out of memory");
        }
        i = p->nFile++;
        p->aFile[i].zName = zName;
        p->aFile[i].zUuid = zUuid;
        p->aFile[i].zPerm = zPerm;
        p->aFile[i].zPrior = zPriorName;
        p->aFile[i].iRename = -1;
        if( i>0 && strcmp(p->aFile[i-1].zName, zName)>=0 ){
          goto manifest_syntax_error;
        }
        break;
      }

      /*
      **     J <name> ?<value>?
      **
      ** Specifies a name value pair for ticket.  If the first character
      ** of <name> is "+" then the <value> is appended to any preexisting
      ** value.  If <value> is omitted then it is understood to be an
      ** empty string.
      */
      case 'J': {
        char *zName, *zValue;
        md5sum_step_text(blob_buffer(&line), blob_size(&line));
        if( blob_token(&line, &a1)==0 ) goto manifest_syntax_error;
        blob_token(&line, &a2);
        if( blob_token(&line, &a3)!=0 ) goto manifest_syntax_error;
        zName = blob_terminate(&a1);
        zValue = blob_terminate(&a2);
        defossilize(zValue);
        if( p->nField>=p->nFieldAlloc ){
          p->nFieldAlloc = p->nFieldAlloc*2 + 10;
          p->aField = realloc(p->aField,
                               p->nFieldAlloc*sizeof(p->aField[0]) );
          if( p->aField==0 ) fossil_panic("out of memory");
        }
        i = p->nField++;
        p->aField[i].zName = zName;
        p->aField[i].zValue = zValue;
        if( i>0 && strcmp(p->aField[i-1].zName, zName)>=0 ){
          goto manifest_syntax_error;
        }
        break;
      }


      /*
      **    K <uuid>
      **
      ** A K-line gives the UUID for the ticket which this control file
      ** is amending.
      */
      case 'K': {
        char *zUuid;
        md5sum_step_text(blob_buffer(&line), blob_size(&line));
        if( blob_token(&line, &a1)==0 ) goto manifest_syntax_error;
        zUuid = blob_terminate(&a1);
        if( blob_size(&a1)!=UUID_SIZE ) goto manifest_syntax_error;
        if( !validate16(zUuid, UUID_SIZE) ) goto manifest_syntax_error;
        if( p->zTicketUuid!=0 ) goto manifest_syntax_error;
        p->zTicketUuid = zUuid;
        break;
      }

      /*
      **     L <wikititle>
      **
      ** The wiki page title is fossil-encoded.  There may be no more than
      ** one L line.
      */
      case 'L': {
        md5sum_step_text(blob_buffer(&line), blob_size(&line));
        if( p->zWikiTitle!=0 ) goto manifest_syntax_error;
        if( blob_token(&line, &a1)==0 ) goto manifest_syntax_error;
        if( blob_token(&line, &a2)!=0 ) goto manifest_syntax_error;
        p->zWikiTitle = blob_terminate(&a1);
        defossilize(p->zWikiTitle);
        if( !wiki_name_is_wellformed(p->zWikiTitle) ){
          goto manifest_syntax_error;
        }
        break;
      }

      /*
      **    M <uuid>
      **
      ** An M-line identifies another artifact by its UUID.  M-lines
      ** occur in clusters only.
      */
      case 'M': {
        char *zUuid;
        md5sum_step_text(blob_buffer(&line), blob_size(&line));
        if( blob_token(&line, &a1)==0 ) goto manifest_syntax_error;
        zUuid = blob_terminate(&a1);
        if( blob_size(&a1)!=UUID_SIZE ) goto manifest_syntax_error;
        if( !validate16(zUuid, UUID_SIZE) ) goto manifest_syntax_error;
        if( p->nCChild>=p->nCChildAlloc ){
          p->nCChildAlloc = p->nCChildAlloc*2 + 10;
          p->azCChild = 
             realloc(p->azCChild, p->nCChildAlloc*sizeof(p->azCChild[0]) );
          if( p->azCChild==0 ) fossil_panic("out of memory");
        }
        i = p->nCChild++;
        p->azCChild[i] = zUuid;
        if( i>0 && strcmp(p->azCChild[i-1], zUuid)>=0 ){
          goto manifest_syntax_error;
        }
        break;
      }

      /*
      **     P <uuid> ...
      **
      ** Specify one or more other artifacts where are the parents of
      ** this artifact.  The first parent is the primary parent.  All
      ** others are parents by merge.
      */
      case 'P': {
        md5sum_step_text(blob_buffer(&line), blob_size(&line));
        while( blob_token(&line, &a1) ){
          char *zUuid;
          if( blob_size(&a1)!=UUID_SIZE ) goto manifest_syntax_error;
          zUuid = blob_terminate(&a1);
          if( !validate16(zUuid, UUID_SIZE) ) goto manifest_syntax_error;
          if( p->nParent>=p->nParentAlloc ){
            p->nParentAlloc = p->nParentAlloc*2 + 5;
            p->azParent = realloc(p->azParent, p->nParentAlloc*sizeof(char*));
            if( p->azParent==0 ) fossil_panic("out of memory");
          }
          i = p->nParent++;
          p->azParent[i] = zUuid;
        }
        break;
      }

      /*
      **     R <md5sum>
      **
      ** Specify the MD5 checksum of the entire baseline in a
      ** manifest.
      */
      case 'R': {
        md5sum_step_text(blob_buffer(&line), blob_size(&line));
        if( p->zRepoCksum!=0 ) goto manifest_syntax_error;
        if( blob_token(&line, &a1)==0 ) goto manifest_syntax_error;
        if( blob_token(&line, &a2)!=0 ) goto manifest_syntax_error;
        if( blob_size(&a1)!=32 ) goto manifest_syntax_error;
        p->zRepoCksum = blob_terminate(&a1);
        if( !validate16(p->zRepoCksum, 32) ) goto manifest_syntax_error;
        break;
      }

      /*
      **    T (+|*|-)<tagname> <uuid> ?<value>?
      **
      ** Create or cancel a tag or property.  The tagname is fossil-encoded.
      ** The first character of the name must be either "+" to create a
      ** singleton tag, "*" to create a propagating tag, or "-" to create
      ** anti-tag that undoes a prior "+" or blocks propagation of of
      ** a "*".
      **
      ** The tag is applied to <uuid>.  If <uuid> is "*" then the tag is
      ** applied to the current manifest.  If <value> is provided then 
      ** the tag is really a property with the given value.
      **
      ** Tags are not allowed in clusters.  Multiple T lines are allowed.
      */
      case 'T': {
        char *zName, *zUuid, *zValue;
        md5sum_step_text(blob_buffer(&line), blob_size(&line));
        if( blob_token(&line, &a1)==0 ){
          goto manifest_syntax_error;
        }
        if( blob_token(&line, &a2)==0 ){
          goto manifest_syntax_error;
        }
        zName = blob_terminate(&a1);
        zUuid = blob_terminate(&a2);
        if( blob_token(&line, &a3)==0 ){
          zValue = 0;
        }else{
          zValue = blob_terminate(&a3);
          defossilize(zValue);
        }
        if( blob_size(&a2)==UUID_SIZE && validate16(zUuid, UUID_SIZE) ){
          /* A valid uuid */
        }else if( blob_size(&a2)==1 && zUuid[0]=='*' ){
          zUuid = 0;
        }else{
          goto manifest_syntax_error;
        }
        defossilize(zName);
        if( zName[0]!='-' && zName[0]!='+' && zName[0]!='*' ){
          goto manifest_syntax_error;
        }
        if( validate16(&zName[1], strlen(&zName[1])) ){
          /* Do not allow tags whose names look like UUIDs */
          goto manifest_syntax_error;
        }
        if( p->nTag>=p->nTagAlloc ){
          p->nTagAlloc = p->nTagAlloc*2 + 10;
          p->aTag = realloc(p->aTag, p->nTagAlloc*sizeof(p->aTag[0]) );
          if( p->aTag==0 ) fossil_panic("out of memory");
        }
        i = p->nTag++;
        p->aTag[i].zName = zName;
        p->aTag[i].zUuid = zUuid;
        p->aTag[i].zValue = zValue;
        if( i>0 && strcmp(p->aTag[i-1].zName, zName)>=0 ){
          goto manifest_syntax_error;
        }
        break;
      }

      /*
      **     U ?<login>?
      **
      ** Identify the user who created this control file by their
      ** login.  Only one U line is allowed.  Prohibited in clusters.
      ** If the user name is omitted, take that to be "anonymous".
      */
      case 'U': {
        md5sum_step_text(blob_buffer(&line), blob_size(&line));
        if( p->zUser!=0 ) goto manifest_syntax_error;
        if( blob_token(&line, &a1)==0 ){
          p->zUser = "anonymous";
        }else{
          p->zUser = blob_terminate(&a1);
          defossilize(p->zUser);
        }
        if( blob_token(&line, &a2)!=0 ) goto manifest_syntax_error;
        break;
      }

      /*
      **     W <size>
      **
      ** The next <size> bytes of the file contain the text of the wiki
      ** page.  There is always an extra \n before the start of the next
      ** record.
      */
      case 'W': {
        int size;
        Blob wiki;
        md5sum_step_text(blob_buffer(&line), blob_size(&line));
        if( blob_token(&line, &a1)==0 ) goto manifest_syntax_error;
        if( blob_token(&line, &a2)!=0 ) goto manifest_syntax_error;
        if( !blob_is_int(&a1, &size) ) goto manifest_syntax_error;
        if( size<0 ) goto manifest_syntax_error;
        if( p->zWiki!=0 ) goto manifest_syntax_error;
        blob_zero(&wiki);
        if( blob_extract(pContent, size+1, &wiki)!=size+1 ){
          goto manifest_syntax_error;
        }
        p->zWiki = blob_buffer(&wiki);
        md5sum_step_text(p->zWiki, size+1);
        if( p->zWiki[size]!='\n' ) goto manifest_syntax_error;
        p->zWiki[size] = 0;
        break;
      }


      /*
      **     Z <md5sum>
      **
      ** MD5 checksum on this control file.  The checksum is over all
      ** lines (other than PGP-signature lines) prior to the current
      ** line.  This must be the last record.
      **
      ** This card is required for all control file types except for
      ** Manifest.  It is not required for manifest only for historical
      ** compatibility reasons.
      */
      case 'Z': {
        int rc;
        Blob hash;
        if( blob_token(&line, &a1)==0 ) goto manifest_syntax_error;
        if( blob_token(&line, &a2)!=0 ) goto manifest_syntax_error;
        if( blob_size(&a1)!=32 ) goto manifest_syntax_error;
        if( !validate16(blob_buffer(&a1), 32) ) goto manifest_syntax_error;
        md5sum_finish(&hash);
        rc = blob_compare(&hash, &a1);
        blob_reset(&hash);
        if( rc!=0 ) goto manifest_syntax_error;
        seenZ = 1;
        break;
      }
      default: {
        goto manifest_syntax_error;
      }
    }
  }
  if( !seenHeader ) goto manifest_syntax_error;

  if( p->nFile>0 || p->zRepoCksum!=0 ){
    if( p->nCChild>0 ) goto manifest_syntax_error;
    if( p->rDate==0.0 ) goto manifest_syntax_error;
    if( p->nField>0 ) goto manifest_syntax_error;
    if( p->zTicketUuid ) goto manifest_syntax_error;
    if( p->nAttach>0 ) goto manifest_syntax_error;
    if( p->zWiki ) goto manifest_syntax_error;
    if( p->zWikiTitle ) goto manifest_syntax_error;
    if( p->zTicketUuid ) goto manifest_syntax_error;
    p->type = CFTYPE_MANIFEST;
  }else if( p->nCChild>0 ){
    if( p->rDate>0.0 ) goto manifest_syntax_error;
    if( p->zComment!=0 ) goto manifest_syntax_error;
    if( p->zUser!=0 ) goto manifest_syntax_error;
    if( p->nTag>0 ) goto manifest_syntax_error;
    if( p->nParent>0 ) goto manifest_syntax_error;
    if( p->zRepoCksum!=0 ) goto manifest_syntax_error;
    if( p->nField>0 ) goto manifest_syntax_error;
    if( p->zTicketUuid ) goto manifest_syntax_error;
    if( p->nAttach>0 ) goto manifest_syntax_error;
    if( p->zWiki ) goto manifest_syntax_error;
    if( p->zWikiTitle ) goto manifest_syntax_error;
    if( !seenZ ) goto manifest_syntax_error;
    p->type = CFTYPE_CLUSTER;
  }else if( p->nField>0 ){
    if( p->rDate==0.0 ) goto manifest_syntax_error;
    if( p->zRepoCksum!=0 ) goto manifest_syntax_error;
    if( p->zWiki ) goto manifest_syntax_error;
    if( p->zWikiTitle ) goto manifest_syntax_error;
    if( p->nCChild>0 ) goto manifest_syntax_error;
    if( p->nTag>0 ) goto manifest_syntax_error;
    if( p->zTicketUuid==0 ) goto manifest_syntax_error;
    if( p->zUser==0 ) goto manifest_syntax_error;
    if( !seenZ ) goto manifest_syntax_error;
    p->type = CFTYPE_TICKET;
  }else if( p->zWiki!=0 ){
    if( p->rDate==0.0 ) goto manifest_syntax_error;
    if( p->zRepoCksum!=0 ) goto manifest_syntax_error;
    if( p->nCChild>0 ) goto manifest_syntax_error;
    if( p->nTag>0 ) goto manifest_syntax_error;
    if( p->zTicketUuid!=0 ) goto manifest_syntax_error;
    if( p->zWikiTitle==0 ) goto manifest_syntax_error;
    if( !seenZ ) goto manifest_syntax_error;
    p->type = CFTYPE_WIKI;
  }else if( p->nTag>0 ){
    if( p->rDate<=0.0 ) goto manifest_syntax_error;
    if( p->zRepoCksum!=0 ) goto manifest_syntax_error;
    if( p->nParent>0 ) goto manifest_syntax_error;
    if( p->nAttach>0 ) goto manifest_syntax_error;
    if( p->nField>0 ) goto manifest_syntax_error;
    if( p->zWiki ) goto manifest_syntax_error;
    if( p->zWikiTitle ) goto manifest_syntax_error;
    if( p->zTicketUuid ) goto manifest_syntax_error;
    if( !seenZ ) goto manifest_syntax_error;
    p->type = CFTYPE_CONTROL;
  }else{
    if( p->nCChild>0 ) goto manifest_syntax_error;
    if( p->rDate==0.0 ) goto manifest_syntax_error;
    if( p->nField>0 ) goto manifest_syntax_error;
    if( p->zTicketUuid ) goto manifest_syntax_error;
    if( p->nAttach>0 ) goto manifest_syntax_error;
    if( p->zWiki ) goto manifest_syntax_error;
    if( p->zWikiTitle ) goto manifest_syntax_error;
    if( p->zTicketUuid ) goto manifest_syntax_error;
    p->type = CFTYPE_MANIFEST;
  }
    
  md5sum_init();
  return 1;

manifest_syntax_error:
  /*fprintf(stderr, "Manifest error on line %i\n", lineNo);fflush(stderr);*/
  md5sum_init();
  manifest_clear(p);
  return 0;
}

/*
** COMMAND: test-parse-manifest
**
** Usage: %fossil test-parse-manifest FILENAME
**
** Parse the manifest and discarded.  Use for testing only.
*/
void manifest_test_parse_cmd(void){
  Manifest m;
  Blob b;
  if( g.argc!=3 ){
    usage("FILENAME");
  }
  db_must_be_within_tree();
  blob_read_from_file(&b, g.argv[2]);
  manifest_parse(&m, &b);
  manifest_clear(&m);
}

/*
** Add a single entry to the mlink table.  Also add the filename to
** the filename table if it is not there already.
*/
static void add_one_mlink(
  int mid,                  /* The record ID of the manifest */
  const char *zFromUuid,    /* UUID for the mlink.pid field */
  const char *zToUuid,      /* UUID for the mlink.fid field */
  const char *zFilename,    /* Filename */
  const char *zPrior        /* Previous filename.  NULL if unchanged */
){
  int fnid, pfnid, pid, fid;

  fnid = db_int(0, "SELECT fnid FROM filename WHERE name=%Q", zFilename);
  if( fnid==0 ){
    db_multi_exec("INSERT INTO filename(name) VALUES(%Q)", zFilename);
    fnid = db_last_insert_rowid();
  }
  if( zPrior==0 ){
    pfnid = 0;
  }else{
    pfnid = db_int(0, "SELECT fnid FROM filename WHERE name=%Q", zPrior);
    if( pfnid==0 ){
      db_multi_exec("INSERT INTO filename(name) VALUES(%Q)", zPrior);
      pfnid = db_last_insert_rowid();
    }
  }
  if( zFromUuid==0 ){
    pid = 0;
  }else{
    pid = uuid_to_rid(zFromUuid, 1);
  }
  if( zToUuid==0 ){
    fid = 0;
  }else{
    fid = uuid_to_rid(zToUuid, 1);
  }
  db_multi_exec(
    "INSERT INTO mlink(mid,pid,fid,fnid,pfnid)"
    "VALUES(%d,%d,%d,%d,%d)", mid, pid, fid, fnid, pfnid
  );
  if( pid && fid ){
    content_deltify(pid, fid, 0);
  }
}

/*
** Locate a file named zName in the aFile[] array of the given
** manifest.  We assume that filenames are in sorted order.
** Use a binary search.  Return turn the index of the matching
** entry.  Or return -1 if not found.
*/
static int find_file_in_manifest(Manifest *p, const char *zName){
  int lwr, upr;
  int c;
  int i;
  lwr = 0;
  upr = p->nFile - 1;
  while( lwr<=upr ){
    i = (lwr+upr)/2;
    c = strcmp(p->aFile[i].zName, zName);
    if( c<0 ){
      lwr = i+1;
    }else if( c>0 ){
      upr = i-1;
    }else{
      return i;
    }
  }
  return -1;
}

/*
** Add mlink table entries associated with manifest cid.  The
** parent manifest is pid.
**
** A single mlink entry is added for every file that changed content
** and/or name going from pid to cid.
**
** Deleted files have mlink.fid=0.
** Added files have mlink.pid=0.
** Edited files have both mlink.pid!=0 and mlink.fid!=0
*/
static void add_mlink(int pid, Manifest *pParent, int cid, Manifest *pChild){
  Manifest other;
  Blob otherContent;
  int i, j;

  if( db_exists("SELECT 1 FROM mlink WHERE mid=%d", cid) ){
    return;
  }
  assert( pParent==0 || pChild==0 );
  if( pParent==0 ){
    pParent = &other;
    content_get(pid, &otherContent);
  }else{
    pChild = &other;
    content_get(cid, &otherContent);
  }
  if( blob_size(&otherContent)==0 ) return;
  if( manifest_parse(&other, &otherContent)==0 ) return;
  content_deltify(pid, cid, 0);

  /* Use the iRename fields to find the cross-linkage between
  ** renamed files.  */
  for(j=0; j<pChild->nFile; j++){
    const char *zPrior = pChild->aFile[j].zPrior;
    if( zPrior && zPrior[0] ){
      i = find_file_in_manifest(pParent, zPrior);
      if( i>=0 ){
        pChild->aFile[j].iRename = i;
        pParent->aFile[i].iRename = j;
      }
    }
  }

  /* Construct the mlink entries */
  for(i=j=0; i<pParent->nFile && j<pChild->nFile; ){
    int c;
    if( pParent->aFile[i].iRename>=0 ){
      i++;
    }else if( (c = strcmp(pParent->aFile[i].zName, pChild->aFile[j].zName))<0 ){
      add_one_mlink(cid, pParent->aFile[i].zUuid,0,pParent->aFile[i].zName,0);
      i++;
    }else if( c>0 ){
      int rn = pChild->aFile[j].iRename;
      if( rn>=0 ){
        add_one_mlink(cid, pParent->aFile[rn].zUuid, pChild->aFile[j].zUuid, 
                      pChild->aFile[j].zName, pParent->aFile[rn].zName);
      }else{
        add_one_mlink(cid, 0, pChild->aFile[j].zUuid, pChild->aFile[j].zName,0);
      }
      j++;
    }else{
      if( strcmp(pParent->aFile[i].zUuid, pChild->aFile[j].zUuid)!=0 ){
        add_one_mlink(cid, pParent->aFile[i].zUuid, pChild->aFile[j].zUuid, 
                      pChild->aFile[j].zName, 0);
      }
      i++;
      j++;
    }
  }
  while( i<pParent->nFile ){
    if( pParent->aFile[i].iRename<0 ){
      add_one_mlink(cid, pParent->aFile[i].zUuid, 0, pParent->aFile[i].zName,0);
    }
    i++;
  }
  while( j<pChild->nFile ){
    int rn = pChild->aFile[j].iRename;
    if( rn>=0 ){
      add_one_mlink(cid, pParent->aFile[rn].zUuid, pChild->aFile[j].zUuid, 
                    pChild->aFile[j].zName, pParent->aFile[rn].zName);
    }else{
      add_one_mlink(cid, 0, pChild->aFile[j].zUuid, pChild->aFile[j].zName,0);
    }
    j++;
  }
  manifest_clear(&other);
}

/*
** Scan artifact rid/pContent to see if it is a control artifact of
** any key:
**
**      *  Manifest
**      *  Control
**      *  Wiki Page
**      *  Ticket Change
**      *  Cluster
**
** If the input is a control artifact, then make appropriate entries
** in the auxiliary tables of the database in order to crosslink the
** artifact.
**
** If global variable g.xlinkClusterOnly is true, then ignore all 
** control artifacts other than clusters.
**
** Historical note:  This routine original processed manifests only.
** Processing for other control artifacts was added later.  The name
** of the routine, "manifest_crosslink", and the name of this source
** file, is a legacy of its original use.
*/
int manifest_crosslink(int rid, Blob *pContent){
  int i;
  Manifest m;
  Stmt q;
  int parentid = 0;

  if( manifest_parse(&m, pContent)==0 ){
    return 0;
  }
  if( g.xlinkClusterOnly && m.type!=CFTYPE_CLUSTER ){
    manifest_clear(&m);
    return 0;
  }
  db_begin_transaction();
  if( m.type==CFTYPE_MANIFEST ){
    if( !db_exists("SELECT 1 FROM mlink WHERE mid=%d", rid) ){
      for(i=0; i<m.nParent; i++){
        int pid = uuid_to_rid(m.azParent[i], 1);
        db_multi_exec("INSERT OR IGNORE INTO plink(pid, cid, isprim, mtime)"
                      "VALUES(%d, %d, %d, %.17g)", pid, rid, i==0, m.rDate);
        if( i==0 ){
          add_mlink(pid, 0, rid, &m);
          parentid = pid;
        }
      }
      db_prepare(&q, "SELECT cid FROM plink WHERE pid=%d AND isprim", rid);
      while( db_step(&q)==SQLITE_ROW ){
        int cid = db_column_int(&q, 0);
        add_mlink(rid, &m, cid, 0);
      }
      db_finalize(&q);
      db_multi_exec(
        "REPLACE INTO event(type,mtime,objid,user,comment,"
        "                  bgcolor,euser,ecomment)"
        "VALUES('ci',%.17g,%d,%Q,%Q,"
        " (SELECT value FROM tagxref WHERE tagid=%d AND rid=%d AND tagtype>0),"
        "  (SELECT value FROM tagxref WHERE tagid=%d AND rid=%d),"
        "  (SELECT value FROM tagxref WHERE tagid=%d AND rid=%d));",
        m.rDate, rid, m.zUser, m.zComment, 
        TAG_BGCOLOR, rid,
        TAG_USER, rid,
        TAG_COMMENT, rid
      );
    }
  }
  if( m.type==CFTYPE_CLUSTER ){
    tag_insert("cluster", 1, 0, rid, m.rDate, rid);
    for(i=0; i<m.nCChild; i++){
      int mid;
      mid = uuid_to_rid(m.azCChild[i], 1);
      if( mid>0 ){
        db_multi_exec("DELETE FROM unclustered WHERE rid=%d", mid);
      }
    }
  }
  if( m.type==CFTYPE_CONTROL || m.type==CFTYPE_MANIFEST ){
    for(i=0; i<m.nTag; i++){
      int tid;
      int type;
      if( m.aTag[i].zUuid ){
        tid = uuid_to_rid(m.aTag[i].zUuid, 1);
      }else{
        tid = rid;
      }
      if( tid ){
        switch( m.aTag[i].zName[0] ){
          case '+':  type = 1; break;
          case '*':  type = 2; break;
          case '-':  type = 0; break;
          default:
            fossil_fatal("unknown tag type in manifest: %s", m.aTag);
            return 0;
        }
        tag_insert(&m.aTag[i].zName[1], type, m.aTag[i].zValue, 
                   rid, m.rDate, tid);
      }
    }
    if( parentid ){
      tag_propagate_all(parentid);
    }
  }
  if( m.type==CFTYPE_WIKI ){
    char *zTag = mprintf("wiki-%s", m.zWikiTitle);
    int tagid = tag_findid(zTag, 1);
    int prior;
    char *zComment;
    tag_insert(zTag, 1, 0, rid, m.rDate, rid);
    free(zTag);
    prior = db_int(0,
      "SELECT rid FROM tagxref"
      " WHERE tagid=%d AND mtime<%.17g"
      " ORDER BY mtime DESC",
      tagid, m.rDate
    );
    if( prior ){
      content_deltify(prior, rid, 0);
    }
    zComment = mprintf("Changes to wiki page [%h]", m.zWikiTitle);
    db_multi_exec(
      "REPLACE INTO event(type,mtime,objid,user,comment,"
      "                  bgcolor,euser,ecomment)"
      "VALUES('w',%.17g,%d,%Q,%Q,"
      "  (SELECT value FROM tagxref WHERE tagid=%d AND rid=%d AND tagtype>1),"
      "  (SELECT value FROM tagxref WHERE tagid=%d AND rid=%d),"
      "  (SELECT value FROM tagxref WHERE tagid=%d AND rid=%d));",
      m.rDate, rid, m.zUser, zComment, 
      TAG_BGCOLOR, rid,
      TAG_BGCOLOR, rid,
      TAG_USER, rid,
      TAG_COMMENT, rid
    );
    free(zComment);
  }
  if( m.type==CFTYPE_TICKET ){
    int i;
    char *zTitle;
    char *zTag;
    Blob comment;
    char *zNewStatus = 0;
    static char *zTitleExpr = 0;
    static char *zStatusColumn = 0;
    static int once = 1;
    int isNew;

    isNew = ticket_insert(&m, 1, 1);
    zTag = mprintf("tkt-%s", m.zTicketUuid);
    tag_insert(zTag, 1, 0, rid, m.rDate, rid);
    free(zTag);
    blob_zero(&comment);
    if( once ){
      once = 0;
      zTitleExpr = db_get("ticket-title-expr", "title");
      zStatusColumn = db_get("ticket-status-column", "status");
    }
    zTitle = db_text("unknown", 
      "SELECT %s FROM ticket WHERE tkt_uuid='%s'",
      zTitleExpr, m.zTicketUuid
    );
    if( !isNew ){
      for(i=0; i<m.nField; i++){
        if( strcmp(m.aField[i].zName, zStatusColumn)==0 ){
          zNewStatus = m.aField[i].zValue;
        }
      }
      if( zNewStatus ){
        blob_appendf(&comment, "%h ticket [%.10s]: <i>%h</i>",
           zNewStatus, m.zTicketUuid, zTitle
        );
        if( m.nField>1 ){
          blob_appendf(&comment, " plus %d other change%s",
            m.nField-1, m.nField==2 ? "" : "s");
        }
      }else{
        zNewStatus = db_text("unknown", 
           "SELECT %s FROM ticket WHERE tkt_uuid='%s'",
           zStatusColumn, m.zTicketUuid
        );
        blob_appendf(&comment, "Ticket [%.10s] <i>%h</i> status still %h with "
             "%d other change%s",
             m.zTicketUuid, zTitle, zNewStatus, m.nField,
             m.nField==1 ? "" : "s"
        );
        free(zNewStatus);
      }
    }else{
      blob_appendf(&comment, "New ticket [%.10s] <i>%h</i>.",
        m.zTicketUuid, zTitle
      );
    }
    free(zTitle);
    db_multi_exec(
      "REPLACE INTO event(type,mtime,objid,user,comment)"
      "VALUES('t',%.17g,%d,%Q,%Q)",
      m.rDate, rid, m.zUser, blob_str(&comment)
    );
    blob_reset(&comment);
  }
  db_end_transaction(0);
  manifest_clear(&m);
  return 1;
}
