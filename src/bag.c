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
** This file contains code used to implement a "bag" of integers.
** A bag is an unordered collection without duplicates.  In this
** implementation, all elements must be positive integers.
*/
#include "config.h"
#include "bag.h"
#include <assert.h>


#if INTERFACE
/*
** An integer can appear in the bag at most once.
** Integers must be positive.
*/
struct Bag {
  int cnt;   /* Number of integers in the bag */
  int sz;    /* Number of slots in a[] */
  int used;  /* Number of used slots in a[] */
  int *a;    /* Hash table of integers that are in the bag */
};
#endif

/*
** Initialize a Bag structure
*/
void bag_init(Bag *p){
  memset(p, 0, sizeof(*p));
}

/*
** Destroy a Bag.  Delete all of its content.
*/
void bag_clear(Bag *p){
  free(p->a);
  bag_init(p);
}

/*
** The hash function
*/
#define bag_hash(i)  (i*101)

/*
** Change the size of the hash table on a bag so that
** it contains N slots
*/
static void bag_resize(Bag *p, int newSize){
  int i;
  Bag old;

  old = *p;
  assert( newSize>old.cnt );
  p->a = malloc( sizeof(p->a[0])*newSize );
  p->sz = newSize;
  memset(p->a, 0, sizeof(p->a[0])*newSize );
  for(i=0; i<old.sz; i++){
    int e = old.a[i];
    if( e>0 ){
      unsigned h = bag_hash(e)%newSize;
      while( p->a[h] ){
        h++;
        if( h==newSize ) h = 0;
      }
      p->a[h] = e;
    }
  }
  p->used = p->cnt;
  bag_clear(&old);
}

/*
** Insert element e into the bag if it is not there already.
** Return TRUE if the insert actually occurred.  Return FALSE
** if the element was already in the bag.
*/
int bag_insert(Bag *p, int e){
  unsigned h;
  int rc = 0;
  assert( e>0 );
  if( p->used+1 >= p->sz/2 ){
    bag_resize(p,  p->cnt*2 + 20 );
  }
  h = bag_hash(e)%p->sz;
  while( p->a[h] && p->a[h]!=e ){
    h++;
    if( h>=p->sz ) h = 0;
  }
  if( p->a[h]==0 ){
    p->a[h] = e;
    p->used++;
    p->cnt++;
    rc = 1;
  }
  return rc;
}

/*
** Return true if e in the bag.  Return false if it is no.
*/
int bag_find(Bag *p, int e){
  unsigned h;
  assert( e>0 );
  if( p->sz==0 ){
    return 0;
  }
  h = bag_hash(e)%p->sz;
  while( p->a[h] && p->a[h]!=e ){
    h++;
    if( h>=p->sz ) h = 0;
  }
  return p->a[h]==e;
}

/*
** Remove element e from the bag if it exists in the bag.
** If e is not in the bag, this is a no-op.
*/
void bag_remove(Bag *p, int e){
  unsigned h;
  assert( e>0 );
  if( p->sz==0 ) return;
  h = bag_hash(e)%p->sz;
  while( p->a[h] && p->a[h]!=e ){
    h++;
    if( h>=p->sz ) h = 0;
  }
  if( p->a[h] ){
    p->a[h] = -1;
    p->cnt--;
    if( p->sz>20 && p->cnt<p->sz/8 ){
      bag_resize(p, p->sz/2);
    }
  }
}

/*
** Return the first element in the bag.  Return 0 if the bag
** is empty.
*/
int bag_first(Bag *p){
  int i;
  for(i=0; i<p->sz && p->a[i]<=0; i++){}
  if( i<p->sz ){
    return p->a[i];
  }else{
    return 0;
  }
}

/*
** Return the next element in the bag after e.  Return 0 if
** is the last element in the bag.  Any insert or removal from
** the bag might reorder the bag.
*/
int bag_next(Bag *p, int e){
  unsigned h;
  assert( p->sz>0 );
  assert( e>0 );
  h = bag_hash(e)%p->sz;
  while( p->a[h] && p->a[h]!=e ){
    h++;
    if( h>=p->sz ) h = 0;
  }
  assert( p->a[h] );
  h++;
  while( h<p->sz && p->a[h]<=0 ){
    h++;
  }
  return h<p->sz ? p->a[h] : 0;
}

/*
** Return the number of elements in the bag.
*/
int bag_count(Bag *p){
  return p->cnt;
}
