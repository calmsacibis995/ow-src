#ident	"@(#)resources.c	26.82	96/10/31 SMI"

/*
 *      (c) Copyright 1989 Sun Microsystems, Inc.
 */

/*
 *      Sun design patents pending in the U.S. and foreign countries. See
 *      LEGAL_NOTICE file for terms of the license.
 */


#ifdef SYSV
#include <sys/types.h>
#endif
#include <ctype.h>
#include <errno.h>
#include <memory.h>
#include <stdio.h>
#include <string.h>
#include <sys/file.h>
#include <X11/Xos.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xresource.h>
#include <X11/keysym.h>
#include <X11/cursorfont.h>

#include "i18n.h"		/* needed by olgx.h */
#include <olgx/olgx.h>

#include "mem.h"
#include "olwm.h"
#include "ollocale.h"
#include "defaults.h"
#include "globals.h"
#include "resources.h"
#include "win.h"
#include "olcursor.h"
#include "events.h"
#include "error.h"

/* static data */

static Bool	updateWorkspaceBackground;
static Bool	forceKeyRegrab;

/* global data */
extern char **ArgVec;			/* Argument vector declared in main */

/* converters */

static Bool cvtWorkspaceStyle();
static Bool cvtBeepStatus();
static Bool cvtBoolean();
static Bool cvtClickTimeout();
static Bool cvtCursorFont();
static Bool cvtFocusStyle();
static Bool cvtFont();
#ifdef OW_I18N_L4
static Bool cvtFontSet();
#endif
static Bool cvtIconLocation();
static Bool cvtInteger();
static Bool cvtKey();
static Bool cvtMouseless();
static Bool cvtString();
#ifdef OW_I18N_L4
static Bool cvtWString();
#endif
static Bool cvtStringList();


/* internationalization stuff */

#ifdef OW_I18N_L3

static void GRVLCInit();
static Bool cvtOLLC();
static void setOLLCPosix();

#endif


/* updaters */

static void updButtonFont();
       void UpdFocusStyle();			/* yes, this one's global */
static void updGlyphFont();
static void updIconFont();
static void updIconLocation();
static void updMenuAccelerators();
static void upCtrlAltMetaKey();
static void updMouseless();
static void updString();
static void updStringList();
static void updSync();
static void updTextFont();
static void updTitleFont();
static void updWindow();
static void updWindowCacheSize();
static void updWorkspaceStyle();
static void updWorkspace();
static void updForeground();
static void updBackground();
static void updBorder();

/* 
 * In the resource table definition below, there's a "default" value that
 * represents the default string value of the the item. For one of the
 * items (meta key simulation -- CtrlAltMetaKey), this default is derived
 * at run time, depending upon what type of keyboard is attached to the
 * server. Hence, the need for the following cheese factor.
 */
static char MetaKeyDefault[6];		/* True or False */

/* resource table */

typedef struct _resourceitem {
    char *instance;
    char *class;
    char *defaultString;
    void *addr;
    Bool (*converter)();
    void (*updater)();
    unsigned long flags;
    XrmQuark instanceQ;
    XrmQuark classQ;
} ResourceItem;

/* values for flags field */

#define RI_IMMUTABLE	(1<<0)	    /* cannot be updated once initialized */
#define RI_LOCALE_DEP	(1<<1)	    /* is locale dependent */
#define RI_LAST_ITEM	(1<<2)	    /* this is the last item in the table */


/* values for ScanResourceItemTable()'s flags argument */

#define	SR_IMMUTABLE	(1<<0)      /* scan immutable items as well */
#define	SR_USE_DEFAULT	(1<<1)      /* If probe missed, use default */
#define	SR_UPDATING	(1<<2)      /* update resource DB */


/*
 * Locale Item Table.  This table contains resource items pertaining to locale
 * information.  It is separate from the Main Item Table (below), because
 * locale announcement may affect which files are read to obtain resources,
 * and also because some items in the main table may require an additional
 * locale-specific resource database probe.  No items in the locale item
 * table should require such a probe, i.e. no items in this table should have
 * the RI_LOCALE_DEP flag set.
 */

ResourceItem LocaleItemTable[] = {

#ifdef OW_I18N_L3
{   "basicLocale",		"BasicLocale",		NULL,
    &(GRV.lc_basic),		cvtOLLC,                NULL,
    0L },
{   "displayLang",		"DisplayLang",		NULL,
    &(GRV.lc_dlang),		cvtOLLC,                NULL,
    0L },
{   "inputLang",		"InputLang",		NULL,
    &(GRV.lc_ilang),		cvtOLLC,                NULL,
    0L },
{   "numericFormat",		"NumericFormat",	NULL,
    &(GRV.lc_numeric),		cvtOLLC,                NULL,
    0L },
{   "timeFormat",		"TimeFormat",		NULL,
    &(GRV.lc_time),		cvtOLLC,                NULL,
    0L },
#endif /* OW_I18N_L3 */

/* NOTE: the following item must always be the last. */

{   NULL,			NULL,			NULL,
    NULL,			NULL,			NULL,
    RI_LAST_ITEM }
};


/*
 * Main Item Table.  Contains resource items corresponding to all global 
 * variables, except those pertaining to locale information.
 */

ResourceItem MainItemTable[] = {

{   "titleFont",		"TitleFont",
    "-b&h-lucida-bold-r-normal-sans-*-120-*-*-*-*-*-*",
#ifdef OW_I18N_L4
    &(GRV.TitleFontSetInfo),	cvtFontSet,		updTitleFont,
#else
    &(GRV.TitleFontInfo),	cvtFont,		updTitleFont,
#endif
    RI_LOCALE_DEP },
{   "textFont",			"TextFont",
    "-b&h-lucida-medium-r-normal-sans-*-120-*-*-*-*-*-*",
#ifdef OW_I18N_L4
    &(GRV.TextFontSetInfo),	cvtFontSet,		updTextFont,
#else
    &(GRV.TextFontInfo),	cvtFont,		updTextFont,
#endif
    RI_LOCALE_DEP },
{   "buttonFont",		"ButtonFont",
    "-b&h-lucida-medium-r-normal-sans-*-120-*-*-*-*-*-*",
#ifdef OW_I18N_L4
    &(GRV.ButtonFontSetInfo),	cvtFontSet,		updButtonFont,
#else
    &(GRV.ButtonFontInfo),	cvtFont,		updButtonFont,
#endif
    RI_LOCALE_DEP },
{   "iconFont",			"IconFont",
    "-b&h-lucida-medium-r-normal-sans-*-120-*-*-*-*-*-*",
#ifdef OW_I18N_L4
    &(GRV.IconFontSetInfo),	cvtFontSet,		updIconFont,
#else
    &(GRV.IconFontInfo),	cvtFont,		updIconFont,
#endif
    RI_LOCALE_DEP },
{   "glyphFont",		"GlyphFont",
    "-sun-open look glyph-*-*-*-*-*-120-*-*-*-*-*-*",
    &(GRV.GlyphFontInfo),	cvtFont,		updGlyphFont,
    RI_LOCALE_DEP },
{   "cursorFont",		"CursorFont",
    "-sun-open look cursor-*-*-*-*-*-120-*-*-*-*-*-*",
    &(GRV.BasicPointer),	cvtCursorFont,		NULL,
    0L },
{   "foreground",		"Foreground",		"#000000",
    &(GRV.ForegroundColor),	cvtString,		updForeground,
    0L },
{   "background",		"Background",		"#ffffff",
    &(GRV.BackgroundColor),	cvtString,		updBackground,
    0L },
{   "reverseVideo",		"ReverseVideo",		"False",
    &(GRV.ReverseVideo),	cvtBoolean,		NULL,
    0L },
{   "borderColor",		"BorderColor",		"#000000",
    &(GRV.BorderColor),		cvtString,		updBorder,
    0L },
{   "windowColor",		"WindowColor",		"#cccccc",
    &(GRV.WindowColor),		cvtString,		updWindow,
    0L },
{   "workspaceStyle",		"WorkspaceStyle",	"paintcolor",
    &(GRV.WorkspaceStyle),	cvtWorkspaceStyle,	updWorkspaceStyle,
    0L },
{   "workspaceColor",		"WorkspaceColor",	"#40a0c0",
    &(GRV.WorkspaceColor),	cvtString,		updWorkspace,
    0L },
{   "workspaceBitmapFile",	"WorkspaceBitmapFile",	"gray",
    &(GRV.WorkspaceBitmapFile),	cvtString,		updWorkspace,
    0L },
{   "workspaceBitmapFg",	"WorkspaceBitmapFg",	"#000000",
    &(GRV.WorkspaceBitmapFg),	cvtString,		updWorkspace,
    0L },
{   "workspaceBitmapBg",	"WorkspaceBitmapBg",	"#ffffff",
    &(GRV.WorkspaceBitmapBg),	cvtString,		updWorkspace,
    0L },
{   "paintWorkspace",		"PaintWorkspace",	"True",
    &(GRV.PaintWorkspace),	cvtBoolean,		NULL,
    0L },
{   "pointerWorkspace",		"PointerWorkspace",	"True",
    &(GRV.PointerWorkspace),	cvtBoolean,		NULL,
    0L },
{   "use3D",			"Use3D",		"True", 
    &(GRV.F3dUsed),		cvtBoolean,		NULL,
    0L },
{   "setInput",			"SetInput",		"Select",
    &(GRV.FocusFollowsMouse),	cvtFocusStyle,		UpdFocusStyle,
    0L },
{   "defaultTitle",		"DefaultTitle",		"No Name", 
#ifdef OW_I18N_L4
    &(GRV.DefaultWinName),	cvtWString,		updString,
#else
    &(GRV.DefaultWinName),	cvtString,		updString,
#endif
    0L },
{   "flashFrequency",		"FlashFrequency",	"100000", 
    &(GRV.FlashTime),		cvtInteger,		NULL,
    0L },
{   "flashTime",		"FlashTime",		"100000",
    &(GRV.FlashTime),		cvtInteger,		NULL,
    0L },
{   "iconLocation",		"IconLocation", 	"bottom",
    &(GRV.IconPlacement),	cvtIconLocation,	updIconLocation,
    0L },
{   "focusLenience",		"FocusLenience", 	"False",
    &(GRV.FocusLenience),	cvtBoolean,		NULL,
    0L },
{   "dragWindow",		"DragWindow", 		"False",
    &(GRV.DragWindow),		cvtBoolean,		NULL,
    0L },
{   "autoRaise",		"AutoRaise",		"False",
    &(GRV.AutoRaise),		cvtBoolean,		NULL,
    0L },
{   "autoRaiseDelay",		"AutoRaiseDelay",	"0",
    &(GRV.AutoRaiseDelay),	cvtInteger,		NULL,
    0L },
{   "dragRightDistance",	"DragRightDistance",	"100",
    &(GRV.DragRightDistance),	cvtInteger,		NULL,
    0L },
{   "moveThreshold",		"MoveThreshold",	"5",
    &(GRV.MoveThreshold),	cvtInteger,		NULL,
    0L },
{   "dragThreshold",		"DragThreshold",	"5",
    &(GRV.MoveThreshold),	cvtInteger,		NULL,
    0L },
{   "clickMoveThreshold",	"ClickMoveThreshold",	"5",
    &(GRV.ClickMoveThreshold),	cvtInteger,		NULL,
    0L },
{   "multiClickTimeout",	"MultiClickTimeout",	"5",
    &(GRV.DoubleClickTime),	cvtClickTimeout,	NULL,
    0L },
{   "frontKey",			"FrontKey",		"Any L5",
    &(GRV.FrontKey),		cvtKey,			NULL,
    0L },
{   "helpKey",			"HelpKey",		"Help",
    &(GRV.HelpKey),		cvtKey,			NULL,
    0L },
{   "openKey",			"OpenKey",		"Any L7",
    &(GRV.OpenKey),		cvtKey,			NULL,
    0L },
{   "confirmKey",		"ConfirmKey",		"Return",
    &(GRV.ConfirmKey),		cvtKey,			NULL,
    0L },
{   "printOrphans",		"PrintOrphans",		"False", 
    &(GRV.PrintOrphans),	cvtBoolean,		NULL,
    0L },
{   "printAll",			"PrintAll",		"False", 
    &(GRV.PrintAll),		cvtBoolean,		NULL,
    0L },
{   "synchronize",		"Synchronize",		"False", 
    &(GRV.Synchronize),		cvtBoolean,		updSync,
    0L },
{   "snapToGrid",		"SnapToGrid",		"False",
    &(GRV.FSnapToGrid),		cvtBoolean,		NULL,
    0L },
{   "saveWorkspaceTimeout",	"SaveWorkspaceTimeout", "30",
    &(GRV.SaveWorkspaceTimeout), cvtInteger,		NULL,
    0L },
{   "saveWorkspaceCmd",		"SaveWorkspaceCmd",	
    "owplaces -silent -multi -local -script -tw -output $HOME/.openwin-init",
    &(GRV.SaveWorkspaceCmd),	cvtString,		NULL,
    0L },
{   "popupJumpCursor",		"PopupJumpCursor",	"True",
    &(GRV.PopupJumpCursor),	cvtBoolean,		NULL,
    0L },
{   "cancelKey",		"CancelKey",		"Escape",
    &(GRV.CancelKey),		cvtKey,			NULL,
    0L },
{   "colorLockKey",		"ColorLockKey",		"Control L2",
    &(GRV.ColorLockKey),	cvtKey,			NULL,
    0L },
{   "colorUnlockKey",		"ColorUnlockKey",	"Control L4",
    &(GRV.ColorUnlockKey),	cvtKey,			NULL,
    0L },
{   "colorFocusLocked",		"ColorFocusLocked",	"False",
    &(GRV.ColorLocked),	cvtBoolean,		NULL,
    0L },
{   "edgeMoveThreshold",	"EdgeMoveThreshold", 	"10",
    &(GRV.EdgeThreshold),	cvtInteger,		NULL,
    0L },
{   "rubberBandThickness",	"RubberBandThickness",	"2",
    &(GRV.RubberBandThickness),	cvtInteger,		NULL,
    0L },
{   "beep",			"Beep",			"always",
    &(GRV.Beep),		cvtBeepStatus,		NULL,
    0L },
{   "pPositionCompat",		"PPositionCompat",	"false",
    &(GRV.PPositionCompat),	cvtBoolean,		NULL,
    0L },
{   "minimalDecor",		"MinimalDecor",		"",
    &(GRV.Minimals),		cvtStringList,		updStringList,
    0L },
{   "use3DFrames",		"Use3DFrames",		"False", 
    &(GRV.F3dFrames),		cvtBoolean,		NULL,
    0L },
{   "use3DResize",		"Use3DResize",		"True",
    &(GRV.F3dResize),		cvtBoolean,		NULL,
    0L },
{   "refreshRecursively",	"RefreshRecursively",	"True",
    &(GRV.RefreshRecursively),	cvtBoolean,		NULL,
    0L },
{   "mouseChordTimeout",	"MouseChordTimeout",	"100",
    &(GRV.MouseChordTimeout),	cvtInteger,		NULL,
    0L },
{   "mouseChordMenu",		"MouseChordMenu",	"False",
    &(GRV.MouseChordMenu),	cvtBoolean,		NULL,
    0L },
{   "singleScreen",		"SingleScreen",		"False",
    &(GRV.SingleScreen),	cvtBoolean,		NULL,
    0L },
{   "autoReReadMenuFile",        "AutoReReadMenuFile",  "True",
    &(GRV.AutoReReadMenuFile),  cvtBoolean,		NULL,
    0L },
{   "keepTransientsAbove",	"KeepTransientsAbove",	"False",
    &(GRV.KeepTransientsAbove),	cvtBoolean,		NULL,
    0L },
{   "transientsSaveUnder",	"TransientsSaveUnder",	"False",
    &(GRV.TransientsSaveUnder),	cvtBoolean,		NULL,
    0L },
{   "transientsTitled",		"TransientsTitled",	"True",
    &(GRV.TransientsTitled),	cvtBoolean,		NULL,
    0L },
{   "selectWindows",		"SelectWindows",	"True",
    &(GRV.SelectWindows),	cvtBoolean,		NULL,
    0L },
{   "showMoveGeometry",		"ShowMoveGeometry",	"False",
    &(GRV.ShowMoveGeometry),	cvtBoolean,		NULL,
    0L },
{   "showResizeGeometry",	"ShowResizeGeometry",	"False",
    &(GRV.ShowResizeGeometry),	cvtBoolean,		NULL,
    0L },
{   "invertFocusHighlighting",	"InvertFocusHighlighting", "False",
    &(GRV.InvertFocusHighlighting), cvtBoolean,		NULL,
    0L },
{   "runSlaveProcess",		"RunSlaveProcess",	"True",
    &(GRV.RunSlaveProcess),	cvtBoolean,		NULL,
    0L },
{   "selectToggleStacking",	"SelectToggleStacking","False",
    &(GRV.SelectToggleStacking),cvtBoolean,		NULL,
    0L },
{   "flashCount",		"FlashCount",		"6",
    &(GRV.FlashCount),		cvtInteger,		NULL,
    0L },
{   "defaultIconImage",		"DefaultIconImage",	NULL,
    &(GRV.DefaultIconImage),	cvtString,		NULL,
    0L },
{   "defaultIconMask",		"DefaultIconMask",	NULL,
    &(GRV.DefaultIconMask),	cvtString,		NULL,
    0L },
{   "serverGrabs",		"ServerGrabs",		"True",
    &(GRV.ServerGrabs),		cvtBoolean,		NULL,
    0L },
{   "iconFlashCount",		"IconFlashCount",	"3",
    &(GRV.IconFlashCount),	cvtInteger,		NULL,
    0L },
{   "selectDisplaysMenu",	"SelectDisplaysMenu",	"False",
    &(GRV.SelectDisplaysMenu),	cvtBoolean,		NULL,
    0L },
{   "selectionFuzz",		"SelectionFuzz",	"1",
    &(GRV.SelectionFuzz),	cvtInteger,		NULL,
    0L },
{   "autoInputFocus",		"AutoInputFocus",	"False",
    &(GRV.AutoInputFocus),	cvtBoolean,		NULL,
    0L },
{   "autoColorFocus",		"AutoColorFocus",	"False",
    &(GRV.AutoColorFocus),	cvtBoolean,		NULL,
    0L },
{   "colorTracksInputFocus",	"ColorTracksInputFocus","False",
    &(GRV.ColorTracksInputFocus),cvtBoolean,		NULL,
    0L },
{   "iconFlashOnTime",		"IconFlashOnTime",	"20000",
    &(GRV.IconFlashOnTime),	cvtInteger,		NULL,
    0L },
{   "iconFlashOffTime",		"IconFlashOffTime",	"1",
    &(GRV.IconFlashOffTime),	cvtInteger,		NULL,
    0L },
{   "keyboardCommands",		"KeyboardCommands",	"Basic",
    &(GRV.Mouseless),		cvtMouseless,		updMouseless,
    0L },
{   "raiseOnActivate",		"RaiseOnActivate",	"True",
    &(GRV.RaiseOnActivate),	cvtBoolean,		NULL,
    0L },
{   "restackWhenWithdraw",	"RestackWhenWithdraw",	"True",
    &(GRV.RestackWhenWithdraw),	cvtBoolean,		NULL,
    0L },
{   "boldFontEmulation",	"BoldFontEmulation",	"False",
    &(GRV.BoldFontEmulation),	cvtBoolean,		NULL,
    RI_LOCALE_DEP },
{   "raiseOnMove",		"RaiseOnMove",		"False",
    &(GRV.RaiseOnMove),		cvtBoolean,		NULL,
    0L },
{   "raiseOnResize",		"RaiseOnResize",	"False",
    &(GRV.RaiseOnResize),	cvtBoolean,		NULL,
    0L },
{   "startDSDM",		"StartDSDM",		"True",
    &(GRV.StartDSDM),		cvtBoolean,		NULL,
    0L },
{   "printWarnings",		"PrintWarnings",	"False",
    &(GRV.PrintWarnings),	cvtBoolean,		NULL,
    0L },
{   "windowCacheSize",		"WindowCacheSize",	"500",
    &(GRV.WindowCacheSize),	cvtInteger,		updWindowCacheSize,
    0L },
{   "menuAccelerators",		"MenuAccelerators",	"True",
    &(GRV.MenuAccelerators),	cvtBoolean,		updMenuAccelerators,
    0L },
{   "windowMenuAccelerators",	"WindowMenuAccelerators", "True",
    &(GRV.WindowMenuAccelerators), cvtBoolean,		updMenuAccelerators,
    0L },
{   "ctrlAltMetaKey",		"CtrlAltMetaKey",	MetaKeyDefault,
    &(GRV.CtrlAltMetaKey),	cvtBoolean,		upCtrlAltMetaKey,
    0L },
/* Bug ID 1252764 - allow windows to be positioned off the screen */
{   "positionOnScreen",     "PositionOnScreen", "True",
    &(GRV.PositionOnScreen),    cvtBoolean,     NULL,
    0L },
#ifdef OW_I18N_L3
{   "characterSet",		"CharacterSet",		ISO_LATIN_1,
    &(GRV.CharacterSet),	cvtString,		NULL,
    RI_LOCALE_DEP },
#endif

/* NOTE: the following item must always be the last. */

{   NULL,			NULL,			NULL,
    NULL,			NULL,			NULL,
    RI_LAST_ITEM }
};


/* ===== Utilities ======================================================== */


/*
 * Copy a string, converting it to lower case.
 */
static void
strnlower(dest, src, n)
    char *dest;
    char *src;
    int  n;
{
    char *p;

    strncpy(dest, src, n);
    dest[n-1] = '\0';		/* force null termination */

    for (p = dest; *p; ++p)
	if (isupper(*p))
	    *p = tolower(*p);
}


#define BSIZE 100

/*
 * Determine whether value matches pattern, irrespective of case.
 * This routine is necessary because not all systems have strcasecmp().
 */
static Bool
matchString(value, pattern)
    char *value;
    char *pattern;
{
    char buf[BSIZE];

    strnlower(buf, value, BSIZE);
    return (0 == strcmp(buf, pattern));
}


/*
 * Match any of the following booleans: yes, no, 1, 0, on, off, t, nil, 
 * true, false.  Pass back the boolean matched in ret, and return True.  
 * Otherwise, return False.  Matches are case-insensitive.
 */
static Bool
matchBool(value, ret)
    char *value;
    Bool *ret;
{
    char buf[BSIZE];

    strnlower(buf, value, BSIZE);

    if (0 == strcmp(buf, "yes") ||
	0 == strcmp(buf, "on") ||
	0 == strcmp(buf, "t") ||
	0 == strcmp(buf, "true") ||
	0 == strcmp(buf, "1"))
    {
	*ret = True;
	return True;
    }

    if (0 == strcmp(buf, "no") ||
	0 == strcmp(buf, "off") ||
	0 == strcmp(buf, "nil") ||
	0 == strcmp(buf, "false") ||
	0 == strcmp(buf, "0"))
    {
	*ret = False;
	return True;
    }

    return False;
}


/*
 * BoolString() - return Bool based on string, returning the default value if 
 * the string can't be converted.
 */
Bool
BoolString(s, dflt)
	char	*s;
	Bool	dflt;
{
	Bool	b;

	if (matchBool(s,&b))
	    return b;
	else
	    return dflt;
}

/*
 * Match any of the WorkspaceStyle keywords: paintcolor, tilebitmap, none
 * Pass back the WorkspaceStyle value by reference, and return True, if
 * a match was found; otherwise return False and do not disturb the
 * passed value.
 */
static Bool
matchWorkspaceStyle(value, ret)
	char	       *value;
	WorkspaceStyle *ret;
{
	if (matchString(value,"paintcolor"))
	{
	    *ret = WkspColor;
	    return True;
	}
	if (matchString(value,"tilebitmap"))
	{
	    *ret = WkspPixmap;
	    return True;
	}
	if (matchString(value,"default"))
	{
	    *ret = WkspDefault;
	    return True;
	}
	return False;
}

/*
 * Match any of the following input focus keywords: followmouse, follow, f, 
 * select, s, click, clicktotype, c.  Pass back True for focusfollows or 
 * False for clicktotype in ret (since FocusFollowsMouse is the global
 * corresponding to this resource), and return True.  
 * Otherwise, return False.
 */
static Bool
matchFocusKeyword(value, ret)
    char *value;
    Bool *ret;
{
    char buf[BSIZE];

    strnlower(buf, value, BSIZE);

    if (0 == strcmp(buf, "followmouse") ||
	0 == strcmp(buf, "follow") ||
	0 == strcmp(buf, "f"))
    {
	*ret = True;
	return True;
    }

    if (0 == strcmp(buf, "select") ||
	0 == strcmp(buf, "click") ||
	0 == strcmp(buf, "clicktotype") ||
	0 == strcmp(buf, "c") ||
	0 == strcmp(buf, "s"))
    {
	*ret = False;
	return True;
    }

    return False;
}


/*
 * Match any of the three possible beep keywords:  always, never, or notices.
 * Pass back the BeepStatus value by reference, and return True, if
 * a match was found; otherwise return False and do not disturb the
 * passed value.
 */
static Bool
matchBeepKeyword(value, ret)
    char *value;
    BeepStatus *ret;
{
	if (matchString(value,"always"))
	{
	    *ret = BeepAlways;
	    return True;
	}
	if (matchString(value,"never"))
	{
	    *ret = BeepNever;
	    return True;
	}
	if (matchString(value,"notices"))
	{
	    *ret = BeepNotices;
	    return True;
	}
	return False;
}


/*
 * Match an icon placement keyword.  Store matched value in ret and return 
 * True, or return False if no match occurred.
 */
static Bool
matchIconPlace( value, ret )
char		*value;
IconPreference	*ret;
{
	if (matchString(value, "top"))
	{
		*ret = AlongTop;
		return True;
	}
	if (matchString(value, "bottom"))
	{
		*ret = AlongBottom;
		return True;
	}
	if (matchString(value, "right"))
	{
		*ret = AlongRight;
		return True;
	}
	if (matchString(value, "left"))
	{
		*ret = AlongLeft;
		return True;
	}
	if (matchString(value, "top-lr"))
	{
		*ret = AlongTop;
		return True;
	}
	if (matchString(value, "top-rl"))
	{
		*ret = AlongTopRL;
		return True;
	}
	if (matchString(value, "bottom-lr"))
	{
		*ret = AlongBottom;
		return True;
	}
	if (matchString(value, "bottom-rl"))
	{
		*ret = AlongBottomRL;
		return True;
	}
	if (matchString(value, "right-tb"))
	{
		*ret = AlongRight;
		return True;
	}
	if (matchString(value, "right-bt"))
	{
		*ret = AlongRightBT;
		return True;
	}
	if (matchString(value, "left-tb"))
	{
		*ret = AlongLeft;
		return True;
	}
	if (matchString(value, "left-bt"))
	{
		*ret = AlongLeftBT;
		return True;
	}

	return False;
}


static Bool
matchMouselessKeyword(str, ret)
    char *str;
    MouselessMode *ret;
{
    if (0 == strcmp(str, "SunView1")) {
	*ret = KbdSunView;
	return True;
    } else if (0 == strcmp(str, "Basic")) {
	*ret = KbdBasic;
	return True;
    } else if (0 == strcmp(str, "Full")) {
	*ret = KbdFull;
	return True;
    }
    return False;
}


/*
 * Parse a key specification of the form
 *
 * [modifier ...] keysym
 *
 * For example, "Control Shift F7".  Returns True if a valid keyspec was
 * parsed, otherwise False.  The modifier mask is returned in modmask, and the
 * keycode is returned in keycode.
 */
static Bool
parseKeySpec(dpy, str, modmask, keycode)
    Display *dpy;
    char *str;
    unsigned int *modmask;
    KeyCode *keycode;
{
    char line[100];
    char *word;
    extern unsigned int FindModiferMask();
    int kc, m;
    int mask = 0;
    int code = 0;
    KeySym ks;

    strcpy(line, str);
    word = strtok(line, " \t");
    if (word == NULL)
	return False;

    while (word != NULL) {
	ks = XStringToKeysym(word);
	if (ks == NoSymbol) {
	    if (strcmp(word, "Any") == 0) {
		mask = AnyModifier;
		word = strtok(NULL, " \t");
		continue;
	    } else if (strcmp(word, "Shift") == 0)
		ks = XK_Shift_L;
	    else if (strcmp(word, "Control") == 0)
		ks = XK_Control_L;
	    else if (strcmp(word, "Meta") == 0)
		ks = XK_Meta_L;
	    else if (strcmp(word, "Alt") == 0)
		ks = XK_Alt_L;
	    else if (strcmp(word, "Super") == 0)
		ks = XK_Super_L;
	    else if (strcmp(word, "Hyper") == 0)
		ks = XK_Hyper_L;
	    else
		return False;
	}
	    
	kc = XKeysymToKeycode(dpy, ks);
	if (kc == 0)
	    return False;

	m = FindModifierMask(kc);
	if (m == 0) {
	    code = kc;
	    break;
	}
	mask |= m;
	word = strtok(NULL, " \t");
    }

    if (code == 0)
	return False;

    *keycode = code;
    *modmask = mask;
    return True;
}


/* ===== Converters ======================================================= */


/*
 * static Bool cvtWhatever(dpy, item, string, addr)
 *
 * The job of the converter is to take a string and convert it into the value
 * appropriate for storage into a global variable.  If the conversion is
 * successful, the value is stored at addr and True is returned.  Otherwise,
 * False is returned.  NOTE: the converted global variable shouldn't have any
 * pointers into the resource database.  If it's necessary to keep a handle on
 * this data, the converter should allocate memory and make a copy.  See also
 * the note about memory allocation in the comment at the top of the updaters
 * section, below.
 */


static Bool
cvtBoolean(dpy, item, string, addr)
    Display	    *dpy;
    ResourceItem    *item;
    char	    *string;
    void	    *addr;
{
    return matchBool(string, (Bool *)addr);
}


#ifdef OW_I18N_L4
static Bool
cvtFontSet(dpy, item, string, addr)
    Display	    *dpy;
    ResourceItem    *item;
    char	    *string;
    void	    *addr;
{
    XFontSetInfo    *dest = addr;
    XFontSet	    info;
    char	    *locale;
    XFontSet	    loadQueryFontSet();
    XFontSetExtents	*XExtentsOfFontSet();

    /* XXX - is this right? the locale may not have been set up properly */
    locale = setlocale(LC_CTYPE, NULL);
    info = loadQueryFontSet(dpy, string, locale);
    if (info == NULL) {
        return False;
    }

    dest->fs = info;
    dest->fsx = XExtentsOfFontSet(info);

    return True;
}
#endif


static Bool
cvtFont(dpy, item, string, addr)
    Display	    *dpy;
    ResourceItem    *item;
    char	    *string;
    void	    *addr;
{
    XFontStruct	    **dest = addr;
    XFontStruct	    *info;
    
    info = XLoadQueryFont(dpy, string);

    if (info == NULL)
	return False;

    *dest = info;
    return True;
}


/*
 * cvtCursorFont -- set up ALL cursors from cursor font specified.
 *
 * NOTE that CursorColor and Bg1Color must be set before the cursors!
 *
 * Notice that six cursors are set up (and stored in six separate GRV
 * elements) from this single resource.  REMIND: this is kind of bogus.  
 * Ideally, all six cursors would have fonts and character indexes specifiable 
 * independently.  Further, addr isn't used; GRV is stored directly.
 *
 * REMIND: this appears to have a resource leak, in that cursorFont is loaded 
 * but never unloaded.
 */
static Bool
cvtCursorFont(dpy, item, string, addr)
    Display	    *dpy;
    ResourceItem    *item;
    char	    *string;
    void	    *addr;
{
    XFontStruct	    *cursorFont;
    int		    ii;
    Cursor	    *tmpVariable;
    unsigned int    tmpFontIndex;
    unsigned int    defaultIndex;
    XColor	    foreColor, backColor;
    
    cursorFont = XLoadQueryFont(dpy, string);

    if (cursorFont == NULL)
	return False;

    /*
     * REMIND: in the future, we will probably want to set up some scheme for 
     * customizing cursor colors.  For now, use black and white.
     */

    foreColor.red = foreColor.green = foreColor.blue = 0;	/* black */
    backColor.red = backColor.green = backColor.blue = 65535;	/* white */

    for (ii = 0; ii < NUM_CURSORS; ++ii) {

	switch (ii) {

	case BASICPTR:
	    tmpVariable = &GRV.BasicPointer;
	    tmpFontIndex = OLC_basic;
	    defaultIndex = XC_left_ptr;
	    break;

	case MOVEPTR:
	    tmpVariable = &GRV.MovePointer;
	    tmpFontIndex = OLC_basic;
	    defaultIndex = XC_left_ptr;
	    break;

	case BUSYPTR:
	    tmpVariable = &GRV.BusyPointer;
	    tmpFontIndex = OLC_busy;
	    defaultIndex = XC_watch;
	    break;

	case ICONPTR:
	    tmpVariable = &GRV.IconPointer;
	    tmpFontIndex = OLC_basic;
	    defaultIndex = XC_left_ptr;
	    break;

	case RESIZEPTR:
	    tmpVariable = &GRV.ResizePointer;
	    tmpFontIndex = OLC_beye;
	    defaultIndex = XC_tcross;
	    break;

	case MENUPTR:
	    tmpVariable = &GRV.MenuPointer;
	    tmpFontIndex = OLC_basic;
	    defaultIndex = XC_sb_right_arrow;
	    break;

	case QUESTIONPTR:
	    tmpVariable = &GRV.QuestionPointer;
	    tmpFontIndex = OLC_basic;
	    defaultIndex = XC_question_arrow;
	    break;

	case TARGETPTR:
	    tmpVariable = &GRV.TargetPointer;
	    tmpFontIndex = OLC_basic;
	    defaultIndex = XC_circle;
	    break;

	case PANPTR:
	    tmpVariable = &GRV.PanPointer;
	    tmpFontIndex = OLC_panning;
	    defaultIndex = XC_sb_v_double_arrow;
	    break;
	}

	*tmpVariable = XCreateGlyphCursor(dpy, cursorFont->fid,
				cursorFont->fid, tmpFontIndex, tmpFontIndex+1, 
				&foreColor, &backColor);

	/* 
	 * Use default cursor index if the specified one didn't work
	 */
	if (*tmpVariable == NULL) {
	    *tmpVariable = XCreateFontCursor( dpy, defaultIndex );
	}
    }

    return True;
}


#ifdef OW_I18N_L4
/*
 * Converting a string simply means making a copy of it.
 */
static Bool
cvtWString(dpy, item, string, addr)
    Display	    *dpy;
    ResourceItem    *item;
    char	    *string;
    void	    *addr;
{
    wchar_t **str = addr;

    if (string == NULL)
	return False;

    *str = mbstowcsdup(string);
    return True;
}
#endif


/*
 * Converting a string simply means making a copy of it.
 */
static Bool
cvtString(dpy, item, string, addr)
    Display	    *dpy;
    ResourceItem    *item;
    char	    *string;
    void	    *addr;
{
    char **str = addr;

    if (string == NULL)
	return False;

    *str = MemNewString(string);
    return True;
}


static Bool
cvtFloat(dpy, item, string, addr)
    Display	    *dpy;
    ResourceItem    *item;
    char	    *string;
    void	    *addr;
{
    return (1 == sscanf(string, "%f", (float *)addr));
}


/*
 * Convert an integer.  Note that %i converts from decimal, octal, and 
 * hexadecimal representations.
 */
static Bool
cvtInteger(dpy, item, string, addr)
    Display	    *dpy;
    ResourceItem    *item;
    char	    *string;
    void	    *addr;
{
    return (1 == sscanf(string, "%i", (int *)addr));
}


/*
 * Convert a string representing WorkspaceStyle
 */
static Bool
cvtWorkspaceStyle(dpy, item, string, addr)
    Display	    *dpy;
    ResourceItem    *item;
    char	    *string;
    void	    *addr;
{
/*
    return False;
*/
    return matchWorkspaceStyle(string, (WorkspaceStyle *)addr);
}


/*
 * Convert a string representing tenths of a second into milliseconds.
 */
static Bool
cvtClickTimeout(dpy, item, string, addr)
    Display	    *dpy;
    ResourceItem    *item;
    char	    *string;
    void	    *addr;
{
    int intval;
    int *dest = addr;

    if (1 != sscanf(string, "%d", &intval))
	return False;

    intval *= 100;			/* convert to milliseconds */

    /*
     * It's nearly impossible for typical mouse hardware to generate two
     * clicks in less than 100ms.  We special-case this and make the minimum
     * timeout value be 150ms.
     */
    if (intval < 150)
	intval = 150;

    *dest = intval;
    return True;
}


static Bool
cvtFocusStyle(dpy, item, string, addr)
    Display	    *dpy;
    ResourceItem    *item;
    char	    *string;
    void	    *addr;
{
    return matchFocusKeyword(string, (Bool *)addr);
}


static Bool
cvtBeepStatus(dpy, item, string, addr)
    Display	    *dpy;
    ResourceItem    *item;
    char	    *string;
    void	    *addr;
{
    return matchBeepKeyword(string, (BeepStatus *)addr);
}


static Bool
cvtMouseless(dpy, item, string, addr)
    Display	    *dpy;
    ResourceItem    *item;
    char	    *string;
    void	    *addr;
{
    return matchMouselessKeyword(string, (MouselessMode *)addr);
}


static Bool
cvtIconLocation(dpy, item, string, addr)
    Display	    *dpy;
    ResourceItem    *item;
    char	    *string;
    void	    *addr;
{
    return matchIconPlace(string, (IconPreference *)addr);
}


/*
 * Convert a key specification.  REMIND: this needs to be reconciled with the 
 * key specification stuff in evbind.c.
 */
static Bool
cvtKey(dpy, item, string, addr)
    Display	    *dpy;
    ResourceItem    *item;
    char	    *string;
    void	    *addr;
{
    KeySpec	    *keyspec = addr;
    unsigned int    modmask;
    KeyCode	    keycode;

    if (!parseKeySpec(dpy, string, &modmask, &keycode))
	return False;

    keyspec->modmask = modmask;
    keyspec->keycode = keycode;
    return True;
}


/*
 * buildStringList -- parse a string into words and build a linked list of 
 * them.
 */
static void
buildStringList(str, pplist)
char *str;
List **pplist;
{
    char *swork, *swork2;
    List *l = NULL_LIST;

    swork2 = swork = MemNewString(str);

    while ((swork2 = strtok(swork2, " \t")) != NULL) {
	l = ListCons(MemNewString(swork2),l);
	swork2 = NULL;
    }
    MemFree(swork);
    *pplist = l;
}


static void *
freeStringList(str,junk)
char *str;
void *junk;
{
	MemFree(str);
	return NULL;
}


static Bool
cvtStringList(dpy, item, string, addr)
    Display	    *dpy;
    ResourceItem    *item;
    char	    *string;
    void	    *addr;
{
    List **dest = addr;
    List *newl = NULL_LIST;

    buildStringList(string, &newl);
    *dest = newl;
    return True;
}


#ifdef OW_I18N_L3

/*
 * cvtOLLC
 * 
 * REMIND: somewhat strange.  This function always returns True, so the
 * default value in the Resource Table is never used.  Further, this function 
 * handles both the conversion and update functions itself.
 */
static Bool
cvtOLLC(dpy, item, string, addr)
    Display	    *dpy;
    ResourceItem    *item;
    char	    *string;
    void	    *addr;
{
    OLLCItem	    *ollcitem = addr;
    char	    *newlocale;

#ifdef DEBUG
    fprintf(stderr, "cvtOLLC: locale#%d, newlocale %s, curlocale %s\n",
	    ollcitem->posixCategory, string,
	    ollcitem->locale ? ollcitem->locale : "(null)");
#endif

    /* don't need to do anything if the new locale is the same as the old */

    if ((string == NULL && ollcitem->locale == NULL) ||
        (string != NULL && ollcitem->locale != NULL &&
	 0 == strcmp(string, ollcitem->locale)))
    {
	return True;
    }

    /* they differ; update the locale */

    if (string == NULL)
	newlocale = NULL;
    else
	newlocale = MemNewString(string);

    if (ollcitem->locale != NULL)
	MemFree(ollcitem->locale);

    ollcitem->locale = newlocale;

#ifdef DEBUG
    fprintf(stderr, "cvtOLLC: locale#%d -> %s\n",
	    ollcitem->posixCategory,
	    ollcitem->locale ? ollcitem->locale : "(null)");
#endif

    return True;
}

#endif /* OW_I18N_L3 */

#if defined (DEBUG) && defined (OW_I18N_L3)
dump_locale()
{
    fprintf(stderr, "  -> %5.5s %5.5s %5.5s %5.5s %5.5s\n",
	    "basic", "dlang", "ilang", "numeric", "date");
    fprintf(stderr, "  -> %5.5s %5.5s %5.5s %5.5s %5.5s\n", 
	    GRV.lc_basic.locale ? GRV.lc_basic.locale : "(null)",
	    GRV.lc_dlang.locale ? GRV.lc_dlang.locale : "(null)",
	    GRV.lc_ilang.locale ? GRV.lc_ilang.locale : "(null)",
	    GRV.lc_numeric.locale ? GRV.lc_numeric.locale : "(null)",
	    GRV.lc_time.locale ? GRV.lc_time.locale : "(null)");
}
#endif


/* ===== Updaters ========================================================= */


/*
 * static void updWhatever(dpy, item, cur, new);
 *
 * The job of the updater is to compare the current value and newly converted
 * values, and update the current value if they differ.  It is responsible
 * for all changes in global state, such as grabbing and ungrabbing keys.  
 * NOTE: if the converter has allocated memory, the updater must free it 
 * appropriately.  Since the updater is called with old and new values, 
 * exactly one of them should be freed by the updater, otherwise a memory leak 
 * will result.
 */

static void
updString(dpy, item, cur, new)
    Display	    *dpy;
    ResourceItem    *item;
    char	    **cur, **new;
{
    MemFree(*cur);
    *cur = *new;
}


static void
updStringList(dpy, item, cur, new)
    Display	    *dpy;
    ResourceItem    *item;
    List	    **cur, **new;
{
    ListApply(*cur, freeStringList, NULL);
    ListDestroy(*cur);
    *cur = *new;
}


static void
updWorkspaceStyle(dpy, item, cur, new)
    Display	    *dpy;
    ResourceItem    *item;
    WorkspaceStyle  *cur, *new;
{
    *cur = *new;
    updateWorkspaceBackground = True;
}


static void
updWorkspace(dpy, item, cur, new)
    Display	    *dpy;
    ResourceItem    *item;
    char	    **cur, **new;
{
    MemFree(*cur);
    *cur = *new;
    updateWorkspaceBackground = True;
}


static void
updWindow(dpy, item, cur, new)
    Display	    *dpy;
    ResourceItem    *item;
    char	    **cur, **new;
{
    MemFree(*cur);
    *cur = *new;
    SetWindowColor(dpy);
}


static void
updForeground(dpy, item, cur, new)
    Display	    *dpy;
    ResourceItem    *item;
    char	    **cur, **new;
{
    MemFree(*cur);
    *cur = *new;
    SetForegroundColor(dpy);
}

static void
updBackground(dpy, item, cur, new)
    Display	    *dpy;
    ResourceItem    *item;
    char	    **cur, **new;
{
    MemFree(*cur);
    *cur = *new;
    SetBackgroundColor(dpy);
}

static void
updBorder(dpy, item, cur, new)
    Display	    *dpy;
    ResourceItem    *item;
    char	    **cur, **new;
{
    MemFree(*cur);
    *cur = *new;
    SetBorderColor(dpy);
}


static void
updSync(dpy, item, cur, new)
    Display	    *dpy;
    ResourceItem    *item;
    Bool	    *cur, *new;
{
    if (*cur != *new) {
	(void) XSynchronize(dpy, *new);
	*cur = *new;
    }
}


#ifdef OW_I18N_L4

/*
 * I18N versions of font update functions (works on font sets)
 *
 * Note: the screen functions (SetTitleFont, etc) typically call an
 *       olgx function which have the old font/fontset cached. It's
 *  	 safer to free the font/set after the new one is opened. This
 * 	 is true of both I18N font set functions and the old 
 * 	 XFontStruct (non-I18N) functions.
 */
static void
updTitleFont(dpy, item, cur, new)
    Display	    *dpy;
    ResourceItem    *item;
    XFontSetInfo    *cur, *new;
{
    XFontSetInfo    del = *cur;

    *cur = *new;
    SetTitleFont(dpy);

    /* Free the font set */
    freeFontSet(dpy, del.fs);
}

static void
updTextFont(dpy, item, cur, new)
    Display         *dpy;
    ResourceItem    *item;
    XFontSetInfo    *cur, *new;
{
    XFontSetInfo    del = *cur;

    *cur = *new;
    SetTextFont(dpy);

    /* Free the font set */
    freeFontSet(dpy, del.fs);
}

static void
updButtonFont(dpy, item, cur, new)
    Display         *dpy;
    ResourceItem    *item;
    XFontSetInfo    *cur, *new;
{
    XFontSetInfo    del = *cur;

    *cur = *new;
    SetButtonFont(dpy);

    /* Free the font set */
    freeFontSet(dpy, del.fs);
}

static void
updIconFont(dpy, item, cur, new)
    Display         *dpy;
    ResourceItem    *item;
    XFontSetInfo    *cur, *new;
{
    XFontSetInfo    del = *cur;

    *cur = *new;
    SetIconFont(dpy);

    /* Free the font set */
    freeFontSet(dpy, del.fs);
}

#else	/* OW_I18N_L4 */

/* 
 * I18N versions of font update functions (works on font sets)   
 */ 
static void
updTitleFont(dpy, item, cur, new)
    Display         *dpy;
    ResourceItem    *item;
    XFontStruct     **cur, **new;
{
    XFontStruct     *del = *cur; 

    *cur = *new;
    SetTitleFont(dpy);

    /* Free the old font */
    XFreeFont(dpy, del);
}

static void
updTextFont(dpy, item, cur, new)
    Display         *dpy;
    ResourceItem    *item;
    XFontStruct     **cur, **new;
{
    XFontStruct     *del = *cur;

    *cur = *new;
    SetTextFont(dpy);

    /* Free the old font */
    XFreeFont(dpy, del);
}

static void
updButtonFont(dpy, item, cur, new)
    Display         *dpy;
    ResourceItem    *item;
    XFontStruct     **cur, **new;
{
    XFontStruct     *del = *cur;

    *cur = *new;
    SetButtonFont(dpy);

    /* Free the old font */
    XFreeFont(dpy, del);
}

static void
updIconFont(dpy, item, cur, new)
    Display         *dpy;
    ResourceItem    *item;
    XFontStruct     **cur, **new;
{
    XFontStruct     *del = *cur;

    *cur = *new;
    SetIconFont(dpy);

    /* Free the old font */
    XFreeFont(dpy, del);
}
#endif 	/* OW_I18N_L4 */


static void
updGlyphFont(dpy, item, cur, new)
    Display	    *dpy;
    ResourceItem    *item;
    XFontStruct	    **cur, **new;
{
    XFontStruct     *del = *cur;

    *cur = *new;
    SetGlyphFont(dpy);

    /* Free the old font */
    XFreeFont(dpy, del);
}


static void
updIconLocation(dpy, item, cur, new)
    Display	    *dpy;
    ResourceItem    *item;
    IconPreference  *cur, *new;
{
    if (*cur != *new) {
	*cur = *new;
	SetIconLocation(dpy);
    }
}


static void
updMouseless(dpy, item, cur, new)
    Display	    *dpy;
    ResourceItem    *item;
    MouselessMode   *cur, *new;
{
    if (*cur != *new) {
	*cur = *new;
	forceKeyRegrab = True;
    }
}


static void
updMenuAccelerators(dpy, item, cur, new)
    Display	    *dpy;
    ResourceItem    *item;
    Bool	    *cur, *new;
{
    if (*cur != *new) {
	*cur = *new;
	forceKeyRegrab = True;
    }
}


static void
upCtrlAltMetaKey(dpy, item, cur, new)
    Display	    *dpy;
    ResourceItem    *item;
    Bool	    *cur, *new;
{
    if(*cur != *new) {
	*cur = *new;
	forceKeyRegrab = True;
    }
}


static void
updWindowCacheSize(dpy, item, cur, new)
    Display	    *dpy;
    ResourceItem    *item;
    int		    *cur, *new;
{
    if (*cur != *new) {
	*cur = *new;
	ScreenUpdateWinCacheSize(dpy);
    }
}


/*
 * unconfigureFocus
 *
 * Tell a client to remove any grabs it may have set up according to the focus 
 * mode.  If this client is the focus, tell it to draw in its unfocused state.
 */
static void *
unconfigureFocus(cli)
    Client *cli;
{
    if (cli->framewin == NULL)
	return NULL;
    FrameSetupGrabs(cli, cli->framewin->core.self, False);
    if (cli->isFocus) {
	cli->isFocus = False;
	WinCallDraw((WinGeneric *)cli->framewin);
	cli->isFocus = True;
    }
    return NULL;
}


/*
 * reconfigureFocus
 *
 * Tell a client to restore any grabs it may need for the new focus mode.  If 
 * this client is the focus, tell it to draw using the proper highlighting for 
 * the new focus mode.
 */
static void *
reconfigureFocus(cli)
    Client *cli;
{
    if (cli->framewin == NULL)
	return NULL;
    FrameSetupGrabs(cli, cli->framewin->core.self, True);
    if (cli->isFocus) {
	WinCallDraw((WinGeneric *)cli->framewin);
    }
    return NULL;
}


/*
 * UpdFocusStyle -- change the focus style on the fly
 *
 * If focus style needs updating, call unconfigureFocus on every client.  This
 * will clear grabs and highlighting and such while the old focus mode is
 * still in effect.  Update the global value, and then call reconfigureFocus
 * on every client to set up stuff for the new focus mode.
 *
 * REMIND: This function is global because it's called from FlipFocusFunc in
 * services.c.  This call passes NULL for item.  This needs to be cleaned up.
 */
void
UpdFocusStyle(dpy, item, cur, new)
    Display	    *dpy;
    ResourceItem    *item;
    Bool	    *cur, *new;
{
    if (*cur != *new) {
	ListApply(ActiveClientList, unconfigureFocus, 0);
	*cur = *new;
	ListApply(ActiveClientList, reconfigureFocus, 0);
    }
}


/* ===== Global Functions ================================================= */


/*
 * datum -- temporary destination for converted values.  On resource updates, 
 * values are converted into this area instead of directly into the global 
 * variable.  This allows the update function to compare the converted values 
 * to see whether an update is really necessary.  This union should be as 
 * large as the largest field in GRV.
 */

static union {
    int		    intval;
    void	    *pointer;
    KeySpec	    keyspec;
#ifdef OW_I18N_L3
    OLLCItem	    ollcitem;
#ifdef OW_I18N_L4
    XFontSetInfo    fontsetinfo;
#endif
#endif /* OW_I18N_L3 */
} datum;


/*
 * ScanResourceItemTable.  Scans a resource item table, probing the
 * resource database for each item.  Ignores any immutable items in
 * the table.  If SR_IMMUTABLE is True, a hit will cause the table
 * entry's immutable flag to be set.  If the probe missed and
 * SR_USE_DEFAULT is True, the default value will be converted.
 *
 * If SR_UPDATING is True, then we are updating the resource database
 * instead of initializing it.  This causes a couple of differences.
 * If an updater function is given for the item, conversions are done
 * into a piece of temporary storage and then the updater function is
 * called to update the real global variable from this temporary
 * storage.
 */

void
ScanResourceItemTable(dpy, table, rdb, oldlocale, flags)
    Display *dpy;
    ResourceItem *table;
    XrmDatabase rdb;
    char *oldlocale;
    unsigned long flags;
{
    XrmRepresentation type;
    XrmValue value;
    XrmValue oldvalue;
    XrmQuark classes[4];
    XrmQuark instances[4];
    ResourceItem *item;
    Bool hit;

#ifdef OW_I18N_L3
    XrmQuark localeQ;
    XrmQuark oldlocaleQ;

    if (GRV.lc_basic.locale != NULL)
	localeQ = XrmStringToQuark(GRV.lc_basic.locale);
    else
	localeQ = NULLQUARK;

    if (oldlocale != NULL)
	oldlocaleQ = XrmStringToQuark(oldlocale);
#endif    

    classes[0] = OpenWinQ;
    instances[0] = TopInstanceQ;

    for (item = table; !(item->flags & RI_LAST_ITEM); ++item) {

	/* never update an immutable item */
	if (item->flags & RI_IMMUTABLE)
	    continue;

	classes[1] = item->classQ;
	instances[1] = item->instanceQ;
	hit = False;

#ifdef OW_I18N_L3
	if (item->flags & RI_LOCALE_DEP) {
	    classes[2] = instances[2] = localeQ;
	    classes[3] = instances[3] = NULLQUARK;
	    hit = XrmQGetResource(rdb, instances, classes, &type, &value);
	}
#endif

	if (!hit) {
	    classes[2] = instances[2] = NULLQUARK;
	    hit = XrmQGetResource(rdb, instances, classes, &type, &value);
	}

	if (flags & SR_UPDATING) {

	    /* ignore if not found */
	    if (!hit)
		continue;

	    /* ignore if old and new values are the same */
#ifdef OW_I18N_L3
	    if ((item->flags & RI_LOCALE_DEP) && (oldlocale != NULL))
		classes[2] = instances[2] = oldlocaleQ;
#endif
	    if (XrmQGetResource(OlwmDB, instances, classes, &type, &oldvalue)
		&& 0 == strcmp((char *)value.addr, (char *)oldvalue.addr))
	    {
		continue;
	    }
#ifdef OW_I18N_L3
	    if ((item->flags & RI_LOCALE_DEP) && (oldlocale != NULL))
		classes[2] = instances[2] = localeQ;
#endif

	    if (item->updater == NULL) {
		(void) (*item->converter)(dpy, item, (char *)value.addr,
					  item->addr);
	    } else {
		(void) memset((char *) &datum, 0, sizeof(datum));
		if ((*item->converter)(dpy, item, (char *)value.addr, &datum))
		    (*item->updater)(dpy, item, item->addr, &datum);
	    }
	} else {
	    if (hit && (*item->converter)(dpy, item, value.addr, item->addr)) {
		if (flags & SR_IMMUTABLE)
		    item->flags |= RI_IMMUTABLE;
	    } else {
		if (flags & SR_USE_DEFAULT)
		    (void) (*item->converter)(dpy, item, item->defaultString,
					      item->addr);
	    }
	}
    }
}

#ifdef OW_I18N_L4
/*
 * Serious cheese factor alert! This Function does some "last resort"
 * processing on the font sets to try to do everything possible to run
 * in at least a cripple configuration. For some time, we've been 
 * simply exiting when a font set is not available. Now, do the best
 * we can by falling back to a guaranteed font. Sigh...
 */
static void
FallBack(dpy, fs)
    Display *dpy;
    void *fs; 
{
    XFontSetInfo *fsinfo;
    char **newargv;
    int count;

    /*
     * The "fixed" font is always guaranteed to be attached to the server,
     * so that's the one we'd like to fall back to. The thing we've got to
     * do is make sure that this font makes sense in this configuration. 
     * If olwm is operating in an non isso locale, we're going to have to
     * do something a little more fancy...
     */
    if(strcmp(GRV.CharacterSet, ISO_LATIN_1) == 0) {	/* We're in business */
	
	/* 
	 * Thus starteth the cheese. Directly call cvtFontSet with the 
	 * appropriate arguments. Note that we're passing in a NULL --
	 * this should probably be fixed.
	 *
	 * First try -b&h-lucida-medium-r-normal-sans-*-120-*-*-*-*-*-*, then
	 * fallback to fixed. If all else fails, exit.
	 */
	cvtFontSet(dpy, NULL, 
		"-b&h-lucida-medium-r-normal-sans-*-120-*-*-*-*-*-*", fs);
	fsinfo = (XFontSetInfo *)fs;
	if(fsinfo->fs != NULL) {
	    ErrorWarning(GetString("Couldn't find some font sets -- falling back to lucida font for some font sets"));
	    /* Mission accomplished, return AOK */
	    return;		
	}
	
	/* Second choice -- fixed. */
	cvtFontSet(dpy, NULL, "fixed", fs);
	fsinfo = (XFontSetInfo *)fs;
	if(fsinfo->fs != NULL) {
	    ErrorWarning(GetString("Couldn't find some font sets -- falling back to fixed font for some font sets"));
	    return;
	}

	/* 
	 * If we're here, we're out of luck. No fonts available anywhere.
	 * Print an error message and bail.
	 */
	ErrorGeneral(GetString("Could not find any fonts to use. Exiting..."));
	/*NOTREACHED*/

    } else {

	/* 
	 * Here, we've got some real problems. The only font we're 
	 * guaranteed to have is fixed, which is an ISO font. That 
	 * don't work so good in some of the Asian locale's. What
	 * we're forced to do is tweak some of resources, and restart
	 * olwm with a special flag. This forces a C locale environment.
	 */

	/* First, unset any of the environment locale that may have been set */
	putenv("LC_CTYPE=");
	putenv("LC_MESSAGES=");
	putenv("LC_NUMERIC=");
	putenv("LC_TIME=");

	/* Next, go ahead and add the command line param to force restart */
	count = 0;
	while(ArgVec[count] != NULL)
	    ++count;
	count += 2;	/* "-fallback" + NULL termination */
	newargv = (char **)MemAlloc(count * sizeof(char *));
	memcpy(newargv, ArgVec, (count - 2) * sizeof(char *));
	newargv[count-2] = "-fallback";
	newargv[count-1] = NULL;
	ArgVec = newargv;

	/* Finally, go ahead and restart */
	RestartOLWM();
	/*NOTREACHED*/
    }
}
#endif /* OW_I18N_L4 */
	
/*
 * InitGlobals.  Zero out all global variables.  Run through resource tables, 
 * interning their quarks.  Called once at startup time.  Destroys cmdDB.
 */
void
InitGlobals(dpy, cmdDB)
    Display *dpy;
    XrmDatabase cmdDB;
{
    ResourceItem *item;
    XrmDatabase userDB;

    (void) memset((char *) &GRV, 0, sizeof(GRV));

    /* 
     * Derive all of the "run time" defaults. Currently there is only 1.
     * If there is a meta key attached to the keyboard, then go ahead
     * and default the CtrlAltMetaKey to false. If there is no meta key,
     * default to true. Either way, the user can override if they see
     * fit.
     */
    if(XKeysymToKeycode(dpy, XK_Meta_L) || XKeysymToKeycode(dpy, XK_Meta_R)) {
	strcpy(MetaKeyDefault, "False");
	GRV.CtrlAltMetaKey = 0;
    } else {
	strcpy(MetaKeyDefault, "True");
	GRV.CtrlAltMetaKey = 1;
    }

    /* Run through the tables and intern the quarks. */

    for (item = LocaleItemTable; !(item->flags & RI_LAST_ITEM); ++item) {
	item->classQ    = XrmStringToQuark(item->class);
	item->instanceQ = XrmStringToQuark(item->instance);
    }

    for (item = MainItemTable; !(item->flags & RI_LAST_ITEM); ++item) {
	item->classQ    = XrmStringToQuark(item->class);
	item->instanceQ = XrmStringToQuark(item->instance);
    }

#ifdef OW_I18N_L3
    GRVLCInit();
#endif

    userDB = GetUserDefaults(dpy);

    ScanResourceItemTable(dpy, LocaleItemTable, cmdDB, NULL, SR_IMMUTABLE);
    ScanResourceItemTable(dpy, LocaleItemTable, userDB, NULL, NULL);

#ifdef OW_I18N_L3
    setOLLCPosix();
    EffectOLLC(dpy, True, NULL, NULL);
#endif

    ScanResourceItemTable(dpy, MainItemTable, cmdDB, NULL, SR_IMMUTABLE);

    OlwmDB = GetAppDefaults();
    XrmMergeDatabases(userDB, &OlwmDB);
    XrmMergeDatabases(cmdDB, &OlwmDB);

    ScanResourceItemTable(dpy, MainItemTable, OlwmDB, NULL, SR_USE_DEFAULT);

    /*
     * Special case for glyph font: if we couldn't find a valid glyph font,
     * it's a fatal error.
     */
    if (GRV.GlyphFontInfo == NULL)
	ErrorGeneral(GetString("can't open glyph font"));
	/*NOTREACHED*/

#ifdef OW_I18N_L4
    /*
     * Next, look though the fontsets and ensure that a valid fontset is
     * in existance for all of the necessary fonts. If an invalid fontset
     * is detected, attempt to "fall back"...
     */  
    if(GRV.TitleFontSetInfo.fs == NULL)
	FallBack(dpy, &GRV.TitleFontSetInfo);
    if(GRV.TextFontSetInfo.fs == NULL)
	FallBack(dpy, &GRV.TextFontSetInfo);
    if(GRV.ButtonFontSetInfo.fs == NULL)
	FallBack(dpy, &GRV.ButtonFontSetInfo);
    if(GRV.IconFontSetInfo.fs == NULL)
        FallBack(dpy, &GRV.IconFontSetInfo);
#else
    /*
     * Make sure that the the various fonts have been opened.
     */  
    if(GRV.TitleFontInfo == NULL)
        ErrorGeneral(GetString("can't open title font"));
    else if(GRV.TextFontInfo == NULL)
        ErrorGeneral(GetString("can't open text font"));
    else if(GRV.ButtonFontInfo == NULL)
        ErrorGeneral(GetString("can't open button font"));
    else if(GRV.IconFontInfo == NULL)
        ErrorGeneral(GetString("can't open icon font"));
#endif  /* OW_I18N_L4 */
}


/*
 * UpdateGlobals -- handle updates to the server's resource database.  Called
 * every time the server's RESOURCE_MANAGER property changes.  Refetches the 
 * user's database and the app-defaults database and merges them, and then 
 * replaces the global database with this new one.  This loses the resources 
 * that corresponded to the command-line arguments, but that should be OK 
 * since we should have set them to be immutable at startup time.
 */
void
UpdateGlobals(dpy)
    Display *dpy;
{
    XrmDatabase		userDB;
    XrmDatabase		newDB;
    Bool		dlangChanged = False;
#ifdef OW_I18N_L3
    char		oldBasicLocale[MAXNAMELEN + 1];
    char		oldDisplayLang[MAXNAMELEN + 1];
#endif

    updateWorkspaceBackground = False;
    forceKeyRegrab = False;

    userDB = GetUserDefaults(dpy);

    ScanResourceItemTable(dpy, LocaleItemTable, userDB, NULL, SR_UPDATING);

#ifdef OW_I18N_L3
    EffectOLLC(dpy, False, oldBasicLocale, oldDisplayLang);
#endif /* OW_I18N_L3 */

    /*
     * This re-fetches the app-defaults file every time the user database
     * changes.  This may be necessary if the locale changes.  It may also be
     * necessary if a resource disappears from the user's database.  In this
     * case, we will want the value to revert to a value in the app-defaults
     * file, a value that had been overridden before.
     */
    newDB = GetAppDefaults();
    XrmMergeDatabases(userDB, &newDB);

#ifdef OW_I18N_L3
    ScanResourceItemTable(dpy, MainItemTable, newDB,
			  oldBasicLocale, SR_UPDATING);

    if (strcmp(GRV.lc_dlang.locale, oldDisplayLang) != 0)
	dlangChanged = True;
#else
    ScanResourceItemTable(dpy, MainItemTable, newDB, NULL, SR_UPDATING);
#endif

    if (updateWorkspaceBackground)
	SetWorkspaceBackground(dpy);

    /* If the display language has changed, restart olwmslave */
    if(dlangChanged && GRV.RunSlaveProcess) {
	if(GRV.SlavePid)
	    SlaveStop();
	GRV.SlavePid = SlaveStart(ArgVec);
    }

    if (dlangChanged || UpdateBindings(dpy, newDB, forceKeyRegrab))
	ReInitAllMenus(dpy);

    XrmDestroyDatabase(OlwmDB);
    OlwmDB = newDB;
}


/* ===== Internationalization ============================================= */

#ifdef OW_I18N_L3

/*
 * setOLLCPosix
 *
 * For each locale category setting that's NULL, fetch its current POSIX 
 * setting and store it into GRV.
 */
static void
setOLLCPosix()
{
    OLLCItem *ollci;
    OLLCItem *last = &GRV.LC[OLLC_LC_MAX];


    (void) setlocale(LC_ALL, "");
#ifdef DEBUG
    fprintf(stderr, "Just bfore OLLCPosix\n");
    dump_locale();
#endif
    for (ollci = GRV.LC; ollci < last; ollci++) {
	if (ollci->locale == NULL && ollci->posixCategory >= 0)
	    ollci->locale =
		MemNewString(setlocale(ollci->posixCategory, NULL));
    }
#ifdef DEBUG
    dump_locale();
#endif
}


/*
 * GRVLCInit
 *
 * For each OPEN LOOK locale category, fill in its corresponding POSIX locale 
 * category identifier.  Note: this is not intended to be a complete mapping.
 */
static void
GRVLCInit()
{
    GRV.lc_basic.posixCategory		=  LC_CTYPE;
    GRV.lc_basic.envName		= "LC_CTYPE";
    GRV.lc_dlang.posixCategory		=  LC_MESSAGES;
    GRV.lc_dlang.envName		= "LC_MESSAGES";
    GRV.lc_ilang.posixCategory		= -1;
    GRV.lc_ilang.envName		= NULL;
    GRV.lc_numeric.posixCategory	=  LC_NUMERIC;
    GRV.lc_numeric.envName		= "LC_NUMERIC";
    GRV.lc_time.posixCategory		=  LC_TIME;
    GRV.lc_time.envName			= "LC_TIME";
}


/*
 * EffectOLLC
 *
 * Apply restrictions to locale category combinations and then effect locale 
 * changes as necessary (using setlocale()).  Restrictions are as follows:
 *
 * + The basic locale can be changed from C to a non-C locale.  However, once
 * in a non-C locale, it can never be changed again.  If Olwm is in a non-C
 * locale, it can support only applications in that locale and applications in
 * the C locale.  It cannot support applications in multiple non-C locales, as
 * that might require Olwm to switch between locales from window to window, 
 * which it can't do.
 *
 * + If the first or initial time through, then ignore the above restriction.
 * This is because the locale may be set to a non-C  locale in olwm.c/main()
 * using an environment variable.  Here we're using the locale setting from
 * the resources which override the environment.  So the first time through
 * set the locale using the resource setting.
 *
 * + If the basic locale is C, all other locale categories must be C.  If the 
 * basic locale is non-C, the other categories must either be C or must match 
 * the basic locale.
 *
 * REMIND: need to check return values from setlocale().
 */
void
EffectOLLC(dpy, initial, oldBasicLocale, oldDisplayLang)
    Display *dpy;
    Bool     initial;
    char    *oldBasicLocale;
    char    *oldDisplayLang;
{
    OLLCItem *ollci;
    char *basic, *new, *cur;
    Bool basic_updated = False;
    Bool sticky_locale;

#ifdef DEBUG
    fprintf(stderr, "Before calling EffectOLLC\n");
    dump_locale();
#endif
    /*
     * Apply restrictions to the basic locale if current locale is not
     * sticky locale (sticky locale is defined to be locale which uses
     * none iso latin1 as characterset), updating if necessary.
     * Ensure that GRV.lc_basic matches reality.
     *
     * Note: update using LC_ALL in order to get the POSIX locale
     * categories that aren't covered by the OPEN LOOK locale
     * categories.  This forces us to update all the other locale
     * categories, even if they otherwise wouldn't need to be updated.
     */
    if (initial || strcmp(GRV.CharacterSet, ISO_LATIN_1) == 0)
	sticky_locale = False;
    else
	sticky_locale = True;

    basic = MemNewString(setlocale(LC_CTYPE, NULL));
    if (oldBasicLocale != NULL)
	(void) strcpy(oldBasicLocale, basic);
    if (oldDisplayLang != NULL)
	(void) strcpy(oldDisplayLang, setlocale(LC_MESSAGES, NULL));
    if (initial || ! sticky_locale ||
	(strcmp(basic, "C") == 0 && strcmp(GRV.lc_basic.locale, "C") != 0) )
    {
#ifdef DEBUG
	fprintf(stderr, "Basic Locale -> %s\n", GRV.lc_basic.locale);
#endif
	setlocale(LC_ALL, GRV.lc_basic.locale);
	basic_updated = True;
#ifdef OW_I18N_L4
	/*
	 * Check with Xlib to see basiclocale/LC_CTYPE is supported or
	 * not.
	 */
	if (! XSupportsLocale()) {
	    /*
	     * Assumption: "C" locale is always supported by the Xlib.
	     */
	    (void) fprintf(stderr, "%s: Supplied locale (%s) is not supported by Xlib - defaulting to C\n",
			   ProgramName, GRV.lc_basic.locale);
	    (void) setlocale(LC_ALL, "C");
	    if (strcmp(basic, "C") == 0)
	        basic_updated = False;
	    MemFree(GRV.lc_basic.locale);
	    GRV.lc_basic.locale = MemNewString("C");
	    MemFree(GRV.lc_dlang.locale);
	    GRV.lc_dlang.locale = MemNewString("C");
	}
	if (! XSetLocaleModifiers(""))
	    (void) fprintf(stderr, "%s: Error in setting locale modifier to Xlib\n",
			   ProgramName);
#endif
	MemFree(basic);
	basic = MemNewString(GRV.lc_basic.locale);
    } else if (strcmp(basic, GRV.lc_basic.locale) != 0) {
	MemFree(GRV.lc_basic.locale);
	GRV.lc_basic.locale = MemNewString(basic);
    }

    /*
     * Run through the other locale categories, applying the restrictions, and
     * updating if necessary.  Skip categories that have no corresponding
     * Posix locale category.  As before, make sure the value in GRV matches
     * the actual current setting.
     */
    for (ollci = GRV.LC + 1; ollci < &GRV.LC[OLLC_LC_MAX]; ++ollci) {

	if (ollci->posixCategory < 0)
	    continue;

	if (sticky_locale) {
	    if (strcmp(basic, "C") != 0
	        && strcmp(ollci->locale, "C") != 0)
	    {
	        new = basic;
	    } else {
	        new = "C";
	    }
	} else
	    new = ollci->locale;

	cur = setlocale(ollci->posixCategory, NULL);
	if (basic_updated || strcmp(cur, new) != 0) {
#ifdef DEBUG
	    fprintf(stderr, "locale#%d -> %s\n", ollci->posixCategory, new);
#endif
	    setlocale(ollci->posixCategory, new);
	}

	if (strcmp(ollci->locale, new) != 0) {
	    MemFree(ollci->locale);
	    ollci->locale = MemNewString(new);
	}
    }

    SetLocaleEnv(dpy);

    MemFree(basic);
#ifdef DEBUG
    dump_locale();
#endif
}

#endif /* OW_I18N_L3 */
