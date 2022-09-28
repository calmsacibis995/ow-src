/*
 * Copyright (c) 1996, Sun Microsystems, Inc.
 * All rights reserved.
 */
 
#pragma ident	"@(#)lcl_cs_info.c 1.1	97/01/28 SMI"

#include "lcl.h"
#include "lcl_internal.h"

int
_lct_is_unicode_locale(LCTd lctd)
{
	if(!strcmp(lctd->lcld->locale, "ko.UTF-8") || !strcmp(lctd->lcld->locale, "en_US.UTF-8"))
		return 1;
	else
		return 0;
}

char *
_lct_get_charsetname_from_mimename(LCTd lctd, char *mime_name)
{
	LclCharsetInfo	*ptr;

	if (mime_name == (char *)NULL)
		return (char *)NULL;

	ptr = lctd->lcld->cs_info;
	while(ptr){
		if(ptr->mime_name)
			if(!lcl_strcasecmp(ptr->mime_name, mime_name))
				return ptr->name;
		ptr = ptr->next;
	}
	return (char *)NULL;
}

char *
_lct_get_charsetname_from_v3name(LCTd lctd, char *v3_name)
{
	LclCharsetInfo	*ptr;

	if (v3_name == (char *)NULL)
		return (char *)NULL;

	ptr = lctd->lcld->cs_info;
	while(ptr){
		if(ptr->v3_name)
			if(!lcl_strcasecmp(ptr->v3_name, v3_name))
				return ptr->name;
		ptr = ptr->next;
	}
	return (char *)NULL;
}

char *
_lct_get_charsetname_from_mailname(LCTd lctd, char *mail_name)
{
	char	*charset;

	charset = _lct_get_charsetname_from_mimename(lctd, mail_name);
	if(charset == (char *)NULL)
		charset = _lct_get_charsetname_from_v3name(lctd, mail_name);

	return charset;
}

char *
_lct_get_charsetname_from_iconvname(LCTd lctd, char *iconvname)
{
	LclCharsetInfo	*ptr;

	if (iconvname == (char *)NULL)
		return (char *)NULL;

	ptr = lctd->lcld->cs_info;
	while(ptr){
		if(ptr->iconv_name)
			if(!strcmp(ptr->iconv_name, iconvname))
				return ptr->name;
		ptr = ptr->next;
	}
	return (char *)NULL;
}

char *
_lct_get_mime_charsetname(LCTd lctd, char *name)
{
	LclCharsetInfo	*ptr;

	if (name == (char *)NULL)
		return (char *)NULL;

	ptr = lctd->lcld->cs_info;
	while(ptr){
		if(ptr->name)
			if(!strcmp(ptr->name, name))
				return ptr->mime_name;
		ptr = ptr->next;
	}
	return (char *)NULL;
}

char *
_lct_get_v3_charsetname(LCTd lctd, char *name)
{
	LclCharsetInfo	*ptr;

	if (name == (char *)NULL)
		return (char *)NULL;

	ptr = lctd->lcld->cs_info;
	while(ptr){
		if(ptr->name)
			if(!strcmp(ptr->name, name))
				return ptr->v3_name;
		ptr = ptr->next;
	}
	return (char *)NULL;
}

char *
_lct_get_iconv_charsetname(LCTd lctd, char *name)
{
	LclCharsetInfo	*ptr;

	if (name == (char *)NULL)
		return (char *)NULL;

	ptr = lctd->lcld->cs_info;
	while(ptr){
		if(ptr->name)
			if(!strcmp(ptr->name, name))
				return ptr->iconv_name;
		ptr = ptr->next;
	}
	return (char *)NULL;
}

static LclMailEncoding
get_mail_header_encoding_from_csname(LCTd lctd, char *name)
{
	LclCharsetInfo	*ptr;

	if (name == (char *)NULL)
		return LclUnKnownEncoding;

	ptr = lctd->lcld->cs_info;
	while(ptr){
		if(ptr->name)
			if(!strcmp(ptr->name, name))
				return ptr->mail_header_encoding;
		ptr = ptr->next;
	}
	return LclUnKnownEncoding;
}

static LclMailEncoding
get_mail_body_encoding_from_csname(LCTd lctd, char *name)
{
	LclCharsetInfo	*ptr;

	if (name == (char *)NULL)
		return LclUnKnownEncoding;

	ptr = lctd->lcld->cs_info;
	while(ptr){
		if(ptr->name)
			if(!strcmp(ptr->name, name))
				return ptr->mail_body_encoding;
		ptr = ptr->next;
	}
	return LclUnKnownEncoding;
}

LclMailType
_lcl_str_to_mailtype(char *str)
{
	if (str == (char *)NULL)
		return LclUnKnownType;
	else if(!strcmp(str, LctNMailTypeMime))
		return LclMIMEType;
	else if(!strcmp(str, LctNMailTypeV3))
		return LclV3Type;
	else if(!strcmp(str, LctNMailType822))
		return Lcl822Type;
	else if(!strcmp(str, LctNMailTypeUnknown))
		return LclUnKnownType;
	else
		return LclUnKnownType;
}

char *
_lcl_mailtype_to_str(LclMailType type)
{
	switch(type){
		case LclMIMEType :
			return _LclCreateStr(LctNMailTypeMime);
			break;
		case LclV3Type :
			return _LclCreateStr(LctNMailTypeV3);
			break;
		case Lcl822Type :
			return _LclCreateStr(LctNMailType822);
			break;
		default :
			return _LclCreateStr(LctNMailTypeUnknown);
			break;
	}
}

LclMailEncoding
_lcl_str_to_mailencoding(LCTd lctd, char *str, int header_flag, char *charset)
{
    LctNEAttribute	encoding;

    if (str == (char *)NULL)
	return LclUnKnownEncoding;

    if(!strcmp(str, LctNDbDefault)){
	if(charset == (char *)NULL)
	    return LclUnKnownEncoding;
	else{
	    if(header_flag)
		return(get_mail_header_encoding_from_csname(lctd, charset));
	    else
		return(get_mail_body_encoding_from_csname(lctd, charset));
	}
    }
    if (!lcl_strcasecmp(str, LctNMailEncodingQP))
	return(LclQPEncoding);
    else if (!lcl_strcasecmp(str, LctNMailEncodingBase64))
	return(LclBase64Encoding);
    else
	return(LclUnKnownEncoding);
}

char *
_lct_get_ascii_end_info(LCTd lctd, char *name)
{
	LclCharsetInfo	*ptr;

	if (name == (char *)NULL)
		return (char *)NULL;

	ptr = lctd->lcld->cs_info;
	while(ptr){
		if(ptr->name)
			if(!strcmp(ptr->name, name))
				return ptr->ascii_end;
		ptr = ptr->next;
	}
	return (char *)NULL;
}

char *
_lct_get_ascii_superset_info(LCTd lctd, char *name)
{
	LclCharsetInfo	*ptr;

	if (name == (char *)NULL)
		return (char *)NULL;

	ptr = lctd->lcld->cs_info;
	while(ptr){
		if(ptr->name)
			if(!strcmp(ptr->name, name))
				return ptr->ascii_superset;
		ptr = ptr->next;
	}
	return (char *)NULL;
}

LclFormat
_lct_get_cs_format_info(LCTd lctd, char *name)
{
	LclCharsetInfo	*ptr;

	if (name == (char *)NULL)
	    return(LclUnKnownFormat);

	ptr = lctd->lcld->cs_info;
	while(ptr){
		if(ptr->name)
			if(!strcmp(ptr->name, name))
				return ptr->format;
		ptr = ptr->next;
	}
	return(LclUnKnownFormat);
}

char *
_lct_get_charsetname_from_format(LCTd lctd, LclFormat format)
{
	LclCharsetInfo	*ptr;

	if (format == LclUnKnownFormat)
	    return (char*)NULL;

	ptr = lctd->lcld->cs_info;
	while(ptr){
		if(ptr->format)
			if(ptr->format == format)
				return ptr->name;
		ptr = ptr->next;
	}
	return (char *)NULL;
}

LclFormat
_lct_get_format_from_charsetname(LCTd lctd, char *charset)
{
	LclCharsetInfo	*ptr;

	if (charset == (char *)NULL)
	    return(LclUnKnownFormat);

	ptr = lctd->lcld->cs_info;
	while(ptr){
		if(ptr->name)
			if(!strcmp(ptr->name, charset))
				return ptr->format;
		ptr = ptr->next;
	}
	return(LclUnKnownFormat);
}

char *
_lct_get_charset_encoding(LCTd lctd, LctNEAttribute form, LctNEAttribute attr)
{
    char	*charset;
    LclMailEncoding encoding;

    if (attr == LctNHeaderCharsetEncoding){
	charset = _lct_get_string_attribute(lctd, form, LctNHeaderCharset);
	encoding = get_mail_header_encoding_from_csname(lctd, charset);
    }
    else if (attr == LctNBodyCharsetEncoding){
	charset = _lct_get_string_attribute(lctd, form, LctNBodyCharset);
	encoding = get_mail_body_encoding_from_csname(lctd, charset);
    }
    else
	return (char *)NULL;

    return(encoding == LclQPEncoding ? LctNMailEncodingQP : 
	   encoding == LclBase64Encoding ? LctNMailEncodingBase64 : (char *)NULL);
}
