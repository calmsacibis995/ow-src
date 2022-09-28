/* static  char sccsid[] = "@(#)editor.h 3.15 93/05/03 Copyr 1991 Sun Microsystems, Inc.";
	editor.h
*/
#define WHAT_LEN        256

typedef struct {
        Panel_item      item;
        char            itemval[WHAT_LEN+1];
} Panel_Text_item;

typedef Panel_Text_item Multiple_text_item[5];

typedef enum {
        minsunit = 0,
        hrsunit = 1,
        daysunit = 2
}Advunit;

typedef struct {
        Frame           frame;
        Panel           panel;
	Canvas		canvas;
	Panel_item	datefield;
	Panel_item	appt_type;
        Panel_item      datetext;
        char            dateval[100];
	Multiple_text_item what;
	Panel_item	timestack;
        Panel_item      time;
        char            timeval[25];
	Panel_item	dnd_target;
	Panel_item	apptbox;
	Panel_item	calbox;
        Panel_item      ampm;
        int             ampmval;
	Panel_item	durationstack;
        Panel_item      duration;
        char            durval[25];
        Panel_item      minhr;
        int             minhrval;
        Panel_item      periodstack;
        Panel_item      periodunit;
        int             periodval;
        int             nthval;
        Panel_item      scopestr;
	Panel_item	scopestack;
        Panel_item      scope;
        char            *scopeval;
	Panel_item	scopeunit;
	char		*scopeunitval;
	Panel_item	privacystack;
	Panel_item	privacyunit;
	int		privacyval;
        Panel_item      reminder;
        int             remindval;
	Panel_item	beepadvance;
	Panel_item	beepunitstack;
	Panel_item	beepunit;
	Advunit		beepunitval;
	Panel_item	flashadvance;
	Panel_item	flashunitstack;
	Panel_item	flashunit;
	Advunit		flashunitval;
	Panel_item	openadvance;
	Panel_item	openunitstack;
	Panel_item	openunit;
	Advunit		openunitval;
	Panel_item	mailadvance;
	Panel_item	mailunitstack;
	Panel_item	mailunit;
	Advunit		mailunitval;
	Panel_item      to;
	char            *toval;
	caddr_t         repeat;
} Editor;

typedef enum {
	INSERTION = 0,
	DELETION = 1,
	CHANGE = 2,
	DELETE_FORWARD_NOT_SUPPORTED = 3,
	CHANGE_FORWARD_NOT_SUPPORTED = 4,
	CHANGE_REPEATING_EVENT_NOT_SUPPORTED = 5,
	INSERT_REPEATING_EVENT_NOT_SUPPORTED = 6,
	INSERT_FILEERROR = 7,
	DELETE_FILEERROR = 8,
	CHANGE_FILEERROR = 9
} Editor_op;

extern void update_mailto_entry(), add_to_calbox(), set_default_calbox();
extern void browser2calbox(), add_times(), e_list_flush();
extern void set_defaults(), e_hide_ampm(), new_editor();
extern void mb_add_index(), redisplay(), add_interest();
extern void mb_redisplay(), get_range();
extern void mb_add_times(), set_editor_time();
extern void reset_date_appts(), reset_time_date_appts();
extern void show_editor();
extern int privacystr_to_int(), periodstr_to_int(), units_to_secs();
extern Boolean showing_browser(), showing(), editor_showing(), name_in_list();
extern Menu e_make_duration_menu(), e_make_time_menu();
extern Notify_value e_reset_proc(), update_handler(), date_notify();
extern void make_editor(), activate_scope();
