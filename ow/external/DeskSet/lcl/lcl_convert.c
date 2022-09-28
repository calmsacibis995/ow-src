/*
 * Copyright (c) 1996, Sun Microsystems, Inc.
 * All rights reserved.
 */
 
#pragma ident	"@(#)lcl_convert.c 1.12	96/08/09 SMI"

#include <stdlib.h>
#include <locale.h>
#include <iconv.h>
#include <errno.h>

#include "lcl_types.h"
#include "lcl.h"
#include "lcl_internal.h"


static LclIconvInfo *
get_iconv_info(LCLd lcld, char *from_encoding, char *to_encoding)
{
	LclIconvInfo	*ptr;

	if((from_encoding == (char *)NULL) || (to_encoding == (char *)NULL))
		return (LclIconvInfo *)NULL;

	ptr = lcld->iconv_info;
	while(ptr){
		if(!strcmp(ptr->from_encoding, from_encoding) && !strcmp(ptr->to_encoding, to_encoding))
			return ptr;
		if(ptr->direction == LclTypeBothway){
			if(!strcmp(ptr->to_encoding, from_encoding) && !strcmp(ptr->from_encoding, to_encoding))
				return ptr;
		}
		ptr = ptr->next;
	}
	return (LclIconvInfo *)NULL;
}

static int
exec_iconv(LCTd lctd, LclIconvInfo *iconv_info, char *from, size_t from_size, char **to, size_t *to_size, int reverse_flag)
{
	size_t	ret;
	char	*from_buf, *to_buf;
	size_t	from_len, to_len;
	char	*from_name, *to_name;
	char	*from_ptr, *to_ptr;
	size_t	from_bytes, to_bytes;
	int	i;
	int	err_code = LctErrorNone;

	from_buf = (char *)NULL, to_buf = (char *)NULL;
	from_len = from_size;
	from_buf = from;
	for(i = 0; i < iconv_info->atom_num; i++){
		if(reverse_flag){
			from_name = iconv_info->atom[iconv_info->atom_num - 1 - i].to;
			to_name = iconv_info->atom[iconv_info->atom_num - 1 - i].from;
		}
		else{
			from_name = iconv_info->atom[i].from;
			to_name = iconv_info->atom[i].to;
		}

		to_len = from_len * 2;
		while(1){
			iconv_t	i_conv;
			int	lcl_iconv_flag;

			i_conv = iconv_open(to_name, from_name);
			if(i_conv == (iconv_t)-1){
				i_conv = _lcl_iconv_open(to_name, from_name);
				if(i_conv == (iconv_t)-1){
					err_code = LctErrorIconvCannotOpen;
					goto err_return;
				}
				else
					lcl_iconv_flag = 1;
			}
			else
				lcl_iconv_flag = 0;

			to_buf = (char *)malloc(to_len + 1);
			if(to_buf == (char *)NULL){
				if(lcl_iconv_flag)
					_lcl_iconv_close(i_conv);
				else
					iconv_close(i_conv);
				err_code = LctErrorNotEnoughResource;
				goto err_return;
			}
			from_ptr = from_buf, to_ptr = to_buf;
			from_bytes = from_len, to_bytes = to_len;

			if(lcl_iconv_flag)
				ret = _lcl_iconv(i_conv, &from_ptr, &from_bytes, &to_ptr, &to_bytes);
			else
				ret = iconv(i_conv, (const char **)(&from_ptr), &from_bytes, &to_ptr, &to_bytes);
			if(ret == -1){
				if(lcl_iconv_flag)
					_lcl_iconv_close(i_conv);
				else
					iconv_close(i_conv);
				if(errno == E2BIG){
					to_len = to_len * 2;
					free(to_buf);
					to_buf = (char *)NULL;
					continue;
				}
				else{
					break;
				}
			}
			else{ /* reset seqence for state dependent conversion */
				size_t	reset_ret;
				char *null_ptr = (char *)NULL;
				typedef const char **cast_type;

				if(lcl_iconv_flag)
					reset_ret = _lcl_iconv(i_conv, &null_ptr, &from_bytes, &to_ptr, &to_bytes);
				else
					reset_ret = iconv(i_conv, (cast_type)&null_ptr, &from_bytes, &to_ptr, &to_bytes);
				if((reset_ret == -1) && (errno == E2BIG)){
					if(lcl_iconv_flag)
						_lcl_iconv_close(i_conv);
					else
						iconv_close(i_conv);
					to_len = to_len * 2;
					free(to_buf);
					to_buf = (char *)NULL;
					continue;
				}

				if(lcl_iconv_flag)
					_lcl_iconv_close(i_conv);
				else
					iconv_close(i_conv);
				break;
			}
		}

		if((ret == -1) && (to_bytes >= to_len)){
			err_code = LctErrorIconvError;
			goto err_return;
		}
		if(ret > 0){
			err_code = LctErrorIconvNonIdenticalConversion;
		}

		from_len = to_len - to_bytes;
		if(from_buf != from)
			free(from_buf);
		from_buf = (char *)realloc(to_buf, from_len + 1);
		from_buf[from_len] = (char)NULL;

		if(ret == -1){
			err_code = LctErrorIconvHalfDone;
			break;
		}
	}
	*to = from_buf;
	*to_size = from_len;
	return err_code;

err_return:
	if(from_buf && (from_buf != from))
		free(from_buf);
	if(to_buf)
		free(to_buf);
	return err_code;
}

static char *
determine_to_charset(LCTd lctd, char *from_charset, LctNEAttribute to_form, LctNEAttribute attr, char *ptr, size_t len)
{
    LctNEAttribute	list_attr;
    LclCharsetLinkedList	*charset_list;
    LclIconvInfo	*iconv_info;
    char	*to_charset = (char *)NULL;

    if(lctd->lcld->iconv_info == (LclIconvInfo *)NULL)
	return (char *)NULL;

    switch(attr) {
    case LctNHeaderCharset:
	list_attr = LctNHeaderCharsetList;
	break;
    case LctNBodyCharset:
	list_attr = LctNBodyCharsetList;
	break;
    case LctNPlainTextCharset:
	list_attr = LctNPlainTextCharsetList;
	break;
    case LctNTaggedTextCharset:
	list_attr = LctNTaggedTextCharsetList;
	break;
    default:
	return (char *)NULL;
    }

    charset_list = _lct_get_string_linkedlist_attribute(lctd, to_form, list_attr);
    if(charset_list == (LclCharsetLinkedList *)NULL)
	return (char *)NULL;

    while(charset_list){
	if(charset_list->name && strcmp(charset_list->name, LctNAsciiCharsetName)){
	    if (_lct_check_convert(lctd, from_charset, charset_list->name, ptr, len) == 0)
		return charset_list->name;
	}
	charset_list = charset_list->next;
    }
    return (char *)NULL;
}

int
_lct_check_iconv(LCTd lctd, char *from_charset, char *to_charset)
{
	int	reverse_flag, i;
	char	*from_name, *to_name;
	iconv_t	i_conv;
	LclIconvInfo *iconv_info;

	if(!strcmp(from_charset, to_charset))
		return 0;
	if(!strcmp(from_charset, LctNAsciiCharsetName))
		return 0;
	if(!strcmp(to_charset, LctNAsciiCharsetName))
		return 0;

	iconv_info = get_iconv_info(lctd->lcld, from_charset, to_charset);
	if(iconv_info == (LclIconvInfo *)NULL)
		return -1;

	if(!strcmp(iconv_info->from_encoding, from_charset))
		reverse_flag = 0;
	else
		reverse_flag = 1;

	for(i = 0; i < iconv_info->atom_num; i++){
		if(reverse_flag){
			from_name = iconv_info->atom[iconv_info->atom_num - 1 - i].to;
			to_name = iconv_info->atom[iconv_info->atom_num - 1 - i].from;
		}
		else{
			from_name = iconv_info->atom[i].from;
			to_name = iconv_info->atom[i].to;
		}

		i_conv = iconv_open(to_name, from_name);
		if(i_conv == (iconv_t)-1){
			i_conv = _lcl_iconv_open(to_name, from_name);
			if(i_conv == (iconv_t)-1)
				return -1;
			else
				_lcl_iconv_close(i_conv);
		}
		else
			iconv_close(i_conv);
	}
	return 0;
}

int
_lct_convert_charset(LCTd lctd, char *from_charset, char *from_buf, size_t from_size, char *to_charset, char **to_buf, size_t *to_size)
{
	LclIconvInfo	*iconv_info;
	char	*from_attr, *to_attr;

	if(!strcmp(from_charset, to_charset)){
		size_t	num;
/*		if (_lct_check_content_from_charset(lctd, from_buf, from_size, &num, from_charset) != 0){ */
			goto simple_copy_label;
/*		} 
		else{ 
			if(num > 0){
				*to_size = num;
				*to_buf = (char *)malloc(num + 1);
				if(*to_buf == (char *)NULL)
					return LctErrorNotEnoughResource;
				memcpy(*to_buf, from_buf, *to_size);
				(*to_buf)[*to_size] = (char)0;
				return LctErrorIconvHalfDone;
			}
			else{
				return LctErrorIconvError;
			}
		}
*/
	}

	if(!strcmp(from_charset, LctNAsciiCharsetName)){
		if(_lcl_check_ascii(from_buf, from_size) == B_TRUE)
			goto simple_copy_label;
		else
			return LctErrorIconvError;
	}
	if(!strcmp(to_charset, LctNAsciiCharsetName)){
		if(_lcl_check_ascii(from_buf, from_size) == B_TRUE)
			goto simple_copy_label;
		else
			return LctErrorIconvError;
	}

	from_attr = _lct_get_ascii_superset_info(lctd, from_charset);
	to_attr = _lct_get_ascii_superset_info(lctd, to_charset);
	if((from_attr != (char *)NULL) && !strcmp(from_attr, LctNDbTrue)){
		if((to_attr != (char *)NULL) && !strcmp(to_attr, LctNDbTrue)){
			if(_lcl_check_printable_ascii(from_buf, from_size) == B_TRUE)
				goto simple_copy_label;
		}
	}


	iconv_info = get_iconv_info(lctd->lcld, from_charset, to_charset);
	if(iconv_info == (LclIconvInfo *)NULL)
		return LctErrorDBCannotFindIconvDef;

	if(!strcmp(iconv_info->from_encoding, from_charset))
		return exec_iconv(lctd, iconv_info, from_buf, from_size, to_buf, to_size, 0);
	else
		return exec_iconv(lctd, iconv_info, from_buf, from_size, to_buf, to_size, 1);

simple_copy_label:
	*to_size = from_size;
	*to_buf = (char *)malloc(*to_size + 1);
	if(*to_buf == (char *)NULL)
		return LctErrorNotEnoughResource;
	memcpy(*to_buf, from_buf, *to_size);
	(*to_buf)[*to_size] = (char)0;
	return 0;
}

int
_lct_check_convert(LCTd lctd, char *from_name, char *to_name, char *ptr, size_t len)
{
	int	ret;
	char	*to_buf;
	size_t	to_len;

	ret = _lct_convert_charset(lctd, from_name, ptr, len, to_name, &to_buf, &to_len);
	if(ret && (ret != LctErrorIconvNonIdenticalConversion) && (ret != LctErrorIconvHalfDone))
		return -1;

	if(to_buf)
		free(to_buf);

	if(ret)
		return -1;
	else
		return 0;
}

static int
convert_from_charset_list(LCTd lctd, LclCharsetLinkedList *from_charset_list, char **from_charset, char *from_buf, size_t from_size, char *to_charset, char **to_buf, size_t *to_size)
{
	LclCharsetLinkedList	*charset_list;
	int	ret;

	charset_list = from_charset_list;
	while(charset_list){
		ret = _lct_convert_charset(lctd, charset_list->name, from_buf, from_size, to_charset, to_buf, to_size);
		if(ret == 0){
			*from_charset = charset_list->name;
			return 0;
		}
		else if((ret == LctErrorIconvNonIdenticalConversion) || (ret == LctErrorIconvHalfDone)){
			free(*to_buf);
		}
		charset_list = charset_list->next;
	}

/* Comment out because if a kind of control character is contained in text, */
/* lcl lib may chop off the part which is after that character              */
/*
	charset_list = from_charset_list;
	while(charset_list){
		ret = _lct_convert_charset(lctd, charset_list->name, from_buf, from_size, to_charset, to_buf, to_size);
		if((ret == LctErrorIconvNonIdenticalConversion) || (ret == LctErrorIconvHalfDone)){
			*from_charset = charset_list->name;
			return ret;
		}
		charset_list = charset_list->next;
	}
*/

	return LctErrorDBCannotFindIconvDef;
}

static int
convert_to_charset_list(LCTd lctd, LclCharsetLinkedList *charset_list, char *from_charset, char *from_buf, size_t from_size, char **to_charset, char **to_buf, size_t *to_size)
{
	int	ret;
	while(charset_list){
		ret = _lct_convert_charset(lctd, from_charset, from_buf, from_size, charset_list->name, to_buf, to_size);
		if(ret == 0){
			*to_charset = charset_list->name;
			return 0;
		}
		else if((ret == LctErrorIconvNonIdenticalConversion) || (ret == LctErrorIconvHalfDone)){
			free(*to_buf);
		}
		charset_list = charset_list->next;
	}
	return LctErrorDBCannotFindIconvDef;
}

static int
check_SS3(char *str, size_t len)
{
	unsigned char *ptr = (unsigned char *)str;

	while((len - 1) > 0){
		if((*ptr == 0x1b) && (*(ptr + 1) == 0x4f))
			return 1;
		ptr++;
		len--;
	}
	return 0;
}

int
_lct_get_convert(LCTd lctd, LctNEAttribute to_form, LctNEAttribute attr, LclCharsetSegmentSet **to_segs)
{
    char	*from_charset, *to_charset;
    char	*from_buf, *to_buf;
    size_t	from_size, to_size;
    LclCharsetSegmentSet	*segs = (LclCharsetSegmentSet *)NULL;
    int	ret = LctErrorNone;
    LctNEAttribute	charset_attr, charset_list_attr;
    boolean_t	free_from_buf = B_FALSE;
    boolean_t	try_to_ascii = B_TRUE;
    LclForm from_type, to_type;
    LctNEAttribute	from_form;

    from_form = lctd->contents->form;

    from_type = _lct_get_formtype_from_formname(lctd, from_form);
    to_type = _lct_get_formtype_from_formname(lctd, to_form);

    switch(attr){
    case LctNHeaderSegment:
    case LctNContentOfHeaderSegment:
	charset_attr = LctNHeaderCharset;
	charset_list_attr = LctNHeaderCharsetList;
	from_buf = lctd->contents->header;
	from_size = strlen(lctd->contents->header);
	break;
    case LctNBodySegment:
	charset_attr = LctNBodyCharset;
	charset_list_attr = LctNBodyCharsetList;
	if(lctd->contents->type == LctNTaggedText){
	    from_buf = lctd->contents->taggedtext;
	    from_size = strlen(lctd->contents->taggedtext);
	}
	else{
	    from_buf = lctd->contents->body;
	    from_size = strlen(lctd->contents->body);
	}
	break;
    case LctNPlainTextSegment:
	charset_attr = LctNPlainTextCharset;
	charset_list_attr = LctNPlainTextCharsetList;
	from_buf = lctd->contents->plaintext;
	from_size = strlen(lctd->contents->plaintext);
	break;
    case LctNTaggedTextSegment:
    case LctNContentOfTaggedTextSegment:
	charset_attr = LctNTaggedTextCharset;
	charset_list_attr = LctNTaggedTextCharsetList;
	from_buf = lctd->contents->taggedtext;
	from_size = strlen(lctd->contents->taggedtext);
    default:
	return -1;
    }

    from_charset = _lct_get_string_attribute(lctd, from_form, charset_attr);
    if(from_charset == (char *)NULL)
	return LctErrorDBCannotFindFromCharset;

    to_charset = _lct_get_string_attribute(lctd, to_form, charset_attr);
    if(to_charset == (char *)NULL)
	return LctErrorDBCannotFindToCharset;

    /* InCommingStreamForm decode */
    if(from_type == LclTypeInComingStream){
	/* header decode */
	if(attr == LctNHeaderSegment){
	    char	*mail_type_str;
	    LclMailType	mail_type;
	    char	*mail_charset;
	    char	*new_buf;

	    new_buf = _lcl_mime_decode_header(lctd->contents->header, strlen(lctd->contents->header), &mail_charset);
	    if(new_buf){
		free_from_buf = B_TRUE;
		from_buf = new_buf;
		from_size = strlen(new_buf);
		if(mail_charset){
		    char *new_charset;
		    new_charset = _lct_get_charsetname_from_mailname(lctd, mail_charset);
		    if(new_charset){
			from_charset = new_charset;
			_lct_set_string_attribute(lctd, lctd->contents->form, charset_attr, new_charset);
		    }
		    free(mail_charset);
		}
	    }
	}
	/* body decode */
	else if(attr == LctNBodySegment){
	    if(lctd->contents->header){
		/* dtmail doesn't use this part */
		/* not fully tested */
		char	*mail_type_str;
		LclMailType	mail_type;
		char	*new_buf;

		mail_type_str = _lct_get_string_attribute(lctd, from_form, LctNMailType);
		mail_type = _lcl_str_to_mailtype(mail_type_str);
		if(lctd->contents->type == LctNMsgText)
		    new_buf = _lcl_parse_body(lctd->contents->header, lctd->contents->body, mail_type);
		else
		    new_buf = _lcl_parse_taggedtext_body(lctd->contents->header, lctd->contents->body, mail_type);
		if(new_buf){
		    free_from_buf = B_TRUE;
		    from_buf = new_buf;
		    from_size = strlen(new_buf);
		}
	    }
	}
	/* tagged text decode */
	else if(attr == LctNTaggedTextSegment){
	    /* dtmail doesn't use this part */
	    /* not fully tested */
	    char	*mail_type_str;
	    LclMailType	mail_type;
	    char	*new_buf;
	    char	*mail_charset;

	    mail_type_str = _lct_get_string_attribute(lctd, from_form, LctNMailType);
	    mail_type = _lcl_str_to_mailtype(mail_type_str);
	    new_buf = _lcl_parse_taggedtext(lctd->contents->taggedtext, &mail_type, &mail_charset);
	    if(new_buf){
		free_from_buf = B_TRUE;
		from_buf = new_buf;
		from_size = strlen(new_buf);
		if(mail_charset){
		    char *new_charset;
		    switch(mail_type){
		    case LclMIMEType :
			new_charset = _lct_get_charsetname_from_mimename(lctd, mail_charset);
			break;
		    case LclV3Type :
			new_charset = _lct_get_charsetname_from_v3name(lctd, mail_charset);
			break;
		    default :
			new_charset = (char *)NULL;
		    }
		    if(new_charset){
			from_charset = new_charset;
			_lct_set_string_attribute(lctd, lctd->contents->form, charset_attr, new_charset);
		    }
		    free(mail_charset);
		}
	    }
	}
    }
		
    /* conversion */
    if(from_type == LclTypeInComingStream){
	LclCharsetLinkedList	*from_charset_list;
	from_charset_list = _lct_get_string_linkedlist_attribute(lctd, from_form, charset_list_attr);
	if(from_charset_list == (LclCharsetLinkedList *)NULL){
	    if(free_from_buf == B_TRUE)
		free(from_buf);
	    return LctErrorDBCannotFindToCharset;
	}
	ret = convert_from_charset_list(lctd, from_charset_list, &from_charset, from_buf, from_size, to_charset, &to_buf, &to_size);
    }
    else if(to_type == LclTypeOutGoingStream){
	LclCharsetLinkedList	*to_charset_list;
	to_charset_list = _lct_get_string_linkedlist_attribute(lctd, to_form, charset_list_attr);
	if(to_charset_list == (LclCharsetLinkedList *)NULL){
	    if(free_from_buf == B_TRUE)
		free(from_buf);
	    return LctErrorDBCannotFindToCharset;
	}
	ret = convert_to_charset_list(lctd, to_charset_list, from_charset, from_buf, from_size, &to_charset, &to_buf, &to_size);
    }
    else{
	ret = _lct_convert_charset(lctd, from_charset, from_buf, from_size, to_charset, &to_buf, &to_size); 
    }

    /* free tmp buffer */
    if(free_from_buf == B_TRUE)
	free(from_buf);

    /* error return */
    if(ret && (ret != LctErrorIconvNonIdenticalConversion) && (ret != LctErrorIconvHalfDone))
	return ret;

    /* try charset degradation from ISO-2022-CN-EXT to ISO-2022-CN */
    if(!strcmp(to_charset, "ISO-2022-CN-EXT")){
	if(!check_SS3(to_buf, to_size))
	    to_charset = "ISO-2022-CN";
    }

    /* OutGoingStreamForm encode */
    if(to_type == LclTypeOutGoingStream){
	/* header */
	if(attr == LctNHeaderSegment || attr ==LctNContentOfHeaderSegment){
	    char	*mail_type_str;
	    LclMailType	mail_type;
	    char	*header_charset;
	    char	*header_mail_charset;
	    char	*header_encoding;
	    LclMailEncoding	header_encoding_type;
	    char	*body_encoding;
	    LclMailEncoding	body_encoding_type;
	    char	*alloc_buf;
	    char	*new_buf = (char *)NULL;
	
	    mail_type_str = _lct_get_string_attribute(lctd, to_form, LctNMailType);
	    mail_type = _lcl_str_to_mailtype(mail_type_str);

	    header_charset = to_charset;
	    header_mail_charset = _lct_get_mime_charsetname(lctd, header_charset);

	    header_encoding = _lct_get_string_attribute(lctd, to_form, LctNHeaderEncoding);
	    header_encoding_type = _lcl_str_to_mailencoding(lctd, header_encoding, 1, header_charset);

	    body_encoding = _lct_get_string_attribute(lctd, to_form, LctNBodyEncoding);
	    body_encoding_type = _lcl_str_to_mailencoding(lctd, body_encoding, 0, (char *)NULL);

	    alloc_buf = (char *)malloc(to_size + 1);
	    if(alloc_buf == (char *)NULL){
		goto encode_error;
	    }
	    memcpy(alloc_buf, to_buf, to_size);
	    alloc_buf[to_size] = (char)0;

	    if(attr == LctNHeaderSegment){
		if(lctd->contents->type == LctNMsgText)
		    new_buf = _lcl_encode_header(alloc_buf, mail_type, header_mail_charset, header_encoding_type, body_encoding_type, B_TRUE);
		else
		    new_buf = _lcl_encode_taggedtext_header(alloc_buf, mail_type, header_mail_charset, header_encoding_type, body_encoding_type, B_TRUE);
	    }
	    else if (attr == LctNContentOfHeaderSegment){
		new_buf = _lcl_encode_header(alloc_buf, mail_type, header_mail_charset, header_encoding_type, body_encoding_type, B_FALSE);
	    }

	    free(alloc_buf);

	    if(new_buf == (char *)NULL){
		goto encode_error;
	    }
	    else{
		free(to_buf);
		to_buf = new_buf;
		to_size = strlen(to_buf);
	    }
	}
	/* body */
	if(attr == LctNBodySegment){
	    char	*mail_type_str;
	    LclMailType	mail_type;
	    char	*header_charset;
	    char	*body_charset;
	    char	*header_mail_charset;
	    char	*body_mail_charset;
	    char	*header_encoding;
	    LclMailEncoding	header_encoding_type;
	    char	*body_encoding;
	    LclMailEncoding	body_encoding_type;
	    char	*alloc_buf;
	    char	*new_buf;
	
	    mail_type_str = _lct_get_string_attribute(lctd, to_form, LctNMailType);
	    mail_type = _lcl_str_to_mailtype(mail_type_str);

	    body_charset = to_charset;

#ifdef AAA
	    /* try to convert body to ascii */
	    if(try_to_ascii == B_TRUE){
		if(strcmp(body_charset, LctNAsciiCharsetName)){
		    if(lctd->contents->body){
			if(_lcl_check_ascii(lctd->contents->body, strlen(lctd->contents->body)) == B_TRUE){
			    body_charset = LctNAsciiCharsetName;
			    _lct_set_string_attribute(lctd, to_form, LctNBodyCharset, LctNAsciiCharsetName);
			}
		    }
		}
	    }
#endif

	    body_mail_charset = _lct_get_mime_charsetname(lctd, body_charset);


	    body_encoding = _lct_get_string_attribute(lctd, to_form, LctNBodyEncoding);
	    body_encoding_type = _lcl_str_to_mailencoding(lctd, body_encoding, 0, body_charset);

	    alloc_buf = (char *)malloc(to_size + 1);
	    if(alloc_buf == (char *)NULL){
		goto encode_error;
	    }
	    memcpy(alloc_buf, to_buf, to_size);
	    alloc_buf[to_size] = (char)0;

	    if(lctd->contents->type == LctNMsgText)
		new_buf = _lcl_encode_body(alloc_buf, mail_type, body_mail_charset, body_encoding_type);
	    else if(lctd->contents->type == LctNSeparatedTaggedText)
		new_buf = _lcl_encode_taggedtext_body(alloc_buf, mail_type, body_mail_charset, body_encoding_type);
	    else
		new_buf = _lcl_encode_taggedtext(alloc_buf, mail_type, body_mail_charset, body_encoding_type, B_FALSE);

	    free(alloc_buf);

	    if(new_buf == (char *)NULL){
		goto encode_error;
	    }
	    else{
		free(to_buf);
		to_buf = new_buf;
		to_size = strlen(to_buf);
	    }
	}
	else if(attr == LctNTaggedTextSegment || attr == LctNContentOfTaggedTextSegment){
	    char	*mail_type_str;
	    LclMailType	mail_type;
	    char	*charset;
	    char	*mail_charset;
	    char	*encoding;
	    LclMailEncoding	encoding_type;
	    char	*alloc_buf;
	    char	*new_buf;
	
	    mail_type_str = _lct_get_string_attribute(lctd, to_form, LctNMailType);
	    mail_type = _lcl_str_to_mailtype(mail_type_str);

	    charset = to_charset;
	    mail_charset = _lct_get_mime_charsetname(lctd, charset);

	    encoding = _lct_get_string_attribute(lctd, to_form, LctNTaggedTextEncoding);
	    encoding_type = _lcl_str_to_mailencoding(lctd, encoding, 0, charset);

	    alloc_buf = (char *)malloc(to_size + 1);
	    if(alloc_buf == (char *)NULL){
		goto encode_error;
	    }
	    memcpy(alloc_buf, to_buf, to_size);
	    alloc_buf[to_size] = (char)0;

	    if(attr == LctNTaggedTextSegment)
		new_buf = _lcl_encode_taggedtext(alloc_buf, mail_type, mail_charset, encoding_type, B_TRUE);
	    else if(attr == LctNContentOfTaggedTextSegment)
		new_buf = _lcl_encode_taggedtext(alloc_buf, mail_type, mail_charset, encoding_type, B_FALSE);

	    free(alloc_buf);

	    if(new_buf == (char *)NULL){
		goto encode_error;
	    }
	    else{
		free(to_buf);
		to_buf = new_buf;
		to_size = strlen(to_buf);
	    }
	}
    }

encode_error:

    if((to_type == LclTypeInComingStream) || (to_type ==  LclTypeOutGoingStream)){
	char *mail_type_str, *ret_charset;

	mail_type_str = _lct_get_string_attribute(lctd, to_form, LctNMailType);
	if(mail_type_str && !strcmp(mail_type_str, LctNMailTypeV3))
	    ret_charset = _lct_get_v3_charsetname(lctd, to_charset);
	else
	    ret_charset = _lct_get_mime_charsetname(lctd, to_charset);
	if(ret_charset)
	    to_charset = ret_charset;
    }
    else{
	char *ret_charset;
	ret_charset = _lct_get_iconv_charsetname(lctd, to_charset);
	if(ret_charset)
	    to_charset = ret_charset;
    }

    segs = _lcl_create_one_segment_set(to_charset, to_buf, to_size);
    if(segs == (LclCharsetSegmentSet *)NULL){
	free(to_buf);
	return -1;
    }

    *to_segs = segs;

    return ret;
}
