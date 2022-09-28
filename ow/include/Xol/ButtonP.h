#ifndef	_XOL_BUTTONP_H
#define	_XOL_BUTTONP_H

#pragma	ident	"@(#)ButtonP.h	302.5	93/02/12 include/Xol SMI"	/* button:include/openlook/ButtonP.h 1.24 	*/

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

/*	Copyright (c) 1990 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#include <Xol/Button.h>
#include <Xol/EventObjP.h>	/* include gadget's superclass header */
#include <Xol/OlgxP.h>
#include <Xol/PrimitiveP.h>	/* include widget's superclass header */

#include <X11/CoreP.h>
#include <X11/IntrinsicP.h>


#ifdef	__cplusplus
extern "C" {
#endif


#define find_button_part(w)	(_OlIsGadget((Widget)w) ? \
	&((ButtonGadget)(w))->button : &((ButtonWidget)(w))->button)


#define find_button_font_color(w)	(_OlIsGadget((Widget)w) ? \
	(((ButtonGadget)(w))->event.font_color) : \
	(((ButtonWidget) (w))->primitive.font_color))

#define find_button_font(w)	(_OlIsGadget((Widget)w) ? \
	(((ButtonGadget)(w))->event.font) : \
	(((ButtonWidget)(w))->primitive.font))
 
#define find_button_foreground(w)	(_OlIsGadget((Widget)w) ? \
	(((ButtonGadget)(w))->event.foreground) : \
	(((ButtonWidget)(w))->primitive.foreground))

#define find_text_format(w)	(_OlIsGadget((Widget)w) ? \
	(((ButtonGadget)(w))->event.text_format ) : \
	(((ButtonWidget)(w))->primitive.text_format))

#define find_button_scale(w)     (_OlIsGadget((Widget)w) ? \
        (((ButtonGadget)(w))->event.scale ) : \
        (((ButtonWidget)(w))->primitive.scale))

#define XtCeil(n)		((int)(n + .5))

#define isOblongType(t)		((int)(t == OL_OBLONG || \
	t == OL_BUTTONSTACK || t == OL_HALFSTACK) ? TRUE : FALSE)

#define NORMAL		1
#define HIGHLIGHTED	0

typedef struct {
    unsigned    	callbackType;
    Drawable		win;
    Graphics_info	*ginfo;
    Position		x, y;
    Dimension		width, height;
    OlDefine		justification;
    Boolean		set;
    Boolean		sensitive;
} ButtonProcLbl;

/*
 *  There are no new fields for the ButtonClassPart
 */
typedef struct _ButtonClass {
	int 			makes_compiler_happy;	/* not used */
}			ButtonClassPart;

/*
 *  declare the ButtonClassRec as a subclass of core
 */
typedef struct _ButtonClassRec {
	CoreClassPart		core_class;
	PrimitiveClassPart	primitive_class;
	ButtonClassPart		button_class;
}			ButtonClassRec;

/*
 *  declare the ButtonGadgetClassRec as a subclass of EventObj
 */
typedef struct _ButtonGadgetClassRec {
	RectObjClassPart	rect_class;
	EventObjClassPart	event_class;
	ButtonClassPart		button_class;
}                       ButtonGadgetClassRec;


/*
 *  buttonClassRec and buttonGadgetClassRec are defined in Button.c
 */
extern ButtonClassRec   	buttonClassRec;
extern ButtonGadgetClassRec	buttonGadgetClassRec;

/*
 *  declaration of the Button widget fields
 */
typedef struct {
	/*
	 * Public resources
	 */
	Pixel                   background_pixel;
	OlStr			label;
	OlDefine                text_format;
	XImage*			label_image;
	OlDefine                label_type;
	OlDefine                button_type;
	OlDefine                menumark;
	OlDefine                label_justify;
	int                     shell_behavior;
	Boolean                 label_tile;
	Boolean                 is_default;
	Boolean                 set;
	Boolean                 dim;
	Boolean                 busy;
	Boolean                 internal_busy;
	Boolean                 recompute_size;
	XtCallbackList          select;
	XtCallbackList          unselect;
	Widget                  preview;
	XtCallbackList          post_select;
	XtCallbackList          label_proc;

	/*
	 * Private fields
	 */
	Dimension               normal_height;
	Dimension               normal_width;
	GC                      normal_GC;
	GC                      inverse_text_GC;
	OlgxAttrs*		pAttrs;
	OlgxAttrs*		pHighlightAttrs;
	Boolean			menu_has_meta;	/* for button labels on menus
						   only,to line up meta marks */
}                       ButtonPart;

typedef struct _ButtonRec {
	CorePart                core;
	PrimitivePart           primitive;
	ButtonPart              button;
}                       ButtonRec;

typedef struct _ButtonGadgetRec {
	ObjectPart              object;
	RectObjPart             rect;
	EventObjPart            event;
	ButtonPart              button;
}                       ButtonGadgetRec;


#if	defined(__STDC__) || defined(__cplusplus)

extern void		_OlButtonPreview(ButtonWidget dest, ButtonWidget src);
extern void		_OlDrawHighlightButton(ButtonWidget bw);
extern void		_OlDrawNormalButton(ButtonWidget bw);

#else	/* __STDC__ || __cplusplus */

extern void		_OlButtonPreview();
extern void		_OlDrawHighlightButton();
extern void		_OlDrawNormalButton();

#endif	/* __STDC__ || __cplusplus */


#ifdef	__cplusplus
}
#endif


#endif	/* _XOL_BUTTONP_H */
