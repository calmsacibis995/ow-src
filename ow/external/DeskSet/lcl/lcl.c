/*
 * Copyright (c) 1996, Sun Microsystems, Inc.
 * All rights reserved.
 */
 
#pragma ident	"@(#)lcl.c 1.6	96/08/27 SMI"

#include <string.h>
#include <stdlib.h>
#include <locale.h>
#include <iconv.h>
#include <errno.h>

#include "lcl_types.h"
#include "lcl.h"
#include "lcl_internal.h"

static struct lcld_list_rec{
	char	*locale;
	LCLd	lcld;
	int	ref_count;
	struct lcld_list_rec	*next;
};

static struct lcld_list_rec	*lcld_list = (struct lcld_list_rec *)NULL;

static void
destroy_form_info(LclFormInfo *form_info)
{
	LclFormInfo	*ptr, *ptr_old;
	LclCharsetLinkedList	*csp, *csp_old;

	if(form_info){
		ptr = form_info;
		while(ptr){
			ptr_old = ptr;
			ptr = ptr_old->next;
			if(ptr_old->msg_header_charset)
				_lcl_destroy_charset_linked_list(ptr_old->msg_header_charset);
			if(ptr_old->msg_body_charset)
				_lcl_destroy_charset_linked_list(ptr_old->msg_body_charset);
			if(ptr_old->plaintext_body_charset)
				_lcl_destroy_charset_linked_list(ptr_old->plaintext_body_charset);
			if(ptr_old->taggedtext_header_charset)
				_lcl_destroy_charset_linked_list(ptr_old->taggedtext_header_charset);
			if(ptr_old->taggedtext_body_charset)
				_lcl_destroy_charset_linked_list(ptr_old->taggedtext_body_charset);
			free(ptr_old);
		}
	}
}

LCLd
lcl_open(char *locale)
{
	struct lcld_list_rec	*rec, *ptr;
	char	*name;
	LCLd	lcld;
	char	pathname[256];

	if(locale == (char *)NULL)
		locale = setlocale(LC_ALL, (char *)NULL);

	ptr = lcld_list;
	while(ptr){
		if(!strcmp(ptr->locale, locale)){
			(ptr->ref_count)++;
			return ptr->lcld;
		}
		ptr = ptr->next;
	}

	if((name = _LclCreateStr(locale)) == (char *)NULL){
		return (LCLd)NULL;
	}

	if((lcld = (LCLd)malloc(sizeof(struct _LCLd))) == (LCLd)NULL){
		free(name);
		return (LCLd)NULL;
	}
	memset(lcld, 0, sizeof(struct _LCLd));

	if((rec = (struct lcld_list_rec *)malloc(sizeof(struct lcld_list_rec))) == (struct lcld_list_rec *)NULL){
		free(name);
		free(lcld);
		return (LCLd)NULL;
	}
	if(lcld_list == (struct lcld_list_rec *)NULL){
		lcld_list = rec;
	}
	else{
		ptr = lcld_list;
		while(ptr->next){
			ptr = ptr->next;
		}
		ptr->next = rec;
	}

	rec->locale = name;
	rec->lcld = lcld;
	rec->ref_count = 1;
	rec->next = (struct lcld_list_rec *)NULL;

	if((lcld->locale = _LclCreateStr(locale)) == (char *)NULL){
		lcl_close(rec->lcld);
		return (LCLd)NULL;
	}

	sprintf(pathname, "/usr/lib/locale/%s/LC_CTYPE/LCL_DEF", rec->locale);
	if(_LclCreateDatabase(lcld, pathname)){
		lcl_close(lcld);
		return (LCLd)NULL;
	}

	if(_LclParseFormInfo(lcld->db, &(lcld->form_info)) < 0){
		lcl_close(lcld);
		return (LCLd)NULL;
	}
	if(_LclParseCharsetInfo(lcld)){;
		lcl_close(lcld);
		return (LCLd)NULL;
	}
	if(_LclParseIconvInfo(lcld)){;
		lcl_close(lcld);
		return (LCLd)NULL;
	}

	return lcld;
}

void
lcl_close(LCLd lcld)
{
	struct lcld_list_rec	*ptr, *prev_ptr;
	int	i;

	ptr = lcld_list;
	if(ptr == (struct lcld_list_rec *)NULL)
		return;

	if(!strcmp(ptr->locale, lcld->locale)){
		(ptr->ref_count)--;
		if(ptr->ref_count <= 0){
			lcld_list = ptr->next;
		}
		else
			return;
	}
	else{
		prev_ptr = lcld_list;
		ptr = lcld_list->next;
		while(ptr){
			if(!strcmp(ptr->locale, lcld->locale)){
				(ptr->ref_count)--;
				if(ptr->ref_count <= 0){
					prev_ptr->next = ptr->next;
					break;
				}
				else
					return;
			}
		}
		if(ptr == (struct lcld_list_rec *)NULL)
			return;
	}

	if(ptr->locale)
		free(ptr->locale);

	if(lcld->locale)
		free(lcld->locale);
	if(lcld->db)
		_LclDestroyDatabase(lcld);
	if(lcld->form_info)
		destroy_form_info(lcld->form_info);
	if(lcld->cs_info){
		LclCharsetInfo	*info, *old_info;
		info = lcld->cs_info;
		while(info){
			if(info->name)
				free(info->name);
			if(info->mime_name)
				free(info->mime_name);
			if(info->v3_name)
				free(info->v3_name);
			if(info->iconv_name)
				free(info->iconv_name);
			if(info->ascii_end)
				free(info->ascii_end);
			if(info->ascii_superset)
				free(info->ascii_superset);
			old_info = info;
			info = info->next;
			free(old_info);
		}
	}
	if(lcld->iconv_info){
		LclIconvInfo	*info, *old_info;
		info = lcld->iconv_info;
		while(info){
			if(info->atom){
				for(i = 0; i < info->atom_num; i++){
					if(info->atom[i].from)
						free(info->atom[i].from);
					if(info->atom[i].to)
						free(info->atom[i].to);
				}
				free(info->atom);
			}
			if(info->from_encoding)
				free(info->from_encoding);
			if(info->to_encoding)
				free(info->to_encoding);
			old_info = info;
			info = info->next;
			free(old_info);
		}
	}
	free(ptr->lcld);
	free(ptr);
}

static void
destroy_attr_info(LclFormAttrInfo *info)
{
	LclFormAttrInfo	*prev_ptr, *ptr;

	ptr = info;
	while(ptr){
		prev_ptr = ptr;
		ptr = ptr->next;
		if(prev_ptr->header_charset)
			_lcl_destroy_charset_linked_list(prev_ptr->header_charset);
		if(prev_ptr->body_charset)
			_lcl_destroy_charset_linked_list(prev_ptr->body_charset);
		free(prev_ptr);
	}
}

static LclFormAttrInfo *
make_attr_info(LCTd lctd, LctNEAttribute type)
{
	LclFormInfo	*form_ptr;
	LclFormAttrInfo	*attr_info, *cur_ptr, *new_ptr;

	attr_info = (LclFormAttrInfo *)NULL;

	form_ptr = lctd->lcld->form_info;
	cur_ptr = (LclFormAttrInfo *)NULL;
	while(form_ptr){
		new_ptr = (LclFormAttrInfo *)malloc(sizeof(LclFormAttrInfo));
		if(new_ptr == (LclFormAttrInfo *)NULL){
			destroy_attr_info(attr_info);
			return (LclFormAttrInfo *)NULL;
		}
		new_ptr->type = form_ptr->type;
		new_ptr->name = form_ptr->name;
		switch(type) {
		case LctNMsgText:
			new_ptr->header_charset = _lcl_copy_charset_linked_list(form_ptr->msg_header_charset);
			new_ptr->body_charset = _lcl_copy_charset_linked_list(form_ptr->msg_body_charset);
			new_ptr->header_encoding = form_ptr->msg_header_encoding;
			new_ptr->body_encoding = form_ptr->msg_body_encoding;
			break;
		case LctNPlainText:
			new_ptr->header_charset = (LclCharsetLinkedList *)NULL;
			new_ptr->body_charset = _lcl_copy_charset_linked_list(form_ptr->plaintext_body_charset);
			new_ptr->header_encoding = LclUnKnownEncoding;
			new_ptr->body_encoding = form_ptr->plaintext_body_encoding;
			break;
		default:
			new_ptr->header_charset = _lcl_copy_charset_linked_list(form_ptr->taggedtext_header_charset);;
			new_ptr->body_charset = _lcl_copy_charset_linked_list(form_ptr->taggedtext_body_charset);
			new_ptr->header_encoding = form_ptr->taggedtext_header_encoding;
			new_ptr->body_encoding = form_ptr->taggedtext_body_encoding;
			break;
		}
		new_ptr->mail_type = form_ptr->mail_type;
		new_ptr->next = (LclFormAttrInfo *)NULL;

		if(cur_ptr)
			cur_ptr->next = new_ptr;
		else
			attr_info = new_ptr;
		cur_ptr = new_ptr;
		form_ptr = form_ptr->next;
	}

	return attr_info;
}
	
static LCTd
lct_create_core(LCLd lcld, LctNEAttribute type)
{
	LCTd	lctd;

	lctd = (LCTd)malloc(sizeof(struct _LCTd));
	if(lctd == (LCTd)NULL)
		return (LCTd)NULL;
	memset(lctd, NULL, sizeof(struct _LCTd));

	lctd->lcld = lcld;

	lctd->attr_info = make_attr_info(lctd, type);
	if(lctd->attr_info == (LclFormAttrInfo *)NULL){
		lct_destroy(lctd);
		return (LCTd)NULL;
	}

	lctd->contents = (LclContents *)malloc(sizeof(LclContents));
	if(lctd->contents == (LclContents *)NULL){
		lct_destroy(lctd);
		return (LCTd)NULL;
	}
	memset(lctd->contents, NULL, sizeof(LclContents));

	return lctd;
}

void
lct_destroy(LCTd lctd)
{
	if(lctd->contents){
		if(lctd->contents->copy == B_TRUE){
			if(lctd->contents->header)
				free(lctd->contents->header);
			if(lctd->contents->body)
				free(lctd->contents->body);
			if(lctd->contents->plaintext)
				free(lctd->contents->plaintext);
			if(lctd->contents->taggedtext)
				free(lctd->contents->taggedtext);
			if(lctd->contents->buf)
				free(lctd->contents->buf);
		}
		free(lctd->contents);
	}

	if(lctd->attr_info)
		destroy_attr_info(lctd->attr_info);

	free(lctd);
}

LCTd
_lct_create_msg(LCLd lcld, LctNEAttribute type, char *header, char *body, LctNEAttribute form, LctNEAttribute keep_str)
{
	LCTd	lctd;
	LclForm	form_type;

	lctd = lct_create_core(lcld, type);
	if (lctd == (LCTd)NULL)
		return (LCTd)NULL;

	lctd->contents->type = type;

	lctd->contents->form = form;

	if (keep_str == LctNKeepByValue) {
		lctd->contents->copy = B_TRUE;
		if(header != (char *)LctNNone){
		        int headerLength = strlen(header);
			lctd->contents->header = (char *)malloc(headerLength + 1);
			if(lctd->contents->header == (char *)NULL){
				lct_destroy(lctd);
				return (LCTd)NULL;
			}
			strcpy(lctd->contents->header, header);
			lctd->contents->header[headerLength] = (char)0;
		}
		if(body != (char *)LctNNone){
		        int bodyLength = strlen(body);
			lctd->contents->body = (char *)malloc(bodyLength + 1);
			if(lctd->contents->body == (char *)NULL){
				lct_destroy(lctd);
				return (LCTd)NULL;
			}
			strcpy(lctd->contents->body, body);
			lctd->contents->body[bodyLength] = (char)0;
		}
	}
	else{
		lctd->contents->copy = B_FALSE;
		if(header != (char *)LctNNone)
			lctd->contents->header = header;
		if(body != (char *)LctNNone)
			lctd->contents->body = body;
	}

#ifdef AAA
/****** due to the bug(4007820) of determining the type and charset ******/
/****** this part should not be used till the bug is fixed          ******/
	form_type = _lct_get_formtype_from_formname(lctd, lctd->contents->form);
	if(form_type == LclTypeInComingStream){
		/* parse header and set body charset */
		char	*mail_type_str;
		LclMailType	mail_type;
		LclFormInfo	*form_info;
		char	*mail_charset;
		char	*mail_type_string;

		mail_type_str = _lct_get_string_attribute(lctd, lctd->contents->form, LctNMailType);
		mail_type = _lcl_str_to_mailtype(mail_type_str);

		if (lctd->contents->type == LctNMsgText)
			mail_type = _lcl_get_body_charset(lctd->contents->header, lctd->contents->body, mail_type, &mail_charset);
		else
			mail_type = _lcl_get_taggedtext_charset(lctd->contents->header, mail_type, &mail_charset);

		mail_type_string = _lcl_mailtype_to_str(mail_type);
		if(mail_type_string){
			_lct_set_string_attribute(lctd, lctd->contents->form, LctNMailType, mail_type_string);
			free(mail_type_string);
		}
		if(mail_charset){
			char *new_charset;
			switch(mail_type){
				case LclMIMEType :
					new_charset = _lct_get_charsetname_from_mimename(lctd, mail_charset);
					break;
				case LclV3Type :
					new_charset = _lct_get_charsetname_from_v3name(lctd, mail_charset);
					break;
			default:
					new_charset = (char *)NULL;
			}
			free(mail_charset);
			if(new_charset)
				_lct_set_string_attribute(lctd, lctd->contents->form, LctNBodyCharset, new_charset);
		}
	}
#endif

	return lctd;
}

LCTd
_lct_create_plain(LCLd lcld, LctNEAttribute type, char *buf, LctNEAttribute form, LctNEAttribute keep_str)
{
	LCTd	lctd;

	lctd = lct_create_core(lcld, type);
	if (lctd == (LCTd)NULL)
		return (LCTd)NULL;

	lctd->contents->type = type;

	lctd->contents->form = form;

	if (keep_str == LctNKeepByValue) {
	        int bufLength = strlen(buf);
		lctd->contents->copy = B_TRUE;
		lctd->contents->plaintext = (char *)malloc(bufLength + 1);
		if(lctd->contents->plaintext == (char *)NULL){
			lct_destroy(lctd);
			return (LCTd)NULL;
		}
		lctd->contents->plaintext[bufLength] = (char)0;
	}
	else{
		lctd->contents->copy = B_FALSE;
		lctd->contents->plaintext = buf;
	}

	return lctd;
}

LCTd
_lct_create_tagged(LCLd lcld, LctNEAttribute type, char *buf, LctNEAttribute form, LctNEAttribute keep_str)
{
	LCTd	lctd;
	LclForm	form_type;

	lctd = lct_create_core(lcld, type);
	if (lctd == (LCTd)NULL)
		return (LCTd)NULL;

	lctd->contents->type = type;

	lctd->contents->form = form;

	if (keep_str == LctNKeepByValue) {
	        int bufLength = strlen(buf);
		lctd->contents->copy = B_TRUE;
		lctd->contents->taggedtext = (char *)malloc(bufLength + 1);
		if(lctd->contents->taggedtext == (char *)NULL){
			lct_destroy(lctd);
			return (LCTd)NULL;
		}
		lctd->contents->taggedtext[bufLength] = (char)0;
	}
	else{
		lctd->contents->copy = B_FALSE;
		lctd->contents->taggedtext = buf;
	}

	form_type = _lct_get_formtype_from_formname(lctd, lctd->contents->form);
	if(form_type == LclTypeInComingStream){
		/* parse header and set body charset */
		char	*mail_type_str;
		LclMailType	mail_type;
		LclFormInfo	*form_info;
		char	*mail_charset;
		char	*mail_type_string;

		mail_type_str = _lct_get_string_attribute(lctd, lctd->contents->form, LctNMailType);
		mail_type = _lcl_str_to_mailtype(mail_type_str);
		mail_type = _lcl_get_taggedtext_charset(lctd->contents->taggedtext, mail_type, &mail_charset);
		mail_type_string = _lcl_mailtype_to_str(mail_type);
		if(mail_type_string){
			_lct_set_string_attribute(lctd, lctd->contents->form, LctNMailType, mail_type_string);
			free(mail_type_string);
		}
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
			free(mail_charset);
			if(new_charset)
				_lct_set_string_attribute(lctd, lctd->contents->form, LctNTaggedTextCharset, new_charset);
		}
	}

	return lctd;
}

LCTd
_lct_create_buf(LCLd lcld, LctNEAttribute type, char *buf, size_t length, LctNEAttribute form, LctNEAttribute keep_str)
{
	LCTd	lctd;

	lctd = lct_create_core(lcld, type);
	if (lctd == (LCTd)NULL)
		return (LCTd)NULL;

	lctd->contents->type = type;

	lctd->contents->form = form;

	if (keep_str == LctNKeepByValue) {
		lctd->contents->copy = B_TRUE;
		lctd->contents->buf = (char *)malloc(length);
		if(lctd->contents->buf == (char *)NULL){
			lct_destroy(lctd);
			return (LCTd)NULL;
		}
		lctd->contents->buf_len = length;
	}
	else{
		lctd->contents->copy = B_FALSE;
		lctd->contents->buf = buf;
		lctd->contents->buf_len = length;
	}

	return lctd;
}

int
_lct_add_header_buffer(LCTd lctd, LctNEAttribute form, char *buf)
{
	char	*new_buf;
	size_t	old_size, new_size;

	if(lctd->contents->copy != B_TRUE)
		return -1;

	if(lctd->contents->header)
		old_size = strlen(lctd->contents->header);
	else
		old_size = 0;

	new_size = strlen(buf);

	new_buf = (char *)malloc(old_size + new_size + 1);
	if(new_buf == (char *)NULL)
		return -1;

	memcpy(new_buf, lctd->contents->header, old_size);
	memcpy(new_buf + old_size, buf, new_size);
	new_buf[new_size + old_size] = (char)0;

	free(lctd->contents->header);
	lctd->contents->header = new_buf;

	return 0;
}
