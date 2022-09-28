/*
 * Copyright (c) 1996, Sun Microsystems, Inc.
 * All rights reserved.
 */
 
#pragma ident	"@(#)lcl_create.c 1.1	97/01/28 SMI"

/*
 *  XXX For now, it should work at least with the followings.
 *
 *  LCTd lclt = 
 *     lct_create(lcld, 
 *		  LctNSourceType, LctNMsgText, buf_header, buf_body,
 *		  LctNSourceForm, LctCInComingStreamForm,
 *		  LctNKeepReference, True,
 *		  NULL);
 */				    
     
#include <string.h>
#include <stdlib.h>
#include <sys/varargs.h>
#include "lcl_types.h"
#include "lcl.h"
#include "lcl_internal.h"
#include <stdio.h>

Public LCTd
lct_create(LCLd lcld,...)
{
    va_list var;
    
    char *attr ;
    LctNEAttribute eattr;
    LctNEAttribute type = LctNUnused;
    LctNEAttribute keep_str = LctNUnused;
    LctNEAttribute form = LctNUnused;

    char *buf1, *buf2;
    size_t	length;

    va_start(var,lcld);

    for (eattr = va_arg(var, LctNEAttribute); eattr; eattr = va_arg(var, LctNEAttribute)) {
	switch(eattr) {
	case LctNSourceType:
	    if(eattr = va_arg(var, LctNEAttribute)){
		switch(type = eattr) {
		case LctNMsgText:
		    if(attr = va_arg(var, char*)){
			buf1 = attr;
			if(attr = va_arg(var, char*)){
			    buf2 = attr;
			} else return NULL;
		    } else return NULL;
		    break;
		case LctNPlainText:
		    if(attr = va_arg(var, char*)){
			buf1 = attr;
		    } else return NULL;
		    break;
		case LctNTaggedText:
		    if(attr = va_arg(var, char*)){
			buf1 = attr;
		    } else return NULL;
		    break;
		case LctNSeparatedTaggedText:
		    if(attr = va_arg(var, char*)){
			buf1 = attr;
			if(attr = va_arg(var, char*)){
			    buf2 = attr;
			} else return NULL;
		    } else return NULL;
		    break;
		case LctNSourceUnknown:
		    if(attr = va_arg(var, char*)){
			buf1 = attr;
			if(attr = va_arg(var, char*)){
			    length = (size_t)attr;
			} else return NULL;
		    } else return NULL;
		    break;
		default:
		    return NULL;
		}
	    } else return NULL ;
	    break;
	case LctNSourceForm:
	    if(eattr = va_arg(var, LctNEAttribute)){
		form = eattr;
	    } else return NULL;
	    break;
	case LctNKeepReference:
	    if(eattr = va_arg(var, LctNEAttribute)){
		keep_str = eattr;
	    } else return NULL; /* error if battr = B_FALSE */
	    break;
	default:
	    return NULL;
	} /* end switch */
    } /* end for */
    
    /* check */
    switch (type) {
    case LctNMsgText:
    case LctNSeparatedTaggedText:
	return _lct_create_msg(lcld, type, buf1, buf2, form, keep_str);
    case LctNPlainText:
	return _lct_create_plain(lcld, type, buf1, form, keep_str);
    case LctNTaggedText:
	return _lct_create_tagged(lcld, type, buf1, form, keep_str);
    case LctNSourceUnknown:
	return _lct_create_buf(lcld, type, buf1, length, form, keep_str);
    default:
	return (LCTd)NULL;
    }
}
    
