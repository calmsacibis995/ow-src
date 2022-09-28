#ifdef lint
#ifdef sccs
static char sccsid[] = "@(#)tapetool.c	3.20 08/16/93 Copyr 1987-1990 Sun Micro";
#endif
#endif
/*      
 *	Copyright (c) 1987, 1988, 1989 Sun Microsystems, Inc.  
 *	All Rights Reserved.
 *
 *	Sun considers its source code as an unpublished, proprietary
 *	trade secret, and it is available only under strict license
 *	provisions.  This copyright notice is placed here only to protect
 *	Sun in the event the source is deemed a published work.  Dissassembly,
 *	decompilation, or other means of reducing the object code to human
 *	readable form is prohibited by the license agreement under which
 *	this code is provided to the user or company in possession of this
 *	copy.
 *
 *	RESTRICTED RIGHTS LEGEND: Use, duplication, or disclosure by the
 *	Government is subject to restrictions as set forth in subparagraph
 *	(c)(1)(ii) of the Rights in Technical Data and Computer Software
 *	clause at DFARS 52.227-7013 and in similar clauses in the FAR and
 *	NASA FAR Supplement.
 */

#include <stdio.h>
#include <errno.h>
#include <ctype.h>
#include <sys/param.h>
#include <unistd.h>
#ifdef SVR4
#include <sgtty.h>
#include <sys/termios.h>
#include <sys/stropts.h>
#else
#include <sun/fbio.h> 
#endif SVR4
#include <sys/types.h> 
#include <sys/stat.h>
#include <sys/file.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/ioctl.h> 

#include <X11/X.h>
#include <xview/xview.h>
#include <xview/panel.h>
#include <xview/canvas.h>
#include <xview/scrollbar.h>
#include <xview/fullscreen.h>
#include <xview/seln.h>
#include <xview/notice.h>
#include <xview/font.h>
#include <xview/dragdrop.h>
#include <ds_listbx.h>
#include <ds_popup.h>
#include <locale.h>
#include "ds_verbose_malloc.h"

#define MGET(s)    (char *)gettext(s)
#define MSGFILE      "SUNW_DESKSET_TAPETOOL" 
#define OFFSET   10
#define MAXNAMELEN 256
#define LM	3		/* left margin in props frame */
#define TAB	11		/* tab stop    in props frame */

#define TAR		0

#define EMPTY 		-1
#define MAXFILES	100
#define NUM_BEEPS	2

#define LENDEVPTY       9	/* used for get_master() */
#define LENDEV          5	/* used for get_slave()  */

#define READ_SELECTED    0
#define READ_ENTIRE_LIST 1
#define READ_ENTIRE_TAPE 2

#define DEFAULT_BLOCK_SIZE  "20"    /* used for remote tars */

#define FOOTER_HGT 22
#define BOTTOM_HGT 16
#define SB_WID    30 

#ifndef SV1
# define STANDALONE 1
#endif

static unsigned short Tt_image[] = {
#include "tapetool.icon"
};
Icon Tool_icon;

static unsigned short Tt_full_image[] = {
#include "tapetool.full.icon"
};

static unsigned short Tt_mask_image[] = {
#include "tapetool.mask.icon"
};

static unsigned short Doc_list_image[] = {
#include "doc_list.icon"
};
static unsigned short Folder_list_image[] = {
#include "folder_list.icon"
};
static unsigned short App_list_image[] = {
#include "app_list.icon"
};
static unsigned short Box_image[] = {
#include "box.icon"
};

Server_image	folder_list_image;
Server_image	app_list_image;
Server_image	doc_list_image;
Server_image	box_image;

Server_image	G_image[4];

typedef enum { FOLDER, APPLICATION, DOCUMENT, UNKNOWN } file_type;

extern int errno;
extern char *sys_errlist[];
extern Xv_drop_site    	Drop_site;
extern int load_event_proc();

static char Err_file[32] =  "/tmp/.tt.err" ;
static FILE     *tmpfp;
char *tempname = NULL;

typedef enum { WRITE, LIST, READ, NO_OP } op_type;
static op_type Op = NO_OP;

#ifdef SVR4
static char *Rsh_cmd = "/usr/bin/rsh";
static char *Write_tape[2] = 
	{  "/usr/sbin/tar cvf%s" ,  "/bin/cpio -o%s  2> %s"  };
#else
static char *Rsh_cmd = "/usr/ucb/rsh";
static char *Write_tape[2] = 
	{  "/bin/tar cvf%s" ,  "/bin/cpio -o%s  2> %s"  };
#endif

static char *List_tape[2] = 
	{  "%star tf%s" ,  "/bin/cpio -it%s 2> %s"  };

static char *Read_tape[2] =
	{  "%star xvf%s" ,  "/bin/cpio -i%s  2> %s"  };
Frame Tt_frame;
Panel Tt_panel;
Panel Prop_panel;

static Panel_item Write_button;
static Menu Tt_q_menu;
static Menu Tt_read_menu;
static Panel_item Tt_q;		/* list of files to go on tape */

static Frame Tt_list_frame;
static Menu Tt_list_menu;
static Panel_item Tt_list;         	/* list of files on tape */
static Menu Tt_list_panel;
static Panel_item list_file_item;

static int Nprocessed;          /* number of files processed */
static int Nfiles;              /* number of files selected */
static int read_op;

static Frame Tt_props_frame;
static Panel_item Device_item;
static Panel_item Host_item;
static Panel_item Dest_item;
static Panel_item File_item;
static Panel_item props_button_item;
Panel_item drop_site_item;
static Panel_item apply_bt;
static Panel_item reset_bt;

#ifdef CPIO
static Panel_item Method_item;
static Panel_item cpio_read_item;
static Panel_item cpio_write_item;
static Panel_item cpio_other_item;
static Panel_item cpio_except_item;
#endif

static Panel_item tar_read_item;
static Panel_item tar_write_item;
static Panel_item tar_deletedir_item;
static Panel_item tar_other_item;
static Panel_item tar_block_item;
static Panel_item tar_exclude_item;
static Panel_item tar_pattern_item;

static char Dest_name[128]   = "";
static char Default_device[] = "/dev/rmt/0mb";
static char Tape_device[80];
static char Device_name[80];
static char Host_name[64];
static char Local_host[64];
static char Write_opts[128];
static char Read_opts[128];
static char List_opts[128];
static char Dir_pattern[128]   = "";
static int Method 	    = TAR;	/* current method selected */
static int Err_exit;

static FILE *Pty_fp;
static int Pid, Master, Slave;	
static char *Line =  "/dev/ptyXX" ;
static int strip_all;
static Server_image	full_image, tape_image, mask_image;

static int 	list_button(), read_button(), write_button(); 
static void	frame_err(), frame_msg(), 
		host_status(), beep(), err_alert();
static void	clear_q(), clear_list();
static Notify_value stop();
static void delete_selected_list(), delete_selected_q();
static void change_icon_fullq(), change_icon_emptyq();
static void resize_read_plist(), resize_write_plist();

int longest;     /* longest panel label length */
char *string;
Font_string_dims dims;
Xv_Font font;
int tt_usersetsize;

main(argc, argv)
	int argc;
	char **argv;
{
        char **av = argv;
	char bind_home[MAXPATHLEN];
	static void make_main_panel(), window_check();
        static void read_env_values();
        static Notify_value frame_event_proc();
        static Notify_value panel_event_proc();
	void file_drop();

        char *s;
        char *relname;
        char *label;
	extern char *ds_relname();

        /* Checkif the user set the size on the command line */
       tt_usersetsize = FALSE;
       while ( *av ) {
	 if ( !strcmp( *av, "-Ws" ) || !strcmp( *av, "-size" ) || !strcmp( *av, "-geometry" ) ) {
	   tt_usersetsize = TRUE;
         }
         av++;
       }      

       xv_init(XV_INIT_ARGC_PTR_ARGV, &argc, argv,
		XV_USE_LOCALE, TRUE, 
		XV_LOCALE_DIR, "",
		0); 

 	ds_expand_pathname( "$OPENWINHOME/lib/locale" , bind_home); 
	bindtextdomain(MSGFILE, bind_home); 
	textdomain(MSGFILE); 

        read_env_values();

        full_image = xv_create(0, SERVER_IMAGE,
                SERVER_IMAGE_BITS, Tt_full_image,
                SERVER_IMAGE_DEPTH, 1,
                XV_WIDTH, 64,
                XV_HEIGHT, 64,
                0);

        tape_image = xv_create(0, SERVER_IMAGE,
                SERVER_IMAGE_BITS, Tt_image,
                SERVER_IMAGE_DEPTH, 1,
                XV_WIDTH, 64,
                XV_HEIGHT, 64,
                0);

        mask_image = xv_create(0, SERVER_IMAGE,
                SERVER_IMAGE_BITS, Tt_mask_image,
                SERVER_IMAGE_DEPTH, 1,
                XV_WIDTH, 64,
                XV_HEIGHT, 64,
                0);

        doc_list_image = xv_create(0, SERVER_IMAGE,
                SERVER_IMAGE_BITS, Doc_list_image,
                SERVER_IMAGE_DEPTH, 1,
                XV_WIDTH, 16,
                XV_HEIGHT, 16,
                0);

        folder_list_image = xv_create(0, SERVER_IMAGE,
                SERVER_IMAGE_BITS, Folder_list_image,
                SERVER_IMAGE_DEPTH, 1,
                XV_WIDTH, 16,
                XV_HEIGHT, 16,
                0);

        app_list_image = xv_create(0, SERVER_IMAGE,
                SERVER_IMAGE_BITS, App_list_image,
                SERVER_IMAGE_DEPTH, 1,
                XV_WIDTH, 16,
                XV_HEIGHT, 16,
                0);

        box_image = xv_create(0, SERVER_IMAGE,
                SERVER_IMAGE_BITS, Box_image,
                SERVER_IMAGE_DEPTH, 1,
                XV_WIDTH, 16,
                XV_HEIGHT, 16,
                0);

	G_image[0] = folder_list_image;
	G_image[1] = app_list_image;
	G_image[2] = doc_list_image;
	G_image[3] = box_image;

        Tool_icon= xv_create(0, ICON,
                        ICON_IMAGE, tape_image,
                        ICON_MASK_IMAGE, mask_image,
                        ICON_TRANSPARENT, TRUE,
			WIN_RETAINED, TRUE,
			XV_LABEL,  MGET("tapetool") ,
                        0);

        relname = ds_relname();
        s = MGET("Tapetool");
	label = ( char * )MALLOC( strlen( relname ) + strlen( s ) + 2 );
        sprintf( label, "%s %s", s, relname );

	Tt_frame = xv_create(NULL, FRAME,
                XV_HEIGHT,              500,
                XV_WIDTH,               500,
        	XV_LABEL, 		label,
/*		FRAME_ARGS, 		argc, argv,*//*old sunview */
                FRAME_ICON,             Tool_icon,
	        FRAME_NO_CONFIRM, 	TRUE,
                FRAME_SHOW_FOOTER,      TRUE,
                XV_HELP_DATA,           "tapetool:FRAME_INFO", 
		0);
	window_check(Tt_frame, "Tt_frame" );
        Tt_panel = xv_create(Tt_frame, PANEL,
			     WIN_CONSUME_EVENTS, WIN_LEFT_KEYS, NULL,
			     PANEL_ACCEPT_KEYSTROKE, TRUE, 0);

        window_check(Tt_panel, "Tt_panel" );

	make_main_panel();

        notify_interpose_event_func(Tt_frame, frame_event_proc, NOTIFY_SAFE);
	notify_interpose_event_func(Tt_panel, panel_event_proc, NOTIFY_SAFE);

	notify_set_signal_func((Notify_client) Tt_frame, (Notify_func) stop, SIGURG, NOTIFY_ASYNC);

	if ( !tt_usersetsize ) {
          window_fit(Tt_panel);
	  window_fit(Tt_frame);
        }
	init_dragdrop();

	place_drop_site();

	xv_main_loop(Tt_frame);
}

static void
read_env_values()
{
  char *device;

  /*
   * Get the local host name.
   */
  gethostname(Local_host, 64);
  strcpy(Host_name, Local_host);
  /*
   * Get the TAPE environment variable and
   * set it to the current Device_name else
   * just use the default device.
   */
  device = ( char * )getenv( "TAPE" );
  device = ( char * )getenv( "TAPE" );
  if ( device == NULL ) 
    strcpy( Tape_device, Default_device );
  else
    strcpy( Tape_device, device );

  strcpy( Device_name, Tape_device );
  sprintf( Write_opts, " %s", Device_name );
  sprintf( Read_opts,  " %s", Device_name );
  sprintf( List_opts,  " %s", Device_name );
  
}

static void
window_check(win, title)
	Xv_Window win;
	char *title;
{
	if (win == NULL)
	{
		fprintf(stderr, MGET("Tapetool: Unable to create window %s\n") , title);
		exit(1);
	}
}


place_drop_site()

{
	Rect	*props_item_rect;
	Rect	*drop_item_rect;
	int	props_left, props_width, drop_width;
	int	gap = xv_get(Tt_panel, PANEL_ITEM_X_GAP);
	int	panel_width = xv_get(Tt_panel, XV_WIDTH);

	props_item_rect = (Rect *) xv_get(props_button_item, PANEL_ITEM_RECT);
	props_left = props_item_rect->r_left;
	props_width = props_item_rect->r_width;

	drop_item_rect = (Rect *) xv_get(drop_site_item, PANEL_ITEM_RECT);
	drop_width = drop_item_rect->r_width;

	if ((panel_width  - drop_width - gap) >  (props_width + props_left + gap))
	{

		/* there is enough space.  Lets place the item 
		   over against the left edge of the panel. */

		xv_set(drop_site_item, 
			PANEL_ITEM_X, panel_width - drop_width - gap, 
		        PANEL_ITEM_Y, xv_row( Tt_panel, 0 ),
		        0);

	}
	else
	{
		/* window too small top move item over.  
		   Butt it against the last item on the panel. */

		xv_set(drop_site_item, 
			PANEL_ITEM_X, props_width + props_left + gap, 
		        PANEL_ITEM_Y, xv_row( Tt_panel, 0 ),
			0);
	}

}

static void
make_main_panel()		/* make panel of main frame */
{
	static void make_props_frame();
        static void add_queue_file();
        static Menu create_read_pulldown();
        static int check_dir(), check_perms();
        Panel_item  l_button, r_button, f_button;

	l_button = ( Panel_item )xv_create(Tt_panel, PANEL_BUTTON,
		PANEL_LABEL_STRING,     MGET("List...") ,
		PANEL_NOTIFY_PROC, 	list_button,
		XV_X,                   xv_col( Tt_panel, 0 ),
		XV_Y,                   xv_row( Tt_panel, 0 ),
                XV_HELP_DATA,           "tapetool:LIST_ITEM",
		0);
	r_button = ( Panel_item )xv_create(Tt_panel, PANEL_BUTTON,
		PANEL_LABEL_STRING, 	MGET("Read") ,
                PANEL_ITEM_MENU,        Tt_read_menu=create_read_pulldown(),
		XV_X,                   xv_get( l_button, XV_X ) +
		                        xv_get( l_button, XV_WIDTH ) +
		                        xv_get( Tt_panel, PANEL_ITEM_X_GAP ),
		XV_Y,                   xv_row( Tt_panel, 0 ),
                XV_HELP_DATA,           "tapetool:READ_ITEM",
		0);
	Write_button = xv_create(Tt_panel, PANEL_BUTTON,
		PANEL_LABEL_STRING, 	 MGET("Write") ,
		PANEL_NOTIFY_PROC, 	write_button,
		XV_X,                   xv_get( r_button, XV_X ) +
		                        xv_get( r_button, XV_WIDTH ) +
		                        xv_get( Tt_panel, PANEL_ITEM_X_GAP ),
		XV_Y,                   xv_row( Tt_panel, 0 ),
                PANEL_INACTIVE,         TRUE,
                XV_HELP_DATA,           "tapetool:WRITE_ITEM",
		0);
	props_button_item = xv_create(Tt_panel, PANEL_BUTTON,
		PANEL_LABEL_STRING, 	 MGET("Props...") ,
		PANEL_NOTIFY_PROC, 	make_props_frame,
		XV_X,                   xv_get( Write_button, XV_X ) +
		                        xv_get( Write_button, XV_WIDTH ) +
		                        xv_get( Tt_panel, PANEL_ITEM_X_GAP ),
		XV_Y,                   xv_row( Tt_panel, 0 ),
                XV_HELP_DATA,           "tapetool:PROPS_ITEM",
		0);

  	drop_site_item = (Panel_item)xv_create(Tt_panel, PANEL_DROP_TARGET,
    		PANEL_DROP_SITE_DEFAULT,	TRUE,
		PANEL_NOTIFY_PROC,		load_event_proc,
		XV_HELP_DATA,			"tapetool:DROP_SITE_ITEM",
    		0);

        /*  "add file"  panel stuff */
        f_button = ( Panel_item )xv_create(Tt_panel, PANEL_BUTTON,
                XV_X,           xv_col(Tt_panel, 0),
                XV_Y,           xv_row(Tt_panel, 1),
                PANEL_LABEL_STRING,      MGET("File To Write:") ,
                PANEL_NOTIFY_PROC,      add_queue_file,
                XV_HELP_DATA,           "tapetool:FILE_TO_WRITE",
                0);
        File_item = xv_create(Tt_panel, PANEL_TEXT,
	        PANEL_ITEM_X,           xv_get( f_button, XV_X ) +
			                xv_get( f_button, XV_WIDTH ) +
			                xv_get( Tt_panel, PANEL_ITEM_X_GAP ),
                PANEL_ITEM_Y,           xv_row(Tt_panel, 1),
                PANEL_VALUE_DISPLAY_LENGTH, 33,
                PANEL_NOTIFY_PROC,      add_queue_file,
                XV_HELP_DATA,           "tapetool:FILE_TO_WRITE",
                0);
	Dest_item = xv_create(Tt_panel, PANEL_TEXT,
		XV_X, 		xv_col(Tt_panel, 0),
		XV_Y, 		xv_row(Tt_panel, 2),
		PANEL_LABEL_STRING, 	 MGET("Destination:") ,
		PANEL_VALUE_DISPLAY_LENGTH, 35,
		PANEL_VALUE, 	(char*)getenv( "PWD" ),
		PANEL_NOTIFY_PROC,	check_perms,
                XV_HELP_DATA,           "tapetool:DEST_ITEM",
		0);
	Tt_q_menu = menu_create(
                MENU_TITLE_ITEM,  MGET("Write Functions") ,
		MENU_ITEM,
			MENU_STRING,  MGET("Delete Selected") ,
                        MENU_ACTION_PROC, delete_selected_q,
			0,
		MENU_ITEM,
			MENU_STRING,  MGET("Delete All") ,
			MENU_ACTION_PROC, clear_q,
			0,
                XV_HELP_DATA,           "tapetool:WRITE_MENU_INFO",
		NULL);
	Tt_q = xv_create(Tt_panel, PANEL_LIST,
		PANEL_LIST_DISPLAY_ROWS,8,
		PANEL_LIST_WIDTH,	360,
		PANEL_CHOOSE_ONE, FALSE,
		XV_Y,                 xv_row(Tt_panel, 3),
		XV_X,	              xv_col(Tt_panel, 0),  
		PANEL_ITEM_MENU,      Tt_q_menu,
                XV_HELP_DATA,         "tapetool:WRITE_SCROLL_LIST",
		0);

        xv_set(Tt_panel,  XV_WIDTH, WIN_EXTEND_TO_EDGE,
		XV_HELP_DATA,         "tapetool:WRITE_PANEL", 0);
        window_fit_height(Tt_panel);
        window_fit_width(Tt_panel);

	host_status();
}


static int
valid_device()
{
	char *device = (char *) xv_get(Device_item, PANEL_VALUE);

	/* check if empty */
	if (isspace(*device) || *device == NULL) {
		frame_err( MGET("Not A Valid Device") );
		return(0);
	}
	return(1);
}


static int
valid_hostname()
{
	char *hostname = (char *) xv_get(Host_item, PANEL_VALUE);

	/* check if empty */
	if (isspace(*hostname) || *hostname == NULL) {
		frame_err( MGET("Not A Valid Host Name") );
		return(0);
	}
	return(1);
}


static int
valid_block_size()
{
	char *size = (char *) xv_get(tar_block_item, PANEL_VALUE);

	/* check if empty and between 1 and 20 */
	if (size && *size != NULL) {
	   int size_int = atoi(size); 
	   if (size_int <= 0 || size_int > 20) {
	      frame_err( MGET("Not A Valid Block Size (1-20)") );
	      return 0;
	   }
	}
	else {
	   frame_err( MGET("Not A Valid Block Size (1-20)") );
	   return 0;
        }
	return(1);
}


static int
valid_exclude_file()
{
	char *file = (char *) xv_get(tar_exclude_item, PANEL_VALUE);
	struct stat fstatus;

	/* check if empty */
	if ( isspace(*file) || *file == NULL) {
		frame_err( MGET("Not A Valid Exclude File") );
		return(0);
	}

	/* make sure its a document file */
        stat(file, &fstatus);
        if ((fstatus.st_mode & S_IFMT) == S_IFDIR) {
		frame_err( MGET("Not A Valid Exclude File (directory)") );
		return(0);
	}
        else if ((fstatus.st_mode & S_IFMT) == S_IFREG &&
                (fstatus.st_mode & S_IEXEC)) {
		frame_err( MGET("Not A Valid Exclude File (executable file)") );
		return(0);
	}

	return(1);
}


static 
apply_button()
{
        int i;

	/* NOTE order based on method option panel setups */
        static char *write_parms[2][6] = {
		{  "F" ,  "FF" ,  "b" ,  "h" ,  "l" ,  "o"  },		/* tar */
		{  "a" ,  "B" ,  "c" ,  "v" ,  "", "" }			/* cpio */
		};

        static char *read_parms[2][8] = {
		{  "i" ,  "m" ,  "p" ,  "" , "", "", "", "" },		/* tar */
 		{  "d" ,  "f" ,  "m" ,  "s" ,  "S" ,  "u" ,  "6" , "" }	/* cpio */
		};

        static char *other_parms[2][3] = {
		{  "e" ,  "X"  },					/* tar */
		{  "c" ,  "v"  }					/* cpio */
		};

#if 0
	Method = (int)xv_get(Method_item, PANEL_VALUE);
#endif

	if (valid_device()) {
                Tape_device[0] = NULL;
                strcpy( Tape_device, (char *) xv_get( Device_item, PANEL_VALUE ) );
		ds_expand_pathname( Tape_device, Device_name );
/*		strcpy( Device_name, Tape_device );*/
	}
	else
		return(0);

	if (valid_hostname())
		strcpy(Host_name, (char *) xv_get(Host_item, PANEL_VALUE));
	else
		return(0);

	/* set write, read & list options */
	Write_opts[0] = NULL;
	Read_opts[0]  = NULL;
	List_opts[0]  = NULL;

#ifdef CPIO
	switch (Method)
	{
	case CPIO:

	    if ( strcmp(Host_name, Local_host) != 0 )
                   frame_err( MGET("Cannot cpio on a Remote System") );
            else {
	    /* write options */
	    for (i=0; i < 4; i++)
		if (xv_get(cpio_write_item, PANEL_TOGGLE_VALUE, i))
		    strcat(Write_opts, write_parms[CPIO][i]);

	    /* read & list options */
	    for (i=0; i < 7; i++)
		if (xv_get(cpio_read_item, PANEL_TOGGLE_VALUE, i))
		    strcat(Read_opts, read_parms[CPIO][i]);

	    /* check for exceptions patterns line */
	    if (xv_get(cpio_read_item, PANEL_TOGGLE_VALUE, 1)) 
	    {
	        strcat(Read_opts, " ");
	        strcat(Read_opts, xv_get(cpio_except_item, PANEL_VALUE));
	    }

	    /* add I/O device name to all options */
	    strcat(Write_opts,  " > " );
	    strcat(Write_opts, Device_name);
	    strcat(Read_opts,  " < " );
	    strcat(Read_opts, Device_name);
	    
	    sprintf(List_opts, "%s", Read_opts);
	    frame_msg( MGET("cpio options applied") );
	    break;
	    }

	case TAR:

#endif
	    /* write options */

	    /* the following are 2 mutually exclusive options */
	   if (xv_get(tar_write_item, PANEL_TOGGLE_VALUE, 1))
		    /* add sccs+ parm */
		    strcat(Write_opts, write_parms[TAR][1]);
	   else if (xv_get(tar_write_item, PANEL_TOGGLE_VALUE, 0))
		    strcat(Write_opts, write_parms[TAR][0]);

	    for (i=2; i < 6; i++)
            {
                if (xv_get(tar_write_item, PANEL_TOGGLE_VALUE, i))
                    strcat(Write_opts, write_parms[TAR][i]);
            }

	    for (i=0; i < 3; i++)
                if (xv_get(tar_read_item, PANEL_TOGGLE_VALUE, i))
                    strcat(Read_opts, read_parms[TAR][i]);

	    /* list options - only other options (below) apply to listing */
	    /* other options - they can affect write, read & list ops */

	    Err_exit = 0;
	    for (i=0; i<2; i++)
		if (xv_get(tar_other_item, PANEL_TOGGLE_VALUE, i))
		{
		    strcat(Write_opts, other_parms[TAR][i]);
		    strcat(Read_opts,  other_parms[TAR][i]);
		    strcat(List_opts,  other_parms[TAR][i]);
		    if (i == 0)
		       Err_exit = 1;
		}

	    /* add I/O device name to all options */

	    /* Add device name later for a remote tar 
	       because other options need to be added first */

	    if (strcmp(Host_name, Local_host) == 0 ) {
	        strcat(Write_opts, " ");
	        strcat(Write_opts, Device_name);
	        strcat(Read_opts, " ");
	        strcat(Read_opts, Device_name);
	    }

	    strcat(List_opts, " ");
	    strcat(List_opts, Device_name);

	    /* check for block size line & put after other write parms.
               check if remote host */
	    if (xv_get(tar_write_item, PANEL_TOGGLE_VALUE, 2))
	    {
		if (!valid_block_size())
		  return(0);
		else if (strcmp(Host_name, Local_host) == 0) {
		    strcat(Write_opts, " ");
		    strcat(Write_opts, (char *) xv_get(tar_block_item, PANEL_VALUE));
		}
		else {
		    strcat(Write_opts,  " - " );
		    strcat(Write_opts, (char *) xv_get(tar_block_item, PANEL_VALUE));
		    strcat(Read_opts,  "Bb - " );
		    strcat(Read_opts, (char *) xv_get(tar_block_item, PANEL_VALUE));
		}
	    }
	    /* Block size not specified, so use default for a remote tar */
	    else if (strcmp(Host_name, Local_host) != 0) {
		strcat(Write_opts,  "b - " );
		strcat(Write_opts, DEFAULT_BLOCK_SIZE);
		strcat(Read_opts,  "Bb - " );
		strcat(Read_opts, DEFAULT_BLOCK_SIZE);
	    }

            strip_all = FALSE;
           /* strip entire path */
	   if (xv_get(tar_deletedir_item, PANEL_TOGGLE_VALUE, 1))
              strip_all = TRUE;
           else if (xv_get(tar_deletedir_item, PANEL_TOGGLE_VALUE, 2))
           {
                strcpy(Dir_pattern,(char *) xv_get(tar_pattern_item, PANEL_VALUE));
                if (Dir_pattern[0] != '\0' && !check_dirpattern())
                {
                   frame_err( MGET("Not A Valid Directory Pattern") );
                   return (0);
                }
           }   
           else /* strip nothing */
              Dir_pattern[0] = '\0';

	    /* check for exclude file & put after all parms */
	   if (xv_get(tar_other_item, PANEL_TOGGLE_VALUE, 1))
   	   {
		if (valid_exclude_file()) {
		  strcat(Write_opts, " ");
		  strcat(Write_opts, (char *) xv_get(tar_exclude_item, PANEL_VALUE));
		  strcat(Read_opts, " ");
		  strcat(Read_opts, (char *) xv_get(tar_exclude_item, PANEL_VALUE));
		  strcat(List_opts, " ");
		  strcat(List_opts, (char *) xv_get(tar_exclude_item, PANEL_VALUE));
		}
		else
		    return(0);
	    }
            

	    frame_msg( MGET("tar options applied") );
#ifdef CPIO
	    break;
	}
#endif
	host_status();

	/* if unpinned, then hide the frame */
        if (xv_get(Tt_props_frame, FRAME_CMD_PUSHPIN_IN) == FALSE)
		xv_set(Tt_props_frame, XV_SHOW, FALSE, 0);

	return(1);
}

/*ARGSUSED*/
static Notify_value
reset_button(button, event)

Panel_item	button;
Event *event;
{
	int i;
#ifdef CPIO
	int choice = (int)xv_get(Method_item, PANEL_VALUE);
#endif
        sprintf(Host_name, Local_host);
        strcpy(Device_name, Tape_device);
	sprintf(Write_opts, " %s", Device_name);
	sprintf(Read_opts, " %s", Device_name);
	sprintf(List_opts, " %s", Device_name);

	xv_set(Device_item, PANEL_VALUE, Device_name, 0);
        xv_set(Host_item, PANEL_VALUE, Host_name, 0);

#ifdef CPIO
	switch (choice)		/* only reset displayed options */
	{
	case CPIO:

	    for (i=0; i < 2; i++)
		if (xv_get(cpio_write_item, PANEL_TOGGLE_VALUE, i))
		    xv_set(cpio_write_item, PANEL_TOGGLE_VALUE, i, FALSE, 0);

	    for (i=0; i < 8; i++)
		if (xv_get(cpio_read_item, PANEL_TOGGLE_VALUE, i))
		    xv_set(cpio_read_item, PANEL_TOGGLE_VALUE, i, FALSE, 0);

	    for (i=0; i < 2; i++)
		if (xv_get(cpio_other_item, PANEL_TOGGLE_VALUE, i))
		    xv_set(cpio_other_item, PANEL_TOGGLE_VALUE, i, FALSE, 0);

	    xv_set(cpio_except_item, PANEL_VALUE, "", NULL);
	    xv_set (cpio_except_item, XV_SHOW, FALSE, 0);

	    frame_msg( MGET("cpio reset applied") );
	    break;

	case TAR:
#endif
	for (i=0; i < 6; i++)
		if (xv_get(tar_write_item, PANEL_TOGGLE_VALUE, i))
			xv_set(tar_write_item, PANEL_TOGGLE_VALUE, i, FALSE, 0);
	for (i=0; i < 3; i++)
		if (xv_get(tar_read_item, PANEL_TOGGLE_VALUE, i))
			xv_set(tar_read_item, PANEL_TOGGLE_VALUE, i, FALSE, 0);
	for (i=0; i < 2; i++)
		if (xv_get(tar_other_item, PANEL_TOGGLE_VALUE, i))
			xv_set(tar_other_item, PANEL_TOGGLE_VALUE, i, FALSE, 0);
	if (!xv_get(tar_deletedir_item, PANEL_TOGGLE_VALUE, 0))
		xv_set(tar_deletedir_item, PANEL_TOGGLE_VALUE, 0, TRUE, 0);
	if (xv_get(tar_deletedir_item, PANEL_TOGGLE_VALUE, 1))
		xv_set(tar_deletedir_item, PANEL_TOGGLE_VALUE, 1, FALSE, 0);
	if (xv_get(tar_deletedir_item, PANEL_TOGGLE_VALUE, 2))
		xv_set(tar_deletedir_item, PANEL_TOGGLE_VALUE, 2, FALSE, 0);
	
      	xv_set(tar_block_item, PANEL_VALUE, "", NULL);
       	xv_set (tar_block_item, XV_SHOW, FALSE, 0);

       	xv_set(tar_exclude_item, PANEL_VALUE, "", NULL);
	xv_set (tar_exclude_item, XV_SHOW, FALSE, 0);

	xv_set(tar_pattern_item, PANEL_VALUE, "", NULL);
	xv_set (tar_pattern_item, XV_SHOW, FALSE, 0);

	Dir_pattern[0] = '\0';
	strip_all = FALSE;

	frame_msg( MGET("tar reset applied") );
#ifdef CPIO
	    break;
	}
#endif
	host_status();

      	xv_set(button, PANEL_NOTIFY_STATUS, XV_ERROR, 0);
	
	return(1);
}

static
display_pgm_options()		/* display the proper method options */
{
	static void 	display_tar_panel();

#ifdef CPIO
	static void 	display_cpio_panel();
	int choice = (int)xv_get(Method_item, PANEL_VALUE);
#endif

#ifdef CPIO
        if (choice == CPIO) {
		display_tar_panel  (FALSE);
		display_cpio_panel (TRUE);
	}
	else {
		display_cpio_panel (FALSE);
#endif
		display_tar_panel  (TRUE);
#ifdef CPIO
	}
#endif
}


static int
check_dir()			/* check if valid directory */
{
	struct stat fstatus;

        strcpy(Dest_name, (char *) xv_get(Dest_item, PANEL_VALUE));
        stat(Dest_name, &fstatus);
        if ((fstatus.st_mode & S_IFMT) != S_IFDIR)
	{
		frame_err( MGET("Not A Valid Directory Name") );
		return(0);
	}

	frame_msg("");		/* clear frame footer */
	return(1);
}

static int
check_perms()                   /* check if write permissions */
{

        if (!check_dir())
           return 0;;
        if (access(Dest_name, W_OK) == -1)
        {
                frame_err( MGET("Destination Directory Does Not Have Write Permission") );
                return(0);
        }

        return(1);
}

static int
check_dirpattern()                     /* check if valid directory pattern */
{
        struct stat fstatus;

        stat((char *) xv_get(tar_pattern_item, PANEL_VALUE), &fstatus);
        if ((fstatus.st_mode & S_IFMT) != S_IFDIR)
        {
                frame_err( MGET("Not A Valid Directory Pattern") );
                return(0);
        }

        frame_msg("");          /* clear frame footer */
        return(1);
}

static void
make_props_frame()		/* make/display props frame */
{
#ifdef CPIO
	static void  display_cpio_panel(), make_cpio_panel();
#endif
	static void make_tar_panel();
	static void display_tar_panel();
	static apply_button();
	static Notify_value props_event_proc();
	int row = 0;

	if (Tt_props_frame)
	{
		xv_set(Tt_props_frame, 
			XV_SHOW, 		TRUE, 
			FRAME_SHOW_FOOTER, 	FALSE,
			0);
		return;
	}
	
	if ((Tt_props_frame = xv_create(Tt_frame, FRAME_CMD,

/*
		FRAME_CLASS, 		FRAME_CLASS_COMMAND,
		FRAME_ADJUSTABLE, 	FALSE,
*/
                FRAME_CMD_PUSHPIN_IN,   TRUE,
		XV_LABEL, 		 MGET("Tapetool: Properties") ,
		FRAME_NO_CONFIRM, 	TRUE,
		FRAME_SHOW_FOOTER, 	FALSE,
                XV_HELP_DATA,           "tapetool:PROPS_FRAME_INFO",
		0)) == NULL)
        {
                xv_set(Tt_frame, FRAME_LEFT_FOOTER,
                                 MGET("Cannot create props window") , 0);
                return;
        }


        Prop_panel = xv_get(Tt_props_frame, FRAME_CMD_PANEL);
    
	/* COMMON panel items */

	string = MGET("Device:");
	Device_item = xv_create(Prop_panel, PANEL_TEXT,
		XV_Y, 		xv_row(Prop_panel, row++),
		PANEL_LABEL_STRING, 	 string,
		PANEL_VALUE, 		Device_name,
		PANEL_VALUE_DISPLAY_LENGTH, 15,
                XV_HELP_DATA,           "tapetool:DEVICE_ITEM",
		0);

	font = (Xv_Font)xv_get(Tt_frame , XV_FONT);
	(void)xv_get(font, FONT_STRING_DIMS, string, &dims);	
	longest = dims.width;

	string = MGET("Host Name:");
	Host_item = xv_create(Prop_panel, PANEL_TEXT,
		XV_Y, 		xv_row(Prop_panel, row++),
		PANEL_LABEL_STRING, 	 string,
		PANEL_VALUE, 		Host_name,
		PANEL_VALUE_DISPLAY_LENGTH, 15,
                XV_HELP_DATA,           "tapetool:HOST_ITEM",
		0);

	(void)xv_get(font, FONT_STRING_DIMS, string, &dims);
	if ( dims.width > longest ) {
		longest = dims.width;
	}

#if 0
	Method_item = xv_create(Prop_panel, PANEL_CHOICE,
		XV_Y, 	xv_row(Prop_panel, row++),
		PANEL_VALUE_X, 		xv_col(Prop_panel, TAB),
		PANEL_LABEL_STRING, 	 MGET("Method:") ,
#ifdef CPIO
		PANEL_CHOICE_STRINGS, 	 MGET("tar") ,  MGET("cpio") , 0,
#endif
		PANEL_CHOICE_STRINGS, 	 MGET("tar") , 0,
		PANEL_NOTIFY_PROC,	display_pgm_options,
                XV_HELP_DATA,           "tapetool:METHOD_ITEM",
		0);
#endif

	(void) xv_create(Prop_panel, PANEL_MESSAGE,
		XV_Y, 		xv_row(Prop_panel, row++)+15,
		XV_X, 		xv_col(Prop_panel, LM),
                PANEL_LABEL_BOLD,       TRUE,
		PANEL_LABEL_STRING, 	 MGET("Tar Options") ,
                XV_HELP_DATA,           "tapetool:METHOD_OPTIONS_ITEM",
		0);
	row++;

	apply_bt = xv_create(Prop_panel, PANEL_BUTTON,
		PANEL_LABEL_STRING, 	 MGET("Apply") ,
		PANEL_NOTIFY_PROC, 	apply_button,
		XV_Y, 		xv_row(Prop_panel, row+8),
		/*
		XV_X, 		xv_col(Prop_panel, TAB+5),
		*/
                XV_HELP_DATA,           "tapetool:APPLY_ITEM",
		0);
	reset_bt = xv_create(Prop_panel, PANEL_BUTTON,
		PANEL_LABEL_STRING, 	 MGET("Reset") ,
		PANEL_NOTIFY_PROC, 	reset_button,
                XV_HELP_DATA,           "tapetool:RESET_ITEM",
		0);

	/* OPTIONAL panels, based on Method_item selected */

#ifdef CPIO
	make_cpio_panel(Prop_panel, row);
#endif
	make_tar_panel(Prop_panel, row);

	xv_set(Device_item, PANEL_VALUE_X, xv_col(Prop_panel, LM) + longest, NULL);
	xv_set(Host_item, PANEL_VALUE_X, xv_col(Prop_panel, LM) + longest, NULL);

	display_pgm_options();		/* display appropiate panel */

        xv_set(Prop_panel,  XV_WIDTH, WIN_EXTEND_TO_EDGE, 0);
        window_fit(Prop_panel);
        window_fit(Tt_props_frame);	

	ds_position_popup(Tt_frame, Tt_props_frame, DS_POPUP_LOR);

        xv_set(Tt_props_frame, XV_SHOW, TRUE, 0);

	notify_interpose_event_func(Tt_props_frame, props_event_proc, NOTIFY_SAFE);
}

static int
make_list_frame()		/* make/display list frame */
{
	static void select_all_list();
	static void clear_all_list();
	static void add_list_file();
        static Notify_value list_frame_event_proc();
	Panel_item f_button;

	if (Tt_list_frame)
	{
		/* make sure the popup won't show when the basewin is in
		icon state */
		if (xv_get(Tt_frame, FRAME_CLOSED)) {
                   if (xv_get(Tt_list_frame, XV_SHOW))
                      xv_set(Tt_list_frame, XV_SHOW, FALSE, 0);
                }
		else if (!xv_get(Tt_list_frame, XV_SHOW)) {
                      xv_set(Tt_list_frame, XV_SHOW, TRUE, 0);
                      return 0;
                   }
		return 1;
	}

	if ((Tt_list_frame = xv_create(Tt_frame, FRAME_CMD,
                XV_HEIGHT,              400,
                XV_WIDTH,               400,
                FRAME_CMD_PUSHPIN_IN,   TRUE,
                FRAME_SHOW_RESIZE_CORNER,       TRUE,
		XV_LABEL, 		 MGET("Tapetool: Tape Contents/Files to Read") ,
		FRAME_NO_CONFIRM, 	TRUE,
		FRAME_SHOW_FOOTER, 	FALSE,
                XV_HELP_DATA,           "tapetool:READ_FRAME",
		0)) == NULL)
	{
		xv_set(Tt_frame, FRAME_LEFT_FOOTER, 
				 MGET("Cannot create list window") , 0);
		return 0;
	}

        Tt_list_panel = xv_get(Tt_list_frame, FRAME_CMD_PANEL);
        xv_set(Tt_list_panel,
                XV_HELP_DATA,           "tapetool:READ_PANEL",
                WIN_WIDTH, WIN_EXTEND_TO_EDGE,
                0);
        window_check(Tt_list_panel, "Tt_list_panel" );

        /* create  "add file"  panel stuff */
        f_button = xv_create(Tt_list_panel, PANEL_BUTTON,
		XV_X,           	xv_col(Tt_list_panel, 0),
		XV_Y,           	xv_row(Tt_list_panel, 0),
                PANEL_LABEL_STRING,     MGET("File To Read:") ,
                PANEL_NOTIFY_PROC,      add_list_file,
                XV_HELP_DATA,           "tapetool:FILE_TO_READ",
                0);
        list_file_item = xv_create(Tt_list_panel, PANEL_TEXT,
		PANEL_ITEM_X,  		xv_get(f_button, XV_X) +
					xv_get(f_button, XV_WIDTH) +
					xv_get(Tt_list_panel, PANEL_ITEM_X_GAP),
		PANEL_ITEM_Y,   	xv_row(Tt_list_panel, 0),
                PANEL_VALUE_DISPLAY_LENGTH, 33,
                PANEL_NOTIFY_PROC,      add_list_file,
                XV_HELP_DATA,           "tapetool:FILE_TO_READ",
                0);

	Tt_list_menu = menu_create(
                MENU_TITLE_ITEM,  MGET("Read Functions") ,
		MENU_ITEM,
			MENU_STRING,  MGET("Select All") ,
			MENU_ACTION_PROC, select_all_list,
			0,
		MENU_ITEM,
			MENU_STRING,  MGET("Deselect All") ,
			MENU_ACTION_PROC, clear_all_list,
			0,
                MENU_ITEM,
                        MENU_STRING,  MGET("Delete Selected") ,
                        MENU_ACTION_PROC, delete_selected_list,
                        0,
                XV_HELP_DATA,           "tapetool:READ_MENU_INFO",
		0);

        Tt_list = (Panel_item)xv_create (Tt_list_panel, PANEL_LIST,
		PANEL_LIST_DISPLAY_ROWS,8,
		PANEL_LIST_WIDTH,	360,
		PANEL_CHOOSE_ONE,       FALSE,
                PANEL_ITEM_MENU,        Tt_list_menu,
		XV_X,           xv_col(Tt_list_panel, 0),
		XV_Y,           xv_row(Tt_list_panel, 1),
		XV_HELP_DATA,           "tapetool:READ_SCROLL_LIST",
                0);

        window_fit(Tt_list_panel);
        window_fit(Tt_list_frame);
         
	ds_position_popup(Tt_frame, Tt_list_frame, DS_POPUP_AOB);

	/* make sure the popup won't show when the basewin is in
	   icon state */
	if (xv_get(Tt_frame, FRAME_CLOSED)) {
	   if (xv_get(Tt_list_frame, XV_SHOW))
	      xv_set(Tt_list_frame, XV_SHOW, FALSE, 0);
        }
	else if (!xv_get(Tt_list_frame, XV_SHOW))
            xv_set(Tt_list_frame, XV_SHOW, TRUE, 0); 

        notify_interpose_event_func(Tt_list_frame, list_frame_event_proc, NOTIFY_SAFE);
        return 0;
}


#ifdef CPIO
static void
make_cpio_panel(panel, row)	/* make cpio method panel items */
	register Panel panel;
	int   row;		/* row to start at */
{
	static Panel_setting cpio_read_proc();

        cpio_write_item = xv_create(panel, PANEL_TOGGLE,
		XV_SHOW,	FALSE,
		PANEL_LABEL_STRING,	 MGET("Write:") ,
                PANEL_LAYOUT,           PANEL_HORIZONTAL,
                PANEL_CHOICE_STRINGS,    MGET("Reset Time") ,
                                         MGET("Block I/O") ,
					0,
                PANEL_VALUE_X,          xv_col(panel, TAB),
                XV_Y,           xv_row(panel, row++),
                XV_HELP_DATA,           "tapetool:CPIO_WRITE_ITEM",
                0);

        cpio_read_item = xv_create(panel, PANEL_TOGGLE,
		XV_SHOW,	FALSE,
                PANEL_LABEL_STRING,      MGET("Read:") , 
                PANEL_LAYOUT,           PANEL_VERTICAL,
                PANEL_CHOICE_STRINGS,    MGET("Make Dirs") ,
                                         MGET("Except") ,
                                         MGET("Retain") ,
					 MGET("Swap bytes") ,
					 MGET("Swap words") ,
					 MGET("Clobber") ,
					 MGET("Version 6") ,
					 MGET("Recursive") ,
					0,
                PANEL_CHOICE_NROWS, 	3, 
                PANEL_LABEL_X,          30,
                PANEL_LABEL_Y,          172,
                XV_HELP_DATA,           "tapetool:CPIO_READ_ITEM",
                PANEL_NOTIFY_PROC,      cpio_read_proc,
                0);
	row +=4;

        cpio_other_item = xv_create(panel, PANEL_TOGGLE,
		XV_SHOW,	FALSE,
		PANEL_LABEL_STRING,	 MGET("Other:") ,
                PANEL_LAYOUT,           PANEL_HORIZONTAL,
                PANEL_CHOICE_STRINGS,    MGET("ASCII") ,
                                         MGET("Verbose") ,
					0,
                PANEL_VALUE_X,          xv_col(panel, TAB),
                XV_Y,          xv_row(panel, row++),
                XV_HELP_DATA,           "tapetool:CPIO_OTHER_ITEM",
                0);

	cpio_except_item = xv_create(panel, PANEL_TEXT,
                PANEL_LABEL_STRING,      MGET("Exceptions:") ,
                PANEL_VALUE_DISPLAY_LENGTH, 25,
                PANEL_LABEL_BOLD,       TRUE,
                XV_SHOW,        FALSE,
                PANEL_VALUE_X,          xv_col(panel, TAB),
                XV_Y,           xv_row(panel, row++),
                XV_HELP_DATA,           "tapetool:CPIO_EXCEPT_ITEM",
                0);
}
#endif

static void
make_tar_panel(panel, row)	/* make tar method panel items */
	register Panel panel;
	int   row;		/* row to start at */
{
#ifdef SVR4
        static Panel_setting tar_write_proc();
	static Panel_setting tar_other_proc();
	static Panel_setting tar_deletedir_proc();
#else
	static void tar_write_proc();
	static void tar_other_proc();
	static void tar_deletedir_proc();
#endif SVR4

		string = MGET("Write:");
        tar_write_item = xv_create(panel, PANEL_TOGGLE,
		XV_SHOW,	FALSE,
		PANEL_LABEL_STRING,	 string,
#if 0
                PANEL_LAYOUT,           PANEL_VERTICAL,
#endif
                PANEL_CHOICE_STRINGS,	 MGET("No SCCS") ,
					 MGET("No SCCS+") ,
                			 MGET("Block I/O") ,
                                         MGET("Sym Links") ,
					 MGET("Show Errs") ,
					 MGET("Suppress") ,
					0,
                PANEL_CHOICE_NROWS,	2,
                XV_Y,           	xv_row(panel, row++),
#if 0
                PANEL_LABEL_Y,          140,
#endif
                XV_HELP_DATA,           "tapetool:TAR_WRITE_ITEM",
                PANEL_NOTIFY_PROC,      tar_write_proc,
                0);

		(void)xv_get(font, FONT_STRING_DIMS, string, &dims);
		if ( dims.width > longest ) {
			longest = dims.width;
		}

        row++;
		string = MGET("Strip Path:");
        tar_deletedir_item = xv_create(panel, PANEL_CHOICE,
                XV_Y,           xv_row(panel, row++),
                PANEL_LABEL_STRING,      string,
                PANEL_CHOICE_STRINGS,    MGET("None") ,  MGET("All") ,  MGET("Pattern") , 0,
                PANEL_NOTIFY_PROC,      tar_deletedir_proc,
                XV_HELP_DATA,           "tapetool:TAR_DELETEDIR_ITEM",
                0);

		(void)xv_get(font, FONT_STRING_DIMS, string, &dims);
		if ( dims.width > longest ) {
			longest = dims.width;
		}

		string = MGET("Read:");
        tar_read_item = xv_create(panel, PANEL_TOGGLE,
		XV_SHOW,	FALSE,
		PANEL_LABEL_STRING,	 string,
                PANEL_CHOICE_STRINGS,    MGET("No Check") ,
					 MGET("Mod Time") ,
					 MGET("Orig Mode") ,
					0,
                XV_Y,           xv_row(panel, row++),
                XV_HELP_DATA,           "tapetool:TAR_READ_ITEM",
                0);

		(void)xv_get(font, FONT_STRING_DIMS, string, &dims);
		if ( dims.width > longest ) {
			longest = dims.width;
		}

		string = MGET("Other:");
        tar_other_item = xv_create(panel, PANEL_TOGGLE,
		XV_SHOW,	FALSE,
		PANEL_LABEL_STRING,	 string,
                PANEL_LAYOUT,           PANEL_HORIZONTAL,
                PANEL_CHOICE_STRINGS,    MGET("Err Exit") ,
					 MGET("Exclude") ,
					0,
                XV_Y,           xv_row(panel, row++),
                XV_HELP_DATA,           "tapetool:TAR_OTHER_ITEM",
                PANEL_NOTIFY_PROC,      tar_other_proc,
                0);

		(void)xv_get(font, FONT_STRING_DIMS, string, &dims);
		if ( dims.width > longest ) {
			longest = dims.width;
		}

	string = MGET("Block Size:");
	tar_block_item = xv_create(panel, PANEL_TEXT,
                PANEL_LABEL_STRING,  string,
                PANEL_VALUE_DISPLAY_LENGTH, 6,
                PANEL_LABEL_BOLD,       TRUE,
                XV_SHOW,        FALSE,
                XV_Y,           xv_row(panel, row++),
                XV_HELP_DATA,           "tapetool:TAR_BLOCK_ITEM",
                0);

		(void)xv_get(font, FONT_STRING_DIMS, string, &dims);
		if ( dims.width > longest ) {
			longest = dims.width;
		}

	string = MGET("Filename:");
	tar_exclude_item = xv_create(panel, PANEL_TEXT,
                PANEL_LABEL_STRING,   string,
                PANEL_VALUE_DISPLAY_LENGTH, 25,
                PANEL_LABEL_BOLD,       TRUE,
                XV_SHOW,        FALSE,
                XV_Y,          xv_row(panel, row++),
                XV_HELP_DATA,           "tapetool:TAR_EXCLUDE_ITEM",
                0);

		(void)xv_get(font, FONT_STRING_DIMS, string, &dims);
		if ( dims.width > longest ) {
			longest = dims.width;
		}

	  string = MGET("Dir Pattern:");
      tar_pattern_item = xv_create(panel, PANEL_TEXT,
                PANEL_LABEL_STRING,    string,
                PANEL_VALUE_DISPLAY_LENGTH, 25,
                PANEL_LABEL_BOLD,       TRUE,
                PANEL_SHOW_ITEM,        FALSE,
                PANEL_ITEM_Y,           xv_row(panel, row++),
                XV_HELP_DATA,           "tapetool:TAR_PATTERN_ITEM",
                0);

	(void)xv_get(font, FONT_STRING_DIMS, string, &dims);
	if ( dims.width > longest ) {
		longest = dims.width;
	}
	longest += OFFSET;
	xv_set(tar_write_item, PANEL_VALUE_X, xv_col(panel, LM) + longest, NULL);
	xv_set(tar_deletedir_item, PANEL_VALUE_X, xv_col(panel, LM) + longest, NULL);
	xv_set(tar_read_item, PANEL_VALUE_X, xv_col(panel, LM) + longest, NULL);
	xv_set(tar_other_item, PANEL_VALUE_X, xv_col(panel, LM) + longest, NULL);
	xv_set(tar_block_item, PANEL_VALUE_X, xv_col(panel, LM) + longest, NULL);
	xv_set(tar_exclude_item, PANEL_VALUE_X, xv_col(panel, LM) + longest, NULL);
	xv_set(tar_pattern_item, PANEL_VALUE_X, xv_col(panel, LM) + longest, NULL);
}

#ifdef CPIO
static void
display_cpio_panel(show)	/* if TRUE show cpio items else hide them */
	int show;
{
	xv_set(cpio_write_item, XV_SHOW, show, 0);
        xv_set(cpio_read_item,  XV_SHOW, show, 0);
        xv_set(cpio_other_item,  XV_SHOW, show, 0);
	if (xv_get (cpio_read_item, PANEL_TOGGLE_VALUE, 1))
		xv_set(cpio_except_item, XV_SHOW, show, 0);
}
#endif

static void
display_tar_panel(show)		/* if TRUE show tar items else hide them */
	int show;
{
	xv_set(tar_write_item, XV_SHOW, show, 0);
        xv_set(tar_read_item,  XV_SHOW, show, 0);
	xv_set(tar_deletedir_item,  XV_SHOW, show, 0);
        xv_set(tar_other_item, XV_SHOW, show, 0);
	if (xv_get (tar_write_item, PANEL_TOGGLE_VALUE, 2))
		xv_set(tar_block_item, XV_SHOW, show, 0);
	if (xv_get (tar_other_item, PANEL_TOGGLE_VALUE, 1))
		xv_set(tar_exclude_item, XV_SHOW, show, 0);
	if (xv_get (tar_deletedir_item, PANEL_TOGGLE_VALUE, 2))
		xv_set(tar_pattern_item, XV_SHOW, show, 0);
}

#ifdef CPIO
static void
cpio_read_proc()      		/* Display cpio EXCEPTIONS line */
{
        xv_set(cpio_except_item, 
		XV_SHOW,
            	(int) xv_get (cpio_read_item, PANEL_TOGGLE_VALUE, 1), 
		0);
}
#endif

#ifdef SVR4
static Panel_setting
#else
static void
#endif SVR4
tar_write_proc()		/* Display tar BLOCK SIZE line */
{
	xv_set(tar_block_item, XV_SHOW, 
		(int) xv_get(tar_write_item, PANEL_TOGGLE_VALUE, 2), 0);
	xv_set(tar_exclude_item, XV_SHOW, 
		(int) xv_get(tar_other_item, PANEL_TOGGLE_VALUE, 1), 0);
}

#ifdef SVR4
static Panel_setting
#else
static void
#endif SVR4
tar_deletedir_proc()                /* for stripping pathnames */
{
	xv_set(tar_pattern_item, XV_SHOW, 
		(int)xv_get(tar_deletedir_item, PANEL_TOGGLE_VALUE, 2), 0);
}

#ifdef SVR4
static Panel_setting
#else
static void
#endif SVR4
tar_other_proc()		/* Display tar EXCLUDE FILE line */
{
	xv_set (tar_exclude_item, XV_SHOW, 
		(int)xv_get(tar_other_item, PANEL_TOGGLE_VALUE, 1), 0);
}

static void 
select_all_list()
{
	list_select_all(Tt_list);
}

static void 
clear_all_list()
{
	list_deselect_all(Tt_list);
}

static void
delete_selected_list()
{

        list_delete_selected(Tt_list);
}

static void
delete_selected_q()
{

        list_delete_selected(Tt_q);
}

static int
add_file_to_q(fname)
	char *fname;
{
	char buf[128], filen[MAXNAMELEN], filename[MAXNAMELEN];
	register char 	*d_p, *b_p;
	struct stat fstatus;
	file_type filetype = UNKNOWN;
	char *common_path = "";
	int last = -1; /* index to last path segment to match between 2 paths */
	extern void expand_path();

	int i = -1;

	(void)expand_path(fname, filename);
	/* find file type and assign appropiate icon */
        if (stat(filename, &fstatus) ==  -1)
        {
                sprintf(buf,  MGET("Unable to access %s") , filename);
                frame_err(buf);
                return(0);
        }
        if ((fstatus.st_mode & S_IFMT) == S_IFDIR)
                filetype = FOLDER;
        else if ((fstatus.st_mode & S_IFMT) == S_IFREG &&
                (fstatus.st_mode & S_IEXEC))
                filetype = APPLICATION;
        else
                filetype = DOCUMENT;

	/*
	 * There are 2 ways to save filenames.  (1) Strip the entire
	 * path from the filename, or (2) Strip a portion of the path 
	 * which matches the Dest_item entry from the filename.
	 *
	 * Option (1) has precedence.  Both require saving the filename 
	 * and stripped pathname in the entry & client_data_entry fields 
	 * of a list entry.  
	 */
	
	if (strip_all) 
	{
		/* strip the entire path from the filename */
		b_p = (char *) strrchr(filename, '/')+1;

		/* setup buf with entire path minus filename */
		strcpy(buf,filename);
		d_p = (char *) strrchr(buf, '/');
		*d_p = NULL;
	}
	else
	{
		/* strip portion of pathname which matches the Dest_item */

		b_p = filename;
		d_p = Dir_pattern;
		buf[0] = NULL;

		/* compare the filename and directory name */
		while (*b_p && *d_p && (*b_p == *d_p))
		{
			i++;
			if (*b_p == '/') 	/* if path segment found */
				last = i;	/* save position */
			b_p++, d_p++;
		}
                /* there is still part of the pattern not compared
                   so they must be different */
                if ((*d_p) != NULL)
                {
                   i = -1;
                   buf[0]='\0';
                   b_p = filename;
                }

		if (i != -1)	/* some common chars existed */
		{
			/* if last path segment completed ok, save segment */
			if ((*b_p == '/' && !(*d_p)) || 
			    (*d_p == '/' && !(*b_p)))
				last = i+1;

			/* set ptr to filename minus last path segment */
			b_p = filename+(last+1);

			/* copy common path to buffer */
			strncpy(buf, filename, last+1);
			if (last==0)
				buf[1] = NULL;	    /* save root slash */
			else
				buf[last] = NULL;   /* overwrite ending slash */
		}
	}

	if (!(common_path = MALLOC(strlen(buf) + 1 ))) 
		fprintf(stderr, 
		 MGET("Tapetool: unable to allocate memory in add_file_to_q()\n")); 
	else
		strcpy(common_path, buf);

        if (common_path[0] != '\0')
            sprintf (filen,  "%s (%s)" , b_p, common_path);
        else strcpy(filen, b_p);
	/* add file with appropiate icon to list */
	list_add_entry(Tt_q, filen, G_image[filetype], common_path,
                      (int)xv_get(Tt_q, PANEL_LIST_NROWS), TRUE);
	return(1);
}

/*ARGSUSED*/
static void                     /* Called when main  "add file"  button pressed */
add_queue_file(item, event)
        Panel_item item;
        Event      *event;
{
        char *filename = (char *) xv_get(File_item, PANEL_VALUE);

        /* make sure its not empty */
        if (*filename == NULL)
        {
                frame_err( MGET("Empty file name") );
                return;
        }

        if (add_file_to_q(filename))
        {
           /* change icon to reflect something is in queue */
           change_icon_fullq();

           /* enable the write button */
           xv_set(Write_button, PANEL_INACTIVE, FALSE, 0);
        }
}


/*ARGSUSED*/
static void                     /* Called when list  "add file"  button pressed */
add_list_file(item, event)
        Panel_item item;
        Event      *event;
{
        char *fname = (char *) xv_get(list_file_item, PANEL_VALUE);
	char filename[MAXNAMELEN];
	extern void expand_path();

        /* make sure its not empty */
        if (*fname == NULL)
        {
                frame_err( MGET("Empty file name") );
                return;
        }
	(void)expand_path(fname, filename);
        list_add_entry(Tt_list, filename, (Server_image) NULL, NULL,
                      (int)xv_get(Tt_list, PANEL_LIST_NROWS), TRUE);

}

void
file_drop(fname)		/* Called when a file list dropped on tool */
	char *fname;
{
	char filename[MAXNAMELEN];
	extern void expand_path();

	/* comment out this code since Dest_name will not be used in
	   dnd file. And makes this consistant to type in filed 
	if (!check_dir())	// check Dest_name is a valid directory
		return;
        */

        /* make sure its not empty */
        if (*fname == NULL)
        {
                frame_err( MGET("Empty file name") );
                return;
        }

        if (add_file_to_q(fname))
        {
           /* change icon to reflect something is in queue */
           change_icon_fullq();

           /* enable the write button */
           xv_set(Write_button, PANEL_INACTIVE, FALSE, 0);
        }
}

static Notify_value
frame_event_proc(frame, evnt, arg, type)
	Frame           frame;
	Event           *evnt;
	Notify_arg      arg;
	Notify_event_type type;
{
	int closed_initial, closed_current;
	Notify_value value;

	/* determine initial state of frame */
	closed_initial = (int) xv_get(frame, FRAME_CLOSED);

	/* let frame operate on the event */
	value = notify_next_event_func(frame, (Notify_event) evnt, arg, type);

	/* determine current state of frame */
	closed_current = (int) xv_get(frame, FRAME_CLOSED);

	/* if state changed and currently open */
	if ((closed_initial != closed_current) && !closed_current)
	{
		/*
		 * There is a bug in OPENLOOK Sunview 1 which disables
		 * WIN_EXTEND when the tool is started iconically.  Reset it.
		 */
		xv_set(Tt_panel,  XV_WIDTH, WIN_EXTEND_TO_EDGE, 0);
	}
	if (event_action(evnt) == WIN_RESIZE)
	{
		resize_write_plist();
		place_drop_site();
	}
	
	return(value);
}

static Notify_value
panel_event_proc( panel, event, arg, type )
     Panel              panel;
     Event             *event;
     Notify_arg         arg;
     Notify_event_type  type;
{
	switch( event_action( event ) ) {
  	  case ACTION_PROPS:
	    if ( event_is_down( event ) )
	      make_props_frame();
	    break;
	  case ACTION_STOP:
	    if ( event_is_down( event ) )
  	      stop( Tt_frame, NULL, NULL );
	    break;
	  default:
	    break;
	}
	
	return( notify_next_event_func( panel, ( Notify_event )event, arg, type ) );

}


static Notify_value
props_event_proc(frame, evnt, arg, type)
	Frame           frame;
	Event           *evnt;
	Notify_arg      arg;
	Notify_event_type type;
{
	Notify_value value;

	/* let frame operate on the event */
	value = notify_next_event_func(frame, (Notify_event) evnt, arg, type);

	if (event_action(evnt) == WIN_RESIZE)
	   ds_center_items(Prop_panel, -1, apply_bt, reset_bt, 0);
	
	return(value);
}


static Notify_value
list_frame_event_proc(frame, evnt, arg, type)
	Frame           frame;
	Event           *evnt;
	Notify_arg      arg;
	Notify_event_type type;
{
	Notify_value value;

	/* let frame operate on the event */
	value = notify_next_event_func(frame, (Notify_event) evnt, arg, type);

	if (event_action(evnt) == WIN_RESIZE)
		resize_read_plist();
	
	return(value);
}

static void
resize_write_plist()
{
	int rows, wid;

        xv_set( Tt_panel, XV_WIDTH, xv_get( Tt_frame, XV_WIDTH ), 
	                  XV_HEIGHT, xv_get( Tt_frame, XV_HEIGHT ), 
	                  NULL );
	rows  = ((int)xv_get(Tt_panel, XV_HEIGHT) - 
		 (int)xv_get(Tt_q, XV_Y) - FOOTER_HGT) /
		 (int)xv_get(Tt_q, PANEL_LIST_ROW_HEIGHT);
	if (rows <= 0) rows = 1;

	wid = (int)xv_get(Tt_panel, XV_WIDTH) - 
	       ((int)xv_get(Tt_q, XV_X) * 2) - SB_WID;
	if (wid <= 0) wid = 1;

	xv_set(Tt_q,
	       PANEL_LIST_DISPLAY_ROWS,        rows,
	       PANEL_LIST_WIDTH,               wid,
	       NULL);
}
static void
resize_read_plist()
{
	int rows, wid;
	
	rows  = ((int)xv_get(Tt_list_panel, XV_HEIGHT) - 
		 (int)xv_get(Tt_list, XV_Y) - 
		 (int)xv_get(Tt_list, XV_X) - BOTTOM_HGT) /
		 (int)xv_get(Tt_list, PANEL_LIST_ROW_HEIGHT);
	if (rows <= 0) rows = 1;

	wid = (int)xv_get(Tt_list_panel, XV_WIDTH) - 
	      ((int)xv_get(Tt_list, XV_X) * 2) - SB_WID;
	if (wid <= 0) wid = 1;

	xv_set(Tt_list,
	       PANEL_LIST_DISPLAY_ROWS,        rows,
	       PANEL_LIST_WIDTH,               wid,
	       NULL);
}

#ifdef CPIO
static int
read_with_cpio()
{
	frame_err( MGET("CPIO read has not been implimented yet...") );
	return(0);
}


static int
write_with_cpio()
{
	frame_err( MGET("CPIO write has not been implimented yet...") );
	return(0);
}
#endif


static void
clear_list()	/* clears the list, including client data if any */
{

	if (Tt_list)
		list_flush(Tt_list);
}


static void
clear_q()	/* clears the list, including client data if any */
	{

	/* disable the write button */
	xv_set(Write_button, PANEL_INACTIVE, TRUE, 0);

	/* change frame icon to show nothing in queue */
	change_icon_emptyq();

	if (Tt_q)
		list_flush(Tt_q);
}


#ifdef CPIO
static void
display_error()
{
	char buf[MAXNAMELEN];
	FILE *fp;
	if (fp=fopen(Err_file, "r"))
	{
		make_list_frame();	/* make/display list frame */
		clear_list();
	 
		while (fgets(buf, sizeof(buf), fp))
		{
		   xv_set(Tt_list, PANEL_LIST_STRING, buf, 
			 (int)xv_get(Tt_list, PANEL_LIST_NROWS), NULL);
		}
	 
		fclose(fp);
	}
}
#endif


static void
frame_err(msg)		/* Report any errors in frame footer */
	char *msg;
{
	beep(NUM_BEEPS);
	xv_set(Tt_frame, FRAME_LEFT_FOOTER, msg, 0);
}


static void
frame_msg(msg)		/* Display message in frame footer */
        char *msg;
{
       xv_set(Tt_frame, FRAME_LEFT_FOOTER, msg, 0);
}

static void
host_status()	/* Display host status in right frame footer */
{
	if (strcmp(Host_name, Local_host) == 0 )
            xv_set(Tt_frame, FRAME_RIGHT_FOOTER,  MGET("Local Host") , 0);
	else
            xv_set(Tt_frame, FRAME_RIGHT_FOOTER,  MGET("Remote Host") , 0);
}


static void
beep(n)			/* ring bell n times */
	int n;
{
	int i;

	for (i=0; i < n; i++)
		 window_bell(Tt_frame);
}


static void
err_alert(msg)		/* display an err msg in an alert */
	char *msg;
{
	notice_prompt(Tt_frame, (Event *) NULL,
		NOTICE_MESSAGE_STRINGS,	msg, 
					0,
		NOTICE_BUTTON,		 MGET("OK") ,	101,
		0);
}


/*ARGSUSED*/
static Notify_value
stop(client, sig, when)                           /* ARGS IGNORED */

Notify_client		client;
int			sig;
Notify_signal_mode	when;

{
	char buf[100];
	int nitems = 0;

        if (Pid <= 0)
		return NOTIFY_DONE;

	if ((kill(Pid, SIGTERM) == -1) && (errno != 3))
	{
		perror( "Tapetool: stop(): Unable to stop" );
		sprintf(buf, "Unable to stop: %s" , sys_errlist[errno]);
		err_alert(buf);
	}

	switch (Op)
	{
	case READ:
		sprintf(buf, MGET("%d Item(s) Read\n") , Nprocessed);
		frame_msg(buf);
		/* check any selected file doesn't on tape */
		if (read_op != READ_ENTIRE_TAPE) 
		   if (xv_get(Tt_list, PANEL_LIST_NROWS)) {
		      char msg[128];
		      if (read_op == READ_SELECTED) {
			 if (list_num_selected(Tt_list))
			    err_alert(MGET("Selected files in the list are not on the tape"));
		      }
		      else { /* read entire list */
		         err_alert(MGET("Files in the list are not on the tape"));
		      }  /* end else */
		   }  /* end if xv_get */
		break;
	case WRITE:
		/* if everything got written to tape */
		if (Nprocessed)
		{
			/* clear both queue and list */
			clear_q();
			clear_list();
			sprintf(buf,
			 MGET("%d Total Item(s) Written\n") , Nprocessed);
		}
		else
			sprintf(buf, MGET("Write Terminated") );
		frame_msg(buf);
		break;
	case LIST:
		/*
		 * If an error occurred, Tt_list might never have
		 * been created, so check for it.
		 */
		if (Tt_list) {
			if (xv_get(Tt_list, PANEL_INACTIVE))
			   xv_set(Tt_list, PANEL_INACTIVE, FALSE, 0);
			nitems = xv_get(Tt_list, PANEL_LIST_NROWS);
                }

		if (nitems)
		{
		    sprintf(buf, MGET("%d Item(s) Found\n") , nitems);
		    frame_msg(buf);
		}
		else
		    frame_msg( MGET("No Files Found") );

		break;
        default:
	  break;
		
	}

	Pid = 0;
	Op = NO_OP;
	(void)close(Master);
	notify_set_input_func((Notify_client) &Pid, NOTIFY_FUNC_NULL, Master);
        tt_busy(FALSE);
}
 

static int
get_master()
{
        char *pty, *bank, *cp;
        struct stat stb;
        int     i;
        
#ifdef SVR4

    if ((i = open("/dev/ptmx", O_RDWR)) < 0)
        return -1;
    if (grantpt(i) == -1)
        perror("grantpt");
    if (unlockpt(i) == -1)
        perror("unlockpt");

    return( i );

#else
        pty = &Line[LENDEVPTY];
        for (bank =  "pqrs" ; *bank; bank++) 
	{
                Line[LENDEVPTY-1] = *bank;
                *pty = '0';
                if (stat(Line, &stb) < 0)
                        break;
                for (cp =  "0123456789abcdef" ; *cp; cp++) 
		{
                        *pty = *cp;
                        i = open(Line, O_RDWR);
                        if (i >= 0) 
			{
                                char *tp = &Line[LENDEV];
                                int ok;
 
                                /* verify Slave side is usable */
                                *tp = 't';
                                ok = access(Line, R_OK|W_OK) == 0;
                                *tp = 'p';
                                if (ok)
                                        return(i);
                                (void) close(i);
                        }
                }
        }
        return(-1);
#endif
}
 
#ifdef SVR4
static  struct  sgttyb  b = {13, 13, CTRL('?'), CTRL('U'), 06310};
#else
static  struct  sgttyb  b = {13, 13, CTRL(?), CTRL(U), 06310};
#endif SVR4
 
static int
get_slave()
{
        int     i;
        
#ifdef SVR4
    extern char *ptsname();

    if ((i = open(ptsname(Master),O_RDWR))<0)
        return -1;
    if (ioctl(i, I_PUSH, "ptem") == -1)
        perror("push ptem");
    if (ioctl(i, I_PUSH, "ldterm") == -1)
        perror("push ldterm");

#else
        Line[LENDEV] = 't';
        i = open(Line, O_RDWR);
        if (i < 0)
                return(-1);
 
        (void) ioctl(i, TIOCSETP, (char *)&b);
#endif

        return(i);
}

#ifdef CPIO
static int
list_with_cpio()
{
        char buf[MAXNAMELEN];
        FILE *fp;
 
        sprintf(buf, List_tape[Method], List_opts, Err_file);
 
        if (fp = popen(buf, "r"))
        {
		make_list_frame();      /* make/display list frame */
                clear_list();
 
                while (fgets(buf, sizeof(buf), fp))
                        list_add_entry(Tt_list, buf, (Server_image)NULL,  1,
				(int)xv_get(Tt_list, PANEL_LIST_NROWS), TRUE);

                if (xv_get(Tt_list, PANEL_LIST_NROWS))
                {
                        frame_msg( MGET("Finished Listing Files On Tape") );
			pclose(fp);
			return(1);
		}
        }

	if (fp) pclose(fp);
	frame_msg( MGET("Unable To List Contents Of Tape") );
	display_error();
	return(0);
}
 
#endif

static int
process_list_output(buf)	/* process tar list output from psuedo tty */	
	char *buf;
{
	register char *filename, *p;
	char	msg[32];
	int 	pos;

	/* do not list directories, s5 has ^M at the end */
	if (buf[strlen(buf)-2] == '/')	
		return(1);
        
	if (strncmp(buf, "tar:", 4) == 0) {
		err_alert(buf+4);
		return(0);
	}

        if ( strstr( buf, " unknown host" ) != NULL ) {
		err_alert(buf);
		return(0);
	}

	if (strstr(buf, "Permission denied") != NULL) {
		sprintf(msg, MGET("Access to %s: %s") , Host_name, buf);
		err_alert(msg);
		return(0);
	}

	if (strstr(buf, "Login incorrect") != NULL) {
		sprintf(msg, MGET("Access to %s: %s") , Host_name, buf);
		err_alert(msg);
		return(0);
	}

	p = filename = buf;

	/* substitue space at end of filename with null */
	while (*p != ' ' && *p != '\0') {
		p++;
	        if ( *p == '\015' )
		  *p = '\0';
	}
	*p = NULL;

	make_list_frame(); 	

	if (!xv_get(Tt_list, PANEL_INACTIVE))
	   xv_set(Tt_list, PANEL_INACTIVE, TRUE, 0);
	pos = xv_get(Tt_list, PANEL_LIST_NROWS);
	xv_set(Tt_list, 
		PANEL_LIST_INSERT, pos,
		PANEL_LIST_STRING, pos, filename, 
		PANEL_LIST_CLIENT_DATA, pos, 1,
		NULL);
 
        Nprocessed++;
 
        if (!(Nprocessed%8))
        {
                sprintf(msg, MGET("%d Item(s) Found") , Nprocessed);
                frame_msg(msg);
        }
	return(1);
}


static int
process_read_output(buf)	/* process tar read output from psuedo tty */	
	char *buf;
{
	register char 	*filename, *p;
	char 		msg[128];
        static int 	already_warned;

	/* 
	 * tar's extract output takes the form  "x <filename...>"  for regular
	 * files, and just  "<filename...>"  for symbolic links.  Assume
	 * tar will let us know if error occurred.
	 */

	if (strncmp(buf, "tar:" ,4) == 0) {
#ifdef SVR4
	        if ( ( strlen( buf ) >= ( size_t )17 ) && 
                     ( strncmp( buf, "tar: blocksize = ", 17 ) == 0 ) ) {
		  return(1);
	        }
                else {
  	 	  err_alert(buf+4);
		  return(0);
                }
#else
		err_alert(buf+4);
		return(0);
#endif
	}
	if (strncmp(buf,  "dd:" , 3) == 0) {
		err_alert(buf+3);
		return(0);
	}
        if ( strstr( buf, " unknown host" ) != NULL ) {
		err_alert(buf);
		return(0);
	}
	if (strstr(buf, "Permission denied") != NULL) {
		sprintf(msg, MGET("Access to %s: %s") , Host_name, buf);
		err_alert(msg);
		return(0);
	}
	if (strstr(buf, "Login incorrect") != NULL) {
		sprintf(msg, MGET("Access to %s: %s") , Host_name, buf);
		err_alert(msg);
		return(0);
	}
        if ( strstr( buf, " records " ) != NULL ) 
		return(1);

	/* skip over  "x "  if there */
	if (*buf == 'x')
			buf += 2;
	p = filename = buf;

		/* substitue space or comma at end of filename with null */
	while (*p && (*p != ' ' && *p != ','))
			p++;
	*p = NULL;

        if (read_op == READ_ENTIRE_TAPE) {
	   /* don't count the directory name */
	   if ( *(--p) == '/')	
	      return(1);

           Nprocessed++;
           if (!already_warned && buf[0] != '/' && Dest_name[0] == '\0') {
              frame_err( MGET("Some Files Restored To The Current Working Directory") );
              already_warned = TRUE;
           }
           return 1;
        }
	if (!list_delete_entry(Tt_list, filename)) {
	   /* show percentage read */
	   sprintf(msg, MGET("%3.0f%% read") ,
		        ((float)++Nprocessed/(float)Nfiles)*100);
	   frame_msg(msg);
	   return(1);
	}
	else {
	   fprintf(stderr,
	   MGET("unknown line returned from tar:\n'%s'\n"), buf);
	   return(1);
	}
}


static int
process_write_output(buf)	/* process tar write output from psuedo tty */	
	char *buf;
{
	register char *p;

	if (*buf == 'a')
	{	
		/* skip over  "a " , assume start at position 2 */
		p = buf+2;

		/* substitue space at end of filename with null */
		while (*p != ' ')
			p++;
		*p = NULL;

		/* don't count directory name */
	        if ( *(--p) != '/')	
		   Nprocessed++;		/* for stats later */
		return(1);

	}
	else if (strncmp (buf, "seek = ", 7) == 0)
	{	
                buf = strstr( buf, "a " );
		
		/* skip over  "a " , assume start at position 2 */
		p = buf+2;

		/* substitue space at end of filename with null */
		while (*p != ' ')
			p++;
		*p = NULL;

		/* don't count directory name */
	        if ( *(--p) != '/')	
		   Nprocessed++;		/* for stats later */
		return(1);

	}
	else if (strncmp(buf, "tar:" ,4) == 0)
		err_alert(buf+4);
	else if (strncmp(buf,  "dd:" , 3) == 0) {
		char msg[128];
#ifdef SVR4
	        if ( ( strlen( buf ) >= ( size_t )27 ) &&
	             ( strncmp( buf, "dd: write: Invalid", 18 ) == 0 ) )
	          return( 1 );

                if (strncmp(buf, "dd: creat", 9) == 0) {
		   sprintf(msg, "%s: %s", Host_name, buf+4);
		   err_alert(msg);
	           Nprocessed = 0;	/* indicates an error occurred */
		   return 0;
		}
#endif
		sprintf(msg, "%s: %s", Host_name, buf+4);
		err_alert(msg);
        }
        else if ( strstr( buf, " unknown host" ) != NULL ){
		err_alert(buf);
	        Nprocessed = 0;		/* indicates an error occurred */
		return 0;
	}
	/* dd standard output from remote tars */
        else if ( strstr( buf, " records " ) != NULL )
		return(1);
        else if (strstr (buf, "seek = OK") == NULL)
	        return (1);
	else {
		char msg[128];
		sprintf(msg, MGET("Access to %s: %s") , Host_name, buf);
		err_alert(msg);
	        Nprocessed = 0;		/* indicates an error occurred */
		return 0;
	}	
	if (Err_exit) {
	   Nprocessed = 0;		/* indicates an error occurred */
	   return 0;
	}
	return 1;
}


/*ARGSUSED*/
static Notify_value	/* process psuedo tty output */
process_pty_output(client, fd)

Notify_client	client;
int		fd;

{
        char buf[MAXNAMELEN];
        
	/*
	 * Note: a problem can occur if you do not read tar's output
	 * fast enough.  Tar's output buffer fills up, and then tar
	 * stops dead with no error msg.  Be careful when adding
	 * code at this point, or in above  "process_*_ouput"  functions.
	 */

        if (fgets(buf, sizeof(buf), Pty_fp))
        {
		if (buf[strlen(buf)-1] == '\n')		/* Strip newline */
			buf[strlen(buf)-1] = NULL;
		
		switch (Op)
		{
		case READ:
			if (!process_read_output(buf))
				stop( client, NULL, NULL );		/* error detected so stop */
			break;
		case WRITE:
			if (!process_write_output(buf))
				stop( client, NULL, NULL );		/* error detected so stop */
			break;
		case LIST:
			if (!process_list_output(buf))
				stop( client, NULL, NULL);		/* error detected so stop */
			break;
		default:
			frame_err( MGET("Unknown operation in process_pty_output()") );
		}
        }
        else{
                stop( client, NULL, NULL);
	}

        return(NOTIFY_DONE);
}


static int 
setup_pty(cmd)
	char *cmd;
{
        int pid;
        char *av[512], buf[128];
#ifdef SVR4
	pid_t status;
#else
        union wait status;
#endif SVR4
 
        /* Fork off the command ... */
 
        if ((Master = get_master()) < 0) {
            	frame_err( MGET("No more pty's - listing aborted") );
                return(0);
        }

#ifdef SVR4
	setsid();
	status = (pid_t)-1;
	while (waitpid(status, (int *)0, WNOHANG) > 0 )
	  continue;
#else
        /* Don't block if no process has status to report */
        while (wait3(&status, WNOHANG, (struct rusage *)0) > 0)
                continue;
#endif SVR4

        if ((pid = fork()) < 0) {
		sprintf(buf,  MGET("Unable to fork: %s") , sys_errlist[errno]);
                frame_err(buf);
                return(0);
        }

        if (pid == 0)		/* child */ 
	{         
		register int i;

		/* build command parms */
		i = 0;
		av[i++] =  "/bin/sh" ;
	        av[i++] =  "-c" ;
		av[i++] = cmd;
		av[i++] = NULL;
/*
		av[i] = (char *)strtok(cmd, " ");
		while (av[i++]) {
			av[i] = (char *)strtok(NULL, " ");
		}
*/
#ifdef DEBUG
		fprintf( stderr, "%s\n", av[0] );
		fprintf( stderr, "%s\n", av[1] );
		fprintf( stderr, "%s\n", av[2] );
#endif

                i = open( "/dev/tty" , O_RDWR);
                if (i >= 0) {
                        (void) ioctl(i, TIOCNOTTY, (char *)0);
                        (void) close(i);
                }
                Slave = get_slave();
                (void) close(Master);
                (void) dup2(Slave, 0);
                (void) dup2(Slave, 1);

/* DO NOT INTERCEPT FOR NOW - problem evaluating output */
/* commented back in temporarily, needed stderr for remote tars */
		(void) dup2(Slave, 2);

#ifdef SVR4
		for (i = sysconf(_SC_OPEN_MAX) - 1; i >= 3; i--)
#else
                for (i = getdtablesize() - 1; i >= 3; i--)
#endif SVR4
                        (void)close(i);

                if (execve(av[0], av, (char **)0) < 0)
		{
			perror( "Tapetool: unable to execute in setup_pty()" );
			exit(1);
		}
                fprintf(stderr, MGET("Tapetool: child failed in setup_pty()\n") );
                exit(127);
	        
        }

        if ((Pty_fp = fdopen(Master,  "r" )) == NULL)	/* parent */
	{
		frame_err( MGET("Aborting: Cannot open pty for requested operation") );
                return(0);
	}
        else
        {
                Pid = pid;
		notify_set_input_func((Notify_client) &Pid, (Notify_func) process_pty_output, Master);
        }
        return(1);
}
 
static int
check_device( op )
    op_type  op;
{
    char *device;
    int   flag;
    int   fd;

/*
 * Check local device ahead only for local tars.
 */
    if (strcmp(Host_name, Local_host) != 0 ) 
      return(1);

    switch( op ) {
    case LIST:  
    case READ:
      flag = O_RDONLY | O_NONBLOCK;
      break;
    case WRITE:
      flag = O_RDWR | O_NONBLOCK | O_CREAT;
      break;
    default:
      flag = O_RDONLY | O_NONBLOCK;
      break;
    }

    fd = open( Device_name, flag, 0644 );
    if ( fd < 0  ) {
      char err_msg[BUFSIZ];
      if ( op == WRITE && errno == EISDIR )
        err_alert( MGET( "The tar file you specified is a directory name.\nThis is not a valid tar file." ) );
      else if ( errno == EACCES ) {
	sprintf(err_msg, "%s %s%s", MGET("You do not have permission to write to the"), 
	       Device_name, MGET(".\nPlease to be sure that it is writable."));
	err_alert(err_msg);
      }
      else if ( errno == EIO )
        err_alert( MGET( "Cannot access tape drive.\nPlease be sure the tape drive is turned on and\n the tape is inserted in the tape drive." ) );
      else {
	 sprintf(err_msg, "%s %s\n%s", MGET("No such device or file"), 
	    Device_name, MGET("Please be sure tape drive is connected or file does exist."));
	 err_alert(err_msg);
      }

      return( 0 );
    }

    close( fd );
    return( 1 );

}

static int
list_button()
{
	char buf[MAXNAMELEN];

	if (Op != NO_OP) {
		err_alert( MGET("Please wait for the current operation to finish") );
		return(0);
	}

        if ( !check_device( LIST ) )
          return( 0 );

	Nprocessed = 0;
	buf[0] = NULL;

#ifndef CPIO
        /* Add this for remote tars */
	if (strcmp(Host_name, Local_host) != 0 ) {
	    strcat(buf, Rsh_cmd );
	    strcat(buf,  " -n " );
	    strcat(buf, Host_name);
	    strcat(buf, " ");
            strcat(buf, "'/bin/sh -c ");
            strcat( buf, "\"PATH=/usr/sbin:/usr/bin; " );
	}
        else
	    strcat(buf, "PATH=/usr/sbin:/usr/bin; " );

	sprintf(buf, List_tape[Method], buf, List_opts);

	if (strcmp(Host_name, Local_host) != 0 ) 
          strcat(buf, "\"'");

#endif
	frame_msg( MGET("Listing Files on Tape...") );
        tt_busy(TRUE);

#ifdef CPIO
	switch (Method)
	{
	case TAR:  
#endif
	
		clear_list();
		
		if (setup_pty(buf)) {
			Op = LIST;
			return(1);
		}
		else {
			Op = NO_OP;
                        tt_busy(FALSE);
			return(0);
		}

#ifdef CPIO
	case CPIO: return(list_with_cpio());
	default:
		fprintf(stderr,  MGET("Unknown method in list_button()\n") );
                tt_busy(FALSE);
		return(0);
	}
#endif
}


static char *
setup_tar_read_cmd()
{
	register int n;
	register char *selected_fname;
	static char *buf;
        int status;
        int length;
 	int list_num_selected();
        char errmsg[80];

	/*
	 * Tar does not allow file names to be input from standard
	 * input.  This means we have to allocate enough space
	 * to contain the tar command and all filenames.
	 */
        if (read_op == READ_ENTIRE_TAPE) 
		Nfiles = 2;
	else if (read_op == READ_SELECTED) {
		Nfiles = list_num_selected(Tt_list); 
		if (Nfiles == 0) {
			frame_err( MGET("Select a File and Choose Read Selected Again") );
			return NULL;
		}
	}
	else {
		/* read entire list */
		Nfiles = xv_get(Tt_list, PANEL_LIST_NROWS);
		if (Nfiles == 0) {
			frame_err( MGET("No Files are Listed to Read") );
			return NULL;
		}
	}

        if ((buf = (char *)MALLOC(BUFSIZ)) == NULL) 
	{
		frame_err( MGET("No Memory: Unable To Read Files From Tape") );
	    	perror( "Tapetool: unable to get memory in setup_tar_read_cmd()" ); 
		return(NULL);
	}

	/* setup tar read command */

        /* Add this for remote tars.
	   Use the default block size if one is not specified */
	buf[0] = '\0';
	if (strcmp(Host_name, Local_host) != 0 ) {
	    buf[0] = NULL;
	    strcat(buf, Rsh_cmd );
	    strcat(buf,  " -n " );
	    strcat(buf, Host_name );
	    strcat(buf," dd if=" );
	    strcat(buf, Device_name );
	    strcat(buf,  " bs=" );
	    if (xv_get(tar_write_item, PANEL_TOGGLE_VALUE, 2)) {
	       strcat(buf, (char *) xv_get(tar_block_item, PANEL_VALUE));
	       strcat(buf,  "b" );
	    }
	    else {
	       strcat(buf, DEFAULT_BLOCK_SIZE);
	       strcat(buf,  "b" );
	    }
            strcat(buf, " | /bin/sh -c \"PATH=/usr/sbin:/usr/bin; " );
	}
        else
          strcat( buf, "PATH=/usr/sbin:/usr/bin; " );

	sprintf(buf, Read_tape[Method], buf, Read_opts);

        if (read_op == READ_SELECTED) {
          if ( !tempname )
            tempname = ( char * )mktemp( "/tmp/tapetool.XXXXXX" );
          if ( ( tmpfp = fopen( tempname, "w" ) ) == 0 ) {
            sprintf( errmsg, MGET( "Unable to open the file %s\n" ), tempname );
            frame_err( errmsg );
            return( NULL );
          }
	  for (n=0; n < xv_get(Tt_list, PANEL_LIST_NROWS); n++) {
	    if (list_item_selected(Tt_list, n)) {
	      selected_fname = (char*)list_get_entry(Tt_list, n); 
	      length = strlen( selected_fname );
	      if ( fwrite( selected_fname, 1, length, tmpfp ) != length ) {
		frame_err( MGET( "Unable to make tar include file." ) );
		unlink( tempname );
		return( NULL );
	      }
	      if ( fwrite( "\n", 1, 1, tmpfp ) != 1 ) {
		frame_err( MGET( "Unable to make tar include file." ) );
		unlink( tempname );
		return( NULL );
	      }
	      if ((Dest_name[0] == NULL) && (selected_fname[0] != '/')) {
		frame_err( MGET("You need to set a destination directory") );
		return(NULL);
	      }
	    }
	  }
	  fclose( tmpfp );
          strcat( buf, " -I " );
          strcat( buf, tempname );
	}
        else if (read_op == READ_ENTIRE_LIST)
        {
          if ( !tempname )
            tempname = ( char * )mktemp( "/tmp/tapetool.XXXXXX" );
          if ( ( tmpfp = fopen( tempname, "w" ) ) == 0 ) {
            sprintf( errmsg, MGET( "Unable to open the file %s" ), tempname );
            frame_err( errmsg );
            return( NULL );
          }
          for (n=0; n < Nfiles; n++) {
	    selected_fname = (char*)list_get_entry(Tt_list, n);
	    length = strlen( selected_fname );
	    if ( fwrite( selected_fname, 1, length, tmpfp ) != length ) {
	      frame_err( MGET( "Unable to make tar include file." ) );
	      unlink( tempname );
	      return( NULL );
	    }
	    if ( fwrite( "\n", 1, 1, tmpfp ) != 1 ) {
	      frame_err( MGET( "Unable to make tar include file." ) );
	      unlink( tempname );
	      return( NULL );
	    }
	    if ((Dest_name[0] == NULL) && (selected_fname[0] != '/')) {
	      frame_err( MGET("You need to set a destination directory") );                
	      return(NULL);
	    }
          }
	  fclose( tmpfp );
          strcat( buf, " -I " );
          strcat( buf, tempname );
        } 
        else
        {  
         if (Dest_name[0] == NULL)
                {
                   status = notice_prompt(Tt_frame,
                            (Event *)NULL,
                            NOTICE_MESSAGE_STRINGS,
                             MGET("The Destination Directory Has\nNot Been Specified. Files Will\nBe Restored To The Current Working\nDirectory. Do You Still Want To\nRead The Tape?") ,
                            0,
                            NOTICE_BUTTON_YES,  MGET("Yes") ,
                            NOTICE_BUTTON_NO,  MGET("No") ,
                            0);
                    if (status == NOTICE_NO)
                       return NULL;
                }
        }

        if ( strcmp( Host_name, Local_host ) != 0 )
	  strcat( buf, "\"" );

#ifdef NEVER
/* FIX LATER - cannot use shell redirection with execcve command  */
	strcat(buf,  " 2> " );
	strcat(buf, Err_file);
#endif

	return(buf);
}


static int
read_button()
{
	static char *cmd;
        static int numfiles;

	if (Op != NO_OP) {
		err_alert( MGET("Please wait for the current operation to finish") );
		return(0);
	}

        if ( !check_device( READ ) )
          return( 0 );

        if (!check_perms())
           return 0;

	Nprocessed = 0;
        switch (read_op)
        {
          case READ_SELECTED:
             numfiles = xv_get(Tt_list, PANEL_LIST_NROWS);
             if (numfiles == 0) {
                frame_err( MGET("No Files Selected") );
                return(0);
             }
             break;
          case READ_ENTIRE_LIST:
             numfiles = xv_get(Tt_list, PANEL_LIST_NROWS);
             if (numfiles == 0) {
                frame_err( MGET("No Files in List") );
                return(0);
             }
             break;
          default: /* entire tape */
             break;
        }

        tt_busy(TRUE);

#if 0
	switch (Method)
	{
	case TAR:  
#endif
		/* change directory to where files will be restored */
		if (chdir(Dest_name))
		{
			frame_err( MGET("Unable To Set Destination Directory") );
			perror( "Tapetool: read_button()" );
                        tt_busy(FALSE);
			return(0);
		}	

		if ((cmd = setup_tar_read_cmd()) == NULL)
                {
                        tt_busy(FALSE);
			unlink( tempname );
			return(0);
                }

#ifdef SVR4
		if (setup_pty(cmd)) {
		        FREE(cmd);
#else
		if (setup_pty(cmd) && FREE(cmd)) {
#endif SVR4
			Op = READ;
			return(1);
		}

		perror( "Tapetool: read_button()" );
		Op = NO_OP;
                tt_busy(FALSE);
		return(0);
#if 0
	case CPIO: return(read_with_cpio());
	default:
		fprintf(stderr,  MGET("Unknown method in read_button()\n") );
                tt_busy(FALSE);
		return(0);
	}
#endif
}


static char * 
setup_tar_write_cmd()
{
	static char *buf;
	int n, count, last, length;
	char *entry, *c_data;
	char  errmsg[80];
	int no_strip_path;

	/*
	 * Tar does not allow file names to be input from standard
	 * input.  This means we have to allocate enough space
	 * to contain the tar command and all filenames.
	 */

        if ((buf = (char *)MALLOC(BUFSIZ)) == NULL) 
	{
		frame_err( MGET("No Memory: Unable To Write Files To Tape") );
	    	perror( "Tapetool: unable to get memory in setup_tar_write_cmd()" ); 
		return(NULL);
	}

	/*
	 * We need to maintain relative pathnames
	 * TO THE DIRECTORY ITEM!  This requires us to change
	 * directories to the nearest common filepath.
	 */

	sprintf(buf, Write_tape[Method], Write_opts);

	/* For strip path , it has to append the files together since
	  each -C has to have a filename after it.  Therefore, tmp
	  file to store all filenames won't work for strip path . */

	if (tar_deletedir_item) 
	   no_strip_path = xv_get(tar_deletedir_item, PANEL_TOGGLE_VALUE, 0);
	else
	   no_strip_path = 1;
	if (no_strip_path) {
	   if ( !tempname )
	     tempname = ( char * )mktemp( "/tmp/tapetool.XXXXXX" );
	   if ( ( tmpfp = fopen( tempname, "w" ) ) == 0 ) {
	     sprintf(errmsg, MGET( "Unable to open the file %s\n"), tempname);
	     frame_err( errmsg );
	     FREE(buf);
	     return( NULL );
	   }
	}

	for (n=0; n < Nfiles; n++) {
	  entry = list_get_entry(Tt_q, n); 
	  
	  c_data = (char*)list_client_data(Tt_q, n);
	  if (c_data != NULL && c_data[0] != '\0') {
	     /* tells tar to change directory */
	     /* allocate enough memory for directory name and filename */
	     if ((buf = (char*)REALLOC(buf, strlen(buf) + strlen(entry) + strlen(c_data) + 20)) == NULL) {
		frame_err( MGET("No Memory: Unable To Write Files To Tape") );
	    	perror("Tapetool: unable to get memory in setup_tar_write_cmd()"); 
		FREE(buf);
		return(NULL);
	     } /* end if buf ... */
	     strcat(buf,  " -C " );
	     strcat(buf, c_data );
	  } /* end if c_data ... */
	  else   /* alloc memory for strip path option has set */
	     if (!no_strip_path) {
	         if ((buf = (char*)REALLOC(buf, strlen(buf) + strlen(entry) + 20)) == NULL) {
		    frame_err(MGET("No Memory: Unable To Write Files To Tape"));
	    	    perror("Tapetool: unable to get memory in setup_tar_write_cmd()"); 
		    FREE(buf);
		    return(NULL);
		 }
	     }

	  strcat(buf, " ");
	  
	  /*EMPTY*/
	  for (last = strlen(buf), count = 0;
	       entry[count] != '\0' &&
	       entry[count] != ' ';
	       count++);

	  if (no_strip_path) {
	     if (entry[count] == ' ')
	       entry[count] = '\0';
	     length = strlen( entry );
	     if ( fwrite( entry, 1, length, tmpfp ) != length ) {
	       frame_err( MGET( "Unable to make tar include file." ) );
	       unlink( tempname );
	       return( NULL );
	     }
	     if ( fwrite( "\n", 1, 1, tmpfp ) != 1 ) {
	       frame_err( MGET( "Unable to make tar include file." ) );
	       unlink( tempname );
	       return( NULL );
	     }
	  }
	  else {
	     if (entry[count] == ' ') {
	        strncat(buf, entry, count);
	        buf [count+last] = '\0';
	     }
	     else 
		strcat(buf, entry);
	  }
	}
	if (no_strip_path) {
	   fclose( tmpfp );
	   strcat( buf, " -I " );
	   strcat( buf, tempname );
	}

	/* For remote tars, add rsh command.
	   Use the default block size if one is not specified. */
	if (strcmp(Host_name, Local_host) != 0 ) {
	   if (!no_strip_path) {
	      if ((buf = (char*)REALLOC(buf, strlen(buf) + 100)) == NULL) {
		  frame_err(MGET("No Memory: Unable To Write Files To Tape"));
	    	  perror("Tapetool: unable to get memory in setup_tar_write_cmd()"); 
		  FREE(buf);
		  return(NULL);
	      }
           } /* end if !no_strip_path */
	   strcat(buf, " | /bin/sh -c 'PATH=/usr/bin:/usr/ucb; rsh " ); 
	   strcat(buf, Host_name);
	   strcat(buf,  " dd of=" );
	   strcat(buf, Device_name);
	   strcat(buf,  " obs=" );
	   if (xv_get(tar_write_item, PANEL_TOGGLE_VALUE, 2)) {
	      strcat(buf, (char *) xv_get(tar_block_item, PANEL_VALUE));
	      strcat(buf,  "b'" );
	   }
	   else {
	      strcat(buf, DEFAULT_BLOCK_SIZE);
	      strcat(buf,  "b'" );
	   }
	}
	return(buf);
}


static int
write_button()
{
	static char *cmd;

	if (Op != NO_OP) {
		err_alert( MGET("Please wait for the current operation to finish") );
		return(0);
	}

        if ( !check_device( WRITE ) )
          return( 0 );

	Nprocessed = 0;
	Nfiles = xv_get(Tt_q, PANEL_LIST_NROWS);
	if (Nfiles == 0) {
		frame_err( MGET("No Files To Write") );
		return(0);
	}

        tt_busy(TRUE);

#ifdef CPIO
	switch (Method)
	{
	case TAR:  
#endif

		if ((cmd = setup_tar_write_cmd()) == NULL)
                {
                        tt_busy(FALSE);
			unlink( tempname );
			return(0);
                }

#ifdef SVR4
		if (setup_pty(cmd)) {
		        FREE(cmd);
#else
		if (setup_pty(cmd) && FREE(cmd)) {
#endif SVR4
			Op = WRITE;
			return(1);
		}

		perror( "Tapetool: write_button()" );
		Op = NO_OP;
                tt_busy(FALSE);
		return(0);

#ifdef CPIO
	case CPIO: return(write_with_cpio());
	default:
		fprintf(stderr,  MGET("Unknown method in write_button()\n") );
                tt_busy(FALSE);
		return(0);
	}
#endif

}
static void read_selected_proc()
{
     if (!make_list_frame())
        frame_err( MGET("'List...' or Add Files to Read") );
     else
     {   
        read_op = READ_SELECTED;
        read_button();
     }
}
static void read_entire_tape_proc()
{
     read_op = READ_ENTIRE_TAPE;
     read_button();
}
static void read_entire_list_proc()
{
     if (!make_list_frame())
        frame_err( MGET("'List...' or Add Files to Read") );
     else
     {   
        read_op = READ_ENTIRE_LIST;
        read_button();
     }
}

static Menu
create_read_pulldown()
{

        Menu            menu;
        Menu_item       tmp_item;

        menu = menu_create(XV_NULL, MENU,
                MENU_PIN, TRUE,
                0);

        tmp_item = menu_create_item(
                MENU_STRING,             MGET("Selected") ,
                MENU_ACTION_PROC,       read_selected_proc,
                XV_HELP_DATA,           "tapetool:READ_ITEM",
                0);
        menu_set(menu,
                MENU_APPEND_ITEM,       tmp_item,
                0);
        tmp_item = menu_create_item(
                MENU_STRING,             MGET("Entire List") ,
                MENU_ACTION_PROC,       read_entire_list_proc,
                XV_HELP_DATA,           "tapetool:READ_ITEM",
                0);
        menu_set(menu,
                MENU_APPEND_ITEM,       tmp_item,
                0);
        tmp_item = menu_create_item(
                MENU_STRING,             MGET("Entire Tape") ,
                MENU_ACTION_PROC,       read_entire_tape_proc,
                XV_HELP_DATA,           "tapetool:READ_ITEM",
                0);
        menu_set(menu,
                MENU_APPEND_ITEM,       tmp_item,
                0);
        return menu;
}

tt_busy(on)

int     on;

{
        window_set(Tt_frame, FRAME_BUSY, on, 0);
        if (Tt_list_frame)
           window_set(Tt_list_frame, FRAME_BUSY, on, 0);
}


static void 
change_icon_fullq()
{
	Tool_icon = (Icon) xv_get (Tt_frame, FRAME_ICON);
	xv_set(Tool_icon, ICON_IMAGE, full_image, 0);
	xv_set(Tt_frame, FRAME_ICON, Tool_icon, 0);
}

static void 
change_icon_emptyq()
{
        Tool_icon = (Icon) xv_get(Tt_frame, FRAME_ICON);
	xv_set(Tool_icon, ICON_IMAGE, tape_image, 0);
	xv_set(Tt_frame, FRAME_ICON, Tool_icon, 0);
}

