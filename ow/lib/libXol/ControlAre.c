#pragma ident	"@(#)ControlAre.c	302.11	94/04/01 lib/libXol SMI"	/* control:src/ControlAre.c 1.45	*/

/*
 *	Copyright (C) 1986,1992  Sun Microsystems, Inc
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

/****************************************************************************
 *
 * Description:	This is the code for the Control Area widget.
 *
  ********************************file*header********************************/


#include <libintl.h>

#include <X11/IntrinsicP.h>
#include <X11/StringDefs.h>

#include <Xol/CaptionP.h>
#include <Xol/ControlArP.h>
#include <Xol/OlDnDVCX.h>
#include <Xol/OlI18nP.h>
#include <Xol/OpenLookP.h>
#include <Xol/ControlAre.h>
#include <Xol/SuperCaret.h>
#include <Xol/Menu.h>
#include <Xol/Button.h>


/**************************************************************************
 *
 * Forward Procedure definitions listed by category:
 *		1. Private Procedures
 *		2. Class   Procedures
 *		3. Action  Procedures
 *		4. Public  Procedures
 *
 **************************forward*declarations****************************/

					/* private procedures		*/

static void		CLayout(ControlAreaWidget w, OlCLayoutList **layoutList, int layoutCount, Dimension *managerWidth, Dimension *managerHeight), RLayout(ControlAreaWidget w, OlCLayoutList **layoutList, int layoutCount, Dimension *managerWidth, Dimension *managerHeight), HLayout(ControlAreaWidget w, OlCLayoutList **layoutList, int layoutCount, Dimension *managerWidth, Dimension *managerHeight), WLayout(ControlAreaWidget w, OlCLayoutList **layoutList, int layoutCount, Dimension *managerWidth, Dimension *managerHeight);
static void		FreeLayoutList(OlCLayoutList **layoutList, register int num_children);
static void		ForceSize(OlCLayoutList **layoutList, int layoutCount);
static void		GetSizes(Dimension *widthList, Dimension *heightList, Dimension *rightList, Dimension *leftList, OlCLayoutList **layoutList, int layoutCount, ControlAreaWidget w);
static Boolean		LayoutChildren(ControlAreaWidget mw, Boolean doit, Boolean resizeok);
static void		MakeLayoutList(ControlAreaWidget mw, OlCLayoutList **layoutList, int layoutCount, Dimension *pmanagerWidth, Dimension *pmanagerHeight);

					/* class procedures		*/

static void		ChangeManaged(Widget w);
static void		ClassInitialize(void);
static void		ControlClassPartInitialize(WidgetClass widgetClass);
static void		Destroy(Widget widget);
static XtGeometryResult	QueryGeometry (Widget widget,
				       XtWidgetGeometry *constraint,
				       XtWidgetGeometry *preferred);
static XtGeometryResult	GeometryManager(Widget w,
					XtWidgetGeometry *request,
					XtWidgetGeometry *reply);
static void		Initialize(Widget request, Widget new,
				   ArgList args, Cardinal *num_args);
static void		InitializeHook (Widget w,
					ArgList args, Cardinal *num_args);
static void		InsertChild(Widget w);
static void		Resize(Widget w);
static Boolean		SetValues(Widget current, Widget request, Widget new,
				  ArgList args, Cardinal *num_args);
static void		ExposeProc (Widget w, XEvent *pe, Region region);
static void		ConstraintInitialize (Widget request, Widget new,
					      ArgList args,
					      Cardinal *num_args);
static void		ConstraintDestroy (Widget w);
static Boolean		ConstraintSetValues (Widget current, Widget request,
					     Widget new, ArgList args,
					     Cardinal *num_args);

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

#define Max(x, y)       (((x) > (y)) ? (x) : (y))
#define Min(x, y)       (((x) < (y)) ? (x) : (y))

#define CONSTRAINT(W)	(*(ControlAreaConstraintRec **)&((W)->core.constraints))
#define CHANGEBARS(W)	(W)->control.allow_change_bars
#define CB(W)		(W)->control.cb

#if	!defined(Array)
# define Array(P,T,N) \
	((N)?								\
		  ((P)?							\
			  (T *)XtRealloc((P), sizeof(T) * (N))		\
			: (T *)XtMalloc(sizeof(T) * (N))		\
		  )							\
		: ((P)? (XtFree(P),(T *)0) : (T *)0)			\
	)
#endif

static int		measure = 1;
static Dimension	hspace = 4;
static Dimension	vspace = 4;
static Dimension	hpad = 4;
static Dimension	vpad = 4;
static Boolean		aligncaptions = FALSE;
static Boolean		center = FALSE;

/*
 *************************************************************************
 *
 * Define Translations and Actions
 *
 ***********************widget*translations*actions**********************
 */


/*
 *************************************************************************
 *
 * Define Resource list associated with the Widget Instance
 *
 ****************************widget*resources*****************************
 */


static XtResource resources[] =
{

   { XtNmeasure, XtCMeasure, XtRInt, sizeof(int),
      XtOffset(ControlAreaWidget, control.measure), XtRInt, (XtPointer) &measure},

   { XtNlayoutType, XtCLayoutType, XtROlDefine, sizeof(OlDefine),
      XtOffset(ControlAreaWidget, control.layouttype), XtRImmediate,
 		(XtPointer) ((OlDefine) OL_FIXEDROWS) },

   { XtNhSpace, XtCHSpace, XtRDimension, sizeof(Dimension),
      XtOffset(ControlAreaWidget, control.h_space), XtRDimension, (XtPointer) &hspace},

   { XtNvSpace, XtCVSpace, XtRDimension, sizeof(Dimension),
      XtOffset(ControlAreaWidget, control.v_space), XtRDimension, (XtPointer) &vspace},

   { XtNhPad, XtCHPad, XtRDimension, sizeof(Dimension),
      XtOffset(ControlAreaWidget, control.h_pad), XtRDimension, (XtPointer) &hpad},

   { XtNvPad, XtCVPad, XtRDimension, sizeof(Dimension),
      XtOffset(ControlAreaWidget, control.v_pad), XtRDimension, (XtPointer) &vpad},

   { XtNsameSize, XtCSameSize, XtROlDefine, sizeof(OlDefine),
      XtOffset(ControlAreaWidget, control.same_size), XtRImmediate,
 		(XtPointer) ((OlDefine) OL_COLUMNS) },

   { XtNalignCaptions, XtCAlignCaptions, XtRBoolean, sizeof(Boolean),
      XtOffset(ControlAreaWidget, control.align_captions), XtRBoolean,  (XtPointer) &aligncaptions},

   { XtNcenter, XtCCenter, XtRBoolean, sizeof(Boolean),
      XtOffset(ControlAreaWidget, control.center), XtRBoolean,  (XtPointer) &center},

   { XtNallowChangeBars, XtCAllowChangeBars, XtRBoolean, sizeof(Boolean),
      XtOffset(ControlAreaWidget, control.allow_change_bars), XtRString,  (XtPointer)"false"},
};

static XtResource constraintResources[] = {
#define offset(F) XtOffset(ControlAreaConstraints, F)

    {
	XtNchangeBar, XtCChangeBar,
	XtROlDefine, sizeof(OlDefine), offset(change_bar),
	XtRString, (XtPointer)"none"
    }

#undef offset
};

/*
 *************************************************************************
 *
 * Define Class Record structure to be initialized at Compile time
 *
 ***************************widget*class*record***************************
 */

ControlClassRec controlClassRec = {
  {
/* core_class fields      */
    /* superclass         */    (WidgetClass) &managerClassRec,
    /* class_name         */    "ControlArea",
    /* widget_size        */    sizeof(ControlRec),
    /* class_initialize   */    ClassInitialize,
    /* class_part_init    */	ControlClassPartInitialize,
    /* class_inited       */	FALSE,
    /* initialize         */    Initialize,
    /* initialize_hook    */    InitializeHook,
    /* realize            */    XtInheritRealize,
    /* actions		  */	NULL,
    /* num_actions	  */	0,
    /* resources          */    resources,
    /* num_resources      */    XtNumber(resources),
    /* xrm_class          */    NULLQUARK,
    /* compress_motion	  */	TRUE,
    /* compress_exposure  */	TRUE,
    /* compress_enterlv   */    TRUE,
    /* visible_interest   */    FALSE,
    /* destroy            */    Destroy,
    /* resize             */    Resize,
    /* expose             */    ExposeProc,
    /* set_values         */    SetValues,
    /* set_values_hook    */    NULL,
    /* set_values_almost  */    XtInheritSetValuesAlmost,
    /* get_values_hook    */	NULL,
    /* accept_focus       */    XtInheritAcceptFocus,
    /* version            */	XtVersion,
    /* callback_private   */	NULL,
    /* tm_table           */	XtInheritTranslations,
    /* query_geometry     */	QueryGeometry,
  },{
/* composite_class fields */
    /* geometry_manager   */    GeometryManager,
    /* change_managed     */    ChangeManaged,
    /* insert_child	  */	InsertChild,
    /* delete_child	  */	XtInheritDeleteChild,
    /* extension          */    NULL
  },{
/* constraint_class fields */
    /* resources	  */	constraintResources,
    /* num_resources	  */	XtNumber(constraintResources),
    /* constraint_size	  */	sizeof(ControlAreaConstraintRec),
    /* initialize	  */	ConstraintInitialize,
    /* destroy		  */	ConstraintDestroy,
    /* set_values	  */	ConstraintSetValues,
    /* extension	  */	NULL
  },{
/* manager_class fields   */
    /* highlight_handler  */	NULL,
    /* reserved		  */	NULL,
    /* reserved		  */	NULL,
    /* traversal_handler  */    NULL,
    /* activate		  */    NULL,
    /* event_procs	  */    NULL,
    /* num_event_procs	  */	0,
    /* register_focus	  */	NULL,
    /* reserved		  */	NULL,
    /* version		  */	OlVersion,
    /* extension	  */	NULL,
    /* dyn_data		  */	{ NULL, 0 },
    /* transparent_proc   */	_OlDefaultTransparentProc,
    /* query_sc_locn_proc */ 	NULL,
  },{
/* control panel class */
    /* display_layout_list */	(OlCrlAreDispLayoutLstProc) 
					MakeLayoutList,
 }	
};

/*
 *************************************************************************
 *
 * Public Widget Class Definition of the Widget Class Record
 *
 *************************public*class*definition*************************
 */

WidgetClass controlAreaWidgetClass = (WidgetClass)&controlClassRec;

/*
 *************************************************************************
 *
 * Private Procedures
 *
 ***************************private*procedures****************************
 */

/*
 *************************************************************************
 *
 * CLayout - Lay out children into w->control.measure columns
 *
 ****************************procedure*header*****************************
 */

static void
CLayout (ControlAreaWidget w, OlCLayoutList **layoutList, int layoutCount, Dimension *managerWidth, Dimension *managerHeight)
{
	OlCLayoutList **tempLayoutList;
	register int	i, j, k;
	CaptionWidget	capChild;
	Dimension *	rightList;
	Dimension *	leftList;
	Dimension *	widthList;
	Dimension *	heightList;
	Dimension	offset;
	Position	columnX, rowY;
	Dimension	widthSum;
	Dimension	hspace = w -> control.h_space;
	Dimension	vspace = w -> control.v_space;
	int		row, column;
	int 		limit = Min(layoutCount, w->control.measure);
	ChangeBar *	cb = CB(w);
	Dimension	cboffset = (cb? cb->width + cb->pad : 0);
	Position *	col_x  = (cb? w->control.col_x : 0);

/*
** Make all widgets within a column the same size?
*/
	if (w->control.same_size == OL_COLUMNS) {
		tempLayoutList =
			(OlCLayoutList **)
			XtMalloc( sizeof (OlCLayoutList *) * layoutCount);

		for (i = 0; i < w->control.measure; i++) {
			for (j=i, k=0; j < layoutCount; j += w->control.measure) {
				tempLayoutList[k++] = layoutList[j];
			}
			ForceSize(tempLayoutList,k);
		}
		XtFree((char *)tempLayoutList);
	}

	widthList = (Dimension *) XtMalloc(sizeof(Dimension) * layoutCount);
	heightList = (Dimension *) XtMalloc(sizeof(Dimension) * layoutCount);
	rightList = (Dimension *) XtMalloc(sizeof(Dimension) * layoutCount);
	leftList = (Dimension *) XtMalloc(sizeof(Dimension) * layoutCount);

	GetSizes(	widthList, heightList, rightList, leftList,
			layoutList, layoutCount, w);

/*
** Place the child widgets
*/
	columnX = (int) w->control.h_pad;
	rowY = (int) w->control.v_pad + (int) *managerHeight;

	i = 0;
	while (i < layoutCount) {
		row = (i/w->control.measure);
		column = i%w->control.measure;
/*
** are we starting a new row?
*/
		if (column == 0) {
			if (row != 0)
				rowY += (int) (heightList[row-1] + vspace);

			columnX = (int) (w->control.h_pad) + cboffset;
		}

/*
** Should we align the captions?
*/
		if (w->control.align_captions &&
			XtIsSubclass(layoutList[i]->widget,captionWidgetClass)) {
			capChild =
				(CaptionWidget) layoutList[i]->widget;

			if (capChild->caption.position == OL_LEFT)
				offset = (int) capChild->caption.caption_width;
			else if (capChild->composite.num_children)
				offset = (int) capChild->composite.children[0]->core.width;
			else
				offset = 0;
			layoutList[i]->x = (int) (
				(int) columnX +
				(int) leftList[column] -
				offset -
#if	defined(SUBTRACT_CAPTION_SPACE)
				(int) capChild->caption.space);
#else
				(int) 0);
#endif

		}

		else if (w->control.center) {
			layoutList[i]->x =  (int) (
				columnX +
				((int)(widthList[column] -
					(layoutList[i]->border +
					layoutList[i]->width)) / 2));
#ifdef oldcode	/* the code below causes ANSI C warning */
				((widthList[column] -
					(layoutList[i]->border +
					layoutList[i]->width)) >> 1));
#endif
		}

		else {
			layoutList[i]->x = columnX;
		}

		/*
		 * If we are showing change bars, save the x-location of
		 * each column (child widget location, not change bar
		 * location, for future reasons....hmmm).
		 */
		if (!row && col_x)   /* if row 0 and doing change bars */
			col_x[column] = columnX;

		CONSTRAINT(layoutList[i]->widget)->col = column;

		if (w->control.align_captions) {
			int	v1, v2;

			v1 = rightList[column] + leftList[column];
			v2 = widthList[column];
			columnX += (int)(Max(v1, v2) + hspace);
#ifdef oldcode	/* the code below causes ANSI C warning */
			columnX += (int) (
				Max(	(rightList[column] +
						leftList[column]),
					(widthList[column])) +
						(int) hspace);
#endif
		}
		else {
			columnX += (int) (widthList[column] + hspace);
		}

		columnX += cboffset;

		layoutList[i]->y = rowY;
		i++;
	}

/*
** Figure out total height and width.
*/

	for (widthSum = (2 * w->control.h_pad) - hspace, i = 0;
			i < limit;
			i++) {
		if (w->control.align_captions) {
			int	v1, v2;

			v1 = rightList[i] + leftList[i];
			v2 = widthList[i];
			widthSum += Max (v1, v2) + hspace;
#ifdef oldcode /* the code below causes ANSI C warning */
			widthSum +=
				Max (	(rightList[i] + leftList[i]),
					(widthList[i])) +
				hspace;
#endif
		}
		else {
			widthSum += (widthList[i] + hspace);
		}
		widthSum += cboffset;
	}

	*managerWidth = Max (widthSum, *managerWidth);
	*managerHeight = (int) rowY + heightList[row] + w->control.v_pad;

	XtFree((char *)widthList);
	XtFree((char *)heightList);
	XtFree((char *)leftList);
	XtFree((char *)rightList);
}

/*
 *************************************************************************
 *
 * RLayout - Lay out children into w->control.measure rows
 *
 ****************************procedure*header*****************************
 */

static void
RLayout (ControlAreaWidget w, OlCLayoutList **layoutList, int layoutCount, Dimension *managerWidth, Dimension *managerHeight)
{
	OlCLayoutList	**tempLayoutList;
	register int	i, j;
	CaptionWidget	capChild;
	Position	columnX, rowY;
    	Dimension	hspace = w -> control.h_space;
	Dimension	vspace = w -> control.v_space;
	Dimension *	widthList;
	Dimension *	heightList;
	Dimension *	leftList;
	Dimension *	rightList;
	Dimension 	offset;
	int	       	column, row;
	Dimension	heightSum;
	int 		limit = Min(layoutCount, w->control.measure);
	ChangeBar *	cb = CB(w);
	Dimension	cboffset = (cb? cb->width + cb->pad : 0);
	Position *	col_x  = (cb? w->control.col_x : 0);

/*
** Make all widgets within a column the same size?
*/
	if (w->control.same_size == OL_COLUMNS) {
		tempLayoutList =
			(OlCLayoutList **)
			XtMalloc( sizeof (OlCLayoutList *) * layoutCount);

		for (i = 0; i < layoutCount; i += w->control.measure) {
			for (j=0; j < w->control.measure; j++) {
				if (i+j >= layoutCount)
					break;
				tempLayoutList[j] = layoutList[i+j];
			}
			ForceSize(tempLayoutList,j);
		}
		XtFree((char *)tempLayoutList);
	}

	widthList = (Dimension *) XtMalloc(sizeof(Dimension) * layoutCount);
	heightList = (Dimension *) XtMalloc(sizeof(Dimension) * layoutCount);
	rightList = (Dimension *) XtMalloc(sizeof(Dimension) * layoutCount);
	leftList = (Dimension *) XtMalloc(sizeof(Dimension) * layoutCount);

	GetSizes(	widthList, heightList ,rightList, leftList,
			layoutList, layoutCount, w);

/*
** Lay out the widgets one column at a time, with measure
** rows per column.
*/

	columnX = (int) (w -> control.h_pad) + cboffset;
	rowY = (int) (*managerHeight + w->control.v_pad);

	i = 0;
	while (i < layoutCount) {
		row = i%w->control.measure;
		column = (i/w->control.measure);
/*
** are we starting a new column?
*/
		if ((row == 0) && (i != 0)) {
			if (w->control.align_captions) {
				int	v1, v2;

				v1 = rightList[column-1] + leftList[column-1];
				v2 = widthList[column-1];
				columnX += (int) (Max(v1, v2) + hspace);
#ifdef oldcode /* the code below causes ANSI C warning */
				columnX += (int) (
					Max (	(rightList[column-1] +
							leftList[column-1]),
						(widthList[column-1])) +
					hspace);
#endif
			}
			else {
				columnX += (int) (widthList[column-1] + hspace);
			}
			columnX += cboffset;
			rowY = (int) (*managerHeight + w->control.v_pad);
		}
/*
** Should we align the captions?
*/

		if (w->control.align_captions &&
			XtIsSubclass(layoutList[i]->widget,captionWidgetClass)) {
			capChild =
				(CaptionWidget) layoutList[i]->widget;

			if (capChild->caption.position == OL_LEFT)
				offset = (int) capChild->caption.caption_width;
			else if (capChild->composite.num_children)
				offset = (int) capChild->composite.children[0]->core.width;
			else
				offset = 0;

			layoutList[i]->x = (int) (
				(int) columnX +
				(int) leftList[column] -
				offset -
#if	defined(SUBTRACT_CAPTION_SPACE)
				(int) capChild->caption.space);
#else
				(int) 0);
#endif

		}


/*
** Should we center the children?
*/

		else if (w->control.center) {
			layoutList[i]->x = (int) (
				columnX +
				((int)(widthList[column] -
						(layoutList[i]->border +
						layoutList[i]->width)) / 2));
#ifdef oldcode
				(int) ((widthList[column] -
						(layoutList[i]->border +
						layoutList[i]->width)) >> 1));
#endif
		}

/*
** Default.  Just make them left-flush.
*/
		else {
			layoutList[i]->x = columnX;
		}

		layoutList[i]->y = rowY;

		/*
		 * If we are showing change bars, save the x-location of
		 * each column (child widget location, not change bar
		 * location, for future reasons....hmmm).
		 */
		if (!row && col_x)   /* if row 0 and doing change bars */
			col_x[column] = columnX;

		CONSTRAINT(layoutList[i]->widget)->col = column;

		rowY += (int) (heightList[row] + vspace);

		i++;
	}

	for (heightSum = 0, i = 0; i < limit; i++) {
		heightSum += (heightList[i] + vspace);
	}

	*managerHeight += (heightSum + (2 * w->control.v_pad) - vspace);

/*
** Total width is current x position plus width of last column.
*/
	column = (layoutCount-1) / w->control.measure;

	if (w->control.align_captions) {
		int	v1, v2;

		v1 = rightList[column] + leftList[column];
		v2 = widthList[column];
		*managerWidth = (int) (columnX +
				       Max(v1, v2) +
				       w->control.h_pad);
#ifdef oldcode /* the code below causes ANSI C warning */
				(int) Max (	(rightList[column] +
							leftList[column]),
						(widthList[column])) +
				w->control.h_pad);
#endif
	}

	else {
		*managerWidth =	(int) (
				(int) columnX +
				(int) widthList[column] +
				(int) w->control.h_pad);
	}

	XtFree((char *)widthList);
	XtFree((char *)heightList);
	XtFree((char *)leftList);
	XtFree((char *)rightList);
}


/*
 *************************************************************************
 *
 * HLayout - Lay out the children in columns, with no column taller than
 * w->control.measure.  If this is not set, default to height of
 * tallest child.
 *
 ****************************procedure*header*****************************
 */


static void
HLayout (ControlAreaWidget w, OlCLayoutList **layoutList, int layoutCount, Dimension *managerWidth, Dimension *managerHeight)
{
	register int	i, j;
	Position	columnX, rowY;
	Dimension	hspace = w -> control.h_space;
	Dimension	vspace = w -> control.v_space;
	Dimension	maxWidthForColumn = 0, maxHeight = 0;
	Dimension	columnHeight;

/*
** Find size of highest widget.
*/

	for (i = 0; i < layoutCount; i++) {
		if ((Dimension)(layoutList[i]->height + layoutList[i]->border)
			 > maxHeight)
			maxHeight = layoutList[i]->height +
					layoutList[i]->border;
	}


/*
** Lay out the widgets in columns, advancing to the next
** row when each column becomes bigger than the larger of
** measure or the tallest widget.
*/
	columnX = (int) (w->control.h_pad);
	rowY = (int) (w->control.v_pad + *managerHeight);

	{
		int	v1, v2;

		v1 = w->control.measure;
		v2 = maxHeight + 2 * w->control.v_pad;
		columnHeight = Max(v1, v2);
	}
#ifdef oldcode /* the code below causes ANSI C warning */
	columnHeight = Max (w->control.measure,
			maxHeight + 2 * w->control.v_pad);
#endif

	i = 0;
	while (i < layoutCount) {

/*
** Do we need to start a new column?
*/
#ifdef oldcode /* the code below causes ANSI C warning */
		if (	((int) rowY +
				(int) layoutList[i]->height +
				(int) layoutList[i]->border +
				(int) w->control.v_pad) >
			columnHeight)
#endif
		if (	(Dimension) (rowY +
					layoutList[i]->height +
					layoutList[i]->border +
					w->control.v_pad) >
			columnHeight) {

			rowY = (int) (w->control.v_pad + *managerHeight);
			columnX += (int) (hspace + maxWidthForColumn);
			maxWidthForColumn = layoutList[i]->width +
					layoutList[i]->border;
		}

		layoutList[i]->x = columnX;
		layoutList[i]->y = rowY;

		rowY += (int) (vspace + layoutList[i]->height +
				layoutList[i]->border);

		if (maxWidthForColumn <
			(Dimension)(layoutList[i]->width +
				    layoutList[i]->border))
			maxWidthForColumn = layoutList[i]->width +
					layoutList[i]->border;

		i++;
	}

	*managerWidth = (int) columnX + maxWidthForColumn + w->control.h_pad;
	*managerHeight += columnHeight;

}

/*
 *************************************************************************
 *
 * Lay out the children in rows, with no row wider than
 * mw->control.measure.  If this is not set, default to width of
 * fattest child.
 *
 ****************************procedure*header*****************************
 */


static void
WLayout (ControlAreaWidget w, OlCLayoutList **layoutList, int layoutCount, Dimension *managerWidth, Dimension *managerHeight)
{
	register int	i, j;
	Position	columnX, rowY;
	Dimension	hspace = w -> control.h_space;
	Dimension	vspace = w -> control.v_space;
	Dimension	maxWidgetWidth= 0, maxHeightForColumn  = 0;
	Dimension	columnWidth;

/*
** Find size of widest widget.
*/

	for (i = 0; i < layoutCount; i++) {
		if ((Dimension)(layoutList[i]->width + layoutList[i]->border) >
				maxWidgetWidth)
			maxWidgetWidth = layoutList[i]->width +
					layoutList[i]->border;
	}

/*
** Lay out the widgets, advancing to the next
** row as needed.
*/
	columnX = (int) (w->control.h_pad);
	rowY = (int) (w->control.v_pad + *managerHeight);

	{
		int	v1, v2;

		v1 = w->control.measure;
		v2 = maxWidgetWidth + 2 * w->control.h_pad;
		columnWidth = Max(v1, v2);
	}
#ifdef oldcode /* the code below causes ANSI C warning */
	columnWidth = Max (w->control.measure,
				maxWidgetWidth + 2 * w->control.h_pad);
#endif

	for (i = 0; i < layoutCount; i++) {

#ifdef oldcode /* the code below causes ANSI C warning */
		if (((int) columnX + layoutList[i]->width + layoutList[i]->border +
				w->control.h_pad) >
				columnWidth)
#endif
		if ((Dimension)(columnX +
				layoutList[i]->width + layoutList[i]->border +
				w->control.h_pad) >
				columnWidth) {
			columnX = (int) (w->control.h_pad);
			rowY += (int) (vspace + maxHeightForColumn);
			maxHeightForColumn = layoutList[i]->height +
					layoutList[i]->border;
		}

		layoutList[i]->x = columnX;
		layoutList[i]->y = rowY;

		columnX += (int) (hspace + layoutList[i]->width +
				layoutList[i]->border);

		if (maxHeightForColumn < (Dimension)(layoutList[i]->height +
				layoutList[i]->border))
			maxHeightForColumn = layoutList[i]->height +
					layoutList[i]->border;

	}

	*managerHeight = (int) rowY + maxHeightForColumn + w->control.v_pad;
	*managerWidth = Max (*managerWidth, columnWidth);
}

/*
 *************************************************************************
 *
 * FreeLayoutList - de-allocates memory for the layout list that was
 * created in Layout().
 *
 ****************************procedure*header*****************************
 */

static void
FreeLayoutList (OlCLayoutList **layoutList, register int num_children)
{
	while(num_children--) {
		XtFree((char *)layoutList[num_children]);
	}
	XtFree((char *)layoutList);
}

/*
 *************************************************************************
 *
 * ForceSize - Force the children's widths to match.
 *
 ****************************procedure*header*****************************
 */

static void
ForceSize (OlCLayoutList **layoutList, int layoutCount)
{
	register int	 i;
	Dimension	width;

	width = 0;

	for (i = 0; i < layoutCount; i++) {
		width = Max (width, layoutList[i]->width);
	}

	for (i = 0; i < layoutCount; i++) {
		layoutList[i]->width = width;
	}
}


/*
 *************************************************************************
 *
 * GetSizes - Find width of widest widget for each column, and height of
 * highest widget in each row.  In case we're aligning the captions, keep
 * track of the largest right and left halves of any caption widgets.
 * Space between caption and child is counted as part of left half.
 * Border widths of caption and child are counted in the right half.
 *
 ****************************procedure*header*****************************
 */


static void
GetSizes(Dimension *widthList, Dimension *heightList, Dimension *rightList, Dimension *leftList, OlCLayoutList **layoutList, int layoutCount, ControlAreaWidget w)
{
	int	i;
	int	row, column;
	Dimension	tempLeft, tempRight;
	Widget	child;
	CaptionWidget	capChild;

	for (i = 0; i < layoutCount; i++) {
		widthList[i] = 0;
		heightList[i] = 0;
		rightList[i] = 0;
		leftList[i] = 0;
	}

	for (i = 0; i < layoutCount; i++) {

		switch (w -> control.layouttype ) {
			case OL_FIXEDROWS:
				row = i%w->control.measure;
				column = (i/w->control.measure);
			break;

			case OL_FIXEDCOLS:
				row = (i/w->control.measure);
				column = i%w->control.measure;
			break;
		}

		child = layoutList[i]->widget;

		if (XtIsSubclass(child, captionWidgetClass)) {
			capChild = (CaptionWidget) child;
			if (capChild->composite.num_children &&
				capChild->caption.position == OL_RIGHT) {
#if	defined(SUBTRACT_CAPTION_SPACE)
				tempLeft = (capChild->composite.children[0]->core.width + capChild->caption.space);
				tempRight = capChild->caption.caption_width + layoutList[i]->border;
#else
				tempLeft = (capChild->composite.children[0]->core.width);
				tempRight = capChild->caption.caption_width + layoutList[i]->border + capChild->caption.space;
#endif
			}
			else {
#if	defined(SUBTRACT_CAPTION_SPACE)
				tempLeft = (capChild->caption.caption_width + capChild->caption.space);
				if (capChild->composite.num_children)
					tempRight = (capChild->composite.children[0]->core.width + layoutList[i]->border);
#else
				tempLeft = capChild->caption.caption_width;
				if (capChild->composite.num_children)
					tempRight = (capChild->composite.children[0]->core.width + layoutList[i]->border + capChild->caption.space);
#endif
				else
					tempRight = layoutList[i]->border;
			}
			if (leftList[column] < tempLeft)
				leftList[column] =  tempLeft;

			if (rightList[column] < tempRight)
				rightList[column] =  tempRight;
		}

		if ( (Dimension)(layoutList[i]->height +
				 layoutList[i]->border) >
				heightList[row]) {
			heightList[row] =
				layoutList[i]->height + layoutList[i]->border;
		}

		if ( (Dimension)(layoutList[i]->width +
				 layoutList[i]->border) >
				widthList[column]) {
			widthList[column] =
				layoutList[i]->width + layoutList[i]->border;
		}
	}
}

/*
 *************************************************************************
 *
 * LayoutChildren -  Calculate the positions of each managed child and
 * try to resize yourself to fit.  Return True if the resize is okay,
 * False otherwise.
 *
 * Don't actually move the children unless doit is True.
 * Don't resize yourself unless resizeok is True.
 *
 ****************************procedure*header*****************************
 */


static Boolean
LayoutChildren (ControlAreaWidget mw, Boolean doit, Boolean resizeok)
{
	OlCLayoutList	**layoutList;
	int		layoutCount = 0;
	Widget		child;
	register int	i;
	Dimension	managerWidth,
			managerHeight,
			requestWidth,
			requestHeight,
			replyWidth,
			replyHeight;

	Boolean		moveFlag        = False;
	Boolean		resizeFlag      = False;
	Boolean		control_resized = False;
	Boolean         return_value    = True;
	int		num_configured  = 0;

/*
** Allocate a layout list to use in layout processing, then
** loop through the children to get their preferred sizes.
*/
	layoutList =
		(OlCLayoutList **) XtMalloc( sizeof (OlCLayoutList *) *
			mw->composite.num_children);

	for (i = 0; i < mw -> composite.num_children; i++) {
		layoutList[i] = (OlCLayoutList *)
			XtMalloc (sizeof (OlCLayoutList));
		layoutList[i]->widget = NULL;
		child = mw -> composite.children[i];

		if (child -> core.managed) {
			layoutList[layoutCount]->widget = child;
			layoutList[layoutCount]->x = 0;
			layoutList[layoutCount]->y = 0;
			layoutList[layoutCount]->width = child->core.width;
			layoutList[layoutCount]->height = child->core.height;
			layoutList[layoutCount]->border =
					child -> core.border_width * 2;
			layoutCount++;
		}
	}

	if (layoutCount == 0)  {
		FreeLayoutList(layoutList, mw->composite.num_children);
		return(True);	/*  no managed children  */
	}

	if (mw->control.same_size == OL_ALL) {
		ForceSize (layoutList, layoutCount);
	}

	managerWidth = managerHeight = 0;

	controlClassRec.control_class.display_layout_list(
			mw, layoutList, layoutCount,
			&managerWidth, &managerHeight);

/*
** We now have an array with the set of minimum position for each
** widget.  We also have a manager width and height needed for
** this layout.
** First see if a geometry request needs to be made.  If so, and if
** resizing is okay, make it.
*/

	requestWidth = managerWidth;
	requestHeight = managerHeight;

	if (	resizeok &&
		(requestWidth != mw->core.width ||
		requestHeight != mw->core.height)) {
		switch (XtMakeResizeRequest (	(Widget) mw,
						requestWidth, requestHeight,
				                &replyWidth, &replyHeight)) {
		case XtGeometryYes:
			control_resized = True;
			return_value = True;
			break;

		case XtGeometryNo:
/*
** This picks up the case where parent answers "No" but sets replyHeight and
** replyWidth to what I asked for.  Don't ask why; I don't understand either.
*/

			if ((replyWidth <= mw->core.width) &&
	    			(replyHeight <= mw->core.height)) {
	    			return_value = True;
			}
			else
				return_value = False;

		case XtGeometryAlmost:
			XtMakeResizeRequest (	(Widget) mw,
						replyWidth, replyHeight,
						&replyWidth, &replyHeight);
			return_value = True;
			control_resized = True;
			break;
		}
	}

/*
** Are we really doing the layout?  If not, just return True to show
** that it would be possible.
*/

	if (!doit) {
		FreeLayoutList(layoutList, mw->composite.num_children);
		return(return_value);
	}

/*
** Relayout the children, making only necessary layout changes.
*/

	_OlDnDSetDisableDSClipping((Widget)mw , True);
	for (i = 0; i < layoutCount; i++) {
		moveFlag = resizeFlag = False;

		if (layoutList[i]->x != layoutList[i]->widget->core.x ||
		    layoutList[i]->y!=layoutList[i]->widget->core.y)
			moveFlag = True;

		if (layoutList[i]->width != layoutList[i]->widget->core.width ||
		    layoutList[i]->height != layoutList[i]->widget->core.height)
			resizeFlag = True;

		if (moveFlag && resizeFlag)
			OlConfigureWidget (layoutList[i]->widget,
					layoutList[i]->x,
					layoutList[i]->y,
					layoutList[i]->width,
					layoutList[i]->height,
					layoutList[i]->widget->core.border_width);
		else if (moveFlag)
			OlMoveWidget (layoutList[i]->widget,
					layoutList[i]->x, layoutList[i]->y);
		else if (resizeFlag)
			OlResizeWidget (layoutList[i]->widget,
					layoutList[i]->width,
					layoutList[i]->height,
					layoutList[i]->widget->core.border_width);
		if (moveFlag || resizeFlag) num_configured++;
	}
	_OlDnDSetDisableDSClipping((Widget)mw , False);
	if (control_resized || num_configured > 0)
		OlDnDWidgetConfiguredInHier((Widget)mw);

   FreeLayoutList(layoutList, mw->composite.num_children);
   return (return_value);
}

/*
 *************************************************************************
 *
 * MakeLayoutList - Switch on the type of layout and fill the widget's
 * layout list appropriately.  Return the required height and width in
 * pmanagerHeight and pmanagerWidth.
 *
 ****************************procedure*header*****************************
 */

static void
MakeLayoutList (ControlAreaWidget mw, OlCLayoutList **layoutList, int layoutCount, Dimension *pmanagerWidth, Dimension *pmanagerHeight)
{
	/*
	 * If change bars are being displayed, allocate an array
	 * large enough to hold the x-position of each column.
	 * We do this only for row-major or column-major
	 * layout types.
	 *
	 * Note: This array may have already been allocated, so check
	 * its previous size and reallocate if different. THIS TAKES
	 * CARE OF HANDLING A CHANGE IN XtNMeasure IN XtSetValues()!
	 */
	if (CHANGEBARS(mw)) {
		ChangeBar *		cb = CB(mw);

		Cardinal		ncolumns;


		switch (mw -> control.layouttype) {
		case OL_FIXEDCOLS:
			ncolumns = mw->control.measure;
			goto Alloc;
		case OL_FIXEDROWS:
			ncolumns = (mw->composite.num_children + mw->control.measure-1)
				 / mw->control.measure;
			if (ncolumns < 1)
				ncolumns = 1;
Alloc:			if (mw->control.ncolumns != ncolumns) {
				mw->control.ncolumns = ncolumns;
				mw->control.col_x = (Position *)Array(
					(char *)mw->control.col_x,
					Position,
					mw->control.ncolumns
				);
			}
			break;
		}
	}

	switch ( mw -> control.layouttype ) {

	case OL_FIXEDROWS:
		RLayout (mw, layoutList, layoutCount,
				pmanagerWidth, pmanagerHeight);
		break;

	case OL_FIXEDCOLS:
		CLayout (mw, layoutList, layoutCount,
				pmanagerWidth, pmanagerHeight);
		break;

	case OL_FIXEDHEIGHT:
		HLayout (mw, layoutList, layoutCount,
				pmanagerWidth, pmanagerHeight);
		break;

	case OL_FIXEDWIDTH:
		WLayout (mw, layoutList, layoutCount,
				pmanagerWidth, pmanagerHeight);
		break;
	}
}

/*
 *************************************************************************
 *
 * Class Procedures
 *
 ****************************class*procedures*****************************
 */

/*
 *************************************************************************
 *
 * ClassInitialize - Register OlDefine string values.
 *
 ****************************procedure*header*****************************
 */

static void
ClassInitialize(void)
{
	_OlAddOlDefineType ("fixedrows",   OL_FIXEDROWS);
	_OlAddOlDefineType ("fixedcols",   OL_FIXEDCOLS);
	_OlAddOlDefineType ("fixedwidth",  OL_FIXEDWIDTH);
	_OlAddOlDefineType ("fixedheight", OL_FIXEDHEIGHT);
	_OlAddOlDefineType ("none",        OL_NONE);
	_OlAddOlDefineType ("columns",     OL_COLUMNS);
	_OlAddOlDefineType ("all",         OL_ALL);
}

/*
 *************************************************************************
 *
 * Resize - Lay out the children, not allowing resizing.
 *
 ****************************procedure*header*****************************
 */

static void
Resize (Widget w)
{
    ControlAreaWidget control_area = (ControlAreaWidget) w;
    
    (void) LayoutChildren (control_area, True, False);
}


/****************************procedure*header*****************************
 * Destroy-
 */
static void
Destroy(Widget widget)
{
    ControlAreaWidget control_area = (ControlAreaWidget)widget;
    
    if (CHANGEBARS(control_area))
	_OlDestroyChangeBar (widget, CB(control_area));
    if (control_area->control.post_select)
        XtFree(control_area->control.post_select);

}


/**
 ** ExposeProc()
 **/

static void
ExposeProc (Widget w, XEvent *pe, Region region)
{
    ControlAreaWidget control_area = (ControlAreaWidget)w;
    Cardinal		i;


	/*
	 * This exposure routine is used to display change bars,
	 * but only if we can handle them.
	 */
	if (!CHANGEBARS(control_area))
		return;

	/*
	 * Expose each change bar, if inside the exposure region.
	 */
	for (i = 0; i < control_area->composite.num_children; i++) {
		Widget	child = control_area->composite.children[i];

		ControlAreaConstraintRec * constraint = CONSTRAINT(child);

		if (XtIsManaged(child) && constraint->change_bar != OL_NONE)
			_OlDrawChangeBar (
				(Widget)control_area,
				CB(control_area),
				constraint->change_bar,
				False,
				control_area->control.col_x[constraint->col]
					- _OlChangeBarSpan(CB(control_area)),
				child->core.y,
				region
			);
	}

	return;
} /* ExposeProc */


/*
 *************************************************************************
 *
 * ChangeManaged - Lay out the children, allowing resizing.
 *
 ****************************procedure*header*****************************
 */

static void
ChangeManaged (Widget w)
{
    ControlAreaWidget	cw = (ControlAreaWidget)w;
    Boolean		tellEveryone = False, menu_has_meta; 
    Widget		shell, child;
    int			i;

    shell = _OlGetShellOfWidget(w);
    if (shell && XtIsSubclass(shell, menuShellWidgetClass)) {
	/* if we're in a MenuShell, we need to check, before laying out
	 * children, to see if any buttons on the Menu have a meta symbol;
	 * if so, all Menu buttons need to know about this so they can
	 * size themselves accordingly.
	 */
        for (i = 0; i < cw->composite.num_children; i++) {
            child = cw->composite.children[i];

            if ( XtIsSubclass(child, buttonWidgetClass) ||
                 XtIsSubclass(child, buttonGadgetClass) ) {

                XtVaGetValues(child, XtNmenuHasMeta, &menu_has_meta, NULL);
                if (menu_has_meta)
                    tellEveryone = True;
            }
        }
        if (tellEveryone) {
            for (i = 0; i < cw->composite.num_children; i++) {
                child = cw->composite.children[i];
                if ( XtIsSubclass(child, buttonWidgetClass) ||
                     XtIsSubclass(child, buttonGadgetClass) )

                    XtVaSetValues(child, XtNmenuHasMeta, True, NULL);
            }
        }
    }

    (void) LayoutChildren ((ControlAreaWidget)w, True, True);
}

/*
 *************************************************************************
 *
 * Initialize
 *
 ****************************procedure*header*****************************
 */

/* ARGSUSED */
static void
Initialize(Widget request, Widget new, ArgList args, Cardinal *num_args)
{
    ControlAreaWidget new_ca = (ControlAreaWidget) new;
    
	if ((int) new_ca->control.h_space < 0) {
		OlWarning (dgettext(OlMsgsDomain,
			"Control: Invalid h_space setting."));
		new_ca->control.h_space = 4;
	}

	if ((int) new_ca->control.v_space < 0) {
		OlWarning (dgettext(OlMsgsDomain,
			"Control: Invalid v_space setting."));
		new_ca->control.v_space = 4;
	}

	if ((int) new_ca->control.h_pad < 0) {
		OlWarning (dgettext(OlMsgsDomain,
			"Control: Invalid h_pad setting."));
		new_ca->control.h_pad = 4;
	}

	if ((int) new_ca->control.v_pad < 0) {
		OlWarning (dgettext(OlMsgsDomain,
			"Control: Invalid v_pad setting."));
		new_ca->control.v_pad = 4;
	}

	if ((new_ca->control.layouttype == OL_FIXEDHEIGHT) ||
			(new_ca->control.layouttype == OL_FIXEDWIDTH)) {
		if (new_ca->control.measure <= 0) {
			OlWarning (dgettext(OlMsgsDomain,
				"Control: Invalid measure setting."));
			new_ca->control.measure = 1;
		}
	}
	else {
		if ((new_ca->control.layouttype != OL_FIXEDROWS) &&
				(new_ca->control.layouttype != OL_FIXEDCOLS)) {
			OlWarning (dgettext(OlMsgsDomain,
				"Control: Invalid layout, using OL_FIXEDROWS"));
			new_ca->control.layouttype = OL_FIXEDROWS;
		}
		if (new_ca->control.measure <= 0) {
			OlWarning (dgettext(OlMsgsDomain,
				"Control: Invalid measure setting."));
			new_ca->control.measure = 1;
		}
	}


	if (new_ca->core.width == 0)
		new_ca->core.width =
		((new_ca->control.h_space != 0) ? new_ca->control.h_space : 10);
	if (new_ca->core.height == 0)
		new_ca->core.height =
		((new_ca->control.v_space != 0) ? new_ca->control.v_space : 10);

	if (	(	new_ca->control.layouttype == OL_FIXEDHEIGHT
		     || new_ca->control.layouttype == OL_FIXEDWIDTH
		)
	     && CHANGEBARS(new_ca)
	) {
		OlWarning (dgettext(OlMsgsDomain,
			"Control: Can't have change bars when layout type is OL_FIXEDHEIGHT or OL_FIXEDWIDTH"));
		CHANGEBARS(new_ca) = False;
	}

	if (CHANGEBARS(new_ca))
		CB(new_ca) = _OlCreateChangeBar((Widget)new_ca);
	else
		CB(new_ca) = 0;

	new_ca->control.col_x    = 0;
	new_ca->control.ncolumns = 0;
} /* Initialize */

/*
 *************************************************************************
 *
 * InitializeHook
 *
 ****************************procedure*header*****************************
 */

static void
InitializeHook (Widget w, ArgList args, Cardinal *num_args)
{
	ControlAreaWidget	cw = (ControlAreaWidget)w;
	static XtCallbackList	list;
	static MaskArg		marg[] = {
		{ XtNpostSelect, (XtArgVal) &list, OL_COPY_SOURCE_VALUE },
		{ NULL, (XtArgVal) sizeof(XtCallbackList), OL_COPY_SIZE }
		};

	list = NULL;
	cw->control.post_select = NULL;

	/* We don't need to supply a count or a return list pointer
	 * since this rule does not generate a return list
	 */
	_OlComposeArgList(args, *num_args, marg, XtNumber(marg), NULL, NULL);

	/* save caller's callback list in widget struct.  this will be passed
	 * to button children (InsertChild)
	 */
	if (list != NULL) {
		int i;
		XtCallbackList cb;

		for (i=0; list[i].callback != NULL; i++)
			;			/* no-op for counting */
		
		i++;				/* for NULL delimiters */
		cb = (XtCallbackList) XtMalloc(i * sizeof (XtCallbackRec));
		cw->control.post_select = cb;

		while (i--) {
			cb[i].callback	= list[i].callback;
			cb[i].closure	= list[i].closure;
		}
	}
}

/*
 *************************************************************************
 *
 * SetValues - real stupid
 *
 ****************************procedure*header*****************************
 */

/* ARGSUSED */
static Boolean
SetValues(Widget current, Widget request, Widget new,
	  ArgList args, Cardinal *num_args)
{
    ControlAreaWidget new_ca = (ControlAreaWidget) new;
    ControlAreaWidget current_ca = (ControlAreaWidget) current;
    Boolean			ret		= False;


	if (	CHANGEBARS(new_ca)
	     && (	new_ca->control.layouttype == OL_FIXEDHEIGHT
		     || new_ca->control.layouttype == OL_FIXEDWIDTH
		)
	) {
		OlWarning (dgettext(OlMsgsDomain,
			"Control: Can't have change bars when layout type is OL_FIXEDHEIGHT or OL_FIXEDWIDTH"));
		CHANGEBARS(new_ca) = False;
	}

	/*
	 * If the XtNchangeBars resource has changed, either we have
	 * to create new auxiliary structures for the widget or we
	 * have to destroy the structures.
	 */
	if (CHANGEBARS(new_ca) != CHANGEBARS(current_ca)) {
		if (!CHANGEBARS(new_ca))
			_OlDestroyChangeBar ((Widget)new_ca, CB(new_ca));
		else
			CB(new_ca) = _OlCreateChangeBar((Widget)new_ca);
	}

	/*
	 * If the background color has changed, we have to create new GCs.
	 * We don't actually get the GCs here, just free the current
	 * ones (if any) and clear the references to GCs. When we
	 * really need the GCs we'll get them.
	 */
         if (CHANGEBARS(new_ca) &&
	    new_ca->core.background_pixel != current_ca->core.background_pixel)
		_OlFreeChangeBarGCs ((Widget)new_ca, CB(new_ca));

	/*
	 * If anything affecting the layout of the children has changed,
	 * lay them out (again).
	 */
	if (	new_ca->control.layouttype != current_ca->control.layouttype
	     || new_ca->control.measure != current_ca->control.measure
         || new_ca->control.center != current_ca->control.center
         || new_ca->control.h_pad != current_ca->control.h_pad
         || new_ca->control.h_space != current_ca->control.h_space
         || new_ca->control.same_size != current_ca->control.same_size
         || new_ca->control.v_pad != current_ca->control.v_pad
         || new_ca->control.v_space != current_ca->control.v_space
	     || CHANGEBARS(new_ca) != CHANGEBARS(current_ca)
	) {
		LayoutChildren (new_ca, True, True);
		ret = True;
	}

	return (ret);
}

/*
 *************************************************************************
 *
 * ControlClassPartInitialize - ensure that any widgets subclassed off
 * control area can inherit the layout routines.
 *
 ****************************procedure*header*****************************
 */

static void
ControlClassPartInitialize(WidgetClass widgetClass)
{
	ControlAreaWidgetClass wc =
		(ControlAreaWidgetClass) widgetClass;
	ControlAreaWidgetClass super =
		(ControlAreaWidgetClass) wc->core_class.superclass;

	if (wc->control_class.display_layout_list ==
		    XtInheritDisplayLayoutList) {
		wc->control_class.display_layout_list =
				super->control_class.display_layout_list;
	}
}

/*
 *************************************************************************
 *
 * QueryGeometry
 *
 ****************************procedure*header*****************************
 */

static XtGeometryResult
QueryGeometry (Widget widget,
	       XtWidgetGeometry *	constraint,
	       XtWidgetGeometry *	preferred)
{
	/*
	 * MORE: SHOULD QUERY OUR CHILDREN ABOUT THIS.
	 */
	return (XtGeometryNo);
}

/*
 *************************************************************************
 *
 * Geometry Manager
 *
 ****************************procedure*header*****************************
 */

static XtGeometryResult
GeometryManager(Widget w, XtWidgetGeometry *request, XtWidgetGeometry *reply)
{
	Dimension		width, height, borderWidth;
	ControlAreaWidget	cw;
	Boolean			tellEveryone = False, menu_has_meta; 
	Widget			shell, child;
	int			i;


/*
** Children aren't allowed to specify their position
*/
	if ((request->request_mode & CWX && request->x != w->core.x) ||
		(request->request_mode & CWY && request->y != w->core.y))
		return (XtGeometryNo);

/*
** Ensure all three fields in the request are valid
*/
	if (request->request_mode & (CWWidth | CWHeight | CWBorderWidth)) {
		if ((request->request_mode & CWWidth) == 0)
			request->width = w->core.width;
		if ((request->request_mode & CWHeight) == 0)
			request->height = w->core.height;
		if ((request->request_mode & CWBorderWidth) == 0)
			request->border_width = w->core.border_width;
	}

/*
** Save current size and try new size
*/
	width = w->core.width;
	w->core.width = request->width;
	height = w->core.height;
	w->core.height = request->height;
	borderWidth = w->core.border_width;
	w->core.border_width = request->border_width;

	cw = (ControlAreaWidget) w->core.parent;
/*
** See if it's possible to lay out the children now.
** If not, reset core values and return No.  Else, really do the
** layout and return Yes.
*/
	if (LayoutChildren(cw,False,True) == False) {
		w->core.width = width;
		w->core.height = height;
		w->core.border_width = borderWidth;
		return (XtGeometryNo);
	}

/*
	(*XtClass((Widget)cw)->core_class.resize)((Widget)cw);
*/

	shell = _OlGetShellOfWidget(w);
	if (shell && XtIsSubclass(shell, menuShellWidgetClass)) {
	    /* if we're in a MenuShell, we need to check, before laying out
	     * children, to see if any buttons on the Menu have a meta symbol;
	     * if so, all Menu buttons need to know about this so they can
	     * size themselves accordingly.
	     */
	    for (i = 0; i < cw->composite.num_children; i++) {
		child = cw->composite.children[i];

		if ( XtIsSubclass(child, buttonWidgetClass) ||
		     XtIsSubclass(child, buttonGadgetClass) ) {

		    XtVaGetValues(child, XtNmenuHasMeta, &menu_has_meta, NULL);
		    if (menu_has_meta)
			tellEveryone = True;
		}
	    }
	    if (tellEveryone) {
		for (i = 0; i < cw->composite.num_children; i++) {
		    child = cw->composite.children[i];
		    if ( XtIsSubclass(child, buttonWidgetClass) ||
			 XtIsSubclass(child, buttonGadgetClass) )

			XtVaSetValues(child, XtNmenuHasMeta, True, NULL);
		}
	    }
	}
	LayoutChildren(cw,True,True);
	OlWidgetConfigured(w);
	return (XtGeometryYes);
}

/*
 *************************************************************************
 *
 * InsertChild - If the control area has its post-select resource set,
 * and the child is a button, add these callbacks to the child's post-select.
 *
 * Call the superclass's insert-child.
 *
 ****************************procedure*header*****************************
 */

static void
InsertChild (w)
	Widget w;
{
    ControlAreaWidget	cw = (ControlAreaWidget) XtParent(w);
    XtWidgetProc	insert_child = ((CompositeWidgetClass)
	(controlClassRec.core_class.superclass))->composite_class.insert_child;

    if ((cw->control.post_select != (XtCallbackList) NULL) &&
	(XtHasCallbacks(w, XtNpostSelect) != XtCallbackNoList))
	XtAddCallbacks(w, XtNpostSelect, cw->control.post_select);

    if (insert_child)
	(*insert_child)(w);
}

/**
 ** ConstraintInitialize()
 **/

static void
ConstraintInitialize (Widget request, Widget new,
		      ArgList args, Cardinal *num_args)
{
    /* empty */
} /* ConstraintInitialize */

/**
 ** ConstraintDestroy()
 **/

static void
ConstraintDestroy (
	Widget			w
)
{
#if	defined(NEED_THIS)
	ControlAreaConstraintRec *	constraint = CONSTRAINT(w);
	return;
#endif
} /* ConstraintDestroy */

/**
 ** ConstraintSetValues()
 **/

static Boolean
ConstraintSetValues (Widget current, Widget request, Widget new,
		     ArgList args, Cardinal *num_args)
{
	ControlAreaWidget	parent = (ControlAreaWidget)XtParent(new);

	ControlAreaConstraintRec * curConstraint = CONSTRAINT(current);
	ControlAreaConstraintRec * newConstraint = CONSTRAINT(new);

	Boolean			needs_redisplay	 = False;


	/*
	 * We play a subtle trick here: We aren't supposed to do any
	 * (re)displaying in this routine, but should just return a
	 * Boolean that indicates whether the Intrinsics should force
	 * a redisplay. But the Intrinsics do this by ``calling the
	 * Xlib XClearArea() function on the [parent] widget's window.''
	 * Instead of returning True here, which will cause the entire
	 * ControlArea to be redisplayed, we instead call XClearArea
	 * ourselves on just the area of the change bar and return False.
	 * The XClearArea() (done inside DrawChangeBar()) will generate
	 * a much smaller expose event. Now this will cause another call
	 * to DrawChangeBar() for the same change bar, but that can't
	 * be helped--we don't know what other events might have occurred,
	 * so we can't do the real drawing here.
	 */
	if (
		CHANGEBARS(parent)
	     && newConstraint->change_bar != curConstraint->change_bar
	)
		_OlDrawChangeBar (
			(Widget)parent,
			CB(parent),
			newConstraint->change_bar,
			True,
			parent->control.col_x[newConstraint->col]
					- _OlChangeBarSpan(CB(parent)),
			new->core.y,
			(Region)0
		);

	return (needs_redisplay);
} /* ConstraintSetValues */


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

