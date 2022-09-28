#ifndef	_XOL_OLREASONS_H
#define	_XOL_OLREASONS_H

#pragma	ident	"@(#)OlReasons.h	302.9	92/12/10 include/Xol SMI"	/* OLIT	*/

/*
 *	Copyright (C) 1992  Sun Microsystems, Inc
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


/************************************************************************
 *
 *      OLIT Callback reasons 
 *
 ************************************************************************/

#ifdef	__cplusplus
extern "C" {
#endif

#define	OL_REASON_NONE				0
#define	OL_REASON_EXPOSE			1
#define	OL_REASON_GRAPHICS_EXPOSE		2
#define	OL_REASON_RESIZE			3
#define	OL_REASON_DND_PREVIEW			4
#define	OL_REASON_DND_TRIGGER			5
#define	OL_REASON_DND_OWNSELECTION		6
#define	OL_REASON_DND_ANIMATE			7
#define	OL_REASON_NO_EXPOSE			8
#define	OL_REASON_PRE_MODIFICATION		9
#define	OL_REASON_PROG_PRE_MODIFICATION		10
#define	OL_REASON_POST_MODIFICATION		11
#define	OL_REASON_PROG_POST_MODIFICATION	12
#define	OL_REASON_MOTION			13
#define	OL_REASON_PROG_MOTION			14
#define	OL_REASON_COMMIT			15
#define	OL_REASON_INCREMENT			16
#define	OL_REASON_DECREMENT			17
#define	OL_REASON_VALIDATE			18 
#define	OL_REASON_ITEM_CURRENT 			19
#define	OL_REASON_ITEM_NOT_CURRENT		20
#define	OL_REASON_DOUBLE_CLICK			21
#define	OL_REASON_USER_DELETE_ITEMS		22
#define	OL_REASON_ERROR 			23
#define	OL_REASON_CHANGED_FONT			24
#define	OL_REASON_APPLY_FONT			25
#define	OL_REASON_REVERT_FONT			26
#define	OL_REASON_CANCEL			27
#define	OL_REASON_OPEN_FOLDER			28
#define	OL_REASON_INPUT_DOCUMENT		29
#define	OL_REASON_OUTPUT_DOCUMENT		30
#define	OL_REASON_FILTER			31
#define	OL_REASON_LIST_CHOICE			32
#define	OL_REASON_FOLDER_OPENED			33
#define	OL_REASON_VERIFY			34

#ifdef	__cplusplus
}
#endif

/* end of OlReasons.h */
#endif	/* _XOL_OLREASONS_H */
