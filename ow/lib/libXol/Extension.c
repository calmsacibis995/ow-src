#pragma ident	"@(#)Extension.c	302.3	97/03/26 lib/libXol SMI"	/* mouseless:Extension.c 1.1	**/

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
 * 		This file contains routines to manipulate class extensions
 * 
 ****************************file*header**********************************
 */


#include <Xol/OpenLookP.h>

#include <X11/IntrinsicP.h>


/*
 *************************************************************************
 *
 * Forward Procedure definitions listed by category:
 *		1. Public  Procedures
 *
 **************************forward*declarations***************************
 */



/*
 *************************************************************************
 *
 * Define global/static variables and #defines, and
 * Declare externally referenced variables
 *
 *****************************file*variables******************************
 */

#define NULL_EXTENSION	((OlClassExtension)NULL)

/*
 *************************************************************************
 *
 * Public Procedures
 *
 ****************************public*procedures****************************
 */

/*
 *************************************************************************
 * _OlGetClassExtension - this routine gets a class extension from a
 * class's extension list.
 ****************************procedure*header*****************************
 */
OlClassExtension
_OlGetClassExtension(OlClassExtension extension, XrmQuark record_type, long int version)
	                           	/* first extension		*/
	        	            	/* type to look for		*/
	    		        	/* if non-zero, look for it	*/
{
	while (extension != NULL_EXTENSION &&
		!(extension->record_type == record_type &&
		  (!version || version == extension->version)))
	{
		extension = (OlClassExtension) extension->next_extension;
	}
	return (extension);
} /* END OF _OlGetClassExtension() */

