/*
 * Copyright (c) 1996, Sun Microsystems, Inc.
 * All rights reserved.
 */
 
#pragma ident	"@(#)lcl_attr.c 1.8	96/07/14 SMI"

#include "lcl.h"
#include "lcl_internal.h"


LclCharsetLinkedList *
_lcl_copy_charset_linked_list(LclCharsetLinkedList *list)
{
	LclCharsetLinkedList	*new_list, *prev_ptr, *new_ptr, *ref_ptr;

	new_list = (LclCharsetLinkedList *)NULL;

	ref_ptr = list;
	prev_ptr = (LclCharsetLinkedList *)NULL;
	while(ref_ptr){
		new_ptr = (LclCharsetLinkedList *)malloc(sizeof(LclCharsetLinkedList));
		if(new_ptr == (LclCharsetLinkedList *)NULL){
			_lcl_destroy_charset_linked_list(new_list);
			return (LclCharsetLinkedList *)NULL;
		}
		new_ptr->name = _LclCreateStr(ref_ptr->name);
		new_ptr->next = (LclCharsetLinkedList *)NULL;
		if(prev_ptr)
			prev_ptr->next = new_ptr;
		else
			new_list = new_ptr;
		prev_ptr = new_ptr;
		ref_ptr = ref_ptr->next;
	}

	return new_list;
}

void
_lcl_destroy_charset_linked_list(LclCharsetLinkedList	*list)
{
	LclCharsetLinkedList	*ptr, *old_ptr;

	ptr = list;
	while(ptr){
		old_ptr = ptr;
		ptr = ptr->next;
		if(old_ptr->name)
			free(old_ptr->name);
		free(old_ptr);
	}
}

LclCharsetLinkedList *
_lcl_create_one_charset_linked_list(char *charset)
{
	LclCharsetLinkedList	*list;

	list = (LclCharsetLinkedList *)malloc(sizeof(LclCharsetLinkedList));
	if(list == (LclCharsetLinkedList *)NULL)
		return (LclCharsetLinkedList *)NULL;

	list->name = _LclCreateStr(charset);
	if(list->name == (char *)NULL){
		free(list);
		return (LclCharsetLinkedList *)NULL;
	}

	list->next = (LclCharsetLinkedList *)NULL;

	return list;
}


static int
substitute_to_one_string(LclCharsetLinkedList **ptr, char *new_str)
{
	_lcl_destroy_charset_linked_list(*ptr);
	*ptr = _lcl_create_one_charset_linked_list(new_str);
	if(*ptr)
		return 0;
	else
		return -1;
}

static int
substitute_string(char **ptr, char *new_str)
{
	if (*ptr)
		free(*ptr);
	*ptr = _LclCreateStr(new_str);
	if(*ptr)
		return 0;
	else
		return -1;
}

static int 
set_mailtype_from_string(LclMailType *mailtype, char *str)
{
    if (!lcl_strcasecmp(str, LctNMailTypeMime))
	*mailtype = LclMIMEType;
    else if (!lcl_strcasecmp(str, LctNMailTypeV3))
	*mailtype = LclV3Type;
    else if (!lcl_strcasecmp(str, LctNMailType822))
	*mailtype = Lcl822Type;
    else 
	*mailtype = LclUnKnownType;
    return 0;
}

static int
set_encoding_from_string(LclMailEncoding *encoding, char *str)
{
  if (!lcl_strcasecmp(str, LctNMailEncodingQP))
      *encoding = LclQPEncoding;
  else if (!lcl_strcasecmp(str, LctNMailEncodingBase64))
      *encoding = LclBase64Encoding;
  else if (!lcl_strcasecmp(str, LctNMailEncodingNone))
      *encoding = LclUnKnownEncoding;
  else
      return -1;
  return 0;
}

static int
set_string_attribute(LCTd lctd, LctNEAttribute form, LctNEAttribute attr, char *str)
{
    LclFormAttrInfo	*ptr;

    ptr = lctd->attr_info;
    while(ptr){
	if(ptr->name == form){
	    switch(attr) {
	    case LctNHeaderCharset:
		return substitute_to_one_string(&(ptr->header_charset), str);
	    case LctNBodyCharset:
		return substitute_to_one_string(&(ptr->body_charset), str);
	    case LctNPlainTextCharset:
		return substitute_to_one_string(&(ptr->body_charset), str);
	    case LctNTaggedTextCharset:
		return substitute_to_one_string(&(ptr->body_charset), str);
	    case LctNHeaderEncoding:
		return set_encoding_from_string(&(ptr->header_encoding), str);
	    case LctNBodyEncoding:
		return set_encoding_from_string(&(ptr->body_encoding), str);
	    case LctNPlainTextEncoding:
		return set_encoding_from_string(&(ptr->body_encoding), str);
	    case LctNTaggedTextEncoding:
		return set_encoding_from_string(&(ptr->body_encoding), str);
	    case LctNMailType:
		return set_mailtype_from_string(&(ptr->mail_type), str);
	    default:
		return -1;
	    }
	}
	ptr = ptr->next;
    }
    return -1;
}

int
_lct_set_string_attribute(LCTd lctd, LctNEAttribute form, LctNEAttribute attr, char *str)
{
	return set_string_attribute(lctd, form, attr, str);
}

static int
set_mail_string_attribute(LCTd lctd, LctNEAttribute form, LctNEAttribute attr, char *str)
{
	char	*mail_type;
	char	*charset;

	mail_type = _lct_get_string_attribute(lctd, form, LctNMailType);

	if(!strcmp(mail_type, LctNMailTypeV3)){
		charset = _lct_get_charsetname_from_v3name(lctd, str);
		if(charset == (char *)NULL)
			charset = _lct_get_charsetname_from_mimename(lctd, str);
	}
	else{
		charset = _lct_get_charsetname_from_mimename(lctd, str);
		if(charset == (char *)NULL)
			charset = _lct_get_charsetname_from_v3name(lctd, str);
	}

	if(charset)
		return set_string_attribute(lctd, form, attr, charset);
	else
		return -1;
}

static int
set_iconv_string_attribute(LCTd lctd, LctNEAttribute form, LctNEAttribute attr, char *str)
{
	char	*charset;

	charset = _lct_get_charsetname_from_iconvname(lctd, str);

	if(charset)
		return set_string_attribute(lctd, form, attr, charset);
	else
		return -1;
}

int
_lct_set_context_string_attribute(LCTd lctd, LctNEAttribute form, LctNEAttribute attr, char *str)
{
	LclForm	form_type = _lct_get_formtype_from_formname(lctd, form);

	if((form_type == LclTypeOutGoingStream) || (form_type == LclTypeInComingStream))
		return set_mail_string_attribute(lctd, form, attr, str);
	else
		return set_iconv_string_attribute(lctd, form, attr, str);
}

static char *
get_one_string(LclCharsetLinkedList *list)
{
	if(list == (LclCharsetLinkedList *)NULL)
		return (char *)NULL;

	return list->name;
}

static char *
get_string_from_mailtype(LclMailType mailtype)
{
    switch(mailtype)	
	{
	case LclMIMEType:
	    return(LctNMailTypeMime);
	case LclV3Type:
	    return(LctNMailTypeV3);
	case Lcl822Type:
	    return(LctNMailType822);
	case LclUnKnownType:
	default:
	    return(LctNMailTypeUnknown);
	}
}

static char *
get_string_from_encoding(LclMailEncoding encoding)
{
  if (encoding == LclBase64Encoding)
      return(LctNMailEncodingBase64);
  else if (encoding == LclQPEncoding)
      return(LctNMailEncodingQP);
  else
      return (char *)NULL;
}

static char *
get_string_attribute(LCTd lctd, LctNEAttribute form, LctNEAttribute attr)
{
    LclFormAttrInfo	*ptr;

    ptr = lctd->attr_info;
    while(ptr){
	if(ptr->name == form){
	    switch(attr) {
	    case LctNHeaderCharset:
		return get_one_string(ptr->header_charset);
	    case LctNBodyCharset:
		return get_one_string(ptr->body_charset);
	    case LctNPlainTextCharset:
		return get_one_string(ptr->body_charset);
	    case LctNTaggedTextCharset:
		return get_one_string(ptr->body_charset);
	    case LctNHeaderEncoding:
		return get_string_from_encoding(ptr->header_encoding);
	    case LctNBodyEncoding:
		return get_string_from_encoding(ptr->body_encoding);
	    case LctNPlainTextEncoding:
		return get_string_from_encoding(ptr->body_encoding);
	    case LctNTaggedTextEncoding:
		return get_string_from_encoding(ptr->body_encoding);
	    case LctNMailType:
		return get_string_from_mailtype(ptr->mail_type);
	    default:
		return (char *)NULL;
	    }
	}
	ptr = ptr->next;
    }
    return (char *)NULL;
}

static char *
get_default_string_attribute(LCTd lctd, LctNEAttribute form, LctNEAttribute attr)
{
    LclFormInfo	*ptr;

    ptr = lctd->lcld->form_info;
    while(ptr){
	if(ptr->name == form){
	    switch(attr) {
	    case LctNHeaderCharset:
		switch(lctd->contents->type) {
		case LctNMsgText:
		    return get_one_string(ptr->msg_header_charset);
		case LctNTaggedText:
		    return get_one_string(ptr->taggedtext_header_charset);
		case LctNSeparatedTaggedText:
		    return get_one_string(ptr->taggedtext_header_charset);
		default:
		    return (char *)NULL;
		}
	    case LctNBodyCharset:
		switch (lctd->contents->type) {
		case LctNMsgText:
		    return get_one_string(ptr->msg_body_charset);
		case LctNPlainText:
		    return get_one_string(ptr->plaintext_body_charset);
		case LctNTaggedText:
		    return get_one_string(ptr->taggedtext_body_charset);
		case LctNSeparatedTaggedText:
		    return get_one_string(ptr->taggedtext_body_charset);
		default:
		    return (char *)NULL;
		}
	    case LctNPlainTextCharset:
		return get_one_string(ptr->plaintext_body_charset);
	    case LctNTaggedTextCharset:
		return get_one_string(ptr->taggedtext_body_charset);
	    case LctNHeaderEncoding:
		switch(lctd->contents->type) {
		case LctNMsgText:
		    return get_string_from_encoding(ptr->msg_header_encoding);
		case LctNTaggedText:
		    return get_string_from_encoding(ptr->taggedtext_header_encoding);
		case LctNSeparatedTaggedText:
		    return get_string_from_encoding(ptr->taggedtext_header_encoding);
		default:
		    return (char *)NULL;
		}
	    case LctNBodyEncoding:
		switch(lctd->contents->type) {
		case LctNMsgText:
		    return get_string_from_encoding(ptr->msg_body_encoding);
		case LctNPlainText:
		    return get_string_from_encoding(ptr->plaintext_body_encoding);
		case LctNTaggedText:
		    return get_string_from_encoding(ptr->taggedtext_body_encoding);
		case LctNSeparatedTaggedText:
		    return get_string_from_encoding(ptr->taggedtext_body_encoding);
		default:
		    return (char *)NULL;
		}
	    case LctNPlainTextEncoding:
		return get_string_from_encoding(ptr->plaintext_body_encoding);
	    case LctNTaggedTextEncoding:
		return get_string_from_encoding(ptr->taggedtext_body_encoding);
	    case LctNMailType:
		return get_string_from_mailtype(ptr->mail_type);
	    default:
		return (char *)NULL;
	    }
	}
	ptr = ptr->next;
    }
    return (char *)NULL;
}

char	*
_lct_get_string_attribute(LCTd lctd, LctNEAttribute form, LctNEAttribute attr)
{
	return get_string_attribute(lctd, form, attr);
}

char	*
_lct_get_default_string_attribute(LCTd lctd, LctNEAttribute form, LctNEAttribute attr)
{
	return get_default_string_attribute(lctd, form, attr);
}

char	*
_lct_get_mail_string_attribute(LCTd lctd, LctNEAttribute form, LctNEAttribute attr)
{
	char	*charset, *mail_charset;
	char	*mail_type_str;
	LclMailType	mail_type;

	charset = get_string_attribute(lctd, form, attr);

	mail_type_str = _lct_get_string_attribute(lctd, form, LctNMailType);
	mail_type = _lcl_str_to_mailtype(mail_type_str);
	switch(mail_type){
		case LclV3Type :
			mail_charset =  _lct_get_v3_charsetname(lctd, charset);
			break;
		default :
			mail_charset = _lct_get_mime_charsetname(lctd, charset);
			break;
	}
	if(mail_charset)
		charset = mail_charset;

	return charset;
}

char	*
_lct_get_iconv_string_attribute(LCTd lctd, LctNEAttribute form, LctNEAttribute attr)
{
	char	*charset, *iconv_charset;
	char	*mail_type_str;
	LclMailType	mail_type;

	charset = get_string_attribute(lctd, form, attr);

	iconv_charset = _lct_get_iconv_charsetname(lctd, charset);
	if(iconv_charset)
		charset = iconv_charset;

	return charset;
}

char	*
_lct_get_context_string_attribute(LCTd lctd, LctNEAttribute form, LctNEAttribute attr)
{
	LclForm	form_type = _lct_get_formtype_from_formname(lctd, form);

	if((form_type == LclTypeOutGoingStream) || (form_type == LclTypeInComingStream))
		return _lct_get_mail_string_attribute(lctd, form, attr);
	else
		return _lct_get_iconv_string_attribute(lctd, form, attr);
}

static LclCharsetLinkedList *
get_string_linkedlist_attribute(LCTd lctd , LctNEAttribute form, LctNEAttribute attr)
{
    LclFormAttrInfo	*ptr;

    ptr = lctd->attr_info;
    while(ptr){
	if(ptr->name == form){
	    switch(attr) {
	    case LctNHeaderCharsetList:
		return ptr->header_charset;
	    case LctNBodyCharsetList:
		return ptr->body_charset;
	    case LctNPlainTextCharsetList:
		return ptr->body_charset;
	    case LctNTaggedTextCharsetList:
		return ptr->body_charset;
	    default:
		return (LclCharsetLinkedList *)NULL;
	    }
	}
	ptr = ptr->next;
    }
    return (LclCharsetLinkedList *)NULL;
}

static LclCharsetLinkedList *
get_default_string_linkedlist_attribute(LCTd lctd, LctNEAttribute form, LctNEAttribute attr)
{
	return _lct_get_default_string_linkedlist_attribute_with_type(lctd, lctd->contents->type, form, attr);
}

LclCharsetLinkedList *
_lct_get_default_string_linkedlist_attribute_with_type(LCTd lctd, LctNEAttribute type, LctNEAttribute form, LctNEAttribute attr)
{
    LclFormInfo	*ptr;

    ptr = lctd->lcld->form_info;
    while(ptr){
	if(ptr->name == form){
	    switch(attr) {
	    case LctNHeaderCharsetList:
		switch(type) {
		case LctNMsgText:
		    return ptr->msg_header_charset;
		case LctNTaggedText:
		    return ptr->taggedtext_header_charset;
		case LctNSeparatedTaggedText:
		    return ptr->taggedtext_header_charset;
		default:
		    return NULL;
		}
	    case LctNBodyCharsetList:
		switch(type) {
		case LctNMsgText:
		    return ptr->msg_body_charset;
		case LctNPlainText:
		    return ptr->plaintext_body_charset;
		case LctNTaggedText:
		    return ptr->taggedtext_body_charset;
		case LctNSeparatedTaggedText:
		    return ptr->taggedtext_body_charset;
		default:
		    return NULL;
		}
	    case LctNPlainTextCharsetList:
		return ptr->plaintext_body_charset;
	    case LctNTaggedTextCharsetList:
		return ptr->taggedtext_body_charset;
	    default:
		return (LclCharsetLinkedList *)NULL;
	    }
	}
	ptr = ptr->next;
    }
    return (LclCharsetLinkedList *)NULL;
}

LclCharsetLinkedList *
_lct_get_string_linkedlist_attribute(LCTd lctd, LctNEAttribute form, LctNEAttribute attr)
{
	return get_string_linkedlist_attribute(lctd, form, attr);
}

LclCharsetLinkedList *
_lct_get_default_string_linkedlist_attribute(LCTd lctd, LctNEAttribute form, LctNEAttribute attr)
{
	return get_default_string_linkedlist_attribute(lctd, form, attr);
}

static LclCharsetList *
create_charset_list(LclCharsetLinkedList *linkedlist)
{
	LclCharsetList	*list;
	LclCharsetLinkedList	*ptr;
	int	num = 0;

	list = (LclCharsetList *)malloc(sizeof(LclCharsetList));
	if (list == (LclCharsetList *)NULL)
		return (LclCharsetList *)NULL;

	ptr = linkedlist;
	while(ptr){
		num++;
		ptr = ptr->next;
	}

	list->num = num;
	list->charset = (char **)malloc(sizeof(char *) * num);
	if (list->charset == (char **)NULL){
		free(list);
		return (LclCharsetList *)NULL;
	}

	ptr = linkedlist;
	num = 0;
	while(ptr){
		list->charset[num++] = _LclCreateStr(ptr->name);
		ptr = ptr->next;
	}

	return list;
}

static LclCharsetList *
create_iconv_charset_list(LCTd lctd, LclCharsetLinkedList *linkedlist)
{
	LclCharsetList	*list;
	LclCharsetLinkedList	*ptr;
	int	num = 0;
	char	*iconv_charset;

	list = (LclCharsetList *)malloc(sizeof(LclCharsetList));
	if (list == (LclCharsetList *)NULL)
		return (LclCharsetList *)NULL;

	ptr = linkedlist;
	while(ptr){
		num++;
		ptr = ptr->next;
	}

	list->num = num;
	list->charset = (char **)malloc(sizeof(char *) * num);
	if (list->charset == (char **)NULL){
		free(list);
		return (LclCharsetList *)NULL;
	}

	ptr = linkedlist;
	num = 0;
	while(ptr){
		iconv_charset = _lct_get_iconv_charsetname(lctd, ptr->name);
		if(iconv_charset)
			list->charset[num++] = _LclCreateStr(iconv_charset);
		else
			list->charset[num++] = _LclCreateStr(ptr->name);
		ptr = ptr->next;
	}

	return list;
}

static LclCharsetList *
create_mail_charset_list(LCTd lctd, LctNEAttribute form, LclCharsetLinkedList *linkedlist)
{
	LclCharsetList	*list;
	LclCharsetLinkedList	*ptr;
	int	num = 0;
	char	*mail_type;
	char	*mail_charset;

	list = (LclCharsetList *)malloc(sizeof(LclCharsetList));
	if (list == (LclCharsetList *)NULL)
		return (LclCharsetList *)NULL;

	ptr = linkedlist;
	while(ptr){
		num++;
		ptr = ptr->next;
	}

	list->num = num;
	list->charset = (char **)malloc(sizeof(char *) * num);
	if (list->charset == (char **)NULL){
		free(list);
		return (LclCharsetList *)NULL;
	}

	mail_type = _lct_get_string_attribute(lctd, form, LctNMailType);

	ptr = linkedlist;
	num = 0;
	while(ptr){
		if(!strcmp(mail_type, LctNMailTypeV3))
			mail_charset = _lct_get_v3_charsetname(lctd, ptr->name);
		else
			mail_charset = _lct_get_mime_charsetname(lctd, ptr->name);
		if(mail_charset)
			list->charset[num++] = _LclCreateStr(mail_charset);
		else
			list->charset[num++] = _LclCreateStr(ptr->name);

		ptr = ptr->next;
	}

	return list;
}

LclCharsetList *
_lct_create_context_charset_list(LCTd lctd, LctNEAttribute form, LclCharsetLinkedList *linkedlist)
{
	LclForm	form_type = _lct_get_formtype_from_formname(lctd, form);

	if((form_type == LclTypeOutGoingStream) || (form_type == LclTypeInComingStream))
		return create_mail_charset_list(lctd, form, linkedlist);
	else
		return create_iconv_charset_list(lctd, linkedlist);
}

LclCharsetList *
_lct_get_string_list_attribute(LCTd lctd, LctNEAttribute form, LctNEAttribute attr)
{
	LclCharsetList *list;
	LclCharsetLinkedList *linkedlist;

	linkedlist = _lct_get_string_linkedlist_attribute(lctd, form, attr);
	if (linkedlist == (LclCharsetLinkedList *)NULL)
		return (LclCharsetList *)NULL;

	return create_charset_list(linkedlist);
}

LclCharsetList *
_lct_get_context_string_list_attribute(LCTd lctd, LctNEAttribute form, LctNEAttribute attr)
{
	LclCharsetList *list;
	LclCharsetLinkedList *linkedlist;

	linkedlist = _lct_get_string_linkedlist_attribute(lctd, form, attr);
	if (linkedlist == (LclCharsetLinkedList *)NULL)
		return (LclCharsetList *)NULL;

	return _lct_create_context_charset_list(lctd, form, linkedlist);
}

LclCharsetList *_lct_get_possible_string_list(LCTd lctd, LctNEAttribute form, LctNEAttribute attr)
{
    LclCharsetLinkedList *list, *default_list, *merge_list;
    LclCharsetList	*ret_list;
    char	*buf, *comp_charset;
    LctNEAttribute charset_attr;
    LclForm	form_type;

    form_type = _lct_get_formtype_from_formname(lctd, form);

    switch(attr) {
    case LctNHeaderPossibleCharsetList:
	charset_attr = LctNHeaderCharset;
	list = _lct_get_string_linkedlist_attribute(lctd, form, LctNHeaderCharsetList);
	default_list = _lct_get_default_string_linkedlist_attribute(lctd, form, LctNHeaderCharsetList);
	buf = lctd->contents->header;
	break;
    case LctNBodyPossibleCharsetList:
	charset_attr = LctNBodyCharset;
	list = _lct_get_string_linkedlist_attribute(lctd, form, LctNBodyCharsetList);
	default_list = _lct_get_default_string_linkedlist_attribute(lctd, form, LctNBodyCharsetList);
	buf = lctd->contents->body;
	break;
    case LctNPlainTextPossibleCharsetList:
	charset_attr = LctNPlainTextCharset;
	list = _lct_get_string_linkedlist_attribute(lctd, form, LctNPlainTextCharsetList);
	default_list = _lct_get_default_string_linkedlist_attribute(lctd, form, LctNPlainTextCharsetList);
	buf = lctd->contents->body;
	break;
    case LctNTaggedTextPossibleCharsetList:
	charset_attr = LctNTaggedTextCharset;
	list = _lct_get_string_linkedlist_attribute(lctd, form, LctNTaggedTextCharsetList);
	default_list = _lct_get_default_string_linkedlist_attribute(lctd, form, LctNTaggedTextCharsetList);
	buf = lctd->contents->body;
	break;
    default:
	return (LclCharsetList *)NULL;
    }

    merge_list = _lcl_copy_charset_linked_list(list);
    if(merge_list == (LclCharsetLinkedList *)NULL)
	merge_list = _lcl_copy_charset_linked_list(default_list);
    else{
	LclCharsetLinkedList	*ref_ptr, *cur_ptr, *last_ptr, *new_ptr;
	last_ptr = merge_list;
	while(last_ptr->next)
	    last_ptr = last_ptr->next;
	cur_ptr = default_list;
	while(cur_ptr){
	    ref_ptr = merge_list;
	    while(ref_ptr){
		if(!strcmp(ref_ptr->name, cur_ptr->name))
		    goto next_loop;
		else
		    ref_ptr = ref_ptr->next;
	    }
	    new_ptr = (LclCharsetLinkedList *)malloc(sizeof(LclCharsetLinkedList));
	    if(new_ptr == (LclCharsetLinkedList *)NULL){
		return (LclCharsetList *)NULL;
	    }
	    new_ptr->name = _LclCreateStr(cur_ptr->name);
	    new_ptr->next = (LclCharsetLinkedList *)NULL;
	    last_ptr->next = new_ptr;
	    last_ptr = last_ptr->next;
	next_loop:
	    cur_ptr = cur_ptr->next;
	}
    }

    if (merge_list == (LclCharsetLinkedList *)NULL)
	return (LclCharsetList *)NULL;

    if(form_type == LclTypeInComingStream){
	LclCharsetLinkedList *prev_ptr, *ptr;

	comp_charset = _lct_get_string_attribute(lctd, LctNDisplayForm, charset_attr);
	if(comp_charset == (char *)NULL)
	    return (LclCharsetList *)NULL;

	prev_ptr = (LclCharsetLinkedList *)NULL;
	ptr = merge_list;
	while(ptr){
	    if(_lct_check_iconv(lctd, ptr->name, comp_charset) || (!strcmp(ptr->name, LctNAsciiCharsetName) && (_lcl_check_ascii(buf, strlen(buf)) != B_TRUE))){
		LclCharsetLinkedList *del_ptr = ptr;
		ptr = ptr->next;

		if(prev_ptr == (LclCharsetLinkedList *)NULL)
		    merge_list = del_ptr->next;
		else
		    prev_ptr->next = del_ptr->next;
		if(del_ptr->name)
		    free(del_ptr->name);
		free(del_ptr);
	    }
	    else{
		prev_ptr = ptr;
		ptr = ptr->next;
	    }
	}
    }
    else if(form_type == LclTypeOutGoingStream){
	LclCharsetLinkedList *prev_ptr, *ptr;

	comp_charset = _lct_get_string_attribute(lctd, LctNDisplayForm, charset_attr);
	if(comp_charset == (char *)NULL)
	    return (LclCharsetList *)NULL;

	ptr = merge_list;
	prev_ptr = (LclCharsetLinkedList *)NULL;
	while(ptr){
	    if(_lct_check_convert(lctd, comp_charset, ptr->name, buf, strlen(buf)) != 0){
		LclCharsetLinkedList *del_ptr = ptr;
		ptr = ptr->next;

		if(prev_ptr == (LclCharsetLinkedList *)NULL)
		    merge_list = del_ptr->next;
		else
		    prev_ptr->next = del_ptr->next;
		if(del_ptr->name)
		    free(del_ptr->name);
		free(del_ptr);
	    }
	    else{
		prev_ptr = ptr;
		ptr = ptr->next;
	    }
#ifdef AAA
	    /* Hardwired for UTF-8 locale */
	    if(_lct_is_unicode_locale(lctd)){
		if(_lct_check_convert(lctd, comp_charset, ptr->name, buf, strlen(buf)) || (!strcmp(ptr->name, LctNAsciiCharsetName) && (_lcl_check_ascii(buf, strlen(buf)) != B_TRUE))){
		    LclCharsetLinkedList *del_ptr = ptr;
		    ptr = ptr->next;

		    if(prev_ptr == (LclCharsetLinkedList *)NULL)
			merge_list = del_ptr->next;
		    else
			prev_ptr->next = del_ptr->next;
		    if(del_ptr->name)
			free(del_ptr->name);
		    free(del_ptr);
		}
		else{
		    prev_ptr = ptr;
		    ptr = ptr->next;
		}
	    }
	    /* general */
	    else{
		if(_lct_check_iconv(lctd, comp_charset, ptr->name) || (!strcmp(ptr->name, LctNAsciiCharsetName) && (_lcl_check_ascii(buf, strlen(buf)) != B_TRUE))){
			
		    LclCharsetLinkedList *del_ptr = ptr;
		    ptr = ptr->next;

		    if(prev_ptr == (LclCharsetLinkedList *)NULL)
			merge_list = del_ptr->next;
		    else
			prev_ptr->next = del_ptr->next;
		    if(del_ptr->name)
			free(del_ptr->name);
		    free(del_ptr);
		}
		else{
		    prev_ptr = ptr;
		    ptr = ptr->next;
		}
	    }
#endif
	}
    }

    ret_list = _lct_create_context_charset_list(lctd, form, merge_list);
    _lcl_destroy_charset_linked_list(merge_list);
	
    return ret_list;
}

void
lcl_destroy_charset_list(LclCharsetList *list)
{
	int	i;

	if (list == (LclCharsetList *)NULL)
		return;

	for(i = 0; i < list->num; i++)
		free(list->charset[i]);
	free(list->charset);
	free(list);
}

void	delete_top_charset_from_list(LclCharsetLinkedList **list)
{
	LclCharsetLinkedList	*del_list;

	del_list = *list;
	if(del_list){
		if(del_list->name)
			free(del_list->name);
		*list = del_list->next;
	}
}
		
void	_lcl_delete_top_charset_from_list(LCTd lctd, LctNEAttribute form, LctNEAttribute attr)
{
    LclFormAttrInfo	*ptr;

    ptr = lctd->attr_info;
    while(ptr){
	if(ptr->name == form){
	    switch(attr){
	    case LctNHeaderCharsetList:
	    case LctNHeaderCharset:
		delete_top_charset_from_list(&(ptr->header_charset));
		return;
	    case LctNBodyCharsetList:
	    case LctNBodyCharset:
		delete_top_charset_from_list(&(ptr->body_charset));
		return;
	    case LctNPlainTextCharsetList:
	    case LctNPlainTextCharset:
		delete_top_charset_from_list(&(ptr->body_charset));
		return;
	    case LctNTaggedTextCharsetList:
	    case LctNTaggedTextCharset:
		delete_top_charset_from_list(&(ptr->body_charset));
		return;
	    default:
		return;
	    }
	}
	ptr = ptr->next;
    }
    return;
}

LclForm
_lct_get_formtype_from_formname(LCTd lctd, LctNEAttribute form)
{
	LclFormAttrInfo	*ptr;

	ptr = lctd->attr_info;
	while(ptr){
		if(ptr->name == form)
			return ptr->type;
		ptr = ptr->next;
	}
	return LclTypeOther;
}
