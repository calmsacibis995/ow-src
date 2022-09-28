#ifndef lint
static char sccsid[] = "@(#)textedit.c 3.30 96/12/02 Copyr 1990 Sun Micro";
#endif

/*
 * Copyright (c) 1986, 1987, 1988 by Sun Microsystems, Inc.
 */

/*
 * This textedit is the envelope that binds various text display and
 *   editing facilities together.
 */

#include <sys/param.h> /* MAXPATHLEN (include types.h if removed) */
#include <ds_verbose_malloc.h>

#ifdef SVR4
#include <dirent.h>   /* MAXNAMLEN */
#include <netdb.h>   /* MAXHOSTNAMLEN */
#else
#include <sys/dir.h>   /* MAXNAMLEN */
#endif

#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <locale.h>
#include <X11/X.h>
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
#include <desktop/tt_c.h>
#include "textedit.h"

extern char	*xv_version;

char	*gectwd(), *ds_hostname(), *strrchr();
void	frame_cmdline_help();
static int		off();
static int		on();
static Notify_value	mysigproc();

int	load_on = FALSE;
extern int	tt_running;

static int		handling_signal = 0;
static char		*cmd_name;
static int		user_label;
static int		caps_lock_on;
static char		cmdline_args[1024];
static char		title [1024];
static char		icon_label [1024];
static char		*iconfont;

Frame		base_frame;
Panel		panel;
char		current_directory[MAXPATHLEN];
char		current_filename[MAXNAMLEN];
Textsw		textsw;
int		edited;
int      	read_only;
int		exiting = FALSE;
int      	debug_on = 0;
char		Hostname[MAXHOSTNAMELEN];

Server_image	 	source_drag_ptr_image;
Server_image	 	source_drop_ptr_image;

int 			textedit_key_data;


static int			PANEL_ITEM_DATA;
Panel_item		file_panel_item, edit_panel_item, display_panel_item;
Panel_item		find_panel_item, source_panel_item;
int			no_file;

static const unsigned short edit_ic_image[256]={
#include "textedit.icon"
};

static const unsigned short edit_mask_image[256]={
#include "textedit.mask.icon"
};

static const unsigned short source_drag_image []={
#include "dupedoc_drag.icon"
};

static const unsigned short source_drop_image []={
#include "dupedoc_drop.icon"
};

/*
 * The textedit command line options.
 */
static const char	*option_names[] =
{
	"auto_indent",		"Ei" ,
	"okay_to_overwrite",	"Eo" ,
	"lower_context",	"EL" ,
	"margin",		"Em" ,
	"multi_click_space",	"ES" ,
	"multi_click_timeout",	"ET" ,
	"number_of_lines",	"En" ,
	"read_only",		"Er" ,
	"scratch_window",	"Es" ,
	"tab_width",		"Et" ,
	"history_limit",	"Eu" ,
	"upper_context",	"EU" ,
	"checkpoint",		"Ec" ,
	"num_cols",		"EC" ,
	"tooltalk",		"Ett" ,
	"message_alliance",	"Ema" ,
#ifdef DEBUG
	"malloc_debug_level",	"Ed" ,
	"dnd_debug_on",		"Ednd" ,
#else
	"dnd_debug_on",		"Ednd" ,
#endif
	0  /* Terminator! */
};

#define OPTION_AUTO_INDENT		(1<<0)
#define OPTION_ALWAYS_OVERWRITE		(1<<1)
#define OPTION_LOWER_CONTEXT		(1<<2)
#define OPTION_MARGIN			(1<<3)
#define OPTION_MULTI_CLICK_SPACE	(1<<4)
#define OPTION_MULTI_CLICK_TIMEOUT	(1<<5)
#define OPTION_NUMBER_OF_LINES		(1<<6)
#define OPTION_READ_ONLY		(1<<7)
#define OPTION_SCRATCH_WINDOW		(1<<8)
#define OPTION_TAB_WIDTH		(1<<9)
#define OPTION_UNDO_HISTORY		(1<<10)
#define OPTION_UPPER_CONTEXT		(1<<11)
#define OPTION_CHECKPOINT_FREQUENCY	(1<<12)
#define OPTION_NUM_COLS	                (1<<13)
#define OPTION_TOOL_TALK                (1<<14)
#define OPTION_MESSAGE_ALLIANCE         (1<<15)

#ifdef MA_DEBUG
#define	DP(block) if(1){ printf("%s:%d ", __FILE__, __LINE__); block }
#else
#define	DP(block) if(0){ block }
#endif

#ifdef DEBUG
#define OPTION_MALLOC_DEBUG_LEVEL	(1<<16)
#define OPTION_EDIT_LOG_WRAPS_AT	(1<<18)
#define OPTION_DND_DEBUG_ON	        (1<<19)
#else
#define OPTION_DND_DEBUG_ON	        (1<<16)
#endif


/*
 * Return pointer to longest suffix not beginning with '/'
 */
static char *
base_name(full_name)
	char *full_name;
{
	register char	*temp;

	DP(printf("called base_name(%s)\n", full_name);)
	if ((temp = strrchr(full_name, '/')) == NULL)
	    return(full_name);
	else
	    return(temp+1);
}

void
textedit_version(char **vend, char **name, char **ver)
{
	DP(printf("called textedit_version\n");)
	*vend = (char *)ds_vendname();
	*name = "Textedit";
	*ver = (char *)ds_relname();
}

set_tool_label(icon_text, was_read_only)
char	*icon_text;
int	was_read_only;
{
    Icon		edit_icon;
    char		frame_label[50+MAXNAMLEN+MAXPATHLEN];
    char		*ptr;


    DP(printf("called set_tool_label(%s, %d)\n", icon_text?icon_text:"(null)", was_read_only);)
    if (current_link_atom && icon_text)
	if (*title)
	     (void)sprintf(frame_label, title);
	else
    	     (void)sprintf(frame_label,  MGET("%s Text Editor %s%s - Link to %s:%s%s, dir; %s") ,
	    	(caps_lock_on) ?  MGET("[CAPS] ")  : "", 
		ds_relname(),
		ds_hostname(xv_get(My_server, XV_DISPLAY)),
	    	dnd_context->source_name?dnd_context->source_name:"",
	    	dnd_context->data_label?dnd_context->data_label:"",
	    	(was_read_only) ?  MGET(" (read only)") 
			: (edited) ?  MGET(" (edited)")  : "",
 	    	current_directory);
    else
	if (*title)
	     (void)sprintf(frame_label, title);
	else
    	     (void)sprintf(frame_label,  MGET("%s Text Editor %s%s - %s%s, dir; %s") ,
	    	(caps_lock_on) ?  MGET("[CAPS] ")  : "",
		ds_relname(),
 	    	ds_hostname(xv_get(My_server, XV_DISPLAY)),
	    	current_filename,
	    	(was_read_only) ?  MGET(" (read only)") 
			: (edited) ?  MGET(" (edited)")  : "",
 	    	current_directory);
    (void)xv_set(base_frame, FRAME_LABEL, frame_label, 0);

    if (!icon_text)
	icon_text =  MGET("NONE") ;

    if (icon_text[0] != '\0') {
	struct rect	text_rect, *icon_rect;
	Xv_Font		font;
	
	edit_icon = xv_get(base_frame, FRAME_ICON);
	icon_rect = (Rect *) xv_get(edit_icon, ICON_IMAGE_RECT);
	font = (Xv_Font) xv_get(edit_icon, XV_FONT);
	if (user_label)
	   strcpy (icon_label, (char *) xv_get(edit_icon, XV_LABEL));
	else
	   strcpy (icon_label, icon_text);

	/* adjust icon text top/height to match font height */
	text_rect.r_height = (int) xv_get(font, FONT_DEFAULT_CHAR_HEIGHT);
	text_rect.r_top =
	    icon_rect->r_height - (text_rect.r_height + 2);

	/* center the icon text */
	text_rect.r_width = strlen(icon_label) *
				(xv_get(font, FONT_DEFAULT_CHAR_WIDTH));
	if (text_rect.r_width > icon_rect->r_width)
	    text_rect.r_width = icon_rect->r_width;
	text_rect.r_left = (icon_rect->r_width-text_rect.r_width)/2;

	(void)xv_set(edit_icon,
	    XV_LABEL,		icon_label,
	    ICON_LABEL_RECT,	&text_rect,
	    0);
	if (iconfont == NULL) 
		if (defaults_exists("icon.font.name", "Icon.Font.Name"))
      			iconfont = strdup((char *)defaults_get_string(
				"icon.font.name", "Icon.Font.Name", "")); 
	if (iconfont != NULL) {
		font = (Xv_Font) xv_find(base_frame, FONT,
		       FONT_NAME, iconfont,
		       0) ;
		if (font != 0)
			xv_set(edit_icon, ICON_FONT, font, 0) ;
	}
	/* xv_set actually makes a copy of all the icon fields */
	(void)xv_set(base_frame, FRAME_ICON, edit_icon, 0);
    }
}

static
set_name_frame(textsw_local, attributes)
Textsw          textsw_local;
Attr_avlist     attributes;
{
	char            icon_text[50 + MAXNAMLEN + MAXPATHLEN];
	char           *ptr;
	int             len, pass_on = 0, repaint = 0;
	int             was_read_only = read_only;
	Attr_avlist     attrs;
	char           *attr_string;
	char           *temp;

	DP(printf("called set_name_frame(...)\n");)

	if (handling_signal)
		return;
	icon_text[0] = '\0';
	ptr = icon_text;
	for (attrs = attributes; *attrs; attrs = attr_next(attrs))
	{
		repaint++;	/* Assume this attribute needs a repaint. */
		switch ((Textsw_action) (*attrs))
		{
		case TEXTSW_ACTION_CAPS_LOCK:
			DP(printf("\tTEXTSW_ACTION_CAPS_LOCK\n");)
			caps_lock_on = (int) attrs[1];
			ATTR_CONSUME(*attrs);
			break;
		case TEXTSW_ACTION_CHANGED_DIRECTORY:
			DP(printf("\tTEXTSW_ACTION_CHANGED_DIRECTORY\n");)
			attr_string = (char *) attrs[1];
			switch (attr_string[0])
			{
			case '/':
				(void) strcpy(current_directory, (char *) attrs[1]);
				break;
			case '.':
				if (attr_string[1] != '\0')
					(void) getcwd(current_directory, MAXPATHLEN);
				break;
			case '\0':
				break;
			default:
				(void) strcat(current_directory, "/");
				(void) strcat(current_directory, (char *) attrs[1]);
				break;
			}
			ATTR_CONSUME(*attrs);
			break;
		case TEXTSW_ACTION_USING_MEMORY:
			DP(printf("\tTEXTSW_ACTION_USING_MEMORY\n");)
			(void) strcpy(current_filename, MGET("(NONE)"));
			(void) strcpy(icon_text, MGET("NO FILE"));
			edited = read_only = 0;
			current_type = NULL;
			ATTR_CONSUME(*attrs);
			if (source_panel_item != (Panel_item) NULL)
				xv_set(source_panel_item, PANEL_DROP_FULL, FALSE, NULL);
			break;
		case TEXTSW_ACTION_LOADED_FILE:
			DP(printf("\tTEXTSW_ACTION_LOADED_FILE\n");)
			temp = base_name((char *) attrs[1]);
			(void) strcpy(current_filename, temp);
			temp = strrchr((char *) attrs[1], '/');
			if (temp != (char *) NULL)
			{
				int             length = (int) (temp - attrs[1]);

				(void) strncpy(current_directory, (char *) attrs[1],
					       length);
				current_directory[length] = '\0';
				chdir(current_directory);
			}
			edited = read_only = no_file = 0;
			discard_load();
			current_type = NULL;
			if (source_panel_item != (Panel_item) NULL)
				xv_set(source_panel_item, PANEL_DROP_FULL, TRUE, NULL);
			/* no break is intentional */
		case TEXTSW_ACTION_EDITED_FILE:
			DP(printf("\tTEXTSW_ACTION_EDITED_FILE\n");)
			if((Textsw_action) (*attrs) == TEXTSW_ACTION_EDITED_FILE)
			{
				if (source_panel_item != (Panel_item) NULL)
					xv_set(source_panel_item, PANEL_DROP_FULL, TRUE, NULL);
				edited = 1;
				*ptr++ = '>';
			}
			len = (strlen((char *) attrs[1]) > sizeof(icon_text) - 2) ?
				sizeof(icon_text) - 2 : strlen((char *) attrs[1]);
			/* need 1 char for edit/not, 1 for null */
			(void) strncpy(ptr, (char *) attrs[1], len);
			ptr[len] = '\0';
			(void) strcpy(ptr, base_name(ptr));	/* strip path */
			ATTR_CONSUME(*attrs);
			break;
		case TEXTSW_ACTION_EDITED_MEMORY:
			DP(printf("\tTEXTSW_ACTION_EDITED_MEMORY\n");)
			if (source_panel_item != (Panel_item) NULL)
				xv_set(source_panel_item, PANEL_DROP_FULL, TRUE, NULL);
			edited = 1;
			break;
		default:
			pass_on = 1;
			repaint--;	/* Above assumption was wrong. */
			break;
		}
	}
	if (pass_on)
		(void) textsw_default_notify(textsw_local, attributes);
	if (repaint)
	{
		set_tool_label(icon_text, was_read_only);
	}
}

/*ARGSUSED*/
static void
button_notify_proc(item, event)
Panel_item      item;
Event          *event;
{
	Textsw          textsw = (Textsw) xv_get(item, XV_KEY_DATA, PANEL_ITEM_DATA);
	Menu            menu = (Menu) xv_get(item, PANEL_ITEM_MENU);
	Panel           p_menu = (Panel) xv_get(menu, MENU_PIN_WINDOW);
	Menu_item       menu_item;
	Menu            pullr_menu;
	int             num_items, i;

	DP(             printf("called button_notify_proc\n");)

	xv_set(menu, XV_KEY_DATA, TEXTSW_MENU_DATA_KEY, textsw, 0);
	if (p_menu)
	{
		num_items = (int) xv_get(menu, MENU_NITEMS);
		for (i = 1; i <= num_items; i++)
		{
			if (menu_item = (Menu_item) xv_get(menu, MENU_NTH_ITEM, i))
			{
				if (pullr_menu = (Menu) xv_get(menu_item, MENU_PULLRIGHT))
				{
					xv_set(pullr_menu, XV_KEY_DATA, TEXTSW_MENU_DATA_KEY, textsw, 0);
				}
			}
		}
	}
}


place_drop_site(window)
Xv_opaque       window;
{
	Rect           *find_item_rect;
	Rect           *source_item_rect;
	int             find_left, find_width, source_width;
	int             gap = xv_get(window, PANEL_ITEM_X_GAP);
	int             panel_width;
	Xv_opaque	sb = xv_get(textsw, TEXTSW_SCROLLBAR);

	/* Fix for 4005325 */
	if (textsw == NULL || find_panel_item == NULL)
		return ;

	panel_width = xv_get(textsw, XV_WIDTH) - (sb?xv_get(sb, XV_WIDTH):0);

	DP(printf("called place_drop_site\n");)
	find_item_rect = (Rect *) xv_get(find_panel_item, PANEL_ITEM_RECT);
	find_left = find_item_rect->r_left;
	find_width = find_item_rect->r_width;

	source_item_rect = (Rect *) xv_get(source_panel_item, PANEL_ITEM_RECT);
	source_width = source_item_rect->r_width;

	if ((panel_width - source_width) > (find_width + find_left + gap))
	{
		/*
		 * there is enough space.  Lets place the item over against
		 * the left edge of the panel.
		 */

		xv_set(source_panel_item,
		       PANEL_ITEM_X, panel_width - source_width,
		       0);
	}
	else
	{
		/*
		 * window too small top move item over.  Butt it against the
		 * last item on the panel.
		 */

		xv_set(source_panel_item,
		       PANEL_ITEM_X, find_width + find_left + gap,
		       0);
	}
}


static Notify_value
frame_event_proc(window, event, arg, type)

Xv_opaque               window;
Event                   *event;
Notify_arg              arg;
Notify_event_type       type;

{
	if (event_action(event) == ACTION_STOP)
	{
		dnd_context->stop_hit = TRUE;
		if (debug_on)
			printf( "Got stop key\n" );
	}

	return notify_next_event_func(window, (Notify_event) event, arg, type);
}

int
drop_target_notify_proc (item, value, event)
Panel_item	item;
unsigned int	value;
Event		*event;
{
	DP(printf("called drop_target_notify_proc\n");)

    switch (event_action(event)) {

       case LOC_DRAG:
	    switch (value) {

	       case XV_OK:
		    xv_set(base_frame, 
			   FRAME_LEFT_FOOTER, MGET("Drag and Drop: Began"), 0);
		    break;
	       case DND_TIMEOUT:
		    xv_set(base_frame, FRAME_LEFT_FOOTER, 
				       MGET("Drag and Drop: Timed Out"), 0);
		    break;
	       case DND_ILLEGAL_TARGET:
		    xv_set(base_frame, FRAME_LEFT_FOOTER,
				     MGET("Drag and Drop: Illegal Target") ,0);
		    break;
	       case DND_SELECTION:
		    xv_set(base_frame, FRAME_LEFT_FOOTER,
				      MGET("Drag and Drop: Bad Selection") ,0);
		    break;
	       case DND_ROOT:
		    xv_set(base_frame, FRAME_LEFT_FOOTER,
				       MGET("Drag and Drop: Root Window") ,0);
		    break;
	       case XV_ERROR:
		    xv_set(base_frame, FRAME_LEFT_FOOTER,  
				       MGET("Drag and Drop: Failed") ,0);
		    break;
	       }


	    if (value != XV_OK)
	       return (XV_OK);
			
	    break;

       case ACTION_DRAG_MOVE:
       case ACTION_DRAG_COPY:

	    event->action = ACTION_DRAG_COPY;

       	    if (value != XV_ERROR) 
	       load_from_dragdrop ();	
	    else {
	       if (debug_on)
            	  printf ( "drop error\n" );
	       return (XV_OK);
	       }
       }

       return (XV_ERROR);

}

Notify_value
textedit_frame_destroy_func(client, status)
Notify_client	client;
Destroy_status	status;
{
	int ret_value;
	DP(printf("called textedit_frame_destroy_func\n");)

/*
 * Ok, so here's what's going on in this function. 
 * Previously, just called discard_load and kept going. That didn't
 * work since mailtool (or any app) that textedit had forged a link to
 * will reply when it gets the discard_load request, and by the time
 * textedit would have gotten the reply, it was gone, thus causing 
 * mailtool to exit with an X error.
 */


	switch (status) {
	case DESTROY_CHECKING:
	DP(printf("\tDESTROY_CHECKING\n");)
/*
 * First, we must check to see if the file was edited. If so,
 * and the user selects "Cancel, do not exit", we just want to
 * return and *NOT* discard the load (the user may save the data back).
 */

		if (edited) {
	DP(printf("\t\tedited\n");)
/*
 * File has been edited, so pass along the event which will bring up
 * the notice and let the user decide if he wants to exit or not.
 */

		   ret_value = notify_next_destroy_func (client, status);

/*
 * Now, if the user selected "Discard changes, and exit", the original
 * copy of the file is loaded in prior to the notify_next_destroy_func
 * returning. Because of this, discard_load is called  (in the notify
 * handler), so we may have already broken our link with the other 
 * application, and don't need to do it again. So, if discard_load was
 * called, the variable 'discarded' is now set to true.
 * Note that if there was no file loaded, and the user had just been
 * typing into an empty textedit, discard_load will never be called.
 * We may have a link to another application, so we will check later on.
 */

	    	   if (discarded == TRUE) {
			DP(printf("\t\tdiscarded\n");)

/*
 * OK, so discard_load was called, and discarded is TRUE meaning that
 * we have to break a link with another application. So, let's not exit
 * after all since we have to wait until the reply is received. In this
 * case, set variable 'exiting' to TRUE, and veto the destroy. We will
 * get the reply from the linked app in discard_load_reply_proc (see
 * file dragdrop.c) at which time we'll check the variable 'exiting'
 * and see that we were in the middle of exiting, at which time we'll
 * do an xv_destroy of the frame to *really* exit. 
 */

		      exiting = TRUE;
		      (void)notify_veto_destroy (client);
		      return NOTIFY_DONE;
		      }

/* 
 * Ok, so discarded was false, meaning that we're not breaking the link.
 * If the user selected "Cancel, do not exit", then we should return
 * since the user doesn't want to exit anyway.
 * If the user selected "Discard edits and exit", and had been editing
 * memory (an empty textedit), we still need to check if we had forged
 * a link to another application. For example, we might have been started
 * by a user double clicking on a mailtool attachment, and then the user
 * cleared out the text and just started typing something new. In this
 * case, when the user selects "Discard edits...", discard_load isn't
 * called, so we continue here so that we do call it.
 */

		   else
			DP(printf("\t\tnot discarded\n");)
		      if (ret_value == NOTIFY_IGNORED)
			 return (NOTIFY_IGNORED);
		   }

/*
 * The file was not edited. However, we're in the middle of exiting. For
 * example, we called discard_load, and got the reply back from 
 * the linked app. We did an xv_destroy in discard_load_reply_proc
 * and now we're here. Since we're in the middle of exiting, we
 * can just return since we've already done all of our checking. 
 */

		else if (exiting == TRUE)
		{
			DP(printf("\t\texiting\n");)
			return(timeout_quit(client));
		}

/*
 * The file wasn't edited (or the user was editing memory and decided to
 * Discard the edits), and we're not in the middle of an exit.
 * So, call discard_load and see if we need to break a link to another
 * application. Again, as above, if we are, then let's not exit now,
 * (instead, set variable 'exiting' to TRUE), and veto this exit
 * request.
 */

		if (discard_load () == TRUE) {
			DP(printf("\t\tdiscard_load () == TRUE\n");)
		   exiting = TRUE;
		   notify_veto_destroy (client);
		   return NOTIFY_DONE;
		}

		break;
	}

	return(notify_next_destroy_func(client, status));
}

my_event_proc(window, event)
Window	window;
Event	*event;
{
	if (event_action(event) == WIN_RESIZE)
		place_drop_site(panel);
}


static void
my_frame_help(name)
	char	*name;
{
	DP(printf("called my_frame_help\n");)
	frame_cmdline_help(name);
	
#ifdef TEXTEDIT_HELP_STRING
	/* STRING_EXTRACTION
	 * Below, "-text_help" does not need to be translated.
	 */
	(void)fprintf(stderr,
		 MGET("\nFor further information, use the switch -text_help.\n") );
#endif

}

main(argc, argv)
	int	  argc;
	char	**argv;
{
	DP(printf("called main\n");)
#ifdef	GPROF
	if (argc > 1 && strcmp(argv[argc-1],  "-gprof" ) == 0) {
	    moncontrol(1);
	    /* Pull the -gprof out of argc/v */
	    argc--;
	    argv[argc] = (char *)0;
	} else {
	    moncontrol(0);
	}
#endif	GPROF
	textedit_main(argc, argv);
}

parameter_error(parameter, num_required)
char	*parameter;
int	num_required;
{
	DP(printf("called parameter_error\n");)

	if (num_required > 1)
		printf(MGET("textedit: The %s option requires %d arguments\n"), parameter, num_required);
	else
		printf(MGET("textedit: The %s option requires an argument\n"), parameter, num_required);
	exit(4);
}

/*Begin workaround for bug 1196662*/
static int
handler(Display *dpy)       /* XIOErrorHandler          */
{
	mysigproc((Notify_client)NULL, SIGHUP, (Notify_signal_mode)NULL);
	exit(3);

}
/*End workaround for bug 1196662 */

textedit_main(argc, argv)
	int	  argc;
	char	**argv;
{
#define	GET_INT_ATTR_VAL(var)						\
	if (argc > 0) {var = (caddr_t) atoi(argv[1]); argc--, argv++;}
	extern struct pixfont	 *pw_pfsysopen();

	char bind_home[MAXPATHLEN];
	Icon			  edit_icon;
	Textsw_status		  status;
	int			  checkpoint = 0;
	int			  margin;
	int			  textswwidth;
	int			  number_of_lines = 0;
	int			  optioncount = 
				   sizeof(option_names)/sizeof(option_names[0]);
	struct stat           	  stb;
	caddr_t			  textsw_attrs[ATTR_STANDARD_SIZE];
	int			  attrc = 0;
	char			 *filen=NULL, *file_to_edit = NULL;
	Menu			  file_panel_menu, edit_panel_menu, display_panel_menu, find_panel_menu;
		
#ifdef DEBUG
	caddr_t			  edit_log_wraps_at = (caddr_t)TEXTSW_INFINITY;
#endif
	
	int			tt_flag = 0;
	int			ma_flag = 0;
        int                       num_cols = 0;
        int                       user_set_size = FALSE;
        int                       user_char_size = FALSE;
	int			  just_version = FALSE;
	char			  **argscanner = argv;
	Server_image		  icon_image;
	Server_image		  mask_image;
	extern	int		  x_error_proc();
	Tt_status		  rc;

	DP(printf("called textedit_main\n");)
	/*
	 * Init data
	 * Implicitely zeroed:	caps_lock_on, handling_signal,
	 *			read_only, edited
	 */

	while (*argscanner)
        {
		if (!strcmp(*argscanner,  "-v" )) {
		   just_version = TRUE;
		   break;
		   }

                if ((!strcmp(*argscanner,  "-Ws" )) || (!strcmp(*argscanner, "-size")) )
		{
		/* set x and y size in pixels  but let it fall thru and
                     X will pick up the settings */
                  user_set_size = TRUE;
                  user_char_size = FALSE;
                  argscanner++;
                  argscanner++;
                  }
                if (!strcmp(*argscanner, "-geometry"))
                  {
                  user_set_size = TRUE;
                  user_char_size = FALSE;
                  argscanner ++ ;
                  }
		if ((!strcmp(*argscanner,  "-Ww" )) || (!strcmp(*argscanner, "-width")) )

                  {
                  /* set width in font character columns */
                  user_set_size = TRUE;
                  user_char_size = TRUE;
                  argscanner++;
                  num_cols=atoi(*argscanner);
                  }
                if ((!strcmp(*argscanner,  "-Wh" )) || (!strcmp(*argscanner, "-height")) )

                  {
                  /* set height in font character lines */
                  user_set_size = TRUE;
                  user_char_size = TRUE;
                  argscanner++;
                  number_of_lines=atoi(*argscanner);
                  }

		if (!strcmp (*argscanner, "-title")) {
		  /* set title in frame label */
		  argscanner++;
		  strncpy (title, *argscanner, sizeof(title));
		  title[1023]=NULL;
		}
		/* Check if we've been started by tooltalk */
		if (strcmp(*argscanner,  "-tooltalk" ) == 0) {
			tt_flag++;
		}
		/* Check if we've been started for message alliance */
		if (strcmp(*argscanner,  "-message_alliance" ) == 0) {
			ma_flag++;
		}
		if (strcmp(*argscanner,  "-icon_font" ) == 0 ||
			strcmp(*argscanner,  "-WT" ) == 0) {
			iconfont = strdup(*(argscanner+1));
		}
                argscanner++;
        }

	gethostname(Hostname, MAXHOSTNAMELEN);

	/* Initialize tooltalk.  Must be done before xv_init() */
	if (tt_flag)
	{
		start_tt_init( "textedit" , TRUE, argc, argv);
	}
	else if (ma_flag)
	{
		int	i;
		if(rc = dstt_check_startup(textedit_version, &argc, &argv))
		{
			print_tt_emsg(MGET("Could not initialize Tool Talk"),
						rc);
                	return(-1);
                }
		tt_running = TRUE;
	}

	My_server = xv_init(XV_INIT_ARGC_PTR_ARGV, &argc, argv,
			    XV_USE_LOCALE, TRUE,
			    XV_X_ERROR_PROC, x_error_proc,
			    0);

	/*Begin workaround for bug 1196662*/
	/* Register our handler to handle server failures */
	XSetIOErrorHandler(handler);
	/*End workaround for bug 1196662*/

	ds_expand_pathname( "$OPENWINHOME/lib/locale" , bind_home);
	bindtextdomain(MSGFILE, bind_home);
	textdomain(MSGFILE);

	if (just_version) {
	   fprintf(stderr, "textedit version %s running on %s\n",
					ds_relname(), xv_version);
	   exit(0);
	   }

	textedit_key_data = xv_unique_key();

     	icon_image = xv_create(0, SERVER_IMAGE,
                SERVER_IMAGE_BITS, edit_ic_image,
                SERVER_IMAGE_DEPTH, 1,
                XV_WIDTH, 64,
                XV_HEIGHT, 64,
                0);

     	mask_image = xv_create(0, SERVER_IMAGE,
                SERVER_IMAGE_BITS, edit_mask_image,
                SERVER_IMAGE_DEPTH, 1,
                XV_WIDTH, 64,
                XV_HEIGHT, 64,
                0);

        edit_icon= xv_create(0, ICON,
                        ICON_IMAGE, icon_image,
			ICON_MASK_IMAGE, mask_image,
			ICON_TRANSPARENT, TRUE,
			WIN_RETAINED, TRUE,
                        0);

     	source_drag_ptr_image = xv_create(0, SERVER_IMAGE,
                SERVER_IMAGE_BITS, source_drag_image,
                SERVER_IMAGE_DEPTH, 1,
                XV_WIDTH, 64,
                XV_HEIGHT, 64,
                0);

     	source_drop_ptr_image = xv_create(0, SERVER_IMAGE,
                SERVER_IMAGE_BITS, source_drop_image,
                SERVER_IMAGE_DEPTH, 1,
                XV_WIDTH, 64,
                XV_HEIGHT, 64,
                0);

	xv_set(edit_icon,
		WIN_CONSUME_EVENTS,
		LOC_WINENTER,
		LOC_WINEXIT,
		LOC_DRAG,
		0,
		0);

	cmd_name = *argv;		/* Must be BEFORE calls on die() */
	current_filename[0] = '\0';
	(void) getcwd(current_directory, MAXPATHLEN);
	    /* Error message is placed into current_directory by getcwd */
	checkpoint =
	    defaults_get_integer_check( "text.checkpointFrequency" ,
                           "Text.CheckpointFrequency" , 0, 0, (int)TEXTSW_INFINITY);

	if(tt_flag || ma_flag)
	{
		base_frame = xv_create((Xv_window)NULL, FRAME_CMD,
		    FRAME_CMD_PIN_STATE,	FRAME_CMD_PIN_OUT,
		    FRAME_NO_CONFIRM,		TRUE,
		    FRAME_SHOW_RESIZE_CORNER,	TRUE,
		    0);
	}
	else
	{
		base_frame = xv_create((Xv_window)NULL, FRAME,
		    FRAME_NO_CONFIRM,		TRUE,
	            FRAME_LABEL,                MGET("Text Editor"), 
		    FRAME_ICON,                 edit_icon,
		    0);
	}
	xv_set(base_frame,
		    FRAME_SHOW_LABEL,		TRUE,
		    FRAME_SHOW_FOOTER,		TRUE,
		    WIN_CONSUME_EVENTS, 	WIN_LEFT_KEYS, 0,
		    WIN_EVENT_PROC,		my_event_proc,
		    FRAME_CMDLINE_HELP_PROC,	my_frame_help,
		    HELP_STRING_FILENAME, 	"manpage_synopsis_help_index" , 
#if 0
		    FRAME_WM_COMMAND_ARGC_ARGV, argc-1, &(argv[1]), 
#endif
		    NULL);


	/*
	 * Set icon's font to system font [if user hasn't set icon font],
	 * but AFTER xv_create has a chance to change it from the
	 * built-in font.
	 * If the user supplies a label, use it and don't override
	 * with our's later.
	 * Note that we get the icon from the Frame in case user
	 * over-rides via argc, argv!
	 */
	edit_icon = xv_get(base_frame, FRAME_ICON);
	user_label = (int)xv_get(edit_icon, XV_LABEL);
	if (!xv_get(edit_icon, ICON_FONT)) {
	    (void)icon_set(edit_icon, ICON_FONT, pw_pfsysopen(), 0);
	    if (!xv_get(edit_icon, ICON_FONT))
		die( MGET("Cannot get default font.\n") , (char *)NULL, (char *)NULL);
	    (void)xv_set(base_frame, FRAME_ICON, edit_icon, 0);
	}

	/*
	 * Pick up command line arguments to modify textsw behavior.
	 * Notes: FRAME_ARGC_PTR_ARGV above has stripped window flags.
	 *        case OPTION_MARGIN is used to compute WIN_WIDTH.
	 */
#ifndef lint
	margin = (int)textsw_get_from_defaults(TEXTSW_LEFT_MARGIN);
#endif
	argc--; argv++;				/* Skip the cmd name */
	
	strcpy(cmdline_args, " ");

	while ((argc--) && (attrc < ATTR_STANDARD_SIZE)) {

	    strcat(cmdline_args, argv[0]);
	    strcat(cmdline_args, " ");

	    if (argv[0][0] == '-') {
		extern int	match_in_table(); 
		int		option =
				match_in_table(&(argv[0][1]), option_names);
		if (option < 0 || option >= optioncount) {
		    die(argv[0],  MGET(" is not a valid option.\n") , (char *)NULL);
		}
		switch (1<<(option/2)) {
		    case OPTION_NUMBER_OF_LINES:
			if (argc > 0) {
			    number_of_lines = atoi(argv[1]);
	    		    strcat(cmdline_args, argv[1]);
	    		    strcat(cmdline_args, " ");
			    argc--, argv++;
			    user_set_size = TRUE;
			    user_char_size = TRUE;
			}
			break;
		    case OPTION_DND_DEBUG_ON:
		 	debug_on = 1;
		    	if ((argc > 0) && (argv[1][0] != '-')) {
	    		    strcat(cmdline_args, argv[1]);
	    		    strcat(cmdline_args, " ");
			    argc--, argv++;
			}
			break;
		    case OPTION_READ_ONLY:
		 	read_only = 1;
		    	if ((argc > 0) && (argv[1][0] != '-')) {
	    		    strcat(cmdline_args, argv[1]);
	    		    strcat(cmdline_args, " ");
			    argc--, argv++;
			    read_only = !off(argv[0]);
			    if ((read_only) && (!on(argv[0]))) {
				if (argv[0][0] != '/') {
					filen = malloc(strlen(current_directory)+strlen(argv[0])+2);
					sprintf(filen, "%s/%s", current_directory, argv[0]);
					file_to_edit = filen;
				}
				else
					file_to_edit = argv[0];
			     }
			}
			break;
			
		    case OPTION_AUTO_INDENT:
			textsw_attrs[attrc++] = (caddr_t) TEXTSW_AUTO_INDENT;
			textsw_attrs[attrc] = (caddr_t) 1;
			if ((argc > 0) && (argv[1][0] != '-')) {
	    		    strcat(cmdline_args, argv[1]);
	    		    strcat(cmdline_args, " ");
			    argc--, argv++;
			    textsw_attrs[attrc] = (caddr_t) !off(argv[0]);
			}
			attrc++;
			break;
		    case OPTION_ALWAYS_OVERWRITE:
		 	textsw_attrs[attrc++] =
		 		(caddr_t) TEXTSW_CONFIRM_OVERWRITE;
		 	textsw_attrs[attrc] = (caddr_t) 0;
		    	if ((argc > 0) && (argv[1][0] != '-')) {
	    		    strcat(cmdline_args, argv[1]);
	    		    strcat(cmdline_args, " ");
			    argc--, argv++;
			    textsw_attrs[attrc] = (caddr_t) off(argv[0]);
			}
			attrc++;
			break;
#ifdef DEBUG
		    case OPTION_EDIT_LOG_WRAPS_AT:
			GET_INT_ATTR_VAL(edit_log_wraps_at)
			break;
#endif
		    case OPTION_LOWER_CONTEXT:
			if (!argv[1])
				parameter_error(argv[0], 1);

			textsw_attrs[attrc++] = (caddr_t) TEXTSW_LOWER_CONTEXT;
			GET_INT_ATTR_VAL(textsw_attrs[attrc++])
			break;
#ifdef DEBUG
		    case OPTION_MALLOC_DEBUG_LEVEL:
			textsw_attrs[attrc++] =
				(caddr_t)TEXTSW_MALLOC_DEBUG_LEVEL;
			GET_INT_ATTR_VAL(textsw_attrs[attrc++])
			break;
#endif
		    case OPTION_MARGIN:
			textsw_attrs[attrc++] =
				(caddr_t) TEXTSW_LEFT_MARGIN;
			margin = atoi(argv[1]);
			GET_INT_ATTR_VAL(textsw_attrs[attrc++])
			break;
		    case OPTION_MULTI_CLICK_SPACE:
			if (!argv[1])
				parameter_error(argv[0], 1);

    		        strcat(cmdline_args, argv[1]);
	    		strcat(cmdline_args, " ");
			textsw_attrs[attrc++] =
				(caddr_t) TEXTSW_MULTI_CLICK_SPACE;
			GET_INT_ATTR_VAL(textsw_attrs[attrc++])
			break;
		    case OPTION_MULTI_CLICK_TIMEOUT:
			if (!argv[1])
				parameter_error(argv[0], 1);

    		        strcat(cmdline_args, argv[1]);
	    		strcat(cmdline_args, " ");
			textsw_attrs[attrc++] =
				(caddr_t) TEXTSW_MULTI_CLICK_TIMEOUT;
			GET_INT_ATTR_VAL(textsw_attrs[attrc++])
			break;
		    case OPTION_TAB_WIDTH:
			if (!argv[1])
				parameter_error(argv[0], 1);

			if (argc > 0) {
	    		    strcat(cmdline_args, argv[1]);
	    		    strcat(cmdline_args, " ");
			}
			textsw_attrs[attrc++] = (caddr_t) TEXTSW_TAB_WIDTH;
			GET_INT_ATTR_VAL(textsw_attrs[attrc++])
			break;
		    case OPTION_UNDO_HISTORY:
			if (!argv[1])
				parameter_error(argv[0], 1);

    		        strcat(cmdline_args, argv[1]);
	    		strcat(cmdline_args, " ");
			textsw_attrs[attrc++] = (caddr_t) TEXTSW_HISTORY_LIMIT;
			GET_INT_ATTR_VAL(textsw_attrs[attrc++])
			break;
		    case OPTION_UPPER_CONTEXT:
			if (!argv[1])
				parameter_error(argv[0], 1);

    		        strcat(cmdline_args, argv[1]);
	    		strcat(cmdline_args, " ");
			textsw_attrs[attrc++] = (caddr_t) TEXTSW_UPPER_CONTEXT;
			GET_INT_ATTR_VAL(textsw_attrs[attrc++])
			break;
		    case OPTION_CHECKPOINT_FREQUENCY:
			if (!argv[1])
				parameter_error(argv[0], 1);

			if (argc > 0) {
	    		    strcat(cmdline_args, argv[1]);
	    		    strcat(cmdline_args, " ");
			    checkpoint = atoi(argv[1]);
			    argc--, argv++;
			}
			break;
		    case OPTION_NUM_COLS:
			if (!argv[1])
				parameter_error(argv[0], 1);

			if (argc > 0) {
			    num_cols = atoi(argv[1]);
	    		    strcat(cmdline_args, argv[1]);
	    		    strcat(cmdline_args, " ");
			    argc--, argv++;
			    user_set_size = TRUE;
			    user_char_size = TRUE;
			}
			break;
		    case OPTION_MESSAGE_ALLIANCE:
		    case OPTION_TOOL_TALK:
			/* Already handled.  Ignore it */
			break;
		    default:
			die( MGET("Unrecognized command line option.") , (char *)NULL, (char *)NULL);
			break;
		}
	    } else if (file_to_edit == NULL) {
		if (argv[0][0] != '/') {
			filen = malloc(strlen(current_directory)+strlen(argv[0])+2);
			sprintf(filen, "%s/%s", current_directory, argv[0]);
			file_to_edit = filen;
		}
		else
			file_to_edit = argv[0];
	    } else {
		die( MGET("Too many files specified.") , (char *)NULL, (char *)NULL);
	    }

	    argv++;
	}
	textsw_attrs[attrc] = 0;	/* A-V list terminator */

	xv_set(base_frame,
		WIN_CMD_LINE, cmdline_args,
		0);
		
	read_only = (read_only & (file_to_edit != NULL));
#ifdef DEBUG
	if (edit_log_wraps_at != (caddr_t)TEXTSW_INFINITY) {
	    textsw_attrs[attrc++] = (caddr_t) TEXTSW_WRAPAROUND_SIZE;
	    textsw_attrs[attrc++] = edit_log_wraps_at;
	    textsw_attrs[attrc] = 0;
	}
#endif
	if ((file_to_edit != NULL) && (stat(file_to_edit, &stb) < 0)) 
	{
		char 	buf[256];
		int	fd;
		int	alert_result;
		Event	event;

		sprintf(buf,  MGET("The file '%s' does not exist") , file_to_edit);
		alert_result = (int) notice_prompt(base_frame, &event,
			NOTICE_MESSAGE_STRINGS,
			buf,
			 MGET("Please confirm creation of new\nfile for textedit.") ,
			0,
			NOTICE_BUTTON_NO,  MGET("Cancel") ,
			NOTICE_BUTTON_YES,  MGET("Confirm") ,
			0);

		if (alert_result == NOTICE_YES)
			if ((fd = creat(file_to_edit, 0666)) == -1)
				die( MGET("Could not create new file.") , (char *)NULL, (char *)NULL);
			else
				close(fd);
		else
			exit(0);


	}
	else if ((file_to_edit != NULL) && (stat(file_to_edit, &stb) == 0)) 
	{
		char 	buf[256];
		int	alert_result;
		Event	event;

		/* see if it was a regular file */

		if (!(stb.st_mode & S_IFREG))
		{
	
			sprintf(buf,  MGET("The file '%s' is not a regular file") , file_to_edit);
			alert_result = (int) notice_prompt(base_frame, &event,
				NOTICE_MESSAGE_STRINGS,
				buf,
				 MGET("Please confirm creation of new\nfile for textedit.") ,
				0,
				NOTICE_BUTTON_YES,  MGET("Continue") ,
				NOTICE_BUTTON_NO,  MGET("Cancel") ,
				0);
	
			if (alert_result == NOTICE_YES)
				file_to_edit = NULL;
			else
				die( MGET("no file to edit") , NULL, NULL);
		}
		else if (access(file_to_edit, R_OK))
		{
	
			sprintf(buf,  MGET("The file '%s' is not a readable file") , file_to_edit);
			alert_result = (int) notice_prompt(base_frame, &event,
				NOTICE_MESSAGE_STRINGS,
				buf,
				 MGET("Please confirm creation of new\nfile for textedit.") ,
				0,
				NOTICE_BUTTON_YES,  MGET("Continue") ,
				NOTICE_BUTTON_NO,  MGET("Cancel") ,
				0);
	
			if (alert_result == NOTICE_YES)
				file_to_edit = NULL;
			else
				die( MGET("Did not have read permission on the file.") , NULL, NULL);
		}
		
	}

	/*
	 * Create subwindows
	 */

	if(tt_flag || ma_flag)
	{
		panel = xv_get(base_frame, FRAME_CMD_PANEL);
		xv_set(panel,
			PANEL_LAYOUT,	PANEL_HORIZONTAL,
			XV_HELP_DATA,	"textedit:Panel",
			0);
	}
	else
	{
		panel = xv_create(base_frame, PANEL,
			PANEL_LAYOUT,	PANEL_HORIZONTAL,
			XV_HELP_DATA,	"textedit:Panel",
			WIN_USE_IM,	FALSE,
			0);
	}
		
	
	textsw = (Textsw)xv_create(base_frame, TEXTSW,
		ATTR_LIST,			textsw_attrs,
		WIN_IS_CLIENT_PANE,
		TEXTSW_STATUS,			&status,
		TEXTSW_READ_ONLY,		read_only,
		TEXTSW_FILE,			file_to_edit,
		TEXTSW_NOTIFY_PROC,		set_name_frame,
		TEXTSW_CHECKPOINT_FREQUENCY,	checkpoint,
		TEXTSW_MEMORY_MAXIMUM,		TEXTSW_INFINITY,
		XV_HELP_DATA,			 "textedit:Textsw",
	    	HELP_STRING_FILENAME, 	 	"manpage_synopsis_help_index" ,
#ifndef	PRIOR_493
	    	TEXTSW_ACCELERATE_MENUS,	TRUE, 
#endif
	    	0);

         if (!user_set_size)
            /* For default size where -Ws -Ww -Wh and -size are not
               specified on the cmd line */
               xv_set(textsw,
                  WIN_ROWS, 45,
                  WIN_COLUMNS, 80,
                  0);
          else
                if (user_set_size && user_char_size)
                /* For -Ww and -Wh command line arguments, size is in
                 rows and columns */
                 xv_set(textsw,
                        WIN_ROWS,     (number_of_lines) ? number_of_lines : 45,
                        WIN_COLUMNS,  (num_cols) ? num_cols : 80,
                        0);

	/* force the panel to be some reasonable size, so that the 
	   buttons lay out reasonably. */

	xv_set(panel, WIN_WIDTH, 800, 0);

	/* Panel item and fitting height here is for performance reason */
	/* This way the textsw will not require to readjust the rect */
	file_panel_menu = (Menu)xv_get(textsw, TEXTSW_SUBMENU_FILE);

	file_panel_item = xv_create(panel, PANEL_BUTTON,
		PANEL_LABEL_STRING,		 MGET("File") ,
		PANEL_ITEM_MENU, 		file_panel_menu, 
		XV_HELP_DATA,			"textedit:File",
                PANEL_NOTIFY_PROC,              button_notify_proc,
		0);
		
	textswwidth = (int)xv_get(textsw, WIN_WIDTH);

	window_fit_height(panel);

	xv_set(textsw, WIN_BELOW, panel, 0);

	switch (status) {
	  case TEXTSW_STATUS_CANNOT_OPEN_INPUT:
	    die( MGET("Cannot open file '"), file_to_edit, MGET("', exiting!\n") );
	  case TEXTSW_STATUS_OKAY:
	    if (textsw)
		break;
	    /* else fall through */
	  default:
	    die( MGET("Cannot create textsw, exiting!\n") , (char *)NULL, (char *)NULL);
	}
	/*
	 * Setup signal handlers.
	 */
	(void)notify_set_signal_func(base_frame, mysigproc, SIGINT,  NOTIFY_ASYNC);
	(void)notify_set_signal_func(base_frame, mysigproc, SIGXCPU, NOTIFY_ASYNC);
	(void)notify_set_signal_func(base_frame, mysigproc, SIGBUS,  NOTIFY_ASYNC);
	(void)notify_set_signal_func(base_frame, mysigproc, SIGHUP,  NOTIFY_ASYNC);
	(void)notify_set_signal_func(base_frame, mysigproc, SIGILL,  NOTIFY_ASYNC);
	(void)notify_set_signal_func(base_frame, mysigproc, SIGSEGV, NOTIFY_ASYNC);
	(void)notify_set_signal_func(base_frame, mysigproc, SIGFPE,  NOTIFY_ASYNC);

	/*Begin workaround for bug 1196662*/
	(void)notify_set_signal_func(base_frame, mysigproc, SIGPIPE,  NOTIFY_SYNC);
	/*End workaround for bug 1196662*/

	/*
	 * Install us in tree of windows
	 */
	 
	PANEL_ITEM_DATA = xv_unique_key();

	display_panel_menu = (Menu)xv_get(textsw, TEXTSW_SUBMENU_VIEW);
	edit_panel_menu = (Menu)xv_get(textsw, TEXTSW_SUBMENU_EDIT);
	find_panel_menu = (Menu)xv_get(textsw, TEXTSW_SUBMENU_FIND);
	 
	 /* This set has to be done before the other panel items are created */
	xv_set(file_panel_item,
		XV_KEY_DATA, 			PANEL_ITEM_DATA, textsw,
		0);
		
	display_panel_item = xv_create(panel, PANEL_BUTTON,
		PANEL_LABEL_STRING,		 MGET("View") ,
		PANEL_ITEM_MENU, 		display_panel_menu,
		XV_HELP_DATA,			"textedit:View",
		XV_KEY_DATA, 			PANEL_ITEM_DATA, textsw,
                PANEL_NOTIFY_PROC,              button_notify_proc,
		0);

	edit_panel_item = xv_create(panel, PANEL_BUTTON,
		PANEL_LABEL_STRING,		 MGET("Edit") ,
		PANEL_ITEM_MENU, 		edit_panel_menu, 
		XV_HELP_DATA,			"textedit:Edit",
		XV_KEY_DATA, 			PANEL_ITEM_DATA, textsw,
                PANEL_NOTIFY_PROC,              button_notify_proc,
		0);
		
	find_panel_item = xv_create(panel, PANEL_BUTTON,
		PANEL_LABEL_STRING,		 MGET("Find") ,
		PANEL_ITEM_MENU, 		find_panel_menu, 
		XV_HELP_DATA,			"textedit:Find",
		XV_KEY_DATA, 			PANEL_ITEM_DATA, textsw,
                PANEL_NOTIFY_PROC,              button_notify_proc,
		0);	
		
	source_panel_item = xv_create(panel, PANEL_DROP_TARGET,
		XV_HELP_DATA,			"textedit:SourceDrag",
		XV_KEY_DATA, 			PANEL_ITEM_DATA, textsw,
	  	PANEL_DROP_SITE_DEFAULT,	TRUE,
		PANEL_NOTIFY_PROC,		drop_target_notify_proc,
#ifndef SUNOS41
		PANEL_DROP_DND_TYPE,		PANEL_DROP_COPY_ONLY,
#endif
		0);	

	if ((!user_set_size) || (user_set_size && user_char_size))
	{
		xv_set(panel, WIN_WIDTH, textswwidth, 0);
        	(void)window_fit(base_frame);
	}
	else
	{
		/* restore the panel width before coming up */
		xv_set(panel, WIN_WIDTH, WIN_EXTEND_TO_EDGE, 0);
	}

	/* set up to catch load events */

	notify_interpose_event_func((Notify_client) base_frame, (Notify_func) frame_event_proc, NOTIFY_SAFE);

	dragdrop_init();

	if ((file_to_edit != NULL) && (xv_get (textsw, TEXTSW_LENGTH) != 0))
	   xv_set (source_panel_item, PANEL_DROP_FULL, TRUE, NULL);

	place_drop_site(panel);

	notify_interpose_destroy_func((Notify_client) base_frame, (Notify_func) textedit_frame_destroy_func);

	/* Complete tool talk initialization */
	if (tt_flag)
	{
		complete_tt_init(base_frame);
	}
	else if (ma_flag)
	{
		textedit_dstt_start(base_frame, textsw);
		dstt_prnt_grp();
	}

	xv_main_loop(base_frame);

	/* Quit tooltalk */
	if (tt_flag)
		quit_tt();

	free(filen);
	exit(0);
}

/*
 *	SIGNAL handlers
 */

/* ARGSUSED */
static Notify_value
mysigproc(me, sig, when)
	Notify_client		me;
	int			sig;
	Notify_signal_mode	when;
{
	char			name_to_use[MAXNAMLEN];
	int			pid = getpid();
	int			was_SIGILL = (sig == SIGILL);
	int			saved = FALSE;

#ifndef SVR4
	struct sigvec vec;
#else
	sigset_t *nullmask;
	struct sigaction act;
	nullmask = MALLOC (sizeof (sigset_t));
	memset ( (char *) nullmask, 0, sizeof (sigset_t));
	(void) sigemptyset (nullmask);
#endif

	handling_signal++;

	DP(printf("called mysigproc(%d)\n", sig);)
	if (handling_signal == 1)
	{
		DP(printf("\thandling signal = %d\n", sig);)
		if (sig == SIGINT)
		{
			DP(printf("\t\tsignal = SIGINT\n");)
			if (xv_get((Xv_window) (textsw), TEXTSW_MODIFIED))
			{
				DP(printf("\t\t\tTEXTSW_MODIFIED\n");)
				/* It will be vetoed */
				(void) xv_destroy(base_frame);	
				DP(printf("\t\t\thandling signal = 0\n");)
				handling_signal = 0;
			}
			else
			{
				DP(printf("\t\t\tTEXTSW not MODIFIED\n");)
				/*
				 * Skip more user confirmation - just die (but
				 * cleanly)!
				 */
				(void) notify_post_destroy(base_frame, 
						DESTROY_PROCESS_DEATH,
						NOTIFY_IMMEDIATE);
				(void) notify_stop();
			}
			return (NOTIFY_DONE);
		}
	
		if (!xv_get((Xv_window) (textsw), TEXTSW_MODIFIED))
		{
			DP(printf("\t\tTEXTSW not MODIFIED\n");)
			saved = TRUE;
		}
		if(!saved)
		{
			DP(printf("\t\tTEXTSW_MODIFIED\n");)
			(void) sprintf(name_to_use, "textedit.%d", pid);
			(void) fprintf(stderr,
				MGET("attempting Store to %s ... "), 
				name_to_use);
			(void) fflush(stderr);
			if (textsw_store_file(textsw, name_to_use, 0, 0) == 0)
			{
				saved = TRUE;
			}
		}
		if(!saved)
		{
			DP(printf("\t\tTEXTSW_MODIFIED\n");)
			(void) sprintf(name_to_use, 
				"/usr/tmp/textedit.%d", pid);
			(void) fprintf(stderr,
				MGET("failed!\nAttempting Store to %s ... "),
				name_to_use);
			(void) fflush(stderr);
			if (textsw_store_file(textsw, name_to_use, 0, 0) == 0)
			{
				saved = TRUE;
			}
		}
		if(!saved)
		{
			DP(printf("\t\tTEXTSW_MODIFIED\n");)
			(void) sprintf(name_to_use, "/tmp/textedit.%d", pid);
			(void) fprintf(stderr,
				MGET("failed!\nAttempting Store to %s ... "), 
				name_to_use);
			(void) fflush(stderr);
			if (textsw_store_file(textsw, name_to_use, 0, 0) == 0)
			{
				saved = TRUE;
			}
		}
		if(!saved)
		{
			(void) fprintf(stderr,
			    MGET("failed!\nSorry, cannot save your edits: "));
		}
		else
		{
			(void) fprintf(stderr, MGET("finished; "));
		}
	}
	else if(handling_signal == 2)
	{
		(void) fprintf(stderr, "Signal catcher called recursively: ");
	}
	else if (handling_signal > 2)
	{
		DP(printf("\t_exit(3)\n");)
		_exit(3);
	}

	if (sig == SIGHUP)
	{
		return(NOTIFY_DONE);
	}

	DP(abort();)
	if (was_SIGILL) {
#ifndef lint
#ifndef SVR4
	    vec.sv_handler = SIG_DFL;
	    vec.sv_mask = vec.sv_onstack = 0;
	    sigvec(SIGSEGV, &vec, 0);
#else
	    nullmask = MALLOC (sizeof (sigset_t));
	    memset ( (char *) nullmask, 0, sizeof (sigset_t));
	    (void) sigemptyset (nullmask);
	    act.sa_handler = SIG_DFL;
	    act.sa_mask = *nullmask;
	    act.sa_flags = 0;
	    act.sa_resv[0] = act.sa_resv[1] = 0;
	    (void) sigaction (SIGSEGV, &act, (struct sigaction *) NULL);
	    free (nullmask);
#endif
#endif
	} else {
#ifndef SVR4
            vec.sv_handler = SIG_DFL; 
	    vec.sv_mask = vec.sv_onstack = 0; 
            sigvec(SIGILL, &vec, 0);
#else
	    nullmask = MALLOC (sizeof (sigset_t));
	    memset ( (char *) nullmask, 0, sizeof (sigset_t));
	    (void) sigemptyset (nullmask);
	    act.sa_handler = SIG_DFL;
	    act.sa_mask = *nullmask;
	    act.sa_flags = 0;
	    act.sa_resv[0] = act.sa_resv[1] = 0;
	    (void) sigaction (SIGILL, &act, (struct sigaction *) NULL);
	    free (nullmask);
#endif
	}

	return(NOTIFY_DONE);
}


/*
 * Misc. utilities
 */
static
die(msg1, msg2, msg3)
	char	*msg1, *msg2, *msg3;
{
	char	*dummy = "";
	DP(printf("called die\n");)
	(void)fprintf(stderr,  "%s: %s%s%s\n" , cmd_name, msg1,
			(msg2?msg2:dummy), (msg3?msg3:dummy));
	exit(4);
}

static int
off(str)
	char	*str;
{
 	DP(printf("called off\n");)
   return ((strcmp(str,  "off" ) == 0) ||
	    (strcmp(str,  "Off" ) == 0) ||
	    (strcmp(str,  "OFF" ) == 0)
	   );
}

static int
on(str)
	char	*str;
{
 	DP(printf("called on\n");)
   return ((strcmp(str,  "on" ) == 0) ||
	    (strcmp(str,  "On" ) == 0) ||
	    (strcmp(str,  "ON" ) == 0)
	   );
}

int
x_error_proc(display, error)
Display *display;
XErrorEvent *error;
{
  char msg[80];
	DP(printf("called x_error_proc\n");)

/*  There are two types of error handlers in Xlib; one to handle fatal
 *  conditions, and one to handle error events from the event server.
 *  This function handles the latter, and aborts with a core dump so
 *  we can determine where the X Error really occurred.
 *
 *  Also: you cannot directly or indirectly perform any operations on the
 *  server while in this error handler.  This means no updating the status
 *  line of the filemgr;  instead send messages to stderr.
 */

  XGetErrorText(display, error->error_code, msg, 80);
  fprintf(stderr, MGET("\nX Error (intercepted): %s\n"), msg) ;
  fprintf(stderr, MGET("Major Request Code   : %d\n"), error->request_code) ;
  fprintf(stderr, MGET("Minor Request Code   : %d\n"), error->minor_code) ;
  fprintf(stderr, MGET("Resource ID (XID)    : %u\n"), error->resourceid) ;
  fprintf(stderr, MGET("Error Serial Number  : %u\n"), error->serial) ;

  mysigproc((Notify_client) NULL, SIGFPE, (Notify_signal_mode) NULL);
 
/*NOTREACHED*/
}

/*
 *  Wrapper to textsw_insert().  If there are non displayable characters
 *  such as ISO_Latin1 characters are not displayable in Asian locales,
 *  then skip over those characters and warn the user and print out those
 *  that are displayable.
 */
void
text_insert(textsw, data, len)
Textsw textsw;
char *data;
int len;
{
    int result = 0;
    int warned = 0;
    Event event;
 
    while ( len >= 0 ) {
        result = textsw_insert(textsw, data, len);
        if ( result == len ) {
            return;
        }
        if ( !warned ) {
            /* STRING_EXTRACTION
             * This message is printed when the 'text subwindow' encounters some
             * non displayable characters.
             */
             notice_prompt(textsw, &event,
                NOTICE_MESSAGE_STRINGS, MGET("Warning:  Some characters in this message could not be\ndisplayed in this window, and the displayed message may\ncontain erroneous characters.  This usually happens when\nthe message contains characters that are not found in\nthe character set of your current locale."), 0,
                NOTICE_BUTTON_YES, MGET("Continue"),
                0);
            warned = 1;
        }
        /* skip over the non displayable character */
        data += (result + 1);
        len -= (result + 1);
    }
}
