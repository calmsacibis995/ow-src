#ifndef _XOL_CONVERTERS_H
#define	_XOL_CONVERTERS_H

#pragma	ident	"@(#)Converters.h	302.2	92/04/22 include/Xol SMI"	/* olmisc:Converters.h 1.5	*/

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


#include <Xol/OpenLook.h>

#include <X11/Intrinsic.h>


#ifdef	__cplusplus
extern "C" {
#endif


#define DeclareConversionClass(display,type,c_class) \
	XtAppContext	__app = XtDisplayToApplicationContext(display); \
	String		__type = type; \
	String		__c_class = (c_class ? c_class : "OlToolkitError")

#define DeclareDestructionClass(app,type,c_class) \
	XtAppContext	__app = app; \
	String		__type = type; \
	String		__c_class = (c_class ? c_class : "OlToolkitError")

#define ConversionError(name,dflt,params,num_params) \
	XtAppErrorMsg(__app, name, __type, __c_class, dflt, params, num_params)

#define ConversionWarning(name,dflt,params,num_params) \
	XtAppWarningMsg(__app, name, __type, __c_class, dflt, params, num_params)

#define DestructionWarning	ConversionWarning
#define DestructionError	ConversionError

#define ConversionDone(type,value) \
	{ \
		if (to->addr) { \
			if (to->size < sizeof(type)) { \
				to->size = sizeof(type); \
				return (False); \
			} \
			*(type *)(to->addr) = (value); \
		} else { \
			static type	static_value; \
			\
			static_value = (value); \
			to->addr = (XtPointer)&static_value; \
		} \
		to->size = sizeof(type); \
	} \
	return (True)


extern void	OlRegisterConverters(void);
extern void OlVendorSetConverters(Boolean flag);
extern void _OlGetVendorTextFormat(Widget w, Cardinal *size, XrmValue *ret_val);


#ifdef	__cplusplus
}
#endif


#endif	/* _XOL_CONVERTERS_H */
