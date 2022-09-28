#ifndef _XOL_NUMERICFIP_H
#define _XOL_NUMERICFIP_H

#pragma ident	"@(#)NumericFiP.h	1.2	92/10/08 lib/libXol SMI"	/* OLIT	*/

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

#ifdef  __cplusplus
extern "C" {
#endif

#include 	<Xol/TextLineP.h>
#include	<Xol/NumericFie.h>

typedef XtPointer OlNFData;

/*************************************************************************
 *
 *      Definition Of The Class structure
 *
 *************************************************************************/

typedef struct _NumericFieldClassPart {
	XtPointer extension;
} NumericFieldClassPart;

typedef struct _NumericFieldClassRec {
	CoreClassPart           core_class;
	PrimitiveClassPart      primitive_class;
	TextLineClassPart       textLine_class;
	NumericFieldClassPart	numericField_class;
} NumericFieldClassRec;

/* External definition for class record */
extern NumericFieldClassRec numericFieldClassRec;


/***********************************************************************
 *
 *      Definition Of The Instance Structure
 *
 ***********************************************************************/

typedef struct {
	/* 	Public Resources */
	XtPointer		value;
	String			type;
	XtPointer		delta;
	OlDefine		delta_state;

	XtPointer		max_value;
	XtPointer		min_value;

	OlNFConvertProc		convert_proc;
	Cardinal		size_of;

	XtCallbackList		delta_callback;
	XtCallbackList		validate_callback;

	/*	Private Data 	*/
	Dimension		delta_width;
	XtIntervalId		delta_timer;
	int			delta_direction;
	XrmQuark		type_quark;
	XtPointer		store;
} NumericFieldPart;

typedef struct _NumericFieldRec {
	CorePart        	core;
	PrimitivePart   	primitive;
	TextLinePart    	textLine;
	NumericFieldPart	numericField;
} NumericFieldRec;


#ifdef  __cplusplus
}
#endif

#endif	/* _XOL_NUMERICFIP_H */

