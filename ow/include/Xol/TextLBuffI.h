#ifndef _XOL_TEXTLBUFFI_H
#define _XOL_TEXTLBUFFI_H

#pragma ident	"@(#)TextLBuffI.h	1.2	92/10/05 lib/libXol SMI"	/* OLIT	*/

/*
 *        Copyright (C) 1986,1991  Sun Microsystems, Inc
 *                    All rights reserved.
 *          Notice of copyright on this source code 
 *          product does not indicate publication. 
 * 
 * RESTRICTED RIGHTS LEGEND: Use, duplication, or disclosure by 
 * the U.S. Government is subject to restrictions as set forth 
 * in subparagraph (c)(1)(ii) of the Rights in Technical Data
 * and Computer Software Clause at DFARS 252.227-7013 (Oct. 1988) 
 * and FAR 52.227-19 (c) (June 1987).
 *
 *    Sun Microsystems, Inc., 2550 Garcia Avenue,
 *    Mountain View, California 94043.
 *
 */

#include 	<Xol/OpenLook.h>
#include 	<Xol/TextLineP.h>

#ifdef  __cplusplus
extern "C" {
#endif


extern 	TLBuffer *_OlTLAllocateBuffer(int initial_size);
extern 	void	_OlTLFreeBuffer(TLBuffer *);

extern 	void   	_OlTLInsertBytes(TLBuffer *b, 
				 char 	  *buff, 
				 int 	  pos, 
				 int 	  num_bytes
				);

extern 	void   	_OlTLDeleteBytes(TLBuffer *b, 
				 int      start, 
				 int      end
				);

extern 	void   	_OlTLInsertString(TLBuffer 	*b, 
				  OlStr   	string, 
				  int      	pos,
				  OlStrRep 	format, 
				  PositionTable *posTable
				 );

extern 	void   	_OlTLDeleteString(TLBuffer 	*b, 
				  int      	start, 
				  int      	end, 
				  OlStrRep 	format, 
				  PositionTable *posTable
				 );

extern 	Boolean	_OlTLReplaceString(TextLineWidget w, 
				   int 		  start, 
				   int  	  end, 
				   OlStr 	  string
				  );

#ifdef  __cplusplus
}
#endif

#endif	/* _XOL_TEXTLBUFFI_H */
