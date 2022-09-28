#pragma       ident   "@(#)FCImpl.c 1.13     97/03/26 SMI"        /* OLIT */

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

#include <libintl.h>
#include <locale.h>
#include <stdio.h>
#include <widec.h>
#include <wctype.h>
#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <Xol/OpenLook.h>
#include <Xol/OlCursors.h>
#include <Xol/ControlAre.h>
#include <Xol/ScrollingL.h>
#include <Xol/StaticText.h>
#include <Xol/NumericFie.h>
#include <Xol/Button.h>
#include <Xol/OblongButt.h>
#include <Xol/TextEdit.h>
#include <Xol/RubberTile.h>
#include <Xol/Exclusives.h>
#include <Xol/RectButton.h>
#include <Xol/Caption.h>

#include <Xol/FontChP.h>

#include <Xol/FCImplI.h>
#include <Xol/FontDB.h>
#include <Xol/OWFsetDB.h>

/* ===========================  Local data ============================= */

/* Specifies XLFD fields of interest to be kept in the databases */
static FontFieldID	interest_fields[] = {
    FC_FOUNDRY, FC_FAMILY_NAME, FC_WEIGHT_NAME,
    FC_SLANT, FC_SETWIDTH_NAME, FC_ADD_STYLE_NAME, FC_POINT_SIZE,
};

static Cardinal		typeface_field_count	= NUM_TYPEFACE_FIELDS;
				/*
				 * FC_FAMILY_NAME
				 * FC_SETWIDTH_NAME
				 * FC_ADD_STYLE_NAME
				 * FC_FOUNDRY
				 */
static Cardinal		style_field_count	= NUM_STYLE_FIELDS;
				/*
				 * FC_SLANT
				 * FC_WEIGHT_NAME
				 */
static Cardinal		size_field_count	= NUM_SIZE_FIELDS;
				/*
				 * FC_POINT_SIZE
				 */

/* Used to construct the preview_string for the preview area */

static int	preview_typeface[] = {
    FC_FOUNDRY, FC_FAMILY_NAME, FC_SETWIDTH_NAME, FC_ADD_STYLE_NAME,
};

static int	preview_style[] = {
    FC_WEIGHT_NAME, FC_SLANT,
};

static int	preview_size[] = {
    FC_POINT_SIZE,
};

/* Used to construct the preview_string in the preview area */
static String	prefix_typeface[] = {
    "", " ", "", "-",
};

static String	prefix_style[] = {
    " ", "-",
};

static String	prefix_size[] = {
    " ",
};

/* Used to put in delimiters while constructing scrolling list labels */

static char * style_field_prefix[NUM_STYLE_FIELDS] = {
    "",	"-",
};

static char * style_field_suffix[NUM_STYLE_FIELDS] = {
    "",	"",
};

static char *	typeface_field_prefix[NUM_TYPEFACE_FIELDS] = { 
    "",		" ",	"-",	"  (",
};

static char *	typeface_field_suffix[NUM_TYPEFACE_FIELDS] = { 
    "",		"",	"",	")",
};

static char * slant_name_map[] = {
    "r",	"Roman",
    "i",	"Italic",
    "o",	"Oblique",
    "ri",	"Reverse Italic",
    "ro",	"Reverse Oblique",
    "ot",	"Other",
    NULL,	NULL,
};

/* ================== Function Prototypes =============== */

static void	typeface_listCB(
			Widget		w,
			XtPointer	client_data,
			XtPointer	call_data);

static void	style_listCB(
			Widget		w,
		 	XtPointer	client_data,
			XtPointer	call_data);

static void	size_listCB(
			Widget		w,
			XtPointer	client_data,
			XtPointer	call_data);

static void     size_ntfCB(
			   Widget          w,
			   XtPointer       client_data,
			   XtPointer       call_data);

static void     preview_on_rbCB(
			   Widget          w,
			   XtPointer       client_data,
			   XtPointer       call_data);

static void     preview_off_rbCB(
			   Widget          w,
			   XtPointer       client_data,
			   XtPointer       call_data);

static void	populate_typeface_list(
			FontChooserWidget	fcw,	       
		 	FontField **	typeface_list,
		 	int		num_typefaces,
		 	int		field_count);

static void	populate_style_list(
			FontChooserWidget	fcw,	       
			FontField **	style_list,
			int		num_styles,
			int		field_count);

static void	populate_size_list(
			FontChooserWidget	fcw,	       
			FontField **	size_list,
			int		num_sizes,
			int		field_count);

static FontField **
		get_typefaces(
			    FontDB	fdb,
			    Cardinal *	typeface_count);
static FontField **
		get_styles(FontDB	fdb,
			   OlFontDesc * cur_fdesc,
			   Cardinal *	style_count);

static FontField **
		get_sizes(FontDB	fdb,
			  OlFontDesc *  cur_fdesc,
	  		  Cardinal *	size_count);

static String	get_full_slant_name(String	short_name);

static void	make_item_current(
			Widget	w,
		        OlListToken	token);

static void	create_gui(FontChooserWidget fcw);

static void	add_internal_callbacks(FontChooserWidget fcw,
				       XtCallbackProc	 apply_CB,
				       XtCallbackProc	 revert_CB,
				       XtCallbackProc	 cancel_CB);

static int	field_strcmp(const void * a1, const void * a2);

static int	size_cmp(const void * a1, const void * a2);

static Widget	create_sl(FontChooserWidget	fcp,
			  String	inst_name,
			  Widget	parent,
			  OlStrRep	text_format,
			  Cardinal	list_height);

static wchar_t *	generate_wc_preview(FontChooserWidget	fcw,
					    wchar_t *		txt);

static char *		generate_sb_preview(FontChooserWidget	fcw,
					    char *		txt);

static void		capitalize_phrase(char *	val);

static void		capitalize_wc_phrase(wchar_t *	val);

static wchar_t *	cvt_to_wc(String	mb_str);


static String		initial_typeface_label(
				String	ini_family_name,
				String	ini_setwidth_name,
				String	ini_add_style_name,
				String	ini_foundry);

static String		initial_style_label(
				String	ini_slant,
				String	ini_weight_name);

static String		initial_size_label(String	ini_point_size);

/* ========================= Function Definitions ========================= */

/* --------------------------- Extern functions --------------------------- */


extern void
InitializeFC(FontChooserWidget	fcw,
	     String		initial_font_name,
	     XtCallbackProc	apply_CB,
	     XtCallbackProc	revert_CB,
	     XtCallbackProc	cancel_CB)
{
    FontChooserPart *	fcp = &(fcw->font_chooser);
    FontFieldID	req_fields[10];
    Cardinal	style_count;
    Cardinal	size_count;
    String *	font_list;
    int     	font_count;
    int		i;

    Boolean	use_initial_font;
    String	ini_foundry;
    String	ini_family_name;
    String	ini_weight_name;
    String	ini_slant;
    String	ini_setwidth_name;
    String	ini_add_style_name;
    String	ini_point_size;

    fcp->rt_top = (Widget) fcw;
    create_gui(fcw);
    add_internal_callbacks(fcw, apply_CB, revert_CB, cancel_CB);

    fcp->cur_fdesc = (OlFontDesc *) XtMalloc(sizeof(OlFontDesc));
    fcp->cur_fdesc->components = (OlFontComponent *) XtMalloc(
					      sizeof(OlFontComponent));
    fcp->cur_fdesc->num_components = 1;

    if (fcp->text_format != OL_SB_STR_REP) {
	String  fsdb_path;
	fsdb_path = XtResolvePathname(XtDisplay((Widget) fcw), "OW_FONT_SETS",
				 "OpenWindows", ".fs",
				 NULL, NULL , 0, NULL);

	if (fsdb_path != (String) NULL) {
	    fcp->fsdb = CreateOWFsetDB(fsdb_path);

	    if (fcp->fsdb == (OWFsetDB) NULL)
		OlWarning(dgettext(OlMsgsDomain,
		   "Cannot use OpenWindows.fs. Trying charset info."));
	    else {
		font_list = ListOWFsets(fcp->fsdb, &font_count);
		fcp->fdb = _OlCreateFontDatabase(font_list, font_count,
			     interest_fields, XtNumber(interest_fields),
			     (String *) NULL, (String *) NULL, 0);
	    }
	}
    }
    if (fcp->fdb == (FontDB) NULL) { /* SB or, OpenWindows.fs was bad */
	font_list = XListFonts(XtDisplay(fcw), fcp->font_search_spec,
			   FC_MAXNAMES, &font_count);

	fcp->fdb = _OlCreateFontDatabase(font_list, font_count,
			     interest_fields, XtNumber(interest_fields),
			     &fcp->registry, &fcp->encoding, 1);
	if (fcp->fdb == (FontDB) NULL) {
	    OlError(dgettext(OlMsgsDomain, "_OlCreateFontDatabase()\n"));
	    exit(1);
	}
    }

    if (fcp->fsdb == (OWFsetDB) NULL) /* SB or, OpenWindows.fs was bad */
	XFreeFontNames(font_list);
    else
	FreeOWFsetList(font_list);
    

    use_initial_font = (Boolean) FALSE;
    if (initial_font_name != (String) NULL) {
	Arg	args[FC_XLFD_LAST];
	int	n;

	for (n = 0; n < XtNumber(interest_fields); n++)
	    XtSetArg(args[n], (String) interest_fields[n], NULL);
	if (!_OlExtractFontFields(initial_font_name, args, n)) {
	    OlFCErrorCallbackStruct	err_call_data;

	    err_call_data.reason	= OL_REASON_ERROR;
	    err_call_data.error_num	= OL_FC_ERR_BAD_INITIAL_FONT;
	    err_call_data.font_name	= initial_font_name;
	    XtCallCallbacks((Widget) fcw, XtNerrorCallback,
			    (XtPointer) &err_call_data);
	} else {
	    ini_foundry 	= (String) args[0].value;
	    ini_family_name 	= (String) args[1].value;
	    ini_weight_name 	= (String) args[2].value;
	    ini_slant 		= (String) args[3].value;
	    ini_setwidth_name 	= (String) args[4].value;
	    ini_add_style_name 	= (String) args[5].value;
	    ini_point_size 	= (String) args[6].value;
	    use_initial_font = (Boolean) TRUE;
	}
    }
	
    /* Get the typeface list */

    fcp->typeface_data = get_typefaces(fcp->fdb,
				       &fcp->typeface_count);
	
    if (fcp->typeface_data == (FontField **)NULL)
	OlWarning(dgettext(OlMsgsDomain,
		  "No matches for typefaces found \n"));
    else {
	/* Find the initial typeface in the list */
	if (use_initial_font == (Boolean) TRUE)
	    fcp->last_typeface = initial_typeface_label(
				    ini_family_name,
				    ini_setwidth_name,
				    ini_add_style_name,
				    ini_foundry);
	
	populate_typeface_list(fcw,
			       fcp->typeface_data,
			       fcp->typeface_count,
			       typeface_field_count);
    }
    
    /* Get the style list corresponding to the first typeface */
    fcp->style_data = get_styles(fcp->fdb, fcp->cur_fdesc, &style_count);

    if (fcp->style_data == (FontField **) NULL)
      OlWarning(dgettext(OlMsgsDomain, "No matches for styles found \n"));
    else { 
	/* Find the initial style in the list */
	if (use_initial_font == (Boolean) TRUE)
	    fcp->last_style = initial_style_label(ini_slant, ini_weight_name);

	populate_style_list(fcw, fcp->style_data,
			    style_count, style_field_count);
    }

    if (fcp->num_pref_sizes > 0) {
	fcp->sorted_prefsizes = (ListItemSortRec *)
	    XtMalloc(sizeof(ListItemSortRec) * fcp->num_pref_sizes);
	for (i = 0; i < fcp->num_pref_sizes; i++) {
	    char	*fval;

		if (fval = malloc(SIZE_FIELD_SIZE)) {
			fcp->sorted_prefsizes[i].label = fcp->pref_size_list[i];
			strncpy(fval, fcp->pref_size_list[i], SIZE_FIELD_SIZE-2);
			strcat(fval, "0");
			fcp->sorted_prefsizes[i].fields = (FontField *)
			XtMalloc(sizeof(FontField));
			fcp->sorted_prefsizes[i].fields[0].quark = XrmStringToQuark(fval);
			fcp->sorted_prefsizes[i].fields[0].name  = XtNewString(fval);
			free(fval);
		}
	}
    }

    fcp->size_data = get_sizes(fcp->fdb, fcp->cur_fdesc, &size_count);

			      
    if (fcp->size_data == (FontField **)NULL)
      OlWarning(dgettext(OlMsgsDomain, "No matches for sizes found \n"));
    else {
	/* Find the initial size in the list */
	if (use_initial_font == (Boolean) TRUE)
	    fcp->last_size = initial_size_label(ini_point_size);

	populate_size_list(fcw, fcp->size_data, size_count, size_field_count);
    }

    if (fcp->style_data != (FontField **)NULL &&
	fcp->size_data != (FontField **)NULL)
	PreviewFont(fcw);
    if (use_initial_font == (Boolean) TRUE)
	fcp->initial_font = fcp->current_font;
}

extern void
CleanupFC(FontChooserWidget	fcw)
{
    FontChooserPart *	fcp = &(fcw->font_chooser);

    if (fcp->typeface_count > 0)
	XtFree((XtPointer) fcp->typeface_tokens);
    if (fcp->prev_size_count > 0)
	XtFree((XtPointer) fcp->size_tokens);
    if (fcp->prev_style_count > 0)
	XtFree((XtPointer) fcp->style_tokens);

    if (fcp->size_ntf_label != (String) NULL)
	XtFree((XtPointer) fcp->size_ntf_label);
    if (fcp->size_ntf_name != (String) NULL)
	XtFree((XtPointer) fcp->size_ntf_name);
    if (fcp->last_style != (String) NULL)
	XtFree((XtPointer) fcp->last_style);
    if (fcp->last_size != (String) NULL)
	XtFree((XtPointer) fcp->last_size);

    if (fcp->fdb != (FontDB) NULL)
	_OlDestroyFontDatabase(fcp->fdb);
    if (fcp->fsdb != (OWFsetDB) NULL)
	DestroyOWFsetDB(fcp->fsdb);

    if (fcp->typeface_data != (FontField **) NULL)
	_OlFreeFontFieldLists(fcp->typeface_data, typeface_field_count);
    if (fcp->size_data != (FontField **) NULL)
	_OlFreeFontFieldLists(fcp->size_data, size_field_count);
    if (fcp->style_data != (FontField **) NULL)
	_OlFreeFontFieldLists(fcp->style_data, style_field_count);
    
    if (fcp->num_pref_sizes > 0) {
	register int	i;
	
	for (i = 0; i < fcp->num_pref_sizes; i++) {
	    XtFree((XtPointer) fcp->sorted_prefsizes[i].fields[0].name);
	    XtFree((XtPointer) fcp->sorted_prefsizes[i].fields);
	}
	XtFree((XtPointer) fcp->pref_size_list[0]);
	XtFree((XtPointer) fcp->pref_size_list);
	XtFree((XtPointer) fcp->sorted_prefsizes);
    }
    
    XtFree((XtPointer) fcp->cur_fdesc);
    XtFree((XtPointer) fcp->cur_fdesc->components);

    if (fcp->registry != (String) NULL)
	XtFree((XtPointer) fcp->registry);
    if (fcp->encoding != (String) NULL)
	XtFree((XtPointer) fcp->encoding);
    if (fcp->no_preview_text != (OlStr) NULL)
	XtFree((XtPointer) fcp->no_preview_text);
    if (fcp->preview_text != (OlStr) NULL)
	XtFree((XtPointer) fcp->preview_text);
    if (fcp->preview_switch_label != (OlStr) NULL)
	XtFree((XtPointer) fcp->preview_switch_label);
    if (fcp->preview_switch_on_label != (OlStr) NULL)
	XtFree((XtPointer) fcp->preview_switch_on_label);
    if (fcp->preview_switch_off_label != (OlStr) NULL)
	XtFree((XtPointer) fcp->preview_switch_off_label);
    if (fcp->apply_label != (OlStr) NULL)
	XtFree((XtPointer) fcp->apply_label);
    if (fcp->revert_label != (OlStr) NULL)
	XtFree((XtPointer) fcp->revert_label);
    if (fcp->cancel_label != (OlStr) NULL)
	XtFree((XtPointer) fcp->cancel_label);
    if (fcp->typeface_label != (OlStr) NULL)
	XtFree((XtPointer) fcp->typeface_label);
    if (fcp->style_label != (OlStr) NULL)
	XtFree((XtPointer) fcp->style_label);
    if (fcp->size_label != (OlStr) NULL)
	XtFree((XtPointer) fcp->size_label);
}

extern void
RevertToInitialFont(FontChooserWidget	fcw)
{
    FontChooserPart *	fcp = &(fcw->font_chooser);
    String		initial_font_name = fcp->initial_font_name;
    Boolean		use_initial_font;
    String		ini_foundry;
    String		ini_family_name;
    String		ini_weight_name;
    String		ini_slant;
    String		ini_setwidth_name;
    String		ini_add_style_name;
    String		ini_point_size;
    Cardinal		sel_token_i;
    register int	i;
    OlListItem *	new_item;

    use_initial_font = (Boolean) FALSE;
    if (initial_font_name != (String) NULL) {
	Arg	args[FC_XLFD_LAST];
	int	n;

	for (n = 0; n < XtNumber(interest_fields); n++)
	    XtSetArg(args[n], (String) interest_fields[n], NULL);
	if (_OlExtractFontFields(initial_font_name, args, n)) {
	    ini_foundry 	= (String) args[0].value;
	    ini_family_name 	= (String) args[1].value;
	    ini_weight_name 	= (String) args[2].value;
	    ini_slant 		= (String) args[3].value;
	    ini_setwidth_name 	= (String) args[4].value;
	    ini_add_style_name 	= (String) args[5].value;
	    ini_point_size 	= (String) args[6].value;
	    use_initial_font = (Boolean) TRUE;
	}
    }
    if (use_initial_font == (Boolean) TRUE) {
	fcp->last_typeface = initial_typeface_label(
				ini_family_name,
				ini_setwidth_name,
				ini_add_style_name,
				ini_foundry);
	fcp->last_style = initial_style_label(ini_slant, ini_weight_name);
	fcp->last_size = initial_size_label(ini_point_size);

	/* Do typeface */
	sel_token_i = 0;
	for (i = 0; i < fcp->prev_typeface_count; i++) {
	    String	 label;
	    
	    new_item = OlListItemPointer(fcp->typeface_tokens[i]);
	    label = new_item->label;
	    if (!strcmp(fcp->last_typeface, label)) {
		sel_token_i = i;
		break;
	    }
	}
	typeface_listCB(fcp->sl_typeface, (XtPointer) fcw,
			(XtPointer) fcp->typeface_tokens[sel_token_i]);

	/* Do style */
	sel_token_i = 0;
	for (i = 0; i < fcp->prev_style_count; i++) {
	    String	 label;
	    
	    new_item = OlListItemPointer(fcp->style_tokens[i]);
	    label = new_item->label;
	    if (!strcmp(fcp->last_style, label)) {
		sel_token_i = i;
		break;
	    }
	}
	style_listCB(fcp->sl_style, (XtPointer) fcw,
			(XtPointer) fcp->style_tokens[sel_token_i]);

	/* Do size */
	sel_token_i = 0;
	for (i = 0; i < fcp->prev_size_count; i++) {
	    String	 label;
	    
	    new_item = OlListItemPointer(fcp->size_tokens[i]);
	    label = new_item->label;
	    if (!strcmp(fcp->last_size, label)) {
		sel_token_i = i;
		break;
	    }
	}
	size_listCB(fcp->sl_size, (XtPointer) fcw,
			(XtPointer) fcp->size_tokens[sel_token_i]);

	if (fcp->style_data != (FontField **)NULL &&
	    fcp->size_data != (FontField **)NULL)
	    PreviewFont(fcw);
    }
}

extern void
PreviewFont(FontChooserWidget	fcw)
{
    if (fcw->font_chooser.preview_present) {
	FontChooserPart *	fcp = &(fcw->font_chooser);
	char		*pattern;
	OlStr		cur_preview_txt = (OlStr) NULL;
	String		font_name;
	OlFont		ol_font;
	OlFontComponent *	fcomp =
	    &(fcp->cur_fdesc->components[0]);
	register int	i;
	int		font_ht = 0;
	Boolean		is_loaded = TRUE;

	if (!(pattern = malloc(FC_MAXFONTLEN))) {
		return;
	}

	if (fcp->fsdb != (OWFsetDB) NULL) {
	    snprintf(pattern, FC_MAXFONTLEN, "-%s-%s-%s-%s-%s-%s-*-%s-*-*-*-*-*-*",
		    fcomp->fields[FC_FOUNDRY].name,
		    fcomp->fields[FC_FAMILY_NAME].name,
		    fcomp->fields[FC_WEIGHT_NAME].name,
		    fcomp->fields[FC_SLANT].name,
		    fcomp->fields[FC_SETWIDTH_NAME].name,
		    fcomp->fields[FC_ADD_STYLE_NAME].name,
		    fcomp->fields[FC_POINT_SIZE].name);
	} else {
	    snprintf(pattern, FC_MAXFONTLEN, "-%s-%s-%s-%s-%s-%s-*-%s-*-*-*-*-%s-%s",
		    fcomp->fields[FC_FOUNDRY].name,
		    fcomp->fields[FC_FAMILY_NAME].name,
		    fcomp->fields[FC_WEIGHT_NAME].name,
		    fcomp->fields[FC_SLANT].name,
		    fcomp->fields[FC_SETWIDTH_NAME].name,
		    fcomp->fields[FC_ADD_STYLE_NAME].name,
		    fcomp->fields[FC_POINT_SIZE].name,
		    fcp->registry, fcp->encoding);
	}

	switch (fcp->text_format) {
	case OL_WC_STR_REP:
	    cur_preview_txt = (OlStr) generate_wc_preview(fcw,
				      (wchar_t *) fcp->preview_text);
	    break;
	case OL_MB_STR_REP:
	    {
		size_t	wlen = strlen(fcp->preview_text);
		wchar_t *	w_tmp = (wchar_t *) XtMalloc((wlen+1)
						     * sizeof(wchar_t));
		wchar_t *	w_prev;
		char *		cur_prev;
		
		mbstowcs(w_tmp, fcp->preview_text, wlen+1);
		w_prev = generate_wc_preview(fcw, w_tmp);
		XtFree((XtPointer) w_tmp);
		wlen = wslen(w_prev);
		cur_prev = (char *) XtMalloc(wlen * MB_CUR_MAX + 1);
		wcstombs(cur_prev, w_prev, wlen * MB_CUR_MAX + 1);
		XtFree((XtPointer) w_prev);
		cur_preview_txt = (OlStr) cur_prev;
	    }
	    break;
	case OL_SB_STR_REP:
	    cur_preview_txt = (OlStr) generate_sb_preview(fcw,
					      (char *) fcp->preview_text);
	    break;
	}

	if (fcp->fsdb != (OWFsetDB) NULL) {
	    String	fset_name;
	    
	    fset_name = GetOWFsetForSpec(fcp->fsdb, pattern);
	    font_name = GetFsetForOWFset(fcp->fsdb, fset_name);
	} else
	    font_name = pattern;

	if (font_name == (String) NULL)
	    OlError(dgettext(OlMsgsDomain,
		     "FontChooser: Internal error (null font_name)."));
	
	if (fcp->internal_preview && XtIsRealized(fcp->rt_top)) {
	    XtVaSetValues(_OlGetShellOfWidget(fcp->rt_top),
				XtNbusy, TRUE, NULL);
	    XGrabPointer(XtDisplay(fcp->rt_top), XtWindow(fcp->rt_top),
		True, NoEventMask, GrabModeAsync, GrabModeAsync,
		None, OlGetBusyCursor(fcp->rt_top), CurrentTime);
	}

	if (fcp->internal_preview) {
	    if (fcp->text_format == OL_SB_STR_REP) {
		XFontStruct * newf;
		
		newf = XLoadQueryFont(XtDisplay(fcp->rt_top), font_name);
		if (newf != (XFontStruct *) NULL) {
		    font_ht = newf->ascent + newf->descent;
		    ol_font = (OlFont) newf;
		} else {
		    is_loaded = FALSE;
		    ol_font = (OlFont) NULL;
		}
	    } else {
		char ** 	miss_chset_list;
		int		miss_chset_cnt;
		char *  	default_string;
		XFontSet	newf;
		
		newf = XCreateFontSet(XtDisplay(fcp->rt_top), font_name,
				      &miss_chset_list, &miss_chset_cnt,
				      &default_string);
		if (newf != (XFontSet) NULL) {
		    if (miss_chset_cnt > 0) {
			OlWarning(dgettext(OlMsgsDomain,
			   "Current font has missing character sets."));
			XFreeStringList(miss_chset_list);
		    }
		    font_ht = XExtentsOfFontSet(newf)->max_logical_extent.height;
		    ol_font = (OlFont) newf;
		} else {
		    is_loaded = FALSE;
		    ol_font = (OlFont) NULL;
		}
	    }
	} else {
	    is_loaded = FALSE;
	    ol_font = (OlFont) NULL;
	}

	if (fcp->internal_preview && XtIsRealized(fcp->rt_top)) {
	    XUngrabPointer(XtDisplay(fcp->rt_top), CurrentTime);
	    XtVaSetValues(_OlGetShellOfWidget(fcp->rt_top),
				XtNbusy, FALSE, NULL);
	}

 
	if (!fcp->internal_preview || is_loaded) {
	    OlFCChangedCallbackStruct	ch_call_data;
	    Arg	args[3];
	    int n;
	    
	    n = 0;
	    if (fcp->internal_preview) {
		XtSetArg(args[n], XtNstring, cur_preview_txt); n++;
	    } else {
		XtSetArg(args[n], XtNstring, fcp->no_preview_text); n++;
	    }
	    if (fcp->internal_preview && is_loaded) {
		if (fcp->cur_preview_height < font_ht * 3) {
		    XtSetArg(args[n], XtNheight, (font_ht * 3)); n++;
		    fcp->cur_preview_height = font_ht * 3;
		}
		XtSetArg(args[n], XtNfont, ol_font); n++;
	    }
	    XtSetValues(fcp->st_preview, args, n);

	    if (fcp->previous_font_name != (String) NULL)
		XtFree((XtPointer) fcp->previous_font_name);
	    fcp->previous_font_name = fcp->current_font_name;
	    fcp->current_font_name  = XtNewString(font_name);
	    
	    if (fcp->internal_preview && fcp->previous_font != (OlFont) NULL) {
		if (fcp->text_format == OL_SB_STR_REP)
		    XFreeFont(XtDisplay((Widget) fcw),
				    (XFontStruct *) fcp->previous_font);
		else
		    XFreeFontSet(XtDisplay((Widget) fcw),
				 (XFontSet) fcp->previous_font);
	    }
	    if (fcp->internal_preview) {
		fcp->previous_font   = fcp->current_font;
		fcp->current_font    = ol_font;
	    }
	    
	    ch_call_data.reason = OL_REASON_CHANGED_FONT;
	    ch_call_data.current_font_name  = fcp->current_font_name;
	    ch_call_data.current_font       = fcp->current_font;
	    ch_call_data.previous_font_name = fcp->previous_font_name;
	    if (fcp->internal_preview)
		ch_call_data.previous_font = fcp->previous_font;
	    else
		ch_call_data.previous_font = (OlFont) NULL;
	    XtCallCallbacks((Widget) fcw, XtNchangedCallback,
			    (XtPointer) &ch_call_data);
	} else {
	    OlFCErrorCallbackStruct	err_call_data;

	    err_call_data.reason	= OL_REASON_ERROR;
	    err_call_data.error_num	= OL_FC_ERR_FONT_UNAVAILABLE;
	    err_call_data.font_name	= fcp->current_font_name;
	    XtCallCallbacks((Widget) fcw, XtNerrorCallback,
			    (XtPointer) &err_call_data);
	}
	XtFree((XtPointer) cur_preview_txt);
	free(pattern);
    }
}

/* -------------------------- Private Functions --------------------------- */

static FontField **
get_typefaces(FontDB		fdb,
	      Cardinal *	typeface_count)
{
    FontFieldID	req_fields[4];
    Cardinal	field_count = 0;
	
    req_fields[field_count] = FC_FAMILY_NAME; field_count++;
    req_fields[field_count] = FC_SETWIDTH_NAME; field_count++;
    req_fields[field_count] = FC_ADD_STYLE_NAME; field_count++;
    req_fields[field_count] = FC_FOUNDRY; field_count++; 

    return(_OlQueryFontDatabase(fdb, req_fields, field_count,
			(ArgList) NULL, (Cardinal) 0, typeface_count));
}

static FontField **
get_styles(FontDB	fdb,
	   OlFontDesc * cur_fdesc,
	   Cardinal *	style_count)
{
    FontFieldID	req_fields[2];
    Arg		args[4];
    Cardinal	arg_count;
    Cardinal	field_count = 0;
	
    req_fields[field_count] = FC_WEIGHT_NAME; field_count++;
    req_fields[field_count] = FC_SLANT; field_count++;

    arg_count = 0;
    XtSetArg(args[arg_count], (String) FC_FAMILY_NAME,
	     cur_fdesc->components[0].fields[FC_FAMILY_NAME].name);
    arg_count++;
    XtSetArg(args[arg_count], (String) FC_SETWIDTH_NAME,
	     cur_fdesc->components[0].fields[FC_SETWIDTH_NAME].name);
    arg_count++;
    XtSetArg(args[arg_count], (String) FC_ADD_STYLE_NAME,
	     cur_fdesc->components[0].fields[FC_ADD_STYLE_NAME].name);
    arg_count++;
    XtSetArg(args[arg_count], (String) FC_FOUNDRY,
	     cur_fdesc->components[0].fields[FC_FOUNDRY].name);
    arg_count++;

    return(_OlQueryFontDatabase(fdb, req_fields, field_count,
				args, arg_count, style_count));
}

static FontField **
get_sizes(FontDB	fdb,
	   OlFontDesc * cur_fdesc,
	   Cardinal *	size_count)
{
    FontFieldID	req_fields[1];
    Arg		args[6];
    Cardinal	arg_count;
    Cardinal	field_count = 0;
	
    req_fields[field_count] = FC_POINT_SIZE; field_count++;

    arg_count = 0;
    XtSetArg(args[arg_count], (String) FC_FAMILY_NAME,
	     cur_fdesc->components[0].fields[FC_FAMILY_NAME].name);
    arg_count++;
    XtSetArg(args[arg_count], (String) FC_SETWIDTH_NAME,
	     cur_fdesc->components[0].fields[FC_SETWIDTH_NAME].name);
    arg_count++;
    XtSetArg(args[arg_count], (String) FC_ADD_STYLE_NAME,
	     cur_fdesc->components[0].fields[FC_ADD_STYLE_NAME].name);
    arg_count++;
    XtSetArg(args[arg_count], (String) FC_FOUNDRY,
	     cur_fdesc->components[0].fields[FC_FOUNDRY].name);
    arg_count++;
    XtSetArg(args[arg_count], (String) FC_WEIGHT_NAME,
	     cur_fdesc->components[0].fields[FC_WEIGHT_NAME].name);
    arg_count++;
    XtSetArg(args[arg_count], (String) FC_SLANT,
	     cur_fdesc->components[0].fields[FC_SLANT].name);
    arg_count++;

    return(_OlQueryFontDatabase(fdb, req_fields, field_count,
				args, arg_count, size_count));
}

static Widget	create_sl(FontChooserWidget	fcw,
			  String	inst_name,
			  Widget	parent,
			  OlStrRep	text_format,
			  Cardinal	list_height)
{
    Arg			args[20];
    Cardinal		n;
    FontChooserPart *	fcp = &(fcw->font_chooser);

    n = 0;
    XtSetArg(args[n], XtNtextFormat, text_format); n++;
    XtSetArg(args[n], XtNfont, fcp->font); n++;
    XtSetArg(args[n], XtNfontColor, fcp->font_color); n++;
    XtSetArg(args[n], XtNbackground, fcw->core.background_pixel); n++;
    XtSetArg(args[n], XtNbackgroundPixmap, fcw->core.background_pixmap); n++;
    XtSetArg(args[n], XtNforeground, fcp->foreground); n++;
    XtSetArg(args[n], XtNinputFocusColor, fcp->input_focus_color); n++;
    XtSetArg(args[n], XtNscale, fcp->scale); n++;
    XtSetArg(args[n], XtNviewHeight, (list_height > 0) ? list_height : 2);
    n++;
    return(XtCreateManagedWidget(inst_name, scrollingListWidgetClass,
				 parent, args, n));
}

static OlFont
get_ol_bold_font(Widget	w)
{
    OlFont	ret;
    XrmValue	from,
		to;

    from.addr = (char *) OlDefaultBoldFont;
    from.size = sizeof(OlDefaultBoldFont) - 1;

    to.size = sizeof(OlFont);
    to.addr = (XtPointer) &ret;

    XtConvertAndStore(w, XtRString, &from, XtROlFont, &to);
    return (ret);
}

static void
create_gui(FontChooserWidget fcw)
{
    FontChooserPart *	fcp = &(fcw->font_chooser);
    Arg			args[20];
    int			n;
    int			i;
    int			inter_obj_sp = fcp->scale + fcp->scale;
    int			horiz_sp = fcp->scale + fcp->scale/2;
    OlFont		cap_font = get_ol_bold_font((Widget) fcw);
    OlStrRep		attrib_tf = (fcp->text_format == OL_SB_STR_REP) ?
	OL_SB_STR_REP : OL_MB_STR_REP;

    /* Enable insertion of children */
    fcp->internal_insert = TRUE;
    
    /* Create top spacer for entire widget */
    fcp->fcw_top_sp = XtVaCreateManagedWidget("fcw_top_sp", 
	    coreWidgetClass, (Widget) fcw,
		    XtNweight,	0,
		    XtNheight,	fcp->scale,
		    XtNwidth,	1,
		    XtNsensitive,	FALSE,
		    XtNmappedWhenManaged,	FALSE,
		    XtNborderWidth,	0,
	    NULL);

    /* Create three rt as children */

    n = 0;
    XtSetArg(args[n], XtNtextFormat, attrib_tf); n++;
    XtSetArg(args[n], XtNborderWidth, fcw->core.border_width); n++;
    XtSetArg(args[n], XtNfontColor, fcp->font_color); n++;
    XtSetArg(args[n], XtNbackground, fcw->core.background_pixel); n++;
    XtSetArg(args[n], XtNbackgroundPixmap, fcw->core.background_pixmap); n++;
    XtSetArg(args[n], XtNforeground, fcp->foreground); n++;
    XtSetArg(args[n], XtNinputFocusColor, fcp->input_focus_color); n++;
    XtSetArg(args[n], XtNscale, fcp->scale); n++;
    XtSetArg(args[n], XtNorientation, OL_HORIZONTAL); n++;
    XtSetArg(args[n], XtNweight, 0); n++;
    XtSetArg(args[n], XtNspace, 0); n++;
    fcp->rt_attributes = XtCreateManagedWidget("rt5", rubberTileWidgetClass,
				 (Widget) fcw, args, n);
    n = 0;
    XtSetArg(args[n], XtNtextFormat, fcp->text_format); n++;
    XtSetArg(args[n], XtNfontColor, fcp->font_color); n++;
    XtSetArg(args[n], XtNborderWidth, fcw->core.border_width); n++;
    XtSetArg(args[n], XtNbackground, fcw->core.background_pixel); n++;
    XtSetArg(args[n], XtNbackgroundPixmap, fcw->core.background_pixmap); n++;
    XtSetArg(args[n], XtNforeground, fcp->foreground); n++;
    XtSetArg(args[n], XtNinputFocusColor, fcp->input_focus_color); n++;
    XtSetArg(args[n], XtNscale, fcp->scale); n++;
    XtSetArg(args[n], XtNweight, 0); n++;
    fcp->extension_area = XtCreateManagedWidget("rt_ext",
						rubberTileWidgetClass,
						(Widget) fcw, args, n);

    if (fcp->preview_present) {
	/* create the preview switch */
	n = 0;
	XtSetArg(args[n], XtNtextFormat, attrib_tf); n++;
	XtSetArg(args[n], XtNborderWidth, fcw->core.border_width); n++;
	XtSetArg(args[n], XtNfontColor, fcp->font_color); n++;
	XtSetArg(args[n], XtNbackground, fcw->core.background_pixel); n++;
	XtSetArg(args[n], XtNbackgroundPixmap, fcw->core.background_pixmap); n++;
	XtSetArg(args[n], XtNforeground, fcp->foreground); n++;
	XtSetArg(args[n], XtNinputFocusColor, fcp->input_focus_color); n++;
	XtSetArg(args[n], XtNscale, fcp->scale); n++;
	XtSetArg(args[n], XtNorientation, OL_HORIZONTAL); n++;
	XtSetArg(args[n], XtNweight, 0); n++;
	XtSetArg(args[n], XtNspace, horiz_sp); n++;
	fcp->rt_prev_sw = XtCreateManagedWidget("rt_preview",
					rubberTileWidgetClass,
					(Widget) fcw, args, n);

	/* Create left spacer: variable width */
	fcp->prv_swl_sp = XtVaCreateManagedWidget("prv_swl_sp", 
		coreWidgetClass, fcp->rt_prev_sw,
			XtNweight,	1,
			XtNheight,	1,
			XtNsensitive,	FALSE,
			XtNmappedWhenManaged,	FALSE,
			XtNborderWidth,	0,
		NULL);

	/* Create the Preview Exclusives */
	n = 0;
	XtSetArg(args[n], XtNtextFormat, attrib_tf); n++;
	XtSetArg(args[n], XtNborderWidth, fcw->core.border_width); n++;
	XtSetArg(args[n], XtNfontColor, fcp->font_color); n++;
	XtSetArg(args[n], XtNbackground, fcw->core.background_pixel); n++;
	XtSetArg(args[n], XtNbackgroundPixmap, fcw->core.background_pixmap); n++;
	XtSetArg(args[n], XtNforeground, fcp->foreground); n++;
	XtSetArg(args[n], XtNinputFocusColor, fcp->input_focus_color); n++;
	XtSetArg(args[n], XtNalignment, OL_CENTER); n++;
	XtSetArg(args[n], XtNposition, OL_LEFT); n++;
	XtSetArg(args[n], XtNweight, 0); n++;
	XtSetArg(args[n], XtNfont, cap_font); n++;
	XtSetArg(args[n], XtNlabel, fcp->preview_switch_label); n++;
        fcp->cap_preview = XtCreateManagedWidget("cap_preview",
					captionWidgetClass, fcp->rt_prev_sw,
					args, n);

	n = 0;
	XtSetArg(args[n], XtNtextFormat, attrib_tf); n++;
	XtSetArg(args[n], XtNborderWidth, fcw->core.border_width); n++;
	XtSetArg(args[n], XtNfontColor, fcp->font_color); n++;
	XtSetArg(args[n], XtNbackground, fcw->core.background_pixel); n++;
	XtSetArg(args[n], XtNbackgroundPixmap, fcw->core.background_pixmap); n++;
	XtSetArg(args[n], XtNforeground, fcp->foreground); n++;
	XtSetArg(args[n], XtNinputFocusColor, fcp->input_focus_color); n++;
	XtSetArg(args[n], XtNlayoutType, OL_FIXEDROWS); n++;
	XtSetArg(args[n], XtNmeasure, 1); n++;
	XtSetArg(args[n], XtNnoneSet, FALSE); n++;
        fcp->ex_preview = XtCreateManagedWidget("ex_preview",
					exclusivesWidgetClass, fcp->cap_preview,
					args, n);
	n = 0;
	XtSetArg(args[n], XtNtextFormat, attrib_tf); n++;
	XtSetArg(args[n], XtNborderWidth, fcw->core.border_width); n++;
	XtSetArg(args[n], XtNfontColor, fcp->font_color); n++;
	XtSetArg(args[n], XtNbackground, fcw->core.background_pixel); n++;
	XtSetArg(args[n], XtNbackgroundPixmap, fcw->core.background_pixmap); n++;
	XtSetArg(args[n], XtNforeground, fcp->foreground); n++;
	XtSetArg(args[n], XtNinputFocusColor, fcp->input_focus_color); n++;
	XtSetArg(args[n], XtNfont, fcp->font); n++;
	XtSetArg(args[n], XtNlabel, fcp->preview_switch_on_label); n++;
        fcp->rb_preview_on = XtCreateManagedWidget("preview_switch_on_label",
			      rectButtonWidgetClass, fcp->ex_preview,
			      args, n);

	n = 0;
	XtSetArg(args[n], XtNtextFormat, attrib_tf); n++;
	XtSetArg(args[n], XtNborderWidth, fcw->core.border_width); n++;
	XtSetArg(args[n], XtNfontColor, fcp->font_color); n++;
	XtSetArg(args[n], XtNbackground, fcw->core.background_pixel); n++;
	XtSetArg(args[n], XtNbackgroundPixmap, fcw->core.background_pixmap); n++;
	XtSetArg(args[n], XtNforeground, fcp->foreground); n++;
	XtSetArg(args[n], XtNinputFocusColor, fcp->input_focus_color); n++;
	XtSetArg(args[n], XtNfont, fcp->font); n++;
	XtSetArg(args[n], XtNlabel, fcp->preview_switch_off_label); n++;
        fcp->rb_preview_off = XtCreateManagedWidget("preview_switch_off_label",
			      rectButtonWidgetClass, fcp->ex_preview,
			      args, n);

	/* Create right spacer : fixed width */
	fcp->prv_swr_sp = XtVaCreateManagedWidget("prv_swr_sp",
		coreWidgetClass, fcp->rt_prev_sw,
			XtNweight,	0,
			XtNheight,	1,
			XtNwidth,	inter_obj_sp,
			XtNsensitive,	FALSE,
			XtNmappedWhenManaged,	FALSE,
			XtNborderWidth,	0,
		NULL);


	/* Create the preview area */
    	n = 0;
	XtSetArg(args[n], XtNtextFormat, attrib_tf); n++;
	XtSetArg(args[n], XtNborderWidth, fcw->core.border_width); n++;
	XtSetArg(args[n], XtNfontColor, fcp->font_color); n++;
	XtSetArg(args[n], XtNbackground, fcw->core.background_pixel); n++;
	XtSetArg(args[n], XtNbackgroundPixmap, fcw->core.background_pixmap); n++;
	XtSetArg(args[n], XtNforeground, fcp->foreground); n++;
	XtSetArg(args[n], XtNinputFocusColor, fcp->input_focus_color); n++;
	XtSetArg(args[n], XtNscale, fcp->scale); n++;
	XtSetArg(args[n], XtNorientation, OL_HORIZONTAL); n++;
	XtSetArg(args[n], XtNweight, 0); n++;
	XtSetArg(args[n], XtNspace, 3); n++;
	fcp->rt_preview = XtCreateManagedWidget("rt_preview",
					rubberTileWidgetClass,
					(Widget) fcw, args, n);

	/* Create left spacer */
	fcp->prv_lef_sp = XtVaCreateManagedWidget("prv_lef_sp", 
		coreWidgetClass, fcp->rt_preview,
			XtNweight,	0,
			XtNheight,	1,
			XtNwidth,	inter_obj_sp,
			XtNsensitive,	FALSE,
			XtNmappedWhenManaged,	FALSE,
			XtNborderWidth,	0,
		NULL);


	/* Create the Preview Area */
	n = 0;
	XtSetArg(args[n], XtNtextFormat, fcp->text_format); n++;
	if (fcp->preview_text != (String) NULL) {
	    XtSetArg(args[n], XtNlabel, fcp->preview_text); n++;
	}
	if (fcp->preview_height == 0 && fcp->num_pref_sizes > 0) {
	    int	max_pt_size = atoi(fcp->pref_size_list[
					fcp->num_pref_sizes-1]);
	    fcp->cur_preview_height = max_pt_size * 3;
	    XtSetArg(args[n], XtNheight, fcp->cur_preview_height); n++;
	}
	XtSetArg(args[n], XtNfont, fcp->font); n++;
	XtSetArg(args[n], XtNfontColor, fcp->preview_font_color); n++;
	XtSetArg(args[n], XtNbackground, fcp->preview_background); n++;
	XtSetArg(args[n], XtNforeground, fcp->preview_foreground); n++;
	XtSetArg(args[n], XtNinputFocusColor, fcp->input_focus_color); n++;
	XtSetArg(args[n], XtNscale, fcp->scale); n++;
	XtSetArg(args[n], XtNrecomputeSize, FALSE); n++;
	XtSetArg(args[n], XtNalignment, OL_CENTER); n++;
	XtSetArg(args[n], XtNgravity, CenterGravity); n++;
	XtSetArg(args[n], XtNborderWidth, fcp->preview_border_width); n++;
	XtSetArg(args[n], XtNspace, 0); n++;
	XtSetArg(args[n], XtNweight, 1); n++;

	fcp->st_preview = XtCreateManagedWidget("previewarea",
						staticTextWidgetClass,
						fcp->rt_preview,
						args, n); 
	/* Create right spacer */
	fcp->prv_rgt_sp = XtVaCreateManagedWidget("prv_rgt_sp", 
		coreWidgetClass, fcp->rt_preview,
			XtNweight,	0,
			XtNheight,	1,
			XtNwidth,	inter_obj_sp,
			XtNsensitive,	FALSE,
			XtNmappedWhenManaged,	FALSE,
			XtNborderWidth,	0,
		NULL);
    }
    
    n = 0;
    XtSetArg(args[n], XtNtextFormat, attrib_tf); n++;
    XtSetArg(args[n], XtNborderWidth, fcw->core.border_width); n++;
    XtSetArg(args[n], XtNfontColor, fcp->font_color); n++;
    XtSetArg(args[n], XtNbackground, fcw->core.background_pixel); n++;
    XtSetArg(args[n], XtNbackgroundPixmap, fcw->core.background_pixmap); n++;
    XtSetArg(args[n], XtNforeground, fcp->foreground); n++;
    XtSetArg(args[n], XtNinputFocusColor, fcp->input_focus_color); n++;
    XtSetArg(args[n], XtNscale, fcp->scale); n++;
    XtSetArg(args[n], XtNorientation, OL_HORIZONTAL); n++;
    XtSetArg(args[n], XtNweight, 0); n++;
    XtSetArg(args[n], XtNspace, horiz_sp); n++;
    fcp->rt_buttons = XtCreateManagedWidget("rt3", rubberTileWidgetClass,
				 (Widget) fcw, args, n);


    /* Create left spacer for attributes */
    fcp->attr_lef_sp = XtVaCreateManagedWidget("attr_lef_sp", 
	    coreWidgetClass, fcp->rt_attributes,
		    XtNweight,	0,
		    XtNheight,	1,
		    XtNwidth,	inter_obj_sp,
		    XtNsensitive,	FALSE,
		    XtNmappedWhenManaged,	FALSE,
		    XtNborderWidth,	0,
	    NULL);

    /* Create the typeface label and typeface list */

    n = 0;
    XtSetArg(args[n], XtNtextFormat, attrib_tf); n++;
    XtSetArg(args[n], XtNborderWidth, fcw->core.border_width); n++;
    XtSetArg(args[n], XtNfontColor, fcp->font_color); n++;
    XtSetArg(args[n], XtNbackground, fcw->core.background_pixel); n++;
    XtSetArg(args[n], XtNbackgroundPixmap, fcw->core.background_pixmap); n++;
    XtSetArg(args[n], XtNforeground, fcp->foreground); n++;
    XtSetArg(args[n], XtNinputFocusColor, fcp->input_focus_color); n++;
    XtSetArg(args[n], XtNscale, fcp->scale); n++;
    XtSetArg(args[n], XtNorientation, OL_VERTICAL); n++;
    XtSetArg(args[n], XtNweight, 0); n++;
    XtSetArg(args[n], XtNspace, 0); n++;
    fcp->rt_typeface = XtCreateManagedWidget("rt_typeface",
				    rubberTileWidgetClass,
				    fcp->rt_attributes,
				    args, n);

    n = 0;
    XtSetArg(args[n], XtNtextFormat, fcp->text_format); n++;
    XtSetArg(args[n], XtNborderWidth, 0); n++;
    XtSetArg(args[n], XtNfont, cap_font); n++;
    XtSetArg(args[n], XtNfontColor, fcp->font_color); n++;
    XtSetArg(args[n], XtNbackground, fcw->core.background_pixel); n++;
    XtSetArg(args[n], XtNbackgroundPixmap, fcw->core.background_pixmap); n++;
    XtSetArg(args[n], XtNforeground, fcp->foreground); n++;
    XtSetArg(args[n], XtNinputFocusColor, fcp->input_focus_color); n++;
    XtSetArg(args[n], XtNscale, fcp->scale); n++;
    XtSetArg(args[n], XtNrecomputeSize, TRUE); n++;
    XtSetArg(args[n], XtNbuttonType, OL_LABEL); n++;
    XtSetArg(args[n], XtNlabel, fcp->typeface_label); n++;
    fcp->but_typeface = XtCreateManagedWidget("typefacelabel",
					      buttonWidgetClass,
					      fcp->rt_typeface,
					      args, n);
    
    fcp->sl_typeface = create_sl(fcw, "typefacelist", fcp->rt_typeface,
				 attrib_tf, fcp->attribute_list_height);

    /* Create the size label and list */

    n = 0;
    XtSetArg(args[n], XtNtextFormat, attrib_tf); n++;
    XtSetArg(args[n], XtNborderWidth, fcw->core.border_width); n++;
    XtSetArg(args[n], XtNfontColor, fcp->font_color); n++;
    XtSetArg(args[n], XtNbackground, fcw->core.background_pixel); n++;
    XtSetArg(args[n], XtNbackgroundPixmap, fcw->core.background_pixmap); n++;
    XtSetArg(args[n], XtNforeground, fcp->foreground); n++;
    XtSetArg(args[n], XtNinputFocusColor, fcp->input_focus_color); n++;
    XtSetArg(args[n], XtNscale, fcp->scale); n++;
    XtSetArg(args[n], XtNorientation, OL_VERTICAL); n++;
    XtSetArg(args[n], XtNweight, 0); n++;
    XtSetArg(args[n], XtNspace, inter_obj_sp); n++;
    fcp->rt_size = XtCreateManagedWidget("rt_size",
				    rubberTileWidgetClass,
				    fcp->rt_attributes,
				    args, n);

    n = 0;
    XtSetArg(args[n], XtNtextFormat, fcp->text_format); n++;
    XtSetArg(args[n], XtNborderWidth, 0); n++;
    XtSetArg(args[n], XtNfont, cap_font); n++;
    XtSetArg(args[n], XtNfontColor, fcp->font_color); n++;
    XtSetArg(args[n], XtNbackground, fcw->core.background_pixel); n++;
    XtSetArg(args[n], XtNbackgroundPixmap, fcw->core.background_pixmap); n++;
    XtSetArg(args[n], XtNforeground, fcp->foreground); n++;
    XtSetArg(args[n], XtNinputFocusColor, fcp->input_focus_color); n++;
    XtSetArg(args[n], XtNscale, fcp->scale); n++;
    XtSetArg(args[n], XtNrecomputeSize, TRUE); n++;
    XtSetArg(args[n], XtNbuttonType, OL_LABEL); n++;
    XtSetArg(args[n], XtNlabel, fcp->size_label); n++;
    fcp->but_size = XtCreateManagedWidget("sizelabel",
					      buttonWidgetClass,
					      fcp->rt_size,
					      args, n);
    
    fcp->sl_size = create_sl(fcw, "sizelist", fcp->rt_size,
			     attrib_tf, fcp->attribute_list_height-2);

    /* Create size numeric text item */
    {
	int	del_val	= 1;
	int	min_val	= 1;
	
	n = 0;
	XtSetArg(args[n], XtNtextFormat, attrib_tf); n++;
	XtSetArg(args[n], XtNborderWidth, fcw->core.border_width); n++;
	XtSetArg(args[n], XtNsensitive, (XtArgVal) FALSE); n++;
	XtSetArg(args[n], XtNfont, fcp->font); n++;
	XtSetArg(args[n], XtNfontColor, fcp->font_color); n++;
	XtSetArg(args[n], XtNbackground, fcw->core.background_pixel); n++;
	XtSetArg(args[n], XtNbackgroundPixmap, fcw->core.background_pixmap); n++;
	XtSetArg(args[n], XtNforeground, fcp->foreground); n++;
	XtSetArg(args[n], XtNinputFocusColor, fcp->input_focus_color); n++;
	XtSetArg(args[n], XtNscale, fcp->scale); n++;
	XtSetArg(args[n], XtNtype, XtRInt); n++;
	XtSetArg(args[n], XtNminValue, &min_val); n++;
	XtSetArg(args[n], XtNmaxValue, &(fcp->maximum_point_size)); n++;
	{
	    /* Fastest way I know of finding the length of a bounded +ve integer */
	    int len = 0;

	    if (fcp->maximum_point_size < 1000)
		len++;
	    if (fcp->maximum_point_size < 100)
		len++;
	    if (fcp->maximum_point_size < 10)
		len++;

	    if (len > 0 && fcp->maximum_point_size > 0) {
		XtSetArg(args[n], XtNcharsVisible, len); n++;
	    } else
		OlWarning(dgettext(OlMsgsDomain,
				   "Bad value for XtNmaximumPointSize"));
	}
	XtSetArg(args[n], XtNdeltaState, OL_ACTIVE); n++;
	XtSetArg(args[n], XtNdelta, &del_val); n++;
	XtSetArg(args[n], XtNspace, fcp->scale); n++;
	fcp->ntf_size = XtCreateManagedWidget("sizentf",
					      numericFieldWidgetClass,
					      fcp->rt_size, args, n);
    }

    /* Create the style label and list */

    n = 0;
    XtSetArg(args[n], XtNtextFormat, attrib_tf); n++;
    XtSetArg(args[n], XtNborderWidth, fcw->core.border_width); n++;
    XtSetArg(args[n], XtNfontColor, fcp->font_color); n++;
    XtSetArg(args[n], XtNbackground, fcw->core.background_pixel); n++;
    XtSetArg(args[n], XtNbackgroundPixmap, fcw->core.background_pixmap); n++;
    XtSetArg(args[n], XtNforeground, fcp->foreground); n++;
    XtSetArg(args[n], XtNinputFocusColor, fcp->input_focus_color); n++;
    XtSetArg(args[n], XtNscale, fcp->scale); n++;
    XtSetArg(args[n], XtNorientation, OL_VERTICAL); n++;
    XtSetArg(args[n], XtNweight, 0); n++;
    XtSetArg(args[n], XtNspace, inter_obj_sp); n++;
    fcp->rt_style = XtCreateManagedWidget("rt_style",
				    rubberTileWidgetClass,
				    fcp->rt_attributes,
				    args, n);

    n = 0;
    XtSetArg(args[n], XtNtextFormat, fcp->text_format); n++;
    XtSetArg(args[n], XtNborderWidth, 0); n++;
    XtSetArg(args[n], XtNfont, cap_font); n++;
    XtSetArg(args[n], XtNfontColor, fcp->font_color); n++;
    XtSetArg(args[n], XtNbackground, fcw->core.background_pixel); n++;
    XtSetArg(args[n], XtNbackgroundPixmap, fcw->core.background_pixmap); n++;
    XtSetArg(args[n], XtNforeground, fcp->foreground); n++;
    XtSetArg(args[n], XtNinputFocusColor, fcp->input_focus_color); n++;
    XtSetArg(args[n], XtNscale, fcp->scale); n++;
    XtSetArg(args[n], XtNrecomputeSize, TRUE); n++;
    XtSetArg(args[n], XtNbuttonType, OL_LABEL); n++;
    XtSetArg(args[n], XtNlabel, fcp->style_label); n++;
    fcp->but_style = XtCreateManagedWidget("stylelabel",
					      buttonWidgetClass,
					      fcp->rt_style,
					      args, n);
    
    fcp->sl_style = create_sl(fcw, "stylelist", fcp->rt_style,
				 attrib_tf, fcp->attribute_list_height);

    /* Create right spacer for attributes */
    fcp->attr_rgt_sp = XtVaCreateManagedWidget("attr_rgt_sp", 
	    coreWidgetClass, fcp->rt_attributes,
		    XtNweight,	0,
		    XtNheight,	1,
		    XtNwidth,	inter_obj_sp,
		    XtNsensitive,	FALSE,
		    XtNmappedWhenManaged,	FALSE,
		    XtNborderWidth,	0,
	    NULL);

    /* Create the left_filler staticText */
    
    n = 0;
    XtSetArg(args[n], XtNtextFormat, fcp->text_format); n++;
    XtSetArg(args[n], XtNfont, fcp->font); n++;
    XtSetArg(args[n], XtNfontColor, fcp->preview_font_color); n++;
    XtSetArg(args[n], XtNbackground, fcp->preview_background); n++;
    XtSetArg(args[n], XtNforeground, fcp->preview_foreground); n++;
    XtSetArg(args[n], XtNinputFocusColor, fcp->input_focus_color); n++;
    XtSetArg(args[n], XtNscale, fcp->scale); n++;
    XtSetArg(args[n], XtNrecomputeSize, FALSE); n++;
    XtSetArg(args[n], XtNmappedWhenManaged, FALSE); n++;
    XtSetArg(args[n], XtNborderWidth, 0); n++;
    XtSetArg(args[n], XtNweight, 1); n++;
    (void) XtCreateManagedWidget("lfiller",
				 staticTextWidgetClass,
				 fcp->rt_buttons, args, n); 

    /* Create the Apply Button */

    n = 0;
    XtSetArg(args[n], XtNtextFormat, fcp->text_format); n++;
    XtSetArg(args[n], XtNborderWidth, fcw->core.border_width); n++;
    XtSetArg(args[n], XtNlabel, fcp->apply_label); n++;
    XtSetArg(args[n], XtNfont, fcp->font); n++;
    XtSetArg(args[n], XtNfontColor, fcp->font_color); n++;
    XtSetArg(args[n], XtNbackground, fcw->core.background_pixel); n++;
    XtSetArg(args[n], XtNbackgroundPixmap, fcw->core.background_pixmap); n++;
    XtSetArg(args[n], XtNforeground, fcp->foreground); n++;
    XtSetArg(args[n], XtNinputFocusColor, fcp->input_focus_color); n++;
    XtSetArg(args[n], XtNscale, fcp->scale); n++;
    XtSetArg(args[n], XtNweight, 0); n++;
    fcp->ob_apply = XtCreateManagedWidget("applybutton",
					  oblongButtonGadgetClass,
					  fcp->rt_buttons,
					  args, n);

    /* Create the Revert Button */

    n = 0;
    XtSetArg(args[n], XtNtextFormat, fcp->text_format); n++;
    XtSetArg(args[n], XtNborderWidth, fcw->core.border_width); n++;
    XtSetArg(args[n], XtNlabel, fcp->revert_label); n++;
    XtSetArg(args[n], XtNfont, fcp->font); n++;
    XtSetArg(args[n], XtNfontColor, fcp->font_color); n++;
    XtSetArg(args[n], XtNbackground, fcw->core.background_pixel); n++;
    XtSetArg(args[n], XtNbackgroundPixmap, fcw->core.background_pixmap); n++;
    XtSetArg(args[n], XtNforeground, fcp->foreground); n++;
    XtSetArg(args[n], XtNinputFocusColor, fcp->input_focus_color); n++;
    XtSetArg(args[n], XtNscale, fcp->scale); n++;
    XtSetArg(args[n], XtNweight, 0); n++;
    XtSetArg(args[n], XtNspace, fcp->scale); n++;
    fcp->ob_revert = XtCreateManagedWidget("revertbutton",
					   oblongButtonGadgetClass,
					   fcp->rt_buttons,
					   args, n);

    /* Create the Cancel Button */

    n = 0;
    XtSetArg(args[n], XtNtextFormat, fcp->text_format); n++;
    XtSetArg(args[n], XtNborderWidth, fcw->core.border_width); n++;
    XtSetArg(args[n], XtNlabel, fcp->cancel_label); n++;
    XtSetArg(args[n], XtNfont, fcp->font); n++;
    XtSetArg(args[n], XtNfontColor, fcp->font_color); n++;
    XtSetArg(args[n], XtNbackground, fcw->core.background_pixel); n++;
    XtSetArg(args[n], XtNbackgroundPixmap, fcw->core.background_pixmap); n++;
    XtSetArg(args[n], XtNforeground, fcp->foreground); n++;
    XtSetArg(args[n], XtNinputFocusColor, fcp->input_focus_color); n++;
    XtSetArg(args[n], XtNscale, fcp->scale); n++;
    XtSetArg(args[n], XtNweight, 0); n++;
    XtSetArg(args[n], XtNspace, fcp->scale); n++;
    fcp->ob_cancel = XtCreateManagedWidget("cancelbutton",
					   oblongButtonGadgetClass,
					   fcp->rt_buttons,
					   args, n);

    /* Create the right_filler staticText */
    
    n = 0;
    XtSetArg(args[n], XtNtextFormat, fcp->text_format); n++;
    XtSetArg(args[n], XtNfont, fcp->font); n++;
    XtSetArg(args[n], XtNfontColor, fcp->preview_font_color); n++;
    XtSetArg(args[n], XtNbackground, fcp->preview_background); n++;
    XtSetArg(args[n], XtNforeground, fcp->preview_foreground); n++;
    XtSetArg(args[n], XtNinputFocusColor, fcp->input_focus_color); n++;
    XtSetArg(args[n], XtNscale, fcp->scale); n++;
    XtSetArg(args[n], XtNrecomputeSize, FALSE); n++;
    XtSetArg(args[n], XtNmappedWhenManaged, FALSE); n++;
    XtSetArg(args[n], XtNborderWidth, 0); n++;
    XtSetArg(args[n], XtNweight, 1); n++;
    (void) XtCreateManagedWidget("rfiller",
				 staticTextWidgetClass,
				 fcp->rt_buttons, args, n); 

    /* Disable insertion of any more children */
    fcp->internal_insert = FALSE;
}				/* End of create_gui */

static void
add_internal_callbacks(FontChooserWidget fcw,
		       XtCallbackProc	 apply_CB,
		       XtCallbackProc	 revert_CB,
		       XtCallbackProc	 cancel_CB)
{
    FontChooserPart *	fcp = &(fcw->font_chooser);
    int		i;

    /* Private callbacks */

    XtAddCallback(fcp->sl_typeface,
		  XtNuserMakeCurrent, typeface_listCB,
		  (XtPointer) fcw);
    XtAddCallback(fcp->sl_style,
		  XtNuserMakeCurrent, style_listCB,
		  (XtPointer) fcw);
    XtAddCallback(fcp->sl_size,
		  XtNuserMakeCurrent, size_listCB,
		  (XtPointer) fcw);

    XtAddCallback(fcp->ntf_size, XtNvalidateCallback,
		  size_ntfCB, (XtPointer) fcw);

    XtAddCallback(fcp->rb_preview_on, XtNselect,
		  preview_on_rbCB, (XtPointer) fcw);

    XtAddCallback(fcp->rb_preview_off, XtNselect,
		  preview_off_rbCB, (XtPointer) fcw);

    /* Meta Callbacks */

    if (apply_CB != (XtCallbackProc) NULL)
	XtAddCallback(fcp->ob_apply, XtNselect, apply_CB, (XtPointer) fcw);
    if (revert_CB != (XtCallbackProc) NULL)
	XtAddCallback(fcp->ob_revert, XtNselect, revert_CB, (XtPointer) fcw);
    if (cancel_CB != (XtCallbackProc) NULL)
	XtAddCallback(fcp->ob_cancel, XtNselect, cancel_CB, (XtPointer) fcw);
}

static void
preview_on_rbCB(Widget w , XtPointer c, XtPointer ca)
{
    FontChooserWidget	fcw = (FontChooserWidget) c;
    FontChooserPart *	fcp = &(fcw->font_chooser);

    fcp->internal_preview = TRUE;
    PreviewFont(fcw);
}

static void
preview_off_rbCB(Widget w , XtPointer c, XtPointer ca)
{
    FontChooserWidget	fcw = (FontChooserWidget) c;
    FontChooserPart *	fcp = &(fcw->font_chooser);

    fcp->internal_preview = FALSE;
    PreviewFont(fcw);
}

static void
size_ntfCB(Widget w , XtPointer c, XtPointer ca)
{
    FontChooserWidget	fcw = (FontChooserWidget) c;
    FontChooserPart *	fcp = &(fcw->font_chooser);
    String		label;
    int			new_val;
    FontField		tmp;
    Widget		tf       = fcp->ntf_size;
    int			pts;
    char		temp_label[SIZE_FIELD_SIZE];
    register int	i;
    OlNFValidateCallbackStruct *	commit_cd;

    if (((OlAnyCallbackStruct *) ca)->reason != OL_REASON_VALIDATE)
	return;

    new_val = *((int *)((OlNFValidateCallbackStruct *) ca)->value);

    if (new_val > 0) {
	int	point_size = new_val;

	if (fcp->size_ntf_label != (String) NULL) {
	    if (atoi(fcp->size_ntf_label) == point_size) {
		return;
	    } else
		XtFree((XtPointer) fcp->size_ntf_label);
	}
	snprintf(temp_label, SIZE_FIELD_SIZE, "%d", point_size);
	fcp->size_ntf_label = XtNewString(temp_label);
    } else
	fcp->size_ntf_label = (String) NULL;
    
    if (fcp->size_ntf_label == (String) NULL || *fcp->size_ntf_label == '\0')
	return;

    pts = atoi(fcp->size_ntf_label);
    snprintf(temp_label, SIZE_FIELD_SIZE, "%d", pts * 10);
    if (fcp->size_ntf_name != (String) NULL)
	XtFree((XtPointer) fcp->size_ntf_name);
    fcp->size_ntf_name = XtNewString(temp_label);
    
    tmp.name  = fcp->size_ntf_name;
    tmp.quark = XrmStringToQuark(fcp->size_ntf_name);
    fcp->cur_fdesc->components[0].fields[FC_POINT_SIZE] = tmp;
    
    if (fcp->prev_size_count > 0) {
	static void         (*ListTouchItem)(Widget, OlListToken);
	static void         (*ListViewItem)(Widget, OlListToken);

	/* Get the methods assoc. with the size list */
	XtVaGetValues(fcp->sl_size,
		      XtNapplTouchItem, (XtArgVal) &ListTouchItem,
		      XtNapplViewItem, (XtArgVal) &ListViewItem,
		      NULL);
	    
	for (i = 0; i < fcp->prev_size_count; i++) {
	    OlListItem * item = OlListItemPointer(fcp->size_tokens[i]);

	    if (! strcmp(fcp->size_ntf_label, item->label))
		break;
	}
	if (i != fcp->prev_size_count) { /* There is a match */
	    if (fcp->size_tokens[i] != fcp->cur_siz_tok) {
		if (fcp->cur_siz_tok != 0) {
		    OlListItem *	prev_item;
		    
		    prev_item = OlListItemPointer(fcp->cur_siz_tok);
		    prev_item->attr &= ~OL_LIST_ATTR_CURRENT;
		    (*ListTouchItem)(fcp->sl_size, fcp->cur_siz_tok);
		}
		make_item_current(fcp->sl_size, fcp->size_tokens[i]);

		fcp->cur_siz_tok = fcp->size_tokens[i];
		if (fcp->last_size != (String) NULL)
		    XtFree((XtPointer) fcp->last_size);
		fcp->last_size = XtNewString(OlListItemPointer(fcp->cur_siz_tok)->label);
	    }
	} else {
	    if (fcp->cur_siz_tok != 0) {
		OlListItem *	prev_item;
		    
		prev_item = OlListItemPointer(fcp->cur_siz_tok);
		prev_item->attr &= ~OL_LIST_ATTR_CURRENT;
		(*ListTouchItem)(fcp->sl_size, fcp->cur_siz_tok);

		fcp->cur_siz_tok = (OlListToken) 0;
		if (fcp->last_size != (String) NULL)
		    XtFree((XtPointer) fcp->last_size);
		fcp->last_size = (String) NULL;
	    }
	}
    }
    PreviewFont(fcw);
}

static void
typeface_listCB(Widget w , XtPointer c , XtPointer callData)
{
    FontChooserWidget	fcw = (FontChooserWidget) c;
    FontChooserPart *	fcp = &(fcw->font_chooser);
    OlListItem *	new_item = OlListItemPointer((OlListToken) callData);
    OlListItem *	prev_item;
    OlListToken		token = (OlListToken) callData;
    static void         (*ListTouchItem)(Widget, OlListToken);
    static void         (*ListViewItem)(Widget, OlListToken);

    FontField *		f;
    Cardinal		size_count;
    Cardinal		style_count;
 
    if (fcp->cur_typeface_tok != token) {
	/* Get the methods assoc. with the typeface list */
	XtVaGetValues(w,
		      XtNapplTouchItem, (XtArgVal)&ListTouchItem,
		      XtNapplViewItem, (XtArgVal)&ListViewItem,
		      NULL);

	/* Do whatever you want here */
	f = (FontField *) new_item->user_data;

	/* Set the current Typeface Attributes */ 
	
	fcp->cur_fdesc->components[0].fields[FC_FAMILY_NAME]	  = f[0];
	fcp->cur_fdesc->components[0].fields[FC_SETWIDTH_NAME]  = f[1];
	fcp->cur_fdesc->components[0].fields[FC_ADD_STYLE_NAME] = f[2];
	fcp->cur_fdesc->components[0].fields[FC_FOUNDRY]        = f[3];

	/* Get the style list corresponding to the selected typeface */

	if (fcp->style_data != (FontField **) NULL)
		_OlFreeFontFieldLists(fcp->style_data, style_field_count);
	fcp->style_data = get_styles(fcp->fdb, fcp->cur_fdesc, &style_count);
	if (fcp->style_data == (FontField **) NULL)
	  OlWarning(dgettext(OlMsgsDomain, "No matches for styles found \n"));
	else {
	    populate_style_list(fcw, fcp->style_data, style_count,
				style_field_count);

	    if (fcp->size_data != (FontField **) NULL)
		_OlFreeFontFieldLists(fcp->size_data, size_field_count);
	    fcp->size_data = get_sizes(fcp->fdb, fcp->cur_fdesc, &size_count);
	    if (fcp->size_data == (FontField **) NULL)
	      OlWarning(dgettext(OlMsgsDomain,
				"No matches for sizes found \n"));
	    else {
		populate_size_list(fcw, fcp->size_data, size_count,
				   size_field_count);
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
		if (fcp->cur_typeface_tok != 0) {
		    prev_item = OlListItemPointer(fcp->cur_typeface_tok);
		    prev_item->attr &= ~OL_LIST_ATTR_CURRENT;
		    (*ListTouchItem)(w, fcp->cur_typeface_tok);
		}
		/*
		 * Keep track of currently selected item in list
		 */
		fcp->cur_typeface_tok = token;
		PreviewFont(fcw);
	    }
	}
    }
}

static void
style_listCB(Widget w, XtPointer c , XtPointer callData)
{
    FontChooserWidget	fcw = (FontChooserWidget) c;
    FontChooserPart *	fcp = &(fcw->font_chooser);
    OlListItem *	new_item = OlListItemPointer((OlListToken) callData);
    OlListItem *	prev_item;
    OlListToken		token = (OlListToken) callData;
    static void         (*ListTouchItem)(Widget, OlListToken);
    static void         (*ListViewItem)(Widget, OlListToken);

    FontField *		f;
    Cardinal		size_count;
    register int	i;

    if(fcp->cur_style_tok != token) {
	/* Get the methods assoc. with the size list */
        XtVaGetValues(w,
                      XtNapplTouchItem, (XtArgVal) &ListTouchItem,
                      XtNapplViewItem, (XtArgVal) &ListViewItem,
                      NULL);

	/* Do whatever you want here */
        f = (FontField *) new_item->user_data;

	/* Set the current Style Attributes */
	fcp->cur_fdesc->components[0].fields[FC_WEIGHT_NAME]	= f[0];
	fcp->cur_fdesc->components[0].fields[FC_SLANT]		= f[1];

	if (fcp->size_data != (FontField **) NULL)
	    _OlFreeFontFieldLists(fcp->size_data, size_field_count);
        fcp->size_data = get_sizes(fcp->fdb, fcp->cur_fdesc, &size_count);

	if (fcp->last_style != (String) NULL)
	    XtFree((XtPointer) fcp->last_style);
	fcp->last_style = XtNewString(new_item->label);

	if (fcp->size_data  == (FontField **) NULL)
	    OlWarning(dgettext(OlMsgsDomain,
			       "No matches for sizes found \n"));
	else {
	    populate_size_list(fcw, fcp->size_data, size_count, size_field_count);
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
	    if (fcp->cur_style_tok != 0) {
		prev_item = OlListItemPointer(fcp->cur_style_tok);
		prev_item->attr &= ~OL_LIST_ATTR_CURRENT;
		(*ListTouchItem)(w, fcp->cur_style_tok);
	    }
	    fcp->cur_style_tok = token;
	    PreviewFont(fcw);
	}
    } 
}

static void
size_listCB(Widget w, XtPointer c , XtPointer callData)
{
    FontChooserWidget	fcw = (FontChooserWidget) c;
    FontChooserPart *	fcp = &(fcw->font_chooser);
    OlListItem *	new_item = OlListItemPointer((OlListToken) callData); 
    OlListItem *	prev_item; 
    OlListToken		token	 = (OlListToken) callData; 
    static void		(*ListTouchItem)(Widget, OlListToken); 
    static void		(*ListViewItem)(Widget, OlListToken); 
    Cardinal		size_count;
    register int	i;

    FontField *		f;

    if (fcp->cur_siz_tok != token) {
	/* Get the methods assoc. with the size list */
        XtVaGetValues(w,
                      XtNapplTouchItem, (XtArgVal) &ListTouchItem,
                      XtNapplViewItem, (XtArgVal) &ListViewItem,
                      NULL);
	/* Do whatever you want here */
        f = (FontField *) new_item->user_data;

	/* Set the current Point Size Attributes */
	fcp->cur_fdesc->components[0].fields[FC_POINT_SIZE] = f[0];

	/* Update the text field */
	{
	    int	val	= atoi(new_item->label);
	    
	    XtVaSetValues(fcp->ntf_size, XtNvalue, &val, NULL);
	}
	if (fcp->last_size != (String) NULL)
	    XtFree((XtPointer) fcp->last_size);
	fcp->last_size = XtNewString(new_item->label);
	
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
	if (fcp->cur_siz_tok != 0) {
	    prev_item = OlListItemPointer(fcp->cur_siz_tok);
	    prev_item->attr &= ~OL_LIST_ATTR_CURRENT;
	    (*ListTouchItem)(w, fcp->cur_siz_tok);
	}
	fcp->cur_siz_tok = token;
	PreviewFont(fcw);
    }
}

static int
field_strcmp(const void * a1, const void * a2)
{
    ListItemSortRec *	li1 = (ListItemSortRec *) a1;
    ListItemSortRec *	li2 = (ListItemSortRec *) a2;

    return(strcmp(li1->label, li2->label));
}

static int
size_cmp(const void * a1, const void * a2)
{
    ListItemSortRec *	li1 = (ListItemSortRec *) a1;
    ListItemSortRec *	li2 = (ListItemSortRec *) a2;
    
    return(atoi(li1->label) - atoi(li2->label));
}

static void
populate_typeface_list(FontChooserWidget	fcw,
		       FontField **	typeface_list_data, 
		       int		num_typefaces,
		       int 		num_fields)
{
    FontChooserPart *	fcp = &(fcw->font_chooser);
    static OlListToken  (*ListAddItem)(Widget, OlListToken,
				       OlListToken, OlListItem);
    static void         (*ListDeleteItem)(Widget, OlListToken);
    static void         (*UpdateView)(Widget, Boolean);
    OlListItem *	listitem;
    int			i;
    int			j;
    char		*compound_typeface;
    FontField *		typeface_item_data;
    ListItemSortRec *	list_sort_data;
    Cardinal		sel_token_i;

    if (num_typefaces <= 0)
		return;
	
	if (!(compound_typeface = malloc(TYPEFACE_FIELD_SIZE)))
		return;

    fcp->prev_typeface_count = num_typefaces;

    /* Get the methods assoc. with the typeface list */
    XtVaGetValues(fcp->sl_typeface,
		  XtNapplUpdateView, (XtArgVal) &UpdateView,
		  XtNapplAddItem, (XtArgVal) &ListAddItem,
		  XtNapplDeleteItem, (XtArgVal) &ListDeleteItem,
		  NULL);

    /* Malloc for listitem data */
    /* Create list items one by one and add them to the list */
    list_sort_data = (ListItemSortRec *) XtMalloc(sizeof(ListItemSortRec) *
						  num_typefaces);
    for(i=0; i < num_typefaces; i++) {
	typeface_item_data = (FontField *) XtMalloc(num_fields * 
						    sizeof(FontField));
	strcpy(compound_typeface, "");
	for(j=0; j < num_fields; j++) {
	    register String	trav;
	    int			state = 0;
	    
	    if ((typeface_list_data[j])[i].name[0] != '\0') {
		String temp = XtNewString((typeface_list_data[j])[i].name);

		temp[0] = toupper(temp[0]);
		switch (j) {
		case 0:		/* FC_FAMILY_NAME */
		    for (trav = &(temp[1]); *trav != '\0'; trav++) {
			switch (state) {
			case 0:
			    if (*trav == ' ')
				state = 1;
			    break;
			case 1:
			    if (*trav != ' ') {
				*trav = toupper(*trav);
				state = 0;
			    }
			    break;
			}
		    }
		    break;
		case 1:		/* FC_SETWIDTH_NAME */
		    if (! strcmp(temp, "Normal"))
			temp[0] = '\0';
		    break;
		}
		if (temp[0] != '\0') {
		    strcat(compound_typeface, typeface_field_prefix[j]);
		    strcat(compound_typeface, temp);
		    strcat(compound_typeface, typeface_field_suffix[j]);
		}
		XtFree((XtPointer) temp);
	    }
	    typeface_item_data[j] = (typeface_list_data[j])[i];
	}
	list_sort_data[i].label  = XtNewString(compound_typeface);
	list_sort_data[i].fields = typeface_item_data;
    }

    qsort(list_sort_data, num_typefaces,
	  sizeof(ListItemSortRec), field_strcmp);
    
    listitem = (OlListItem *) XtCalloc(num_typefaces, sizeof(OlListItem));

    fcp->typeface_tokens = (OlListToken *) XtMalloc(sizeof(OlListToken) *
						    num_typefaces);
    UpdateView(fcp->sl_typeface, FALSE);
    sel_token_i = 0;
    for(i=0; i < num_typefaces; i++) {
	String	label = list_sort_data[i].label;
	
	if (fcp->last_typeface != (String) NULL) {
	    if (!strcmp(fcp->last_typeface, label))
		sel_token_i = i;
	}
	listitem[i].label_type = (OlDefine) OL_STRING;
	listitem[i].label = label;
	listitem[i].attr = 0;
	listitem[i].user_data = (XtPointer) list_sort_data[i].fields;
	fcp->typeface_tokens[i] = (*ListAddItem)(fcp->sl_typeface,
						 NULL, NULL, listitem[i]); 
    }
    
    XtFree((XtPointer) list_sort_data);
    XtFree((XtPointer) listitem);
    
    /* Make fcp->typeface_tokens[sel_token_i] to be selected, if i > 0 */
    if ((i > 0)) {
	OlListItem *	item	= OlListItemPointer(
				    fcp->typeface_tokens[sel_token_i]);
	FontField *	f	= (FontField *) item->user_data;
	
	make_item_current(fcp->sl_typeface, fcp->typeface_tokens[sel_token_i]);
	fcp->cur_typeface_tok = fcp->typeface_tokens[sel_token_i];
	fcp->cur_fdesc->components[0].fields[FC_FAMILY_NAME]    = f[0];
	fcp->cur_fdesc->components[0].fields[FC_SETWIDTH_NAME]  = f[1];
	fcp->cur_fdesc->components[0].fields[FC_ADD_STYLE_NAME] = f[2];
	fcp->cur_fdesc->components[0].fields[FC_FOUNDRY]        = f[3];
    }
    UpdateView(fcp->sl_typeface, TRUE);
	free(compound_typeface);
}

static void
make_item_current(Widget	w,
		   OlListToken	token)
{
    OlListItem *	new_item;
    static void         (*ListTouchItem)(Widget, OlListToken);
    static void         (*ListViewItem)(Widget, OlListToken);

    XtVaGetValues(w,
		  XtNapplTouchItem, (XtArgVal)&ListTouchItem,
		  XtNapplViewItem, (XtArgVal)&ListViewItem,
		  NULL);
    /*
     * Mark the item as selected or current by setting a bit
     * in its attribute field.  Notify the widget that we have
     * touched an item.
     */
    new_item = OlListItemPointer(token);
    new_item->attr |= OL_LIST_ATTR_CURRENT;
    (*ListTouchItem)(w, token);

    (*ListViewItem)(w, token);
}

static String
get_full_slant_name(String	short_name)
{
    register int	i;

    for (i = 0; slant_name_map[i] != (String) NULL; i += 2)
      if (!strcmp(slant_name_map[i], short_name))
	return(slant_name_map[i+1]);
    return("Unknown");
}

static void
populate_style_list(FontChooserWidget	fcw,
		    FontField **	style_list_data, 
		    int			num_styles,
		    int			num_fields)
{
    FontChooserPart *	fcp = &(fcw->font_chooser);
    static void         (*UpdateView)(Widget, Boolean);
    static OlListToken  (*ListAddItem) (Widget, OlListToken,
					OlListToken, OlListItem);
    static void		(*ListDeleteItem) (Widget, OlListToken);
    OlListItem *	listitem;
    int			i;
    int			j;
    FontField *		style_item_data;
    char		*compound_style;
    ListItemSortRec *	list_sort_data;
    Cardinal		sel_token_i;

    /* Get the methods assoc. with the typeface list */
    XtVaGetValues(fcp->sl_style,
		  XtNapplUpdateView, (XtArgVal) &UpdateView,
		  XtNapplAddItem, (XtArgVal) &ListAddItem,
		  XtNapplDeleteItem, (XtArgVal) &ListDeleteItem,
		  NULL);

    /* First delete any items that might be there */

    UpdateView(fcp->sl_style, FALSE);
    for(i = fcp->prev_style_count-1; i >= 0; i--) {
	if(fcp->style_tokens) {
	    OlListItem *	item = OlListItemPointer(fcp->style_tokens[i]);

	    XtFree((XtPointer) item->user_data);
	    (*ListDeleteItem)((Widget) fcp->sl_style, fcp->style_tokens[i]);
	}
    }
    if (fcp->prev_style_count > 0)
	XtFree((XtPointer) fcp->style_tokens);
    fcp->prev_style_count = num_styles;

    if (num_styles <= 0) {
	UpdateView(fcp->sl_style, TRUE);
	return;
    }
    
	if (!(compound_style = malloc(STYLE_FIELD_SIZE)))
		return;

    /* Create list items one by one and add them to the list */
    list_sort_data = (ListItemSortRec *) XtMalloc(sizeof(ListItemSortRec) *
						  num_styles);
    for(i=0; i < num_styles; i++) {
	style_item_data = (FontField *) XtMalloc(num_fields * 
						 sizeof(FontField));
	strcpy(compound_style, "");
	for(j=0; j < num_fields; j++) {
	    String	temp;

	    if ((style_list_data[j])[i].name[0] != '\0') {
		if (j == 1) {	/* FC_SLANT */
                    temp = get_full_slant_name((style_list_data[j])[i].name);
                    strcat(compound_style, style_field_prefix[j]);
                    strcat(compound_style, temp);
		} else {
                    temp = XtNewString((style_list_data[j])[i].name);
                    temp[0] = toupper(temp[0]);
                    strcat(compound_style, style_field_prefix[j]);
                    strcat(compound_style, temp);
                    XtFree((XtPointer) temp);
		}
                strcat(compound_style, style_field_suffix[j]);
	    }
	    style_item_data[j] = (style_list_data[j])[i];
	}
	list_sort_data[i].label  = XtNewString(compound_style);
	list_sort_data[i].fields = style_item_data;
    }

    qsort(list_sort_data, num_styles,
	  sizeof(ListItemSortRec), field_strcmp);
    
    listitem = (OlListItem *) XtCalloc(num_styles, sizeof(OlListItem));

    fcp->style_tokens = (OlListToken *) XtMalloc(sizeof(OlListToken) * num_styles);
    sel_token_i = 0;
    for(i=0; i < num_styles; i++) {
	String	label = list_sort_data[i].label;
	
	if (fcp->last_style != (String) NULL) {
	    if (!strcmp(fcp->last_style, label))
		sel_token_i = i;
	}
	listitem[i].label_type = (OlDefine) OL_STRING;
	listitem[i].label = list_sort_data[i].label;
	listitem[i].attr = 0;
	listitem[i].user_data = (XtPointer) list_sort_data[i].fields;
	fcp->style_tokens[i] = (*ListAddItem)((Widget) fcp->sl_style,
				   NULL, NULL, listitem[i]); 
    }
    XtFree((XtPointer) list_sort_data);
    XtFree((XtPointer) listitem);

    /* Make sel_token_i'th token to be selected, if i > 0 */
    if (i > 0) {
	OlListItem *	item = OlListItemPointer(fcp->style_tokens[sel_token_i]);
	FontField *	f    = (FontField *) item->user_data;
	
	make_item_current(fcp->sl_style, fcp->style_tokens[sel_token_i]);
	if (fcp->last_style != (String) NULL)
	    XtFree((XtPointer) fcp->last_style);
	fcp->last_style = XtNewString(item->label);
	fcp->cur_style_tok = fcp->style_tokens[sel_token_i];
	fcp->cur_fdesc->components[0].fields[FC_WEIGHT_NAME] = f[0];
	fcp->cur_fdesc->components[0].fields[FC_SLANT]       = f[1];
    }
    UpdateView(fcp->sl_style, TRUE);
	free(compound_style);
}

static void
populate_size_list(FontChooserWidget	fcw,
		   FontField **	size_list_data, 
		   int		num_sizes,
		   int		num_fields)
{
    FontChooserPart *	fcp = &(fcw->font_chooser);
    static void         (*UpdateView)(Widget, Boolean);
    static OlListToken	(*ListAddItem)(Widget, OlListToken,
					OlListToken, OlListItem);
    static void		(*ListDeleteItem)(Widget, OlListToken);
    OlListItem *	listitem;
    int			i;
    int			j;
    FontField *		size_item_data;
    char		*compound_size;
    ListItemSortRec *	list_sort_data;
    ListItemSortRec *	list_trav_data;
    int			sel_token_i;

    /* Get the methods assoc. with the typeface list */
    XtVaGetValues(fcp->sl_size,
		  XtNapplUpdateView, (XtArgVal) &UpdateView,
		  XtNapplAddItem, (XtArgVal) &ListAddItem,
		  XtNapplDeleteItem, (XtArgVal) &ListDeleteItem,
		  NULL);

    /* First delete any items that might be there */
    UpdateView(fcp->sl_size, FALSE);
    for(i = fcp->prev_size_count-1; i >= 0; i--) {
	if(fcp->size_tokens) {
	    OlListItem *	item = OlListItemPointer(fcp->size_tokens[i]);
	    XtFree((XtPointer) item->user_data);
	    (*ListDeleteItem)((Widget)fcp->sl_size, fcp->size_tokens[i]);
	}
    }
    if (fcp->prev_size_count > 0)
	XtFree((XtPointer) fcp->size_tokens);
    fcp->prev_size_count = num_sizes;
    
    if (num_sizes <= 0) {
	UpdateView(fcp->sl_size, TRUE);
	return;
    }

    list_sort_data = (ListItemSortRec *) XtMalloc(num_sizes *
						  sizeof(ListItemSortRec));
    for (i = 0; i < num_sizes; i++) {
	int	val;
	
	size_item_data = (FontField *) XtMalloc(num_fields * 
						sizeof(FontField));
	if (compound_size = malloc(SIZE_FIELD_SIZE)) {
		strncpy(compound_size, (size_list_data[0])[i].name, SIZE_FIELD_SIZE);
		val = atoi(compound_size);
		snprintf(compound_size, SIZE_FIELD_SIZE, "%d", val/10);
		*size_item_data = (size_list_data[0])[i];
		list_sort_data[i].label  = XtNewString(compound_size);
		free(compound_size);
	}
	list_sort_data[i].fields = size_item_data;
    }

    qsort(list_sort_data, num_sizes,
	  sizeof(ListItemSortRec), size_cmp);
    
    /* Malloc for listitem data */

    if (! strcmp(list_sort_data[0].label, "0")) {
	fcp->is_scalable = (Boolean) TRUE;
	num_sizes--;
	fcp->prev_size_count--;
	list_trav_data = &(list_sort_data[1]);
    } else {
	fcp->is_scalable = (Boolean) FALSE;
	list_trav_data = list_sort_data;
    }
    XtSetSensitive(fcp->ntf_size, fcp->is_scalable);

    if (fcp->is_scalable) {		/* Merge with preferred list; already sorted */
	int			merge_count;
	ListItemSortRec *	merged_list;
	Cardinal		merged_list_size;
	
	merge_count = i = j = 0;
	merged_list_size = num_sizes + fcp->num_pref_sizes;
	merged_list = (ListItemSortRec *) XtMalloc(sizeof(ListItemSortRec) *
						   merged_list_size);
	do {
	    int diff;
	    
	    if (i == num_sizes || j == fcp->num_pref_sizes)
		break;

	    diff = atoi(list_trav_data[i].label) -
		atoi(fcp->sorted_prefsizes[j].label);

	    if (diff < 0) {	/* first is smaller */
		merged_list[merge_count++] = list_trav_data[i++];
	    } else if (diff > 0) { /* second is smaller */
		FontField * temp = (FontField *) XtMalloc(sizeof(FontField));

		*temp = fcp->sorted_prefsizes[j].fields[0];
		merged_list[merge_count].label  = fcp->sorted_prefsizes[j].label;
                merged_list[merge_count].fields = temp;
		j++; merge_count++;
	    } else { /* both are equal */
		merged_list[merge_count++] = list_trav_data[i++];
		j++;
	    }
	} while (i < num_sizes && j < fcp->num_pref_sizes);

	if (i < num_sizes)
	    for (; i < num_sizes; i++)
		merged_list[merge_count++] = list_trav_data[i];
	if (j < fcp->num_pref_sizes)
	    for (; j < fcp->num_pref_sizes; j++) {
		FontField * temp = (FontField *) XtMalloc(sizeof(FontField));

		*temp = fcp->sorted_prefsizes[j].fields[0];
		merged_list[merge_count].label  = fcp->sorted_prefsizes[j].label;
                merged_list[merge_count].fields = temp;
		merge_count++;
	    }

	if (merge_count == 0) {
	    printf("Should not happen.");
	} else if (merge_count < merged_list_size)
	    merged_list = (ListItemSortRec *)
		XtRealloc((XtPointer) merged_list,
			  sizeof(ListItemSortRec) * merge_count);
	num_sizes = fcp->prev_size_count = merge_count;
	XtFree((XtPointer) list_sort_data);
	list_trav_data = list_sort_data = merged_list;
    }

    if (num_sizes > 0) {
	listitem = (OlListItem *) XtCalloc(num_sizes, sizeof(OlListItem));

	/* Create list items one by one and add them to the list */

	fcp->size_tokens = (OlListToken *) XtMalloc(sizeof(OlListToken) *
					       num_sizes);
	sel_token_i = 0;
	for(i = 0; i < num_sizes; i++) {
	    String	label = list_trav_data[i].label;
	    
	    if (fcp->last_size != (String) NULL) {
		if (!strcmp(fcp->last_size, label)) {
		    sel_token_i = i;
		}
	    }
	    listitem[i].label_type = (OlDefine) OL_STRING;
	    listitem[i].label = label;
	    listitem[i].attr = 0;
	    listitem[i].user_data = (XtPointer) list_trav_data[i].fields;
	    fcp->size_tokens[i] = (*ListAddItem)((Widget) fcp->sl_size,
					    NULL, NULL, listitem[i]);
	}
	XtFree((XtPointer) listitem);
    }
    XtFree((XtPointer) list_sort_data);
    
    /* Make sel_token_i'th token to be selected, if there is at least one */
    if (num_sizes > 0) {
	OlListItem *	item = OlListItemPointer(fcp->size_tokens[sel_token_i]);
	FontField *	f    = (FontField *) item->user_data;
	register int	i;
	
	make_item_current(fcp->sl_size, fcp->size_tokens[sel_token_i]);
	if (fcp->last_size != (String) NULL)
	    XtFree((XtPointer) fcp->last_size);
	fcp->last_size  = XtNewString(item->label);
	fcp->cur_siz_tok = fcp->size_tokens[sel_token_i];
	fcp->cur_fdesc->components[0].fields[FC_POINT_SIZE] = f[0];
	{
	    int	val	= atoi(item->label);
	    
	    XtVaSetValues(fcp->ntf_size, XtNvalue, &val, NULL);
	}
    }
    UpdateView(fcp->sl_size, TRUE);
}

/*
   Customized for small strings known to be smaller than
   FC_MAXFONTLEN and only for temporary use
 */   
static wchar_t *
cvt_to_wc(String	mb_str)
{
   static wchar_t	 temp[FC_MAXFONTLEN];

   (void) mbstowcs(temp, mb_str, FC_MAXFONTLEN);
   return(temp);
}

static void
capitalize_wc_phrase(wchar_t *	val)
{
    register wchar_t *	trav;
    int			state = 1;
    
    for (trav = val; *trav != L'\0'; trav++) {
	switch (state) {
	case 0:
	    if (*trav == L' ' || *trav == L'-')
		state = 1;
	    break;
	case 1:
	    if (*trav != L' ' && *trav != L'-') {
		*trav = towupper(*trav);
		state = 0;
	    }
	    break;
	}
    }
}

static void
capitalize_phrase(char *	val)
{
    register char *	trav;
    int			state = 1;
    
    for (trav = val; *trav != '\0'; trav++) {
	switch (state) {
	case 0:
	    if (*trav == ' ' || *trav == '-')
		state = 1;
	    break;
	case 1:
	    if (*trav != ' ' && *trav != '-') {
		*trav = toupper(*trav);
		state = 0;
	    }
	    break;
	}
    }
}

static wchar_t *
generate_wc_preview(FontChooserWidget	fcw,
		    wchar_t *		preview_text)
{
    /* assumes text given in widechar */
    register int	i;
    register int	j;
    FontChooserPart *	fcp = &(fcw->font_chooser);
    OlFontComponent *	fcomp =
	&(fcp->cur_fdesc->components[0]);
    wchar_t *		prv_text = preview_text;
    wchar_t *		pattern;
    wchar_t *		gen_text;
    register wchar_t *  cur_ch;
    int			state = 0;
    size_t		clen;

    if  (prv_text == (wchar_t *) NULL)
	return((wchar_t *) NULL);

    clen = wslen(prv_text);
    gen_text = (wchar_t *) XtCalloc(1, clen * sizeof(wchar_t)
				    + FC_MAXFONTLEN);
    gen_text[0] = L'\0';
    
    state = 0;
    for (i=0, cur_ch = prv_text; *cur_ch != L'\0'; cur_ch++) {
	if (state == 1) {
	    if (*cur_ch == L'%') {
		gen_text[i++] = *cur_ch;
		state = 0;
		continue;
	    } else if  (*cur_ch == L'T') {
		wchar_t	val[FC_MAXFONTLEN];

		for (j = 0; j < XtNumber(preview_typeface); j++) {
		    if (fcomp->fields[preview_typeface[j]].name[0] != '\0') {
			wscpy(val, cvt_to_wc(
			     fcomp->fields[preview_typeface[j]].name));
			switch(preview_typeface[j]) {
			case FC_SETWIDTH_NAME:
			    if (! wscmp(val, L"normal")) {
				val[0] = L'\0';
				break;
			    }
			default:
			    capitalize_wc_phrase(val);
			    break;
			}
			if (val[0] != L'\0') {
			    wscat(gen_text, cvt_to_wc(prefix_typeface[j]));
			    wscat(gen_text, val);
			    i = wslen(gen_text);
			}
		    }
		}
		state = 0;
	    } else if  (*cur_ch == L'S') {
		wchar_t	val[FC_MAXFONTLEN];

		for (j = 0; j < XtNumber(preview_style); j++) {
		    if (fcomp->fields[preview_style[j]].name[0] != '\0') {
			wscpy(val, cvt_to_wc(
			     fcomp->fields[preview_style[j]].name));
			switch(preview_style[j]) {
			case FC_SLANT:
			    wscpy(val, cvt_to_wc(get_full_slant_name(
				fcomp->fields[preview_style[j]].name)));
			    break;
			default:
			    capitalize_wc_phrase(val);
			    break;
			}
			if (val[0] != L'\0') {
			    wscat(gen_text, cvt_to_wc(prefix_style[j]));
			    wscat(gen_text, val);
			    i = wslen(gen_text);
			}
		    }
		}
		state = 0;
	    } else if  (*cur_ch == L's') {
		wchar_t	val[FC_MAXFONTLEN];

		for (j = 0; j < XtNumber(preview_size); j++) {
		    if (fcomp->fields[preview_size[j]].name[0] != '\0') {
			int		deci_pts;
	    
			wscpy(val, cvt_to_wc(
				     fcomp->fields[preview_size[j]].name));
			switch(preview_size[j]) {
			case FC_POINT_SIZE:
			    deci_pts = watoi(val);
			    wsprintf(val, "%d", deci_pts/10);
			    break;
			default:
			    capitalize_wc_phrase(val);
			    break;
			}
			if (val[0] != L'\0') {
			    wscat(gen_text, cvt_to_wc(prefix_size[j]));
			    wscat(gen_text, val);
			    i = wslen(gen_text);
			}
		    }
		}
		state = 0;
	    }
	} else {
	    if (*cur_ch == L'%') {
		state = 1;
	    } else {
		gen_text[i++] = *cur_ch;
	    }
	}
    }
    gen_text[i] = L'\0';
    return(gen_text);
}

static char *
generate_sb_preview(FontChooserWidget	fcw,
		    char *		preview_text)
{
    /* assumes text given in singlebyte */
    register int	i;
    register int	j;
    FontChooserPart *	fcp = &(fcw->font_chooser);
    OlFontComponent *	fcomp =
	&(fcp->cur_fdesc->components[0]);
    char *		prv_text = preview_text;
    char *		pattern;
    char *		gen_text;
    register char *	cur_ch;
    int			state = 0;
    size_t		clen;

    if  (prv_text == (char *) NULL)
	return((char *) NULL);

    clen = strlen(prv_text);
    gen_text = (char *) XtCalloc(1, clen * sizeof(char)
				 + FC_MAXFONTLEN);
    gen_text[0] = '\0';
    
    state = 0;
    for (i=0, cur_ch = prv_text; *cur_ch != '\0'; cur_ch++) {
	if (state == 1) {
	    if (*cur_ch == '%') {
		gen_text[i++] = *cur_ch;
		state = 0;
		continue;
	    } else if  (*cur_ch == 'T') {
			char	*val;

			if (val = malloc(FC_MAXFONTLEN)) {
				for (j = 0; j < XtNumber(preview_typeface); j++) {
					if (fcomp->fields[preview_typeface[j]].name[0] != '\0') {
					int		state = 1;
					int		deci_pts;
				
					strncpy(val, fcomp->fields[preview_typeface[j]].name, FC_MAXFONTLEN);
					switch(preview_typeface[j]) {
					case FC_SETWIDTH_NAME:
						if (! strcmp(val, "normal")) {
						val[0] = '\0';
						break;
						}
					default:
						capitalize_phrase(val);
						break;
					}
					if (val[0] != '\0') {
						strcat(gen_text, prefix_typeface[j]);
						strcat(gen_text, val);
						i = strlen(gen_text);
					}
					}
				}
				state = 0;
				free(val);
			}
		} else if  (*cur_ch == 'S') {
			char	*val;
	
			if (val = malloc(FC_MAXFONTLEN)) {
				for (j = 0; j < XtNumber(preview_style); j++) {
					if (fcomp->fields[preview_style[j]].name[0] != '\0') {
					int		state = 1;
					int		deci_pts;
				
					strncpy(val, fcomp->fields[preview_style[j]].name, FC_MAXFONTLEN);
					switch(preview_style[j]) {
					case FC_SLANT:
						strncpy(val, get_full_slant_name(
							fcomp->fields[preview_style[j]].name), FC_MAXFONTLEN);
						break;
					default:
						capitalize_phrase(val);
						break;
					}
					if (val[0] != '\0') {
						strcat(gen_text, prefix_style[j]);
						strcat(gen_text, val);
						i = strlen(gen_text);
					}
					}
				}
				state = 0;
				free(val);
			}
	    } else if  (*cur_ch == 's') {
			char    *val;

			if (val = malloc(FC_MAXFONTLEN)) {
				for (j = 0; j < XtNumber(preview_size); j++) {
					if (fcomp->fields[preview_size[j]].name[0] != '\0') {
					int		deci_pts;
				
					strncpy(val, fcomp->fields[preview_size[j]].name, FC_MAXFONTLEN);
					switch(preview_size[j]) {
					case FC_POINT_SIZE:
						deci_pts = atoi(val);
						snprintf(val, FC_MAXFONTLEN, "%d", deci_pts/10);
						break;
					default:
						capitalize_phrase(val);
						break;
					}
					if (val[0] != '\0') {
						strcat(gen_text, prefix_size[j]);
						strcat(gen_text, val);
						i = strlen(gen_text);
					}
					}
				}
				state = 0;
				free(val);
			}
		}
	} else {
	    if (*cur_ch == '%') {
		state = 1;
	    } else {
		gen_text[i++] = *cur_ch;
	    }
	}
    }
    gen_text[i] = '\0';
    return(gen_text);
}

static String
initial_typeface_label(
		String	ini_family_name,
		String	ini_setwidth_name,
		String	ini_add_style_name,
		String	ini_foundry)
{
    char	*label;
    int		j;
    String	temp;
    String	trav;
	
    j = 0;
	if (!(label = malloc(TYPEFACE_FIELD_SIZE)))
		return(NULL);

    *label = '\0';
    if (ini_family_name[0] != '\0') {
	int    state = 0;

	temp = XtNewString(ini_family_name);
	temp[0] = toupper(temp[0]);
	for (trav = &(temp[1]); *trav != '\0'; trav++) {
	    switch (state) {
	    case 0:
		if (*trav == ' ')
		    state = 1;
		break;
	    case 1:
		if (*trav != ' ') {
		    *trav = toupper(*trav);
		    state = 0;
		}
		break;
	    }
	}
	strcat(label, typeface_field_prefix[j]);
	strcat(label, temp);
	strcat(label, typeface_field_suffix[j]);
	XtFree((XtPointer) temp);
    }
    j++;
    if (ini_setwidth_name[0] != '\0') {
	temp = XtNewString(ini_setwidth_name);
	temp[0] = toupper(temp[0]);
	if (strcmp(temp, "Normal")) {
	    strcat(label, typeface_field_prefix[j]);
	    strcat(label, temp);
	    strcat(label, typeface_field_suffix[j]);
	}
	XtFree((XtPointer) temp);
    }
    j++;
    if (ini_add_style_name[0] != '\0') {
	temp = XtNewString(ini_add_style_name);
	temp[0] = toupper(temp[0]);
	strcat(label, typeface_field_prefix[j]);
	strcat(label, temp);
	strcat(label, typeface_field_suffix[j]);
	XtFree((XtPointer) temp);
    }
    j++;
    if (ini_foundry[0] != '\0') {
	temp = XtNewString(ini_foundry);
	temp[0] = toupper(temp[0]);
	strcat(label, typeface_field_prefix[j]);
	strcat(label, temp);
	strcat(label, typeface_field_suffix[j]);
	XtFree((XtPointer) temp);
    }
    temp = XtNewString(label);
	free(label);
    return(temp);
}

static String
initial_style_label(
		String	ini_slant,
		String	ini_weight_name)
{
    char	*label;
    int		j;
    String	temp;
	
    j = 0;
	if (label = malloc(STYLE_FIELD_SIZE)) {
		*label = '\0';
		if (ini_weight_name[0] != '\0') {
			temp = XtNewString(ini_weight_name);
			temp[0] = toupper(temp[0]);
			strcat(label, typeface_field_prefix[j]);
			strcat(label, temp);
			strcat(label, typeface_field_suffix[j]);
			XtFree((XtPointer) temp);
		}
		j++;
		if (ini_slant[0] != '\0') {
			temp = get_full_slant_name(ini_slant);
			strcat(label, style_field_prefix[j]);
			strcat(label, temp);
		}
		temp = XtNewString(label);
		free(label);
	} else
		temp = NULL;
	return(temp);
}

static String
initial_size_label(String	ini_point_size)
{
    char		label[SIZE_FIELD_SIZE];
    int			val;

    val = atoi(ini_point_size);
    snprintf(label, SIZE_FIELD_SIZE, "%d", val/10);
    return(XtNewString(label));
}

