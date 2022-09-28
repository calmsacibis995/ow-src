#ident  "@(#)dragdrop.c 3.2 92/12/23 Copyr 1990 Sun Micro"

/*
 * Copyright (c) 1986, 1987, 1988 by Sun Microsystems, Inc.
 */

#include <sys/param.h> /* MAXPATHLEN (include types.h if removed) */
#ifdef SVR4
#include <dirent.h>   /* MAXNAMLEN */
#include <netdb.h>   /* MAXNAMLEN */
#else
#include <sys/dir.h>   /* MAXNAMLEN */
#endif /* SVR4 */
#include <sys/stat.h>

#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <xview/defaults.h>
#include <xview/font.h>
#include <xview/notice.h>
#include <xview/frame.h>
#include <xview/xview.h>
#include <xview/scrollbar.h>
#include <xview/text.h>
#include <xview/panel.h>
#include <xview/canvas.h>
#include <xview/dragdrop.h>
#include <xview/xv_xrect.h>
#include <xview/cursor.h>
#include <xview/cms.h>
#include "colorchooser.h"

#define Min(a,b) (a > b ? b : a)
#define Max(a,b) (a > b ? a : b)

#define POINT_IN_RECT(event, rect) ((event_x(event) > rect->r_left) && \
                                   (event_x(event) < (rect->r_left + rect->r_width)) && \
                                   (event_y(event) > rect->r_top) && \
                                   (event_y(event) < (rect->r_top + rect->r_height)))


/* Temp buffer size for managing INCR transfers thru 
   the selection service */

int	ColorchooserSelectionConvert();

static char		Hostname[MAXHOSTNAMELEN];
static int		debug_on = 0;
static Xv_cursor	drag_cursor;

typedef struct {
	Atom text;
	Atom targets;
	Atom host;
	Atom selection_end;
	Atom data_label;
	Atom load;
	Atom available_types;
	Atom null;
	Atom color;
} Atom_list;

Atom_list	*A;

extern Frame		frame;
extern Panel		panel;
extern Panel_item	preview;

Dnd			Dnd_object;

static char	color_buf[64];

static unsigned short source_ptr_ic_image[]={
#include "source.ptr.icon"
};

static
panel_event_proc(window, event, arg, type)

Xv_opaque               window;
Event                   *event;
Notify_arg              arg;
Notify_event_type       type;

{
       	Xv_Server       server = XV_SERVER_FROM_WINDOW(event_window(event));
	int	status;
	Rect		*swatch_rect = (Rect *) xv_get(preview, XV_RECT);
	Xv_singlecolor	scolor;
	Xv_singlecolor	ccolor;


	if ((event_action(event) == ACTION_SELECT))
	{

		/* A selection action within the canvas might be 
		   the beginning of a drag/drop action, but not 
		   if one is still in progress */

		if (event_is_down(event) && (POINT_IN_RECT(event, swatch_rect)))
		{

			if (debug_on)
				printf("initiating drag/drop operation\n");

			/* set up cursor color */

			current_color(&scolor);

			/* Choose a complimentary color for the background.  
			   This is not necessarily one that looks good, 
			   just one that is guaranteed to contrast. */

			ccolor.red = 255 - scolor.red;
			ccolor.green = 255 - scolor.green;
			ccolor.blue = 255 - scolor.blue;

			xv_set(drag_cursor, CURSOR_FOREGROUND_COLOR, &scolor, 0);
			xv_set(drag_cursor, CURSOR_BACKGROUND_COLOR, &ccolor, 0);

			switch(status = dnd_send_drop(Dnd_object)) {

			    case DND_TIMEOUT:
				xv_set(frame, FRAME_LEFT_FOOTER, MGET( "Drag and Drop: Timed Out" ),0);
				break;
			    case DND_ILLEGAL_TARGET:
				xv_set(frame, FRAME_LEFT_FOOTER,
							MGET( "Drag and Drop: Illegal Target" ),0);
				break;
			    case DND_SELECTION:
				xv_set(frame, FRAME_LEFT_FOOTER,
							MGET( "Drag and Drop: Bad Selection" ),0);
				break;
			    case DND_ROOT:
				xv_set(frame, FRAME_LEFT_FOOTER,
							MGET( "Drag and Drop: Root Window" ),0);
				break;
			    case XV_ERROR:
				xv_set(frame, FRAME_LEFT_FOOTER, MGET( "Drag and Drop: Failed" ),0);
				break;
			}

		}
	}
	return(notify_next_event_func(window, ( Notify_event )event, arg, type));
}


/* The convert proc is called whenever someone makes a request to the dnd
 * selection.  Two cases we handle within the convert proc: DELETE and
 * _SUN_SELECTION_END.  Everything else we pass on to the default convert
 * proc which knows about our selection items.
 */


int
ColorchooserSelectionConvert(seln, type, data, length, format)
    Selection_owner	 seln;
    Atom		*type;
    Xv_opaque		*data;
    long		*length;
    int			*format;
{
    int			length_buf;
    Xv_Server 		server = XV_SERVER_FROM_WINDOW(xv_get(seln, XV_OWNER));
    char		*atom_name;
    static Atom		target_list[10];
    static Atom		types_list[3];


	/* try to turn the type and target atoms into 
	   some useful text for debugging. */

	if (debug_on)
	{
		printf("ColorchooserSelectionConvert conversion called\n");

		if (type)
			atom_name = XGetAtomName( ( Display * )xv_get(server, XV_DISPLAY), *type);
		else
			atom_name = "[None]";

		printf("ColorchooserSelectionConvert, being asked to convert %s\n", atom_name);
	}

    /* Interesting sidelight here.  You cannot simply set the type 
       in the reply to the type requested.  It must be the actual 
       type of the data returned.  HOST_NAME, for example would 
       be returnd as type STRING. */

    if (*type == A->selection_end)
    {
			/* Destination has told us it has completed the drag
			 * and drop transaction.  We should respond with a
			 * zero-length NULL reply.
			 */

	/* Yield ownership of the selection */

	xv_set(Dnd_object, SEL_OWN, False, 0);

	*format = 32;
	*length = 0;
	*data = NULL;
	*type = A->null;
	return(True);
    } 
    else if (*type == A->host)
    {
	/* Return the hostname that the application 
	   is running on */

	*format = 8;
	*length = strlen(Hostname);
	*data = (Xv_opaque) Hostname;
	*type = XA_STRING;
	return(True);
    } 
    else if (*type == A->targets)
    {

	/* This request should return all of the targets 
	   that can be converted on this selection.  This 
	   includes the types, as well as the queries that 
	   can be issued. */

	*format = 32;
	*length = 0;
	*type = XA_ATOM;
	target_list[(*length)++] = XA_STRING;
	target_list[(*length)++] = A->text;
	target_list[(*length)++] = A->targets;
	target_list[(*length)++] = A->host;
	target_list[(*length)++] = A->selection_end;
	target_list[(*length)++] = A->available_types;
	target_list[(*length)++] = A->color;
	target_list[(*length)++] = A->data_label;
	*data = (Xv_opaque) target_list;
	return(True);
    } 
    else if (*type == A->available_types)
    {

	/* This target returns all of the data types that 
	   the holder can convert on the selection. */

	*format = 32;
	*length = 0;
	*type = XA_ATOM;
	types_list[(*length)++] = A->color;
	types_list[(*length)++] = XA_STRING;
	*data = (Xv_opaque) types_list;
	return(True);
    } 
    else if (*type == A->data_label)
    {
	/* Return the hostname that the application 
	   is running on */

	*format = 8;
	*length = strlen("color.def");
	*data = (Xv_opaque) "color.def";
	*type = XA_STRING;
	return(True);
    } 
    else if ((*type == XA_STRING) || (*type == A->text) || (*type == A->color)) 
    {
	/* Return the color currently held */

	Xv_singlecolor	scolor;

	current_color(&scolor);

	sprintf(color_buf, "#%2.2x%2.2x%2.2x", scolor.red, scolor.green, scolor.blue);

	*format = 8;
	*length = strlen(color_buf);
	*data = (Xv_opaque) color_buf;
	*type = A->color;
	return(True);
    } 
    else
    {
	/* Let the default convert procedure deal with the
	 * request.
	 */

	return(sel_convert_proc(seln, type, data, (unsigned long *)length, format));
    }
}

void
init_dragdrop()

{
	Server_image	source_ptr_image;
       	Xv_Server       server = XV_SERVER_FROM_WINDOW(frame);

	gethostname(Hostname, MAXHOSTNAMELEN);

	A = (Atom_list *) calloc(sizeof(Atom_list), 1);

	A->text = xv_get(server, SERVER_ATOM, "TEXT");
	A->targets = xv_get(server, SERVER_ATOM, "TARGETS");
	A->host = xv_get(server, SERVER_ATOM, "_SUN_FILE_HOST_NAME");
	A->selection_end = xv_get(server, SERVER_ATOM, "_SUN_SELECTION_END");
	A->data_label = xv_get(server, SERVER_ATOM, "_SUN_DATA_LABEL");
	A->available_types = xv_get(server, SERVER_ATOM, "_SUN_AVAILABLE_TYPES");

	A->color = xv_get(server, SERVER_ATOM, "_SUN_TYPE_color");
	A->null = xv_get(server, SERVER_ATOM, "NULL");

     	source_ptr_image = xv_create(0, SERVER_IMAGE,
                SERVER_IMAGE_BITS, source_ptr_ic_image,
                SERVER_IMAGE_DEPTH, 1,
                XV_WIDTH, 16,
                XV_HEIGHT, 16,
                0);

	drag_cursor =  xv_create(NULL, CURSOR,
                                 CURSOR_IMAGE, source_ptr_image,
                                 CURSOR_XHOT, 0,
                                 CURSOR_YHOT, 15,
				 CURSOR_OP, PIX_SRC^PIX_DST,
                                 0);

	Dnd_object = xv_create(frame, DRAGDROP,
				SEL_CONVERT_PROC, ColorchooserSelectionConvert,
				DND_TYPE, DND_COPY,
				DND_CURSOR, drag_cursor,
				0);

	notify_interpose_event_func(panel, (Notify_func)panel_event_proc, 
				    NOTIFY_SAFE);
}
