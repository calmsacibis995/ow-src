#ifndef	_NEWS_ENVIRON_H
#define _NEWS_ENVIRON_H


#ident "@(#)environ.h	1.2 06/11/93 NEWS SMI"


#include <Core/core.h>

/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */


extern TimeStamp default_key_repeat_thresh;
extern TimeStamp default_key_repeat_time;

#define DEFAULT_KEYBOARD_CLICK    0			/* percent of max */
#define DEFAULT_BELL              50			/* percent of max */
#define DEFAULT_BELL_PITCH        0			/* Hz */
#define DEFAULT_BELL_DURATION     100			/* 100 ms */
#define DEFAULT_AUTOREPEAT        TRUE			/* true */
#define DEFAULT_LEDS              0x0        		/* all off */
#define DEFAULT_KEY_REPEAT_THRESH {0, 0, 500000}	/* 1/2 second */
#define DEFAULT_KEY_REPEAT_TIME	  {0, 0,  50000}	/* 1/20 second */

struct kbd_environment {
    int				click;

    int				bell;
    int				bell_pitch;
    int				bell_duration;

    /*
     * NeWS does autorepeat in PostScript.
     */
    int				autoRepeat;		/* boolean */
    TimeStamp			key_repeat_thresh;
    TimeStamp			key_repeat_time;

    unsigned int		leds;			/* mask */

    struct object		keysymmap;		/* for NeWS */
    int          		minkeycode;		/* for NeWS */
    int                         maxkeycode;		/* for NeWS */

    struct object		modifiermap;		/* for NeWS */

    struct object		ledmap;			/* for NeWS */
    int                         ledcount;		/* for NeWS */

    struct object		repeatkeymap;		/* for NeWS */
};

#define DEFAULT_ACCEL_NUM	2		        /* NUM/DEN0M = 2 */
#define DEFAULT_ACCEL_DENOM	1
#define DEFAULT_THRESHOLD	15			/* 2000 pixels */
#define DEFAULT_COMPRESSION	TRUE			/* true */

struct ptr_environment {
    int accelNum;
    int accelDenom;
    int threshold;
    int do_compression;					/* boolean */
};

enum dev_type {
	keyboard_type,
	pointer_type
};

struct environment {
	char *devname;
	enum dev_type type;
	union {
		struct kbd_environment keybd;
		struct ptr_environment ptr;
	} env;
};



void	ringBell();
void	kbdCtrl();
extern struct environment *currentenvironment;
extern int collapsePtrMotion;

/*
 * Screen saver stuff - this stuff MUST match the #defines in the X11 .h files
 */
#define DontPreferBlanking      0
#define PreferBlanking          1
#define DefaultBlanking         2

#define DisableScreenSaver      0
#define DisableScreenInterval   0

#define DontAllowExposures      0
#define AllowExposures          1
#define DefaultExposures        2

/* for ForceScreenSaver */

#define ScreenSaverReset 0
#define ScreenSaverActive 1

#define INITIAL_SCREEN_SAVER_TIME     	{0, 0, 0}	/* turned off */
#define DEFAULT_SCREEN_SAVER_TIME     	{600, 0, 0}	/* 600 seconds */
#define DEFAULT_SCREEN_SAVER_INTERVAL 	{600, 0, 0}	/* 600 seconds */
#define DEFAULT_SCREEN_SAVER_BLANKING 1		/* PreferBlanking */
#define DEFAULT_SCREEN_SAVER_EXPOSURES 1	/* AllowExposures */

#define SCREEN_SAVER_ON   0
#define SCREEN_SAVER_OFF  1
#define SCREEN_SAVER_FORCER 2

#endif /* _NEWS_ENVIRON_H */
