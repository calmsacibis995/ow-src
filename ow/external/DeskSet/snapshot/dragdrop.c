
#ifndef lint
static char sccsid[] = "@(#)dragdrop.c	3.3 06/01/93 Copyright 1987-1990 Sun Microsystem, Inc." ;
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

#include <fcntl.h>
#include "snapshot.h"
#include "xdefs.h"
#include <xview/selection.h>
#include <xview/sel_svc.h>
#include <xview/sel_attrs.h>

#define  TARGET_RESPONSE      0x1
#define  HOST_RESPONSE        0x2
#define  TRANSPORT_RESPONSE   0x4
#define  FILE_NAME_RESPONSE   0x8
#define  DATA_LABEL_RESPONSE  0x10
#define  COUNT_RESPONSE       0x20
#define  TYPES_RESPONSE	      0x40
#define  BYTES_REQUESTED      0x80

#define  RESPONSE_LEVEL_1     0x7f

#define  Max(a, b)            (a > b ? a : b)
#define  Min(a, b)            (a > b ? b : a)
#define  MAXALLOC             10000

DNDVars DND ;
char    dnd_compress_file [MAXPATHLEN];

extern Vars v ;
extern XVars X ;
extern char *hstr[] ;
extern char *mstr[] ;
extern Panel_item print_button;

extern int drop_target_notify_proc P((Panel_item, unsigned int, Event *));

static CHAR_BUF *buf_alloc P(()) ;
static char *have_file_name P(()) ;

static int atom_in_list P((Atom, Atom *, int)) ;
static int buf_append P((CHAR_BUF *, char *, int)) ;
static int buf_free P((CHAR_BUF *)) ;
static int clear_context P((DND_CONTEXT *)) ;
static int get_selection_size P(()) ;
static int init_data_xfer P(()) ;
static int save_new_image P((CHAR_BUF *)) ;
static int snapshot_convert_proc P((Selection_owner, Atom *, Xv_opaque *,
                                    long *, int *)) ;

static unsigned int get_data P((char **)) ;

static void snapshot_reply_proc P((Selection_requestor, Atom, Atom,
                                   Xv_opaque, unsigned long, int)) ;

enum transfer_state  { ST_NONE, ST_DATA } ;

static enum transfer_state t_state ;

int  drag_compressed = FALSE;

static int
atom_in_list(atom, atom_list, length)
Atom atom ;
Atom *atom_list ;
int length ;
{
  int i ;

  if (!atom_list)
    return(False);

  for (i = 0; i < length; i++)
    if (atom_list[i] == atom) return(True) ;

  return(False) ;
}


static CHAR_BUF *
buf_alloc()
{
  CHAR_BUF *ptr ;

  if (!(ptr = (CHAR_BUF *) calloc(1, sizeof(CHAR_BUF)))) return(NULL) ;

  if (!(ptr->data = (char *) malloc(MAXALLOC)))
    {
      FREE(ptr) ;
      return(NULL) ;
    }

  ptr->alloc_size = MAXALLOC ;
  ptr->bytes_used = 0 ;

  return(ptr) ;
}


static int
buf_append(ptr, data, bytes)
CHAR_BUF *ptr ;
char     *data ;
int      bytes ;
{
  int increment ;
  char *new_block ;
 
  if ((ptr->alloc_size - ptr->bytes_used) > bytes)
    {

/* We have enough allocated memory space to just copy the bytes in. */
 
      MEMCPY(&(ptr->data[ptr->bytes_used]), data, bytes) ;
      ptr->bytes_used += bytes ;
      return(0) ;
    }
  else
    {

/* We have to allocate enough memory */
        
      increment = Max(MAXALLOC, bytes) ;
      new_block = (char *) realloc(ptr->data, ptr->alloc_size + increment) ;
      if (!new_block) return(1) ;
      else
        {
          ptr->data = new_block ;
          MEMCPY(&(ptr->data[ptr->bytes_used]), data, bytes) ;
          ptr->bytes_used += bytes ;
          ptr->alloc_size += increment ;

          return(0) ;
        }
    }
}


static int
buf_free(ptr)
CHAR_BUF *ptr ;
{
  if (!ptr) return ;

  if (ptr->data) FREE(ptr->data) ;
  FREE(ptr) ;
}


/* Clears the context block in preparation for a new drag/drop transaction. */

static int
clear_context(ptr)
DND_CONTEXT *ptr ;
{
  if (ptr->target_list)     FREE(ptr->target_list) ;
  ptr->target_list = NULL ;

  ptr->num_targets = 0 ;
  if (ptr->transport_list)  FREE(ptr->transport_list) ;
  ptr->transport_list = NULL ;

  ptr->num_transports = 0 ;
  if (ptr->types_list)  FREE(ptr->types_list) ;
  ptr->types_list = NULL ;

  ptr->num_types = 0 ;

  if (ptr->data_label)      FREE(ptr->data_label) ;
  ptr->data_label = NULL ;
 
  ptr->num_objects = 0 ;
  if (ptr->source_host)     FREE(ptr->source_host) ;
  ptr->source_host = NULL ;
 
  if (ptr->source_filename) FREE(ptr->source_filename) ;
  ptr->source_filename = NULL ;
         
  ptr->chosen_target = 0 ;
  ptr->chosen_transport = 0 ;

  ptr->state_mask        = 0 ;
  ptr->stop_hit          = 0 ;
  ptr->transfer_data     = 0 ;
  ptr->processing_stream = 0 ;
}


int
dragdrop_init()
{
  int panel_width;
  int starting_x;
  int gap;
  Rect *rect;

  DND = (DNDVars) LINT_CAST(malloc(sizeof(DNDObject))) ;

  DND->atom[(int) A_TEXT]     = GET_ATOM("TEXT") ;
  DND->atom[(int) A_INCR]     = GET_ATOM("INCR") ;
  DND->atom[(int) A_TARGETS]  = GET_ATOM("TARGETS") ;
  DND->atom[(int) A_LENGTH]   = GET_ATOM("LENGTH") ;
  DND->atom[(int) A_HOST]     = GET_ATOM("_SUN_FILE_HOST_NAME") ;
  DND->atom[(int) A_FNAME]    = GET_ATOM("FILE_NAME") ;
  DND->atom[(int) A_ATMFNAME] = GET_ATOM("_SUN_ATM_FILE_NAME") ;
  DND->atom[(int) A_DELETE]   = GET_ATOM("DELETE") ;
  DND->atom[(int) A_SELEND]   = GET_ATOM("_SUN_SELECTION_END") ;
  DND->atom[(int) A_DRAGDONE] = GET_ATOM("_SUN_DRAGDROP_DONE") ;
  DND->atom[(int) A_DLABEL]   = GET_ATOM("_SUN_DATA_LABEL") ;
  DND->atom[(int) A_ATRANS]   = GET_ATOM("_SUN_ALTERNATE_TRANSPORT_METHODS") ;
  DND->atom[(int) A_ATYPES]   = GET_ATOM("_SUN_AVAILABLE_TYPES") ;
  DND->atom[(int) A_INSSEL]   = GET_ATOM("INSERT_SELECTION") ;
  DND->atom[(int) A_ENUMCNT]  = GET_ATOM("_SUN_ENUMERATION_COUNT") ;
  DND->atom[(int) A_NULL]     = GET_ATOM("NULL") ;
  DND->atom[(int) A_CURTYPE]  = GET_ATOM("_SUN_TYPE_sun-raster") ;
  DND->atom[(int) A_COMPRESS] = GET_ATOM("_SUN_TYPE_compress") ;

  gethostname(DND->hostname, MAXHOSTNAMELEN) ;
 
  DND->context = (DND_CONTEXT *) malloc(sizeof(DND_CONTEXT)) ;
  memset((char *) DND->context, NULL, sizeof(DND_CONTEXT)) ;
 
  DND->drag_cursor = xv_create(XV_NULL,      CURSOR,
                          CURSOR_IMAGE, X->sv[(int) SV_DRAG_PTR],
                          CURSOR_XHOT,  17,
                          CURSOR_YHOT,  24,
			  CURSOR_OP,	PIX_SRC^PIX_DST,
                          0) ;

  DND->drop_cursor = xv_create(XV_NULL,      CURSOR,
                          CURSOR_IMAGE, X->sv[(int) SV_DROP_PTR],
                          CURSOR_XHOT,  17,
                          CURSOR_YHOT,  24,
			  CURSOR_OP,	PIX_SRC^PIX_DST,
                          0) ;

  DND->object = xv_create(X->panel[(int) P_MAIN], DRAGDROP,
                          SEL_CONVERT_PROC,       snapshot_convert_proc,
                          DND_TYPE,               DND_COPY,
                          DND_CURSOR,             DND->drag_cursor,
			  DND_ACCEPT_CURSOR,	  DND->drop_cursor,
                          0) ;
 
  panel_width = xv_get (X->panel[(int) P_MAIN], XV_WIDTH);
  gap = xv_get (X->panel[(int) P_MAIN], PANEL_ITEM_X_GAP);

  X->pitems [(int) PI_DRAG] = xv_create (X->panel[(int) P_MAIN], 
							PANEL_DROP_TARGET,
                          PANEL_DROP_DND,          DND->object,
			  PANEL_DROP_DND_TYPE,	   PANEL_DROP_COPY_ONLY,
			  PANEL_DROP_SITE_DEFAULT, TRUE,
                          PANEL_NOTIFY_PROC,       drop_target_notify_proc,
                          PANEL_ITEM_Y,      P_ROW(P_MAIN, MR_LSSPD),
                          XV_HELP_DATA,      hstr[(int) H_SOURCE],
                          NULL);

  rect = (Rect *) xv_get (X->pitems [(int) PI_DRAG], PANEL_ITEM_RECT);

  starting_x = xv_get (print_button, XV_X) + xv_get (print_button, XV_WIDTH);

  if ((starting_x + (gap * 2) + rect->r_width) > panel_width) {
     panel_width = starting_x + (gap * 2) + rect->r_width;
     xv_set (X->panel [(int) P_MAIN], XV_WIDTH, panel_width, NULL);
     }

  xv_set (X->pitems [(int) PI_DRAG], 
			PANEL_ITEM_X, panel_width - (gap + rect->r_width), 
			NULL);

  DND->sel = (Selection_requestor) 
			xv_get (X->pitems [(int) PI_DRAG], PANEL_DROP_SEL_REQ) ;

  sprintf (dnd_compress_file, "/tmp/snapshot%d.Z", getpid ());
}


static char *
have_file_name()
{
  if (v->havename) {
     if (v->debug_on) PRINTF("have file name: %s\n",v->path) ;
        return(v->path) ;
   }
  else             return(NULL) ;
}


static int
init_data_xfer()
{
  t_state = ST_NONE ;
}


/*ARGSUSED*/
static unsigned int
get_data(buffer)
char **buffer ;             /* The base address of the buffer to read from */
{
  int rastersize ;

  switch (t_state)
    {
      case ST_NONE    : if (!v->image)
                         {
                           *buffer = NULL ;
                           return(0) ;
                         }
                       savefile(v->path, R_DATA, 0) ;

                       rastersize = get_raster_len(v->image) ;
                       *buffer = (char *) v->rbuf ;
                       t_state = ST_DATA ;
                       return((unsigned int) rastersize) ;

      case ST_DATA    : *buffer = NULL ;
                       return(0) ;
    }
}


static int
save_new_image(image_data)
CHAR_BUF *image_data ;
{
  int load_status;

  if (v->debug_on) PRINTF("save_new_image called\n") ;

  if (drag_compressed == TRUE) {
     int fd;
     int bytes_written;
     if ((fd = open (dnd_compress_file, O_WRONLY | O_CREAT | O_TRUNC, 0644 ))
		== NULL) {
	message (mstr[(int) M_NOTOPEN]) ;
	return ;
	}
     if ((bytes_written = write (fd, DND->context->transfer_data->data,
				     DND->context->transfer_data->bytes_used))
		!= DND->context->transfer_data->bytes_used) {
	message (mstr[(int) M_TFAIL]);
	return;
	}
     load_status = loadfile (dnd_compress_file, R_FILE) ;
     unlink (dnd_compress_file);
     }
  else {
     if (v->rbuf != (unsigned char *) NULL) 
        FREE (v->rbuf);     
     v->rbuf = (unsigned char *) malloc ((unsigned int) image_data->bytes_used);
     memcpy ( (char *) v->rbuf, image_data->data, image_data->bytes_used);
     load_status = loadfile(v->path, R_DATA) ;
     }

  if (load_status == TRUE) {
     v->ls_status = ISDRAG;
     v->havename = FALSE ;
     set_namestripe ();
     }
}


static int
get_selection_size()
{

/*  The transfer length will be the size of an XImage structure plus the
 *  image data. The size of the image data is bytes_per_line * height *
 *  depth.
 */

  if (X->ximage) return(0) ;

  if (v->debug_on)
    PRINTF("get_transfer_length : %d.\n", sizeof(XImage) +
           (X->ximage->bytes_per_line * X->ximage->height * X->ximage->depth)) ;
  return(sizeof(XImage) +
         (X->ximage->bytes_per_line * X->ximage->height * X->ximage->depth)) ;
}


/*ARGSUSED*/
int
load_from_dragdrop(server, event)
Xv_server server ;
Event *event ;
{
  if (v->debug_on) PRINTF("load_from_dragdrop: called\n") ;

/* Clear the left footer for new response status */
 
  XV_SET(X->frames[(int) F_MAIN], FRAME_LEFT_FOOTER, "", 0) ;
 
/*  Display an alert here, asking if the user wants to have the current
 *  view (if any) discarded.  If they don't, then abort the load.
 *
 *  Get target types, and see if we like any of them.
 */
 
  clear_context(DND->context) ;

  XV_SET(DND->sel,
         SEL_REPLY_PROC, snapshot_reply_proc,
         SEL_TYPES,
           DND->atom[(int) A_TARGETS],
           DND->atom[(int) A_HOST],
           DND->atom[(int) A_ATRANS],
           DND->atom[(int) A_FNAME],
           DND->atom[(int) A_DLABEL],
           DND->atom[(int) A_ENUMCNT],
	   DND->atom[(int) A_ATYPES],
           0,
         0) ;

  sel_post_req(DND->sel) ;

  drag_compressed = FALSE;

  if (v->debug_on) PRINTF("load_from_dragdrop: after sel_post_req\n") ;
}


/*  The convert proc is called whenever someone makes a request to the dnd
 *  selection.  Two cases we handle within the convert proc: DELETE and
 *  _SUN_SELECTION_END.  Everything else we pass on to the default convert
 *  proc which knows about our selection items.
 */

static int
snapshot_convert_proc(seln, type, data, length, format)
Selection_owner seln ;
Atom            *type ;
Xv_opaque       *data ;
long            *length ;
int             *format ;
{
  static int  length_buf ;
  char        *atom_name ;
  static int  in_progress = 0;
  static Atom target_list[11] ;
  static Atom types_list[2] ;

/*  Try to turn the type and target atoms into some useful text for
 *  debugging.
 */

  if (in_progress == 0) {
     init_data_xfer();
     in_progress = 1;
     }

  if (v->debug_on)
    {
      PRINTF("snapshot_convert_proc conversion called\n") ;

      if (*type > 0)
        atom_name = XGetAtomName((Display *) xv_get(X->server, XV_DISPLAY),
                                 *type) ;
      else
        atom_name = "[None]" ;
 
      PRINTF("snapshot_convert_proc, being asked to convert %s\n", atom_name) ;
    }       
                
/*  Interesting sidelight here. You cannot simply set the type in the reply
 *  to the type requested. It must be the actual type of the data returned.
 *  HOST_NAME, for example would be returnd as type STRING.
 */

  if ((*type == DND->atom[(int) A_SELEND]) || 
      (*type == DND->atom[(int) A_DRAGDONE]))
    {

/*
 * Reset the in_progress flag for next time.
 */

      in_progress = 0;

/*  Destination has told us it has completed the drag and drop transaction.
 *  We should respond with a zero-length NULL reply.
 *
 *  Yield ownership of the selection.
 */

      XV_SET(DND->object, SEL_OWN, False, 0) ;

      SET_FOOTER(MGET("Drag and Drop: Completed")) ;
      *format = 32 ;
      *length = 0 ;
      *data   = NULL ;
      *type = DND->atom[(int) NULL] ;
      return(True) ;
    }
  else if (*type == DND->atom[(int) A_DELETE])
    {

/*  In our case, we chose not to listen to delete commands on the drag/drop
 *  item. We never mean to do a move, always a copy.
 */

      *format = 32 ;
      *length = 0 ;
      *data = NULL ;
      *type = DND->atom[(int) NULL] ;
      return(True) ;
    }
  else if (*type == DND->atom[(int) A_LENGTH])
    {
      length_buf = get_selection_size() ;
      *format = 32 ;
      *length = 1 ;
      *type = XA_INTEGER ;
      *data = (Xv_opaque) &length_buf ;
      return(True) ;
    }
  else if (*type == DND->atom[(int) A_ENUMCNT])
    {
      length_buf = 1 ;
      *format = 32 ;
      *length = 1 ;
      *type = XA_INTEGER ;
      *data = (Xv_opaque) &length_buf;
      return(True) ;
    }
  else if (*type == DND->atom[(int) A_DLABEL])
    {
      char *file_name;

/*  Return the data label for the snapshot. If the data is already in a file,
 *  the label is the last token in the full pathname. If there is no disk file
 *  that represents this item, it's snapshot.rs.
 */

      *format = 8 ;
      if ((file_name = have_file_name()) != NULL)
        {

/* The length and data should be set up to match the leaf 
 * string of the full pathname of the current image 
 */

          *length = strlen (base_name(file_name)) ;
          *data = (Xv_opaque) base_name(file_name) ;
          *type = XA_STRING ;
          return(True) ;
        }
    }
  else if (*type == DND->atom[(int) A_FNAME])
    {

/*  Return the full pathname to the file that holds the current image.
 *  If there is no file that can be named, then the target is not converted
 *  successfully
 */
      
      if (have_file_name() != NULL)
        {
          *format = 8 ;
          *length = strlen(have_file_name()) ;
          *data = (Xv_opaque) have_file_name() ;
          *type = XA_STRING ;
          return(True) ;
        }
    }
  else if (*type == DND->atom[(int) A_HOST])
    {

/* Return the hostname that the application is running on. */

      *format = 8 ;
      *length = strlen(DND->hostname) ;
      *data = (Xv_opaque) DND->hostname ;
      *type = XA_STRING ;
      return(True) ;
    }
  else if (*type == DND->atom[(int) A_ATRANS])
    {

/*  This request should return all of the alternate transport methods
 *  that the applicatiopn supports.  This conversion is not honored if
 *  the application cannot support any alternate transport method other 
 *  than thru the server. 
 */

      if (have_file_name() != NULL)
        {
          *format = 32 ;
          *length = 1 ;
          *type = XA_ATOM ;
          target_list[0] = DND->atom[(int) A_ATMFNAME] ;
          *data = (Xv_opaque) target_list ;
          return(True) ;
        }
    }
  else if (*type == DND->atom[(int) A_TARGETS])
    {

/*  This request should return all of the targets that can be converted on
 *  this selection. This includes the types, as well as the queries that
 *  can be issued.
 */

      *format = 32 ;
      *length = 10 ;
      *type = XA_ATOM ;
      target_list[0] = XA_STRING ;
      target_list[1] = DND->atom[(int) A_TEXT] ;
      target_list[2] = DND->atom[(int) A_DELETE] ;
      target_list[3] = DND->atom[(int) A_TARGETS] ;
      target_list[4] = DND->atom[(int) A_HOST] ;
      target_list[5] = DND->atom[(int) A_LENGTH] ;
      target_list[6] = DND->atom[(int) A_SELEND] ;
      target_list[7] = DND->atom[(int) A_DRAGDONE] ;
      target_list[8] = DND->atom[(int) A_ATYPES] ;
      target_list[9] = DND->atom[(int) A_CURTYPE] ;
      if (have_file_name() != NULL)
        {
          target_list[*length] = DND->atom[(int) A_FNAME] ;
          length++;
        }
      *data = (Xv_opaque) target_list ;
      return(True) ;
    }
  else if (*type == DND->atom[(int) A_ATYPES])
    {

/*  This target returns all of the data types that the holder can convert on
 *  the selection.
 */

      *format = 32 ;
      *length = 2 ;
      *type = XA_ATOM ;
      types_list[0] = DND->atom[(int) A_CURTYPE] ;
      types_list[1] = XA_STRING ;
      *data = (Xv_opaque) types_list ;
      return(True) ;
    }
  else if ((*type == XA_STRING) ||
           (*type == DND->atom[(int) A_TEXT]) ||
           (*type == DND->atom[(int) A_CURTYPE]))
    {
      if (v->debug_on) PRINTF("Transfer data into buffer.\n") ;
      *format = 8 ;
      *length = get_data((char **) data) ;
      *type = DND->atom[(int) A_CURTYPE] ;
      return (True);
    }    
  else
    {   

/* Let the default convert procedure deal with the request. */

      return(sel_convert_proc(seln, type, data,
                              (unsigned long *) length, format)) ;
    }

    return (False);
}


/*  Huge monolithic routine that processes all the replies to the questions
 *  that I ask about the current drag/drop selection. These requests are made
 *  in groups, and the routine contains a state machine whose current state
 *  is stored in the dnd_context block. This routine updates the state machine
 *  as replies, or failed replies come in. Changes in state may require issuing
 *  new requests for data, which are processed by this same routine.
 */

/*ARGSUSED*/
static void
snapshot_reply_proc(sel_req, target, type, replyBuf, len, format)
Selection_requestor sel_req ;
Atom                target ;
Atom                type ;
Xv_opaque           replyBuf ;
unsigned long       len ;
int                 format ;
{
  DND_CONTEXT *c = DND->context ;
  char  *atom_name ;
  char  *target_name ;
  int   *err_ptr = (int *) replyBuf ;
  char  *char_buf = (char *) replyBuf ;
  Atom  *atom_buf = (Atom *) replyBuf ;
  int   old_length ;
  Event event ;

/* Try to turn type and target atoms into some useful text for debugging. */
 
  if (v->debug_on)
    {
      if (type > 0)
        atom_name = XGetAtomName((Display *) xv_get(X->server, XV_DISPLAY),
                                 type) ;
      else
        atom_name = "[None]" ;

      if (target > 0)
        target_name = XGetAtomName((Display *) xv_get(X->server, XV_DISPLAY),
                                   target) ;
      else
        target_name = "[None]";
    
      PRINTF("entered reply proc, type name  = %s, type atom = %d\n",
             atom_name, type) ;
      PRINTF("target_name = %s, target atom = %d\n",
             target_name, target) ;
      PRINTF("len = %d, format = %d, buf = %d\nstate = %d\n",
             len, format, err_ptr, c->state_mask) ;
    }

/*  Simply processing the return from the termination request.  No action
 *  necessary.
 */

  if (target == DND->atom[(int) A_SELEND]) return ;

  if ((len == SEL_ERROR) && ((*err_ptr) == SEL_BAD_CONVERSION))
    {

/* A conversion of some type failed.  Mark the state variable. */

           if (target == DND->atom[(int) A_TARGETS])
        c->state_mask |= TARGET_RESPONSE ;
      else if (target == DND->atom[(int) A_HOST])
        c->state_mask |= HOST_RESPONSE ;
      else if (target == DND->atom[(int) A_ATRANS])
        c->state_mask |= TRANSPORT_RESPONSE ;
      else if (target == DND->atom[(int) A_FNAME])
        c->state_mask |= FILE_NAME_RESPONSE ;
      else if (target == DND->atom[(int) A_DLABEL])
        c->state_mask |= DATA_LABEL_RESPONSE ;
      else if (target == DND->atom[(int) A_ENUMCNT])
        c->state_mask |= COUNT_RESPONSE ;

      else if ((target == XA_STRING) || (target == DND->atom[(int) A_TEXT]) ||
               (target == DND->atom[(int) A_CURTYPE]) ||
	       (target == DND->atom[(int) A_COMPRESS])) {

/*
 * We got an error while receiving the data. Tell the user the dnd
 * operation failed.
 */
        
        buf_free ( c->transfer_data);
        c->transfer_data = NULL; 
        SET_FOOTER (MGET("Drag and Drop: Data Transfer Failed!"));
	stop_dragdrop ();
	}

    }
  else if (len == SEL_ERROR)
    {

/*  Some internal error happened as a result of an earlier posted request.
 *  Tell the user.
 */

      switch (*err_ptr)
        {
          case SEL_BAD_PROPERTY : SET_FOOTER(MGET("ReplyProc: Bad property!")) ;
                                  break ;
          case SEL_BAD_TIME     : SET_FOOTER(MGET("ReplyProc: Bad time!")) ;
                                  break ;
          case SEL_BAD_WIN_ID   : SET_FOOTER(MGET("ReplyProc: Bad window id!"));
                                  break ;
          case SEL_TIMEDOUT     : SET_FOOTER(MGET("ReplyProc: Timed out!")) ;
                                  break ;

          case SEL_PROPERTY_DELETED :

                              SET_FOOTER(MGET("ReplyProc: Property deleted!")) ;
                              break ;

          case SEL_BAD_PROPERTY_EVENT :

                            SET_FOOTER(MGET("ReplyProc: Bad property event!")) ;
                            break ;
        }
      stop_dragdrop() ;
      return ;
    }
  else if (type == DND->atom[(int) A_INCR]) c->processing_stream = TRUE ;
  else if ((target == XA_STRING) || 
	   (target == DND->atom[(int) A_TEXT]) || 
	   (target == DND->atom[(int) A_CURTYPE]) ||
	   (target == DND->atom[(int) A_COMPRESS]))
    {

/* Data stream coming through. */

      if (len && !c->stop_hit)
        {

/* The length is non-zero, so data, and not the end of transmission. */

          if (!c->transfer_data)
            c->transfer_data = buf_alloc() ;

          if (buf_append(c->transfer_data, char_buf, len))
            {

/* Memory allocation failed. */

              buf_free(c->transfer_data) ;

              c->transfer_data = NULL ;

              NOTICE_PROMPT(X->frames[(int) F_MAIN], &event,
                            NOTICE_MESSAGE_STRINGS,
                              "There was not enough space to make",
                              "the data transfer.  The drop operation",
                              "has been cancelled.",
                              0,
                            NOTICE_BUTTON_NO, "Continue",
                            0) ;

              stop_dragdrop() ;
              return ;
            }

          if (!c->processing_stream)
            {

/* Very unlikely that one buffer will blow the system out. Will not bother
 * to check for memory limit.
 */

              save_new_image(c->transfer_data) ;
              buf_free(c->transfer_data) ;
              c->transfer_data = NULL ;


/*  To complete the drag and drop operation, we tell the source that we are
 *  all done.
 */

              stop_dragdrop() ;
	      xv_set (X->frames[(int) F_MAIN], FRAME_CLOSED, FALSE, NULL);
              return ;
            }
        }
      else if (c->processing_stream)
        {

/* The length was 0, so we have the end of a data transmission. */

          save_new_image(c->transfer_data);

          buf_free(c->transfer_data) ;
          c->transfer_data = NULL ;

/* To complete the drag and drop operation, we tell the source that we are
 * all done.
 */

          stop_dragdrop() ;
	  xv_set (X->frames[(int) F_MAIN], FRAME_CLOSED, FALSE, NULL);
          return ;
        }
      else
        {
          stop_dragdrop() ;
          return ;
        }
    }
  else if (target == DND->atom[(int) A_TARGETS])
    {
      if (len)
        {
          if (c->target_list && !c->processing_stream)
            {
              FREE(c->target_list) ;
              c->target_list = NULL ;
            }

          if (!c->target_list)
            {
              c->target_list = (Atom *) malloc(len * 4) ;
              MEMCPY((char *) c->target_list, (char *) replyBuf, len * 4) ;
              if (!c->processing_stream)
                c->state_mask |= TARGET_RESPONSE ;
            }
          else
            {   
              c->target_list = (Atom *) realloc(c->target_list,
                               c->num_targets * 4 + len * 4) ;
              MEMCPY((char *) &c->target_list[c->num_targets - 1],
                     (char *) replyBuf, len * 4) ;
            }
        }
      else
        {   
          c->state_mask |= TARGET_RESPONSE ;
          c->processing_stream = FALSE ;
        }

      c->num_targets += len ;
    }
  else if (target == DND->atom[(int) A_ATYPES])
    {
      if (len)
        {
          if (c->types_list && !c->processing_stream)
            {
              FREE(c->types_list) ;
              c->types_list = NULL ;
            }

          if (!c->types_list)
            {
              c->types_list = (Atom *) malloc(len * 4) ;
              MEMCPY((char *) c->types_list, (char *) replyBuf, len * 4) ;
              if (!c->processing_stream)
                c->state_mask |= TYPES_RESPONSE ;
            }
          else
            {   
              c->types_list = (Atom *) realloc(c->types_list,
                               c->num_types * 4 + len * 4) ;
              MEMCPY((char *) &c->types_list[c->num_types - 1],
                     (char *) replyBuf, len * 4) ;
            }
        }
      else
        {   
          c->state_mask |= TYPES_RESPONSE ;
          c->processing_stream = FALSE ;
        }

      c->num_types += len ;
      if (atom_in_list(DND->atom[(int) A_COMPRESS], c->types_list,
						    c->num_types))
	 drag_compressed = TRUE;

    }
  else if (target == DND->atom[(int) A_ATRANS])
    {
      if (len)
        {
          if (c->transport_list && !c->processing_stream)
            {
              FREE(c->transport_list) ;
              c->transport_list = NULL ;
            }

          if (!c->transport_list)
            {
              c->transport_list = (Atom *) malloc(len * 4) ;
              MEMCPY((char *) c->transport_list, (char *) replyBuf, len * 4) ;
              if (!c->processing_stream)
                c->state_mask |= TRANSPORT_RESPONSE ;
            }
          else
            {   
              c->transport_list = (Atom *) realloc(c->transport_list,
                                           c->num_transports * 4 + len * 4) ;
              MEMCPY((char *) &c->transport_list[c->num_transports - 1],
                     (char *) replyBuf, len * 4) ;
            }
        }
      else
        {   
          c->state_mask |= TRANSPORT_RESPONSE ;
          c->processing_stream = FALSE ;
        }

      c->num_transports += len ;
    }
  else if (target == DND->atom[(int) A_HOST])
    {
      if (len)
        {
          if (c->source_host && !c->processing_stream)
            {
              FREE(c->source_host) ;
              c->source_host = NULL ;
            }

          if (!c->source_host)
            {
              c->source_host = malloc(len + 1) ;
              MEMCPY(c->source_host, (char *) replyBuf, len) ;
              c->source_host[len] = NULL ;
              if (!c->processing_stream)
                c->state_mask |= HOST_RESPONSE ;
            }
          else
            {   
              old_length = strlen(c->source_host) ;
              c->source_host = (char *) realloc(c->source_host,
                                                old_length + len + 1) ;
              MEMCPY(&c->source_host[old_length], (char *) replyBuf, len) ;
              c->source_host[old_length + len] = NULL ;
            }
        }
      else
        {   
          c->state_mask |= HOST_RESPONSE ;
          c->processing_stream = FALSE ;
        }
    }
  else if (target == DND->atom[(int) A_FNAME])
    {
      if (len)
        {
          if (c->source_filename && !c->processing_stream)
            {
              FREE(c->source_filename) ;
              c->source_filename = NULL ;
            }

          if (!c->source_filename)
            {
              c->source_filename = malloc(len + 1) ;
              MEMCPY(c->source_filename, (char *) replyBuf, len) ;
              c->source_filename[len] = NULL ;
              if (!c->processing_stream)
                c->state_mask |= FILE_NAME_RESPONSE ;
            }
          else
            {   
              old_length = strlen(c->source_filename) ;
              c->source_filename = (char *)
                realloc(c->source_filename, old_length + len + 1) ;
              MEMCPY(&c->source_filename[old_length], (char *) replyBuf, len) ;
              c->source_filename[old_length + len] = NULL ;
            }
        }
      else
        {   
          c->state_mask |= FILE_NAME_RESPONSE ;
          c->processing_stream = FALSE ;
        }
    }
  else if (target == DND->atom[(int) A_DLABEL])
    {
      if (len)
        {
          if (c->data_label && !c->processing_stream)
            {
              FREE(c->data_label) ;
              c->data_label = NULL ;
            }

          if (!c->data_label)
            {
              c->data_label = malloc(len + 1) ;
              MEMCPY(c->data_label, (char *) replyBuf, len) ;
              c->data_label[len] = NULL ;
              if (!c->processing_stream)
                c->state_mask |= DATA_LABEL_RESPONSE ;
            }
          else
            {   
              old_length = strlen(c->data_label) ;
              c->data_label = (char *)
                realloc(c->data_label, old_length + len + 1) ;
              MEMCPY(&c->data_label[old_length], (char *) replyBuf, len) ;
              c->data_label[old_length + len] = NULL ;
            }
        }
      else
        {   
          c->state_mask |= DATA_LABEL_RESPONSE ;
          c->processing_stream = FALSE ;
        }
    }
  else if (target == DND->atom[(int) A_ENUMCNT])
    {
      c->num_objects = atom_buf[0] ;
      c->state_mask |= COUNT_RESPONSE ;
    }
  else return ;

  if (c->state_mask == RESPONSE_LEVEL_1)
    {
      if (v->debug_on)
        PRINTF("first batch of replies processed, asking for second\n") ;

      if (c->num_objects > 1)
        {
          NOTICE_PROMPT(X->frames[(int) F_MAIN], &event,
                        NOTICE_MESSAGE_STRINGS,
                          "Snapshot cannot handle multiple",
                          "files at once.  Please select one file,",
                          "and try again.",
                          0,
                        NOTICE_BUTTON_NO, "Continue",
                        0) ;

          stop_dragdrop() ;
          return ;
        }

      if ((!atom_in_list(XA_STRING, c->target_list,
                                    c->num_targets)) &&
          (!atom_in_list(DND->atom[(int) A_ATMFNAME], c->transport_list,
                                             c->num_targets)) &&
          (!atom_in_list(DND->atom[(int) A_TEXT], c->target_list,
                                    c->num_targets)))
        {  
          NOTICE_PROMPT(X->frames[(int) F_MAIN], &event,
                        NOTICE_MESSAGE_STRINGS,
                          "The sourcing application cannot",
                          "send data that snapshot can operate on.",
                          "The drop operation has been cancelled.",
                          0,
                        NOTICE_BUTTON_NO, "Continue",
                        0) ;

          stop_dragdrop() ;
          return ;
        }

      if (v->debug_on) PRINTF("Before atom in list XA_STRING check.\n") ;
      if (!atom_in_list(XA_STRING, c->target_list, c->num_targets))
        {
          if (v->debug_on)
            PRINTF("Chosen_target getting set to TEXT atom.\n") ;
          c->chosen_target = DND->atom[(int) A_TEXT] ;
        }
      else
        {
          if (v->debug_on)
            PRINTF("Chosen_target getting set to XA_STRING.\n") ;
          c->chosen_target = XA_STRING ;
        }

/*  Determine what sort of data we have coming.
 *  Get the host name to go with the file name.
 */

      if (atom_in_list(DND->atom[(int) A_ATMFNAME], c->transport_list,
          c->num_transports) && c->source_filename)
        {
          if (!strcmp(DND->hostname, c->source_host))
            {

/* Life is hunky dory.  Use the file name in place. */

              STRCPY(v->path, c->source_filename) ;
              if (loadfile(v->path, R_FILE) == TRUE)
	         v->ls_status = ISLOAD;

              if (v->debug_on)
                PRINTF("Doing loadfile on %s\n", c->source_filename) ;

/*  To complete the drag and drop operation, we tell the source that we are
 *  all done.
 */
              stop_dragdrop() ;
	      xv_set (X->frames[(int) F_MAIN], FRAME_CLOSED, FALSE, NULL);
              return ;
            }
        }

/* Else the data is just a copy.  Set the name to [No File]. */

      STRCPY(DND->filename, "(NONE)") ;
  
/* Read the data stream. */

      XV_SET(DND->sel,
                 SEL_REPLY_PROC, snapshot_reply_proc,
                 SEL_TYPES,      c->chosen_target, 0,
                 0) ;
  
      c->state_mask |= BYTES_REQUESTED ;
      sel_post_req(DND->sel) ;

    }
}


/*  Stop the drag and drop operation.  Converts _SUN_SELECTION_END on the
 *  current selection, signaling the end, and then puts the rest of the
 *  application back to normal.
 */

void
stop_dragdrop()
{
  if (v->debug_on) PRINTF("stop_dragdrop called\n") ;

/* Signal termination of transaction. */

  XV_SET(DND->sel,
         SEL_REPLY_PROC, snapshot_reply_proc,
         SEL_TYPES,      DND->atom[(int) A_SELEND], 0,
         0) ;

  sel_post_req(DND->sel) ;

/* Free up any outstanding transfer data. */

  if (DND->context->transfer_data)
    {
      buf_free(DND->context->transfer_data) ;
      DND->context->transfer_data = NULL ;
    }
}
