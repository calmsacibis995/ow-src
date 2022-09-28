#pragma ident	"@(#)TextField.c	302.31	97/03/26 lib/libXol SMI"	/* textfield:src/TextField.c 1.62	*/

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


#include <locale.h>
#include <widec.h>
#include <string.h>
#include <libintl.h>

#include <X11/IntrinsicP.h>
#include <X11/StringDefs.h>

#include <Xol/Dynamic.h>
#include <Xol/OpenLookP.h>
#include <Xol/TextDisp.h>
#include <Xol/TextEdit.h>
#include <Xol/TextEditP.h>
#include <Xol/TextFieldP.h>
#include <Xol/TextWrap.h>
#include <Xol/TextUtil.h>
#include <Xol/Util.h>
#include <Xol/buffutil.h>
#include <Xol/memutil.h>
#include <Xol/textbuff.h>
#include <Xol/Oltextbuff.h>


#define LeftArrowActive(ctx)		(ctx->textedit.xOffset != 0)

#define RightArrowActive(ctx)		(ctx->textedit.maxX + \
	ctx->textedit.xOffset > PAGEWID(ctx))

#define InLeftArrow(width,arrow,x)	(0 <= (int)x && (int)x < (int)arrow)

#define InRightArrow(width,arrow,x)	((int)width - (int)arrow < x && x <= \
	(int)width)

#define TF_HORIZONTAL_SHIFT(ctx, text)	(_OlMax((text->charsVisible / 3) , 1) * \
	ENSPACE(ctx))

#define SHIFT_AMOUNT(ctx,text)		((text->selectMode == 6) ||  \
	(text->selectMode == 8) || (text->selectMode == 9) || \
	(text->selectMode == 65) || (text->selectMode == 85) || \
	(text->selectMode == 95) || text->charsVisible < 10 ? FONTWID(ctx) : \
	TF_HORIZONTAL_SHIFT(ctx, text))

#define RemoveTimer(timer, w) \
	if (timer != NULL) \
		XtRemoveTimeOut(timer); \
	timer = NULL; \
	w->textedit.shouldBlink = TRUE


/* static procedures */
static void		GetBitmaps();
static void		MarginCallback(Widget w, XtPointer client_data, XtPointer call_data);
static void		VerifyKey(Widget w, XtPointer client_data, XtPointer call_data);

static void		VerifyMotion(Widget w, XtPointer client_data,
	XtPointer call_data);

static void		VerifyMotion(Widget w, XtPointer client_data, XtPointer call_data);

static Boolean		ScrollField(TextFieldWidget tfw, Position x, int amount);

static void		PollMouse(XtPointer client_data, XtIntervalId *id);
static void		ButtonDown(Widget w, XtPointer client_data, XtPointer call_data);
static void		ButtonUp(Widget w, XtPointer client_data, XEvent *event, Boolean *continue_to_dispatch);
static Widget		EditsRegisterFocus(Widget w);

static void HighlightHandler (Widget, OlDefine);

static Boolean		AcceptFocus(Widget w, Time *timestamp);

static void              ChangeManaged(TextFieldWidget w);
static void              Destroy(Widget w);
static XtGeometryResult  GeometryManager(Widget w, XtWidgetGeometry *request, XtWidgetGeometry *reply);
static void              GetValuesHook(Widget w, ArgList args, Cardinal *num_args);
static void              Initialize(TextFieldWidget request, TextFieldWidget new, ArgList args, Cardinal *num_args);
static Boolean           LayoutChildren(Widget w);
static Boolean           PassIntoHook(Widget w, ArgList args, Cardinal *num_args);
static void              Resize(Widget w);
static Widget		 RegisterFocus(Widget w);
static void		 TransparentProc(Widget w, Pixel pixel, Pixmap pixmap);
static Boolean           SetValues(Widget current, Widget request, Widget new,
				   ArgList call_args, Cardinal *num_call_args);

static XtIntervalId      Timer;

#define BYTE_OFFSET	XtOffsetOf(TextFieldRec, textField.dyn_flags)
static _OlDynResource dyn_res[] = {
#ifdef COLORED_LIKE_TEXT
{ { XtNbackground, XtCTextBackground, XtRPixel, sizeof(Pixel), 0,
	XtRString, XtDefaultBackground }, BYTE_OFFSET,
	OL_B_TEXTFIELD_BG, NULL },
#endif
{ { XtNforeground, XtCForeground, XtRPixel, sizeof(Pixel), 0,
	XtRString, XtDefaultForeground }, BYTE_OFFSET,
	OL_B_TEXTFIELD_FG, NULL },
{ { XtNfontColor, XtCFontColor, XtRPixel, sizeof(Pixel), 0,
        XtRString, XtDefaultForeground }, BYTE_OFFSET,
        OL_B_TEXTFIELD_FC, NULL },
};
#undef BYTE_OFFSET

/*
 * Define Resource list associated with the Widget Instance
 *
 */

#define OFFSET(field) XtOffsetOf(TextFieldRec, textField.field)

static XtResource resources[] =
 {
  /* XtNtextFormat resource should be the first resource */
  { XtNtextFormat, XtCTextFormat, XtROlStrRep, sizeof(OlStrRep),
   OFFSET(text_format),XtRCallProc,(XtPointer)_OlGetDefaultTextFormat
  },

  {XtNimPreeditStyle, XtCImPreeditStyle, XtROlImPreeditStyle, 
	sizeof(OlImPreeditStyle), OFFSET(pre_edit_style), 
		XtRImmediate, (XtPointer)OL_NO_PREEDIT 
  },
#ifdef COLORED_LIKE_TEXT
  { XtNbackground, XtCTextBackground, XtRPixel, sizeof(Pixel),
    XtOffsetOf(TextFieldRec, core.background_pixel), XtRString,
    XtDefaultBackground
  },
#endif
  { XtNtextEditWidget, XtCTextEditWidget, XtRWidget, sizeof(Widget),
    OFFSET(textWidget), XtRWidget, NULL
  },
  { XtNstring, XtCString, XtROlStr, sizeof (OlStr),
     OFFSET(string), XtROlStr, NULL
  },
  { XtNinitialDelay, XtCInitialDelay, XtRInt, sizeof(int),
    OFFSET(initialDelay), XtRImmediate, (XtPointer)500
  },
  { XtNrepeatRate, XtCRepeatRate, XtRInt, sizeof(int),
     OFFSET(repeatRate), XtRImmediate, (XtPointer)100
  },
  { XtNmaximumSize, XtCMaximumSize, XtRInt, sizeof(int),
     OFFSET(maximumSize), XtRImmediate, 0
  },
  { XtNcharsVisible, XtCCharsVisible, XtRInt, sizeof(int),
     OFFSET(charsVisible), XtRImmediate, 0
  },
  { XtNfont, XtCFont, XtROlFont, sizeof(OlFont),
     OFFSET(font), XtRString, XtDefaultFont
  },
  { XtNfontColor, XtCFontColor, XtRPixel, sizeof(Pixel),
     OFFSET(font_color),XtRString, XtDefaultForeground
  },
  { XtNinputFocusColor, XtCInputFocusColor, XtRPixel, sizeof(Pixel),
      OFFSET(input_focus_color), XtRCallProc, (XtPointer)_OlGetDefaultFocusColor
  },
  { XtNverification, XtCCallback, XtRCallback, sizeof(XtCallbackProc),
     OFFSET(verification), XtRCallback, NULL
  },
  { XtNinsertTab, XtCInsertTab, XtRBoolean, sizeof(Boolean),
     OFFSET(insertTab), XtRImmediate, (XtPointer)FALSE
  },
  { XtNforeground, XtCForeground, XtRPixel, sizeof(Pixel),
    OFFSET(foreground), XtRString, XtDefaultForeground
  },
  { XtNscale, XtCScale, XtROlScale, sizeof(int),
    OFFSET(scale), XtRImmediate, (XtPointer) OL_DEFAULT_POINT_SIZE
  },
  { XtNeditType, XtCEditType, XtROlEditMode, sizeof(OlEditMode),
	OFFSET(editMode), XtRString, "textedit"
  }
 };

#undef OFFSET

/*
 * Define Class Record structure to be initialized at Compile time
 *
 */

TextFieldClassRec /*NEW*/textFieldClassRec = {
  {
/* core_class fields      */
    /* superclass         */    (WidgetClass) &managerClassRec,
    /* class_name         */    "TextField",
    /* widget_size        */    sizeof(TextFieldRec),
    /* class_initialize   */    NULL,
    /* class_part_init    */	NULL,
    /* class_inited       */	FALSE,
    /* initialize         */    (XtInitProc)Initialize,
    /* initialize_hook    */    (XtArgsProc)PassIntoHook,
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
    /* expose             */    NULL,
    /* set_values         */    SetValues,
    /* set_values_hook    */    (XtArgsFunc)PassIntoHook,
    /* set_values_almost  */    XtInheritSetValuesAlmost,
    /* get_values_hook    */	GetValuesHook,
    /* accept_focus       */    AcceptFocus,
    /* version            */	XtVersion,
    /* callback_private   */	NULL,
    /* tm_table           */	XtInheritTranslations,
    /* query_geometry     */	NULL, 
  },{
/* composite_class fields */
    /* geometry_manager   */    (XtGeometryHandler) GeometryManager,
    /* change_managed     */    (XtWidgetProc) ChangeManaged,
    /* insert_child	  */	XtInheritInsertChild,
    /* delete_child	  */	XtInheritDeleteChild,
    /* extension	  */    NULL
  },{
/* constraint_class fields */
    /* resources	  */	(XtResourceList)NULL,
    /* num_resources	  */	0,
    /* constraint_size	  */	0,
    /* initialize	  */	(XtInitProc)NULL,
    /* destroy		  */	(XtWidgetProc)NULL,
    /* set_values	  */	(XtSetValuesFunc)NULL
   },{
/* manager_class fields   */
    /* highlight_handler  */	HighlightHandler,
    /* reserved		  */	NULL,
    /* reserved		  */	NULL,
    /* traversal_handler  */    NULL,
    /* activate		  */    NULL,
    /* event_procs	  */    NULL,
    /* num_event_procs	  */	0,
    /* register_focus	  */	RegisterFocus,
    /* reserved		  */	NULL,
    /* version		  */	OlVersion,
    /* extension	  */	NULL,
    /* dyn_data		  */	{ dyn_res, XtNumber(dyn_res) },
    /* transparent_proc   */	TransparentProc,
    /* query_sc_locn_proc */	NULL,
   },{
/* textField class */     
    /* none		*/	NULL
 }	
};
WidgetClass /*NEW*/textFieldWidgetClass = (WidgetClass)&/*NEW*/textFieldClassRec;

/* Pad between scrollbuttons and textline as Per OL Spec */
#define	OL_SCALE_TO_SCROLLPAD_RATIO	2  /* Table B-33, row c (p.452) */

#define TOP_MARGIN(s)	 (Dimension)(s < 14? 3 : 5)
#define BOTTOM_MARGIN(s) (Dimension)(s < 14? 4 : 6)
#define SIDE_MARGIN(s)   (Dimension)(s / 3)


/*
 * MarginCallback
 *
 */

static void 
MarginCallback(Widget w, XtPointer client_data, XtPointer call_data)
{
	TextFieldWidget tfw              = (TextFieldWidget)client_data;
	TextEditWidget  ctx              = (TextEditWidget)w;
	TextEditPart *  text             = &ctx-> textedit;
	TextFieldPart * tf               = &tfw-> textField;
	Display *       dpy              = XtDisplay(w);
	Window          win              = XtWindow(w);
	GC              gc               = text-> gc;
	Position        baseline         = ctx-> core.height - 1;
	Dimension       width            = ctx-> core.width;
	Dimension       fontwid          = FONTWID(ctx);
	Position        lineLeftX        = PAGE_L_MARGIN(ctx);
	Position        lineRightX       = PAGE_R_MARGIN(ctx);
	XRectangle      l_rect;
	XRectangle      r_rect;
	OlgxAttrs	*pAttrs = tf->pAttrs;
        Screen          *scr = XtScreen((Widget)tfw);
	int		state = OLGX_ERASE;

	if (!XtIsSensitive((Widget)tfw))
	    state |= OLGX_INACTIVE;

	/* Figure out coordinates of scrollbuttons */
	l_rect.width = tf->arrowWidth;
	l_rect.height = tf->arrowHeight;
	l_rect.y = baseline - tf->arrowHeight;
	l_rect.x = 0;

	r_rect.x = ctx->core.width - (tf->arrowWidth + 1);
	r_rect.width = tf->arrowWidth;
	r_rect.y = baseline - tf->arrowHeight;
	r_rect.height = tf->arrowHeight;

	if (LeftArrowActive(ctx)) {
		tf-> l_on = TRUE;
		if (tf->polling == POLL_LEFT)
			state |= OLGX_INVOKED;

		lineLeftX = tf->arrowWidth + 1;

                /* OLGX_TODO: check on bug with OLGX_ERASE not working.
                 * Also need to text ABOVE the top of scrollbutton (because we
                 * offset our text so high.
                 */
                XClearArea(     dpy, win, l_rect.x, l_rect.y-3,
                                l_rect.width+1, l_rect.height+3, False);

		olgx_draw_textscroll_button(pAttrs->ginfo, win, 
				l_rect.x, l_rect.y, state | OLGX_SCROLL_BACKWARD); 
	}
	else if (tf-> l_on == TRUE) {
		tf-> l_on = FALSE;
		XClearArea(	dpy, win, l_rect.x, l_rect.y, 
				l_rect.width, l_rect.height, False);
		_DisplayText(ctx, &l_rect);
		_TurnTextCursorOff(ctx);
	}

	if (RightArrowActive(ctx)) {
		tf-> r_on = TRUE;
		if (tf->polling == POLL_RIGHT)
			state |= OLGX_INVOKED;

		lineRightX = width - tf->arrowWidth - 2;

                /* OLGX_TODO: check on bug with OLGX_ERASE not working.
                 * Also need to text ABOVE the top of scrollbutton (because we
                 * offset our text so high.
                 */
                XClearArea(     dpy, win, r_rect.x - 2, r_rect.y-3,
                                r_rect.width+2, r_rect.height+3, False);

                olgx_draw_textscroll_button(pAttrs->ginfo, win,
                                r_rect.x, r_rect.y, state | OLGX_SCROLL_FORWARD);

	}
	else if (tf-> r_on == TRUE) {
		tf-> r_on = FALSE;
		XClearArea(	dpy, win, r_rect.x, r_rect.y, 
				r_rect.width, r_rect.height, False);
		_DisplayText(ctx, &r_rect);
		_TurnTextCursorOff(ctx);
	}

	if (lineRightX > lineLeftX) {
		width = lineRightX - lineLeftX;
		olgx_draw_text_ledge(pAttrs->ginfo, win, lineLeftX, baseline-2, width);
	}

} /* end of MarginCallback */

/*
 * VerifyKey
 *
 */

static void 
VerifyKey(Widget w, XtPointer client_data, XtPointer call_data)
{
TextEditWidget ctx = (TextEditWidget)w;
TextFieldWidget        tfw        = (TextFieldWidget)client_data;
TextEditPart *         text       = &ctx-> textedit;
TextBuffer *           textBuffer = text-> textBuffer;
OlInputCallDataPointer cd         = (OlInputCallDataPointer)call_data;
OlTextFieldVerify      verify_call_data;
OlStrRep		rep = ctx->primitive.text_format;

switch(cd-> ol_event)
   {
   case OL_NEXTFIELD:
   case OL_PREVFIELD:
   case OL_RETURN:
      if(rep == OL_SB_STR_REP)
      	verify_call_data.string = GetTextBufferLocation(textBuffer, 0, NULL);
      else
	verify_call_data.string = (String)OlGetTextBufferStringAtLine(
					(OlTextBufferPtr)textBuffer,
					(TextLine)0, (TextLocation *)NULL);
      verify_call_data.reason = 
         cd-> ol_event == OL_RETURN ? OlTextFieldReturn :
         cd-> ol_event == OL_NEXTFIELD ? OlTextFieldNext :
         OlTextFieldPrevious;
      verify_call_data.ok = True;
      XtCallCallbacks((Widget)tfw, XtNverification, &verify_call_data);
      cd-> consumed = !verify_call_data.ok;
      break;
   default:
      break;
   }

} /* end of VerifyKey */
/*
 * VerifyModify
 *
 */

static void 
VerifyModify(Widget w, XtPointer client_data, XtPointer call_data)
{
TextEditWidget ctx = (TextEditWidget)w;
TextFieldWidget             tfw  = (TextFieldWidget)client_data;
TextEditPart *              text = &ctx-> textedit;
OlTextModifyCallDataPointer cd   = (OlTextModifyCallDataPointer)call_data;
OlStrRep	rep	= ctx->primitive.text_format;
Boolean		temp = False;

if (( (rep == OL_WC_STR_REP) ?
	(wschr((wchar_t *)cd->text, (int)L'\n') != (wchar_t *)NULL) :
	(strchr((const char *)cd-> text, (int)'\n') != (char *)NULL)))
   {
   _OlBeepDisplay((Widget)ctx, 1);
   cd-> ok = False;
   }
else
   if (tfw-> textField.maximumSize > 0) {
	switch(rep) {
		case OL_SB_STR_REP:
      			temp = (LengthOfTextBufferLine(
					ctx-> textedit.textBuffer, 0) - 1
          			- (cd-> select_end - cd-> select_start)
          			+ (int)str_methods[rep].StrNumChars((char *)cd-> text) > 
					tfw-> textField.maximumSize);
			break;
		case OL_MB_STR_REP:
			temp = 	(OlNumCharsInTextBufferLine(
					(OlTextBufferPtr)
					ctx->textedit.textBuffer,
					0) - 1
          				- (cd-> select_end - cd-> select_start)
          				+ (int)str_methods[rep].StrNumChars((char *)cd-> text) >
					tfw-> textField.maximumSize);
			break;
		case OL_WC_STR_REP:
			temp = 	(OlNumCharsInTextBufferLine(
					(OlTextBufferPtr)
					ctx->textedit.textBuffer,
					0) - 1
          				- (cd-> select_end - cd-> select_start)
					+ (int)wslen((wchar_t *)cd->text) >
					tfw-> textField.maximumSize);
			break;
	} /* end of switch */
				
         if(temp) {
         _OlBeepDisplay((Widget)ctx, 1);
         cd-> ok = False;
         }
   }

} /* end of VerifyModify */


static void
VerifyMotion(Widget w, XtPointer client_data, XtPointer call_data)
{
	TextEditWidget		ctx = (TextEditWidget)w;
	TextFieldWidget		tfw = (TextFieldWidget)client_data;
	TextEditPart*		text = &ctx->textedit;
	OlTextMotionCallDataPointer
				cd = (OlTextMotionCallDataPointer)call_data;
	Boolean			need_left_arrow = LeftArrowActive(ctx);
	Boolean			need_right_arrow = RightArrowActive(ctx);
	Position		delta = 0;
	OlStr			p;
	Position		x;
	XRectangle		rect;
	Position		leftArrowX;
	Position		rightArrowX;
	OlStrRep		rep = ctx->primitive.text_format;
	UnitPosition		offset;

	if (need_left_arrow || need_right_arrow) {

		leftArrowX = tfw->textField.arrowWidth;
		rightArrowX = ctx->core.width - tfw->textField.arrowWidth;

		if(rep == OL_SB_STR_REP) 
			p = (OlStr)GetTextBufferLocation(text->textBuffer, 0,
				(TextLocation*) NULL);
		else
			p = OlGetTextBufferStringAtLine((OlTextBufferPtr)
					text->textBuffer, (TextLine)0, 
					(TextLocation *)NULL);

		if(rep != OL_SB_STR_REP) {
			if(cd->new_cursor > 0) 
				offset = OlUnitPositionOfTextPosition(
					(OlTextBufferPtr)text->textBuffer,
					 (cd->new_cursor -1));
			else
				offset = cd->new_cursor - 1;
		} else
			offset = cd->new_cursor - 1;
			
		x = (offset >= 0 ? _StringWidth(0, p, 0, offset,
			ctx->primitive.font, rep, text->tabs) :
			 0 ) + PAGE_L_MARGIN(ctx) + text->xOffset;


		if (need_left_arrow && x <= leftArrowX) {
			delta = MAX(text->xOffset,
				-(leftArrowX - x + SHIFT_AMOUNT(ctx, text)));
		} else if (need_right_arrow && x >= rightArrowX) {
			delta = x - rightArrowX + SHIFT_AMOUNT(ctx, text);
		}
	}

	if (delta) {
		text->xOffset -= delta;
		text->cursorPosition = cd->new_cursor;
		if(rep == OL_SB_STR_REP)
			text->cursorLocation = 
			LocationOfPosition(text->textBuffer,
							cd->new_cursor);
			else
				(void)OlLocationOfPosition((OlTextBufferPtr)
						text->textBuffer,
						cd->new_cursor,
						&text->cursorLocation);
		text->selectStart = cd->select_start;
		text->selectEnd = cd->select_end;
		(void) OlTextEditRedraw(ctx);
		cd->ok = False;
	}
}


static Boolean 
ScrollField(TextFieldWidget tfw, Position x, int amount)
{
	TextEditWidget		ctx = (TextEditWidget)tfw->textField.textWidget;
	TextEditPart*		text = &ctx->textedit;
	TextFieldPart*		tf = &tfw->textField;
	Display*		dpy = XtDisplay(ctx);
	Window			win = XtWindow(ctx);
	GC			gc = text->invgc;
	Position		baseline = ctx->core.height - 1;
	Dimension		width = ctx->core.width;
	Dimension		arrowWidth = tf->arrowWidth;
	Dimension		arrowHeight = tf->arrowHeight;
	Dimension		fontwid = FONTWID(ctx);
	Boolean			retval = False;
	int			delta;
	XRectangle		rect;
	XtAppContext		ac = XtWidgetToApplicationContext((Widget)tfw);
	OlFont			font = ctx->primitive.font;
	OlStrRep		rep = ctx->primitive.text_format;
	XFontSetExtents		*extents;

	rect.x = 0;
	rect.y = 0;
	rect.width = ctx->core.width;
	rect.height = ctx->core.height;

	if (InLeftArrow(width, arrowWidth, x) && LeftArrowActive(ctx)) {

		_TurnTextCursorOff(ctx);
		if(rep == OL_SB_STR_REP)
			delta = MAX(-(((XFontStruct *)font)->max_bounds.width),
						ctx->textedit.xOffset);
		else {
			extents = XExtentsOfFontSet((XFontSet)font);
			delta = MAX(((int)(-extents->max_logical_extent.width)), 
						ctx->textedit.xOffset);
		}

		ctx->textedit.xOffset -= delta;
		RemoveTimer(Timer, ctx);

		if (LeftArrowActive(ctx)) {
			tf->polling = POLL_LEFT;
			ctx->textedit.shouldBlink = FALSE;
			Timer = XtAppAddTimeOut(ac, amount, PollMouse, tfw);
		} else
			XClearArea(dpy, win, 1, baseline - arrowHeight,
				arrowWidth, arrowHeight, False);

		_DisplayText(ctx, &rect);
		retval = True;

	} else if (InRightArrow(width, arrowWidth, x) && RightArrowActive(ctx)) {

		_TurnTextCursorOff(ctx);
		if(rep == OL_SB_STR_REP)
			delta = MIN(((XFontStruct *)font)->max_bounds.width,
				ctx->textedit.maxX + ctx->textedit.xOffset - 
					PAGEWID(ctx));
		else {
			extents = XExtentsOfFontSet((XFontSet)font);
			delta = MIN(((int)extents->max_logical_extent.width),
				ctx->textedit.maxX + ctx->textedit.xOffset - 
							PAGEWID(ctx));
		}
			
		ctx->textedit.xOffset -= delta;
		RemoveTimer(Timer, ctx);

		if (RightArrowActive(ctx)) {
			tf->polling = POLL_RIGHT;
			ctx->textedit.shouldBlink = FALSE;
			Timer = XtAppAddTimeOut(ac, amount, PollMouse, tfw);
		} else
			XClearArea(dpy, win, width - arrowWidth - fontwid,
				baseline - arrowHeight, arrowWidth + fontwid,
				arrowHeight, False);

		_DisplayText(ctx, &rect);
		retval = True;

	}

	return (retval);
}


static void 
PollMouse(XtPointer client_data, XtIntervalId *id)
{
TextFieldWidget tfw = (TextFieldWidget)client_data;
TextEditWidget  ctx = (TextEditWidget)tfw-> textField.textWidget;
TextFieldPart * tf  = &tfw-> textField;

Timer = NULL;
if (tfw-> textField.polling == POLL_RIGHT)
   (void) ScrollField(tfw, (Position)ctx-> core.width, tf-> repeatRate);
else
   (void) ScrollField(tfw, (Position)0, tf-> repeatRate);

} /* end of PollMouse */
/*
 * ButtonDown
 *
 */

static void 
ButtonDown(Widget w, XtPointer client_data, XtPointer call_data)
{
TextEditWidget    ctx   = (TextEditWidget)w;
TextFieldWidget   tfw   = (TextFieldWidget)client_data;
OlInputCallData * cd    = (OlInputCallData *)call_data;
XEvent *          event = cd-> event;

switch (LookupOlInputEvent(w, event, NULL, NULL, NULL))
   {
   case OL_SELECT:
   case OL_ADJUST:
      if (event-> type == ButtonPress)
         cd-> consumed = 
            ScrollField(tfw, event-> xbutton.x, tfw-> textField.initialDelay);
      break;
   default:
      break;
   }

} /* end of ButtonDown */
/*
 * ButtonUp
 *
 */

/* ARGSUSED */
static void 
ButtonUp(Widget w, XtPointer client_data, XEvent *event, Boolean *continue_to_dispatch)
{
TextEditWidget  ctx    = (TextEditWidget)w;
TextEditPart *  text   = &ctx-> textedit;
TextFieldWidget tfw    = (TextFieldWidget)client_data;
TextFieldPart * tf     = &tfw-> textField;
Dimension       width  = ctx-> core.width;
Dimension       arrow  = tf-> arrowWidth;
int             left   = text-> cursor_x - text-> CursorP-> xoffset;
int             right  = left + text-> CursorP-> width;

	RemoveTimer(Timer, ctx);
	tf-> polling = POLL_OFF;
	MarginCallback((Widget)ctx, (XtPointer)tfw, (XtPointer)NULL);

	if ((LeftArrowActive(ctx)  && InLeftArrow(width, arrow, left)) ||
		(RightArrowActive(ctx) && InRightArrow(width, arrow, right))) {
		_TurnTextCursorOff(ctx);
		ctx-> textedit.shouldBlink = FALSE;
	}

} /* end of ButtonUp */

static Widget
EditsRegisterFocus(Widget w)
{
	return(XtParent(w));
}

static void
HighlightHandler (Widget w, OlDefine type)
{
	_OlCallHighlightHandler(((TextFieldWidget)w)->textField.textWidget,
		 type);
} /* END OF HighlightHandler */

static Boolean
AcceptFocus(Widget w, Time *timestamp)
{
	if(OlCanAcceptFocus(w, *timestamp))
	  return(OlCallAcceptFocus(((TextFieldWidget)w)->textField.textWidget,
		 *timestamp));
	else return(FALSE);
} /* END OF AcceptFocus */

/*
 * Initialize
 *
 */

/* ARGSUSED */
static void 
Initialize(TextFieldWidget request, TextFieldWidget new, ArgList args, Cardinal *num_args)
{
	TextFieldWidget tfw	= (TextFieldWidget)new;
	TextFieldPart * tf	= &tfw-> textField;
	ManagerPart *mp	= &tfw-> manager;
	int	charsVisible	= tf-> charsVisible;
	Arg	arg[23];
	TextEditWidget	ctx;
	OlStrRep	rep = tf->text_format;
	int		validScale;

	if (tf-> string == (OlStr)NULL) 
		tf->string = (*str_methods[rep].StrEmptyString)();

	/* Ensure scale is valid for OLGX */
	validScale = OlgxGetValidScale(tf->scale);
	tf->scale = validScale;

	tf-> l_on = tf-> r_on = FALSE;
	tf-> polling = POLL_OFF;

	if (charsVisible == 0)
		charsVisible = (tf-> maximumSize == 0) ? 20 : tf-> maximumSize;

	XtSetArg(arg[0], XtNsource,       tf-> string);
	XtSetArg(arg[1], XtNsourceType,   OL_STRING_SOURCE);
	XtSetArg(arg[2], XtNwrapMode,     OL_WRAP_OFF);
	XtSetArg(arg[3], XtNlinesVisible, 1);
	if (tfw-> core.width > 0)
   		XtSetArg(arg[4], XtNwidth,        tfw-> core.width);
	else
		XtSetArg(arg[4], XtNcharsVisible, charsVisible);
	XtSetArg(arg[5], XtNtopMargin,    TOP_MARGIN(tf->scale));
	XtSetArg(arg[6], XtNbottomMargin, BOTTOM_MARGIN(tf->scale));
	XtSetArg(arg[7], XtNleftMargin,   SIDE_MARGIN(tf->scale));
	XtSetArg(arg[8], XtNrightMargin,  SIDE_MARGIN(tf->scale));
	XtSetArg(arg[9], XtNfont,         tf-> font);
	XtSetArg(arg[10], XtNregisterFocusFunc, EditsRegisterFocus);
	XtSetArg(arg[11], XtNinsertTab, tf->insertTab);
	XtSetArg(arg[12], XtNinsertReturn, FALSE);
	XtSetArg(arg[13], XtNbackground, tfw->core.background_pixel);
	XtSetArg(arg[14], XtNinputFocusColor, tf->input_focus_color);
        XtSetArg(arg[15], XtNfontColor,tf->font_color);
	XtSetArg(arg[16],XtNtextFormat,rep);
	XtSetArg(arg[17],XtNimPreeditStyle,tf->pre_edit_style);
	XtSetArg(arg[18], XtNcursorPosition, (*str_methods[rep].StrNumChars)(tf->string));
	XtSetArg(arg[19], XtNeditType, tf->editMode);
	tf-> textWidget = XtCreateManagedWidget   
   			("Textedit", textEditWidgetClass, 
						(Widget)tfw, arg, 20);

	ctx = (TextEditWidget)tf-> textWidget;
	tf-> string = (OlStr)ctx-> textedit.textBuffer;

	_OlDeleteDescendant((Widget)ctx);	/* delete from traversal list */
						/*  and add myself instead */
	_OlUpdateTraversalWidget((Widget)tfw, mp->reference_name,
				 mp->reference_widget, True);

	XtAddCallback(tf-> textWidget, XtNmargin, MarginCallback, 
					(XtPointer)tfw);
	XtAddCallback(tf-> textWidget, XtNmodifyVerification, 
			VerifyModify, (XtPointer)tfw);
	XtAddCallback(tf-> textWidget, XtNmotionVerification, 
			VerifyMotion, (XtPointer)tfw);
	XtAddCallback(tf-> textWidget, XtNbuttons, ButtonDown, 
						(XtPointer)tfw);
	XtAddCallback(tf-> textWidget, XtNkeys, VerifyKey, 
						(XtPointer)tfw);

	XtAddEventHandler(tf-> textWidget, ButtonReleaseMask, False, 
			ButtonUp, tfw);


	tf->pAttrs = 
		OlgxCreateAttrs (	(Widget)tfw,
					tfw->textField.foreground,
					(OlgxBG *)&(tfw->core.background_pixel),
					False,
					tf->scale, (OlStrRep)0, (OlFont)NULL);

	tfw->textField.arrowWidth = 
		(Dimension)TextScrollButton_Width(tf->pAttrs->ginfo);
	tfw->textField.arrowHeight = 
		(Dimension)TextScrollButton_Height(tf->pAttrs->ginfo);
		
} /* end of Initialize */
/*
 * GetValuesHook
 *
 */

static void 
GetValuesHook(Widget w, ArgList args, Cardinal *num_args)
{
TextFieldWidget tfw        = (TextFieldWidget)w;
TextEditWidget  ctx        = (TextEditWidget) tfw-> textField.textWidget;
TextEditPart *  text       = &ctx-> textedit;
TextBuffer *    textBuffer = text-> textBuffer;
OlStrRep	rep = ctx->primitive.text_format;

register int i;
register int length;
OlStr       string;
OlStr       p;

if (*num_args > 0)
   for (i = 0; i < *num_args; i++) {
      if (0 == strcmp(args[i].name, XtNstring)) {
         OlStr *retarea = (OlStr *)(args[i].value);
	 
	 if(rep == OL_SB_STR_REP) 
         	p = (OlStr)
		GetTextBufferLocation(textBuffer, 0, (TextLocation *)NULL); 
	 else
		p = (OlStr)OlGetTextBufferStringAtLine((OlTextBufferPtr)
				text->textBuffer, (TextLine)0,
				(TextLocation *)NULL); 
     	 string = (OlStr)XtMalloc((*str_methods[rep].StrNumBytes)(p));
	 (*str_methods[rep].StrCpy)(string,p);
         *retarea = string;
      } /* if */
    } /* for */

} /* end of GetValuesHook */
/*
 * PassIntoHook
 *
 */

static Boolean 
PassIntoHook(Widget w, ArgList args, Cardinal *num_args)
{
TextFieldWidget tfw  = (TextFieldWidget)w;
TextEditWidget  ctx  = (TextEditWidget) tfw-> textField.textWidget;
TextEditPart *  text = &ctx-> textedit;
int             argc = 0;
Arg             arg[20];

register int    i;
register int    j;

static char *   pass_into_names[] =
   {
   XtNfontColor,
   XtNfont,
   XtNtraversalOn,
   XtNbackground,
   XtNmodifyVerification,
   XtNmotionVerification,
   XtNpostModifyNotification,
   XtNinputFocusColor,
   XtNfontColor,
   XtNeditType,
   };

if (*num_args > 0)
   for (i = 0; i < XtNumber(pass_into_names); i++)
      {
      for (j = 0; j < *num_args; j++)
         if (0 == strcmp(pass_into_names[i], args[j].name))
            {
            arg[argc] = args[j];
            argc++;
            break;
            }
      }

if (argc != 0)
   XtSetValues((Widget)ctx, arg, argc);

return(FALSE);

} /* end of PassIntoHook */

/******************************function*header****************************
 * RegisterFocus - return widget id to register on Shell
 */
static Widget
RegisterFocus(Widget w)
{
    return (w);		/* return self */
}

static void
TransparentProc (Widget w, Pixel pixel, Pixmap pixmap)
{
	Arg arg[1];

	if (pixmap != XtUnspecifiedPixmap)
		XtSetArg(arg[0], XtNbackgroundPixmap, ParentRelative);
	else
		XtSetArg(arg[0], XtNbackground, pixel);

	XtSetValues(((TextFieldWidget)w)->textField.textWidget, arg, 1);
}

/*
 * SetValues
 *
 */

/* ARGSUSED */
static Boolean 
SetValues (Widget current, Widget request, Widget new,ArgList call_args, Cardinal *num_call_args)
{
Arg arg[10];
TextFieldWidget newtf = (TextFieldWidget)new;
TextFieldWidget curtf = (TextFieldWidget)current;
TextEditWidget ctx = (TextEditWidget)newtf->textField.textWidget;
TextEditWidget currctx = (TextEditWidget)curtf->textField.textWidget;
OlStr p;
int i = 0;
Boolean newAttrs = False;
OlStrRep	rep = ctx->primitive.text_format;
 
	newtf->textField.text_format =  curtf->textField.text_format;
if(rep == OL_SB_STR_REP)
	p = (OlStr)GetTextBufferLocation(ctx-> textedit.textBuffer, 
					0, (TextLocation *)NULL);
else
	p = OlGetTextBufferStringAtLine((OlTextBufferPtr)
					ctx->textedit.textBuffer,
					(TextLine)0,
					(TextLocation *)NULL);

if (newtf->textField.string == (OlStr)NULL)
		newtf->textField.string = (*str_methods[rep].StrEmptyString)();

if ((newtf->textField.string != (OlStr)ctx-> textedit.textBuffer) &&
    ((*str_methods[rep].StrCmp)(p, newtf->textField.string) != 0)) {
   XtSetArg(arg[i], XtNsource,         newtf-> textField.string); i++;
   XtSetArg(arg[i], XtNsourceType,     OL_STRING_SOURCE); i++;
   XtSetArg(arg[i], XtNcursorPosition, 
		(*str_methods[rep].StrNumChars)(newtf->textField.string)); i++;
   XtSetArg(arg[i], XtNselectStart,    0); i++;
   XtSetArg(arg[i], XtNselectEnd,      0); i++;
   }
 
if (newtf->textField.insertTab != curtf->textField.insertTab) {
        XtSetArg(arg[i], XtNinsertTab, newtf->textField.insertTab); i++;
}
 
if (newtf->core.background_pixel != curtf->core.background_pixel) {
        XtSetArg(arg[i], XtNbackground, newtf->core.background_pixel); i++;
        newAttrs = True;
}
 
if (newtf->textField.foreground != curtf->textField.foreground)
        newAttrs = True;

if (newtf->textField.scale != curtf->textField.scale) {
    /* Note: this doesn't completely work, because the font remains
     * unaffected.
     */
    int validScale = OlgxGetValidScale(newtf->textField.scale);
    newtf->textField.scale = validScale;

    XtSetArg(arg[i], XtNleftMargin,   SIDE_MARGIN(validScale)); i++;
    XtSetArg(arg[i], XtNrightMargin,  SIDE_MARGIN(validScale)); i++;
    XtSetArg(arg[i], XtNtopMargin,    TOP_MARGIN(validScale));  i++;
    XtSetArg(arg[i], XtNbottomMargin, BOTTOM_MARGIN(validScale));i++;
    newAttrs = True;
}
 
if (newAttrs)
{
	if (newtf->textField.pAttrs != (OlgxAttrs *)NULL)
		OlgxDestroyAttrs((Widget)newtf, newtf->textField.pAttrs);

        newtf->textField.pAttrs = OlgxCreateAttrs ((Widget)newtf,
                                        newtf->textField.foreground,
                                        (OlgxBG *)&(newtf->core.background_pixel),
                                        False,
                                        newtf->textField.scale,
					(OlStrRep)0, (OlFont)NULL);
}
 
/* if XtNinputFocusColor is different */
if(newtf->textField.input_focus_color
                != curtf->textField.input_focus_color) {
                XtSetArg(arg[i],XtNinputFocusColor,
                        newtf->textField.input_focus_color); i++;
                }
 
/* if XtNfontColor is different */
if(newtf->textField.font_color
                != curtf->textField.font_color) {
                XtSetArg(arg[i],XtNfontColor,
                        newtf->textField.font_color); i++;
                }
 
if(newtf->textField.editMode
                 != curtf->textField.editMode) {
	XtSetArg(arg[i],XtNeditType,
                         newtf->textField.editMode); i++;
}
        
if (i) {
   XtSetValues((Widget)ctx, arg, i);
}
 
newtf-> textField.string = (OlStr)ctx-> textedit.textBuffer;
 
        
return (newAttrs);
 
} /* end of SetValues */
/*
 * Resize 
 *
 */

static void 
Resize(Widget w)
{
    TextFieldWidget tfw           = (TextFieldWidget)w;
    TextEditWidget  ctx           = (TextEditWidget)tfw-> textField.textWidget;
    int             bw            = (int)ctx-> core.border_width;
    int             bwx2          = bw * 2;

    XtResizeWidget((Widget)ctx, tfw->core.width - bwx2, 
		   tfw->core.height - bwx2, bw);
} /* end of Resize */

/*
 * GeometryManager
 * 
 */

static XtGeometryResult 
GeometryManager(Widget w, XtWidgetGeometry *request, XtWidgetGeometry *reply)
{
TextFieldWidget tfw         = (TextFieldWidget) w-> core.parent;

if ((request-> request_mode & CWWidth) == 0)
   request-> width = w-> core.width;
if ((request-> request_mode & CWHeight) == 0)
   request-> height = w-> core.height;
w-> core.width        = request-> width;
w-> core.height       = request-> height;

return (XtGeometryYes);

} /* end of GeometryManager */
/*
 *  LayoutChildren 
 *  
 */

static Boolean 
LayoutChildren (Widget w)
{
TextFieldWidget tfw           = (TextFieldWidget)w;
TextEditWidget  ctx           = (TextEditWidget)tfw-> textField.textWidget;
int             bw            = (int)ctx-> core.border_width;
int             bwx2          = bw * 2;
Dimension       managerWidth  = tfw-> core.width;
Dimension       managerHeight = tfw-> core.height;
Dimension       childWidth    = ctx-> core.width + bwx2;
Dimension       childHeight   = ctx-> core.height + bwx2;
Dimension       replyWidth;
Dimension       replyHeight;
Boolean		retval;

replyWidth = managerWidth  = MAX(managerWidth,  childWidth);
replyHeight = managerHeight = MAX(managerHeight, childHeight);

if ((managerWidth != w-> core.width || managerHeight != w-> core.height))
   {
   switch 
      (XtMakeResizeRequest
       (w, managerWidth, managerHeight, &replyWidth, &replyHeight))
      {
      case XtGeometryYes:
         break;
      case XtGeometryNo:
         /*
          * This picks up the case where parent answers "No" but sets 
          * replyHeight and replyWidth to what I asked for.  Don't ask 
          * why; I don't understand either.
          */
         if ((replyWidth <= w->core.width)&&(replyHeight <= w->core.height))
            {
            XtResizeWidget((Widget)ctx, replyWidth - bwx2, 
				replyHeight - bwx2, bw);
	    retval = True;
            }
         else
            {
	    retval = False;
            }
	 return(retval);
      case XtGeometryAlmost:
         XtMakeResizeRequest(w, replyWidth, replyHeight, NULL, NULL);
         break;
      }
   }

if (replyWidth != managerWidth || replyHeight != managerHeight)
	XtResizeWidget((Widget)ctx, replyWidth - bwx2, 
					replyHeight - bwx2, bw);

return (True);

} /* end of LayoutChildren */
/*
 * ChangeManaged 
 * 
 */

static void 
ChangeManaged(TextFieldWidget w)
{

(void) LayoutChildren ((Widget)w);

} /* end of ChangeManaged */
/*
 * Destroy
 *
 */

static void 
Destroy(Widget w)
{
    TextFieldWidget tfw = (TextFieldWidget)w;

    OlgxDestroyAttrs((Widget)tfw, tfw->textField.pAttrs);
    XtRemoveAllCallbacks((Widget)tfw, XtNverification);

} /* end of Destroy */
/*
 * OlTextFieldCopyString
 *
 * The \fIOlTextFieldCopyString\fR function is used to copy the string
 * string associated with the TextField widget \fItfw\fR into the user
 * supplied area pointed to by \fIstring\fR.  The function returns the
 * length of this string.
 *
 * See also:
 *
 * OlTextFieldGetString(3)
 *
 * Synopsis:
 *
 * #include <buffutil.h>
 * #include <textbuff.h>
 * #include <TextField.h>
 * ...
 */

extern int 
OlTextFieldCopyString(TextFieldWidget tfw, char *string)
{
TextEditWidget ctx        = (TextEditWidget) tfw-> textField.textWidget;
TextEditPart * text       = &ctx-> textedit;
TextBuffer *   textBuffer = text-> textBuffer;
int            length     = LengthOfTextBufferLine(textBuffer, 0);
char *         p;

p = GetTextBufferLocation(textBuffer, 0, (TextLocation *)NULL);
memcpy(string, p, length);

return (length);

} /* end of OlTextFieldCopyString */
/*
 * OlTextFieldGetString
 *
 * The \fIOlTextFieldGetString\fR function is used to retrieve a \fInew\fR
 * copy of the string associated with the TextField widget \fItfw\fR.
 * The function returns a pointer to the newly allocated string copy.
 * Optionally, if \fIsize\fR is not NULL, the function returns in \fIsize\fR
 * the length of the string.
 *
 * See also:
 *
 * OlTextFieldCopyString(3)
 *
 * Synopsis:
 *
 * #include <buffutil.h>
 * #include <textbuff.h>
 * #include <TextField.h>
 * ...
 */

extern char * 
OlTextFieldGetString(TextFieldWidget tfw, int *size)
{
TextEditWidget ctx        = (TextEditWidget) tfw-> textField.textWidget;
TextEditPart * text       = &ctx-> textedit;
TextBuffer *   textBuffer = text-> textBuffer;
int            length     = LengthOfTextBufferLine(textBuffer, 0);
char *         p;
char *         string     = MALLOC(length);

p = GetTextBufferLocation(text-> textBuffer, 0, (TextLocation *)NULL);
memcpy(string, p, length);

if (size)
   *size = length;

return (string);

} /* end of OlTextFieldGetString */

/*
 * OlTextFieldCopyOlString
 *
 * The \fIOlTextFieldCopyOlString\fR function is used to copy the OlStr 
 * string associated with the TextField widget \fItfw\fR into the user
 * supplied area pointed to by \fIstring\fR.  The function returns the
 * length of this string in number of units.
 *
 * See also:
 *
 * OlTextFieldGetOlString(3)
 *
 * Synopsis:
 *
 * #include <Oltextbuff.h>
 * #include <TextField.h>
 * ...
 */

extern int 
OlTextFieldCopyOlString(TextFieldWidget tfw, OlStr string)
{
OlStr         p;
TextEditWidget ctx        = (TextEditWidget) tfw-> textField.textWidget;
TextEditPart * text       = &ctx-> textedit;
OlTextBufferPtr   textBuffer = (OlTextBufferPtr)text-> textBuffer;
OlStrRep	rep = ctx->primitive.text_format;

if(rep == OL_SB_STR_REP)
    p = GetTextBufferLocation(textBuffer, 0, NULL);
else
    p = OlGetTextBufferStringAtLine(textBuffer, 
			(TextLine)0, (TextLocation *)NULL);
(*str_methods[rep].StrCpy)(string,p);

return ((*str_methods[rep].StrNumUnits)(p));

} /* end of OlTextFieldCopyOlString */

/*
 * OlTextFieldGetOlString
 *
 * The \fIOlTextFieldGetOlString\fR function is used to retrieve a \fInew\fR
 * copy of the string associated with the TextField widget \fItfw\fR.
 * The function returns a pointer to the newly allocated string copy.
 * Optionally, if \fIsize\fR is not NULL, the function returns in \fIsize\fR
 * the length of the string in number of units.
 *
 * See also:
 *
 * OlTextFieldCopyOlString(3)
 *
 * Synopsis:
 *
 * #include <Oltextbuff.h>
 * #include <TextField.h>
 * ...
 */

extern OlStr 
OlTextFieldGetOlString(TextFieldWidget tfw, int *size)
{
OlStr         p;
OlStr         string;
TextEditWidget ctx        = (TextEditWidget) tfw-> textField.textWidget;
TextEditPart * text       = &ctx-> textedit;
OlTextBufferPtr   textBuffer = (OlTextBufferPtr)text-> textBuffer;
OlStrRep	rep = ctx->primitive.text_format;

if(rep == OL_SB_STR_REP)
    p = GetTextBufferLocation(textBuffer, 0, NULL);
else
    p = OlGetTextBufferStringAtLine(textBuffer, 
			(TextLine)0, (TextLocation *)NULL);
string = (OlStr)XtMalloc((*str_methods[rep].StrNumBytes)(p));
(*str_methods[rep].StrCpy)(string,p);

if (size != (int *)NULL)
   *size = (*str_methods[rep].StrNumUnits)(string);

return (string);

} /* end of OlTextFieldGetOlString */
