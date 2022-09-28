/*
 *	@(#)helplib.h 1.6 96/11/26 Copyright 1990-91 Sun Microsystems
 *
 * This file is a product of Sun Microsystems, Inc. and is provided for
 * unrestricted use provided that this legend is included on all tape
 * media and as a part of the software program in whole or part.  Users
 * may copy or modify this file without charge, but are not authorized to
 * license or distribute it to anyone else except as part of a product
 * or program developed by the user.
 * 
 * THIS FILE IS PROVIDED AS IS WITH NO WARRANTIES OF ANY KIND INCLUDING THE
 * WARRANTIES OF DESIGN, MERCHANTIBILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE, OR ARISING FROM A COURSE OF DEALING, USAGE OR TRADE PRACTICE.
 * 
 * This file is provided with no support and without any obligation on the
 * part of Sun Microsystems, Inc. to assist in its use, correction,
 * modification or enhancement.
 * 
 * SUN MICROSYSTEMS, INC. SHALL HAVE NO LIABILITY WITH RESPECT TO THE
 * INFRINGEMENT OF COPYRIGHTS, TRADE SECRETS OR ANY PATENTS BY THIS FILE
 * OR ANY PART THEREOF.
 * 
 * In no event will Sun Microsystems, Inc. be liable for any lost revenue
 * or profits or other special, indirect and consequential damages, even
 * if Sun has been advised of the possibility of such damages.
 * 
 * Sun Microsystems, Inc.
 * 2550 Garcia Avenue
 * Mountain View, California  94043
 */

#ifndef _HELPLIBINCLUDES_
#define _HELPLIBINCLUDES_

#include <portable/c_varieties.h>
/*
**	Removed by DPT 26-Nov-96, bug ID 4016493
**	#include <wire/wire.h>
*/

/*  Main callback function for the help library. Takes care of message
    lookup, opening the help window, displaying help text
*/
EXTERN_FUNCTION( void Help_RequestHelpHandler, (int tag, caddr_t data));

/*  Open online help handler */
EXTERN_FUNCTION( void Help_MoreHelpHandler,    (int tag, caddr_t data));

/*  Initialize the help library, set up tags and tokens */
EXTERN_FUNCTION( void Help_Initialize,         (wire_Wire w));

/*  Update the display in the Help window. Should be called in application
    notification loop
*/
EXTERN_FUNCTION( void Help_UpdateView,         (_VOID_));

/*  Set the Help window label. Hook for i18n. Default is "Help"    
*/
EXTERN_FUNCTION( void Help_SetHelpWindowLabel, (char * newlabel));

#endif
