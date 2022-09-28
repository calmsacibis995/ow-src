#ifndef	_XOL_TEXTFIELD_H
#define	_XOL_TEXTFIELD_H

#pragma	ident	"@(#)TextField.h	302.5	93/05/11 include/Xol SMI"	/* textfield:include/Xol/TextField.h 1.9 	*/

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


#include <Xol/Manager.h>   /* include superclasses' header */
#include <Xol/TextEdit.h>
#include <Xol/OpenLook.h>

#include <X11/Intrinsic.h>


#ifdef	__cplusplus
extern "C" {
#endif


typedef struct _TextFieldClassRec*	TextFieldWidgetClass;
typedef struct _TextFieldRec*		TextFieldWidget;

typedef enum {
	OlTextFieldReturn, OlTextFieldPrevious, OlTextFieldNext
}			OlTextVerifyReason;

typedef struct {
	String			string;
	Boolean			ok;
	OlTextVerifyReason	reason;
}			OlTextFieldVerify, *OlTextFieldVerifyPointer;


extern WidgetClass		textFieldWidgetClass;


#if	defined(__STDC__) || defined(__cplusplus)

extern int	OlTextFieldCopyString(TextFieldWidget tfw, char* string);
extern char*	OlTextFieldGetString(TextFieldWidget tfw, int* size);

extern int	OlTextFieldCopyOlString(TextFieldWidget tfw, OlStr string);
extern OlStr 	OlTextFieldGetOlString(TextFieldWidget tfw, int* size);

#else	/* __STDC__ || __cplusplus */

extern int	OlTextFieldCopyString();
extern char*	OlTextFieldGetString();

extern int	OlTextFieldCopyOlString();
extern OlStr 	OlTextFieldGetOlString();

#endif	/* __STDC__ || __cplusplus */


#ifdef	__cplusplus
}
#endif


#endif	/* _XOL_TEXTFIELD_H */
