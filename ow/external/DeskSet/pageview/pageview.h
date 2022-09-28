/*
 * @(#)pageview.h 3.6 93/08/03
 */

/*
 * Copyright (c) 1990 - Sun Microsystems Inc.
 */

/*
 * pageview.h - pageview global definitions.
 */

#include <locale.h>
#include <sys/param.h>

#ifdef SVR4
#include <netdb.h>
#endif

#include <xview/xview.h>
#include <xview/frame.h>
#include <xview/panel.h>
#include <xview/text.h>
#include <xview/dragdrop.h>
#include <xview/notice.h>
#include <xview/cursor.h>
#include <xview/cms.h>

#include <NeWS/psmacros.h>
#include <DPS/dpsfriends.h>
#include <DPS/dpsXclient.h>

#ifdef XGETTEXT
#define MSGFILE_ERROR		"SUNW_DESKSET_PAGEVIEW_ERR"
#define MSGFILE_LABEL		"SUNW_DESKSET_PAGEVIEW_LABEL"
#define MSGFILE_MESSAGE		"SUNW_DESKSET_PAGEVIEW_MSG"
#else
extern char *MSGFILE_ERROR;
extern char *MSGFILE_LABEL;
extern char *MSGFILE_MESSAGE;
#endif

#define DGET(s)			s
#define EGET(s)			(char *) dgettext (MSGFILE_ERROR, s)
#define LGET(s)			(char *) dgettext (MSGFILE_LABEL, s)
#define MGET(s)			(char *) dgettext (MSGFILE_MESSAGE, s)

extern char *NONAME;

#define XFER_BUFSIZ		65535

#define COUNT_RESPONSE		0x1
#define HOST_RESPONSE		0x2
#define TRANSPORT_RESPONSE	0x4
#define FILE_NAME_RESPONSE	0x8
#define DATA_LABEL_RESPONSE	0x10
#define TYPES_RESPONSE		0x20
#define TARGET_RESPONSE		0x40
#define BYTES_REQUESTED		0x80

#define RESPONSE_LEVEL_1	0x7f

#define POINT_IN_RECT(event, rect) ((event_x(event) > rect->r_left) && \
                                   (event_x(event) < (rect->r_left + rect->r_width)) && \
                                   (event_y(event) > rect->r_top) && \
                                   (event_y(event) < (rect->r_top + rect->r_height)))

#define Min(a,b) 		(a > b ? b : a)
#define Max(a,b)		(a > b ? a : b)

#define	ON			1
#define	OFF			0

typedef struct {
    int         pagesize;
    int         dpi;
    int         orient;
    int		aa;
    int		timeout;
    int		method;
}           PageView;

PageView    pageview;

/*
 * We want some of the features of the features of the
 * Wire Service without the dependence upon the NeWS
 * package being around on the rendering connection.
 * Pageview now uses 2 NeWS connections: 1 for Adobe-style
 * document rendering without NeWS, and another with complete
 * access to NeWS features.
 *
 * Therefore, we need a structure that keeps the 2 PSFILEs
 * that make up a connection to the NeWS server and a couple
 * of utility routines to switch the current cps output connection
 * back and forth between the 2 connections.
 */
typedef struct {
    PSFILE *input;
    PSFILE *output;
} PageViewConn;
extern PageViewConn *NeWSconn;
extern PageViewConn *PSL1conn;
extern PageViewConn *get_current();

int         UI1,
            UI2,
            UI3,
            UI4,	
	    UI5,
	    UI6,
	    UI7,
	    UI8,
 	    UI9,
	    UI10,
	    UI11,
	    UI12,
	    UI13,
	    UI14;

typedef struct {
		char		*data;
		int		 alloc_size;
		int		 bytes_used;
} CHAR_BUF;

typedef struct {
		Atom		*target_list;
		int		 num_targets;
		Atom		*transport_list;
		int		 num_transports;
		Atom		*types_list;
		int		 num_types;
		char		*data_label;
		int		 num_objects;
		char		*source_host;
		char		*source_filename;
		Atom		 chosen_target;
		Atom		 chosen_transport;
		int		 transfer_state;
		int		 processing_stream;
		int		 state_mask;	
		int		 stop_hit;
		CHAR_BUF	*transfer_data;
} DND_CONTEXT;

typedef enum {
	DND_OP,
	TOOLTALK_OP,
	DND_CANVAS_OP,
	DND_ICON_OP
} Selection_Op;

typedef enum {
	NONE,
	PS_ERROR,
	TIMEOUT,
	NO_CONVENTIONS
} Notice_Types;
	
#define MAX_LINE_LENGTH 512

extern Frame baseframe;
extern Frame init_file();
extern Frame init_frame();
extern Frame init_print();
extern Frame init_props();
extern void  recreate_AAcanvas();
extern void  dps_text_handler ();
extern void  dps_error_handler ();
extern void  dps_status_handler ();

extern Display *dsp;
extern DPSContext dps_context;
extern XStandardColormap *gray_cmap;
extern XStandardColormap *rgb_cmap;
extern int dps;
extern FILE *fp;
extern GC gc;
extern GC pixmap_gc;
extern GC low_memory_gc;
extern Panel panel;
extern Canvas canvas;
extern Pixmap pixmap;
extern int depth;
extern Xv_server My_server;
extern Window win;
extern int AAscale;
extern Panel_item textsw_drop_target;
extern Panel_item main_panel_drop_target;
extern Selection_requestor Current_Sel;
extern Dnd Current_Dnd_object;
extern Selection_Op Current_Op;
extern Notice_Types pgv_notice;
extern Panel_item the_source_item;
extern Server_image source_drag_ptr_image;
extern Server_image source_drop_ptr_image;
extern Textsw edit_textsw;
extern char *PV_Name;
extern char *ProgramName;
extern char Directory [];
extern char *dnd_file;
extern char *edit_tmpfile;
extern char *basename();
extern char iconname [];
extern char pathname [];
extern char newsserver [];
extern char *icon_label;
extern char line[MAX_LINE_LENGTH];
extern char hostname [MAXHOSTNAMELEN + 1];
extern float pageheight;
extern float pagewidth;
extern int  CurrentPage;
extern int  NumberOfPages;
extern int  dpi;
extern int  inImportedDocument;
extern int  mono;
extern int  pixh;
extern int  pixw;
extern double ctm_a;
extern double ctm_b;
extern double ctm_c;
extern double ctm_d;
extern int  ctm_tx;
extern int  ctm_ty;
extern int  screen;
extern int  standardin;
extern int  verbose;
extern int  footer_set;
extern int  reverse;
extern int  low_memory;
extern long *pagestart;
extern long BeginSetup;
extern long EndProlog;
extern long EndSetup;
extern void update_page();
extern void edit_file();
extern void file_not_found();
extern void newfile();
extern void setbusy();
extern void setactive();
extern void set_right_footer();
extern void set_footers();
extern void set_icon_label();
extern void append_output();
extern void place_drop_site ();
extern int drop_target_notify_proc ();
extern int PageviewSelectionConvert ();
extern void quit_pageview ();
extern void pageview_ps_close ();
extern void home_page ();

#define DEFAULT_PAGESIZE 0
#define DEFAULT_DPI 1
#define DEFAULT_ORIENT 3
#define DEFAULT_AA 0
#define DEFAULT_TIMEOUT 30
#define DEFAULT_METHOD 0
#define NULL 0
