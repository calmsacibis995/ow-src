#ifndef lint
#ifdef sccs
static  char sccsid[] = "@(#)props.c 3.18 96/01/04 Copyr 1987 Sun Micro";
#endif
#endif

/*	Copyright (c) 1987, 1988, Sun Microsystems, Inc.  All Rights Reserved.
	Sun considers its source code as an unpublished, proprietary
	trade secret, and it is available only under strict license
	provisions.  This copyright notice is placed here only to protect
	Sun in the event the source is deemed a published work.  Dissassembly,
	decompilation, or other means of reducing the object code to human
	readable form is prohibited by the license agreement under which
	this code is provided to the user or company in possession of this
	copy.

	RESTRICTED RIGHTS LEGEND: Use, duplication, or disclosure by the 
	Government is subject to restrictions as set forth in subparagraph 
	(c)(1)(ii) of the Rights in Technical Data and Computer Software 
	clause at DFARS 52.227-7013 and in similar clauses in the FAR and 
	NASA FAR Supplement. */

/*
 * Mailtool - creation of property panels
 */
#define NULLSTRING (char *) 0
#include <stdio.h>
#ifdef SVR4
#include <unistd.h>
#include <sys/fcntl.h>
#endif SVR4
#include <signal.h>
#include <ctype.h>
#include <string.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/file.h>
#include <sys/param.h>

#include <xview/panel.h>
#include <xview/text.h>
#include <xview/font.h>
#include <xview/xview.h>
#include <xview/canvas.h>
#include <xview/scrollbar.h>
#include <xview/alert.h>

#include "glob.h"
#include "tool.h"
#include "cmds.h"
#include "tool_support.h"
#include "ds_popup.h"
#include "instrument.h"
#include "buttons.h"
#include "mle.h"
#include "../maillib/ck_strings.h"
#include "../maillib/assert.h"

#define DEBUG_FLAG mt_debugging
#include "debug.h"

/*
 * Note that "current_value" is the current or new value of the .mailrc
 * variable specified by "label"  In the case of MT_TOGGLEs this will
 * be different from the PANEL_VALUE of "item" (see next comment)
 * "default_value" is the value displayed in the prop sheet if no
 * entry in the .mailrc file is found.
 * Note that for toggles this default is always FALSE
 */
typedef struct	{
	char	*label;
	char	*help_data;
	char	*lead_label;
	char	*trail_label;
	char	*choice_string; /* for check boxes */
	char	type;
	short	size;
	/* for spacing, if need more rows underneath */ 
	short	additional_rows;  
	Panel_item	itemtraillabel;
	Panel_item	item;
	char	*default_value;
	char	*current_value;
	void	(*manual_create_proc)();
	void	(*manual_reset_proc)();
	void	(*manual_apply_proc)();
} PROPS_ENTRY;

typedef struct  {
	Panel_item  add;
	Panel_item  delete;
	Panel_item  change;
	Panel_item  text_field1;
	Panel_item  text_field2;
	Panel_item  scroll_list;
} PROPS_LIST_BOX;

typedef struct {
	char *list_label;
	char *text1_label;
	char *text2_label;
	char *help_data;
	int   longestlabel;
	int   list_width_in_chars;
	int   list_display_rows;
} LABELS;       /* to pass labels to mt_build_header_list routine */

/* 
 * to hold the programable buttons,
 * for external and internal label strings
 */
typedef struct {
	char *external_current;
	char *external_factory;
	char *in_menu_only;
	char *internal;
} PROG_BUTTON;  

extern char	**get_sys_button_list();
extern void	free_sys_button_list();
extern char	*Getf();

extern int	mt_props_proc();
extern int 	mt_aliases_proc();
extern int	mt_props_init();
extern int	mt_build_propspanel();
extern int	mt_set_props_values();
extern int	mt_set_props_group_values();
extern int	props_done_proc();
extern int	category_proc();
extern int	mt_show_props();
extern int	mt_show_header_fields();
extern PROPS_ENTRY * mt_get_current_props();
extern Panel 	mt_get_current_panel();
extern int 	report_change();
extern int 	report_change1();
extern int 	report_change2();
extern int 	report_custom_buttons_txtfld_change();
extern int 	report_sl_txtfld_change();
extern int	mt_props_reset_proc();
extern void	list_flush();
extern int	load_header_fields();
extern int	mt_props_apply_proc();
extern int 	check_mail_file_directory();
extern int	mt_check_and_create();
extern int	write_values();
extern void	custom_buttons_proc();
extern void	create_commands_choices();
extern void	commands_choices_proc();
extern void	reset_custom_buttons();
extern void	apply_custom_buttons();
extern void	prepare_list_box();
extern PROPS_LIST_BOX * build_header_list_box();
extern void	field_menu_proc();
extern void	add_proc();
extern void	delete_proc();
extern void	change_proc();
extern int	get_selected_list_no();
extern int	add_header_field_to_list();
extern int	validate_header_field();
extern int	mt_header_field_add_before();
extern int	mt_header_field_add_after();
extern int	mt_header_field_delete();
extern int	list_notify();
extern int	write_header_fields();
extern void	put_with_escape();
extern int	write_message_fields();
extern int 	write_alias_fields();
extern int 	load_ignore_defaults();
extern int 	load_alias_defaults();
extern int 	load_ignore_header_proc();
extern int 	load_alias_header_proc();
extern int 	parse_external_string();

/* Functions in generate_menus.c */
extern void	mt_props_dynamic_update_menus();
/* Function in filelist.c */
extern void	mt_props_dynamic_update_mailfiles_popup();


static void	add_buttons(Panel);
static void	create_custom_buttons(PROPS_ENTRY *);
static void	create_custom_buttons_textfield(PROPS_ENTRY *);

/*
 * Property panel item types.  Note that for toggle PANEL_VALUEs Yes==0
 * and No==1 (since Yes is the first choice) so we must invert the .mailrc
 * value when we manipulate the PANEL_VALUE.
 *
 * MT_INV_TOGGLE is a special case for when the .mailrc variable is worded
 * opposite to the item on the property panel.  For example, when
 * "Automatically Display Headers: " is Yes on the property sheet we want
 * the .mailrc variable "suppressautoretrieve" to be No. Got it?
 *
 * MT_NUMBER is implemented as NUMERIC_TEXT so its PANEL_VALUE is an int
 * as opposed to a string.  That's why it's always converted back and
 * forth when displaying it.
 *
 */
#define	MT_TOGGLE		1
#define	MT_TEXT			2
#define	MT_NUMBER		3
#define	MT_MSG			4
#define	MT_INV_TOGGLE		5
#define	MT_CHECK_BOX		6	
#define	MT_INV_CHECK_BOX	7	
#define	MT_MANUAL_CREATE	8

Frame	mt_props_frame = NULL;
/*
 * For OPEN LOOK Compliance, we need top panel
 * for border box. The other panels are to
 * save real estate since some panels take up more space
 */
static	Panel	mt_props_panel_top = NULL;
static	Panel	mt_props_panel1     = NULL;
static	Panel	mt_props_panel2     = NULL;
Panel_item	mt_category_item = NULL;

PROPS_ENTRY	mt_propentries[25];
PROPS_ENTRY	*mt_get_current_props();

PROPS_ENTRY	mt_header_props[15];
PROPS_ENTRY	mt_message_props[15];
PROPS_ENTRY	mt_compose_props[15];
PROPS_ENTRY	mt_filing_props[15];
PROPS_ENTRY     mt_template_props[15];
PROPS_ENTRY	mt_alias_props[15];
PROPS_ENTRY	mt_expert_props[15];

PROPS_LIST_BOX  *mt_message_propslist;
PROPS_LIST_BOX  *mt_compose_propslist; 
PROPS_LIST_BOX  *mt_filing_propslist; 
PROPS_LIST_BOX  *mt_template_propslist;
PROPS_LIST_BOX  *mt_alias_propslist;


#define KEY_DONTLOGMESSAGES	 1
#define BORDER_WIDTH		 1
#define PROG_BUTTONS1_NUMBER	 4   	/* needed for create loop */
#define PROG_BUTTONS2_NUMBER	52	/* needed for create loop */
/* prog_buttons1[] used to store the 4 user programable button values
 * prog_buttons2[] used to store the complete list, including
 * external and internal names
 */
static PROG_BUTTON	prog_buttons1[] =
{
	{NULL,  NULL, NULL, NULL },
	{NULL,  NULL, NULL, NULL },
	{NULL,  NULL, NULL, NULL },
	{NULL,  NULL, NULL, NULL },
};
static PROG_BUTTON	prog_buttons2[] =
{
	{NULL,  NULL, NULL, NULL },
	{NULL,  NULL, NULL, NULL },
	{NULL,  NULL, NULL, NULL },
	{NULL,  NULL, NULL, NULL },
	{NULL,  NULL, NULL, NULL },
	{NULL,  NULL, NULL, NULL },
	{NULL,  NULL, NULL, NULL },
	{NULL,  NULL, NULL, NULL },
	{NULL,  NULL, NULL, NULL },
	{NULL,  NULL, NULL, NULL },
	{NULL,  NULL, NULL, NULL },
	{NULL,  NULL, NULL, NULL },
	{NULL,  NULL, NULL, NULL },

	{NULL,  NULL, NULL, NULL },
	{NULL,  NULL, NULL, NULL },
	{NULL,  NULL, NULL, NULL },
	{NULL,  NULL, NULL, NULL },
	{NULL,  NULL, NULL, NULL },
	{NULL,  NULL, NULL, NULL },
	{NULL,  NULL, NULL, NULL },
	{NULL,  NULL, NULL, NULL },
	{NULL,  NULL, NULL, NULL },
	{NULL,  NULL, NULL, NULL },
	{NULL,  NULL, NULL, NULL },
	{NULL,  NULL, NULL, NULL },
	{NULL,  NULL, NULL, NULL },

	{NULL,  NULL, NULL, NULL },
	{NULL,  NULL, NULL, NULL },
	{NULL,  NULL, NULL, NULL },
	{NULL,  NULL, NULL, NULL },
	{NULL,  NULL, NULL, NULL },
	{NULL,  NULL, NULL, NULL },
	{NULL,  NULL, NULL, NULL },
	{NULL,  NULL, NULL, NULL },
	{NULL,  NULL, NULL, NULL },
	{NULL,  NULL, NULL, NULL },
	{NULL,  NULL, NULL, NULL },
	{NULL,  NULL, NULL, NULL },
	{NULL,  NULL, NULL, NULL },

	{NULL,  NULL, NULL, NULL },
	{NULL,  NULL, NULL, NULL },
	{NULL,  NULL, NULL, NULL },
	{NULL,  NULL, NULL, NULL },
	{NULL,  NULL, NULL, NULL },
	{NULL,  NULL, NULL, NULL },
	{NULL,  NULL, NULL, NULL },
	{NULL,  NULL, NULL, NULL },
	{NULL,  NULL, NULL, NULL },
	{NULL,  NULL, NULL, NULL },
	{NULL,  NULL, NULL, NULL },
	{NULL,  NULL, NULL, NULL },
	{NULL,  NULL, NULL, NULL },
};

/* prog_buttons2 is the data structure PROG_BUTTONS, 
   custom_buttons is ptr to PROPS_ENTRY to access 
    the panel item with the label "Custom Buttons",
 */
static int change;
static int expert_ttlock_change;
static int custom_buttons_txtfld_change;
static int last_commands_value;
static int sl_txtfld_change;
static int from_category_proc;
static PROPS_ENTRY *custom_buttons;
static PROPS_ENTRY *custom_buttons_textfield;
static PROPS_ENTRY *commands_choices;
extern int ignore_exists;
static int longest_alias_value;
extern Xv_Font textsw_font;
/* Set by props init, checked by apply proc, to see if need to check
 * for compose windows to remove the logmessages checkbox
 */
static int old_record_exists = FALSE;
static int PROPS_ENTRY_KEY;


mt_props_proc(menu, menu_item)
Menu		menu;
Menu_item	menu_item;

{
	TRACK_BUTTON(menu, menu_item, "properties");

	if (!mt_props_frame) {
		mt_props_init();
		ds_position_popup(mt_frame, mt_props_frame, DS_POPUP_LOR);
	}

	xv_set(mt_props_frame,
		XV_SHOW, TRUE,
		FRAME_PROPS_PUSHPIN_IN,	TRUE,
		0);
}

mt_alias_proc(item, ie)
Panel_item      item; /* Aliases... button from Compose Window */
Event           *ie;

{
	if (!mt_props_frame) {
		mt_props_init();
		ds_position_popup(mt_frame, mt_props_frame, DS_POPUP_LOR);
	}

	xv_set(mt_props_frame,
		XV_SHOW, TRUE,
		FRAME_PROPS_PUSHPIN_IN,	TRUE,
		0);

	xv_set(mt_category_item,
		PANEL_VALUE, 5, 0);
	category_proc(mt_category_item, 5, NULL);
}

mt_props_init()

{
	register int	i;
	Menu		menu;
	Menu_item	menu_item;

	/* initialize tokens for XV_KEY_DATA */
	if (! PROPS_ENTRY_KEY) PROPS_ENTRY_KEY = xv_unique_key();

	/* 
	 * For those simple variables (set or set no),
	 * mt_value gives "" or NULL.
	 * If they don't exist, mt_value also gives NULL.
	 * So if I get a NULL, and default is "set",
	 * I get confused (don't know to set it or not),
	 * because I don't know if NULL is
	 * from the "set no" or because it does not exist.
	 * Therefore, I always want to word my variables
	 * such that default is ALWAYS NULL, so we won't
	 * get any conflicts.
	 */
	/*
	 * For both the CHECK_BOX and INV_CHECK_BOX items,
	 * we set the default_value field as NULL if
	 * default is "set no<var>",
	 * and "1" if we mean "set <var>" as default.
	 */
        /* STRING_EXTRACTION -
         *
         * The period to check for new mail
         */
	i = 0;
	mt_header_props[i].help_data = "mailtool:PropsRetrieveinterval";
	mt_header_props[i].label = "retrieveinterval";
	mt_header_props[i].type = MT_NUMBER;
	mt_header_props[i].size = 4;
	mt_header_props[i].lead_label = gettext("Retrieve Every:");
	mt_header_props[i].trail_label = gettext("Seconds");
	mt_header_props[i].default_value = "300";

        /* STRING_EXTRACTION -
         *
         * indicate the reception of new mail with the specified number
         * of beeps and/or flashes.
         */
	i++;
	mt_header_props[i].help_data = "mailtool:PropsBell";
	mt_header_props[i].label = "bell";
	mt_header_props[i].type = MT_NUMBER;
	mt_header_props[i].size = 4;
	mt_header_props[i].lead_label = gettext("Signal With:");
	mt_header_props[i].trail_label = gettext("Beep(s)");
	mt_header_props[i].default_value = "0";

	i++;
	mt_header_props[i].help_data = "mailtool:PropsFlash";
	mt_header_props[i].label = "flash";
	mt_header_props[i].type = MT_NUMBER;
	mt_header_props[i].size = 4;
	mt_header_props[i].lead_label = "";
	mt_header_props[i].trail_label = gettext("Flash(es)");
	mt_header_props[i].default_value = "0";

        /* STRING_EXTRACTION -
         *
         * Allow number lines of mail summary headers to be initially
         * displayed in mailtool's main mail summary window
         */
	i++;
	mt_header_props[i].help_data = "mailtool:PropsHeaderlines";
	mt_header_props[i].label = "headerlines";
	mt_header_props[i].type = MT_NUMBER;
	mt_header_props[i].size = 4;
	mt_header_props[i].lead_label = gettext("Display:");
	mt_header_props[i].trail_label = gettext("Headers");
	mt_header_props[i].default_value = "15";

        /* STRING_EXTRACTION -
         *
         * Allow number columns of text to be
         * displayed in mailtool's main mail summary window
         */
	i++;
	mt_header_props[i].help_data = "mailtool:PropsToolcols";
	mt_header_props[i].label = "toolcols";
	mt_header_props[i].type = MT_NUMBER;
	mt_header_props[i].size = 4;
	mt_header_props[i].lead_label = "";
	mt_header_props[i].trail_label = gettext("Characters wide");
	mt_header_props[i].default_value = "80";

        /* STRING_EXTRACTION -
         *
         * Whether or not new mail should be automatically received or not.
         */
	i++;
	mt_header_props[i].help_data = "mailtool:PropsAutoretrieve";
	mt_header_props[i].label = "suppressautoretrieve";
	mt_header_props[i].type = MT_INV_CHECK_BOX;
	mt_header_props[i].size = 0;
	mt_header_props[i].lead_label = gettext("Delivery:");
	mt_header_props[i].choice_string = gettext("Automatically display headers");
	mt_header_props[i].trail_label = (char *)NULL;
	mt_header_props[i].default_value = (char *)NULL;

        /* STRING_EXTRACTION -
         *
         * Too long to explain, explanation is given in help text.
         * (The help text says "When displaying the header summary 
         * and the message is from you, print the recipient's name
         * instead of the author's name".)
         */
	i++;
	mt_header_props[i].help_data = "mailtool:PropsShowto";
	mt_header_props[i].label = "showto";
	mt_header_props[i].type = MT_CHECK_BOX;
	mt_header_props[i].size = 0;
	mt_header_props[i].lead_label = "";
	mt_header_props[i].choice_string = gettext("Show \"To: recipient\" when mail is from me");
	mt_header_props[i].trail_label = (char *)NULL;
	mt_header_props[i].additional_rows = 1;
	mt_header_props[i].default_value = (char *)NULL;

        /* STRING_EXTRACTION -
         *
         * Allow user to customize the second line of buttons
         * in mailtool
         */
	i++;
	mt_header_props[i].help_data = "mailtool:PropsCustombuttons";
	mt_header_props[i].label = "";
	mt_header_props[i].type = MT_MANUAL_CREATE;
	mt_header_props[i].size = 0;
	mt_header_props[i].lead_label = gettext("Custom Buttons:");
	mt_header_props[i].trail_label = (char *)NULL;
	mt_header_props[i].default_value = "0";
	mt_header_props[i].manual_create_proc = create_custom_buttons;
	mt_header_props[i].manual_reset_proc  = reset_custom_buttons;
	mt_header_props[i].manual_apply_proc  = apply_custom_buttons;

        /* STRING_EXTRACTION -
         *
         * Allow user to select the command for one of the buttons
         * in mailtool's second line of buttons
         */
	i++;
	mt_header_props[i].help_data = "mailtool:PropsCommandbuttons";
	mt_header_props[i].label = "";
	mt_header_props[i].type = MT_MANUAL_CREATE;
	mt_header_props[i].size = 0;
	mt_header_props[i].lead_label = gettext("Command:");
	mt_header_props[i].trail_label = (char *)NULL;
	mt_header_props[i].default_value = "0";
	mt_header_props[i].manual_create_proc = create_commands_choices;
	mt_header_props[i].manual_reset_proc  = NULL;
	mt_header_props[i].manual_apply_proc  = NULL;

        /* STRING_EXTRACTION -
         *
         * Allow user to change the name of the command they have chosen
         * for the customized buttons.
         */
	i++;
	mt_header_props[i].help_data = "mailtool:PropsLabelbuttons";
	mt_header_props[i].label = "";
	mt_header_props[i].type = MT_MANUAL_CREATE;
	mt_header_props[i].size = 80;
	mt_header_props[i].lead_label = gettext("Label:");
	mt_header_props[i].trail_label = (char *)NULL;
	mt_header_props[i].default_value = "";
	mt_header_props[i].manual_create_proc = create_custom_buttons_textfield;
	mt_header_props[i].manual_reset_proc  = NULL;
	mt_header_props[i].manual_apply_proc  = NULL;

	i++;
	mt_header_props[i].label = NULL;
	mt_header_props[i].type = NULL;
	mt_header_props[i].size = NULL;


        /* STRING_EXTRACTION -
         *
         * Allow for up to specified number of text displayed
         * for mail message pop-up window.
         */
	i = 0;
	mt_message_props[i].help_data = "mailtool:PropsPopuplines";
	mt_message_props[i].label = "popuplines";
	mt_message_props[i].type = MT_NUMBER;
	mt_message_props[i].size = 4;
	mt_message_props[i].lead_label = gettext("Display:");
	mt_message_props[i].trail_label = gettext("Lines of Text");
	mt_message_props[i].default_value = "30";


        /* STRING_EXTRACTION -
         *
         * Specify the print script used to print mail messages.
         */
	i++;
	mt_message_props[i].help_data = "mailtool:PropsPrint";
	mt_message_props[i].label = "printmail";
	mt_message_props[i].lead_label = gettext("Print Script:");
	mt_message_props[i].trail_label = (char *)NULL;
	mt_message_props[i].type = MT_TEXT;
	mt_message_props[i].size = 160;
#ifdef SVR4
	mt_message_props[i].default_value = "lp -s";
#else
	mt_message_props[i].default_value = "lpr -p";
#endif SVR4

	i++;
	mt_message_props[i].label = NULL;
	mt_message_props[i].type = NULL;
	mt_message_props[i].size = NULL;


        /* STRING_EXTRACTION -
         *
         * Specify the short string used to mark included mail messages with.
         */
	i = 0;
	mt_compose_props[i].help_data = "mailtool:PropsIndentprefix";
	mt_compose_props[i].label = "indentprefix";
	mt_compose_props[i].lead_label = gettext("Included Text Marker:");
	mt_compose_props[i].trail_label = (char *)NULL;
	mt_compose_props[i].type = MT_TEXT;
	mt_compose_props[i].size = 80;
	mt_compose_props[i].default_value = "> ";

        /* STRING_EXTRACTION -
         *
         * Specify a file to log outgoing messages
         */
	i++;
	mt_compose_props[i].help_data = "mailtool:PropsRecord";
	mt_compose_props[i].label = "record";
	mt_compose_props[i].type = MT_TEXT;
	mt_compose_props[i].size = 160;
	mt_compose_props[i].lead_label = gettext("Logged Messages File:");
	mt_compose_props[i].trail_label = (char *)NULL;
	/* Specify "" instead of NULL so reset will xv_set
	 * an empty string and wipe out what's there.
	 * During apply, it will see it is empty string and will set no
	 * to it.
	 */
	mt_compose_props[i].default_value = "";

        /* STRING_EXTRACTION -
         *
         * "Log all messages"
         * controls whether or not the compose window, by default,
         * will display the check box to log all outgoing 
	 * messages. This is ignored if the above textfield
	 * for the variable "record" is blank (i.e.,
	 * no file is specified to log any messages, 
	 * so we ignore this item).
         */
	i++;
	mt_compose_props[i].help_data = "mailtool:PropsLogallmessages";
	mt_compose_props[i].label = "dontlogmessages";
	mt_compose_props[i].type = MT_INV_CHECK_BOX;
	mt_compose_props[i].size = 0;
	mt_compose_props[i].lead_label = "";
	mt_compose_props[i].choice_string = gettext("Log all messages");
	mt_compose_props[i].trail_label = (char *)NULL;
	mt_compose_props[i].default_value = (char *)NULL;


        /* STRING_EXTRACTION -
         *
         * "Ask for confirmations"
         * controls whether or not we should check for 
         * dangerous operations (also known as "expert" mode).
         */
	i++;
	mt_compose_props[i].help_data = "mailtool:PropsExpert";
	mt_compose_props[i].label = "expert";
	mt_compose_props[i].type = MT_INV_CHECK_BOX;
	mt_compose_props[i].size = 0;
	mt_compose_props[i].lead_label = gettext("Defaults:");
	mt_compose_props[i].choice_string = gettext("Request confirmations");
	mt_compose_props[i].trail_label = (char *)NULL;
	mt_compose_props[i].default_value = (char *)NULL;

        /* STRING_EXTRACTION -
         *
         * Whether or not to have attachments in the Compose window
         */
	i++;
	mt_compose_props[i].help_data = "mailtool:PropsShowAttach";
	mt_compose_props[i].label = "hideattachments";
	mt_compose_props[i].type = MT_INV_CHECK_BOX;
	mt_compose_props[i].size = 0;
	mt_compose_props[i].lead_label = "";
	mt_compose_props[i].choice_string = gettext("Show attachment list");
	mt_compose_props[i].trail_label = (char *)NULL;
	mt_compose_props[i].default_value = (char *)NULL;

#ifdef NEVER
        /* STRING_EXTRACTION -
         *
         * We ask about whether or not the Subject, Cc, or Bcc fields
         * should be included (by default) in a message compose pane.
         */
	i++;
	mt_compose_props[i].help_data = "mailtool:PropsAsksubj";
	mt_compose_props[i].label = "asksub";
	mt_compose_props[i].type = MT_TOGGLE;
	mt_compose_props[i].size = 0;
	mt_compose_props[i].lead_label = gettext("Compose with Subject Line:");
	mt_compose_props[i].trail_label = (char *)NULL;

	i++;
	mt_compose_props[i].help_data = "mailtool:PropsAskcc";
	mt_compose_props[i].label = "askcc";
	mt_compose_props[i].type = MT_TOGGLE;
	mt_compose_props[i].size = 0;
	mt_compose_props[i].lead_label = gettext("Cc Line:");
	mt_compose_props[i].trail_label = (char *)NULL;

	i++;
	mt_compose_props[i].help_data = "mailtool:PropsAskbcc";
	mt_compose_props[i].label = "askbcc";
	mt_compose_props[i].type = MT_TOGGLE;
	mt_compose_props[i].size = 0;
	mt_compose_props[i].lead_label = gettext("Bcc Line:");
	mt_compose_props[i].trail_label = (char *)NULL;
#endif

	i++;
	mt_compose_props[i].label = NULL;
	mt_compose_props[i].type = NULL;
	mt_compose_props[i].size = NULL;

        /* STRING_EXTRACTION -
         *
         * Specify the directory to store all your mail folders.
         */
	i = 0;
	mt_filing_props[i].help_data = "mailtool:PropsFolder";
	mt_filing_props[i].label = "folder";
	mt_filing_props[i].type = MT_TEXT;
	mt_filing_props[i].size = 160;
	mt_filing_props[i].lead_label = gettext("Mail File Directory:");
	mt_filing_props[i].trail_label = (char *)NULL;
	mt_filing_props[i].default_value = "";

        /* STRING_EXTRACTION -
         *
         * Specify maximum number of menu items allowed in
         * the Move, Copy, and Load menu stacks.
         */
	i++;
	mt_filing_props[i].help_data = "mailtool:PropsFilemenusize";
	mt_filing_props[i].label = "filemenusize";
	mt_filing_props[i].type = MT_NUMBER;
	mt_filing_props[i].size = 4;
	mt_filing_props[i].lead_label = gettext("Display Up To:");
	mt_filing_props[i].trail_label = gettext("Files in Menus");
	mt_filing_props[i].default_value = "10";

	i++;
	mt_filing_props[i].label = NULL;
	mt_filing_props[i].type = NULL;
	mt_filing_props[i].size = NULL;


	i = 0;
	mt_template_props[i].label = NULL;
	mt_template_props[i].type = NULL;
	mt_template_props[i].size = NULL;

	i = 0;
	mt_alias_props[i].label = NULL;
	mt_alias_props[i].type = NULL;
	mt_alias_props[i].size = NULL;

	i = 0;
        /* STRING_EXTRACTION -
         *
         * Following two (allnet and metoo) are too long to explain, 
	 * explanation is given in help text.
	 * The term allnet and metoo should not be translated;
	 * they are mailtool variables and should be in English.
         */
	mt_expert_props[i].help_data = "mailtool:Metoo";
	mt_expert_props[i].label = "metoo";
	mt_expert_props[i].type = MT_CHECK_BOX;
	mt_expert_props[i].size = 0;
	mt_expert_props[i].lead_label = gettext("Defaults:");
	mt_expert_props[i].choice_string = gettext("Include me when I \"Reply To All\" (metoo)");
	mt_expert_props[i].trail_label = (char *)NULL;
	mt_expert_props[i].default_value = (char *)NULL;

	i++;
	mt_expert_props[i].help_data = "mailtool:Allnet";
	mt_expert_props[i].label = "allnet";
	mt_expert_props[i].type = MT_CHECK_BOX;
	mt_expert_props[i].size = 0;
	mt_expert_props[i].lead_label = "";
	mt_expert_props[i].choice_string = gettext("Ignore host name in address (allnet)");
	mt_expert_props[i].trail_label = (char *)NULL;
	mt_expert_props[i].default_value = (char *)NULL;

	i++;
	mt_expert_props[i].help_data = "mailtool:Ttlock";
	mt_expert_props[i].label = "ttlock";
	mt_expert_props[i].type = MT_CHECK_BOX;
	mt_expert_props[i].size = 0;
	mt_expert_props[i].lead_label = "";
	mt_expert_props[i].choice_string = gettext("Use network aware mail file locking");
	mt_expert_props[i].trail_label = (char *)NULL;
	mt_expert_props[i].default_value = (char *)NULL;

	i++;
	mt_expert_props[i].label = NULL;
	mt_expert_props[i].type = NULL;
	mt_expert_props[i].size = NULL;

	mt_set_props_values();

	mt_props_frame = (Frame)xv_create(mt_frame, FRAME_CMD,
		FRAME_SHOW_FOOTER,		FALSE,
		FRAME_PROPS_PUSHPIN_IN,		TRUE,
		FRAME_SHOW_RESIZE_CORNER,	TRUE,
		FRAME_DONE_PROC,		props_done_proc,
		0);
	
	/* For OPEN LOOK Compliance with border box we create two panels */
	mt_props_panel_top = xv_get(mt_props_frame, FRAME_CMD_PANEL);

        /* STRING_EXTRACTION -
         *
         * The title of the property sheet is "Properties"
         */
	mt_label_frame(mt_props_frame, gettext("Properties"));

        /* STRING_EXTRACTION -
         *
         * The choice pulldown menu for choosing which property sheet
         * will be edited is labeled "Category".  The six different
         * panels are Header Window,
	 * Message Window 
	 * Compose Window
	 * Mail Filing
	 * Template
	 * Alias
	 * Expert
         */
	mt_category_item = xv_create(mt_props_panel_top, PANEL_CHOICE_STACK,
				     XV_X, xv_col(mt_props_panel_top, 0),
				     XV_Y, xv_row(mt_props_panel_top, 0),
				     PANEL_LABEL_STRING, gettext("Category:"),
				     PANEL_CHOICE_STRINGS,
					gettext("Header Window"),
					gettext("Message Window"),
				        gettext("Compose Window"),
					gettext("Mail Filing"),
				        gettext("Template"),
					gettext("Alias"),
					gettext("Expert"),
				        0,
				     PANEL_NOTIFY_PROC, category_proc,
				     XV_HELP_DATA,	"mailtool:PropsCategory",
				     0);

	window_fit_height(mt_props_panel_top);

	/* Create the bottom panels */

	mt_props_panel1 = (Panel)xv_create(mt_props_frame, PANEL, 
					WIN_BORDER, TRUE,
					WIN_BELOW, mt_props_panel_top,
					XV_X, 0,
					XV_SHOW, FALSE,
					0);

	mt_props_panel2 = (Panel)xv_create(mt_props_frame, PANEL, 
					WIN_BORDER, TRUE,
					WIN_BELOW, mt_props_panel_top,
					XV_SHOW, FALSE,
					XV_X, 0,
					0);

	mt_build_propspanel(mt_header_props,   mt_props_panel1);
	mt_build_propspanel(mt_message_props,  mt_props_panel1);
	mt_build_propspanel(mt_compose_props,  mt_props_panel1);
	mt_build_propspanel(mt_filing_props,   mt_props_panel1);
	mt_build_propspanel(mt_template_props, mt_props_panel1);
	mt_build_propspanel(mt_alias_props,    mt_props_panel2);
	mt_build_propspanel(mt_expert_props,   mt_props_panel1);

	/*
	 * Order of these 2 next sets should be correct 
	 * we want to window fit everything first.
	 * We don't want just window_fit_width because
	 * we want to fit width and height snugly
	 * before going further (or else height of
	 * mt_props_panel2 will be too much).
	 */
	window_fit(mt_props_panel1);
	window_fit(mt_props_panel2);

	mt_show_props(mt_props_panel1, mt_message_props, FALSE);
	mt_show_props(mt_props_panel1, mt_compose_props, FALSE);
	mt_show_props(mt_props_panel1, mt_filing_props, FALSE);
	mt_show_props(mt_props_panel1, mt_template_props, FALSE);
	mt_show_props(mt_props_panel2, mt_alias_props, FALSE);
	mt_show_props(mt_props_panel1, mt_expert_props, FALSE);

	/* 
	 * We shouldn't have to set width and height again
	 * before it's visible
	 */
	/* Set to first prop sheet */
	mt_show_props(mt_props_panel1, mt_header_props, TRUE);
	xv_set(mt_props_panel1, XV_SHOW, TRUE, 0);

	add_buttons(mt_props_panel1);
	add_buttons(mt_props_panel2);

	window_fit_height(mt_props_panel1);
	window_fit_height(mt_props_panel2);

	xv_set(mt_props_panel_top, 
	   XV_WIDTH, xv_get(mt_props_panel1, XV_WIDTH) + 2 * BORDER_WIDTH,
	   0);

	window_fit(mt_props_frame);
	/*
	 * Apparently it works without this
	xv_set(frame, XV_SHOW, TRUE, NULL);
	 */

	/* Init the change flags */
	change = FALSE;
	sl_txtfld_change = FALSE;
	from_category_proc = FALSE;
}


mt_build_propspanel(props_p, panel)

	PROPS_ENTRY	*props_p;
	Panel		panel;
{
	short		i;
	char		*newlabel;
	Panel_item	panel_item;
	Xv_Font		pf = NULL;
	int		longestlabel = 0;
	Font_string_dims font_size;
	char		*str;
	char		*s;
	int		charwidth;
	int		column;
	int		column2;
	register PROPS_ENTRY	*p;
	Panel_item	record_item;


	p = props_p;

	while (p->label)
	{
		newlabel = p->lead_label;

		if (p->type == MT_TOGGLE || p->type == MT_INV_TOGGLE)
		{
			/* 
			 * MT_INV_TOGGLE means that the property sheet item
			 * is worded opposite of the .mailrc value
			 */
			if (p->type == MT_TOGGLE)
				i = (p->current_value == NULL ? 1 : 0);
			else /* MT_INV_TOGGLE */
				i = (p->current_value == NULL ? 0 : 1);

			p->item = xv_create(panel, PANEL_CHOICE,
				PANEL_LABEL_STRING, newlabel,
				PANEL_LABEL_BOLD, TRUE,
                                /* STRING_EXTRACTION -
                                 *
                                 * A boolean choice: either "yes"
                                 * or "no"
                                 */
				PANEL_CHOICE_STRINGS,
					gettext("Yes"), gettext("No"), 0,
				XV_HELP_DATA,	p->help_data,
				PANEL_NOTIFY_PROC, report_change1,
				PANEL_VALUE, i,
				XV_KEY_DATA, PROPS_ENTRY_KEY, p,
				0);
		} 
		else if (p->type == MT_CHECK_BOX || p->type == MT_INV_CHECK_BOX)
		{
			/* 
			 * MT_INV_CHECK_BOX means that the property sheet item
			 * is worded opposite of the .mailrc value
			 */
			if (p->type == MT_CHECK_BOX)
				i = (p->current_value == NULL ? 0 : 1);
			else /* MT_INV_CHECK_BOX */
				i = (p->current_value == NULL ? 1 : 0);

			p->item = xv_create(panel, PANEL_CHECK_BOX,
				PANEL_LABEL_BOLD, TRUE,
				PANEL_LABEL_STRING, newlabel,
				PANEL_CHOICE_STRINGS,
					p->choice_string, 0,
				XV_HELP_DATA,	p->help_data,
				PANEL_NOTIFY_PROC, report_change1,
				PANEL_VALUE, i, 
				XV_KEY_DATA, PROPS_ENTRY_KEY, p,
				0);
			if (strcmp(p->label, "dontlogmessages")==0){
				if (!mt_value("record"))
				     xv_set(p->item, 
					PANEL_INACTIVE, TRUE,
					0);
				/* 
				 * We assume previous item is 
				 * textfield for record var.
				 * Store this panel item addr in
				 * record's panel item
				 */
				xv_set(record_item,
					XV_KEY_DATA,
					   KEY_DONTLOGMESSAGES,
					   p->item,
					0);
			}
		}
		else if (p->type == MT_NUMBER)
		{
			p->item = xv_create(panel,
				PANEL_NUMERIC_TEXT,
				PANEL_LABEL_STRING, newlabel,
				PANEL_LABEL_BOLD, TRUE,
				PANEL_VALUE_DISPLAY_LENGTH,
					p->size > 10 ? 10 : p->size,
				PANEL_VALUE_STORED_LENGTH, p->size,
				XV_HELP_DATA,	p->help_data,
				PANEL_VALUE, atoi(p->current_value),
				PANEL_MAX_VALUE, 999,
				PANEL_NOTIFY_LEVEL, PANEL_ALL,
				PANEL_NOTIFY_PROC, report_change,
				XV_KEY_DATA, PROPS_ENTRY_KEY, p,
				/*PANEL_DELIMITERS,	NULL,*/
				0);
		}
		else if (p->type == MT_TEXT)
		{
			p->item = xv_create(panel, PANEL_TEXT,
				PANEL_LABEL_STRING, newlabel,
				PANEL_LABEL_BOLD, TRUE,
				PANEL_VALUE_DISPLAY_LENGTH,
					p->size > 20 ? 20 : p->size,
				PANEL_VALUE_STORED_LENGTH, p->size,
				XV_HELP_DATA,	p->help_data,
				PANEL_VALUE, p->current_value, 
				PANEL_NOTIFY_LEVEL, PANEL_ALL,
				PANEL_NOTIFY_PROC, report_change,
				XV_KEY_DATA, PROPS_ENTRY_KEY, p,
				0);
			if (strcmp(p->label, "record")==0){
				xv_set(p->item, 
					PANEL_NOTIFY_PROC, 
					report_change2,
					0);
				record_item = p->item;

				if (mt_value("record"))
					old_record_exists = TRUE;
			}
		}
		else if (p->type == MT_MANUAL_CREATE)
		{
			(*(p->manual_create_proc))(p);
		}

		if (pf == NULL)
			pf = (Xv_Font) xv_get(p->item/*label*/, PANEL_LABEL_FONT);
		if (p->trail_label != (char *)NULL)
		{
			p->itemtraillabel = xv_create(panel,
				PANEL_MESSAGE,
				PANEL_LABEL_STRING, p->trail_label,
				XV_HELP_DATA,   p->help_data,
				XV_KEY_DATA, PROPS_ENTRY_KEY, p,
				0);
		}
		p++;
	}


	if (pf == NULL)
		pf = (Xv_Font) xv_get(panel, XV_FONT);

	/* determine longest label */
	p = props_p;
	while (p->label)
	{
		if (p->lead_label != (char *)NULL)
		{
		        xv_get(pf, FONT_STRING_DIMS, p->lead_label, &font_size);
			if (font_size.width > longestlabel)
				longestlabel = font_size.width;
		}
		p++;
	}
	
	xv_get(pf, FONT_STRING_DIMS, "n", &font_size);
	charwidth = font_size.width;

	longestlabel += charwidth;

	/* now go thru and lay out all of the items */

	p = props_p;
	i = 0;
	while (p->label)
	{
		str = (char *) xv_get(p->item, PANEL_LABEL_STRING);
		xv_get(pf, FONT_STRING_DIMS, str, &font_size);

		xv_set(p->item, 
		       PANEL_LABEL_X, 	longestlabel - font_size.width,
		       PANEL_LABEL_Y, 	xv_row(panel, i),
		       0);

		column = longestlabel + 2 * charwidth;
		xv_set(p->item, 
		       PANEL_VALUE_X, 	column,
		       PANEL_VALUE_Y, 	xv_row(panel, i),
		       0);

		if (p->trail_label != (char *)NULL)
		{
			column2 =  xv_get(p->item, XV_X)
				   + xv_get(p->item, XV_WIDTH)
				   + charwidth;
			if (p->lead_label == (char *)NULL)
				column2 += 2 * charwidth;
                        xv_set(p->itemtraillabel,
                                XV_X,   column2,
                                XV_Y,   xv_row(panel, i),
                                0);
		}

		i = i + 1 + p->additional_rows;
		p++;
	}

	/* pass in longestlabel 
	 * for scrolling list layouts 
	 */
	if (props_p == mt_message_props) {
		prepare_list_box(mt_message_props, i + 1,
				 longestlabel);
	} else if (props_p == mt_compose_props) {
		prepare_list_box(mt_compose_props, i + 1,
				 longestlabel);
	/* Have to place a textfield after scroll list
	 * a special case, so hard-coded layout stuff 
	 */
	} else if (props_p == mt_filing_props) {
		i = 0;
		prepare_list_box(mt_filing_props, i+3,
				 longestlabel);
		xv_set(mt_filing_props[0].item, 
		       PANEL_LABEL_Y, 	xv_row(panel, i),
		       PANEL_VALUE_Y, 	xv_row(panel, i),
		       0);
		xv_set(mt_filing_props[1].item, 
		       PANEL_LABEL_Y, 	xv_row(panel, i+2),
		       PANEL_VALUE_Y, 	xv_row(panel, i+2),
		       0);
		xv_set(mt_filing_props[1].itemtraillabel,
                       XV_Y,   xv_row(panel, i+2),
                       0);
	} else if (props_p == mt_template_props) {
		/*
		 * Dummy up a label for templates.
		 * We pass in a row=1 here since it is by itself 
		 *    on one sheet
		 */
		prepare_list_box(mt_template_props, 0,
				 longestlabel);
	} else if (props_p == mt_alias_props) {
		prepare_list_box(mt_alias_props, 0,
				 longestlabel);
	}
}




static void
add_buttons(
	Panel panel
)
{
	int i;
	Panel_item	apply_item, reset_item;

	/* mt_props_panel1 is longest, so use that one */
	i = (int)xv_get(panel, WIN_ROWS) + 1;

        /* STRING_EXTRACTION -
         *
         * The two options for each property sheet are "apply" and "reset"
         */

	apply_item = xv_create(panel, PANEL_BUTTON,
		PANEL_NOTIFY_PROC, mt_props_apply_proc,
		PANEL_LABEL_STRING, gettext("Apply"),
		XV_Y, xv_row(panel, i),
		XV_SHOW,	TRUE,
		XV_HELP_DATA,   "mailtool:PropsApply",
		0);

	xv_set(panel, PANEL_DEFAULT_ITEM, apply_item, 0);

	reset_item = xv_create(panel, PANEL_BUTTON,
		PANEL_NOTIFY_PROC, mt_props_reset_proc,
		PANEL_LABEL_STRING, gettext("Reset"),
		XV_Y, xv_row(panel, i),
		XV_SHOW,	TRUE,
		XV_HELP_DATA,   "mailtool:PropsReset",
		0);

	ds_center_items(panel, i, apply_item, reset_item, 0);
}


mt_set_props_values()

{

	/* Read our environment, and save the values 
	   into the structure */
	/* This is noop for mt_template_props, mt_alias_props,
	   since they are blank */

	mt_set_props_group_values(mt_header_props);
	mt_set_props_group_values(mt_message_props);
	mt_set_props_group_values(mt_compose_props);
	mt_set_props_group_values(mt_filing_props);
	mt_set_props_group_values(mt_template_props);
	mt_set_props_group_values(mt_alias_props);
	mt_set_props_group_values(mt_expert_props);
}

/* This has no effects on MT_MANUAL_CREATE types 
 * since PROPS_ENTRY.labels are empty strings 
 */

mt_set_props_group_values(group)

PROPS_ENTRY	*group;

{
	char	*s;
	int     num;

	while (group->label)
	{
		if ((s = mt_value(group->label)) == NULL)
			s = group->default_value;

		if ((group->type == MT_NUMBER) ||
		    (group->type == MT_TEXT))
		{
			if (group->current_value)
				free(group->current_value);

			if (s)
			{
				group->current_value = (char *)strdup(s);
			} else {
				group->current_value = s;
			}
		} else {
			group->current_value = s;
		}

		group++;
	}
}


props_done_proc(frame)

	Frame 	frame;
{
	static int old_value;
	int notice_val;
	char *str;

	if (change)
	{

		/* STRING_EXTRACTION -
		 *
		 * The user has changed something in a property sheet,
		 * but not applied the changes and then tried to exit
		 * the property sheet.  The three choices are to do an
		 * apply (and quit), to discard the changes (and quit),
		 * or to abort the quit (which is the "cancel"
		 * choice).
		 */

		/* If scroll list txtfield is changed, bring up 
		 * the "scrolling list" notice, 
		 * set from_category_proc to TRUE so apply won't
		 * bring up the notice again
		 */
		if (sl_txtfld_change) {
		  /* display notice */
		  notice_val = mt_vs_confirm3(mt_props_frame, 0,
			gettext("Apply Changes"),
			gettext("Discard Changes"),
			gettext("Cancel"),
		        gettext(
			"Changes to this category have not been applied."));
		  switch (notice_val) {
		  case 1:
		  	from_category_proc = TRUE;
			/* Must first add txtfld into scroll list */
			add_proc(NULL,NULL);
			if (mt_props_apply_proc() == XV_ERROR)
			{
				return;
			}
		  	from_category_proc = FALSE;
			break;
		  case 2:
			mt_props_reset_proc();
			break;

		  default:
			/* Cancel, so don't dismiss props sheet */
			/* xview won't dismiss it, so do nothing 
			 * although pushpin is now unpinned
			 */
/*
			xv_set(frame, FRAME_CMD_PUSHPIN_IN, TRUE, NULL);
*/
			return;
	  	  }
		} else {
		  str = gettext("Changes to this category have not been applied.");
		  /* display notice */
		  notice_val = mt_vs_confirm3(mt_props_frame, 0,
			gettext("Apply Changes"),
			gettext("Discard Changes"),
			gettext("Cancel"),
			str);

		  switch (notice_val) {
		  case 1:
			if (mt_props_apply_proc() == XV_ERROR)
			{
				return;
			}
			break;
		  case 2:
			mt_props_reset_proc();
			break;

		  default:
			/* Cancel, so don't dismiss props sheet */
			/* xview won't dismiss it, so do nothing 
			 * although pushpin is now unpinned
			 */
/*
			xv_set(frame, FRAME_CMD_PUSHPIN_IN, TRUE, NULL);
*/
			return;
		  }
		}
	}
  
	change = FALSE;
	sl_txtfld_change = FALSE;
	/* If get here, either user hit Apply or reset,
	 * so dismiss props sheet
	 */
	xv_set(frame, XV_SHOW, FALSE, NULL);
}

category_proc(item, value, event)

	Panel_item item;
	int value;
	Event *event;
{
	static int old_value = 0;
	static Panel old_panel = 0;
	static PROPS_ENTRY * old_props = mt_header_props;
	int notice_val;
	char *str;

        Panel           panel;
	PROPS_ENTRY * props;
        int         height;
        int         width;

	if (value == old_value)
		return;
 	if (old_panel == 0)
		old_panel = mt_props_panel1;
	if (change)
	{
		xv_set(item, PANEL_VALUE, old_value, 0);

		/* STRING_EXTRACTION -
		 *
		 * The user has changed something in a property sheet,
		 * but not applied the changes and then tried to exit
		 * the property sheet.  The three choices are to do an
		 * apply (and quit), to discard the changes (and quit),
		 * or to abort the quit (which is the "cancel"
		 * choice).
		 */

		/* If scroll list txtfield is changed, bring up 
		 * the "scrolling list" notice, 
		 * set from_category_proc to TRUE so apply won't
		 * bring up the notice again
		 */
		if (sl_txtfld_change) {
		  str = gettext("Changes to this category have not been applied.");
		  /* display notice */
		  notice_val = mt_vs_confirm3(mt_props_frame, 0,
			gettext("Apply Changes"),
			gettext("Discard Changes"),
			gettext("Cancel"),
			str);
		  switch (notice_val) {
		  case 1:
		  	from_category_proc = TRUE;
			/* Must first add txtfld into scroll list */
			add_proc(NULL,NULL);
			if (mt_props_apply_proc() == XV_ERROR)
			{
				return;
			}
			xv_set(mt_category_item, PANEL_VALUE, value, 0);
		  	from_category_proc = FALSE;
			break;
		  case 2:
			mt_props_reset_proc();
			xv_set(item, PANEL_VALUE, value, 0);
			break;

		  default:
			return;
	  	  }
		} else {
		  str = gettext("Changes to this category have not been applied.");
		  /* display notice */
		  notice_val = mt_vs_confirm3(mt_props_frame, 0,
			gettext("Apply Changes"),
			gettext("Discard Changes"),
			gettext("Cancel"),
			str);

		  switch (notice_val) {
		  case 1:
			if (mt_props_apply_proc() == XV_ERROR)
			{
				return;
			}
			xv_set(mt_category_item, PANEL_VALUE, value, 0);
			break;
		  case 2:
			mt_props_reset_proc();
			xv_set(item, PANEL_VALUE, value, 0);
			break;

		  default:
			return;
		  }
		}
	}

	mt_show_props(old_panel, old_props, FALSE);

	panel = mt_get_current_panel();
	props = mt_get_current_props();

	if (old_props != props) {
	   /* 
   	    * We get width, height before showing it,
   	    * because once shown the panel will expand
   	    * (WIN_EXTEND) to fit the frame,
   	    * so we then reset it back to original size
   	    */
       	   height = (int) xv_get(panel, XV_HEIGHT);
       	   width = (int) xv_get(panel, XV_WIDTH);
   	   mt_show_props(panel, props, TRUE);
       	   xv_set(panel, 
   	   	   XV_HEIGHT, height,
              	   XV_WIDTH, width,
              	   NULL);
   
   	   /*
   	    * Resize width of top panel since bottom panels
   	    * have different sizes
   	    */
       	   width = width + 2 * BORDER_WIDTH;
       	   xv_set(mt_props_panel_top,
              	   XV_WIDTH, width,
              	   NULL);
       	   height = (int) xv_get(mt_props_panel_top, XV_HEIGHT) +
           	   	height + 2 * BORDER_WIDTH;
       	   xv_set(mt_props_frame,
              	   XV_HEIGHT, height,
              	   XV_WIDTH, width,
              	   NULL);
	}

	old_value = value;
	old_panel = panel;
	old_props = props;
	change = FALSE;
	sl_txtfld_change = FALSE;
}

mt_show_props(panel, props_p, show)

	Panel		panel;
	PROPS_ENTRY	*props_p;
	int		show;

{
	register PROPS_ENTRY	*p;
    	int         height;
    	int         width;

	xv_set(panel, XV_SHOW, show, 0);

	p = props_p;
	while (p->label != NULL)
	{
		if (p->item != NULL)
			xv_set(p->item, XV_SHOW, show, 0);
		if (p->itemtraillabel != NULL)
			xv_set(p->itemtraillabel,
				XV_SHOW, show, 0);
		p++;
	}

	if (props_p == mt_message_props) {

		/* Special case for hide header fields list box code */
		mt_show_header_fields(show, mt_message_propslist);

	} else if (props_p == mt_compose_props) {

		/* Special case for custom header fields list box code */
		mt_show_header_fields(show, mt_compose_propslist);

	} else if (props_p == mt_filing_props) {

		/* Special case for file menu list box code */
		mt_show_header_fields(show, mt_filing_propslist);

	} else if (props_p == mt_template_props) {

		/* Special case for template fields list box code */
		mt_show_header_fields(show, mt_template_propslist);

	} else if (props_p == mt_alias_props) {

		/* Special case for alias list box code */
		mt_show_header_fields(show, mt_alias_propslist);

	}
}


mt_show_header_fields(show, propslist)

	short	show;
        PROPS_LIST_BOX *propslist;

{
	xv_set(propslist->add, XV_SHOW, show, 0);
	xv_set(propslist->delete, XV_SHOW, show, 0);
	xv_set(propslist->change, XV_SHOW, show, 0);

	if (propslist->text_field2)
	    xv_set(propslist->text_field2, XV_SHOW, show, 0);
	xv_set(propslist->text_field1, XV_SHOW, show, 0);
	xv_set(propslist->scroll_list, XV_SHOW, show, 0);

	/* Make sure caret is on the first text field */
	if (show) {
		xv_set(mt_get_current_panel(), PANEL_CARET_ITEM,
			propslist->text_field1, 0);
	}
}


PROPS_ENTRY	*
mt_get_current_props()
{
	PROPS_ENTRY	*p;

	switch((int)xv_get(mt_category_item, PANEL_VALUE))
	{
	case 0:
		p = mt_header_props;
		break;
	case 1:
		p = mt_message_props;
		break;
	case 2:
		p = mt_compose_props;
		break;
	case 3:
		p = mt_filing_props;
		break;
	case 4:
		p = mt_template_props;
		break;
	case 5:
		p = mt_alias_props;
		break;
	case 6:
		p = mt_expert_props;
		break;
	default:
		p = mt_header_props;
		break;
	}
	return(p);
}


Panel
mt_get_current_panel()
{
	Panel	p;

	switch((int)xv_get(mt_category_item, PANEL_VALUE))
	{
	case 5:
		p = mt_props_panel2;
		break;
	default:
		p = mt_props_panel1;
		break;
	}
	return(p);
}


int
report_change(item, event)

	Panel_item	item;
	Event		*event;

{
	switch(event_action(event)) {
	 case '\n':
	 case '\t':
	 case '\r':
		break;
	 default:
		change = TRUE;
		break;
	}
 	return(panel_text_notify(item, event));
}


int
report_change1(item, value, event)

	Panel_item	item;
	int		value;
	Event		*event;

{
	PROPS_ENTRY	*p;

	DP(("report_change1: item %x, value %d\n", item, value))
	p = (PROPS_ENTRY *)xv_get(item, XV_KEY_DATA, PROPS_ENTRY_KEY);
	DP(("report_change1: entry %x, label %s\n", p, p?p->label:"<null>"));


	switch(event_action(event)) {
	case '\n':
	case '\t':
	case '\r':
		break;
	default:
		change = TRUE;

		if (strcmp(p->label, "ttlock") == 0) {
			DP(("report_change1: setting expert_ttlock_change\n"));
			expert_ttlock_change = TRUE;
		}
		break;
	}
 	return(panel_text_notify(item, event));
}

/* 
 * This is for record variable's textfield only,
 * we want to make active and inactive the dontlogmessages checkbox
 * based on what happens to this textfield
 */
int
report_change2(item, event)

	Panel_item	item;
	Event		*event;

{
	static int 	set_active = FALSE;
	Panel_item	logall_item = 0;
	char		*string;

	switch(event_action(event)) {
	 case '\n':
	 case '\t':
	 case '\r':
		break;
	 default:
		change = TRUE;

		if (!logall_item)
			logall_item = xv_get(item,
				XV_KEY_DATA,
				KEY_DONTLOGMESSAGES);
		/*
		 * Check if textfield is blank, 
		 */
		string = (char *)xv_get(item, PANEL_VALUE);
		if (!string || (strcmp(string, "")==0)) {
		    if (set_active) {
			set_active = FALSE;
		    	xv_set(logall_item,
				PANEL_INACTIVE, TRUE, 
			   0);
		    }
		} else {
		/* 
		 * User hits a character, so turn on dontlogmessages
		 * checkbox
		 */
		    set_active = TRUE;
		    xv_set(logall_item,
				PANEL_INACTIVE, FALSE, 
			   0);
		}
		break;
	}
 	return(panel_text_notify(item, event));
}


int
report_custom_buttons_txtfld_change(item, event)

	Panel_item	item;
	Event		*event;

{
	int 	j;
	char   *s;

	switch(event_action(event)) {
	 case '\t':
	      break;
	 case '\n':
	 case '\r':
	      break;
	 default:
	      change = TRUE;
	      custom_buttons_txtfld_change = TRUE;
	      /* If return is hit remember it */
	      if (custom_buttons_txtfld_change == TRUE) {

  	        s = (char *) xv_get(custom_buttons_textfield->item, 
	 	 		    PANEL_VALUE);

	        if (prog_buttons2[last_commands_value].external_current)
	         free(prog_buttons2[last_commands_value].external_current);

	        prog_buttons2[last_commands_value].external_current
  		 = (char *)strdup((char *)xv_get(custom_buttons_textfield->item,
				  PANEL_VALUE));

	        for (j = 0; j < PROG_BUTTONS1_NUMBER; j++) {
		 if (strcmp(prog_buttons2[last_commands_value].internal,
		           prog_buttons1[j].internal) == 0) {

	    	    prog_buttons1[j].external_current
	      	      = prog_buttons2[last_commands_value].external_current;
	    	    xv_set(custom_buttons->item, 
		           PANEL_CHOICE_STRING, j, 
	                   prog_buttons1[j].external_current,
			   0);
		 }
	        }

	        custom_buttons_txtfld_change = FALSE;
	      }
	      break;
	}
 	return(panel_text_notify(item, event));
}


int
report_sl_txtfld_change(item, event)

	Panel_item	item;
	Event		*event;

{
	switch(event_action(event)) {
	 case '\n':
	 case '\t':
	 case '\r':
		break;
	 default:
		change = TRUE;
		sl_txtfld_change = TRUE;
	}
 	return(panel_text_notify(item, event));
}


int  
mt_props_reset_proc()

{
	PROPS_ENTRY	*props_p;
	char		*value;

	props_p = mt_get_current_props();

	if (props_p == mt_message_props)
	{
		/* 
		 * Case of no ignore set has already been checked
		 * by either startup or in apply
		 */
		list_flush(mt_message_propslist->scroll_list);
		ignore_enumerate(load_ignore_header_proc, 
			mt_message_propslist->scroll_list);
                xv_set(mt_message_propslist->text_field1, 
			PANEL_VALUE, "", 0);
	}
	else if (props_p == mt_compose_props)
	{
		list_flush(mt_compose_propslist->scroll_list);
		load_header_fields(mt_compose_propslist->scroll_list,
					mt_value("additionalfields"));
                xv_set(mt_compose_propslist->text_field1, 
			PANEL_VALUE, "", 0);
                xv_set(mt_compose_propslist->text_field2, 
			PANEL_VALUE, "", 0);
	} 
	else if (props_p == mt_filing_props)
	{
		list_flush(mt_filing_propslist->scroll_list);
		load_header_fields(mt_filing_propslist->scroll_list,
					mt_value("filemenu2"));
                xv_set(mt_filing_propslist->text_field1, 
			PANEL_VALUE, "", 0);
	} 
	else if (props_p == mt_template_props) 
	{
		if ((value = mt_value("templates")) == NULL)
			value = "calendar:$OPENWINHOME/share/xnews/client/templates/calendar.tpl";
		list_flush(mt_template_propslist->scroll_list);
		load_header_fields(mt_template_propslist->scroll_list,
			value);
                xv_set(mt_template_propslist->text_field1, 
			PANEL_VALUE, "", 0);
                xv_set(mt_template_propslist->text_field2, 
			PANEL_VALUE, "", 0);
	}
	else if (props_p == mt_alias_props) 
	{
		int active;

		/* save off active stat and de-activate to
		 * eliminate flashing and speed performance
		 */

		active = (int)xv_get(mt_alias_propslist->scroll_list, XV_SHOW);
		xv_set(mt_alias_propslist->scroll_list, XV_SHOW, FALSE, 0);

		list_flush(mt_alias_propslist->scroll_list);
		/* make sure text_field2 stores at least 1024 bytes, and
		   as long as the longest one if that's bigger */
		longest_alias_value = 1024;
		alias_enumerate(load_alias_header_proc, 
			mt_alias_propslist->scroll_list);

		if (active) {
			xv_set(mt_alias_propslist->scroll_list, XV_SHOW,
				active, 0);
		}
		xv_set (mt_alias_propslist->text_field2,
			PANEL_VALUE_STORED_LENGTH, longest_alias_value,
			0);
                xv_set(mt_alias_propslist->text_field1, 
			PANEL_VALUE, "", 0);
                xv_set(mt_alias_propslist->text_field2, 
			PANEL_VALUE, "", 0);
	}

	/*
	 * Reset back to the values from the RC file
	 * remember, this has no effect on types MT_MANUAL_CREATE
	 */
	mt_set_props_group_values(props_p);

	while (props_p->label)
	{
	        if (props_p->type == MT_MANUAL_CREATE) {

	        	if ( props_p->manual_reset_proc != NULL)
                           (*(props_p->manual_reset_proc))(props_p);

		} else if (props_p->type == MT_NUMBER) {

		        xv_set(props_p->item, PANEL_VALUE,
			       atoi(props_p->current_value), 0);

		} else if (props_p->type == MT_TOGGLE) {

			xv_set(props_p->item, PANEL_VALUE,
				props_p->current_value == NULL ? 1 : 0, 0);

		} else if (props_p->type == MT_CHECK_BOX) {

			xv_set(props_p->item, PANEL_VALUE,
				props_p->current_value, 0);

		} else if (props_p->type == MT_INV_CHECK_BOX) {

			xv_set(props_p->item, PANEL_VALUE,
				props_p->current_value == NULL ? 1 : 0, 0);

		} else {/* Including MT_INV_TOGGLE */
		        /* Including MT_TEXT */

			xv_set(props_p->item, PANEL_VALUE,
				props_p->current_value, 0);
		}
		props_p++;
	}

	change = FALSE;
	sl_txtfld_change = FALSE;
}


static void
list_flush(list)
     Panel_item list;

{
  
  int row;

  row = (int)xv_get(list, PANEL_LIST_NROWS);
  if (row == 0)
    return;
  
  for ( ; row > 0; row--)
    {
      xv_set (list, PANEL_LIST_DELETE, row -1, 0);
    }
}


static
load_header_fields(list, value_buffer)

	Panel_item	list;
	char	*value_buffer;

{
	int    row;

	char	*name;
	char	*value;

	/*
	 * Parse buffer and stuff field names into list
	 */
	if (value_buffer == NULL || *value_buffer == '\0')
		return;


	row = 0;
	while (parse_external_string(&value_buffer, &name, &value)) {
		/* check to see if there is client data */
		if (*value)
		{
			name = strdup(name);
			value = strdup(value);

			if (! name || !value) break;

			xv_set(list, 
				PANEL_LIST_INSERT, row,
				PANEL_LIST_STRING, row, name,
				PANEL_LIST_CLIENT_DATA, row, value,
				0);
		}
		else
		{
			name = strdup(name);

			if (! name) break;

			xv_set(list,
				PANEL_LIST_INSERT, row,
				PANEL_LIST_STRING, row, name,
				PANEL_LIST_CLIENT_DATA, row, NULL,
				0);
		}
		row++;
	}
}


int
mt_props_apply_proc()

{
	FILE	*mailrc = NULL;
	FILE	*real_mailrc = NULL;
	short	fromscratch = FALSE;
	short	writing = TRUE;
	char	tmp_filename[30];
	char	mailrc_buffer[1024];
	char	mailrc_name[1024];
	char	full_path[1024];
	register PROPS_ENTRY	*p;
	char	s0[20];
	char	*s;
	int     num;
        int 	notice_val;
        char 	*str;
	struct stat buf;
        struct reply_panel_data *ptr;
        struct header_data      *hd;


        hd = mt_get_header_data(mt_frame);
	strcpy(mailrc_name, Getf("MAILRC"));

	/* check to see if there is a .mailrc file 
	 * in the home directory.  
	 * If not, then we are free to create one 
	 */

	if (access(mailrc_name, F_OK) == 0)
	{
		if (access(mailrc_name, W_OK) == 0)
		{
		  /* extra check - want to see if it's a regular 
		   * file (no dirs)
		   */
		  if ((stat(mailrc_name, &buf) == 0) 
	   	     && (S_ISREG(buf.st_mode))) {
			strcpy(tmp_filename, "/tmp/TMPXXXXXX");
			mktemp(tmp_filename);
			mailrc = fopen(tmp_filename, "w+");
		  }
		}
	}
     	else
     	{
		fromscratch = TRUE;
		mailrc = fopen(mailrc_name, "w+");
    	}

	if (!mailrc)
	{

                /* STRING_EXTRACTION -
                 *
                 * We are trying to write out the .mailrc file when
                 * the user has hit "apply" from the mail property
                 * sheet, but the open failed.
                 */
		mt_vs_warn(mt_props_frame,
	          gettext("Unable to write into your file:\n%s\nPlease make sure this is a writable file"), mailrc_name);
		return(0);
	}

	/* place the new values in the .mailrc file */

	/*
	 * sl_txtfld_change means user touched scroll list textfield
	 * and did not yet insert it in (it means we have to add
	 * before we can do any apply).
	 *
	 * if sl_txtfld_change is T, change is T
	 * but if change is T, sl_txtfld_change can be F
	 * (user has added, but not yet applied).
	 * both could happen on one prop sheet, 
	 */

	/* STRING_EXTRACTION -
	 *
	 * The user has changed something in the textfield related
         * to the scrolling list,
	 * but has not yet added the changes to the scrolling list 
         * itself, and now the user has tried to exit
	 * the property sheet.  The three choices are to do an
	 * add (and quit), to discard the changes (and quit),
	 * or to abort the quit (which is the "cancel"
	 * choice).
	 */
	/* STRING_EXTRACTION -
	 *
	 * For the Mail File Directory textfield in the Mail Filing
	 * property sheet, user used a '~' or '$', without fully expanding
	 * their path; since /usr/ucb/mail does not
	 * recognize this and will give an error, 
	 * we are asking the user whether they wish to
	 * store the path as expanded or not.
	 */

	if (!from_category_proc && sl_txtfld_change) {
		str = gettext("You have not added the text item to the scrolling list. You can:");
		/* display notice */
		notice_val = mt_vs_confirm3(mt_props_frame, 0,
			gettext("Add Item"),
			gettext("Ignore Item"),
			gettext("Cancel"),
			str);

		switch (notice_val) {
		case 1:
			/* Must first add txtfld into scroll list */
			add_proc(NULL,NULL);
			break;
		case 2:
			break;

		default:
			return(0);
		}
	}

	if (expert_ttlock_change) {
		struct header_data *hd;
		int need_to_write = 0;
		int result;

		for (hd = mt_header_data_list; hd != NULL; hd = hd->hd_next) {
			if (CURRENT_FOLDER(hd) != NULL) {
				need_to_write = 1;
				break;
			}
		}

		if (need_to_write) {
			result = mt_vs_confirm3(mt_props_frame, 1,
				gettext("Save and reopen now"),
				gettext("Continue, apply on next open"),
				gettext("Abort, return to property sheet"),
				gettext(
"You have changed the network locking property, but\n\
you currently have open mail folders.  This property\n\
only takes effect when a folder is first opened.\n\
\n\
If you want, all your folders can be saved and reopened\n\
for you now, committing all edits and saving changes.\n\
\n\
Or, you can continue with mailtool and the locking\n\
will be changed the next time you open a folder.\n\
\n\
Or you can go back to editing the property sheet."));
			DP(("confirm returned %d\n", result));

			if (result == NOTICE_NO) {
				/* return to editing */
				return (0);
			} else if(result == NOTICE_YES) {
				/* close and reopen the folders */
				hd = mt_header_data_list;
				while (hd) {
					mt_commit_proc_hd(hd);
					hd = hd->hd_next;
				}
			} else {
				ASSERT(result == 2);	/* take effect later */
			}


		}
	}

	/* save the current values for the items on the screen 
	 * here is place to check validity of things for 
	 * ONLY current prop sheet,
	 * later, write_values will write for ALL sheets
	 */
	p = mt_get_current_props();
	while (p->label)
	{
		if (p->type == MT_NUMBER) {

			/* PANEL_VALUE of a NUMERIC_TEXT is an int */
			num = (int)xv_get(p->item, PANEL_VALUE);
			/* Convert to a string, then malloc */
			sprintf(s0, "%d", num);
			s = strdup(s0);
			if (p->current_value)
				free(p->current_value);
			p->current_value = s;

		} else if (p->type == MT_TEXT) {

			s = (char *) xv_get(p->item, PANEL_VALUE);
			/* We don't strip indentprefix txtfld */
			if (strcmp(p->label, "indentprefix")) {
				s = mt_strip_leading_blanks(s);
				xv_set(p->item, PANEL_VALUE, s, 0);
				s = (char *) xv_get(p->item, PANEL_VALUE);
			}
			if (s)
				s = (char *)strdup(s);
			if (p->current_value)
				free(p->current_value);
			p->current_value = s;

			/*
			 * Checks for record is done in write_values
			 */
			/*
			 * Make sure the mail file directory exists. This will
			 * create it if the user wants to.
			 * If ~ or $ is used, ask user if they want
			 * expanded string.
			 */
			if (strcmp(p->label, "folder") == 0) {
			   check_mail_file_directory(p->current_value, full_path);
			   if (*p->current_value != '/') {
			      if ((strchr(p->current_value, '~')) 
			         || (strchr(p->current_value, '$'))) {

			         str = gettext("Warning: The \'~\' or \'$\' string you specified in your Mail File Directory,\nif saved as is and not expanded into a full pathname,\nwill not work with your /usr/bin/mail program.\nDo you wish to:");
		  		 /* display notice */
		  		 notice_val = mt_vs_confirm(mt_props_frame,
				   0,
				   gettext("Expand String"),
				   gettext("Leave As Is"),
				   str);

				 switch (notice_val) {
		  		 case 1:
                                   s = (char *)strdup(full_path);
                                   if (p->current_value)
                                        free(p->current_value);
                                   p->current_value = s;
				   xv_set(p->item, PANEL_VALUE, s, 0);
				   break;
		                 case 2:
				   break;
		                 default:
				   break;
				 }
			      }
			   }
			}

		} else if (p->type == MT_TOGGLE) {

			s = (char *) xv_get(p->item, PANEL_VALUE);
			p->current_value = (char *)(!(int)s);

		} else if (p->type == MT_CHECK_BOX) {

			s = (char *) xv_get(p->item, PANEL_VALUE);
			p->current_value = s;

		} else if (p->type == MT_INV_CHECK_BOX) {

			s = (char *) xv_get(p->item, PANEL_VALUE);
			p->current_value = (char *)(!(int)s);

		} else {
			s = (char *) xv_get(p->item, PANEL_VALUE);
			/* MT_INV_TOGGLE */
			p->current_value = s;
		}

		p++;
	}


	/* if there was an old .mailrc file, look for any lines 
	   deposited there by previous props commands, and remove 
	   them.  Then copy in the user added comments.  After 
	   this, copy the new file to the regular .mailrc 
	   filename */

	if (!fromscratch)
	{
		real_mailrc = fopen(mailrc_name, "r");
		while(fgets(mailrc_buffer, 256, real_mailrc))
		{
			if (writing && strncmp(mailrc_buffer,
				"# mailtool defaults from prop", 29) == 0)
					writing = FALSE;
			else if (!writing && strncmp(mailrc_buffer,
				"# end mailtool defaults from prop", 33) == 0)
					writing = TRUE;
			else if (writing)
				fputs(mailrc_buffer, mailrc);
		}

		fclose(real_mailrc);

		unlink(mailrc_name);

		real_mailrc = fopen(mailrc_name, "w");
		rewind(mailrc);
		while(fgets(mailrc_buffer, 256, mailrc))
			fputs(mailrc_buffer, real_mailrc);

		fclose(mailrc);
		unlink(tmp_filename);

	}
	else {
		real_mailrc = mailrc;
	}

        /* STRING_EXTRACTION -
         *
         * We warn the user in .mailrc file to make all their changes
	 * above this line and not touch anything below this line
	 */
	fprintf(real_mailrc, "# mailtool defaults from prop sheet\n");
	fprintf(real_mailrc, "# %s\n", gettext("PLEASE MAKE ALL YOUR CHANGES ABOVE THESE 2 LINES"));
	write_values(real_mailrc, mt_header_props);
	write_values(real_mailrc, mt_message_props);
	write_values(real_mailrc, mt_compose_props);
	write_values(real_mailrc, mt_filing_props);
	write_values(real_mailrc, mt_template_props);
	write_values(real_mailrc, mt_alias_props);
	write_values(real_mailrc, mt_expert_props);

	fprintf(real_mailrc, "# end mailtool defaults from prop sheet\n");
	fclose(real_mailrc);

	/*
	 * Set ignore_free, we automatically had set alias_free via
	 * the call #-clearaliases at startup and at props reset.
	 */
	ignore_exists = FALSE;
	ignore_free();

	/* mt_init_mailtool_defaults 
	 * calls load() which calls commands() which
	 * reads in .mailrc again and sets each variable FOUND
	 * into internal memory/ hash table
	 * (probably calls mt_assign)
	 */
	mt_start_timer();
	mt_start_timer();	/* in case user changed interval */
	mt_init_mailtool_defaults();


	/* Read our environment, and save the values 
	   into the structure */

	mt_set_props_values();
	change = FALSE;
	sl_txtfld_change = FALSE;
	expert_ttlock_change = FALSE;

	/* Dynamic changes to Move/Copy/Load Menus */
	mt_props_dynamic_update_menus();

	/* Dynamic changes to Mail Files popup */
	mt_props_dynamic_update_mailfiles_popup();

	/* Dynamic updates of log checkbox to compose windows */
	if (old_record_exists) {
	   if (!mt_value("record")) {
	   	old_record_exists = FALSE;
        	ptr = MT_RPD_LIST(hd);
		while (ptr) {
		   if (ptr->rpd_record_item) {
			(void)xv_set(ptr->rpd_record_item, XV_SHOW, FALSE, 0);
		   }
               	   ptr = ptr->next_ptr;
		}
	   }
	} else {
	   if (mt_value("record")) {
		old_record_exists = TRUE;
	   }
	}

	/* handle the case where "retrieveinterval" might have changed */
	mt_start_timer();

	return(XV_OK);
}

static
check_mail_file_directory(path, full_path)
	char *path;
	char *full_path;
{
	int	rcode;

	/*
	 * Convert path to an absolute path
	 */
        ds_expand_pathname(path, full_path);

	/* 
	 * If it is not absolute path by now,
	 * assume variable path is relative to $HOME
	 * and proceed from there, 
	 * first reseting full_path.
	 */
	if (*full_path != '/') {
		strcpy(full_path, getenv("HOME"));
		strcat(full_path, "/");
		strcat(full_path, path);
	}

	/*
	 * Check if path exists.  If it doesn't mt_check_and_create() will
	 * ask the user if they would like it created.
	 */
	rcode = mt_check_and_create(full_path, TRUE, mt_props_frame, TRUE);

        /* STRING_EXTRACTION -
         *
         * We tried to create the mail file directory, but
         * something went wrong.  We give the name of the directory,
         * and then the system error message.
	 * rcode could be < 0 or > 0 (all error cases).
         */
	if (rcode > 0) {
		mt_vs_warn(mt_props_frame,
			gettext("Could not create\n%s\n%s"),
			full_path, strerror(rcode));

	} else if (rcode < 0) {
		mt_vs_warn(mt_props_frame,
			gettext("Could not create\n%s"),
			full_path);
	}
}


int
write_values(mailrc_file, props_p)

FILE		*mailrc_file;
PROPS_ENTRY	*props_p;

{
	register char type;
	register PROPS_ENTRY	*p;


	/*
	 * Write values into the .mailrc file.  If a toggle is turned
	 * off, make sure that we remove it from the cache of .mailrc values
	 */
	p = props_p;
	while (p->label)
	{
		type = p->type;
		
		if (type == MT_TOGGLE || type == MT_INV_TOGGLE )
		{
			if (p->current_value != NULL)
				fprintf(mailrc_file, "set %s\n", p->label);
			else
			{
				fprintf(mailrc_file, "set no%s\n", p->label);
				if (mt_value(p->label) != NULL)
					mt_deassign(p->label);
			}
		}
		else if (type == MT_CHECK_BOX || type == MT_INV_CHECK_BOX )
		{
			if (p->current_value != NULL)
				fprintf(mailrc_file, "set %s\n", p->label);
			else
			{
				fprintf(mailrc_file, "set no%s\n", p->label);
				if (mt_value(p->label) != NULL)
					mt_deassign(p->label);
			}
		}
		else if (type == MT_NUMBER || type == MT_TEXT)
		{
			/* In record variable, if it is not empty and
			 * not just a "+", write it out.
			 * Use "set outfolder" also to be compatible
			 * with /bin/mail.
			 * If it is a ~, write it out, but set nooutfolder.
			 */
			/* Check for *p in case it is "" (empty string)*/
			if (strcmp(p->label, "record") == 0) {
			   if ((p->current_value && *p->current_value)
			      && (strcmp(p->current_value, "+") != 0)) {
				 if (p->current_value[0] == '~')
			           fprintf(mailrc_file, "set nooutfolder\n");
				 else
			           fprintf(mailrc_file, "set outfolder\n");
				 fprintf(mailrc_file, "set %s='%s'\n", 
					p->label, p->current_value);
                           } else {
				 fprintf(mailrc_file, "set no%s\n", p->label);
				 if (mt_value(p->label) != NULL)
					mt_deassign(p->label);

			   }
                        } else {
			   if (p->current_value && *p->current_value)
				fprintf(mailrc_file, "set %s='%s'\n", 
					p->label, p->current_value);
			   else
			   {
			       /* set it to no and to be doubly safe,
			  	* clear internal memory/hash table
			  	* with mt_deassign
			  	*/
				fprintf(mailrc_file, "set no%s\n", p->label);
				if (mt_value(p->label) != NULL)
					mt_deassign(p->label);
		   	   }
			}
		}
		else if (type == MT_MANUAL_CREATE)
		{
			if (p->manual_apply_proc != NULL)
                           (*(p->manual_apply_proc))
					(mailrc_file, p);
		}
		p++;
	}

	/* Now, handle scrolling list cases */
	if (props_p == mt_message_props) {
		write_message_fields(mailrc_file,
			mt_message_propslist->scroll_list);
	} else if (props_p == mt_compose_props) {
		write_header_fields(mailrc_file,
			mt_compose_propslist->scroll_list);
	} else if (props_p == mt_filing_props) {
		write_header_fields(mailrc_file,
			mt_filing_propslist->scroll_list);
	} else if (props_p == mt_template_props) {
		write_header_fields(mailrc_file,
			mt_template_propslist->scroll_list);
	} else if (props_p == mt_alias_props) {
		write_alias_fields(mailrc_file,
			mt_alias_propslist->scroll_list);
	}
}


/*
 * Create cutomized buttons, along with the abbrev button stack
 * from which to choose the commands to put in those buttons
 */

static void
create_custom_buttons(
	PROPS_ENTRY	*p
)
{
	int 	j;

        /* STRING EXTRACTION -
         * Following are hard coded menu items, each of which is
         * taken from Mailtool's first row of buttons and 
         * button stacks. Please refer to create_panels.c
         * for their description (that's where they are created).
         * The fields external_current and external_factory
         * are the menu items, and the field in_menu_only
         * is used to describe the previous two fields to user.
         * The internal field is for internal hardcoded functions
         * so it does not need a gettext.
         */
	/* Initialize with gettext */
	j = 0;
	prog_buttons2[j].external_current = strdup(gettext("Load"));
	prog_buttons2[j].external_factory = gettext("Load");
	prog_buttons2[j].in_menu_only = gettext("Load");
	prog_buttons2[j].internal = "loadInBox";

	j++;
	prog_buttons2[j].external_current = strdup(gettext("Print"));
	prog_buttons2[j].external_factory = gettext("Print");
	prog_buttons2[j].in_menu_only = gettext("Print");
	prog_buttons2[j].internal = "print";

	j++;
	prog_buttons2[j].external_current = strdup(gettext("Save"));
	prog_buttons2[j].external_factory = gettext("Save");
	prog_buttons2[j].in_menu_only = gettext("Save");
	prog_buttons2[j].internal = "saveChanges";

	j++;
	prog_buttons2[j].external_current = strdup(gettext("Done"));
	prog_buttons2[j].external_factory = gettext("Done");
	prog_buttons2[j].in_menu_only = gettext("Done");
	prog_buttons2[j].internal = "done";

	j++;
	prog_buttons2[j].external_current = strdup(gettext("MailFiles"));
	prog_buttons2[j].external_factory = gettext("MailFiles");
	prog_buttons2[j].in_menu_only = gettext("Mail Files");
	prog_buttons2[j].internal = "mailfiles";

	j = 13;
	prog_buttons2[j].external_current = strdup(gettext("Messages"));
	prog_buttons2[j].external_factory = gettext("Messages");
	prog_buttons2[j].in_menu_only = gettext("Messages (menu)");
	prog_buttons2[j].internal = "viewMessages";

	j++;
	prog_buttons2[j].external_current = strdup(gettext("AbbvHdr"));
	prog_buttons2[j].external_factory = gettext("AbbvHdr");
	prog_buttons2[j].in_menu_only = gettext("Abbreviated Header");
	prog_buttons2[j].internal = "abbreviatedHeader";

	j++;
	prog_buttons2[j].external_current = strdup(gettext("FullHdr"));
	prog_buttons2[j].external_factory = gettext("FullHdr");
	prog_buttons2[j].in_menu_only = gettext("Full Header");
	prog_buttons2[j].internal = "fullHeader";

	j++;
	prog_buttons2[j].external_current = strdup(gettext("Previous"));
	prog_buttons2[j].external_factory = gettext("Previous");
	prog_buttons2[j].in_menu_only = gettext("Previous");
	prog_buttons2[j].internal = "previousMessage";

	j++;
	prog_buttons2[j].external_current = strdup(gettext("Next"));
	prog_buttons2[j].external_factory = gettext("Next");
	prog_buttons2[j].in_menu_only = gettext("Next");
	prog_buttons2[j].internal = "nextMessage";

	j++;
	prog_buttons2[j].external_current = strdup(gettext("Sort"));
	prog_buttons2[j].external_factory = gettext("Sort");
	prog_buttons2[j].in_menu_only = gettext("Sort By (menu)");
	prog_buttons2[j].internal = "sortMenu";

	j++;
	prog_buttons2[j].external_current = strdup(gettext("SortTime"));
	prog_buttons2[j].external_factory = gettext("SortTime");
	prog_buttons2[j].in_menu_only = gettext("Sort By Time & Date");
	prog_buttons2[j].internal = "sortByTimeAndDate";

	j++;
	prog_buttons2[j].external_current = strdup(gettext("SortSndr"));
	prog_buttons2[j].external_factory = gettext("SortSndr");
	prog_buttons2[j].in_menu_only = gettext("Sort By Sender");
	prog_buttons2[j].internal = "sortBySender";

	j++;
	prog_buttons2[j].external_current = strdup(gettext("SortSubj"));
	prog_buttons2[j].external_factory = gettext("SortSubj");
	prog_buttons2[j].in_menu_only = gettext("Sort By Subject");
	prog_buttons2[j].internal = "sortBySubject";

	j++;
	prog_buttons2[j].external_current = strdup(gettext("SortSize"));
	prog_buttons2[j].external_factory = gettext("SortSize");
	prog_buttons2[j].in_menu_only = gettext("Sort By Size");
	prog_buttons2[j].internal = "sortBySize";

	j++;
	prog_buttons2[j].external_current = strdup(gettext("SortStatus"));
	prog_buttons2[j].external_factory = gettext("SortStatus");
	prog_buttons2[j].in_menu_only = gettext("Sort By Status");
	prog_buttons2[j].internal = "sortByStatus";

	j++;
	prog_buttons2[j].external_current = strdup(gettext("SortMsgNo"));
	prog_buttons2[j].external_factory = gettext("SortMsgNo");
	prog_buttons2[j].in_menu_only = gettext("Sort By Message Number");
	prog_buttons2[j].internal = "sortByMsgno";

	j++;
	prog_buttons2[j].external_current = strdup(gettext("Find"));
	prog_buttons2[j].external_factory = gettext("Find");
	prog_buttons2[j].in_menu_only = gettext("Find");
	prog_buttons2[j].internal = "findButton";

	j = 26;
	prog_buttons2[j].external_current = strdup(gettext("Cut"));
	prog_buttons2[j].external_factory = gettext("Cut");
	prog_buttons2[j].in_menu_only = gettext("Cut");
	prog_buttons2[j].internal = "cutMessage";

	j++;
	prog_buttons2[j].external_current = strdup(gettext("Copy"));
	prog_buttons2[j].external_factory = gettext("Copy");
	prog_buttons2[j].in_menu_only = gettext("Copy");
	prog_buttons2[j].internal = "copyMessage";

	j++;
	prog_buttons2[j].external_current = strdup(gettext("Delete"));
	prog_buttons2[j].external_factory = gettext("Delete");
	prog_buttons2[j].in_menu_only = gettext("Delete");
	prog_buttons2[j].internal = "deleteMessage";

	j++;
	prog_buttons2[j].external_current = strdup(gettext("Undelete"));
	prog_buttons2[j].external_factory = gettext("Undelete");
	prog_buttons2[j].in_menu_only = gettext("Undelete (menu)");
	prog_buttons2[j].internal = "undeleteMenu";

	j++;
	prog_buttons2[j].external_current = strdup(gettext("Undelete"));
	prog_buttons2[j].external_factory = gettext("Undelete");
	prog_buttons2[j].in_menu_only = gettext("Undelete Last Message");
	prog_buttons2[j].internal = "undeleteLastMessage";

	j++;
	prog_buttons2[j].external_current = strdup(gettext("UndelList"));
	prog_buttons2[j].external_factory = gettext("UndelList");
	prog_buttons2[j].in_menu_only = gettext("Undelete From List");
	prog_buttons2[j].internal = "undeleteFromList";

	j++;
	prog_buttons2[j].external_current = strdup(gettext("Properties"));
	prog_buttons2[j].external_factory = gettext("Properties");
	prog_buttons2[j].in_menu_only = gettext("Properties");
	prog_buttons2[j].internal = "properties";

	j = 39;
	prog_buttons2[j].external_current = strdup(gettext("New"));
	prog_buttons2[j].external_factory = gettext("New");
	prog_buttons2[j].in_menu_only = gettext("New");
	prog_buttons2[j].internal = "composeNew";

	j++;
	prog_buttons2[j].external_current = strdup(gettext("Reply"));
	prog_buttons2[j].external_factory = gettext("Reply");
	prog_buttons2[j].in_menu_only = gettext("Reply (menu)");
	prog_buttons2[j].internal = "replyMenu";

	j++;
	prog_buttons2[j].external_current = strdup(gettext("Reply"));
	prog_buttons2[j].external_factory = gettext("Reply");
	prog_buttons2[j].in_menu_only = gettext("Reply To Sender");
	prog_buttons2[j].internal = "replySender";

	j++;
	prog_buttons2[j].external_current = strdup(gettext("ReplyAll"));
	prog_buttons2[j].external_factory = gettext("ReplyAll");
	prog_buttons2[j].in_menu_only = gettext("Reply To All");
	prog_buttons2[j].internal = "replyAll";

	j++;
	prog_buttons2[j].external_current = strdup(gettext("ReplyInc"));
	prog_buttons2[j].external_factory = gettext("ReplyInc");
	prog_buttons2[j].in_menu_only = gettext("Reply To Sender, Include");
	prog_buttons2[j].internal = "replySenderInclude";

	j++;
	prog_buttons2[j].external_current = strdup(gettext("ReplyAI"));
	prog_buttons2[j].external_factory = gettext("ReplyAI");
	prog_buttons2[j].in_menu_only = gettext("Reply To All, Include");
	prog_buttons2[j].internal = "replyAllInclude";

	j++;
	prog_buttons2[j].external_current = strdup(gettext("Forward"));
	prog_buttons2[j].external_factory = gettext("Forward");
	prog_buttons2[j].in_menu_only = gettext("Forward");
	prog_buttons2[j].internal = "forwardMessage";

	j++;
	prog_buttons2[j].external_current = strdup(gettext("Vacation"));
	prog_buttons2[j].external_factory = gettext("Vacation");
	prog_buttons2[j].in_menu_only = gettext("Vacation (menu)");
	prog_buttons2[j].internal = "vacationMenu";

	p->item = xv_create(mt_props_panel1, PANEL_CHOICE,
		/*PANEL_CHOICE_NROWS, 2,*/
		PANEL_LABEL_STRING, p->lead_label,
		PANEL_LABEL_BOLD, TRUE,
		XV_HELP_DATA,	p->help_data,
		PANEL_NOTIFY_PROC, custom_buttons_proc,
		PANEL_VALUE, 0,
		0);
	custom_buttons = p;
}

void
create_custom_buttons_textfield(
	PROPS_ENTRY	*p
)
{
	p->item = xv_create(mt_props_panel1, PANEL_TEXT,
		PANEL_LABEL_STRING, p->lead_label,
		PANEL_LABEL_BOLD, TRUE,
		PANEL_VALUE_DISPLAY_LENGTH, p->size > 20 ? 20 : p->size,
		PANEL_VALUE_STORED_LENGTH, p->size,
		XV_HELP_DATA,	p->help_data,
		PANEL_NOTIFY_LEVEL, PANEL_ALL,
		PANEL_NOTIFY_PROC, report_custom_buttons_txtfld_change,
		0);
	custom_buttons_textfield = p;
	/* Do this here now because txtfld is created last */
	xv_set(custom_buttons_textfield->item, 
		PANEL_VALUE, prog_buttons1[0].external_current, 0);
}

void
custom_buttons_proc(item, event)

	/* these 2 passed args not used */
	Panel_item	item;
	Event		*event;

{
	int custom_value;
	int j;

	custom_value   = xv_get(custom_buttons->item, PANEL_VALUE);

	for (j = 0; j < PROG_BUTTONS2_NUMBER; j++) {
	    /* check for strcmp NULL's */
    	    if (!prog_buttons1[custom_value].internal ||
	        !prog_buttons2[j].internal) 
		continue;
    	    if (strcmp( prog_buttons1[custom_value].internal,
		        prog_buttons2[j].internal) == 0) {
			 xv_set(custom_buttons_textfield->item, 
				PANEL_VALUE, 
				prog_buttons1[custom_value].external_current, 0);
			 xv_set(commands_choices->item, PANEL_VALUE, j, 0);
			 last_commands_value = j;
			 return;
	    }
	}
}

void
create_commands_choices(p)

	PROPS_ENTRY	*p;

{
	int 	i;
	int 	j;
	int 	remember_to_set = 0;
	Attr_attribute choice_list[PROG_BUTTONS2_NUMBER + 4]; /* might need
								 only 3 */
	Attr_attribute *choice_ptr;
	PROG_BUTTON *but2;

	/* for bug # 1105813: don't call PANEL_CHOICE_STRING if possible */
	choice_ptr = choice_list;
	*choice_ptr++ = PANEL_CHOICE_STRINGS;

	p->item = xv_create(mt_props_panel1, PANEL_CHOICE_STACK,
		PANEL_LAYOUT, PANEL_VERTICAL,
		PANEL_CHOICE_NCOLS, 4,
		PANEL_LABEL_STRING, p->lead_label,
		PANEL_LABEL_BOLD, TRUE,
		XV_HELP_DATA,	p->help_data,
		PANEL_NOTIFY_PROC, commands_choices_proc,
		PANEL_VALUE, 0,
		0);
	commands_choices = p;
	/*
	 * 1) for prog_buttons1, get the 4 user programable buttons,
	 *    (prog_buttons1 is read-only, so no need to strdup)
	 * 2) for prog_buttons2, set the current value,
 	 * 3) create the abbrev button stack
	 */
  	get_button_at_position(1, 0, &prog_buttons1[0].external_current, 
				     &prog_buttons1[0].internal);
  	get_button_at_position(1, 1, &prog_buttons1[1].external_current, 
				     &prog_buttons1[1].internal);
  	get_button_at_position(1, 2, &prog_buttons1[2].external_current, 
				     &prog_buttons1[2].internal);
  	get_button_at_position(1, 3, &prog_buttons1[3].external_current, 
				     &prog_buttons1[3].internal);

	for (j = 0, but2 = prog_buttons2; j < PROG_BUTTONS2_NUMBER; but2++, j++)
	{

	    /* 
	     * if it is not a padding item,
	     * see if user has already defined it, else
	     * set to factory value
	     */
	    if (but2->external_factory != NULL) {
		/* Clear it up just in case */
	        if (but2->external_current != NULL) {
	            free (but2->external_current);
	            but2->external_current = NULL;
		}

		for (i = 0; i < PROG_BUTTONS1_NUMBER; i++) {
		    /* check for strcmp NULL's */
	    	    if (!prog_buttons1[i].internal ||
		        !but2->internal) 
			continue;
	    	    if (strcmp( prog_buttons1[i].internal,
		        	but2->internal) == 0) {
			/* Check again in case user has multiple
			   values for same thing */
	        	if (but2->external_current != NULL) {
	            	   free(but2->external_current);
	            	   but2->external_current = NULL;
			}
		        but2->external_current = (char *)
		           strdup(prog_buttons1[i].external_current);
			if (i == 0) 
				remember_to_set = j;
		        break;
	    	    }
	    	}
	        if (but2->external_current == NULL) {
		    but2->external_current = (char *)
			strdup(but2->external_factory);
	    	}
	    }

	    /* Set them all, even if it is a padding item */
	    xv_set(p->item, PANEL_CHOICE_STRING,
	           j, but2->in_menu_only, 0);
	    
	    *choice_ptr = (Attr_attribute) but2->in_menu_only;
	    if (*choice_ptr == NULL) {
		*choice_ptr = (Attr_attribute) "";
	    }
	    choice_ptr++;
	}

	/* terminate the list of panel choice strings */
	*choice_ptr = NULL;
	/* add an extra NULL to terminate the ATTR_LIST attribute */
	choice_ptr++;
	*choice_ptr = NULL;
	xv_set(p->item, ATTR_LIST, choice_list, NULL);

	/* Now populate the previous panel item, the custom buttons */
	for (i = 0; i < PROG_BUTTONS1_NUMBER; i++) {
	    xv_set(custom_buttons->item, PANEL_CHOICE_STRING,
	           i, prog_buttons1[i].external_current, 0);
   	}
	/* Now initially set commands choices stack 
	   to value of first custom button */
	xv_set(commands_choices->item, PANEL_VALUE, remember_to_set, 0);
	last_commands_value = remember_to_set;
}

void
commands_choices_proc(item, event)

	/* these 2 passed args not used */
	Panel_item	item;
	Event		*event;

{
	int custom_value;
	int commands_value;

	change = TRUE;
	custom_value   = xv_get(custom_buttons->item, PANEL_VALUE);
	commands_value = xv_get(commands_choices->item, PANEL_VALUE);
	/* if a padding item, ignore it, set it back to previous */
	if (prog_buttons2[commands_value].internal == NULL) {
		xv_set(commands_choices->item, PANEL_VALUE, 
			last_commands_value, 0);
		return;
	}
	last_commands_value = commands_value;

	/* 
	 * update prog_buttons1[] so that it always reflects current value 
	 * remember, it is read-only, nothing ever malloc'd
	 */
	prog_buttons1[custom_value].external_current =
	       prog_buttons2[commands_value].external_current;
	prog_buttons1[custom_value].internal =
	       prog_buttons2[commands_value].internal;
	xv_set(custom_buttons->item, PANEL_CHOICE_STRING,
  	       custom_value, 
	       prog_buttons1[custom_value].external_current, 0);
	xv_set(custom_buttons_textfield->item, 
	       PANEL_VALUE, 
	       prog_buttons1[custom_value].external_current, 0);
	/* need to do smarter updating width */
	window_fit_width(mt_props_panel1);
}

/* This resets commands_buttons also */
void
reset_custom_buttons(p)

	PROPS_ENTRY	*p;

{
	int 	j;
	int 	i;
	int 	remember_to_set;

  	get_button_at_position(1, 0, &prog_buttons1[0].external_current, 
				     &prog_buttons1[0].internal);
  	get_button_at_position(1, 1, &prog_buttons1[1].external_current, 
				     &prog_buttons1[1].internal);
  	get_button_at_position(1, 2, &prog_buttons1[2].external_current, 
				     &prog_buttons1[2].internal);
  	get_button_at_position(1, 3, &prog_buttons1[3].external_current, 
				     &prog_buttons1[3].internal);

	for (j = 0; j < PROG_BUTTONS2_NUMBER; j++) {

	    /* 
	     * if it is not a padding item,
	     * see if user has already defined it, else
	     * set to factory value
	     */
	    if (prog_buttons2[j].external_factory != NULL) {
		/* Clear it up just in case */
	        if (prog_buttons2[j].external_current != NULL) {
	            free (prog_buttons2[j].external_current);
		    prog_buttons2[j].external_current = NULL;
		}

		for (i = 0; i < PROG_BUTTONS1_NUMBER; i++) {
		    /* check for strcmp NULL's */
	    	    if (!prog_buttons1[i].internal ||
		        !prog_buttons2[j].internal) 
			continue;
	    	    if (strcmp( prog_buttons1[i].internal,
		        	prog_buttons2[j].internal) == 0) {
			/* Check this again in case user has multiple 
			   values for same same */
	        	if (prog_buttons2[j].external_current != NULL) {
	            	   free(prog_buttons2[j].external_current);
	            	   prog_buttons2[j].external_current = NULL;
			}
		        prog_buttons2[j].external_current = (char *)
		           strdup(prog_buttons1[i].external_current);
			if (i == 0) 
				remember_to_set = j;
		        break;
	    	    }
	    	}
	        if (prog_buttons2[j].external_current == NULL) {
		    prog_buttons2[j].external_current = (char *)
			strdup(prog_buttons2[j].external_factory);
	    	}
	    }
	}

        /* Now populate the previous panel item, the custom buttons */
	for (i = 0; i < PROG_BUTTONS1_NUMBER; i++) {
	    xv_set(custom_buttons->item, PANEL_CHOICE_STRING,
	           i, prog_buttons1[i].external_current, 0);
   	}

	xv_set(custom_buttons->item, PANEL_VALUE, 0, 0);
        /* Now initially set commands choices stack
           to value of first custom button */
        xv_set(commands_choices->item, PANEL_VALUE, remember_to_set, 0);
        last_commands_value = remember_to_set;

        xv_set(custom_buttons_textfield->item,
                PANEL_VALUE, prog_buttons1[0].external_current, 0);
	change = FALSE; /* because custom_buttons_proc sets it TRUE */
}

/* This applys the commands_buttons also (a noop) */
/* No gettext called, already in localized strings */
void
apply_custom_buttons(mailrc_file, p)

	FILE            *mailrc_file;
	PROPS_ENTRY	*p;

{
	int 	i;

	/* No need to xv_get, since all values are already stored */
	for (i = 0; i < PROG_BUTTONS1_NUMBER; i++) {
	    if (prog_buttons1[i].external_current != NULL) {
		/* If empty string, write out a space, 
		 * else button code won't like that
		 */
	      	if (*prog_buttons1[i].external_current == '\0') {
			free (prog_buttons1[i].external_current);
			prog_buttons1[i].external_current = strdup(" ");
	    		xv_set(custom_buttons->item, PANEL_CHOICE_STRING,
	           		i, prog_buttons1[i].external_current, 0);
        		xv_set(custom_buttons_textfield->item,
                		PANEL_VALUE, prog_buttons1[i].external_current, 0);
	    	}
		fprintf(mailrc_file, "#-button 1 %i \"%s\" %s\n",
		 	i, prog_buttons1[i].external_current,
		 	prog_buttons1[i].internal);
	    }
   	}
}


static void
prepare_list_box(props, row, longestlabel)
     PROPS_ENTRY *props;
     int row;
     int longestlabel;
{
	LABELS     generic_list;
	char	   *value;

	generic_list.longestlabel = longestlabel;

	if (props == mt_message_props) {

                /* STRING_EXTRACTION -
                 *
                 * We are in the message window property sheet, working
                 * on setting up the scrolling list to hide mail headers.
                 * The "Header Field" is a text field that allows
                 * user to specify the mail message header s/he does
                 * not want displayed in view window.
                 */
		generic_list.list_label = gettext("Hide:");
		generic_list.text1_label = gettext("Header Field:");
		generic_list.text2_label = NULL;
		generic_list.help_data = "mailtool:PropsHideList";
		generic_list.list_width_in_chars = 30;
		generic_list.list_display_rows   =  8;
		mt_message_propslist = build_header_list_box(mt_props_frame,
			mt_props_panel1, props, row, generic_list);
		ignore_enumerate(load_ignore_header_proc, 
			mt_message_propslist->scroll_list);
		/* 
		 * Case of no ignore set has already been checked
		 * by either startup or in apply
		 */
	} else if (props == mt_compose_props) {

                /* STRING_EXTRACTION -
                 *
                 * We are in the composition header property sheet, working
                 * on setting up the "Custom Fields scrolling list.
                 * A "Custom Field" is a text field that will be added
                 * to every outgoing message.  "Field" is the title that
                 * will be shown on the text field that the user will
                 * initialize.
                 */
		generic_list.list_label = gettext("Custom Fields:");
		generic_list.text1_label = gettext("Header Field:");
		generic_list.text2_label = gettext("Default Value:");
		generic_list.help_data = "mailtool:PropsCustomList";
		generic_list.list_width_in_chars = 30;
		generic_list.list_display_rows   =  6;

		mt_compose_propslist = build_header_list_box(mt_props_frame,
			mt_props_panel1, props, row, generic_list);

      		load_header_fields(mt_compose_propslist->scroll_list,
			 	mt_value("additionalfields"));
	} else if (props == mt_filing_props) {

                /* STRING_EXTRACTION -
                 *
                 * We are in the Mail Filing property sheet.
                 * The "Permanent File" textfield 
                 * allows one to specify names 
                 * of folders to be permanently displayed in the
                 * Move, Copy, and Load menu stacks.
                 */
		generic_list.list_label = gettext("Move, Copy, Load Menus:");
		generic_list.text1_label = gettext("Permanent File:");
		generic_list.text2_label = NULL;
		generic_list.help_data = "mailtool:PropsMenuList";
		generic_list.list_width_in_chars = 22;
		generic_list.list_display_rows   = 10;

		mt_filing_propslist = build_header_list_box(mt_props_frame,
			mt_props_panel1, props, row, generic_list);

      		load_header_fields(mt_filing_propslist->scroll_list,
			 	mt_value("filemenu2"));

	} else if (props == mt_template_props) {

                /* STRING_EXTRACTION -
                 *
                 * We are in "Template properties".  A template is a standard
                 * message outline that can be included in one's message
                 * via a special menu in the compose pane.
                 * 
                 * The "Name" field controls the title of the template
                 * as displayed in the compose pane template menu.  The
                 * "File" field is a pathname to the actual template file.
                 */
		generic_list.list_label = gettext("Templates:");
		generic_list.text1_label = gettext("Name:");
		generic_list.text2_label = gettext("File:");
		generic_list.help_data = "mailtool:PropsTemplatesList";
		generic_list.list_width_in_chars = 30;
		generic_list.list_display_rows   =  8;
      
		mt_template_propslist = build_header_list_box(mt_props_frame,
			mt_props_panel1, props, row, generic_list);

		/* no gettext needed, since initially files will
                   be in Engligh */
		if ((value = mt_value("templates")) == NULL)
			value = "calendar:$OPENWINHOME/share/xnews/client/templates/calendar.tpl";
		load_header_fields(mt_template_propslist->scroll_list,
			 value);

	} else if (props == mt_alias_props) {

                /* STRING_EXTRACTION -
                 *
                 * We are in "Alias properties".  
                 * The "Alias" and "Addresses" textfields allows you 
                 * to specify your aliases and login addresses into
                 * the scrolling list.
                 */
		generic_list.list_label = gettext("Aliases:");
		generic_list.text1_label = gettext("Alias:");
		generic_list.text2_label = gettext("Addresses:");
		generic_list.help_data = "mailtool:PropsAliasList";
		generic_list.list_width_in_chars = 45;
		generic_list.list_display_rows   = 12;

		mt_alias_propslist = build_header_list_box(mt_props_frame,
			mt_props_panel2, props, row, generic_list);
		/* make sure text_field2 stores at least 1024 + 128 bytes,
		 * and as long as the longest one + 128,
		 * if that's bigger
		 * (the 128 is in case user wants to add more)
		 */
		longest_alias_value = 1024;
		alias_enumerate(load_alias_header_proc, 
			mt_alias_propslist->scroll_list);
		xv_set (mt_alias_propslist->text_field2,
			PANEL_VALUE_STORED_LENGTH, longest_alias_value + 128,
			0);

	}
}


static PROPS_LIST_BOX *
build_header_list_box(frame, panel, props, row, labels)

	Frame	frame;
	Panel	panel;
	PROPS_ENTRY	*props; /* used only for a4 case */
	int	row;
        LABELS  labels;

{
        PROPS_LIST_BOX *propslist;
	Menu	menu;
	Menu_item	menu_item;
	Font_string_dims font_size;
	Xv_Font pf = NULL;
	int	charwidth;
	int	charheight;
	int	longest_width;
	int	list_label_width;
	int	text1_label_width;
	int	text2_label_width;
	int	max;

	/*
	 * Create the list box.
	 */
	propslist = (PROPS_LIST_BOX *)malloc(sizeof(PROPS_LIST_BOX));      

     	/* figure out how big a "character" is; no gettext needed */
        pf = (Xv_Font) xv_get(panel, XV_FONT);
        xv_get(pf, FONT_STRING_DIMS, "n", &font_size);
        charwidth = font_size.width;
        charheight = font_size.height;

	/* If longest label so far in prop sheet 
	 * 	is less than 15 chars
	 * and if scroll_list label string is less than 15 chars
	 * and is less than the longest label in that prop sheet,
	 * align it with the colons,  else start it left at 0
	 */

        xv_get(pf, FONT_STRING_DIMS, labels.list_label, &font_size);
	list_label_width  = font_size.width;
        xv_get(pf, FONT_STRING_DIMS, labels.text1_label, &font_size);
	text1_label_width = font_size.width;
        xv_get(pf, FONT_STRING_DIMS, labels.text2_label, &font_size);
	text2_label_width = font_size.width;

	/* Check longest so far with the 2 text field strings */
	if ((labels.longestlabel < (15 * charwidth))
	    && (list_label_width < (15 * charwidth))
	    && (list_label_width < labels.longestlabel)) {
		longest_width = labels.longestlabel;
	} else {
		longest_width = list_label_width/* + charwidth*/;
	}
	if (text1_label_width > longest_width) 
		longest_width = text1_label_width;
	if (text2_label_width > longest_width) 
		longest_width = text2_label_width;
	/* Some visual adjustment or else too far left */
	if (longest_width != labels.longestlabel)
		longest_width += (2 * charwidth);
	
	propslist->scroll_list = xv_create(panel, PANEL_LIST,
		PANEL_LABEL_X, 	longest_width - list_label_width,
		XV_Y,	xv_row(panel, row),
		/*PANEL_LAYOUT,  PANEL_VERTICAL, */
		PANEL_LABEL_STRING, labels.list_label,
		PANEL_LIST_DISPLAY_ROWS, labels.list_display_rows,
		PANEL_LIST_WIDTH, labels.list_width_in_chars * charwidth,
		PANEL_READ_ONLY, TRUE,
		PANEL_CHOOSE_ONE, TRUE,
		PANEL_CHOOSE_NONE, TRUE,
		PANEL_NOTIFY_PROC, list_notify,
		XV_SHOW, FALSE,
		XV_HELP_DATA,	labels.help_data,
		0);

        /* STRING_EXTRACTION -
         *
         * The labels for the 3 buttons that appear next to all
         * the scrolling lists.
         *
         * The first button is "Add".
         * Sometimes the first button is just a button, other
         * times it is a button stack with 2 menu items, 
         * "Before" and "After".
         * "Before" says to add the field before the current selection.
         * "After" says to add the field after the current selection.
         * If it is just a button then it will sort the entry
         * for you alphabetically.
         * "Delete" (second button) says to delete the field from the list.
         * "Change" (third button) says to replace the selected field in the list
         * with what's on the text field.
         */

	menu = xv_create(XV_NULL, MENU,
		MENU_ITEM, 
			MENU_STRING, gettext("After"),
			MENU_ACTION_PROC, field_menu_proc,
			MENU_VALUE, 1,
			0,
		MENU_ITEM,
			MENU_STRING, gettext("Before"),
			MENU_ACTION_PROC, field_menu_proc,
			MENU_VALUE, 2,
			0,
		0);

	propslist->add = xv_create(panel, PANEL_BUTTON,
		PANEL_LABEL_STRING, gettext("Add"),
		/* Try and center the 2 buttons */
        	XV_X, 	xv_get(propslist->scroll_list, PANEL_LABEL_X) 
        		+ xv_get(propslist->scroll_list, XV_WIDTH) 
			+ (3 * charwidth),
		XV_Y,	xv_get(propslist->scroll_list, XV_Y)
        		+ (xv_get(propslist->scroll_list, XV_HEIGHT)/2)
			- (3 * charheight),
		XV_HELP_DATA,	"mailtool:PropsAddChangeDelete",
		0);

	if (props == mt_alias_props) {
		xv_set (propslist->add,
			PANEL_NOTIFY_PROC, add_proc,
			0);
	} else {
		xv_set (propslist->add,
			PANEL_ITEM_MENU, menu,
			0);
	}

	propslist->delete = xv_create(panel, PANEL_BUTTON,
		PANEL_LABEL_STRING, gettext("Delete"),
		PANEL_NOTIFY_PROC,  delete_proc,
        	XV_X, 	xv_get(propslist->scroll_list, PANEL_LABEL_X) 
        		+ xv_get(propslist->scroll_list, XV_WIDTH) 
			+ (3 * charwidth),
		XV_Y,	xv_get(propslist->scroll_list, XV_Y)
        		+ (xv_get(propslist->scroll_list, XV_HEIGHT)/2)
			- (1 * charheight),
		XV_HELP_DATA,	"mailtool:PropsAddChangeDelete",
		0);

	propslist->change = xv_create(panel, PANEL_BUTTON,
		PANEL_LABEL_STRING, gettext("Change"),
		PANEL_NOTIFY_PROC,  change_proc,
        	XV_X, 	xv_get(propslist->scroll_list, PANEL_LABEL_X) 
        		+ xv_get(propslist->scroll_list, XV_WIDTH) 
			+ (3 * charwidth),
		XV_Y,	xv_get(propslist->scroll_list, XV_Y)
        		+ (xv_get(propslist->scroll_list, XV_HEIGHT)/2)
			+ (1 * charheight),
		XV_HELP_DATA,	"mailtool:PropsAddChangeDelete",
		0);
	
	/* Set width to be the same for 3 buttons */
	max = xv_get(propslist->add, XV_WIDTH);
	if (xv_get(propslist->delete, XV_WIDTH) > max)
		max = xv_get(propslist->delete, XV_WIDTH);
	if (xv_get(propslist->change, XV_WIDTH) > max)
		max = xv_get(propslist->change, XV_WIDTH);

	xv_set(propslist->add, PANEL_LABEL_WIDTH, 
		xv_get(propslist->add, PANEL_LABEL_WIDTH)
		+ max - xv_get(propslist->add, XV_WIDTH), 
		0);
	xv_set(propslist->delete, PANEL_LABEL_WIDTH,
		xv_get(propslist->delete, PANEL_LABEL_WIDTH)
		+ max - xv_get(propslist->delete, XV_WIDTH), 
		0);
	xv_set(propslist->change, PANEL_LABEL_WIDTH,
		xv_get(propslist->change, PANEL_LABEL_WIDTH)
		+ max - xv_get(propslist->change, XV_WIDTH), 
		0);

	propslist->text_field1 = xv_create(panel, PANEL_TEXT,
		PANEL_NOTIFY_LEVEL, PANEL_ALL,
		PANEL_NOTIFY_PROC,  report_sl_txtfld_change,
		PANEL_LABEL_STRING, labels.text1_label,
		/* Align relative to scroll list's label colon */
		PANEL_VALUE_X, 
		 xv_get(propslist->scroll_list, PANEL_VALUE_X),
		PANEL_VALUE_DISPLAY_LENGTH,
		 (xv_get(propslist->scroll_list, PANEL_LIST_WIDTH)
		  / charwidth) + 2, /* add in space for scrollbar */
		XV_Y,	xv_get(propslist->scroll_list, XV_Y)
        		+ xv_get(propslist->scroll_list, XV_HEIGHT)
			+ (1 * charheight),
		PANEL_VALUE_STORED_LENGTH, 512,
		XV_HELP_DATA,	"mailtool:PropsAddChangeDelete",
		0);

	/* Check if I want to create second textfield */
	if (labels.text2_label) {
		propslist->text_field2 
		   = xv_create(panel, PANEL_TEXT,
			PANEL_LABEL_STRING, labels.text2_label,
			PANEL_NOTIFY_LEVEL, PANEL_ALL,
			PANEL_NOTIFY_PROC,  report_change,
			PANEL_VALUE_X, 
		 	 xv_get(propslist->scroll_list, PANEL_VALUE_X),
			XV_Y, xv_get(propslist->text_field1, XV_Y)
        		      + xv_get(propslist->text_field1, XV_HEIGHT)
			      + (1 * charheight),
			PANEL_VALUE_DISPLAY_LENGTH,
		 	 (xv_get(propslist->scroll_list, PANEL_LIST_WIDTH)
		 	  / charwidth) + 2,
			PANEL_VALUE_STORED_LENGTH, 1024,
			XV_HELP_DATA,	"mailtool:PropsAddChangeDelete",
			0);
	} else {
		propslist->text_field2 = NULL;
	}
          
	return(propslist);
}

static void
field_menu_proc(menu, menu_item)

	Menu	menu;
	Menu_item	menu_item;
{
	int	nsel;
	int	nitems;
	int	menu_value;
	Panel_item    mt_fields_list;

	if (mt_get_current_props() == mt_message_props)
	  {
	    mt_fields_list = mt_message_propslist->scroll_list;
	  }
	else if (mt_get_current_props() == mt_compose_props)
	  {
	    mt_fields_list = mt_compose_propslist->scroll_list;
	  }
	else if (mt_get_current_props() == mt_filing_props)
	  {
	    mt_fields_list = mt_filing_propslist->scroll_list;
	  }
	else if (mt_get_current_props() == mt_template_props)
	  {
	    mt_fields_list = mt_template_propslist->scroll_list;
	  }
	else if (mt_get_current_props() == mt_alias_props)
	  {
	    mt_fields_list = mt_alias_propslist->scroll_list;
	  }
	
	nitems = (int)xv_get(mt_fields_list, PANEL_LIST_NROWS);
	nsel = get_selected_list_no(mt_fields_list);
	menu_value = (int)xv_get(menu_item, MENU_VALUE);

	switch (menu_value) 
	{

	case 1: /* Add After */
	  if (nsel == -1)  /* if nothing selected, add to end */
	    add_header_field_to_list(mt_fields_list, nitems);
	  else
	    mt_header_field_add_after(mt_fields_list, nsel);
	  break;
	case 2:	/* Add Before */
	  if (nsel == -1)
	    add_header_field_to_list(mt_fields_list, 0);	    
	  else 
	    mt_header_field_add_before(mt_fields_list, nsel);
	  break;
	default:
	  break;
	}
	
}

/* Used only for Add button in Alias Props */
/* Now also used in category_proc()'s special case
 * of flag sl_txtfld_change - a cludge since
 * I call add_proc(0,0) in that case
 */
static void
add_proc(item, event)

	Panel_item	item;
	Event		*event;
{
	Panel_item    mt_fields_list;

	if (mt_get_current_props() == mt_message_props)
	  {
	    mt_fields_list = mt_message_propslist->scroll_list;
	  }
	else if (mt_get_current_props() == mt_compose_props)
	  {
	    mt_fields_list = mt_compose_propslist->scroll_list;
	  }
	else if (mt_get_current_props() == mt_filing_props)
	  {
	    mt_fields_list = mt_filing_propslist->scroll_list;
	  }
	else if (mt_get_current_props() == mt_template_props)
	  {
	    mt_fields_list = mt_template_propslist->scroll_list;
	  }
	else if (mt_get_current_props() == mt_alias_props)
	  {
	    mt_fields_list = mt_alias_propslist->scroll_list;
	  }
	
	add_header_field_to_list(mt_fields_list, 0);
}

static void
delete_proc(item, event)

	Panel_item	item;
	Event		*event;
{
	int	nsel;
	Panel_item    mt_fields_list;
	int	row;
	int	new_nsel;
	int	view_window_start;
        PROPS_LIST_BOX *p;
	char *s1;
	char *s2;
        char *string;
        char *client_data;

	if (mt_get_current_props() == mt_message_props)
	  {
	    mt_fields_list = mt_message_propslist->scroll_list;
            p = mt_message_propslist;
	  }
	else if (mt_get_current_props() == mt_compose_props)
	  {
	    mt_fields_list = mt_compose_propslist->scroll_list;
            p = mt_compose_propslist;
	  }
	else if (mt_get_current_props() == mt_filing_props)
	  {
	    mt_fields_list = mt_filing_propslist->scroll_list;
            p = mt_filing_propslist;
	  }
	else if (mt_get_current_props() == mt_template_props)
	  {
	    mt_fields_list = mt_template_propslist->scroll_list;
            p = mt_template_propslist;
	  }
	else if (mt_get_current_props() == mt_alias_props)
	  {
	    mt_fields_list = mt_alias_propslist->scroll_list;
            p = mt_alias_propslist;
	  }
	
	row = (int)xv_get(mt_fields_list, PANEL_LIST_NROWS);
	nsel = get_selected_list_no(mt_fields_list);
  	if (nsel < 0)
	     return;  /* nothing selected or empty list */
	view_window_start = 
	     xv_get(xv_get(mt_fields_list, PANEL_LIST_SCROLLBAR),
			SCROLLBAR_VIEW_START);
  	if (nsel >= 0)
	  mt_header_field_delete(mt_fields_list, nsel);
	/*
	 * If there are other rows left, make next entry highlighted.
	 */
	if (row > 1) {
	  /*
 	   * if it was last row, highlight previous one
	   */
	  if (row == (nsel + 1)) { /* fencepost adjustment */
		new_nsel = nsel - 1;
	        /* also check to see if viewing window needs 
		 * to shift up one
	   	 */
		if (view_window_start == nsel)
			xv_set(xv_get(mt_fields_list, 
				PANEL_LIST_SCROLLBAR),
				SCROLLBAR_VIEW_START,
				new_nsel,
				0);
	  } else
		new_nsel = nsel;
	  xv_set(mt_fields_list, PANEL_LIST_SELECT, 
			new_nsel, TRUE, 0);

	  string = (char *) xv_get(mt_fields_list, 
			PANEL_LIST_STRING, new_nsel, 0);
	  client_data = (char *) xv_get(mt_fields_list, 
			PANEL_LIST_CLIENT_DATA, new_nsel);

	  /* Took this section from list_notify() */
	  if (mt_get_current_props() == mt_alias_props) {
             /* Copy value into a buffer
                since strtok is destructive */
             s1 = (char *)strdup(string);

             if ((s2 = (char *) strtok(s1, " = ")) == NULL) {
                   /* should never get here, since I put
                      in the delimiter, not the user */
             } else {
                   /* s2 points to the first part */
                   xv_set(p->text_field1, PANEL_VALUE, s2, 0);
             }
             ck_free(s1);  
          } else {
             xv_set(p->text_field1, PANEL_VALUE, string, 0);
          }
          if (p->text_field2)
             /* Do this because if NULL xv_set ignores it and
                txtfld does not get cleared */
             if (client_data == NULL)
                  xv_set(p->text_field2, PANEL_VALUE,
                         "", 0);
             else
                  xv_set(p->text_field2, PANEL_VALUE,
                         (char *) client_data, 0);
	}
	if (row == 1) {
             xv_set(p->text_field1, PANEL_VALUE, "", 0);
             if (p->text_field2)
                 xv_set(p->text_field2, PANEL_VALUE, "", 0);
	}
}

static void
change_proc(item, event)

	Panel_item	item;
	Event		*event;
{
	int	nsel;
	Panel_item    mt_fields_list;

	if (mt_get_current_props() == mt_message_props)
	  {
	    mt_fields_list = mt_message_propslist->scroll_list;
	  }
	else if (mt_get_current_props() == mt_compose_props)
	  {
	    mt_fields_list = mt_compose_propslist->scroll_list;
	  }
	else if (mt_get_current_props() == mt_filing_props)
	  {
	    mt_fields_list = mt_filing_propslist->scroll_list;
	  }
	else if (mt_get_current_props() == mt_template_props)
	  {
	    mt_fields_list = mt_template_propslist->scroll_list;
	  }
	else if (mt_get_current_props() == mt_alias_props)
	  {
	    mt_fields_list = mt_alias_propslist->scroll_list;
	  }
	
	nsel = get_selected_list_no(mt_fields_list);
  	if (nsel >= 0) {
	    mt_header_field_delete(mt_fields_list, nsel);
    	    add_header_field_to_list(mt_fields_list, nsel);
	}
}

int
get_selected_list_no(list)
Panel_item	list;
{
	int	row;

	row = (int)xv_get(list, PANEL_LIST_NROWS);

	if (row == 0)
		return (-1);

	for ( ; row > 0; row--)
	{
		if (xv_get (list, PANEL_LIST_SELECTED, row -1))
			break;
	}

	return (row - 1);
}


static 	char buffer[MAXPATHLEN];
static  char full_name[MAXPATHLEN];

add_header_field_to_list(list, position)

        Panel_item  list;
	int	position;

{
	char * s;
	char * s1;
	char * t;
	char * t1;
	PROPS_LIST_BOX *propslist;
	int row;
	int i;
	int j;
	char *compare;
        static  displayed_message;
	
	if (mt_get_current_props() == mt_message_props)
		propslist = mt_message_propslist;
	else if (mt_get_current_props() == mt_compose_props)
		propslist = mt_compose_propslist;
	else if (mt_get_current_props() == mt_filing_props)
		propslist = mt_filing_propslist;
	else if (mt_get_current_props() == mt_template_props)
		propslist = mt_template_propslist;
	else if (mt_get_current_props() == mt_alias_props)
		propslist = mt_alias_propslist;

	s = (char *)xv_get(propslist->text_field1, PANEL_VALUE);
	/* check if s is empty */
	if ((s == NULL) || (*s == '\0'))
		return;
	/* 
	 * mt_strip_leading_blanks returns empty string,
	 * because xv_set does not work with NULL strings
	 */
	if (*(s = mt_strip_leading_blanks(s)) == '\0')
 		return;
	xv_set(propslist->text_field1, PANEL_VALUE, s, 0);
	/* Original s freed, so get it again */
	s = (char *)xv_get(propslist->text_field1, PANEL_VALUE);
	
	/* if it's a template, get the file name and store it as client data */
	if (propslist == mt_template_propslist) {
		t = (char *)xv_get(propslist->text_field2, PANEL_VALUE);
		t = (char *)strdup(t);
		t = mt_strip_leading_blanks(t);
		xv_set(propslist->text_field2, PANEL_VALUE, t, 0);
		ck_free(t);

		t = (char *)xv_get(propslist->text_field2, PANEL_VALUE);
                ds_expand_pathname(t, buffer);

		if (buffer[0] == '/')
	      		strcpy(full_name, buffer);
	    	else
      		      sprintf(full_name, "%s/%s", getenv("HOME"), buffer);

	    	if (access(full_name, F_OK) == -1) {
                        /* STRING_EXTRACTION -
                         *
                         * We are warning the user that the .mailrc file does
                         * not exist.  %s is the full name of the .mailrc
                         * file we are looking for.
                         */
			mt_vs_warn(mt_props_frame,
			      gettext("Warning:\n%s\ndoes not exist"),
			      full_name);
	      	}

		t1 = (char *)strdup(buffer);
	    	xv_set(list, PANEL_LIST_INSERT, position,
		   PANEL_LIST_STRING, position, s, 
		   PANEL_LIST_CLIENT_DATA, position, t1, 
		   0);
	} else if (propslist == mt_alias_propslist) {
		t = (char *)xv_get(propslist->text_field2, PANEL_VALUE);
		/* malloc for t for CLIENT_DATA,
 		   temporary malloc for t1 */
		t = (char *)strdup(t);
 		t1 = (char *) malloc(strlen(s) + 1 
			  	   + strlen("                = ") 
		          	   + strlen(t) + 1);
		sprintf(t1, "%-12s = %s", s, t);
		xv_set(list, PANEL_LIST_INSERT, position,
		 	PANEL_LIST_STRING, position, t1,
		   	PANEL_LIST_CLIENT_DATA, position, t, 
		    	PANEL_LIST_FONT, position, textsw_font,
		 	0);
		xv_set(list, PANEL_LIST_SORT, PANEL_FORWARD, 0);

		/* first, deselect all others */
  		row = (int)xv_get(list, PANEL_LIST_NROWS);
		for (i=0; i < row; i++) {
			compare =(char *) xv_get(list, PANEL_LIST_STRING, i);
			if (strcmp(compare, t1) == 0) {
			   j = i;
			}

			if (xv_get(list, PANEL_LIST_SELECTED, i)) {
				xv_set(list, PANEL_LIST_SELECT, i, FALSE, 0);
			}
		}
		/* set view to where the new item is */
    	      	xv_set(xv_get(list, PANEL_LIST_SCROLLBAR),
      	        	SCROLLBAR_VIEW_START, 
		  	(j<2) ? 0 : (j-2), 
        	  	0);
		/* goto that new row and mark it as selected */
  		xv_set(list, PANEL_LIST_SELECT, j, TRUE, 0);
		ck_free(t1);
	} else if (propslist == mt_compose_propslist) {
		validate_header_field(s);
		t = (char *)xv_get(propslist->text_field2, PANEL_VALUE);
		/* malloc for t for CLIENT_DATA */
		t = (char *)strdup(t);
	    	xv_set(list, PANEL_LIST_INSERT, position,
		   PANEL_LIST_STRING, position, s, 
		   PANEL_LIST_CLIENT_DATA, position, t, 
		   0);
	} else if (propslist == mt_filing_propslist) {
                  /*
                   * Check for leading plus and tell the user that they don't
                   * need it.  We only tell them once.
                   */
                  if (*s == '+') {
                        if (!displayed_message) {
                                mt_vs_warn(mt_props_frame,
gettext("The leading + is no longer required for accessing Mail Files.\nAll path names are considered relative to the Mail File directory.\nContinuing operation..."));

                                displayed_message = TRUE;
                        }
                  }
	          xv_set(list, PANEL_LIST_INSERT, position,
		 	PANEL_LIST_STRING, position, s,
		 	0);
	} else {
		xv_set(list, PANEL_LIST_INSERT, position,
		 	PANEL_LIST_STRING, position, s,
		 	0);
	}
  change = TRUE;
  /* This is now false, since we added */
  sl_txtfld_change = FALSE;
}

static
validate_header_field(field)

	char	*field;

{
	register char 	*p;

	/*
	 * We just check for trailing :'s which the user may feel he
	 * has to put in
	 */
	for (p = field; *p != '\0'; p++)
		;

	p--;
	if (*p == ':') {
		while (*p == ':') {
			*p-- = '\0';
		}
                /* STRING_EXTRACTION -
                 *
                 * We are warning the user that the specified header field
                 * should not have a trailing ':', and we are deleting it
                 * the ':'.
                 */
		mt_vs_warn(mt_props_frame, gettext(
"You do not need to specify the trailing :\nThis field will be inserted without the :\n(%s)"),
field);
	}

	return;
}


mt_header_field_add_before(list, list_entry)
     
     Panel_item		list;
     int                list_entry;
{
  if (list_entry != -1)
    add_header_field_to_list(list, list_entry);
}


mt_header_field_add_after(list, list_entry)

     Panel_item		list;
     int list_entry;
{
  if (list_entry != -1)
    add_header_field_to_list(list, list_entry + 1);
}


mt_header_field_delete(list, list_entry)

     Panel_item		list;
     int	        list_entry;
{
     char  *client_data;

  if (list_entry == -1)
    return;
  
  client_data = (char *) xv_get(list, 
		 PANEL_LIST_CLIENT_DATA, list_entry);
  if (client_data) ck_free(client_data);
  xv_set(list, PANEL_LIST_DELETE, list_entry, 0);

  change = TRUE;
}



int
list_notify(item, string, client_data, op, event)
	Panel_item item;
	char *string;
	Xv_opaque client_data;
	Panel_list_op op;
	Event  *event;
{
	PROPS_LIST_BOX *p;
	char *s1;
	char *s2;

	if (mt_get_current_props() == mt_message_props)
		p = mt_message_propslist;
	else if (mt_get_current_props() == mt_compose_props)
		p = mt_compose_propslist;
	else if (mt_get_current_props() == mt_filing_props)
		p = mt_filing_propslist;
	else if (mt_get_current_props() == mt_template_props)
		p = mt_template_propslist;
	else if (mt_get_current_props() == mt_alias_props)
		p = mt_alias_propslist;

	switch (op)
	{
	case PANEL_LIST_OP_DESELECT:
		break;

	case PANEL_LIST_OP_SELECT:
		if (mt_get_current_props() == mt_alias_props) {
                   /* Copy value into a buffer 
		      since strtok is destructive */
		   s1 = (char *)strdup(string);

                   if ((s2 = (char *) strtok(s1, " = ")) == NULL) {
			 /* should never get here, since I put 
			    in the delimiter, not the user */
			 return(-1); 
		   } else {
			 /* s2 points to the first part */
		    	 xv_set(p->text_field1, PANEL_VALUE, s2, 0);
		   }
		   ck_free(s1);
		} else {
		   xv_set(p->text_field1, PANEL_VALUE, string, 0);
		}
		if (p->text_field2)
		   /* Do this because if NULL xv_set ignores it and
		      txtfld does not get cleared */
		   if (client_data == NULL)
			xv_set(p->text_field2, PANEL_VALUE,
			       "", 0);
		   else
			xv_set(p->text_field2, PANEL_VALUE,
			       (char *) client_data, 0);
		break;

	case PANEL_LIST_OP_VALIDATE:
		break;

	case PANEL_LIST_OP_DELETE:
		break;
	}
}


write_header_fields(fp, list)

     FILE	*fp;
     Panel_item	list;

{
	int	nitems;
	char  *entry_text, *entry_client_data;
	int	n;


	if (list == mt_template_propslist->scroll_list) {
		fprintf(fp, "set templates='");
	} 
	else if (list == mt_compose_propslist->scroll_list) {
		fprintf(fp, "set additionalfields='");
	}
	else if (list == mt_filing_propslist->scroll_list) {
		fprintf(fp, "set filemenu2='");
	}

	/*
	* Concatenate fields, separating them by " "
	*/
	nitems = (int)xv_get(list, PANEL_LIST_NROWS);
	for (n = 0; n < nitems ; n++)
	{
		if (n > 0)
		{
			putc(' ', fp);
		}

		entry_text = (char *)
			xv_get(list, PANEL_LIST_STRING, n, 0);

		put_with_escape(fp, entry_text);

		entry_client_data = (char *)
			xv_get(list, PANEL_LIST_CLIENT_DATA, n);

		if (entry_client_data)
		{
			fprintf(fp, ":");

			put_with_escape(fp, entry_client_data);
		}
	}

	fprintf(fp, "'\n");
}


static void
put_with_escape(fp, string)
	FILE	*fp;
	char	*string;
{
	/* the theory: escape spaces, backslashes, and colons so they
	 * don't interfear with our parsing when we read this back in...
	 */

	while (*string) {
		switch(*string) {
		case ' ':
		case ':':
		case '\\':
			/* escape the character */
			putc('\\', fp);
			/* FALL THROUGH */
		default:
			putc(*string, fp);
			break;
		}
		string++;
	}
}


write_message_fields(fp, list)

     FILE	*fp;
     Panel_item	list;

{
	int	nitems;
	char  *entry_text, *entry_client_data;
	int	n;


	nitems = (int)xv_get(list, PANEL_LIST_NROWS);
	/* If empty, write out "ignore nothing" */
	if (nitems == 0) {
		fprintf(fp, "set ignorenothing\n");
		return;
	}
	if (list == mt_message_propslist->scroll_list) {
		fprintf(fp, "ignore");
	} 

	/*
	* Concatenate fields, separating them by single quotes
	*/
	for (n = 0; n < nitems ; n++)
	{
		fprintf(fp, " \'");

		entry_text = (char *)
			xv_get(list, PANEL_LIST_STRING, n, 0);
		fprintf(fp, "%s", entry_text);

		putc('\'', fp);
	}

	fprintf(fp, "\n");
}


write_alias_fields(fp, list)

     FILE	*fp;
     Panel_item	list;

{
	int	nitems;
	char    *string;
	char	*s1;
	char	*s2;
	int	n;


	if (list == mt_alias_propslist->scroll_list) {
		fprintf(fp, "#-clearaliases\n");
	} 

	/*
	* One alias for every row in scroll list
	*/
	nitems = (int)xv_get(list, PANEL_LIST_NROWS);
	for (n = 0; n < nitems ; n++)
	{
		fprintf(fp, "alias ");

                /* Copy value into a buffer 
			since strtok is destructive */
		string = (char *)
			   xv_get(list, PANEL_LIST_STRING, n, 0);
                s1 = (char *)strdup(string);
                if ((s2 = (char *) strtok(s1, " = ")) == NULL) {
                     /* should never get here, since I put
                            in the delimiter, not the user */
                        return(-1);
                } else {
                     /* s2 points to the first part */
		     fprintf(fp, "%s", s2);
                }
                ck_free(s1);

                string = (char *)
                        xv_get(list, PANEL_LIST_CLIENT_DATA, n);
		fprintf(fp, " %s", string);

		fprintf(fp, "\n");
	}
}


static int
load_ignore_header_proc(key, value, list)
	char *   key;
	char *   value;   /* NULL for ignore */
	Panel_item list;
{
  	int row;

	ignore_exists = TRUE;
  	row = (int)xv_get(list, PANEL_LIST_NROWS);
	xv_set( list,
		PANEL_LIST_INSERT, row,
		PANEL_LIST_STRING, row, key,
		PANEL_LIST_CLIENT_DATA, row, NULL,
		0);
	return (0);
}


static int
load_alias_header_proc(key, value, list)
	char *   key;
	char *   value; /* is read-only string for alias (can't touch this) */
	Panel_item list;
{
  	int row;
	char *   s;
	char *   t;

	/* Also add in potential spaces used to justify the = sign */
 	s = (char *) malloc(strlen(key) + 1 
			  + strlen("                = ") 
		          + strlen(value) + 1);
	sprintf(s, "%-12s = %s", key, value);
	t = strdup(value);
	if (longest_alias_value < (int) (strlen(s) + 1))
		longest_alias_value = strlen(s) + 1;

  	row = (int)xv_get(list, PANEL_LIST_NROWS);
	xv_set( list,
		PANEL_LIST_INSERT, row,
		PANEL_LIST_STRING, row, s,
		PANEL_LIST_CLIENT_DATA, row, t,
		PANEL_LIST_FONT, row, textsw_font, 
		0);
	ck_free(s);
	/* cannot free t since CLIENT_DATA needs it */

	xv_set( list,
		PANEL_LIST_SORT, PANEL_FORWARD,
		0);

	return (0);
}


/*
 * this routine is kind of like strtok(), but it is hardwired to
 * to the string processing that we use to represent an external form
 * of a template or additionalfields string.  Basically, these strings
 * are of the form of "name" or "name:value" where each name is separated
 * from the next by a space.  The problem is that people want embedded
 * spaces in the name part, so we need to provide a way to escape
 * these embedded tokens.
 *
 * This routine sets namep and valuep to point to static buffers that
 * point to the processed name and value part of the string.  The
 * return value is the point in string after name and string.  If
 * there were no more name and value pairs then the return value
 * is null.
 */
int
parse_external_string(stringp, namep, valuep )
char **stringp;
char **namep;
char **valuep;
{
	static char *name = NULL, *value = NULL;
	static int namelen = 0;
	int stringlen;
	char *string;
	char *s;
	int field;

	string = *stringp;

        /* sanity check: if the string is null, then go away */
        if (!string) return (0);
 
	/* allocate name and value, if we need to */
	stringlen = strlen(string);
	if (stringlen + 1 > namelen) {
		/* get a minimum that should be big enough, normally... */
		namelen = stringlen + 1;
		if (namelen < 128) namelen = 128;

		ck_free(name);
		ck_free(value);

		name = ck_malloc(namelen);
		value = ck_malloc(namelen);
	}

	/* now go and parse out the name and value */

	/* strip leading spaces */
	while (*string == ' ') string++;

	if (!*string) return (0);

	*name = *value = '\0';
	s = name;
	field = 0;
	while (*string && field < 2) {
		switch (*string) {
		case '\\':
			/* escape the next character */
			if (*++string) {
				*s++ = *string++;
			}
			break;
		case ':':
			/* switch to the 2nd half of the name */
			if (field == 0) {
				field = 1;
				*s = '\0';
				s = value;
				string++;
			} else {
				/* just copy the character */
				*s++ = *string++;
			}
			break;
		case ' ':
			/* an unescaped space: we're done */
			field = 2;
			break;
		default:
			/* just copy the character */
			*s++ = *string++;
			break;
		}
	}
	*s = '\0';

	*namep = name;
	*valuep = value;
	*stringp = string;
	return(1);
}

