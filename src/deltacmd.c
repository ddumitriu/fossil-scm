/*
** Copyright (c) 2006 D. Richard Hipp
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
** This module implements the interface to the delta generator.
*/
#include "config.h"
#include "deltacmd.h"
 
/*
** Create a delta that describes the change from pOriginal to pTarget
** and put that delta in pDelta.  The pDelta blob is assumed to be
** uninitialized.
*/
int blob_delta_create(Blob *pOriginal, Blob *pTarget, Blob *pDelta){
  const char *zOrig, *zTarg;
  int lenOrig, lenTarg;
  int len;
  char *zRes;
  blob_zero(pDelta);
  zOrig = blob_buffer(pOriginal);
  lenOrig = blob_size(pOriginal);
  zTarg = blob_buffer(pTarget);
  lenTarg = blob_size(pTarget);
  blob_resize(pDelta, lenTarg+16);
  zRes = blob_buffer(pDelta);
  len = delta_create(zOrig, lenOrig, zTarg, lenTarg, zRes);
  blob_resize(pDelta, len);
  return 0;
}

/*
** COMMAND:  test-delta-create
**
** Given two input files, create and output a delta that carries
** the first file into the second.
*/
void delta_create_cmd(void){
  Blob orig, target, delta;
  if( g.argc!=5 ){
    fprintf(stderr,"Usage: %s %s ORIGIN TARGET DELTA\n", g.argv[0], g.argv[1]);
    exit(1);
  }
  if( blob_read_from_file(&orig, g.argv[2])<0 ){
    fprintf(stderr,"cannot read %s\n", g.argv[2]);
    exit(1);
  }
  if( blob_read_from_file(&target, g.argv[3])<0 ){
    fprintf(stderr,"cannot read %s\n", g.argv[3]);
    exit(1);
  }
  blob_delta_create(&orig, &target, &delta);
  if( blob_write_to_file(&delta, g.argv[4])<blob_size(&delta) ){
    fprintf(stderr,"cannot write %s\n", g.argv[4]);
    exit(1);
  }
  blob_reset(&orig);
  blob_reset(&target);
  blob_reset(&delta);
}

/*
** Apply the delta in pDelta to the original file pOriginal to generate
** the target file pTarget.  The pTarget blob is initialized by this
** routine.
**
** It works ok for pTarget and pOriginal to be the same blob.
**
** Return the length of the target.  Return -1 if there is an error.
*/
int blob_delta_apply(Blob *pOriginal, Blob *pDelta, Blob *pTarget){
  int len, n;
  Blob out;

  n = delta_output_size(blob_buffer(pDelta), blob_size(pDelta));
  blob_zero(&out);
  blob_resize(&out, n);
  len = delta_apply(
     blob_buffer(pOriginal), blob_size(pOriginal),
     blob_buffer(pDelta), blob_size(pDelta),
     blob_buffer(&out));
  if( len<0 ){
    blob_reset(&out);
  }else if( len!=n ){
    blob_resize(&out, len);
  }
  if( pTarget==pOriginal ){
    blob_reset(pOriginal);
  }
  *pTarget = out;
  return len;
}

/*
** COMMAND:  test-delta-apply
**
** Given an input files and a delta, apply the delta to the input file
** and write the result.
*/
void delta_apply_cmd(void){
  Blob orig, target, delta;
  if( g.argc!=5 ){
    fprintf(stderr,"Usage: %s %s ORIGIN DELTA TARGET\n", g.argv[0], g.argv[1]);
    exit(1);
  }
  if( blob_read_from_file(&orig, g.argv[2])<0 ){
    fprintf(stderr,"cannot read %s\n", g.argv[2]);
    exit(1);
  }
  if( blob_read_from_file(&delta, g.argv[3])<0 ){
    fprintf(stderr,"cannot read %s\n", g.argv[3]);
    exit(1);
  }
  blob_delta_apply(&orig, &delta, &target);
  if( blob_write_to_file(&target, g.argv[4])<blob_size(&target) ){
    fprintf(stderr,"cannot write %s\n", g.argv[4]);
    exit(1);
  }
  blob_reset(&orig);
  blob_reset(&target);
  blob_reset(&delta);
}

/*
** COMMAND:  test-delta
**
** Read two files named on the command-line.  Create and apply deltas
** going in both directions.  Verify that the original files are
** correctly recovered.
*/
void cmd_test_delta(void){
  Blob f1, f2;     /* Original file content */
  Blob d12, d21;   /* Deltas from f1->f2 and f2->f1 */
  Blob a1, a2;     /* Recovered file content */
  if( g.argc!=4 ) usage("FILE1 FILE2");
  blob_read_from_file(&f1, g.argv[2]);
  blob_read_from_file(&f2, g.argv[3]);
  blob_delta_create(&f1, &f2, &d12);
  blob_delta_create(&f2, &f1, &d21);
  blob_delta_apply(&f1, &d12, &a2);
  blob_delta_apply(&f2, &d21, &a1);
  if( blob_compare(&f1,&a1) || blob_compare(&f2, &a2) ){
    fossil_panic("delta test failed");
  }
  printf("ok\n");
}
