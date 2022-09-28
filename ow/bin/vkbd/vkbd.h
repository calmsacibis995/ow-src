/*	@(#)vkbd.h 1.1 90/09/20 SMI	*/

typedef char * String;

typedef enum {
    REQUEST_LANGUAGE,
    REQUEST_FN_KEYS
} Request;

#define GAP 3

#define NBR_COLS 15
#define NBR_ROWS 6

#define NBR_SHIFTS 2

#define NBR_FN_KEYS 12
#define NBR_LANGUAGES 14
#define KEYSYMS_PER_KEYCODE 4
#define FIRST_KEYCODE 8
#define LAST_KEYCODE 132
#define NBR_KEYCODES LAST_KEYCODE - FIRST_KEYCODE + 1

#define SHOW_KEYSYM XK_F10
#define SET_KEYSYM XK_F11
#define MORE_KEYSYM XK_F12

#define LANGUAGE_KEYSYM XK_l
#define LANGUAGE_ID 'l'

#define FN_KEY_MAX_STR_SIZE 10

#define OL_ENTER_LANG_MODE	0
#define OL_SHOW_SFK_WIN		1
#define OL_EXIT_LANG_MODE	2
#define OL_SOFTKEY_LABELS	3
#define OL_TRANSLATE_KEY	4
#define OL_TRANSLATED_KEY	5
#define NBR_ATOMS		6

#define BACKSPACE 0xFF08
#define TAB 0x09
#define LINEFEED 0x0A
#define RETURN 0x0D
#define ESCAPE 0x1B
#define SPACE 0x20
#define DELETE 0xFFFF


#define SET_SHIFTS(e, state, metamask, altmask) \
        event_set_shiftmask(e, \
            (((state) & ShiftMask) ? SHIFTMASK : 0) | \
            (((state) & ControlMask) ? CTRLMASK : 0) | \
            (((state) & metamask) ? META_SHIFT_MASK : 0) | \
            (((state) & altmask) ? ALTMASK : 0) | \
            (((state) & Button1Mask) ? MS_LEFT_MASK : 0) | \
            (((state) & Button2Mask) ? MS_MIDDLE_MASK : 0) | \
            (((state) & Button3Mask) ? MS_RIGHT_MASK : 0))

#define DESET_SHIFTS(e, state, metamask, altmask) \
        event_set_shiftmask(e, \
            (((state) & SHIFTMASK) ? ShiftMask : 0) | \
            (((state) & CTRLMASK) ? ControlMask : 0) | \
            (((state) & META_SHIFT_MASK) ?  metamask : 0) | \
            (((state) & ALTMASK ) ? altmask: 0) | \
            (((state) & MS_LEFT_MASK ) ? Button1Mask: 0) | \
            (((state) & MS_MIDDLE_MASK ) ? Button2Mask: 0) | \
            (((state) & MS_RIGHT_MASK ) ? Button3Mask: 0))
