#ifndef lint
static char sccsid[] = "@(#)ck_zmalloc.c	3.1 04/03/92 Copyright 1987-1990 Sun Microsystem, Inc." ;
#endif

/*  Copyright (c) 1987, 1988, Sun Microsystems, Inc.  All Rights Reserved.
 *  Sun considers its source code as an unpublished, proprietary
 *  trade secret, and it is available only under strict license
 *  provisions.  This copyright notice is placed here only to protect
 *  Sun in the event the source is deemed a published work.  Dissassembly,
 *  decompilation, or other means of reducing the object code to human
 *  readable form is prohibited by the license agreement under which
 *  this code is provided to the user or company in possession of this
 *  copy.
 *
 *  RESTRICTED RIGHTS LEGEND: Use, duplication, or disclosure by the
 *  Government is subject to restrictions as set forth in subparagraph
 *  (c)(1)(ii) of the Rights in Technical Data and Computer Software
 *  clause at DFARS 52.227-7013 and in similar clauses in the FAR and
 *  NASA FAR Supplement.
 */

/*  Routines taken from V3 maillib/ck_zmalloc.c
 *
 *  ck_zmalloc  - use mmap(2) to allocate memory from /dev/zero
 *  ck_zfree    - use munmap(2) to unmap (free) memory.
 *  ck_mmap     - use mmap(2) to map file for read-only.
 *  ck_unmap    - identical to ck_zfree().
 *
 *  These functions should be better than malloc(3) for large memory
 *  allocation.
 */

#include <stdio.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>

/*  A utility structure to keep track of the (possibly) multiple mmaps
 *  that we have done...
 */

struct buffer_map {
  struct buffer_map *bm_next ;
  char              *bm_buffer ;
  int                bm_size ;
};

static void *bm_empty = (void *) "" ;    /* Special buffer */
static struct buffer_map *bm_list ;      /* NULL by default */

/* For all function declarations, if ANSI then use a prototype. */

#if  defined(__STDC__)
#define P(args)  args
#else  /* ! __STDC__ */
#define P(args)  ()
#endif  /* STDC */

static struct buffer_map *insert_bm P((char *, size_t)) ;
static size_t delete_bm P((char *)) ;


static struct buffer_map *
insert_bm(buf, size)
char *buf ;
size_t size ;
{
  struct buffer_map *bm ;

  bm = (struct buffer_map *) malloc(sizeof (struct buffer_map)) ;
  if (bm != NULL)
    {
      bm->bm_buffer = buf ;
      bm->bm_size   = size ;
      bm->bm_next   = bm_list ;
    }
  bm_list = bm ;
  return(bm_list) ;
}


static size_t
delete_bm(buf)
char *buf ;
{
  size_t size ;
  register struct buffer_map *p_curr ;
  register struct buffer_map *p_prev ;

  p_prev = NULL ;
  p_curr = bm_list ;
  while (p_curr != NULL)
    {
      if (p_curr->bm_buffer == buf)
        {
          if (p_prev == NULL) bm_list         = p_curr->bm_next ;
          else                p_prev->bm_next = p_curr->bm_next ;
          size = p_curr->bm_size ;
          if (p_curr != NULL) free(p_curr) ;
          return(size) ;
        }

      p_prev = p_curr ;
      p_curr = p_curr->bm_next ;
    }
  return(0) ;
}


void *
ck_zmalloc(size)
size_t size ;
{
  int  fd ;
  caddr_t mbuf ;

/* Special case: never allocate 0 bytes, use a special buffer */

  if (size == 0) return(bm_empty) ;

  if ((fd = open("/dev/zero", O_RDWR)) < 0)
    {
      perror("/dev/zero") ;
      return(NULL) ;
    }
  
  mbuf = mmap(0, size, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0) ;
  close(fd) ;

  if (mbuf == (caddr_t) -1)
    {
      perror("ck_zmalloc: mmap") ;
      return ((void *) NULL) ;
    }

  (void) insert_bm(mbuf, size) ;
  return ((void *) mbuf) ;
}


void *
ck_mmap(p_file, p_size)
char *p_file ;
size_t *p_size ;
{
  int fd ;
  caddr_t buf ;
  struct stat sb ;

  if ((fd = open(p_file, O_RDONLY)) < 0)
    return((void *) NULL) ;
  fstat(fd, &sb) ;
  *p_size = sb.st_size ;

  buf = mmap(0, sb.st_size, PROT_READ, MAP_PRIVATE, fd, 0) ;
  close(fd) ;

  if (buf == (caddr_t) -1)
    {
      perror("ck_mmap: mmap") ;
      return((void *) NULL) ;
    }

  (void) insert_bm(buf, sb.st_size) ;

  return((void *) buf) ;
}


void *
ck_zfree(mbuf, used_malloc)
caddr_t mbuf ;
int used_malloc ;
{
  size_t size ;

  if (used_malloc)
    {
      if (mbuf != NULL) (void) free (mbuf) ;
      return(NULL) ;
    }

  if (mbuf == bm_empty) return(NULL) ;

  if (mbuf != NULL)
    {
      if ((size = delete_bm(mbuf)) >= 0)
        {
          if (munmap (mbuf, size) < 0) perror("ck_zfree: munmap") ;
          return(NULL) ;
        }
    }
  return(NULL) ;
}


void *
ck_unmap(buf)
caddr_t buf ;
{
  return(ck_zfree (buf)) ;
}
