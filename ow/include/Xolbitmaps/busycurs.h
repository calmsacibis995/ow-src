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
#pragma ident	"@(#)olbitmaps:busycurs.h	1.1"
#endif

#define busycurs_width 18
#define busycurs_height 17
#define busycurs_x_hot 8
#define busycurs_y_hot 8
static char busycurs_bits[] = {
   0x00, 0x00, 0x00, 0xc0, 0xc7, 0x00, 0xf0, 0xdf, 0x01, 0x38, 0xb8, 0x01,
   0x0c, 0x60, 0x00, 0x0c, 0x60, 0x00, 0x06, 0xc0, 0x00, 0x06, 0xc0, 0x00,
   0xf6, 0xc1, 0x00, 0x06, 0xc1, 0x00, 0x06, 0xc1, 0x00, 0x0c, 0x61, 0x00,
   0x0c, 0x61, 0x00, 0x38, 0x38, 0x00, 0xf0, 0x1f, 0x00, 0xc0, 0x07, 0x00,
   0x00, 0x00, 0x00};
