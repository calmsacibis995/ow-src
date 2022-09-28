char	copyrite[] = 
"This file is a product of Sun Microsystems, Inc. and is provided for \n\
unrestricted use provided that this legend is included on all tape media \n\
and as a part of the software program in whole or part. Users may copy, \n\
modify or distribute this file at will. \n\
 \n\
THIS FILE IS PROVIDED AS IS WITH NO WARRANTIES OF ANY KIND INCLUDING THE \n\
WARRANTIES OF DESIGN, MERCHANTIBILITY AND FITNESS FOR A PARTICULAR \n\
PURPOSE, OR ARISING FROM A COURSE OF DEALING, USAGE OR TRADE PRAC- TICE. \n\
 \n\
This file is provided with no support and without any obligation on the part \n\
of Sun Microsystems, Inc. to assist in its use, correction, modification \n\
or enhancement. \n\
 \n\
SUN MICROSYSTEMS, INC. SHALL HAVE NO LIABILITY WITH RESPECT TO THE \n\
INFRINGEMENT OF COPYRIGHTS, TRADE SECRETS OR ANY PATENTS BY THIS FILE OR \n\
ANY PART THEREOF. \n\
 \n\
In no event will Sun Microsystems, Inc. be liable for any lost revenue or \n\
profits or other special, indirect and consequential damages, even if Sun \n\
has been advised of the possibility of such damages. \n\
 \n\
Sun Microsystems, Inc. 2550 Garcia Avenue Mountain View, California 94043 \n\
 \n\
Copyright (C) 1991 by Sun Microsystems. All rights reserved. \n\
";

/* cc -o xview_dnd2 xview_dnd2.c -I$(OPENWINHOME)/include -lxview -lolgx -lX11      */
/* if you use /usr/5bin/cc on 4.x.y you need to add the  -DSVR4 flag  */


#define MAX_ATOM_NAME_LEN 256

#include <sys/file.h>
#include <sys/param.h>
#include <xview/xview.h>
#include <xview/textsw.h>
#include <xview/panel.h>
#include <xview/dragdrop.h>	/* Must be included to use the drag and drop
				 * package */
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

char	*find_config_file(char *file_name, char *env_var, char *arg0);
char	*search_for(char *file, ...);

int             natoms;
char		*filename = NULL;
extern char	*optarg;
extern int      optind, opterr;

Xv_opaque       frame;
Xv_opaque       panel;
Xv_opaque       textpane;
Xv_opaque       drag_object;
Xv_Server       My_server;
Display		*dpy;

typedef	struct	sel_request
{
	char	*name;
	Atom	atom;
	int	length;
	int	format;
	void	*data;
} Request;

/*VARGGS2*/
tprintf(Xv_opaque textsw, char *format, ...)
{
	char	buffer[2000];
	va_list	ap;


	va_start(ap, format);
	vsprintf(buffer, format, ap);
	va_end(ap);

	textsw_insert(textsw, buffer, strlen(buffer));
}

get_info(sel, req)
Selection_requestor 	sel;
Request			*req;
{
	char	*name;

	if(req->name == NULL && req->atom == NULL)
	{
		return(FALSE);
	}

	if(req->name == NULL)
	{
		fflush(stdout);
		name = XGetAtomName(dpy, req->atom);
		req->name = strdup(name);
		XFree(name);		
	}
	
	if(req->atom == NULL)
	{
		req->atom = xv_get(My_server, SERVER_ATOM, req->name);
	}

	if(strcmp(req->name, "DELETE") == 0 ||
	   strcmp(req->name, "_SUN_DRAGDROP_DONE") == 0 ||
	   strcmp(req->name, "_SUN_SELECTION_END") == 0)
	{
		req->length = 0;
		req->format = 32;
		return(TRUE);
	}

	xv_set(sel, SEL_TYPE, req->atom, NULL);
	req->data = (void *)xv_get(sel, SEL_DATA, &req->length, &req->format);
	if (req->length == SEL_ERROR || req->data == NULL)
	{
		return(FALSE);
	}
	else
	{
		return(TRUE);
	}
}

/*
 * If the data is one of the "list" type atoms print the names, otherwise return
 * and let the calling func print the data.
 */
a_list(req)
Request	*req;
{
	int	i;
	char	*name;
	char	buffer[200];
	Atom	cur_atom;

	if(!strcmp(req->name, "TARGETS") ||
	   !strcmp(req->name, "_SUN_AVAILABLE_TYPES") ||
	   !strcmp(req->name, "_SUN_ALTERNATE_TRANSPORT_METHODS"))
	{
		for(i = 0; i < req->length; i++)
		{
			name = XGetAtomName(dpy, ((Atom *)req->data)[i]);
			tprintf(textpane, "%s%s",
					name,
					(i+1 != req->length)?", ":"\n");
			XFree(name);		
		}
		return(TRUE);
	}
	else
	{
		return(FALSE);
	}
}

/*
 * Display appropriate information to the text subwindow
 * regarding type of data
 */
void
display_info(req)
Request	*req;
{
	unsigned short		*fmt16;
	unsigned long		*fmt32;
	int			i;
	char			buffer[1000];

	tprintf(textpane, "\n%s\n", req->name);

	tprintf(textpane, "\tlength = %ld\n", req->length);

	tprintf(textpane, "\tformat = %d\n", req->format);

	tprintf(textpane, "\tdata:");
	if(req->length < 0)
	{
		tprintf(textpane, "ERROR\n");
		return;
	}
	switch(req->format)
	{
	case	0:
	case	8:
		textsw_insert(textpane, req->data,
			(req->length < 240)?req->length:240);
		break;
	case	16:
		fmt16 = (unsigned short *)req->data;
		for(i = 0; i < req->length; i++)
		{
			tprintf(textpane, "%04X ", fmt16[i]);
		}
		break;
	case	32:
		fmt32 = (unsigned long *)req->data;
		if(!a_list(req))
		{
			for(i = 0; i < req->length && i < 240; i++)
			{
				tprintf(textpane, "%08X ", fmt32[i]);
			}
		}
		break;
	default:
		tprintf(textpane, "Unknown format %d", req->format);
	}
	textsw_insert(textpane, "\n", 1);
}

/*
 * Do this when receive a drop
 */
static void
drop_proc(item, value, event)
Xv_opaque       item;
unsigned int    value;
Event          *event;
{
	Selection_requestor sel;
	Request	req;
	Request	targets;
	Request	*target_list;
	Request	*t_item;
	char		atom_name[MAX_ATOM_NAME_LEN];
	char		buffer[1024];
	int		i, j, k;
	FILE		*fd, *fopen();
	int		found;
	int		old_context;

	/*
	 * allocate memory
         */
	memset(&req, '\0', sizeof(req));
	memset(&targets, '\0', sizeof(targets));

	/* 
	 * Get pointer to the drop selection
         */
	sel = xv_get(item, PANEL_DROP_SEL_REQ);

	old_context = xv_get(textpane, TEXTSW_LOWER_CONTEXT);
	xv_set(textpane, TEXTSW_LOWER_CONTEXT, -1, 0);

	/*
         * Reset the text subwindow
         */
	textsw_reset(textpane, 0, 0);

	/* 
	 * Perform the appropriate action
	 */
	switch (event_action(event))
	{
	case ACTION_DRAG_MOVE:	/* they are moving the object */
		tprintf(textpane, "Drag MOVE\n");
		break;
	case ACTION_DRAG_COPY:	/* they are copying the object */
		tprintf(textpane, "Drag COPY\n");
		break;
	default:
		printf("unknown event %d\n", event_action(event));
		return;
	}

	/* A request to get the list of items and print it out */
	targets.name = "TARGETS";
	if(!get_info(sel, &targets))
	{
		tprintf(textpane, "*** Unable to get TARGETS list.\n");
		return;
	}
	tprintf(textpane, "TARGETS contains %d atom%s\n",
			targets.length, (targets.length == 1)?"":"s");
	a_list(&targets);

	/*
	 * Allocate space for target list
         */
	target_list = (Request *)malloc(targets.length*sizeof(Request));
	memset(target_list, '\0', targets.length*sizeof(Request));

	tprintf(textpane, "----------------------------- info for each ");
	tprintf(textpane, "-----------------------------\n");

	/*
	 * Get the appropriate atoms from the target data
	 * and print them in the text subwindow
         */
	t_item = target_list;
	for(i = 0; i < targets.length; i++)
	{
		t_item->atom = ((Atom *)targets.data)[i];
		if(!get_info(sel, t_item))
		{
				tprintf(textpane, "NOT FOUND->\t%s\n", t_item->name);
		}
		t_item++;
	}

	for(i = 0; i < targets.length; i++)
	{
		display_info(target_list[i]);
	}
	
	for(i = 0; i < targets.length; i++)
	{
		free(target_list[i].name);
	}
	free(target_list);
	textsw_normalize_view(textpane, 0);

	/* reset the textsw */
	xv_set(textpane, TEXTSW_LOWER_CONTEXT, old_context, NULL);
}

/* 
 * Do the proper cleanup and destoying 
 */
Notify_value
destroy_func(client, status)
Notify_client   client;
Destroy_status  status;
{
	switch(status)
	{
	case	DESTROY_CHECKING:
		break;
	case	DESTROY_CLEANUP:
		textsw_reset(textpane, 0, 0);
		return(notify_next_destroy_func(client, status));
		break;
	case	DESTROY_SAVE_YOURSELF:
		break;
	case	DESTROY_PROCESS_DEATH:
		break;
	}
	return(NOTIFY_DONE);
}

main(argc, argv)
int             argc;
char          **argv;
{
	int             i;
	char            c;


	/*
         * Initialize XView
         */
	My_server = xv_init(XV_INIT_ARGC_PTR_ARGV, &argc, argv, NULL);

	/*
	 * Create a Frame
         */
	frame = xv_create(NULL, FRAME,
			  XV_LABEL, "Drag-n-Drop Demo",
			  FRAME_SHOW_FOOTER, FALSE,
			  FRAME_SHOW_RESIZE_CORNER, TRUE,
			  NULL);

	/*
         * Get display pointer
         */
	dpy = (Display *) xv_get(frame, XV_DISPLAY);

	/* 
	 * Create a panel
         */
	panel = xv_create(frame, PANEL,
			  XV_X, 0,
			  XV_Y, 0,
			  XV_WIDTH, WIN_EXTEND_TO_EDGE,
			  WIN_BORDER, FALSE,
			  NULL);


	/*
	 * Create drag object
	 */
	drag_object = xv_create(panel, DRAGDROP, NULL);

	/*
	 * Create panel drop targe
	 */
	xv_create(panel, PANEL_DROP_TARGET,
		  XV_X, 50,
		  PANEL_DROP_DND, drag_object,
		  PANEL_NOTIFY_PROC, drop_proc,
		  PANEL_DROP_FULL, FALSE,
		  NULL);

	window_fit_height(panel);

	/*
	 * Create a text subwindow
         */
	textpane = xv_create(frame, TEXTSW,
			     XV_X, 0,
			     WIN_BELOW, panel,
			     WIN_COLUMNS,	80,
			     XV_HEIGHT, WIN_EXTEND_TO_EDGE,
			     OPENWIN_SHOW_BORDERS, TRUE,
			     TEXTSW_CONTENTS,	copyrite,
			     NULL);

	window_fit(frame);

	notify_interpose_destroy_func(frame, destroy_func);

	/*
	 * Turn control over to XView.
	 */
	xv_main_loop(frame);
	exit(0);
}

