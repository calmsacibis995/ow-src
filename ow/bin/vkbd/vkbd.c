#ifndef lint
#ifdef sccs
static char     sccsid[] = "@(#)vkbd.c 1.14 92/12/11";
#endif
#endif

/*
 * vkbd.c - Virtual Keyboard
 *
 * To build:
 *	cc -o vkbd -O vkbd.c vkbd_data.c kcode_data.c -I$OPENWINHOME/include \
 *	-L$OPENWINHOME/lib -lxview -lolgx -lX11
 */
#include <string.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <X11/keysym.h>
#include <xview/xview.h>
#include <xview/font.h>
#include <xview/panel.h>
#include <xview/server.h>
#include "vkbd.h"
/* could already be defined in win_input.h */
#ifndef KEYBOARD_KEYSM_MASK
#define KEYBOARD_KYSM_MASK  0xFF00 
#endif
#ifndef KEYBOARD_KYSM
#define KEYBOARD_KYSM  0xFF00
#endif
extern String keyboards[NBR_LANGUAGES][NBR_SHIFTS][NBR_ROWS][NBR_COLS];
extern KeySym keysyms_table[NBR_LANGUAGES][NBR_KEYCODES][KEYSYMS_PER_KEYCODE];
extern String languages[NBR_LANGUAGES];
extern String language_labels[NBR_LANGUAGES];
extern String property_name[NBR_ATOMS];
#ifdef KEYPADS
extern String left_keypad[NBR_ROWS][2];
extern String right_keypad[NBR_ROWS][4];
#endif /* KEYPADS */

Panel_item  altg_choice;
int	    altg_choice_value = 0;  /* value of AltG choice:
				     * 0= use standard graphic
				     * 1= use alternate graphic */
int	    altg_down = 0; /* state of AltGraph key:
			    * 0= up (use standard graphic)
			    * 1= down (use alternate graphic) */
Atom	    atom[NBR_ATOMS];
Frame	    base_frame;
Panel_item  btn[NBR_ROWS*NBR_COLS];
Panel_item  caps_choice;
int	    caps_choice_value = 0;
int	    caps_lock = FALSE;
Panel_item  ctrl_choice;
int	    ctrl_choice_value = 0; /* value of Control choice */
int	    current_language = 0;    /* default is US English */
Display	   *display;
Font	    fixed12_font;   /* 12 pt bold fixedwidth font */
Font	    fixed14_font;   /* 14 pt bold fixedwidth font */
Panel_item  fn_key[NBR_FN_KEYS];
Frame	    fk_frame;	/* Function Keys frame */
int	    fk_frame_mapped;  /* state of Function Keys frame's XV_SHOW upon
			       * entering languages mode */
Panel	    fk_panel;	/* Function Keys panel */
String	    fn_key_str[NBR_FN_KEYS];  /* save function keys' strings while
				       * in Languages mode */
int	    F1_language = 0;	/* index of language in F1 key */
int	    kbd_language = 0;	/* physical keyboard langauge.
				 * Default is US English.  Can be modified
				 * by using the "Set" command.
				 */
int	    kbd_F1_language = 0; /* index of language in F1 key when
				  * physical keyboard language is current
				  * language. */
int	    key_top_key;	/* XV_KEY_DATA key: button's KEY_TOP() value */
int	    languages_mode = FALSE;
Panel_item  meta_choice[2];
int	    meta_choice_value = 0;
int	    nbr_vkbd_btns; /* number of Virtual Keyboard PANEL_BUTTON's */
int	    shifted = 0;  /* 0= use unshifted, 1= use shifted character */
Panel_item  shift_choice[2];
Frame	    vkbd_frame;	/* Virtual Keyboard frame */
Panel	    vkbd_panel;	/* alphanumeric keypad panel in vkbd_frame */
Xv_Server   server;
int 	    vkbd_fr_created  = 0;
int 	    fk_frame_created = 0;

static void	    altg_proc();
static void	    button_notify_proc();
static void	    ctrl_proc();
static void	    create_fk_frame();
static void	    create_vkbd_frame();
static void	    define_atoms();
static void	    fk_btn_notify_proc();
static void         frame_event_proc();
static char	   *get_fn_key_labels();
static void	    init_seln();
static void	    load_languages();
static void	    meta_proc();
static void	    restore_fn_keys();
static void	    send_translated_keysym();
static void	    set_vkbd_labels();
static void	    shift_proc();


/************************************************************************
 *			PROCEDURES					*
 ************************************************************************/

/*ARGSUSED*/
static void
altg_proc(item, value, event)
    Panel_item	    item;
    int		    value;
    Event	   *event;
{
    altg_choice_value = value;
}


/*ARGSUSED*/
static void
button_notify_proc(item, event)
    Panel_item	    item;
    Event	   *event;
{
    String	    keycap;
    Xv_opaque  server_public;
    


   
    server_public = XV_SERVER_FROM_WINDOW(event->ie_win); 
    keycap = (char *) xv_get(item, PANEL_LABEL_STRING);
    event->ie_code = 0;
    if (strlen(keycap) == 2 && strcmp("<>", keycap)) {
	/* Two character button which is not the Meta key */
	event->ie_code = (short) keycap[altg_choice_value] & 0xFF;
	if (event->ie_code == (short) ' ')
	    event->ie_code = (short) keycap[0] & 0xFF;
	if (ctrl_choice_value == 1) {
	    /* Ctrl choice is on */
	    if (event->ie_code >= (int) 'a' && event->ie_code <= (int) 'z')
		event->ie_code -= 0x60;
	    else if (event->ie_code >= (int) 'A' && event->ie_code <= (int) 'Z')
		event->ie_code -= 0x40;
	} else if (caps_choice_value == 1 && /* Caps Lock choice is on */
	    ((event->ie_code >= 'a' && event->ie_code <= 'z') ||
	     (event->ie_code >= 0xE0 && event->ie_code <= 0xEF) ||
	     (event->ie_code >= 0xF1 && event->ie_code <= 0xF6) ||
	     (event->ie_code >= 0xF8 && event->ie_code <= 0xFE))) {
	    /* Use uppercase version of character */
	    event->ie_code -= 0x20;
	}
    } else if (!strncmp("   ", keycap, 3)) {
	event->ie_code = SPACE;
    } else if (!strncmp("Del", keycap, 3)) {
	event->ie_code = DELETE;
    } else if (!strncmp("Esc", keycap, 3)) {
	event->ie_code = ESCAPE;
    } else if (!strncmp("Back Space", keycap, 10)) {
	event->ie_code = BACKSPACE;
    } else if (!strncmp("Tab", keycap, 3)) {
	event->ie_code = TAB;
    } else if (!strncmp(" Return", keycap, 6)) {
	event->ie_code = RETURN;
    } else if (!strncmp("LnFd", keycap, 4)) {
	event->ie_code = LINEFEED;
    }
    event->ie_shiftmask = 0;
    if (shifted == 1)
	event->ie_shiftmask |= ShiftMask;
    if (ctrl_choice_value == 1)
	event->ie_shiftmask |= ControlMask;
    if (meta_choice_value == 1)
	event->ie_shiftmask |= xv_get(server_public,SERVER_META_MOD_MASK);
    if (event->ie_code) {
	event_set_down(event);
	send_translated_keysym(event,event->ie_code,KeyPress,event->ie_shiftmask);
	event_set_up(event);
	send_translated_keysym(event,event->ie_code,KeyRelease,event->ie_shiftmask);
    }
    xv_set(item,
	PANEL_NOTIFY_STATUS, XV_ERROR,   /* don't take down Properties Frame */
	0);
}


/*ARGSUSED*/
static void
caps_proc(item, value, event)
    Panel_item	    item;
    int		    value;
    Event	   *event;
{
    caps_choice_value = value;
}


/*ARGSUSED*/
static void
ctrl_proc(item, value, event)
    Panel_item	    item;
    int		    value;	/* 0 (unshifted) or 1 (shifted) */
    Event	   *event;
{
    ctrl_choice_value = value;
}



/*
 * Create Function Keys Frame 
 */
static void
create_fk_frame()
{
    char	    buffer[4];
    int		    fn_key_width;
    Rect	    frame_rect;
    int		    i;
    Xv_Window	    root_window;
    Rect	    root_window_rect;


    define_atoms();
    fk_frame = xv_create(base_frame, FRAME_PROPS,
                         FRAME_LABEL, "Function Keys",
                         0);
    xv_set(xv_get(fk_frame, FRAME_PROPS_PANEL),
           XV_WIDTH, 1,
           XV_HEIGHT, 1,
           XV_SHOW, FALSE,
           0);

    fixed12_font = xv_find(xv_default_server, FONT,
                         FONT_FAMILY, FONT_FAMILY_DEFAULT_FIXEDWIDTH,
                         FONT_STYLE, FONT_STYLE_BOLD,
                         FONT_SIZE, 12,
                         0);
    fk_panel = xv_create(fk_frame, PANEL,
                         PANEL_LAYOUT, PANEL_VERTICAL,
                         XV_HELP_DATA,"vkbd:generic",
                         XV_FONT, fixed12_font,
                         XV_WIDTH,1100,    
                         XV_X, 0,
                         XV_Y, 0,
                         PANEL_ITEM_X_GAP,GAP,
                         PANEL_ITEM_Y_GAP,GAP,
                         0);
    fn_key_width = FN_KEY_MAX_STR_SIZE *
	(int) xv_get(fixed12_font, FONT_DEFAULT_CHAR_WIDTH);
    for (i=0; i < NBR_FN_KEYS; i++) {
	sprintf(buffer, "F%d", i+1);
	xv_create(fk_panel, PANEL_MESSAGE,
		  PANEL_LABEL_STRING, buffer,
		  PANEL_NEXT_COL, -1, 
		  0);
	fn_key[i] = xv_create(fk_panel, PANEL_BUTTON,
			      PANEL_LABEL_STRING, " ",
			      PANEL_LABEL_WIDTH, fn_key_width,
			      PANEL_NOTIFY_PROC, fk_btn_notify_proc,
			      XV_KEY_DATA, key_top_key, KEY_TOP(i+1),
			      0);
    }

    window_fit(fk_panel);
    window_fit(fk_frame);


    /* Center the frame at the bottom of the screen */
    XSync((Display *)XV_DISPLAY_FROM_WINDOW(fk_frame), False);
    root_window = xv_get(fk_frame, XV_ROOT);
    root_window_rect = *(Rect *) xv_get(root_window, XV_RECT);
#ifdef FRAME_GET_RECT_FIXED
    frame_get_rect(fk_frame, &frame_rect);
#else
    /* There is no handshake between the application and the Window Manager
     * to tell us that the frame has been resized.  Doing a frame_get_rect
     * before the resize is complete will yield the incorrect rect.
     */
    rect_construct(&frame_rect, 0, 0, 1111, 90)
#endif
    frame_rect.r_top = root_window_rect.r_height - frame_rect.r_height;
    frame_rect.r_left = (root_window_rect.r_width - frame_rect.r_width)/2;
    frame_set_rect(fk_frame, &frame_rect);
}


/*
 * Create Virtual Keyboard Frame
 */
static void
create_vkbd_frame()
{
    int		    choice_nbr;
    int		    col;
    char	    frame_label[32];
    int		    i;
    int		    inactive;
    char	   *keycap;
    int		    meta_nbr;
    int		    row;
    int		    x;
#ifdef KEYPADS
    int		    vkbd_panel_height;
    Panel	    l_panel;	/* left keypad panel */
    Panel	    r_panel;	/* right keypad panel */
#endif /* KEYPADS */

    sprintf(frame_label, "Virtual Keyboard - (%s)",
	    language_labels[current_language]);
    vkbd_frame = xv_create(base_frame, FRAME_PROPS,
	FRAME_LABEL, frame_label,
	XV_WIDTH, 900,  /* enough to accomodate the 3 panels */
	0);

    xv_set(xv_get(vkbd_frame, FRAME_PROPS_PANEL),
	   XV_WIDTH, 1,
	   XV_HEIGHT, 1,
	   XV_SHOW, FALSE,
	   0);

    fixed14_font = xv_find(xv_default_server, FONT,
                          FONT_FAMILY, FONT_FAMILY_DEFAULT_FIXEDWIDTH,
                          FONT_STYLE, FONT_STYLE_BOLD,
                          FONT_SIZE, 14,
                          0);
    vkbd_panel = xv_create(vkbd_frame, PANEL,
			   XV_FONT, fixed14_font,
                           XV_HELP_DATA,"vkbd:generic",
			   XV_X, 0,
			   XV_Y, 0,
			   0);


#ifdef KEYPADS
    /*************************************
     * Left keypad
     *************************************/
    l_panel = xv_create(vkbd_frame, PANEL,
		      XV_FONT, fixed14_font,
		      0);

    for (row=0; row < NBR_ROWS; row++) {
	for (col=0; col < 2; col++) {
	    keycap = left_keypad[row][col];
	    if (!keycap)
		continue;
	    xv_create(l_panel, PANEL_BUTTON,
		      PANEL_LABEL_STRING, keycap,
                      PANEL_ACCEPT_KEYSTROKE,FALSE,
		      PANEL_NOTIFY_PROC, button_notify_proc,
		      (col == 0 ? PANEL_NEXT_ROW : 0), -1,
		      0);
	    if (row == 0 && col == 0)
		xv_set(l_panel,
		       PANEL_ITEM_X_GAP, GAP,
		       PANEL_ITEM_Y_GAP, GAP,
		       0);
	}
    }

    window_fit_width(l_panel);
#endif /* KEYPADS */

    /*************************************
     * Alphanumeric keys
     *************************************/
    choice_nbr = 0;
    meta_nbr = 0;
    i = 0;
    for (row=0; row < NBR_ROWS; row++) {
	x = PANEL_ITEM_X_START;
	for (col=0; col < NBR_COLS; col++) {
	    keycap = keyboards[kbd_language][shifted][row][col];
	    if (keycap && !strncmp("Shift", keycap, 5)) {
		shift_choice[choice_nbr] = xv_create(vkbd_panel, PANEL_TOGGLE,
		    PANEL_CHOICE_STRINGS, keycap, 0,
		    PANEL_NOTIFY_PROC, shift_proc,
		    XV_X, x,
		    (col == 0 ? PANEL_NEXT_ROW : 0), -1,
		    0);
		x += (int) xv_get(shift_choice[choice_nbr], XV_WIDTH) + GAP;
		choice_nbr++;
	    } else if (keycap && !strncmp("Caps", keycap, 4)) {
		caps_choice = xv_create(vkbd_panel, PANEL_TOGGLE,
		    PANEL_CHOICE_STRINGS, keycap, 0,
		    PANEL_NOTIFY_PROC, caps_proc,
		    XV_X, x,
		    (col == 0 ? PANEL_NEXT_ROW : 0), -1,
		    0);
		x += (int) xv_get(caps_choice, XV_WIDTH) + GAP;
	    } else if (keycap && !strcmp("<>", keycap)) {
		meta_choice[meta_nbr] = xv_create(vkbd_panel, PANEL_TOGGLE,
		    PANEL_CHOICE_STRINGS, keycap, 0,
		    PANEL_NOTIFY_PROC, meta_proc,
		    XV_X, x,
		    (col == 0 ? PANEL_NEXT_ROW : 0), -1,
		    0);
		x += (int) xv_get(meta_choice[meta_nbr], XV_WIDTH) + GAP;
		meta_nbr++;
	    } else if (keycap && !strncmp("AltG", keycap, 4)) {
		altg_choice = xv_create(vkbd_panel, PANEL_TOGGLE,
		    PANEL_CHOICE_STRINGS, keycap, 0,
		    PANEL_NOTIFY_PROC, altg_proc,
		    XV_X, x,
		    (col == 0 ? PANEL_NEXT_ROW : 0), -1,
		    0);
		x += (int) xv_get(altg_choice, XV_WIDTH) + GAP;
	    } else if (keycap &&
		       (!strncmp("Control", keycap, 7) ||
			!strncmp("Ctrl", keycap, 4))) {
		ctrl_choice = xv_create(vkbd_panel, PANEL_TOGGLE,
		    PANEL_CHOICE_STRINGS, keycap, 0,
		    PANEL_NOTIFY_PROC, ctrl_proc,
		    XV_X, x,
		    (col == 0 ? PANEL_NEXT_ROW : 0), -1,
		    0);
		x += (int) xv_get(ctrl_choice, XV_WIDTH) + GAP;
	    } else {
		if (keycap && !strcmp(keycap, "Compose")) {
		    inactive = TRUE;
		    keycap = "       ";
		} else if (keycap && !strcmp(keycap, "Alt")) {
		    inactive = TRUE;
		    keycap = "   ";
		} else
		    inactive = i < 12;
		btn[i] = xv_create(vkbd_panel, PANEL_BUTTON,
		    PANEL_LABEL_STRING, keycap ? keycap : "  ",
		    PANEL_NOTIFY_PROC, button_notify_proc,
		    XV_SHOW, keycap ? TRUE : FALSE,
                    PANEL_ACCEPT_KEYSTROKE, FALSE,
		    XV_X, x,
		    (col == 0 ? PANEL_NEXT_ROW : 0), -1,
		    0);
                xv_set(btn[i],PANEL_INACTIVE, inactive,0);
		if (keycap)
		    x += (int) xv_get(btn[i], XV_WIDTH) - 1 + GAP;
		i++;
	    }
	    if (row == 0 && col == 0)
		xv_set(vkbd_panel,
		       PANEL_ITEM_X_GAP, GAP,
		       PANEL_ITEM_Y_GAP, GAP,
		       0);
	}
    }
    nbr_vkbd_btns = i;

    window_fit(vkbd_panel);


#ifdef KEYPADS
    /*************************************
     * Right keypad
     *************************************/
    r_panel = xv_create(vkbd_frame, PANEL,
		        XV_FONT, fixed14_font,
		        0);

    for (row=0; row < NBR_ROWS; row++) {
	for (col=0; col < 4; col++) {
	    keycap = right_keypad[row][col];
	    if (!keycap)
		continue;
	    xv_create(r_panel, PANEL_BUTTON,
		      PANEL_LABEL_STRING, keycap,
                      PANEL_ACCEPT_KEYSTROKE, FALSE,
		      PANEL_NOTIFY_PROC, button_notify_proc,
		      (col == 0 ? PANEL_NEXT_ROW : 0), -1,
		      0);
	    if (row == 0 && col == 0)
		xv_set(r_panel,
		       PANEL_ITEM_X_GAP, GAP,
		       PANEL_ITEM_Y_GAP, GAP,
		       0);
	}
    }

    window_fit_width(r_panel);

    vkbd_panel_height = (int) xv_get(vkbd_panel, XV_HEIGHT);
    xv_set(l_panel, XV_HEIGHT, vkbd_panel_height, 0);
    xv_set(r_panel, XV_HEIGHT, vkbd_panel_height, 0);
#endif /* KEYPADS */

    window_fit(vkbd_frame);
}


static void
define_atoms()
{
    int		    i;

    for (i= 2; i < NBR_ATOMS; i++)
	atom[i] = XInternAtom(display, property_name[i], False);
}


/*ARGSUSED*/
static void
fk_btn_notify_proc(item, event)
    Panel_item	    item;
    Event	   *event;
{
    int		    i;
    char	   *label;
    int		    new_language;
    Panel_item	    old_default_item;
    short	    original_action;
#ifdef REMAP_MODIFIER_KEYS
    int		    col;
    XModifierKeymap *modifier_map;
    KeyCode	   *keycodes;
    KeySym	   *keysym_map;
    KeySym	   *keysyms;
    int		    row;
#endif /* REMAP MODIFIER_KEYS */

    if (event_action(event) == ACTION_SELECT)
	xv_set(item,
	    PANEL_NOTIFY_STATUS, XV_ERROR,/* don't take down Properties Frame */
	    0);
	
    original_action = event_action(event);
    event->ie_code = (int) xv_get(item, XV_KEY_DATA, key_top_key);
    if (languages_mode) {
	/* Languages mode */
	switch (event->ie_code) {
	  case KEY_TOP(10):
	    if (event_is_down(event))
		break;

            /* Part of Delayed realisation. The Vkbd frame is created
             * when it is needed 
             */

            if (!vkbd_fr_created){
               create_vkbd_frame();
               vkbd_fr_created = 1;
            }
               
	    xv_set(vkbd_frame, XV_SHOW, TRUE, 0);
	    break;
	  case KEY_TOP(11):
	    if (event_is_down(event))
		break;
	    /* Change the keyboard to the desired language */
	    printf("vkbd: Setting keyboard to %s\n",
		language_labels[current_language]);
	    kbd_language = current_language;
	    kbd_F1_language = F1_language;
#ifdef REMAP_MODIFIER_KEYS   /* This code doesn't work (yet) */
	    /* Translate the Modifier Keycode Map into keysyms */
	    modifier_map = XGetModifierMapping(display);
	    keycodes = modifier_map->modifiermap;
	    keysym_map = (KeySym *)
		malloc(8*modifier_map->max_keypermod*sizeof(KeySym));
	    keysyms = keysym_map;
	    for (row = 0; row <= 7; row++) {
		for (col = 0; col < modifier_map->max_keypermod; col++) {
		    *keysyms++ = XKeycodeToKeysym(display, *keycodes++, 0);
		}
	    }
#endif /* REMAP_MODIFIER_KEYS */
	    /* Change all the keycode to keysym bindings to the new language */
	    XChangeKeyboardMapping(display, FIRST_KEYCODE, KEYSYMS_PER_KEYCODE,
		&(keysyms_table[kbd_language][0][0]), NBR_KEYCODES);
#ifdef REMAP_MODIFIER_KEYS   /* This code doesn't work (yet) */
	    /* Translate the Keysym Modifier Map back into keycodes */
	    keycodes = modifier_map->modifiermap;
	    keysyms = keysym_map;
	    for (row = 0; row <= 7; row++) {
		for (col = 0; col < modifier_map->max_keypermod; col++) {
		    *keycodes++ = XKeysymToKeycode(display, *keysyms++);
		}
	    }
	    /* Set the new Modifier Keycode Map */
	    XSetModifierMapping(display, modifier_map);
	    free(keysym_map);
#endif /* REMAP_MODIFIER_KEYS */
	    break;
	  case KEY_TOP(12):
	    if (event_is_down(event))
		break;
	    /* Show the next (or first if showing last) language set */
	    F1_language += NBR_FN_KEYS - 3;
	    if (F1_language > NBR_LANGUAGES)
		F1_language = 0;
	    for (i = 0; i < NBR_FN_KEYS - 3; i++) {
		if (F1_language + i < NBR_LANGUAGES)
		    label = languages[F1_language+i];
		else
		    label = " ";
		xv_set(fn_key[i],
		       PANEL_LABEL_STRING, label,
		       0);
	    }
	    item = fn_key[0];
	    /* ... fall through to "default" */
	  default:
	    new_language = F1_language +
		(int) xv_get(item, XV_KEY_DATA, key_top_key) - KEY_TOP(1);
	    if (new_language >= NBR_LANGUAGES ||
		    /* blank function key (button) selected */
		new_language == current_language)
		    /* current language selected */
		break;
	    current_language = new_language;
	    /* Language selected: set default item to this item */
	    old_default_item = xv_get(fk_panel, PANEL_DEFAULT_ITEM);
	    xv_set(fk_panel, PANEL_DEFAULT_ITEM, item, 0);
	    if (old_default_item)
		panel_paint(old_default_item, PANEL_CLEAR);
	    panel_paint(item, PANEL_CLEAR);
	    /* Set Virtual Keyboard to the specified language */
	    set_vkbd_labels(TRUE);
	    break;
	}
    } else {
	if (original_action == ACTION_SELECT) {
	    send_translated_keysym(event,event->ie_code,KeyPress,event->ie_shiftmask);
	}
	send_translated_keysym(event,event->ie_code,KeyRelease,event->ie_shiftmask);
    }
}


/*ARGSUSED*/
static void
frame_event_proc(window, event, arg)
    Xv_window	    window;
    Event	   *event;
    Notify_arg      arg;
{
    int		    col;
    char	   *cp;		/* pointer to char data */
    int		   *idata;	/* pointer to int data */
    int		    fn_key_index;
    int		    i;
    short	    kbd_id;
    String	    keycap;
    Atom	    msg_type;
    int		    shift;
    int		    row;
    long	   *ldata;	/* pointer to short data */
    static int     alt_modmask;
    static int     meta_modmask;
    unsigned int   *key_map;
    unsigned int    key_value;
    unsigned int    key_sym;
    int             keyboard_key;
    static unsigned long TimeStamp = 0;

    if (event_action(event) != WIN_CLIENT_MESSAGE)
	return ;

    msg_type = (Atom) xv_get(event_window(event), WIN_MESSAGE_TYPE);
    if (msg_type == atom[OL_ENTER_LANG_MODE]) {
       /* printf("Debug: entering LANG_MODE\n"); */
       if (!fk_frame_created) {

          /* Part of delayed realisation. The function keys
           * panel is created , only if it has not been created
           */

           fk_frame_created = 1;
           create_fk_frame();	 /* create Function Keys frame */
           xv_set(fk_frame, XV_SHOW,TRUE,0);
       }
        load_languages();
	languages_mode = TRUE;
	fk_frame_mapped = (int) xv_get(fk_frame, XV_SHOW);
        /* frame pkg might have iconified fk_frame so uniconify before
	   showing */
        xv_set(fk_frame,FRAME_CLOSED, FALSE, NULL);
	xv_set(fk_frame, XV_SHOW, TRUE, 0);
	return ;
      } else if (msg_type == atom[OL_EXIT_LANG_MODE]) {
	/* printf("Debug: exiting LANG_MODE\n"); */
	/* Restore Function Key button labels */
	restore_fn_keys();
	if (current_language != kbd_language) {
	    /* Switch to physical keyboard language */
	    current_language = kbd_language;
	    F1_language = kbd_F1_language;
	    /* Load current language into Virtual Keyboard window */
	    set_vkbd_labels(TRUE);
	}
	/* Exit languages mode and unmap windows (if unpinned) */
	languages_mode = FALSE;
	fk_frame_mapped = (int) xv_get(fk_frame, XV_SHOW);
        if (xv_get(fk_frame,FRAME_CMD_PIN_STATE) != FRAME_CMD_PIN_IN) {
	  xv_set(fk_frame, XV_SHOW, FALSE, 0);
	}
        if (vkbd_fr_created)
	   xv_set(vkbd_frame, XV_SHOW, FALSE, 0);
	return ;
    } else if (msg_type == atom[OL_SOFTKEY_LABELS]) {
	if (languages_mode)
	    return ;
	/* The data is a window xid that has the OL_LABEL_HOLDER property */
	idata = (int *) xv_get(event_window(event), WIN_MESSAGE_DATA);
        /* Store the Timestamp if latest and discard if older */
        if ((unsigned long)idata[1] < TimeStamp)
           return ; 
        else
           TimeStamp = (unsigned long) idata[1];
	if (idata[0]) {
	    /* Get the Soft Function Key labels.  The data returned is a
	     * null-terminated string.  Each Function Key label is separated
	     * by a newline ('\n').
	     */
	    cp = get_fn_key_labels(idata[0]);
	    cp = strtok(cp, "\n");
	} else {
	    cp = NULL;	/* no labels */
	}

	for (i = 0; i < NBR_FN_KEYS; i++) {
	    xv_set(fn_key[i], PANEL_LABEL_STRING, cp ? cp : " ", 0);
	    if (cp)
		cp = strtok(NULL, "\n");
	}
	return ;
    } else if (msg_type == atom[OL_SHOW_SFK_WIN]) {
        if (!fk_frame_created) {

         /* Part of delayed realisation. The function keys
          * panel is created , only if it has not been created
          */

         fk_frame_created = 1;
         create_fk_frame();	 /* create Function Keys frame */
        }
        xv_set(fk_frame, XV_SHOW,TRUE,0);
	return ;
    } else if (msg_type == atom[OL_TRANSLATE_KEY]) {

        /* ldata[0] - keysym
         * ldata[1] - keyevent->type 
         * ldata[2] - keyevent->state
         */

	ldata = (long *) xv_get(event_window(event), WIN_MESSAGE_DATA);        

	/* Create a XView event from an X Keyevent */

	key_map      = (unsigned int *) xv_get(server,SERVER_XV_MAP);
	key_sym = XKeycodeToKeysym(display,(KeyCode) ldata[0],0);
	key_value = key_sym;
	keyboard_key = ((key_sym & KEYBOARD_KYSM_MASK) == KEYBOARD_KYSM);
	if (keyboard_key)
	  key_value = ((key_map[ key_sym & 0xFF] == key_sym) ||
		       (!key_map[ key_sym & 0xFF])) ? key_value :
			 key_map[ key_sym & 0xFF];


	event_set_id(event,key_value);
	event_set_flags(event, ldata[1]);
        alt_modmask  = (int) xv_get(server,SERVER_ALT_MOD_MASK); 
        meta_modmask = (int) xv_get(server,SERVER_META_MOD_MASK); 
        SET_SHIFTS(event,ldata[2],alt_modmask,meta_modmask);


	if (event_is_key_top(event)) {
	    fn_key_index = event_id(event) - KEY_TOP(1);
	    fk_btn_notify_proc(fn_key[fn_key_index], event);
	} else if (event_id(event) <= (short) ' ') {
	    /* Control character: no translation done */
	  send_translated_keysym(event,key_sym,ldata[1],ldata[2]);
	  return ;
	} else if (event_id(event) <= ISO_LAST) {
	    for (row = 0; row < NBR_ROWS; row++) {
		for (col = 0; col < NBR_COLS; col++) {
		    for (shift = 0; shift <= 1; shift++) {
			keycap = keyboards[kbd_language][shift][row][col];
			if (keycap && strlen(keycap) == 2) {
			    kbd_id = (short) keycap[altg_down] & 0xFF;
			    if (kbd_id == (short) ' ')
				kbd_id = (short) keycap[0] & 0xFF;
			    if (kbd_id == event_id(event)) {
				keycap =
				  keyboards[current_language][shift][row][col];
				event->ie_code =
				    (short) keycap[altg_down] & 0xFF;
				if (event->ie_code == (short) ' ')
				    event->ie_code =
					(short) keycap[0] & 0xFF;
				if (caps_lock && /* Caps Lock key is on */
				    ((event->ie_code >= 'a' &&
				      event->ie_code <= 'z') ||
				     (event->ie_code >= 0xE0 &&
				      event->ie_code <= 0xEF) ||
				     (event->ie_code >= 0xF1 &&
				      event->ie_code <= 0xF6) ||
				     (event->ie_code >= 0xF8 &&
				      event->ie_code <= 0xFE))) {
				    /* Use uppercase version of character */
				    event->ie_code -= 0x20;
				}

				send_translated_keysym(event,event->ie_code,ldata[1],ldata[2]);
				return ;
			    }
			}
		    }
		}
	    }
	} else if (event_action(event) == SHIFT_ALTG) {
	    altg_down = event_is_down(event);
	} else if (event_action(event) == SHIFT_CAPSLOCK && event_is_down(event)) {
	    caps_lock = !caps_lock;
	}
	return ;
    }
}


/*
 * Get the string of chars (soft key labels) stored off the property
 * "OL_LABEL_HOLDER" from the specified window.  Return NULL if it is not
 * available.
 */
static char     *
get_fn_key_labels(xid)
    Window	    xid;
{
    XTextProperty   text_prop;
    Atom            label_prop;
    
    label_prop = XInternAtom(display, "_OL_LABEL_HOLDER", False);
    if (XGetTextProperty(display, xid, &text_prop, label_prop))
	return ((char *) text_prop.value);
    else
	return NULL;
}


static void
init_seln(dpy, window)
    Display        *dpy;
    Xv_Window	    window;
{
    Atom            atom_start_sftkeys;
    int		   *data;
    Window          seln_owner;
    Window          xid;

    /* See if there is a clone running */

    atom_start_sftkeys = XInternAtom(dpy, "_OL_SOFT_KEYS_PROCESS", False);

    seln_owner = XGetSelectionOwner(dpy, atom_start_sftkeys);
    if (seln_owner != None) {

	/*
	 * A Function Keys process is already running.  Send a client message
	 * requesting that the Function Keys window be mapped, and exit.
	 */
	data = (int *) calloc(2, sizeof(int));
	xv_send_message(window, seln_owner, property_name[OL_SHOW_SFK_WIN],
			32, (Xv_opaque *)data, 8);
	exit(1);

    } else {

	/*
	 * No Function Keys process is running.  Own the selection and continue.
	 */
        atom[OL_ENTER_LANG_MODE] = XInternAtom(dpy, property_name[OL_ENTER_LANG_MODE], 
                                              False);
        atom[OL_SHOW_SFK_WIN] = XInternAtom(dpy,property_name[OL_SHOW_SFK_WIN],
                                            False);
	xid = (Window) xv_get(window, XV_XID);
	XSetSelectionOwner(dpy, atom_start_sftkeys, xid, CurrentTime);
	if (XGetSelectionOwner(dpy, atom_start_sftkeys) != xid)
	    xv_error(NULL,
		ERROR_LAYER, ERROR_PROGRAM,
		ERROR_SEVERITY, ERROR_NON_RECOVERABLE,
		ERROR_STRING, "Cannot acquire selection ownership",
		0);
	return;
    }
}


static void
load_languages()
{
    int		    i;
    char	   *label;
    Panel_item	    old_default_item;


    /* Load current set of languages into Function Key buttons */

    for (i = 0; i < NBR_FN_KEYS - 3; i++) {
	/* Save current contents */
	fn_key_str[i] = strdup((char *) xv_get(fn_key[i],
	    PANEL_LABEL_STRING));
	/* Store new contents */
	if (F1_language + i < NBR_LANGUAGES)
	    label = languages[F1_language+i];
	else
	    label = " ";
	xv_set(fn_key[i], PANEL_LABEL_STRING, label, 0);
    }
    fn_key_str[i] = strdup((char *) xv_get(fn_key[i],
	PANEL_LABEL_STRING));
    xv_set(fn_key[i++], PANEL_LABEL_STRING, "Show", 0);
    fn_key_str[i] = strdup((char *) xv_get(fn_key[i],
	PANEL_LABEL_STRING));
    xv_set(fn_key[i++], PANEL_LABEL_STRING, "Set", 0);
    fn_key_str[i] = strdup((char *) xv_get(fn_key[i],
	PANEL_LABEL_STRING));
    xv_set(fn_key[i], PANEL_LABEL_STRING, "More", 0);
    old_default_item = xv_get(fk_panel, PANEL_DEFAULT_ITEM);
    xv_set(fk_panel,
	   PANEL_DEFAULT_ITEM, fn_key[current_language - F1_language],
	   0);
    if (old_default_item)
	panel_paint(old_default_item, PANEL_CLEAR);
    panel_paint(fn_key[current_language - F1_language], PANEL_CLEAR);
}


/*ARGSUSED*/
static void
meta_proc(item, value, event)
    Panel_item	    item;
    int		    value;
    Event	   *event;
{
    int		    i;

    if (meta_choice_value == value)
	return;
    meta_choice_value = value;
    if (meta_choice[0] == item)
	i = 1;
    else
	i = 0;
    xv_set(meta_choice[i], PANEL_VALUE, value, 0);
}


static void
restore_fn_keys()
{
    int		    i;
    Panel_item	    old_default_item;


    if (fn_key_str[2]) {

          /* Due to focus problems, there is a possiblity that
           * the labels will not get blanked out and might
           * the inherit the Language labels permanently
           * This hack  will avoid it
           */

        if (!strcmp ("French",fn_key_str[2])) {
	    for (i = 0; i < NBR_FN_KEYS; i++) {
		fn_key_str[i] = strdup(" ");
	    }
	}
    }
        
    for (i = 0; i < NBR_FN_KEYS; i++) {
	if (fn_key_str[i]) {
	    xv_set(fn_key[i], PANEL_LABEL_STRING, fn_key_str[i], 0);
	    free(fn_key_str[i]);
	    fn_key_str[i]=NULL;
	}
    }
    old_default_item = xv_get(fk_panel, PANEL_DEFAULT_ITEM);
    xv_set(fk_panel, PANEL_DEFAULT_ITEM, NULL, 0);
    if (old_default_item)
	panel_paint(old_default_item, PANEL_CLEAR);
}


static void
send_translated_keysym(event,keysym,xevent_type,xevent_state)
     Event          *event;
     KeySym         keysym;
     long           xevent_type;
     long           xevent_state;
{
    int		    revert_to;
    Window	    target_window;
    long            ldata[3];

    /* Send the Translated Key Client Message to the Input Focus window.
     * The data contains 3 longs: KeySym, KeyPress/KeyRelease and
     * XEvent_State  .
     */
    XGetInputFocus(display, &target_window, &revert_to);
    
    ldata[0] = keysym;
    ldata[1] = xevent_type;
    ldata[2] = xevent_state;
    
    xv_send_message(event_window(event),	/* window */
		    target_window,		/* addressee */
		    property_name[OL_TRANSLATED_KEY],	/* msg_type */
		    32,				/* format */
		    (Xv_opaque *) ldata,	/* data */
		    12);	    	       /* length */
}


/*ARGSUSED*/
static void
set_vkbd_labels(new_language)
    int		    new_language;	/* TRUE or FALSE */
{
    int		    choice_nbr;
    int		    col;
    int		    i;
    int		    inactive;
    char	    frame_label[32];
    char	   *keycap;
    int		    meta_nbr;
    int		    row;
    int		    x;


    if (!vkbd_fr_created){
       return;
    }
    if (new_language) {
	sprintf(frame_label, "%s Virtual Keyboard",
		language_labels[current_language]);
	xv_set(vkbd_frame,
	       FRAME_LABEL, frame_label,
	       0);
	/* Hide all buttons to prevent buttons from being partially cleared */
	for (i=0; i < nbr_vkbd_btns; i++)
	    xv_set(btn[i], XV_SHOW, FALSE, 0);
	xv_set(altg_choice, XV_SHOW, FALSE, 0);
	xv_set(caps_choice, XV_SHOW, FALSE, 0);
	xv_set(ctrl_choice, XV_SHOW, FALSE, 0);
	xv_set(meta_choice[0], XV_SHOW, FALSE, 0);
	xv_set(meta_choice[1], XV_SHOW, FALSE, 0);
	xv_set(shift_choice[0], XV_SHOW, FALSE, 0);
	xv_set(shift_choice[1], XV_SHOW, FALSE, 0);
    }

    /* Relayout all items */
    choice_nbr = 0;
    meta_nbr = 0;
    i = 0;
    for (row=0; row < NBR_ROWS; row++) {
	x = PANEL_ITEM_X_START;
	for (col=0; col < NBR_COLS; col++) {
	    keycap = keyboards[current_language][shifted][row][col];
	    if (keycap && !strncmp("Shift", keycap, 5)) {
		/* Shift */
		xv_set(shift_choice[choice_nbr],
		       PANEL_CHOICE_STRINGS, keycap, 0,
		       XV_SHOW, TRUE,
		       XV_X, x,
		       0);
		x += (int) xv_get(shift_choice[choice_nbr], XV_WIDTH) + GAP;
		choice_nbr++;
	    } else if (keycap && !strncmp("Caps", keycap, 4)) {
		/* Caps Lock
		 *
		 * Note: The Caps Lock key exists on different rows for
		 *       different keyboards.  Thus, we must set XV_Y.
		 */
		 xv_set(caps_choice,
		       PANEL_CHOICE_STRINGS, keycap, 0,
		       XV_SHOW, TRUE,
		       XV_X, x,
		       XV_Y, (int) xv_get(btn[i], XV_Y),
		       0);
		x += (int) xv_get(caps_choice, XV_WIDTH) + GAP;
	    } else if (keycap && !strcmp("<>", keycap)) {
		/* Meta */
		xv_set(meta_choice[meta_nbr],
		       PANEL_CHOICE_STRINGS, keycap, 0,
		       XV_SHOW, TRUE,
		       XV_X, x,
		       0);
		x += (int) xv_get(meta_choice[meta_nbr], XV_WIDTH) + GAP;
		meta_nbr++;
	    } else if (keycap && !strncmp("AltG", keycap, 4)) {
		/* Alt Graph */
		xv_set(altg_choice,
		       PANEL_CHOICE_STRINGS, keycap, 0,
		       XV_SHOW, TRUE,
		       XV_X, x,
		       0);
		x += (int) xv_get(altg_choice, XV_WIDTH) + GAP;
	    } else if (keycap &&
		       (!strncmp("Control", keycap, 7) ||
			!strncmp("Ctrl", keycap, 4))) {
		/* Control
		 *
		 * Note: The Control key exists on different rows for
		 *       different keyboards.  Thus, we must set XV_Y.
		 */
		xv_set(ctrl_choice,
		       PANEL_CHOICE_STRINGS, keycap, 0,
		       XV_SHOW, TRUE,
		       XV_X, x,
		       XV_Y, (int) xv_get(btn[i], XV_Y),
		       0);
		x += (int) xv_get(ctrl_choice, XV_WIDTH) + GAP;
	    } else {
		if (keycap && !strcmp(keycap, "Compose")) {
		    inactive = TRUE;
		    keycap = "       ";
		} else if (keycap && !strcmp(keycap, "Alt")) {
		    inactive = TRUE;
		    keycap = "   ";
		} else
		    inactive = i < 12;
		/* Not a Shift or Alt Graph key */
		xv_set(btn[i],
		       PANEL_INACTIVE, inactive,
		       PANEL_LABEL_STRING, keycap ? keycap : "",
		       XV_SHOW, keycap ? TRUE : FALSE,
		       XV_X, x,
		       0);
		if (keycap)
		    x += (int) xv_get(btn[i], XV_WIDTH) - 1 + GAP;
		i++;
	    }
	}
    }
}


/*ARGSUSED*/
static void
shift_proc(item, value, event)
    Panel_item	    item;
    int		    value;	/* 0 (unshifted) or 1 (shifted) */
    Event	   *event;
{
    int		    i;

    if (shifted == value)
	return;
    shifted = value;
    set_vkbd_labels(FALSE);
    if (shift_choice[0] == item)
	i = 1;
    else
	i = 0;
    xv_set(shift_choice[i], PANEL_VALUE, value, 0);
}


main(argc, argv)
    int		    argc;
    char	  **argv;
{

    int      	vkbd_x_error_proc();

    xv_init(XV_INIT_ARGC_PTR_ARGV, &argc, argv,
            XV_X_ERROR_PROC,vkbd_x_error_proc, 0);
    


    /* Create a dummy base frame.  Making the Function Keys and
     * Virtual Keyboard windows subframes will prevent them from
     * being destroyed when they are unpinned.
     */

    base_frame = xv_create(NULL, FRAME,
                           WIN_CMD_LINE,-1,
                           WIN_EVENT_PROC,frame_event_proc, 0);
    server = XV_SERVER_FROM_WINDOW(base_frame);
    display = (Display *) xv_get(server, XV_DISPLAY);
    init_seln(display, base_frame);

/*EMPTY*/
    if ((argc > 1) && (strcmp (argv[1],"-nopopup") == 0));
    else {

       /* Part of delayed realisation. The function keys
        * panel is created , only when necessary
        */

       fk_frame_created = 1;
       create_fk_frame();	 /* create Function Keys frame  */
       xv_set(fk_frame, XV_SHOW,TRUE,0);
    }
    xv_set(server, SERVER_SYNC_AND_PROCESS_EVENTS, 0);
    XFlush(display);
    key_top_key = xv_unique_key();
    notify_start();  /* start the notifier */
    exit(0);
}


/*ARGSUSED*/
int
vkbd_x_error_proc(dpy,event)
    Display *dpy;
    XErrorEvent *event;

 {
   int i;

   if (event->request_code == 20) {

      /* This error in XGetTextProperty is generated
       * when the window sends a OL_SOFT_KEY_LABELS to vkbd
       * and dies immediately. Vkbd tries to get a property
       * from this window, and we reach here
       */
    for (i = 0; i < NBR_FN_KEYS; i++) 
	fn_key_str[i] = " ";
        

     return XV_OK;
   }
   else 
     return XV_ERROR;

 }
