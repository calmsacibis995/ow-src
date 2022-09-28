#ifndef	_ISAMREC_H
#define	_ISAMREC_H

#ident "@(#)isamrec.h	1.4 06/11/93 Copyright 1989 Sun Microsystems, Inc."



#include "isamschema.h"


// ISAM record flag values.
//
#define	IF_ISSYMLINK	0x0001
#define	IF_DOCFILE	0x0002
#define	IF_NOSHOW	0x0004
#define	IF_NOSHOWKIDS	0x0008



// Maximum lengths for various ISAM record fields.
//
#define	FLDLEN(x)		(sizeof(((ISAMREC *)0)->x) - 1)

#define	ISAMREC_ID_LEN			FLDLEN(id)
#define	ISAMREC_TITLE_LEN		FLDLEN(title)
#define	ISAMREC_LABEL_LEN		FLDLEN(label)
#define	ISAMREC_VIEW_METHOD_LEN		FLDLEN(view_method)
#define	ISAMREC_PRINT_METHOD_LEN	FLDLEN(print_method)
#define	ISAMREC_SYM_LINK_LEN	(ISAMREC_VIEW_METHOD_LEN + ISAMREC_PRINT_METHOD_LEN)

#endif	_ISAMREC_H
