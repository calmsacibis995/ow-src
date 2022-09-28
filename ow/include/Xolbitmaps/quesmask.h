/*
**   ----------------------------------------------------------------- 
**        Copyright (C) 1990, 1991 Sun Microsystems, Inc
**                   All rights reserved.
**         Notice of copyright on this source code
**         product does not indicate publication.
**   
**   RESTRICTED RIGHTS LEGEND: Use, duplication, or disclosure by
**   the U.S. Government is subject to restrictions as set forth
**   in subparagraph (c)(1)(ii) of the Rights in Technical Data
**   and Computer Software Clause at DFARS 252.227-7013 (Oct. 1988)
**   and FAR 52.227-19 (c) (June 1987).
**   
**     Sun Microsystems, Inc., 2550 Garcia Avenue,
**     Mountain View, California 94043.
**   ----------------------------------------------------------------- 
*/
/*	Copyright (c) 1990 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#pragma ident	"@(#)olbitmaps:quesmask.h	1.1"
#endif

#define quesmask_width 9
#define quesmask_height 15
#define quesmask_x_hot 2
#define quesmask_y_hot 13
static char quesmask_bits[] = {
   0x7c, 0x00, 0xfe, 0x00, 0xff, 0x01, 0xe6, 0x01, 0xe0, 0x01, 0xf8, 0x00,
   0x7c, 0x00, 0x1e, 0x00, 0x1e, 0x00, 0x1e, 0x00, 0x0c, 0x00, 0x1e, 0x00,
   0x1e, 0x00, 0x1e, 0x00, 0x1e, 0x00};
