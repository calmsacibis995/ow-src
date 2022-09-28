
/*  @(#)snapshot.h	2.24 5/23/91
 *
 *  Copyright (c) 1987, 1988, Sun Microsystems, Inc.  All Rights Reserved.
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

#include <stdio.h>
#include <sys/param.h>
#include <memory.h>
#include <sys/time.h>
#include <locale.h>

#define  MSGFILE		"SUNW_DESKSET_SNAPSHOT_MSG"
#define  EGET(s)         (char *) dgettext("SUNW_DESKSET_SNAPSHOT_ERR",   s)
#define  LGET(s)         (char *) dgettext("SUNW_DESKSET_SNAPSHOT_LABEL",   s)
#define  MGET(s)         (char *) dgettext("SUNW_DESKSET_SNAPSHOT_MSG", s)
#define  DGET(s)         MGET(s)

#define  FCLOSE          (void) fclose
#define  FPRINTF         (void) fprintf
#define  FREE            (void) free
#define  GETCWD          (void) getcwd
#define  MEMCPY          (void) memcpy
#define  PCLOSE          (void) pclose
#define  PRINTF          (void) printf
#define  SELECT          (void) select
#define  SPRINTF         (void) sprintf
#define  STRCAT          (void) strcat
#define  STRCPY          (void) strcpy
#define  STRNCPY         (void) strncpy
#define  UNLINK          (void) unlink

#ifndef  FALSE             /* Boolean definitions. */
#define  FALSE      0
#endif /*FALSE*/

#ifndef  TRUE
#define  TRUE       1
#endif /*TRUE*/

typedef struct {              /* Description of rasterfile image. */
  int type ;                  /* Type of image. */
  int width ;                 /* Width in pixels. */
  int height ;                /* Height in pixels. */
  int depth ;                 /* Depth of image. */
  int cmaptype ;              /* Colormap type. */
  int cmapused ;              /* Number of entries used in colormap. */
  int bytes_per_line ;        /* Accelerator to the next image line. */
  int length ;		      /* length of data. */
  unsigned char *red ;        /* Red colormap entries. */
  unsigned char *green ;      /* Green colormap entries. */
  unsigned char *blue ;       /* Blue colormap entries. */
  unsigned char *data ;       /* Image data. */
} image_t ;

extern image_t *rast_load (), *new_image ();
