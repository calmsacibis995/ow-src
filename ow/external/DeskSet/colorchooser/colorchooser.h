/*
 * @(#)colorchooser.h	1.1 11/9/90
 */

/*
 * props.h - declarations for external interfaces to the props program.
 */

#include <xview/xview.h>
#include <xview/panel.h>

#define BORDER_WIDTH	1
#define MGET(s)         ( char * )gettext( s )
#define MSGFILE         "SUNW_DESKSET_COLORCHOOSER"

#define	COLOR_PANEL 	0
#define	ICON_PANEL 	0
#define	MENU_PANEL 	1
#define	MISC_PANEL 	2
#define	MOUSE_SET_PANEL 3
#ifdef COLORTOOL
#define TOTAL_PANELS    0
#else
#define	TOTAL_PANELS	4	/* or 5 if color terminal */
#endif

typedef enum {
    D_number, D_string, D_boolean, D_nop
}           Deftype;

typedef struct {
    char       *class;
    Deftype     type;
    caddr_t     default_value;
    caddr_t     misc;
    int         change_mark;	/* TRUE or FALSE */
    Panel_message_item change_mark_item;
    Panel_item  panel_item;
}           Description;

extern Frame frame;
extern Panel panel;
extern int  showing_factory;
extern Display *dsp;
extern Drawable drawable;
extern char *strtok();

#ifdef OLD_WAY
extern Notify_value   panel_interpose_proc();
#endif

