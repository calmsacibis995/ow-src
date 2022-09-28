#pragma ident	"@(#)TextEdit.c	302.64	97/03/26 lib/libXol SMI"	/* textedit:TextEdit.c 1.32	*/
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
 * Description:  This file contains the TextEdit widget code.  The
 *               TextEdit widget allows editing/display of string and disk
 *               sources.
 */


#include <widec.h>
#include <libintl.h>
#include <string.h>
#include <wctype.h>

#include <X11/IntrinsicP.h>
#include <X11/StringDefs.h>
#include <X11/Xatom.h>
#include <X11/cursorfont.h>
#include <X11/keysym.h>
#ifdef sun
#include <X11/XSunExt.h>
#endif

#include <Xol/BulletinBP.h>
#include <Xol/Dynamic.h>
#include <Xol/Menu.h>
#include <Xol/Oltextbuff.h>
#include <Xol/OblongButt.h>
#include <Xol/OlCursors.h>
#include <Xol/OlDnDVCX.h>		/* new drag and drop */
#include <Xol/OlI18nP.h>
#include <Xol/OlStrMthdsI.h>
#include <Xol/OpenLookP.h>
#include <Xol/RootShell.h>
#include <Xol/Scrollbar.h>
#include <Xol/ScrolledWP.h>
#include <Xol/TextDisp.h>
#include <Xol/TextEPos.h>
#include <Xol/TextEdit.h>
#include <Xol/TextEditP.h>
#include <Xol/TextFieldP.h>
#include <Xol/TextUtil.h>
#include <Xol/TextWrap.h>
#include <Xol/Util.h>
#include <Xol/buffutil.h>
#include <Xol/memutil.h>
#include <Xol/textbuff.h>
#include <Xol/OlIm.h>


#define APPL_FG      (1L<<0)
#define APPL_BG      (1L<<1)
#define APPL_FOCUS   (1L<<2)
#define VORDER(x,y)  ((x) ? (x) : (y))

#define HAS_FOCUS(w)	(((TextEditWidget)(w))->primitive.has_focus == TRUE)

extern int      _CharWidth(int, XFontStruct *, XCharStruct *, int, int, int);
extern int      _CharWidthMB(char *, XFontSet);

extern int PreeditStartCallbackFunc(XIC ,XPointer, XPointer );
void PreeditEndCallbackFunc(XIC , XPointer , XPointer );
void PreeditDrawCallbackFunc(XIC , XPointer , XIMPreeditDrawCallbackStruct *);
void PreeditCaretCallbackFunc(XIC, XPointer, XIMPreeditCaretCallbackStruct *);

static void CheckMenuLabels(Widget current, Widget request, Widget new);
static void InitMenuLabels(Widget request, Widget new);
static int AdjustPreeditStartAndEndPos(
			TextEditWidget ctx, 
			OlStr buffer);
static void	_DoImplicitCommit(
			TextEditWidget ctx, 
			OlVirtualEvent ve,
			Boolean		conversion_on);
static void     Delete(
		       TextEditWidget	ctx,
		       XEvent *		event,
		       OlInputEvent	keyarg,
		       int		unused);
static void     DeleteSB(
		       TextEditWidget	ctx,
		       XEvent *		event,
		       OlInputEvent	keyarg,
		       int		unused);
static void     DeleteMBWC(
		       TextEditWidget	ctx,
		       XEvent *		event,
		       OlInputEvent	keyarg,
		       int		unused);

static Boolean  _AdjustDisplayLocation(TextEditWidget ctx, TextEditPart *text, WrapTable *wrapTable, TextLocation *currentDP, int page);
static TextPosition ValidatePosition(TextPosition min, TextPosition pos, TextPosition last, char *name);
static void     ValidatePositions(TextEditPart *text, Boolean warn);
static void     ValidateMargins(TextEditWidget ctx, TextEditPart *text);
static void     ValidateGrowMode(TextEditPart *text);
static void     GetGCs(TextEditWidget ctx, TextEditPart *text);
static void     ScrolledWindowInterface(TextEditWidget ctx, OlSWGeometries *gg);
static void     ResizeScrolledWindow(TextEditWidget ctx, ScrolledWindowWidget sw, OlSWGeometries *gg, Dimension width, Dimension height, int resize);
static void     SetScrolledWindowScrollbars(TextEditWidget ctx, Dimension width, Dimension height);
static void     VSBCallback(Widget w, XtPointer client_data, XtPointer call_data);
static void     HSBCallback(Widget w, XtPointer client_data, XtPointer call_data);

static void     ClassInitialize(void);
static void     ClassPartInitialize(WidgetClass class);
static void     Initialize(Widget request, Widget new, ArgList args, Cardinal *num_args);
static void     Realize(Widget w, Mask *valueMask, XSetWindowAttributes *attributes);
static Widget   RegisterFocus(Widget);
static void     Resize(Widget w);
static void     Redisplay(Widget w, XEvent *event, Region region);
static void     Destroy(Widget w);
static Boolean  SetValues(Widget current, Widget request, Widget new, ArgList args, Cardinal *num_args);
static Boolean  ActivateWidget(Widget, OlVirtualName, XtPointer);
static void     FocusHandler(Widget, OlDefine);
static void     LoseClipboardOrDnD(Widget w, Atom *atom);
static Boolean  ConvertClipboardOrDnD(Widget w, Atom *selection, Atom *target, Atom *type_return, XtPointer *value_return, long unsigned int *length_return, int *format_return);
static void     Paste(TextEditWidget ctx, XEvent *event, String *params, Cardinal *num_params);
static void     Paste2(Widget w, XtPointer client_data, Atom *selection, Atom *type, XtPointer value, long unsigned int *length, int *format);
static void     SelectTarget(Widget w, XtPointer client_data, Atom *selection, Atom *type, XtPointer value, long unsigned int *length, int *format);
static void     Key(Widget, OlVirtualEvent);
static void     Button(Widget, OlVirtualEvent);
static void     Select(TextEditWidget ctx, XEvent *event);
static void     Adjust(TextEditWidget ctx, XEvent *event);
static void     PollPan(XtPointer client_data, XtIntervalId *id);
static void     PollMouse(XtPointer client_data, XtIntervalId *id);
static void     DragText(TextEditWidget ctx, TextEditPart *text, TextPosition position, OlDragMode drag_mode);
static void     TextDropOnWindow(Widget ctx, Window drop_window, Position x, Position y, TextEditPart *text, OlDragMode drag_mode, OlDnDDragDropInfoPtr rinfo);
static Boolean	TextEditResizeRequest(TextEditWidget ctx, int width, int height);

#ifdef NOT_USE
    static void     DynamicInitialize();
    static XtArgsFunc DynamicHook();
#endif

#define XVIEW_CLICK_MODE	1

static void     ReceivePasteMessage(Widget w, XEvent *event, String *params, Cardinal *num_params);
static void     SendPasteMessage(TextEditWidget ctx, Window window, Position x, Position y, int operation);
static void     PopupTextEditMenu(TextEditWidget ctx);
static void     PopdownTextEditMenuCB(Widget w, XtPointer client_data, XtPointer call_data);
static void     MenuUndo(Widget w, XtPointer client_data, XtPointer call_data);
static void     MenuCut(Widget w, XtPointer client_data, XtPointer call_data);
static void     MenuCopy(Widget w, XtPointer client_data, XtPointer call_data);
static void     MenuPaste(Widget w, XtPointer client_data, XtPointer call_data);
static void     MenuDelete(Widget w, XtPointer client_data, XtPointer call_data);
static void     IgnorePropertyNotify(Widget w, XEvent *event, String *params, Cardinal *num_params);
static Boolean  TextEditCheckAndInsert(TextEditWidget ctx, OlStr buffer, int length);

static void     ProcessSource(TextEditWidget ctx);
static void	ComputeSWResizes(
				 TextEditWidget	 	ctx,
				 OlSWGeometries * 	gg,
				 Dimension		width,
				 Dimension		height,
				 int			vsb_width,
				 int			hsb_height);

static int      PanX;
static int      PanY;
static int      click_mode = XVIEW_CLICK_MODE;
static const size_t	peb_alloc = 16;
static const size_t	num_feedbacks = 6;

static char     defaultTextEditTranslations[] = "\
	<FocusIn>:	OlAction()	\n\
	<FocusOut>:	OlAction()	\n\
	<Key>:		OlAction()	\n\
	<BtnDown>:	OlAction()	\n\
	<BtnUp>:	OlAction()	\n\
	<Message>:	message()	\n\
	<Prop>:		property()	\n\
";

static XtActionsRec textEditActionsTable[] = {
	{"message", ReceivePasteMessage},
	{"property", IgnorePropertyNotify},
	{NULL, NULL}
    };

static OlEventHandlerRec event_procs[] = {
	{KeyPress, Key},
	{ButtonPress, Button}
    };

#define BYTE_OFFSET	XtOffsetOf(TextEditRec, textedit.dyn_flags)
    static _OlDynResource dyn_res[] = {
	{{XtNbackground, XtCTextBackground, XtRPixel, sizeof(Pixel), 0,
	XtRString, XtDefaultBackground}, BYTE_OFFSET,
	OL_B_TEXTEDIT_BG, NULL},
	{{XtNfontColor, XtCFontColor, XtRPixel, sizeof(Pixel), 0,
	XtRString, XtDefaultForeground}, BYTE_OFFSET,
	OL_B_TEXTEDIT_FONTCOLOR, NULL},
    };
#undef BYTE_OFFSET

#define OFFSET(field)    XtOffsetOf(TextEditRec, textedit.field)

static XtResource resources[] = {
	/* this should be the first resource */
	{XtNsourceType, XtCSourceType, XtROlSourceType, sizeof(OlSourceType),
	OFFSET(sourceType), XtRString, "stringsource"},

	{XtNbackground, XtCTextBackground, XtRPixel, sizeof(Pixel),
	    XtOffsetOf(TextEditRec, core.background_pixel), XtRString,
	XtDefaultBackground},

	{XtNtabTable, XtCTabTable, XtRPointer, sizeof(TabTable),
	OFFSET(tabs), XtRPointer, NULL},

	{XtNcharsVisible, XtCCharsVisible, XtRInt, sizeof(int),
	OFFSET(charsVisible), XtRString, "50"},

	{XtNlinesVisible, XtCLinesVisible, XtRInt, sizeof(int),
	OFFSET(linesVisible), XtRString, "16"},

	{XtNfontColor, XtCFontColor, XtRPixel, sizeof(Pixel),
	    XtOffsetOf(TextEditRec, primitive.font_color), XtRString,
	XtDefaultForeground},

 	{XtNinputFocusColor, XtCInputFocusColor, XtRPixel, sizeof(Pixel),
 	    XtOffsetOf(TextEditRec, primitive.input_focus_color), 
 	    XtRCallProc, (XtPointer)_OlGetDefaultFocusColor},

	{XtNdisplayPosition, XtCTextPosition, XtRInt, sizeof(TextPosition),
	OFFSET(displayPosition), XtRString, "0"},

	{XtNcursorPosition, XtCTextPosition, XtRInt, sizeof(TextPosition),
	OFFSET(cursorPosition), XtRString, "0"},

	{XtNselectStart, XtCTextPosition, XtRInt, sizeof(TextPosition),
	OFFSET(selectStart), XtRString, "0"},

	{XtNselectEnd, XtCTextPosition, XtRInt, sizeof(TextPosition),
	OFFSET(selectEnd), XtRString, "0"},

	{XtNleftMargin, XtCMargin, XtRDimension, sizeof(Dimension),
	OFFSET(leftMargin), XtRString, "4"},

	{XtNrightMargin, XtCMargin, XtRDimension, sizeof(Dimension),
	OFFSET(rightMargin), XtRString, "4"},

	{XtNtopMargin, XtCMargin, XtRDimension, sizeof(Dimension),
	OFFSET(topMargin), XtRString, "4"},

	{XtNbottomMargin, XtCMargin, XtRDimension, sizeof(Dimension),
	OFFSET(bottomMargin), XtRString, "4"},

	{XtNeditType, XtCEditType, XtROlEditMode, sizeof(OlEditMode),
	OFFSET(editMode), XtRString, "textedit"},

	{XtNimPreeditStyle, XtCImPreeditStyle, XtROlImPreeditStyle, 
	sizeof(OlImPreeditStyle), OFFSET(pre_edit_style), 
		XtRImmediate, (XtPointer)OL_NO_PREEDIT},

	{XtNwrapMode, XtCWrapMode, XtROlWrapMode, sizeof(OlWrapMode),
	OFFSET(wrapMode), XtRString, "wrapwhitespace"},

	{XtNsource, XtCSource, XtROlTextSource, sizeof(OlTextSource),
	OFFSET(source), XtRString, ""},

	{XtNbuttons, XtCCallback, XtRCallback, sizeof(XtCallbackProc),
	OFFSET(buttons), XtRCallback, (XtPointer) NULL},

	{XtNkeys, XtCCallback, XtRCallback, sizeof(XtCallbackProc),
	OFFSET(keys), XtRCallback, (XtPointer) NULL},

	{XtNmotionVerification, XtCCallback, XtRCallback, sizeof(XtCallbackProc),
	OFFSET(motionVerification), XtRCallback, (XtPointer) NULL},

	{XtNmodifyVerification, XtCCallback, XtRCallback, sizeof(XtCallbackProc),
	OFFSET(modifyVerification), XtRCallback, (XtPointer) NULL},

	{XtNpostModifyNotification, XtCCallback, XtRCallback, sizeof(XtCallbackProc),
	OFFSET(postModifyNotification), XtRCallback, (XtPointer) NULL},

	{XtNblinkRate, XtCBlinkRate, XtRInt, sizeof(long),	/* NOTE: no XtRLong!!! */
	OFFSET(blinkRate), XtRString, "666"},

	{XtNmargin, XtCCallback, XtRCallback, sizeof(XtCallbackProc),
	OFFSET(margin), XtRCallback, (XtPointer) NULL},

	{XtNregisterFocusFunc, XtCRegisterFocusFunc, XtRFunction, sizeof(OlRegisterFocusFunc),
	OFFSET(register_focus), XtRFunction, (XtPointer) NULL},

	{XtNinsertTab, XtCInsertTab, XtRBoolean, sizeof(Boolean),
	OFFSET(insertTab), XtRImmediate, (XtPointer) TRUE},

	/* Some private resources */
	/*
	 * XtNinsertReturn is a private resource that is used only by
	 * TextField to tell textEdit widget not to consume or beep on a
	 * OL_RETURN key.
	 */
	{XtNinsertReturn, XtCInsertReturn, XtRBoolean, sizeof(Boolean),
	OFFSET(insertReturn), XtRImmediate, (XtPointer) TRUE},

	 /* XtNcursorVisible is a Private resource. Do not document it.
	    It is used to tell the textedit widget to make its caret 
	invisible when it no longer is the initial focus widget. */

 	{XtNcursorVisible, XtCCursorVisible, XtRBoolean, sizeof(Boolean),
 	OFFSET(cursor_visible), XtRString, "False"},

	{XtNtabTable, XtCTabTable, XtRPointer, sizeof(TabTable),
	OFFSET(tabs), XtRPointer, NULL},

	{XtNdropSiteID, XtCReadOnly, XtRPointer, sizeof(OlDnDDropSiteID),
	OFFSET(dropsiteid), XtRImmediate, (XtPointer) NULL},

	{XtNgrowMode, XtCGrowMode, XtROlDefine, sizeof(OlDefine),
	OFFSET(growMode), XtRImmediate, (XtPointer) OL_GROW_OFF},

	/* resources added for Level 3 I18N */
#ifdef  XGETTEXT
	{XtNmenuTitle, XtCMenuTitle, XtROlStr, sizeof(OlStr),
	OFFSET(menuTitle), XtRString, 
	(XtPointer) dgettext(OlMsgsDomain,"Edit")},

	{XtNundoLabel, XtCUndoLabel, XtROlStr, sizeof(OlStr),
	OFFSET(undoLabel), XtRString, 
	(XtPointer)  dgettext(OlMsgsDomain,"Undo")},

	{XtNcutLabel, XtCCutLabel, XtROlStr, sizeof(OlStr),
	OFFSET(cutLabel), XtRString, 
	(XtPointer) dgettext(OlMsgsDomain,"Cut")},

	{XtNcopyLabel, XtCCopyLabel, XtROlStr, sizeof(OlStr),
	OFFSET(copyLabel), XtRString, 
	(XtPointer) dgettext(OlMsgsDomain,"Copy")},

	{XtNpasteLabel, XtCPasteLabel, XtROlStr, sizeof(OlStr),
	OFFSET(pasteLabel), XtRString, 
	(XtPointer) dgettext(OlMsgsDomain,"Paste")},

	{XtNdeleteLabel, XtCDeleteLabel, XtROlStr, sizeof(OlStr),
	OFFSET(deleteLabel), XtRString, 
	(XtPointer) dgettext(OlMsgsDomain,"Delete")},
#else
	{XtNmenuTitle, XtCMenuTitle, XtROlStr, sizeof(OlStr),
	OFFSET(menuTitle), XtRLocaleString, (XtPointer) "Edit"},

	{XtNundoLabel, XtCUndoLabel, XtROlStr, sizeof(OlStr),
	OFFSET(undoLabel), XtRLocaleString, (XtPointer) "Undo"},

	{XtNcutLabel, XtCCutLabel, XtROlStr, sizeof(OlStr),
	OFFSET(cutLabel), XtRLocaleString, (XtPointer) "Cut"},

	{XtNcopyLabel, XtCCopyLabel, XtROlStr, sizeof(OlStr),
	OFFSET(copyLabel), XtRLocaleString, (XtPointer) "Copy"},

	{XtNpasteLabel, XtCPasteLabel, XtROlStr, sizeof(OlStr),
	OFFSET(pasteLabel), XtRLocaleString, (XtPointer) "Paste"},

	{XtNdeleteLabel, XtCDeleteLabel, XtROlStr, sizeof(OlStr),
	OFFSET(deleteLabel), XtRLocaleString, (XtPointer) "Delete"},
#endif
	{XtNundoMnemonic, XtCUndoMnemonic, OlRChar, sizeof(char),
	OFFSET(undoMnemonic), OlRChar, (XtPointer) NULL},

	{XtNcutMnemonic, XtCCutMnemonic, OlRChar, sizeof(char),
	OFFSET(cutMnemonic), OlRChar, (XtPointer) NULL},

	{XtNcopyMnemonic, XtCCopyMnemonic, OlRChar, sizeof(char),
	OFFSET(copyMnemonic), OlRChar, (XtPointer) NULL},

	{XtNpasteMnemonic, XtCPasteMnemonic, OlRChar, sizeof(char),
	OFFSET(pasteMnemonic), OlRChar, (XtPointer) NULL},

	{XtNdeleteMnemonic, XtCDeleteMnemonic, OlRChar, sizeof(char),
	OFFSET(deleteMnemonic), OlRChar, (XtPointer) NULL},

    };
#undef OFFSET

TextEditClassRec textEditClassRec = {
	{
	     /* superclass          */ (WidgetClass) & primitiveClassRec,
	     /* class_name          */ "TextEdit",
	     /* widget_size         */ sizeof(TextEditRec),
	     /* class_initialize    */ ClassInitialize,
	     /* class_part_init     */ ClassPartInitialize,
	     /* class_inited        */ FALSE,
	     /* initialize          */ Initialize,
	     /* initialize_hook     */ NULL,
	     /* realize             */ Realize,
	     /* actions             */ textEditActionsTable,
	     /* num_actions         */ XtNumber(textEditActionsTable),
	     /* resources           */ resources,
	     /* num_ resource       */ XtNumber(resources),
	     /* xrm_class           */ NULLQUARK,
	     /* compress_motion     */ TRUE,
	     /* compress_exposure   */ TRUE,
	     /* compress_enterleave */ TRUE,
	     /* visible_interest    */ FALSE,
	     /* destroy             */ Destroy,
	     /* resize              */ Resize,
	     /* expose              */ Redisplay,
	     /* set_values          */ SetValues,
	     /* set_values_hook     */ NULL,
	     /* set_values_almost   */ XtInheritSetValuesAlmost,
	     /* get_values_hook     */ NULL,
	     /* accept_focus        */ XtInheritAcceptFocus,
	     /* version             */ XtVersion,
	     /* callback_private    */ NULL,
	     /* tm_table            */ defaultTextEditTranslations
	},
	{
	    /* primitive class 	 */
	     /* reserved       	 */ NULL,
	     /* highlight_handler	 */ FocusHandler,
	     /* traversal_handler	 */ NULL,
	     /* register_focus	 */ RegisterFocus,
	     /* activate		 */ ActivateWidget,
	     /* event_procs		 */ event_procs,
	     /* num_event_procs	 */ XtNumber(event_procs),
	     /* version		 */ OlVersion,
	     /* extension		 */ NULL,
	     /* dyn_data		 */ {dyn_res, XtNumber(dyn_res)},
	     /* transparent_proc	 */ NULL,
	     /* query_sc_locn_proc      */ NULL
	},
	{
	    /* TextEdit fields */
	    NULL,
	}
    };

WidgetClass     textEditWidgetClass = (WidgetClass) & textEditClassRec;

/*
 * Delete
 *
 */
static void
Delete(TextEditWidget ctx, XEvent *event, OlInputEvent keyarg, int unused)
{
    if (ctx->textedit.text_format == OL_SB_STR_REP)
	DeleteSB(ctx, event, keyarg, unused);
    else
	DeleteMBWC(ctx, event, keyarg, unused);
}

/*
 * DeleteSB
 *
 */
static void
DeleteSB(TextEditWidget ctx, XEvent *event, OlInputEvent keyarg, int unused)
{
    OlStr	    empty_str;
    TextEditPart   *text = &ctx->textedit;
    TextBuffer     *textBuffer = text->textBuffer;
    TextPosition    position;
    TextLocation    loc;
    TextLocation    newloc;
    TextLocation    oldloc;
    TextPosition    newpos;

    empty_str = (OlStr)"";
    if (text->selectStart < text->selectEnd)
	TextEditCheckAndInsert(ctx, empty_str, 0);
    else {
	switch (keyarg) {
	case OL_DELCHARFWD:
	    position = text->selectEnd + 1;
	    if (position <= LastTextBufferPosition(textBuffer)) {
		text->selectEnd = position;
		TextEditCheckAndInsert(ctx, empty_str, 0); 
	    }
	    break;
	case OL_DELCHARBAK:
	    position = text->selectStart - 1;
	    if (position >= 0) {
		text->selectStart = position;
		TextEditCheckAndInsert(ctx, empty_str, 0); 
	    }
	    break;
	case OL_DELWORDFWD:
	    oldloc = LocationOfPosition(textBuffer, text->selectEnd);
	    newloc = EndCurrentTextBufferWord(textBuffer, oldloc);
	    if (SameTextLocation(newloc, oldloc)) {
		newloc = NextTextBufferWord(textBuffer, newloc);
		newloc = EndCurrentTextBufferWord(textBuffer, newloc);
	    }
	    newpos = PositionOfLocation(textBuffer, newloc);
	    if (newpos > text->selectEnd) {
		text->selectEnd = PositionOfLocation(textBuffer, newloc);
		TextEditCheckAndInsert(ctx, empty_str, 0);
	    }
	    break;
	case OL_DELWORDBAK:
	    oldloc = LocationOfPosition(textBuffer, text->selectStart);
	    newloc = StartCurrentTextBufferWord(textBuffer, oldloc);
	    if (SameTextLocation(newloc, oldloc))
		newloc = PreviousTextBufferWord(textBuffer, newloc);
	    newpos = PositionOfLocation(textBuffer, newloc);
	    if (newpos < text->selectStart) {
		text->selectStart = PositionOfLocation(textBuffer, newloc);
		TextEditCheckAndInsert(ctx, empty_str, 0);
	    }
	    break;
	case OL_DELLINEBAK:
	    loc = LocationOfPosition(textBuffer, text->selectStart);
	    loc.offset = 0;
	    position = PositionOfLocation(textBuffer, loc);
	    if (position < text->selectStart) {
		text->selectStart = position;
		TextEditCheckAndInsert(ctx, empty_str, 0);
	    }
	    break;
	case OL_DELLINEFWD:
	    loc = LocationOfPosition(textBuffer, text->selectEnd);
	    loc.offset = LastCharacterInTextBufferLine(textBuffer, loc.line);
	    position = PositionOfLocation(textBuffer, loc);
	    if (position > text->selectEnd) {
		text->selectEnd = position;
		TextEditCheckAndInsert(ctx, empty_str, 0);
	    }
	    break;
	case OL_DELLINE:
	    if (!TextBufferEmpty(textBuffer)) {
		loc = LocationOfPosition(textBuffer, text->selectStart);
		loc.offset = 0;
		text->selectStart = PositionOfLocation(textBuffer, loc);

		loc = LocationOfPosition(textBuffer, text->selectEnd);
		loc.offset = LastCharacterInTextBufferLine(textBuffer, loc.line);
		text->selectEnd = PositionOfLocation(textBuffer, loc);

		/*
		 * include the newline (if it's there)
		 */

		position = text->selectEnd + 1;
		if (position <= LastTextBufferPosition(textBuffer))
		    text->selectEnd = position;

		TextEditCheckAndInsert(ctx, empty_str, 0);
	    }
	    break;
	default:
	    return;
	}
    }
}				/* end of DeleteSB */

/*
 * DeleteMBWC
 *
 */

static void
DeleteMBWC(TextEditWidget ctx, XEvent *event, OlInputEvent keyarg, int unused)
{
    TextEditPart *     text = &ctx->textedit;
    OlTextBufferPtr    mltextBuf = (OlTextBufferPtr)text->textBuffer;
    TextPosition       position;
    TextLocation       loc;
    TextLocation       newloc;
    TextLocation       oldloc;
    TextPosition       newpos;
    OlStr              empty_str;
    
    empty_str = str_methods[text->text_format].StrEmptyString();
    
    if (text->selectStart < text->selectEnd)
	TextEditCheckAndInsert(ctx, empty_str, 0);
    else {
	TextPosition lastpos;
	
	switch (keyarg) {
	case OL_DELCHARFWD:
	    position = text->selectEnd + 1;
	    lastpos = OlLastTextBufferPosition(mltextBuf);
	    if (position <= lastpos) {
		text->selectEnd = position;
		TextEditCheckAndInsert(ctx, empty_str, 0); 
	    }
	    break;
	case OL_DELCHARBAK:
	    position = text->selectStart - 1;
	    if (position >= 0) {
		text->selectStart = position;
		TextEditCheckAndInsert(ctx, empty_str, 0); 
	    }
	    break;
	case OL_DELWORDFWD:
	    OlLocationOfPosition(mltextBuf, text->selectEnd, &oldloc);
	    newloc = oldloc;
	    OlEndCurrentTextBufferWord(mltextBuf, &newloc);
	    if (SameTextLocation(newloc, oldloc)) { /* I18N: where is this fn */
		OlNextTextBufferWord(mltextBuf, &newloc);
		OlEndCurrentTextBufferWord(mltextBuf, &newloc);
	    }
	    newpos = OlPositionOfLocation(mltextBuf, &newloc);
	    if (newpos > text->selectEnd) {
		text->selectEnd = OlPositionOfLocation(mltextBuf, &newloc);
		TextEditCheckAndInsert(ctx, empty_str, 0);
	    }
	    break;
	case OL_DELWORDBAK:
	    OlLocationOfPosition(mltextBuf, text->selectStart, &oldloc);
	    newloc = oldloc;
	    OlStartCurrentTextBufferWord(mltextBuf, &newloc);
	    if (SameTextLocation(newloc, oldloc))
		OlPreviousTextBufferWord(mltextBuf, &newloc);
	    newpos = OlPositionOfLocation(mltextBuf, &newloc);
	    if (newpos < text->selectStart) {
		text->selectStart = OlPositionOfLocation(mltextBuf, &newloc);
		TextEditCheckAndInsert(ctx, empty_str, 0);
	    }
	    break;
	case OL_DELLINEBAK:
	    OlLocationOfPosition(mltextBuf, text->selectStart, &loc);
	    loc.offset = 0;
	    position = OlPositionOfLocation(mltextBuf, &loc);
	    if (position < text->selectStart) {
		text->selectStart = position;
		TextEditCheckAndInsert(ctx, empty_str, 0);
	    }
	    break;
	case OL_DELLINEFWD:
	    OlLocationOfPosition(mltextBuf, text->selectEnd, &loc);
	    loc.offset = OlLastCharInTextBufferLine(mltextBuf, loc.line);
	    position = OlPositionOfLocation(mltextBuf, &loc);
	    if (position > text->selectEnd) {
		text->selectEnd = position;
		TextEditCheckAndInsert(ctx, empty_str, 0);
	    }
	    break;
	case OL_DELLINE:
	    if (!OlIsTextBufferEmpty(mltextBuf)) {
		OlLocationOfPosition(mltextBuf, text->selectStart, &loc);
		loc.offset = 0;
		text->selectStart = OlPositionOfLocation(mltextBuf, &loc);

		OlLocationOfPosition(mltextBuf, text->selectEnd, &loc);
		loc.offset = OlLastCharInTextBufferLine(mltextBuf, loc.line);
		text->selectEnd = OlPositionOfLocation(mltextBuf, &loc);

		/*
		 * include the newline (if it's there)
		 */

		position = text->selectEnd + 1;
		if (position <= OlLastTextBufferPosition(mltextBuf))
		    text->selectEnd = position;

		TextEditCheckAndInsert(ctx, empty_str, 0);
	    }
	    break;
	default:
	    return;
	}
    }
} /* end of DeleteMBWC */

/*
 * AdjustPosition
 *
 */
static          TextPosition
AdjustPosition(TextPosition current, int dlen, int ilen, TextPosition editstart, TextPosition delend)
{

    if (current < editstart);
    else {
	if (current >= delend) {
	    current -= dlen;
	    current += ilen;
	} else
	    current = editstart;
    }

    return (current);

}				/* end of AdjustPosition */
/*
 * _AdjustDisplayLocation
 *
 */

static          Boolean
_AdjustDisplayLocation(TextEditWidget ctx, TextEditPart *text, WrapTable *wrapTable, TextLocation *currentDP, int page)
{
    int             diff;
    TextLocation    min;
    TextLocation    max;
    Boolean         retval;
    OlStrRep   	    rep = ctx->primitive.text_format;

    if(text->linesVisible == 0 	||
	text->charsVisible == 0	)
	return FALSE;
    if (text->lineCount <= text->linesVisible &&
	currentDP->line == 0 &&
	currentDP->offset == 0) {
	text->displayLocation = *currentDP;
	text->displayPosition = 0;
	retval = FALSE;
    } else {
	min = _FirstWrapLine(wrapTable);
	max = _LastWrapLine(wrapTable);

	(void) _IncrementWrapLocation(wrapTable, *currentDP, page + 0, max, &diff);

	/*
	if ((diff = diff - page + 1) < 0)
	    *currentDP = _IncrementWrapLocation(wrapTable, *currentDP, diff, min, NULL);
	*/
	text->displayLocation = _LocationOfWrapLocation(wrapTable, *currentDP);
	text->displayPosition = (rep == OL_SB_STR_REP)?
		PositionOfLocation(text->textBuffer, text->displayLocation):
	        OlPositionOfLocation((OlTextBufferPtr)text->textBuffer, 
						&(text->displayLocation));
	retval = (diff < 0);
    }

    return (retval);

}				/* end of _AdjustDisplayLocation */
/*
 * _CheckForScrollbarStateChange
 *
 */

Boolean
_CheckForScrollbarStateChange(TextEditWidget ctx, TextEditPart *text)
{
    Boolean         saveUpdateState;
    int             vsb_state_change;
    int             hsb_state_change;
    Boolean         retval;

    if(text->linesVisible == 0 	||
	text->charsVisible == 0	)
	return FALSE;

    if (text->vsb == NULL)
	retval = FALSE;
    else {
	vsb_state_change =
	    (text->need_vsb == 2 && text->linesVisible >= text->lineCount) ||
	    (text->need_vsb == 0 && text->linesVisible < text->lineCount);
	hsb_state_change =
	    (text->need_hsb == 2 && PAGEWID(ctx) >= text->maxX) ||
	    (text->need_hsb == 0 && PAGEWID(ctx) < text->maxX &&
	     text->wrapMode == OL_WRAP_OFF);

	if (retval = (hsb_state_change || vsb_state_change)) {
	    saveUpdateState = text->updateState;
	    text->updateState = FALSE;

	    OlLayoutScrolledWindow((ScrolledWindowWidget)
					(XtParent(XtParent(ctx))), 0);

	    text->updateState = saveUpdateState;
	}
    }

    return (retval);

}				/* end of _CheckForScrollbarStateChange */

static Boolean
TextEditResizeRequest(TextEditWidget ctx,
		      int            width,
		      int            height)
{
    TextEditPart   *text = &ctx->textedit;
    Boolean         save_updateState = text->updateState;
    Boolean	    resized = (Boolean)False;
	    
    text->updateState = (Boolean) False;
    if (XtGeometryYes == XtMakeResizeRequest((Widget) ctx, width, height, NULL, NULL)) {
	resized = (Boolean)True;
	Resize((Widget)ctx);
    }
    text->updateState = save_updateState;
    return(resized);
}
/*
 * UpdateDisplay
 *
 */

void
UpdateDisplay(TextEditWidget  ctx,
	      XtPointer       genericBuffer,
	      int             requestor)
{
    TextEditPart   *text = &ctx->textedit;
    int             page;
    Boolean         moved = FALSE;

    TextPosition    editStart;
    TextPosition    delEnd;
    TextPosition    position;
    TextLocation    currentDP;
    TextLocation    currentIP;
    WrapTable      *wrapTable = text->wrapTable;
    /* set AFTER _UpdateWrapTable stuff is done */
    int             diff;
    int             i;
    int             row;
    int             xoffset;
    int             space_needed;
    int             LastValidLine;
    int             start;
    Boolean         resized = False;
    TextBuffer     *textBuffer = (TextBuffer *)genericBuffer;
    OlTextBufferPtr mltBuf = (OlTextBufferPtr)genericBuffer; 
    
    XRectangle      rect;

    OlTextMotionCallData     motion_call_data;
    OlTextPostModifyCallData post_modify_call_data;

    OlStrRep        rep = text->text_format;
    TextLocation    del_start;
    int		    num_lines;
    OlTextUndoItem  insert;
    OlTextUndoItem  delete;
    int		    new_start = wrapTable->cur_start;
    int		    new_end   = wrapTable->cur_end;

    if (rep == OL_SB_STR_REP) {
	unsigned char*	cstring;

	insert.hint = textBuffer->insert.hint;
	delete.hint = textBuffer->deleted.hint;
	if (insert.hint == 0 && delete.hint == 0) /* nothing to do !!! */
	    return;
	delete.start  = textBuffer->deleted.start;
	delete.end    = textBuffer->deleted.end;
	delete.string = (char *)textBuffer->deleted.string;
	insert.start  = textBuffer->insert.start;
	insert.end    = textBuffer->insert.end;
	insert.string = (char *)textBuffer->insert.string;
	editStart     = PositionOfLocation(textBuffer, delete.start);
	cstring = (unsigned char*)(insert.string);
	if (cstring == NULL || cstring[0] == '\0')
	    position  = editStart;
	else
	    position  = PositionOfLocation(textBuffer, insert.end);
	num_lines     =	LinesInTextBuffer(textBuffer);
    } else {
	insert = OlGetTextUndoInsertItem(mltBuf);
	delete = OlGetTextUndoDeleteItem(mltBuf);
	if (insert.hint == 0 && delete.hint == 0)
	    return;
	editStart = OlPositionOfLocation(mltBuf, &(delete.start));
	if (insert.string == (OlStr)NULL ||
	    str_methods[rep].StrCmp(insert.string, str_methods[rep].StrEmptyString()) == 0)
	    position  = editStart;
	else
	    position  = OlPositionOfLocation(mltBuf, &(insert.end));
	num_lines     = OlLinesInTextBuffer(mltBuf);
    }
    space_needed  = num_lines + 1 - wrapTable->contents->size;
    LastValidLine = delete.end.line + 1;
    del_start     = delete.start;
    start         = del_start.line;
    if (space_needed == 0) {
        int i;

        new_start = delete.start.line;
        new_end = delete.end.line;
        for (i = new_start; i <= new_end; i++)
            if (wrapTable->contents->p[i] != NULL)
                text->lineCount -= wrapTable->contents->p[i]->used;
#ifdef DEBUG
            else
                fprintf (stderr, "UpdateDisplay: space_needed == 0, wrc->p[%d] == NULL\n", i);
#endif /* DEBUG */

	_UpdateWrapTable(ctx, new_start, new_end); 
    } else {
	for (i = start; i < LastValidLine; i++) {
	    if (wrapTable->contents->p[i] != NULL)
	    {
		text->lineCount -= wrapTable->contents->p[i]->used;
		FreeBuffer((Buffer *) (wrapTable->contents->p[i]));
		wrapTable->contents->p[i] = NULL;
	    }
#ifdef DEBUG
            else
            {   
                fprintf (stderr, "UpdateDisplay: wrapTable->contents->p[%d] == ULL\n", i);
                fflush (stderr);
            }
#endif /* DEBUG */
	}

	if (space_needed > 0) {
	    GrowBuffer((Buffer *) (wrapTable->contents), space_needed);
	    memmove(&wrapTable->contents->p[LastValidLine + space_needed],
		    &wrapTable->contents->p[LastValidLine],
		    sizeof(wrapTable->contents->p[0]) *
		    (wrapTable->contents->size - (LastValidLine + space_needed)));
	    for(i=LastValidLine; 
			i < LastValidLine + space_needed; i++)
			wrapTable->contents->p[i] = NULL;
	    wrapTable->contents->used = num_lines;
	    new_start = start;
	    new_end   = LastValidLine + space_needed - 1;
	    _UpdateWrapTable(ctx, new_start, new_end);
	} else if (space_needed < 0) {
	    memmove(&wrapTable->contents->p[LastValidLine + space_needed],
		    &wrapTable->contents->p[LastValidLine],
		    sizeof(wrapTable->contents->p[0]) *
		    (wrapTable->contents->size - LastValidLine ));
	    GrowBuffer((Buffer *) (wrapTable->contents), space_needed);
	    wrapTable->contents->used = num_lines;
	    new_start = start;
	    new_end   = LastValidLine + space_needed - 1;
	    _UpdateWrapTable(ctx, new_start, new_end);
	}
    }
    {
	int cur_start = wrapTable->cur_start;
	int	cur_end   = wrapTable->cur_end;

	/* Update cur_start and cur_end to reflect the edits */
	if (space_needed > 0) {
	    if (cur_start >= LastValidLine)
		cur_start += space_needed;
	    if (cur_end >= LastValidLine)
		cur_end += space_needed;
	} else if (space_needed < 0) {
	    if (cur_start >= LastValidLine)
		cur_start += space_needed;
	    else if (cur_start >= start)
		cur_start = LastValidLine + space_needed - 1;
	    if (cur_end >= LastValidLine)
		cur_end += space_needed;
	    else if (cur_end >= start)
		cur_end = LastValidLine + space_needed - 1;
	}

	/* Merge changes to wrap table */
	if (cur_end < new_start) {
	    if (new_start - cur_end > 1)
		_UpdateWrapTable(ctx, cur_end+1, new_start-1);
	    new_start = cur_start;
	} else if (new_end < cur_start) {
	    if (cur_start - new_end > 1)
		_UpdateWrapTable(ctx, new_end+1, cur_start-1);
	    new_end = cur_end;
	} else {
	    if (new_start > cur_start)
		new_start = cur_start;
	    if (new_end < cur_end)
		new_end   = cur_end;
	}
	wrapTable->cur_start = new_start;
	wrapTable->cur_end   = new_end;
    }
    /****************************************************************************
     * AutoGrow stuff
     * ToDo - If Resize request fails ,maybe we can note it to avoid future
     * resize requests in that direction . Ref TextPane.c/Text.c
     ****************************************************************************/
    if (text->growMode != OL_GROW_OFF) {
	Dimension       width = ctx->core.width;
	Dimension       height = ctx->core.height;

	if (text->growMode != OL_GROW_VERTICAL) {	/* Horizontal or Both */
	    Dimension       maxWidth;
	    
	    maxWidth = text->leftMargin + text->rightMargin + text->maxX;
	    width = (maxWidth > ctx->core.width) ?
		maxWidth : ctx->core.width;
	}
	if (text->growMode != OL_GROW_HORIZONTAL) {	/* Vertical or Both */
	    Dimension       maxHeight;
	    
	    maxHeight = text->lineHeight * text->lineCount +
		PAGE_T_GAP(ctx) + PAGE_B_GAP(ctx);
	    height = (maxHeight > ctx->core.height) ?
		maxHeight : ctx->core.height;
	}
	if ((width != ctx->core.width) || (height != ctx->core.height))
	    resized = TextEditResizeRequest(ctx, width, height);
    }
    if (requestor) {
	_TurnTextCursorOff(ctx);
	if (text->displayPosition > editStart) {
	    moved = TRUE;
	    text->displayPosition = editStart;
	    text->displayLocation = del_start;
	}
	page                            = LINES_VISIBLE(ctx);
	currentDP                       = _WrapLocationOfLocation(wrapTable,
								  text->displayLocation);
	moved                          |= _AdjustDisplayLocation(ctx, text,
								 wrapTable, &currentDP, page);
	moved                          |= _CheckForScrollbarStateChange(ctx, text);
	page                            = LINES_VISIBLE(ctx);
	text->cursorPosition            =
	text->selectStart               =
	text->selectEnd                 = position;
	if( !(text->pre_edit == TRUE && text->pre_edit_end >
					text->pre_edit_start)) {
	motion_call_data.current_cursor =
	motion_call_data.new_cursor     =
	motion_call_data.select_start   =
	motion_call_data.select_end     = position;
	motion_call_data.ok             = TRUE;
		XtCallCallbacks((Widget) ctx, 
			XtNmotionVerification, &motion_call_data);
	}

	if (rep == OL_SB_STR_REP)
	    text->cursorLocation = LocationOfPosition(textBuffer, text->cursorPosition);
	else
	    OlLocationOfPosition((OlTextBufferPtr)textBuffer, text->cursorPosition,
				 &(text->cursorLocation));
	currentIP                = _WrapLocationOfLocation(wrapTable, text->cursorLocation);
	_CalculateCursorRowAndXOffset(ctx, &row, &xoffset, currentDP, currentIP);
	if (xoffset) {
	    moved = TRUE;
	    text->xOffset -= xoffset;
	}
	if (row < 0 || page <= row) {
	    moved = TRUE;
	    currentDP = (row >= page) ?
		_IncrementWrapLocation(wrapTable, currentDP, row - (page - 1),
			    _LastDisplayedWrapLine(wrapTable, page), &row) :
		_IncrementWrapLocation(wrapTable, currentDP, row,
				       _FirstWrapLine(wrapTable), &row);
	    text->displayLocation = _LocationOfWrapLocation(wrapTable, currentDP);
	    text->displayPosition = (rep == OL_SB_STR_REP)?
		PositionOfLocation(text->textBuffer, text->displayLocation):
		OlPositionOfLocation((OlTextBufferPtr)text->textBuffer, &(text->displayLocation));
	}
    } else {			/* non-requestor */
	int             Dlen;
	int             Ilen;

	delEnd = (int)delete.end.buffer;
	Dlen   = delEnd - (int)delete.start.buffer;
	Ilen   = (int)insert.end.buffer - (int)insert.start.buffer;
	/* Adjust the Cursor Position */
	text->displayPosition = AdjustPosition(text->displayPosition, Dlen,
					       Ilen, editStart, delEnd);
	text->cursorPosition  = AdjustPosition(text->cursorPosition, Dlen,
					       Ilen, editStart, delEnd);
	text->selectEnd       = AdjustPosition(text->selectEnd, Dlen,
					       Ilen, editStart, delEnd);
	text->selectStart     = AdjustPosition(text->selectStart, Dlen,
					       Ilen, editStart, delEnd);
	if (rep == OL_SB_STR_REP) {
	    text->cursorLocation  = LocationOfPosition(textBuffer, text->cursorPosition);
	    text->displayLocation = LocationOfPosition(textBuffer, text->displayPosition);
	} else {
	    OlLocationOfPosition((OlTextBufferPtr)textBuffer, text->cursorPosition, &(text->cursorLocation));
	    OlLocationOfPosition((OlTextBufferPtr)textBuffer, text->displayPosition, &(text->displayLocation));
	}
	currentDP = _WrapLocationOfLocation(wrapTable, text->displayLocation);
	page  = LINES_VISIBLE(ctx);
	moved = _AdjustDisplayLocation(ctx, text, wrapTable, &currentDP, page);
	moved = _CheckForScrollbarStateChange(ctx, text);
    }
    if (resized) {
	rect.x =
	rect.y = 0;
	rect.width = ctx->core.width;
	rect.height = ctx->core.height;
    } else if (moved) {
	rect.x =
	rect.y = 0;
	rect.width = ctx->core.width;
	rect.height = ctx->core.height;
	if (XtIsRealized((Widget) ctx) && text->updateState)
	    XClearArea(XtDisplay(ctx), XtWindow(ctx),
		       rect.x, rect.y, rect.width, rect.height, False);
    } else {
	rect.x     =
	rect.y     =
	rect.width =
	rect.height = 0;
    }

    _DisplayText(ctx, &rect);
    SetScrolledWindowScrollbars(ctx, ctx->core.width, ctx->core.height);

    if( !(text->pre_edit == TRUE && text->pre_edit_end >
				text->pre_edit_start) &&
				text->postModifyNotification != NULL) {
	post_modify_call_data.requestor        = requestor;
	post_modify_call_data.new_cursor       = text->cursorPosition;
	post_modify_call_data.new_select_start = text->selectStart;
	post_modify_call_data.new_select_end   = text->selectEnd;
	post_modify_call_data.inserted         = insert.string;
	post_modify_call_data.deleted          = delete.string;
	post_modify_call_data.delete_start     = delete.start;
	post_modify_call_data.delete_end       = delete.end;
	post_modify_call_data.insert_start     = insert.start;
	post_modify_call_data.insert_end       = insert.end;
	post_modify_call_data.cursor_position  = text->cursorPosition;
	XtCallCallbacks((Widget)ctx, XtNpostModifyNotification, &post_modify_call_data);
    }
} /* end of UpdateDisplay */

extern Boolean
_BGDisplayChange(TextEditWidget ctx)
{
    TextEditPart *     text = &ctx->textedit;
    Boolean	       ret_val = FALSE;
    XRectangle	       rect;

    if (text->vsb != NULL &&
	text->wrapTable &&
	text->wrapTable->build_wrap) {    /* must be in a scrolled window
					   * and the size changed */
	if (text->wrapMode == OL_WRAP_OFF && PAGEWID(ctx) < text->maxX)	{
	    Boolean saveUpdateState = text->updateState;
	    
	    text->wrapTable->build_wrap = FALSE;

	    text->updateState           = FALSE;
	    OlLayoutScrolledWindow((ScrolledWindowWidget)
					(XtParent(XtParent(ctx))), 0);
	    text->updateState = saveUpdateState;
	    
	    rect.x      = rect.y = 0;
	    rect.width  = ctx->core.width;
	    rect.height = ctx->core.height;
	    if (XtIsRealized((Widget) ctx) && text->updateState)
		XClearArea(XtDisplay(ctx), XtWindow(ctx),
			   rect.x, rect.y, rect.width, rect.height, False);
	    
	    _DisplayText(ctx, &rect);
	    SetScrolledWindowScrollbars(ctx, ctx->core.width, ctx->core.height);
	    
	    text->wrapTable->build_wrap = TRUE;
	    ret_val                     = TRUE;
	}
    }
    return(ret_val);
}

/*
 * UndoUpdate
 *
 */
static void
UndoUpdate(TextEditWidget ctx)
{
    TextEditPart   *text = &ctx->textedit;
    TextBuffer     *textBuffer = text->textBuffer;
    TextPosition    ins_beg;
    TextPosition    ins_end;
    OlStr	    ins_str;
    OlStr	    del_str;
    OlStr	    buffer;
    int             length;
    OlStrRep	    str_rep;

    if ((str_rep = ctx->textedit.text_format) == OL_SB_STR_REP) {
	TextUndoItem    inserted;
	TextUndoItem    deleted;

	inserted = textBuffer->insert;
	ins_str  = (OlStr)inserted.string;
	ins_beg  = PositionOfLocation(textBuffer, inserted.start);
	ins_end  = PositionOfLocation(textBuffer, inserted.end);
	deleted  = textBuffer->deleted;
	del_str  = (OlStr)deleted.string;
    } else {
	OlTextUndoItem    inserted;
	OlTextUndoItem    deleted;
	
	inserted = OlGetTextUndoInsertItem((OlTextBufferPtr)textBuffer);
	ins_str  = inserted.string;
	ins_beg  = OlPositionOfLocation((OlTextBufferPtr)textBuffer, &(inserted.start));
	ins_end  = OlPositionOfLocation((OlTextBufferPtr)textBuffer, &(inserted.end));
	deleted  = OlGetTextUndoDeleteItem((OlTextBufferPtr)textBuffer);
	del_str  = deleted.string;
    }
	
    if (ins_str == (OlStr)NULL && del_str == (OlStr)NULL)
	_OlBeepDisplay((Widget) ctx, 1);
    else {
	OlStr	empty_str = str_methods[str_rep].StrEmptyString();
	int     empty_len = str_methods[str_rep].StrNumBytes(empty_str);
	
	if (del_str != NULL) {
	    length = str_methods[str_rep].StrNumBytes(del_str);
	    buffer = str_methods[str_rep].StrCpy((OlStr)XtMalloc(length), 
								del_str);
	} else {
	    length = 0;
	    buffer = str_methods[str_rep].StrCpy((OlStr)XtMalloc(empty_len), 
							empty_str);
	}

	if (_MoveSelection(ctx, ins_beg, 0, 0, 0)) {
	    text->selectEnd = ins_end;
	    TextEditCheckAndInsert(ctx, buffer, length);
	    XtFree(buffer);
	}
    }

} /* end of UndoUpdate */

/*
 * ValidatePosition
 *
 */
static TextPosition
ValidatePosition(TextPosition min, TextPosition pos, TextPosition last, char *name)
{
    TextPosition    retval;

    if (min <= pos && pos <= last)
		retval = pos;
    else {
		retval = min;

		if (name) {		/* Print warning message .. */
			char *errmsg;
			if (errmsg = malloc(1000)) {
				(void) snprintf(errmsg, 1000, dgettext(OlMsgsDomain,
					"TextEdit: Invalid %1$s position %2$d set to %3$d!\n"), name, pos, min);

				OlWarning(errmsg);
				free(errmsg);
			}
		}
    }

    return (retval);

}				/* end of ValidatePosition */
/*
 * ValidatePositions
 *
 */

static void
ValidatePositions(TextEditPart *text, Boolean warn)
{
    TextPosition    last = (text->text_format == OL_SB_STR_REP)?
	LastTextBufferPosition(text->textBuffer):
	    OlLastTextBufferPosition((OlTextBufferPtr)text->textBuffer);

    text->displayPosition =
	ValidatePosition(0, text->displayPosition, last, 
							warn ? "displayPosition" : (char *)NULL);

    text->cursorPosition =
	ValidatePosition(0, text->cursorPosition, last, 
							warn ? "cursorPosition" : (char *)NULL);

    text->selectStart =
	ValidatePosition(0, text->selectStart, last, 
							warn ? "selectStart" : (char *)NULL);

    text->selectEnd =
	ValidatePosition(0, text->selectEnd, last, 
							warn ? "selectEnd" : (char *)NULL);

    if (text->cursorPosition != text->selectStart &&
	text->cursorPosition != text->selectEnd)
	text->selectStart = text->selectEnd = text->cursorPosition;

    if (text->text_format == OL_SB_STR_REP) {
	text->displayLocation =
	    LocationOfPosition(text->textBuffer, text->displayPosition);
	text->cursorLocation =
	    LocationOfPosition(text->textBuffer, text->cursorPosition);
    } else {
	OlLocationOfPosition((OlTextBufferPtr)text->textBuffer, text->displayPosition,
			     &(text->displayLocation));
	OlLocationOfPosition((OlTextBufferPtr)text->textBuffer, text->cursorPosition,
			     &(text->cursorLocation));
    }
}				/* end of ValidatePositions */
/*
 * ValidateMargins
 *
 */

static void
ValidateMargins(TextEditWidget ctx, TextEditPart *text)
{
    Dimension       defaultMargin;

    if (text->leftMargin < 0)
	text->leftMargin = defaultMargin = _FontSetWidth(ctx) / 2 + 1;
    if (text->rightMargin < 0)
	text->rightMargin = defaultMargin;
    if (text->topMargin < 0)
	text->topMargin = defaultMargin = FONTHT(ctx) / 3 + 1;
    if (text->bottomMargin < 0)
	text->bottomMargin = defaultMargin;

}				/* end of ValidateMargins */

static void
ValidateGrowMode(TextEditPart *text)
{
    if (((text->growMode == OL_GROW_HORIZONTAL) ||
	 (text->growMode == OL_GROW_BOTH)) &&
	(text->wrapMode != OL_WRAP_OFF)) {
	OlWarning(dgettext(OlMsgsDomain,
			   "WrapMode is incompatible with current GrowMode.\n Resetting WrapMode to wrapoff\n"));
	text->wrapMode = OL_WRAP_OFF;
    }
}

/*
 * GetGCs
 * NOTE: ctx and text must refer to the same widget.
 *
 */

static void
GetGCs(TextEditWidget  ctx,
       TextEditPart *  text)
{
    XGCValues       values;
    XtGCMask        v1mask;
    XtGCMask        v2mask;

    v1mask = GCGraphicsExposures | GCForeground | GCBackground;
    v2mask = GCStipple | GCFillStyle;
    if (text->text_format == OL_SB_STR_REP) {
	values.font  = ((XFontStruct *)ctx->primitive.font)->fid;
	v2mask      |= GCFont;
    }

    values.graphics_exposures = TRUE;
    values.background         = ctx->core.background_pixel;
    values.foreground         = ctx->primitive.font_color;
    values.stipple            = OlGet50PercentGrey(XtScreen((Widget)ctx));
    values.line_width = 0;
    values.line_style = LineSolid;
    values.ts_x_origin        =
    values.ts_y_origin        = 0;
    values.fill_style         = XtIsSensitive((Widget)ctx) ? FillSolid :
							     FillStippled;
    text->gc = XtGetGC((Widget)ctx,
		       v1mask | v2mask | GCLineWidth |
			GCLineStyle | GCTileStipXOrigin | GCTileStipYOrigin,
		       &values);

    values.foreground = ctx->core.background_pixel;
    values.background = ctx->primitive.font_color;
    values.fill_style         = FillSolid;
    text->invgc = XtGetGC((Widget)ctx, v1mask | v2mask, &values);

    values.graphics_exposures = FALSE;
    values.foreground         = ctx->primitive.input_focus_color;
    values.background         = ctx->core.background_pixel;
    values.function           = GXxor;
    text->insgc = XtGetGC((Widget)ctx, v1mask | GCFunction, &values);

    if(ctx->textedit.feedbackGCs != (GC *)NULL) {
	ctx->textedit.feedbackGCs[0] = ctx->textedit.invgc; /* reverse*/
	ctx->textedit.feedbackGCs[1] = ctx->textedit.gc; /* text underline */
    }

    _CreateTextCursors(ctx);
} /* end of GetGCs */
/*
 * WrapModeConverter
 *
 */

static          Boolean
WrapModeConverter(Display *dpy, XrmValue *args, Cardinal *num_args, XrmValue *from, XrmValue *to, XtPointer *converter_data)
{
    static OlWrapMode mode;
    OlWrapMode     *mp;
    char           *p = from->addr;

    if (0 == strcmp(p, "wrapany") || 0 == strcmp(p, "olwrapany"))
	mode = OL_WRAP_ANY;
    else if (0 == strcmp(p, "wrapwhitespace") || 0 == strcmp(p, "olwrapwhitespace"))
	mode = OL_WRAP_WHITE_SPACE;
    else if (0 == strcmp(p, "wrapoff") || 0 == strcmp(p, "olwrapoff"))
	mode = OL_WRAP_OFF;
    else {
	mode = OL_WRAP_OFF;
    }

    mp = (OlWrapMode *) (to->addr);
    if (mp == (OlWrapMode *) NULL || to->size != sizeof(OlWrapMode)) {
	to->addr = (caddr_t) & mode;
	to->size = sizeof(OlWrapMode);
    } else
	*mp = mode;

    return (True);
}				/* end of WrapModeConverter */
/*
 * SourceTypeConverter
 *
 */

static          Boolean
SourceTypeConverter(Display *dpy, XrmValue *args, Cardinal *num_args, XrmValue *from, XrmValue *to, XtPointer *converter_data)
{
    static OlSourceType mode;
    OlSourceType   *mp;
    char           *p = from->addr;

    if (0 == strcmp(p, "stringsource") || 0 == strcmp(p, "olstringsource"))
	mode = OL_STRING_SOURCE;
    else if (0 == strcmp(p, "disksource") || 0 == strcmp(p, "oldisksource"))
	mode = OL_DISK_SOURCE;
    else {
	mode = OL_STRING_SOURCE;
    }

    mp = (OlSourceType *) (to->addr);
    if (mp == (OlSourceType *) NULL || to->size != sizeof(OlSourceType)) {
	to->addr = (caddr_t) & mode;
	to->size = sizeof(OlSourceType);
    } else
	*mp = mode;

    return (True);
}				/* end of SourceTypeConverter */

/*
 * ScrolledWindowInterface
 *
 */

static void
ScrolledWindowInterface(TextEditWidget ctx, OlSWGeometries *gg)
{
    TextEditPart   *text = &ctx->textedit;
    Boolean         force_vsb = gg->force_vsb;
    Boolean         force_hsb = gg->force_hsb;

    ResizeScrolledWindow(ctx,
			 (ScrolledWindowWidget)XtParent(XtParent((Widget)ctx)),
			 gg, gg->sw_view_width, gg->sw_view_height, False);

    gg->bbc_real_width = text->wrapMode != OL_WRAP_OFF ? gg->bbc_width :
	text->maxX;
    gg->bbc_real_height = text->lineHeight * text->lineCount;

    text->need_vsb = force_vsb ? 1 : (gg->force_vsb ? 2 : 0);
    text->need_hsb = force_hsb ? 1 : (gg->force_hsb ? 2 : 0);

    if (text->need_vsb || text->need_hsb)
	SetScrolledWindowScrollbars(ctx, gg->bbc_width, gg->bbc_height);

    /* if vertical scrollbar is not needed, start display from first character
     * in first line.
     */
    if (!text->need_vsb) {
	text->displayLocation.line = 0;
	text->displayLocation.offset = 0;
	text->displayPosition = 0;
    }
}				/* end of ScrolledWindowInterface */
/*
 * ComputeSWResizes
 *
 */
static void
ComputeSWResizes(
		    TextEditWidget ctx,
		    OlSWGeometries * gg,
		    Dimension width,
		    Dimension height,
		    int vsb_width,
		    int hsb_height)
{
    TextEditPart *     text       = &ctx->textedit;
    OlStrRep	       str_rep    = text->text_format;
    TextBuffer *       textBuffer = text->textBuffer;
    OlTextBufferPtr    mltbptr    = (OlTextBufferPtr)textBuffer;
    TextLine           lines_in_tb;

    if (str_rep == OL_SB_STR_REP) {
	if (text->wrapTable != (WrapTable *)NULL)
	    text->displayPosition = PositionOfLocation(textBuffer, text->displayLocation);
	lines_in_tb = LinesInTextBuffer(textBuffer);
    } else {
	if (text->wrapTable != (WrapTable *)NULL)
	    text->displayPosition = OlPositionOfLocation(mltbptr, &(text->displayLocation));
	lines_in_tb = OlLinesInTextBuffer(mltbptr);
    }
    if (gg->force_vsb && gg->force_hsb) {
	ctx->core.width = width - vsb_width;
	ctx->core.height = height - hsb_height;
	_BuildWrapTable(ctx);
    } else if (gg->force_vsb || PAGE_LINE_HT(ctx) < lines_in_tb) {
	gg->force_vsb = TRUE;
	ctx->core.width -= vsb_width;
	_BuildWrapTable(ctx);
	if (text->wrapMode == OL_WRAP_OFF && text->maxX > PAGEWID(ctx)) {
	    gg->force_hsb = TRUE;
	    ctx->core.height = height - hsb_height;
	}
    } else if (gg->force_hsb) {
	ctx->core.height = height - hsb_height;
	_BuildWrapTable(ctx);
	gg->force_vsb = text->linesVisible > 0 	&& 
			text->charsVisible > 0 	&&
			text->linesVisible < text->lineCount;
    } else {
	_BuildWrapTable(ctx);
	if (text->wrapMode == OL_WRAP_OFF) {
	    if (text->maxX > PAGEWID(ctx)) {
		gg->force_hsb = TRUE;
		ctx->core.height = height - hsb_height;
	    }
	    text->linesVisible = PAGE_LINE_HT(ctx);
	    if (text->linesVisible > 0 	&& 
		text->charsVisible > 0 	&&
		text->linesVisible < text->lineCount) {
		gg->force_vsb = TRUE;
		ctx->core.width -= vsb_width;
	    }
	} else {
	    if (text->linesVisible > 0 	&&
		text->charsVisible > 0 	&&
		text->linesVisible < text->lineCount) {
		gg->force_vsb = TRUE;
		ctx->core.width -= vsb_width;
		_BuildWrapTable(ctx);
	    }
	}
    }
    if(text->linesVisible == 0 	||
	text->charsVisible == 0	)
	return;
    if (str_rep == OL_SB_STR_REP)
	text->displayLocation = LocationOfPosition(textBuffer, text->displayPosition);
    else
	OlLocationOfPosition(mltbptr, text->displayPosition, &(text->displayLocation));
}
    
/*
 * ResizeScrolledWindow
 *
 */
static void
ResizeScrolledWindow(TextEditWidget ctx, ScrolledWindowWidget sw, OlSWGeometries *gg, Dimension width, Dimension height, int resize)
{
    int             vsb_width = resize ? 0 : gg->vsb_width;
    int             hsb_height = resize ? 0 : gg->hsb_height;
    Arg             arg[5];
    Dimension       save_width = ctx->core.width;
    Dimension       save_height = ctx->core.height;
    TextLocation    currentDP;
    TextEditPart *  text = &ctx->textedit;

    ctx->core.width = width;
    ctx->core.height = height;
    
    ComputeSWResizes(ctx, gg, width, height, vsb_width, hsb_height);

    text->linesVisible = PAGE_LINE_HT(ctx);
    text->charsVisible = PAGEWID(ctx) / text->charWidth;
    if(text->linesVisible == 0 	||
	text->charsVisible == 0) 
	return;

    if (save_width != ctx->core.width ||
	save_height != ctx->core.height)
	text->xOffset = 0;

    currentDP = _WrapLocationOfLocation(text->wrapTable, text->displayLocation);
    (void) _AdjustDisplayLocation(ctx, text, text->wrapTable,
				  &currentDP, text->linesVisible);

    width = gg->bbc_width = ctx->core.width;
    height = gg->bbc_height = ctx->core.height;

    ctx->core.width = save_width;
    ctx->core.height = save_height;

    if (resize) {
	if (gg->force_vsb)
	    width += gg->vsb_width;
	if (gg->force_hsb)
	    height += gg->hsb_height;
	XtSetArg(arg[0], XtNwidth, 2 * gg->bb_border_width + width);
	XtSetArg(arg[1], XtNheight, 2 * gg->bb_border_width + height);
	XtSetValues((Widget) sw, arg, 2);
	OlDnDWidgetConfiguredInHier((Widget) sw);
    }
}				/* end of ResizeScrolledWindow */
/*
 * SetScrolledWindowScrollbars
 *
 */

static void
SetScrolledWindowScrollbars(TextEditWidget ctx, Dimension width, Dimension height)
{
    TextEditPart   *text      = &ctx->textedit;
    TextBuffer *    textBuffer= text->textBuffer;
    WrapTable *	    wrapTable = text->wrapTable;
    Arg             arg[10];
    int             pagewid = width - PAGE_L_GAP(ctx) - PAGE_R_GAP(ctx);
    int             proportionLength;
    int		    sliderMax = 0;
    int             position = _PositionOfWrapLocation(textBuffer, wrapTable,
	            _WrapLocationOfLocation(wrapTable, text->displayLocation));
    int		    reregister_callback = FALSE;
    ScrolledWindowWidget sw;

    if(text->linesVisible == 0 	||
	text->charsVisible == 0)
	return;

    if (text->need_vsb) {
	TextLine	line;
	TextLocation	loc =
	    _WrapLocationOfLocation(wrapTable, text->displayLocation);

	if (text->text_format == OL_SB_STR_REP) {
	    sliderMax = PositionOfLine(textBuffer,
			(line = LastTextBufferLine(textBuffer)));
	} else {
	    sliderMax = OlPositionOfLine((OlTextBufferPtr) textBuffer, 
			 (line = OlLastTextBufferLine((OlTextBufferPtr) textBuffer)));
	}
	if (wrapTable->cur_end < line) {
	    _UpdateWrapTable(ctx, line, line);
	    sliderMax +=
		wrapTable->contents->p[line]->p[wrapTable->contents->p[line]->used-1];
	    text->lineCount -= wrapTable->contents->p[line]->used;
	    FreeBuffer((Buffer *) wrapTable->contents->p[line]);
	    wrapTable->contents->p[line] = NULL;
	} else
	    sliderMax +=
		wrapTable->contents->p[line]->p[wrapTable->contents->p[line]->used-1];

	loc = _IncrementWrapLocation(wrapTable, loc, text->linesVisible,
				     _LastWrapLine(wrapTable), NULL);
	proportionLength = _PositionOfWrapLocation(textBuffer, wrapTable, loc)
	                   - text->displayPosition + 1;
	sliderMax += proportionLength;
 
        /*
         *  Rollover CTE# 500288; BID# 1190728
         *  Remove the VSBCallback callback here since we are doing
         *  the incrememnting ourselves.  This only needs to take place
         *  when useSetValCallback is True.
         *
         *  The situation this fixes is that when useSetValCallback == True,
         *  then all callbacks registered to the XtNsliderMoved callback
         *  resources will get activated.  Since the TextEdit widget does
         *  the scrolling here, the normally registered VSBCallback would
         *  also get activated, and that scrolls the TextEdit window troo
         *  many times.
         */
        if (XtIsSubclass(XtParent(XtParent(ctx)), scrolledWindowWidgetClass)) {
            sw = (ScrolledWindowWidget) XtParent(XtParent(ctx));
 
            XtRemoveCallback((Widget) sw, XtNvSliderMoved, VSBCallback,
                (XtPointer) ctx);
 
            reregister_callback = TRUE;
        }

	XtSetArg(arg[0], XtNsliderMax, sliderMax);
	XtSetArg(arg[1], XtNproportionLength, proportionLength);
	XtSetArg(arg[2], XtNsliderValue, position);
	XtSetValues(ctx->textedit.vsb, arg, 3);
        if(reregister_callback) {
            XtAddCallback((Widget) sw, XtNvSliderMoved, VSBCallback,
                (XtPointer) ctx);
 
            reregister_callback = FALSE;
        }
	wrapTable->proportion = proportionLength;
    }
    if (text->need_hsb) {
	proportionLength = MIN(pagewid, ctx->textedit.maxX);

	if (text->xOffset && (text->maxX + text->xOffset < proportionLength))
	    text->xOffset = proportionLength - text->maxX;

	XtSetArg(arg[0], XtNsliderMax, ctx->textedit.maxX);
	XtSetArg(arg[1], XtNproportionLength, proportionLength);
	XtSetArg(arg[2], XtNsliderValue, -ctx->textedit.xOffset);
	XtSetValues(ctx->textedit.hsb, arg, 3);
    }
}				/* end of SetScrolledWindowScrollbars */

/*
 * _OLSetScrolledWindow_HScrollbars:  update Horizontal scrollbars.
 * ESC#502991,BID#1222062
 */
void
_OLSetScrolledWindow_HScrollbars(TextEditWidget ctx, Dimension width, Dimension height)
{
	TextEditPart   *text      = &ctx->textedit;
	TextBuffer *    textBuffer= text->textBuffer;
	Arg             arg[10];
	int             pagewid = width - PAGE_L_GAP(ctx) - PAGE_R_GAP(ctx);
	int             proportionLength;

	if(text->linesVisible == 0        ||
	  text->charsVisible == 0)
	  return;

	if (text->need_hsb) {
	  proportionLength = MIN(pagewid, ctx->textedit.maxX);

	  if (text->xOffset && (text->maxX + text->xOffset < proportionLength))
		  text->xOffset = proportionLength - text->maxX;

	  XtSetArg(arg[0], XtNsliderMax, ctx->textedit.maxX);
	  XtSetArg(arg[1], XtNproportionLength, proportionLength);
	  XtSetArg(arg[2], XtNsliderValue, -ctx->textedit.xOffset);
	  XtSetValues(ctx->textedit.hsb, arg, 3);
	}
}                             /* end of _OLSetScrolledWindow_HScrollbars */

/*
 * VSBCallback
 *
 */

static void
VSBCallback(Widget w, XtPointer client_data, XtPointer call_data)
{
    TextEditWidget      ctx = (TextEditWidget) client_data;
    OlScrollbarVerify * p = (OlScrollbarVerify *) call_data;
    TextPosition        act_del;

    if (p->ok) {
    	if(ctx->textedit.linesVisible == 0	||
		ctx->textedit.charsVisible == 0	)
		return;
	act_del = _ScrollDisplayByTextPosDelta(ctx, p->delta);
	if (act_del != p->delta)
	    p->new_location += (act_del - p->delta);
	if (p->new_location < p->slidermin)
	    p->new_location = p->slidermin;
	else {
	    int	prop_len = ctx->textedit.wrapTable->proportion;
	
	    if (p->new_location > p->slidermax-prop_len)
		p->new_location = p->slidermax-prop_len;
	}
    }
}				/* end of VSBCallback */
/*
 * HSBCallback
 *
 */

static void
HSBCallback(Widget w, XtPointer client_data, XtPointer call_data)
{
    TextEditWidget  ctx = (TextEditWidget) client_data;
    OlScrollbarVerify *p = (OlScrollbarVerify *) call_data;

    if(ctx->textedit.linesVisible == 0	||
	ctx->textedit.charsVisible == 0	)
	return;

    if (p->ok)
	_MoveDisplayLaterally(ctx, &ctx->textedit, p->delta, FALSE);

}				/* end of HSBCallback */
/*
 * ClassInitialize
 *
 * This procedure registers the converters used by TextEdit widget.
 *
 */

static void
ClassInitialize(void)
{

    XtSetTypeConverter(XtRString, XtROlWrapMode, WrapModeConverter,
    (XtConvertArgList) NULL, (Cardinal) 0, XtCacheNone, (XtDestructor) NULL);
    XtSetTypeConverter(XtRString, XtROlSourceType, SourceTypeConverter,
    (XtConvertArgList) NULL, (Cardinal) 0, XtCacheNone, (XtDestructor) NULL);
    _OlAddOlDefineType("grow_off", OL_GROW_OFF);
    _OlAddOlDefineType("grow_horizontal", OL_GROW_HORIZONTAL);
    _OlAddOlDefineType("grow_vertical", OL_GROW_VERTICAL);
    _OlAddOlDefineType("grow_both", OL_GROW_BOTH);
    _OlAddOlDefineType("wrapoff", OL_WRAP_OFF);
}				/* end of ClassInitialize */

/*
 * ClassPartInitialize
 *
 * This routine allows inheritance of the TextEdit class procedure
*/

static void
ClassPartInitialize(WidgetClass class)
{
#ifdef SHARELIB
    void          **__libXol__XtInherit = _libXol__XtInherit;
#undef _XtInherit
#define _XtInherit		(*__libXol__XtInherit)
#endif

    OlClassSearchTextDB(class);

    return;
}

/*
 * Registers an input source for the textedit widget while keeping track of
 * string representations:
 *
 * called by:	Initialize(), SetValues()
 */
static void
ProcessSource(TextEditWidget ctx)
{
    OlStrRep        str_rep;
    TextEditPart   *text = &(ctx->textedit);

    str_rep = ctx->textedit.text_format;

    switch (str_rep) {
    case OL_SB_STR_REP:
	switch (text->sourceType) {
	case OL_DISK_SOURCE:
	    text->textBuffer =
		ReadFileIntoTextBuffer((char *)text->source, 
			(TextUpdateFunction)UpdateDisplay, (XtPointer) ctx);
	    if (text->textBuffer == NULL)
		text->textBuffer = ReadStringIntoTextBuffer("", 
				(TextUpdateFunction)UpdateDisplay,
							    (XtPointer) ctx);

	    break;
	case OL_TEXT_BUFFER_SOURCE:
	    text->textBuffer = (TextBuffer *) text->source;
	    RegisterTextBufferUpdate(text->textBuffer, 
				(TextUpdateFunction)UpdateDisplay, 
					(XtPointer) ctx);
	    break;
	case OL_OLTEXT_BUFFER_SOURCE:
	    OlError(dgettext(OlMsgsDomain,
			     "OL_OLTEXT_BUFFER_SOURCE not supported with SB string rep. Use OL_TEXT_BUFFER_SOURCE instead."));
	    break;
	case OL_STRING_SOURCE:
	default:
	    text->textBuffer = ReadStringIntoTextBuffer(text->source,
					  (TextUpdateFunction) UpdateDisplay, 
							(XtPointer) ctx);
	    break;

	}
	break;
    case OL_MB_STR_REP:
    case OL_WC_STR_REP:
	switch (text->sourceType) {
	case OL_DISK_SOURCE:
	    text->textBuffer = (TextBuffer *)
		OlReadFileIntoTextBuffer(str_rep, text->source, 
				(TextUpdateFunction)UpdateDisplay, 
							(XtPointer) ctx);
	    if (text->textBuffer == NULL)
		text->textBuffer = (TextBuffer *)
		    OlReadStringIntoTextBuffer(str_rep, 
			(str_rep == OL_WC_STR_REP ? (OlStr)L"": (OlStr)""), 
					(TextUpdateFunction)UpdateDisplay, 
						(XtPointer) ctx);
	    break;
	case OL_TEXT_BUFFER_SOURCE:
	    OlError(dgettext(OlMsgsDomain,
			     "OL_TEXT_BUFFER_SOURCE not supported with MB or WC string rep. Use OL_OLTEXT_BUFFER_SOURCE instead."));
	    break;
	case OL_OLTEXT_BUFFER_SOURCE:
	    text->textBuffer = (TextBuffer *) text->source;
	    OlRegisterTextBufferUpdate((OlTextBufferPtr) text->textBuffer, 
				(TextUpdateFunction)UpdateDisplay, 
					(XtPointer) ctx);
	    break;
	case OL_STRING_SOURCE:
	default:
	    text->textBuffer = (TextBuffer *)
		OlReadStringIntoTextBuffer(str_rep, text->source, 
			(TextUpdateFunction)UpdateDisplay, (XtPointer) ctx);
	    break;
	}
    }
}

static void
InitMenuLabels(Widget request, Widget new)
{
    TextEditWidget  ctx = (TextEditWidget) new;
    TextEditPart   *text = &(ctx->textedit);
    OlStrRep 	    tf = ctx->primitive.text_format;
    OlStr (*StrCpy)(OlStr s1, OlStr s2) = 
			str_methods[tf].StrCpy;
    int (*StrNumBytes)(OlStr s) =
			str_methods[tf].StrNumBytes;

	/* Make a personal copy of the Menu labels */
	text->menuTitle = StrCpy((OlStr)XtMalloc(StrNumBytes(
					text->menuTitle) ),
					text->menuTitle);
	text->undoLabel = StrCpy((OlStr)XtMalloc(StrNumBytes(
					text->undoLabel) ),
					text->undoLabel);
	text->cutLabel = StrCpy((OlStr)XtMalloc(StrNumBytes(
					text->cutLabel) ),
					text->cutLabel);
	text->copyLabel = StrCpy((OlStr)XtMalloc(StrNumBytes(
					text->copyLabel) ),
					text->copyLabel);
	text->pasteLabel = StrCpy((OlStr)XtMalloc(StrNumBytes(
					text->pasteLabel) ),
					text->pasteLabel);
	text->deleteLabel = StrCpy((OlStr)XtMalloc(StrNumBytes(
					text->deleteLabel) ),
					text->deleteLabel);
}
	

/*
 *
 * Initialize
 *
 *
 */

/* ARGSUSED */
static void
Initialize(Widget request, Widget new, ArgList args, Cardinal *num_args)
{
    TextEditWidget  ctx = (TextEditWidget) new;
    TextEditPart   *text = &(ctx->textedit);
    OlFont          fs;
    Dimension       width;
    Dimension       height;
    Arg             arg[5];
    Arg             icarg[5];
    int			i =0;
    XPoint	    spot =  {0, 0};
    OlInputMethodID	input_method = NULL;
    XIMCallback preeditStart, preeditDone, preeditDraw, preeditCaret;

    text->text_format = ctx->primitive.text_format;
				/* this is to eliminate unnecessary    */
				/* access to the TextEdit structure in */
				/* routines which just use the         */
				/* TextEditPart structure              */

    InitMenuLabels(request, new);
    if (text->linesVisible <= 0)
	text->linesVisible = 1;
    if (text->charsVisible <= 0)
	text->charsVisible = 1;

    fs = ctx->primitive.font;

    ProcessSource(ctx);
    
    text->source = (char *) text->textBuffer;

    ValidateGrowMode(text);
    ValidatePositions(text, True);
    ValidateMargins(ctx, text);

    text->clip_contents = (OlStr)NULL;
    text->dnd_contents  = (OlStr)NULL;
    text->shouldBlink   = TRUE;
    text->blink_timer   = (XtIntervalId) NULL;
    text->lineHeight    = FONTHT(ctx) + 0; /* this is the place where we need to
						make changes for line spacing */
    text->charWidth     = ENSPACE(ctx);
    text->xOffset       = 0;
    text->maxX          = 0;
    text->vsb           = (Widget) NULL;
    text->hsb           = (Widget) NULL;
    text->need_vsb      = FALSE;
    text->need_hsb      = FALSE;
    text->save_offset   = -1;
    text->selectMode    = 0;
    text->wrapTable     = (WrapTable *) NULL;
    text->mask          = 0;
    text->dynamic       = 0;
    text->CursorIn      = (Pixmap) 0;
    text->CursorOut     = (Pixmap) 0;
    text->cursor_state  = OlCursorOff;
    text->cursor_visible = FALSE;
    text->updateState   = TRUE;
    text->DT            = (DisplayTable *) NULL;
    text->DTsize        = 0;
    text->anchor        = -1;
    text->prev_width    =
    text->prev_height   = 0;
    text->hadFocus      = False;
    text->feedbackGCs   = NULL;
    text->pre_edit_feedback   = NULL;
    text->ic_id = NULL; 
    text->pre_edit 	= False;
    text->pre_edit_start =
    text->pre_edit_end   = 0;
    text->wrap_work_proc_id = (XtWorkProcId) NULL;

    width = text->charWidth * text->charsVisible +
	PAGE_L_GAP(ctx) + PAGE_R_GAP(ctx);
    height = text->lineHeight * text->linesVisible +
	PAGE_T_GAP(ctx) + PAGE_B_GAP(ctx);

    if (ctx->core.width == 0)
	ctx->core.width = MAX(ctx->core.width, width);
    if (ctx->core.height == 0)
	ctx->core.height = MAX(ctx->core.height, height);

    if (XtIsSubclass(XtParent(ctx), scrolledWindowWidgetClass)) {
	ScrolledWindowWidget sw = (ScrolledWindowWidget) XtParent(ctx);
	Dimension       vwidth = sw->scrolled_window.view_width;
	Dimension       vheight = sw->scrolled_window.view_height;
	OlSWGeometries  geometries;
	geometries = GetOlSWGeometries(sw);

	ctx->core.border_width = 0;

	XtSetArg(arg[0], XtNcomputeGeometries, ScrolledWindowInterface);
	XtSetArg(arg[1], XtNvAutoScroll, FALSE);
	XtSetArg(arg[2], XtNhAutoScroll, FALSE);
	XtSetValues((Widget) sw, arg, 3);

	XtAddCallback((Widget) sw, XtNvSliderMoved, VSBCallback, (XtPointer) ctx);
	XtAddCallback((Widget) sw, XtNhSliderMoved, HSBCallback, (XtPointer) ctx);

	text->vsb = (Widget) geometries.vsb;
	text->hsb = (Widget) geometries.hsb;

	ResizeScrolledWindow(ctx, sw, &geometries,
			     (vwidth > 0) ? vwidth : ctx->core.width,
			  (vheight > 0) ? vheight : ctx->core.height, True);

	XtSetArg(arg[0], XtNgranularity, 1);
	XtSetArg(arg[1], XtNsliderMin, 0);
	XtSetValues(text->vsb, arg, 2);

	XtSetArg(arg[0], XtNgranularity, text->charWidth);
	XtSetArg(arg[1], XtNsliderMin, 0);
	XtSetValues(text->hsb, arg, 2);
	SetScrolledWindowScrollbars(ctx, ctx->core.width, ctx->core.height);
    } else {
	if (XtGeometryAlmost == XtMakeResizeRequest((Widget) ctx,
					  ctx->core.width, ctx->core.height,
				       &ctx->core.width, &ctx->core.height))
	    XtMakeResizeRequest((Widget) ctx, ctx->core.width, ctx->core.height, NULL, NULL);
	_BuildWrapTable(ctx);
    }

    text->linesVisible = text->lineHeight && FONTHT(ctx) ?
	PAGEHT(ctx) / text->lineHeight : 0; /* need new macro */


    text->dropsiteid = (OlDnDDropSiteID) NULL;
    text->transient = (Atom) NULL;

    if(text->text_format != OL_SB_STR_REP) {

	if(text->pre_edit_style == OL_OVER_THE_SPOT) {
    		i =0;
    		XtSetArg(icarg[i],XNFontSet,fs); i++;
    		XtSetArg(icarg[i],XNSpotLocation,&spot); i++;
		XtSetArg(icarg[i],XNBackground,ctx->core.background_pixel); i++;
		XtSetArg(icarg[i],XNForeground,ctx->primitive.font_color); i++;
	} else if(text->pre_edit_style == OL_ON_THE_SPOT) {
		preeditStart.client_data = (XPointer)ctx;
        	preeditStart.callback = (XIMProc)PreeditStartCallbackFunc;
        	preeditDone.client_data = (XPointer)ctx;
        	preeditDone.callback = (XIMProc)PreeditEndCallbackFunc;
        	preeditDraw.client_data = (XPointer) ctx;
        	preeditDraw.callback = (XIMProc)PreeditDrawCallbackFunc;
        	preeditCaret.client_data = (XPointer) ctx;
        	preeditCaret.callback = (XIMProc)PreeditCaretCallbackFunc;

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
    		text->ic_id = OlCreateIC(input_method,new,
					text->pre_edit_style,
					icarg,
					i);
		if (text->ic_id == (OlInputContextID) NULL &&
			text->editMode != OL_TEXT_READ) {
	    		char *msg;
	    		String format;

				if (msg = malloc(1024)) {
					format = dgettext(OlMsgsDomain, 
						"%s: Preedit will be disabled (No InputStyle).");
					snprintf(msg, 1024, format, ctx->core.name);
					OlWarning(msg);
					free(msg);
				}
		}

	} else {
		if(text->editMode != OL_TEXT_READ) {
			char *msg;
			String format;
			
			if (msg = malloc(1024)) {
				format = dgettext(OlMsgsDomain,
					"%s: Preedit will be disabled (No InputMethod).");
				snprintf(msg, 1024, format, ctx->core.name);
				OlWarning(msg);
				free(msg);
			}
		}
	}

     }
}				/* end of Initialize */
/*
 * Redisplay
 *
 * This routine processes all "expose region" XEvents. In general, its
 * job is to the best job at minimal re-paint of the text, displayed in
 * the window, that it can.
 *
 */

static void
Redisplay(Widget w, XEvent *event, Region region)
{
    TextEditWidget  ctx = (TextEditWidget) w;
    TextEditPart   *text = &ctx->textedit;
    XRectangle      rect;

    if (region != (Region) NULL) {
	XClipBox(region, &rect);
    } else {
	switch (event->type) {
	case Expose:
	    rect.x = event->xexpose.x;
	    rect.y = event->xexpose.y;
	    rect.width = event->xexpose.width;
	    rect.height = event->xexpose.height;
	    break;
	case GraphicsExpose:
	    rect.x = event->xgraphicsexpose.x;
	    rect.y = event->xgraphicsexpose.y;
	    rect.width = event->xgraphicsexpose.width;
	    rect.height = event->xgraphicsexpose.height;
	    break;
	case NoExpose:
	default:
	    return;
	}
    }


    if (text->cursor_state == OlCursorOn) {
	    XSetClipRectangles(XtDisplay(ctx), text->insgc, 0, 0, 
						&rect, 1, Unsorted);

 	/* Make sure the window is clear; 
	 a blinking cursor can be drawn 
         due to some race condition between 
         calliing of the _BlinkCursor timer proc,
	 calling of this Expose method, 
	  and delivery of Expose event on the event queue 
	  and actual exposure of window */
 
 	XClearArea(XtDisplay(w),XtWindow(w),rect.x,rect.y,rect.width,
 					rect.height, False);

 	/* The following call to _TurnTextCursorOff
		 call actually turns the cursor on. Why? 
 		Because the window is clear and a xor gc
 		is used in _TurnTextCursorOff */
 
	_TurnTextCursorOff(ctx);
	text->cursor_state = OlCursorOn;
	XSetClipMask(XtDisplay(ctx), text->insgc, None);
    }

    _DisplayText(ctx, &rect);
}				/* end of Redisplay */

/*
 * AckDel
 *
 */
static void
AckDel(Widget w, XtPointer client_data, Atom *selection, Atom *type, XtPointer value, long unsigned int *length, int *format)
{
    TextEditWidget  ctx = (TextEditWidget) w;

    if ((Boolean) client_data) {
	OlDnDDragNDropDone((Widget) ctx, *selection,
			   XtLastTimestampProcessed(
						    XtDisplay((Widget) ctx)),
			   NULL, NULL);
    }
}

typedef struct _drop_closure {
    TextPosition    		position;
    OlDnDTriggerOperation 	operation;
    Boolean         		send_done;
} DropClosure, *DropClosurePtr;

typedef struct _selectTargetRec {
	DropClosurePtr drop;
	Time		timestamp;
} SelectTargetRec, *SelectTargetPtr;	

/*
 * TriggerNotify
 *
 */
static void 
TriggerNotify(Widget			widget,
	      Window			window,
	      Position			x,
	      Position			y,
	      Atom			selection,
	      Time			timestamp,
	      OlDnDDropSiteID		drop_site,
	      OlDnDTriggerOperation	operation,
	      Boolean			send_done,
	      Boolean			forwarded,
	      XtPointer			closure)
{
    DropClosurePtr  drop; 
    SelectTargetPtr select;
    Window          dummy;
    int             lx, ly;


    if (!XtIsSensitive(widget)) {
                if (send_done)
                        OlDnDDragNDropDone(widget, selection,
                           XtLastTimestampProcessed(
                                XtDisplay(widget)),
                           NULL, NULL);
                return ;
     }
  
    if(((TextEditWidget)widget)->textedit.editMode != OL_TEXT_EDIT) {
                _OlBeepDisplay(widget,1);
                if (send_done)
                        OlDnDDragNDropDone(widget, selection,
                           XtLastTimestampProcessed(
                                XtDisplay(widget)),
                           NULL, NULL);
                return;
    }

    drop = (DropClosurePtr)XtMalloc(sizeof(DropClosure)); 
    select = (SelectTargetPtr)XtMalloc(sizeof(SelectTargetRec));

    XTranslateCoordinates(XtDisplay(widget),
			  RootWindowOfScreen(XtScreen(widget)),
			  XtWindow(widget), (int) x, (int) y,
			  &lx, &ly, &dummy);
    drop->position = _PositionFromXY((TextEditWidget) widget, lx, ly, OL_CARET);
    drop->operation = operation;
    drop->send_done = send_done;

    select->drop = drop;
    select->timestamp = timestamp;

    XtGetSelectionValue(widget, selection, OlInternAtom(XtDisplay(widget),
				TARGETS_NAME), SelectTarget, 
				(XtPointer) select, timestamp);
}

/*
 * Realize
 *
 * This procedure creates the window for the Text widget, and defines
 * the cursor.
 *
 */

static void
_unrealize 
(Widget w, XtPointer clientd, XtPointer calld)
{
    TextEditWidget  ctx = (TextEditWidget) w;

    if (ctx->textedit.dropsiteid != (OlDnDDropSiteID) NULL) {
#if	0
	OlDnDDestroyDropSite(ctx->textedit.dropsiteid);
	/* this is done automatically by the DnD code */
#endif
	ctx->textedit.dropsiteid = (OlDnDDropSiteID) NULL;
    }
}

static void
Realize(Widget w, Mask *valueMask, XSetWindowAttributes *attributes)
{
    TextEditWidget ctx = (TextEditWidget)w;
/* DynamicInitialize(ctx); */
    GetGCs(ctx, &ctx->textedit);

    *valueMask |= CWBitGravity;
    *valueMask |= CWBackPixel;
    attributes->bit_gravity = NorthWestGravity;
    attributes->background_pixel = ctx->core.background_pixel;

    XtCreateWindow((Widget) ctx, InputOutput, (Visual *) CopyFromParent,
		   *valueMask, attributes);

    XDefineCursor(XtDisplay((Widget) ctx),
		  XtWindow((Widget) ctx),
		  OlGetStandardCursor((Widget) ctx));

    XGrabButton(XtDisplay(ctx),
	Button1Mask | Button2Mask | Button3Mask | Button4Mask | Button5Mask,
		AnyModifier,
		XtWindow(ctx), False,
		ButtonPressMask | ButtonReleaseMask | ButtonMotionMask,
		GrabModeAsync, GrabModeAsync, None, None);

    if (ctx->textedit.vsb != NULL) {	/* must be in a scrolled window */
	Arg             arg[1];
	XtSetArg(arg[0], XtNbackground, ctx->core.background_pixel);
	XtSetValues(XtParent(ctx), arg, 1);
    } else
	_BuildWrapTable(ctx);

    {
	OlDnDSiteRect   rect;

	rect.x = rect.y = 0;
	rect.width = ctx->core.width;
	rect.height = ctx->core.height;

	ctx->textedit.dropsiteid =
	    OlDnDRegisterWidgetDropSite((Widget) ctx,
					XtIsSensitive((Widget)ctx) &&
					(ctx->textedit.editMode == 
					 OL_TEXT_EDIT)	?
					OlDnDSitePreviewNone :
					OlDnDSitePreviewInsensitive,
					&rect, 1, TriggerNotify,
					NULL, (Boolean) True, NULL);
	XtAddCallback((Widget) ctx, XtNunrealizeCallback,
		      _unrealize, (XtPointer) NULL);
    }
}				/* end of Realize */

/*
 * RegisterFocus
 *
 */
static          Widget
RegisterFocus(Widget w)
{
    OlRegisterFocusFunc focus_func =
    ((TextEditWidget) w)->textedit.register_focus;

    return ((focus_func == NULL) ? w : (*focus_func) (w));
}

/*
 * Resize
 *
 */

static void
Resize(Widget w)
{
    TextEditWidget  ctx = (TextEditWidget) w;

    if (XtIsRealized((Widget) ctx)) {
	_BuildWrapTable(ctx);
	SetScrolledWindowScrollbars(ctx, ctx->core.width, ctx->core.height);
	if (ctx->textedit.updateState) {
	    XRectangle      rect;
	    OlDnDSiteRect   dsrect;
	    rect.x = 0;
	    rect.y = 0;
	    dsrect.x = dsrect.y = 0;
	    dsrect.width = rect.width = ctx->core.width;
	    dsrect.height = rect.height = ctx->core.height;
	    _TurnTextCursorOff(ctx);
	    XClearArea(XtDisplay(ctx), XtWindow(ctx), 0, 0, 0, 0, False);
	    _DisplayText(ctx, &rect);
	    OlDnDUpdateDropSiteGeometry(ctx->textedit.dropsiteid, &dsrect, 1);
	}
    }
} /* end of Resize */

static void
CheckMenuLabels(Widget current, Widget request, Widget new)
{
    TextEditWidget  ntw = (TextEditWidget) new;
    TextEditWidget  ctw = (TextEditWidget) current;
    TextEditPart   *text = &(ntw->textedit);
    TextEditPart   *ctext = &(ctw->textedit);
    OlStrRep 	    tf = ntw->primitive.text_format;
    OlStr (*StrCpy)(OlStr s1, OlStr s2) = 
			str_methods[tf].StrCpy;
    int (*StrNumBytes)(OlStr s) =
			str_methods[tf].StrNumBytes;

	if(text->menuTitle != NULL && text->menuTitle !=
			ctext->menuTitle) 	{
		if(ctext->menuTitle != NULL)
			XtFree((char *)ctext->menuTitle);
		text->menuTitle = StrCpy((OlStr)XtMalloc(StrNumBytes(
					text->menuTitle) ),
					text->menuTitle);
	}

	if(text->undoLabel != NULL && text->undoLabel !=
			ctext->undoLabel) 	{
		if(ctext->undoLabel != NULL)
			XtFree((char *)ctext->undoLabel);
		text->undoLabel = StrCpy((OlStr)XtMalloc(StrNumBytes(
					text->undoLabel) ),
					text->undoLabel);
	}

	if(text->cutLabel != NULL && text->cutLabel !=
			ctext->cutLabel) 	{
		if(ctext->cutLabel != NULL)
			XtFree((char *)ctext->cutLabel);
		text->cutLabel = StrCpy((OlStr)XtMalloc(StrNumBytes(
					text->cutLabel) ),
					text->cutLabel);
	}

	if(text->copyLabel != NULL && text->copyLabel !=
			ctext->copyLabel) 	{
		if(ctext->copyLabel != NULL)
			XtFree((char *)ctext->copyLabel);
		text->copyLabel = StrCpy((OlStr)XtMalloc(StrNumBytes(
					text->copyLabel) ),
					text->copyLabel);
	}

	if(text->pasteLabel != NULL && text->pasteLabel !=
			ctext->pasteLabel) 	{
		if(ctext->pasteLabel != NULL)
			XtFree((char *)ctext->pasteLabel);
		text->pasteLabel = StrCpy((OlStr)XtMalloc(StrNumBytes(
					text->pasteLabel) ),
					text->pasteLabel);
	}

	if(text->deleteLabel != NULL && text->deleteLabel !=
			ctext->deleteLabel) 	{
		if(ctext->deleteLabel != NULL)
			XtFree((char *)ctext->deleteLabel);
		text->deleteLabel = StrCpy((OlStr)XtMalloc(StrNumBytes(
					text->deleteLabel) ),
					text->deleteLabel);
	}

}

/*
 * SetValues
 *
 */
/* ARGSUSED */
static Boolean
SetValues(
	  Widget        current,
	  Widget        request,
	  Widget        new,
	  ArgList       args,
	  Cardinal *	num_args)
{
    TextEditWidget  oldtw = (TextEditWidget) current;
    TextEditWidget  newtw = (TextEditWidget) new;
    TextEditPart   *oldtext = &(oldtw->textedit);
    TextEditPart   *newtext = &(newtw->textedit);
    Boolean         realized = XtIsRealized(current);
    Boolean         redisplay = FALSE;
    Boolean         source_changed = FALSE;
    OlCursorState   save_cursorState = oldtext->cursor_state;
    Arg             arg[9];

    /* Do implcit commit if neccessary */
    if(newtext->pre_edit)	
	if(newtext->cursorPosition != oldtext->cursorPosition	||
 	oldtext->displayPosition   != newtext->displayPosition	||
	newtext->selectEnd	   != oldtext->selectEnd	||
	newtext->selectStart	   != oldtext->selectStart	||
 	XtIsSensitive(current)	   != XtIsSensitive(new)	||
 	oldtext->editMode          != newtext->editMode)	{
	 	Boolean save_updateState = newtext->updateState;

 		newtext->updateState = FALSE;
		if(!XtIsSensitive(new) 		||	
			newtext->editMode == OL_TEXT_READ) 
				_DoImplicitCommit(newtw, NULL, FALSE);
		else if(newtext->pre_edit_end >
					newtext->pre_edit_start) 
				_DoImplicitCommit(newtw, NULL, TRUE);
		newtext->updateState = save_updateState;
	}
     

    _TurnTextCursorOff(oldtw);

    CheckMenuLabels(current, request, new);
/*
 * Handle resources which affect GCs
 */
    
    if (oldtw->core.background_pixel       != newtw->core.background_pixel       ||
	oldtw->primitive.font_color        != newtw->primitive.font_color        ||
	oldtw->primitive.input_focus_color != newtw->primitive.input_focus_color ||
	oldtw->primitive.font              != newtw->primitive.font 		 ||
        XtIsSensitive(current) != XtIsSensitive(new) ||
	oldtw->textedit.editMode != newtw->textedit.editMode)
    {
	XtReleaseGC(new, newtext->gc);
	XtReleaseGC(new, newtext->invgc);
	XtReleaseGC(new, newtext->insgc);
	GetGCs(newtw, newtext);
	newtext->lineHeight = FONTHT(newtw) + 0;

	if (((XtIsSensitive(current) != XtIsSensitive(new)) ||
		(oldtw->textedit.editMode != newtw->textedit.editMode))
							&& realized ) {
      		OlDnDSitePreviewHints preview_hints;

		/* At the time this code is called, we have already lost focus,
		 * however since we are insensitive we are not going to
		 * receive FocusOut events. Therefore, we set these 
		 * values explicitly.
		 */
		if(XtIsSensitive(new) != XtIsSensitive(current)) {
			newtext->hadFocus = FALSE;
			newtw->primitive.has_focus = FALSE;
		}

      		OlDnDQueryDropSiteInfo(newtw->textedit.dropsiteid, (Widget *)NULL,
    		(Window *)NULL, &preview_hints, (OlDnDSiteRectPtr *)NULL,
                (unsigned int *)NULL, (Boolean *)NULL);
      	if (XtIsSensitive(new) && (newtw->textedit.editMode == OL_TEXT_EDIT))
                preview_hints &= ~OlDnDSitePreviewInsensitive;
      	else
                preview_hints |= OlDnDSitePreviewInsensitive;
      	OlDnDChangeDropSitePreviewHints(newtw->textedit.dropsiteid,
                                        preview_hints);
   	}

	if ((oldtw->primitive.font != newtw->primitive.font) &&
	    (newtext->hsb          != NULL))
	{
	    newtext->charWidth = ENSPACE(newtw);
	    XtSetArg(arg[0], XtNgranularity, newtext->charWidth);
	    XtSetValues(newtext->hsb, arg, 1);
	}
	if ((oldtw->core.background_pixel != newtw->core.background_pixel) &&
	    (newtw->textedit.vsb          != NULL))
	{
	    XtSetArg(arg[0], XtNbackground, newtw->core.background_pixel);
	    XtSetValues(XtParent(newtw), arg, 1);
	}
    }
    if ((newtext->sourceType != oldtext->sourceType) ||
	(newtext->source     != oldtext->source))
    {
	if (newtw->textedit.text_format == OL_SB_STR_REP)
	    FreeTextBuffer(newtext->textBuffer, 
			(TextUpdateFunction)UpdateDisplay, (XtPointer)newtw);
	else
	    OlFreeTextBuffer((OlTextBufferPtr)newtext->textBuffer,
			     (TextUpdateFunction)UpdateDisplay, 
						(XtPointer)newtw);
	ProcessSource(newtw);
	newtext->source = (char *) newtext->textBuffer;
	newtext->xOffset = 0;
	newtext->selectMode = 0;
	redisplay = TRUE;
	source_changed = True;
    } else {
	if ((oldtext->leftMargin          != newtext->leftMargin)           ||
	    (oldtext->topMargin           != newtext->topMargin)            ||
	    (oldtext->rightMargin         != newtext->rightMargin)          ||
	    (oldtext->bottomMargin        != newtext->bottomMargin)         ||
	    (oldtext->wrapMode            != newtext->wrapMode)             ||
	    (oldtext->displayPosition     != newtext->displayPosition)      ||
	    (oldtext->cursorPosition      != newtext->cursorPosition)       ||
	    (oldtext->selectStart         != newtext->selectStart)          ||
	    (oldtext->selectEnd           != newtext->selectEnd)            ||
	    (oldtext->growMode            != newtext->growMode)             ||
            (XtIsSensitive((Widget)oldtw) != XtIsSensitive((Widget)newtw))  ||	
	    (oldtw->core.background_pixel != newtw->core.background_pixel)  ||
	    (oldtext->editMode 		  != newtext->editMode)	            ||
	    (oldtw->primitive.font        != newtw->primitive.font)         ||
	    (oldtw->primitive.font_color  != newtw->primitive.font_color))
	{
	    newtext->selectMode = 0;
	    redisplay = TRUE;
	    if (oldtext->wrapMode != newtext->wrapMode)
		newtext->xOffset = 0;
	}
    }

    if (redisplay) {
	Boolean         save_updateState = newtext->updateState;
	TextLocation    currentDP;

	newtext->prev_width = newtext->prev_height = 0;
	newtext->cursor_state = OlCursorOff;
	newtext->updateState = FALSE;

	ValidateGrowMode(newtext);
	ValidatePositions(newtext, !source_changed);
	ValidateMargins(newtw, newtext);

	if (newtext->vsb != NULL)
	    OlLayoutScrolledWindow((ScrolledWindowWidget)
					 XtParent(XtParent(newtw)), 0);
	else
	    _BuildWrapTable(newtw);

	if ((oldtext->growMode != newtext->growMode) &&
	    (newtext->growMode != OL_GROW_OFF))
	{
	    if (newtext->growMode != OL_GROW_VERTICAL) {
		Dimension       maxWidth = newtext->leftMargin +
		newtext->rightMargin + newtext->maxX;
		if (maxWidth > newtw->core.width)
		    newtw->core.width = maxWidth;
	    }
	    if (newtext->growMode != OL_GROW_HORIZONTAL) {
		Dimension       maxHeight = newtext->lineHeight *
		newtext->lineCount +
		PAGE_T_GAP(newtw) + PAGE_B_GAP(newtw);
		if (maxHeight > newtw->core.height)
		    newtw->core.height = maxHeight;
	    }
	}

	currentDP = _WrapLocationOfLocation(newtext->wrapTable,
					    newtext->displayLocation);
	(void) _AdjustDisplayLocation(newtw, newtext, newtext->wrapTable,
				      &currentDP, LINES_VISIBLE(newtw));
	newtext->save_offset = -1;
	newtext->updateState = save_updateState;
    } else {
	newtext->cursor_state = save_cursorState;
	_TurnTextCursorOff(newtw);
	newtext->cursor_state = save_cursorState;
    }
	

    if(newtext->editMode != oldtext->editMode ||
	XtIsSensitive(new) != XtIsSensitive(current)) {
	if(newtext->editMode == OL_TEXT_READ ||
			!XtIsSensitive(new)) {
		if(newtext->ic_id != (OlInputContextID)NULL)
			OlUnsetFocusIC(newtext->ic_id);
	 	if(HAS_FOCUS(newtw) && 
			newtext->cursor_state == OlCursorBlinkOff) 
			_ChangeTextCursor(newtw);
		else
			if (newtext-> blink_timer != NULL) {
                         XtRemoveTimeOut(newtext-> blink_timer);
                         newtext-> blink_timer = NULL;
                }

	} else if(newtext->editMode 
			== OL_TEXT_EDIT && 
			XtIsSensitive(new) && HAS_FOCUS(newtw)) {
		_TurnCursorBlinkOn(newtw);

		if(newtext->ic_id != (OlInputContextID)NULL) {
			if(OlSetFocusIC(newtext->ic_id) == NULL) {
				char *msg;
	    		String format;

				if (msg = malloc(1024)) {
					format = dgettext(OlMsgsDomain, 
						"%s: Preedit will be disabled (No Focus).");
					snprintf(msg, 1024, format, new->core.name);
					OlWarning(msg);
					free(msg);
				}
				newtext->ic_id = (OlInputContextID)NULL;
			}

		} else if(newtext->text_format != OL_SB_STR_REP) {
			char *msg;
			String format;

			if (msg = malloc(1024)) {
				format = dgettext(OlMsgsDomain, 
					"%s: Preedit will be disabled (No InputStyle or InputMethod).");
	    		snprintf(msg, 1024, format, new->core.name);
	    		OlWarning(msg);
				free(msg);
			}

		}

	  }
     }

    if(oldtext->cursor_visible && !newtext->cursor_visible){
	if(newtext->cursor_state == OlCursorOn) 
		_TurnTextCursorOff(newtw);
	else 
		newtext->cursor_state = OlCursorOff;
    } else if(!oldtext->cursor_visible && newtext->cursor_visible) 
	_MakeTextCursorVisible(newtw);

    return (realized                               &&
	    redisplay                              &&
	    newtext->updateState                   &&
	    newtw->core.width == oldtw->core.width &&
	    newtw->core.height == oldtw->core.height);
} /* end of SetValues */

/*
 * Destroy
 *
 */

static void
Destroy(Widget w)
{
    TextEditWidget ctx = (TextEditWidget)w;
    Display        *dpy = XtDisplay(ctx);
    Window          win = XtWindow(ctx);
    int             i;
	WrapTable *    wrapTable  = ctx->textedit.wrapTable;
    WrapContents *  wrapcon = wrapTable->contents;

    if (ctx->textedit.text_format == OL_SB_STR_REP)
	FreeTextBuffer(ctx->textedit.textBuffer, 
			(TextUpdateFunction)UpdateDisplay, (XtPointer)ctx);
    else
	OlFreeTextBuffer((OlTextBufferPtr)ctx->textedit.textBuffer,
			 (TextUpdateFunction)UpdateDisplay, (XtPointer)ctx);

    for (i = 0; i < wrapTable->cur_start; i++)
		if ((Buffer *) (wrapcon->p[i]) != (Buffer *)NULL)
			FreeBuffer((Buffer *) (wrapcon->p[i]));
    for (i = wrapTable->cur_start; i <= wrapTable->cur_end; i++)
		FreeBuffer((Buffer *) (wrapcon->p[i]));
    for (i = wrapTable->cur_end+1; i <= wrapcon->used; i++)
		if ((Buffer *) (wrapcon->p[i]) != (Buffer *)NULL)
			FreeBuffer((Buffer *) (wrapcon->p[i]));	

    XtFree((char *)wrapcon->p);
    if (wrapcon != (WrapContents *)NULL)
	XtFree((char *)wrapcon);
    if (ctx->textedit.wrapTable != (WrapTable *)NULL)
	XtFree((char *)ctx->textedit.wrapTable);
    if(ctx->textedit.feedbackGCs != NULL)
			XtFree((char *)ctx->textedit.feedbackGCs);
    if(ctx->textedit.pre_edit_feedback != NULL)
		FreeBuffer((Buffer *) ctx->textedit.pre_edit_feedback);

    for (i = 0; i < ctx->textedit.DTsize; i++)
	if (ctx->textedit.DT[i].p)
	    FREE(ctx->textedit.DT[i].p);
    if (ctx->textedit.DT)
	FREE((char *)ctx->textedit.DT);

    if (ctx->textedit.blink_timer != NULL)
	XtRemoveTimeOut(ctx->textedit.blink_timer);

	if (ctx->textedit.wrap_work_proc_id != (XtWorkProcId) NULL)
        XtRemoveWorkProc(ctx->textedit.wrap_work_proc_id);

    XtRemoveAllCallbacks((Widget) ctx, XtNmotionVerification);
    XtRemoveAllCallbacks((Widget) ctx, XtNmodifyVerification);
    XtRemoveAllCallbacks((Widget) ctx, XtNpostModifyNotification);
    XtRemoveAllCallbacks((Widget) ctx, XtNmargin);
    XtRemoveAllCallbacks((Widget) ctx, XtNkeys);
    XtRemoveAllCallbacks((Widget) ctx, XtNbuttons);

    /* If we own the CLIPBOARD contents, give up ownership */
    /* and free the contents. */
    if (win != (Window) NULL && XGetSelectionOwner(dpy, XA_CLIPBOARD(dpy)) == win) {
	XSetSelectionOwner(dpy, XA_CLIPBOARD(dpy),
			   None, _XtLastTimestampProcessed((Widget) ctx));
	if (ctx->textedit.clip_contents)
	    FREE(ctx->textedit.clip_contents);
    }
    if (ctx->textedit.transient != (Atom) NULL &&
	XGetSelectionOwner(dpy, ctx->textedit.transient) == win) {
	OlDnDDisownSelection((Widget) ctx, ctx->textedit.transient,
			     CurrentTime);
	FREE(ctx->textedit.dnd_contents);
    }
    if (ctx->textedit.transient != (Atom) NULL) {
	OlDnDFreeTransientAtom((Widget) ctx, ctx->textedit.transient);
	ctx->textedit.transient = (Atom) NULL;
    }
    
    if (ctx->textedit.CursorIn != (Pixmap)0) {
	XFreePixmap(dpy, ctx->textedit.CursorIn);
	XFreePixmap(dpy, ctx->textedit.CursorOut);
    }
}				/* end of Destroy */

/*
 * FocusHandler
 */
static void
FocusHandler(Widget	w,
	     OlDefine	highlight_type)
{
    TextEditWidget  ctx = (TextEditWidget) w;

    switch ((int) highlight_type) {
    case OL_IN:
	if(ctx->textedit.ic_id != (OlInputContextID)NULL &&
			ctx->textedit.editMode == OL_TEXT_EDIT)
	    if(OlSetFocusIC(ctx->textedit.ic_id) == NULL) {
			char *msg;

			if (msg = malloc(1024)) {
				snprintf(msg, 1024, dgettext(OlMsgsDomain,
					"%s: Preedit will be disabled. (No Focus)"),
								ctx->core.name);
				OlWarning(msg);
				free(msg);
			}
			ctx->textedit.ic_id = (OlInputContextID)NULL;
	    }
	if (HAS_FOCUS(ctx) && (ctx->textedit.hadFocus == False)) {
	    if(!ctx->textedit.cursor_visible) {
		_MakeTextCursorVisible(ctx);
	    } else if(ctx->textedit.editMode == OL_TEXT_EDIT)
		_ChangeTextCursor(ctx);
	    ctx->textedit.hadFocus = True;
	}
	break;
    case OL_OUT:
	if(ctx->textedit.ic_id != (OlInputContextID)NULL)
		OlUnsetFocusIC(ctx->textedit.ic_id);
	if (!HAS_FOCUS(ctx) && (ctx->textedit.hadFocus == True)) {
	    if(ctx->textedit.editMode == OL_TEXT_EDIT) 
	    	_ChangeTextCursor(ctx);
	    ctx->textedit.hadFocus = False;
	}
	break;
    default:
	OlWarning(dgettext(OlMsgsDomain,
			   "TextEdit: Undefined highlight_type!"));
    }

}				/* end of FocusHandler */
/*
 * ConvertClipboardOrDnD
 *	If somebody wants XA_STRING we always give a multi-byte string.
 *	If we are wide_char we convert to multi-byte. We
 *	now support COMPOUND_TEXT too 
 *
 */

static          Boolean
ConvertClipboardOrDnD(Widget w, Atom *selection, Atom *target, Atom *type_return, XtPointer *value_return, long unsigned int *length_return, int *format_return)
{
    TextEditWidget  ctx = (TextEditWidget) w;
    Atom           *atoms = NULL;
    int             i;
    OlStr	    clip_cont = ctx->textedit.clip_contents;
    OlStr	    dnd_cont = ctx->textedit.dnd_contents;
    OlStr	    str = NULL;
    char *          buffer;
    Boolean         retval = False;
    OlStrRep	    rep = ctx->textedit.text_format;

    if (*selection == XA_CLIPBOARD(XtDisplay(ctx)) ||
	*selection == ctx->textedit.transient)
    {
	if (*target == OlInternAtom(XtDisplay(ctx), TARGETS_NAME)) {
	    *format_return = (int) (8 * sizeof(Atom));
	    *length_return = (unsigned long) 5;
	    atoms          = (Atom *) MALLOC((unsigned) 
				((*length_return) * (sizeof(Atom))));
	    atoms[0]       = OlInternAtom(XtDisplay(ctx), TARGETS_NAME);
	    atoms[1]       = XA_STRING;
	    atoms[2]       = OlInternAtom(XtDisplay(ctx), DELETE_NAME);
	    atoms[3]       = OlInternAtom(XtDisplay(ctx), LENGTH_NAME);
	    atoms[4]       = OlInternAtom(XtDisplay(ctx),COMPOUND_TEXT_NAME);
	    *value_return  = (XtPointer) atoms;
	    *type_return   = XA_ATOM;
	    
	    retval         = True;
	} else if (*target == XA_STRING) {
	    size_t size;

            str = (*selection == ctx->textedit.transient ?
					dnd_cont : clip_cont);

	    if(rep != OL_WC_STR_REP) { 
		buffer = XtNewString(((char *)str));
	    } else { /* convert to MB */
		 size = sizeof(char)*MB_CUR_MAX*(wslen((wchar_t *)str)+1);
		 buffer = (char *)XtMalloc(size);
		 (void)wcstombs(buffer, (wchar_t *)str, size);
	    }
	    *format_return = 8;
	    *length_return = strlen((const char *)buffer); 
	    *value_return = (XtPointer)buffer;
	    *type_return   = XA_STRING;
	    retval = True;

	} else if(*target == 
			OlInternAtom(XtDisplay(ctx),COMPOUND_TEXT_NAME)) {   
	    char *      ctbuf;
	    int         ctlen;

            str = (*selection == ctx->textedit.transient ?
					dnd_cont : clip_cont);

	    ctbuf          = str_methods[rep].StrToCT(XtDisplay(w), 
							str, &ctlen);
	    buffer         = XtMalloc(ctlen + 1);
	    memcpy(buffer, ctbuf, ctlen);
	    buffer[ctlen]  = '\0';
	    *format_return = 8;
	    *length_return = ctlen;
	    *value_return  = (XtPointer) buffer;
	    *type_return   = OlInternAtom(XtDisplay(ctx),COMPOUND_TEXT_NAME);

	    retval         = True;
	} else if ((*target == OlInternAtom(XtDisplay(ctx), DELETE_NAME))){ 
		/* delete the selection */
	    if(ctx->textedit.editMode == OL_TEXT_EDIT) 
             	TextEditCheckAndInsert(ctx, "", 0);
	    *format_return = 8;
	    *length_return = NULL;
	    *value_return  = NULL;
	    *type_return   = OlInternAtom(XtDisplay(ctx), DELETE_NAME);
	    retval         = True;
	}

#ifdef sun			/* for supporting target LENGTH_NAME and to suppress */
				/* warnings for targets generated by Xview ... */

	else if (*target == OlInternAtom(XtDisplay(ctx), LENGTH_NAME)) {
	    int *	intbuffer;
	    size_t size;

            str = (*selection == ctx->textedit.transient ?
					dnd_cont : clip_cont);

	    if(rep == OL_WC_STR_REP) { 
		 size = sizeof(char)*MB_CUR_MAX*(wslen((wchar_t *)str)+1);
		 buffer = (char *)XtMalloc(size);
		 size = wcstombs(buffer, (wchar_t *)str, size) + 1;
	    } else
	  	size = strlen((const char *)str) + 1;

	    intbuffer      = (int *)XtMalloc(sizeof(int));
	    *intbuffer     = (int)size;
	    *value_return  = (XtPointer)intbuffer;
	    *length_return = 1;
	    *format_return = sizeof(int)*8;
	    *type_return   = (Atom)*target;
	    
	    retval         = True;
	}
	else {
	    char *          atom;
	    static char     prefix[] = "_SUN_SELN";

	    atom = XGetAtomName(XtDisplay(ctx), *target);
	    if (strncmp(prefix, atom, strlen(prefix)) != 0 &&
		   strcmp("LENGTH_CHARS", atom) != 0)
		OlWarning(dgettext(OlMsgsDomain, "TextEdit: Can't convert"));
	    XFree(atom);
	    retval = False;
	}
	
#else				/* without LENGTH_NAME and warning suppression */
	else {
	    OlWarning(dgettext(OlMsgsDomain, "TextEdit: Can't convert"));
	    retval = False;
	} /* else for #ifndef sun */
#endif
    }
    return (retval);

}				/* end of ConvertClipboardOrDnD */
/*
 * LoseClipboardOrDnD
 *
 */

static void
LoseClipboardOrDnD(Widget w, Atom *atom)
{
    TextEditWidget  ctx = (TextEditWidget) w;

    if (ctx->textedit.transient != (Atom) NULL &&
	ctx->textedit.transient == *atom) {
	if(ctx->textedit.dnd_contents != NULL) {
		FREE(ctx->textedit.dnd_contents);
		ctx->textedit.dnd_contents = NULL;
	}
	OlDnDFreeTransientAtom((Widget) ctx, ctx->textedit.transient);
	ctx->textedit.transient = (Atom) NULL;
    } else 	if (ctx->textedit.clip_contents != NULL) {
			FREE(ctx->textedit.clip_contents);
			ctx->textedit.clip_contents = NULL;
    		}
}				/* LoseClipboardOrDnD */

/*
 * Paste
 *
 */

static void
Paste(TextEditWidget ctx, XEvent *event, String *params, Cardinal *num_params)
{
    Display        *dpy = XtDisplay(ctx);

	XtGetSelectionValue((Widget) ctx, XA_CLIPBOARD(dpy),
			XA_STRING, Paste2, NULL,
			_XtLastTimestampProcessed((Widget) ctx));
}				/* end of Paste */

static void
SelectTarget(Widget w, XtPointer client_data, Atom *selection, Atom *type, XtPointer value, long unsigned int *length, int *format)
{
SelectTargetPtr select = (SelectTargetPtr)client_data;
DropClosurePtr drop = select->drop;
Time		timestamp = select->timestamp; 

	if(*type == XA_ATOM && *length != 0) {
		int i;
		Atom *targets = (Atom *)value;

		for(i=0; i < *length; i++) 
			if(targets[i] == XA_STRING || 
			targets[i] == 
			OlInternAtom(XtDisplay(w),COMPOUND_TEXT_NAME)) {
				XtGetSelectionValue(w,*selection,
					targets[i], Paste2, (XtPointer)drop,timestamp);
				break;
			}  
	}  
		
}
/*
 * Paste2
 *
 */

static void
Paste2(Widget w, XtPointer client_data, Atom *selection, Atom *type, XtPointer value, long unsigned int *length, int *format)
{
    TextEditWidget  ctx = (TextEditWidget) w;
    DropClosurePtr  drop = (DropClosurePtr) client_data;

	
    if (*length != 0) {
	TextEditPart *       text = &ctx->textedit;
	TextBuffer *         textBuffer = text->textBuffer;
	TextLocation         startloc;
	TextLocation         endloc;
	OlTextModifyCallData call_data;
	Boolean              retval = FALSE;
	OlStrRep             rep = text->text_format;
	OlStr *              strbufptr = NULL;
	OlStr                cvt_str;
	int		     cvt_len;

	if (((char *) value)[*length] != '\0' && ((char *) value)[*length - 1] != '\0') {
	    value = (XtPointer) XtRealloc((char *) value, ++*length);
	    ((char *) value)[*length - 1] = '\0';
	}

	if(*type == XA_STRING) {
		if(rep == OL_WC_STR_REP) { /* incoming value is always multi-byte */
			cvt_str = (OlStr)XtMalloc(sizeof(wchar_t)*(*length+1));
			(void)mbstowcs((wchar_t *)cvt_str, (char *)value, *length);
			cvt_len = wslen((wchar_t *)cvt_str)+1;
		} else {
			cvt_len = *length;
			cvt_str = (OlStr)value;
		}
			
	} else if(*type == OlInternAtom(XtDisplay(ctx),COMPOUND_TEXT_NAME)) {
			strbufptr = str_methods[rep].StrFromCT(
					XtDisplay(w), (char *)value);
			if (strbufptr != (OlStr *)NULL) {
	    			cvt_str = *strbufptr;
	   	       		 cvt_len = str_methods[rep].StrNumBytes(cvt_str);
			} else {
	    			cvt_str = str_methods[rep].StrEmptyString();
	    			cvt_len = 0;
			}
	} 
            
	call_data.ok = TRUE;
	call_data.current_cursor = text->cursorPosition;
	call_data.new_cursor =
	    call_data.new_select_start =
	    call_data.select_start = text->selectStart;

	if (drop != (DropClosurePtr) NULL) {
	    call_data.new_select_end =
	    call_data.select_end     = text->selectEnd;
	} else {
	    /* find number of bytes */
	    call_data.select_end       = text-> selectEnd;
	    call_data.new_cursor       =
	    call_data.new_select_start =
	    call_data.new_select_end   = text->cursorPosition + cvt_len +
					 text->selectEnd - text->selectStart;
	}
	call_data.text        = (String)cvt_str;
	call_data.text_length = cvt_len;

	XtCallCallbacks((Widget) ctx, XtNmodifyVerification, &call_data);

	if (call_data.ok == TRUE) {
	    if (rep == OL_SB_STR_REP) {
		if (drop != (DropClosurePtr) NULL) {
		    startloc = endloc = LocationOfPosition(textBuffer,
							   drop->position);
		} else {
		    startloc = LocationOfPosition(textBuffer, text->selectStart);
		    endloc   = LocationOfPosition(textBuffer, text->selectEnd);
		}
		ReplaceBlockInTextBuffer(textBuffer, &startloc, &endloc,
					 (char *) cvt_str, 
				(TextUpdateFunction)UpdateDisplay, (caddr_t) ctx);
	    } else {
		OlTextBufferPtr mltx = (OlTextBufferPtr)textBuffer;
		
		if (drop != (DropClosurePtr) NULL) {
		    OlLocationOfPosition(mltx, drop->position, &startloc);
		    endloc = startloc;
		} else {
		    OlLocationOfPosition(mltx, text->selectStart, &startloc);
		    OlLocationOfPosition(mltx, text->selectEnd, &endloc);
		}
		OlReplaceBlockInTextBuffer(mltx, &startloc, &endloc,
					   cvt_str, 
					(TextUpdateFunction)
					UpdateDisplay, (caddr_t) ctx);
	    }
	}
	if(strbufptr != (OlStr *)NULL)
		(*str_methods[rep].StrFreeList)(strbufptr);
	XtFree(value);
	/* NOW that we got the data, if it was a DRAG_MOVE, request the
                 * selection to be deleted.
                 */
                if ((drop != (DropClosurePtr)NULL) &&
                                (drop->operation == OlDnDTriggerMoveOp)) {
                        XtGetSelectionValue((Widget)ctx, *selection,
                                        OlInternAtom(XtDisplay((Widget)ctx),
                                                DELETE_NAME),
                                        AckDel,
                                        (XtPointer)drop->send_done ,
                                        _XtLastTimestampProcessed((Widget)ctx)); 
                }
    }
	 /* AckDel() calls OlDnDDragNDropDone(), so if we call 
          * XtGetSelectionValue() above on DELETE_NAME, 
          * we do NOT want to call the done proc again here.
          */
        if (drop != (DropClosurePtr)NULL && drop->send_done &&
              !(((*length) != 0) && (drop->operation == OlDnDTriggerMoveOp))){
                OlDnDDragNDropDone((Widget)ctx, *selection,
                           XtLastTimestampProcessed(XtDisplay((Widget)ctx)),
                                   NULL, NULL);
        }

} /* end of Paste2 */

static Boolean
ActivateWidget(
	       Widget		w,
	       OlVirtualName	type,
	       XtPointer	call_data)
{
    Boolean         consumed = False;
    TextEditWidget	ctx = (TextEditWidget)w;
    TextEditPart	*text = &(ctx->textedit);

    switch (type) {
    case OL_COPY:
	consumed = True;
	(void) OlTextEditCopySelection(ctx, False);
	break;
    case OL_CUT:
	consumed = True;
	if(text->editMode == OL_TEXT_EDIT) {
		text->selectMode = 0;
		(void) OlTextEditCopySelection(ctx, True);
	} else
		_OlBeepDisplay(w,1);
	break;
    default:
	break;
    }
    return (consumed);
}				/* end of ActivateWidget */
/*
 * Key
 *
 * The \fIKey\fR procedure is called whenever a key press event occurs
 * within the TextEdit window.  The procedure is called indirectly by
 * OlAction().
 * Even though the XtNconsumedCallback callback was called by OlAction,
 * This procedure calls the keys callback list to maintain compatibility
 * with older releases to see if the application used this mechanism
 * to intercept the event.  If it has, the event is consumed to prevent
 * the generic routine from handling it.
 * If no callbacks exist or if they all indicate disinterest in the
 * event then the internal procedure to handle interesting key press
 * activity associated with the event is called.
 *
 */

static void
Key(Widget w, OlVirtualEvent ve)
{
    XEvent         *event = ve->xevent;
    TextEditWidget  ctx = (TextEditWidget) w;
    KeySym          keysym = ve->keysym;
    int             length = ve->length;
    OlInputCallData cd;		
    TextPosition    position;
    int  num_chars = 0;
    int  preedit_chars = 0;
    OlStrRep	rep = ctx->primitive.text_format;


    cd.consumed = False;
    cd.event    = event;
    cd.keysym   = &keysym;
    cd.buffer   = ve->buffer;
    cd.length   = &length;
    cd.ol_event = (OlInputEvent)ve->virtual_name;
				

	if(ctx->textedit.pre_edit == True &&
			ctx->textedit.pre_edit_end > 
			ctx->textedit.pre_edit_start)  
			switch(cd.ol_event) {
			case OL_MENUKEY:
			case OL_COPY:
			case OL_CUT:
			case OL_REDRAW:
			case OL_UNKNOWN_KEY_INPUT:
				break;
			case OL_UNDO:
				_DoImplicitCommit(ctx, ve, TRUE);
				{  
				/*
				 * We do not want to UNDO the deleted
				 * preedit string that must be in the
				 * ctx->textedit.deleted after
				 * implcit commit.
				 */
				OlTextBufferPtr oltb =
					(OlTextBufferPtr)ctx->textedit.textBuffer;
				OlTextUndoItem oltu = 
						{NULL,0,0,TEXT_BUFFER_NOP}; 
				OlSetTextUndoDeleteItem(oltb, oltu);
				}
				break;
			default:
				_DoImplicitCommit(ctx, ve, TRUE);
				break;
			}
		
		
/* BEGIN ISO-8859 WATCH */    
    /* Remap an unmodified tab key (iff mapped to OL_NEXTFIELD) to  */
    /* OL_UNKNOWN_KEY_INPUT, thus recognized as an insertable char. */
    
    if ((ctx->textedit.insertTab == TRUE)         &&
	(cd.ol_event             == OL_NEXTFIELD) &&
       !(event->xkey.state & ~ve->dont_care)      &&
	(*cd.length              == 1)            &&
	( ( (rep == OL_WC_STR_REP) && 
		(*((wchar_t *)cd.buffer) == L'\t') ) ||
	  (*cd.buffer == '\t') ) ) {
	cd.ol_event = OL_UNKNOWN_KEY_INPUT;
    }
/* END ISO-8859 WATCH */
    
    if (XtHasCallbacks(w, XtNkeys) == XtCallbackHasSome)
	XtCallCallbacks(w, XtNkeys, &cd);

    if (cd.consumed == False) {
	cd.consumed = True;

	switch (cd.ol_event) {
	    
	case OL_UNKNOWN_KEY_INPUT:
	    if(event->xkey.keycode == 0) {

		/* only possibility is a composed string from
		 * input method.
		 */
		/* Assuming that TextEditCheckAndInsert() will succeed.
		 */
		if(ctx->textedit.pre_edit == True) 
			preedit_chars = AdjustPreeditStartAndEndPos(ctx,
							(OlStr)cd.buffer);
		if(*cd.length)
			if ( !TextEditCheckAndInsert((TextEditWidget)w,
					       (OlStr)cd.buffer, *cd.length) )
			{
			    ctx->textedit.pre_edit_start -= preedit_chars;
			    ctx->textedit.pre_edit_end -= preedit_chars;
			}
	    } else {  
		
	    if (IsModifierKey(keysym))
		break;
	    /*
	     * Keys like F1..., L1... etc have length == 0. But these could
	     * be valid accelerators. So we check for this condition too
	     * below - as per bug # 1054075
	     */
	    if (*cd.length == 0) {
		if ((_OlFetchMnemonicOwner(w, (XtPointer *) NULL, ve) ||
		     _OlFetchAcceleratorOwner(w, (XtPointer *) NULL, ve)))
		{
		    /* mnemonic/accelerator: needs further processing */
		    cd.consumed = False;
		}
		break;
	    }
	    if (!(event->xkey.state & ~(ShiftMask | LockMask)))
	    {
		/* only modifiers present are SHIFT and/or CAPS LOCK */
		if(ctx->textedit.pre_edit == True) 
			preedit_chars = AdjustPreeditStartAndEndPos(ctx,
							(OlStr)cd.buffer);
		if ( !TextEditCheckAndInsert((TextEditWidget)w,
				       (OlStr)cd.buffer, *cd.length))
		{
			    ctx->textedit.pre_edit_start -= preedit_chars;
			    ctx->textedit.pre_edit_end -= preedit_chars;
		}
	    } else if ((_OlFetchMnemonicOwner(w, (XtPointer *) NULL, ve) ||
		        _OlFetchAcceleratorOwner(w, (XtPointer *) NULL, ve)))
	    {
		/* mnemonic/accelerator: needs further processing */
		cd.consumed = False;
	    } else {		/* crazy modifiers; insert anyway */
		if(ctx->textedit.pre_edit == True) 
			preedit_chars = AdjustPreeditStartAndEndPos(ctx,
							(OlStr)cd.buffer);
		if ( !TextEditCheckAndInsert((TextEditWidget)w,
				       (OlStr)cd.buffer, *cd.length) )
		{
			    ctx->textedit.pre_edit_start -= preedit_chars;
			    ctx->textedit.pre_edit_end -= preedit_chars;
		}
	    }

	    } /* else if keycode != 0 */
	    break;
	case OL_UNDO:
	    UndoUpdate((TextEditWidget) w);
	    break;
	    
	case OL_PASTE:
	    {
		Cardinal        num_params = 0;
		if(ctx->textedit.editMode == OL_TEXT_EDIT)
			Paste((TextEditWidget)w, event, (String *) NULL, &num_params);
		else
			_OlBeepDisplay(w,1);
	    }
	    break;

#ifndef sun			/* Sun's model has no FlipEnds */
	case OL_SELFLIPENDS:
	    if (ctx->textedit.selectStart != ctx->textedit.selectEnd) {
		if (ctx->textedit.cursorPosition == ctx->textedit.selectStart)
		    _MoveSelection(ctx, ctx->textedit.selectEnd,
		     ctx->textedit.selectStart, ctx->textedit.selectEnd, 7);
		else
		    _MoveSelection(ctx, ctx->textedit.selectStart,
		     ctx->textedit.selectStart, ctx->textedit.selectEnd, 7);
	    }
	    break;
#endif
	    
	case OL_SELCHARFWD:
	case OL_SELCHARBAK:
	case OL_SELWORDFWD:
	case OL_SELWORDBAK:
	case OL_SELLINEFWD:
	case OL_SELLINEBAK:
	case OL_SELLINE:
	    _ExtendSelection(ctx, cd.ol_event);
	    break;
	    
	case OL_DELCHARFWD:
	case OL_DELCHARBAK:
	case OL_DELWORDFWD:
	case OL_DELWORDBAK:
	case OL_DELLINEFWD:
	case OL_DELLINEBAK:
	case OL_DELLINE:
	    if(ctx->textedit.editMode == OL_TEXT_EDIT)
	    	Delete((TextEditWidget) w, event, cd.ol_event, 0);
	    else
		_OlBeepDisplay(w,1);
	    break;

	case OL_RETURN:
	    if (ctx->textedit.insertReturn == TRUE) {
		TextEditCheckAndInsert(ctx,
				       (ctx->textedit.text_format == OL_WC_STR_REP)?
				       (OlStr)L"\n":(OlStr)"\n",
				       1);
	    } else {
		/*
		 * If textedit doesn't consume OL_RETURN, it needs to
		 * continue the lookup as an OL core key. Because only
		 * textedit widget knows about OL_RETURN.
		 */
		OlLookupInputEvent(w, event, ve, OL_CORE_IE);
		cd.consumed = False;
	    }
	    break;
	    
	case OL_REDRAW:
	    OlTextEditRedraw(ctx);
	    break;
	    
	case OL_CHARFWD:
	case OL_CHARBAK:
	case OL_ROWDOWN:
	case OL_ROWUP:
	case OL_WORDFWD:
	case OL_WORDBAK:
	case OL_LINESTART:
	case OL_LINEEND:
	case OL_DOCSTART:
	case OL_DOCEND:
	case OL_PANESTART:
	case OL_PANEEND:
	    _MoveCursorPosition(ctx, event, cd.ol_event, 0);
	    break;
	case OL_PAGEUP:
	case OL_PAGEDOWN:
	case OL_HOME:
	case OL_END:
	case OL_SCROLLUP:
	case OL_SCROLLDOWN:
	    (void) _MoveDisplayPosition(ctx, event, cd.ol_event, 0);
	    break;
	case OL_SCROLLRIGHT:
	    _MoveDisplayLaterally(ctx, &ctx->textedit, MIN(PAGEWID(ctx), ctx->textedit.maxX + ctx->textedit.xOffset - PAGEWID(ctx)), TRUE);
	    break;
	case OL_SCROLLRIGHTEDGE:
	    _MoveDisplayLaterally(ctx, &ctx->textedit, ctx->textedit.maxX + ctx->textedit.xOffset - PAGEWID(ctx), TRUE);
	    break;
	case OL_SCROLLLEFT:
	    _MoveDisplayLaterally(ctx, &ctx->textedit, MAX(-PAGEWID(ctx), ctx->textedit.xOffset), TRUE);
	    break;
	case OL_SCROLLLEFTEDGE:
	    _MoveDisplayLaterally(ctx, &ctx->textedit, ctx->textedit.xOffset, TRUE);
	    break;
	case OL_MENUKEY:
	    PopupTextEditMenu(ctx);
	    break;
#ifndef sun			/* Sun's model does not have mouseless DnD */
	case OL_DRAG:
	    position = _PositionFromXY(ctx, event->xkey.x, event->xkey.y, OL_CHAR);
	    DragText(ctx, &ctx->textedit, position, OlMoveDrag);
	    break;
#endif
	default:
	    /*
	     * The default action is to let the event pass through.  We do
	     * this by marking it as being unconsumed.  (Remember, we marked
	     * as being consumed in this 'if' block.
	     */
	    cd.consumed = False;
	    break;
	}
    }
	
    ve->consumed = cd.consumed;
} /* end of Key */

/*
 * _OlSetClickMode
 *
 */
extern void 
_OlSetClickMode(int mode)
{
    click_mode = mode;
}				/* end of _OlSetClickMode */

/*
 * Button
 *
 * The \fIButton\fR procedure is called whenever a button down event occurs
 * within the TextEdit window.  The procedure is called indirectly by
 * OlAction().
 * Even though the XtNconsumedCallback callback was called by OlAction,
 * This procedure calls the buttons callback list to maintain compatibility
 * with older releases to see if the application used this mechanism
 * to intercept the event.  If it has, the event is consumed to prevent
 * the generic routine from handling it.
 * If no callbacks exist or if they all indicate disinterest in the
 * event then the internal procedure to handle interesting button down
 * activity associated with the event is called.
 *
 */

static void
Button(Widget w, OlVirtualEvent ve)
{
    XEvent         *event = ve->xevent;
    TextEditWidget  ctx = (TextEditWidget) w;
    TextEditPart   *text = &ctx->textedit;
    TextPosition    position;

    OlInputCallData cd;

    cd.consumed = False;
    cd.event = event;
    cd.ol_event = (OlInputEvent)ve->virtual_name;

	if(ctx->textedit.pre_edit == True &&
			ctx->textedit.pre_edit_end > 
			ctx->textedit.pre_edit_start) 
		switch(cd.ol_event) {
		case OL_MENU:
		case OL_CONSTRAIN:
			break;
		case OL_SELECT:
			position = _PositionFromXY(ctx, 
						event->xbutton.x,
						event->xbutton.y,
						OL_CARET);
			if(position != ctx->textedit.cursorPosition)
				_DoImplicitCommit(ctx, ve, TRUE);
			break;
		default:
			_DoImplicitCommit(ctx, ve, TRUE);
			break;
		}

    if (XtHasCallbacks(w, XtNbuttons) == XtCallbackHasSome)
	XtCallCallbacks(w, XtNbuttons, &cd);

    if (cd.consumed == False) {
	cd.consumed = True;
	switch (cd.ol_event) {
	case OL_SELECT:
	    _TextEditOwnPrimary(ctx, ((XButtonEvent *) event)->time);
	    if (!HAS_FOCUS(ctx))
		(void) OlCallAcceptFocus(w, ((XButtonEvent *) event)->time);
	    if ((XtIsSubclass(XtParent(ctx), textFieldWidgetClass)) ||
		(click_mode == XVIEW_CLICK_MODE))
		Select(ctx, event);
	    break;
	case OL_ADJUST:
	    if (XtIsSubclass(XtParent(ctx), textFieldWidgetClass))
		_TextEditOwnPrimary(ctx, ((XButtonEvent *) event)->time);
	    Adjust(ctx, event);
	    break;
	case OL_DUPLICATE:
	    position = _PositionFromXY(ctx, event->xbutton.x, 
						event->xbutton.y, OL_CHAR);
	    if (text->selectStart <= position && position <= text->selectEnd - 1)
		DragText(ctx, text, position, OlCopyDrag);
	    break;
	case OL_PAN:
	    {
		text->mask = event->xbutton.state | 1 << (event->xbutton.button + 7);
		PanX = event->xbutton.x;
		PanY = event->xbutton.y - PAGE_T_MARGIN(ctx);
		OlGrabDragPointer((Widget) ctx,
				  OlGetPanCursor((Widget) ctx),
				  XtWindow((Widget) ctx));
		XtAppAddTimeOut(XtWidgetToApplicationContext((Widget) ctx),
				0, PollPan, (XtPointer) ctx);
	    }
	    break;
	case OL_MENU:
	    PopupTextEditMenu(ctx);
	    break;
	case OL_CONSTRAIN:
	default:
	    /*
	     * The default action is to let the event pass through.  We do
	     * this by marking it as being unconsumed.  (Remember, we marked
	     * as being consumed in this 'if' block.
	     */
	    cd.consumed = False;
	    break;
	}
    }
    ve->consumed = cd.consumed;
}				/* end of Button */


/*
 * Select
 *
 * The \fISelect\fR procedure is called when a SELECT mouse press is
 * discovered.  The routine determines what kind of SELECT activity
 * the user is attempting: a simple click - to extend the selection
 * to the point of the click, a double click  - to select the next
 * larger unit of text (chr, word, line, paragraph, document, char...),
 * a drag and drop operation - to copy a selection,
 * or if the user is attempting to alter the selection by dragging SELECT.
 * In the latter case the selection is selection is initiated here and
 * a poll loop is established using PollMouse.  The poll loop will
 * continue to extend the selection until the state of the mouse
 * changes (i.e., the user releases the SELECT button).
 *
 */

static void
Select(TextEditWidget ctx, XEvent *event)
{
    TextEditPart   *text = &ctx->textedit;
    TextPosition    position;
    ButtonAction    action;
    Boolean         multi_click_pending = False;

    do {
	switch ((action = _OlPeekAheadForEvents((Widget) ctx, event))) {
	case MOUSE_MOVE:
	    position = _PositionFromXY(ctx, event->xbutton.x,
				       event->xbutton.y, OL_CHAR);
	    if ((text->selectStart <= position
		 && position <= text->selectEnd - 1)
		&& !multi_click_pending)
		DragText(ctx, text, position, OlMoveDrag);
	    else {
		switch (text->selectMode) {
		case 0:
		    _MoveSelection(ctx, position, 0, 0, 0);
		    text->selectMode = 6;
		    break;
		case 1:
		    text->selectMode = 8;
		    break;
		case 2:
		    text->selectMode = 9;
		    break;
		default:
		    _MoveSelection(ctx, position, 0, 0, 0);
		    text->selectMode = 6;
		    break;
		}		/* switch */
		_MoveSelection(ctx, position, 0, 0, text->selectMode);
		text->shouldBlink = FALSE;
		text->mask =
		    event->xbutton.state | 1 << (event->xbutton.button + 7);
		_TurnTextCursorOff(ctx);
		XtAppAddTimeOut(XtWidgetToApplicationContext((Widget) ctx), 0,
				PollMouse, (XtPointer) ctx);
	    }			/* else */
	    break;
	case MOUSE_CLICK:
	    _MoveSelection
		(ctx, _PositionFromXY(ctx, event->xbutton.x,
				      event->xbutton.y, OL_CARET), 0, 0, 0);
	    break;
	case MOUSE_MULTI_CLICK:
	    if (++text->selectMode == 5)
		text->selectMode = 0;
	    _MoveSelection
		(ctx, _PositionFromXY(ctx, event->xbutton.x,
				 event->xbutton.y,OL_CHAR), 0, 0, text->selectMode);
	    break;
	case MOUSE_MULTI_CLICK_PENDING:
	    if (++text->selectMode == 5)
		text->selectMode = 0;
	    _MoveSelection
		(ctx, _PositionFromXY(ctx, event->xbutton.x,
				 event->xbutton.y,OL_CHAR), 0, 0, text->selectMode);
	    multi_click_pending = True;
	    break;
	case MOUSE_MULTI_CLICK_DONE:
	    multi_click_pending = False;
	    break;
	default:
	    break;
	}
    } while ((action == MOUSE_MULTI_CLICK_PENDING)
	     || (action == NOT_DETERMINED));


}				/* end of Select */
/*
 * Adjust
 *
 * The \fIAdjust\fR procedure is called when a ADJUST mouse press is
 * discovered.  The routine determines what kind of ADJUST activity
 * the user is attempting: a simple click - to extend the selection
 * to the point of the click (double click is considered the same as
 * two sequential single clicks) or if the user is attempting to
 * alter the selection by dragging the ADJUST.  In the latter case
 * the slection is adjusted here and a poll loop is established using
 * PollMouse.  The poll loop will continue to extend the selection
 * until the state of the mouse changes (i.e., the user releases the
 * ADJUST button).
 *
 */

static void
Adjust(TextEditWidget ctx, XEvent *event)
{
    TextEditPart   *text = &ctx->textedit;
    ButtonAction    action;
    TextPosition    position;

    if (!ctx->primitive.has_focus)
	return; /* to prevent race condition if focus is gone */

    do {
	switch ((action = _OlPeekAheadForEvents((Widget) ctx, event))) {
	case MOUSE_MOVE:
	    position = _PositionFromXY(ctx, event->xbutton.x,
				       event->xbutton.y,OL_CHAR);
	    switch (text->selectMode) {
	    case 0:
		text->selectMode = 6;
		break;
	    case 5:
	    case 6:
	    case 65:
		text->selectMode = 65;
		break;
	    case 1:
		text->selectMode = 8;
		break;
	    case 51:
	    case 8:
	    case 85:
		text->selectMode = 85;
		break;
	    case 2:
		text->selectMode = 9;
		break;
	    case 52:
	    case 9:
	    case 95:
		text->selectMode = 95;
		break;
	    default:
		text->selectMode = 5;
		break;
	    }			/* switch */

	    _MoveSelection(ctx, position, 0, 0, text->selectMode);
	    text->shouldBlink = FALSE;
	    text->mask =
		event->xbutton.state | 1 << (event->xbutton.button + 7);
	    _TurnTextCursorOff(ctx);
	    XtAppAddTimeOut(XtWidgetToApplicationContext((Widget) ctx),
			    0, PollMouse, (XtPointer) ctx);
	    break;
	case MOUSE_MULTI_CLICK:
	case MOUSE_CLICK:
	    switch (text->selectMode) {
	    case 0:
	    case 5:
	    case 6:
	    case 65:
		_MoveSelection
		    (ctx, _PositionFromXY(ctx, event->xbutton.x,
					  event->xbutton.y,OL_CHAR), 0, 0, 5);
		_MakeTextCursorVisible(ctx);
		break;
	    case 1:
	    case 51:
	    case 8:
	    case 85:
		_MoveSelection
		    (ctx, _PositionFromXY(ctx, event->xbutton.x,
					  event->xbutton.y,OL_CHAR), 0, 0, 51);
		break;
	    case 2:
	    case 52:
	    case 9:
	    case 95:
		_MoveSelection
		    (ctx, _PositionFromXY(ctx, event->xbutton.x,
					  event->xbutton.y,OL_CHAR), 0, 0, 52);
		break;
	    }
	    break;
	default:
	    break;
	}
    } while ((action == MOUSE_MULTI_CLICK_PENDING)
	     || (action == NOT_DETERMINED));

}				/* end of Adjust */

/*
 * PollPan
 *
 * The \fIPollPan\fR procedure is used to poll the mouse until the press
 * which initiated the pan is released.
 *
 * Note: This routine currently constrains panning to vertical movement.
 * Care must be taken to intelligently support horizontal panning
 * since it is unlike a user will very often want to use it and a fine
 * grain will cause lots of activity (i.e., redraws and copy areas)
 *
 */

static void
PollPan(XtPointer client_data, XtIntervalId *id)
{
    TextEditWidget  ctx = (TextEditWidget) client_data;
    TextEditPart   *text = &ctx->textedit;

/* for query pointer */
    Window          root;
    Window          child;
    int             rootx, rooty, winx, winy;
    unsigned int    mask;
    XEvent	    dummy;

    XQueryPointer(XtDisplay(ctx), XtWindow(ctx),
		  &root, &child, &rootx, &rooty, &winx, &winy, &mask);

    if (mask != text->mask) {
	text->shouldBlink = TRUE;	/* let the timer expire */
	OlUngrabDragPointer((Widget) ctx);
    } else {
	int             vdelta = (int) (PanY - (winy - (int) 
					PAGE_T_MARGIN(ctx))) / (int)FONTHT(ctx);

#ifdef SUPPORT_HORIZONTAL_PAN
	int             hdelta = PanX - winx;
	TextEditPart   *text = &ctx->textedit;
	if (vdelta && text->wrapMode == OL_WRAP_OFF && text->maxX > PAGEWID(ctx))
	    _MoveDisplayLaterally(ctx, &ctx->textedit, vdelta, TRUE);
#endif

	if (vdelta)
	    PanY -= (_MoveDisplayPosition(ctx, 
			(*id ? &dummy: (XEvent *)NULL), 
					OL_PGM_GOTO, vdelta) * FONTHT(ctx));

	XtAppAddTimeOut(XtWidgetToApplicationContext((Widget) ctx),
			(unsigned long) 0, (XtTimerCallbackProc) PollPan,
			(XtPointer) ctx);
    }

}				/* end of PollPan */
/*
 * PollMouse
 *
 * The \fIPollMouse\fR procedure is used to poll the mouse until
 * the state of the mouse changes relative to when the poll was started
 * (e.g., as a result of a Select or Adjust drag).  If the state
 * remains the same the selection is extended (by calling _MoveSelection)
 * and another poll is scheduled.  If the state of the mouse changes
 * the poll is ignored and the polling is stopped.
 *
 */
void dummy_fn(void)
{
}

static void
PollMouse(XtPointer client_data, XtIntervalId *id)
{
    TextEditWidget  ctx = (TextEditWidget) client_data;
    TextEditPart   *text = &ctx->textedit;

/* for query pointer */
    Window          root;
    Window          child;
    int             rootx, rooty, winx, winy;
    unsigned int    mask;

    if (text->cursor_state == OlCursorOn) {
	_TurnTextCursorOff(ctx);
    }
    if (text->blink_timer != NULL) {
	XtRemoveTimeOut(text->blink_timer);
	text->blink_timer = NULL;
    }

    XQueryPointer(XtDisplay(ctx), XtWindow(ctx),
		  &root, &child, &rootx, &rooty, &winx, &winy, &mask);

    if (mask != text->mask) {
	text->shouldBlink = TRUE;	/* let the timer expire */
	XUngrabPointer(XtDisplay(ctx), CurrentTime);

	if (text->cursor_visible) { /* ensure OL_IN has been processed */
	    text->cursor_state = OlCursorOn;
	    _TurnTextCursorOff(ctx);
	    text->cursor_state = OlCursorOn;
	    _TurnCursorBlinkOn(ctx);
        }
	if (text->selectMode == 65 ||
	    text->selectMode == 85 ||
	    text->selectMode == 95)
	    text->anchor = text->selectStart;
    } else {
	_MoveSelection(ctx, _PositionFromXY(ctx, winx, winy,OL_CHAR), 0, 0,
		       text->selectMode);
	XtAppAddTimeOut(XtWidgetToApplicationContext((Widget) ctx),
			(unsigned long) 0, (XtTimerCallbackProc) PollMouse,
			(XtPointer) ctx);
    }

}				/* end of PollMouse */

static void
CleanupTransaction 
(Widget widget, Atom selection, OlDnDTransactionState state, Time timestamp, XtPointer closure)
{
    TextEditWidget  ctx = (TextEditWidget) widget;

    switch (state) {
    case OlDnDTransactionDone:
    case OlDnDTransactionRequestorError:
    case OlDnDTransactionRequestorWindowDeath:
	if (selection != ctx->textedit.transient)
	    break;
	OlDnDDisownSelection(widget, selection, CurrentTime);
	OlDnDFreeTransientAtom((Widget) ctx,
			       ctx->textedit.transient);
	ctx->textedit.transient = (Atom) NULL;
	if (ctx->textedit.dnd_contents != (char *) NULL) {
	    FREE(ctx->textedit.dnd_contents);
	    ctx->textedit.dnd_contents = (char *) NULL;
	}
	break;
    case OlDnDTransactionEnds:
    case OlDnDTransactionBegins:
	;
    }
}

/*
 * DragText
 *
 * The \fIDragText\fR procedure handles the drag-and-drop operation.
 * It creates the cursor to be used during the drag and calls the utility
 * drag and drop functions to monitor the drag.  Once the user has dropped
 * the text the DragText procedure determines where the drop was made.
 * If the drop occurs on the widget where the drag initiated then either
 * the drop point was in the midst of the selection - indicating that
 * the user wished to abort the drop or the drop was outside the selection
 * indicating that the text is to be copied at the drop point.
 * If the drop occurs on another window the SendPasteMessage function
 * is called to tell the drop window that text had been dropped on
 * it - leaving the transfer of data to the dropee.
 */


static void
DragText(TextEditWidget ctx, TextEditPart *text, TextPosition position, OlDragMode drag_mode)
{
    Window          	drop_window;
    Position        	x;
    Position        	y;
    OlStr	   	buffer;
    OlDnDDragDropInfo 	rinfo;
    OlDnDCursors 	cursors;


    OlTextEditReadSubString(ctx, (char **)&buffer, text->selectStart,
			_OlMin(text->selectStart + 3, text->selectEnd) - 1);

    cursors = _OlCreateDnDCursors((Widget)ctx, buffer, 
				  ctx->primitive.font,
				  ctx->primitive.text_format,
				  drag_mode);

    OlGrabDragPointer((Widget) ctx, cursors.Drag, None);
    if (OlDnDDragAndDrop((Widget) ctx, &drop_window, &x, &y, &rinfo,
			 (OlDnDPreviewAnimateCbP)_OlDnDAnimate, (XtPointer)&cursors)) {
	TextDropOnWindow((Widget) ctx, drop_window, x, y, text, drag_mode,
			 &rinfo);
    } else
	_OlBeepDisplay((Widget) ctx, 1);

    OlUngrabDragPointer((Widget) ctx);

    _OlFreeDnDCursors((Widget)ctx, cursors);
}				/* end of DragText */

static void
TextDropOnWindow(Widget ctx, Window drop_window, Position x, Position y, TextEditPart *text, OlDragMode drag_mode, OlDnDDragDropInfoPtr rinfo)
{
    Widget          drop_widget;
    Display        *dpy = XtDisplay(ctx);
    Window          win = DefaultRootWindow(dpy);
    Widget          shell;
    TextPosition    position;

    drop_widget = XtWindowToWidget(dpy, drop_window);

    shell = (drop_widget == NULL ||
	     drop_window == RootWindowOfScreen(XtScreen(ctx))) ?
	NULL : _OlGetShellOfWidget(drop_widget);

if (drop_widget == ctx) {
    if(((TextEditWidget)ctx)->textedit.editMode == OL_TEXT_EDIT) {
	position = _PositionFromXY((TextEditWidget)ctx, x, y,OL_CARET);
	if (text->selectStart <= position && position <= text->selectEnd - 1);
	else {
	    if (drag_mode == OlMoveDrag) {
		position = AdjustPosition
		    (position, text->selectEnd - text->selectStart,
		     0, text->selectStart, text->selectEnd);
		OlTextEditCopySelection((TextEditWidget) ctx, 
				(drag_mode == OlCopyDrag) ? FALSE: TRUE); 
	    } else {
		OlTextEditCopySelection((TextEditWidget) ctx, 
				(drag_mode == OlCopyDrag) ? FALSE: TRUE); 
				 
	    }
	    if (_MoveSelection((TextEditWidget) ctx, position, 0, 0, 0)) {
		Cardinal        num_params = 0;
		
		Paste((TextEditWidget)ctx, (XEvent *)NULL, (String *)NULL, &num_params);
	    }
	}

      } else
		/* cannot accept a Drop if Read-only */
		_OlBeepDisplay(ctx,1);
    } else {
	Boolean         got_selection;
	Boolean         drop_failed = False;

	if (text->transient == (Atom) NULL) {
	    text->transient = OlDnDAllocTransientAtom(ctx);
	    got_selection = False;
	} else
	    got_selection = True;
	if (!got_selection)
	    got_selection = OlDnDOwnSelection(ctx, text->transient, rinfo->drop_timestamp,
				ConvertClipboardOrDnD, LoseClipboardOrDnD,
					      (XtSelectionDoneProc)NULL, CleanupTransaction, NULL);
	if (got_selection) {
	    OlStr *	ptr = &text->dnd_contents;

	    if (text->dnd_contents != (OlStr)NULL) {
		XtFree(text->dnd_contents);
		text->dnd_contents = (OlStr)NULL;
	    }
	    OlTextEditReadSubString((TextEditWidget) ctx, (char  **)ptr,
				    text->selectStart, text->selectEnd - 1);
	    if (!OlDnDDeliverTriggerMessage(ctx, rinfo->root_window,
					    rinfo->root_x, rinfo->root_y, 
					    text->transient,
					    (drag_mode == OlMoveDrag)?
					      OlDnDTriggerMoveOp:
						OlDnDTriggerCopyOp,
					    rinfo->drop_timestamp)) {
		_OlBeepDisplay((Widget) ctx, 1);
		OlDnDDisownSelection(ctx, text->transient, 
					rinfo->drop_timestamp);
		OlDnDFreeTransientAtom(ctx, text->transient);
		text->transient = (Atom) NULL;
	    } 
	}
    }				/* else */
}

/*
 * ReceivePasteMessage
 *
 */

static void
ReceivePasteMessage(Widget w, XEvent *event, String *params, Cardinal *num_params)
{
    TextEditWidget  ctx = (TextEditWidget) w;
    TextEditPart   *text = &ctx->textedit;
    Position        x;
    Position        y;
    int             operation;

    if (event->xclient.message_type == OlInternAtom(event->xany.display,
						    OL_PASTE_MSG_NAME)) {
	x = (Position) event->xclient.data.l[0];
	y = (Position) event->xclient.data.l[1];
	operation = (int) event->xclient.data.l[2];

	if (_MoveSelection(ctx, _PositionFromXY(ctx, x, y,OL_CARET), 0, 0, 0)) {
	    Cardinal        num_params = 0;
	    Paste(ctx, event, (String *) NULL, &num_params);
	}
    }
}				/* end of ReceivePasteMessage */
/*
 * SendPasteMessage
 *
 */

static void
SendPasteMessage(TextEditWidget ctx, Window window, Position x, Position y, int operation)
{
    XEvent          event;
    Status          Result;

    event.xclient.type = ClientMessage;
    event.xclient.display = XtDisplay(ctx);
    event.xclient.window = window;
    event.xclient.message_type = OlInternAtom(XtDisplay(ctx), OL_PASTE_MSG_NAME);
    event.xclient.format = 32;
    event.xclient.data.l[0] = (long) x;
    event.xclient.data.l[1] = (long) y;
    event.xclient.data.l[2] = (long) operation;

    Result = XSendEvent(XtDisplay(ctx), window, False, NoEventMask, &event);

/*
 * check the result and report failure !!!
 */

    XSync(XtDisplay(ctx), False);

}				/* end of SendPasteMessage */
/*
 * PopdownTextEditMenuCB (XtNpopdownCallback)
 */

static void
PopdownTextEditMenuCB(Widget w, XtPointer client_data, XtPointer call_data)
{
    XtDestroyWidget(w);
    return;
}				/* end of PopdownTextEditMenuCB */
/*
 * PopupTextEditMenu
 *
 *	note: the old code will destroy the menu when poping up the
 *		menu next time. This is OK in the general
 *		case but will fail in the following scenario:
 *			if an applic. (like olwsm) creates/destroys
 *			a popup window when poping it up/down.
 *			and that popup window contains at least
 *			one textedit. If you use the textedit menu
 *			at least once before poping it down. Most
 *			likely, the applic. will run into problems
 *			when pop up the popup window again and then
 *			use the textedit menu.
 *	      the new code will not defer this destroy process and
 *		this process will be done in the pop down call back.
 *		also get rid of the static definitions for MenuShell,
 *		UndoWidget, etc...
 */

static void
PopupTextEditMenu(TextEditWidget ctx)
{
    Widget          MenuShell;
    Widget          UndoWidget;
    Widget          CutWidget;
    Widget          CopyWidget;
    Widget          PasteWidget;
    Widget          DeleteWidget;

    TextEditPart   *text = &ctx->textedit;
    int             canedit = text->editMode == OL_TEXT_EDIT;
    TextBuffer     *textBuffer = text->textBuffer;

    {
	Arg             menuargs[3];
	Widget          pane;
	char *		mbyte_str;

	XtSetArg(menuargs[0], XtNmenuAugment, FALSE);
	XtSetArg(menuargs[1], XtNshellTitle, ctx->textedit.menuTitle);
	XtSetArg(menuargs[2], XtNtextFormat, ctx->primitive.text_format);
	MenuShell = XtCreatePopupShell("Edit", menuShellWidgetClass,
				       (Widget) ctx, menuargs, 3);
	/* do destroy when poping it down */
	XtAddCallback(MenuShell, XtNpopdownCallback, PopdownTextEditMenuCB, NULL);

	XtSetArg(menuargs[0], XtNmenuPane, &pane);
	XtGetValues(MenuShell, menuargs, 1);

	XtSetArg(menuargs[0], XtNmnemonic, ctx->textedit.undoMnemonic);
	XtSetArg(menuargs[1], XtNlabel, ctx->textedit.undoLabel);
	XtSetArg(menuargs[2], XtNtextFormat, ctx->primitive.text_format);
	UndoWidget = XtCreateManagedWidget("undogd",
				oblongButtonGadgetClass, pane, menuargs, 3);
	XtSetArg(menuargs[0], XtNmnemonic, ctx->textedit.cutMnemonic);
	XtSetArg(menuargs[1], XtNlabel, ctx->textedit.cutLabel);
	CutWidget = XtCreateManagedWidget("cutgd",
				oblongButtonGadgetClass, pane, menuargs, 3);
	XtSetArg(menuargs[0], XtNmnemonic, ctx->textedit.copyMnemonic);
	XtSetArg(menuargs[1], XtNlabel, ctx->textedit.copyLabel);
	CopyWidget = XtCreateManagedWidget("copygd",
				oblongButtonGadgetClass, pane, menuargs, 3);
	XtSetArg(menuargs[0], XtNmnemonic, ctx->textedit.pasteMnemonic);
	XtSetArg(menuargs[1], XtNlabel, ctx->textedit.pasteLabel);
	PasteWidget = XtCreateManagedWidget("pastegd",
				oblongButtonGadgetClass, pane, menuargs, 3);
	XtSetArg(menuargs[0], XtNmnemonic, ctx->textedit.deleteMnemonic);
	XtSetArg(menuargs[1], XtNlabel, ctx->textedit.deleteLabel);
	DeleteWidget = XtCreateManagedWidget("deletegd",
				oblongButtonGadgetClass, pane, menuargs, 3);

	XtAddCallback(UndoWidget, XtNselect, MenuUndo, (XtPointer) ctx);
	XtAddCallback(CutWidget, XtNselect, MenuCut, (XtPointer) ctx);
	XtAddCallback(CopyWidget, XtNselect, MenuCopy, (XtPointer) ctx);
	XtAddCallback(PasteWidget, XtNselect, MenuPaste, (XtPointer) ctx);
	XtAddCallback(DeleteWidget, XtNselect, MenuDelete, (XtPointer) ctx);
    }
    {
	OlStr	ins_str;
	OlStr	del_str;
	
	if (text->text_format == OL_SB_STR_REP) {
	    ins_str = (OlStr)textBuffer->insert.string;
	    del_str = (OlStr)textBuffer->deleted.string;
	} else {
	    ins_str = (OlGetTextUndoInsertItem((OlTextBufferPtr)textBuffer)).string;
	    del_str = (OlGetTextUndoDeleteItem((OlTextBufferPtr)textBuffer)).string;
	}
	XtSetSensitive(UndoWidget, (ins_str != (OlStr)NULL || del_str != (OlStr)NULL));
    }
    XtSetSensitive(CutWidget, text->selectEnd != text->selectStart && canedit);
    XtSetSensitive(CopyWidget, text->selectEnd != text->selectStart);
    XtSetSensitive(PasteWidget, canedit);	
    XtSetSensitive(DeleteWidget, text->selectEnd != text->selectStart && canedit);

    OlMenuPost(MenuShell);
}				/* end of PopupTextEditMenu */
/*
 * MenuUndo
 *
 */

static void
MenuUndo(Widget w, XtPointer client_data, XtPointer call_data)
{
    TextEditWidget  ctx = (TextEditWidget) client_data;

    UndoUpdate(ctx);

}				/* end of MenuUndo */
/*
 * MenuCut
 *
 */

static void
MenuCut(Widget w, XtPointer client_data, XtPointer call_data)
{
    TextEditWidget  ctx = (TextEditWidget) client_data;

	(void) OlTextEditCopySelection(ctx, True);

}				/* end of MenuCut */
/*
 * MenuCopy
 *
 */

static void
MenuCopy(Widget w, XtPointer client_data, XtPointer call_data)
{
    TextEditWidget  ctx = (TextEditWidget) client_data;

    (void) OlTextEditCopySelection(ctx, False);

}				/* end of MenuCopy */

/*
 * MenuPaste
 *
 */
static void
MenuPaste(Widget w, XtPointer client_data, XtPointer call_data)
{
    TextEditWidget  ctx = (TextEditWidget) client_data;
    Display        *dpy = XtDisplay(ctx);

	if(ctx->textedit.pre_edit &&   
		ctx->textedit.pre_edit_end > ctx->textedit.pre_edit_start)
		_DoImplicitCommit(ctx,NULL,TRUE);
    	XtGetSelectionValue((Widget) ctx, XA_CLIPBOARD(dpy),
			XA_STRING, Paste2, NULL,
			_XtLastTimestampProcessed((Widget) ctx));

}				/* end of MenuPaste */

/*
 * MenuDelete
 *
 */
static void
MenuDelete(Widget w, XtPointer client_data, XtPointer call_data)
{
    TextEditWidget  ctx = (TextEditWidget) client_data;

    	TextEditCheckAndInsert(ctx,
	   str_methods[ctx->textedit.text_format].StrEmptyString(),0);
}				/* end of MenuDelete */

/*
 * OlTextEditClearBuffer
 *
 * The \fIOlTextEditClearBuffer\fR function is used to delete all of the text
 * associated with the TextEdit widget \fIctx\fR.
 *
 * Return value:
 *
 * FALSE is returned if the widget supplied in not a TextEdit Widget or
 * if the clear operation fails; otherwise TRUE is returned.
 *
 * See also:
 *
 * OlTextEditUpdate(3)
 *
 * Synopsis:
 *
 *#include <buffutil.h>
 *#include <textbuff.h>
 *#include <Dynamic.h>
 *#include <TextEdit.h>
 * ...
 */

extern          Boolean
OlTextEditClearBuffer(TextEditWidget ctx)
{
    Boolean         retval;

    GetToken();
    if (!XtIsSubclass((Widget)ctx, textEditWidgetClass))
	retval = FALSE;
    else {
		OlStrRep rep = ctx->primitive.text_format;
	
		ctx->textedit.selectStart = 0;
		if (rep == OL_SB_STR_REP) 
	    		ctx->textedit.selectEnd = 
				LastTextBufferPosition(ctx->textedit.textBuffer);
		else {
			if(ctx->textedit.pre_edit == TRUE &&
				ctx->textedit.pre_edit_end >
				ctx->textedit.pre_edit_start)
					_DoImplicitCommit(ctx, NULL, TRUE);
			ctx->textedit.selectEnd = 
				OlLastTextBufferPosition((OlTextBufferPtr)
							ctx->textedit.textBuffer);
		}

    		retval = OlTextEditInsert(ctx, str_methods[rep].StrEmptyString(), 0);
    	}

    ReleaseToken();
    return (retval);
}				/* end of OlTextEditClearBuffer */
/*
 * OlTextEditCopyBuffer
 *
 * The \fIOlTextEditCopyBuffer\fR function is used to retrieve a copy
 * of the TextBuffer associated with the TextEdit Widget
 * \fIctx\fR.  The storage required for the copy is allocated
 * by this routine; it is the responsibility of the caller to
 * free this storage when appropriate.
 *
 * Return value:
 *
 * FALSE is returned if the widget supplied in not a TextEdit Widget or
 * if the buffer cannot be read; otherwise TRUE is returned.
 *
 * See also:
 *
 * OlTextEditReadSubString(3)
 *
 * Synopsis:
 *
 *#include <buffutil.h>
 *#include <textbuff.h>
 *#include <Dynamic.h>
 *#include <TextEdit.h>
 * ...
 */

extern Boolean
OlTextEditCopyBuffer(TextEditWidget ctx, char **buffer)
{
    Boolean         retval;

    GetToken();
    if (!XtIsSubclass((Widget)ctx, textEditWidgetClass))
	retval = FALSE;
    else {
	OlStrRep	rep = ctx->textedit.text_format;

	if (rep == OL_SB_STR_REP) {
	    TextBuffer *	textBuffer = ctx->textedit.textBuffer;
	    
	    if (TextBufferEmpty(textBuffer)) {
		retval  = (Boolean) TRUE;
		*buffer = (char *)XtNewString(((char *)""));
	    } else
		retval  = OlTextEditReadSubString(ctx, buffer, 0,
				  LastTextBufferPosition(textBuffer) - 1);
	} else {
	    OlTextBufferPtr	textBuffer = (OlTextBufferPtr)ctx->textedit.textBuffer;
	    
	    if (OlIsTextBufferEmpty(textBuffer)) {
		OlStr	str = str_methods[rep].StrEmptyString();
		int	nb  = str_methods[rep].StrNumBytes(str);
		
		retval  = (Boolean) TRUE;
		*buffer = str_methods[rep].StrCpy((OlStr)XtMalloc(nb), str);
	    } else {
			if(ctx->textedit.pre_edit == TRUE &&
				ctx->textedit.pre_edit_end >
				ctx->textedit.pre_edit_start)
					_DoImplicitCommit(ctx, NULL, TRUE);
			retval  = OlTextEditReadSubString(ctx, buffer, 0,
				  OlLastTextBufferPosition(textBuffer) - 1);
	    }
	}
    }
    ReleaseToken();
    return (retval);
} /* end of OlTextEditCopyBuffer */

/*
 * OlTextEditCopySelection
 *
 * The \fIOlTextEditCopySelection\fR function is used to Copy or Cut the current
 * selection in the TextEdit \fIctx\fR.  If no selection exists or if the
 * TextEdit cannot acquire the CLIPBOARD, FALSE is returned.
 * Otherwise the * selection is copied to the CLIPBOARD, then
 * if the \fIdelete\fR flag
 * is non-zero, the text is then deleted from the TextBuffer associated
 * with the TextEdit widget (i.e., a CUT operation is performed).  Finally
 * TRUE is returned.
 *
 * Return value:
 *
 * FALSE is returned if the widget supplied is not a TextEdit Widget or
 * if the operation fails; otherwise TRUE is returned.
 *
 * See also:
 *
 * OlTextEditUpdate(3), OlTextEditGetCursorPosition(3),
 * OlTextEditSetCursorPosition(3), OlTextEditReadSubString(3),
 * OlTextEditCopyBuffer(3)
 *
 * Synopsis:
 *
 *#include <buffutil.h>
 *#include <textbuff.h>
 *#include <Dynamic.h>
 *#include <TextEdit.h>
 * ...
 */

extern Boolean
OlTextEditCopySelection(TextEditWidget  ctx,
			int             delete)
{
    TextEditPart *  text;
    OlStr *        ptr;
    Boolean         retval;
    Display *       dpy;

    GetToken();
    text   = &ctx->textedit;
    ptr    = &text->clip_contents;
    retval = False;
    dpy    = XtDisplay(ctx);

    if (text->selectStart == text->selectEnd) {
	_OlBeepDisplay((Widget) ctx, 1);
    } else {
	if (!XtOwnSelection((Widget) ctx, XA_CLIPBOARD(dpy),
			    _XtLastTimestampProcessed((Widget)ctx),
			    ConvertClipboardOrDnD, LoseClipboardOrDnD, NULL))
	    OlWarning(dgettext(OlMsgsDomain,
			       "TextEdit: We didn't get the selection!\n"));
	else {
	    if (text->clip_contents != (OlStr)NULL) {
		XtFree(text->clip_contents);
		text->clip_contents = (OlStr)NULL;
	    }
	    OlTextEditReadSubString(ctx, (char **)ptr, text->selectStart, text->selectEnd - 1);
	    if (delete)
		TextEditCheckAndInsert(ctx,
		       str_methods[text->text_format].StrEmptyString(), 0);
	    retval = True;
	}
    }
    ReleaseToken();
    return (retval);
} /* end of OlTextEditCopySelection */

/*
 * OlTextEditGetCursorPosition
 *
 * The \fIOlTextEditGetCursorPosition\fR function is used to retrieve the
 * current selection \fIstart\fR and \fIend\fR and \fIcursorPosition\fR.
 *
 * Return value:
 *
 * FALSE is returned is the widget supplied is not a TextEdit Widget;
 * otherwise TRUE is returned.
 *
 * See also:
 *
 * OlTextEditSetCursorPosition(3)
 *
 * Synopsis:
 *
 *#include <buffutil.h>
 *#include <textbuff.h>
 *#include <Dynamic.h>
 *#include <TextEdit.h>
 * ...
 */

extern          Boolean
OlTextEditGetCursorPosition(TextEditWidget ctx, TextPosition *start, TextPosition *end, TextPosition *cursorPosition)
{
    Boolean         retval;

    GetToken();
    if (!XtIsSubclass((Widget)ctx, textEditWidgetClass))
	retval = FALSE;
    else {
	TextEditPart   *text = &ctx->textedit;

	*start = text->selectStart;
	*end = text->selectEnd;
	*cursorPosition = text->cursorPosition;
	retval = TRUE;
    }

    ReleaseToken();
    return (retval);

}				/* end of OlTextEditGetCursorPosition */
/*
 * OlTextEditGetLastPosition
 *
 * The \fIOlTextEditGetLastPosition\fR function is used to retrieve the
 * \fIposition\fR of the last character in the TextBuffer associated with
 * the TextEdit widget \fIctx\fR.
 *
 * Return value:
 *
 * FALSE is returned is the widget supplied is not a TextEdit Widget;
 * otherwise TRUE is returned.
 *
 * See also:
 *
 * OlTextEditGetCursorPosition(3)
 *
 * Synopsis:
 *
 *#include <buffutil.h>
 *#include <textbuff.h>
 *#include <Dynamic.h>
 *#include <TextEdit.h>
 * ...
 */
extern Boolean
OlTextEditGetLastPosition(
    TextEditWidget  ctx,
    TextPosition   *position)
{
    Boolean         retval;

    GetToken();
    if (!XtIsSubclass((Widget)ctx, textEditWidgetClass))
	retval = FALSE;
    else {
	if (ctx->textedit.text_format == OL_SB_STR_REP)
	    *position = LastTextBufferPosition(ctx->textedit.textBuffer);
	else
	    *position = OlLastTextBufferPosition((OlTextBufferPtr)ctx->textedit.textBuffer);
	retval = TRUE;
    }
    ReleaseToken();
    return (retval);
} /* end of OlTextEditGetLastPosition */

/*
 * OlTextEditInsert
 *
 * The \fIOlTextEditInsert\fR function is used to insert a \fIbuffer\fR
 * containing \fIlength\fR bytes in the TextBuffer associated with
 * the TextEdit widget \fIctx\fR.  The inserted text replaces the
 * current (if any) selection.
 *
 * Return Value:
 *
 * FALSE is returned if the widget supplied is not a TextEdit Widget or
 * if the insert operation fails; otherwise TRUE is returned.
 *
 * See also:
 *
 * OlTextEditGetCursorPosition(3)
 *
 * Synopsis:
 *
 *#include <buffutil.h>
 *#include <textbuff.h>
 *#include <Dynamic.h>
 *#include <TextEdit.h>
 * ...
 */
extern Boolean
OlTextEditInsert(
    TextEditWidget  ctx,
    char 	    *buf,
    int             length)
{
    TextEditPart   *text;
    TextBuffer     *textBuffer;
    TextLocation    startloc;
    TextLocation    endloc;
    OlTextModifyCallData call_data; 
    Boolean         retval;
    OlStr	    buffer;

    GetToken();
    text = &ctx->textedit;
    textBuffer = text->textBuffer;
    retval = FALSE;
    buffer = (OlStr)buf;

    call_data.ok               = TRUE;
    call_data.current_cursor   = text->cursorPosition;
    call_data.select_start     = text->selectStart;
    call_data.select_end       = text->selectEnd;
    call_data.new_cursor       =
    call_data.new_select_start =
    call_data.new_select_end   = text->cursorPosition + length -
	                         ((text->cursorPosition == text->selectEnd) ?
                                  (text->selectEnd - text->selectStart) : 0);
    call_data.text             = (String)buffer;
    call_data.text_length      = length;

    XtCallCallbacks((Widget) ctx, XtNmodifyVerification, &call_data);

    if (call_data.ok == TRUE) {
	if (text->text_format == OL_SB_STR_REP) {
	    startloc = LocationOfPosition(textBuffer, text->selectStart);
	    endloc = LocationOfPosition(textBuffer, text->selectEnd);
	    retval = EDIT_SUCCESS ==
		ReplaceBlockInTextBuffer(textBuffer,
					 &startloc, &endloc, buffer, 
					(TextUpdateFunction)UpdateDisplay,
					 (caddr_t) ctx);
	} else {
	    OlLocationOfPosition((OlTextBufferPtr)textBuffer, text->selectStart, &startloc);
	    OlLocationOfPosition((OlTextBufferPtr)textBuffer, text->selectEnd, &endloc);
	    retval = EDIT_SUCCESS ==
		OlReplaceBlockInTextBuffer((OlTextBufferPtr)textBuffer,
					   &startloc, &endloc, buffer, 
					(TextUpdateFunction)UpdateDisplay,
					   (caddr_t) ctx);
	}
    }
    ReleaseToken();
    return (retval);
} /* end of OlTextEditInsert */

/*
 * OlTextEditMoveDisplayPosition - This routine allows display traversal
 * within the textEdit's view area. It takes care of scroll updates.
 */
extern void
OlTextEditMoveDisplayPosition(TextEditWidget ctx, OlInputEvent move_type)
{
 
   GetToken();
   if (!XtIsSubclass((Widget)ctx, textEditWidgetClass)) {
    	ReleaseToken();
        return;
   } else {
      XEvent    dummy;
 
      switch (move_type) {
         case OL_SCROLLUP:
         case OL_SCROLLDOWN:
         case OL_PAGEUP:
         case OL_PAGEDOWN:
         case OL_HOME:
         case OL_END:
	    if(ctx->textedit.pre_edit == TRUE &&
				ctx->textedit.pre_edit_end >
				ctx->textedit.pre_edit_start)
					_DoImplicitCommit(ctx, NULL, TRUE);
            _MoveDisplayPosition(ctx, (XEvent *) &dummy, move_type, 0);
            break;
         default:
 	    OlWarning(dgettext(OlMsgsDomain,
		"TextEdit: Bad OlInputEvent in OlTextEditMoveDisplayPosition\n"));
	    break;
       }
   }
   ReleaseToken();
}

/* Wrapper around OlTextEditInsert() ,which checks the current edit-mode
 * before issuing call. All user edits on TextEdit go thru this.
 * Programmatic edits bypass this check invoking OlTextEditInsert()
 * directly ... as per bug #1052482
 */
static          Boolean
TextEditCheckAndInsert(TextEditWidget ctx, OlStr buffer, int length)
{
    TextEditPart   *text = &ctx->textedit;

    if (text->editMode == OL_TEXT_READ) {
	_OlBeepDisplay((Widget) ctx, 1);
	return False;
    } else {
	if ( OlTextEditInsert(ctx, buffer, length) )
	    return True;
	else
	    return False;
    }
}

static void _DoImplicitCommit(
			TextEditWidget ctx, 
			OlVirtualEvent ve,
			Boolean 	conversion_on)
{

	OlStr	committed_string = NULL;
	int	len = 0;
	OlEditMode save_mode = ctx->textedit.editMode;
	int space_needed = 0;
	int i = 0;
	int	num_chars = 0;
	OlStrRep  rep	= ctx->primitive.text_format;
	TextPosition start = ctx->textedit.pre_edit_start;
	TextPosition end  = ctx->textedit.pre_edit_end;
	TextPosition sstart = ctx->textedit.selectStart;
	TextPosition send = ctx->textedit.selectEnd;
	OlStr empty_string;
	XIC	ic;
	OlInputEvent ol_event = (ve != NULL ? 
				(OlInputEvent)ve->virtual_name: 
				OL_UNKNOWN_INPUT);

	if(ctx->textedit.ic_id != NULL)
		ic = OlXICOfIC(ctx->textedit.ic_id);
	else
		OlError(dgettext(OlMsgsDomain,
				"Implicit Commit without an IC\n"));

	switch(rep) {
		case OL_MB_STR_REP:
			committed_string = (OlStr)XmbResetIC(ic);
			len = (committed_string != NULL ? strlen(committed_string) : 0);
			num_chars = (committed_string != NULL ? 
				(*str_methods[rep].StrNumChars)(committed_string) : 0);
			committed_string = (committed_string == NULL ?  
						(OlStr) "": committed_string);
			break;
		case  OL_WC_STR_REP:
			committed_string = (OlStr)XwcResetIC(ic);
			num_chars = len = 
				(committed_string != NULL ? 
				(int)wslen((wchar_t *)committed_string) : (int)0);
			committed_string = (committed_string == NULL ?  
						(OlStr) L"": committed_string);
			break;
		default:
			OlError(dgettext(OlMsgsDomain,
				"TextEdit: Invalid text format encountered\n"));
			break;
	} /* end of switch */

	switch(ol_event) { /* there may be use for this switch */
		default:
			ctx->textedit.editMode = OL_TEXT_EDIT;
			/* set preedit string to zero length */
			ctx->textedit.pre_edit_start =
			ctx->textedit.pre_edit_end = ctx->textedit.cursorPosition;

			/* Make selectSatrt and selectEnd equal
			   to the saved preedit start and preedit end.
			   This lets us replace the preedit string with the
			   committed string. We want to make the implicit
			   operation an atomic operation, that is why we
				are not leaving it upto
				the preedit draw callback to delete the preedit stuff.
				Also, by setting the preedit string length to zero,
				(see above) we fake the draw call back into
				not doing anything when the input method
				tells it to delete the preedit string. */

			ctx->textedit.selectStart = start;
			ctx->textedit.selectEnd = 
			ctx->textedit.cursorPosition = end;

			if(num_chars != (end - start) && ve != NULL) {
			XEvent *event = ve->xevent;
			TextPosition position = 0;
			XRectangle rect;

			switch(event->type) {

				/* If we have a button press or a keypress
				   event, maintain the relationship between
				   the character the user pressed his pointer on
				   and its updated x and y position. The position
				   of the character may change due to the replacing
				   of the preedit string by the committed string.
				   There is no guarantee  ithat the committed
				   string is  the same width in pixels as the 
					preedit string */

			case ButtonPress:
				{
				PositionType type = OL_CARET;
				if((OlInputEvent)ve->virtual_name == OL_ADJUST)
					type = OL_CHAR;
	    			position = 
					_PositionFromXY(ctx, 
					event->xbutton.x, event->xbutton.y,type);
				position += (num_chars - (end - start)) ;
				TextEditCheckAndInsert(ctx,committed_string,len);
				rect = _RectFromPositions(ctx, position -1, 
								position);
				event->xbutton.x = rect.x + rect.width + 1;
				}
				break;
			case KeyPress:
				{
				PositionType type = OL_CARET;
				if((OlInputEvent)ve->virtual_name == OL_ADJUST)
					type = OL_CHAR;
				position = 
               				_PositionFromXY(ctx, 
                                         event->xkey.x, event->xkey.y,type);
				position += (num_chars - (end - start)) ;
				TextEditCheckAndInsert(ctx,committed_string,len);
				rect = _RectFromPositions(ctx, position -1, 
								position);
				event->xkey.x = rect.x + rect.width + 1;
				}
                                break;
			default:
				TextEditCheckAndInsert(ctx,committed_string,len);
				break;
			} /* switch */

			} else 
				TextEditCheckAndInsert(ctx,committed_string,len);	

			ctx->textedit.pre_edit_start =
			ctx->textedit.pre_edit_end = 
					ctx->textedit.cursorPosition; 

			/* Reset the select start and end to saved values */
			ctx->textedit.editMode = save_mode;
			ctx->textedit.selectStart =  sstart;
			ctx->textedit.selectEnd = send;
			break;
	} /* end of switch */

	if(num_chars > 0)
		XFree((char *)committed_string);

	/* Turn conversion on: atleast if it is a sun input method */
#ifdef sun
	if(conversion_on)
		XSetICValues(ic,XNExtXimp_Conversion, XIMEnable, NULL);
	else
#endif
		ctx->textedit.pre_edit = FALSE;
} /* _DoImplicitCommit */

/*
 * OlTextEditPaste
 *
 * The \fIOlTextEditPaste\fR function is used to programmatically paste
 * the contents of the CLIPBOARD into the TextEdit widget \fIctx\fR.
 * The current (if any) selection is replaced by the contents of the
 * CLIPBOARD,
 *
 * Return value:
 *
 * FALSE is returned if the widget supplied in not a TextEdit Widget;
 * otherwise TRUE is returned.
 *
 * See also:
 *
 * OlTextEditCopySelection(3)
 *
 * Synopsis:
 *
 *#include <buffutil.h>
 *#include <textbuff.h>
 *#include <Dynamic.h>
 *#include <TextEdit.h>
 * ...
 */

extern Boolean
OlTextEditPaste(TextEditWidget ctx)
{
    Boolean         retval = TRUE;

    GetToken();
    if (!XtIsSubclass((Widget)ctx, textEditWidgetClass))
	retval = FALSE;
    else {
	Cardinal        num_params = 0;

	if(ctx->textedit.pre_edit == TRUE &&
		ctx->textedit.pre_edit_end >
		ctx->textedit.pre_edit_start)
			_DoImplicitCommit(ctx, NULL, TRUE);
	Paste(ctx, (XEvent *) NULL, (String *) NULL, &num_params);
    }
    ReleaseToken();
    return (retval);
} /* end of OlTextEditPaste */

/*
 * OlTextEditReadSubString
 *
 * The \fIOlTextEditReadSubString\fR function is used to retreive a copy
 * of a substring from the TextBuffer associated with the TextEdit Widget
 * \fIctx\fR between positions \fIstart\fR through \fIend\fR inclusive.
 * The storage required for the copy is allocated by this routine; it
 * is the responsibility of the caller to free this storage when appropriate.
 *
 * Return value:
 *
 * FALSE is returned if the widget supplied in not a TextEdit Widget;
 * otherwise TRUE is returned.
 *
 * See also:
 *
 * OlTextEditCopyBuffer(3)
 *
 * Synopsis:
 *
 *#include <buffutil.h>
 *#include <textbuff.h>
 *#include <Dynamic.h>
 *#include <TextEdit.h>
 * ...
 */
extern Boolean
OlTextEditReadSubString(
    TextEditWidget  ctx,
    char **buf,
    TextPosition    start,
    TextPosition    end)
{
    Boolean         retval;
    OlStr *buffer = (OlStr *)buf;

    GetToken();
    if (!XtIsSubclass((Widget)ctx, textEditWidgetClass))
	retval = FALSE;
    else {
	TextLocation    startloc;
	TextLocation    endloc;

	if (start > end || buffer == NULL)
	    retval = FALSE;
	else {
	    if (ctx->textedit.text_format == OL_SB_STR_REP) {
		TextBuffer     *textBuffer = ctx->textedit.textBuffer;
		
		startloc = LocationOfPosition(textBuffer, start);
		endloc = LocationOfPosition(textBuffer, end);
		if (startloc.buffer == (OlStr)NULL || endloc.buffer == (OlStr)NULL)
		    retval = FALSE;
		else {
		    *buffer = GetTextBufferBlock(textBuffer, startloc, endloc);
		    retval = (*buffer != (OlStr)NULL);
		}
	    } else {
		OlTextBufferPtr	textBuffer = (OlTextBufferPtr)ctx->textedit.textBuffer;

		OlLocationOfPosition(textBuffer, start, &startloc);
		OlLocationOfPosition(textBuffer, end, &endloc);
		if (startloc.buffer == NULL || endloc.buffer == NULL)
		    retval = FALSE;
		else {
		    *buffer = OlGetTextBufferBlock(textBuffer, &startloc, &endloc);
		    retval = (*buffer != (OlStr)NULL);
		}
	    }
	}
    }
    ReleaseToken();
    return (retval);
} /* end of OlTextEditReadSubString */

/*
 * OlTextEditRedraw
 *
 * The \fIOlTextEditRedraw\fR function is used to force a complete refresh
 * of the TextEdit widget display.  This routine does nothing if the
 * TextEdit widget is not realized or if the update state is set to FALSE.
 *
 * Return value:
 *
 * FALSE is returned if the widget supplied in not a TextEdit Widget or
 * if the widget is not realized or if the update state is FALSE;
 * otherwise TRUE is returned.
 *
 * See also:
 *
 * OlTextEditUpdate(3)
 *
 * Synopsis:
 *
 *#include <buffutil.h>
 *#include <textbuff.h>
 *#include <Dynamic.h>
 *#include <TextEdit.h>
 * ...
 */
extern Boolean
OlTextEditRedraw(TextEditWidget ctx)
{
    Boolean         retval;
    XRectangle      rect;

    GetToken();
    if (!XtIsSubclass((Widget)ctx, textEditWidgetClass))
	retval = FALSE;
    else {
	if (XtIsRealized((Widget) ctx) && ctx->textedit.updateState) {
	    TextEditPart * text = &ctx->textedit;
	
	    if (text->cursor_state == OlCursorOn)
		_TurnTextCursorOff(ctx);
	    rect.x = 0;
	    rect.y = 0;
	    rect.width = ctx->core.width;
	    rect.height = ctx->core.height;
	    XClearArea(XtDisplay(ctx), XtWindow(ctx),
		       rect.x, rect.y, rect.width, rect.height, FALSE);
	    _DisplayText(ctx, &rect);
	    retval = TRUE;
	} else
	    retval = FALSE;
    }
    ReleaseToken();
    return (retval);
} /* end of OlTextEditRedraw */

/*
 * OlTextEditSetCursorPosition
 *
 * The \fIOlTextEditSetCursorPosition\fR function is used to change the
 * current selection \fIstart\fR and \fIend\fR and \fIcursorPosition\fR.
 * The function does NOT check (for efficiency) the validity of
 * the positions.  If invalid values are given results are unpredictable.
 * The function attempts to ensure that the cursorPosition is visible.
 *
 * Return value:
 *
 * FALSE is returned is the widget supplied is not a TextEdit Widget;
 * otherwise TRUE is returned.
 *
 * See also:
 *
 * OlTextEditGetCursorPosition(3)
 *
 * Synopsis:
 *
 *#include <buffutil.h>
 *#include <textbuff.h>
 *#include <Dynamic.h>
 *#include <TextEdit.h>
 * ...
 */

extern          Boolean
OlTextEditSetCursorPosition(TextEditWidget ctx, TextPosition start, TextPosition end, TextPosition cursorPosition)
{
    Boolean         retval;

    GetToken();
    if (!XtIsSubclass((Widget)ctx, textEditWidgetClass)
	|| (start != cursorPosition && end != cursorPosition)
	|| start == -1 || end == -1 || cursorPosition == -1)
	retval = FALSE;
    else {
	TextEditPart   *text = &ctx->textedit;

	if(ctx->textedit.pre_edit == TRUE &&
		ctx->textedit.pre_edit_end >
		ctx->textedit.pre_edit_start) 
			_DoImplicitCommit(ctx, NULL, TRUE);
	_MoveSelection(ctx, cursorPosition, start, end, 7);
	ctx->textedit.selectMode = 0;

	retval = TRUE;
    }

    ReleaseToken();
    return (retval);

}				/* end of OlTextEditSetCursorPosition */

/*
 * OlTextEditTextBuffer
 *
 * The \fIOlTextEditTextBuffer\fR function is used to retrieve the
 * TextBuffer pointer associated with the TextEdit widget \fIctx\fR.
 * This buffer exists only when XtNtextFormat is OL_SB_STR_REP.
 * This pointer can be used to access the facilities provided by
 * the Text Buffer Utilities module.
 * The function returns NULL if XtNtextFormat is not OL_SB_STR_REP.
 *
 * See also:
 *
 * Text Buffer Utilities(3)
 *
 * Synopsis:
 *
 *#include <buffutil.h>
 *#include <textbuff.h>
 *#include <Dynamic.h>
 *#include <TextEdit.h>
 * ...
 */

extern TextBuffer *
OlTextEditTextBuffer(TextEditWidget ctx)
{
TextBuffer *retval;

    GetToken();
    if (XtIsSubclass((Widget)ctx, textEditWidgetClass) &&
	(ctx->textedit.text_format == OL_SB_STR_REP)) {
	retval =  (ctx->textedit.textBuffer);
    	ReleaseToken();
	return retval;
    } else {
    	ReleaseToken();
	return((TextBuffer *)NULL);
    }
} /* end of OlTextEditTextBuffer */
    
/*
 * OlTextEditOlTextBuffer
 *
 * The \fIOlTextEditOlTextBuffer\fR function is used to retrieve the
 * OlTextBufferPtr associated with the TextEdit widget \fIctx\fR. This buffer
 * exists only when the value of XtNtextFormat for the widget is not OL_SB_STR_REP.
 * This pointer can be used to access the facilities provided by the OlText Buffer
 * Utilities module.
 * In case, XtNtextFormat is OL_SB_STR_REP, this function returns a NULL pointer.
 *
 * See also:
 *
 * OlText Buffer Utilities(3)
 *
 * Synopsis:
 *
 *#include <buffutil.h>
 *#include <Oltextbuff.h>
 *#include <Dynamic.h>
 *#include <TextEdit.h>
 * ...
 */
extern OlTextBufferPtr
OlTextEditOlTextBuffer(TextEditWidget ctx)
{
OlTextBufferPtr retval;

    GetToken();
    if (XtIsSubclass((Widget)ctx, textEditWidgetClass) &&
	(ctx->textedit.text_format != OL_SB_STR_REP)) {
	retval =  ((OlTextBufferPtr)ctx->textedit.textBuffer);
    	ReleaseToken();
	return retval;
    } else {
    	ReleaseToken();
	return((OlTextBufferPtr)NULL);
    }

} /* end of OlTextEditOlTextBuffer */

/*
 * OlTextEditUpdate
 *
 * The \fIOlTextEditUpdate\fR function is used to set the \fIupdateState\fR
 * of a TextEdit Widget.  Setting the state to FALSE turns screen update
 * off; setting the state to TRUE turns screen updates on and refreshes
 * the display.
 * .P
 *
 * Return value:
 *
 * FALSE is returned is the widget supplied is not a TextEdit Widget;
 * otherwise TRUE is returned.
 *
 * See also:
 *
 * OlTextEditRedraw(3)
 *
 * Synopsis:
 *
 *#include <buffutil.h>
 *#include <textbuff.h>
 *#include <Dynamic.h>
 *#include <TextEdit.h>
 * ...
 */

extern          Boolean
OlTextEditUpdate(TextEditWidget ctx, Boolean state)
{
    Boolean         retval;

    GetToken();
    if (!XtIsSubclass((Widget)ctx, textEditWidgetClass))
	retval = FALSE;
    else {
	ctx->textedit.updateState = state;
	if (ctx->textedit.updateState)
	    OlTextEditRedraw(ctx);
	else
	    _TurnTextCursorOff(ctx);
	retval = TRUE;
    }

    ReleaseToken();
    return (retval);

}				/* end of OlTextEditUpdate */
/*
 * OlTextEditResize
 *
 * The \fIOlTextEditResize\fR procedure is used to request a size change
 * of a TextEdit widget \fIctx\fR to display \fIrequest_linesVisible\fR and
 * \fIrequest_charsVisible\fR.  This routine calculates the appropriate
 * geometry, then requests a resize of its parent.  Note: the new size may
 * or may not be honored by the widget'''s parent, therefor the outcome is
 * non-deterministic.
 *
 * Synopsis:
 *
 *#include <buffutil.h>
 *#include <textbuff.h>
 *#include <Dynamic.h>
 *#include <TextEdit.h>
 * ...
 */

extern void
OlTextEditResize(TextEditWidget ctx, int request_linesVisible, int request_charsVisible)
{
    TextEditPart   *text;
    Dimension       request_height;
    Dimension       request_width;

    GetToken();
    text = &ctx->textedit;
    request_height = text->lineHeight * request_linesVisible +
	PAGE_T_GAP(ctx) + PAGE_B_GAP(ctx);
    request_width = ENSPACE(ctx) * request_charsVisible +
	PAGE_L_GAP(ctx) + PAGE_R_GAP(ctx);
    request_width = VORDER(request_width, ctx->core.width);
    request_height = MAX(text->lineHeight, (int) request_height);

    if (text->vsb != NULL) {	/* must be in a scrolled window */
	ScrolledWindowWidget sw = (ScrolledWindowWidget) XtParent(XtParent(ctx));
	OlSWGeometries  geometries;

	geometries = GetOlSWGeometries(sw);
	ResizeScrolledWindow(ctx, sw, &geometries, request_width, request_height, TRUE);
    } else {
	if (XtGeometryAlmost == XtMakeResizeRequest((Widget) ctx,
	request_width, request_height, &ctx->core.width, &ctx->core.height))
	    XtMakeResizeRequest((Widget) ctx, ctx->core.width, ctx->core.height, NULL, NULL);
    }

    ReleaseToken();
} /* end of OlTextEditResize */

static int
AdjustPreeditStartAndEndPos(TextEditWidget ctx, OlStr buffer)
{
	OlTextBufferPtr mltbuf = (OlTextBufferPtr)ctx->textedit.textBuffer;
	OlStrRep rep = ctx->primitive.text_format;
	int num_chars = 0;

	num_chars = (*str_methods[rep].StrNumChars)(buffer); 
	ctx->textedit.cursorPosition =  
	ctx->textedit.selectStart =
	ctx->textedit.selectEnd = ctx->textedit.pre_edit_start; 
	num_chars = (num_chars == -1 ? 0 : num_chars); 
	ctx->textedit.pre_edit_start += num_chars;
	ctx->textedit.pre_edit_end += num_chars;

	return num_chars;
}

/*
 * IgnorePropertyNotify
 *
 */
static void
IgnorePropertyNotify(Widget w, XEvent *event, String *params, Cardinal *num_params)
{


/*
 *
 *           this space intentionally left blank
 *
 */



} /* end of IgnorePropertyNotify */

extern int
_FontSetAscent(TextEditWidget tew)
{
    if (tew->textedit.text_format == OL_SB_STR_REP)
	return ((XFontStruct *)tew->primitive.font)->ascent;
    else
	return (-(XExtentsOfFontSet((XFontSet)tew->primitive.font))->max_logical_extent.y);
}

extern int
_FontSetDescent(TextEditWidget tew)
{
    if (tew->textedit.text_format == OL_SB_STR_REP)
	return ((XFontStruct *)tew->primitive.font)->descent;
    else {
	XRectangle r  = (XExtentsOfFontSet((XFontSet)tew->primitive.font))->max_logical_extent;
	
	return(r.height + r.y); /* ascent = -r.y */
    }
}

extern int
_FontSetWidth(TextEditWidget tew)
{
    if (tew->textedit.text_format == OL_SB_STR_REP)
	return ((XFontStruct *)tew->primitive.font)->max_bounds.width;
    else
	return (XExtentsOfFontSet((XFontSet)tew->primitive.font))->max_logical_extent.width;
}

extern int
_FontSetEnspace(TextEditWidget tew)
{
	int enspace;

    if (tew->textedit.text_format == OL_SB_STR_REP) {
		XFontStruct * font = (XFontStruct *)tew->primitive.font;
	
		if ((enspace = _CharWidth('n', font, font->per_char,
								 font->min_char_or_byte2, 
								 font->max_char_or_byte2, 
								 font->max_bounds.width)) == 0)
			enspace = font->max_bounds.width;
    } else {
		XFontSet font = tew->primitive.font;

		if ((enspace = _CharWidthWC(L'n', font)) == 0)
			enspace = (XExtentsOfFontSet(font))->max_logical_extent.width;
	}
	return enspace;
}
