#ifndef	_XOL_FONT_H
#define	_XOL_FONT_H

#pragma	ident	"@(#)Font.h	302.1	92/03/26 include/Xol SMI"	/* button:include/openlook/Font.h 1.5	*/

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


#ifdef	__cplusplus
extern "C" {
#endif


#define	NORMAL_LEFT	1
#define	NORMAL_LINES	2
#define	NORMAL_RIGHT	3

#define	HIGHLIGHT_LEFT	4
#define	HIGHLIGHT_LINES	5
#define	HIGHLIGHT_RIGHT	6

#define	DEFAULT_LEFT	7
#define	DEFAULT_LINES	8
#define	DEFAULT_RIGHT	9

#define	STACK_LEFT	10
#define	STACK_LINES	11
#define	STACK_RIGHT	12

#define	STACK_HI_LEFT	13
#define	STACK_HI_LINES	14
#define	STACK_HI_RIGHT	15

#define	STACK_DE_LEFT	16
#define	STACK_DE_LINES	17
#define	STACK_DE_RIGHT	18

#define	HALF_LINES	19
#define	HALF_HI_LINES	20
#define	HALF_DE_LINES	21

#define HALF_HALF	22
#define HALF_HALF_HI	23
#define HALF_HALF_DEF	24

#define	FILL_LEFT	25
#define	FILL_LINES	26
#define	FILL_RIGHT	27

#define	STACK_FILL_LEFT		28
#define	STACK_FILL_LINES	29
#define	STACK_FILL_RIGHT	30

#define POPUP		31
#define ARROW		32
#define BLANK		33

#define NORMAL_LINES1	34
#define HIGHLIGHT_LINES1	35
#define DEFAULT_LINES1	36
#define STACK_LINES1	37
#define STACK_HI_LINES1	38
#define STACK_DE_LINES1	39
#define HALF_LINES1	40
#define HALF_HI_LINES1	41
#define HALF_DE_LINES1	42
#define FILL_LINES1	43
#define STACK_FILL_LINES1	44
#define BLANK1			45

#define MENU_PULLRIGHT		46
#define MENU_PULLDOWN		47

#define	MENU_DEFAULT_LEFT	48
#define MENU_DEFAULT_LINES	49
#define	MENU_DEFAULT_RIGHT	50
#define	MENU_DEFAULT_LINES1	51

#define	MENU_SET_LEFT		52
#define MENU_SET_LINES		53
#define	MENU_SET_RIGHT		54
#define	MENU_SET_LINES1		55

#define	ABB_MENU_LEFT		56
#define	ABB_MENU_RIGHT		57
#define	ABB_MENU_LEFT_SET	58
#define	ABB_MENU_RIGHT_SET	59

#define	NONMENU_PULLRIGHT	60
#define	NONMENU_PULLDOWN	61
#define CHECKBOX_LEFT		62
#define CHECKBOX_RIGHT		63
#define CHECKBOX_LEFT_SET	64
#define CHECKBOX_RIGHT_SET	65


#ifdef	__cplusplus
}
#endif


#endif	/* _XOL_FONT_H */
