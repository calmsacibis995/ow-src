#ifndef lint 
#ifdef sccs
static char     sccsid[] = "@(#)xv_slots.c 1.4 94/04/13";
#endif
#endif 
 
/* 
 *      (c) Copyright 1989 Sun Microsystems, Inc. Sun design patents 
 *      pending in the U.S. and foreign countries. See LEGAL NOTICE 
 *      file for terms of the license. 
 */ 

/*
  
  Name:		xv_slots.c

  Description:

    These routines use user-suppplied contiguous memory
    to store 32 bits of data with 32 bits of key.  Note that
    key 0 is not a legal key.  This can be changed by defining
    FREE_KEY.

    Note that since these routines do not allocate
    memory or use ptrs, they are suitable for
    use in shared memory.  They are currently limited to 
    max_u_short elements.

    These routines manage a hashed heap.

  Usage:

     unsigned int 	xv_slots_bytes	(unsigned int num_elements);

    This routines returns the number of bytes required to store the
    specifed number of elements.  Returns 0 if num_elements > MAX_U_SHORT.

    
     unsigned int 	xv_slots_init	(void * ptr, unsigned int size);

    This routines initializes the supplied memory area as a slot table.
    The size of the table in bytes must be specified.  Note that the
    ptr must be word aligned.  The routine returns the number of
    slots available, or zero if the specified space would hold more
    than  MAX_U_SHORT elements.

     unsigned int * 	xv_slots_get	(void * ptr, unsigned int key);

    This routine attempts to find a slot in the table with the specified
    key.  It creates the slot if it is not found.  The routine returns a
    pointer to the data item associated with the specified key, or NULL if
    the key doesn't exist and no more room is left in the table.

     unsigned int * 	xv_slots_find	(void * ptr, unsigned int key);

    Slots_find attempts to find a slot in the table with the specified
    key.  It returns a pointer to the data item if the key was found,
    else NULL.

     unsigned int 	xv_slots_delete	(void * ptr, unsigned int key);

    The routine deletes the specified key.  It returns the data associated with
    the specified key, or 0 if the key wasn't found.  Caution: this can be
    ambigous.

     unsigned int	        xv_slots_copy	(void * new, unsigned int newsize, void * old);

    This routine allows one to copy an existing table to a new one.  Newsize is the
    number of bytes available in the area pointed to by new, while old is the address
    of the existing table.  The routine returns the number of elements copied,
    or -1 if insufficient room exists.


    Bart Smaalders

  */

#include <sys/types.h>
#include <values.h>
#include <assert.h>
#include <xview_private/xv_slots.h>

static char addrnum;
#define FREE_KEY 	((unsigned int)(&addrnum))


typedef struct d_block {
  u_int key;		/* storage key 		*/
  u_int data;		/* data assoc. with key */
  u_short first;	/* points to first key,data
			   to hash to this spot */
  u_short next;		/* points to next key,data
			   in _this_ chain */
} d_block_t;


u_int xv_slots_bytes(u_int num_elements)
{
  if(num_elements > (((u_short)-1)-1))
    return(0);
  else
    return(sizeof(d_block_t)*(num_elements + 1));
}

u_int xv_slots_init(void * ptr, u_int size)
{
  u_int  num_elements;
  u_short i;
  d_block_t * start, * tmp;

  num_elements = (size / sizeof(d_block_t));
  
  if(num_elements > (u_short) -1)
    return(0);

  start = (d_block_t *)ptr;

  for(tmp=start, i=1;i<=num_elements;i++, tmp++) {
    tmp->key = FREE_KEY;
    tmp->first = 0;
    tmp->next = i;	/* build free list */
  }

  start->key = 0; /* num elements now stored */
  start->first = num_elements-1; /* max size in elements*/
  start->data = size;	/* save number of bytes 	*/
  
  tmp--;
  tmp->next = 0;
  return(num_elements - 1);

}

u_int * xv_slots_get(void * ptr, u_int key)
{
  register u_short bucket, next;
  register u_short * add_ptr;

  register d_block_t * blks = (d_block_t *)ptr;

  if(key == FREE_KEY)
    return(NULL);

  bucket = (key%(((d_block_t *)ptr)->first)) + 1;

  if(blks[bucket].first == 0) {
    add_ptr = & blks[bucket].first;
    goto add;
  }
  else {
    register d_block_t * dptr; 

    dptr = blks + blks[bucket].first;

    while(key != dptr->key) {
      if((next = dptr->next)==NULL) {
	add_ptr = & dptr->next;
	goto add;
      }
      dptr = blks + next;
    }
    return(& dptr->data);
  }

 add:

  if((*add_ptr = next = blks->next)==NULL)
    return(NULL); /* no more room */

  /*
    remove new slot from free list 
    */

  blks->next = blks[next].next;
  blks->key++;

  {
    register d_block_t * dptr = blks + next; 
    
    dptr->key = key;
    dptr->next = 0;
    dptr->data = 0;

    return(&dptr->data);
  }
}
  
u_int * xv_slots_find(void * ptr, u_int key)
{
  u_short size;
  u_short bucket;

  register d_block_t * blks = (d_block_t *)ptr;

  size = ((d_block_t *)ptr)->first;
  
  bucket = blks[(key%size) + 1].first;

  if(bucket == 0) {
    return(NULL);
  }
  else {
    register d_block_t * dptr;

    dptr = blks+bucket;

    while(key != dptr->key) {
      if(dptr->next==NULL) {
	return(NULL);
      }
      dptr = blks + dptr->next;
    }
    return(& dptr->data);
  }

}

u_int xv_slots_delete(void * ptr, u_int key)
{
  u_short size;
  register u_short bucket, next;
  register u_short * patch;

  register d_block_t * blks = (d_block_t *)ptr;

  size = ((d_block_t *)ptr)->first;

  patch = & blks[key%size + 1].first;
  bucket = *patch;

  if(bucket == 0) {
    return(NULL);
  }
  else {
    while(key != blks[bucket].key) {
      if((next = blks[bucket].next)==NULL) {
	return(NULL);
      }
      patch = & blks[bucket].next;
      bucket = next;
    }
    /* fix up ptr to this element in chain */

    *patch = blks[bucket].next;

    /* place on 0th element free list */

    blks[bucket].next = blks->next;
    blks->next = bucket;
    blks->key--;
    return(blks[bucket].data);
  }
}

unsigned int xv_slots_copy(void * new, unsigned int newsize, void * old)
{
  register d_block_t * blks = (d_block_t *) old;

  u_int num_elements = (newsize / sizeof(d_block_t));
  u_int i, num;

  if(num_elements-1 < blks->key) /* not enough room */
    return(-1);

  xv_slots_init(new, newsize);

  num = blks->first;

  {
    d_block_t * t = blks + 1;

    for(i=1;i<=num;i++, t++) {
      if(t->key != FREE_KEY) 
	*xv_slots_get(new, t->key) = t->data;
    }
  }
  
}

#ifdef TEST_SLOTS

#include <stdio.h>
#include <stdlib.h>

#define SIZE  1000
#define SIZE2 11111

u_int keys[SIZE];

/* ARGSUSED */
main(int argc, char ** argv)
{
  u_int i,j,n;

  u_int len = xv_slots_bytes(SIZE);
  u_int len2 = xv_slots_bytes(SIZE2);


  struct timeval t1,t2;


  void * f = malloc(len);
  void * f2 = malloc(len2);

  for(i=0;i<SIZE;i++)
    keys[i] = (1+i) * 51;

  gettimeofday(&t1,NULL);
  n = xv_slots_init(f,len);
  gettimeofday(&t2,NULL);
  
  printf("First tests conducted using %d elements\n", SIZE);

  prtdiff("Initialized elements in %lf seconds\n", &t1, &t2);

  gettimeofday(&t1,NULL);
  for(i=0;i<SIZE;i++) {
    *xv_slots_get(f, keys[i]) = i;
  }

  gettimeofday(&t2,NULL);

  prtdiff("filled table using xv_slots_get in %lf seconds\n", &t1, &t2);

  gettimeofday(&t1,NULL);
  for(i=0;i<SIZE;i++)
    if(*xv_slots_get(f, keys[i]) != i)
      abort();
  gettimeofday(&t2,NULL);

  prtdiff("checked table using xv_slots_get in %lf seconds\n", &t1, &t2);

  gettimeofday(&t1,NULL);
  for(i=0;i<SIZE;i++)
    if(*xv_slots_find(f, keys[i]) != i)
      abort();
  gettimeofday(&t2,NULL);

  prtdiff("checked table using xv_slots_find in %lf seconds\n", &t1, &t2);

  printf("subsequent tests use larger table of size %d\n", SIZE2);

  gettimeofday(&t1,NULL);
  xv_slots_copy(f2, len2, f);
  gettimeofday(&t2,NULL);

  prtdiff("copied table to using xv_slots_copy in %lf seconds\n", &t1, &t2);

  gettimeofday(&t1,NULL);

  for(i=0;i<SIZE;i++)
    if(*xv_slots_find(f2, keys[i]) != i)
      abort();

  gettimeofday(&t2,NULL);

  prtdiff("checked larger table using xv_slots_find in %lf seconds\n", &t1, &t2);

  gettimeofday(&t1,NULL);
  for(i=0;i<SIZE;i++)
    if(xv_slots_delete(f, keys[i]) != i)
      abort();
  gettimeofday(&t2,NULL);
  
  prtdiff("emptied small table using xv_slots_delete in %lf seconds\n", &t1, &t2);

  gettimeofday(&t1,NULL);
  for(i=0;i<SIZE;i++)
    if(xv_slots_delete(f2, keys[i]) != i)
      abort();
  gettimeofday(&t2,NULL);
  
  prtdiff("emptied large table using xv_slots_delete in %lf seconds\n", &t1, &t2);
  
}

prtdiff(char * s, struct timeval * before, struct timeval * after)
{
  int usecs = (after->tv_sec - before->tv_sec)*1000000 +
    after->tv_usec - before->tv_usec;

  fprintf(stdout, s, (double)usecs/1000000.);
}






#endif TEST





