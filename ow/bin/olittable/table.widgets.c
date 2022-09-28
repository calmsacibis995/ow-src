#pragma ident  "@(#)table.widgets.c	1.5 91/10/10 lib/Xol SMI"
/*
 *      Copyright (C) 1990, 1991 Sun Microsystems, Inc
 *                 All rights reserved.
 *       Notice of copyright on this source code 
 *       product does not indicate publication. 
 *
 * RESTRICTED RIGHTS LEGEND: Use, duplication, or disclosure by 
 * the U.S. Government is subject to restrictions as set forth 
 * in subparagraph (c)(1)(ii) of the Rights in Technical Data
 * and Computer Software Clause at DFARS 252.227-7013 (Oct. 1988) 
 * and FAR 52.227-19 (c) (June 1987).
 * 
 *   Sun Microsystems, Inc., 2550 Garcia Avenue,
 *   Mountain View, California 94043.
 */ 

/**********************************************************************
 *
 * Copyright (c) 1991 by Sun Microsystems, Inc.
 *
 * table.widgets.c:  module for creating example widgets within each
 *		     widget table entry.
 *
 **********************************************************************/

/* WARNING: this code is not yet documented....*/


#include "table.h"
#include "widgethelp.h"

#include "canvas.bitmap"
#include "flatmsg.bitmap"

extern OlFont font[NUMFONTS];
extern depth, color;

static int need_stub_gc = TRUE;


extern Pixmap pixmaps[4];/* array of pixmaps used for tiles when 
				run on non-colordisplays */
extern Pixel colors[NCOLORS];


extern TableEntry p_table[NUMENTRIES];

extern int entry_width, entry_height, button_height;

static unsigned char t_string[] = "Welcome to the OpenLook\nIntrinsics Toolkit.\nFrom here you can view\n all the widgets.";
static unsigned char st_string[] = "I cannot be edited.\nBut Go ahead and\ntry...Make my day!";

static char	lc_messages[1024];
char *lc_help, *lc_label;

/************************************************
 * Datastructures for Flat widget implementation
 ************************************************/

typedef struct {
	XtArgVal label;
	XtArgVal background;
} FlatNonExclusives;

String
nonexc_fields[] = { XtNlabel, XtNbackground };

typedef struct {
	XtArgVal label;
	XtArgVal background;
} FlatExclusives;

String
exc_fields[] = { XtNlabel, XtNbackground };


typedef struct {
	XtArgVal label;
} FlatCheckBox;


String
chk_fields[] = { XtNlabel };

static FlatExclusives exc_items[4];
static FlatNonExclusives nonexc_items[3];
static FlatCheckBox chk_items[2];

/**********************************************
 *	Private procedures
**********************************************/

static void 	popupCB1(Widget w,XtPointer clientData,XtPointer callData);
static void	abbrevCB(Widget w, XtPointer clientData, XtPointer callData);
static void	leMenuCB(Widget w, XtPointer clientData, XtPointer callData);
static void	daResizeCB(Widget widget, caddr_t closure, OlDrawAreaCallbackStruct *calldata);
static void	daExposeCB(Widget widget, caddr_t closure, OlDrawAreaCallbackStruct *calldata);
void		DrawOnStub(Widget widget, XEvent *xevent, Region region);

/******************************************
 * popupCB1: pops up popupWindow
 ******************************************/
static void popupCB1(Widget w,XtPointer clientData,XtPointer callData)
{
	Widget shell = (Widget)clientData;
	XtPopup(shell,XtGrabNone);
}

/*********************************************
 * abbrevCB: Changes "current" widget to
 * display value selected off abbreviated
 * menu button's menu.
 ********************************************/
static void abbrevCB(Widget w, XtPointer clientData, XtPointer callData)
{
	Arg arg0;
	Widget currentselection = (Widget) clientData;
	char *current;

	XtSetArg(arg0, XtNlabel, &current);
	XtGetValues(w, &arg0, 1);

	XtSetArg(arg0, XtNstring, current);
	XtSetValues(currentselection, &arg0, 1);

}

/* For MenuShell */

char * comment[] = {"with Meatballs?","Good choice.","White or Red Sauce?",
			"Don't like Italian?"};

/*********************************************
 * leMenuCB: 
 ********************************************/
static void leMenuCB(Widget w, XtPointer clientData, XtPointer callData)
{
	int index = (int)clientData;
	Widget msg;

	XtVaGetValues(w,
			XtNuserData,	&msg,
			NULL);

	(void)sprintf((char *)lc_messages,
			dgettext(OlittableDomain,"with Meatballs?"));
	strcpy(comment[0],(char *)lc_messages);
	(void)sprintf((char *)lc_messages,
			dgettext(OlittableDomain,"Good choice."));
	strcpy(comment[1],(char *)lc_messages);
	(void)sprintf((char *)lc_messages,
			dgettext(OlittableDomain,"White or Red Sauce?"));
	strcpy(comment[2],(char *)lc_messages);
	(void)sprintf((char *)lc_messages,
			dgettext(OlittableDomain,"Don't like Italian?"));
	strcpy(comment[3],(char *)lc_messages);

	XtVaSetValues(msg,
			XtNstring,	comment[index],
			NULL);
}


/*
 * Define some Data for Gauge widget implementation
 */

/* Gauge Widget's State: */
#define OFF	0	
#define ON	1


int current_value = 0;  /* Keep track of Gauge's Current sliderValue */
int state = ON;		/* Keep track of Gauge's state */
XtIntervalId timerID;	/* Used to ADd/Remove Gauge's timer proc     */

#define	INTERVAL	500 /* Interval used to call Gauge timer routine */
#define SPACE_OFFSET	(10)	/* pixels */ 


/*************************************************************
 * moveGauge:  This is a timer proc which increases the value
 * of the Gauge widget (so that it appears to move constantly.
 *************************************************************/
void
moveGauge(Widget w,XtIntervalId id)
{
	if (current_value < 100)
		current_value = current_value + 2;
	else
		current_value = 0;

	XtVaSetValues(w,
			XtNsliderValue,(XtArgVal)current_value,
			NULL);

	timerID = XtAppAddTimeOut(XtWidgetToApplicationContext(w),
            INTERVAL, (XtTimerCallbackProc)moveGauge, w);

} 

/*************************************************************
 * toggleMode: Callback called when popupmenu item is
 *		selected to toggle the mode of the Gauge...
 *************************************************************/
void
toggleMode(Widget w, XtPointer clientData,XtPointer callData)
{
	if (state == ON) {
		XtRemoveTimeOut(timerID);
		state = OFF;
		XtVaSetValues(w,
				XtNlabel,	(XtArgVal)"Start",
				NULL);
	}
	else {
		state = ON;
		timerID = XtAppAddTimeOut(XtWidgetToApplicationContext(w),
                    INTERVAL, (XtTimerCallbackProc)moveGauge, clientData);
		XtVaSetValues(w,
				XtNlabel,	(XtArgVal)"Stop",
				NULL);
	}

}	
/*******************************************************************/

/* For ScrollingList */
static OlListToken	(*ListAddItem)();
static void 		(*ListTouchItem)();
static void		(*ListViewItem)();

/********************************************************************
 * makeCurrentCB:  Callback called when the user selects one of the
 *		   items in the ScrollingList.  Note it is UP TO THE
 *		   application to make the item appear "current"
 *		   (indented) by setting the attr field in the 
 *		   OlListItem structure to be OL_LIST_ATTR_CURRENT.
 *		   as well as making the previously selected item
 *		   no longer appear selected.
 *******************************************************************/
void
makeCurrentCB(Widget w, XtPointer clientData, XtPointer callData)
{
	OlListItem* new_item = OlListItemPointer((OlListToken)callData);
	OlListItem* prev_item;
	OlListToken token = (OlListToken) callData;
	static OlListToken selectedtoken;

	if (selectedtoken != token)
		{	
		/*
		* Mark the item as selected or current by setting a bit
		* in its attribute field.  Notify the widget that we have
		* touched an item.
		*/
		new_item->attr |= OL_LIST_ATTR_CURRENT;
		(*ListTouchItem)(w, token);
	
		(*ListViewItem)(w, token);

		/*
		 * If there was a previously selected item, unselect by
		 * clearing the CURRENT attribute bit.   Again, since we have
		 * modified an item, we must notify the widget.
		 */		
		if (selectedtoken != 0)
			{
			prev_item = OlListItemPointer(selectedtoken);
			prev_item->attr &= ~OL_LIST_ATTR_CURRENT;
			(*ListTouchItem)(w, selectedtoken);
			}
		/*
		 * Keep track of currently selected item in list
		 */
		selectedtoken = token;
		}
}
/***************************************************************************/	


		
/* For Menubutton */
static char *mb_choicestring[] = {" 8","10","12","14","18","24" };

#define FIND_MENUBUTTON_PART(w) &(((MenuButtonWidget)(w))->menubutton)


/* For ControlArea */
static char *ca_caption_label[] = {"Product:","UI-Type:"};
static char *ca_textfield_string[] = {"$$Maker", "OL"};

/* For ScrollingList */
static 	char *sl_liststring[] = { "Easter", "July4th", "LaborDay", "Halloween",
	"T-giving", "Hanakuh","Christmas","NewYears" };
static char sl_mnemonic[] = {'E','J','L','H','T','a','C','N'};


int ReadyToDraw;

static void
DropTargetCB(Widget w, XtPointer clientData, OlDropTargetCallbackStruct *dnd)
{

    switch (dnd->reason) { 
	case OL_REASON_DND_OWNSELECTION: {
	    printf("DropTarget: acting as SOURCE of D&D operation...\n");
            }
            break;

        case OL_REASON_DND_TRIGGER: { /* case when we detect a drop */
	    printf("DropTarget: Received DROP!\n");
            }
            break;

        default:
            break;
 
    } /* switch */
}


/****************************************************************************
 * CreateExampleWidgets: routine which sequentially creates the
 * example widget implementations shown within each table entry.
 * INdividual Help text is registered with each widget.
 ****************************************************************************/	
void
CreateExampleWidgets(void)
{
	Arg arg[24];
	Widget 	button, shell, parent, menupane,
		onlychild, childs[4], grandchilds[4], greatgrandchilds[3];
	OlListItem *sl_listitem;
	int n, reset, i, j[4];
	int xoffset, yoffset;
	int max_hgt, widget_hgt, widget_wth;

	/* 
	 * Since the info button takes up height in the entry, calculate the
	 * height available to hold the widget example.
	 */
	max_hgt = entry_height - button_height;


	/************************************************************
	 * Create BulletinBoard Example.
	 ************************************************************/

	p_table[C_BULLETINBOARD].widget = XtVaCreateManagedWidget(
				"constraintexample",
				bulletinBoardWidgetClass,
				p_table[C_BULLETINBOARD].entry,
				XtNwidth,	(XtArgVal)90,
				XtNheight,	(XtArgVal)54,
				XtNborderWidth,	(XtArgVal) 1,
				XtNlayout, 	(XtArgVal) OL_IGNORE,
				NULL);

	p_table[C_BULLETINBOARD].entry_widget = p_table[C_BULLETINBOARD].widget;

	if (color)
		XtVaSetValues(p_table[C_BULLETINBOARD].widget,
				XtNbackground, (XtArgVal)colors[BACKGROUND],
				NULL);

	(void)sprintf((char *)lc_messages,dgettext(OlittableDomain," Pin Me"));
	lc_help = (char *)XtMalloc(strlen(lc_messages)+1);
	strcpy(lc_help,lc_messages);
	onlychild = XtVaCreateManagedWidget("constraintchild",
				textFieldWidgetClass,
				p_table[C_BULLETINBOARD].widget,
				XtNimPreeditStyle,	OL_ON_THE_SPOT,
				XtNborderWidth, 	(XtArgVal) 1,
				XtNmaximumSize, 	(XtArgVal)8,
				XtNstring, 		(XtArgVal)lc_help,
				XtNfont, 		(XtArgVal)font[SIZE10],
				NULL);

	if (color)
		XtVaSetValues(onlychild,
				XtNbackground, (XtArgVal)colors[CONSTCHILD],
				NULL); 


	(void)sprintf((char *)lc_messages,"%s",
				dgettext(OlittableDomain,bulletinboardhelp));
	lc_help = (char *)XtMalloc(strlen(lc_messages)+1);
	strcpy(lc_help,lc_messages);
	OlRegisterHelp(OL_WIDGET_HELP, 
				(XtPointer)(p_table[C_BULLETINBOARD].widget), 
				"Example BulletinBoard", 
				OL_STRING_SOURCE, 
				lc_help);


	/************************************************************
	 * Create ControlArea Widget Example
	 ***********************************************************/

	p_table[C_CONTROLAREA].widget = XtVaCreateManagedWidget(
					"constraintexample", 
					controlAreaWidgetClass,
					p_table[C_CONTROLAREA].entry,
					XtNborderWidth,	(XtArgVal) 1,
					XtNlayoutType, 	(XtArgVal)OL_FIXEDCOLS,
					XtNalignCaptions,(XtArgVal) True,
					XtNvSpace, 	(XtArgVal)0,
					NULL);

        p_table[C_CONTROLAREA].entry_widget = p_table[C_CONTROLAREA].widget;

	if (color)
		XtVaSetValues(p_table[C_CONTROLAREA].widget,
				XtNbackground, (XtArgVal)colors[BACKGROUND],
				NULL);


	for (i = 0; i < XtNumber(ca_caption_label); i++){

		(void)sprintf((char *)lc_messages,
					dgettext(OlittableDomain,"Product:"));
		strcpy(ca_caption_label[0],(char *)lc_messages);
		(void)sprintf((char *)lc_messages,
					dgettext(OlittableDomain,"UI-Type:"));
		strcpy(ca_caption_label[1],(char *)lc_messages);
		childs[i] = XtVaCreateManagedWidget("constraintchild",
					captionWidgetClass,
					p_table[C_CONTROLAREA].widget, 
					XtNborderWidth,	(XtArgVal)0,
					XtNfont,	(XtArgVal)font[SIZE10],
					XtNlabel,(XtArgVal)ca_caption_label[i],
					NULL);

		grandchilds[i] = XtVaCreateManagedWidget("constraintchild",
				textFieldWidgetClass,
				childs[i], 
				XtNborderWidth,	(XtArgVal)0,
				XtNfont, 	(XtArgVal)font[SIZE10],
				XtNstring,(XtArgVal)ca_textfield_string[i],
				XtNmaximumSize,	(XtArgVal)10,
				XtNcharsVisible, (XtArgVal)10,
				XtNimPreeditStyle,	OL_ON_THE_SPOT,
				NULL);

		if (color){
			XtVaSetValues(childs[i],
				XtNbackground, (XtArgVal)colors[BACKGROUND],
				NULL);
			XtVaSetValues(grandchilds[i],
				XtNbackground, (XtArgVal)colors[BACKGROUND],
				NULL);
		}
	}


	(void)sprintf((char *)lc_messages,
				dgettext(OlittableDomain,controlareahelp));
	lc_help = (char *)XtMalloc(strlen(lc_messages)+1);
	strcpy(lc_help,lc_messages);
	OlRegisterHelp(OL_WIDGET_HELP,
				(XtPointer)(p_table[C_CONTROLAREA].widget), 
				"Example ControlArea", 
				OL_STRING_SOURCE, 
				lc_help);

	/*************************************************************
 	 * Create RubberTile Widget Example 
	 *************************************************************/

	(void)sprintf((char *)lc_messages,
				dgettext(OlittableDomain,"PopUp RubberTile.."));
	lc_help = (char *)XtMalloc(strlen(lc_messages)+1);
	strcpy(lc_help,lc_messages);
	button = XtVaCreateManagedWidget("shellbutton", 
				oblongButtonWidgetClass,
				p_table[C_RUBBERTILE].entry,
				XtNlabel, 	(XtArgVal)lc_help,
				NULL);

	p_table[C_RUBBERTILE].entry_widget = button;

	(void)sprintf((char *)lc_messages,
				dgettext(OlittableDomain,"Stretch Me...."));
	lc_help = (char *)XtMalloc(strlen(lc_messages)+1);
	strcpy(lc_help,lc_messages);
	shell = XtVaCreatePopupShell((char *)lc_help,
				transientShellWidgetClass,
				button,
				XtNwinType,	(XtArgVal)OL_WT_CMD,
				XtNresizeCorners, (XtArgVal)True,
				XtNpushpin, 	(XtArgVal)OL_OUT,
				XtNx, 		(XtArgVal)(2 * entry_width) + 3,
				XtNy, 		(XtArgVal)entry_height + 3,
				NULL);

	p_table[C_RUBBERTILE].widget = XtVaCreateManagedWidget(
				"constraintexample",
				rubberTileWidgetClass,
				shell,
				XtNorientation,	(XtArgVal)OL_VERTICAL,
				NULL);

	(void)sprintf((char *)lc_messages,
		dgettext(OlittableDomain,"I Absorb 1/6 height changes"));
	lc_help = (char *)XtMalloc(strlen(lc_messages)+1);
	strcpy(lc_help,lc_messages);
	childs[0] = XtVaCreateManagedWidget("constchild",
			staticTextWidgetClass,
			p_table[C_RUBBERTILE].widget,
			XtNweight,	(XtArgVal)1,
			XtNstring,	(XtArgVal)lc_help,
			XtNgravity,	(XtArgVal)CenterGravity,
			XtNborderWidth,	(XtArgVal)1,
			NULL);

	(void)sprintf((char *)lc_messages,
		dgettext(OlittableDomain,"I get 1/3 height changes"));
	lc_help = (char *)XtMalloc(strlen(lc_messages)+1);
	strcpy(lc_help,lc_messages);
	childs[1] = XtVaCreateManagedWidget("constchild",
			staticTextWidgetClass,
			p_table[C_RUBBERTILE].widget,
			XtNweight,	(XtArgVal)2,
			XtNstring,	(XtArgVal)lc_help,
			XtNgravity,	(XtArgVal)CenterGravity,
			XtNborderWidth,	(XtArgVal)1,
			NULL);

	(void)sprintf((char *)lc_messages,
		dgettext(OlittableDomain,"I get 1/2 height changes"));
	lc_help = (char *)XtMalloc(strlen(lc_messages)+1);
	strcpy(lc_help,lc_messages);
	childs[2] = XtVaCreateManagedWidget("constchild",
			staticTextWidgetClass,
			p_table[C_RUBBERTILE].widget,
			XtNweight,	(XtArgVal)3,
			XtNstring,	(XtArgVal)lc_help,
			XtNgravity,	(XtArgVal)CenterGravity,
			XtNborderWidth,	(XtArgVal)1,
			NULL);

	if (color){
		XtVaSetValues(button,
				XtNbackground, (XtArgVal)colors[CONSTRAINT],
				NULL);
		XtVaSetValues(childs[0],
				XtNbackground, (XtArgVal)colors[CONSTCHILD],
				NULL);
		XtVaSetValues(childs[1],
				XtNbackground, (XtArgVal)colors[SHELLBUTTON],
				NULL);
		XtVaSetValues(childs[2],
				XtNbackground, (XtArgVal)colors[SHELL],
				NULL);
	}

	/* 
   	 * Add callback to popup button now that we have popupshell widget ID. 
	*/
	XtAddCallback(button, XtNselect, popupCB1, shell);

(void)sprintf((char *)lc_messages,dgettext(OlittableDomain,rubbertilehelp));
        lc_help = (char *)XtMalloc(strlen(lc_messages)+1);
        strcpy(lc_help,lc_messages);
        OlRegisterHelp(OL_WIDGET_HELP,
                                (XtPointer)button,
                                "Example RubberTile",
                                OL_STRING_SOURCE,
                                lc_help);




	/************************************************************
	 * Create Form Widget Example
	 ***********************************************************/

	(void)sprintf((char *)lc_messages,
			dgettext(OlittableDomain,"PopUp Form.."));
	lc_help = (char *)XtMalloc(strlen(lc_messages)+1);
	strcpy(lc_help,lc_messages);
	button = XtVaCreateManagedWidget("shellbutton", 
				oblongButtonWidgetClass,
				p_table[C_FORM].entry,
				XtNlabel, 	(XtArgVal)lc_help,
				NULL);
	p_table[C_FORM].entry_widget = button;


	(void)sprintf((char *)lc_messages,
			dgettext(OlittableDomain,"Stretch Me...."));
	lc_help = (char *)XtMalloc(strlen(lc_messages)+1);
	strcpy(lc_help,lc_messages);
	shell = XtVaCreatePopupShell((char *)lc_help,
				transientShellWidgetClass,
				button,
				XtNwinType,	(XtArgVal)OL_WT_CMD,
				XtNresizeCorners, (XtArgVal)True,
				XtNpushpin, 	(XtArgVal)OL_OUT,
				XtNx, 		(XtArgVal)(2 * entry_width) + 3,
				XtNy, 		(XtArgVal)entry_height + 3,
				NULL);

	p_table[C_FORM].widget = XtVaCreateManagedWidget("constraintexample",
					formWidgetClass,
					shell,
					NULL);

	(void)sprintf((char *)lc_messages,
			dgettext(OlittableDomain,"I Span The Top"));
	lc_help = (char *)XtMalloc(strlen(lc_messages)+1);
	strcpy(lc_help,lc_messages);
	childs[0] = XtVaCreateManagedWidget("constchild",
				staticTextWidgetClass,
				p_table[C_FORM].widget,
				XtNstring,	(XtArgVal)lc_help,
				XtNgravity,	(XtArgVal)CenterGravity,
				XtNborderWidth,	(XtArgVal)1,
				XtNxAttachRight,(XtArgVal)True,
				XtNxResizable,	(XtArgVal)True,
				XtNyResizable,	(XtArgVal)True,
				XtNrecomputeSize,(XtArgVal)True,
				XtNxVaryOffset,	(XtArgVal)False,
				XtNyVaryOffset,	(XtArgVal)False,
				NULL);

	(void)sprintf((char *)lc_messages,
			dgettext(OlittableDomain,"I sit on LowerLeft"));
	lc_help = (char *)XtMalloc(strlen(lc_messages)+1);
	strcpy(lc_help,lc_messages);
	childs[1] = XtVaCreateManagedWidget("constchild",
				staticTextWidgetClass,
				p_table[C_FORM].widget,
				XtNstring,	(XtArgVal)lc_help,
				XtNgravity,	(XtArgVal)CenterGravity,
				XtNborderWidth,	(XtArgVal)1,
				XtNyRefWidget,	(XtArgVal)childs[0],
				XtNyAddHeight,	(XtArgVal)True,
				XtNyAttachBottom,(XtArgVal)True,
				XtNyResizable,	(XtArgVal)True,
				XtNyVaryOffset,	(XtArgVal)False,
				XtNxVaryOffset,	(XtArgVal)False,
				XtNxResizable,	(XtArgVal)True,
				XtNyResizable,	(XtArgVal)True,
				XtNrecomputeSize,(XtArgVal)True,
				NULL);

	(void)sprintf((char *)lc_messages,
			dgettext(OlittableDomain,"I span LowerRight"));
	lc_help = (char *)XtMalloc(strlen(lc_messages)+1);
	strcpy(lc_help,lc_messages);
	childs[2] = XtVaCreateManagedWidget("constchild",
				staticTextWidgetClass,
				p_table[C_FORM].widget,
				XtNstring,	(XtArgVal)lc_help,
				XtNgravity,	(XtArgVal)CenterGravity,
				XtNborderWidth,	(XtArgVal)1,
				XtNyRefWidget,	(XtArgVal)childs[0],
				XtNyAddHeight,	(XtArgVal)True,
				XtNyAttachBottom,(XtArgVal)True,
				XtNyResizable,	(XtArgVal)True,
				XtNyVaryOffset,	(XtArgVal)False,
				XtNxRefWidget,	(XtArgVal)childs[1],
				XtNxAddWidth,	(XtArgVal)True,
				XtNxAttachRight,(XtArgVal)True,
				XtNxResizable,	(XtArgVal)True,
				XtNxVaryOffset,	(XtArgVal)False,
				XtNrecomputeSize,(XtArgVal)True,
				NULL);

	if (color){
		XtVaSetValues(button,
				XtNbackground, (XtArgVal)colors[CONSTRAINT],
				NULL);
		XtVaSetValues(childs[0],
				XtNbackground, (XtArgVal)colors[CONSTCHILD],
				NULL);
		XtVaSetValues(childs[1],
				XtNbackground, (XtArgVal)colors[SHELLBUTTON],
				NULL);
		XtVaSetValues(childs[2],
				XtNbackground, (XtArgVal)colors[SHELL],
				NULL);
	}

	/* 
   	 * Add callback to popup button now that we have popupshell widget ID. 
	*/
	XtAddCallback(button, XtNselect, popupCB1, shell);



	(void)sprintf((char *)lc_messages,dgettext(OlittableDomain,formhelp));
	lc_help = (char *)XtMalloc(strlen(lc_messages)+1);
	strcpy(lc_help,lc_messages);
        OlRegisterHelp(OL_WIDGET_HELP,
                                (XtPointer)button,
                                "Example Form",
                                OL_STRING_SOURCE,
                                lc_help);



	/************************************************************
	 * Create ScrolledWindow Example
	 ***********************************************************/

	widget_wth = 100;
	widget_hgt = 82;

	n = 0;
	XtSetArg(arg[n], XtNwidth, 	(XtArgVal) widget_wth);	n++;
	XtSetArg(arg[n], XtNheight, 	(XtArgVal) widget_hgt);	n++;
	XtSetArg(arg[n], XtNimPreeditStyle,(XtArgVal) OL_ON_THE_SPOT);	n++;

	p_table[C_SCROLLEDWINDOW].widget = XtCreateManagedWidget("scrollwindow",
					scrolledWindowWidgetClass,
					p_table[C_SCROLLEDWINDOW].entry, 
					arg, n);

	p_table[C_SCROLLEDWINDOW].entry_widget = p_table[C_SCROLLEDWINDOW].widget;

	/*
	 * We want the scrollbars to also have color, we must explicitly set it
	 */
	if (color){
		n = 0;
		XtSetArg(arg[n], XtNvScrollbar,	&childs[0]);	n++;
		XtSetArg(arg[n], XtNhScrollbar, &childs[1]);	n++;
		XtGetValues(p_table[C_SCROLLEDWINDOW].widget, arg, n);

		XtSetArg(arg[0], XtNbackground, colors[BACKGROUND]);
		XtSetValues(childs[0], arg, 1);
		XtSetValues(childs[1], arg, 1);
		}

	n = 0;
	if (color) 
		XtSetArg(arg[n], XtNbackground, (Pixel)colors[BACKGROUND]);n++;
	XtSetArg(arg[n], XtNwidth, 		(XtArgVal) 120);	n++;
	XtSetArg(arg[n], XtNheight,		(XtArgVal) 100);	n++;
	XtSetArg(arg[n], XtNborderWidth, 	(XtArgVal)0);		n++;
	onlychild = XtCreateManagedWidget("constraintexample", 
					stubWidgetClass, 
					p_table[C_SCROLLEDWINDOW].widget,
					 arg, n);

	XtAddEventHandler(onlychild, ExposureMask, FALSE,
					 (XtEventHandler)DrawOnStub, NULL);

	(void)sprintf((char *)lc_messages,
				dgettext(OlittableDomain,scrolledwinhelp));
	lc_help = (char *)XtMalloc(strlen(lc_messages)+1);
	strcpy(lc_help,lc_messages);
	OlRegisterHelp(OL_WIDGET_HELP, 
				(XtPointer)(p_table[C_SCROLLEDWINDOW].widget),
				"Example ScrolledWindow", 
				OL_STRING_SOURCE, 
				lc_help);




	/************************************************************
	 * Create TextField Example
	 ***********************************************************/

	widget_wth = 106;
	widget_hgt = 16;

	(void)sprintf((char *)lc_messages,
			dgettext(OlittableDomain,"  any text..."));
	lc_help = (char *)XtMalloc(strlen(lc_messages)+1);
	strcpy(lc_help,lc_messages);
	n = 0;
	if (color) 
		XtSetArg(arg[n], XtNbackground, (Pixel)colors[BACKGROUND]);n++;
	XtSetArg(arg[n], XtNmaximumSize, 	(XtArgVal)16);		n++;
	XtSetArg(arg[n], XtNstring, 		(String)lc_help);n++;
	XtSetArg(arg[n], XtNborderWidth, 	(XtArgVal)1);		n++;
	XtSetArg(arg[n], XtNfont, 		(XtArgVal)font[SIZE10]);n++;
	XtSetArg(arg[n], XtNimPreeditStyle,	(XtArgVal)OL_ON_THE_SPOT);n++;
	p_table[C_TEXTFIELD].widget = XtCreateManagedWidget("constraintexample",
						textFieldWidgetClass, 
						p_table[C_TEXTFIELD].entry, 
						arg, n);

	p_table[C_TEXTFIELD].entry_widget = p_table[C_TEXTFIELD].widget;

	(void)sprintf((char *)lc_messages,
				dgettext(OlittableDomain,textfieldhelp));
	lc_help = (char *)XtMalloc(strlen(lc_messages)+1);
	strcpy(lc_help,lc_messages);
	OlRegisterHelp(OL_WIDGET_HELP, 
				(XtPointer)(p_table[C_TEXTFIELD].widget), 
				"Example TextField", 
				OL_STRING_SOURCE, 
				lc_help);



	/***********************************************************
	 * Create TextEdit Example
	 ***********************************************************/

	widget_wth = 120;
	widget_hgt = 40;

	(void)sprintf((char *)lc_messages,
		dgettext(OlittableDomain,"I'm a Multi-Line Text Editor..."));
	lc_help = (char *)XtMalloc(strlen(lc_messages)+1);
	strcpy(lc_help,lc_messages);
	n = 0;
	if (color){
		XtSetArg(arg[n], XtNbackground,      (Pixel)colors[BACKGROUND]);	  	n++;
	}
	XtSetArg(arg[n], XtNheight, (XtArgVal) 40);			n++;
	XtSetArg(arg[n], XtNwidth, (XtArgVal)108);			n++;
	XtSetArg(arg[n], XtNborderWidth, (XtArgVal)1);	n++;
	XtSetArg(arg[n], XtNimPreeditStyle, OL_ON_THE_SPOT);	n++;
	XtSetArg(arg[n], XtNwrapMode, OL_WRAP_WHITE_SPACE);	n++;
	XtSetArg(arg[n], XtNsource, (String)lc_help);	n++;
	XtSetArg(arg[n], XtNfont,(OlFont)font[SIZE10]);		n++;
	p_table[C_TEXTEDIT].widget = XtCreateManagedWidget("primitiveexample", 
						textEditWidgetClass,
						p_table[C_TEXTEDIT].entry, 
						arg, n);

	p_table[C_TEXTEDIT].entry_widget = p_table[C_TEXTEDIT].widget;

	(void)sprintf((char *)lc_messages,
				dgettext(OlittableDomain,textedithelp));
	lc_help = (char *)XtMalloc(strlen(lc_messages)+1);
	strcpy(lc_help,lc_messages);
	OlRegisterHelp(OL_WIDGET_HELP, 
				(XtPointer)(p_table[C_TEXTEDIT].widget),
				"Example TextEdit ", 
				OL_STRING_SOURCE, 
				lc_help);




	/************************************************************
	 * Create ScrollingList Example
	 ***********************************************************/

	p_table[C_SCROLLINGLIST].widget = XtVaCreateManagedWidget(
				"constraintexample", 
				scrollingListWidgetClass, 
				p_table[C_SCROLLINGLIST].entry, 
				XtNviewHeight, 	(XtArgVal) 3,
				XtNimPreeditStyle,	OL_ON_THE_SPOT,
				XtNfont, 	(XtArgVal)font[SIZE10],
				XtNselectable, 	(XtArgVal)FALSE,
				NULL);

	p_table[C_SCROLLINGLIST].entry_widget = p_table[C_SCROLLINGLIST].widget;

	if (color)
		XtVaSetValues(p_table[C_SCROLLINGLIST].widget,
				XtNbackground,	(XtArgVal)colors[BACKGROUND],
				NULL);



	/*
	 * Get handle to routines
	 */
	n = 0;
	XtSetArg(arg[n], XtNapplAddItem, (XtArgVal) &ListAddItem); 	n++;
	XtVaGetValues(p_table[C_SCROLLINGLIST].widget, 
				XtNapplAddItem,    (XtArgVal)&ListAddItem,
				XtNapplTouchItem,  (XtArgVal)&ListTouchItem,
				XtNapplViewItem,   (XtArgVal)&ListViewItem,
				NULL);

	/*
	 * Malloc up space for listitem data.
	 */
	sl_listitem = 
	    (OlListItem *)XtMalloc(XtNumber(sl_liststring)*sizeof(OlListItem));

	/*
	 * Add items to List
	 */

	for(i=0;i < XtNumber(sl_liststring);i++) {
		sl_listitem[i].label_type = (XtArgVal) OL_STRING;
		sl_listitem[i].label = XtNewString(sl_liststring[i]);
		sl_listitem[i].attr = 0;
		sl_listitem[i].mnemonic = sl_mnemonic[i];
		(*ListAddItem) (p_table[C_SCROLLINGLIST].widget,NULL,NULL,sl_listitem[i]); 
	}

	XtAddCallback(p_table[C_SCROLLINGLIST].widget, XtNuserMakeCurrent, 
			makeCurrentCB, NULL);


	(void)sprintf((char *)lc_messages,
				dgettext(OlittableDomain,scrollinglisthelp));
	lc_help = (char *)XtMalloc(strlen(lc_messages)+1);
	strcpy(lc_help,lc_messages);
	OlRegisterHelp(OL_WIDGET_HELP, 
				(XtPointer)(p_table[C_SCROLLINGLIST].widget),
				"Example ScrollingList", 
				OL_STRING_SOURCE, 
				lc_help);



	/************************************************************
	 * Create PopupWindowShell Example
	 ***********************************************************/

	(void)sprintf((char *)lc_messages,
		dgettext(OlittableDomain,"PopUp Window.."));
	lc_help = (char *)XtMalloc(strlen(lc_messages)+1);
	strcpy(lc_help,lc_messages);
	button = XtVaCreateManagedWidget("shellbutton", 
				oblongButtonWidgetClass,
				p_table[C_POPUPWINDOWSHELL].entry,
				XtNlabel, 	(XtArgVal)lc_help,
				NULL);

	p_table[C_POPUPWINDOWSHELL].entry_widget = button;

	if (color)
		XtVaSetValues(button,
				XtNbackground, (XtArgVal)colors[SHELLBUTTON],
				NULL);

	p_table[C_POPUPWINDOWSHELL].widget = XtVaCreatePopupShell(
			"File Selection Window",
			popupWindowShellWidgetClass,
			button,
			XtNpushpin, 	(XtArgVal)OL_OUT,
			XtNx, 		(XtArgVal)(2 * entry_width) + 3,
			XtNy, 		(XtArgVal)entry_height + 3,
			NULL);

	/* 
   	 * Add callback to popup button now that we have popupshell widget ID. 
	*/
	XtAddCallback(button, XtNselect, popupCB1, 
				p_table[C_POPUPWINDOWSHELL].widget);

	/*
	 The popup window automatically makes three children: upper and
	 lower control areas and footer: get widget IDs to populate them.
	*/
	XtVaGetValues (p_table[C_POPUPWINDOWSHELL].widget,
					XtNupperControlArea, 	&childs[0],
					XtNlowerControlArea, 	&childs[1],
					XtNfooterPanel, 	&childs[2],
					NULL);

	XtVaSetValues(childs[0],
				XtNlayoutType, 	(XtArgVal)OL_FIXEDHEIGHT,
				XtNmeasure,	(XtArgVal)1,
				XtNwidth, 	(XtArgVal) entry_width,
				NULL);

	XtVaSetValues(childs[1],
				XtNlayoutType, 	(XtArgVal)OL_FIXEDHEIGHT,
				XtNmeasure,	(XtArgVal)1,
				XtNwidth, 	(XtArgVal) entry_width,
				XtNhSpace,	(XtArgVal) 56,
				XtNhPad,  	(XtArgVal) 48,
				XtNvPad,  	(XtArgVal) 16,
				NULL);
	
	/* 
	  Populate popup upper, lower control areas, and footer.
	*/
	(void)sprintf((char *)lc_messages,
			dgettext(OlittableDomain,"Filename:"));
	lc_help = (char *)XtMalloc(strlen(lc_messages)+1);
	strcpy(lc_help,lc_messages);
	grandchilds[0] =XtVaCreateManagedWidget("caption", 
				captionWidgetClass, 
				childs[0],
				XtNlabel, 	(XtArgVal)lc_help,
				NULL);


	greatgrandchilds[0]= XtVaCreateManagedWidget("shellchild", 
				textFieldWidgetClass, 
				grandchilds[0],
				XtNmaximumSize, (XtArgVal)24,
				XtNimPreeditStyle, OL_ON_THE_SPOT,
				NULL); 


	grandchilds[2] = XtVaCreateManagedWidget("Load...",
				oblongButtonWidgetClass,
				childs[1],
				XtNdefault, 	(XtArgVal)TRUE,
				NULL);


	grandchilds[3] = XtVaCreateManagedWidget("Cancel",
				oblongButtonWidgetClass,
				childs[1],
				NULL);

	(void)sprintf((char *)lc_messages, dgettext(OlittableDomain,"errors:"));
	lc_help = (char *)XtMalloc(strlen(lc_messages)+1);
	strcpy(lc_help,lc_messages);
	grandchilds[1] = XtVaCreateManagedWidget("shellchild",
				staticTextWidgetClass, 
				childs[2],
				XtNstring,	(XtArgVal)lc_help,
				XtNgravity,	(XtArgVal)WestGravity,
				NULL);

	if (color){
		XtVaSetValues(childs[0],
				XtNbackground,(XtArgVal)colors[BACKGROUND],
				NULL);
		XtVaSetValues(childs[1],
				XtNbackground,(XtArgVal)colors[BACKGROUND],
				NULL);
		XtVaSetValues(childs[2],
				XtNbackground,(XtArgVal)colors[BACKGROUND],
				NULL);
		XtVaSetValues(grandchilds[0],
				XtNbackground,(XtArgVal)colors[BACKGROUND],
				NULL);
		XtVaSetValues(grandchilds[1],
				XtNbackground,(XtArgVal)colors[BACKGROUND],
				NULL);
		XtVaSetValues(greatgrandchilds[0],
				XtNbackground,(XtArgVal)colors[BACKGROUND],
				NULL);
		XtVaSetValues(grandchilds[2],
				XtNbackground,(XtArgVal)colors[BACKGROUND],
				NULL);
		XtVaSetValues(grandchilds[3],
				XtNbackground,(XtArgVal)colors[BACKGROUND],
				NULL);


	}


	(void)sprintf((char *)lc_messages,
				dgettext(OlittableDomain,popupwindowhelp));
	lc_help = (char *)XtMalloc(strlen(lc_messages)+1);
	strcpy(lc_help,lc_messages);
	OlRegisterHelp(OL_WIDGET_HELP, (XtPointer)button, 
				"Example PopupWindow", OL_STRING_SOURCE, 
				lc_help);

	/************************************************************
	 * Create NoticeShell Example
	 ***********************************************************/

	(void)sprintf((char *)lc_messages,
			dgettext(OlittableDomain,"Show Notice.."));
	lc_help = (char *)XtMalloc(strlen(lc_messages)+1);
	strcpy(lc_help,lc_messages);
	button = XtVaCreateManagedWidget("shellbutton", 
					oblongButtonWidgetClass,
					p_table[C_NOTICESHELL].entry,
					XtNlabel, (XtArgVal)lc_help,
					NULL);

	p_table[C_NOTICESHELL].entry_widget = button;

	p_table[C_NOTICESHELL].widget = XtVaCreatePopupShell("shellexample", 
		     			noticeShellWidgetClass, 
					button,
					XtNx, (XtArgVal)(3 * entry_width) + 3,
					XtNy, (XtArgVal)entry_height + 3,
					XtNemanateWidget, (XtArgVal)button,
					NULL);

	/* 
   	 * Add callback to popup button now that we have noticeshell widget ID. 
	*/
	XtAddCallback(button, XtNselect, popupCB1, 
						p_table[C_NOTICESHELL].widget);

	XtVaGetValues(p_table[C_NOTICESHELL].widget, 
				XtNtextArea, &childs[0],
				XtNcontrolArea, &childs[1],
				NULL);

	parent = XtParent(childs[0]);

	/* add text to text area of noticebox */
	(void)sprintf((char *)lc_messages,
			dgettext(OlittableDomain,"Read This Yet?:\nConfirm"));
	lc_help = (char *)XtMalloc(strlen(lc_messages)+1);
	strcpy(lc_help,lc_messages);
	XtVaSetValues(childs[0],
				XtNstring,(XtArgVal)lc_help,
				NULL);

	XtVaSetValues(childs[1],
				XtNhPad,	(XtArgVal)20,
				XtNspace, (XtArgVal)30,
				NULL);


	/* this button has an exit() callback */
	grandchilds[0]= XtVaCreateManagedWidget("shellchild", 
						oblongButtonWidgetClass,
						childs[1],
						XtNlabel, (XtArgVal)"OKAY",
						NULL);

	if (color){
		XtVaSetValues(button,
				XtNbackground,(XtArgVal)colors[SHELLBUTTON],
				NULL);
		XtVaSetValues(p_table[C_NOTICESHELL].widget,
				XtNbackground,(XtArgVal)colors[BACKGROUND],
				NULL);
		XtVaSetValues(childs[0],
				XtNbackground,(XtArgVal)colors[BACKGROUND],
				NULL);
		XtVaSetValues(childs[1],
				XtNbackground,(XtArgVal)colors[BACKGROUND],
				NULL);
		XtVaSetValues(grandchilds[0],
				XtNbackground,(XtArgVal)colors[BACKGROUND],
				NULL);
		XtVaSetValues(parent,
				XtNbackground,(XtArgVal)colors[BACKGROUND],
				NULL);

	}

	(void)sprintf((char *)lc_messages,
				dgettext(OlittableDomain,noticehelp));
	lc_help = (char *)XtMalloc(strlen(lc_messages)+1);
	strcpy(lc_help,lc_messages);
	OlRegisterHelp(OL_WIDGET_HELP, (XtPointer)button, 
				"Example Notice ", OL_STRING_SOURCE, 
				lc_help);



	/************************************************************
	 * Create MenuShell Example
	 ***********************************************************/

	(void)sprintf((char *)lc_messages,
			dgettext(OlittableDomain,"Click Here for Menu"));
	lc_help = (char *)XtMalloc(strlen(lc_messages)+1);
	strcpy(lc_help,lc_messages);
	n = 0;
	if (color) 
		XtSetArg(arg[n], XtNbackground, colors[SHELL]);n++;
	XtSetArg(arg[n], XtNrecomputeSize, False);n++;
	XtSetArg(arg[n], XtNwidth, entry_width - 6);n++;
	XtSetArg(arg[n], XtNheight, 18);n++;
	XtSetArg(arg[n], XtNalignment, OL_CENTER);n++;
	XtSetArg(arg[n], XtNfont, font[SIZE12]);n++;
	XtSetArg(arg[n], XtNstring, lc_help);n++;

	parent = XtCreateManagedWidget("shellbutton", 
					staticTextWidgetClass,
					p_table[C_MENUSHELL].entry, arg, n);

	p_table[C_MENUSHELL].entry_widget = parent;

	onlychild = XtVaCreateManagedWidget("comment",
				staticTextWidgetClass,
				p_table[C_MENUSHELL].entry,
				XtNx,		(XtArgVal)2,
				XtNy,		(XtArgVal)58,
				XtNrecomputeSize,(XtArgVal)False,
				XtNwidth,	(XtArgVal)entry_width -4,
				XtNstring,(XtArgVal)"                       ",
				XtNstrip,	(XtArgVal)False,
				XtNgravity,	(XtArgVal)CenterGravity,
				XtNfont,	(XtArgVal)font[SIZE10],
				NULL);

	(void)sprintf((char *)lc_messages,
			dgettext(OlittableDomain,"Le Menu"));
	lc_help = (char *)XtMalloc(strlen(lc_messages)+1);
	strcpy(lc_help,lc_messages);
	n = 0;
	if (color) 
		XtSetArg(arg[n], XtNbackground, colors[BACKGROUND]);n++;
	XtSetArg(arg[n], XtNpushpin, OL_OUT);n++;
	XtSetArg(arg[n], XtNtitle, lc_help);n++;
	p_table[C_MENUSHELL].widget = XtCreatePopupShell("shellexample",
			 menuShellWidgetClass, parent, arg,n);

	n = 0;
	XtSetArg(arg[n], XtNmenuPane, &menupane);n++;
	XtGetValues(p_table[C_MENUSHELL].widget, arg, n);
	n = 0;
	(void)sprintf((char *)lc_messages,
			dgettext(OlittableDomain,"spaghetti"));
	lc_help = (char *)XtMalloc(strlen(lc_messages)+1);
	strcpy(lc_help,lc_messages);
	childs[0] =XtVaCreateManagedWidget((char *)lc_help,
					oblongButtonWidgetClass, 
					menupane,
					XtNaccelerator,	(XtArgVal)"Ctrl<s>",
					XtNacceleratorText, (XtArgVal)"^s",
					NULL);

	(void)sprintf((char *)lc_messages,
			dgettext(OlittableDomain,"ravioli"));
	lc_help = (char *)XtMalloc(strlen(lc_messages)+1);
	strcpy(lc_help,lc_messages);
	childs[1] =XtVaCreateManagedWidget((char *)lc_help,
					oblongButtonWidgetClass, 
					menupane,
					XtNaccelerator,	(XtArgVal)"Ctrl<r>",
					XtNacceleratorText, (XtArgVal)"^r",
					NULL);	

	(void)sprintf((char *)lc_messages,
			dgettext(OlittableDomain,"linguini"));
	lc_help = (char *)XtMalloc(strlen(lc_messages)+1);
	strcpy(lc_help,lc_messages);
	childs[2] =XtVaCreateManagedWidget((char *)lc_help,
					oblongButtonWidgetClass, 
					menupane,
					XtNaccelerator,	(XtArgVal)"Ctrl<l>",
					XtNacceleratorText, (XtArgVal)"^l",
					NULL);

	(void)sprintf((char *)lc_messages,
			dgettext(OlittableDomain,"hamburger"));
	lc_help = (char *)XtMalloc(strlen(lc_messages)+1);
	strcpy(lc_help,lc_messages);
	childs[3] =XtVaCreateManagedWidget((char *)lc_help,
					oblongButtonWidgetClass, 
					menupane, 
					XtNaccelerator,	(XtArgVal)"Ctrl<h>",
					XtNacceleratorText, (XtArgVal)"^h",
					NULL);

	for (i=0; i < 4;i++){
		XtVaSetValues(childs[i],
					XtNuserData,	(XtArgVal)onlychild,
					NULL);
		XtAddCallback(childs[i],XtNselect,leMenuCB,(XtPointer)i);
	}

	if (color){
		for(i=0; i < 4; i++)
			XtVaSetValues(childs[i],
					XtNbackground,	
					   (XtArgVal)colors[BACKGROUND],
					NULL);

		XtVaSetValues(onlychild,
				XtNbackground,	
					  (XtArgVal)colors[SHELL],
				NULL);
	}
	else
		XtVaSetValues(onlychild,
				XtNbackgroundPixmap, (XtArgVal)pixmaps[SHELL],
				NULL);



	(void)sprintf((char *)lc_messages,
				dgettext(OlittableDomain,menuhelp));
	lc_help = (char *)XtMalloc(strlen(lc_messages)+1);
	strcpy(lc_help,lc_messages);
	OlRegisterHelp(OL_WIDGET_HELP, (XtPointer)parent, 
				"Example Menu", OL_STRING_SOURCE, 
				lc_help);

	/************************************************************
	 * Create AbbrevMenuButton Example
	 ***********************************************************/

	n = 0;
	XtSetArg(arg[n], XtNx, (XtArgVal) 22);n++;
	XtSetArg(arg[n], XtNy, (XtArgVal) 40);n++;
	if (color) 
		XtSetArg(arg[n], XtNbackground, colors[BACKGROUND]);n++;
	p_table[C_ABBREVMENUBUTTON].widget = XtCreateManagedWidget(
				"primitiveexample",
				abbrevMenuButtonWidgetClass, 
				p_table[C_ABBREVMENUBUTTON].entry, arg, n);

	p_table[C_ABBREVMENUBUTTON].entry_widget = NULL;

	n = 0;
	if (color) 
		{XtSetArg(arg[n], XtNbackground, colors[PRIMITIVE]);n++;}
	else	
		XtSetArg(arg[n], XtNbackgroundPixmap, pixmaps[PRIMITIVE]);n++;
	XtSetArg(arg[n], XtNstring, "Sparcstation1+");n++;
	XtSetArg(arg[n], XtNx, 42);n++;
	XtSetArg(arg[n], XtNy, 40);n++;
	onlychild = XtCreateManagedWidget("Sparcstation1+", 
			staticTextWidgetClass,
			p_table[C_ABBREVMENUBUTTON].entry, arg, n);
	n = 0;				
	XtSetArg(arg[n], XtNpreviewWidget, onlychild);n++;
	XtSetValues(p_table[C_ABBREVMENUBUTTON].widget, arg, n);


	n = 0;
	XtSetArg(arg[n], XtNmenuPane, &menupane);n++;
	XtGetValues(p_table[C_ABBREVMENUBUTTON].widget, arg, n);
	n = 0;
	if (color) 
		XtSetArg(arg[n], XtNbackground, colors[BACKGROUND]);n++;
	XtSetValues(menupane, arg, n);
	XtAddCallback(XtCreateManagedWidget("Sparcstation1+",
			  oblongButtonWidgetClass, menupane,
			 arg, n), XtNselect, abbrevCB, (Widget)onlychild);
	XtAddCallback(XtCreateManagedWidget("SparcstationSLC",
			    oblongButtonWidgetClass, menupane,
			 arg, n), XtNselect, abbrevCB, (Widget)onlychild);
	XtAddCallback(XtCreateManagedWidget("SparcstationIPC",
			   oblongButtonWidgetClass, menupane, 
			arg, n), XtNselect, abbrevCB, (Widget)onlychild);

	
	(void)sprintf((char *)lc_messages,
				dgettext(OlittableDomain,abbrevmenubuthelp));
	lc_help = (char *)XtMalloc(strlen(lc_messages)+1);
	strcpy(lc_help,lc_messages);
	OlRegisterHelp(OL_WIDGET_HELP, 
			(XtPointer)(p_table[C_ABBREVMENUBUTTON].widget),
			 "Example AbbrevMenuButton", OL_STRING_SOURCE, 
				lc_help);
	OlRegisterHelp(OL_WIDGET_HELP, (XtPointer)onlychild, 
				"Example AbbrevMenuButton", OL_STRING_SOURCE, 
				lc_help);


	/************************************************************
	 * Create Gauge Example
	 ***********************************************************/

	
	p_table[C_GAUGE].widget = XtVaCreateManagedWidget("primexample", 
					gaugeWidgetClass, 
					p_table[C_GAUGE].entry,
					XtNorientation,	(XtArgVal)OL_HORIZONTAL,
					XtNwidth,	(XtArgVal)100,
					XtNsliderValue,	(XtArgVal)45,
					XtNtickUnit,	(XtArgVal)OL_PERCENT,
					XtNticks,	(XtArgVal)5,
					NULL); 

	p_table[C_GAUGE].entry_widget = p_table[C_GAUGE].widget;


	timerID = XtAppAddTimeOut(
		XtWidgetToApplicationContext(p_table[C_GAUGE].widget), INTERVAL,
		(XtTimerCallbackProc)moveGauge, p_table[C_GAUGE].widget);

	/*
	 * Build PopupMenu so user can toggle Mode of Gauge
	 * (and stop/start the timer)
	 */
	shell = XtVaCreatePopupShell("menu",
				menuShellWidgetClass,
				p_table[C_GAUGE].widget,
				XtNtitle,	(XtArgVal)"Gauge Mode",
				NULL);

	XtVaGetValues(shell,
				XtNmenuPane,	&menupane,
				NULL);

	onlychild = XtVaCreateManagedWidget("Stop",
				oblongButtonWidgetClass,
				menupane,
				NULL);

	if (color){
		XtVaSetValues(p_table[C_GAUGE].widget,
					XtNbackground,	colors[BACKGROUND],
					NULL);
		XtVaSetValues(shell,
					XtNbackground,	colors[BACKGROUND],
					NULL);
	}




	XtAddCallback(onlychild, XtNselect, toggleMode, p_table[C_GAUGE].widget);



	(void)sprintf((char *)lc_messages,
				dgettext(OlittableDomain,gaugehelp));
	lc_help = (char *)XtMalloc(strlen(lc_messages)+1);
	strcpy(lc_help,lc_messages);
	OlRegisterHelp(OL_WIDGET_HELP, 
				(XtPointer)(p_table[C_GAUGE].widget),
				"Example Gauge", OL_STRING_SOURCE, 
				lc_help);


	/************************************************************
	 * Create NonExclusives Example
	 ***********************************************************/
	n = 0;
	XtSetArg(arg[n], XtNlayoutType, (XtArgVal) OL_FIXEDCOLS);n++;
	XtSetArg(arg[n], XtNmeasure, (XtArgVal) 1);n++;
	p_table[C_NONEXCLUSIVES].widget = XtCreateManagedWidget(
				"constraintexample",
				 nonexclusivesWidgetClass, 
				p_table[C_NONEXCLUSIVES].entry, arg, n);

	p_table[C_NONEXCLUSIVES].entry_widget = p_table[C_NONEXCLUSIVES].widget;

	n = 0;
	if (color) 
		XtSetArg(arg[n], XtNbackground, colors[BACKGROUND]);n++;
	XtSetArg(arg[n], XtNlabelJustify, OL_CENTER);n++;
	XtSetArg(arg[n], XtNset, True);n++;
	reset = n;

	(void)sprintf((char *)lc_messages,
			dgettext(OlittableDomain,"PickMe"));
	lc_help = (char *)XtMalloc(strlen(lc_messages)+1);
	strcpy(lc_help,lc_messages);
	XtSetArg(arg[n], XtNlabel, lc_help);n++;
	childs[0] = XtCreateManagedWidget("PickMe",
			 rectButtonWidgetClass,
			p_table[C_NONEXCLUSIVES].widget, arg, n);
	n = reset;
	(void)sprintf((char *)lc_messages,
			dgettext(OlittableDomain,"And/Or Me"));
	lc_help = (char *)XtMalloc(strlen(lc_messages)+1);
	strcpy(lc_help,lc_messages);
	XtSetArg(arg[n], XtNlabel, lc_help);n++;
	childs[1] = XtCreateManagedWidget((char *)lc_messages,
			rectButtonWidgetClass,
			p_table[C_NONEXCLUSIVES].widget, arg, n);

	(void)sprintf((char *)lc_messages,
				dgettext(OlittableDomain,nonexclhelp));
	lc_help = (char *)XtMalloc(strlen(lc_messages)+1);
	strcpy(lc_help,lc_messages);
	OlRegisterHelp(OL_WIDGET_HELP, 
			(XtPointer)(p_table[C_NONEXCLUSIVES].widget),
			 "Example Nonexclusives", 
			OL_STRING_SOURCE, lc_help);



	/************************************************************
	 * Create Exclusives Example
	 ***********************************************************/
	n = 0;
	XtSetArg(arg[n], XtNlayoutType, (XtArgVal) OL_FIXEDCOLS);n++;
	XtSetArg(arg[n], XtNmeasure, (XtArgVal) 1);n++;
	p_table[C_EXCLUSIVES].widget = XtCreateManagedWidget(
				"constraintexample", exclusivesWidgetClass, 
				p_table[C_EXCLUSIVES].entry, arg, n);

	p_table[C_EXCLUSIVES].entry_widget = p_table[C_EXCLUSIVES].widget;
	n = 0;
	if (color) 
		XtSetArg(arg[n], XtNbackground, colors[BACKGROUND]);n++;
	XtSetArg(arg[n], XtNlabelJustify, OL_CENTER);n++;
	reset = n;

	(void)sprintf((char *)lc_messages,
			dgettext(OlittableDomain,"Pick Me"));
	lc_help = (char *)XtMalloc(strlen(lc_messages)+1);
	strcpy(lc_help,lc_messages);
	XtSetArg(arg[n], XtNlabel, lc_help);n++;
	childs[0] = XtCreateManagedWidget("PickMe", rectButtonWidgetClass,
					p_table[C_EXCLUSIVES].widget, arg, n);
	n = reset;
	(void)sprintf((char *)lc_messages,
			dgettext(OlittableDomain,"Or Me"));
	lc_help = (char *)XtMalloc(strlen(lc_messages)+1);
	strcpy(lc_help,lc_messages);
	XtSetArg(arg[n], XtNlabel, lc_messages);n++;
	childs[1] = XtCreateManagedWidget((char *)lc_help,
					rectButtonWidgetClass,
					p_table[C_EXCLUSIVES].widget, arg, n);

	(void)sprintf((char *)lc_messages,
				dgettext(OlittableDomain,exclhelp));
	lc_help = (char *)XtMalloc(strlen(lc_messages)+1);
	strcpy(lc_help,lc_messages);
	OlRegisterHelp(OL_WIDGET_HELP, 
				(XtPointer)(p_table[C_EXCLUSIVES].widget),
				 "Example Exclusives", OL_STRING_SOURCE, 
				lc_help);




	/************************************************************
	 * Create Checkbox Example
	 ***********************************************************/
	(void)sprintf((char *)lc_messages,
			dgettext(OlittableDomain,"Check Me:"));
	lc_help = (char *)XtMalloc(strlen(lc_messages)+1);
	strcpy(lc_help,lc_messages);
	n = 0;
	XtSetArg(arg[n], XtNlabel, lc_help);n++;
	XtSetArg(arg[n], XtNset, True);n++;

	p_table[C_CHECKBOX].widget = XtCreateManagedWidget(
				"constraintexample", checkBoxWidgetClass, 
				p_table[C_CHECKBOX].entry, arg, n);

	p_table[C_CHECKBOX].entry_widget =  p_table[C_CHECKBOX].widget;

	XtSetArg(arg[0], XtNforeground, 
		BlackPixel(XtDisplay(p_table[C_CHECKBOX].widget), 
		DefaultScreen(XtDisplay(p_table[C_CHECKBOX].widget))));n++;
	XtSetValues(p_table[C_CHECKBOX].widget, arg, 1);


	(void)sprintf((char *)lc_messages,
				dgettext(OlittableDomain,checkboxhelp));
	lc_help = (char *)XtMalloc(strlen(lc_messages)+1);
	strcpy(lc_help,lc_messages);
	OlRegisterHelp(OL_WIDGET_HELP, 
			(XtPointer)(p_table[C_CHECKBOX].widget),
			 "Example CheckBox", OL_STRING_SOURCE, 
				lc_help);



	/************************************************************
	 * Create Slider Example
	 ***********************************************************/
	n = 0;
	if (color) 
		XtSetArg(arg[n], XtNbackground, colors[BACKGROUND]);n++;
	XtSetArg(arg[n], XtNorientation, OL_HORIZONTAL);n++;
	XtSetArg(arg[n], XtNgranularity, (XtArgVal) 3);n++;
	XtSetArg(arg[n], XtNwidth,  (XtArgVal) 95);n++;
	XtSetArg(arg[n], XtNsliderMax, 70);n++;
	XtSetArg(arg[n], XtNsliderValue, 15);n++;
	p_table[C_SLIDER].widget = XtCreateManagedWidget(
				"primitiveexample", sliderWidgetClass, 
				p_table[C_SLIDER].entry, arg, n);

	 p_table[C_SLIDER].entry_widget = p_table[C_SLIDER].widget;

	(void)sprintf((char *)lc_messages,
				dgettext(OlittableDomain,sliderhelp));
	lc_help = (char *)XtMalloc(strlen(lc_messages)+1);
	strcpy(lc_help,lc_messages);
	OlRegisterHelp(OL_WIDGET_HELP, 
			(XtPointer)(p_table[C_SLIDER].widget),
				 "Example Slider", OL_STRING_SOURCE, 
				lc_help);


	/************************************************************
	 * Create MenuButton Example
	 ***********************************************************/
	if (color) 
		XtSetArg(arg[n], XtNbackground, colors[BACKGROUND]);n++;
	(void)sprintf((char *)lc_messages,
			dgettext(OlittableDomain,"BringUpMenu"));
	lc_help = (char *)XtMalloc(strlen(lc_messages)+1);
	strcpy(lc_help,lc_messages);
	p_table[C_MENUBUTTON].widget = XtVaCreateManagedWidget(
				"primitiveexample", 
				menuButtonWidgetClass, 
				p_table[C_MENUBUTTON].entry,
				XtNlabel, 	(XtArgVal)lc_help,
				XtNpushpin, 	(XtArgVal)OL_OUT,
				XtNtitle,	(XtArgVal)"FontSize",
				NULL);

	 p_table[C_MENUBUTTON].entry_widget = p_table[C_MENUBUTTON].widget;

	XtVaGetValues(p_table[C_MENUBUTTON].widget,
				XtNmenuPane, &menupane,
				NULL);

	onlychild = XtVaCreateManagedWidget("primchild", 
				exclusivesWidgetClass,
				menupane,
				XtNlayoutType, (XtArgVal) OL_FIXEDCOLS,
				XtNmeasure, (XtArgVal) 2,
				NULL);

	for (i=0; i < XtNumber(mb_choicestring); i++){
		grandchilds[i] = XtVaCreateManagedWidget(mb_choicestring[i], 
				rectButtonWidgetClass,
				onlychild,
				XtNlabelJustify, (XtArgVal)OL_CENTER,
				NULL);
		if (color)				
			XtVaSetValues(grandchilds[i],
				XtNbackground,(XtArgVal)colors[BACKGROUND],
				NULL);
	}

	if (color){
		XtVaSetValues(p_table[C_MENUBUTTON].widget,
				XtNbackground,(XtArgVal)colors[BACKGROUND],
				NULL);
		XtVaSetValues(menupane,
				XtNbackground,(XtArgVal)colors[BACKGROUND],
				NULL);
	}

	(void)sprintf((char *)lc_messages,
				dgettext(OlittableDomain,menubuttonhelp));
	lc_help = (char *)XtMalloc(strlen(lc_messages)+1);
	strcpy(lc_help,lc_messages);
	OlRegisterHelp(OL_WIDGET_HELP,(XtPointer)(p_table[C_MENUBUTTON].widget),
				"Example MenuButton", OL_STRING_SOURCE, 
				lc_help);


	/************************************************************
	 * Create RectButton Example
	 ***********************************************************/
	n = 0;
	if (color) 
		XtSetArg(arg[n], XtNbackground, colors[BACKGROUND]);n++;
	(void)sprintf((char *)lc_messages,
			dgettext(OlittableDomain,"UseMe2Build 12&13"));
	lc_help = (char *)XtMalloc(strlen(lc_messages)+1);
	strcpy(lc_help,lc_messages);
	XtSetArg(arg[n], XtNlabel, (char *)lc_help);n++;
	p_table[C_RECTBUTTON].widget = XtCreateManagedWidget("primitiveexample",
					 rectButtonWidgetClass, 
					p_table[C_RECTBUTTON].entry, arg, n);

	p_table[C_RECTBUTTON].entry_widget = p_table[C_RECTBUTTON].widget;

	(void)sprintf((char *)lc_messages,
				dgettext(OlittableDomain,rectbuttonhelp));
	lc_help = (char *)XtMalloc(strlen(lc_messages)+1);
	strcpy(lc_help,lc_messages);
	OlRegisterHelp(OL_WIDGET_HELP, 
				(XtPointer)(p_table[C_RECTBUTTON].widget), 
				"Example RectButton", OL_STRING_SOURCE, 
				lc_help);



	/************************************************************
	 * Create Stub Example
	 ***********************************************************/

	n = 0;
	if (color) 
		XtSetArg(arg[n], XtNbackground, colors[BACKGROUND]);n++;
	XtSetArg(arg[n], XtNborderWidth, (XtArgVal) 1);n++;
	XtSetArg(arg[n], XtNwidth, (XtArgVal) 75);n++;
	XtSetArg(arg[n], XtNheight, (XtArgVal) 75);n++;
	XtSetArg(arg[n], XtNexpose, (XtArgVal)DrawOnStub);n++;

	p_table[C_STUB].widget = XtCreateManagedWidget("constraintexample",
					 stubWidgetClass, 
					p_table[C_STUB].entry, arg, n);

	p_table[C_STUB].entry_widget = p_table[C_STUB].widget;


	(void)sprintf((char *)lc_messages,
				dgettext(OlittableDomain,stubhelp));
	lc_help = (char *)XtMalloc(strlen(lc_messages)+1);
	strcpy(lc_help,lc_messages);
	OlRegisterHelp(OL_WIDGET_HELP, 
			(XtPointer)(p_table[C_STUB].widget),
				"Example Stub", OL_STRING_SOURCE, 
				lc_help);




	/************************************************************
	 * Create Flatnonexclusives Example
	 ***********************************************************/
	n = 0;
	if (color) {
		nonexc_items[0].background = (XtArgVal)colors[BACKGROUND];
		nonexc_items[1].background = (XtArgVal)colors[BACKGROUND];
		nonexc_items[2].background = (XtArgVal)colors[BACKGROUND];

		}
	(void)sprintf((char *)lc_messages,dgettext(OlittableDomain,"Bold"));
	nonexc_items[0].label = (XtArgVal)XtNewString((char *)lc_messages);
	(void)sprintf((char *)lc_messages,dgettext(OlittableDomain,"Italic"));
	nonexc_items[1].label = (XtArgVal)XtNewString((char *)lc_messages);
       (void)sprintf((char *)lc_messages,dgettext(OlittableDomain,"Underline"));
	nonexc_items[2].label = (XtArgVal)XtNewString((char *)lc_messages);
	
	XtSetArg(arg[n], XtNlayoutType, (XtArgVal) OL_FIXEDCOLS);n++;
	XtSetArg(arg[n], XtNitems, nonexc_items);n++;
	XtSetArg(arg[n], XtNnumItems, XtNumber(nonexc_items));n++;
	XtSetArg(arg[n], XtNitemFields, nonexc_fields);n++;
	XtSetArg(arg[n], XtNnumItemFields, XtNumber(nonexc_fields));n++;
	p_table[C_FLATNONEXCLUSIVES].widget = XtCreateManagedWidget(
				"flatnonexclusive",
				flatNonexclusivesWidgetClass, 
				p_table[C_FLATNONEXCLUSIVES].entry, arg, n);

	p_table[C_FLATNONEXCLUSIVES].entry_widget = p_table[C_FLATNONEXCLUSIVES].widget;



	/************************************************************
	 * Create FlatExclusives Example
	 ***********************************************************/
	n = 0;
	XtSetArg(arg[n], XtNwidth, (XtArgVal) 60);n++;
	XtSetArg(arg[n], XtNheight, (XtArgVal) 50);n++;

	if (color) {
		exc_items[0].background = (XtArgVal) colors[BACKGROUND];
		exc_items[1].background = (XtArgVal) colors[PRIMITIVE];
		exc_items[2].background = (XtArgVal) colors[SHELL];
		exc_items[3].background = (XtArgVal) colors[CONSTRAINT];
	}

	(void)sprintf((char *)lc_messages,dgettext(OlittableDomain,"Gray"));
	exc_items[0].label = (XtArgVal)XtNewString((char *)lc_messages);
	(void)sprintf((char *)lc_messages,dgettext(OlittableDomain,"Blue"));
	exc_items[1].label = (XtArgVal)XtNewString((char *)lc_messages);
	(void)sprintf((char *)lc_messages,dgettext(OlittableDomain,"Pink"));
	exc_items[2].label = (XtArgVal)XtNewString((char *)lc_messages);
	(void)sprintf((char *)lc_messages,dgettext(OlittableDomain,"Aqua"));
	exc_items[3].label = (XtArgVal)XtNewString((char *)lc_messages);

	
	XtSetArg(arg[n], XtNlayoutType, (XtArgVal) OL_FIXEDCOLS);n++;
	XtSetArg(arg[n], XtNmeasure,	(XtArgVal) 2); n++;
	XtSetArg(arg[n], XtNitems, exc_items);n++;
	XtSetArg(arg[n], XtNnumItems, 4);n++;
	XtSetArg(arg[n], XtNitemFields, exc_fields);n++;
	XtSetArg(arg[n], XtNnumItemFields, XtNumber(exc_fields));n++;
	p_table[C_FLATEXCLUSIVES].widget = XtCreateManagedWidget(
				"flatexclusive",flatExclusivesWidgetClass,
				p_table[C_FLATEXCLUSIVES].entry, arg, n);

	p_table[C_FLATEXCLUSIVES].entry_widget = p_table[C_FLATEXCLUSIVES].widget;



	/************************************************************
	 * Create FlatCheckbox Example
	 ***********************************************************/

	n = 0;
	XtSetArg(arg[n], XtNwidth, (XtArgVal) 60);n++;
	XtSetArg(arg[n], XtNheight, (XtArgVal) 50);n++;

	(void)sprintf((char *)lc_messages,
				dgettext(OlittableDomain,"Owner Access:"));
	chk_items[0].label = (XtArgVal)XtNewString((char *)lc_messages);
	(void)sprintf((char *)lc_messages,
				dgettext(OlittableDomain,"Group Access:"));
	chk_items[1].label = (XtArgVal)XtNewString((char *)lc_messages);
	
	XtSetArg(arg[n], XtNlayoutType, (XtArgVal) OL_FIXEDCOLS);n++;
	XtSetArg(arg[n], XtNfont,	(XtArgVal)font[LABEL]);n++;
	XtSetArg(arg[n], XtNitems, chk_items);n++;
	XtSetArg(arg[n], XtNnumItems, 2);n++;
	XtSetArg(arg[n], XtNitemFields, chk_fields);n++;
	XtSetArg(arg[n], XtNnumItemFields, XtNumber(chk_fields));n++;
	p_table[C_FLATCHECKBOX].widget = XtCreateManagedWidget(
				"flatcheckbox",flatCheckBoxWidgetClass,
				p_table[C_FLATCHECKBOX].entry, arg, n);

	p_table[C_FLATCHECKBOX].entry_widget = p_table[C_FLATCHECKBOX].widget;



	/************************************************************
	 * Create OblongButton Example
	 ***********************************************************/
	(void)sprintf((char *)lc_messages,
			dgettext(OlittableDomain,"Press Me..."));
	lc_help = (char *)XtMalloc(strlen(lc_messages)+1);
	strcpy(lc_help,lc_messages);
	n = 0;
	if (color) 
		XtSetArg(arg[n], XtNbackground, colors[BACKGROUND]);n++;
	XtSetArg(arg[n], XtNlabel, (char *)lc_help);n++;
	p_table[C_OBLONGBUTTON].widget = XtCreateManagedWidget(
					"primitiveexample", 
					oblongButtonWidgetClass, 
					p_table[C_OBLONGBUTTON].entry, arg, n);

	p_table[C_OBLONGBUTTON].entry_widget =  p_table[C_OBLONGBUTTON].widget;

	(void)sprintf((char *)lc_messages,
				dgettext(OlittableDomain,oblongbuttonhelp));
	lc_help = (char *)XtMalloc(strlen(lc_messages)+1);
	strcpy(lc_help,lc_messages);
	OlRegisterHelp(OL_WIDGET_HELP, 
				(XtPointer)(p_table[C_OBLONGBUTTON].widget), 
				"Example OblongButton", OL_STRING_SOURCE, 
				lc_help);


	/************************************************************
	 * Create ScrollBar Example
	 ***********************************************************/
	n = 0;
	if (color) 
		XtSetArg(arg[n], XtNbackground, colors[BACKGROUND]);n++;
	XtSetArg(arg[n], XtNorientation, OL_HORIZONTAL);n++;
	XtSetArg(arg[n], XtNwidth,  (XtArgVal) 110);n++;
	XtSetArg(arg[n], XtNsliderMin,	(XtArgVal)	 0);n++;
	XtSetArg(arg[n], XtNsliderMax,	(XtArgVal)	150);n++;
	p_table[C_SCROLLBAR].widget = XtCreateManagedWidget("primitiveexample",
				 scrollbarWidgetClass, 
				p_table[C_SCROLLBAR].entry, arg, n);

	p_table[C_SCROLLBAR].entry_widget = p_table[C_SCROLLBAR].widget;

	(void)sprintf((char *)lc_messages,
				dgettext(OlittableDomain,scrollbarhelp));
	lc_help = (char *)XtMalloc(strlen(lc_messages)+1);
	strcpy(lc_help,lc_messages);
	OlRegisterHelp(OL_WIDGET_HELP, 
			(XtPointer)(p_table[C_SCROLLBAR].widget),
			"Example Scrollbar", OL_STRING_SOURCE, 
				lc_help);



	/************************************************************
	 * Create Caption Example
	 ***********************************************************/
	(void)sprintf((char *)lc_messages,
			dgettext(OlittableDomain,"Generic Label:"));
	lc_help = (char *)XtMalloc(strlen(lc_messages)+1);
	strcpy(lc_help,lc_messages);
	n = 0;
	XtSetArg(arg[n], XtNborderWidth, 1);n++;
	if (color) 
		XtSetArg(arg[n], XtNbackground, colors[BACKGROUND]);n++;
	XtSetArg(arg[n], XtNfont, font[LABEL]);n++;
	XtSetArg(arg[n], XtNlabel, lc_help);n++;
	p_table[C_CAPTION].widget = XtCreateManagedWidget("constraintexample", 
				captionWidgetClass, 
				p_table[C_CAPTION].entry, arg, n);

	p_table[C_CAPTION].entry_widget = p_table[C_CAPTION].widget;

	(void)sprintf((char *)lc_messages, dgettext(OlittableDomain,"Text"));
	lc_help = (char *)XtMalloc(strlen(lc_messages)+1);
	strcpy(lc_help,lc_messages);
	n = 0;
	if (color) 
		XtSetArg(arg[n], XtNbackground, colors[BACKGROUND]);n++;
	XtSetArg(arg[n], XtNborderWidth, (XtArgVal) 0);n++;
	XtSetArg(arg[n], XtNmaximumSize, 5);n++;
	XtSetArg(arg[n], XtNstring, lc_help);n++;
	XtSetArg(arg[n], XtNfont, font[SIZE10]);n++;
	XtSetArg(arg[n], XtNcharsVisible, 	(XtArgVal)4);	n++;
	XtSetArg(arg[n], XtNimPreeditStyle, OL_ON_THE_SPOT);n++;
	XtCreateManagedWidget("primchild", textFieldWidgetClass,
					p_table[C_CAPTION].widget, arg, n);

	(void)sprintf((char *)lc_messages,
				dgettext(OlittableDomain,captionhelp));
	lc_help = (char *)XtMalloc(strlen(lc_messages)+1);
	strcpy(lc_help,lc_messages);
	OlRegisterHelp(OL_WIDGET_HELP, 
			(XtPointer)(p_table[C_CAPTION].widget),
			 "Example Caption", OL_STRING_SOURCE, 
				lc_help);



	/************************************************************
	 * Create StaticText Example
	 ***********************************************************/
	(void)sprintf((char *)lc_messages, dgettext(OlittableDomain,
		"I cannot be edited.\nBut Go ahead and\ntry...Make my day!"));
	lc_help = (char *)XtMalloc(strlen(lc_messages)+1);
	strcpy(lc_help,lc_messages);
	n = 0;
	XtSetArg(arg[n], XtNborderWidth, 1);n++;
	if (color) 
		XtSetArg(arg[n], XtNbackground, colors[BACKGROUND]);n++;
	XtSetArg(arg[n], XtNfont, font[SIZE10]);n++;
	XtSetArg(arg[n], XtNstring, lc_help);n++;
	p_table[C_STATICTEXT].widget = XtCreateManagedWidget(
				"primitiveexample", staticTextWidgetClass, 
				p_table[C_STATICTEXT].entry, arg, n);

	p_table[C_STATICTEXT].entry_widget = p_table[C_STATICTEXT].widget;

	(void)sprintf((char *)lc_messages,
				dgettext(OlittableDomain,statictexthelp));
	lc_help = (char *)XtMalloc(strlen(lc_messages)+1);
	strcpy(lc_help,lc_messages);
	OlRegisterHelp(OL_WIDGET_HELP, 
				(XtPointer)(p_table[C_STATICTEXT].widget), 
				"Example StaticText", OL_STRING_SOURCE, 
				lc_help);

	/************************************************************
	 * Create DropTarget Example
	 ************************************************************/

	p_table[C_DROPTARGET].widget = XtVaCreateManagedWidget("primitiveexample",
			dropTargetWidgetClass,
			p_table[C_DROPTARGET].entry,
			XtNfull,(XtArgVal)TRUE,
                	XtNdndMoveCursor,
                          (XtArgVal)OlGetMoveDocDragCursor(p_table[C_DROPTARGET].entry),
                	XtNdndCopyCursor,
                          (XtArgVal)OlGetDupeDocDragCursor(p_table[C_DROPTARGET].entry),
                	XtNdndAcceptCursor,
                          (XtArgVal)OlGetDupeDocDropCursor(p_table[C_DROPTARGET].entry),
                	XtNdndRejectCursor,
                          (XtArgVal)OlGetDupeDocNoDropCursor(p_table[C_DROPTARGET].entry),
			NULL);
        p_table[C_DROPTARGET].entry_widget =  p_table[C_DROPTARGET].widget;

        (void)sprintf((char *)lc_messages,
                dgettext(OlittableDomain,"Drag From Me..."));
        lc_help = (char *)XtMalloc(strlen(lc_messages)+1);
        strcpy(lc_help,lc_messages);

	childs[0] = XtVaCreateManagedWidget("primitivechild",
			staticTextWidgetClass,
			p_table[C_DROPTARGET].entry,
			XtNfont,	(XtArgVal)font[SIZE10],
			XtNstring, 	(XtArgVal)lc_help,
                        XtNx,   	(XtArgVal)30,
                        XtNy,   	(XtArgVal)14,
			NULL);

        (void)sprintf((char *)lc_messages,
                dgettext(OlittableDomain,"Drop On Me!"));
        lc_help = (char *)XtMalloc(strlen(lc_messages)+1); 
        strcpy(lc_help,lc_messages); 
 
        childs[1] = XtVaCreateManagedWidget("primitivechild",
                        staticTextWidgetClass,
                        p_table[C_DROPTARGET].entry,
                        XtNfont,        (XtArgVal)font[SIZE10],
                        XtNstring,      (XtArgVal)lc_help,
                        XtNx,           (XtArgVal)36,
                        XtNy,           (XtArgVal)65,
                        NULL);


	XtAddCallback(p_table[C_DROPTARGET].widget, XtNdndTriggerCallback, 
				DropTargetCB, NULL);
	XtAddCallback(p_table[C_DROPTARGET].widget, XtNownSelectionCallback,
				DropTargetCB, NULL);

	if (color) {
	    XtVaSetValues(p_table[C_DROPTARGET].widget,
				XtNbackground,	colors[BACKGROUND],
				NULL);
	    XtVaSetValues(childs[0], XtNbackground, colors[PRIMITIVE], NULL);
            XtVaSetValues(childs[1], XtNbackground, colors[PRIMITIVE], NULL);
	}

        (void)sprintf((char *)lc_messages,"%s",
                                dgettext(OlittableDomain,droptargethelp));
        lc_help = (char *)XtMalloc(strlen(lc_messages)+1);
        strcpy(lc_help,lc_messages);
        OlRegisterHelp(OL_WIDGET_HELP,
                                (XtPointer)p_table[C_DROPTARGET].widget,
                                "Example DropTarget",
                                OL_STRING_SOURCE,
                                lc_help);

	/************************************************************
	 * Create FooterPanel Example
	 ***********************************************************/
	n = 0;
	p_table[C_FOOTERPANEL].widget = XtCreateManagedWidget(
				"footer panel", footerPanelWidgetClass, 
				p_table[C_FOOTERPANEL].entry, arg, n);

	p_table[C_FOOTERPANEL].entry_widget = p_table[C_FOOTERPANEL].widget;
	n = 0;
	if (color) 
		XtSetArg(arg[n], XtNbackground, colors[BACKGROUND]);n++;
	XtSetArg(arg[n], XtNborderWidth, 1);n++;
	reset = n;
	XtSetArg(arg[n], XtNwidth,  95);n++;
	XtSetArg(arg[n], XtNheight, 40);n++;
	childs[1]= XtCreateManagedWidget("constraintchild", stubWidgetClass,
				 p_table[C_FOOTERPANEL].widget, arg, n);
	n = reset;
	(void)sprintf((char *)lc_messages,
				dgettext(OlittableDomain,"error: messages"));
	lc_help = (char *)XtMalloc(strlen(lc_messages)+1);
	strcpy(lc_help,lc_messages);
	XtSetArg(arg[n], XtNstring, lc_help);n++;
	XtSetArg(arg[n], XtNfont, font[SIZE10]);n++;
	childs[0]= XtCreateManagedWidget(
				"constraintchild", staticTextWidgetClass,
				p_table[C_FOOTERPANEL].widget, arg, n);

	(void)sprintf((char *)lc_messages,
				dgettext(OlittableDomain,footerphelp));
	lc_help = (char *)XtMalloc(strlen(lc_messages)+1);
	strcpy(lc_help,lc_messages);
	OlRegisterHelp(OL_WIDGET_HELP, 
				(XtPointer)(p_table[C_FOOTERPANEL].widget), 
				"Example FooterPanel", OL_STRING_SOURCE, 
				lc_help);


	/*************************************************************
 	 * Create DrawArea Example
	 *************************************************************/

	(void)sprintf((char *)lc_messages,
				dgettext(OlittableDomain,"PopUp DrawArea.."));
	lc_help = (char *)XtMalloc(strlen(lc_messages)+1);
	strcpy(lc_help,lc_messages);
	button = XtVaCreateManagedWidget("shellbutton", 
					oblongButtonWidgetClass,
					p_table[C_DRAWAREA].entry,
					XtNlabel,(XtArgVal)lc_help,
					NULL);

	p_table[C_DRAWAREA].entry_widget = button;

	shell = XtVaCreatePopupShell("Stretch Me....",
					transientShellWidgetClass,
					button,
					XtNwinType,	(XtArgVal)OL_WT_CMD,
					XtNresizeCorners, (XtArgVal)True,
					XtNpushpin, 	(XtArgVal)OL_OUT,
					XtNx,	(XtArgVal)(2 * entry_width) + 3,
					XtNy, 	(XtArgVal)entry_height + 3,
					NULL);

	p_table[C_DRAWAREA].widget = XtVaCreateManagedWidget(
					"constraintexample",
					drawAreaWidgetClass,
					shell,
					XtNlayout,	(XtArgVal)OL_IGNORE,
					XtNheight,	220,
					XtNwidth,	300,
					NULL);

	XtAddCallback(p_table[C_DRAWAREA].widget, XtNexposeCallback, 
			(XtCallbackProc)daExposeCB, (XtPointer)NULL);
	XtAddCallback(p_table[C_DRAWAREA].widget, XtNresizeCallback, 
			(XtCallbackProc)daResizeCB, (XtPointer)NULL);


	(void)sprintf((char *)lc_messages,
		dgettext(OlittableDomain,"This DrawArea contains a Box..."));
	lc_help = (char *)XtMalloc(strlen(lc_messages)+1);
	strcpy(lc_help,lc_messages);
	onlychild = XtVaCreateManagedWidget("constchild",
			staticTextWidgetClass,
			p_table[C_DRAWAREA].widget,
			XtNstring, (XtArgVal)lc_help,
			XtNx,		8,
			XtNy,		8,
			NULL);

	if (color){
		XtVaSetValues(button,
				XtNbackground, (XtArgVal)colors[CONSTRAINT],
				NULL);
		XtVaSetValues(p_table[C_DRAWAREA].widget,
				XtNbackground, (XtArgVal)colors[BACKGROUND],
				NULL);
		XtVaSetValues(onlychild,
				XtNbackground, (XtArgVal)colors[BACKGROUND],
				NULL);
	}

	/* 
   	 * Add callback to popup button now that we have popupshell widget ID. 
	*/
	XtAddCallback(button, XtNselect, popupCB1, shell);

	(void)sprintf((char *)lc_messages,
				dgettext(OlittableDomain,drawareahelp));
	lc_help = (char *)XtMalloc(strlen(lc_messages)+1);
	strcpy(lc_help,lc_messages);
	OlRegisterHelp(OL_WIDGET_HELP, (XtPointer)button, 
				"Example DrawArea", 
				OL_STRING_SOURCE, 
				lc_help);

	/************************************************************
	 * Create FontChooser Example
	 ***********************************************************/

	(void)sprintf((char *)lc_messages,
			dgettext(OlittableDomain,"PopUp FontChooser.."));
	lc_help = (char *)XtMalloc(strlen(lc_messages)+1);
	strcpy(lc_help,lc_messages);

	button = XtVaCreateManagedWidget("shellbutton", 
					oblongButtonWidgetClass,
					p_table[C_FONTCHOOSER].entry,
					XtNlabel, (XtArgVal)lc_help,
					NULL);
	p_table[C_FONTCHOOSER].entry_widget = button;

	p_table[C_FONTCHOOSER].widget = XtVaCreatePopupShell("shellexample", 
		     			fontChooserShellWidgetClass, 
					button,
					XtNx, (XtArgVal)(3 * entry_width) + 3,
					XtNy, (XtArgVal)entry_height + 3,
					XtNemanateWidget, (XtArgVal)button,
					XtNinitialFontName,(XtArgVal)SIZE14FONT,
					NULL);

        if (color) {
                XtVaSetValues(button,
                                XtNbackground, (XtArgVal)colors[CONSTRAINT],
                                NULL);
                XtVaSetValues(p_table[C_FONTCHOOSER].widget,
                                XtNbackground, (XtArgVal)colors[BACKGROUND],
                                NULL);
	}

	/* 
   	 * Add callback to popup button now that we have noticeshell widget ID. 
	*/
	XtAddCallback(button, XtNselect, popupCB1, 
						p_table[C_FONTCHOOSER].widget);
	(void)sprintf((char *)lc_messages,"%s",
				dgettext(OlittableDomain,fontchooserhelp));
	lc_help = (char *)XtMalloc(strlen(lc_messages)+1);
	strcpy(lc_help,lc_messages);
        OlRegisterHelp(OL_WIDGET_HELP,
				(XtPointer)button,
                                "Example FontChooser",
                                OL_STRING_SOURCE,
                                lc_help);


	/************************************************************
	 * Create FileChooser Example
	 ***********************************************************/

	(void)sprintf((char *)lc_messages,
			dgettext(OlittableDomain,"FileChooser"));
	lc_help = (char *)XtMalloc(strlen(lc_messages)+1);
	strcpy(lc_help,lc_messages);
	button = XtVaCreateManagedWidget("shellbutton", 
					menuButtonWidgetClass,
					p_table[C_FILECHOOSER].entry,
					XtNlabel, (XtArgVal)lc_help,
					NULL);

	p_table[C_FILECHOOSER].entry_widget = button;

        XtVaGetValues(button, XtNmenuPane, &menupane, NULL);
 
        /*
         * Open
         */
        childs[0] = XtVaCreateManagedWidget("open_ob", 
					oblongButtonWidgetClass,
					menupane,
                        		XtNlabel,       "Open...",
                			NULL);

        p_table[C_FILECHOOSER].widget =XtVaCreatePopupShell("constraintexample",
                                        fileChooserShellWidgetClass,
                                        childs[0],
                                        NULL, NULL);

	XtAddCallback(childs[0], XtNselect, popupCB1, 
				(XtPointer) p_table[C_FILECHOOSER].widget);
 
        /*
         * Save
         */
        childs[1] = XtVaCreateManagedWidget("save_ob", 
					oblongButtonWidgetClass,
					menupane,
                        		XtNlabel,       "Save...",
                			NULL);

	grandchilds[1]= XtVaCreatePopupShell("constraintexample", 
                                        fileChooserShellWidgetClass, 
                                        childs[1], 
					XtNoperation,	(XtArgVal)OL_SAVE,
                                        NULL);

	XtAddCallback(childs[1], XtNselect, popupCB1, (XtPointer)grandchilds[1]);

        /*
         * Save As
         */
        childs[2] = XtVaCreateManagedWidget("save_as_ob",
					oblongButtonWidgetClass,
					menupane,
					XtNlabel,       "Save As...",
					NULL);

        grandchilds[2] = XtVaCreatePopupShell("constraintexample",  
                                        fileChooserShellWidgetClass,  
                                        childs[2],  
                                        XtNoperation,   (XtArgVal)OL_SAVE_AS,
                                        NULL);

	XtAddCallback(childs[2], XtNselect, popupCB1, (XtPointer)grandchilds[2]);

        if (color) {
                XtVaSetValues(button,
                                XtNbackground, (XtArgVal)colors[CONSTRAINT],
                                NULL);
		XtVaSetValues(p_table[C_FILECHOOSER].widget,
				XtNbackground,	(XtArgVal)colors[BACKGROUND],
				NULL);
		XtVaSetValues(grandchilds[1],
                                XtNbackground,  (XtArgVal)colors[BACKGROUND], 
                                NULL); 
                XtVaSetValues(grandchilds[2], 
                                XtNbackground,  (XtArgVal)colors[BACKGROUND],  
                                NULL);  
	}
	/* 
   	 * Add callback to popup button now that we have noticeshell widget ID. 
	*/

	(void)sprintf((char *)lc_messages,"%s",
				dgettext(OlittableDomain,filechooserhelp));
	lc_help = (char *)XtMalloc(strlen(lc_messages)+1);
	strcpy(lc_help,lc_messages);
	OlRegisterHelp(OL_WIDGET_HELP,
                                (XtPointer)button,
                                "Example FileChooser",
                                OL_STRING_SOURCE,
                                lc_help);


	/************************************************************
	 * Create TextLine Example
	 ***********************************************************/

	widget_wth = 106;
	widget_hgt = 16;

	(void)sprintf((char *)lc_messages,
			dgettext(OlittableDomain,"TextLine:"));
	lc_label = (char *)XtMalloc(strlen(lc_messages)+1);
	strcpy(lc_label,lc_messages);
	(void)sprintf((char *)lc_messages,
			dgettext(OlittableDomain,"  any text..."));
	lc_help = (char *)XtMalloc(strlen(lc_messages)+1);
	strcpy(lc_help,lc_messages);

	p_table[C_TEXTLINE].widget = XtVaCreateManagedWidget("primitiveexample",
				textLineWidgetClass, 
				p_table[C_TEXTLINE].entry, 
				XtNborderWidth, (XtArgVal) 1,
				XtNstring, (OlStr) lc_help,
				XtNcaptionLabel, (OlStr) lc_label,
				XtNcharsVisible, (XtArgVal) 6,	
				NULL);

	p_table[C_TEXTLINE].entry_widget =  p_table[C_TEXTLINE].widget;

	if (color) 
		XtVaSetValues(p_table[C_TEXTLINE].widget, 
				XtNbackground, (Pixel)colors[BACKGROUND],
				NULL);

	(void)sprintf((char *)lc_messages,"%s",
				dgettext(OlittableDomain,textlinehelp));
	lc_help = (char *)XtMalloc(strlen(lc_messages)+1);
	strcpy(lc_help,lc_messages);
	OlRegisterHelp(OL_WIDGET_HELP, 
				(XtPointer)(p_table[C_TEXTLINE].widget), 
				"Example TextLine", 
				OL_STRING_SOURCE, 
				lc_help);

	/************************************************************
	 * Create NumericField Example
	 ***********************************************************/

	widget_hgt = 16;

	(void)sprintf((char *)lc_messages,
			dgettext(OlittableDomain,"Data:"));
	lc_label = (char *)XtMalloc(strlen(lc_messages)+1);
	strcpy(lc_label,lc_messages);
	(void)sprintf((char *)lc_messages,
			dgettext(OlittableDomain,"12345"));
	lc_help = (char *)XtMalloc(strlen(lc_messages)+1);
	strcpy(lc_help,lc_messages);

	p_table[C_NUMERICFIELD].widget = XtVaCreateManagedWidget(
				"primitiveexample",
				numericFieldWidgetClass, 
				p_table[C_NUMERICFIELD].entry, 
				XtNborderWidth, (XtArgVal) 1,
				XtNstring, (OlStr) lc_help,
				XtNcaptionLabel, (OlStr) lc_label,
				XtNcharsVisible, (XtArgVal)4,	
				NULL);
	p_table[C_NUMERICFIELD].entry_widget = p_table[C_NUMERICFIELD].widget;

	if (color) 
		XtVaSetValues(p_table[C_NUMERICFIELD].widget, 
				XtNbackground, (Pixel)colors[BACKGROUND],
				NULL);

	(void)sprintf((char *)lc_messages,"%s",
				dgettext(OlittableDomain,numericfieldhelp));
	lc_help = (char *)XtMalloc(strlen(lc_messages)+1);
	strcpy(lc_help,lc_messages);
	OlRegisterHelp(OL_WIDGET_HELP, 
				(XtPointer)(p_table[C_NUMERICFIELD].widget), 
				"Example NumericField", 
				OL_STRING_SOURCE, 
				lc_help);
} /* CreateExampleWidgets */

void
PositionExampleWidgets()
{
	Dimension avail_height = entry_height - button_height;
	int i;

	for (i=0; i < NUMENTRIES; i++) {
	    if (p_table[i].entry_widget != NULL) {
	    	Dimension w_width, w_height;
		Position x, y;

        	XtVaGetValues(p_table[i].entry_widget,
                        XtNwidth,       &w_width,
                        XtNheight,      &w_height,
                        NULL);
		
		x = (Position)(entry_width-w_width)/2;
		y = (Position)(avail_height-w_height)/2 + 1;

		/* Make sure the Element Symbol is not obscured */
		if ((x < 25) && (y < 20)) {
		    if (x > y)
			x = 28;
		    else if (y > x)
			y = 20;
		    else {
			x = 28; y=20;
		    }
		}
        	XtVaSetValues(p_table[i].entry_widget,
                        XtNx,   (XtArgVal)x > 0? x : 0,
                        XtNy,   (XtArgVal)y > 0? y : 1,
                        NULL);
	    }
	}
} /* PositionExampleWidgets */


/**********************************************************************
 * DrawOnStub: routine to handle expose events on the Stub widgets
 * (both in the Stub table entry and the ScrolledWindow table entry).
 * This basically copies a bitmap to the STub's window.
 **********************************************************************/
void
DrawOnStub(Widget widget, XEvent *xevent, Region region)
{
	Display *display;
	static GC gc;
	XGCValues gc_values;
	Pixel bg_color, fg_color;
	Window paint_win;
	static Pixmap canvas_pixmap;

	display = XtDisplay(widget);
	paint_win = XtWindow(widget);
	if (need_stub_gc) {
		if (color){
			fg_color = colors[PRIMITIVE];
			bg_color = colors[BACKGROUND];
		}
		else {
			fg_color = WhitePixel(display, DefaultScreen(display));
			bg_color = BlackPixel(display, DefaultScreen(display));
		}
		gc_values.foreground = fg_color;
		gc_values.background = bg_color; 
		gc = XCreateGC(display, DefaultRootWindow(display),
			 GCForeground | GCBackground, &gc_values);
		canvas_pixmap = XCreatePixmapFromBitmapData(display, 
			DefaultRootWindow(display),
			(char *)canvas_bits, 64, 64, fg_color, bg_color,
			DefaultDepth(display,DefaultScreen(display)));

		need_stub_gc = FALSE;
	}

	XCopyArea(display, canvas_pixmap, paint_win, gc, 0, 0, 64, 64, 5, 5);

}

/*
 * Resize Callback for DrawArea
 */
static void
daResizeCB(Widget widget, caddr_t closure, OlDrawAreaCallbackStruct *calldata)
{

	(void)sprintf((char *)lc_messages, dgettext(OlittableDomain,
			"olittable: DrawArea was resized to %d by %d\n"));
	lc_help = (char *)XtMalloc(strlen(lc_messages)+1);
	strcpy(lc_help,lc_messages);
        printf((char *)lc_help, calldata->width, calldata->height);
}

/*
 * Expose Callback for DrawArea
 */
static void
daExposeCB(Widget widget, caddr_t closure, OlDrawAreaCallbackStruct *calldata)
{
	static Boolean gotGC = False;
	static GC gc[2];
	Window win = XtWindow(widget);
	Display *display = XtDisplay(widget);

	/*
	 * Draw in Window
	 */
	if (!gotGC) {
		XtGCMask mask;
		XGCValues values;
		/*
	 	 * Get GC
		 */
		mask = (XtGCMask)(GCForeground | GCLineWidth);
		
		values.foreground =
	color? colors[CONSTRAINT] : BlackPixel(display, DefaultScreen(display));
		values.line_width = 6;
		gc[0] = XtGetGC(widget,mask,&values);
		values.foreground = 
	color?  colors[SHELL]:  BlackPixel(display, DefaultScreen(display));
		gc[1] = XtGetGC(widget,mask,&values);

		gotGC = True;
	}
	/*
	 * Draw...
	 */
	XClearArea(display,win, 0, 0, calldata->width, calldata->height, False);
	XFillRectangle(display,win,gc[1],30,30,
			 (calldata->width)-60, (calldata->height)-60 );
	XDrawRectangle(display,win,gc[0],30,30,
			 (calldata->width)-60, (calldata->height)-60 );

}

