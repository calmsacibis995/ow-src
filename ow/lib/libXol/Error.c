#pragma ident	"@(#)Error.c	302.6	93/01/27 lib/libXol SMI"	/* olcommon:src/Error.c 1.15	*/

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

/*	Copyright (c) 1990 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*
 *************************************************************************
 *
 * Description:
 *		The error diagnostic functions are held in this file.
 *	There are two types of error procedures and two warning procedures.
 *
 *		OlError()		- is a simple fatal error handler
 *		OlWarning()		- is a simple non-fatal error handler
 *		OlVaDisplayErrorMsg()	- Variable argument list fatal
 *					  error handler
 *		OlVaDisplayWarningMsg() - Variable argument list non-fatal
 *					  error handler
 *
 *******************************file*header*******************************
 */


#include <Xol/Error.h>
#include <Xol/OpenLookP.h>

#include <X11/IntrinsicP.h>

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


/*
 *************************************************************************
 *
 * Forwarr Procedure definitions listed by category:
 *		1. Private Procedures
 *		2. Public  Procedures
 *
 **************************forward*declarations***************************
 */

					/* private procedures		*/

static void	EchoApplicationName (Display *, const char *);
static void	ErrorHandler (String);
static void	WarningHandler (String);
static void	VaDisplayErrorMsgHandler (Display *, String,
				String, String, String, va_list vargs);
static void	VaDisplayWarningMsgHandler (Display *, String,
				String, String, String, va_list vargs);
static void	GetMessage (Display *, char *, int, String,
				String, String, String);


/*
 *************************************************************************
 *
 * Define global/static variables and #defines, and
 * Declare externally referenced variables
 *
 *****************************file*variables******************************
 */

static OlErrorHandler		error_handler	= ErrorHandler;
static OlWarningHandler		warning_handler	= WarningHandler;
static OlVaDisplayErrorMsgHandler	va_display_error_msg_handler =
					VaDisplayErrorMsgHandler;
static OlVaDisplayErrorMsgHandler	va_display_warning_msg_handler =
					VaDisplayWarningMsgHandler;
static const char	error_prefix[] = "Error";
static const char	warning_prefix[] = "Warning";
static const char	unknown_application[] = "??Unknown??";
static const char	name_format[] =
				"OPEN LOOK Toolkit %s in application \"%s\": ";

#define VARARGS_START(a,m)	va_start(a, m)

			/* define a macro to swap the old handler with the
			 * new one.  Before swapping, set the new one to
			 * the default handler if the new one is NULL.	*/

#define CHK_AND_SWAP(type,new,old,def) \
	type	tmp;\
	GetToken();\
	if (new == NULL) new = def;\
	tmp	= new;\
	new	= old;\
	old	= tmp;\
	ReleaseToken();

/*
 *************************************************************************
 *
 * Private Procedures
 *
 ****************************public*procedures****************************
 */

/*
 *************************************************************************
 * ErrorHandler - default warning handler
 ****************************procedure*header*****************************
 */
static void
ErrorHandler (String msg)
{
	EchoApplicationName((Display *)NULL, error_prefix);
	(void) fprintf(stderr, "%s\n", (char *)msg);
	exit(EXIT_FAILURE);
} /* END OF ErrorHandler() */

/*
 *************************************************************************
 * WarningHandler - default warning handler
 ****************************procedure*header*****************************
 */
static void
WarningHandler (String msg)
{
	EchoApplicationName((Display *)NULL, warning_prefix);
	(void) fprintf(stderr, "%s\n", (char *)msg);
} /* END OF WarningHandler() */

/*
 *************************************************************************
 * VaDisplayErrorMsgHandler - default error handler
 ****************************procedure*header*****************************
 */
static void
VaDisplayErrorMsgHandler (Display *display, String name,String type,
			  String c_class, String message, va_list vargs)
{
	char buf[BUFSIZ];

	GetMessage(display, buf, BUFSIZ, name, type, c_class, message);
	EchoApplicationName(display, error_prefix);
	(void) vfprintf(stderr, buf, vargs);
	(void) fputc('\n', stderr);
	exit(EXIT_FAILURE);
} /* END OF VaDisplayErrorMsgHandler() */

/*
 *************************************************************************
 * VaDisplayWarningMsgHandler - default warning handler
 ****************************procedure*header*****************************
 */
static void
VaDisplayWarningMsgHandler (Display *display, String name, String type,
			    String c_class, String message, va_list vargs)
{
	char buf[BUFSIZ];

	GetMessage(display, buf, BUFSIZ, name, type, c_class, message);
	EchoApplicationName(display, warning_prefix);
	(void) vfprintf(stderr, buf, vargs);
	(void) fputc('\n', stderr);
} /* END OF VaDisplayWarningMsgHandler() */

/*
 *************************************************************************
 * GetMessage - looks in the error database for a error/warning message.
 ****************************procedure*header*****************************
 */
static void
GetMessage (Display *display, char *buf, int buf_size, String name, String type, String class, String default_msg)
{
	XtAppContext ac = (XtAppContext) (display != (Display *)NULL ||
			   (display = OlDefaultDisplay) != (Display *)NULL ?
				XtDisplayToApplicationContext(display) : NULL);

	if (ac != (XtAppContext)NULL)
	{
		XtAppGetErrorDatabaseText(ac, name, type, class, default_msg,
					buf, buf_size, (XrmDatabase)NULL);
	}
	else
	{
		(void) strncpy(buf, (const char *)default_msg, buf_size);
	}
			/* make sure the string in NULL terminated	*/

	buf[buf_size-1] = '\0';
} /* END OF GetMessage() */

/*
 *************************************************************************
 * EchoApplicationName - echos the application name to stderr along
 * with a warning or error prefix string.
 ****************************procedure*header*****************************
 */
static void
EchoApplicationName (Display *display, const char *prefix)
{
	String	name;

	if (display != (Display *)NULL ||
	    (display = OlDefaultDisplay) != (Display *)NULL)
	{
		String	class;		/* ignored	*/

		XtGetApplicationNameAndClass(display, &name, &class);
	}
	else
	{
		name = (String)unknown_application;
	}

	(void) fprintf(stderr, name_format, prefix, (char *)name);
} /* END OF EchoApplicationName() */

/*
 *************************************************************************
 *
 * Public Procedures
 *
 ****************************public*procedures****************************
 */

/*
 *************************************************************************
 * OlVaDisplayErrorMsg - this procedure handles errors of a fatal type.
 ****************************procedure*header*****************************
 */
void
OlVaDisplayErrorMsg (Display *display,String name,String type,
		     String class,String message,...)
{
	va_list	args;			/* the variable arguments	*/

	GetToken();
	VARARGS_START(args, message);
	(*va_display_error_msg_handler)(display, name, type, class,
						message, args);
	va_end(args);
	ReleaseToken();
} /* END OF OlVaDisplayErrorMsg() */

/*
 *************************************************************************
 * OlVaDisplayWarningMsg - this procedure handles errors of a non-fatal type.
 ****************************procedure*header*****************************
 */
void
OlVaDisplayWarningMsg (Display *display,String name,String type,
		       String class,String message,...)
{
	va_list		args;		/* the variable arguments	*/

	GetToken();
	VARARGS_START(args, message);
	(*va_display_warning_msg_handler)(display, name, type,
						class, message, args);
	va_end(args);
	ReleaseToken();
} /* END OF OlVaDisplayWarningMsg() */
/*
 *************************************************************************
 * OlError - this procedure writes the application supplied message to
 * stderr and then exits with a status of 1 if the application did not
 * specify an overriding warning handler.
 ****************************procedure*header*****************************
 */
void
OlError (String s)
{
	GetToken();
	(*error_handler)(s);
	ReleaseToken();
} /* END OF OlError() */

/*
 *************************************************************************
 * OlSetVaDisplayErrorMsgHandler - sets the procedure to be called when a
 * fatal error occurs.
 ****************************procedure*header*****************************
 */
OlVaDisplayErrorMsgHandler
OlSetVaDisplayErrorMsgHandler (OlVaDisplayErrorMsgHandler handler)
{
	CHK_AND_SWAP(OlVaDisplayErrorMsgHandler, handler,
		va_display_error_msg_handler, VaDisplayErrorMsgHandler);
	return(handler);
} /* END OF OlSetVaDisplayErrorMsgHandler() */

/*
 *************************************************************************
 * OlSetVaDisplayWarningMsgHandler - sets the procedure to be called when
 * a non-fatal error occurs.
 ****************************procedure*header*****************************
 */
OlVaDisplayWarningMsgHandler
OlSetVaDisplayWarningMsgHandler (OlVaDisplayWarningMsgHandler handler)
{
	CHK_AND_SWAP(OlVaDisplayWarningMsgHandler, handler,
		va_display_warning_msg_handler, VaDisplayWarningMsgHandler);
	return(handler);
} /* END OF OlSetVaDisplayWarningMsgHandler() */

/*
 *************************************************************************
 * OlSetErrorHandler - registers/unregisters the fatal error handler
 ****************************procedure*header*****************************
 */
OlErrorHandler
OlSetErrorHandler (OlErrorHandler handler)
{
	CHK_AND_SWAP(OlErrorHandler, handler, error_handler, ErrorHandler);
	return(handler);
} /* END OF OlSetErrorHandler() */

/*
 *************************************************************************
 * OlSetWarningHandler - registers/unregisters the non-fatal error handler
 ****************************procedure*header*****************************
 */
OlWarningHandler
OlSetWarningHandler (OlWarningHandler handler)
{
	CHK_AND_SWAP(OlWarningHandler, handler, warning_handler,
				WarningHandler);
	return(handler);
} /* END OF OlSetWarningHandler() */

/*
 *************************************************************************
 * OlWarning - this procedure writes the application supplied message to
 * stderr and then returns if the application did not specify an
 * overriding warning handler.
 ****************************procedure*header*****************************
 */
void
OlWarning (String s)
{
	GetToken();
	(*warning_handler)(s);
	ReleaseToken();
} /* END OF OlWarning() */

