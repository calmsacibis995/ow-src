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
#pragma ident	"@(#)olbitmaps:targcurs.h	1.1"
#endif

#define targcurs_width 24
#define targcurs_height 24
#define targcurs_x_hot 7
#define targcurs_y_hot 7
static char targcurs_bits[] = {
   0x00, 0x00, 0x00, 0xe0, 0x03, 0x00, 0xf0, 0x07, 0x00, 0x18, 0x0c, 0x00,
   0x0c, 0x18, 0x00, 0x06, 0x30, 0x00, 0x06, 0x30, 0x00, 0x86, 0x31, 0x00,
   0x86, 0x37, 0x00, 0x06, 0x3f, 0x00, 0x0c, 0x7f, 0x00, 0x18, 0xfe, 0x01,
   0xf0, 0xff, 0x07, 0xe0, 0xff, 0x07, 0x00, 0xfc, 0x00, 0x00, 0xf8, 0x01,
   0x00, 0xb8, 0x03, 0x00, 0x30, 0x07, 0x00, 0x30, 0x0e, 0x00, 0x00, 0x1c,
   0x00, 0x00, 0x38, 0x00, 0x00, 0x70, 0x00, 0x00, 0x60, 0x00, 0x00, 0x00};
