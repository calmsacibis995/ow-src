/*
 * Copyright (c) 1996, Sun Microsystems, Inc.
 * All rights reserved.
 */
 
#pragma ident	"@(#)lcl_parse_header.c 1.9	96/08/09 SMI"

/*
 * Determine velues by parsing mime header, then update the
 * attribute values. It override the default values in DB.
 */
#include <string.h>
#include <stdlib.h>
#include "lcl_types.h"
#include "lcl.h"
#include "lcl_internal.h"

#define ENCODE_BEGIN		"=?"
#define CHARSET_DONE		'?'
#define ENCODING_DONE		'?'
#define ENCODE_DONE		"?="

#define ENCODE_BASE64		"?B?"
#define	ENCODE_QP		"?Q?"
#define ENCODE_NONE		 0

#define MIME_VERSION		"Mime-Version"
#define CONTENT_TYPE		"Content-Type"
#define TYPE_TEXTPLAIN		"Text/Plain"
#define CHARSET			"charset"

#define X_SUN_ATTACHEMENT	"X-sun-attachment"
#define X_SUN			"X-Sun"
#define X_SUN_CHARSET		"X-Sun-Charset"


/*
 * Public functions.
 * ----------------
 * 1.
 * char* _lcl_parse_body(char* h, // unparsed header:in
 *		         char* b  // unparsed body:in 
 *		         LclMailType type // Mail type:in 
 *        )
 * returns the parsed(converted) Body
 *
 * 2.
 * char* _lcl_parse_header(char* h, // unparsed header:in
 *			   char* b, // unparsed body:in
 *                         LclMailType *type // Mail type:inout
 *                         char** charset // charset of parsed header:out
 *        )
 * returns the parsed(converted) header
 *
 * 3
 * returns Mail type.
 * LclMailType _lcl_get_body_charset(char* h, // unparsed header:in
 *			   char* b, // unparsed body:in
 *			   LclMailType type // Mail type:in 
 *                         char** charset // charset of parsed header:out
 * charset CAN BE NULL if it is UNKNOWN.(or NOT IMPLEMENTED YET ;-)
 *
 */

/*/// Need lcld for all functions for MT */

Private Bool
_is_mime_header(char* h){
    Bool is_mime = False ;
    if(!h) return False ;
    if(strcasestr(h, MIME_VERSION) ||
       strcasestr(h, CONTENT_TYPE) ||
       (strstr(h, ENCODE_BEGIN) && strstr(h, ENCODE_DONE) &&
	(strcasestr(h, ENCODE_BASE64) || strcasestr(h, ENCODE_QP)))
      ) {
      is_mime = True ;
    }
    return is_mime ;
}

Private Bool
_is_V3_header(char* h){
    /*
     * case 1: X-Sun-Attachment is found.
     * case 2: Header looks like 822, but other X-Sun- header is found.
     * case 3: Header looks like 822, but X-Sun-Charset is found in Body.
     * otherwise it should be 822.
     */
    if(h && strcasestr(h, X_SUN)){
	return True ;
    }
    /* case 3 is missing */
    return False ; /* should be 822 */
}

Private char*
_lcl_V3_header(char* h){
    int len = strlen(h);
    char* orig = malloc(len +1);
    memcpy(orig, h, len+1);
    return orig ; /* Waiting for full implementation */
}

Private char*
_lcl_822_header(char* h){
    int len = strlen(h);
    char* orig = malloc(len +1);
    memcpy(orig, h, len+1);
    return orig ; /* Waiting for full implementation */
}

Private char *
copy_next_line(char** cur_p, char* copy_p, int *len_p){
    char *p = *cur_p ;
    char* next_line = strchr(p, '\n') ;
    if(next_line){
	int len = next_line - p ;
	strncpy(copy_p, p, len);
	*cur_p = next_line ;
	*len_p = len ;
	return copy_p;
    }
    return 0 ;
}

Private char *
dup_next_line(char** cur_p, int *len_p){
    char *p = *cur_p ;
    char* next_line = strchr(p, '\n') ;
    if(next_line){
	int len = next_line - p ;
	char *buf = (char*)malloc(len + 1);
	strncpy(buf, p, len);
	buf[len] = (char)0;
	*cur_p = next_line ;
	*len_p = len ;
	return buf;
    }
    return 0 ;
}


Private char*
_lcl_mime_header(char* h, char **charset){
    int len = strlen(h) ; 
    char* orig = malloc(len +1);
    char* buf = malloc(len*10/8); /* XXX guestimation to be safer side. */
    memcpy(orig, h, len+1);
    MIME_strHeaderDecode(orig, buf, len, charset);
    free(orig);
    return buf ;
}
/*
 * returns Mail type.
 * LclMailType _lcl_get_body_charset(char* t, // unparsed taggedtext:in
 * 		                     LclMailType *type // Mail type:out
 *                                   char** charset // charset of parsed header:out
 *        )
 */
Public LclMailType
_lcl_get_taggedtext_charset(char* t, LclMailType type, char** charset){
    if( type == LclV3Type ||
       (type == LclUnKnownType && _is_V3_header(t) == True)){

    } else { /* Must be MIME - No 822type for tagged text*/
	/*
	 * Body charset information is in header.
	 */
	char *p = strcasestr(t, "\nContent-Type:");
	if(!p){
	    char *x = strcasestr(t, "Content-Type:");
	    if(x == t)
		p = t ;
	}
	if(p){
	    char* eoline = strchr(p+1, '\n');
	    if(eoline){
		 p = strcasestr(p+14, "charset=") ;
		 if(p && (p < eoline)){
		     int len = eoline - (p+=8) ; /* skip "charset=" */
		     *charset = malloc(len +1);
		     strncpy(*charset, p, len);
		     (*charset)[len] = NULL ;
		 } else *charset = NULL ;
	     } else *charset = NULL ;
	} else *charset = NULL ;
	return LclMIMEType ;
    }
}
/*
 * returns Mail type.
 * LclMailType _lcl_get_body_charset(char* h, // unparsed header:in
 *			   char* b, // unparsed body:in
 *                         LclMailType *type // Mail type:out
 *                         char** charset // charset of parsed header:out
 *        )
 */
Public LclMailType
_lcl_get_body_charset(char* h, char* b, LclMailType type, char** charset){
    if( type == LclMIMEType ||
       (type == LclUnKnownType && _is_mime_header(h) == True)){
	/*
	 * Body charset information is in header.
	 */
	char *p = strcasestr(h, "\nContent-Type:");
	if(!p){
	    char *x = strcasestr(h, "Content-Type:");
	    if(x == h)
		p = h ;
	}
	if(p){
	    char* eoline = strchr(p+1, '\n');
	    if(eoline){
		 p = strcasestr(p+14, "charset=") ;
		 if(p && (p < eoline)){
		     int len = eoline - (p+=8) ; /* skip "charset=" */
		     *charset = malloc(len +1);
		     strncpy(*charset, p, len);
		     (*charset)[len] = NULL ;
		 } else *charset = NULL ;
	     } else *charset = NULL ;
	} else *charset = NULL ;
	return LclMIMEType ;
    } else if( type == LclV3Type ||
	      (type == LclUnKnownType && _is_V3_header(h) == True)){
	/*
	 * Body charset information is in header when there is no
	 * attachment.
	 */
	char *p = strcasestr(h, "\nX-Sun-Charset:");
	if(!p) p = strcasestr(b, "\nX-Sun-Charset:"); 
	if(p){
	    char* eoline = strchr(p+1, '\n');
	    if(eoline){
		p+=15 ; /* skipping "\nX-Sun-Charset:" */
		while(isspace(*++p)); /* skipping space */
		if(p < eoline){
		    int len = eoline - p ; /* skip "charset=" */
		    *charset = malloc(len +1);
		    strncpy(*charset, p, len);
		    (*charset)[len] = NULL ;
		} else *charset = NULL ;
	    } else *charset = NULL ;
	} else *charset = NULL ;
	/*
	 * Body charset information is in body
	 */
	return LclV3Type ;
    } else {
	*charset = NULL ; /* Unknown */
	return Lcl822Type ;
    }
}
/*
 * returns the parsed(converted) header
 * char* _lcl_parse_header(char* h, // unparsed header:in
 *			   char* b, // unparsed body:in
 *                         LclMailType *type // Mail type:out
 *                         char** charset // charset of parsed header:out
 *        )
 */
Public char*
_lcl_parse_header(char* h, char* b, LclMailType *type ,char** charset){
    LclMailType	mail_type = _lcl_get_body_charset(h, b, *type, charset);
    if(*charset)
	free(*charset);
    *charset = (char *)NULL;

    switch(mail_type){
      case LclMIMEType:
	return _lcl_mime_decode_header(h, strlen(h), charset);
      case LclV3Type:
	return _lcl_V3_header(h);
      case Lcl822Type:
	return _lcl_mime_decode_header(h, strlen(h), charset);
      default:
	break ;
    }
}

Private Bool
_is_mime_body(char* h){
    return strcasestr(h, MIME_VERSION)?True:False ; /* Too easy? */
}

Private Bool
_is_V3_body(char* h){
    /*
     * case 1: X-Sun-Attachment is found.
     * case 2: Header looks like 822, but other X-Sun- header is found.
     * case 3: Header looks like 822, but X-Sun-Charset is found in Body.
     * otherwise it should be 822.
     */
    if(strcasestr(h, X_SUN)){
	return True ;
    }
    /* case 3 is missing */
    return False ; /* should be 822 */
}

Private char*
_lcl_mime_body(char* b){
    return b ; /* Waiting for full implementation */
}


Private char*
_lcl_V3_body(char* b){
    return b ; /* Waiting for full implementation */
}

Private char*
_lcl_822_body(char* b){
    return b ; /* Waiting for full implementation */
}

/*
 * returns the parsed(converted) Body
 * char* _lcl_parse_body(char* h, // unparsed header:in
 *		         char* b  // unparsed body:in 
 *        )
 */
Public char*
_lcl_parse_body(char* h, /* unparsed header:in */
		char* b, /* unparsed body:in */  
		LclMailType type /* Mail type:in */){
    if( type == LclMIMEType ||
       (type == LclUnKnownType && h &&_is_mime_header(h) == True)){
	char *p ;
	if(h){
	    p = strcasestr(h, "\nContent-Transfer-Encoding:");
	}
	if(!p){
	    char *x = strcasestr(h, "Content-Transfer-Encoding:");
	    if(x == h)
		p = h ;
	}
	if(p){
	    char* eoline = strchr(p+1, '\n');
	    if(eoline){
		int b_len = strlen(b) ;
		p = strcasestr(p+27, "quoted-printable");
		if(p && (p < eoline)){
		    char *eb = malloc(b_len + 1);
		    char *db = malloc(b_len + 1);
		    memcpy(eb, b, b_len + 1);
		    str_fromqp(eb, b_len, db, b_len);
		    free(eb);
		    return db ;
		 } else {
		     p = strcasestr(p+27, "base64");
		     if(p && (p < eoline)){
			 char *eb = malloc(b_len + 1);
			 char *db = malloc(b_len + 1);
			 memcpy(eb, b, b_len + 1);
			 str_from64(eb, b_len, db, b_len);
			 free(eb);
			 return db ;
		     } 
		 }
	    }
	}
	return NULL ;
    } else { 
	/* No decoding for V3 and 822 */
	return NULL ;
    } 
}

char* 
_lcl_encode_body(char* b, /* unparsed body:in */
		 LclMailType type, /* Mail type:in */
		 char* charset, /* to charset:in */
		 LclMailEncoding e  /* base64/QP */
		 ){
    int b_len = strlen(b);
    char* orig = malloc(b_len+1);
    char* eb ;
    memcpy(orig, b, b_len + 1);
    switch(e){
      case LclBase64Encoding:
	eb = malloc(b_len*3+1);
	str_to64(orig, b_len, eb, b_len*3, 0);/* no portable newline */
	free(orig);
	break ;
      case LclQPEncoding:
	eb = malloc(b_len*2+1);
	str_toqp(orig, b_len, eb, b_len*2, 0);/* no portable newline */
	free(orig);
	break ;
      case Lcl822Encoding:
      case LclUnKnownEncoding:
      default:
	eb = orig ;
	break ;
    }
    return eb ;
}
char* 
_lcl_encode_header(char* h,            /*  unparsed header:in */
		   LclMailType type,   /* Mail type:in */
		   char* charset,      /* to charset:in */
		   LclMailEncoding he, /* header B/Q:in */
		   LclMailEncoding be, /* body base64/QP:in */
		   boolean_t add_header/* add if true:in */
		   ){
    char *eh;
    switch(he){
      case LclBase64Encoding:
/*
	eh = _lcl_b_encode_header(h, strlen(h), charset);
*/
	eh = _lcl_mime_encode_header(h, strlen(h), he, charset);
	if(add_header == B_TRUE){
	    
	}
	break ;
      case LclQPEncoding:
/*
	eh = _lcl_q_encode_header(h, strlen(h), charset);
*/
	eh = _lcl_mime_encode_header(h, strlen(h), he, charset);
	if(add_header == B_TRUE){
	    
	}
	break ;
      case Lcl822Encoding:
      case LclUnKnownEncoding:
      default:
    	eh = (char *)NULL;
	break ;
    }
    return eh ; 
}
Public char* 
_lcl_encode_taggedtext(char* t, /* unparsed taggedtext:in */
		       LclMailType type, /* Mail type:in */
		       char* charset, /* to charset:in */
		       LclMailEncoding e,  /* base64/QP */
		       boolean_t add_header/* add if true:in */
		       ){
    char* et = _lcl_encode_body(t, type, charset, e);
    if(add_header == B_TRUE){
	
    }
}
Public char* 
_lcl_parse_taggedtext(char* t, /* unparsed taggedtext:in */
		      LclMailType *type, /* Mail type:in */
		      char** charset /* charset of parsed text:out */
	){
    if(*type == LclUnKnownType){
	if(_is_V3_header(t)){
	    *type = LclV3Type ;
	} else {
	    *type = LclMIMEType ;
	}
    }
    return _lcl_parse_header(t, t, type , charset);
}
/*//////////////////////////////////////////////////*/
char*
_lcl_encode_taggedtext_body(char* b, /* unparsed body:in */
			    LclMailType type, /* Mail type:in */
			    char* charset, /* to charset:in */
			    LclMailEncoding e  /* base64/QP */
			    ){
    if(type == LclUnKnownType){
	type = LclMIMEType ; /* Brute force */
    }
    return _lcl_encode_body(b, type, charset, e);
}
char*
_lcl_encode_taggedtext_header(char* h, /*  unparsed header:in */
			      LclMailType type, /* Mail type:in */
			      char* charset,  /* to charset:in */
			      LclMailEncoding he, /* header B/Q:in */
			      LclMailEncoding be, /* body base64/QP:in */
			      boolean_t add_header /* add if true:in */
			      ){
    if(type == LclUnKnownType){
	if(_is_V3_header(h)){
	    type = LclV3Type ;
	} else {
	    type = LclMIMEType ;
	}
    }
    return _lcl_encode_header(h, type, charset, he, be, add_header);
}
char* 
_lcl_parse_taggedtext_body(char* h, /* unparsed header:in */
			   char* b, /* unparsed body:in */
			   LclMailType type /* Mail type:in */
			   ){
    if(type == LclUnKnownType){
	if(_is_V3_header(h)){
	    type = LclV3Type ;
	} else {
	    type = LclMIMEType ;
	}
    }
    return _lcl_parse_body(h, b, type);
}
char* 
_lcl_parse_taggedtext_header(char* h, /*  unparsed header:in */
			     char* b, /* unparsed body:in */
			     LclMailType *type, /* Mail type:inout */
			     char** charset /* charset of parsed header:out */
			     ){
    if(*type == LclUnKnownType){
	if(_is_V3_header(h)){
	    *type = LclV3Type ;
	} else {
	    *type = LclMIMEType ;
	}
    }
    return _lcl_parse_header(h, b, type , charset);
}
