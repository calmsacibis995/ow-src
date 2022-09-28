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

/****************************************************************************
 *
 * Copyright (c) 1991 by Sun Microsystems, Inc.
 *  
 * table.main.c  : main module for olit_table program.  This file contains 
 *		   ALL routines associated with drawing and dealing with the 
 *		   main window and the overall table construction.
 *
 *****************************************************************************/

#pragma ident  "@(#)table.main.c	1.5 91/10/10 bin/olittable SMI"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <widec.h>
#include "table.h"
#include "widgetdata.h"
#include "mainhelp.h"


#define TITLE_STRING	"OPEN LOOK Intrinsics Toolkit\nWIDGET SET\nRelease 3.4\nPeriodic Table"

OlFont font[NUMFONTS];

static Display	*display;
static int	 screen;
int depth, color;

static void GetColors(Widget w);
static void DrawMapping(Widget widget);
Pixel colors[NCOLORS];

static void	GetPixmaps(Display *display);
Pixmap pixmaps[4];
	/* array of pixmaps used for tiles when run on non-color displays */

#include "const.bitmap"
#include "prim.bitmap"
#include "shell.bitmap"
#include "flat.bitmap"

#include "icon.bitmap"

#include "map.bitmap"
#include "mapinvert.bitmap"
#include "open.bitmap"

#define T_NORMAL 0
#define INVERTED 1
#define OPEN	 2

TableEntry p_table[NUMENTRIES];
int	entry_height, entry_width, border_width, button_height;


static Pixmap map_pixmap, mapinvert_pixmap, open_pixmap;
static int mapbuttonstate = 0;

static Widget motifmap_shell;

static char * atomicnumtext[] = {" 1"," 2"," 3"," 4"," 5"," 6"," 7"," 8",
			  " 9","10","11","12","13","14","15","16",
			  "17","18","19","20","21","22","23","24",
			  "25","26","27","28","29","30","31","32","33","34" };

static char *info_notice_string = "Sorry -\nCould not locate the Information File for this object.\nTry setting the $TABLEINFO environment variable \nto point to the directory containing the *.txt files.";


	
static Widget title, legend, map;

static char *legendstring[] = {"Constraint Widgets", "Primitive Widgets", 
		"Shell Widgets", "Flat Widgets", "-G","Available in Gadget"};

static char *program_name;


static int need_legend_gc = TRUE;
static int need_entry_gc = TRUE;
static int need_map_gc = TRUE;

extern void CreateExampleWidgets(void);
extern void PositionExampleWidgets(void);
extern unsigned char *find_config_file(unsigned char *file_name,char *env_var,char *arg0);

static void widgetCB(Widget widget,XtPointer clientData, XtPointer callData);
static void popdownNoticeCB(Widget w,caddr_t clientData, caddr_t callData);
static void popdownCB(Widget w, caddr_t clientData, caddr_t callData);
static void doneCB(Widget widget, XtPointer clientData, XtPointer callData);
static void DrawMapping(Widget widget);
static void MapInvert(Widget w, XEvent *event);
static void MapActivate(Widget w,XEvent *event, String *params, Cardinal *num_params);

void BuildTableStructure(Widget parent,TableEntry *table_ptr,Dimension entry_width,Dimension entry_height, int border_width);
void InitializeFonts(Widget widget, OlFont *fontarray);
void CreateTableTitle(Widget parent, OlFont f);
void CreateTableLegend(Widget parent, OlFont f);
void CreateMotifMap(Widget parent);
void PositionTableTitle(Widget title);

char		lc_messages[2048];
char		*lc_help;
char		*lc_help2;
Widget		toplevel;

/********************************************************************
 * main: initialize Intrinsics, create toplevel shell, build
 * Periodic Table, enter event loop...
 ********************************************************************/
void main(int argc, char *argv[])
{
	Widget 		table_container;
	Pixmap 		iconpixmap;
	Arg    		arg[16];
	XtAppContext	app;
	int    		n, i;
	char		*dirname;
	char		*pwdpath;
	char		*localename;
	char		domainpath[126];
	char		mofile[126];
	int		fd;

	if((dirname = getenv("OPENWINHOME")) == NULL){
		(void)sprintf((char *)lc_messages,"%s",
			dgettext(OlittableDomain, "Please set OPENWINHOME"));
		lc_help = (char *)XtMalloc(strlen(lc_messages)+1);
		strcpy(lc_help,lc_messages);
		fprintf(stderr,"%s",lc_help);
		exit(1);
	}
	(void)sprintf(domainpath,"%s/lib/locale",dirname);
	if((localename = getenv("LANG")) != NULL){
		(void)sprintf(mofile, "%s/lib/locale/%s/LC_MESSAGES/%s.mo",
				dirname,localename,OlittableDomain);
	}
	else{
		(void)sprintf(mofile, "%s/lib/locale/C/LC_MESSAGES/%s.mo",
				dirname,OlittableDomain);
	}
	if( (fd = open(mofile,O_RDONLY)) != -1){
		close(fd);
		(void)bindtextdomain(OlittableDomain,domainpath);
	}
	else{
		pwdpath = getenv("PWD");
		(void)bindtextdomain(OlittableDomain,pwdpath);
	}


   /*****************************
    * init intrinsics 
    *****************************/

	
	lc_help = XtNewString(dgettext(OlittableDomain,
                "Please be patient...I'm creating A LOT of widgets..."));
	fprintf(stderr,"%s",lc_help);

	program_name = argv[0];

	OlToolkitInitialize(NULL);
	OlSetDefaultTextFormat(OL_MB_STR_REP);
	toplevel = XtAppInitialize(&app , "PeriodicTable", 
			(XrmOptionDescRec *)NULL, (Cardinal)0, (int *)&argc,
			 argv, (String *)NULL, (ArgList)NULL,0);
	XtSetArg(arg[0], XtNimStatusStyle, OL_IM_DISPLAYS_IN_CLIENT);
        XtSetValues(toplevel,arg,1);

	display = XtDisplay(toplevel);
	screen = DefaultScreen(display);

   /****************************************
    * Set up icon for toplevel window
    ****************************************/
	iconpixmap = XCreateBitmapFromData(display,XDefaultRootWindow(display),
		(char *)icon_bits, icon_width, icon_height);

	XtVaSetValues(toplevel, 
		XtNiconPixmap,	(XtArgVal)iconpixmap,
		XtNmaxWidth, (XtArgVal)(TABLE_ENTRY_WIDTH + 2) * 7 + 10,
		XtNmaxHeight,(XtArgVal)(TABLE_ENTRY_HEIGHT + 2) * 5 + 12,
		NULL);

  /**************************************************************************
   * Check for a Color Display, if so, Get desired colors from default colormap,
   * else get pixmaps to give variance to monochrome table. 
   ***************************************************************************/
	if ((DisplayPlanes(display, screen)) > 1)
		color = TRUE;
	else
		color = FALSE;

	if (argc > 1) /* OR if the USER SPECIFIES NO COLOR...*/
		if (!strcmp(argv[1], "-nocolor"))
			color = FALSE;

  	if (color)
		GetColors(toplevel);
	else
		GetPixmaps(display);

        /* Search for Desired Fonts */
        InitializeFonts(toplevel,font);

  /*****************************************************
   * Create Main Form widget to contain the table 
   *****************************************************/

	table_container =XtVaCreateManagedWidget("tablecontainer",
						bulletinBoardWidgetClass,
						toplevel,
						NULL);
	if (color)
		XtVaSetValues(table_container,
				XtNbackground,(XtArgVal)colors[BACKGROUND],
				NULL);


	(void)sprintf((char *)lc_messages,
                                dgettext(OlittableDomain,tablehelp));
        lc_help = (char *)XtMalloc(strlen(lc_messages)+1);
        strcpy(lc_help,lc_messages);

        (void)sprintf((char *)lc_messages,"%s", 
                                dgettext(OlittableDomain,tablewarning)); 
        lc_help2 = (char *)XtMalloc(strlen(lc_messages)+strlen(lc_help)+1); 
        (void)sprintf(lc_help2,"%s%s",lc_help,lc_messages); 
	XtFree(lc_help);

	OlRegisterHelp(OL_WIDGET_HELP, (XtPointer)table_container, 
					"OLIT Periodic Table", 
					OL_STRING_SOURCE, 
					lc_help2);


  /*********************************
   * Start building Table: 
   *********************************/
	entry_height = TABLE_ENTRY_HEIGHT;
	entry_width = TABLE_ENTRY_WIDTH;
	border_width = 1;
	button_height = 19;
	
	/* Create 1 Form Widget for each Periodical Table Entry
	 *(lay them out like a periodical table) 
	 */
	BuildTableStructure(table_container,
				p_table,
				entry_width,entry_height,
				border_width);

	/* Put up Table Title (at top) and Legend (at bottom) of Main FOrm */
	CreateTableTitle(table_container, font[SIZE14]);
	CreateTableLegend(table_container, font[SIZE14]);
	CreateMotifMap(table_container);

	/* Create Example of each widget in its corresponding entry */

	CreateExampleWidgets();

	(void)sprintf((char *)lc_messages,"%s",
				dgettext(OlittableDomain,"done.\n"));
	lc_help = (char *)XtMalloc(strlen(lc_messages)+1);
	strcpy(lc_help,lc_messages);
	fprintf(stderr,"%s",lc_help);

	  
   /***************************************************************************
    * realize the widgets, enter event loop... 
    **************************************************************************/
	XtRealizeWidget(toplevel);

	PositionExampleWidgets();
	PositionTableTitle(title);

	XtAppMainLoop(app);

}



/**************************************************
 * GetColors: routine to set up static 8-color
 * colormap.
 *************************************************/
static void
GetColors(Widget w)
{
	Pixel ConvertColornameToPixel();

	colors[CONSTRAINT] = ConvertColornameToPixel(w, CONSTRAINT_COLOR);
	colors[PRIMITIVE] = ConvertColornameToPixel(w, PRIMITIVE_COLOR);
	colors[FLAT] = ConvertColornameToPixel(w, FLAT_COLOR);
	colors[SHELL] = ConvertColornameToPixel(w, SHELL_COLOR);
	colors[OTHER] = ConvertColornameToPixel(w, "dark slate gray");
	colors[BACKGROUND] = ConvertColornameToPixel(w, TABLE_BG_COLOR);
	colors[SHELLBUTTON] = ConvertColornameToPixel(w, SHELL_BUTTON_COLOR);
	colors[CONSTCHILD] = ConvertColornameToPixel(w, CONST_CHILD_COLOR);


}


/*************************************************
 * GetPixmaps: routine to create the pixmaps used
 * with the black&white version of this program.
 * **********************************************/
static void
GetPixmaps(Display *display)
{	
	int screen = DefaultScreen(display);
	Pixel white, black;
	white = WhitePixel(display, screen);
	black = BlackPixel(display, screen);


	
	pixmaps[CONSTRAINT] = XCreatePixmapFromBitmapData(display,
					DefaultRootWindow(display), 
					(char *)const_bits, 8, 8, black, white, 
					DefaultDepth(display, screen));
	pixmaps[PRIMITIVE] = XCreatePixmapFromBitmapData(display, 
					DefaultRootWindow(display), 
					(char *)prim_bits, 8, 8, black, white, 
					DefaultDepth(display, screen));
	pixmaps[SHELL] = XCreatePixmapFromBitmapData(display, 
					DefaultRootWindow(display), 
					(char *)shell_bits, 8, 8, black,white, 
					DefaultDepth(display, screen));
	pixmaps[FLAT] = XCreatePixmapFromBitmapData(display, 
					DefaultRootWindow(display), 
					(char *)flat_bits, 8, 8, black, white, 
					DefaultDepth(display, screen));

}

void
CreateTableEntry(TableEntry *t_entry,int number, String type,Widget parent,Dimension wd,Dimension ht, int bd_width, int row,int col)
{
	Position xpos, ypos;
	void DrawEntry(Widget widget, XEvent *xevent, Region region);


	ypos = (ht + (2 * bd_width)) * row;
	xpos = (wd + (2 * bd_width)) * col;

	t_entry->entry = XtVaCreateManagedWidget(type,
					bulletinBoardWidgetClass,
					parent,
					XtNx,		(XtArgVal)xpos,
					XtNy,		(XtArgVal)ypos,
					XtNborderWidth,	(XtArgVal)bd_width,
					XtNwidth,	(XtArgVal)wd,
					XtNheight,	(XtArgVal)ht,
					XtNlayout,	OL_IGNORE,
					NULL);

	XtVaSetValues(t_entry->entry,
				XtNuserData,	(XtArgVal)number,
				NULL);

	XtAddEventHandler(t_entry->entry, ExposureMask, FALSE, 
				(XtEventHandler)DrawEntry, NULL);

	t_entry->info_button = XtVaCreateManagedWidget(
				widgetnametext[number][0],
				oblongButtonWidgetClass, t_entry->entry,
				XtNx,		(XtArgVal)0,
				XtNy,		(XtArgVal)ht-button_height,
				XtNfont,	(XtArgVal)font[LABEL],
				XtNlabelJustify,(XtArgVal)OL_CENTER,
				XtNrecomputeSize,(XtArgVal)FALSE,
				XtNwidth,	(XtArgVal)wd,
				XtNheight,	(XtArgVal)button_height,
				NULL);

	XtAddCallback(t_entry->info_button, XtNselect, widgetCB,
			 (XtPointer)number);

	if (!strcmp(type, "constraint"))
		{
		if (color)
			{

			XtVaSetValues(t_entry->entry,
				XtNbackground,	(XtArgVal)colors[CONSTRAINT],
				NULL);
			XtVaSetValues(t_entry->info_button,
				XtNbackground,	(XtArgVal)colors[CONSTRAINT],
				NULL);
			}
		else
			XtVaSetValues(t_entry->entry,
				XtNbackgroundPixmap,
				(XtArgVal)pixmaps[CONSTRAINT],
				NULL);
		}
	else if (!strcmp(type, "primitive"))
		{
		if (color)
			{
			XtVaSetValues(t_entry->entry,
				XtNbackground,	(XtArgVal)colors[PRIMITIVE],
					NULL);
			XtVaSetValues(t_entry->info_button,
				XtNbackground,	(XtArgVal)colors[PRIMITIVE],
					NULL);
			}
		else
			XtVaSetValues(t_entry->entry,
				XtNbackgroundPixmap,
				(XtArgVal)pixmaps[PRIMITIVE],
				NULL);

		}
	else if (!strcmp(type, "shell"))
		{
		if (color)
			{
			XtVaSetValues(t_entry->entry,
				XtNbackground,	(XtArgVal)colors[SHELL],
					NULL);
			XtVaSetValues(t_entry->info_button,
				XtNbackground,	(XtArgVal)colors[SHELL],
					NULL);
			}
		else
			XtVaSetValues(t_entry->entry,
				XtNbackgroundPixmap, (XtArgVal)pixmaps[SHELL],
				NULL);
		}
	else
		{
		if (color)
			{
			XtVaSetValues(t_entry->entry,
				XtNbackground,	(XtArgVal)colors[FLAT],
					NULL);
			XtVaSetValues(t_entry->info_button,
				XtNbackground,	(XtArgVal)colors[FLAT],
					NULL);
			}
		else
			XtVaSetValues(t_entry->entry,
				XtNbackgroundPixmap,	(XtArgVal)pixmaps[FLAT],
				NULL);

		}


	(void)sprintf((char *)lc_messages,"%s",
                                dgettext(OlittableDomain,buttonhelp));
        lc_help = (char *)XtMalloc(strlen(lc_messages)+1);
        strcpy(lc_help,lc_messages);
	OlRegisterHelp(OL_WIDGET_HELP, (XtPointer)(t_entry->info_button),
						widgetnametext[number][0],
				 		OL_STRING_SOURCE, 
						lc_help);
 
	(void)sprintf((char *)lc_messages,"%s",
                                dgettext(OlittableDomain,entryhelp));
        lc_help = (char *)XtMalloc(strlen(lc_messages)+1);
        strcpy(lc_help,lc_messages);
	OlRegisterHelp(OL_WIDGET_HELP, (XtPointer)(t_entry->entry), 
						"Table Entry",
				 		OL_STRING_SOURCE, 
						lc_help); 


}						

/****************************************
 ******************************************/
void
BuildTableStructure(Widget parent,TableEntry *table_ptr,Dimension entry_width,Dimension entry_height, int border_width)
{
	int i;
	

	/*************************************************
	 **************************************************/

	/*
	 * Build first 2 cells in first row
	 */
	for (i=0; i < 2; i++)
		 CreateTableEntry(&(table_ptr[i]), i,  widget_class_type[i],
			 parent, entry_width, entry_height, border_width, 0, i);
	/*
	 * Build last 2 cells in first row
	 */
	for (i=2; i < 4; i++)
		 CreateTableEntry(&(table_ptr[i]), i, widget_class_type[i],
			 parent,entry_width, entry_height, border_width, 0,i+3);
	/*
	 * Build 2nd row
	 */
	for (i=4; i < 11; i++)
		 CreateTableEntry(&(table_ptr[i]), i, widget_class_type[i],
			parent,entry_width, entry_height, border_width, 1, i-4);

	/*
	 * Build 3rd row
	 */
	for (i=11; i < 18; i++)
		 CreateTableEntry(&(table_ptr[i]), i, widget_class_type[i],
			parent,entry_width, entry_height, border_width, 2,i-11);
	/*
	 * Build 4th row
	 */
	for (i=18; i < 25; i++)
		 CreateTableEntry(&(table_ptr[i]), i, widget_class_type[i],
			parent,entry_width, entry_height, border_width, 3,i-18);
	/*
	 * Build first 3 cells in 5th row
	 */
	for (i=25; i < 28; i++)
		 CreateTableEntry(&(table_ptr[i]), i, widget_class_type[i],
			parent,entry_width, entry_height, border_width, 4,i-25);

	/*
	 * Build last 2 cells in 5th row
	 */
	for (i=28; i < 30; i++)
		CreateTableEntry(&(table_ptr[i]), i, widget_class_type[i],
			parent,entry_width, entry_height, border_width, 4,i-23);
	/*
	 * Build first 2 cells in 6th row
	 */
	for (i=30; i < 32; i++)
		 CreateTableEntry(&(table_ptr[i]), i, widget_class_type[i],
			parent,entry_width, entry_height, border_width, 5,i-30);

	/*
	 * Build last 2 cells in 6th row
	 */
	for (i=32; i < 34; i++)
		CreateTableEntry(&(table_ptr[i]), i, widget_class_type[i],
			parent,entry_width, entry_height, border_width, 5,i-27);
}

/*******************************************************
 * doneCB: routine used pop down the information
 * popup windows when they are pulled down with the
 * "Done" button.
 *******************************************************/
static void
doneCB(Widget widget, XtPointer clientData, XtPointer callData)
{
	Widget *shell = (Widget*)clientData;
	XtPopdown(*shell);
}


/*******************************************************
 * popdownCB: routine called when info windows are
 * popped down - this destroys the information popup.
 *******************************************************/
static void popdownCB(Widget w, caddr_t clientData, caddr_t callData)
{
    Widget *shell = (Widget*)clientData;

    XtDestroyWidget(w);
    *shell = NULL;

}


/*******************************************************
 * popdownNoticeCB: routine called when user clicks
 * "Okay" on the Notice popped up when the info file
 * could not be found.
 *******************************************************/
static void popdownNoticeCB(Widget w,caddr_t clientData, caddr_t callData)
{

	XtDestroyWidget(w);

}


/***********************************************************
 * PutUpInfoNotice: routine used to put up a Notice 
 * informing the user that the info file (*.txt) could
 * not be found, and they should set the $TABLEINFO env
 * variable.
 ***********************************************************/
void
PutUpInfoNotice(Widget parent)
{
	Widget info_notice, message_area, message_control, notice_button;

	(void)sprintf((char *)lc_messages,"%s",
			dgettext(OlittableDomain,info_notice_string));
	    lc_help = (char *)XtMalloc(strlen(lc_messages)+1);
	    strcpy(lc_help,lc_messages);

	info_notice = XtVaCreatePopupShell("infonotice",
				    noticeShellWidgetClass,
				    parent,
				    XtNemanateWidget,	(XtArgVal)parent,
				    XtNstring,(XtArgVal)lc_help,
				    XtNhPad,	(XtArgVal)90,
				    NULL);


	XtVaGetValues(info_notice,
			    XtNcontrolArea,	&message_control,
			    XtNtextArea,	&message_area,
			    NULL);

	XtVaSetValues(message_area,
			    XtNalignment,	(XtArgVal)OL_CENTER,
			    NULL);


	notice_button = XtVaCreateManagedWidget("Okay",
			    oblongButtonWidgetClass,
			    message_control,
			    NULL);


	if (color) {

	    XtVaSetValues(info_notice,
			    XtNbackground,	(XtArgVal)colors[BACKGROUND],
			    NULL);
	    XtVaSetValues(message_area,
			    XtNbackground,	(XtArgVal)colors[BACKGROUND],
			    NULL);
	    XtVaSetValues(message_control,
			    XtNbackground,	(XtArgVal)colors[BACKGROUND],
			    NULL);

	    XtVaSetValues(XtParent(message_control),
			    XtNbackground,	(XtArgVal)colors[BACKGROUND],
			    NULL);

	    XtVaSetValues(notice_button,
			    XtNbackground,	(XtArgVal)colors[BACKGROUND],
			    NULL);

	   }

	XtAddCallback(info_notice, XtNpopdownCallback,
				 (XtCallbackProc)popdownNoticeCB, NULL);

	XtPopup(info_notice, XtGrabExclusive);

}


/********************************************************
 * widgetCB: routine used to create and map the information
 * popup windows when the user presses the widget name button.
 * We first check to make sure the window is not already
 * currently mapped - if it is, then we just raise it to
 * the top.
 ***********************************************************/
static void
widgetCB(Widget widget,XtPointer clientData, XtPointer callData)
{
	Widget form, sw, infotext,
		uppercontrol, lowercontrol, done_button;
	int GetWidgetIndex();
	unsigned char info_string[36], info_filename[8];
	int index;
	char *absolutefile;

	index = (int)clientData;

	/*
	 * Check to see if this info window is already up -
	 * if so, raise it to the top.
	 */
	if (p_table[index].info_shell != NULL)
		XRaiseWindow((Display*)XtDisplay(p_table[index].info_shell),
			(Window)XtWindow(p_table[index].info_shell));
	else
		/*
	 	 * Time to create the info window...
	 	*/
		{
		/* figure out filename where information is...
		 */

		strcpy((char *)info_filename, widgetnametext[index][1]);
		strcat((char *)info_filename, ".txt");

		if (absolutefile = (char*)find_config_file(
			info_filename, "$TABLEINFO", program_name))
		   {

		    Widget hsb, vsb;				
		
		    strcpy((char *)info_string, widgetnametext[index][0]);
		    (void)sprintf((char *)lc_messages,"%s",
				dgettext(OlittableDomain," Widget Info"));
		    lc_help = (char *)XtMalloc(strlen(lc_messages)+1);
		    strcpy(lc_help,lc_messages);
		    strcat((char *)info_string, lc_help);
		    XtFree(lc_help);

		    p_table[index].info_shell = XtVaCreatePopupShell(
				(char *)info_string,		
				popupWindowShellWidgetClass, 
				widget, 
			XtNuserData,(XtArgVal)&p_table[index].info_shell,
				XtNresizeCorners,	(XtArgVal)False,
				NULL);

		    XtVaGetValues(p_table[index].info_shell,
					XtNupperControlArea, &uppercontrol,
					XtNlowerControlArea, &lowercontrol,
					NULL); 


		    form = XtVaCreateManagedWidget("form",
						formWidgetClass,
						uppercontrol,
						NULL);

		    sw = XtVaCreateManagedWidget("scrollwin",
						scrolledWindowWidgetClass,
						form,
						XtNxVaryOffset,	(XtArgVal)FALSE,
						XtNyVaryOffset, (XtArgVal)FALSE,
						XtNxAttachRight,(XtArgVal)TRUE,
						XtNyAttachBottom,(XtArgVal)TRUE,
						NULL);

		    XtVaGetValues(sw,
					XtNhScrollbar,	&hsb,
					XtNvScrollbar, &vsb,
					NULL);
	


		    infotext = XtVaCreateManagedWidget("widgetinfotext", 
				textEditWidgetClass, 
				sw,
				XtNsourceType, 	(XtArgVal) OL_DISK_SOURCE,
				XtNeditType, 	(XtArgVal) OL_TEXT_READ,
				XtNfont,	(XtArgVal) font[FIXED],
				XtNsource, 	(XtArgVal) absolutefile,
				XtNheight, 	(XtArgVal) 500,
				XtNwidth, 	(XtArgVal) 708,
				XtNwrapMode,	(XtArgVal)OL_WRAP_OFF,
				NULL);
	
		
		    done_button = XtVaCreateManagedWidget("Done",
						oblongButtonWidgetClass, 
						lowercontrol, 
						NULL);
	
		    if (color){
				XtVaSetValues(uppercontrol, 
				XtNbackground,	(XtArgVal)colors[BACKGROUND],
					NULL);

				XtVaSetValues(lowercontrol, 
					XtNhPad,	(XtArgVal)343,
				XtNbackground,	(XtArgVal)colors[BACKGROUND],
					NULL);
	
				XtVaSetValues(done_button, 
				XtNbackground,	(XtArgVal)colors[BACKGROUND],
					NULL);
	
				XtVaSetValues(form, 
				XtNbackground,	(XtArgVal)colors[BACKGROUND],
					NULL);
	
				XtVaSetValues(hsb, 
				XtNbackground,	(XtArgVal)colors[BACKGROUND],
					NULL);

				XtVaSetValues(vsb, 
				XtNbackground,	(XtArgVal)colors[BACKGROUND],
					NULL);
	
		    }
	
		    XtAddCallback(done_button, XtNselect, doneCB,
					(Widget*)&(p_table[index].info_shell));
	
		    XtAddCallback(p_table[index].info_shell, XtNpopdownCallback,
				 (XtCallbackProc)popdownCB, 
				(Widget*)&(p_table[index].info_shell));
	
		    XtPopup(p_table[index].info_shell, XtGrabNone);
	
	
		   }
		   else	
		      PutUpInfoNotice(widget);
		}

}




/****************************************************
 * DrawEntry: routine to handle expose events on
 * each Form entry widget in the table.  This routine
 * draws the atomic number and the widget symbol
 * characters write on to the Form's window.
 ****************************************************/
void
DrawEntry(Widget widget, XEvent *xevent, Region region)
{
	Display *display;
	Window paint_win;
	static GC gc;
	XGCValues gc_values;
	int index;
	OlStrRep	text_format;
	XrmValue	rmvalue;

	(void)_OlGetDefaultTextFormat(widget,NULL , &rmvalue);
	text_format = *(OlStrRep *)(rmvalue.addr);

	display = XtDisplay(widget);
	paint_win = XtWindow(widget);

	XtVaGetValues(widget, 
				XtNuserData,	&index,
				XtNtextFormat,	&text_format,
				NULL);

	if (need_entry_gc){
		gc_values.foreground = BlackPixel(display,
						 DefaultScreen(display));
		gc = XCreateGC(display, DefaultRootWindow(display), 
						GCForeground, &gc_values);						
		need_entry_gc = FALSE;
	}

	if(text_format == OL_MB_STR_REP){
		XmbDrawString(display, paint_win, (XFontSet)(font[SIZE14]),gc, 
			3, 18, widgetnametext[index][1],
			strlen(widgetnametext[index][1]));
		XmbDrawString(display, paint_win, (XFontSet)(font[SIZE12]), gc, 
			entry_width - 16, 14, 
			atomicnumtext[index], strlen(atomicnumtext[index]));
	}
	else if(text_format == OL_WC_STR_REP){
		wchar_t	*wcstring;

		wcstring = (wchar_t *)XtMalloc(
					strlen(widgetnametext[index][1])+1);
		mbstowcs(wcstring,widgetnametext[index][1], 
					strlen(widgetnametext[index][1]));
		XwcDrawString(display, paint_win, (XFontSet)(font[SIZE14]) ,gc,
			3, 18, (wchar_t *)wcstring, wslen(wcstring));
		XtFree((char *)wcstring);

		wcstring = (wchar_t *)XtMalloc(strlen(atomicnumtext[index])+1);
		mbstowcs(wcstring,atomicnumtext[index],
					strlen(atomicnumtext[index]));
		XwcDrawString(display, paint_win,(XFontSet)(font[SIZE12]), gc, 
			entry_width - 16, 14, wcstring,wslen(wcstring));
		XtFree((char *)wcstring);
	}
	else{
		XSetFont(display, gc, ((XFontStruct *)font[SIZE14])->fid);
		XDrawString(display, paint_win, gc, 3, 18, 
		widgetnametext[index][1], strlen(widgetnametext[index][1]));

		XSetFont(display, gc, ((XFontStruct *)font[SIZE12])->fid);
		XDrawString(display, paint_win, gc, entry_width - 16, 14, 
			atomicnumtext[index], strlen(atomicnumtext[index]));
	}

}
 

/****************************************************
 * CreateTableTitle: create the StaticText widget
 * used for the table title.
 ****************************************************/
void
CreateTableTitle(Widget parent, OlFont f)
{
	Pixel table_bg_color;

	lc_help = XtNewString(dgettext(OlittableDomain,TITLE_STRING));
	title = XtVaCreateManagedWidget("tabletitle", 
			staticTextWidgetClass, 
			parent,
			XtNalignment, 	(XtArgVal)OL_CENTER,
			XtNstring, 	(XtArgVal) lc_help,
			XtNfont, 	(XtArgVal)f,
			NULL);

	if (color)
		{
		XtVaGetValues(parent,
					XtNbackground,	&table_bg_color,
					NULL);
		XtVaSetValues(title,
					XtNbackground,	table_bg_color,
					NULL);
		}

	(void)sprintf((char *)lc_messages,"%s",
                                dgettext(OlittableDomain,tablehelp));
        lc_help = (char *)XtMalloc(strlen(lc_messages)+1);
        strcpy(lc_help,lc_messages);
	OlRegisterHelp(OL_WIDGET_HELP, (XtPointer)title, 
					"OLIT Periodic Table", 
					OL_STRING_SOURCE, 
					lc_help);



}


void
PositionTableTitle(Widget w)
{
    Dimension title_w, title_h;

    XtVaGetValues(w, 
			XtNwidth, &title_w,
			XtNheight, &title_h,
			NULL);

    XtVaSetValues(w,
			XtNx,	(XtArgVal)(7*entry_width - title_w)/2,
			XtNy,	(XtArgVal)15,
			NULL);
}


/***************************************************
 * MapInvert: routine called when user clicks
 * LEFT mouse button DOWN on the Motif Map icon
 * (to kind of 'act' like a button being activated..
 * but we have no way of doing a de-activate if hte
 * user lifts up outside of the icon area...oh well).
 ***************************************************/
static void
MapInvert(Widget w, XEvent *event)
{
	if (mapbuttonstate == INVERTED)
		mapbuttonstate = T_NORMAL;
	else 

		mapbuttonstate = INVERTED;

	DrawMapping(w);
}

void
timedDraw(Widget w, XtIntervalId id)
{
	DrawMapping(w);
}



/****************************************************
 * popdownMotifMapCB: routine called when the WM_DELETE_WINDOW
 * message is sent when user selects "dismiss" off
 * the Motif Map popup window's system menu.
 * This routine destroys the popup window.
 ****************************************************/
static void popdownMotifMapCB(Widget	w, caddr_t clientData,caddr_t callData)
    {
    Widget *shell = (Widget*)clientData;


    XtDestroyWidget(w);
    *shell = NULL;
    mapbuttonstate = T_NORMAL;

    /* This is a hoaky workaround for a problem where the server
     * is not delivering the VisibilityNotify events when this stub
     * becomes unobscured because the MotifMap help window was dismissed.
     * This pretty-much ensures the draw routine will get called
     * AFTER the MotifMap window has been pulled down and the Stub
     * is already visible.
     */
    XtAppAddTimeOut(XtWidgetToApplicationContext(w), 500,
        (XtTimerCallbackProc)timedDraw, map);    
    }


/*****************************************************
 * MapActivate: routine called when there's a LEFT
 * mouse button UP event on the Motif map icon.
 * It creates the popup window which contains the
 * MOtif to OPEN LOOK widget mapping text.
 * We first check to make sure the Motif Map Popup is
 * not already mapped - in which case we just bring it
 * to the top.
 ****************************************************/
static void
MapActivate(Widget w,XEvent *event, String *params, Cardinal *num_params)
{
	/*** Bring up Motif Window...***/
	Widget sw, text, topcontrol, lowercontrol, done_button ;
	char *absolutefile;

	if (motifmap_shell != NULL)
		XRaiseWindow((Display*)XtDisplay(motifmap_shell),
			(Window)XtWindow(motifmap_shell));
	else
		{
		if (absolutefile = (char *)find_config_file(
					(unsigned char *)"motifmap.txt",
						 "$TABLEINFO", program_name))
		    {
				
		    Widget hsb, vsb;
	
		    motifmap_shell = XtVaCreatePopupShell(
				"Motif to OPEN LOOK Widget Conversion Formulas",
				popupWindowShellWidgetClass, w, 
				XtNuserData,	(XtArgVal)&motifmap_shell,
				XtNresizeCorners,(XtArgVal)False,
				NULL);

		    XtVaGetValues(motifmap_shell, 
					XtNupperControlArea, &topcontrol,
					XtNlowerControlArea, &lowercontrol,
					NULL);


		    sw = XtVaCreateManagedWidget("scrollwin",
						scrolledWindowWidgetClass,
						topcontrol,
						NULL);

		    XtVaGetValues(sw,
						XtNhScrollbar,	&hsb,
						XtNvScrollbar,  &vsb,
						NULL);

	
		    text = XtVaCreateManagedWidget("motifmaptext", 
				textEditWidgetClass, 
				sw, 
				XtNimPreeditStyle, OL_ON_THE_SPOT,
				XtNsourceType,	(XtArgVal)OL_DISK_SOURCE,
				XtNeditType,	(XtArgVal)OL_TEXT_READ,
				XtNfont,	(XtArgVal)font[FIXED],
				XtNsource,	(XtArgVal)absolutefile,
				XtNheight,	(XtArgVal)500,
				XtNwidth,	(XtArgVal)680,
				NULL);
	
		    done_button = XtVaCreateManagedWidget("Done",
						oblongButtonWidgetClass, 
						lowercontrol, 
						NULL);

		    if (color){
			XtVaSetValues(topcontrol, 
				XtNbackground,	(XtArgVal)colors[BACKGROUND],
				NULL);
			XtVaSetValues(lowercontrol, 
				XtNbackground,	(XtArgVal)colors[BACKGROUND],
				XtNhPad,	(XtArgVal)329,
				NULL);

			XtVaSetValues(done_button, 
				XtNbackground,	(XtArgVal)colors[BACKGROUND],
				NULL);
			XtVaSetValues(hsb, 
				XtNbackground,	(XtArgVal)colors[BACKGROUND],
				NULL);
			XtVaSetValues(vsb, 
				XtNbackground,	(XtArgVal)colors[BACKGROUND],
				NULL);
		    }


		    XtAddCallback(done_button, XtNselect, doneCB, 
						(Widget*)&motifmap_shell);

    		    XtAddCallback(motifmap_shell, XtNpopdownCallback, 
				(XtCallbackProc)popdownMotifMapCB, 
					(Widget*)&motifmap_shell);
	
		    XtPopup(motifmap_shell, XtGrabNone);

		    mapbuttonstate = OPEN;


		   }
		else
		   {
		    PutUpInfoNotice(w);
		    mapbuttonstate = T_NORMAL;
		   }
		}

	DrawMapping(w); 
}


/*********************************************
 * CreateMotifMap: routine to create the
 * icon 'button' which can be pressed to bring
 * up a popup window containing information
 * on mapping Motif widgets to OPEN LOOK widgets.
 ************************************************/
void
CreateMotifMap(Widget parent)
{
	static GC mapgc;
	XGCValues gc_values;
	Pixel bg_color, fg_color;



	if (color)
		XtVaGetValues(parent,
					XtNbackground,	&bg_color,
					NULL);		
	else 
		bg_color = WhitePixel(display, DefaultScreen(display));
	
	fg_color = BlackPixel(display, DefaultScreen(display));

	gc_values.foreground = fg_color;
	mapgc = XCreateGC(display, DefaultRootWindow(display), GCForeground,
			 &gc_values);

	/* Create Pixmaps for Motif Mapping button */

	map_pixmap = XCreatePixmapFromBitmapData(display, 
			DefaultRootWindow(display), 
		(char *)map_bits, map_width, map_height, fg_color, bg_color,
			   DefaultDepth(display,DefaultScreen(display)));

	mapinvert_pixmap = XCreatePixmapFromBitmapData(display, 
			DefaultRootWindow(display), 
		(char *)mapinvert_bits, mapinvert_width, mapinvert_height, 
				fg_color, bg_color,
			   DefaultDepth(display,DefaultScreen(display)));

	open_pixmap = XCreatePixmapFromBitmapData(display, 
			DefaultRootWindow(display), 
		(char *)open_bits, open_width, open_height, fg_color, bg_color,
			   DefaultDepth(display,DefaultScreen(display)));



	map = XtVaCreateManagedWidget("mapping", 
			stubWidgetClass, 
			parent,
			XtNx,(XtArgVal)4*entry_width + (8*border_width) + 40,
			XtNy,(XtArgVal)4*entry_height+ (8*border_width) + 16,
			XtNwidth,  	(XtArgVal)72,
			XtNheight,	(XtArgVal)64,
			XtNexpose,	(XtArgVal)DrawMapping,
			XtNuserData,	(XtArgVal)mapgc,
			NULL); 

	if (color)
		XtVaSetValues(map, XtNbackground,	(XtArgVal)bg_color,	
					NULL);


	XtAddEventHandler(map, ButtonPressMask,
			   FALSE, (XtEventHandler)MapInvert, mapgc);
	XtAddEventHandler(map, ButtonReleaseMask,
			 FALSE, (XtEventHandler)MapActivate, NULL);

	(void)sprintf((char *)lc_messages,"%s",
                                dgettext(OlittableDomain,maphelp));
        lc_help = (char *)XtMalloc(strlen(lc_messages)+1);
        strcpy(lc_help,lc_messages);
	OlRegisterHelp(OL_WIDGET_HELP, (XtPointer)map, 
					"Motif to OPEN LOOK Mapping", 
					OL_STRING_SOURCE, lc_help);
}

/**************************************************
 * DrawMapping: routine which handles expose events
 * on the map 'button' (really a Stub).  It just
 * copies the appropriate bitmap of the map icon to the 
 * Stub's window.
 **************************************************/
static void
DrawMapping(Widget widget)
{
	Display *display;
	Window paint_win;
	GC gc;
	Arg arg;
	XtSetArg(arg, XtNuserData, &gc);
	XtGetValues(widget, &arg, 1);

	display = XtDisplay(widget);
	paint_win = XtWindow(widget);

	switch(mapbuttonstate){
		case T_NORMAL:{
			XCopyArea(display, map_pixmap, paint_win, 
						gc, 0, 0, 72, 64, 0, 0);
			break;
			}
		case INVERTED:{
			XCopyArea(display, mapinvert_pixmap, paint_win, 
						gc, 0, 0, 64, 64, 0, 0);
			break;
			}
		case OPEN:{
			XCopyArea(display, open_pixmap, paint_win, 
						gc, 0, 0, 72, 64, 0, 0);
			break;
			}
		default:{

			break;
			}
		}
	XFlush(display);


}


/**********************************************
 * CreateTableLegend: routine to create the
 * table legend.  We use a STub widget and we
 * just draw the legend information on expose
 * events.
 **********************************************/
void
CreateTableLegend(Widget parent, OlFont f)
{
	void DrawLegend(Widget widget);
	Pixel bg_color;
	char	*lc_legendhelp;

	legend = XtVaCreateManagedWidget("legend", 
			stubWidgetClass, 
			parent,
			XtNx,   (XtArgVal)3*entry_width+(6*border_width),
			XtNy,	(XtArgVal)4*entry_height+(8*border_width)+6, 
			XtNwidth,  	(XtArgVal)entry_width,
			XtNheight, 	(XtArgVal)entry_height,
			XtNexpose,	(XtArgVal)DrawLegend,
			XtNborderWidth,	(XtArgVal)0,
			NULL);

	if (color) {
		XtVaGetValues(parent,
					XtNbackground,	&bg_color,
					NULL);
		XtVaSetValues(legend,
					XtNbackground,	bg_color,
					NULL);
	}

	(void)sprintf((char *)lc_messages,"%s",
                                dgettext(OlittableDomain,legendhelp));
        lc_help = (char *)XtMalloc(strlen(lc_messages)+1);
        strcpy(lc_help,lc_messages);

	(void)sprintf((char *)lc_messages,"%s",
                                dgettext(OlittableDomain,legendhelp2));
        lc_legendhelp = (char *)XtMalloc(strlen(lc_messages)+strlen(lc_help)+1);
        (void)sprintf(lc_legendhelp,"%s%s",lc_help,lc_messages);
	XtFree(lc_help);

	(void)sprintf((char *)lc_messages,"%s",
                                dgettext(OlittableDomain,legendhelp3));
        lc_help2 = (char *)XtMalloc(strlen(lc_messages)+strlen(lc_legendhelp)+1);
        (void)sprintf(lc_help2,"%s%s",lc_legendhelp,lc_messages);
	XtFree(lc_legendhelp);

	OlRegisterHelp(OL_WIDGET_HELP, (XtPointer)legend, 
			"Legend", OL_STRING_SOURCE, lc_help2);

}

/********************************************************
 * DrawLegend: routine to handle expose events on the
 * legend Stub widget.
 ********************************************************/
void
DrawLegend(Widget widget)
{
	Display *display;
	static GC gc;
	XGCValues gc_values;
	Window paint_win;
	int hgt, font_hgt, chip_hgt;
	XFontSetExtents *fset_extents;
	OlStrRep	text_format;
	XrmValue	rmvalue;

	_OlGetDefaultTextFormat(widget,NULL , &rmvalue);
	text_format = *(OlStrRep *)(rmvalue.addr);

	display = XtDisplay(widget);
	paint_win = XtWindow(widget);

	XtVaGetValues(widget, XtNtextFormat,	&text_format, NULL);

	if(text_format == OL_SB_STR_REP){
		font_hgt = ((XFontStruct *)font[SIZE12])->max_bounds.ascent 
			+ ((XFontStruct *)font[SIZE14])->max_bounds.descent;
		chip_hgt = ((XFontStruct *)font[SIZE12])->max_bounds.ascent + 2;
	}
	else{
		fset_extents = XExtentsOfFontSet((XFontSet)font[SIZE12]);
		font_hgt = fset_extents->max_logical_extent.height+2;
		chip_hgt = fset_extents->max_logical_extent.height+2;
	}


	if (need_legend_gc){	
		if (color){
			gc_values.foreground = colors[CONSTRAINT];
			gc =  XCreateGC(display, DefaultRootWindow(display), 
				GCForeground, &gc_values);
		}
		else{
			gc_values.foreground = BlackPixel(display, 
						DefaultScreen(display));
			gc_values.fill_style = FillTiled;
			gc = XCreateGC(display, DefaultRootWindow(display), 
				GCForeground | GCFillStyle,
				 &gc_values);
		}
		need_legend_gc = FALSE;
	}

	if (color)
		XSetForeground(display, gc, colors[CONSTRAINT]);
	else
		XSetTile(display, gc, pixmaps[CONSTRAINT]);
	XFillRectangle(display, paint_win, gc, 8,
			   font_hgt-chip_hgt+2, chip_hgt, chip_hgt);

	if (color)
		XSetForeground(display, gc, colors[PRIMITIVE]);
	else
		XSetTile(display, gc, pixmaps[PRIMITIVE]);
	XFillRectangle(display, paint_win, gc, 8, 
				2*font_hgt-chip_hgt+2, chip_hgt, chip_hgt);

	if (color)
		XSetForeground(display, gc, colors[SHELL]);
	else
		XSetTile(display, gc, pixmaps[SHELL]);
	XFillRectangle(display, paint_win, gc, 8, 
			3*font_hgt-chip_hgt+2, chip_hgt, chip_hgt);

	if (color)	
		XSetForeground(display, gc, colors[FLAT]);
	else
		XSetTile(display, gc, pixmaps[FLAT]);
	XFillRectangle(display, paint_win, gc, 8, 
			4*font_hgt-chip_hgt+2, chip_hgt, chip_hgt);


	if (color)
		XSetForeground(display, gc, 
				BlackPixel(display, DefaultScreen(display)));
	else
		XSetFillStyle(display, gc, FillSolid);

	/* If monochrome, draw borders around legend glyphs */
	if (!color)
		{
		XDrawRectangle(display, paint_win, gc, 7 ,
			     font_hgt-chip_hgt+1, chip_hgt+1, chip_hgt+1);
		XDrawRectangle(display, paint_win, gc, 7,
			(2*font_hgt)-chip_hgt+1, chip_hgt+1, chip_hgt+1);
		XDrawRectangle(display, paint_win, gc, 7,
			 (3*font_hgt)-chip_hgt+1, chip_hgt+1, chip_hgt+1);
		XDrawRectangle(display, paint_win, gc, 7,
			 (4*font_hgt)-chip_hgt+1, chip_hgt+1, chip_hgt+1);
		}
	
	if(text_format == OL_MB_STR_REP){
		XmbDrawString(display, paint_win, (XFontSet)font[SIZE12],
			gc, 8,  5*font_hgt, legendstring[GADGETSYM], 
			strlen(legendstring[GADGETSYM]));
		XmbDrawString(display, paint_win, (XFontSet)font[SIZE12],
			gc, 28,   font_hgt, legendstring[CONSTRAINT], 
			strlen(legendstring[CONSTRAINT]));
		XmbDrawString(display, paint_win, (XFontSet)font[SIZE12],
			gc, 28, 2*font_hgt, legendstring[PRIMITIVE], 
			strlen(legendstring[PRIMITIVE]));
		XmbDrawString(display, paint_win, (XFontSet)font[SIZE12],
			gc, 28, 3*font_hgt, legendstring[SHELL], 
			strlen(legendstring[SHELL]));
		XmbDrawString(display, paint_win, (XFontSet)font[SIZE12],
			gc, 28, 4*font_hgt, legendstring[FLAT], 
			strlen(legendstring[FLAT]));
		XmbDrawString(display, paint_win, (XFontSet)font[SIZE12],
			gc, 28, 5*font_hgt, legendstring[GADGET], 
			strlen(legendstring[GADGET]));
	}
	else if(text_format == OL_WC_STR_REP){
		wchar_t	*wcstring;

		wcstring = (wchar_t *)XtMalloc(
					strlen(legendstring[GADGETSYM])+1);
		mbstowcs(wcstring,legendstring[GADGETSYM],
					strlen(legendstring[GADGETSYM]));
		XwcDrawString(display, paint_win, (XFontSet)font[SIZE12],
			gc, 8,  5*font_hgt, wcstring, wslen(wcstring));
		XtFree((char *)wcstring);
		wcstring = (wchar_t *)XtMalloc(
					strlen(legendstring[CONSTRAINT])+1);
		mbstowcs(wcstring,legendstring[CONSTRAINT],
					strlen(legendstring[CONSTRAINT]));
		XwcDrawString(display, paint_win, (XFontSet)font[SIZE12],
			gc, 28,   font_hgt, wcstring, wslen(wcstring));
		XtFree((char *)wcstring);
		wcstring = (wchar_t *)XtMalloc(
					strlen(legendstring[PRIMITIVE])+1);
		mbstowcs(wcstring,legendstring[PRIMITIVE],
					strlen(legendstring[PRIMITIVE]));
		XwcDrawString(display, paint_win, (XFontSet)font[SIZE12],
			gc, 28, 2*font_hgt, wcstring, wslen(wcstring));
		XtFree((char *)wcstring);
		wcstring = (wchar_t *)XtMalloc(strlen(legendstring[SHELL])+1);
		mbstowcs(wcstring,legendstring[SHELL],
					strlen(legendstring[SHELL]));
		XwcDrawString(display, paint_win, (XFontSet)font[SIZE12],
			gc, 28, 3*font_hgt, wcstring, wslen(wcstring));
		XtFree((char *)wcstring);
		wcstring = (wchar_t *)XtMalloc(strlen(legendstring[FLAT])+1);
		mbstowcs(wcstring,legendstring[FLAT], 
					strlen(legendstring[FLAT]));
		XwcDrawString(display, paint_win, (XFontSet)font[SIZE12],
			gc, 28, 4*font_hgt, wcstring, wslen(wcstring));
		XtFree((char *)wcstring);
		wcstring = (wchar_t *)XtMalloc(strlen(legendstring[GADGET])+1);
		mbstowcs(wcstring,legendstring[GADGET],
					strlen(legendstring[GADGET]));
		XwcDrawString(display, paint_win, (XFontSet)font[SIZE12],
			gc, 28, 5*font_hgt, wcstring, wslen(wcstring));
		XtFree((char *)wcstring);
	}
	else{
		XSetFont(display, gc, ((XFontStruct *)font[SIZE12])->fid);
		XDrawString(display, paint_win, gc, 8,  5*font_hgt, 
		legendstring[GADGETSYM], strlen(legendstring[GADGETSYM]));
		XDrawString(display, paint_win, gc, 28,   font_hgt, 
		legendstring[CONSTRAINT], strlen(legendstring[CONSTRAINT]));
		XDrawString(display, paint_win, gc, 28, 2*font_hgt, 
		legendstring[PRIMITIVE], strlen(legendstring[PRIMITIVE]));
		XDrawString(display, paint_win, gc, 28, 3*font_hgt, 
			legendstring[SHELL], strlen(legendstring[SHELL]));
		XDrawString(display, paint_win, gc, 28, 4*font_hgt, 
			legendstring[FLAT], strlen(legendstring[FLAT]));
		XDrawString(display, paint_win, gc, 28, 5*font_hgt, 
			legendstring[GADGET], strlen(legendstring[GADGET]));
	}

}
/****************************************************************************/
OlFont
ConvertFontnameToOlFont(Widget w,String fontname)
{
	XrmValue src, dst;
	OlFont font;

	/* Given a fontname, we need to Convert it to a OlFont
	 * and return that value.
	 */

	src.size = strlen(fontname) + 1;
	src.addr = fontname;
	dst.size = sizeof(OlFont);
	dst.addr = (caddr_t)NULL;

	if (!XtConvertAndStore(w, XtRString, &src, XtROlFont, &dst))
		return(NULL);

	return(*(OlFont *)(dst.addr));
}
/***************************************************************************/

Pixel
ConvertColornameToPixel(Widget w, String colorname)
{
	XrmValue src, dst;
	Pixel color;

	/* Given a colorname, we need to Convert it to a Pixel
	 * and return that value.
	 */

	src.size = strlen(colorname) + 1;
	src.addr = colorname;
	dst.size = sizeof(Pixel);
	dst.addr = (caddr_t)&color;

	if (!XtConvertAndStore(w, XtRString, &src, XtRPixel, &dst))
		return(NULL);

	return(color);
}

/*********************************************************
 * InitializeFonts: routine to get handles to teh
 * necessary fonts required to display the Table.
 * We do this hardcoded because we must prevent the
 * user from using fonts too large to fit in the table.
 ********************************************************/
void
InitializeFonts(Widget widget, OlFont *fontarray)
{
	int i;
	char errormsg[100];
	char *fontname[NUMFONTS];
	String	defaultFont;

	fontname[SIZE12] = XtNewString(SIZE12FONT);
	fontname[SIZE10] = XtNewString(SIZE10FONT);
	fontname[SIZE14] = XtNewString(SIZE14FONT);;
	fontname[SIZE18] = XtNewString(SIZE18FONT);
	fontname[LABEL] = XtNewString(LABELFONT);
	fontname[FIXED] = XtNewString(FIXEDFONT);


	for (i = 0; i < NUMFONTS; i++) {
	    if ((fontarray[i] = (OlFont)ConvertFontnameToOlFont(widget,
						 fontname[i])) == NULL){
		sprintf(errormsg,dgettext(OlittableDomain,
				 "Could not open %s font\n"), fontname[i]);
		OlError(errormsg);
		exit(1);
	    }
	}

	XtFree(fontname[SIZE12]);
        XtFree(fontname[SIZE10]);         
	XtFree(fontname[SIZE14]); 
        XtFree(fontname[SIZE18]); 
        XtFree(fontname[LABEL]);
        XtFree(fontname[FIXED]);
	
}

