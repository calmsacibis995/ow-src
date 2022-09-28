#ifndef	_XOL_FONTCHP_H
#define	_XOL_FONTCHP_H

#pragma	ident	"@(#)FontChP.h	1.8	93/05/21 include/Xol SMI"	/* OLIT */

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
 ************************************************************************
 *
 * Description:
 *	This is the "private" header file for the FontChooser Widget
 *
 *****************************file*header********************************
 */

#include <X11/Intrinsic.h>
/*
#include <X11/CoreP.h>
#include <X11/CompositeP.h>
#include <X11/ConstrainP.h>
*/
#include <Xol/ManagerP.h>
#include <Xol/RubberTilP.h>
#include <Xol/ScrollingL.h>


#include <Xol/FontCh.h>

#ifdef	__cplusplus
extern "C" {
#endif

/*
 ***********************************************************************
 *
 * Widget Private Data
 *
 ***********************************************************************
 */

/* Dynamic resources bit masks */
#define	OL_FONTC_FOREGROUND		(1 << 0)
#define	OL_FONTC_FONTCOLOR		(1 << 1)
#define OL_FONTC_PREVIEWFONTCOLOR	(1 << 2)
#define OL_FONTC_PREVIEWFOREGROUND	(1 << 3)


typedef struct _FontDBRec *	FontDB;	/* opaque */
typedef struct _FontFieldRec *	FontFieldList;	/* opaque */
typedef struct _OWFsetDBRec *	OWFsetDB;	/* opaque */
typedef struct _OlFontDescRec *	OlFontDescPtr;	/* opaque */

typedef struct _ListItemSortRec {
    String		label;
    FontFieldList	fields;
} ListItemSortRec;

			/* New fields for the widget class record	*/

typedef struct {
	int keep_compiler_happy;   /* No new procedures, for now */
} FontChooserClassPart;

				/* Full class record declaration 	*/

typedef struct _FontChooserClassRec {
  	CoreClassPart		core_class;
	CompositeClassPart	composite_class;
	ConstraintClassPart	constraint_class;
	ManagerClassPart	manager_class;
	RubberTileClassPart	rubber_tile_class;
	FontChooserClassPart	font_chooser_class;
} FontChooserClassRec;

extern FontChooserClassRec fontChooserClassRec;

/*
 ***********************************************************************
 *
 * Instance (widget) structure 
 *
 ***********************************************************************
 */

				/* New fields for the widget record	*/

typedef struct {
    /* Resources */
    OlStrRep		text_format;		/* XtNtextFormat	*/
    OlFont		font;			/* XtNfont		*/
    Pixel		font_color;		/* XtNfontColor		*/
    Pixel		foreground;		/* XtNforeground	*/
    Pixel		input_focus_color;	/* XtNinputFocusColor	*/
    int			scale;			/* XtNscale		*/
    Dimension		preview_height;		/* XtNpreviewHeight	*/
    Dimension		preview_border_width;	/* XtNpreviewBorderWidth*/
    Pixel		preview_background;	/* XtNpreviewBackground	*/
    Pixel		preview_font_color;	/* XtNpreviewFontColor	*/
    Pixel		preview_foreground;	/* XtNpreviewForeground	*/
    String		font_search_spec;	/* XtNfontSearchSpec	*/
    String		charset_info;		/* XtNcharsetInfo	*/
    String		initial_font_name;	/* XtNinitialFontName	*/
    Cardinal		maximum_point_size;	/* XtNmaximumPointSize  */
    String		preferred_point_sizes;	/* XtNpreferredPointSizes */
    Dimension		attribute_list_height;	/* XtNattributeListHeight */
    Widget		extension_area;		/* XtNextensionArea	*/
    Boolean		preview_present;	/* XtNpreviewPresent	*/
    OlStr		no_preview_text;	/* XtNnoPreviewText	*/
    OlStr		preview_text;		/* XtNpreviewText	*/
    OlStr		preview_switch_label;	/* XtNpreviewSwitchLabel*/
    OlStr		preview_switch_on_label;/* XtNpreviewSwitchOnLabel*/
    OlStr		preview_switch_off_label;/* XtNpreviewSwitchOffLabel*/
    XtCallbackList	apply_callback;		/* XtNapplyCallback	*/
    XtCallbackList	revert_callback;	/* XtNrevertCallback	*/
    XtCallbackList	cancel_callback;	/* XtNcancelCallback	*/
    XtCallbackList	changed_callback;	/* XtNchangedCallback	*/
    XtCallbackList	error_callback;		/* XtNerrorCallback	*/
    OlStr		apply_label;		/* XtNapplyLabel	*/
    OlStr		revert_label;		/* XtNrevertLabel	*/
    OlStr		cancel_label;		/* XtNcancelLabel	*/
    OlStr		typeface_label;		/* XtNtypefaceLabel	*/
    OlStr		style_label;		/* XtNstyleLabel	*/
    OlStr		size_label;		/* XtNsizeLabel		*/

    /* Private */
/* ==== GUI ==== */
    Widget		rt_top;
    Widget   		rt_attributes;
    Widget		rt_buttons;

    Widget		rt_style;
    Widget		but_style;
    Widget		sl_style;
    Widget		rt_size;
    Widget		but_size;
    Widget		sl_size;
    Widget		ntf_size;
    Widget		ob_cancel;
    Widget		ob_revert;
    Widget		ob_apply;
    Widget		rt_prev_sw;
    Widget		cap_preview;
    Widget		ex_preview;
    Widget		rb_preview_on;
    Widget		rb_preview_off;
    Widget		rt_preview;
    Widget		st_preview;

    Widget		rt_typeface;
    Widget		but_typeface;
    Widget		sl_typeface;

    Widget		fcw_top_sp;
    Widget		prv_lef_sp;
    Widget		prv_rgt_sp;
    Widget		attr_lef_sp;
    Widget		attr_rgt_sp;
    Widget		prv_swl_sp;
    Widget		prv_swr_sp;

    OlListToken		cur_typeface_tok;
    Cardinal		typeface_count;

    Cardinal		prev_size_count;
    Cardinal		prev_style_count;
    Cardinal		prev_typeface_count;

    OlListToken *	typeface_tokens;	
    OlListToken *	style_tokens;
    OlListToken *	size_tokens;

    OlListToken		cur_style_tok;
    OlListToken		cur_siz_tok;
    String		size_ntf_label;
    String		size_ntf_name;

    String		last_typeface;
    String		last_style;
    String		last_size;

    Cardinal		cur_preview_height;

/* ==== Database manipulation ==== */

    FontDB  		fdb;
    OWFsetDB		fsdb;

    FontFieldList *	typeface_data;
    FontFieldList *	style_data;
    FontFieldList *	size_data;
    
    ListItemSortRec *	sorted_prefsizes;
    
    OlFontDescPtr	cur_fdesc; /* Keeps the current font desc. */

    Boolean		is_scalable;

/* ==== Charset info ==== */

    String		registry;
    String		encoding;

/* ==== Preferred sizes ==== */
    String *		pref_size_list;
    Cardinal		num_pref_sizes;

/* ==== Font trail ==== */
    OlFont		current_font;
    String		current_font_name;

    OlFont		previous_font;
    String		previous_font_name;

    OlFont		initial_font;

/* ==== To insert or not to insert, that is the question ===== */
    Boolean		internal_insert;

/* ==== To preview or not to preview, that is the question ===== */
    Boolean		internal_preview;

/* ==== Dynamic resource spec ===== */
    unsigned char	dynamic_resources_flags;
} FontChooserPart;

					/* Full Widget declaration	*/
typedef struct _FontChooserRec {
	CorePart 	core;
	CompositePart 	composite;
	ConstraintPart	constraint;
	ManagerPart	manager;
	RubberTilePart	rubber_tile;
	FontChooserPart	font_chooser;
} FontChooserRec;

/*
 * Constraint record:
 */

typedef struct {
	int			no_fields;
} FontChooserConstraintPart;

typedef struct _FontChooserConstraintRec {
	RubberTileConstraintRec		rubber_tile;
	FontChooserConstraintPart	font_chooser;
} FontChooserConstraintRec, *FontChooserConstraint;

#ifdef	__cplusplus
}
#endif

#endif	/* _XOL_FONTCHP_H */
