#pragma ident	"@(#)TextLine.c	1.24	97/03/26 lib/libXol SMI"	/* OLIT	*/

/*
 *        Copyright (C) 1986,1991  Sun Microsystems, Inc
 *                    All rights reserved.
 *          Notice of copyright on this source code 
 *          product does not indicate publication. 
 * 
 * RESTRICTED RIGHTS LEGEND: Use, duplication, or disclosure by 
 * the U.S. Government is subject to restrictions as set forth 
 * in subparagraph (c)(1)(ii) of the Rights in Technical Data
 * and Computer Software Clause at DFARS 252.227-7013 (Oct. 1988) 
 * and FAR 52.227-19 (c) (June 1987).
 *
 *    Sun Microsystems, Inc., 2550 Garcia Avenue,
 *    Mountain View, California 94043.
 *
 */

#include 	<widec.h>
#include 	<libintl.h>
#include 	<string.h>
#include	<stdlib.h>
#include 	<wctype.h>

#include 	<X11/IntrinsicP.h>
#include 	<X11/StringDefs.h>
#include	<X11/Xatom.h>
#include 	<X11/cursorfont.h>
#include 	<X11/keysym.h>

#include	<Xol/Dynamic.h>
#include	<Xol/OpenLookP.h>
#include	<Xol/OlCursors.h>
#include	<Xol/OlDnDVCX.h>
#include	<Xol/OlI18nP.h>
#include	<Xol/OlStrMthdsI.h>
#include	<Xol/OlIm.h>
#include	<Xol/OlgxP.h>
#include	<Xol/RootShell.h>
#include	<Xol/TextUtil.h>
#include	<Xol/TextWrap.h>	/* for _StringWidth .. */
#include	<Xol/Menu.h>		/* for PopupMenu .. */
#include	<Xol/OblongButt.h>

#include 	<Xol/TextLineP.h>
#include 	<Xol/TextLineI.h>
#include        <Xol/TextLBuffI.h>
#include 	<Xol/TextLPreEdI.h>


/* Gap : gap between scrollbuttons & text-entryline . Ref OL spec Fig B-53 */
#define Gap(scale)		((float)scale/5.0 - 0.4)

#define DGETTEXT(string)	dgettext(OlMsgsDomain, string)

#define	Max3(a,b,c)		((a)>(b) ? ((a)>(c) ? (a):(c)) : ((b)>(c) ? (b):(c)))
#define String(tlp)		(OlStr)(tlp->buffer->p)
#define	PreEdString(tlp)	(OlStr)(tlp->preed_buffer->p)

#define	LeftArrowWidth(tlp)	(tlp->leftarrow_present ?  \
				 TextScrollButton_Width(tlp->pAttrs->ginfo) : 0)

#define	RightArrowWidth(tlp)	(tlp->rightarrow_present ?  \
				 TextScrollButton_Width(tlp->pAttrs->ginfo) : 0)

#define	LabelWidth(tlp)		(tlp->caption_width ? 	\
					(tlp->caption_width + tlp->caption_space) :0)

#define TextStartPos(tlp) 	(LabelWidth(tlp) + LeftArrowWidth(tlp) + tlp->gap)
#define TextEndPos(w,tlp) 	(tlp->real_width - RightArrowWidth(tlp) - tlp->gap)
#define LeftArrowRequired(tlp)	(tlp->char_offset != 0)

#define RightArrowRequired(w, tlp) \
				((int) (DisplayPositionOfString(w, 	\
				 _OLTLStartOfDisplay(tlp),  \
				 tlp->num_chars + tlp->num_preed_chars ) \
				 + TextStartPos(tlp)) > (int)(tlp->real_width - tlp->gap))

#define	UNDERLINE_HEIGHT	2	

/* Opcodes to  SetupGCsAndCursor ... */
#define	NORMAL			(1L)
#define	INVERSE			(1L << 1)
#define	CAPTION			(1L << 2)
#define	CARET			(1L << 3)
#define	GINFO			(1L << 4)

/* Indices into the GC array  */
#define NORM_GC                 0
#define INV_GC                  1
#define CAPTION_GC		2

/* Scroll Directions */
#define SCROLL_LEFT 		1
#define SCROLL_RIGHT 		2

#define	BUF_SIZE		10	/* This should be Tuned to an appropriate value */

/****************************************************************
		Private Function Declarations .... 
 ****************************************************************/

Private void 	ValidateResources(TextLinePart *tlp, TextLinePart *old_tlp);
Private void 	SetupGCsAndCursor(TextLineWidget w, unsigned long what);
Private void 	InheritBackground(Widget w, int offset, XrmValue *value);

Private Boolean SetString(TextLineWidget w, 
			  OlStr string, 
			  OlTLSetStringHints hints,
			  Boolean cursor
			 );
Private OlStr 	GetString(TextLineWidget w);

Private Boolean	KeepDisplayCursorInView(TextLineWidget w);

Private Boolean	TextLineEditString(TextLineWidget w, 
				   int start,int end, 
				   OlStr string, XEvent *ev
				  );

Private Boolean InvokeMotionCallbacks(TextLineWidget w, 
				      int current, int new, 
				      XEvent *ev, Boolean flag
				     );
Private Boolean InvokeCommitCallbacks(TextLineWidget w, XEvent *ev);

Private int 	toUnitPos(OlStr str, int pos, OlStrRep format, PositionTable *posTable);

Private int 	DisplayPositionOfString(TextLineWidget w, int start, int end);

Private void 	DrawText(TextLineWidget w, int pos, int length, int x, int y, int max_x);
Private int 	DrawTextMB(TextLineWidget w, OlStr str, 
			   int pos, int length, 
			   int x, int y, int max_x, int type
			  );
Private int 	DrawTextWC(TextLineWidget w, OlStr str, 
			   int pos, int length, 
			   int x, int y,int max_x, int type
			  );
Private int 	DrawTextSB(TextLineWidget w, OlStr str, 
			   int pos, int length, 
			   int x, int y,int max_x, int type
			  );

Private void 	DrawLabel(TextLineWidget w);
Private void 	DrawCaret(TextLineWidget w);

Private int 	CharWidthSB(int x, char *p, OlFont font);
Private int 	CharWidthWC(int x, wchar_t ch, OlFont font);
Private int 	CharWidth(OlStr str, int pos, OlFont font, OlStrRep format);

Private int 	ConvertForwardToPos(TextLineWidget w, int start, int width);
Private int 	ConvertBackwardToPos(TextLineWidget w, int start, int width);

Private void 	BlinkCursor(XtPointer client_data, XtIntervalId *id);

Private void 	ScrollField(TextLineWidget w,int direction,unsigned long delay);
Private void 	Scroller(XtPointer client_data, XtIntervalId *id);

Private void 	Key(Widget w, OlVirtualEvent ve);
Private Boolean EventHandler(TextLineWidget w, OlVirtualName olvn, XEvent *xev);
Private void 	ButtonDown(Widget w, OlVirtualEvent ve);
Private void 	ButtonUp(Widget w, OlVirtualEvent ve);
Private void 	ButtonHandler(TextLineWidget w, OlVirtualEvent ve);

Private void 	CreateMenu(TextLineWidget w);
Private void 	MenuUndo(Widget w, XtPointer client_data, XtPointer call_data);
Private void 	MenuCut(Widget w, XtPointer client_data, XtPointer call_data);
Private void 	MenuCopy(Widget w, XtPointer client_data, XtPointer call_data);
Private void 	MenuPaste(Widget w, XtPointer client_data, XtPointer call_data);
Private void 	MenuDelete(Widget w, XtPointer client_data, XtPointer call_data);
Private void 	SetSensitivity(TextLineWidget w);

Private void 	Delete(TextLineWidget w, OlVirtualName olvn, XEvent *xev);
Private void 	MoveCursor(TextLineWidget w, OlVirtualName olvn, XEvent *xev);


Private void 	Select(TextLineWidget w, XEvent *ev);
Private void 	Adjust(TextLineWidget w, XEvent *ev);
Private void 	WipeThru(XtPointer client_data, XtIntervalId *id);
Private void 	ExtendSelection(TextLineWidget w, int pos);
Private void 	SetSelection(TextLineWidget w, int pos);

Private Boolean inWordSB(OlStr str, int pos);
Private Boolean inWordMB(OlStr str, int pos);
Private Boolean inWordWC(OlStr str, int pos);

Private int 	StartOfCurrentWord(TextLineWidget w, int pos);
Private int 	EndOfCurrentWord(TextLineWidget w, int pos);
Private int 	StartOfPreviousWord(TextLineWidget w, int pos);
Private int 	EndOfPreviousWord(TextLineWidget w, int pos);
Private int 	EndOfNextWord(TextLineWidget w, int pos);


/* Selection & DnD related functions ... */

Private void 	GetPrimary(Widget w, XtPointer client_data, 
			   Atom *selection, 
			   Atom *type, XtPointer value, 
			   unsigned long *length, 
			   int *format
			  );
Private Boolean ConvertPrimary(Widget w, Atom *selection, 
			       Atom *target, Atom *type_return,  
			       XtPointer *value_return, 
			       unsigned long *length_return,
			       int *format_return
			      );
Private void 	LosePrimary(Widget w, Atom *atom);

Private void 	GetClipboardOrDnD(Widget w, XtPointer client_data, 
				  Atom *selection, 
				  Atom *type,XtPointer value, 
				  unsigned long *length,
				  int *format
				 );
Private Boolean ConvertClipboardOrDnD(Widget w, Atom *selection, 
				      Atom *target, Atom *type_return,
				      XtPointer *value_return, 
				      unsigned long *length_return, 
				      int *format_return
			      	     );
Private void 	LoseClipboardOrDnD(Widget w, Atom *atom);

Private void 	Paste(TextLineWidget w);
Private void 	AckDel(Widget w, XtPointer client_data, 
		       Atom *selection, Atom *type, 
		       XtPointer value,long unsigned int *length,
		       int *format
		      );
Private void 	SelectTarget(Widget w, XtPointer client_data, 
			    Atom *selection, 
			    Atom *type, XtPointer value, 
			    unsigned long *length, 
			    int *format
			   );
Private void 	dummy();

Private void 	DragText(TextLineWidget w, int position, OlDragMode drag_mode);
Private void 	TextDropOnWindow(Widget w, Window drop_window, 
				 Position x, Position y,
		 		 OlDragMode drag_mode, 
				 OlDnDDragDropInfoPtr rinfo
				);

Private void 	TriggerNotify(Widget w, Window win,
			      Position x, Position y,
			      Atom selection, Time timestamp, 
			      OlDnDDropSiteID drop_site, 
			      OlDnDTriggerOperation op, 
			      Boolean send_done, 
			      Boolean forwarded, 
			      XtPointer closure
			     );

Private void 	CleanupTransaction(Widget w, 
				   Atom selection, 
				   OlDnDTransactionState state, 
		   		   Time timestamp,
				   XtPointer closure
				  );

/*******************************************************************
			Class Methods ... 
 *******************************************************************/

ClassMethod void 	ClassInitialize(void);
ClassMethod void 	ClassPartInitialize(WidgetClass class);
ClassMethod void 	Initialize(Widget req, 
				   Widget new, 
				   ArgList args, 
				   Cardinal *num_args
				  );

ClassMethod void 	Realize(Widget w, 
				Mask *valueMask, 
				XSetWindowAttributes *attributes
			       );

ClassMethod void 	Redisplay(Widget w, XEvent *event, Region region);

ClassMethod void 	Resize(Widget w);

ClassMethod void 	FocusHandler(Widget w, OlDefine highlight_type);

ClassMethod Boolean 	SetValues(Widget current,
				  Widget request, 
				  Widget new,
				  ArgList args,
				  Cardinal * num_args
				 );
ClassMethod void 	SetValuesAlmost(Widget old, 
					Widget new, 
					XtWidgetGeometry *req, 
					XtWidgetGeometry *reply
				       );

ClassMethod Boolean  	ActivateWidget(Widget, OlVirtualName, XtPointer);

ClassMethod void 	Destroy(Widget w);

ClassMethod void 	GetValuesHook(Widget w, ArgList args, Cardinal *num_args);

ClassMethod Widget 	RegisterFocus(Widget w);


/**********************************************************************
			Event Handler declarations 
 **********************************************************************/
Private OlEventHandlerRec event_procs[] = {
	{ KeyPress,     	Key     },
	{ ButtonRelease, 	ButtonUp },
	{ ButtonPress,  	ButtonDown  }
};


/***********************************************************************
			Resource Declarations
 ***********************************************************************/

Private XtResource resources[] = {
#define OFFSET(field) XtOffsetOf(TextLineRec, textLine.field)

 { 
	XtNbackground, XtCBackground, XtRPixel, sizeof(Pixel),
   	XtOffsetOf(TextLineRec, core.background_pixel), XtRCallProc, 
	(XtPointer)InheritBackground
 },{ 
	XtNblinkRate, XtCBlinkRate, XtRInt, sizeof(int), 
	OFFSET(blink_rate), XtRImmediate, (XtPointer)0
 },{ 
	XtNcharsVisible, XtCCharsVisible, XtRInt, sizeof(int), 
	OFFSET(chars_visible), XtRImmediate, (XtPointer)0
 },{
	XtNcursorPosition, XtCTextPosition, XtRInt, sizeof(int),
	OFFSET(cursor_position), XtRImmediate, (XtPointer)0
 },{
        XtNinputFocusColor, XtCInputFocusColor, XtRPixel, sizeof(Pixel),
            XtOffsetOf(TextLineRec, primitive.input_focus_color),
            XtRCallProc, (XtPointer)_OlGetDefaultFocusColor
 },{
	XtNeditType, XtCEditType, XtROlDefine, sizeof(OlDefine),
	OFFSET(edit_type), XtRImmediate, (XtPointer)OL_TEXT_EDIT
 },{
	XtNinitialDelay, XtCInitialDelay, XtRInt, sizeof(int),
	OFFSET(initial_delay), XtRImmediate, (XtPointer)500
 },{
	XtNrepeatRate, XtCRepeatRate, XtRInt, sizeof(int),
	OFFSET(repeat_rate), XtRImmediate, (XtPointer)100
 },{
	XtNinsertTab, XtCInsertTab, XtRBoolean, sizeof(Boolean),
	OFFSET(insert_tab), XtRImmediate, (XtPointer)False
 },{
	XtNmaximumChars, XtCMaximumChars, XtRInt, sizeof(int),
	OFFSET(maximum_chars), XtRImmediate, (XtPointer)0
 },{
	XtNstring, XtCString, XtROlStr, sizeof(OlStr),
	OFFSET(string), XtROlStr, (XtPointer)NULL
 },{
	XtNupdateDisplay, XtCUpdateDisplay, XtRBoolean, sizeof(Boolean),
	OFFSET(update_display), XtRImmediate, (XtPointer)True
 },{
	XtNunderline, XtCUnderline, XtRBoolean, sizeof(Boolean),
	OFFSET(underline), XtRImmediate, (XtPointer)True
 },{
		/* Caption related resources ... */
	XtNcaptionLabel, XtCCaptionLabel, XtROlStr, sizeof(OlStr),
	OFFSET(caption_label), XtROlStr, (XtPointer)NULL
 },{
	XtNcaptionFont, XtCCaptionFont, XtROlFont, sizeof(OlFont),
	OFFSET(caption_font), XtRString, (XtPointer)OlDefaultBoldFont
 },{
	XtNcaptionPosition, XtCCaptionPosition, XtROlDefine, sizeof(OlDefine),
	OFFSET(caption_position), XtRImmediate, (XtPointer)OL_LEFT
 },{
	XtNcaptionWidth, XtCCaptionWidth, XtRDimension, sizeof(Dimension),
	OFFSET(caption_width), XtRImmediate, (XtPointer)0
 },{
	XtNcaptionAlignment, XtCCaptionAlignment, XtROlDefine, sizeof(OlDefine),
	OFFSET(caption_alignment), XtRImmediate, (XtPointer)OL_CENTER
 },{
	XtNcaptionSpace, XtCCaptionSpace, XtRDimension, sizeof(Dimension),
	OFFSET(caption_space), XtRImmediate, (XtPointer)4
 },{
		
	XtNimPreeditStyle, XtCImPreeditStyle, XtROlImPreeditStyle, 
	sizeof(OlImPreeditStyle), OFFSET(pre_edit_style), 
	XtRImmediate, (XtPointer)OL_NO_PREEDIT
 },{
	XtNmenu, XtCReadOnly, XtRWidget, sizeof(Widget), 
	OFFSET(menu), XtRImmediate, (XtPointer)NULL
 },{
	XtNpreModifyCallback, XtCCallback, XtRCallback, 
	sizeof(XtCallbackList), OFFSET(pre_modify_callback), 
	XtRCallback, (XtPointer)NULL
 },{
	XtNpostModifyCallback, XtCCallback, XtRCallback, 
	sizeof(XtCallbackList), OFFSET(post_modify_callback), 
	XtRCallback, (XtPointer)NULL
 },{
	XtNmotionCallback, XtCCallback, XtRCallback, 
	sizeof(XtCallbackList), OFFSET(motion_callback), 
	XtRCallback, (XtPointer)NULL
 },{
	XtNcommitCallback, XtCCallback, XtRCallback, 
	sizeof(XtCallbackList), OFFSET(commit_callback), 
	XtRCallback, (XtPointer)NULL
 },{
	/* cursorVisible - private resource for Traversal & FocusHandling routines */
	XtNcursorVisible, XtCCursorVisible, XtRBoolean, sizeof(Boolean),
	OFFSET(caret_visible), XtRImmediate, (XtPointer)False
 },

#undef OFFSET
};
	
/******************************************************************
 *	Class Record Structure ...
 ******************************************************************/

TextLineClassRec textLineClassRec = {
 {		/* Core Class */
	/* superclass		*/ 	(WidgetClass)	&primitiveClassRec,
	/* class_name 		*/ 			"TextLine",
	/* widget_size 		*/			sizeof(TextLineRec),
	/* class_init		*/			ClassInitialize,
	/* class_part_init 	*/			ClassPartInitialize,
	/* class_inited 	*/			False,
	/* initialize		*/			Initialize,
	/* initialize_hook	*/			NULL,
	/* realize		*/			Realize,
	/* actions		*/			NULL,
	/* num_actions		*/			0,
	/* resources		*/			resources,
	/* num_resources      	*/    			XtNumber(resources),
	/* xrm_class		*/			NULLQUARK,
	/* compress_motion	*/			True,
	/* compress_exposure	*/			True,
	/* compress_enterleave	*/			True,
	/* visible_interest	*/			False,
	/* destroy		*/			Destroy,
	/* resize		*/			Resize,
	/* expose		*/			Redisplay,
	/* set_values 		*/			SetValues,
	/* set_values_hook 	*/			NULL,
	/* set_values_almost	*/			SetValuesAlmost,
	/* get_values_hook	*/			GetValuesHook,
	/* accept_focus		*/			XtInheritAcceptFocus,
	/* version		*/			XtVersion,
	/* callback_private	*/	(XtPointer)	0,
	/* tm_table		*/			XtInheritTranslations,
	/* query_geometry	*/			XtInheritQueryGeometry,
	/* display_accelerator	*/	(XtStringProc)	_XtInherit,
	/* extension		*/	(XtPointer)	0
 },
 {		/* Primitive Class */
	/* reserved             */ 	(XtPointer)	NULL,
	/* highlight_handler	*/			FocusHandler,
	/* traversal_handler	*/			NULL,
	/* register_focus	*/			RegisterFocus,
	/* activate		*/			ActivateWidget,
	/* event_procs		*/			event_procs,
	/* num_event_procs	*/			XtNumber(event_procs),
	/* version		*/			OlVersion,
	/* extension		*/			NULL,
	/* dyn_data		*/			{0, 0},
	/* transparent_proc	*/			NULL,
	/* query_sc_locn_proc   */			NULL,
 },
 {		/* TextLine Class */
	/* set_string		*/			SetString,
	/* get_string		*/			GetString,
	/* extension		*/	(XtPointer)	NULL,	
 }
};

PublicInterface WidgetClass 
textLineWidgetClass = (WidgetClass) & textLineClassRec;

/******************************************************
	InheritBackground - inherit parent's background 

 ******************************************************/
Private void
InheritBackground(Widget w, int offset, XrmValue *value)
{
	static Pixel pixel;

	pixel = XtParent(w)->core.background_pixel;
	value->addr = (caddr_t) &pixel;
}

/******************************************************
	ClassInitialize - class_initialize class-method

 ******************************************************/
ClassMethod void
ClassInitialize(void)
{
	_OlAddOlDefineType( "text_edit", OL_TEXT_EDIT);
	_OlAddOlDefineType( "text_read", OL_TEXT_READ);
	_OlAddOlDefineType ("left",   OL_LEFT);
	_OlAddOlDefineType ("center", OL_CENTER);
	_OlAddOlDefineType ("right",  OL_RIGHT);
	_OlAddOlDefineType ("top",    OL_TOP);
	_OlAddOlDefineType ("bottom", OL_BOTTOM);
}

/***************************************************************
	ClassPartInitialize - class_part_initialize class-method

 **************************************************************/
ClassMethod void
ClassPartInitialize(WidgetClass class)
{
	TextLineWidgetClass tlc = (TextLineWidgetClass)class;
	TextLineWidgetClass super = (TextLineWidgetClass)tlc->core_class.superclass;
	
	if (tlc->textLine_class.set_string == XtInheritOlTLSetStringProc)
		tlc->textLine_class.set_string = super->textLine_class.set_string;
	if (tlc->textLine_class.get_string == XtInheritOlTLGetStringProc)
		tlc->textLine_class.get_string = super->textLine_class.get_string;
	
	OlClassSearchTextDB(class);
}

/**************************************************************
	SetupGCsAndCursor

	Create GCs - normal_gc, inverse_gc caption_gc & caret_gc
	Create OlgxAttrs 

 **************************************************************/
Private void
SetupGCsAndCursor(TextLineWidget w, unsigned long what)
{
	TextLinePart *tlp = &w->textLine;
	Display *dpy = XtDisplay((Widget)w);
	GC	*GCs = tlp->GCs;
	XGCValues values;
	XtGCMask fontMask = (w->primitive.text_format == OL_SB_STR_REP) ? GCFont : 0;

#define MASK1  	(GCGraphicsExposures | GCForeground | GCBackground |  	\
		 GCStipple | GCFillStyle |  GCTileStipXOrigin | GCTileStipYOrigin)
#define MASK2	(GCGraphicsExposures | GCForeground | GCBackground | GCFont)

	values.graphics_exposures = True;
	values.stipple = OlGet50PercentGrey(XtScreen((Widget)w));
	values.ts_x_origin =  0;
	values.ts_y_origin =  0;
	values.fill_style  = XtIsSensitive((Widget)w) ? FillSolid : FillStippled;

	if (fontMask) 
		values.font = ((XFontStruct *)w->primitive.font)->fid;

	if (what & NORMAL) {
		values.foreground = w->primitive.font_color;
		values.background = w->core.background_pixel;

		if (GCs[NORM_GC] != NULL) 
			XtReleaseGC((Widget)w, GCs[NORM_GC]);
		GCs[NORM_GC] = XtGetGC((Widget)w, MASK1 | fontMask, &values);
	}

	if (what & INVERSE) {
		values.background = w->primitive.font_color;
		values.foreground = w->core.background_pixel;

		if (GCs[INV_GC] != NULL) 
			XtReleaseGC((Widget)w, GCs[INV_GC]);
		GCs[INV_GC] = XtGetGC((Widget)w, MASK1 | fontMask, &values);
	}

	if (what & CAPTION) {
		values.foreground = w->primitive.font_color;
		values.background = w->core.background_pixel;

		if (GCs[CAPTION_GC] != NULL) 
			XtReleaseGC((Widget)w, GCs[CAPTION_GC]);
		if (fontMask) 
			values.font = ((XFontStruct *)tlp->caption_font)->fid;
		GCs[CAPTION_GC] = XtGetGC((Widget)w, MASK1 | fontMask, &values);
	}	

	if (what & GINFO) {
		XFontStruct *font_info;
		XCharStruct active_char_info, inactive_char_info;

		if (tlp->pAttrs) 
			OlgxDestroyAttrs((Widget)w,tlp->pAttrs);
		tlp->pAttrs = OlgxCreateAttrs((Widget)w, 
						w->primitive.foreground,
						(OlgxBG *)&(w->core.background_pixel), 
						False, w->primitive.scale, 
						w->primitive.text_format, (OlFont)NULL);

		font_info = GlyphFont_Struct(tlp->pAttrs->ginfo);

		if (font_info->per_char) {
			active_char_info = font_info->per_char[OLGX_ACTIVE_CARET];
			inactive_char_info = font_info->per_char[OLGX_INACTIVE_CARET];
		} else {
			active_char_info = font_info->min_bounds;
			inactive_char_info = font_info->min_bounds;
		}

		tlp->caret_info.caret_ascent = 
			_OlMax(active_char_info.ascent, inactive_char_info.ascent);
		tlp->caret_info.caret_descent = 
			_OlMax(active_char_info.descent, inactive_char_info.descent);
		tlp->caret_info.caret_width = 
			_OlMax(active_char_info.width, inactive_char_info.width);
		tlp->caret_info.caret_height = 
			tlp->caret_info.caret_ascent + tlp->caret_info.caret_descent;

		if (tlp->caret_info.saved_pixmap)
			XFreePixmap(dpy, tlp->caret_info.saved_pixmap);
		tlp->caret_info.saved_pixmap = XCreatePixmap(dpy, 
						RootWindowOfScreen(XtScreen(w)),
						tlp->caret_info.caret_width, 
						tlp->caret_info.caret_height,
						w->core.depth);
	}

	if (what & CARET) {
		values.graphics_exposures = False;
		values.foreground = w->primitive.input_focus_color;
		values.background = w->core.background_pixel;
		values.font = GlyphFont_Struct(tlp->pAttrs->ginfo)->fid;

		if (tlp->caret_info.caret_gc != NULL)
			XtReleaseGC((Widget)w, tlp->caret_info.caret_gc);
		tlp->caret_info.caret_gc = XtGetGC((Widget)w, MASK2 ,&values);
	}

}

/******************************************************
	ValidateResources 

 ******************************************************/
Private void
ValidateResources(TextLinePart *tlp, TextLinePart *old_tlp)
{
	if (tlp->edit_type != OL_TEXT_EDIT && tlp->edit_type != OL_TEXT_READ) {
		_OLTLWarn("TextLineWidget: Invalid value for XtNeditType resource");
		tlp->edit_type = OL_TEXT_EDIT;
	}

	/* Caption stuff -  maintain them as READ-ONLY ... */
	if (tlp->caption_position != OL_LEFT) {
		_OLTLWarn("TextLineWidget: XtNcaptionPosition resource is not settable");
		tlp->caption_position = OL_LEFT;
	}
	if (tlp->caption_alignment != OL_CENTER) {
		_OLTLWarn("TextLineWidget: XtNcaptionAlignment resource is not settable");
		tlp->caption_alignment = OL_CENTER;
	}

	if (old_tlp) {	/* Non-Settable resources ... */
		if (tlp->chars_visible != old_tlp->chars_visible) {
			_OLTLWarn("TextLineWidget: XtNcharsVisible is not settable");
			tlp->chars_visible = old_tlp->chars_visible;
		}
		if (tlp->maximum_chars != old_tlp->maximum_chars) {
			_OLTLWarn("TextLineWidget: XtNmaximumSize is not settable");
			tlp->maximum_chars = old_tlp->maximum_chars;
		}
		if (tlp->pre_edit_style != old_tlp->pre_edit_style) {
			_OLTLWarn("TextLineWidget: XtNimPreeditStype is not settable");
			tlp->pre_edit_style = old_tlp->pre_edit_style;
		}
		if (tlp->menu != old_tlp->menu) {
			_OLTLWarn("TextLineWidget: XtNmenu is not settable");
			tlp->menu = old_tlp->menu;
		}
	}
}

/***************************************************************************
	SetString - Replace current internal-buffer with string

	cursor - True: XtNcursorPosition has been set externally, we need to
		 	honour it if it is valid
		 False:	We can setup the cursor_position ; set it to 
			the end_of_string.

 ***************************************************************************/
Private Boolean
SetString(TextLineWidget w, OlStr string, OlTLSetStringHints hints, Boolean cursor)
{
	TextLinePart *tlp = &w->textLine;

	UndoBuffer *u = &tlp->undo_buffer;
	unsigned long what = 0;
	OlStrRep format = w->primitive.text_format;

	if (hints == NFSetVal || hints == NFInit || hints == NFOther) 
		tlp->string = string;

	/* Clear Undo buffer */
	if (_OLStrCmp(format, u->string, _OLStrEmptyString(format)) != 0)
		XtFree(u->string);
	u->string = _OLStrEmptyString(format);
	u->start = u->end = 0;

	if (tlp->num_chars > 0)
		_OlTLDeleteString(tlp->buffer, 0, tlp->num_chars, format, tlp->pos_table);
	tlp->num_chars = _OLStrNumChars(format, tlp->string);

	if (tlp->maximum_chars && tlp->num_chars > tlp->maximum_chars) {
		_OLTLWarn("TextLineWidget: length of XtNstring is greater than XtNmaximumSize. Setting XtNstring to NULL");
		tlp->num_chars = 0;
		tlp->string = _OLStrEmptyString(format);
	}

	if (_OLStrCmp(format, tlp->string, _OLStrEmptyString(format)) != 0)
		_OlTLInsertString(tlp->buffer, tlp->string, 0, format,tlp->pos_table);

	if (cursor) {
		if (tlp->cursor_position > tlp->num_chars || tlp->cursor_position < 0) {
		    _OLTLWarn("TextLineWidget: XtNcursorPosition is beyond bounds");
		     tlp->cursor_position = tlp->num_chars;
		}
	} else /* Position cursor @ end_of_string */
		tlp->cursor_position = tlp->num_chars;

	tlp->select_start = tlp->select_end = tlp->caret_pos = tlp->cursor_position;

	if (hints != TLInit && hints != TLSetVal)
		what = _OlTLSetupArrowsAfterTextChange(w);
	if (hints == NFOther)
		_OlTLDrawWidget(w, _OLTLStartOfDisplay(tlp), TLEndOfDisplay,
					what | TLDrawText | TLMoveCaret);

	return True;
}

/******************************************************
	GetString

 ******************************************************/
Private OlStr
GetString(TextLineWidget w)
{
	TextLinePart *tlp = &w->textLine;

	return String(tlp);
}

/******************************************************
	Initialize - initialize class-method

 ******************************************************/
ClassMethod void
Initialize(Widget req, Widget new, ArgList args, Cardinal *num_args)
{
	TextLineWidgetClass tlc = (TextLineWidgetClass)XtClass(new);
	TextLineWidget tlw = (TextLineWidget)new;
	TextLinePart *tlp = &tlw->textLine;
	OlStrRep format = tlw->primitive.text_format;
	OlFont	font = tlw->primitive.font;

	if (tlp->string == (OlStr)NULL) tlp->string = _OLStrEmptyString(format);

	/* Set up the buffer for use ... */

#define WC_NULL_LEN 4
#define NULL_LEN 1
	tlp->buffer = _OlTLAllocateBuffer(BUF_SIZE);
	_OlTLInsertBytes(tlp->buffer, _OLStrEmptyString(format), 0,
			format == OL_WC_STR_REP ? WC_NULL_LEN: NULL_LEN);
#undef NULL_LEN
#undef WC_NULL_LEN

	if (format == OL_MB_STR_REP) {
		tlp->pos_table = _OlTLAllocateBuffer(BUF_SIZE);
		*((int *)(tlp->pos_table->p)) = 0;
		tlp->pos_table->used = sizeof(int);
	} else	
		tlp->pos_table = NULL;

	if (tlp->chars_visible == 0)
		tlp->chars_visible = (tlp->maximum_chars == 0? 20: tlp->maximum_chars);
	
	if (tlp->caption_label) {
		XRectangle ink, logical;
		OlStr buff;

		buff = XtMalloc(_OLStrNumBytes(format, tlp->caption_label));
		tlp->caption_label = _OLStrCpy(format, buff, tlp->caption_label);

		_OLStrExtents(format, tlp->caption_font, 
				buff, 
				_OLStrNumUnits(format, buff), 
				&ink, &logical
			       );
		tlp->caption_width = logical.width;
		tlp->caption_height = logical.height;
	} else {
		tlp->caption_width = 0;
		tlp->caption_height = 0;
	}

	ValidateResources(tlp, (TextLinePart *)NULL);

	if (tlp->edit_type == OL_TEXT_READ)
		tlw->primitive.traversal_on = False;
	
	/*  Set Private Data */

	tlp->blink_on 		= 	True;
	tlp->blink_timer 	= 	NULL;
	tlp->caret_pos 		= 	tlp->cursor_position;
	tlp->select_start 	= 	0;
	tlp->select_end		=	0;
	tlp->last_pos_width	=	0;
	tlp->max_char_width	=	_OlFontWidth(new);
	tlp->gap 		= 	OlScreenPointToPixel(OL_HORIZONTAL, 
					     		     Gap(tlw->primitive.scale), 
							     XtScreen(new));
	tlp->num_chars		=	0;

	tlp->undo_buffer.string	=	_OLStrEmptyString(format);
	tlp->undo_buffer.start	=	0;
	tlp->undo_buffer.end	=	0;

	tlp->preed_on		=	False;
	tlp->preed_start	=	-1;
	tlp->preed_caret	=	0;
	tlp->num_preed_chars	=	0;
	tlp->preed_buffer	=	NULL;
	tlp->preed_pos_table	=	NULL;
	tlp->feedback_table	=	NULL;

	tlp->leftarrow_present 	=	False;
	tlp->rightarrow_present =	False;

	tlp->char_offset	=	0;

	tlp->scroll_direction	=	NULL;
	tlp->scroll_timer	=	NULL;

	tlp->wipethru_timer	=	NULL;
	tlp->select_mode	=	OlselectChar;

	tlp->ic_id		= 	NULL;
	tlp->mask		=	0;

	tlp->transient		=	(Atom)NULL;
	tlp->clip_contents	=	(OlStr)NULL;
	tlp->dnd_contents	=	(OlStr)NULL;

	tlp->redraw		=	0;

	if ((*tlc->textLine_class.set_string)(tlw ,tlp->string,TLInit,True) == False) {
		tlp->string = _OLStrEmptyString(format);
		tlp->select_start = tlp->select_end 
				  = tlp->caret_pos 
				  = tlp->cursor_position 
				  = tlp->num_chars 
				  = 0;
	}
	
	/*	Get GCs & Graphics_infos ... */
	tlp->GCs[0] = tlp->GCs[1] 
		    = tlp->GCs[2] 
		    = tlp->caret_info.caret_gc 
		    = (GC)NULL;

	tlp->pAttrs = (OlgxAttrs *)NULL;
	tlp->caret_info.saved_pixmap = (Pixmap)NULL;
	tlp->caret_info.caret_state = TLCaretInvisible;
	SetupGCsAndCursor(tlw, NORMAL | INVERSE | CARET | CAPTION | GINFO);

	if (tlp->menu)	/* External menu has been installed */
		tlp->my_menu = False;
	else
		tlp->my_menu = True;

	/* 	IM stuff */
	if (format != OL_SB_STR_REP) {
		int 	i = 0;
		Arg	icarg[5];
		XPoint	spot = {0, 0};
		XIMCallback preeditStart, preeditDone, preeditDraw, preeditCaret;
		OlInputMethodID     input_method = NULL;

		if (tlp->pre_edit_style == OL_OVER_THE_SPOT) {
			i = 0;
			XtSetArg(icarg[i],XNFontSet,font); i++;
			XtSetArg(icarg[i],XNSpotLocation,&spot); i++;
			XtSetArg(icarg[i],XNBackground,tlw->core.background_pixel); i++;
			XtSetArg(icarg[i],XNForeground,tlw->primitive.font_color); i++;
		} else if(tlp->pre_edit_style == OL_ON_THE_SPOT) {
                	preeditStart.client_data = (XPointer)tlw;
                	preeditStart.callback = (XIMProc)_OlTLPreeditStartCallbackFunc;
                	preeditDone.client_data = (XPointer)tlw;
                	preeditDone.callback = (XIMProc)_OlTLPreeditEndCallbackFunc;
                	preeditDraw.client_data = (XPointer) tlw;
                	preeditDraw.callback = (XIMProc)_OlTLPreeditDrawCallbackFunc;
                	preeditCaret.client_data = (XPointer) tlw;
                	preeditCaret.callback = (XIMProc)_OlTLPreeditCaretCallbackFunc;

                	i = 0;
                	XtSetArg(icarg[i],XNPreeditStartCallback,
                                        (XtArgVal)&preeditStart); i++;
                	XtSetArg(icarg[i],XNPreeditDoneCallback,
                                        (XtArgVal)&preeditDone); i++;
                	XtSetArg(icarg[i],XNPreeditDrawCallback,
                                        (XtArgVal)&preeditDraw); i++;
        	}

		input_method = OlDefaultIMOfWidget(new);
		if(input_method != (OlInputMethodID)NULL){
			tlp->ic_id = OlCreateIC(input_method, new,
                                        tlp->pre_edit_style, icarg, i);
			if (tlp->ic_id == (OlInputContextID) NULL &&
			    tlp->edit_type != OL_TEXT_READ) {
					char *msg;

					if (msg = malloc(1024)) {
						snprintf(msg, 1024, "%s: Preedit will be disabled (No InputStyle).", tlw->core.name);
						_OLTLWarn(msg); 
						free(msg);
					}
        		}
		} else {
			if (tlp->edit_type != OL_TEXT_READ) {
				char *msg;

				if (msg = malloc(1024)) {
					 snprintf(msg, 1024, "%s: Preedit will be disabled (No InputMethod).", tlw->core.name);
					 _OLTLWarn(msg);
					free(msg);
				}
			}
		}
	}

	tlw->core.width = tlp->real_width 
			= LabelWidth(tlp) 
				+ 2 * tlp->gap 
				+ tlp->chars_visible * tlp->max_char_width;
	tlw->core.height = tlp->real_height 
			 = Max3(
				(Dimension)(_OlFontAscent(new) + _OlFontDescent(new) + 
					UNDERLINE_HEIGHT), 
				tlp->caption_height,
				(Dimension)TextScrollButton_Height(tlp->pAttrs->ginfo)
			       );

	(void)_OlTLSetupArrowsAfterTextChange(tlw);
}

/******************************************************
	Realize  - realize class-method

 ******************************************************/
ClassMethod void
Realize(Widget w, Mask *valueMask, XSetWindowAttributes *attributes)
{
	TextLineWidget tlw = (TextLineWidget)w;
	TextLinePart *tlp = &tlw->textLine;
	OlDnDSiteRect   rect;

	/* Let the super-class do the dirty work ... */
	(*textLineClassRec.core_class.superclass->core_class.realize)
		(w, valueMask, attributes);
	
	/* DnD stuff ... */
	rect.x = rect.y = 0;
	rect.width = w->core.width;
	rect.height = w->core.height;
	tlp->dropsite_id = OlDnDRegisterWidgetDropSite(w,
				XtIsSensitive(w) && (tlp->edit_type == OL_TEXT_EDIT) ?
				  OlDnDSitePreviewNone : OlDnDSitePreviewInsensitive,
				&rect, 1, TriggerNotify, NULL, (Boolean)True, NULL);
}

/******************************************************
	Redisplay - redisplay class-method 

 ******************************************************/
ClassMethod void
Redisplay(Widget w, XEvent *event, Region region)
{
	TextLinePart *tlp = &(((TextLineWidget)w)->textLine);
	unsigned long what = 0;

	if (event->type == Expose || event->type == GraphicsExpose) {

		/* In case of race-condition between Expose & BlinkCursor */
		XClearArea(XtDisplay(w),XtWindow(w),0,0, tlp->real_width,
				tlp->real_height, False);
		tlp->caret_info.caret_state = TLCaretInvisible;

		if (tlp->leftarrow_present) 
			what |= TLDrawLeftArrow;
		if (tlp->rightarrow_present) 
			what |= TLDrawRightArrow;

		what |= (TLDrawText | TLMoveCaret | TLDrawUnderline | TLDrawLabel);
		_OlTLDrawWidget((TextLineWidget)w, _OLTLStartOfDisplay(tlp),
					TLEndOfDisplay, what);
	}
}

/******************************************************
	Resize - resize class-method 

 ******************************************************/
ClassMethod void
Resize(Widget w)
{
	TextLinePart *tlp = &((TextLineWidget)w)->textLine;
	OlDnDSiteRect   rect;

	tlp->real_width = w->core.width;
	tlp->real_height = w->core.height;

	if ((tlp->chars_visible = tlp->max_char_width ?
	    (int)(w->core.width - LabelWidth(tlp) - 2 * tlp->gap)/tlp->max_char_width : 0) <=0) {
		_OLTLWarn("TextLineWidget: Widget size is too small");
		tlp->chars_visible = 1;
	}

#define TEXT_FITS(w, tlp) 	\
	((int)(DisplayPositionOfString(w, 0, tlp->num_chars + tlp->num_preed_chars) + \
		LabelWidth(tlp) + tlp->gap) <= 	\
	 (int)(tlp->real_width - tlp->gap)	\
	)

	if (TEXT_FITS((TextLineWidget)w, tlp)) {
		tlp->char_offset = 0;
		tlp->leftarrow_present = tlp->rightarrow_present = False;
	}
	else 
		(void)_OlTLSetupArrowsAfterTextChange((TextLineWidget)w);

#undef TEXT_FITS

	if (XtIsRealized(w)) {
		/* DnD  stuff ... */
		rect.x = rect.y = 0;
		rect.width = w->core.width;
		rect.height = w->core.height;
		OlDnDUpdateDropSiteGeometry(tlp->dropsite_id, &rect, 1);
	}
}

/******************************************************
	FocusHandler - focus_handler class-method 

 ******************************************************/
ClassMethod void
FocusHandler(Widget w, OlDefine highlight_type)
{
        TextLinePart *tlp = &((TextLineWidget)w)->textLine;

	switch ((int) highlight_type) {
	case OL_IN:
		tlp->caret_visible = True;
		if (tlp->ic_id != NULL && tlp->edit_type == OL_TEXT_EDIT) {
			if(OlSetFocusIC(tlp->ic_id) == NULL) {
				char *msg;

				if (msg = malloc(1024)) {
					snprintf(msg, 1024, "%s: Unable to use IC.\nPreedit will be disabled.", w->core.name);
					_OLTLWarn(msg);
					free(msg);
				}
				tlp->ic_id = (OlInputContextID)NULL;
			}
		}
		break;
	case OL_OUT:
		if(tlp->ic_id != NULL)
			OlUnsetFocusIC(tlp->ic_id);
		break;
	default:
		_OLTLWarn("TextLineWidget: Undefined highlight type");
		break;
	}
	_OlTLDrawWidget((TextLineWidget)w, 0, 0, TLDrawCaret);
}

/******************************************************
	SetValues - set_values class-method 

 ******************************************************/
ClassMethod Boolean
SetValues(Widget current,Widget request, Widget new,ArgList args,Cardinal * num_args)
{
	TextLineWidget newtl = (TextLineWidget)new;
	TextLineWidget oldtl = (TextLineWidget)current;
	TextLinePart *newtlp = &newtl->textLine;
	TextLinePart *oldtlp = &oldtl->textLine;
	OlInputContextID ic_id = newtlp->ic_id;
	OlStrRep format = newtl->primitive.text_format;
	Boolean cursor_set = False;
	Boolean redisplay = False;
	Boolean resize = False;
	Boolean realized = XtIsRealized(new);
	unsigned long gc_to_change = 0;
	unsigned long what = 0;

#define CHANGED_PRIMITIVE(field) (newtl->primitive.field != oldtl->primitive.field)
#define CHANGED_CORE(field) (newtl->core.field != oldtl->core.field)
#define CHANGED_TEXT(field) (newtlp->field != oldtlp->field)

	ValidateResources(newtlp, oldtlp);

	if (CHANGED_PRIMITIVE(input_focus_color)) {
		gc_to_change |= CARET;
		what |= TLMoveCaret;
	}

	if (CHANGED_PRIMITIVE(foreground)) {
		gc_to_change |= GINFO;
		redisplay = True;
	}

	if (CHANGED_PRIMITIVE(scale)) {
		newtlp->gap = OlScreenPointToPixel(OL_HORIZONTAL,
						   Gap(newtl->primitive.scale), 
						   XtScreen(new));
		gc_to_change =  CARET | GINFO;
		resize = True;
	}

	if (CHANGED_TEXT(caption_font) && newtlp->caption_label) {
		XRectangle ink, logical;

		gc_to_change |= CAPTION;
		_OLStrExtents(format,
				newtlp->caption_font,
				newtlp->caption_label,
				_OLStrNumUnits(format, newtlp->caption_label), 
				&ink, &logical);

		newtlp->caption_width = logical.width;
		newtlp->caption_height = logical.height;
		redisplay = resize = True;
	}

	if (CHANGED_PRIMITIVE(font)) {
		newtlp->max_char_width = _OlFontWidth(new);
		gc_to_change |= (NORMAL | INVERSE | CARET);
		redisplay = resize = True;
	}

	if (CHANGED_PRIMITIVE(font_color)) {
		gc_to_change |= (NORMAL | INVERSE | CAPTION);
		what |= TLDrawText;
	}

	if ((XtIsSensitive(current) != XtIsSensitive(new)) || CHANGED_TEXT(edit_type)) {
		OlDnDSitePreviewHints preview_hints; 
		if (realized) {
		   OlDnDQueryDropSiteInfo(newtlp->dropsite_id, (Widget *)NULL,
		  	(Window *)NULL, &preview_hints, (OlDnDSiteRectPtr *)NULL,
		  	(unsigned int *)NULL, (Boolean *)NULL);
		   if (XtIsSensitive(new) && (newtlp->edit_type == OL_TEXT_EDIT))
			preview_hints &= ~OlDnDSitePreviewInsensitive;
		   else
			preview_hints |= OlDnDSitePreviewInsensitive;
		   OlDnDChangeDropSitePreviewHints(newtlp->dropsite_id, preview_hints);
		}

		if (XtIsSensitive(current) != XtIsSensitive(new)) {
			gc_to_change |= (NORMAL | INVERSE | CAPTION);
			redisplay = True;
		}
		if (CHANGED_TEXT(edit_type)) {
			if (newtlp->edit_type == OL_TEXT_READ) {
				newtl->primitive.traversal_on = False;
				if (newtl->primitive.has_focus == True)
					OlMoveFocus(new, OL_NEXTFIELD, CurrentTime);
				if (ic_id != (OlInputContextID)NULL)
					OlUnsetFocusIC(newtlp->ic_id);
			}
			else { 	/* OL_TEXT_EDIT */
				newtl->primitive.traversal_on = True;
				if (newtl->primitive.has_focus == True)
				    if (ic_id != (OlInputContextID)NULL)
					if(OlSetFocusIC(newtlp->ic_id) == NULL) {
					   char *msg;

						if (msg = malloc(1024)) {
						   snprintf(msg, 1024, "%s: Unable to set focus to IC.\nPreedit will be disabled.", newtl->core.name);
						   _OLTLWarn(msg);
							free(msg);
						}
					   newtlp->ic_id = (OlInputContextID)NULL;
					}
			}
			what |= TLDrawCaret;
		}
	}

	if (CHANGED_CORE(background_pixel)) {
		gc_to_change |= (NORMAL | INVERSE | CAPTION | CARET | GINFO);
		redisplay = True;
	}


	if (CHANGED_TEXT(caret_visible))
		what |= TLDrawCaret;

	if (CHANGED_TEXT(cursor_position))
		cursor_set = True;

	/* Check for XtNstring in the arglist. Comparing the pointers in
	 * the old & new widget structs may fail if the same buffer is used ..
	 */
	{
	   int i;

	   for (i = 0; i < *num_args; i++)
		if (strcmp(args[i].name, XtNstring) == 0) {
			TextLineWidgetClass tlc = (TextLineWidgetClass)XtClass(new);

			if (newtlp->string == (OlStr)NULL) 
				newtlp->string = _OLStrEmptyString(format);

			if ((*tlc->textLine_class.set_string)
			    (newtl ,newtlp->string,TLSetVal, cursor_set) == False) {
				newtlp->string = oldtlp->string;
				newtlp->cursor_position = oldtlp->cursor_position;
			} else
				what |= (TLDrawText | TLMoveCaret);
			break;
		}
	}

	if (cursor_set) {

		if (newtlp->cursor_position > newtlp->num_chars || 
		    newtlp->cursor_position < 0) {
		    _OLTLWarn("TextLineWidget: XtNcursorPosition is beyond bounds");
		     newtlp->cursor_position = newtlp->num_chars;
		}
		if (InvokeMotionCallbacks(newtl, oldtlp->cursor_position, 
			newtlp->cursor_position , (XEvent *)NULL, False)) {
			what |= TLMoveCaret;
			newtlp->select_start = newtlp->select_end = 
				newtlp->caret_pos = newtlp->cursor_position;
		}
	}

	if (CHANGED_TEXT(caption_label)) {
		OlStr buff;
		XRectangle ink, logical;

		if (oldtlp->caption_label)
			XtFree(oldtlp->caption_label);

		if (newtlp->caption_label) {	
			buff = XtMalloc(_OLStrNumBytes(format, newtlp->caption_label));
			newtlp->caption_label = _OLStrCpy(format, buff, 
							  newtlp->caption_label);
			_OLStrExtents(format, 
					newtlp->caption_font, buff, 
					_OLStrNumUnits(format, buff), 
					&ink, &logical);
			newtlp->caption_width = logical.width;
			newtlp->caption_height = logical.height;
		} else 
			newtlp->caption_width = newtlp->caption_height = 0;
		redisplay = resize = True;
	}

	if (CHANGED_TEXT(underline)) 
		what |= (newtlp->underline ? TLDrawUnderline : TLClearUnderline);

	if (CHANGED_TEXT(update_display)) 
		what = newtlp->update_display ? (what |= newtlp->redraw) : 0;

	if (gc_to_change) SetupGCsAndCursor(newtl, gc_to_change);

	if (resize) {
		newtl->core.width = newtlp->real_width 
				  = LabelWidth(newtlp) 
					+ 2 * newtlp->gap 
					+ newtlp->chars_visible * newtlp->max_char_width;
		newtl->core.height = newtlp->real_height 
				   = Max3(
				       (Dimension)
				          (_OlFontAscent(new) + _OlFontDescent(new) + 
						UNDERLINE_HEIGHT), 
				       newtlp->caption_height,
				       (Dimension)
				          TextScrollButton_Height(newtlp->pAttrs->ginfo));

		/* If dimensions have'nt changed .we can't depend on Resize to
		 * generate an Expose event ... to do the Redisplay 
		 */
		if (newtlp->real_width == oldtlp->real_width &&
		    newtlp->real_height == oldtlp->real_height)
			resize = False;
	} else if (what & TLDrawText) {
		/* If resize is True , then the Resize/SetValAlmost would do the
	 	 * arrow-resetting stuff. But, if not, then we need to do it here 
	 	 * TLDrawText - indicates here that "XtNstring" has changed 
	  	 * and we need to recompute arrows & draw the new string
	  	 */
		what |= _OlTLSetupArrowsAfterTextChange(newtl);
	} else if (what & TLMoveCaret) {
		/* If resize is True , then the Resize/SetValAlmost would do the
	 	 * arrow-resetting stuff. But, if not, then we need to do it here 
		 */
		what |= _OlTLSetupArrowsAfterCursorChange(newtl);
	}

	if (what && !redisplay && !resize && realized)
		_OlTLDrawWidget(newtl, _OLTLStartOfDisplay(newtlp),TLEndOfDisplay, what);
	return (redisplay && !resize); 
}

/******************************************************************************
	SetValuesAlmost - set_values_almost class-method

 	Certain resources result in a change in the widget's dimensions when set.
  The SetValues proc, updates the widgets width & height fields to reflect the
  new geometry. The Intrinsics then request the new size from the widget's parent
  If the parent agrees, the widget's window gets resized & the server consequently
  generates an Expose event on the window . The widget's expose proc then takes 
  care of redrawing the new image... However, if the parent refused to grant the
  request , we still need to update the display with the new images 
  (Ex : caption_label ). One way to do this would be to make SetValues return
  True when any of these resources change. However that would result in the
  widget getting two expose events - If the resize IS done by the parent. The
  alternative is to use the setvalues_almost proc to do this .. as it would
  be invoked only if the resize was not/partially granted by the parent ..

 ******************************************************************************/
ClassMethod void
SetValuesAlmost(Widget old, Widget new, XtWidgetGeometry *req, XtWidgetGeometry *reply)
{
	TextLineWidget w = (TextLineWidget)new;
	if (XtIsRealized(new) && reply->request_mode == 0) { 
		/* XtGeometryNo - must recompute arrow-state & do redisplay ourselves */
		w->textLine.real_width = new->core.width;
		w->textLine.real_height = new->core.height;
		(void)_OlTLSetupArrowsAfterTextChange(w);
		 XClearArea (XtDisplay(new), XtWindow(new), 0, 0, 0, 0, TRUE);
	}
	*req = *reply;
}


/******************************************************
	ActivateWidget - activate_widget class-method 

 ******************************************************/
ClassMethod Boolean
ActivateWidget(Widget w, OlVirtualName type, XtPointer call_data)
{
	Boolean retval = True;

	if (type == OL_NEXTFIELD || type == OL_PREVFIELD) {
		if (InvokeCommitCallbacks((TextLineWidget)w, (XEvent *)NULL) &&
		    ((TextLineWidget)w)->primitive.has_focus == True)
		/* If we have focus, transfer to next widget */
			OlMoveFocus(w, type, CurrentTime);
	} else 
		retval = EventHandler((TextLineWidget)w, type, (XEvent *)NULL);

    	return retval;
}

/******************************************************
	Destroy - destroy class-method 

 ******************************************************/
ClassMethod void
Destroy(Widget w)
{
	TextLinePart *tlp = &((TextLineWidget)w)->textLine;
	Display *dpy = XtDisplay(w);
	Window win = XtWindow(w);
	OlStrRep format = ((TextLineWidget)w)->primitive.text_format;
	int i;

	if (tlp->caption_label) 
		XtFree(tlp->caption_label);

	if (tlp->buffer) 
		_OlTLFreeBuffer(tlp->buffer);
	if (tlp->pos_table) 
		_OlTLFreeBuffer(tlp->pos_table);
	if (_OLStrCmp(format, tlp->undo_buffer.string,_OLStrEmptyString(format)) != 0)
		XtFree(tlp->undo_buffer.string);

	if (tlp->preed_buffer) 
		_OlTLFreeBuffer(tlp->preed_buffer);
	if (tlp->preed_pos_table) 
		_OlTLFreeBuffer(tlp->preed_pos_table);
	if (tlp->feedback_table) 
		_OlTLFreeBuffer(tlp->feedback_table);

	for (i = 0; i < 3; i++)
		if (tlp->GCs[i] != NULL) 
			XtReleaseGC(w, tlp->GCs[i]);
	if (tlp->pAttrs) 
		OlgxDestroyAttrs(w,tlp->pAttrs);
	if (tlp->caret_info.saved_pixmap)
		XFreePixmap(dpy, tlp->caret_info.saved_pixmap);

	if (tlp->clip_contents) 
		XtFree(tlp->clip_contents);
	if (tlp->dnd_contents) 
		XtFree(tlp->dnd_contents);

	if (tlp->blink_timer) 
		XtRemoveTimeOut(tlp->blink_timer);
	if (tlp->scroll_timer) 
		XtRemoveTimeOut(tlp->scroll_timer);
	if (tlp->wipethru_timer) 
		XtRemoveTimeOut(tlp->wipethru_timer);

	if (win !=(Window)NULL && XGetSelectionOwner(dpy,XA_CLIPBOARD(dpy)) == win)
		XSetSelectionOwner(dpy, XA_CLIPBOARD(dpy), 
					None, XtLastTimestampProcessed(dpy));
	if (tlp->transient != (Atom)NULL) {
		if (XGetSelectionOwner(dpy,tlp->transient) == win)
			OlDnDDisownSelection(w, tlp->transient, CurrentTime);
		OlDnDFreeTransientAtom(w, tlp->transient);
	}
}

/******************************************************
	GetValuesHook - get_values_hook class-method

 ******************************************************/
ClassMethod void 
GetValuesHook(Widget w, ArgList args, Cardinal *num_args)
{
	TextLinePart *tlp = &((TextLineWidget)w)->textLine;
	int i;

	for (i = 0; i < *num_args; i++) {

		if (strcmp(args[i].name, XtNstring) == 0)
			*((OlStr *)(args[i].value)) = String(tlp);

		if (strcmp(args[i].name, XtNmenu) == 0) {
			if (tlp->menu == NULL)
				CreateMenu((TextLineWidget)w);
			*((Widget *)(args[i].value)) = tlp->menu;
		}
	}
}

/******************************************************
	RegisterFocus  - register_focus class-method

 ******************************************************/
ClassMethod Widget
RegisterFocus(Widget w)
{
	return w;
}

/**********************************************************************
	_OlTLDrawWidget 

 	 Public interface to the widget drawing routines. The individual
  components of the widget are the left & right arrows, text, underline,
  caret and caption_label
	If any of the components that are drawn,intersect with the caret, 
  the caret should be removed before the component is drawn & redrawn
  after the component is drawn. This routine optimizes the step by removing
  the caret the first time its need to be removed, and redrawing the
  caret after all drawing operations are done. Also note the efficiency hack
  where we don't bother checking the caret in TLDraw*Arrow since we "know"
  that TLDraw*Arrow is mostly in conjunction with TLClear*Arrow (where the
  caret-hiding is done) - the exceptions guarentee that the caret does not 
  overlap the arrow (if not, its a bug!).

 ***********************************************************************/
WidgetInternal void
_OlTLDrawWidget(TextLineWidget w, int start, int end,  unsigned long what)
{
	TextLinePart *tlp = &w->textLine;
        Graphics_info *ginfo = tlp->pAttrs->ginfo;
	Display *dpy = XtDisplay(w);
	Window win = XtWindow(w);
	Boolean caret_visible =  tlp->caret_visible;
	Boolean caret_set_invisible = False;
	int sensitive = XtIsSensitive((Widget)w) ? 0 : OLGX_INACTIVE;
	int arrow_chars = _OLTLNumArrowChars(tlp);
	int x, y;

#define SetCaretInvisible(w, tlp) 			\
	   	if (!caret_set_invisible) {		\
			tlp->caret_visible = False; 	\
			DrawCaret(w);			\
			caret_set_invisible = True;	\
		}

	if (tlp->update_display == False) {
		tlp->redraw |= what;
		return;
	}

	if (what & TLClearLeftArrow) {
		x = LabelWidth(tlp);
		if (tlp->caret_x <= x + TextScrollButton_Width(ginfo) + tlp->gap) 
			SetCaretInvisible(w, tlp);
		XClearArea(dpy, win, x, 0, TextScrollButton_Width(ginfo) + tlp->gap,
				tlp->real_height, False);
	}

	if (what & TLDrawLeftArrow || what & TLDrawInvokedLeftArrow) {
		int state = what & TLDrawLeftArrow ? 0 : OLGX_INVOKED;
		x = LabelWidth(tlp);
		y = tlp->real_height - TextScrollButton_Height(ginfo);

		state |= (OLGX_SCROLL_BACKWARD | sensitive);
		olgx_draw_textscroll_button(ginfo, win, x, y, state);
	}

	if (what & TLClearRightArrow) {
		x = tlp->real_width - TextScrollButton_Width(ginfo) - tlp->gap;
		if ((int)(tlp->caret_x + tlp->caret_info.caret_width) >= x) 
			SetCaretInvisible(w, tlp);
		XClearArea(dpy, win, x, 0, TextScrollButton_Width(ginfo) + tlp->gap,
				tlp->real_height, False);
	}

	if (what & TLDrawRightArrow || what & TLDrawInvokedRightArrow) {
		int state = what & TLDrawRightArrow ? 0 : OLGX_INVOKED;
		x = tlp->real_width - TextScrollButton_Width(ginfo);
		y = tlp->real_height - TextScrollButton_Height(ginfo);

		state |= (OLGX_SCROLL_FORWARD | sensitive);
		olgx_draw_textscroll_button(ginfo, win, x, y, state);
	}

	if (what & TLDrawUnderline || what & TLClearUnderline) {
		int width;
		x = TextStartPos(tlp);
		y = tlp->real_height - UNDERLINE_HEIGHT;
		width = TextEndPos(w, tlp) - x;
		
		SetCaretInvisible(w, tlp);

		if (what & TLDrawUnderline && tlp->underline)
			olgx_draw_text_ledge(ginfo, win, x, y, width);
		if (what & TLClearUnderline)
			XClearArea(dpy, win, x, y, width, UNDERLINE_HEIGHT,False);
	}

	if (what & TLDrawText) {
		int max_x;

		if (end == TLEndOfDisplay) {
			max_x = TextEndPos(w, tlp);
			end = tlp->num_preed_chars + tlp->num_chars;
		} else {
			max_x = DisplayPositionOfString(w, 
					tlp->char_offset + arrow_chars, end) + 
				TextStartPos(tlp);
		}

		y = tlp->real_height - UNDERLINE_HEIGHT - _OlFontDescent((Widget)w);
		x = DisplayPositionOfString(w,tlp->char_offset +arrow_chars, 
				start) + TextStartPos(tlp);

		if ((int)(tlp->caret_x + tlp->caret_info.caret_width) >= x &&
		    tlp->caret_x <= max_x) {
		/* Caret is partially OR fully within text region ... Erase caret
		 * first, Draw text and finally redisplay caret 
		 */
			SetCaretInvisible(w, tlp);
		}
		DrawText(w, start, end - start, x, y, max_x);
	}

	if (what & TLMoveCaret) {
		OlInputContextID   ic_id = tlp->ic_id;
		x = DisplayPositionOfString(w,tlp->char_offset +arrow_chars, 
				tlp->caret_pos) + TextStartPos(tlp);
		y = tlp->real_height - tlp->caret_info.caret_descent;

		/* Clear off old cursor at old (x,y) */
		SetCaretInvisible(w, tlp);

		tlp->caret_x = x - tlp->caret_info.caret_width/2;
		tlp->caret_y = y;

		if (ic_id != (OlInputContextID)NULL && 
				tlp->pre_edit_style == OL_OVER_THE_SPOT) {
			XPoint spot;
			spot.x = x, spot.y = y;
			OlVaSetValuesIC(ic_id, True,XNSpotLocation, &spot,NULL);
		}
	}

	if (what & TLDrawCaret || caret_set_invisible) {
		tlp->caret_visible = caret_visible;
		DrawCaret(w);
	}

	if (what & TLDrawLabel) 
		DrawLabel(w);

#undef SetCaretVisible

}

/************************************************************
	KeepDisplayCursorInView 

 	Constrains cursor_position within visible textregion. 
  Returns True, if char_offset has changed
  	Updates:	tlp->char_offset

 ************************************************************/
Private Boolean
KeepDisplayCursorInView(TextLineWidget w)
{
	TextLinePart *tlp = &w->textLine;
	int pos = tlp->caret_pos;
	int arrow_chars = _OLTLNumArrowChars(tlp);
	int real_char_offset = tlp->char_offset + arrow_chars;
	int x, y;
	
#define	ShiftAmount(tlp)	(tlp->chars_visible/3)

	if (pos < real_char_offset) { /* Left arrow IS present */
		tlp->char_offset -= _OlMin(tlp->char_offset, 
				       (real_char_offset- pos) + ShiftAmount(tlp));
		return True;
	} else if ((x = DisplayPositionOfString(w, real_char_offset,pos)
			+ TextStartPos(tlp)) > (y = TextEndPos(w, tlp))) {
		tlp->char_offset = ConvertForwardToPos(w, real_char_offset, x-y) +
					1 + ShiftAmount(tlp) - arrow_chars;
		return True; 
	}
	return False;

#undef ShiftAmount
}

/******************************************************
	TextLineEditString 

  Interface for non-programmatic edit operations. 

 ******************************************************/
Private Boolean
TextLineEditString(TextLineWidget w, int start, int end, OlStr string, XEvent *ev)
{
	TextLinePart *tlp = &w->textLine;
	int start_pos;
	OlTLPreModifyCallbackStruct pre_cd;
	OlTLPostModifyCallbackStruct post_cd;
	unsigned long what = 0;
	int num_insert_chars = _OLStrNumChars(w->primitive.text_format, string);

	if ((tlp->edit_type == OL_TEXT_READ) || (tlp->maximum_chars && 
	    ((tlp->num_chars + num_insert_chars - (end-start)) > tlp->maximum_chars))) {
		_OlBeepDisplay((Widget)w, 1);
		return False;	
	}

	pre_cd.reason = OL_REASON_PRE_MODIFICATION;
	pre_cd.event = ev;
	pre_cd.current_cursor = tlp->cursor_position;
	pre_cd.new_cursor = start + num_insert_chars;  /* same_as undo_buff.end */
	pre_cd.start = start;
	pre_cd.replace_length = end - start;
	pre_cd.buffer = String(tlp);
	pre_cd.valid = True;
	pre_cd.insert_buffer = string;
	pre_cd.insert_length = num_insert_chars;
	XtCallCallbackList((Widget)w, tlp->pre_modify_callback, &pre_cd);
	if (pre_cd.valid == False)
		return False;

	if (_OlTLReplaceString(w, start, end, string) == False)
		return False;

	/* Clear selections if any .... */
	tlp->select_start = tlp->select_end = 
		tlp->cursor_position = tlp->undo_buffer.end;

	/* If preedit is ON, it means that we have been handed "string" to be
	 * "committed" ... but note that preedit has not yet been Done.
	 */
	if (tlp->preed_on == True) {
		tlp->preed_start = tlp->cursor_position;
		tlp->caret_pos = tlp->preed_start + tlp->preed_caret;
	} else {
		tlp->caret_pos = tlp->cursor_position;
	}

	if ((what = _OlTLSetupArrowsAfterTextChange(w)) & TLDrawText)
		start_pos = _OLTLStartOfDisplay(tlp);
	else {
		int start_of_display = _OLTLStartOfDisplay(tlp);
		start_pos = start < start_of_display ? start_of_display : start;
	}

	_OlTLDrawWidget(w, start_pos, TLEndOfDisplay, what | TLDrawText | TLMoveCaret);

	post_cd.reason = OL_REASON_POST_MODIFICATION;
	post_cd.event = ev;
	post_cd.cursor = tlp->cursor_position;
	post_cd.buffer = String(tlp);
	XtCallCallbackList((Widget)w, tlp->post_modify_callback, &post_cd);
	return True;
}

/***********************************************************************
	_OlTLSetupArrowsAfterCursorChange 

  Set the status of left & right arrows after caret_position has changed.
  We attempt to keep the caret visible - by adjusting the char_offset.

 ***********************************************************************/
WidgetInternal unsigned long
_OlTLSetupArrowsAfterCursorChange(TextLineWidget w)
{
	unsigned long what = 0;
	TextLinePart *tlp = &w->textLine;
	Boolean leftarrow_present = tlp->leftarrow_present;
	Boolean rightarrow_present = tlp->rightarrow_present;

	if (KeepDisplayCursorInView(w) == True) {
		/* char_offset has changed */
		if (LeftArrowRequired(tlp)) 
			tlp->leftarrow_present = True;
		else 
			tlp->leftarrow_present = False;

		if (RightArrowRequired(w,tlp)) 
			tlp->rightarrow_present = True;
		else 
			tlp->rightarrow_present = False;
		
		what |= ((leftarrow_present != tlp->leftarrow_present) ? 
		  	 	(tlp->leftarrow_present ? 
					TLCreateLeftArrow: TLDestroyLeftArrow):
			0);
		what |= ((rightarrow_present != tlp->rightarrow_present) ? 
		  	 	(tlp->rightarrow_present ? 
					TLCreateRightArrow: TLDestroyRightArrow):
			0);
		what |= TLDrawText;
	}
	return what;
}

/********************************************************************
	_OlTLSetupArrowsAfterTextChange 

  Set the status of left & right arrows after text has changed.
  We attempt to keep the caret visible - by adjusting the char_offset.

 ********************************************************************/
WidgetInternal unsigned long
_OlTLSetupArrowsAfterTextChange(TextLineWidget w)
{
	TextLinePart *tlp = &w->textLine;
	unsigned long what = 0;
	Boolean leftarrow_present = tlp->leftarrow_present;
	Boolean rightarrow_present = tlp->rightarrow_present;

	if (RightArrowRequired(w,tlp)) 
		tlp->rightarrow_present = True;
	else 
		tlp->rightarrow_present = False;

	if (KeepDisplayCursorInView(w) == True) {
		/* char_offset has changed */
		if (LeftArrowRequired(tlp)) 
			tlp->leftarrow_present = True;
		else 
			tlp->leftarrow_present = False;

		if (RightArrowRequired(w,tlp)) 
			tlp->rightarrow_present = True;
		else 
			tlp->rightarrow_present = False;

		what |= TLDrawText;
	}

	what |= ((leftarrow_present != tlp->leftarrow_present) ? 
	  	 	(tlp->leftarrow_present ? 
				TLCreateLeftArrow: TLDestroyLeftArrow):
		0);
	what |= ((rightarrow_present != tlp->rightarrow_present) ? 
	  	 	(tlp->rightarrow_present ? 
				TLCreateRightArrow: TLDestroyRightArrow):
		0);
	return what;
}

/******************************************************
	InvokeMotionCallbacks 

 ******************************************************/
Private Boolean 
InvokeMotionCallbacks(TextLineWidget w, int current, int new, XEvent *ev, Boolean flag)
{
	OlTLMotionCallbackStruct cd;

	cd.reason = (flag ? OL_REASON_MOTION : OL_REASON_PROG_MOTION);
	cd.event = ev;
	cd.current_cursor = current;
	cd.new_cursor = new;
	cd.valid = True;

	XtCallCallbackList((Widget)w, w->textLine.motion_callback, &cd);
	return(cd.valid);
}

/******************************************************
	InvokeCommitCallbacks 

 ******************************************************/
Private Boolean
InvokeCommitCallbacks(TextLineWidget w, XEvent *ev)
{
	TextLinePart *tlp = &w->textLine;
	OlTLCommitCallbackStruct cd;

	cd.reason = OL_REASON_COMMIT;
	cd.event = ev;
	cd.buffer = _OlTLGetSubString(tlp->buffer, 0, 
				      tlp->num_chars -1,
			     	      w->primitive.text_format, 
				      tlp->pos_table
				     );
	cd.length = _OLStrNumChars(w->primitive.text_format, cd.buffer);
	cd.valid = True;

	XtCallCallbackList((Widget)w, w->textLine.commit_callback, &cd);
	XtFree(cd.buffer);

	return(cd.valid);
}

/******************************************************
	toUnitPos 

 ******************************************************/
Private int
toUnitPos(OlStr str, int pos, OlStrRep format, PositionTable *posTable)
{
	if (format == OL_MB_STR_REP) {
		char *p; int len;
		if (posTable) return *((int *)(posTable->p) + pos);
		for (p = (char *)str; 
		     pos > 0 && (len = mblen(p, MB_CUR_MAX)) > 0; p += len, pos--)
		     ;
		return (p - (char *)str);
	}
	else 
		return pos;
}

/********************************************************************************
	DisplayPositionOfString 

  Returns the width in pixels of character-string starting at "start" in the buffer
  (which comprises of the main-buffer & preedit buffer). till "end"
  NON-INCLUSIVELY . No error checking for  validity of "start" & "end" done 
  Note that all positions are normalized to include both buffers ...

 ********************************************************************************/
Private int 
DisplayPositionOfString(TextLineWidget w, int start, int end)
{
	TextLinePart *tlp = &w->textLine;
	OlStr str; 
	int preeditStart =  tlp->preed_start;
	int preeditEnd = tlp->preed_start + tlp->num_preed_chars - 1;
	OlFont font = w->primitive.font;
	OlStrRep format	= w->primitive.text_format;
	int width = 0; int sign = 1;
	
	if (start == end) 
		return 0;
	if (start > end) {
		int t = start; start = end; end = t;
		sign = -1;
	}
		
	end--;	/* Since the below algorithm treats "end" as INCLUSIVE .. */

	/* Phase - 1 : stuff before preedit */
	if (start < preeditStart) {
		str = String(tlp);
		if (end < preeditStart) {
			return ( sign * _StringWidth(width, str,
				   toUnitPos(str, start, format, tlp->pos_table),
				   toUnitPos(str, end, format, tlp->pos_table),
				   font, format, NULL));
		} else {
			width = _StringWidth(width, str,
				   toUnitPos(str, start, format, tlp->pos_table),
				   toUnitPos(str, preeditStart-1, format, tlp->pos_table),
				   font, format, NULL);
			start = preeditStart;
		}
	}
	/* Phase -2 : stuff in preedit */
	if (start <= preeditEnd) {
		str = PreEdString(tlp);
		if (end <= preeditEnd) {
			return (sign * _StringWidth(width, str,
				   toUnitPos(str, start-preeditStart, format, NULL), 
				   toUnitPos(str, end-preeditStart, format, NULL),
				   font, format, NULL));
		} else {
			width = _StringWidth(width, str,
				   toUnitPos(str, start-preeditStart, format, NULL), 
				   toUnitPos(str, preeditEnd-preeditStart,format, NULL),
				   font, format, NULL);
			start = preeditEnd + 1;
		}
	}
	/* Phase - 3:	stuff after preedit */
	str = String(tlp);
	return (sign *  _StringWidth(width, str, 
		   toUnitPos(str, start -tlp->num_preed_chars, format, tlp->pos_table), 
		   toUnitPos(str, end -tlp->num_preed_chars, format, tlp->pos_table),
		   font, format, NULL));
}

/******************************************************************************
	DrawText 

  Draws text starting at "pos" in the buffer, for "length" characters
  Sets tlp->last_pos_width ... which is actually the width of the last+1 character
  displayed in the window. The only use of this is to avoid determining 
  the last_char_width each time the RightScrollbutt is activated ...

 ******************************************************************************/
typedef int (*DrawTextFunc)(TextLineWidget w, OlStr str,
		int start, int end, int x, int y, int max_x, int type);

Private DrawTextFunc draw_text_array[NUM_SUPPORTED_REPS] = {
	DrawTextSB,
	DrawTextMB,
	DrawTextWC,
};


#define NORMAL_TEXT 	1
#define PREED_TEXT	2

Private void
DrawText(TextLineWidget w, int pos, int length, int x, int y, int max_x)
{
	TextLinePart *tlp = &w->textLine;
	OlStrRep format = w->primitive.text_format;
	int end;
	int preeditStart =  tlp->preed_start;
	int preeditEnd = tlp->preed_start + tlp->num_preed_chars - 1;
	OlFont font = w->primitive.font;
	int font_ascent = _OlFontAscent((Widget)w);
	int caret_visible = tlp->caret_visible;
	DrawTextFunc draw_text = draw_text_array[format];

	/* Clear area from x till max_x . If max_x == x, we mean that nothing
	 * needs to be cleared. However Xlib interprets "width == 0" to mean 
	 * that the whole window width from x should be cleared :< (Ref Xlib spec)
	 */
	if (max_x != x) 
		XClearArea(XtDisplay((Widget)w), XtWindow((Widget)w), 
		   	   x, y - font_ascent, 
		   	   (unsigned int)(max_x - x),  
		   	   (unsigned int)(font_ascent + _OlFontDescent((Widget)w)),
		   	   False
		  	  );

	if (length == 0)	/* Nothing to be drawn .. */ 
		return; 

	end = pos + length - 1;

	/* Phase - 1 : stuff before preedit */
	if (pos < preeditStart) {
		if (end < preeditStart) {
			if ((x = draw_text(w, String(tlp), pos, end, x, y, max_x, 
					   NORMAL_TEXT)) < 0) 
				tlp->last_pos_width = -x; 
			return;
		} else {
			if ((x = draw_text(w, String(tlp), pos, preeditStart-1,
				x,y, max_x, NORMAL_TEXT)) < 0) {
				tlp->last_pos_width = -x;
				return;
			} else {
				pos = preeditStart;	
			}
		}
	}
	/* Phase - 2 : stuff in preedit */
	if (pos <= preeditEnd) {
		if (end <= preeditEnd) {
			if ((x = draw_text(w, PreEdString(tlp), pos - preeditStart,
				end- preeditStart, x, y, max_x, PREED_TEXT)) < 0) 
				tlp->last_pos_width = -x;
			return;
		} else {
			if ((x = draw_text(w, PreEdString(tlp), pos -preeditStart,
				preeditEnd- preeditStart,x,y,max_x,PREED_TEXT)) < 0) {
				tlp->last_pos_width = -x;
				return;
			} else {
				pos = preeditEnd + 1;
			}
		}
	}

	/* Phase -3 : stuff after preedit */
	if ((x = draw_text(w, String(tlp), pos -tlp->num_preed_chars,
			end - tlp->num_preed_chars, x, y, max_x, NORMAL_TEXT)) < 0) 
		tlp->last_pos_width = -x;
}


/*********************************************************************
	DrawTextMB 

  Draws text starting at "start" till "end" - inclusive in MB format
 
  Returns the length covered by this drawing session ... if max_x
  has not yet been exceeded ; else returns the position of the character
  which caused this .... We return this as a negative value to distinguish
  it from the former.
  We assume that the widget has been cleared from "start" till "end" 
 
 *********************************************************************/

#define GetFeedbackStyle(type, pos, s_start, s_end, fdbk_t) 			\
	((type == PREED_TEXT) ? *((unsigned long *)(fdbk_t->p) + pos) :		\
				((pos < s_start || pos >= s_end) ? 		\
					TLNormalFeedback: TLInvFeedback))

#define GetGC(gctable, fdbk)							\
	(fdbk == XIMReverse || fdbk == XIMPrimary || fdbk == XIMHighlight || 	\
	 fdbk == XIMTertiary || fdbk == TLInvFeedback) ? 			\
	 	gctable[INV_GC]: gctable[NORM_GC]
	 
Private int
DrawTextMB(TextLineWidget w, OlStr str, int start, int end, int x, int y, 
			int max_x, int type)
{
	TextLinePart *tlp = &w->textLine;
	OlFont font = w->primitive.font;
	Display *dpy = XtDisplay(w);
	Window win = XtWindow(w);
	int font_ascent = _OlFontAscent((Widget)w);
	int recttop = y - font_ascent;
	GC gc; 
	char *p, *left_pos;
	wchar_t wch;
	int left_x;
	int len, width;
	int fontht = font_ascent + _OlFontDescent((Widget)w);
	unsigned long style;
	unsigned long current_feedback = GetFeedbackStyle(type, start, 
					 tlp->select_start, tlp->select_end,
					 tlp->feedback_table);

	void (*drawFunc)(Display *dpy, Drawable d, XFontSet fs, GC gc, int x, int y,
				const char *text, int bytes_in_text);

	drawFunc = XtIsSensitive((Widget)w) ? XmbDrawImageString: XmbDrawString;	

	gc = GetGC(tlp->GCs,current_feedback);

	if (type == NORMAL_TEXT)
		p = (char *)str + 
		    toUnitPos(str, start, w->primitive.text_format, tlp->pos_table);
	else
		p = (char *)str + 
		    toUnitPos(str, start, w->primitive.text_format, tlp->preed_pos_table);

	for (left_pos = p, left_x = x; 
	     start <= end && ((len = mbtowc(&wch, p, MB_CUR_MAX)) > 0) && 
	      	((width = CharWidthWC(x,wch, font)) + x) <= max_x; 
		p += len, start++) {
		if (wch == L'\t') {
			/* Draw string from left_pos till (p-1) inclusive*/
			if (p > left_pos) 
				(*drawFunc)(dpy,win ,(XFontSet)font, gc, left_x,y, 
					left_pos, p - left_pos);
			XFillRectangle(dpy,win,tlp->GCs[INV_GC], x,recttop,width,fontht);
			left_x = x + width;
			left_pos = p + len;
		} else if ((style = GetFeedbackStyle(type, start, tlp->select_start,
			tlp->select_end, tlp->feedback_table)) != current_feedback) {
			if (p > left_pos) {
				(*drawFunc)(dpy, win,(XFontSet)font, gc, left_x, y, 
					left_pos, p - left_pos);
				if (current_feedback == XIMUnderline) 
					XDrawLine(dpy,win,gc, left_x, y, x,y);
			}
			left_pos = p;
			left_x = x;
			current_feedback = style;
			gc = GetGC(tlp->GCs, current_feedback);
		}

		x += width;
	}

	if (p > left_pos) 
		(*drawFunc)(dpy, win, (XFontSet)font, gc, left_x, y, left_pos, 
					p - left_pos);

	if (width + x >= max_x || len < 0) 
		return (-width);
	else 
		return x;
}

/*********************************************************************
	DrawTextWC 

  Draws text starting at "start" till "end" - inclusive in WC format
 
  Returns the length covered by this drawing session ... if max_x
  has not yet been exceeded ; else returns the position of the character
  which caused this .... We return this as a negative value to distinguish
  it from the former.
  We assume that the widget has been cleared from "start" till "end" 
 
 *********************************************************************/
Private int
DrawTextWC(TextLineWidget w, OlStr str, int start, int end, int x, int y, 
				int max_x, int type)
{
	TextLinePart *tlp = &w->textLine;
	OlFont font = w->primitive.font;
	Display *dpy = XtDisplay((Widget)w);
	Window win = XtWindow((Widget)w);
	int font_ascent = _OlFontAscent((Widget)w);
	int recttop = y - font_ascent;
	GC gc; 
	wchar_t *wstr = (wchar_t *)str;
	wchar_t *p, *left_pos;
	int len, width;
	int left_x;
	int fontht = font_ascent + _OlFontDescent((Widget)w);
	unsigned long style;
	unsigned long current_feedback = GetFeedbackStyle(type, start, 
					 tlp->select_start, tlp->select_end,
					 tlp->feedback_table);
	void (*drawFunc)(Display *dpy, Drawable d, XFontSet fs, GC gc, int x, int y,
				wchar_t *text, int num_wchars);

	drawFunc = XtIsSensitive((Widget)w) ? XwcDrawImageString: XwcDrawString;	
	
	gc = GetGC(tlp->GCs,current_feedback);
	
	for (left_pos = p = (wstr + start), left_x = x; 
	     start <= end && ((width = CharWidthWC(x,*p, font)) + x) <= max_x; 
		p++, start++) {
		if (*p == L'\t') {
			/* Draw string from left_pos till (p-1) inclusive*/
			if (p > left_pos) 
				(*drawFunc)(dpy,win , (XFontSet)font, gc, left_x,y, 
					left_pos, p - left_pos);
			XFillRectangle(dpy,win, tlp->GCs[INV_GC],x,recttop,width,fontht);
			left_x = x + width;
			left_pos = p+1;
		} else if ((style = GetFeedbackStyle(type, start, tlp->select_start,
			tlp->select_end, tlp->feedback_table)) != current_feedback) {
			if (p > left_pos) {
				(*drawFunc)(dpy, win,(XFontSet)font, gc, left_x, y, 
					left_pos, p - left_pos);
				if (current_feedback == XIMUnderline) 
                                        XDrawLine(dpy,win,gc, left_x, y, x,y);
			}
			left_pos = p;
			left_x = x;
			current_feedback = style;
			gc = GetGC(tlp->GCs, current_feedback);
		}

		x += width;
	}

	if (p > left_pos) 
		(*drawFunc)(dpy, win, (XFontSet)font, gc, left_x, y, left_pos, 
					p - left_pos);

	if (width + x >= max_x) 
		return (-width);
	else 
		return x;
}

/*********************************************************************
	DrawTextSB 

  Draws text starting at "start" till "end" - inclusive in SB format
 
  Returns the length covered by this drawing session ... if max_x
  has not yet been exceeded ; else returns the position of the character
  which caused this .... We return this as a negative value to distinguish
  it from the former.
  We assume that the widget has been cleared from "start" till "end" 
 
 *********************************************************************/
Private int
DrawTextSB(TextLineWidget w, OlStr str, int start, int end, int x, int y, 
				int max_x, int type)
{
	TextLinePart *tlp = &w->textLine;
	OlFont font = w->primitive.font;
	Display *dpy = XtDisplay(w);
	Window win = XtWindow(w);
	int font_ascent = _OlFontAscent((Widget)w);
	int recttop = y - font_ascent;
	GC gc; 
	char *p, *left_pos;
	char *cstr = (char *)str;
	int len, width;
	int left_x;
	int fontht = font_ascent + _OlFontDescent((Widget)w);
	unsigned long style;
	unsigned long current_feedback = GetFeedbackStyle(type, start, 
					 tlp->select_start, tlp->select_end, 
					 tlp->feedback_table);
	int (*drawFunc)(Display *dpy, Drawable d, GC gc, int x, int y,
				const char *text, int length);

	drawFunc = XtIsSensitive((Widget)w) ? XDrawImageString: XDrawString;	
	
	gc = GetGC(tlp->GCs,current_feedback);
	for (left_pos = p = (cstr+start), left_x = x; 
	     start <= end && ((width = CharWidthSB(x,p, font)) + x) <= max_x; 
		p++, start++) {
		if (*p == '\t') {
			/* Draw string from left_pos till (p-1) inclusive*/
			if (p > left_pos) 
				(*drawFunc)(dpy,win , gc, left_x,y, left_pos, 
					p - left_pos);
			XFillRectangle(dpy, win,tlp->GCs[INV_GC],x,recttop,width,fontht);
			left_x = x + width;
			left_pos = p+1;
		} else if ((style = GetFeedbackStyle(type, start, tlp->select_start,
			tlp->select_end, tlp->feedback_table)) != current_feedback) {
			if (p > left_pos) 
				(*drawFunc)(dpy, win, gc, left_x, y, left_pos,
					p - left_pos);
			left_pos = p;
			left_x = x;
			current_feedback = style;
			gc = GetGC(tlp->GCs,current_feedback);
		}

		x += width;
	}

	if (p > left_pos) 
		(*drawFunc)(dpy, win, gc, left_x, y, left_pos, p - left_pos);

	if (width + x >= max_x) 
		return (-width);
	else 
		return x;
}

#undef GetFeedbackStyle
#undef GetGC

/******************************************************
	DrawLabel 

 ******************************************************/
Private void
DrawLabel(TextLineWidget w)
{
	int x,y;
	OlStrRep format = w->primitive.text_format;
	TextLinePart *tlp = &w->textLine;
	int font_descent = _OlFontDescent((Widget)w);
	Dimension height = Max3(
				(Dimension)(_OlFontAscent((Widget)w) + font_descent + 
				  UNDERLINE_HEIGHT), 
				tlp->caption_height,
				(Dimension)TextScrollButton_Height(tlp->pAttrs->ginfo));

	if (!(tlp->caption_label)) 
		return; /* No caption */

	x = 0;
	y = w->core.height - font_descent - (int)(height - tlp->caption_height)/2;

	if (XtIsSensitive((Widget)w))
		_OLStrDrawImage(format, 
				XtDisplay((Widget)w), XtWindow((Widget)w),
				tlp->caption_font, tlp->GCs[CAPTION_GC], 
				x, y, 
				tlp->caption_label, 
				_OLStrNumUnits(format, tlp->caption_label));
	else 
		_OLStrDraw(format, 
			   XtDisplay((Widget)w), XtWindow((Widget)w),
			   tlp->caption_font, tlp->GCs[CAPTION_GC], 
			   x, y, 
			   tlp->caption_label, 
			   _OLStrNumUnits(format, tlp->caption_label));
}
		

/******************************************************
	CharWidthSB & CharWidthWC 

 Handles <TABS> & <cntrl> chars too !

 ******************************************************/
Private int
CharWidthSB(int x, char *p, OlFont font)
{
	XFontStruct *fs = (XFontStruct *)font;
	if (*p == '\t') 
		return(_NextTabFrom(x, NULL, fs->max_bounds.width));
	else
		return(_CharWidth(*p, fs, fs->per_char, fs->min_char_or_byte2,
				fs->max_char_or_byte2, fs->max_bounds.width));
}

Private int
CharWidthWC(int x, wchar_t ch, OlFont font)
{
	XFontSet fs = (XFontSet)font;
	int max_width = (XExtentsOfFontSet(fs))->max_logical_extent.width;
	if (ch == L'\t') 
		return(_NextTabFrom(x, NULL, max_width));
	else 
		return(_CharWidthWC(ch, fs));
}

/**********************************************************************
	ConvertForwardToPos 

  Given a position in the textbuffer & a width (in pixels) , the routine
  converts the "width" into a character-position within the textbuffer.
  This routine goes "forward" - ie to the right of "start" to return the 
  position ...
 
 ***********************************************************************/
Private int
ConvertForwardToPos(TextLineWidget w, int start, int width)
{
	TextLinePart *tlp = &w->textLine;
	int preedStart = tlp->preed_start;
	int preedEnd = tlp->preed_start + tlp->num_preed_chars - 1;
	OlStr str;
	OlStrRep format = w->primitive.text_format;
	OlFont	font = w->primitive.font;

	if (start < preedStart) {
		str = String(tlp);
		while ((start < preedStart) && 
			(width -=CharWidth(str,toUnitPos(str,start,format,tlp->pos_table),
					font, format)) >0) 
			start++;
		if (width <= 0) 
			return start;
	}
	if (start <= preedEnd) {
		str = PreEdString(tlp);
		while ((start <= preedEnd) &&
			(width -= CharWidth(str, toUnitPos(str,start-preedStart,format,
					tlp->preed_pos_table),font,format)) >0)
			start++;
		if (width <= 0) 
			return start;
	}
	str = String(tlp);
	while ((start < (tlp->num_chars + tlp->num_preed_chars)) &&
		(width -= CharWidth(str, toUnitPos(str, start - tlp->num_preed_chars,
					format, tlp->pos_table),font,format)) >0)
			start++;
	return start;
}

/***********************************************************************
	ConvertBackwardToPos 

  Given a position in the textbuffer & a width (in pixels) , the routine
  converts the "width" into a character-position within the textbuffer.
  This routine goes "backward" - ie to the left of "start" to return the 
  position ...
 
 ***********************************************************************/
Private int
ConvertBackwardToPos(TextLineWidget w, int start, int width)
{
	TextLinePart *tlp = &w->textLine;
	int preedStart; 
	int preedEnd; 
	OlStr str;
	OlStrRep format = w->primitive.text_format;
	OlFont font = w->primitive.font;

	if (tlp->preed_on) {
		preedStart = tlp->preed_start;
		preedEnd = tlp->preed_start + tlp->num_preed_chars -1;
	} else	/* Need to set these up so that we skip the following "if" loops .. */
		preedStart = preedEnd = start + 1;

	if (start > preedEnd) {
		str = String(tlp);
		while (start > preedEnd &&
			(width -= CharWidth(str,toUnitPos(str,start-tlp->num_preed_chars,
					format, tlp->pos_table), font, format)) >0)
				start--;
		if (width <= 0) 
			return start;
	}
	if (start >= preedStart) {
		str = PreEdString(tlp);
		while (start >= preedStart &&
			(width -= CharWidth(str,toUnitPos(str,start-preedStart,format,
					tlp->preed_pos_table),font,format)) > 0)
				start--;
		if (width <= 0) 
			return start;
	}
	str = String(tlp);
	while (start > 0 && (width -= CharWidth(str, toUnitPos(str,start,format,
					tlp->pos_table),font, format)) > 0)
			start--;
	return start;
}

/******************************************************
	CharWidth 

 ******************************************************/
Private int
CharWidth(OlStr str, int pos, OlFont font, OlStrRep format)
{
	switch(format) {
		case OL_SB_STR_REP:
		{
			char *string = (char *)str;
			XFontStruct *fs = (XFontStruct *)font;
			return(_CharWidth(string[pos], fs, fs->per_char, 
				fs->min_char_or_byte2, fs->max_char_or_byte2, 
				fs->max_bounds.width));
		}
		case OL_MB_STR_REP:
		{
			char *string = (char *)str;
			XFontSet fs = (XFontSet)font;
			return(_CharWidthMB(string + pos, fs));
		}
		case OL_WC_STR_REP:
		{
			wchar_t *string = (wchar_t *)str;
			XFontSet fs = (XFontSet)font;
			return(_CharWidthWC(string[pos], fs));
		}
	}
}

/******************************************************
 	DrawCaret - Caret rendering routine

 ******************************************************/

Private void
DrawCaret(TextLineWidget w)
{
	Display *dpy = XtDisplay(w);
	Window win = XtWindow(w);
	TextLinePart *tlp = &w->textLine;
	CaretInfo *ci = &tlp->caret_info;
	TLCaretMode state;
	char caret_string[2];

	/* If caret is beyond bounds, Make it invisible */
	if ((tlp->caret_x +(int)(tlp->caret_info.caret_width) < (int)TextStartPos(tlp)) ||
	    (tlp->caret_x > (int)TextEndPos(w, tlp))) {
		ci->caret_state = TLCaretInvisible;
		return;
	}

	/* Determine caret-state - either Triangle OR Diamond OR Invisible */
	state = (tlp->caret_visible ? 
			(tlp->edit_type == OL_TEXT_EDIT ?
				(w->primitive.has_focus ?
					(tlp->blink_on ? 
						TLCaretTriangle:
						TLCaretInvisible):
					TLCaretDiamond):
				TLCaretInvisible):
			TLCaretInvisible);

	switch (state) {
	  case TLCaretInvisible:
		if (ci->caret_state == TLCaretInvisible) 
			return;

		ci->caret_state= TLCaretInvisible;

		/* Restore saved regions ... */
		XCopyArea(dpy, ci->saved_pixmap, win, ci->caret_gc, 
			  	0, 0, ci->caret_width, ci->caret_height, 
			  	tlp->caret_x, tlp->caret_y - ci->caret_ascent);
		break;

	  case TLCaretDiamond:
		if (ci->caret_state == TLCaretDiamond)
			return;

		if (ci->caret_state == TLCaretTriangle)
			/* Restore saved regions ... */
			XCopyArea(dpy, ci->saved_pixmap, win, ci->caret_gc,
				0, 0, ci->caret_width, ci->caret_height,
				tlp->caret_x, tlp->caret_y - ci->caret_ascent);
		else {
			/* Save to-be-hidden region */
			XSync(dpy, False);
			XCopyArea(dpy, win, ci->saved_pixmap, ci->caret_gc,
				tlp->caret_x , tlp->caret_y - ci->caret_ascent,
				ci->caret_width, ci->caret_height, 0, 0);
		}

		caret_string[0] = OLGX_INACTIVE_CARET;
		caret_string[1] = '\0';
		XDrawString(dpy, win, ci->caret_gc, tlp->caret_x, tlp->caret_y,
				caret_string, 1);	

		ci->caret_state = TLCaretDiamond;
		break;

	  case TLCaretTriangle:
		if (ci->caret_state == TLCaretTriangle)
			return;

		if (ci->caret_state == TLCaretDiamond)
			/* Restore saved regions ... */
			XCopyArea(dpy, ci->saved_pixmap, win, ci->caret_gc,
				0, 0, ci->caret_width, ci->caret_height,
				tlp->caret_x, tlp->caret_y - ci->caret_ascent);
		else {
			/* Save to-be-hidden region */
			XSync(dpy, False);
			XCopyArea(dpy, win, ci->saved_pixmap, ci->caret_gc,
				tlp->caret_x, tlp->caret_y - ci->caret_ascent,
				ci->caret_width, ci->caret_height, 0, 0);
		}
			
		caret_string[0] = OLGX_ACTIVE_CARET; 
		caret_string[1] = '\0'; 
		XDrawString(dpy, win, ci->caret_gc, tlp->caret_x, tlp->caret_y,
				caret_string, 1);

		ci->caret_state = TLCaretTriangle;
		break;

	  default:
		break;
	}
}

/******************************************************
	BlinkCursor 

 ******************************************************/
Private void
BlinkCursor(XtPointer client_data, XtIntervalId *id)
{
	TextLineWidget w = (TextLineWidget)client_data;

	w->textLine.blink_on = w->textLine.blink_on ? False: True;
	DrawCaret(w);

	XFlush(XtDisplay(w));

	XtAppAddTimeOut(XtWidgetToApplicationContext((Widget)w), w->textLine.blink_rate,
			BlinkCursor, (XtPointer)w);
}

/******************************************************
	ScrollField 

 ******************************************************/
Private void
ScrollField(TextLineWidget w,int direction,unsigned long delay)
{
	TextLinePart *tlp = &w->textLine;
	XtAppContext ac = XtWidgetToApplicationContext((Widget)w);
	unsigned long what = 0;

	if (direction == SCROLL_LEFT && tlp->leftarrow_present) { /* Scroll left */
		tlp->char_offset--;

		/* If we still need the left arrow ... */
		if (LeftArrowRequired(tlp)) {
			tlp->scroll_timer = XtAppAddTimeOut(ac,delay,Scroller,w);
		} else {  /* Cease Scrolling , Remove Arrow ,Reset caret visibility,
			   * Move caret to new position 
			   */
			tlp->leftarrow_present = False;
			tlp->caret_visible = True;
			tlp->scroll_direction = NULL;
			what |= (TLDestroyLeftArrow | TLMoveCaret);
		}

		if (RightArrowRequired(w,tlp) && !tlp->rightarrow_present) {
		/* The only situation is - we now require a right_arrow, we didn't
		 * have one ... Not vice-verca
		*/
				tlp->rightarrow_present = True;
				what |= TLCreateRightArrow;
		}
	}
	else
	if (direction == SCROLL_RIGHT && tlp->rightarrow_present) { /* Scroll right */
		int last_pos_width = tlp->last_pos_width;
		int arrow_chars = _OLTLNumArrowChars(tlp);
		OlStr str;

		tlp->char_offset = ConvertForwardToPos(w,tlp->char_offset+arrow_chars,
					last_pos_width) + 1 - arrow_chars;

		if (RightArrowRequired(w,tlp)) {
			tlp->scroll_timer = XtAppAddTimeOut(ac,delay,Scroller,w);
		} else { /* Cease Scrolling , Remove Arrow, Reset caret visibility,
		   	  * Move caret to new position 
			  */
			tlp->rightarrow_present = False;
			tlp->caret_visible = True;
			tlp->scroll_direction = NULL;
			what |= (TLDestroyRightArrow | TLMoveCaret);
		}

		if (LeftArrowRequired(tlp) && !tlp->leftarrow_present) {
		/* The only situation is - we now require a left_arrow, we didn't
		 * have one ... Not vice-versa
		*/
			tlp->leftarrow_present = True;
			what |= TLCreateLeftArrow;
		}
	}

	what |= TLDrawText; 
	_OlTLDrawWidget(w, _OLTLStartOfDisplay(tlp), TLEndOfDisplay, what);
}

/******************************************************
	Scroller - Scroll Timer handler

 ******************************************************/
Private void
Scroller(XtPointer client_data, XtIntervalId *id)
{
	TextLineWidget w = (TextLineWidget)client_data;
	TextLinePart *tlp = &w->textLine;

	tlp->scroll_timer = NULL;
	if (tlp->scroll_direction)
		ScrollField(w, tlp->scroll_direction, tlp->repeat_rate);
}

/******************************************************
	ButtonDown 

 ******************************************************/
Private void
ButtonDown(Widget w, OlVirtualEvent ve)
{
	TextLineWidget tlw = (TextLineWidget)w;
	TextLinePart *tlp = &tlw->textLine;
	XEvent *ev = ve->xevent;
	int x = ev->xbutton.x; 
	int y = ev->xbutton.y;
        Graphics_info *ginfo = tlp->pAttrs->ginfo;

#define LEFT_EDGE(tlp)  	((int)LabelWidth(tlp))
#define RIGHT_EDGE(tlp, g)  	((int)(tlp->real_width - TextScrollButton_Width(g)))
#define TOP_EDGE(tlp, g)	((int)(tlp->real_height - TextScrollButton_Height(g)))

	ve->consumed = True;

	if (x < LEFT_EDGE(tlp))
		return;

	if (tlp->leftarrow_present 	&& 
	    x > LEFT_EDGE(tlp) 		&&
	    x <= (int)(LEFT_EDGE(tlp) + TextScrollButton_Width(ginfo))) { 

	    if (y >= TOP_EDGE(tlp, ginfo)) {
		tlp->scroll_direction = SCROLL_LEFT;
		tlp->caret_visible = False;

		_OlTLDrawWidget(tlw, 0, 0, TLDrawInvokedLeftArrow | TLDrawCaret);
		ScrollField(tlw, SCROLL_LEFT, tlp->initial_delay);
	    }
	    else 
		return;	/* Dead Zone above left-arrow .. */

	} else if ( tlp->rightarrow_present && x > RIGHT_EDGE(tlp, ginfo)) {
	    if (y >= TOP_EDGE(tlp, ginfo)) {
		tlp->scroll_direction = SCROLL_RIGHT;
		tlp->caret_visible = False;
		_OlTLDrawWidget(tlw, 0, 0, TLDrawInvokedRightArrow |TLDrawCaret);
		ScrollField(tlw, SCROLL_RIGHT, tlp->initial_delay);
	    }
	    else 
		return;	/* Dead Zone above right-arrow .. */

	} else { /* Button Press in Text Area .... */
		if (x > (int)TextEndPos(w,tlp)) 
			return;	/* Dead Zone -gap between right-arrow & text-end !*/
		ButtonHandler(tlw, ve);
	}

#undef LEFT_EDGE
#undef RIGHT_EDGE
#undef TOP_EDGE

}

/******************************************************
	ButtonUp 

 ******************************************************/
Private void
ButtonUp(Widget w, OlVirtualEvent ve)
{
	TextLinePart *tlp = &((TextLineWidget)w)->textLine;
	unsigned long what = 0;

	if (tlp->scroll_timer) {
	/* Cease Scrolling, Un-Depress arrow, Reset caret visibility &
	 * Move caret to new position
	 */
		XtRemoveTimeOut(tlp->scroll_timer);	
		tlp->caret_visible = True;
		_OlTLDrawWidget((TextLineWidget)w, 0, 0,
			(tlp->scroll_direction == SCROLL_LEFT ? 
			 	TLDrawLeftArrow: TLDrawRightArrow) |
			TLMoveCaret);
		tlp->scroll_direction = NULL;
		tlp->scroll_timer = NULL;
	}
	/* Else - The WipeThru timer routine takes care of itself ... 
	 * So don't bother about it
	 */
}

/******************************************************
	Key 

 ******************************************************/
Private void
Key(Widget w, OlVirtualEvent ve)
{
	XEvent *ev = ve->xevent;
	TextLineWidget tlw = (TextLineWidget)w;
	TextLinePart *tlp = &tlw->textLine;
	OlVirtualName olvn = ve->virtual_name;
	int start, end;

	if (tlp->insert_tab == True && 
		olvn== OL_NEXTFIELD &&
		!(ev->xkey.state)   &&
		(ve->length == 1)   &&
		*ve->buffer == '\011')
		olvn = OL_UNKNOWN_KEY_INPUT;
	
	ve->consumed = True;

	switch(olvn) {
		case OL_UNKNOWN_KEY_INPUT:
			start = tlp->select_start;
			end = tlp->select_end;
			/* Deal with Keycode == 0 first ... cause some of the following
			 * routines apparently cannot handle this ..
			 * Anyway Keycode == 0 => committed string from input-method
			 */
			if (ev->xkey.keycode == 0 && ve->length)
				TextLineEditString(tlw, start, end, ve->buffer,ev);
			else if (IsModifierKey(ve->keysym))
				;
			else if (!(ev->xkey.state & ~(ShiftMask | LockMask)) && ve->length)
				TextLineEditString(tlw, start, end, ve->buffer,ev);
			else if ((_OlFetchMnemonicOwner(w, (XtPointer *) NULL, ve) ||
			     _OlFetchAcceleratorOwner(w, (XtPointer *) NULL, ve)))
				ve->consumed = False; 
			else if (ve->length)
				TextLineEditString(tlw, start, end, ve->buffer,ev);
			break;
		
		case OL_RETURN:
		case OL_NEXTFIELD:
		case OL_PREVFIELD:
			if (InvokeCommitCallbacks(tlw, ev)) {
				if (olvn == OL_RETURN)
					OlLookupInputEvent(w, ev, ve, OL_CORE_IE);
			/* Need to traverse to next widget: Let Primitive handle it*/
				ve->consumed = False;
			}
			break;

		default:
			ve->consumed = EventHandler(tlw, olvn, ev);
			break;
	}
}


/******************************************************************
	EventHandler

	Common routine to handle key events & activation types ...
	Returns -	True: If event is consumed by self
			False: Not consumed , send it to superclass ..

 ******************************************************************/
Private Boolean
EventHandler(TextLineWidget w, OlVirtualName olvn, XEvent *xev)
{
	TextLinePart *tlp = &w->textLine;
	Boolean retval = True;

	/* Fix for 4016104 - undo buffer is freed prematurely causing core dump */
	OlStr undo;
	int size;

	switch (olvn) {
		case OL_DELCHARFWD:
                case OL_DELWORDFWD:
                case OL_DELLINEFWD:
                case OL_DELCHARBAK:
                case OL_DELWORDBAK:
                case OL_DELLINEBAK:
                case OL_DELLINE:
                        Delete(w, olvn, xev);
                        break;

                case OL_CHARFWD:
                case OL_WORDFWD:
                case OL_LINESTART:
                case OL_CHARBAK:
                case OL_WORDBAK:
                case OL_LINEEND:
                        MoveCursor(w, olvn, xev);
                        break;	
		
				case OL_UNDO:

	/* Fix for 4016104 - undo buffer is freed prematurely causing core dump */
						size = strlen(tlp->undo_buffer.string) + 1;
						undo = (OlStr)XtMalloc(sizeof(char *) * size);
						memset((void *)undo, '\0', size);
						memmove((void *)undo, (void *)tlp->undo_buffer.string, size);
						TextLineEditString(w, tlp->undo_buffer.start,
							tlp->undo_buffer.end, undo,xev);
						XtFree(undo);

                        break;
 
                case OL_COPY:
                        (void)OlTLOperateOnSelection((Widget)w, OL_COPY);
                        break;
 
                case OL_CUT:
                        if(w->textLine.edit_type == OL_TEXT_EDIT)
                                (void)OlTLOperateOnSelection((Widget)w, OL_CUT);
                        else
                                _OlBeepDisplay((Widget)w,1);
                        break;
 
                case OL_PASTE:
                        if(w->textLine.edit_type == OL_TEXT_EDIT)
                                Paste(w);
                        else
                                _OlBeepDisplay((Widget)w,1);
                        break;
		default:
			retval = False;
			break;
	}
	return retval;
}

/******************************************************
	Delete 

 ******************************************************/
Private void
Delete(TextLineWidget w, OlVirtualName olvn, XEvent *xev)
{
	TextLinePart *tlp = &w->textLine;
	int start, end;

	if (tlp->select_start != tlp->select_end) {
	  /* If any selection exists, just delete it .... */
		start = tlp->select_start;
		end = tlp->select_end;

	} else {

		switch(olvn) {
			case OL_DELCHARBAK:
				if (tlp->cursor_position == 0) {
					start = end = 0;
				} else {
					start = tlp->cursor_position - 1;
					end = tlp->cursor_position;
				}
				break;
			case OL_DELCHARFWD:
				if (tlp->cursor_position == tlp->num_chars) {
					start = end = tlp->cursor_position;
				} else {
					start = tlp->cursor_position;
					end = tlp->cursor_position + 1;
				}
				break;
			case OL_DELWORDBAK:
				if (tlp->cursor_position == 0) {
					start = end = 0;
				} else {
					end = tlp->cursor_position;
					start = StartOfPreviousWord(w, end);
				}
				break;
			case OL_DELWORDFWD:
				if (tlp->cursor_position == tlp->num_chars) {
					start = end = tlp->cursor_position;
				} else {
					start = tlp->cursor_position;
					end = EndOfCurrentWord(w, start) +1;
				}
				break;
			case OL_DELLINEBAK:
				if (tlp->cursor_position == 0) {
					start = end = 0;
				} else {
					end = tlp->cursor_position;
					start = 0;
				}
				break;
			case OL_DELLINEFWD:
				if (tlp->cursor_position == tlp->num_chars) {
					start = end = tlp->cursor_position;
				} else {
					start = tlp->cursor_position;
					end = tlp->num_chars;
				}
				break;
			case OL_DELLINE:
				start = 0; end = tlp->num_chars;
				break;
		}
	}
	
	TextLineEditString(w, start, end, 
			   _OLStrEmptyString(w->primitive.text_format), 
			   xev);
}

/******************************************************
	MoveCursor 

 ******************************************************/
Private void
MoveCursor(TextLineWidget w, OlVirtualName olvn, XEvent *xev)
{
	TextLinePart *tlp = &w->textLine;
	int new = tlp->cursor_position;
	unsigned long what = 0;

	switch(olvn) {
		case OL_CHARBAK:
			if (tlp->cursor_position == 0)
				return;
			else 
				new--;
			break;
		case OL_CHARFWD:
			if (tlp->cursor_position == tlp->num_chars) 
				return;
			else 
				new++;
			break;
		case OL_WORDBAK:
			if (tlp->cursor_position == 0)
				return;
			else 
				new = StartOfPreviousWord(w, tlp->cursor_position);
			break;
		case OL_WORDFWD:
			if (tlp->cursor_position == tlp->num_chars) 
				return;
			else 
				new = EndOfNextWord(w, tlp->cursor_position) +1;
			break;
		case OL_LINESTART:
			if (tlp->cursor_position == 0)
				return;
			else
				new = 0;
			break;
		case OL_LINEEND:
			if (tlp->cursor_position == tlp->num_chars )
				return;
			else
				new = tlp->num_chars ;
			break;
		default:
			break;
		}

	if (InvokeMotionCallbacks(w,tlp->cursor_position,new, xev, (Boolean)True)) {
		tlp->select_start = tlp->select_end 
				  = tlp->cursor_position 
				  = tlp->caret_pos
				  = new;

		what = _OlTLSetupArrowsAfterCursorChange(w) | TLMoveCaret;
		_OlTLDrawWidget(w, _OLTLStartOfDisplay(tlp), TLEndOfDisplay, what);
	}
}

/******************************************************
	ButtonHandler 

 ******************************************************/
Private void
ButtonHandler(TextLineWidget w, OlVirtualEvent ve)
{
	XEvent *ev = ve->xevent;
	TextLinePart *tlp = &w->textLine;
	OlInputEvent olev = (OlInputEvent)ve->virtual_name;

	if (tlp->preed_on == True) {
		_OlBeepDisplay((Widget)w, 1);
		return;
	}

	if (tlp->edit_type == OL_TEXT_READ) /* NOTE: MENU Key also has no effect */
		return;
	
	ve->consumed = True;

	switch (olev) {
		case OL_SELECT:
			if (w->primitive.has_focus == False)
				OlCallAcceptFocus((Widget)w,((XButtonEvent *)ev)->time);
			Select (w, ev);
			break;
		case OL_ADJUST:
			Adjust (w, ev);
			break;

		case OL_DUPLICATE:
		     {
			int position = ConvertForwardToPos(w, _OLTLStartOfDisplay(tlp),
					      ev->xbutton.x - TextStartPos(tlp));
			if (tlp->select_start <= position && tlp->select_end > position)
				DragText(w, position, OlCopyDrag);
			break;
		     }

		case OL_MENU:
			if (tlp->menu == NULL)
				CreateMenu(w);
			if (tlp->my_menu)
				SetSensitivity(w);
			OlMenuPost(tlp->menu);
			break;
		default:
			ve->consumed = False;
			break;
	}
}

/******************************************************
	CreateMenu 

 ******************************************************/
Private void
CreateMenu(TextLineWidget w)
{
	TextLinePart *tlp = &w->textLine;
	OlStrRep format = w->primitive.text_format;
	Widget menu_pane;
	Widget undo_widget, cut_widget, copy_widget, paste_widget, delete_widget;
	wchar_t *wctitle, *wcundo, *wccut, *wccopy, *wcpaste, *wcdelete;
	char	*title, *undo, *cut, *copy, *paste, *delete;

	title = DGETTEXT ("Edit");
	undo = DGETTEXT ("Undo");
	cut = DGETTEXT ("Cut");
	copy = DGETTEXT ("Copy");
	paste = DGETTEXT ("Paste");
	delete = DGETTEXT ("Delete");
	if (format == OL_WC_STR_REP)
	{
	    size_t num;

	    num = strlen (title) + 1;
	    wctitle = (wchar_t *)XtMalloc (num * sizeof (wchar_t));
	    mbstowcs (wctitle, title, num);

	    num = strlen (undo) + 1;
	    wcundo = (wchar_t *)XtMalloc (num * sizeof (wchar_t));
	    mbstowcs (wcundo, undo, num);

	    num = strlen (cut) + 1;
	    wccut = (wchar_t *)XtMalloc (num * sizeof (wchar_t));
	    mbstowcs (wccut, cut, num);

	    num = strlen (copy) + 1;
	    wccopy = (wchar_t *)XtMalloc (num * sizeof (wchar_t));
	    mbstowcs (wccopy, copy, num);

	    num = strlen (paste) + 1;
	    wcpaste = (wchar_t *)XtMalloc (num * sizeof (wchar_t));
	    mbstowcs (wcpaste, paste, num);

	    num = strlen (delete) + 1;
	    wcdelete = (wchar_t *)XtMalloc (num * sizeof (wchar_t));
	    mbstowcs (wcdelete, delete, num);

	/* Create MenuShell to hold menu-itams */
	    tlp->menu =  XtVaCreatePopupShell("Edit", 
				          menuShellWidgetClass, (Widget)w,
				          XtNmenuAugment, FALSE,
				          XtNshellTitle, wctitle,
				          XtNtextFormat, format,
				          NULL
				         );

	    XtVaGetValues(tlp->menu, XtNmenuPane, &menu_pane, NULL);

	    undo_widget = XtVaCreateManagedWidget("undo_widget", 
				oblongButtonGadgetClass, menu_pane,
				XtNlabel, 	wcundo,
				XtNtextFormat, 	format,
				NULL
			       );
	    cut_widget = XtVaCreateManagedWidget("cut_widget", 
				oblongButtonGadgetClass, menu_pane,
				XtNlabel, 	wccut,
				XtNtextFormat, 	format,
				NULL
			       );
	    copy_widget = XtVaCreateManagedWidget("copy_widget", 
				oblongButtonGadgetClass, menu_pane,
				XtNlabel, 	wccopy,
				XtNtextFormat, 	format,
				NULL
			       );
	    paste_widget = XtVaCreateManagedWidget("paste_widget", 
				oblongButtonGadgetClass, menu_pane,
				XtNlabel, 	wcpaste,
				XtNtextFormat, 	format,
				NULL
			       );
	    delete_widget = XtVaCreateManagedWidget("delete_widget", 
				oblongButtonGadgetClass, menu_pane,
				XtNlabel, 	wcdelete,
				XtNtextFormat, 	format,
				NULL
				);

	    XtFree (wctitle);
	    XtFree (wcundo);
	    XtFree (wccut);
	    XtFree (wccopy);
	    XtFree (wcpaste);
	    XtFree (wcdelete);
	}
	else
	{
	/* Create MenuShell to hold menu-itams */
	    tlp->menu =  XtVaCreatePopupShell("Edit", 
				          menuShellWidgetClass, (Widget)w,
				          XtNmenuAugment, FALSE,
				          XtNshellTitle, title,
				          XtNtextFormat, format,
				          NULL
				         );

	    XtVaGetValues(tlp->menu, XtNmenuPane, &menu_pane, NULL);

	    undo_widget = XtVaCreateManagedWidget("undo_widget", 
				oblongButtonGadgetClass, menu_pane,
				XtNlabel, 	undo,
				XtNtextFormat, 	format,
				NULL
			       );
	    cut_widget = XtVaCreateManagedWidget("cut_widget", 
				oblongButtonGadgetClass, menu_pane,
				XtNlabel, 	cut,
				XtNtextFormat, 	format,
				NULL
			       );
	    copy_widget = XtVaCreateManagedWidget("copy_widget", 
				oblongButtonGadgetClass, menu_pane,
				XtNlabel, 	copy,
				XtNtextFormat, 	format,
				NULL
			       );
	    paste_widget = XtVaCreateManagedWidget("paste_widget", 
				oblongButtonGadgetClass, menu_pane,
				XtNlabel, 	paste,
				XtNtextFormat, 	format,
				NULL
			       );
	    delete_widget = XtVaCreateManagedWidget("delete_widget", 
				oblongButtonGadgetClass, menu_pane,
				XtNlabel, 	delete,
				XtNtextFormat, 	format,
				NULL
			       );
	}
	

	XtAddCallback(undo_widget, XtNselect, MenuUndo, (XtPointer)w);
	XtAddCallback(cut_widget, XtNselect, MenuCut, (XtPointer)w);
	XtAddCallback(copy_widget, XtNselect, MenuCopy, (XtPointer)w);
	XtAddCallback(paste_widget, XtNselect, MenuPaste, (XtPointer)w);
	XtAddCallback(delete_widget, XtNselect, MenuDelete, (XtPointer)w);
}

/******************************************************
	MenuUndo 

 ******************************************************/
Private void
MenuUndo(Widget w, XtPointer client_data, XtPointer call_data)
{
	TextLineWidget tlw = (TextLineWidget)client_data;
	TextLinePart *tlp = &tlw->textLine;

	/* Fix for 4016104 - undo buffer is freed prematurely causing core dump */
	OlStr undo;
	int size;

	size = strlen(tlp->undo_buffer.string) + 1;
	undo = (OlStr)XtMalloc(sizeof(char *) * size);
	memset((void *)undo, '\0', size);
	memmove((void *)undo, (void *)tlp->undo_buffer.string, size);
	TextLineEditString(tlw, tlp->undo_buffer.start, tlp->undo_buffer.end, undo, NULL);
	XtFree(undo);
}

/******************************************************
	MenuCut 

 ******************************************************/
Private void
MenuCut(Widget w, XtPointer client_data, XtPointer call_data)
{
	Widget tlw = (Widget)client_data; 

	(void)OlTLOperateOnSelection(tlw, OL_CUT);
}

/******************************************************
	MenuCopy 

 ******************************************************/
Private void
MenuCopy(Widget w, XtPointer client_data, XtPointer call_data)
{
	Widget tlw = (Widget)client_data; 

	(void)OlTLOperateOnSelection(tlw, OL_COPY);
}

/******************************************************
	MenuPaste 

 ******************************************************/
Private void
MenuPaste(Widget w, XtPointer client_data, XtPointer call_data)
{
	Paste((TextLineWidget)client_data);
}

/******************************************************
	MenuDelete

 ******************************************************/
Private void
MenuDelete(Widget w, XtPointer client_data, XtPointer call_data)
{
        TextLineWidget tlw = (TextLineWidget)client_data;  
	TextLinePart *tlp = &tlw->textLine;
	
	TextLineEditString(tlw,
			   tlp->select_start,
			   tlp->select_end,
			   _OLStrEmptyString(tlw->primitive.text_format),
			   NULL
			  );
}

/******************************************************
	SetSensitivity 

 ******************************************************/
Private void
SetSensitivity(TextLineWidget w)
{

#define UNDO_WIDGET(w) 		(((CompositeWidget)w)->composite.children[0])
#define CUT_WIDGET(w) 		(((CompositeWidget)w)->composite.children[1])
#define COPY_WIDGET(w) 		(((CompositeWidget)w)->composite.children[2])
#define PASTE_WIDGET(w) 	(((CompositeWidget)w)->composite.children[3])
#define DELETE_WIDGET(w) 	(((CompositeWidget)w)->composite.children[4])

	TextLinePart *tlp = &w->textLine;
	OlStrRep format = w->primitive.text_format;
	Widget menu_pane;
	
	XtVaGetValues(tlp->menu, XtNmenuPane, &menu_pane, NULL);

	XtSetSensitive(UNDO_WIDGET(menu_pane), 
		       (tlp->undo_buffer.start != tlp->undo_buffer.end ||
		        _OLStrNumChars(format, tlp->undo_buffer.string) != 0)
		      );
	XtSetSensitive(CUT_WIDGET(menu_pane),
			tlp->select_start != tlp->select_end);
	XtSetSensitive(COPY_WIDGET(menu_pane), tlp->select_start != tlp->select_end); 
	XtSetSensitive(DELETE_WIDGET(menu_pane), 
			tlp->select_start != tlp->select_end);

#undef UNDO_WIDGET
#undef CUT_WIDGET
#undef COPY_WIDGET
#undef PASTE_WIDGET
#undef DELETE_WIDGET

}

/******************************************************
	Select 

 ******************************************************/
Private void
Select(TextLineWidget w, XEvent *ev)
{
	TextLinePart *tlp = &w->textLine;
	ButtonAction action;
	unsigned long what = 0;
	Boolean multiClickPending = False;
	int select_start = tlp->select_start;
	int select_end = tlp->select_end;
	int pos = ConvertForwardToPos(w, _OLTLStartOfDisplay(tlp),
					ev->xbutton.x - TextStartPos(tlp));

	if (InvokeMotionCallbacks(w,tlp->cursor_position, pos, ev, True) == False)
		return;

	/* If No selection exists, Move cursor to pointer-pos on ButtonPress */
	if (select_start == select_end && pos != select_start) {
		tlp->select_start = tlp->select_end 
				  = tlp->cursor_position 
				  = tlp->caret_pos 
				  = pos;

		_OlTLDrawWidget(w, 0, 0, TLMoveCaret);	
	} else if ((select_start !=select_end) && (pos<select_start || pos>=select_end)) {
	/* If selection exists & pos is outside selection-region, Clear
	 * the selection & move cursor to position ... 
	*/
		tlp->select_start = tlp->select_end 
				  = tlp->cursor_position 
				  = tlp->caret_pos 
				  = pos;

		_OlTLDrawWidget(w, _OLTLStartOfDisplay(tlp), TLEndOfDisplay, 
				TLDrawText | TLMoveCaret);	
	}

	do {
		switch((action = _OlPeekAheadForEvents((Widget)w, ev))) {
		case MOUSE_MOVE:
			/* If we are within selection-region, try Drag */
			if (pos >= select_start && pos < select_end &&
				!multiClickPending)
				DragText(w, pos, OlMoveDrag);

			else {	/* Start wipe-thru selection ... */
				tlp->caret_visible = False;
				tlp->anchor = (pos < tlp->select_start ? 
						tlp->select_end : tlp->select_start); 

				ExtendSelection(w, pos);
				(void)_OlTLSetupArrowsAfterCursorChange(w); /*Needed ?*/
				_OlTLDrawWidget(w, _OLTLStartOfDisplay(tlp),
						TLEndOfDisplay,
						TLDrawText | TLDrawCaret);
				tlp->mask = 
			    	  ev->xbutton.state | 1 << (ev->xbutton.button+7);
				XtAppAddTimeOut(XtWidgetToApplicationContext((Widget)w),
					0, WipeThru, (XtPointer)w);
			}
			break;

		case MOUSE_CLICK:
			/* If we are in selection_region, need to ClearSelection etc
			 * If outside, we've already done it during ButtonPress
			*/
			tlp->select_mode = OlselectChar;
			if (pos >= select_start && pos < select_end) {
				tlp->select_start = tlp->select_end 
						  = tlp->cursor_position 
						  = tlp->caret_pos 
						  = pos;

				_OlTLDrawWidget(w, _OLTLStartOfDisplay(tlp),
						TLEndOfDisplay, 
						TLDrawText | TLMoveCaret
					       );
			}
			break;

		case MOUSE_MULTI_CLICK_PENDING:
			multiClickPending = True;
			/* fall thru .... */

		case MOUSE_MULTI_CLICK:
			if (++tlp->select_mode > OlselectLine)
				tlp->select_mode = OlselectChar;

			SetSelection(w, pos);
			what = _OlTLSetupArrowsAfterCursorChange(w);
			_OlTLDrawWidget(w, _OLTLStartOfDisplay(tlp), TLEndOfDisplay,
					what| TLDrawText| TLMoveCaret);
			break;

		case MOUSE_MULTI_CLICK_DONE:
			multiClickPending = False;
			break;

		default:
			break;
		}
	} while (action == MOUSE_MULTI_CLICK_PENDING || action == NOT_DETERMINED);
}

/******************************************************
	Adjust 

 ******************************************************/
Private void
Adjust(TextLineWidget w, XEvent *ev)
{
	TextLinePart *tlp = &w->textLine;
	ButtonAction action;
	int pos;

	pos = ConvertForwardToPos(w, _OLTLStartOfDisplay(tlp), 
					ev->xbutton.x - TextStartPos(tlp));

	if (InvokeMotionCallbacks(w,tlp->cursor_position, pos, ev, True) == False)
		return;

	/* If position is outside selection/no selection exists, Extend selection
	 * to position . If inside selection, Contract selection till position.
	 * Set Anchor point accordingly ...
	 */
	tlp->anchor = (pos < tlp->select_start ? tlp->select_end :
						     tlp->select_start); 
	ExtendSelection(w, pos);
	_OlTLDrawWidget(w, _OLTLStartOfDisplay(tlp), TLEndOfDisplay, 
			TLDrawText | TLMoveCaret);

	do {
		switch((action = _OlPeekAheadForEvents((Widget)w, ev))) {
		case MOUSE_MOVE:
			/* Hide caret ... */
			tlp->caret_visible = False;

			_OlTLDrawWidget(w, 0, 0,  TLDrawCaret);
			tlp->mask =ev->xbutton.state | 1 << (ev->xbutton.button+7);
			XtAppAddTimeOut(XtWidgetToApplicationContext((Widget)w),
					0, WipeThru, (XtPointer)w);
			break;
		default:
			break;
		}
	} while (action == MOUSE_MULTI_CLICK_PENDING || action == NOT_DETERMINED);
}

/******************************************************
	WipeThru 

 ******************************************************/
Private void
WipeThru(XtPointer client_data, XtIntervalId *id)
{
	TextLineWidget w = (TextLineWidget)client_data;
	TextLinePart *tlp = &w->textLine;
	Window root, child;
	int rootX, rootY, winX, winY;
	unsigned int mask;

	XQueryPointer(XtDisplay(w), XtWindow(w), &root, &child, &rootX, &rootY,
			&winX, &winY, &mask);

	if (mask != tlp->mask) {
		tlp->caret_visible = True;
	/* NOTE: Actually we don't really need wipethru_timer at all.
	 * Since Removing the timer is done by the intrinsics  within the
	 * timerhandler & no other function XtRemoves this timer, we don't
	 * need to track the existence of this timer. Hmmmm ... 
	 */
		tlp->wipethru_timer = NULL;
		XUngrabPointer(XtDisplay(w),CurrentTime);
		_OlTLDrawWidget(w, 0, 0, TLMoveCaret);
	} else {
		int pos;
		unsigned long what = 0;
		Boolean select_start_changed = False;
		int start_pos = 0, end_pos = 0;
		int old_select_start = tlp->select_start,
		    old_select_end = tlp->select_end;

		if (winX >= (int)TextStartPos(tlp)) {
			winX -= TextStartPos(tlp);
			pos = ConvertForwardToPos(w, _OLTLStartOfDisplay(tlp), winX);
		} else {
			if (winX < 0) winX = -winX + TextStartPos(tlp);
			pos = ConvertBackwardToPos(w, _OLTLStartOfDisplay(tlp), winX);
		}

		ExtendSelection(w, pos);

#define CHANGED(a, b) ((a) != (b))

		/* Compute the minimal region to be redrawn ... */
		if (CHANGED (tlp->select_start, old_select_start)) {
			select_start_changed = True;
                       	start_pos = _OlMin(tlp->select_start, old_select_start);
                       	end_pos = _OlMax(tlp->select_start, old_select_start);
		}

		if (CHANGED (tlp->select_end, old_select_end)) {
			if (!select_start_changed)
				start_pos = _OlMin(tlp->select_end, old_select_end);
			end_pos = _OlMax(tlp->select_end, old_select_end);
		}
#undef CHANGED

		/* Caret is invisible .. Hence no TLMoveCaret below */
		if ((what = _OlTLSetupArrowsAfterCursorChange(w)) & TLDrawText) {
                        /* Whole text needs to be redrawn ... */
                        start_pos = _OLTLStartOfDisplay(tlp);
                        end_pos = TLEndOfDisplay;
                }
		_OlTLDrawWidget(w, start_pos, end_pos, what | TLDrawText);

		XtAppAddTimeOut(XtWidgetToApplicationContext((Widget)w),
					0, WipeThru, (XtPointer)w);
	}
}

/******************************************************
	ExtendSelection 

 ******************************************************/
Private void
ExtendSelection(TextLineWidget w, int pos)
{
	TextLinePart *tlp = &w->textLine;
	int anchor = tlp->anchor;

	switch(tlp->select_mode) {
	case OlselectLine:
		tlp->select_start = 0;
		tlp->select_end = tlp->num_chars;
		tlp->cursor_position = tlp->select_end ;
		break;
	case OlselectWord:
		if (pos > anchor) {
			tlp->select_start = anchor;
			tlp->select_end = EndOfCurrentWord(w, pos) +1;
			tlp->cursor_position = tlp->select_end;
		} else {
			tlp->select_end = anchor +1;
			tlp->select_start = StartOfCurrentWord(w, pos);
			tlp->cursor_position = tlp->select_start;
		}
		break;
	case OlselectChar:
		if (pos >= anchor) {
			tlp->select_start = anchor;
			tlp->select_end = pos ;
			tlp->cursor_position = tlp->select_end;
		} else {
			tlp->select_end = anchor +1;
			tlp->select_start = pos;
			tlp->cursor_position = tlp->select_start;
		}
		break;
	default:
		break;
	}

	tlp->caret_pos = tlp->cursor_position;

	if (tlp->select_start < tlp->select_end && !XtOwnSelection((Widget)w, XA_PRIMARY,
	  	XtLastTimestampProcessed(XtDisplay((Widget)w)),
		ConvertPrimary, LosePrimary, NULL))
	{
		_OLTLWarn("TextLineWidget: Cannot get Primary Selection");
		tlp->select_start = tlp->select_end = tlp->cursor_position;
	}
}

/******************************************************
	SetSelection 

 ******************************************************/
Private void
SetSelection(TextLineWidget w, int pos)
{
	TextLinePart *tlp = &w->textLine;

	switch(tlp->select_mode) {
	case OlselectLine:
		tlp->select_start = 0;
		tlp->select_end = tlp->num_chars ;
		break;
	case OlselectWord:
		tlp->select_start = StartOfCurrentWord(w, pos);
		tlp->select_end = EndOfCurrentWord(w, tlp->select_start) + 1;
		break;
	case OlselectChar:
		tlp->select_start = pos;
		tlp->select_end = pos;
		break;
	default:
		break;
	}

	tlp->caret_pos = tlp->cursor_position = tlp->select_end;

	if (tlp->select_start <tlp->select_end && !XtOwnSelection((Widget)w, XA_PRIMARY,
	  	XtLastTimestampProcessed(XtDisplay((Widget)w)), 
		ConvertPrimary, LosePrimary, NULL))
	{
		_OLTLWarn("TextLineWidget: Cannot get Primary Selection");
		tlp->select_start = tlp->select_end = tlp->cursor_position;
	}
}

/******************************************************
	inWordSB 

 ******************************************************/
Private Boolean 
inWordSB(OlStr str, int pos)
{
	unsigned char  c = (unsigned char)(*((char *)str + pos));
	Boolean retval = isalnum((int)c) || (c == '_') ||
			(192 <= (int)c  && (int)c <=255);

	return (retval);
}

/******************************************************
	inWordMB 

 ******************************************************/
Private Boolean 
inWordMB(OlStr str, int pos)
{
	Boolean retval = False;
	int size;
	char *s = (char *)str + pos;

	if((size = mblen((char *)s, MB_CUR_MAX)) == 1)
        	retval = isgraph((int)(*((unsigned char *)s)));
	else if(size > 1)
        	retval = True;
        
	return (retval);
}

/******************************************************
	inWordWC 

 ******************************************************/
Private Boolean 
inWordWC(OlStr str, int pos)
{
	wchar_t wc = (wchar_t)(*((wchar_t *)str + pos));
	return (iswgraph(wc));
}

typedef Boolean (*InWordFunc)(OlStr str, int pos);
Private InWordFunc in_word_array[NUM_SUPPORTED_REPS] = {inWordSB, inWordMB, inWordWC};

/***************************************************************************
	StartOfCurrentWord 

  Locates the beginning of the word within which "position" lies. If "position"
  is in a "white-space" region, the start of the word before the white-space
  is returned ...

 ***************************************************************************/
Private int
StartOfCurrentWord(TextLineWidget w, int pos)
{
	TextLinePart *tlp = &w->textLine;
	OlStrRep format = w->primitive.text_format;
	OlStr str = String(tlp); 
	int uPos = toUnitPos(str, pos, format, tlp->pos_table);
	InWordFunc in_word = in_word_array[format]; 

	if (in_word(str, uPos)) {
		while (pos >= 0 && in_word(str, uPos)) {
			pos--;
			uPos = toUnitPos(str, pos,format, tlp->pos_table);
		}
	} else {
		while (pos >= 0 && !in_word(str, uPos)) {
			pos--;
			uPos = toUnitPos(str, pos,format, tlp->pos_table);
		}
		while (pos >= 0 && in_word(str, uPos)) {
			pos--;
			uPos = toUnitPos(str, pos,format, tlp->pos_table);
		}
	}
	return ++pos;
}

/***********************************************************************
	EndOfCurrentWord 

  Locates the end of the word within which "position" lies. If "position"
  is in a "white-space" region, the end of the word after the white-space
  is returned ...

 *********************************************************************/
Private int 
EndOfCurrentWord(TextLineWidget w, int pos)
{
	TextLinePart *tlp = &w->textLine;
	OlStrRep format = w->primitive.text_format;
	OlStr str = String(tlp);
	int uPos = toUnitPos(str, pos, format, tlp->pos_table);
	InWordFunc in_word = in_word_array[format];

	if (in_word(str, uPos)) {
		while (pos < tlp->num_chars && in_word(str, uPos)) {
			pos++;
			uPos = toUnitPos(str, pos,format, tlp->pos_table);
		}
	} else {
		while (pos < tlp->num_chars && !in_word(str, uPos)) {
			pos++;
			uPos = toUnitPos(str, pos,format, tlp->pos_table);
		}
		while (pos < tlp->num_chars && in_word(str, uPos)) {
			pos++;
			uPos = toUnitPos(str, pos,format, tlp->pos_table);
		}
	}
	return --pos;
}

/******************************************************
	StartOfPreviousWord 

 ******************************************************/
Private int
StartOfPreviousWord(TextLineWidget w, int pos)
{
	if (pos == 0) 
		return pos;
	else 
		return StartOfCurrentWord(w, --pos);
}

/******************************************************
	EndOfNextWord 

 ******************************************************/
Private int
EndOfNextWord(TextLineWidget w, int pos)
{
	if (pos == w->textLine.num_chars-1)
		return pos;
	else
		return EndOfCurrentWord(w, ++pos);
}


/******************************************************
	ConvertPrimary 

 ******************************************************/
Private Boolean
ConvertPrimary(Widget w, Atom *selection, Atom *target, Atom *type_return,
               XtPointer *value_return, unsigned long *length_return,int *format_return)
{
	TextLineWidget 	tlw = (TextLineWidget)w;
	TextLinePart 	*tlp = &tlw->textLine;
    	Boolean		retval = False;
    	Atom		*atoms = NULL;
    	OlStr		ptr;
    	Atom		atom;
    	Display		*dpy = XtDisplay(w);
	OlStrRep	format = tlw->primitive.text_format;

    if (*selection == XA_PRIMARY) {
	if (*target == OlInternAtom(dpy, TARGETS_NAME)) {
	    *format_return = (int)(8*sizeof(Atom));
	    *length_return = (unsigned long)3;
	    atoms          = (Atom *)XtMalloc((unsigned)
				((*length_return)*(sizeof(Atom))));
	    atoms[0]       = OlInternAtom(dpy, TARGETS_NAME);
	    atoms[1]       = XA_STRING;
	    atoms[2]       = OlInternAtom(dpy, DELETE_NAME);
	    atoms[3] 	   = OlInternAtom(dpy, COMPOUND_TEXT_NAME); 
	    *value_return  = (XtPointer)atoms;
	    *type_return   = XA_ATOM;
	    retval         = True;
	} else if (*target == XA_STRING) {	/* We give him a multi-byte string */
	    OlStr buffer;
	    if (tlp->select_start != tlp->select_end) { /* Selection exists */

	    	buffer = _OlTLGetSubString(tlp->buffer, tlp->select_start,
			    tlp->select_end -1, format, tlp->pos_table);

	   	if (format == OL_WC_STR_REP) {  /* WideChar - convert to MB  */
			size_t size = sizeof(char) * MB_CUR_MAX * 
					(wslen((wchar_t *)buffer)+1);
			ptr = (char *)XtMalloc(size);
			(void)wcstombs(ptr, (wchar_t *)buffer, size);
			XtFree(buffer);
		}
		else ptr = buffer;
			
		*format_return = 8;
		*length_return = strlen((const char *)ptr) + 1;
						/* include null byte */
		*value_return = (XtPointer)ptr;
		*type_return   = XA_STRING;
		retval = True;
	    }
	    else retval = False;
	} else if (*target == OlInternAtom(dpy, COMPOUND_TEXT_NAME)) {
	    int	        cvt_len;
	    OlStr	cvt_str;
      
	    if (tlp->select_start != tlp->select_end) { /* Selection exists */
		ptr = _OlTLGetSubString(tlp->buffer, tlp->select_start,
					tlp->select_end -1, format, tlp->pos_table);
	/* Convert to CompoundText ... This function returns a null-terminated
	 * compound string . But, the "length" of the string returned (cvt_len)
	 * does not include the NULL byte ... So add 1 ...
	 */
	    	cvt_str = _OLStrToCT(format, dpy, ptr, &cvt_len);
	    	*format_return = 8;
	    	*value_return = (XtPointer)cvt_str;
	    	*length_return = cvt_len + 1; 
	    	*type_return = XA_STRING;
	    	retval = True;
	    }
	    else 
		retval = False;
	} else if (*target == OlInternAtom(dpy, DELETE_NAME)) {
	/* Yup, someone's done a CUT on us ! */
	    *format_return = NULL;
	    *length_return = NULL;
	    *value_return = NULL;
	    *type_return = (Atom)*target;
	    TextLineEditString(tlw, tlp->select_start, tlp->select_end,
				      _OLStrEmptyString(format), (XEvent *)NULL);
	    retval = True;
	} else if (*target == OlInternAtom(dpy, _SUN_SELN_YIELD_NAME)) {
	    LosePrimary(w, selection);
	    *format_return = NULL;
	    *length_return = NULL;
	    *value_return = NULL;
	    *type_return = NULL;
	    *target = NULL;
	    retval = False;
	} else 
	    OlWarning(DGETTEXT( "TextLine: Can't convert PRIMARY"));
    }
    return (retval);

} /* end of ConvertPrimary */

/******************************************************
	LosePrimary 

 ******************************************************/
Private void
LosePrimary(Widget w, Atom *atom)
{
	TextLinePart *tlp = &((TextLineWidget)w)->textLine;

	/* Reset selections */
	tlp->select_start = tlp->select_end = tlp->cursor_position;

	/* Since the cursor is visible (Right ?!) no need to invoke
	 * _OlTLSetupArrowsAfterCursorChange now ...
	 */
	_OlTLDrawWidget((TextLineWidget)w, _OLTLStartOfDisplay(tlp), 
			TLEndOfDisplay, TLDrawText);
}

/******************************************************
	ConvertClipboardOrDnD 

 ******************************************************/
Private Boolean
ConvertClipboardOrDnD(Widget w, Atom *selection, Atom *target, 
		Atom *type_return, XtPointer *value_return, 
		unsigned long *length_return, int *format_return)
{
    TextLineWidget	tlw = (TextLineWidget)w;
    TextLinePart 	*tlp = &tlw->textLine;
    Display 		*dpy = XtDisplay(w);
    Atom          	*atoms = NULL;
    int             	i;
    OlStr		data; 
    char *          	buffer;
    Boolean         	retval = False;
    OlStrRep	    	format = tlw->primitive.text_format;

    if (*selection == XA_CLIPBOARD(dpy) || *selection == tlp->transient)
    {
	data = (*selection == XA_CLIPBOARD(dpy) ? 
			tlp->clip_contents : tlp->dnd_contents);
	if (*target == OlInternAtom(dpy,TARGETS_NAME)) {
	    *format_return = (int) (8 * sizeof(Atom));
	    *length_return = (unsigned long) 5;
	    atoms          = (Atom *) XtMalloc((unsigned) 
				((*length_return) * (sizeof(Atom))));
	    atoms[0]       = OlInternAtom(dpy,TARGETS_NAME);
	    atoms[1]       = XA_STRING;
	    atoms[2]       = OlInternAtom(dpy, DELETE_NAME);
	    atoms[3]       = OlInternAtom(dpy,LENGTH_NAME);
	    atoms[4]       = OlInternAtom(dpy,COMPOUND_TEXT_NAME);
	    *value_return  = (XtPointer) atoms;
	    *type_return   = XA_ATOM;
	    retval         = True;
	} else if (*target == XA_STRING) {
	    /* NOTE: Clip_conts is always in MB format .. */
            /* clip_contents is freed in OlTlOperate..(COPY/CUT) */

	    buffer = XtNewString(((char *)data));
	    *format_return = 8;
	    *length_return = strlen((const char *)buffer) + 1; 
						/* include null byte */ 
	    *value_return = (XtPointer)buffer;
	    *type_return   = XA_STRING;
	    retval = True;
	} else if(*target == OlInternAtom(dpy, COMPOUND_TEXT_NAME)) {   
	    OlStrRep format = OL_MB_STR_REP;	/* override format */
	    int cvt_len; OlStr cvt_str;  
	    cvt_str        = _OLStrToCT(format, dpy, data, &cvt_len);
	    *format_return = 8;
	    *length_return = cvt_len + 1;	/* include null byte */
	    *value_return  = (XtPointer) cvt_str;
	    *type_return   = (Atom)*target;
	    retval         = True;
	} 
	/* DnD Move-Operation .... */
	else if (*target == OlInternAtom(dpy,  DELETE_NAME)) {
	    *format_return = 8;
	    *length_return = NULL;
	    *value_return  = NULL;
	    *type_return   = (Atom)*target;
            (void) TextLineEditString(tlw, tlp->select_start, tlp->select_end,
					_OLStrEmptyString(format), (XEvent *)NULL);
	    retval         = True;
	}
#ifdef sun			/* for supporting target LENGTH_NAME and to suppress */
				/* warnings for targets generated by Xview ... */

	else if (*target == OlInternAtom(dpy, LENGTH_NAME)) {
	    int *	intbuffer;
	    size_t size; 
	    
	    if (format == OL_WC_STR_REP) {  
	    /* Strings are passed thru the selection stuff in MB format . 
	     * Hence we need to find the length of this string in MB format ..
	     * Maybe we can store into clip_conts in MB format in the first place !
	    */
	    	size = sizeof(char) * MB_CUR_MAX * (wslen((wchar_t *)data)) +1;
		buffer = (char *)XtMalloc(size);
		size = wcstombs(buffer, (wchar_t *)data, size) + 1;
		XtFree(buffer);
	    } else
		size = strlen((const char *)data) + 1;

	    intbuffer      = (int *)XtMalloc(sizeof(int));
	    *intbuffer     = (int)size;
	    *value_return  = (XtPointer)intbuffer;
	    *length_return = 1;
	    *format_return = sizeof(int)*8;
	    *type_return   = (Atom)*target;
	    retval         = True;
	} else {
	    char *          atom;
	    static char     prefix[] = "_SUN_SELN";

	    atom = XGetAtomName(dpy, *target);
	    if (strncmp(prefix, atom, strlen(prefix)) != 0)
		OlWarning(DGETTEXT("TextLine: Can't convert"));
	    XFree(atom);
	    retval = False;
	}
	
#else				/* without LENGTH_NAME and warning suppression */
	else {
	    OlWarning(DGETTEXT("TextLine: Can't convert"));
	    retval = False;
	} 
#endif
    }
    return (retval);
}				/* end of ConvertClipboardOrDnD */

/******************************************************
	LoseClipboardOrDnD 

 ******************************************************/
Private void
LoseClipboardOrDnD(Widget w, Atom *atom)
{
	TextLinePart *tlp = &((TextLineWidget)w)->textLine;

	if (tlp->transient != (Atom)NULL && tlp->transient == *atom) {
		/* Lost DnD selection .. */
		if (tlp->dnd_contents != (OlStr)NULL) {
			XtFree (tlp->dnd_contents);
			tlp->dnd_contents = (OlStr)NULL;
		}
		OlDnDFreeTransientAtom(w , tlp->transient);
		tlp->transient = (Atom) NULL;
	} else if (tlp->clip_contents != NULL) {
		/* Lost Clipboard ... */
			XtFree(tlp->clip_contents);
			tlp->clip_contents = NULL;
		}
}

/******************************************************
	Paste 

 ******************************************************/
Private void
Paste(TextLineWidget w)
{
	XtGetSelectionValue((Widget) w, XA_CLIPBOARD(XtDisplay((Widget)w)),
	    	XA_STRING, GetClipboardOrDnD, NULL, 
		XtLastTimestampProcessed(XtDisplay((Widget)w)));
}

/*  DragNDrop stuff .... */
typedef struct _drop_closure {
	int                	position;
	OlDnDTriggerOperation	operation;
	Boolean			send_done;
} DropClosure, *DropClosurePtr;

typedef struct _selectTargetRec {
	DropClosurePtr drop;
	Time            timestamp;
} SelectTargetRec, *SelectTargetPtr;

/******************************************************
	GetClipboardOrDnD 

  Selection callbackProc that gets invoked when a selection is available in
  the ClipBoard OR when DnD is done over us ...
 
 ******************************************************/
Private void 
GetClipboardOrDnD(Widget w, XtPointer client_data, Atom *selection, Atom *type,
XtPointer value, unsigned long *length, int *format)
{
	TextLineWidget tlw = (TextLineWidget)w;
	TextLinePart *tlp = &tlw->textLine;
	DropClosurePtr  drop = (DropClosurePtr) client_data;
	Display *dpy = XtDisplay(w);

	if (value != NULL && *length != 0) {
		Boolean retval = False;
		OlStrRep format = tlw->primitive.text_format;
		OlStr *strbufptr = NULL;
		OlStr cvt_str = (OlStr)NULL;
		int cvt_len;
		int start, end;

		/* Force NULL termination ! */
		if (((char *) value)[*length - 1] != '\0') {
	    		value = (XtPointer) XtRealloc((char *) value, ++*length);
	    		((char *) value)[*length - 1] = '\0';
		}

		if (*type == XA_STRING) {
			if(format == OL_WC_STR_REP) { 
				/* incoming value is always multi-byte */
				cvt_str = (OlStr)XtMalloc(sizeof(wchar_t) * *length);
				(void)mbstowcs((wchar_t *)cvt_str,(char *)value,*length);
				cvt_len = wslen((wchar_t *)cvt_str) + 1;
			} else {
				cvt_len = *length;
				cvt_str = (OlStr)value;
			}
			
		} else if(*type == OlInternAtom(dpy,COMPOUND_TEXT_NAME)) {
			strbufptr = _OLStrFromCT(format, dpy, (char *)value);
			if (strbufptr != (OlStr *)NULL) {
	    			cvt_str = *strbufptr;
	   	       		cvt_len = _OLStrNumBytes(format, cvt_str);
			} else {
	    			cvt_str = _OLStrEmptyString(format);
	    			cvt_len = 0;
			}
		} 
            
	/* Insert string : at caret_pos if PASTE; at pointer-position if DnD */
		if (drop != (DropClosurePtr)NULL)
			start = end = drop->position;
		else {
			start = tlp->select_start;
			end = tlp->select_end;
		}
		TextLineEditString(tlw, start, end, cvt_str, (XEvent *)NULL);

		if(strbufptr != (OlStr *)NULL)
			_OLStrFreeList(format, strbufptr);
		XtFree(value);

	/* If the DnD performed was a DRAG_MOVE, request the selection to be
	 * deleted ....
 	 */
		if ((drop !=(DropClosurePtr)NULL) && 
			(drop->operation ==OlDnDTriggerMoveOp)) 
			XtGetSelectionValue(w, *selection,
				OlInternAtom(dpy, DELETE_NAME), AckDel,
				(XtPointer)drop->send_done, 
				XtLastTimestampProcessed(dpy));
	}

	/* AckDel() calls OlDnDDragNDropDone(), so if we call
	 * XtGetSelectionValue() above on DELETE_NAME,
	 * we do NOT want to call the done proc again here.
	 */

    if (drop != (DropClosurePtr) NULL && drop->send_done && 
	!(((*length) != 0) && (drop->operation == OlDnDTriggerMoveOp)))
	OlDnDDragNDropDone(w, *selection, XtLastTimestampProcessed(dpy), NULL, NULL);
}

/******************************************************
	AckDel 

 ******************************************************/
Private void
AckDel(Widget w, XtPointer client_data, Atom *selection, Atom *type, XtPointer value, 
	long unsigned int *length, int *format)
{
	if ((Boolean) client_data) 
		OlDnDDragNDropDone(w, *selection, 
			XtLastTimestampProcessed(XtDisplay(w)), NULL, NULL);
}

/**********************************************************************
	OlTLOperateOnSelection 

  COPY / CUT from the owner of the PrimarySelection & stuff it into the
  ClipBoard. NOTE that the PrimarySelection owner need not be myself ...
  Ideally, I should get XA_TARGETS, confirm that XA_STRING is supported &
  then do the remaining stuff .... !
 
 **********************************************************************/
PublicInterface Boolean
OlTLOperateOnSelection(Widget w, int mode)
{
	Display *dpy = XtDisplay(w);
	Boolean retval = True;

	switch (mode) {
	  case OL_CUT:
	  case OL_COPY:
		XtGetSelectionValue(w, XA_PRIMARY, XA_STRING, GetPrimary, 
			(XtPointer)mode,XtLastTimestampProcessed(dpy));
		break;
	  case OL_PASTE:
		if (((TextLineWidget)w)->textLine.edit_type == OL_TEXT_EDIT)
			Paste((TextLineWidget)w);
		else {
			_OlBeepDisplay(w,1);
			retval = False;
		}
		break;
	  case OL_CANCEL:
		XtDisownSelection(w, XA_PRIMARY,XtLastTimestampProcessed(dpy));
		/* LosePrimary proc does the rest ... ! */
		break;
	}
	return retval;
}

/******************************************************
	GetPrimary 

 ******************************************************/
Private void
GetPrimary(Widget w, XtPointer client_data, Atom *selection, Atom *type,
		XtPointer value, unsigned long *length, int *format)
{
	TextLinePart *tlp = &((TextLineWidget)w)->textLine;
	Display *dpy = XtDisplay(w);

	if (*type == None) {
		_OlBeepDisplay(w, 1);
		return;
	} else if (*type == XA_STRING) {
		if (!XtOwnSelection( w, XA_CLIPBOARD(dpy),
			XtLastTimestampProcessed(dpy),
			ConvertClipboardOrDnD, LoseClipboardOrDnD, NULL))
			OlWarning(DGETTEXT("TextLine: Cannot get the selection\n"));
		else {
			if (tlp->clip_contents != (OlStr)NULL) {
				XtFree(tlp->clip_contents);
				tlp->clip_contents = (OlStr)NULL;
			}
	/* NOTES: (1) I am not looking for COMPOUND_TEXT now ...
		  (2) I assume that the PRIMARY stuff is in MB format ..
		  (3) I Assume that the PRIMARY stuff is NULL-terminated ..
	*/
			tlp->clip_contents = (OlStr)value;
			if ((int)client_data == OL_CUT)
				XtGetSelectionValue(w, XA_PRIMARY, 
					OlInternAtom(dpy, DELETE_NAME),
					dummy, NULL,
					XtLastTimestampProcessed(dpy));
		}
	}
}

Private void
dummy()
{
}

/******************************************************
	TriggerNotify 

 ******************************************************/
Private void
TriggerNotify(Widget w, Window win, Position x, Position y, Atom selection,
		Time timestamp, OlDnDDropSiteID drop_site,
		OlDnDTriggerOperation op, Boolean send_done, Boolean forwarded,
		XtPointer closure)
{
	DropClosurePtr  drop;
	SelectTargetPtr select;
	Window          dummy_w;
	int             lx, ly;
	Display 	*dpy = XtDisplay(w);
	TextLinePart 	*tlp = &((TextLineWidget)w)->textLine;

	if (!XtIsSensitive(w)) {
		if (send_done)
			OlDnDDragNDropDone(w, selection, 
				XtLastTimestampProcessed(dpy), NULL,NULL);
                return ;
	}	
	if (tlp->edit_type != OL_TEXT_EDIT) {
		_OlBeepDisplay(w, 1);
		if (send_done)
			OlDnDDragNDropDone(w, selection,
				XtLastTimestampProcessed(dpy), NULL,NULL);
		return;
	}

	drop = (DropClosurePtr)XtMalloc(sizeof(DropClosure));
	select = (SelectTargetPtr)XtMalloc(sizeof(SelectTargetRec));

	XTranslateCoordinates(dpy, RootWindowOfScreen(XtScreen(w)), XtWindow(w),
			(int)x, (int)y, &lx, &ly, &dummy_w);
	if (lx < (int)TextStartPos(tlp)) lx = TextStartPos(tlp);
	if (lx > (int)TextEndPos(w, tlp)) lx = TextEndPos(w, tlp);

	drop->position = ConvertForwardToPos((TextLineWidget)w, 
				_OLTLStartOfDisplay(tlp), lx - TextStartPos(tlp));
	drop->operation = op;
	drop->send_done = send_done;
	select->drop = drop;
	select->timestamp = timestamp;

	XtGetSelectionValue(w, selection, OlInternAtom(dpy, TARGETS_NAME),
				SelectTarget, (XtPointer) select, timestamp);
	return;
}

/******************************************************
	SelectTarget 

 ******************************************************/
Private void
SelectTarget(Widget w, XtPointer client_data, Atom *selection, Atom *type,
		XtPointer value, unsigned long *length, int *format)
{
	SelectTargetPtr select = (SelectTargetPtr)client_data;
	DropClosurePtr drop = select->drop;
	Time timestamp = select->timestamp;
	Atom *targets = (Atom *)value;
	Boolean file_name_present, string_present, compound_text_present; 
	Atom compound_text = OlInternAtom(XtDisplay(w),COMPOUND_TEXT_NAME);
	/* "FILE_NAME" to be put in OpenLookP.h */
	Atom file_name = OlInternAtom(XtDisplay(w), "FILE_NAME");

	file_name_present = string_present = compound_text_present = False;

	if (*type == XA_ATOM && *length != 0) {
		int i;

		for(i= 0; i < *length; i++) {
		   if (targets[i] == file_name) 
			file_name_present = True;
		   else if (targets[i] == XA_STRING)
			string_present = True;
		   else if (targets[i] == compound_text)
			compound_text_present = True;
		}

		if (file_name_present)
			XtGetSelectionValue(w, *selection, file_name,
				GetClipboardOrDnD, (XtPointer)drop, timestamp);
		else if (string_present)
			XtGetSelectionValue(w, *selection, XA_STRING,
				GetClipboardOrDnD, (XtPointer)drop, timestamp);
		else if (compound_text_present)
			XtGetSelectionValue(w, *selection, compound_text,
				GetClipboardOrDnD, (XtPointer)drop, timestamp);
	}
}

/******************************************************
	DragText 

 ******************************************************/
Private void
DragText(TextLineWidget w, int position, OlDragMode mode)
{
	TextLinePart 		*tlp = &w->textLine;
	Window 			drop_window;
	Position 		x;
	Position 		y;
	OlStr 			buffer;
	OlDnDCursors 		cursors;
	OlDnDDragDropInfo 	rinfo;

	buffer = OlTLGetSubString((Widget)w, tlp->select_start, 
		  	_OlMin(3, tlp->select_end - tlp->select_start));

	cursors = _OlCreateDnDCursors((Widget)w, buffer, w->primitive.font, 
			w->primitive.text_format, mode);

	OlGrabDragPointer((Widget)w, cursors.Drag, None);

	if (OlDnDDragAndDrop((Widget)w, &drop_window, &x, &y, &rinfo,
			 (OlDnDPreviewAnimateCbP)_OlDnDAnimate , (XtPointer)&cursors)) {
		TextDropOnWindow((Widget)w, drop_window, x, y, mode, &rinfo);
	} else 
		_OlBeepDisplay((Widget)w, 1);
			  
	OlUngrabDragPointer((Widget)w);
	_OlFreeDnDCursors((Widget)w, cursors);
}

/******************************************************
	TextDropOnWindow 

 ******************************************************/
Private void
TextDropOnWindow(Widget w, Window drop_window, Position x, Position y,
		 OlDragMode drag_mode, OlDnDDragDropInfoPtr rinfo)
{
	TextLineWidget tlw = (TextLineWidget)w;
	TextLinePart *tlp = &tlw->textLine;
	OlStrRep format = tlw->primitive.text_format;
	Boolean got_selection;

	if (tlp->transient == (Atom)NULL) {
		tlp->transient = OlDnDAllocTransientAtom(w);
		got_selection = False;
	} else
		got_selection = True;
	if (!got_selection)
		got_selection = OlDnDOwnSelection(w, tlp->transient,
				  rinfo->drop_timestamp, ConvertClipboardOrDnD,
				  LoseClipboardOrDnD,(XtSelectionDoneProc)NULL,
				  CleanupTransaction, NULL);
	if (got_selection) {
		OlStr buff;
		if (tlp->dnd_contents != (OlStr)NULL) {
			XtFree(tlp->dnd_contents);
			tlp->dnd_contents = (OlStr)NULL;
		}
		buff = _OlTLGetSubString(tlp->buffer, tlp->select_start,
				tlp->select_end-1, format,tlp->pos_table);
		if (format == OL_WC_STR_REP) {
		/* Convert to MB */
			size_t size = sizeof(char) * MB_CUR_MAX * 
					wslen((wchar_t *)buff) + 1;
			tlp->dnd_contents = (OlStr)XtMalloc(size);
			(void)wcstombs(tlp->dnd_contents,(wchar_t *)buff,size);
			XtFree(buff);
		} else
			tlp->dnd_contents = buff;
		if (!OlDnDDeliverTriggerMessage(w, rinfo->root_window,
				rinfo->root_x, rinfo->root_y,tlp->transient,
				(drag_mode == OlMoveDrag)? OlDnDTriggerMoveOp:
					OlDnDTriggerCopyOp,
				rinfo->drop_timestamp)) {
			_OlBeepDisplay(w, 1);
			OlDnDDisownSelection(w, tlp->transient, rinfo->drop_timestamp);
			OlDnDFreeTransientAtom(w, tlp->transient);
			tlp->transient = (Atom)NULL;
			/* Should we free dnd_contents ? */
		}
	}
}

/******************************************************
	CleanupTransaction 

 ******************************************************/
Private void
CleanupTransaction(Widget w, Atom selection, OlDnDTransactionState state, 
		   Time timestamp,XtPointer closure)
{
	TextLinePart *tlp = &((TextLineWidget)w)->textLine;
	
	switch (state) {
		case OlDnDTransactionDone:
		case OlDnDTransactionRequestorError:
		case OlDnDTransactionRequestorWindowDeath:
			if (selection != tlp->transient)
				break;
			OlDnDDisownSelection(w, selection, CurrentTime);
			OlDnDFreeTransientAtom(w, tlp->transient);
			tlp->transient = (Atom)NULL;
			if (tlp->dnd_contents != (OlStr)NULL) {
				XtFree(tlp->dnd_contents);
				tlp->dnd_contents = (OlStr)NULL;
			}
			break;
		case OlDnDTransactionEnds:
		case OlDnDTransactionBegins:
			break;
	}
}


/******************************************************
	OlTLGetPosition 

 ******************************************************/
PublicInterface int
OlTLGetPosition(Widget w, int position)
{
	TextLineWidget tlw; 
	TextLinePart *tlp; 
	int pos;

	GetToken();

	tlw = (TextLineWidget)w;
	tlp = &tlw->textLine;

	switch (position) {
	  case OL_CURSORPOS:
		pos = tlp->cursor_position;
		break;
	  case OL_BEGIN_CURRENT_WORD:
		pos = StartOfCurrentWord(tlw, tlp->cursor_position);
		break;
	  case OL_END_CURRENT_WORD:
		pos = EndOfCurrentWord(tlw, tlp->cursor_position) + 1;
		break;
	  case OL_END_LINE:
		pos = tlp->num_chars;
		break;
	}

	ReleaseToken();
	return pos;
}

/******************************************************
	OlTLGetSubString 

 ******************************************************/
PublicInterface OlStr
OlTLGetSubString(Widget w, int start, int length)
{
	TextLinePart *tlp;
	int end = start + length - 1;
	OlStr retval = (OlStr)NULL;
	
	GetToken();

	tlp= &((TextLineWidget)w)->textLine;

	if (end >= tlp->num_chars || start > end) {
		ReleaseToken();
		return retval;
	}
	
	/* Gets the string between start & end INCLUSIVELY .. */
	retval = _OlTLGetSubString(tlp->buffer, start, end, 
			((TextLineWidget)w)->primitive.text_format,
			tlp->pos_table);

	ReleaseToken();
	return retval;
}

/******************************************************
	OlTLSetSubString 

 ******************************************************/
PublicInterface Boolean
OlTLSetSubString(Widget w, int start, int length, OlStr string)
{
	TextLinePart *tlp; 
	OlStrRep format; 
	int num_insert_chars; 
	int end; 
	int select_start, select_end;
	OlTLPreModifyCallbackStruct pre_cd;
	OlTLPostModifyCallbackStruct post_cd;
	unsigned long what = 0;

	GetToken();

	tlp = &((TextLineWidget)w)->textLine;
	format = ((TextLineWidget)w)->primitive.text_format;

	if (string == (OlStr)NULL) 
		string = _OLStrEmptyString(format);
	num_insert_chars = _OLStrNumChars(format, string);
	end = start + length;

	if (end > tlp->num_chars || start > end) {
		ReleaseToken();
		return False;
	}

	if (tlp->maximum_chars && 
		((tlp->num_chars + num_insert_chars - length) > tlp->maximum_chars)) {
		ReleaseToken();
		return False;
	}

	/* Compute new selection positions - if this edit were to occur .. */
	if (tlp->select_end < start) { 	/* Selection before replacement area */
		select_start = tlp->select_start;
		select_end = tlp->select_end;
	} else if (tlp->select_start >= end) {  /* Selection after replacement area ..*/
		select_start = tlp->select_start + (num_insert_chars - length);
		select_end = tlp->select_end + (num_insert_chars - length);
	} else {	/* Selection intersects replacement region .. */
		select_start = select_end = start + num_insert_chars;
	}
	pre_cd.reason = OL_REASON_PROG_PRE_MODIFICATION;
	pre_cd.event = (XEvent *)NULL;
	pre_cd.current_cursor = tlp->cursor_position;
	pre_cd.new_cursor = select_end;
	pre_cd.start = start;
	pre_cd.replace_length = length;
	pre_cd.buffer = String(tlp);
	pre_cd.valid = True;
	pre_cd.insert_buffer = string;
	pre_cd.insert_length = num_insert_chars;
	XtCallCallbackList(w, tlp->pre_modify_callback, &pre_cd);
	if (pre_cd.valid == False) {
		ReleaseToken();
		return False;
	}

	if (_OlTLReplaceString((TextLineWidget)w, start, end, string) == False) {
		ReleaseToken();
		return False;
	}

	tlp->select_start = select_start;
	tlp->select_end = tlp->caret_pos = tlp->cursor_position = select_end;

	if ((what = _OlTLSetupArrowsAfterTextChange((TextLineWidget)w)) & TLDrawText)
		start = _OLTLStartOfDisplay(tlp);
	else {
		int start_of_display = _OLTLStartOfDisplay(tlp);
		start = start < start_of_display ? start_of_display : start;
	}

	_OlTLDrawWidget((TextLineWidget)w, start, TLEndOfDisplay, 
			what | TLDrawText | TLMoveCaret);

	post_cd.reason = OL_REASON_PROG_POST_MODIFICATION;
	post_cd.event =  (XEvent *)NULL;
	post_cd.cursor = tlp->cursor_position;
	post_cd.buffer = String(tlp);
	XtCallCallbackList((Widget)w, tlp->post_modify_callback, &post_cd);

	ReleaseToken();
	return True;
}

/******************************************************
	OlTLSetSelection 

 ******************************************************/
PublicInterface Boolean
OlTLSetSelection (Widget w, int start, int length)
{
	TextLinePart *tlp; 
	int end; 
	unsigned long what; 

	GetToken();

	tlp = &((TextLineWidget)w)->textLine;
	end = start + length;
	what = 0;

	if (end > tlp->num_chars || start > end) {
		ReleaseToken();
		return False;
	}

	tlp->select_start = start;
	tlp->select_end = tlp->cursor_position = tlp->caret_pos = end;

	if (tlp->select_start <tlp->select_end && !XtOwnSelection((Widget)w, XA_PRIMARY,
	  	XtLastTimestampProcessed(XtDisplay((Widget)w)), 
		ConvertPrimary, LosePrimary, NULL))
	{
		_OLTLWarn("TextLineWidget: Cannot get Primary Selection");
		tlp->select_start = tlp->select_end ;
		ReleaseToken();
		return False;
	}

	if ((what = _OlTLSetupArrowsAfterCursorChange((TextLineWidget)w)) & TLDrawText)
		start = _OLTLStartOfDisplay(tlp);
	else {
		int start_of_display = _OLTLStartOfDisplay(tlp);
		start = start < start_of_display ? start : start_of_display;
	}
	_OlTLDrawWidget((TextLineWidget)w, start, TLEndOfDisplay, 
			what | TLDrawText | TLMoveCaret);
	ReleaseToken();
	return True;
}

/******************************************************
	OlTLGetSelection 

 ******************************************************/
PublicInterface OlStr
OlTLGetSelection (Widget w, int *start, int *length )
{
	TextLinePart *tlp; 
	OlStr retval;

	GetToken();

	tlp = &((TextLineWidget)w)->textLine;
	retval = (OlStr)NULL;

	if (tlp->select_start >= tlp->select_end) {
		ReleaseToken();
		return retval;
	}
	
	*start = tlp->select_start;
	*length = tlp->select_end - tlp->select_start;
	retval =  _OlTLGetSubString(tlp->buffer, *start, tlp->select_end -1,
		((TextLineWidget)w)->primitive.text_format, tlp->pos_table);
	
	ReleaseToken();
	return retval;
}
