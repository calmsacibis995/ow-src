#ifndef _XOL_NUMERICFIEL_H
#define _XOL_NUMERICFIEL_H

#pragma ident	"@(#)NumericFie.h	1.1	92/10/08 lib/libXol SMI"	/* OLIT	*/

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

#include	<X11/Intrinsic.h>
#include	<Xol/Primitive.h>
#include	<Xol/TextLine.h>

#ifdef  __cplusplus
extern "C" {
#endif


typedef struct _NumericFieldRec           *NumericFieldWidget;
typedef struct _NumericFieldClassRec      *NumericFieldWidgetClass;

extern WidgetClass      numericFieldWidgetClass;

/* NumericField Calldata structures */

/* for XtNdeltaCallback */
typedef struct
{
	int                     reason;
	XEvent                  *event;
	XtPointer               delta;
	XtPointer               current_value;
	OlDefine                current_delta_state;	
	XtPointer               new_value;
	OlDefine                new_delta_state;	
	Boolean			update;
} OlNFDeltaCallbackStruct;

/* for XtNvalidateCallback */
typedef struct
{
	int                     reason;
	XEvent                  *event;
	XtPointer               value;
	OlDefine                delta_state;	
	Boolean			update;
	Boolean                 valid;
} OlNFValidateCallbackStruct;

typedef Boolean (*OlNFConvertProc) (Widget w,  OlStrRep format, 
				    XrmQuark from_type, XrmQuark to_type, 
				    XtPointer *value, 
				    XtPointer *string, Cardinal *string_length
				   );


#ifdef  __cplusplus
}
#endif

#endif /* _XOL_NUMERICFIEL_H */
