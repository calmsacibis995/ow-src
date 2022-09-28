#ifndef	_XOL_OPENLOOKI_H
#define	_XOL_OPENLOOKI_H

#pragma	ident	"@(#)OpenLookI.h	302.4	92/12/04 include/Xol SMI"	/* olmisc:OpenLookI.h 1.4 	*/

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


#include <Xol/OpenLookP.h>

#include <X11/Intrinsic.h>


#ifdef	__cplusplus
extern "C" {
#endif


/*
 *	This file contains all private tables for Applic.c.
 */

				/* 
				 * Define a structure that holds the
				 * internal attributes of this OLIT
				 * application.  New internal attributes
				 * should be added to this structure.
				 */

/*************************************************************************
 * WARNING!WARNING!WARNING!WARNING!WARNING!WARNING!WARNING!WARNING!WARNING
 *
 * From OWV3 onwards the application resources for the toolkit have
 * been 'widgetised' in a DisplayShellWidget for each Display connection.
 *
 * This structure is maintained for backwards compatibility reasons ONLY
 * all new app resources should be added to the DisplayShellWidget
 * instance Record in RootShellP.h .....
 *
 *************************************************************************/

typedef struct
__OlAppAttributes {
	Cardinal	mouse_damping_factor;	/* in points */
	Cardinal	multi_click_timeout;	/* in milliseconds */
	int		beep_volume;		/* Beep volumn percentage */
	int		beep_duration;		/* in milliseconds */	
	OlDefine	beep;			/* Beep for which levels */
	Boolean		select_does_preview;	/* Does select preview?	*/
	Boolean		grab_pointer;		/* can we grab the pointer */
	Boolean		grab_server;		/* can we grab the server */
	Boolean         menu_accelerators;      /* accelerators operation on? */
	Boolean         mouseless;              /* mouseless operation on? */
	Cardinal	multi_object_count;
	OlBitMask	dont_care;		/* should use Modifiers,
						   but can't find a
						   repesentation type */
	Boolean		three_d;		/* use 3-D visuals? */
	char*		scale_map_file;		/* name of scale to screen
						   resolution map file */

			/*
			 * Resources that control accelerators
			 * and mnemonics:
			 */
	Modifiers	mnemonic_modifiers;
	OlDefine	show_mnemonics;
	OlDefine	show_accelerators;
	String		shift_name;
	String		lock_name;
	String		control_name;
	String		mod1_name;
	String		mod2_name;
	String		mod3_name;
	String		mod4_name;
	String		mod5_name;

	OlDefine	help_model;
	Boolean		mouse_status;

	Dimension	drag_right_distance;
	Dimension	menu_mark_region;
	Cardinal	key_remap_timeout;	/* in seconds	*/
	Boolean		use_short_OlWinAttr;
} _OlAppAttributes;


/*
 * function prototype section
 */

/*
 * RootShell module
 */

extern _OlAppAttributes*	_OlGetAppAttributesRef(Widget w);


#ifdef	__cplusplus
}
#endif


#endif	/* _XOL_OPENLOOKI_H */
