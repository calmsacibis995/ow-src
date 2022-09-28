#pragma ident	"@(#)olitsampler.c	1.33    93/11/01 bin/olitsampler SMI"    /* olexamples:tutorial/s_sampler.c 1.33 */

/*
 *	Copyright (C) 1989, 1993  Sun Microsystems, Inc
 *			All rights reserved.
 *		Notice of copyright on this source code
 *		product does not indicate publication.
 *
 * RESTRICTED RIGHTS LEGEND: Use, duplication, or disclosure by
 * the U.S. Government is subject to restrictions as set forth
 * in subparagraph (c)(1)(ii) of the Rights in Technical Data
 * and Computer Software Clause at DFARS 252.227-7013 (Oct. 1988)
 * and FAR 52.227-19 (c) (June 1987).
 *
 * This file is a product of Sun Microsystems, Inc. and is
 * provided for unrestricted use provided that this legend is
 * included on all media and as a part of the software
 * program in whole or part.  Users may copy or modify this
 * file without charge, but are not authorized to license or
 * distribute it to anyone else except as part of a product or
 * program developed by the user.
 *
 * THIS FILE IS PROVIDED AS IS WITH NO WARRANTIES OF ANY KIND
 * INCLUDING THE WARRANTIES OF DESIGN, MERCHANTIBILITY AND
 * FITNESS FOR A PARTICULAR PURPOSE, OR ARISING FROM A COURSE
 * OF DEALING, USAGE OR TRADE PRACTICE.
 *
 * This file is provided with no support and without any
 * obligation on the part of Sun Microsystems, Inc. to assist
 * in its use, correction, modification or enhancement.
 *
 * SUN MICROSYSTEMS, INC. SHALL HAVE NO LIABILITY WITH RESPECT
 * TO THE INFRINGEMENT OF COPYRIGHTS, TRADE SECRETS OR ANY
 * PATENTS BY THIS FILE OR ANY PART THEREOF.
 *
 * In no event will Sun Microsystems, Inc. be liable for any
 * lost revenue or profits or other special, indirect and
 * consequential damages, even if Sun has been advised of the
 * possibility of such damages.
 *
 *	Sun Microsystems, Inc., 2550 Garcia Avenue,
 *	Mountain View, California 94043.
 *
 */

/*	Copyright (c) 1990 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988 AT&T */
/*	All Rights Reserved	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF		*/
/*	UNIX System Laboratories, Inc.				*/
/*	The copyright notice above does not evidence any	*/
/*	actual or intended publication of such source code.	*/

/************************************************************************/
/*	OPEN LOOK WIDGET SAMPLER : prototype widgets and use of form	*/
/*									*/
/*	Copyright (c) 1989 AT&T						*/
/*	Copyright (c) 1988 Hewlett-Packard Company			*/
/*	Copyright (c) 1988 Massachusetts Institute of Technology	*/
/************************************************************************/


/************************************************************************
 *
 *	Implementation of the olitsampler demo
 *		
 ************************************************************************/


/************************************************************************
 *
 *      Imported Interfaces 
 *
 ************************************************************************/

#include <Xol/OpenLook.h>

/*
 * Headers required for creating widget instances.
 */
#include <Xol/AbbrevMenu.h>
#include <Xol/BulletinBo.h>
#include <Xol/Caption.h>
#include <Xol/CheckBox.h>
#include <Xol/ControlAre.h>
#include <Xol/DrawArea.h>
#include <Xol/Exclusives.h>
#include <Xol/FileChSh.h>
#include <Xol/FontChSh.h>
#include <Xol/Form.h>
#include <Xol/Gauge.h>
#include <Xol/MenuButton.h>
#include <Xol/Nonexclusi.h>
#include <Xol/Notice.h>
#include <Xol/NumericFie.h>
#include <Xol/OblongButt.h>
#include <Xol/PopupWindo.h>
#include <Xol/RectButton.h>
#include <Xol/Scrollbar.h>
#include <Xol/ScrolledWi.h>
#include <Xol/ScrollingL.h>
#include <Xol/Slider.h>
#include <Xol/StaticText.h>
#include <Xol/TextEdit.h>
#include <Xol/TextLine.h>

/*
 * Headers required for creating flat widget instances.
 */
#include <Xol/FExclusive.h>
#include <Xol/FNonexclus.h>
#include <Xol/FCheckBox.h>

/*
 * Other required OLIT supporting headers.
 */
#include <Xol/OlCursors.h>	/* GetOlQuestionCursor() */
#include <Xol/textbuff.h>

#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>

#include <locale.h>
#include <ctype.h>
#include <errno.h>      	/* errno */
#include <fcntl.h>
#include <libintl.h>		/* dgettext() */
#include <libgen.h>		/* basename() */
#include <limits.h>		/* MAX_PATH */

#ifndef	MAX_PATH
	#include <sys/param.h>	/* MAXPATHLEN */
#endif	/* MAX_PATH */

#include <stdio.h>		/* printf() */
#include <stdlib.h>		/* exit(), EXIT_FAILURE, mbstowcs() */
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <widec.h>


/************************************************************************
 *
 *      Module Private Macro Definitions
 *
 ************************************************************************/

#ifdef	MAX_PATH
	#define	BIGGEST_PATH	(MAX_PATH + 1)
#else
	#define	BIGGEST_PATH	(MAXPATHLEN)
#endif	/* MAX_PATH */

/*
 * Ol_PointToPixel() scales to the current screen resolution.
 */
#define N1_H_PIXEL	Ol_PointToPixel(OL_HORIZONTAL, 1)
#define N1_V_PIXEL	Ol_PointToPixel(OL_VERTICAL, 1)
#define N10_H_PIXELS	Ol_PointToPixel(OL_HORIZONTAL, 10)
#define N10_V_PIXELS	Ol_PointToPixel(OL_VERTICAL, 10)
#define N50_H_PIXELS	Ol_PointToPixel(OL_HORIZONTAL, 50)
#define N50_V_PIXELS	Ol_PointToPixel(OL_VERTICAL, 50)
#define N100_H_PIXELS	Ol_PointToPixel(OL_HORIZONTAL, 100)
#define N100_V_PIXELS	Ol_PointToPixel(OL_VERTICAL, 100)
#define N150_H_PIXELS	Ol_PointToPixel(OL_HORIZONTAL, 150)
#define N150_V_PIXELS	Ol_PointToPixel(OL_VERTICAL, 150)
#define N200_H_PIXELS	Ol_PointToPixel(OL_HORIZONTAL, 200)

/* Domain name for localized messages */
#define	MessagesDomain	"SUNW_WST_OLITSAMPLER"


/************************************************************************
 *
 *      Module Private Global Storage Allocation 
 *
 ************************************************************************/

static const char*	fallback_resources[] = { 
	"*Background:			gray",

	"*file_mb.label:		File",
	"*new_ob.label:			New",
	"*open_ob.label:		Open...",
	"*save_ob.label:		Save",
	"*save_as_ob.label:		Save As...",
	"*include_ob.label:		Include...",
	"*print_one_ob.label:		Print One",
	"*print_ob.label:		Print...",

	NULL
};

static char		lc_messages[2 * BUFSIZ];

static Pixel		red_pixel,
			blue_pixel,
			purple_pixel,
			green_pixel,
			yellow_pixel,
			orange_pixel,
			skyblue_pixel;

static int		i;
static Boolean		rainbow = FALSE;

/*
 * THE WIDGET TREE:
 *
 *				Toplevel
 *				   |
 *			         Caption
 *			           |
 *			         Form
 *			           |
 *			Remaining OPEN LOOK widgets, gadgets, and flats
 */

/*
 * Note: In many cases, a widget is labelled by making it the child of a
 * caption, which itself is a child of the form.
 */
static Widget
		abbmenu_caption,	/* caption for abbreviatedmenubutton */
		bb_caption,		/* caption for bulletinboard */
		caption,		/* "True" caption */
		ca_caption,		/* caption for controlarea */
		cb_caption,		/* caption for checkbox */
		f_caption,		/* caption for flats */
		fm_caption,		/* caption for form */
		g_caption,		/* caption for gauge */
		gd_caption,		/* caption for gadgets */
		slider_caption,		/* caption for slider */
		slist_caption,		/* caption for scrollinglist */
		sw_caption,		/* caption for scrolledwindow */
		te_caption,		/* caption for textedit */

		basewin,		/* top level widget */
		saveas,
		textedit,

		controlarea,
		drawarea,
		fontchshell,
		form,
		gauge,
		nonexclusives,
		nebutton2,
		nebutton3,
		noticeshell,
		noticebox,
		popupshell0,
		popupshell2,
		scrollbar,
		basewin;

/*
 * Form constraint resources used for most widgets in application.
 */
static Arg	generic_args[] = {
	{ XtNxRefName,		NULL },
	{ XtNyRefName,		NULL },
	{ XtNxOffset,		0 },	/* to be initialized below */
	{ XtNyOffset,		0 },	/* to be initialized below */
	{ XtNxAddWidth,		TRUE },
	{ XtNyAddHeight,	TRUE },
};


/************************************************************************
 *
 *      Forward Declaration Of Module Private Functions
 *
 ************************************************************************/

static void	update_gauge(const Widget widget);

static void	update_footers(const String left_string, 
	const String right_string);

static void	update_left_footer(const String string);
static void	update_right_footer(const String string);
static void	initialize_colors(const Widget basewin);
static void	set_position(const Widget widget, String xwidget, String ywidget);
static void	set_draw_area_cursor(Widget widget);

static void	draw_area_eh(Widget widget, XtPointer closure,
			XEvent* event, Boolean* continue_to_dispatch);

static void	fontch_cb(Widget widget, XtPointer client_data,
			XtPointer call_data);

static void	fontchshell_cb(Widget widget, XtPointer client_data,
			XtPointer call_data);

static void	drawarea_cb(Widget widget, XtPointer client_data,
			XtPointer call_data);

static void	generic_cb(Widget widget, XtPointer client_data,
			XtPointer call_data);

static void	menu_select_cb(Widget widget, XtPointer client_data,
			XtPointer call_data);

static void	nonexclusives_cb(Widget widget, XtPointer client_data,
			XtPointer call_data);

static void	notice_cb1(Widget widget, XtPointer call_data,
			XtPointer client_data);

static void	notice_cb2(Widget widget, XtPointer client_data,
			XtPointer call_data);

static void	popup_cb(Widget widget, XtPointer client_data,
			XtPointer call_data);

static void	rainbow_cb(Widget widget, XtPointer client_data,
			XtPointer call_data);

static void	scrollbar_cb(Widget widget, XtPointer client_data,
			XtPointer call_data);

static void	scrollinglist_cb(Widget widget, XtPointer client_data,
			XtPointer call_data);

static void	slider_cb(Widget widget, XtPointer client_data,
			XtPointer call_data);

static void	textline_cb(Widget widget, XtPointer client_data,
			XtPointer call_data);

static void	construct_file_menu_button(const Widget parent);

static void	select_cb(Widget wid, XtPointer client_data, 
			XtPointer call_data);

static void	new_cb(Widget wid, XtPointer client_data, 
	XtPointer call_data);

static void	save_cb(Widget wid, XtPointer client_data, 
			XtPointer call_data);

static void	any_cb(Widget widget, XtPointer client_data,
			XtPointer call_data);

static void	print_cb(Widget widget, XtPointer client_data,
			XtPointer call_data);

static void	save_document(const TextEditWidget tew, const String path_name);

static void	insert_document(const TextEditWidget tew, 
	const String path_name);


/************************************************************************
 *
 *      Implementation Of this Module's External Functions
 *
 ************************************************************************/


/************************************************************************
 *
 *  main --
 *
 ************************************************************************/

void
main(const int argc, const char*const argv[])
{
	XtAppContext		app_context;
	String			openwin_path;
	char            	domain_path[BIGGEST_PATH];
        char            	messages_file[BIGGEST_PATH];
        String			locale;
        String			lang_locale;
	Widget			tmpwidget;

	/* Make sure the syscall error handler is reset */
        errno = 0;
	
	/* Warn if OPENWINHOME is unset... messaging defaults to English */
        if ((openwin_path = getenv("OPENWINHOME")) == NULL) {
                (void) fprintf(stderr, "%s\n", dgettext(MessagesDomain, 
		   "Please set OPENWINHOME"));
		exit(EXIT_FAILURE);
        }

	/* Initialize OLIT and the Intrinsics */

	(void) OlToolkitInitialize(NULL); 
	
	/* This instructs toolkit to create multibyte widgets by default */
	OlSetDefaultTextFormat(OL_MB_STR_REP);

	basewin = XtVaAppInitialize(&app_context, "Olitsampler", 
		(XrmOptionDescList)NULL, 0, (int*)&argc, (String*)argv, 
		(String*)fallback_resources, 
			XtNfooterPresent,	TRUE,
			XtNresizeCorners,	FALSE,
		NULL);
	
	/* Locate locale-specific message file for olitsampler */

        (void)sprintf(domain_path, "%s/lib/locale", openwin_path);
	(void)bindtextdomain(MessagesDomain, domain_path);

	/*
	 * Get colors to use later.
	 */
	initialize_colors(basewin);

	/*
	 * Set FORM resources for the specific screen.
	 */
	generic_args[2].value = N10_H_PIXELS;
	generic_args[3].value = N10_V_PIXELS;

	/*
	 * Make all the widgets and then do placement on the form last.
	 */

	/*
	 * FORM: Form's caption is the top child of the footerpanel.
	 */
	fm_caption = XtVaCreateManagedWidget("fm_caption",
		captionWidgetClass, basewin,
			XtNposition,		OL_TOP,
			XtNalignment,		OL_CENTER,
		NULL);

	form = XtVaCreateManagedWidget("form", formWidgetClass, fm_caption, 
		NULL);

	/*
	 * Register help for the form, using the file "form.help"
	 */
	(void) OlRegisterHelp(OL_WIDGET_HELP, form, "Form Widget", 
		OL_DISK_SOURCE, "olitsampler.info");

	/*
	 * All the remaining widgets will be on the form.
	 *
	 * CAPTION: to label the controlarea, latter to be created next.
	 */
	ca_caption = XtVaCreateManagedWidget("ca_caption",
		captionWidgetClass, form,
			XtNxResizable,		TRUE,
			XtNxAttachRight,	TRUE,
		NULL);

	/*
	 * CONTROLAREA: the control area contains a popup command window,
	 * a popup property window, and an Delete button with a notice widget
	 * which pops up when the button is selected.
	 */
	controlarea = XtVaCreateManagedWidget("controlarea",
		controlAreaWidgetClass, ca_caption, NULL);

	/*
	 * FILECHOOSERS
	 */
	construct_file_menu_button(controlarea);

	/*
	 * POPUP: Used for Command Window.
	 */
	{
		Widget	popupca11,
			popupca12,
			popupshell1;

		tmpwidget = XtVaCreateManagedWidget("popupbutton1", 
			oblongButtonWidgetClass, controlarea, NULL);

		/*
		 * Make the popup shell first.
		 */
		popupshell1 = XtVaCreatePopupShell("popupshell1",
			popupWindowShellWidgetClass, tmpwidget, NULL);

		/*
		 * Add callback to popup button now that we have popupshell
		 * widget ID.
		 */
		XtAddCallback(tmpwidget, XtNselect, popup_cb, popupshell1);

		/*
		 * The popup window automatically makes three children: upper
		 * and lower control areas and footer: get widget IDs to
		 * populate them.
		 */
		XtVaGetValues(popupshell1,
				XtNupperControlArea,	&popupca11,
				XtNlowerControlArea,	&popupca12,
			NULL);

		tmpwidget = XtVaCreateManagedWidget("textline",
			textLineWidgetClass, popupca11, NULL);

		/*
		 * Callback to "read" user input on <TAB> , <CR> or <FocusOut>
		 */
		XtAddCallback(tmpwidget, XtNcommitCallback, textline_cb, NULL);

		/*
		 * Populate popup lower controlarea with buttons: one must be a
		 * default.
		 */
		XtVaCreateManagedWidget("option1", oblongButtonWidgetClass,	
			popupca12,
				XtNdefault,		TRUE,
			NULL);

		XtVaCreateManagedWidget("option2", oblongButtonWidgetClass, 
			popupca12, NULL);
	}


	/*
	 * POPUP: Used for Property Sheet.
	 */
	{
		Widget		exclusives,
				widget,
				popupca21,
				popupbutton2;

		/*
		 * XtCallbackRec used for widget SetValues().
		 */

		static XtCallbackRec popup_applyCBR[] = {
			{ generic_cb, (XtPointer)5 },
			{ NULL, NULL },
		};

		static XtCallbackRec popup_setdefaultsCBR[] = {
			{ generic_cb, (XtPointer)6 },
			{ NULL, NULL },
		};

		static XtCallbackRec popup_resetCBR[] = {
			{ generic_cb, (XtPointer)7 },
			{ NULL, NULL },
		};

		static XtCallbackRec popup_resetfactoryCBR[] = {
			{ generic_cb, (XtPointer)8 },
			{ NULL, NULL },
		};

		popupbutton2 = XtVaCreateManagedWidget("popupbutton2",
			oblongButtonWidgetClass, controlarea, NULL);

		/*
		 * Make the popup shell first: NOTE: callbacks must be set for
		 * creation of the automatic buttons. In this example, mapping
		 * is to one function but does not have to be.
		 */
		popupshell2 = XtVaCreatePopupShell("popupshell2",
			popupWindowShellWidgetClass, popupbutton2,
				XtNreset,		popup_resetCBR,
				XtNapply,		popup_applyCBR,
				XtNresetFactory,	popup_setdefaultsCBR,
			NULL);

		/*
		 * Add callback to popup button now that we have popupshell
		 * widget ID.
		 */
		XtAddCallback(popupbutton2, XtNselect, popup_cb, popupshell2);

		/*
		 * Get widget IDs of popup children needed.  Note that lower
		 * control area ID not needed since automatic buttons are
		 * created by code above.
		 */
		XtVaGetValues(popupshell2,
				XtNupperControlArea,	&popupca21,
			NULL);

		/*
		 * Populate popup upper control area with EXCLUSIVES and
		 * NONEXLCUSIVES. Note that there is no need to populate popup
		 * lower controlarea.
		 */
		XtVaSetValues(popupca21,
				XtNlayoutType,		OL_FIXEDCOLS,
			NULL);

		widget = XtVaCreateManagedWidget("exclus", captionWidgetClass, 
			popupca21, NULL);

		exclusives = XtVaCreateManagedWidget("exclusives",
			exclusivesWidgetClass, widget,
				XtNlayoutType,		OL_FIXEDROWS,
				XtNmeasure,		1,
			NULL);

		widget = XtVaCreateManagedWidget("rbutton",
			rectButtonWidgetClass, exclusives, NULL);

		XtAddCallback(widget, XtNselect, generic_cb, (XtPointer)2);

		widget = XtVaCreateManagedWidget("rbutton1",
			rectButtonWidgetClass, exclusives, NULL);

		XtAddCallback(widget, XtNselect, generic_cb, (XtPointer)3);

		widget = XtVaCreateManagedWidget("rbutton2",
			rectButtonWidgetClass, exclusives, NULL);

		XtAddCallback(widget, XtNselect, generic_cb, (XtPointer)4);

		widget = XtVaCreateManagedWidget("nexclus", captionWidgetClass, 
			popupca21, NULL);

		nonexclusives = XtVaCreateManagedWidget("nonexclusives",
			nonexclusivesWidgetClass, widget,
				XtNlayoutType,		OL_FIXEDROWS,
				XtNmeasure,		1,
			NULL);

		widget = XtVaCreateManagedWidget("rbutton3",
			rectButtonWidgetClass, nonexclusives, NULL);

		XtAddCallback(widget, XtNselect, generic_cb, (XtPointer)2);

		nebutton2 = XtVaCreateManagedWidget("rbutton4",
			rectButtonWidgetClass, nonexclusives, NULL);

		XtAddCallback(nebutton2, XtNselect, nonexclusives_cb, 
			nonexclusives);

		XtAddCallback(nebutton2, XtNselect, generic_cb, (XtPointer)3);
	}


	/*
	 * TO MAKE A MENU:
	 *
	 * Widget Tree for creating menu:
	 *
	 *		( parent widget )
	 *			|
	 *		menubutton ( visible/mouse sensitive symbol for menu )
	 *			|
	 *		pane ( used for placement of menu button set )
	 *			|
	 *		       / \
	 *		     /	   \
	 *		button1	  button2
	 *
	 * Create a menubutton and menu with a pushpin.
	 */
	{
		Widget	menupane;

		tmpwidget = XtVaCreateManagedWidget("menubutton1",
			menuButtonWidgetClass, controlarea,
				XtNpushpin,	OL_OUT,
			NULL);

		/*
		 * Get the Widget id of the menupane of the menubutton.
		 */
		XtVaGetValues(tmpwidget,
				XtNmenuPane,	&menupane,
			NULL);

		/*
		 * Make two oblongbuttons on the menupane, with select
		 * callbacks.
		 */
		tmpwidget = XtVaCreateManagedWidget("oblong1",
			oblongButtonWidgetClass, menupane, NULL);

		XtAddCallback(tmpwidget, XtNselect, menu_select_cb, (XtPointer)1);

		tmpwidget = XtVaCreateManagedWidget("oblong2",
			oblongButtonWidgetClass, menupane, NULL);

		XtAddCallback(tmpwidget, XtNselect, menu_select_cb, (XtPointer)2);
	}


	/*
	 * NOTICE: attach to an DELETE button in the control area.
	 */
	{
		tmpwidget = XtVaCreateManagedWidget("delbutton",
			oblongButtonWidgetClass, controlarea, NULL);

		/*
		 * Attach notice popup callback to "DELETE" controlarea button.
		 */
		XtAddCallback(tmpwidget, XtNselect, notice_cb1, NULL);

		/*
		 * Create notice popup shell.
		 */
		noticeshell = XtVaCreatePopupShell("noticeshell",
			noticeShellWidgetClass,	tmpwidget, NULL);

		/*
		 * Get the widget ids of noticebox and textarea of noticebox.
		 */
		XtVaGetValues(noticeshell, XtNcontrolArea, &noticebox, NULL);

		/*
		 * Add two buttons to noticebox.
		 *
		 * First button has an exit() callback.
		 */
		tmpwidget = XtVaCreateManagedWidget("delete", 
			oblongButtonWidgetClass, noticebox, NULL);

		XtAddCallback(tmpwidget, XtNselect, notice_cb2, noticebox);

		/*
		 * Second button is a no-op that pops down notice widget
		 * without exiting.
		 */
		XtVaCreateManagedWidget("cancel", oblongButtonWidgetClass,	
			noticebox, NULL);
	}


	/*
	 * FONTCHOOSER
	 */
	{
		tmpwidget = XtVaCreateManagedWidget("fontbutton",
			oblongButtonWidgetClass, controlarea, NULL);

		/*
		 * Attach Font Chooser to "fontbutton" controlarea button.
		 */
		XtAddCallback(tmpwidget, XtNselect, fontchshell_cb, NULL);

		/*
		 * Create Font Chooser
		 */
		fontchshell = XtVaCreatePopupShell("fontchoosershell",
			fontChooserShellWidgetClass, tmpwidget, NULL);

		XtVaGetValues(fontchshell,
			XtNfontChooserWidget, &tmpwidget, NULL);

		XtAddCallback(tmpwidget, XtNapplyCallback, fontch_cb, NULL);
		XtAddCallback(tmpwidget, XtNrevertCallback, fontch_cb, NULL);
	}


	/*
	 * CAPTION
	 */
	{
		caption = XtVaCreateManagedWidget("maincaption",
			captionWidgetClass, form,
				XtNposition,		OL_TOP,
				XtNalignment,		OL_CENTER,
			NULL);

		tmpwidget = XtVaCreateManagedWidget("rainbo",
			oblongButtonWidgetClass, caption, NULL);

		XtAddCallback(tmpwidget, XtNselect, rainbow_cb, NULL);
	}


	/*
	 * DRAWAREA: used as a drawing canvas.
	 *
	 * 
	 */
	{
		drawarea = XtVaCreateManagedWidget("drawarea",
			drawAreaWidgetClass, form,
				XtNheight,		(XtArgVal)N100_V_PIXELS,
				XtNwidth,		(XtArgVal)N100_H_PIXELS,
				XtNbackground,		skyblue_pixel,
			NULL);

		XtAddCallback(drawarea, XtNexposeCallback, drawarea_cb, NULL);

		/*
		 * Add an eventhandler to track when the pointer enters and
		 * leaves the DrawArea widget window.
		 */
		XtAddEventHandler(drawarea, EnterWindowMask|LeaveWindowMask,
			FALSE, draw_area_eh, NULL);

		/*
		 * Set the special cursor for DrawArea widget window
		 * below once the widget tree realized.
		 */
	}


	/*
	 * BULLETINBOARD: with ABBREVIATEDMENUBUTTON, NUMERICFIELD,
	 * CHECKBOX, and TEXTEDIT widgets in it, each within a
	 * caption labelling the widget.
	 */
	{
		Widget		bulletinboard,
				abbmenubutton,
				numericfield,
				abbmenupane,
				checkbox;

		Dimension	height,
				yvalue,
				xpad = (Dimension)N10_H_PIXELS,
				ypad = (Dimension)N10_V_PIXELS;

		int		initValue = 0,
				minValue = -10,
				maxValue = 10;


		/*
		 * BULLETINBOARD
		 */
		bb_caption = XtVaCreateManagedWidget("bb_caption",
			captionWidgetClass, form,
				XtNposition,	OL_TOP,
				XtNalignment,	OL_CENTER,
			NULL);

		bulletinboard = XtVaCreateManagedWidget("bulletinboard",
			bulletinBoardWidgetClass, bb_caption,
				XtNborderWidth,	2,
				XtNborderColor,	orange_pixel,
			NULL);

		/*
		 * ABBREVIATEDMENUBUTTON
		 */
		yvalue = ypad;

		abbmenu_caption = XtVaCreateManagedWidget("abbmenu_caption",
			captionWidgetClass, bulletinboard,
				XtNx,		xpad,
				XtNy,		yvalue,
			NULL);

		abbmenubutton = XtVaCreateManagedWidget("abbmenubutton",
			abbrevMenuButtonWidgetClass, abbmenu_caption, NULL);

		/*
		 * Get the Widget ID of the menupane of the abbreviated menu
		 * button.
		 */
		XtVaGetValues(abbmenubutton,
				XtNmenuPane,	&abbmenupane,
			NULL);

		tmpwidget = XtVaCreateManagedWidget("am_butt1",
			oblongButtonWidgetClass, abbmenupane, NULL);

		XtAddCallback(tmpwidget, XtNselect, menu_select_cb, (XtPointer)1);

		tmpwidget = XtVaCreateManagedWidget("am_butt2",
			oblongButtonWidgetClass, abbmenupane, NULL);

		XtAddCallback(tmpwidget, XtNselect, menu_select_cb, (XtPointer)2);


		/*
		 * NUMERICFIELD
		 */
		XtVaGetValues(abbmenubutton,
				XtNheight,	&height,
			NULL);

		yvalue = yvalue + height + ypad;

		numericfield = XtVaCreateManagedWidget("numericfield",
			numericFieldWidgetClass, bulletinboard,
				XtNx,			xpad,
				XtNy,			yvalue,
				XtNvalue,		&initValue,
				XtNminValue,		&minValue,
				XtNmaxValue,		&maxValue,
				XtNmaximumChars,	3,
			NULL);

		/*
		 * CHECKBOX
		 */
		XtVaGetValues(abbmenubutton,
				XtNheight,	&height,
			NULL);

		yvalue = yvalue + height + ypad;

		cb_caption = XtVaCreateManagedWidget("cb_caption",
			captionWidgetClass, bulletinboard,
				XtNx,		xpad,
				XtNy,		yvalue,
			NULL);

		checkbox = XtVaCreateManagedWidget("checkbox",
			checkBoxWidgetClass, cb_caption, NULL);

		/*
		 * TEXTEDIT
		 */
		XtVaGetValues(checkbox,
				XtNheight,	&height,
			NULL);

		yvalue = yvalue + height + ypad;

		te_caption = XtVaCreateManagedWidget("te_caption",
			captionWidgetClass, bulletinboard,
				XtNx,		xpad,
				XtNy,		yvalue,
				XtNalignment,	OL_TOP,
			NULL);

		textedit = XtVaCreateManagedWidget("textedit",
			textEditWidgetClass, te_caption,
			/*	XtNtextFormat,	OL_WC_STR_REP, */
				XtNheight,	(XtArgVal)N50_V_PIXELS,
				XtNwidth,	(XtArgVal)N150_H_PIXELS,
				XtNuserData, 	NULL,
			NULL);
	}

	/*
	 * SCROLLINGLIST
	 */
	{
		OlSlistItemAttrs	item_attr;
		int			natoi = (int)'A';
		char			pad[] = "  ";

		/*
		 * Use a caption to label the scrollinglist.
		 */
		slist_caption = XtVaCreateManagedWidget("slist_caption",
			captionWidgetClass, form,
				XtNposition,		OL_TOP,
				XtNalignment,		OL_CENTER,
			NULL);

		tmpwidget = XtVaCreateManagedWidget("scrollinglist",
			scrollingListWidgetClass, slist_caption,
				XtNscrollingListMode,	OL_EXCLUSIVE_NONESET,
				XtNviewHeight,		7,
			NULL);

		(void) sprintf(lc_messages, dgettext(MessagesDomain, "ITEM"));

		for (i = 0; i < 10; i++) {
			/*
			 * set the label
			 */
			item_attr.flags = OlItemLabelType;
			item_attr.label_type = (OlDefine)OL_STRING;
			item_attr.flags |= OlItemLabel;
			item_attr.item_label = 
				(String)XtMalloc(strlen(lc_messages) + 3);
			strcpy(item_attr.item_label, lc_messages);
			pad[1] = natoi;
			strcat(item_attr.item_label, pad);

			/*
			 * set the mnemonic
			 */
			item_attr.flags |= OlItemMnemonic;
			item_attr.item_mnemonic = natoi++;

			/*
			 * Add item to end of the list
			 */
			OlSlistAddItem(tmpwidget, &item_attr,
				(OlSlistItemPtr)NULL);
		}

		/*
		 * Callbacks to be invoked when user selects/unselects item.
		 */
		XtAddCallback(tmpwidget, XtNitemCurrentCallback,
			scrollinglist_cb, NULL);
		XtAddCallback(tmpwidget, XtNitemNotCurrentCallback,
			scrollinglist_cb, NULL);
	}


	/*
	 * SCROLLED WINDOW
	 */
	{
		sw_caption = XtVaCreateManagedWidget("sw_caption",
			captionWidgetClass, form,
				XtNposition,	OL_TOP,
				XtNalignment,	OL_CENTER,
			NULL);

		tmpwidget = XtVaCreateManagedWidget("scrolledwindow",
			scrolledWindowWidgetClass, sw_caption,
				XtNheight,	(XtArgVal)N150_V_PIXELS,
				XtNwidth,	(XtArgVal)N100_H_PIXELS,
				XtNvStepSize,	20,
				XtNhStepSize,	20,
			NULL);

		XtAddCallback(tmpwidget, XtNvSliderMoved, scrollbar_cb, 
			(XtPointer)1);

		XtAddCallback(tmpwidget, XtNhSliderMoved, scrollbar_cb, 
			(XtPointer)2);

		/*
		 * Make a statictext widget in the scrolled window to scroll
		 * with the scrollbars.
		 */
		XtVaCreateManagedWidget("scroll_text",
			staticTextWidgetClass, tmpwidget,
				XtNheight,	(XtArgVal)N150_V_PIXELS,
			NULL);
	}


	/*
	 * SLIDER
	 *
	 * The slider will be used to change the background of the
	 * DrawArea widget:
	 * calculate the number of colors for the screen and set the slider
	 * range from 0 to (N-1) with a granularity of 1; see the slider_cb
	 * function for the rest of the code/functionality.
	 */
	{
		Display*	display = XtDisplay(basewin);
		int		screen = XDefaultScreen(display);
		int		n,
				ncolors = 2;

		n = XDefaultDepth(display, screen);

		for (i = 1; i < n; i++)
			ncolors = ncolors * 2;

		ncolors = ncolors - 1;

		slider_caption = XtVaCreateManagedWidget("slider_caption",
			captionWidgetClass, form,
				XtNposition,	OL_TOP,
				XtNalignment,	OL_CENTER,
			NULL);

		tmpwidget = XtVaCreateManagedWidget("slider",
			sliderWidgetClass, slider_caption,
				XtNwidth,	(XtArgVal)N200_H_PIXELS,
				XtNorientation,	OL_HORIZONTAL,
				XtNsliderMax,	ncolors,
				XtNgranularity,	1,
				XtNticks,	5,
				XtNtickUnit,	OL_SLIDERVALUE,
				XtNdragCBType,	OL_RELEASE,
			NULL);

		XtAddCallback(tmpwidget, XtNsliderMoved, slider_cb, NULL);
	}


	/*
	 * GAUGE
	 */
	g_caption = XtVaCreateManagedWidget("g_caption",
		captionWidgetClass, form,
			XtNposition,	OL_TOP,
			XtNalignment,	OL_CENTER,
		NULL);

	gauge = XtVaCreateManagedWidget("gauge",
		gaugeWidgetClass, g_caption,
			XtNwidth,	(XtArgVal)N200_H_PIXELS,
			XtNorientation,	OL_HORIZONTAL,
			XtNsliderMax,	100,
			XtNgranularity,	10,
			XtNticks,	10,
			XtNtickUnit,	OL_SLIDERVALUE,
		NULL);

	/*
	 * GADGETS: the oblongbutton gadget and abbreviatedmenubutton gadget
	 * are displayed in a bulletinboard, the latter in a caption entitled,
	 * "Gadgets."
	 */
	{
		Widget		carea,
				menupane;

		gd_caption = XtVaCreateManagedWidget("gd_caption",
			captionWidgetClass, form,
				XtNposition,	OL_TOP,
				XtNalignment,	OL_CENTER,
			NULL);

		carea = XtVaCreateManagedWidget("controlarea",
			controlAreaWidgetClass, gd_caption,
				XtNborderWidth,	2,
				XtNborderColor,	orange_pixel,
			NULL);

		tmpwidget = XtVaCreateManagedWidget("buttongadget",
			oblongButtonGadgetClass, carea,
				XtNx,		(XtArgVal)N10_H_PIXELS,
				XtNy,		(XtArgVal)N10_V_PIXELS,
			NULL);

		XtAddCallback(tmpwidget, XtNselect, generic_cb, (XtPointer)9);

		tmpwidget = XtVaCreateManagedWidget("menubutton",
			menuButtonGadgetClass, carea,
				XtNx,		(XtArgVal)N100_H_PIXELS,
				XtNy,		(XtArgVal)N10_V_PIXELS,
			NULL);

		XtVaGetValues(tmpwidget,
				XtNmenuPane,	&menupane,
			NULL);

		tmpwidget = XtVaCreateManagedWidget("mbutton_gd1",
			oblongButtonGadgetClass, menupane, NULL);

		XtAddCallback(tmpwidget, XtNselect, menu_select_cb, 
			(XtPointer)1);

		tmpwidget = XtVaCreateManagedWidget("mbutton_gd2",
			oblongButtonGadgetClass, menupane, NULL);

		XtAddCallback(tmpwidget, XtNselect, menu_select_cb, 
			(XtPointer)2);
	}


	/*
	 * FLAT WIDGETS: the flatexclusives, flatnonexclusives, and
	 * flatcheckbox are displayed in a bulletinboard, the latter in a
	 * caption entitled, "Flat Widgets."
	 */
	{
		static const char*	fields1[] = {
			XtNbackground, XtNlabel, XtNselectProc, XtNclientData
		};

		typedef struct _FlatData1 {
			XtArgVal	background;	/* item's background */
			XtArgVal	label;		/* item's label */
			XtArgVal	select;		/* ...select callback */
			XtArgVal	client_data;	/* client_data for
							 * callback */
		} FlatData1;

		static FlatData1*	items1;


		/*
		 * For flatcheckbox: note "customized" fields.
		 */
		static const char*	fields2[] = {
			XtNbackground, XtNlabel, XtNset, XtNselectProc, 
			XtNclientData
		};

		typedef struct _FlatData2 {
			XtArgVal	background;	/* item's background */
			XtArgVal	label;		/* item's label */
			XtArgVal	set;		/* item's set status */
			XtArgVal	select;		/* ... select callback */
			XtArgVal	client_data;	/* client_data for
							 * callback */
		} FlatData2;

		static FlatData2*	items2;

		#define NUM_BUTTONS	(3)
		
		typedef struct _ApplicationData {
			String		label[NUM_BUTTONS];
		} ApplicationData, *ApplicationDataPtr;

		ApplicationData	data;

		static XtResource resources[] = {
 			{ "flatchecklabel1", "FlatCheckLabel1", 
 				XtRString, sizeof(String), 
 				XtOffset(ApplicationDataPtr, label[0]), 
 				XtRString, "CheckBox 1" },

			{ "flatchecklabel2", "FlatCheckLabel2", 
				XtRString, sizeof(String),
				XtOffset(ApplicationDataPtr, label[1]), 
				XtRString, "CheckBox 2" },

			{ "flatchecklabel3", "FlatCheckLabel3", 
				XtRString, sizeof(String),
				XtOffset(ApplicationDataPtr, label[2]), 
				XtRString, "CheckBox 3" },
		};

		Widget		widget,
				carea,
				flatcbox;
		int		num_subobjects = 3;
		Dimension	hspace = N10_H_PIXELS,
				vspace = N10_V_PIXELS;

		/*
		 * For flatexclusives and flatnonexclusives: note "customized"
		 * fields.
		 */
		items1 = (FlatData1*)XtMalloc(
			(Cardinal)(num_subobjects * sizeof (FlatData1)));

		items2 = (FlatData2*)XtMalloc(
			(Cardinal)(num_subobjects * sizeof (FlatData2)));

		f_caption = XtVaCreateManagedWidget("f_caption",
			captionWidgetClass, form,
				XtNposition,	OL_TOP,
				XtNalignment,	OL_CENTER,
			NULL);

		carea = XtVaCreateManagedWidget("controlarea",
			controlAreaWidgetClass, f_caption,
				XtNborderWidth,		2,
				XtNborderColor,		orange_pixel,
				XtNhSpace,		hspace,
				XtNvSpace,		vspace,
				XtNhPad,		hspace,
				XtNvPad,		vspace,
			NULL);

		widget = XtVaCreateManagedWidget("exc_caption",
			captionWidgetClass, carea, NULL);

		items1[0].background = red_pixel;
		items1[1].background = green_pixel;
		items1[2].background = blue_pixel;

		items1[0].label =
			items1[1].label =
			items1[2].label = (XtArgVal)"";

		items1[0].select =
			items1[1].select =
			items1[2].select = (XtArgVal)generic_cb;

		items1[0].client_data =
			items1[1].client_data =
			items1[2].client_data = 10;

		XtVaCreateManagedWidget("flatexclusives",
			flatExclusivesWidgetClass, widget,
				XtNitems,		items1,
				XtNnumItems,		num_subobjects,
				XtNitemFields,		fields1,
				XtNnumItemFields,	XtNumber(fields1),
			NULL);

		widget = XtVaCreateManagedWidget("nexc_caption",
			captionWidgetClass, carea, NULL);

		XtVaCreateManagedWidget("flatnonexclusives",
			flatNonexclusivesWidgetClass, widget,
				XtNitems,		items1,
				XtNnumItems,		num_subobjects,
				XtNitemFields,		fields1,
				XtNnumItemFields,	XtNumber(fields1),
			NULL);

		items2[0].background = red_pixel;
		items2[1].background = green_pixel;
		items2[2].background = blue_pixel;

		items2[0].label = (XtArgVal)"CheckBox 1";
		items2[1].label = (XtArgVal)"CheckBox 2";
		items2[2].label = (XtArgVal)"CheckBox 3";

		items2[0].set =
			items2[1].set =
			items2[2].set = TRUE;

		items2[0].select =
			items2[1].select =
			items2[2].select = (XtArgVal)generic_cb;

		items2[0].client_data =
			items2[1].client_data =
			items2[2].client_data = 10;

		flatcbox = XtVaCreateManagedWidget("flatcheckbox",
			flatCheckBoxWidgetClass, carea,
				XtNitems,		items2,
				XtNnumItems,		num_subobjects,
				XtNitemFields,		fields2,
				XtNnumItemFields,	XtNumber(fields2),
			NULL);

		#undef NUM_BUTTONS
	}


	/*
	 * SCROLLBAR: attach to the right & bottom of the form.
	 */
	{
		scrollbar = XtVaCreateManagedWidget("scrollbar",
			scrollbarWidgetClass, form,
				XtNproportionLength,	10,
				XtNshowPage,		OL_LEFT,
				XtNyResizable,		TRUE,
				XtNyAttachBottom,	TRUE,
				XtNxAttachRight,	TRUE,
				XtNxVaryOffset,		TRUE,
			NULL);

		XtAddCallback(scrollbar, XtNsliderMoved, scrollbar_cb, 0);
	}


	/*
	 * Position all the widgets on the form.  Note that reference names are
	 * used. Note that 10 horizontal and vertical pixels are used in most
	 * cases; in a	few cases special values are used.
	 */
	{
		static			Dimension
					width1,
					width2;
		static Position		x1,
					x2;

		generic_args[2].value = 0;
		generic_args[3].value = 0;
		set_position(ca_caption, "form", "form");

		generic_args[2].value = N10_H_PIXELS;
		generic_args[3].value = N10_V_PIXELS;
		set_position(caption, "form", "ca_caption");
		set_position(drawarea, "form", "maincaption");

		/*
		 * Position bulletinboard to right of caption or drawarea,
		 * whichever is wider.
		 */
		XtVaGetValues(caption,
				XtNwidth,	&width1,
			NULL);

		XtVaGetValues(drawarea,
				XtNwidth,	&width2,
			NULL);

		if (width1 > width2)
			set_position(bb_caption, "maincaption", "ca_caption");
		else
			set_position(bb_caption, "drawarea", "ca_caption");

		set_position(slider_caption, "form", "bb_caption");
		set_position(g_caption, "form", "slider_caption");
		set_position(f_caption, "form", "g_caption");
		set_position(slist_caption, "bb_caption", "ca_caption");
		set_position(sw_caption, "slist_caption", "ca_caption");

		generic_args[2].value = (8 * N10_H_PIXELS);
		generic_args[3].value = (2 * N10_V_PIXELS);
		set_position(gd_caption, "slider_caption", "sw_caption");

		/*
		 * Position scrollbar to right of flat widgets or
		 * scrolledwindow, whichever extends further to the right of
		 * the form.
		 */
		generic_args[2].value = N50_H_PIXELS;
		generic_args[3].value = 0;

		XtVaGetValues(f_caption,
				XtNx,		&x1,
				XtNwidth,	&width1,
			NULL);

		XtVaGetValues(sw_caption,
				XtNx,		&x2,
				XtNwidth,	&width2,
			NULL);

		width1 = width1 + (Dimension)x1;
		width2 = width2 + (Dimension)x2;

		if (width1 > width2)
			set_position(scrollbar, "f_caption", "ca_caption");
		else
			set_position(scrollbar, "sw_caption", "ca_caption");

		generic_args[3].value = N10_V_PIXELS;
	}

	/*
	 * Realize the widget tree.
	 */
	XtRealizeWidget(basewin);

	/*
	 * The special cursor for the DrawArea widget can be set
	 * now that it has been realized as part of the widget tree.
	 */
	set_draw_area_cursor(drawarea);

	/*
	 * Turn control over to the Xt intrinsics and OPEN LOOK.
	 */
	XtAppMainLoop(app_context);
	/*NOTREACHED*/
	exit(EXIT_FAILURE);
	/*NOTREACHED*/
} /* end of main() */


/************************************************************************
 *
 *      Implementation Of this Module's Internal Functions
 *
 ************************************************************************/


/*
 * Function to vary value displayed in gauge widget.
 */
static void
update_gauge(const Widget widget)
{
	int		current_value;

	XtVaGetValues(widget,
			XtNsliderValue,	&current_value,
		NULL);

	current_value += 10;
	if (current_value > 100)
		current_value = 0;

	/*
	 * The following convenience routine is faster than a SetValues().
	 */
	OlSetGaugeValue(gauge, current_value);
} /* end of update_gauge() */


/*
 * Xt and Xlib examples to give functionality to the drawarea widget.
 */
/* ARGSUSED1 */
static void
drawarea_cb(Widget widget, XtPointer client_data, XtPointer call_data)
{
	Display*	display;
	static Window	window;
	static Boolean	done1 = FALSE,
			done2 = FALSE;
	static GC	gc[6];
	XGCValues	values;
	int		i,
			ix5,
			ix10,
			coord,
			axis,
			finish = 180 * 64;
	static int	hpixel,
			vpixel;
	Font		fid = NULL;
	XtGCMask	mask;
	String		fontname = 
			"-*-lucida*-bold-r-normal-sans-*-140-*-p-*-iso8859-1";

	display = XtDisplay(widget);
	window = XtWindow(widget);

	if (!done1) {
		hpixel = (int)N1_H_PIXEL;
		vpixel = (int)N1_V_PIXEL;
		if (hpixel == 0)
			hpixel = 1;
		if (vpixel == 0)
			vpixel = 1;
		done1 = TRUE;
	}

	if (rainbow) {
		values.line_width = 5 * hpixel;

		if (!done2) {
			mask = (XtGCMask)(GCForeground | GCLineWidth);

			values.foreground = red_pixel;
			gc[0] = XtGetGC(widget, mask, &values);
			values.foreground = orange_pixel;
			gc[1] = XtGetGC(widget, mask, &values);
			values.foreground = yellow_pixel;
			gc[2] = XtGetGC(widget, mask, &values);
			values.foreground = green_pixel;
			gc[3] = XtGetGC(widget, mask, &values);
			values.foreground = blue_pixel;
			gc[4] = XtGetGC(widget, mask, &values);
			values.foreground = purple_pixel;
			gc[5] = XtGetGC(widget, mask, &values);
		}

		for (i = 0; i < 6; i++) {
			ix5 = i * 5;
			ix10 = i * 10;

			axis = 95 - ix10;
			coord = ix5 + 5;

			XDrawArc(display, window, gc[i], 
				coord * hpixel, coord * vpixel,
				(unsigned int)(axis * hpixel),
				(unsigned int)(axis * vpixel),
				0, finish);
		}
	} else
		XClearWindow(display, window);

	/*
	 * Get font for this screen resolution.
	 */
	fid = XLoadFont(display, fontname);
	if (fid != (Font)NULL) {
		values.font = fid;
		gc[0] = XtGetGC(widget, (XtGCMask)GCFont, &values);
		XDrawString(display, window, gc[0], 8 * hpixel,
			70 * vpixel, "DrawArea", 11);
		XtReleaseGC(drawarea, gc[0]);
		XUnloadFont(display, fid);
	}

	/*
	 * This function merely varies the value displayed in the gauge.
	 */
	update_gauge(gauge);
} /* end of drawarea_cb() */


/*
 * Functions for changing the footer messages.
 */
 
/************************************************************************
 *
 *  update_footers -- 
 *
 ************************************************************************/

static void
update_footers(const String left_string, const String right_string)
{

	update_left_footer(left_string);
	update_right_footer(right_string);
} /* end of update_footers() */


/************************************************************************
 *
 *  update_left_footer -- 
 *
 ************************************************************************/

static void
update_left_footer(const String string)
{

	if (NULL != string)
		XtVaSetValues(basewin, XtNleftFooterString, string, NULL);
} /* end of update_left_footer() */


/************************************************************************
 *
 *  update_right_footer -- 
 *
 ************************************************************************/

static void
update_right_footer(const String string)
{

	if (NULL != string)
		XtVaSetValues(basewin, XtNrightFooterString, string, NULL);
} /* end of update_right_footer() */


/*
 * Function for getting color values for the display.
 */
static void
initialize_colors(const Widget basewin)
{
	static String	colors[] = {
		"purple", "blue", "green", "yellow", "orange", "red", "skyblue"
	};

	XrmValue	fromValue,
			toValue;

	for (i = 0; i < 7; i++) {
		fromValue.size = sizeof (colors[i]);
		fromValue.addr = colors[i];
		XtConvert(basewin, XtRString, &fromValue, XtRPixel, &toValue);

		switch (i) {
		case 0:
			purple_pixel = *((Pixel*)toValue.addr);
			break;
		case 1:
			blue_pixel = *((Pixel*)toValue.addr);
			break;
		case 2:
			green_pixel = *((Pixel*)toValue.addr);
			break;
		case 3:
			yellow_pixel = *((Pixel*)toValue.addr);
			break;
		case 4:
			orange_pixel = *((Pixel*)toValue.addr);
			break;
		case 5:
			red_pixel = *((Pixel*)toValue.addr);
			break;
		case 6:
			skyblue_pixel = *((Pixel*)toValue.addr);
			break;
		}
	}
} /* end of initialize_colors() */


/*
 * Function for positioning widgets on the form.
 *
 * Note: XtNx[y]RefName are used instead of XtNx[y]RefWidget so
 * that resources for each widget can be set in an .Xdefaults
 * file.  In addition, this also allows specifying the placement
 * of any widget on the form without first having to place the
 * corresponding reference widget(s) on the form.  (This would
 * NOT be the case with the XtNx[y]RefWidget resource).
 */
static void
set_position(const Widget widget, String xwidget, String ywidget)
{
	static int	nargs;

	if (nargs == 0)
		nargs = XtNumber(generic_args);

	generic_args[0].value = (XtArgVal)xwidget;
	generic_args[1].value = (XtArgVal)ywidget;
	XtSetValues(widget, generic_args, nargs);
} /* end of set_position() */


/*
 * Function for creating a special cursor for the DrawArea widget.
 */
static void
set_draw_area_cursor(Widget widget)
{
	static Cursor	cursor;

	/*
	 * See OlCursors.c for other cursor possibilities.
	 */
	cursor = GetOlQuestionCursor(XtScreen(widget));
	XDefineCursor(XtDisplay(widget), XtWindow(widget), cursor);
} /* end of set_draw_area_cursor() */


/*
 *  Event handler example to give functionality to the DrawArea widget.
 */
/*ARGSUSED*/
static void
draw_area_eh(
	Widget		widget,
	XtPointer	closure,
	XEvent*		event, 
	Boolean*	continue_to_dispatch
)
{

	if (event->type == EnterNotify) {
		(void) sprintf(lc_messages, "%s", dgettext(MessagesDomain, 
			"Pointer entered DrawArea widget"));
		update_footers(lc_messages, "");
	} else if (event->type == LeaveNotify) {
		(void) sprintf(lc_messages, "%s", dgettext(MessagesDomain, 
			"Pointer left DrawArea widget"));
		update_footers(lc_messages, "");
	}
} /* end of draw_area_eh() */


/*
 * CALLBACKS FOR WIDGETS
 */


/*ARGSUSED*/
static void
fontch_cb(Widget widget, XtPointer client_data, XtPointer call_data)
{
	OlAnyCallbackStruct *any_cd = (OlAnyCallbackStruct *)call_data;

	if (any_cd->reason == OL_REASON_APPLY_FONT) {
		OlFCApplyCallbackStruct* cd = (OlFCApplyCallbackStruct*)call_data;

		(void) sprintf(lc_messages, dgettext(MessagesDomain, 
			"Apply Callback (%s)"), cd->current_font_name);
		update_footers(lc_messages, "");
	} else if (any_cd->reason == OL_REASON_REVERT_FONT) {
		(void) sprintf(lc_messages, dgettext(MessagesDomain, 
			"Reverted Callback"));
		update_footers(lc_messages, "");
	}
} /* end of fontch_cb() */


/*ARGSUSED*/
static void
fontchshell_cb(Widget widget, XtPointer client_data, XtPointer call_data)
{

	XtPopup(fontchshell, XtGrabNone);
} /* end of fontchshell_cb() */


/*
 * With this callback, each widget passes its index as
 * client_data and thus maps to its own footerpanel message.
 */
/*ARGSUSED*/
static void
generic_cb(Widget widget, XtPointer client_data, XtPointer call_data)
{
	int		n = (int)client_data;

	switch (n) {
	case 1:
		(void) sprintf(lc_messages, "%s",
			dgettext(MessagesDomain, "OblongButton callback"));
			update_footers(lc_messages, "");
		break;

	case 2:
	case 3:
	case 4:
		(void) sprintf(lc_messages, "%s %d", dgettext(MessagesDomain,
			"[Non-]Exclusives callback for button "), n - 1);
		update_footers(lc_messages, "");
		break;
	case 5:
		(void) sprintf(lc_messages, "%s", dgettext(MessagesDomain, 
			"Properties Apply callback"));
		update_footers(lc_messages, "");
		break;
	case 6:
		(void) sprintf(lc_messages, "%s",
			dgettext(MessagesDomain, 
			"Properties Set Defaults callback"));
		update_footers(lc_messages, "");
		break;
	case 7:
		(void) sprintf(lc_messages, "%s",
		dgettext(MessagesDomain, "Properties Reset callback"));
		update_footers(lc_messages, "");
		break;
	case 8:
		(void) sprintf(lc_messages, "%s",
			dgettext(MessagesDomain, 
			"Properties Reset To Factory callback"));
		update_footers(lc_messages, "");
		break;
	case 9:
		(void) sprintf(lc_messages, "%s",
			dgettext(MessagesDomain, 
			"OblongButtonGadget callback"));
		update_right_footer(lc_messages);
		break;
	case 10:
		(void) sprintf(lc_messages, "%s",
			dgettext(MessagesDomain, 
			"Flat widget select callback"));
		update_right_footer(lc_messages);
		break;
	}
} /* end of generic_cb() */


/*
 * The MenuSelect_cb callback is used by the pulldown menu
 * popup menu, and abbreviatedmenubutton widget and gadget.
 */
/*ARGSUSED*/
static void
menu_select_cb(Widget widget, XtPointer client_data, XtPointer call_data)
{
	int		n = (int)client_data;

	(void) sprintf(lc_messages, dgettext(MessagesDomain, 
		"Button %d selected"), n);
	update_footers(lc_messages, "");
} /* end of menu_select_cb() */


/*ARGSUSED2*/
static void
nonexclusives_cb(Widget widget, XtPointer client_data, XtPointer call_data)
{
	Widget		parent = (Widget)client_data;

	/*
	 * If only "More" button, create "Fewer" button.
	 */
	if (widget == nebutton2 && nebutton3 == (Widget)0) {
		nebutton3 = XtVaCreateManagedWidget("Fewer",
			rectButtonWidgetClass, parent, NULL);

		XtAddCallback(nebutton3, XtNselect, nonexclusives_cb, 
			nonexclusives);

		XtAddCallback(nebutton3, XtNselect, generic_cb, (XtPointer)4);
	/*
	 * If all three buttons, delete "Fewer" button.
	 */
	} else if (widget == nebutton3 && nebutton3 != NULL) {
		XtDestroyWidget(nebutton3);
		nebutton3 = NULL;
	}
} /* end of nonexclusives_cb() */


/*ARGSUSED1*/
static void
notice_cb1(Widget widget, XtPointer call_data, XtPointer client_data)
{
/*
 * w = emanating button: where notice
 * does popup
 */
	XtVaSetValues(noticebox,
			XtNemanateWidget,	widget,
		NULL);

	XtPopup(noticeshell, XtGrabNonexclusive);
} /* end of notice_cb1() */


/*ARGSUSED0*/
static void
notice_cb2(Widget widget, XtPointer client_data, XtPointer call_data)
{
	(void) sprintf(lc_messages, "%s", dgettext(MessagesDomain, 
		"Delete callback"));
	update_footers(lc_messages, "");
} /* end of notice_cb2() */


/*ARGSUSED*/
static void
popup_cb(Widget widget, XtPointer client_data, XtPointer call_data)
{

	XtPopup(client_data, XtGrabNone);
} /* end of popup_cb() */


/*
 * The rainbow_cb callback is for the RAINBOW button.
 */
/*ARGSUSED*/
static void
rainbow_cb(Widget widget, XtPointer client_data, XtPointer call_data)
{

	if (rainbow)
		rainbow = FALSE;
	else
		rainbow = TRUE;

	drawarea_cb(drawarea, NULL, NULL);
} /* end of rainbow_cb() */


/*
 * This callback is used by all three scrollbars.
 */
static void
scrollbar_cb(Widget widget, XtPointer client_data, XtPointer call_data)
{
	OlScrollbarVerify*	sbv = (OlScrollbarVerify*)call_data;
	int			n = (int)client_data;

	sbv->ok = TRUE;

	switch (n) {
	case 0:
		(void) sprintf(lc_messages, "%s %d%%", dgettext(
			MessagesDomain, "Form ScrollBar moved to "), 
			sbv->new_location);
		break;
	case 1:
		(void) sprintf(lc_messages, "%s", dgettext(MessagesDomain, 
			"ScrolledWindow vertical scrollbar moved"));
		break;
	case 2:
		(void) sprintf(lc_messages, "%s", dgettext(MessagesDomain, 
			"ScrolledWindow horizontal scrollbar moved"));
		break;
	}

	update_footers(lc_messages, "");

	/*
	 * Update the form's scrollbar page indicator.
	 */
	if (widget == scrollbar)
		sbv->new_page = (int)(sbv->new_location / 10) + 1;
} /* end of scrollbar_cb() */


/*ARGSUSED*/
static void
scrollinglist_cb(Widget widget, XtPointer client_data, XtPointer call_data)
{
	OlAnyCallbackStruct *any_cd = (OlAnyCallbackStruct *)call_data;

	if (any_cd->reason == OL_REASON_ITEM_CURRENT) {
		OlSlistCallbackStruct* cd = (OlSlistCallbackStruct*)call_data;

		(void) sprintf(lc_messages, dgettext(MessagesDomain, 
			"ScrollingList item label %s (position %d)"),
			OlSlistGetItemLabel(widget, cd->item), cd->item_pos);
		update_footers(lc_messages, "");
	} else if (any_cd->reason == OL_REASON_ITEM_NOT_CURRENT)
		update_left_footer("");
} /* end of scrollinglist_cb() */


/*ARGSUSED*/
static void
slider_cb(Widget widget, XtPointer client_data, XtPointer call_data)
{
	OlSliderVerify*		sv = (OlSliderVerify*)call_data;

	/*
	 * Slider returns current value.
	 */
	XtVaSetValues(drawarea,
			XtNbackground,		sv->new_location,
		NULL);
} /* end of slider_cb() */


/*ARGSUSED1*/
static void
textline_cb(Widget widget, XtPointer client_data, XtPointer call_data)
{
	OlTLCommitCallbackStruct*	cd = 
					(OlTLCommitCallbackStruct*)call_data;

	if (cd->reason == OL_REASON_COMMIT) {
		(void) sprintf(lc_messages, dgettext(MessagesDomain,
			"TextLine User Input: %s"), cd->buffer);
		update_footers(lc_messages, "");
	}
} /* end of textline_cb() */


/************************************************************************
 *
 *  construct_file_menu_button --
 *
 ************************************************************************/

static void
construct_file_menu_button(const Widget parent)
{
	#define MAX_CHILDREN	(8)
	Widget			children[MAX_CHILDREN];
	Cardinal		num_children;

	Widget          
				file_mb,
					file_m,
						new_ob,
						open_ob,
							open_fcs,
								open_fc,
						save_ob,
							save_fcs,
								save_fc,
						save_as_ob,
							save_as_fcs,
								save_as_fc,
						include_ob,
							include_fcs,
								include_fc,
						spacer_ob,
						print_one_ob,
						print_ob
				;


	/* Instantiate the widget tree */

	file_mb = XtCreateWidget("file_mb", menuButtonWidgetClass, 
		parent, 0, NULL);

	XtVaGetValues(file_mb, XtNmenuPane, &file_m, NULL);

	/* 
	 * New 
	 */
	new_ob = XtVaCreateWidget("new_ob", oblongButtonWidgetClass, file_m,
		NULL);

	XtAddCallback(new_ob, XtNselect, new_cb, NULL);

	#define	ADD_CALLBACKS(widget_base_name) \
		{ \
			XtAddCallback(widget_base_name##_fc, \
				XtNopenFolderCallback, any_cb, NULL); \
			XtAddCallback(widget_base_name##_fc, \
				XtNlistChoiceCallback, any_cb, NULL); \
			XtAddCallback(widget_base_name##_fc, \
				XtNfolderOpenedCallback, any_cb, NULL); \
			XtAddCallback(widget_base_name##_fc, \
				XtNcancelCallback, any_cb, NULL); \
			XtAddCallback(widget_base_name##_fcs, \
				XtNverifyCallback, any_cb, NULL); \
			XtAddCallback(widget_base_name##_ob, \
				XtNselect, select_cb, widget_base_name##_fcs); \
		}

	/* 
	 * Open 
	 */
	open_ob = XtVaCreateWidget("open_ob", oblongButtonWidgetClass, file_m,
			XtNdefault,	TRUE,
		NULL);

	open_fcs = XtVaCreatePopupShell("open_fcs", fileChooserShellWidgetClass, 
		open_ob, NULL);

	XtVaGetValues(open_fcs, XtNfileChooserWidget, &open_fc, NULL);
	ADD_CALLBACKS(open);
	XtAddCallback(open_fc, XtNinputDocumentCallback, any_cb, NULL);

	/* 
	 * Save 
	 */
	save_ob = XtVaCreateWidget("save_ob", oblongButtonWidgetClass, file_m,
		NULL);

	save_fcs = XtVaCreatePopupShell("save_fcs", fileChooserShellWidgetClass, 
		save_ob, 
			XtNoperation, OL_SAVE,
		NULL);

	XtVaGetValues(save_fcs, XtNfileChooserWidget, &save_fc, NULL);
	XtAddCallback(save_fc, XtNopenFolderCallback, any_cb, NULL);
	XtAddCallback(save_fc, XtNlistChoiceCallback, any_cb, NULL);
	XtAddCallback(save_fc, XtNfolderOpenedCallback, any_cb, NULL);
	XtAddCallback(save_fc, XtNcancelCallback, any_cb, NULL);	
	XtAddCallback(save_ob, XtNselect, save_cb, save_fcs);

	XtAddCallback(save_fcs, XtNverifyCallback, any_cb, NULL);
	XtAddCallback(save_fc, XtNoutputDocumentCallback, any_cb, NULL);

	/* 
	 * Save As 
	 */
	save_as_ob = XtVaCreateWidget("save_as_ob", oblongButtonWidgetClass, 
		file_m,
		NULL);
	
	save_as_fcs = XtVaCreatePopupShell("save_as_fcs", 
		fileChooserShellWidgetClass, save_as_ob, 
			XtNoperation, 	OL_SAVE_AS,
		NULL);

	XtVaGetValues(save_as_fcs, XtNfileChooserWidget, &save_as_fc, NULL);
	saveas = save_as_fc;
	ADD_CALLBACKS(save_as);
	XtAddCallback(save_as_fc, XtNoutputDocumentCallback, any_cb, NULL);

	/* 
	 * Include 
	 */
	include_ob = XtVaCreateWidget("include_ob", oblongButtonWidgetClass, 
		file_m,	NULL);
	
	include_fcs = XtVaCreatePopupShell("include_fcs", 
		fileChooserShellWidgetClass, include_ob, 
			XtNoperation, 	OL_INCLUDE,
		NULL);

	XtVaGetValues(include_fcs, XtNfileChooserWidget, &include_fc, NULL);
	ADD_CALLBACKS(include);
	XtAddCallback(include_fc, XtNinputDocumentCallback, any_cb, NULL);

	/* 
	 * Spacer 
	 */
	spacer_ob = XtVaCreateWidget("spacer_ob", oblongButtonWidgetClass, 
		file_m,
			XtNsensitive,	FALSE,
			XtNlabel,	"",
		NULL);

	/* 
	 * Print One 
	 */
	print_one_ob = XtVaCreateWidget("print_one_ob", oblongButtonWidgetClass, 
		file_m, NULL);
	XtAddCallback(print_one_ob, XtNselect, print_cb, NULL);

	/* 
	 * Print
	 */
	print_ob = XtVaCreateWidget("print_ob", oblongButtonWidgetClass, 
		file_m, NULL);
	XtAddCallback(print_ob, XtNselect, print_cb, NULL);

	/* Manage components by their respective parents */
	num_children = 0;
	children[num_children++] = new_ob;
	children[num_children++] = open_ob;
	children[num_children++] = save_ob;
	children[num_children++] = save_as_ob;
	children[num_children++] = include_ob;
	children[num_children++] = spacer_ob;
	children[num_children++] = print_one_ob;
	children[num_children++] = print_ob;
	XtManageChildren(children, num_children);
	
	XtManageChild(file_mb);
	
	#undef MAX_CHILDREN
	
} /* end of construct_file_menu_button() */



/************************************************************************
 *
 *  select_cb -- 
 *
 ************************************************************************/

/*ARGSUSED*/
static void
select_cb(
	Widget			wid,		/* unused */
	XtPointer		client_data,
	XtPointer		call_data	/* unused */
)
{
	const Widget		shellw = (Widget)client_data;

	XtPopup(shellw, XtGrabNone);
	/* if already popped up than raise it to top */

	update_footers("", "");
} /* end of select_cb() */


/************************************************************************
 *
 *  any_cb -- 
 *
 ************************************************************************/

/*ARGSUSED*/
static void
any_cb(
	Widget			widget,		/* unused */
	XtPointer		client_data,	/* unused */
	XtPointer		call_data
)
{
	static char		path_buf[BIGGEST_PATH] = { '\0' };
	OlAnyCallbackStruct*	any_cd = (OlAnyCallbackStruct*)call_data;

	switch (any_cd->reason) {
	case OL_REASON_LIST_CHOICE: {
		OlFileChListChoiceCallbackStruct* cd =
			(OlFileChListChoiceCallbackStruct*)call_data;

		(void) sprintf(lc_messages, dgettext(MessagesDomain, 
			"FileChooser list choice callback (%s)"), 
			cd->chosen_item_node->name);
		update_footers(lc_messages, "");
		}
		break;
	case OL_REASON_OPEN_FOLDER: {
		OlFileChFolderCallbackStruct* cd =
			(OlFileChFolderCallbackStruct*)call_data;

		(void) sprintf(lc_messages, dgettext(MessagesDomain, 
			"FileChooser open folder callback (%s)"), 
			cd->request_folder);
		update_left_footer(lc_messages);
		}
		break;
	case OL_REASON_FOLDER_OPENED: {
		OlFileChGenericCallbackStruct* cd =
			(OlFileChGenericCallbackStruct*)call_data;

		(void) sprintf(lc_messages, dgettext(MessagesDomain, 
			"...folder opened callback"));
		update_right_footer(lc_messages);
		}
		break;
	case OL_REASON_INPUT_DOCUMENT: {
		OlFileChDocumentCallbackStruct* cd =
			(OlFileChDocumentCallbackStruct*)call_data;

		(void) strcat(strcat(strcpy(path_buf, 
			cd->request_document_folder), "/"), 
			cd->request_document);

		switch (cd->operation) {
		case OL_OPEN:
			XtVaSetValues(textedit, 
					XtNwrapMode,	OL_WRAP_OFF,
					XtNsourceType,	OL_DISK_SOURCE,
					XtNsource,	path_buf,
				NULL);

			(void) sprintf(lc_messages, dgettext(MessagesDomain, 
				"Opened document %s"), cd->request_document);
			update_left_footer(lc_messages);

			break;

		case OL_INCLUDE:
			insert_document((TextEditWidget)textedit, path_buf);
			break;
		}

		}
		break;
	case OL_REASON_OUTPUT_DOCUMENT: {
		OlFileChDocumentCallbackStruct*	cd =
			(OlFileChDocumentCallbackStruct*)call_data;

		(void) strcat(strcat(strcpy(path_buf, 
			cd->request_document_folder), "/"), 
			cd->request_document);

		save_document((TextEditWidget)textedit, path_buf);

		if (OL_SAVE == cd->operation) {
			XtVaSetValues(textedit, XtNuserData, path_buf, NULL);
			
			XtVaSetValues(saveas, 
					XtNlastDocumentName, basename(path_buf),
				NULL);
		}

		}
		break;
	case OL_REASON_CANCEL:
		(void) sprintf(lc_messages, dgettext(MessagesDomain, 
			"FileChooser cancel callback"), NULL);
		update_footers(lc_messages, "");
		break;
	case OL_REASON_VERIFY:
		(void) sprintf(lc_messages, dgettext(MessagesDomain, 
			"FileChooserShell verify callback"), NULL);
		update_right_footer(lc_messages);
		break;
	}
} /* end of any_cb() */


/************************************************************************
 *
 *  new_cb -- 
 *
 ************************************************************************/

/*ARGSUSED1*/
static void
new_cb(
	Widget			widget,
	XtPointer		client_data,	/* unused */
	XtPointer		call_data	/* unused */
)
{

	OlTextEditClearBuffer((TextEditWidget)textedit);
	XtVaSetValues(textedit, XtNuserData, NULL, NULL);

	update_footers("", "");
} /* end of new_cb() */


/************************************************************************
 *
 *  save_cb -- 
 *
 ************************************************************************/

/*ARGSUSED*/
static void
save_cb(
	Widget			wid,		/* unused */
	XtPointer		client_data,
	XtPointer		call_data	/* unused */
)
{
	const Widget		shellw = (Widget)client_data;
	String			path_name;

	XtVaGetValues(textedit, XtNuserData, &path_name, NULL);
	
	if (NULL == path_name || '\0' == *path_name) {
		XtPopup(shellw, XtGrabNone);
		update_footers("", "");
	} else
		save_document((TextEditWidget)textedit, path_name);
} /* end of save_cb() */


/************************************************************************
 *
 *  print_cb -- 
 *
 ************************************************************************/

/*ARGSUSED*/
static void
print_cb(
	Widget			wid,		/* unused */
	XtPointer		client_data,	/* unused */
	XtPointer		call_data	/* unused */
)
{

	(void) sprintf(lc_messages, dgettext(MessagesDomain, 
		"Sorry, printing is the application's responsibility "
		"(not OLIT's)."));
	update_footers(lc_messages, "");
} /* end of print_cb() */


/************************************************************************
 *
 *  save_document -- 
 *
 ************************************************************************/

static void
save_document(const TextEditWidget tew, const String path_name)
{
	OlStrRep	text_format;
	SaveResult	save_result;
	
	XtVaGetValues((Widget)tew, XtNtextFormat, &text_format, NULL);

	switch (text_format) {
	case OL_SB_STR_REP:
		save_result = SaveTextBuffer(OlTextEditTextBuffer(tew), 
			path_name);
		break;
	case OL_MB_STR_REP:
	/*FALLTHROUGH*/
	case OL_WC_STR_REP:
		save_result = OlSaveTextBuffer(OlTextEditOlTextBuffer(tew), 
			path_name);
		break;
	}

	if (SAVE_FAILURE == save_result) {
		(void) sprintf(lc_messages, dgettext(MessagesDomain, 
			"Save to document %s failed!"), path_name);
		update_footers(lc_messages, "");
	} else {
		(void) sprintf(lc_messages, dgettext(MessagesDomain,
			"Saved to document %s"), path_name);
		update_footers(lc_messages, "");
	}
} /* end of save_document() */


/************************************************************************
 *
 *  insert_document -- 
 *
 ************************************************************************/

static void
insert_document(const TextEditWidget tew, const String path_name)
{
	#define	BUFSIZE		(1024)
	#define WC_BUFSIZE	(256)

	OlStrRep	text_format;
	FILE*		fp;
	char		buf[BUFSIZE];
	wchar_t		wc_buf[WC_BUFSIZE];
	
	if (NULL == (fp = fopen(path_name, "r"))) {
		(void) sprintf(lc_messages, dgettext(MessagesDomain, 
			"Could not open document %s for insertion!"), path_name);
		update_footers(lc_messages, "");
		return;
	}

	XtVaGetValues((Widget)tew, XtNtextFormat, &text_format, NULL);

	(void) OlTextEditUpdate(tew, FALSE);
	
	while (NULL != fgets(buf, sizeof buf, fp)) {
		if (OL_WC_STR_REP == text_format)
			(void) mbstowcs(wc_buf, buf, WC_BUFSIZE);

		if (EDIT_SUCCESS != OlTextEditInsert(tew, 
				(OL_WC_STR_REP == text_format) ? 
					(char*)wc_buf : buf, 
				BUFSIZE)) {

			(void) sprintf(lc_messages, 
				dgettext(MessagesDomain, 
				"Insertion of document %s failed!"), 
				path_name);

			update_footers(lc_messages, "");
			(void) OlTextEditUpdate(tew, TRUE);
			return;
		}
	}
	
	(void) OlTextEditUpdate(tew, TRUE);

	(void) sprintf(lc_messages, dgettext(MessagesDomain,
		"Inserted document %s"), path_name);
	update_footers(lc_messages, "");

} /* end of insert_document() */


