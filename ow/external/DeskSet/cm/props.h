/*static  char sccsid[] = "@(#)props.h 3.18 94/08/17 Copyr 1991 Sun Microsystems, Inc.";

	props.h

*/

#define RC_FILENAME ".cm.rc"
#define DS_FILENAME ".desksetdefaults"
#define OFFSET 3
extern char *property_names[];

typedef enum {
        hour12, hour24
} DisplayType;

typedef struct props_entry {
	char	*property_name;
	char	*property_value;
	int	update;
	struct props_entry *next;
} Props_entry;

typedef struct {
        Frame           frame;
        Panel           panel;
	Panel_item	category_item;

	/*=================== Reminders ====================*/
        Panel_item      reminderstr;   
	Panel_item	reminder;
	Panel_item	beepadvance;
	Panel_item	beepunitstack;
	Panel_item	beepunit;
	Panel_item	flashadvance;
	Panel_item	flashunitstack;
	Panel_item	flashunit;
	Panel_item	openadvance;
	Panel_item	openunitstack;
	Panel_item	openunit;
	Panel_item	mailadvance;
	Panel_item	mailunitstack;
	Panel_item	mailunit;
	Panel_item	mailtostr;
	Panel_item	mailto;
        Panel_item      privacystr;
        Panel_item      privacystack;
        Panel_item      privacyunit;
/* SCRIPTS
	Panel_item	unixadvance;
	Panel_item	unixhrs;
	Panel_item	unixcmd;
*/
	/*================Display Settings ===== */
	Panel_item	dayboundstr;
	Panel_item	beginstr;
	Panel_item	beginslider;
	Panel_item	beginslider_str;
	Panel_item	endstr;
	Panel_item	endslider;
	Panel_item	endslider_str;
	Panel_item	defviewstr;
	Panel_item	default_view;
	Panel_item	defdispstr;
	Panel_item	default_disp;
	Panel_item	defcalstr;
	Panel_item	defcal;
	Panel_item	clocstr;
	Panel_item	cloc;
	/*================Access list ===== */
	Panel_item	access_list;
	Panel_item	access_msg;
	Panel_item	perms;
	Panel_item	addbutton;
	Panel_item	removebutton;
	Panel_item	sortbutton;
	Panel_item	username;
	/*================Date Format ====== */
	Panel_item	ordering_str;
	Panel_item	ordering_list;     
	Panel_item	separator_str;
	Panel_item	separator_list;		
	Ordering_Type       ordering_VAL;
	Separator_Type      separator_VAL;
	/*================PRINTER OPTS ===== */
	Panel_item	dest_item;
	Panel_item	dest_choice;
	Panel_item	meo_str;
	Panel_item	meo;
	Panel_item	printerstr;
	Panel_item	dirstr;
	Panel_item	filestr;
	Panel_item	printer_name;
	Panel_item	dir_name;
	Panel_item	file_name;
	Panel_item	optionstr;
	Panel_item	options;
	Panel_item	out_width;
	Panel_item	scale_width;
	Panel_item	height;
	Panel_item	scale_height;
	Panel_item	position;
	Panel_item	offset_width;
	Panel_item	inches_left;
	Panel_item	offset_height;
	Panel_item	inches_bottom;
	Panel_item	repeatstr;
	Panel_item	repeat_times;
	Panel_item	copiesstr;
	Panel_item	copies;
	Panel_item	apply_button;
	Panel_item	reset_button;
	Panel_item	defaults_button;

	int		reminder_VAL;
	char		beepadvance_VAL[5];
	int		beepunit_VAL;
	char		flashadvance_VAL[5];
	int		flashunit_VAL;
	char		openadvance_VAL[5];
	int		openunit_VAL;
	char		mailadvance_VAL[5];
	int		mailunit_VAL;
	char		mailto_VAL[512];
	int		privacyunit_VAL;
/* SCRIPTS
	char		unixadvance_VAL[5];
	char		unixcmd_VAL[256];
*/
	char 		beginslider_str_VAL[10];
	char 		endslider_str_VAL[10];
	int 		default_view_VAL;
	DisplayType 	default_disp_VAL;
	char 		defcal_VAL[512];
	char 		cloc_VAL[512];
	int		begin_slider_VAL;
	int		end_slider_VAL;

	int		perms_VAL;

	int             dest_choiceVAL;
	int             meoVAL;
        char            printer_nameVAL[81];
        char            dir_nameVAL[BUFSIZ];
        char            file_nameVAL[BUFSIZ];
        char            optionVAL[81];
        char            heightVAL[10];
        char            widthVAL[10];
        char            xoffsetVAL[10];
        char            yoffsetVAL[10];
        char            repeatVAL[5];
        char            copiesVAL[5];

	int 		longest_str;   /* for internationalization */
	int		last_props_pane; /* for going back to prev pane
					when canceling a dismissed window */
} Props;

/* if you add to this you must also add a property_names[] to props.c */ 
/* these are indexes into the property_names[] array */
#define	CP_BEEPON		0
#define	CP_BEEPADV		1
#define	CP_BEEPUNIT		2
#define	CP_FLASHON		3
#define	CP_FLASHADV		4
#define	CP_FLASHUNIT		5
#define	CP_OPENON		6
#define	CP_OPENADV		7
#define	CP_OPENUNIT		8
#define	CP_MAILON		9
#define	CP_MAILADV		10
#define	CP_MAILUNIT		11	
#define	CP_MAILTO		12	
#define	CP_UNIXON		13	
#define	CP_UNIXADV		14	
#define	CP_UNIXCOMMAND		15
#define	CP_DAYBEGIN		16
#define	CP_DAYEND		17
#define	CP_DAYCALLIST		18
#define	CP_DEFAULTVIEW		19
#define	CP_DEFAULTDISP		20
#define	CP_PRINTDEST		21 
#define	CP_PRINTPRIVACY		22 
#define	CP_PRINTERNAME		23 
#define	CP_PRINTOPTIONS		24 
#define	CP_PRINTDIRNAME		25 
#define	CP_PRINTFILENAME	26 
#define	CP_PRINTWIDTH		27 
#define	CP_PRINTHEIGHT		28 
#define	CP_PRINTPOSXOFF		29 
#define	CP_PRINTPOSYOFF		30 
#define	CP_PRINTMONTHS		31 
#define	CP_PRINTCOPIES		32 
#define	CP_DEFAULTCAL		33
#define	CP_CALLOC		34
#define CP_DATEORDERING		35
#define	CP_DATESEPARATOR	36
#define	CP_PRIVACY		37

#define NUM_PROPS		37

#define EDITOR_DEFAULTS      0
#define DISPLAY_SETTINGS     1
#define GROUP_ACCESS_LISTS   2
#define PRINTER_OPTS         3
#define DATE_FORMAT          4

#define NO_OF_PANES          5

extern caddr_t make_props(/* Calendar *c */);
extern void cm_show_props (/* Menu m; Menu_item mi */);
extern char * cm_get_property(/* char *property_name */);
extern void cal_set_property();
extern Boolean cal_update_props();
extern void set_default_vals();
extern void set_rc_vals();
extern void p_show_proc();

/* property defaults */
#define REMINDER_DEF 0
#define BEEPADV_DEF  "5"
#define FLASHADV_DEF "5"
#define OPENADV_DEF  "5"
#define MAILADV_DEF  "2"
#define BEGINS_DEF   7
#define ENDS_DEF     19
#define DISP_DEF     0
#define VIEW_DEF     1
#define CHOICE_DEF   0
#define MEO_DEF      7
#define PRIVACY_DEF  0
#ifdef SVR4
#define OPTIONS_DEF  ""
#else
#define OPTIONS_DEF  "-h"
#endif
#define SCALEH_DEF   "10.00"
#define SCALEW_DEF   "7.50"
#define XWID_DEF     "1.00"
#define XHGT_DEF     "1.00"
#define REP_DEF      1
#define COPIES_DEF   1
#define ORDER_DEF    0
#define SEP_DEF      1
#define PSCAL_DEF    "calendar.ps"
