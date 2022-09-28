
/*  @(#)xdefs.h	3.1 04/03/92
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

#include <netdb.h>
#include <xview/xview.h>
#include <xview/panel.h>
#include <xview/cms.h>
#include <xview/cursor.h>
#include <xview/dragdrop.h>
#include <xview/fullscreen.h>
#include <xview/font.h>
#include <xview/notice.h>
#include <xview/xv_xrect.h>
#include <X11/Xlib.h>

#define  NOTICE_PROMPT   (void) notice_prompt     /* To make lint happy. */
#define  XV_CREATE       (void) xv_create
#define  XV_DESTROY      (void) xv_destroy
#define  XV_SET          (void) xv_set

#define  GET_ATOM(str)        xv_get(X->server, SERVER_ATOM, str)
#define  MAXATOMS             19    /* Maximum number of drag-n-drop atoms. */
#define  POINT_IN_RECT(e, r)  ((event_x(e) > r->r_left)                && \
                              (event_x(e) < (r->r_left + r->r_width)) && \
                              (event_y(e) > r->r_top)                 && \
                              (event_y(e) < (r->r_top + r->r_height)))

#define  SET_FOOTER(str)      XV_SET(X->frames[(int) F_MAIN],         \
                                     FRAME_LEFT_FOOTER, str, 0)

#define  XFER_BUFSIZ          5000

/* Atom types. */
enum atom_type  { A_TEXT,    A_INCR,    A_TARGETS,  A_LENGTH,
                  A_HOST,    A_FNAME,   A_ATMFNAME, A_DELETE,
                  A_SELEND,  A_DLABEL,  A_ATRANS,   A_ATYPES,
                  A_INSSEL,  A_ENUMCNT, A_NULL,     A_CURTYPE,
                  A_CURLINK, A_DRAGDONE, A_COMPRESS } ;

typedef struct Xobject {    /* XView/Xlib graphics object. */
  Canvas       canvas ;
  Cms          cms ;
  Colormap     cmap ;
  Colormap     xcmap ;
  Frame        frames[MAXFRAMES] ;
  Fullscreen   fs ;
  Icon         frame_icon ;
  Panel        panel[MAXPANELS] ;
  Panel_item   pitems[MAXPITEMS] ;
  Rect         rect[MAXRECTS] ;
  Server_image sv[MAXSVS] ;
  Xv_Cursor    cursors[MAXCURS] ;
  Xv_Server    server ;

  Pixfont      *font ;

  int handle ;

  Display *dpy ;                      /* Display ids of snapshot frames. */
  Drawable rxid ;                     /* Xid for root window. */
  Drawable vxid ;                     /* Xid for view window. */
  GC rootgc ;                         /* Root window graphics context. */
  GC viewgc ;                         /* View window graphics context. */
  Pixmap pixmap ;
  Visual *visual ;
  XVisualInfo *vlist ;		      /* List of visuals. */
  Window root ;
  XGCValues gc_val ;                  /* To setup graphics context values. */
  XImage *ximage ;                    /* Contents of current snapshot. */
  int gc_flags ;                      /* To set up graphics context flags. */
  int screen ;                        /* Default graphics display screen. */
  unsigned long backgnd ;             /* Default background color. */
  unsigned long foregnd ;             /* Default foreground color. */
  unsigned long gc_mask ;             /* Mask for graphic context values. */
  unsigned long palette[CMAP_SIZE] ;  /* Xlib color palette. */
} XObject ;

typedef struct Xobject *XVars ;

typedef struct {
  char *data ;
  int  alloc_size ;
  int  bytes_used ;
} CHAR_BUF ;
 
/* Context structure for describing the current drag/drop action. */
 
typedef struct {
  Atom     *target_list ;
  int      num_targets ;
  Atom     *transport_list ;
  int      num_transports ;
  Atom     *types_list ;
  int	   num_types ;
  char     *data_label ;
  int      num_objects ;
  char     *source_host ;
  char     *source_filename ;
  Atom     chosen_target ;
  Atom     chosen_transport ;
  int      transfer_state ;
  int      processing_stream ;
  int      state_mask ;
  int      stop_hit ;
  CHAR_BUF *transfer_data ;
} DND_CONTEXT ;

typedef struct DNDobject {      /* Drag-n-drop object. */
  Atom                atom[MAXATOMS] ;
  Dnd                 object ;
  DND_CONTEXT         *context ;
  Selection_requestor sel ;
  Xv_Cursor           drag_cursor ;
  Xv_Cursor	      drop_cursor;
  char                filename[MAXPATHLEN] ;
  char                hostname[MAXHOSTNAMELEN] ;
  char                xfer_data_buf[XFER_BUFSIZ] ;
} DNDObject ;
 
typedef struct DNDobject *DNDVars ;

void drawbox P((Rect *)) ;

XImage *gen_image P((image_t *)) ;
