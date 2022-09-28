#pragma ident	"@(#)Nonexclusi.c	302.10	97/03/26 lib/libXol SMI" /* nonexclus:src/Nonexclusi.c 1.45*/

/*
 *	Copyright (C) 1986,1991  Sun Microsystems, Inc
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
 *	Sun Microsystems, Inc., 2550 Garcia Avenue,
 *	Mountain View, California 94043.
 *
 */

/*	Copyright (c) 1990 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*
 *************************************************************************
 *
 * Description: Nonexclusi.c - Nonexclusives widget
 *
 ******************************file*header********************************
 */

						/* #includes go here	*/
#include <stdio.h>
#include <ctype.h>
#include <libintl.h>

#include <X11/StringDefs.h>
#include <X11/IntrinsicP.h>
#include <X11/Xatom.h>
#include <Xol/OpenLookP.h>
#include <Xol/ButtonP.h>
#include <Xol/RectButtoP.h>
#include <Xol/CheckBoxP.h>
#include <Xol/NonexclusP.h>
#include <Xol/OlI18nP.h>
#include <Xol/RootShellP.h>
#include <Xol/SuperCaret.h>


/*
 *************************************************************************
 *
 * Forward Procedure definitions listed by category:
 *		1. Private Procedures
 *		2. Class   Procedures
 *		3. Action  Procedures
 *		4. Public  Procedures 
 *
 **************************forward*declarations***************************
 */

					/* private procedures		*/
static void CalcColsRows(NonexclusivesWidget ew, int nbuttons, int *columnsptr, int *rowsptr); 
static void CalcNewMax(NonexclusivesWidget ew, int nbuttons);
static void GetDefaultButtons(NonexclusivesWidget ew, Widget *default1, Widget *default2); 
static Boolean IsMenuMode(NonexclusivesWidget ew);
static void Layout(NonexclusivesWidget ew);
static void ResizeSelf(NonexclusivesWidget ew, Boolean greq);
static void TriggerDefault(NonexclusivesWidget ew, Widget w);
static void UnsetDimButtons(NonexclusivesWidget ew);
					/* class procedures		*/
static void ChangedManaged(Widget widget);
static void ClassInitialize(void);
static void DeleteChild(Widget widget, XtPointer client_data, XtPointer call_data);
static XtGeometryResult GeometryManager(Widget widget, XtWidgetGeometry *request, XtWidgetGeometry *reply);
static void Initialize(NonexclusivesWidget request, NonexclusivesWidget new);
static void InsertChild(Widget widget);
static XtGeometryResult QueryGeometry(Widget widget, XtWidgetGeometry *request, XtWidgetGeometry *preferred);
static Boolean SetValues(Widget current, Widget request, Widget new);
static void Realize(Widget widget, Mask *ValueMask, XSetWindowAttributes *attributes);

static int GetFocusItems(int idx, int nbuttons1, int ncolumns, int nrows, OlDefine layout, Boolean row_wanted, int *focus_array, int **return_array, int *nitems);
static int NewIndex(int nitems, int ofocus_index, int n, Boolean plus);
static Widget TraversalHandler(Widget traversal_manager, Widget w, OlVirtualName direction, Time time);

					/* action procedures		*/

					/* public procedures		*/

/*
 *************************************************************************
 *
 * Define global/static variables and #defines, and
 * Declare externally referenced variables
 *
 *****************************file*variables******************************
 */

#define RECTBUTTON 11
#define CHECKBOX 22

#define offset(field) XtOffset(NonexclusivesWidget, field)

static int defNEITHER = (int) 3;
static Boolean defFALSE = (Boolean) FALSE;
static Boolean defTRUE = (Boolean) TRUE;
static int def1 = (int) 1;
static XtPointer defPtr = (XtPointer) NULL;
static Widget defWidget = (Widget) NULL; 
static int defShellDefault  = (int) OtherBehavior; 

#define ARRAY_SIZE 50

/*
 *************************************************************************
 *
 * Define Translations and Actions
 *
 ***********************widget*translations*actions***********************
 */

/*
 *************************************************************************
 *
 * Define Resource list associated with the Widget Instance
 *
 ****************************widget*resources*****************************
 */

static XtResource resources[] = { 

 	{ XtNlayoutType, XtCLayoutType, XtROlDefine, sizeof(OlDefine),
 	  	offset(nonexclusives.layout), XtRImmediate,
 		(XtPointer) ((OlDefine) OL_FIXEDROWS) },

	{ XtNmeasure, XtCMeasure, XtRInt, sizeof(int),
	offset(nonexclusives.measure), XtRInt, (XtPointer) &def1 },

	{ XtNrecomputeSize, XtCRecomputeSize, XtRBoolean, sizeof(Boolean),
	offset(nonexclusives.recompute_size),XtRBoolean,(XtPointer) &defTRUE},

	{ XtNdefault, XtCDefault, XtRBoolean, sizeof(Boolean),
	 offset(nonexclusives.is_default), XtRBoolean,(XtPointer) &defFALSE },

	{ XtNdefaultData, XtCDefaultData, XtRPointer, sizeof(XtPointer),
	 offset(nonexclusives.default_data), XtRPointer, (XtPointer) &defPtr},

	{ XtNpreview, XtCPreview, XtRPointer, sizeof(Widget), 
	  offset(nonexclusives.preview), XtRPointer,(XtPointer) &defWidget },

	{ XtNtrigger, XtCTrigger, XtRBoolean, sizeof(Boolean),
	  offset(nonexclusives.trigger),XtRBoolean,(XtPointer) &defFALSE },

	{ XtNshellBehavior, XtCShellBehavior, XtRInt, sizeof(int),
		offset(nonexclusives.shell_behavior),XtRInt,
		(XtPointer)&defShellDefault },

	{ XtNresetDefault, XtCResetDefault, XtRInt, sizeof(int),
	  offset(nonexclusives.reset_default), XtRInt,(XtPointer) &defNEITHER},

	{XtNpostSelect, XtCCallback,XtRCallback, sizeof(XtPointer), 
	offset(nonexclusives.postselect), XtRCallback, (XtPointer) NULL},
};

#undef offset

/*
 *************************************************************************
 *
 * Define Class Record structure to be initialized at Compile time
 *
 ***************************widget*class*record***************************
 */

NonexclusivesClassRec nonexclusivesClassRec = {
  {
    (WidgetClass) &(managerClassRec),	/* superclass		  */	
    "Nonexclusives",			/* class_name		  */
    sizeof(NonexclusivesRec),		/* widget_size		  */
    ClassInitialize,			/* class_initialize	  */
    NULL,				/* class_part_initialize  */
    FALSE,				/* class_inited		  */
    (XtInitProc) Initialize,		/* initialize		  */
    NULL,				/* initialize_hook	  */
    (XtRealizeProc) Realize,		/* realize		  */
    NULL,				/* actions		  */
    0,					/* num_actions		  */
    resources,				/* resources		  */
    XtNumber(resources),		/* num_resources	  */
    NULLQUARK,				/* xrm_class		  */
    TRUE,				/* compress_motion	  */
    TRUE,				/* compress_exposure	  */
    TRUE,				/* compress_enterleave    */
    FALSE,				/* visible_interest	  */
    NULL,				/* destroy		  */
    NULL,				/* resize		  */
    NULL,				/* expose		  */
    (XtSetValuesFunc) SetValues,	/* set_values		  */
    NULL,				/* set_values_hook	  */
    XtInheritSetValuesAlmost,		/* set_values_almost	  */
    NULL,				/* get_values_hook	  */
    XtInheritAcceptFocus,		/* accept_focus		  */
    XtVersion,				/* version		  */
    NULL,				/* callback_private	  */
    XtInheritTranslations,		/* tm_table		  */
    (XtGeometryHandler) QueryGeometry,	/* query_geometry	  */
  },  /* CoreClass fields initialization */
  {
    (XtGeometryHandler) GeometryManager,/* geometry_manager	*/
    (XtWidgetProc) ChangedManaged,	/* changed_managed	*/
    (XtWidgetProc) InsertChild,		/* insert_child		*/
    XtInheritDeleteChild,		/* delete_child		*/
    NULL,				/* extension         	*/
  },  /* CompositeClass fields initialization */
  {
    /* resources	  */	(XtResourceList)NULL,
    /* num_resources	  */	0,
    /* constraint_size	  */	0,
    /* initialize	  */	(XtInitProc)NULL,
    /* destroy		  */	(XtWidgetProc)NULL,
    /* set_values	  */	(XtSetValuesFunc)NULL
  },	/* constraint_class fields */
  {
    /* highlight_handler  */	NULL,
    /* reserved		  */	NULL,
    /* reserved		  */	NULL,
    /* traversal_handler  */    (OlTraversalFunc)TraversalHandler,
    /* activate		  */    NULL,
    /* event_procs	  */    NULL,
    /* num_event_procs	  */	0,
    /* reserved		  */	NULL,
    /* reserved		  */	NULL,
    /* version		  */	OlVersion,
    /* extension	  */	NULL,
    /* dyn_data		  */	{ NULL, 0 },
    /* transparent_proc   */	_OlDefaultTransparentProc,
  },	/* manager_class fields   */
  {
    0					/* not used now */
  }  /* NonexclusivesClass fields initialization */
};

/*
 *************************************************************************
 *
 * Public Widget Class Definition of the Widget Class Record
 *
 *************************public*class*definition*************************
 */

WidgetClass nonexclusivesWidgetClass = (WidgetClass) &nonexclusivesClassRec;

/*
 *************************************************************************
 *
 * Private Procedures
 *
 ***************************private*procedures****************************
 */

/*
 ************************************************************
 *
 *  CalcColsRows - According to the layout type, this function
 *	calculates the number of columns and rows needed for
 *	the Nonexclusives widget.
 *
 *********************function*header************************
 */

static void CalcColsRows(NonexclusivesWidget ew, int nbuttons, int *columnsptr, int *rowsptr)
{
	int columns,rows;
	OlDefine layout;

	if(nbuttons==0) {
		*columnsptr=0;
		*rowsptr=0;
		return;
	}

	layout=ew->nonexclusives.layout;

	if(layout==(OlDefine) OL_FIXEDCOLS) {	/* calculate number rows */
		columns= ew->nonexclusives.measure;

		if(columns>nbuttons) {
			columns=nbuttons;
			rows=1;
		}
		else {
			rows= nbuttons/columns;
			if(rows*columns<nbuttons) rows+=1;
		}
	}
		
	if(layout==(OlDefine) OL_FIXEDROWS) {	/* calculate number columns */
		rows= ew->nonexclusives.measure;
		if(rows>nbuttons) {
			rows=nbuttons;
			columns=1;
		}
		else {
			columns= nbuttons/rows;
			if(rows*columns<nbuttons) columns+=1;
		}
	}
		
	*columnsptr=columns;
	*rowsptr=rows;
}

/*
 ************************************************************
 *
 *  CalcNewMax - This function recalculates the maximum 
 *	size button needed for all children.
 *
 *********************function*header************************
 */

static void CalcNewMax(NonexclusivesWidget ew, int nbuttons)
{
	int i,class;
	ButtonWidget bw;
	CheckBoxWidget cw;
	Widget child;
	Dimension max_height,max_width;

	if(nbuttons==0) {
		ew->nonexclusives.max_height=(Dimension)1;
		ew->nonexclusives.max_width=(Dimension)1;
		return;
	}

	max_height=(Dimension) 1;
	max_width=(Dimension) 1;
	class= ew->nonexclusives.class_children;

	if(class==RECTBUTTON) {
		for(i=0;i<nbuttons;i++) {
			child=ew->composite.children[i];
			bw= (ButtonWidget) child;
			if(bw->core.height>max_height) {
				max_height=bw->core.height;
			}
			if(bw->button.normal_height>max_height) {
				max_height=bw->button.normal_height;
			}
			if(bw->core.width>max_width) {
				max_width=bw->core.width;
			}
			if(bw->button.normal_width>max_width) {
				max_width=bw->button.normal_width;
			}
		}
	}

	else if(class==CHECKBOX) {
		for(i=0;i<nbuttons;i++) {
			child=ew->composite.children[i];
			cw= (CheckBoxWidget) child;
			if(cw->checkBox.normal_height>max_height) {
				max_height=cw->checkBox.normal_height;
			}
			if(cw->checkBox.normal_width>max_width) {
				max_width=cw->checkBox.normal_width;
			}
		}
	}

	ew->nonexclusives.max_height=max_height;/* store new values */
	ew->nonexclusives.max_width=max_width;

} /* CalcNewMax */

/*
 ************************************************************
 *
 *  GetDefaultButtons - This function finds the new (and
 *	possibly) old default buttons set.
 *
 *********************function*header************************
 */

static void GetDefaultButtons(NonexclusivesWidget ew, Widget *default1, Widget *default2)
{
	Widget child;
	RectButtonWidget bw;
	int i,nchildren,class;

	*default1=(Widget)0;
	*default2=(Widget)0;

	nchildren=ew->composite.num_children;
	if(nchildren==0) return;

	class=ew->nonexclusives.class_children;	

	if(class==RECTBUTTON) {
		for(i=0;i<nchildren;i++) {
			child=ew->composite.children[i];
			bw= (RectButtonWidget) child;
			if(bw->button.is_default!=FALSE) {	
				if(*default1==(Widget)0) {
					*default1=child;
				}
				else {
					*default2=child;
				}
			}
		}
	}

} /* GetDefaultButtons */

/*
 ************************************************************
 *
 *  IsMenuMode - This function checks for exclusives
 *	as menu child for notification of resetting default.
 *
 *********************function*header************************
 */

static Boolean IsMenuMode(NonexclusivesWidget ew)
{
        ShellBehavior sb = ew->nonexclusives.shell_behavior;

        if (sb == PressDragReleaseMenu
			|| sb == StayUpMenu
            		|| sb == PinnedMenu
            		|| sb == UnpinnedMenu) {

                return TRUE;
	}
	else return FALSE;

} /* IsMenuMode */

/*
 ************************************************************
 *
 *  Layout - This function configures a Nonexclusives
 *	widget with either row or column layout.
 *
 *********************function*header************************
 */

static void Layout(NonexclusivesWidget ew)
{
	int i,j,nbuttons,nbuttons1,rows,columns,idx,iheight,class_children;
	OlDefine layout = ew->nonexclusives.layout;
	int hspace,vspace;
	Widget ewidget,child;
	RectButtonWidget bw;
	CheckBoxWidget cw;
	Position x,y,xstart,ystart,lastx;
	Dimension max_height,max_width,last_width;

	if(!(nbuttons=ew->composite.num_children)) {
		ew->nonexclusives.normal_height=(Dimension)1;
		ew->nonexclusives.normal_width=(Dimension)1;
		ew->nonexclusives.max_height=(Dimension)1;
		ew->nonexclusives.max_width=(Dimension)1;
		return;
	}
	
	
	ewidget = (Widget) ew;

	hspace = OlgxScreenPointToPixel(OL_HORIZONTAL, 
					(unsigned)_OlGetScale(XtScreen(ewidget)),
					XtScreen(ewidget));
	vspace = OlgxScreenPointToPixel(OL_VERTICAL,
					((unsigned)_OlGetScale(XtScreen(ewidget))/2) + 1,
					XtScreen(ewidget));


	CalcColsRows(ew,nbuttons,&columns,&rows);
	CalcNewMax(ew,nbuttons); 	/* used to get maximum height */
	max_height= ew->nonexclusives.max_height;
	iheight = (int) max_height;
	ew->nonexclusives.normal_width=(Dimension)0;/* add to this per column */
	class_children=ew->nonexclusives.class_children;

	xstart=(Position)0;
	ystart=(Position)0;
	lastx = (Position)0;
	last_width=(Dimension)0;
	x = (Position) 0;
	nbuttons1 = nbuttons -1;

	for(i=0;i<columns;i++) {
		max_width=(Dimension)0;
		for(j=0;j<rows;j++) {	/* get maximum width */
			if(layout==(OlDefine)OL_FIXEDCOLS) {
				idx=j*columns+i;
			}
			else if(layout==(OlDefine)OL_FIXEDROWS) {
				idx=i*rows+j;
			}
			if(idx>nbuttons1) break;
			child=ew->composite.children[idx];
				if(class_children==RECTBUTTON) {

				bw= (RectButtonWidget) child;
				if(bw->core.width>max_width) {
					max_width=bw->core.width;
				}
				if(bw->button.normal_width>max_width) {
					max_width=bw->button.normal_width;
				}
				}
				else if(class_children==CHECKBOX) {

				cw= (CheckBoxWidget) child;
				if(cw->checkBox.recompute_size &&
					cw->checkBox.normal_width>max_width) {
					max_width=cw->checkBox.normal_width;
				}
				
				else if(!cw->checkBox.recompute_size &&
					cw->core.width>max_width) {
					max_width=cw->core.width;
				}
				}
		}

		if(i!=0)
			x = lastx + (Position)((int)last_width - 1 + hspace);

		for(j=0;j<rows;j++) {	/* set maximum width for col */
			if(layout==(OlDefine)OL_FIXEDCOLS) {
				idx=j*columns+i;
			}
			else if(layout==(OlDefine)OL_FIXEDROWS) {
				idx=i*rows+j;
			}
			if(idx>nbuttons1) break;
			child=ew->composite.children[idx];

			y=(Position) ((int) ystart + (j*(iheight-1+vspace)));

			OlConfigureWidget(child,x,y,
				max_width,max_height,(Dimension)0);
		}
	ew->nonexclusives.normal_width+=max_width;
	last_width=max_width;
	lastx = x;

	} /* i loop */

	if(ew->nonexclusives.layout==(OlDefine)OL_FIXEDCOLS 
		&& nbuttons<columns) {
		columns=nbuttons;
	}
	ew->nonexclusives.normal_width+=((Dimension)(hspace*(columns-1))); 

	if(ew->nonexclusives.layout==(OlDefine)OL_FIXEDROWS && nbuttons<rows) {
		rows=nbuttons;
	}
	ew->nonexclusives.normal_height=
		(Dimension) ( rows * ((int)max_height) + (rows-1)*vspace);

} /* Layout */

/*
 ************************************************************
 *
 *  ResizeSelf - This function calculates the size needed
 *	for the Nonexclusives widget and calls functions to 
 *	reconfigure the button children accordingly.
 *
 *********************function*header************************
 */

static void ResizeSelf(NonexclusivesWidget ew, Boolean greq)
{
	XtWidgetGeometry request,reply;
	Widget ewidget;

	Layout(ew);

	if(greq) {

	request.request_mode = CWHeight | CWWidth | CWBorderWidth;

/*
	if(ew->nonexclusives.recompute_size!=FALSE) {
*/
		request.height = ew->nonexclusives.normal_height;
		request.width  = ew->nonexclusives.normal_width;
/*
	}

	else {			
		request.height = ew->core.height;
		request.width = ew->core.width;
	}
*/
	if(request.height==(Dimension)0) request.height=(Dimension)1;
	if(request.width==(Dimension)0) request.width=(Dimension)1;/* no 0x0 */

	request.border_width =  ew->core.border_width;

	ewidget = (Widget) ew;

	switch(XtMakeGeometryRequest(ewidget,&request,&reply)) {
		case XtGeometryDone:
			break;
		case XtGeometryYes:
			break;
		case XtGeometryAlmost:
			XtMakeGeometryRequest(ewidget,&reply,&reply);
			break;
		case XtGeometryNo:
			break;
	}
	}

} /* ResizeSelf */

/*
 ************************************************************
 *
 *  TriggerDefault - This function simulates a button
 *	selection on the default button.
 *
 *********************function*header************************
 */

static void TriggerDefault(NonexclusivesWidget ew, Widget w)
{
	Arg arg[2];
	RectButtonWidget bw;

	if(w==(Widget)0) return;
	if(ew->nonexclusives.class_children==CHECKBOX) return;
	bw = (RectButtonWidget) w;
	if(!bw->button.is_default) return;

	if(bw->button.set) {
		XtSetArg(arg[0],XtNset,(XtArgVal) FALSE);
		XtSetArg(arg[1],XtNparentReset,(XtArgVal)TRUE);
		XtSetValues(w,arg,2);	

		_OlManagerCallPreSelectCBs(w, (XEvent *)NULL);

		XtCallCallbacks(w,XtNunselect,(XtPointer)NULL);

		_OlManagerCallPostSelectCBs(w, (XEvent *)NULL);
		return;
	}

				/* here if button not set */

	XtSetArg(arg[0],XtNset,(XtArgVal) TRUE);
	XtSetArg(arg[1],XtNparentReset,(XtArgVal)TRUE);
	XtSetValues(w,arg,2);	

	_OlManagerCallPreSelectCBs(w, (XEvent *)NULL);

	XtCallCallbacks(w,XtNselect,(XtPointer)NULL);

	_OlManagerCallPostSelectCBs(w, (XEvent *)NULL);

} /* TriggerDefault */

/*
 ************************************************************
 *
 *  UnsetDimButtons - This function is called when a new
 *	button is set.
 *
 *********************function*header************************
 */
static void UnsetDimButtons(NonexclusivesWidget ew)
{
	Widget child;
	RectButtonWidget bw;
	CheckBoxWidget cw;
	int i,nchildren,class;
	Arg arg;

	nchildren=ew->composite.num_children;
	class= ew->nonexclusives.class_children;
	if(nchildren==0) return;

	if(class==RECTBUTTON) {
		for(i=0;i<nchildren;i++) {
			child= ew->composite.children[i];
			bw= (RectButtonWidget) child;
			if(bw->button.dim!=FALSE) {	
				XtSetArg(arg,XtNdim,(XtArgVal) FALSE);
				XtSetValues(child,&arg,1);	/* unset it */
			}
		}
	}

	else if(class==CHECKBOX) {
		for(i=0;i<nchildren;i++) {
			child= ew->composite.children[i];
			cw= (CheckBoxWidget) child;
			if(cw->checkBox.dim!=FALSE) {	
				XtSetArg(arg,XtNdim,(XtArgVal) FALSE);
				XtSetValues(child,&arg,1);	/* unset it */
			}
		}
	}

} /* UnsetDimButtons */

/*
 *************************************************************************
 *
 * Class Procedures
 *
 ****************************class*procedures*****************************
 */

/*
 ************************************************************
 *
 *  GetFocusItems - utility function
 *
 *********************function*header************************
 */

static int
GetFocusItems(int idx, int nbuttons1, int ncolumns, int nrows, OlDefine layout, Boolean row_wanted, int *focus_array, int **return_array, int *nitems)
{
int cs,ce,dim1,dim2,i,n,ni,current_index,*tmp_array;

if((layout==(OlDefine)OL_FIXEDROWS && !row_wanted) 
	|| (layout==(OlDefine)OL_FIXEDCOLS && row_wanted)) {
	if(layout==(OlDefine)OL_FIXEDROWS)
		dim1 = nrows;
	else
		dim1 = ncolumns;


	cs = idx - (idx % dim1);
	ce = cs + dim1 - 1;
	if(ce > nbuttons1)
		ce = nbuttons1;
	n = ce - cs + 1;

	if (n > ARRAY_SIZE) {
		tmp_array = (int *)XtMalloc(n * sizeof(int));
	} else {
		tmp_array = focus_array;
	}

	for(i=0;i<n;i++) {
		tmp_array[i] = cs + i;
		if(tmp_array[i]==idx)
			current_index = i;
	}
}
else if((layout==(OlDefine)OL_FIXEDROWS && row_wanted) 
	|| (layout==(OlDefine)OL_FIXEDCOLS && !row_wanted)) {

	if(layout==(OlDefine)OL_FIXEDROWS) {
		dim1 = nrows;
		dim2 = ncolumns;
	}
	else {
		dim1 = ncolumns;
		dim2 = nrows;
	}
	cs = idx % dim1;
	ce = cs + (dim2-1)*dim1;
	if(ce > nbuttons1)
		ce = ce - dim1;

	n = (ce - cs)/dim1 + 1;

	if (n > ARRAY_SIZE) {
		tmp_array = (int *)XtMalloc(n * sizeof(int));
	} else {
		tmp_array = focus_array;
	}

	for(i=0;i<n;i++) {
		tmp_array[i] = cs + i*dim1;
		if(tmp_array[i]==idx)
			current_index = i;
	}
}
	*nitems = n;
	*return_array = tmp_array;
	return current_index;

} /* GetFocusItems */

/*
 ************************************************************
 *
 *  NewIndex - utility function
 *
 *********************function*header************************
 */

static int
NewIndex(int nitems, int ofocus_index, int n, Boolean plus)
{
	int tmp_idx;

	if(plus) {
		tmp_idx = ofocus_index + n;
		if(tmp_idx > (nitems-1))
			tmp_idx = 0;
	}
	else	{	/* if !plus */
		tmp_idx = ofocus_index - n;
		if(tmp_idx < 0)
			tmp_idx = nitems - 1;
	}
	
	return tmp_idx;

} /* NewIndex */

/*
 ************************************************************
 *
 *  TraversalHandler - This function manages the traversal
 *	among the nonexclusives's button children.
 *
 *********************function*header************************
 */
static Widget 
TraversalHandler(Widget traversal_manager, Widget w, OlVirtualName direction, Time time)
{
	NonexclusivesWidget ew = (NonexclusivesWidget) traversal_manager;

	int k,nbuttons,nbuttons1,nrows,ncolumns,focus_index,idx,tmp_idx,
		focus_array[ARRAY_SIZE],*return_array,multi_count,nitems;
	Widget child, *children = ew->composite.children;
	CheckBoxWidget cb;

	if(!(nbuttons=ew->composite.num_children)) 
		return (Widget) NULL;

	if(w==(Widget)NULL) 
		return ew->composite.children[0];

	CalcColsRows(ew,nbuttons,&ncolumns,&nrows);

	nbuttons1 = nbuttons -1;

	for(k=0;k<nbuttons;k++) {
		child=children[k];
		idx=k;
		if(child==w)
			break;
	}

	if(direction==(OlVirtualName)OL_MULTIRIGHT 
		|| direction==(OlVirtualName)OL_MOVERIGHT
		|| direction==(OlVirtualName)OL_MULTILEFT
		|| direction==(OlVirtualName)OL_MOVELEFT)

	focus_index =GetFocusItems(idx,nbuttons1,ncolumns,nrows,
		ew->nonexclusives.layout,TRUE,focus_array,&return_array,&nitems);

	else if(direction==(OlVirtualName)OL_MULTIUP 
		|| direction==(OlVirtualName)OL_MOVEUP
		|| direction==(OlVirtualName)OL_MULTIDOWN
		|| direction==(OlVirtualName)OL_MOVEDOWN)

	focus_index = GetFocusItems(idx,nbuttons1,ncolumns,nrows,
		ew->nonexclusives.layout,FALSE,focus_array,&return_array,&nitems);

	switch(direction) {

		case OL_IMMEDIATE:
			for(k=0;k<nbuttons;k++) {
				tmp_idx = idx + k+1;
				if(tmp_idx>nbuttons1)
					tmp_idx = tmp_idx % nbuttons;
				child=children[idx];
				if(OlCallAcceptFocus(child,time)) 
					break;
			}
			break;

		case OL_MULTILEFT:
		case OL_MULTIUP:

		multi_count = (int)
			_OlGetMultiObjectCount(traversal_manager);

			focus_index = NewIndex(nitems,focus_index,
						multi_count,FALSE);

			child = children[return_array[focus_index]];
			if(OlCallAcceptFocus(child,time)) 
				break;

		case OL_MOVELEFT:
		case OL_MOVEUP:

			for(k=0;k<nitems;k++) {

			focus_index = NewIndex(nitems,focus_index,
						1,FALSE);

			child = children[return_array[focus_index]];
			if(OlCallAcceptFocus(child,time)) 
				break;
			}

			break;

		case OL_MULTIRIGHT:
		case OL_MULTIDOWN:

		multi_count = (int)
			_OlGetMultiObjectCount(traversal_manager);

			focus_index = NewIndex(nitems,focus_index,
						multi_count,TRUE);

			child = children[return_array[focus_index]];
			if(OlCallAcceptFocus(child,time)) 
				break;

		case OL_MOVERIGHT:
		case OL_MOVEDOWN:

			for(k=0;k<nitems;k++) {

			focus_index = NewIndex(nitems,focus_index,
						1,TRUE);

			child = children[return_array[focus_index]];
			if(OlCallAcceptFocus(child,time)) 
				break;
			}
			break;

		default:
			return NULL;
	}

	if (return_array != focus_array) 
		XtFree((char *)return_array);

	return child;

} /*TraversalHandler */

/*
 ************************************************************
 *
 *  ChangedManaged - This function keeps all rectangular
 *	button children managed, to prevent chaos.
 *
 *********************function*header************************
 */

static void ChangedManaged(Widget widget)
{
	NonexclusivesWidget ew;
	RectButtonWidget bw;
	CheckBoxWidget cw;
	Widget child;
	int i,nchildren,breaktime=0,class;

	ew= (NonexclusivesWidget) widget;
	nchildren=ew->composite.num_children;
	class=ew->nonexclusives.class_children;

	if(class==RECTBUTTON) {
		for(i=0;i<nchildren;i++) {
			child=ew->composite.children[i];
			bw= (RectButtonWidget) child;
			if(bw->core.managed==FALSE) {
				XtManageChild(child);
				if(XtIsRealized(child)) XtMapWidget(child);
				breaktime=1;
				break;
			}
			if(breaktime) break;
		}
	}

	if(class==CHECKBOX) {
		for(i=0;i<nchildren;i++) {
			child=ew->composite.children[i];
			cw= (CheckBoxWidget) child;
 			if(cw->core.managed==FALSE) {
				XtManageChild(child);
				if(XtIsRealized(child)) XtMapWidget(child);
				breaktime=1;
				break;
			}
			if(breaktime) break;
	}

	}

	ResizeSelf(ew,TRUE); 

} /* ChangedManaged */

/*
 ************************************************************
 *
 *  ClassInitialize - Register OlDefine string values.
 *
 *********************function*header************************
 */

static void ClassInitialize(void)
{
	_OlAddOlDefineType ("fixedcols", OL_FIXEDCOLS);
	_OlAddOlDefineType ("fixedrows", OL_FIXEDROWS);
}

/*
 ************************************************************
 *
 *  DeleteChild - This function keeps all rectangular
 *	button children managed, to prevent chaos and sees
 *	that the Nonexclusives widget is reconfigured accordingly.
 *
 *********************function*header************************
 */

static void DeleteChild(Widget widget, XtPointer client_data, XtPointer call_data)
{
	Widget parent;
	RectButtonWidget bw;
	CheckBoxWidget cw;
	NonexclusivesWidget ew;
	int class;
	
	parent = XtParent(widget);
	ew= (NonexclusivesWidget) parent;
	class=ew->nonexclusives.class_children;

	if(class==RECTBUTTON) {
		bw= (RectButtonWidget) widget;
	}
	else if(class==CHECKBOX) {
		cw= (CheckBoxWidget) XtParent(widget);
	}

	if(ew->core.being_destroyed!=FALSE) {
		return;
	}

	if(ew->composite.num_children==0) {
		return; 
	}

	if(class==RECTBUTTON && bw->button.is_default!=FALSE) {
		ew->nonexclusives.is_default=FALSE;
		ew->nonexclusives.default_child=(Widget)0;
	}

	ResizeSelf(ew,TRUE); 

} /* DeleteChild */

/*
 ************************************************************
 *
 *  GeometryManager - This function is called when a button or
 *	checkbox child wants to resize itself; current policy is to 
 *	allow the requested resizing.
 *
 *********************function*header************************
 */

static XtGeometryResult GeometryManager(Widget widget, XtWidgetGeometry *request, XtWidgetGeometry *reply)
{
	NonexclusivesWidget ew;
	Widget parent;
	RectButtonWidget bw;
	CheckBoxWidget cw;
	int class;

	if(((request->request_mode & CWHeight) 
		&& request->height==(Dimension)0) 

		|| ((request->request_mode & CWWidth)
			&& request->width==(Dimension)0)) {

		OlWarning(dgettext(OlMsgsDomain,
		  "Nonexclusives: Height and/or width must be greater than 0"));
		return XtGeometryNo;
	}

	parent=XtParent(widget);
	ew = (NonexclusivesWidget) parent;
	class = ew->nonexclusives.class_children;

	ResizeSelf(ew,TRUE);			/* does all children too */

	reply->request_mode=CWWidth | CWHeight;

	if(class==RECTBUTTON) {
		bw=(RectButtonWidget) widget;
		reply->height=bw->core.height;
		reply->width=bw->core.width;
	}
	else if(class==CHECKBOX) {
		cw=(CheckBoxWidget) widget;
		reply->height=cw->core.height;
		reply->width=cw->core.width;
	}

	if(reply->height==request->height && reply->width==request->width) {
		OlWidgetConfigured(widget);
		return XtGeometryYes;
	}
	else return XtGeometryAlmost;

} /* GeometryManager */

/*
 ************************************************************
 *
 *  Initialize - This function checks that Nonexclusives variable
 *	values are within range and initializes private fields
 *
 *********************function*header************************
 */

static void
Initialize(NonexclusivesWidget request, NonexclusivesWidget new)
{
	Widget parent;
	Widget w = (Widget) new;
	CorePart *cp;
	NonexclusivesPart *ep;
	ManagerPart	*mp;

	cp = &(new->core);
	ep = &(new->nonexclusives);
	mp = &(new->manager);

	parent= cp->parent;

/* Set relevant core fields */

	if(ep->recompute_size!=FALSE) {
		cp->height=(Dimension) 1;		
		cp->width=(Dimension) 1;		
	}
	else {
		if(cp->height==(Dimension)0) cp->height=(Dimension)1;
		if(cp->width==(Dimension)0) cp->width=(Dimension)1;
	}

	if(cp->border_width!=(Dimension)0) cp->border_width= (Dimension) 0;

	if (cp->background_pixel!=parent->core.background_pixel) {
		cp->background_pixel=parent->core.background_pixel;
	}

	if (cp->background_pixmap!=parent->core.background_pixmap) {
		cp->background_pixmap=parent->core.background_pixmap;
	}

/* Check/Set relevant nonexclusives fields */

	/* public resources */

	if(ep->layout!= (OlDefine) OL_FIXEDCOLS 
		&& ep->layout!= (OlDefine) OL_FIXEDROWS) {
		OlWarning(dgettext(OlMsgsDomain,
			"XtNlayoutType must be OL_FIXEDROWS or \
OL_FIXEDCOLS: former will be used"));
		ep->layout=(OlDefine) OL_FIXEDROWS;
	}

	if(ep->measure<1) {
		OlWarning(dgettext(OlMsgsDomain,
			"XtNmeasure must be >= one : one will be used"));
		ep->measure=1;
	}

	/* private resources */

	if(ep->is_default!=FALSE) {
		OlWarning(dgettext(OlMsgsDomain,
				"XtNdefault must be FALSE"));
		ep->is_default=FALSE;
	}

	if(ep->trigger!=FALSE) {
		ep->trigger=TRUE;
	}

	if(ep->reset_default!=defNEITHER) {
		OlWarning(dgettext(OlMsgsDomain,
				"XtNresetDefault must be NEITHER"));
		ep->reset_default=defNEITHER;
	}

	if(		ep->shell_behavior!=OtherBehavior
		&&	ep->shell_behavior!=BaseWindow
		&&	ep->shell_behavior!=PopupWindow
		&&	ep->shell_behavior!=PinnedWindow
		&&	ep->shell_behavior!=PinnedMenu
		&&	ep->shell_behavior!=PressDragReleaseMenu
		&&	ep->shell_behavior!=StayUpMenu
		&&	ep->shell_behavior!=UnpinnedMenu ) {

		OlWarning(dgettext(OlMsgsDomain,
			"XtNshellBehavior illegal value: OtherBehavior used"));
		new->nonexclusives.shell_behavior=OtherBehavior;
	}

	/* private fields */

	ep->class_children = 			0;
    	ep->default_child =		(Widget) 0;
    	ep->max_height =		(Dimension)1;
    	ep->max_width =			(Dimension)1;
    	ep->normal_height =		(Dimension)1;
    	ep->normal_width =		(Dimension)1;

		/* add me to the traversal list */
	_OlUpdateTraversalWidget(w, mp->reference_name,
				 mp->reference_widget, True);
} /* Initialize */

/*
 ************************************************************
 *
 *  InsertChild - This function checks that all children are
 *	rectangular buttons or checkboxes, that there is no more 
 *	than one default button, adds the event handler for
 *	the nonexclusives, and sees that the Nonexclusives widget
 *	is reconfigured accordingly. 
 *
 *********************function*header************************
 */

static void
InsertChild(Widget widget)
{
    Arg arg[2];
    int class=0;
    RectButtonWidget	bw = (RectButtonWidget) widget;
    Widget		parent = XtParent(widget);
    NonexclusivesWidget	ew = (NonexclusivesWidget) parent;
    XtWidgetProc	insert_child = ((CompositeWidgetClass)
	(nonexclusivesClassRec.core_class.superclass))->
		composite_class.insert_child;

	if(XtIsSubclass(widget,rectButtonWidgetClass))
		class=RECTBUTTON;
	else if(XtIsSubclass(widget,checkBoxWidgetClass))
		class=CHECKBOX;

		/* check that a rectangular button or checkbox */

	if(class!=RECTBUTTON && class!=CHECKBOX) {
		OlWarning(dgettext(OlMsgsDomain,
			"Not a rectangular button or checkbox:\n\tdestroyed and results unpredictable"));
		XtDestroyWidget(widget);
		return;
	}
				/* if first child record type */
	if(ew->nonexclusives.class_children==0) {
		ew->nonexclusives.class_children=class;
	}
				/* check for consistency of children */
	if((ew->nonexclusives.class_children==RECTBUTTON && class==CHECKBOX) 
	|| (ew->nonexclusives.class_children==CHECKBOX && class==RECTBUTTON)) {
		OlWarning(dgettext(OlMsgsDomain,
			"cannot mix types of children:\n\tdestroyed and results unpredictable"));
		XtDestroyWidget(widget);
		return;
	}

				/* insert the child since right type */
 	if (!insert_child) {
 		OlError(dgettext(OlMsgsDomain,
				"Nonexclusives: no InsertChild function."));
 		return;
	}

	    (*insert_child)(widget);

	XtManageChild(widget);
	XtSetMappedWhenManaged(widget,TRUE); /* default but do it anyhow */

					
			/* check for no more than one default button: */
			/* 	defaults only for rectbuttons         */

	if(class==RECTBUTTON && bw->button.is_default!=FALSE) {
		if(ew->nonexclusives.default_child==(Widget)0) {
			ew->nonexclusives.default_child=widget;	/* record it */
			ew->nonexclusives.is_default=TRUE;
		} else {
			XtSetArg(arg[0],XtNdefault,(XtArgVal)FALSE);
			XtSetValues(widget,arg,1);/* unset this one */
			OlWarning(dgettext(OlMsgsDomain,
				"default has been set already - ignored"));
		}
	}

				/* add callbacks needed */
	XtAddCallback(widget,XtNdestroyCallback,DeleteChild,parent);

	_OlDeleteDescendant(widget);	/* remove button from traversal list */


} /* InsertChild */

/*
 ************************************************************
 *
 *  QueryGeometry - This function is a rather rigid prototype
 *	since the widget insists upon its core height and width.
 *
 ************************************************************
*/

static XtGeometryResult QueryGeometry(Widget widget, XtWidgetGeometry *request, XtWidgetGeometry *preferred)
{
	NonexclusivesWidget ew;

	ew= (NonexclusivesWidget) widget;

		/* if not height or width it is okay with widget */

	if(!(request->request_mode & CWHeight 
			|| request->request_mode &CWWidth)) {

		return XtGeometryYes;
	}

		/* if still here look at requested height and/or width */

	if((request->request_mode & CWHeight)
		&& (request->height!=ew->nonexclusives.normal_height)) {

		preferred->request_mode |=CWHeight;
		preferred->height=ew->nonexclusives.normal_height;
	};
		
	if((request->request_mode & CWWidth)
		&& (request->width!=ew->nonexclusives.normal_width)) {

		preferred->request_mode |=CWWidth;
		preferred->width=ew->nonexclusives.normal_width;
	};

	if((preferred->request_mode & CWHeight)
		|| (preferred->request_mode & CWWidth)) {

		return XtGeometryAlmost;
	}

	else return XtGeometryNo;

} /* QueryGeometry */

/*
 ************************************************************
 *
 *  Realize - This function realizes the Nonexclusives widget in
 *	order to be able to set window properties
 *
 *********************function*header************************
 */

static void Realize(Widget widget, Mask *ValueMask, XSetWindowAttributes *attributes)
{

	*ValueMask|=CWBitGravity;
	attributes->bit_gravity=NorthWestGravity;

	XtCreateWindow(	widget,(unsigned int)InputOutput,
			(Visual *)CopyFromParent, *ValueMask, attributes);

} /* Realize */

/*
 ************************************************************
 *
 *  SetValues - This function checks and allows setting and
 *	resetting of Nonexclusives resources.
 *
 *********************function*header************************
 */

static Boolean SetValues(Widget current, Widget request, Widget new)
{
	int newmeasure;
	OlDefine newlayout;
	Boolean needs_redisplay=FALSE,Grequest=FALSE;
	Widget button,widget,default1,default2,parent;
	Arg arg;
	XSetWindowAttributes attributes;

	NonexclusivesWidget currew =	(NonexclusivesWidget) current;
	NonexclusivesWidget newew =	(NonexclusivesWidget) new;

	widget = (Widget) current;
	parent=XtParent(widget);

	/* XtNtrigger: allow user to simulate default hit */

	if(newew->nonexclusives.trigger!=FALSE) {
		newew->nonexclusives.trigger=FALSE;	/* reset */

		button=newew->nonexclusives.default_child;
		if(button!=(Widget)0) {			/* if default exists */
			TriggerDefault(newew,button);
		}
		else {
			OlWarning(dgettext(OlMsgsDomain,
				"No button or no default currently set"));
		}
	}

	/* ******************************************* */
	/* check that core values correct or not reset */
	/* ******************************************* */

	if (newew->core.height==(Dimension)0) {
		newew->core.height=(Dimension)1;
	}

	if (newew->core.width==(Dimension)0) {
		newew->core.width=(Dimension)1;
	}

	if (newew->core.border_width!=(Dimension)0) {
		newew->core.border_width=(Dimension)0;
	}

	if (newew->core.background_pixel!=parent->core.background_pixel) {
		newew->core.background_pixel=parent->core.background_pixel;
		if (XtIsRealized((Widget)newew)) {
		attributes.background_pixel=parent->core.background_pixel;
		XChangeWindowAttributes(XtDisplay(newew),XtWindow(newew),
			CWBackPixel,&attributes);
		needs_redisplay=TRUE;
		}
	}

	if (newew->core.background_pixmap!=parent->core.background_pixmap) {
		newew->core.background_pixmap=parent->core.background_pixmap;
		if((newew->core.background_pixmap!=XtUnspecifiedPixmap) &&
		   XtIsRealized((Widget)newew)) {

		attributes.background_pixmap=newew->core.background_pixmap;
		XChangeWindowAttributes(XtDisplay(newew),XtWindow(newew),
			CWBackPixmap,&attributes);
		}
		needs_redisplay=TRUE;
	}

	/* XtNsensitive resource */

	if(XtIsSensitive(new)!=XtIsSensitive(current)) {
			needs_redisplay=TRUE;
	}

	/* ************************************* */
	/* check nonexclusives private resources */
	/* ************************************* */

	/* XtNdefault: used by menu on nonexclusives as follows */

	if (newew->nonexclusives.is_default!=currew->nonexclusives.is_default) {

		if(currew->nonexclusives.is_default==FALSE 
				&& newew->nonexclusives.is_default!=FALSE) {

			if(newew->nonexclusives.default_child==(Widget)0) {
				if(newew->composite.num_children!=0) {

		XtSetArg(arg,XtNdefault,(XtArgVal) TRUE);
		XtSetValues(newew->composite.children[0],&arg,1);
		newew->nonexclusives.default_child=newew->composite.children[0];

				}
				else {
					newew->nonexclusives.is_default=FALSE;
					OlWarning(dgettext(OlMsgsDomain,
						"No child to set as default"));
				}
			}
		}
		else if(currew->nonexclusives.is_default!=FALSE 
				&& newew->nonexclusives.is_default==FALSE) {
					
			GetDefaultButtons(newew,&default1,&default2); 
			if(default1!=(Widget)0) {

		XtSetArg(arg,XtNdefault,(XtArgVal) FALSE);
		XtSetValues(default1,&arg,1);
		newew->nonexclusives.default_child=(Widget)0;

			}
		}

	} /* XtNdefault */	

	/* XtNresetDefault : used to signal setting or unsetting of default */

	if(newew->nonexclusives.reset_default==TRUE) {

	newew->nonexclusives.reset_default=defNEITHER;	/* reset */
/*
	needs_redisplay=TRUE;
*/
	GetDefaultButtons(newew,&default1,&default2); 
						/* error condition only */
	if(default1==(Widget)0 && default2==(Widget)0) {		
		newew->nonexclusives.is_default=FALSE;
	} 				
	else if(default1!=(Widget)0 && default2==(Widget)0) {
		newew->nonexclusives.default_child=default1;
		newew->nonexclusives.is_default=TRUE;
	}					/* one new & one old */
	else if(default1!=(Widget)0 && default2!=(Widget)0) {
					/* default1 old && default2 new */
		if(default1==newew->nonexclusives.default_child) {
			XtSetArg(arg,XtNdefault,(XtArgVal) FALSE);
			XtSetValues(default1,&arg,1);	/* unset old */
			newew->nonexclusives.default_child=default2;
		}
		else {			/* default2 old && default1 new */
			XtSetArg(arg,XtNdefault,(XtArgVal) FALSE);
			XtSetValues(default2,&arg,1);	/* unset old */
			newew->nonexclusives.default_child=default1;
		}
	}
	} /* TRUE */

	else if(newew->nonexclusives.reset_default==FALSE) {

	newew->nonexclusives.is_default=defNEITHER;	/* reset */
/*
	needs_redisplay=TRUE;
*/
	GetDefaultButtons(newew,&default1,&default2); 
						/* no more default */
	if(default1==(Widget)0 && default2==(Widget)0) {		
		newew->nonexclusives.default_child=(Widget)0;
		newew->nonexclusives.is_default=FALSE;
	}
	} /* FALSE */

	/* XtNdefaultData resource */

		/* let widget writer set and get any pointer needed 
		   for updating "cloned" menus - intrinsics does work here */

	/* XtNpreview : pass to Button for display */

	widget=newew->nonexclusives.preview;
	if(widget!=(Widget)0) {
		GetDefaultButtons(newew,&default1,&default2);
		if(default1!=(Widget)0) {
			XtSetArg(arg,XtNpreview,(XtArgVal) widget);
			XtSetValues(default1,&arg,1);
		}
		else {
			OlWarning(dgettext(OlMsgsDomain,
					"No default button to preview"));
		}
		newew->nonexclusives.preview=(Widget)0;   /* reset */
	}
						
	/* XtNshellBehavior */

	if(newew->nonexclusives.shell_behavior
			!=currew->nonexclusives.shell_behavior) {

	if(		newew->nonexclusives.shell_behavior!=OtherBehavior
		&&	newew->nonexclusives.shell_behavior!=BaseWindow
		&&	newew->nonexclusives.shell_behavior!=PopupWindow
		&&	newew->nonexclusives.shell_behavior!=PinnedWindow
		&&	newew->nonexclusives.shell_behavior!=PinnedMenu
		&&	newew->nonexclusives.shell_behavior!=PressDragReleaseMenu
		&&	newew->nonexclusives.shell_behavior!=StayUpMenu
		&&	newew->nonexclusives.shell_behavior!=UnpinnedMenu ) {

		OlWarning(dgettext(OlMsgsDomain,
			"XtNshellBehavior illegal value: Not changed"));
		newew->nonexclusives.shell_behavior=
			currew->nonexclusives.shell_behavior;
	}
	}

	/* ********************************** */
	/* check exclusives public resources */
	/* ********************************** */

	/* XtNlayoutType : change layout configuration of buttons */

	newlayout=newew->nonexclusives.layout;

	if(currew->nonexclusives.layout!=newlayout) {
	  					/* bad request */
	if(newlayout!= (OlDefine) OL_FIXEDCOLS 
		&& newlayout!= (OlDefine) OL_FIXEDROWS) {
		OlWarning(dgettext(OlMsgsDomain,
			"XtNlayoutType incorrect: current value will be used"));
		newew->nonexclusives.layout=currew->nonexclusives.layout;
	}
					/* change in layout */
	else {
		Grequest=TRUE;
	}
	} /* XtNlayoutType */

	/* XtNmeasure : change measure configuration of buttons */

	newmeasure=newew->nonexclusives.measure;

	if(currew->nonexclusives.measure!=newmeasure) {
	  					/* bad request */
	if(newmeasure<1) {
		OlWarning(dgettext(OlMsgsDomain,
			"XtNmeasure incorrect: current value will be used"));
		newew->nonexclusives.measure=currew->nonexclusives.measure;
	}
					/* change in measure */
	else {
		Grequest=TRUE;
	}
	} /* XtNmeasure */


	/* XtNrecomputeSize */

	if(newew->nonexclusives.recompute_size
				!=currew->nonexclusives.recompute_size) { 
		needs_redisplay=TRUE;
		Grequest=TRUE;
	}

	/* do not make geometry request - intrinsics will request */

	if(Grequest) {
		ResizeSelf(newew,FALSE); 
		newew->core.width=newew->nonexclusives.normal_width;
		newew->core.height=newew->nonexclusives.normal_height;
	}

	return needs_redisplay;

}	/* SetValues */

/*
 *************************************************************************
 *
 * Action Procedures
 *
 ****************************action*procedures****************************
 */

/*
 *************************************************************************
 *
 * Public Procedures
 *
 ****************************public*procedures****************************
 */
