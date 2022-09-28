#pragma       ident   "@(#)FontCh.c 1.12     97/03/26 SMI"        /* OLIT */

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

/*
 *---------------------------------------------------------------------------
 * Description:
 *	This file contains the implementation of the FontChooser Widget
 *---------------------------------------------------------------------------
 */

#include <ctype.h>
#include <widec.h>
#include <libintl.h>
#include <stdio.h>
#include <unistd.h>

#include <X11/IntrinsicP.h>
#include <X11/StringDefs.h>
#include <X11/Xatom.h>
#include <Xol/OpenLookP.h>
#include <Xol/Converters.h>
#include <Xol/ManagerP.h>
#include <Xol/RubberTilP.h>

#include <Xol/OWFsetDB.h>
#include <Xol/FontDB.h>
#include <Xol/FontChP.h>
#include <Xol/FCImplI.h>

#include <Xol/OblongButt.h>
#include <Xol/Caption.h>
#include <Xol/ListPane.h>
#include <Xol/StaticText.h>
#include <Xol/TextField.h>

/*
 *---------------------------------------------------------------------------
 * Private function prototypes
 *---------------------------------------------------------------------------
 */

static Boolean
CvtLocaleStringToString(Display *display, 
			XrmValue *args, 
			Cardinal *num_args,
			XrmValue *from,
			XrmValue *to,
			XtPointer *converter_data);

static void	ob_applyCB(
			Widget		w,
			XtPointer	client_data,
			XtPointer	call_data);

static void	ob_revertCB(
			Widget		w,
			XtPointer	client_data,
			XtPointer	call_data);

static void	ob_cancelCB(
			Widget		w,
			XtPointer	client_data,
			XtPointer	call_data);

static void	init_pref_list(FontChooserWidget fcw);

static void	copy_olstring(OlStrRep tf, OlStr * copied_str_ret);
/*
 *---------------------------------------------------------------------------
 * Class method prototypes
 *---------------------------------------------------------------------------
 */

static void	ClassInitialize(void);
					/* Initializes widget class stuff */

static void	Initialize(		/* Initializes widget part */
			   Widget	request,
			   Widget	new,
			   ArgList	args,
			   Cardinal *	num_args);

static void	Destroy(Widget w);	/* Destroy this */

static Boolean	SetValues(
			  Widget	current,
			  Widget	request,
			  Widget	new,
			  ArgList	args,
			  Cardinal *	num_args);

static void	InsertChild (Widget w);

/*
 *---------------------------------------------------------------------------
 * Action prototypes
 *---------------------------------------------------------------------------
 */

/*
 *---------------------------------------------------------------------------
 * Define global/static variables and #defines, and
 * declare externally referenced variables
 *---------------------------------------------------------------------------
 */
XtConvertArgRec convertOlStrListArgs[]= {
    {XtResourceString, (String)XtNtextFormat, sizeof(OlStrRep)}
};

/*
 *---------------------------------------------------------------------------
 * Define Resource list associated with the Widget Instance
 *---------------------------------------------------------------------------
 */
/* For the pixmap */
static Pixmap	def_bg_pixmap = None;

#define OFFSET(FIELD) XtOffsetOf(FontChooserRec, FIELD)
#define FC_OFFSET(FIELD) XtOffsetOf(FontChooserRec, font_chooser.FIELD)

static XtResource resources[] = {
    /*
     * NOTE: textFormat must be the first resource
     */
  {
    XtNtextFormat, XtCTextFormat, 
    XtROlStrRep, sizeof(OlStrRep), FC_OFFSET(text_format),
    XtRCallProc, (XtPointer) _OlGetDefaultTextFormat,
  }, {
    XtNfont, XtCFont,
    XtROlFont, sizeof(OlFont), FC_OFFSET(font),
    XtRString, XtDefaultFont,
  },  {
    XtNborderWidth, XtCBorderWidth,
    XtRDimension, sizeof(Dimension), OFFSET(core.border_width),
    XtRImmediate, (XtPointer) 0,
  }, {
    XtNbackground, XtCBackground,
    XtRPixel, sizeof(Pixel), OFFSET(core.background_pixel),
    XtRString, (XtPointer) XtDefaultBackground
  }, {
    XtNfontColor, XtCFontColor,
    XtRPixel, sizeof(Pixel), FC_OFFSET(font_color), 
    XtRString, XtDefaultForeground,
  }, {
    XtNforeground, XtCForeground,
    XtRPixel, sizeof(Pixel), FC_OFFSET(foreground),
    XtRString, XtDefaultForeground,
  }, {
    XtNinputFocusColor, XtCInputFocusColor,
    XtRPixel, sizeof(Pixel), FC_OFFSET(input_focus_color),
    XtRCallProc, (XtPointer)_OlGetDefaultFocusColor
  }, {
    XtNscale, XtCScale,
    XtROlScale, sizeof(int), FC_OFFSET(scale),
    XtRImmediate, (XtPointer) OL_DEFAULT_POINT_SIZE,
  }, {
    XtNfontSearchSpec, XtCFontSearchSpec,
    XtRString, sizeof(String), FC_OFFSET(font_search_spec),
    XtRLocaleString,
    #ifndef  XGETTEXT
    	(XtPointer) "*-*-*-*-*-*-*-*-*-*-*-*-*-*-*",
    #else
    	dgettext(OlMsgsDomain, "*-*-*-*-*-*-*-*-*-*-*-*-*-*-*"),
    #endif
  }, {
    XtNcharsetInfo, XtCCharsetInfo,
    XtRString, sizeof(String), FC_OFFSET(charset_info),
    XtRLocaleString,
    #ifndef  XGETTEXT
    	(XtPointer) "iso8859-1",
    #else
    	dgettext(OlMsgsDomain, "iso8859-1"),
    #endif
  }, {
    XtNinitialFontName, XtCInitialFontName,
    XtRString, sizeof(String), FC_OFFSET(initial_font_name),
    XtRImmediate, (XtPointer) NULL,
  }, {
    XtNattributeListHeight, XtCAttributeListHeight,
    XtRDimension, sizeof(Dimension), FC_OFFSET(attribute_list_height),
    XtRImmediate, (XtPointer) 6,
  }, {
    XtNmaximumPointSize, XtCMaximumPointSize,
    XtRCardinal, sizeof(Cardinal), FC_OFFSET(maximum_point_size),
    XtRString, (XtPointer) "99"
  }, {
    XtNpreferredPointSizes, XtCPreferredPointSizes,
    XtRString, sizeof(String), FC_OFFSET(preferred_point_sizes),
    XtRLocaleString,
    #ifndef  XGETTEXT
    	(XtPointer) "8 10 12 14 18 24",
    #else
    	dgettext(OlMsgsDomain, "8 10 12 14 18 24"),
    #endif
  }, {
    XtNpreviewPresent, XtCPreviewPresent,
    XtRBoolean, sizeof(Boolean), FC_OFFSET(preview_present),
    XtRImmediate, (XtPointer) TRUE,
  }, {
    XtNextensionArea, XtCExtensionArea,
    XtRPointer, sizeof(Widget), FC_OFFSET(extension_area),
    XtRPointer, NULL,
  }, {
    XtNpreviewSwitchLabel, XtCPreviewSwitchLabel,
    XtROlStr, sizeof(OlStr), FC_OFFSET(preview_switch_label),
    XtRLocaleString,
    #ifndef  XGETTEXT
    	(XtPointer) "Preview:",
    #else
    	dgettext(OlMsgsDomain, "Preview:"),
    #endif
  }, {
    XtNpreviewSwitchOnLabel, XtCPreviewSwitchOnLabel,
    XtROlStr, sizeof(OlStr), FC_OFFSET(preview_switch_on_label),
    XtRLocaleString,
    #ifndef  XGETTEXT
    	(XtPointer) "On",
    #else
    	dgettext(OlMsgsDomain, "On"),
    #endif
  }, {
    XtNpreviewSwitchOffLabel, XtCPreviewSwitchOffLabel,
    XtROlStr, sizeof(OlStr), FC_OFFSET(preview_switch_off_label),
    XtRLocaleString,
    #ifndef  XGETTEXT
    	(XtPointer) "Off",
    #else
    	dgettext(OlMsgsDomain, "Off"),
    #endif
  }, {
    XtNnoPreviewText, XtCNoPreviewText,
    XtROlStr, sizeof(OlStr), FC_OFFSET(no_preview_text),
    XtRLocaleString,
    #ifndef  XGETTEXT
    	(XtPointer) "The preview is turned off",
    #else
    	dgettext(OlMsgsDomain, "The preview is turned off"),
    #endif
  }, {
    XtNpreviewText, XtCPreviewText,
    XtROlStr, sizeof(OlStr), FC_OFFSET(preview_text),
    XtRLocaleString,
    #ifndef  XGETTEXT
    	(XtPointer) "%T %S %s",
    #else
    	dgettext(OlMsgsDomain, "%T %S %s"),
    #endif
  }, {
    XtNpreviewHeight, XtCPreviewHeight,
    XtRDimension, sizeof(Dimension), FC_OFFSET(preview_height),
    XtRImmediate, (XtPointer) 0,
  },  {
    XtNpreviewBorderWidth, XtCBorderWidth,
    XtRDimension, sizeof(Dimension), FC_OFFSET(preview_border_width),
    XtRImmediate, (XtPointer) 2,
  }, {
    XtNpreviewBackground, XtCBackground,
    XtRPixel, sizeof(Pixel), FC_OFFSET(preview_background),
    XtRString, (XtPointer) "White",
  }, {
    XtNpreviewFontColor, XtCFontColor,
    XtRPixel, sizeof(Pixel), FC_OFFSET(preview_font_color), 
    XtRString, "Black",
  }, {
    XtNpreviewForeground, XtCForeground,
    XtRPixel, sizeof(Pixel), FC_OFFSET(preview_foreground),
    XtRString, XtDefaultForeground,
  }, {
    XtNapplyCallback, XtCCallback,
    XtRCallback, sizeof(XtCallbackProc), FC_OFFSET(apply_callback),
    XtRCallback, (XtPointer) NULL,
  }, {
    XtNrevertCallback, XtCCallback,
    XtRCallback, sizeof(XtCallbackProc), FC_OFFSET(revert_callback),
    XtRCallback, (XtPointer) NULL,
  }, {
    XtNcancelCallback, XtCCallback,
    XtRCallback, sizeof(XtCallbackProc), FC_OFFSET(cancel_callback),
    XtRCallback, (XtPointer) NULL,
  }, {
    XtNchangedCallback, XtCCallback,
    XtRCallback, sizeof(XtCallbackProc), FC_OFFSET(changed_callback),
    XtRCallback, (XtPointer) NULL,
  }, {
    XtNerrorCallback, XtCCallback,
    XtRCallback, sizeof(XtCallbackProc), FC_OFFSET(error_callback),
    XtRCallback, (XtPointer) NULL,
  }, {
    XtNapplyLabel, XtCApplyLabel,
    XtROlStr, sizeof(OlStr), FC_OFFSET(apply_label),
    XtRLocaleString,
    #ifndef  XGETTEXT
    	(XtPointer) "Apply",
    #else
    	dgettext(OlMsgsDomain, "Apply"),
    #endif
  }, {
    XtNrevertLabel, XtCRevertLabel,
    XtROlStr, sizeof(OlStr), FC_OFFSET(revert_label),
    XtRLocaleString,
    #ifndef  XGETTEXT
    	(XtPointer) "Revert",
    #else
    	dgettext(OlMsgsDomain, "Revert"),
    #endif
  }, {
    XtNcancelLabel, XtCCancelLabel,
    XtROlStr, sizeof(OlStr), FC_OFFSET(cancel_label),
    XtRLocaleString,
    #ifndef  XGETTEXT
    	(XtPointer) "Cancel",
    #else
    	dgettext(OlMsgsDomain, "Cancel"),
    #endif
  }, {
    XtNtypefaceLabel, XtCTypefaceLabel,
    XtROlStr, sizeof(OlStr), FC_OFFSET(typeface_label),
    XtRLocaleString,
    #ifndef  XGETTEXT
    	(XtPointer) "Typeface",
    #else
    	dgettext(OlMsgsDomain, "Typeface"),
    #endif
  }, {
    XtNstyleLabel, XtCStyleLabel,
    XtROlStr, sizeof(OlStr), FC_OFFSET(style_label),
    XtRLocaleString,
    #ifndef  XGETTEXT
    	(XtPointer) "Style",
    #else
    	dgettext(OlMsgsDomain, "Style"),
    #endif
  }, {
    XtNsizeLabel, XtCSizeLabel,
    XtROlStr, sizeof(OlStr), FC_OFFSET(size_label),
    XtRLocaleString,
    #ifndef  XGETTEXT
    	(XtPointer) "Size",
    #else
    	dgettext(OlMsgsDomain, "Size"),
    #endif
  },
};

#undef	FC_OFFSET
#undef	OFFSET

/* Dynamic resources */

#define BYTE_OFFSET     XtOffsetOf(FontChooserRec, \
                                font_chooser.dynamic_resources_flags)
static _OlDynResource dynamic_resources[] = {
    {
	{
	    XtNforeground, XtCForeground,
	    XtRPixel, sizeof (Pixel), 0,
	    XtRString, XtDefaultForeground
	}, BYTE_OFFSET, OL_FONTC_FOREGROUND, NULL
    }, {
	{
	    XtNfontColor, XtCFontColor,
	    XtRPixel, sizeof(Pixel), 0,
	    XtRString, XtDefaultForeground
	}, BYTE_OFFSET, OL_FONTC_FONTCOLOR, NULL
    }, {
	{
	    XtNpreviewForeground, XtCForeground,
	    XtRPixel, sizeof(Pixel), 0,
	    XtRString, XtDefaultForeground
	}, BYTE_OFFSET, OL_FONTC_PREVIEWFOREGROUND, NULL
    }, {
	{
	    XtNpreviewFontColor, XtCFontColor,
	    XtRPixel, sizeof(Pixel), 0,
	    XtRString, XtDefaultForeground
	}, BYTE_OFFSET, OL_FONTC_PREVIEWFONTCOLOR, NULL
    },
}; /* end of dynamic_resources */

#undef BYTE_OFFSET

/*
 *---------------------------------------------------------------------------
 * Define Class Record structure to be initialized at Compile time
 *---------------------------------------------------------------------------
 */

FontChooserClassRec fontChooserClassRec = {
  {
	(WidgetClass) &rubberTileClassRec,	/* superclass		*/
	"FontChooser",				/* class_name		*/
	sizeof(FontChooserRec),			/* widget_size		*/
	ClassInitialize,			/* class_initialize	*/
	NULL,					/* class_part_initialize*/
	FALSE,					/* class_inited		*/
	Initialize,				/* initialize		*/
	NULL,					/* initialize_hook	*/
	XtInheritRealize,			/* realize		*/
	NULL,					/* actions		*/
	NULL,					/* num_actions		*/
	resources,				/* resources		*/
	XtNumber(resources),			/* num_resources	*/
	NULLQUARK,				/* xrm_class		*/
	TRUE,					/* compress_motion	*/
	TRUE,					/* compress_exposure	*/
	TRUE,					/* compress_enterleave	*/
	FALSE,					/* visible_interest	*/
	Destroy,				/* destroy		*/
	XtInheritResize,			/* resize		*/
	NULL,					/* expose		*/
	SetValues,				/* set_values		*/
	NULL,					/* set_values_hook	*/
	XtInheritSetValuesAlmost,		/* set_values_almost	*/
	NULL,					/* get_values_hook	*/
	NULL,					/* accept_focus		*/
	XtVersion,				/* version		*/
	NULL,					/* callback_private	*/
	NULL,					/* tm_table		*/
	XtInheritQueryGeometry,			/* query_geometry	*/
	NULL,					/* display_accelerator	*/
	NULL					/* extension		*/
  },	/* End of CoreClass field initializations */
  {
	XtInheritGeometryManager,    		/* geometry_manager	*/
	XtInheritChangeManaged,			/* change_managed	*/
	InsertChild,    			/* insert_child		*/
	XtInheritDeleteChild,    		/* delete_child		*/
	NULL    				/* extension         	*/
  },	/* End of CompositeClass field initializations */
  {
    	NULL,					/* resources		*/
    	0,					/* num_resources	*/
    	sizeof(FontChooserConstraintRec),	/* constraint_size	*/
    	NULL,					/* initialize		*/
    	NULL,					/* destroy		*/
    	NULL					/* set_values		*/
  },	/* End of ConstraintClass field initializations */
  {
    	NULL,					/* highlight_handler	*/
    	NULL,					/* reserved		*/
    	NULL,					/* reserved		*/
        NULL,					/* traversal_handler	*/
	NULL,					/* activate_widget	*/
	NULL,					/* event_procs		*/
	0,					/* num_event_procs	*/
	NULL,					/* register_focus	*/
    	NULL,					/* reserved		*/
	OlVersion,				/* version		*/
	NULL,					/* extension		*/
	{ dynamic_resources,
	    XtNumber(dynamic_resources) },	/* dyn_data */
	_OlDefaultTransparentProc,		/* transparent_proc */

  },	/* End of ManagerClass field initializations */
  {
	NULL					/* field not used	*/
  },	/* End of RubberTileClass field initializations */
  {
	NULL,					/* field not used	*/
  },	/* End of FontChooserClass field initializations */
};

/*
 *---------------------------------------------------------------------------
 * Public Widget Class Definition of the Widget Class Record
 *---------------------------------------------------------------------------
 */

WidgetClass fontChooserWidgetClass = (WidgetClass) &fontChooserClassRec;

/*
 *---------------------------------------------------------------------------
 * Private Procedures
 *---------------------------------------------------------------------------
 */

static Boolean
CvtLocaleStringToString(Display *display, 
			XrmValue *args, 
			Cardinal *num_args,
			XrmValue *from,
			XrmValue *to,
			XtPointer *converter_data)
{
    DeclareConversionClass (display, "localeStringToString", (String)0);
    String		arg_str;
    String		loc_str;
    int			len;
    Boolean 		success = True;
    XrmValue		src = {0, 0};

    if (*num_args != 0)
	ConversionError(
			"too many args",
			"LocaleString to String conversion needs no args",
			(String *)NULL,
			(Cardinal *)NULL); /* NOT REACHED */

    arg_str = (String) from->addr;
    if (arg_str != (String) NULL) {
	loc_str = dgettext(OlMsgsDomain, arg_str);
	loc_str = XtNewString(loc_str);
    }
    if (to->addr != NULL) {
	if (to->size < sizeof(String)) {
	    success = False;
	} else {
	    *(String *)(to->addr) = loc_str;
	}
    } else {
	static String	static_val;

	static_val = loc_str;
	to->addr = (XtPointer) &static_val;
    }
    to->size = sizeof(String);
    return(success);
}	/* CvtLocaleStringToString */

static void
ob_cancelCB(Widget w, XtPointer c, XtPointer ca)
{
    FontChooserWidget		fcw		= (FontChooserWidget) c;
    FontChooserPart *		fcp		= &(fcw->font_chooser);
    OlFCCancelCallbackStruct	ca_call_data;

    ca_call_data.reason = OL_REASON_CANCEL;
    ca_call_data.current_font_name  = fcp->current_font_name;
    ca_call_data.current_font       = fcp->current_font;
    XtCallCallbacks((Widget) fcw, XtNcancelCallback,
		    (XtPointer) &ca_call_data);
}

static void
ob_revertCB(Widget w, XtPointer c, XtPointer ca)
{
    FontChooserWidget		fcw		= (FontChooserWidget) c;
    FontChooserPart *		fcp		= &(fcw->font_chooser);
    OlFCRevertCallbackStruct	rv_call_data;

    RevertToInitialFont(fcw);
    rv_call_data.reason = OL_REASON_REVERT_FONT;
    rv_call_data.current_font_name  = fcp->current_font_name;
    rv_call_data.current_font       = fcp->current_font;
    rv_call_data.revert_font_name   = fcp->initial_font_name;
    rv_call_data.revert_font        = fcp->initial_font;
    XtCallCallbacks((Widget) fcw, XtNrevertCallback,
		    (XtPointer) &rv_call_data);
}

static void
ob_applyCB(Widget w, XtPointer c, XtPointer ca)
{
    FontChooserWidget		fcw		= (FontChooserWidget) c;
    FontChooserPart *		fcp		= &(fcw->font_chooser);
    OlFCApplyCallbackStruct	ap_call_data;

    ap_call_data.reason = OL_REASON_APPLY_FONT;
    ap_call_data.current_font_name  = fcp->current_font_name;
    ap_call_data.current_font       = fcp->current_font;
    XtCallCallbacks((Widget) fcw, XtNapplyCallback,
		    (XtPointer) &ap_call_data);
}

static void
init_pref_list(FontChooserWidget fcw)
{
    int			state;
    Cardinal		count;
    register String	trav;
    register int	i;
    
    /* Initialize preferred size list */
    trav = fcw->font_chooser.preferred_point_sizes;
    if (trav == (String) NULL) {
	OlWarning(dgettext(OlMsgsDomain,
	 "No preferred sizes. Size list will be empty for scalable fonts"));
    }
    state = 0; count = 0;
    for (; *trav != '\0'; trav++) {
	switch(state) {
	case 0:
	    if (isdigit(*trav)) {
		state = 1;
		count++;
	    } else if (!isspace(*trav))
		OlError(dgettext(OlMsgsDomain, "Bad preferred size list."));
	    break;
	case 1:
	    if (isspace(*trav))
		state = 0;
	    else if (!isdigit(*trav))
		OlError(dgettext(OlMsgsDomain, "Bad preferred size list."));
	    break;
	}
    }
    if (count == 0)
	OlWarning(dgettext(OlMsgsDomain,
	 "No preferred sizes. Size list will be empty for scalable fonts"));
    else {
	trav = XtNewString(fcw->font_chooser.preferred_point_sizes);
	fcw->font_chooser.pref_size_list = (String *)
	    XtMalloc(sizeof(String) * count);
	i = 0; state = 0;
	for (; *trav != '\0'; trav++) {
	    switch(state) {
	    case 0:
		if (isdigit(*trav)) {
		    fcw->font_chooser.pref_size_list[i++] = trav;
		    state = 1;
		} else
		    OlError(dgettext(OlMsgsDomain,
				     "Bad preferred size list."));
		break;
	    case 1:
		if (isspace(*trav)) {
		    *trav = '\0';
		    state = 0;
		} else if (!isdigit(*trav))
		    OlError(dgettext(OlMsgsDomain,
				     "Bad preferred size list."));
		break;
	    }
	}
	if (i != count)
	    OlError(dgettext(OlMsgsDomain, "Bad preferred size list."));
    }
    fcw->font_chooser.num_pref_sizes = count;
}

static void
copy_olstring(OlStrRep tf, OlStr * copied_str_ret)
{
    if(tf != OL_WC_STR_REP)
	*copied_str_ret = (*copied_str_ret != (OlStr) NULL) ?
	    XtNewString((XtPointer)(*copied_str_ret)) : (OlStr) NULL;
    else {
	wchar_t *ws;
	if (*copied_str_ret != (OlStr) NULL) {
	    ws = (wchar_t *)XtMalloc((wslen((wchar_t *)*copied_str_ret)
				      +1)*sizeof(wchar_t));
	    wscpy(ws, (wchar_t *) *copied_str_ret); 
	    *copied_str_ret = (OlStr)ws;
	}
    }
}

/*
 *---------------------------------------------------------------------------
 * Class Procedures
 *---------------------------------------------------------------------------
 */

/*
 * ClassInitialize - 
 */
/* ARGSUSED */
static void 
ClassInitialize(void)
{
    XtSetTypeConverter(XtRLocaleString, XtRString, CvtLocaleStringToString,
		       (XtConvertArgList) NULL, (Cardinal) 0,
		       XtCacheNone, (XtDestructor) NULL);
}

/* ARGSUSED */
static void
Initialize(Widget request, Widget new, ArgList args, Cardinal *num_args)
{
    FontChooserWidget	fcw 	= (FontChooserWidget) new;
    FontChooserPart *	fcp	= &(fcw->font_chooser);
    register String	trav	= fcp->charset_info;

    fcp->cur_typeface_tok = (OlListToken) 0;
    fcp->typeface_count = (Cardinal) 0;

    fcp->prev_size_count = (Cardinal) 0;
    fcp->prev_style_count = (Cardinal) 0;
    fcp->prev_typeface_count = (Cardinal) 0;

    fcp->typeface_tokens = (OlListToken *) NULL;
    fcp->style_tokens = (OlListToken *) NULL;
    fcp->size_tokens = (OlListToken *) NULL;

    fcp->cur_style_tok = (OlListToken) 0;
    fcp->cur_siz_tok = (OlListToken) 0;
    fcp->size_ntf_label = (String) NULL;
    fcp->size_ntf_name = (String) NULL;

    fcp->last_typeface = (String) NULL;
    fcp->last_style = (String) NULL;
    fcp->last_size = (String) NULL;

    fcp->cur_preview_height = (Cardinal) 0;

    fcp->fdb  = (FontDB) NULL;
    fcp->fsdb = (OWFsetDB) NULL;

    fcp->typeface_data = (FontField **) NULL;
    fcp->style_data = (FontField **) NULL;
    fcp->size_data = (FontField **) NULL;
	
    fcp->sorted_prefsizes = (ListItemSortRec *) NULL;
       
    fcp->cur_fdesc = (OlFontDesc *) NULL; /* Keeps the current font desc. */

    fcp->is_scalable = (Boolean) False;

    fcp->registry = (String) NULL;
    fcp->encoding = (String) NULL;
    fcp->pref_size_list = (String *) NULL;
    fcp->num_pref_sizes = (Cardinal) 0;

    fcp->current_font = (OlFont) NULL;
    fcp->current_font_name = (String) NULL;
    
    fcp->previous_font = (OlFont) NULL;
    fcp->previous_font_name = (String) NULL;
    
    fcp->initial_font = (OlFont) NULL;
    fcp->internal_insert = FALSE;
    fcp->internal_preview = TRUE;

    /* Initialize encoding info */
    if (trav == (String) NULL) {
	OlError(dgettext(OlMsgsDomain,
			 "No charset info. Cannot choose fonts."));
    }
    trav = XtNewString(trav);
    fcp->registry = trav;
    for (; *trav != '\0'; trav++)
	if (*trav == '-')
	    break;
    if (*trav == '-') {
	*trav = '\0';
	fcp->encoding = ++trav;
    } else {
	OlWarning(dgettext(OlMsgsDomain,
			   "Incomplete charset info; no encoding."));
    }

    init_pref_list(fcw);

    /* Copy all labels and string resources that may be settable */
    copy_olstring(fcp->text_format, &(fcp->no_preview_text));
    copy_olstring(fcp->text_format, &(fcp->preview_text));
    copy_olstring(fcp->text_format, &(fcp->preview_switch_label));
    copy_olstring(fcp->text_format, &(fcp->preview_switch_on_label));
    copy_olstring(fcp->text_format, &(fcp->preview_switch_off_label));
    copy_olstring(fcp->text_format, &(fcp->apply_label));
    copy_olstring(fcp->text_format, &(fcp->revert_label));
    copy_olstring(fcp->text_format, &(fcp->cancel_label));
    copy_olstring(fcp->text_format, &(fcp->typeface_label));
    copy_olstring(fcp->text_format, &(fcp->style_label));
    copy_olstring(fcp->text_format, &(fcp->size_label));
    
    /* Initialize the widget internals */
    InitializeFC(fcw, fcp->initial_font_name,
		 ob_applyCB, ob_revertCB, ob_cancelCB);
} /* END OF Initialize */

/*
 * Destroy - this procedure destroys the fontChooser widget.
 */
/* ARGSUSED */
static void
Destroy(Widget w)
{
    CleanupFC((FontChooserWidget) w);
} /* END OF Destroy() */

/* ARGSUSED */
static Boolean
SetValues(
	  Widget	current,
	  Widget	request,
	  Widget	new,
	  ArgList	args,
	  Cardinal *	num_args)
{
    Arg			garg[15];
    Arg			sarg[15];
    int			g;
    int			s;
    FontChooserWidget	fcw 	= (FontChooserWidget) new;
    FontChooserPart *	fcp	= &(fcw->font_chooser);
    FontChooserWidget	ofcw 	= (FontChooserWidget) current;
    FontChooserPart *	ofcp	= &(ofcw->font_chooser);
    
    /* Disable changes to resources that do not have S access */
    fcp->text_format 		= ofcp->text_format;
    fcp->scale 			= ofcp->scale;
    fcp->font_search_spec 	= ofcp->font_search_spec;
    fcp->charset_info 		= ofcp->charset_info;
    fcp->initial_font_name 	= ofcp->initial_font_name;
    fcp->maximum_point_size 	= ofcp->maximum_point_size;
    fcp->preferred_point_sizes 	= ofcp->preferred_point_sizes;
    fcp->extension_area 	= ofcp->extension_area;
    fcp->preview_present 	= ofcp->preview_present;

    /* Collect changes to generic resources that will propagate down */
    g = 0;
    s = 0;
    if (fcp->input_focus_color != ofcp->input_focus_color) {
	XtSetArg(garg[g], XtNinputFocusColor, fcp->input_focus_color); g++;
	XtSetArg(sarg[s], XtNinputFocusColor, fcp->input_focus_color); s++;
    }
    if (fcp->font != ofcp->font) {
	XtSetArg(garg[g], XtNfont, fcp->font); g++;
    }
    if (fcw->core.background_pixel != ofcw->core.background_pixel) {
	XtSetArg(garg[g], XtNbackground, fcw->core.background_pixel); g++;
    }
    if (fcp->font_color != ofcp->font_color) {
	XtSetArg(garg[g], XtNfontColor, fcp->font_color); g++;
    }
    if (fcp->foreground != ofcp->foreground) {
	XtSetArg(garg[g], XtNforeground, fcp->foreground); g++;
    }
    if (g > 0) {
	XtSetValues(fcp->rt_top, garg, g);
	XtSetValues(fcp->rt_size, garg, g);
	XtSetValues(fcp->but_size, garg, g);
	XtSetValues(fcp->rt_attributes, garg, g);
	XtSetValues(fcp->rt_buttons, garg, g);
	XtSetValues(fcp->rt_style, garg, g);
	XtSetValues(fcp->but_style, garg, g);
	XtSetValues(fcp->sl_style, garg, g);
	XtSetValues(fcp->rt_size, garg, g);
	XtSetValues(fcp->sl_size, garg, g);
	XtSetValues(fcp->ntf_size, garg, g);
	XtSetValues(fcp->ob_cancel, garg, g);
	XtSetValues(fcp->ob_revert, garg, g);
	XtSetValues(fcp->ob_apply, garg, g);
	XtSetValues(fcp->rt_typeface, garg, g);
	XtSetValues(fcp->but_typeface, garg, g);
	XtSetValues(fcp->sl_typeface, garg, g);
	XtSetValues(fcp->rt_prev_sw, garg, g);
	XtSetValues(fcp->rt_preview, garg, g);
	XtSetValues(fcp->ex_preview, garg, g);
	XtSetValues(fcp->rb_preview_on, garg, g);
	XtSetValues(fcp->rb_preview_off, garg, g);
	XtSetValues(fcp->fcw_top_sp, garg, g);
	XtSetValues(fcp->prv_swl_sp, garg, g);
	XtSetValues(fcp->prv_swr_sp, garg, g);
	XtSetValues(fcp->prv_lef_sp, garg, g);
	XtSetValues(fcp->prv_rgt_sp, garg, g);
	XtSetValues(fcp->attr_lef_sp, garg, g);
	XtSetValues(fcp->attr_rgt_sp, garg, g);
    }

    /* Collect changes to preview resources that will propagate down */
    if (fcp->preview_background != ofcp->preview_background) {
	XtSetArg(sarg[s], XtNbackground, fcp->preview_background); s++;
    }
    if (fcp->preview_font_color != ofcp->preview_font_color) {
	XtSetArg(sarg[s], XtNfontColor, fcp->preview_font_color); s++;
    }
    if (fcp->preview_foreground != ofcp->preview_foreground) {
	XtSetArg(sarg[s], XtNforeground, fcp->preview_foreground); s++;
    }
    if (fcp->preview_height != ofcp->preview_height) {
	XtSetArg(sarg[s], XtNheight, fcp->preview_height); s++;
    }
    if (fcp->preview_border_width != ofcp->preview_border_width) {
	XtSetArg(sarg[s], XtNborderWidth, fcp->preview_border_width); s++;
    }
    if (s > 0)
	XtSetValues(fcp->st_preview, sarg, s);
    if (fcp->preview_text != ofcp->preview_text) {
	copy_olstring(fcp->text_format, &(fcp->preview_text));
	PreviewFont(fcw);
    }
    if (fcp->no_preview_text != ofcp->no_preview_text) {
	copy_olstring(fcp->text_format, &(fcp->no_preview_text));
	PreviewFont(fcw);
    }

    /* Collect all label resource changes and propagate them */
    if (fcp->preview_switch_label != ofcp->preview_switch_label) {
	XtFree((XtPointer) ofcp->preview_switch_label);
	copy_olstring(fcp->text_format, &(fcp->preview_switch_label));
	XtVaSetValues(fcp->cap_preview,
			XtNlabel, fcp->preview_switch_label, NULL);
    }
    if (fcp->preview_switch_on_label != ofcp->preview_switch_on_label) {
	XtFree((XtPointer) ofcp->preview_switch_on_label);
	copy_olstring(fcp->text_format, &(fcp->preview_switch_on_label));
	XtVaSetValues(fcp->rb_preview_on,
			XtNlabel, fcp->preview_switch_on_label, NULL);
    }
    if (fcp->preview_switch_off_label != ofcp->preview_switch_off_label) {
	XtFree((XtPointer) ofcp->preview_switch_off_label);
	copy_olstring(fcp->text_format, &(fcp->preview_switch_off_label));
	XtVaSetValues(fcp->rb_preview_off,
			XtNlabel, fcp->preview_switch_off_label, NULL);
    }
    if (fcp->apply_label != ofcp->apply_label) {
	XtFree((XtPointer) ofcp->apply_label);
	copy_olstring(fcp->text_format, &(fcp->apply_label));
	XtVaSetValues(fcp->ob_apply, XtNlabel, fcp->apply_label, NULL);
    }
    if (fcp->revert_label != ofcp->revert_label) {
	XtFree((XtPointer) ofcp->revert_label);
	copy_olstring(fcp->text_format, &(fcp->revert_label));
	XtVaSetValues(fcp->ob_revert, XtNlabel, fcp->revert_label, NULL);
    }
    if (fcp->cancel_label != ofcp->cancel_label) {
	XtFree((XtPointer) ofcp->cancel_label);
	copy_olstring(fcp->text_format, &(fcp->cancel_label));
	XtVaSetValues(fcp->ob_cancel, XtNlabel, fcp->cancel_label, NULL);
    }
    if (fcp->typeface_label != ofcp->typeface_label) {
	XtFree((XtPointer) ofcp->typeface_label);
	copy_olstring(fcp->text_format, &(fcp->typeface_label));
	XtVaSetValues(fcp->but_typeface, XtNlabel, fcp->typeface_label, NULL);
    }
    if (fcp->style_label != ofcp->style_label) {
	XtFree((XtPointer) ofcp->style_label);
	copy_olstring(fcp->text_format, &(fcp->style_label));
	XtVaSetValues(fcp->but_style, XtNlabel, fcp->style_label, NULL);
    }
    if (fcp->size_label != ofcp->size_label) {
	XtFree((XtPointer) ofcp->size_label);
	copy_olstring(fcp->text_format, &(fcp->size_label));
	XtVaSetValues(fcp->but_size, XtNlabel, fcp->size_label, NULL);
    }

    return(FALSE);
} /* END OF SetValues() */

static void
InsertChild (Widget w)
{
    FontChooserWidget	fcw = (FontChooserWidget) XtParent(w);
    XtWidgetProc	insert_child = ((CompositeWidgetClass)
	(fontChooserClassRec.core_class.superclass))->
	    composite_class.insert_child;

    if (fcw->font_chooser.internal_insert == (Boolean) TRUE && insert_child)
	(*insert_child)(w);
    else
	OlWarning(dgettext(OlMsgsDomain,
	   "Trying to insert children in fontChooserWidget; ignored."));
}


/*
 *---------------------------------------------------------------------------
 * Action Procedures
 *---------------------------------------------------------------------------
 */

/*
 *---------------------------------------------------------------------------
 * Public Procedures
 *---------------------------------------------------------------------------
 */

