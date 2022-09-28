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
#pragma ident	"@(#)olbitmaps:pancurs.h	1.1"
#endif

#define pancurs_width 18
#define pancurs_height 14
#define pancurs_x_hot 1
#define pancurs_y_hot 12
static char pancurs_bits[] = {
   0x00, 0x00, 0x00, 0xc0, 0xf8, 0x01, 0xc0, 0xfc, 0x01, 0xe0, 0x0e, 0x00,
   0xe0, 0x07, 0x00, 0xf0, 0x03, 0x00, 0xf0, 0x1f, 0x00, 0xf8, 0x1f, 0x00,
   0xf8, 0x07, 0x00, 0xfc, 0x01, 0x00, 0x7c, 0x00, 0x00, 0x1e, 0x00, 0x00,
   0x06, 0x00, 0x00, 0x00, 0x00, 0x00};
