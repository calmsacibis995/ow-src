/*
 * Copyright (c) 1996, Sun Microsystems, Inc.
 * All rights reserved.
 */
 
#pragma ident	"@(#)lcl_internal.h 1.1	97/01/28 SMI"

#include <iconv.h>

/* need to delete
#include "lcl_db.h"
#include "lcl_str.h"
#include "lcl_cs_info.h"
*/

/* These defines should be the real names */
#define LctNMailEncodingBase64	"Base64"
#define LctNMailEncodingQP	"Quoted-Printable"
#define LctNMailEncodingNone	"None"
#define LctNAsciiCharsetName	"ASCII"
#define LctNCsFormatASCII	"ASCII"
#define LctNCsFormat7bit	"7BIT"
#define LctNCsFormatISO9496	"ISO_94_96"
#define LctNCsFormatISO94Ext	"ISO_94_Ext"
#define LctNCsFormatMBString	"MB_String"
#define LctNDbTrue		"True"
#define LctNDbDefault		"Default"

/* private structs */
typedef struct _LclSegmentList	LclSegmentList;
struct _LclSegmentList{
	char *segment;
	char *charset;
	size_t  size;
	LclSegmentList  *next;
};

typedef struct{
	char	*ptr;
	int	pos;
	int	length;
} LclBuffer;

/* lcl.c */
LCTd	_lct_create_msg(LCLd lcld, LctNEAttribute type, char *header, char *body, LctNEAttribute form, LctNEAttribute keep_str);
LCTd	_lct_create_plain(LCLd lcld, LctNEAttribute type, char *buf, LctNEAttribute form, LctNEAttribute keep_str);
LCTd	_lct_create_tagged(LCLd lcld, LctNEAttribute type, char *buf, LctNEAttribute form, LctNEAttribute keep_str);
LCTd	_lct_create_buf(LCLd lcld, LctNEAttribute type, char *buf, size_t length, LctNEAttribute form, LctNEAttribute keep_str);
int	_lct_add_header_buffer(LCTd lctd, LctNEAttribute form, char *buf);

/* lcl_convert.c */
int	_lct_get_convert(LCTd lctd, LctNEAttribute to_form, LctNEAttribute attr, LclCharsetSegmentSet **to_seg);
int	_lct_check_iconv(LCTd lctd, char *from_charset, char *to_charset);
int	_lct_check_convert(LCTd lctd, char *from_name, char *to_name, char *ptr, size_t len);

/* lcl_attr.c */
int	_lct_set_string_attribute(LCTd lctd, LctNEAttribute form, LctNEAttribute attr, char *str);
int	_lct_set_context_string_attribute(LCTd lctd, LctNEAttribute form, LctNEAttribute attr, char *str);
char	*_lct_get_string_attribute(LCTd lctd, LctNEAttribute form, LctNEAttribute attr);
char	*_lct_get_default_string_attribute(LCTd lctd, LctNEAttribute form, LctNEAttribute attr);
char	*_lct_get_context_string_attribute(LCTd lctd, LctNEAttribute form, LctNEAttribute attr);
char	*_lct_get_mail_string_attribute(LCTd lctd, LctNEAttribute form, LctNEAttribute attr);
char	*_lct_get_iconv_string_attribute(LCTd lctd, LctNEAttribute form, LctNEAttribute attr);
LclCharsetLinkedList	*_lct_get_string_linkedlist_attribute(LCTd lctd, LctNEAttribute form, LctNEAttribute attr);
LclCharsetLinkedList	*_lct_get_default_string_linkedlist_attribute(LCTd lctd, LctNEAttribute form, LctNEAttribute attr);
LclCharsetLinkedList	*_lct_get_default_string_linkedlist_attribute_with_type(LCTd lctd, LctNEAttribute type, LctNEAttribute form, LctNEAttribute attr);
LclCharsetList	*_lct_get_string_list_attribute(LCTd lctd, LctNEAttribute form, LctNEAttribute attr);
LclCharsetList *_lct_get_context_string_list_attribute(LCTd lctd, LctNEAttribute form, LctNEAttribute attr);
LclCharsetList	*_lct_get_possible_string_list(LCTd lctd, LctNEAttribute form, LctNEAttribute attr);
void	_lcl_delete_top_charset_from_list(LCTd lctd, LctNEAttribute form, LctNEAttribute attr);
LclCharsetLinkedList	*_lcl_create_one_charset_linked_list(char *charset);
LclCharsetLinkedList	*_lcl_copy_charset_linked_list(LclCharsetLinkedList *list);
void	_lcl_destroy_charset_linked_list(LclCharsetLinkedList *list);
LclCharsetList	*_lct_create_context_charset_list(LCTd lctd, LctNEAttribute form, LclCharsetLinkedList *linkedlist);
LclForm	_lct_get_formtype_from_formname(LCTd lctd, LctNEAttribute form);

/* lcl_segment.c */
LclSegmentList	*_lcl_add_segment_list(LclSegmentList *list, char *charset, char *buf, size_t size);
void	_lcl_destroy_segment_list(LclSegmentList *list);
LclCharsetSegmentSet	*_lcl_create_one_segment_set(char *charset, char *buf, size_t size);
LclCharsetSegmentSet	*_lcl_make_charset_segment_set(LclSegmentList *list);

/* lcl_cs_info.c */
char	*_lct_get_charsetname_from_mimename(LCTd lctd, char *mimename);
char	*_lct_get_charsetname_from_v3name(LCTd lctd, char *v3name);
char	*_lct_get_charsetname_from_iconvname(LCTd lctd, char *iconvname);
char	*_lct_get_charsetname_from_mailname(LCTd lctd, char *mail_name);
char	*_lct_get_mime_charsetname(LCTd lctd, char *name);
char	*_lct_get_v3_charsetname(LCTd lctd, char *name);
char	*_lct_get_iconv_charsetname(LCTd lctd, char *name);
LclMailType	_lcl_str_to_mailtype(char *str);
char	*_lcl_mailtype_to_str(LclMailType type);
LclMailEncoding	_lcl_str_to_mailencoding(LCTd lctd, char *str, int header_flag, char *charset);
LclFormInfo	*_lct_get_forminfo_from_name(LCTd lctd, char *name);
char	*_lct_get_ascii_end_info(LCTd lctd, char *charset);
char	*_lct_get_ascii_superset_info(LCTd lctd, char *charset);
LclFormat	_lct_get_cs_format_info(LCTd lctd, char *name);
char	*_lct_get_charsetname_from_format(LCTd lctd, LclFormat format);
LclFormat	_lct_get_format_from_charsetname(LCTd lctd, char *charset);
int	_lct_is_unicode_locale(LCTd lctd);
char	*_lct_get_charset_encoding(LCTd lctd, LctNEAttribute form, LctNEAttribute attr);

/* lcl_str.c */
char	*_LclCreateStr(char *str);
char	*_LclCreateStrn(char *str, int len);

/* lcl_db.c */
int	_LclCreateDatabase(LCLd lcld, char *pathname);
void	_LclDestroyDatabase(LCLd lcld);
int	_LclParseCharsetInfo(LCLd lcld);
int	_LclParseIconvInfo(LCLd lcld);
int	_LclParseFormInfo(void *db, LclFormInfo **form_info);

/* lcl_ascii.c */
char	*_lcl_convert_to_ascii(char *buf, size_t len);
boolean_t	_lcl_check_ascii(char *buf, size_t len);
boolean_t	_lcl_check_printable_ascii(char *buf, size_t len);

/* lcl_parse_header.c */
char* _lcl_parse_body(char* h, /* unparsed header:in */
		      char* b, /* unparsed body:in */
		      LclMailType type /* Mail type:in */
	);
char* _lcl_parse_header(char* h, /*  unparsed header:in */
			char* b, /* unparsed body:in */
			LclMailType *type, /* Mail type:inout */
			char** charset /* charset of parsed header:out */
	);
char* _lcl_parse_taggedtext_body(char* h, /* unparsed header:in */
				 char* b, /* unparsed body:in */
				 LclMailType type /* Mail type:in */
	);
char* _lcl_parse_taggedtext_header(char* h, /*  unparsed header:in */
				   char* b, /* unparsed body:in */
				   LclMailType *type, /* Mail type:inout */
				   char** charset /* charset of parsed header:out */
	);
char* _lcl_parse_taggedtext(char* t, /* unparsed taggedtext:in */
			    LclMailType *type, /* Mail type:inout */
			    char** charset /* charset of parsed text:out */
	);
LclMailType _lcl_get_body_charset(char* h, /* unparsed header:in */
				  char* b, /* unparsed body:in */
				  LclMailType type, /* Mail type:in */
				  char** charset /* charset of parsed header:out */
	);
LclMailType _lcl_get_taggedtext_charset(char* t, /* unparsed taggedtext:in*/
		                  LclMailType type, /* Mail type:in */
                                  char** charset /* charset of parsed header:out */
        );
char* _lcl_encode_taggedtext(char* t, /* unparsed taggedtext:in */
		       LclMailType type, /* Mail type:in */
		       char* charset, /* to charset:in */
		       LclMailEncoding e,  /* base64/QP */
		       boolean_t add_header/* add if true:in */
	);
char* _lcl_encode_body(char* b, /* unparsed body:in */
		       LclMailType type, /* Mail type:in */
		       char* charset, /* to charset:in */
		       LclMailEncoding e  /* base64/QP */
	);
char* _lcl_encode_header(char* h, /*  unparsed header:in */
			LclMailType type, /* Mail type:in */
		        char* charset,  /* to charset:in */
			LclMailEncoding he, /* header B/Q:in */
			LclMailEncoding be, /* body base64/QP:in */
			boolean_t add_header /* add if true:in */
	);
char* _lcl_encode_taggedtext_body(char* b, /* unparsed body:in */
		       LclMailType type, /* Mail type:in */
		       char* charset, /* to charset:in */
		       LclMailEncoding e  /* base64/QP */
	);
char* _lcl_encode_taggedtext_header(char* h, /*  unparsed header:in */
			LclMailType type, /* Mail type:in */
		        char* charset,  /* to charset:in */
			LclMailEncoding he, /* header B/Q:in */
			LclMailEncoding be, /* body base64/QP:in */
			boolean_t add_header /* add if true:in */
	);

/* lsl_strstr.c */
char * strcasestr(char* s1,char* s2);

/* lcl_buf.c */
LclBuffer	*_LclBuffer_create(int length);
int	_LclBuffer_add(LclBuffer *buf, char *str, int len);
char	*_LclBuffer_get_string(LclBuffer *buf);
void	_LclBuffer_destroy(LclBuffer *buf);

/* lcl_mime_encode.c */
int	_lcl_next_token(char **p, int *len, char **token, int *token_len, char **delimiter_str);
int	_lcl_strncmp_with_string_list(char *ptr, int len, char **str_list, int case_flag);
char	*_lcl_mime_encode_header(char *header, int header_len, LclMailEncoding encoding, char *charset);
char	*_lcl_mime_decode_header(char *header, int length, char **charset);

/* lcl_qencode.c */
int	_lcl_q_encode_header_line(LclBuffer *buf, char *ptr, int len, char *charset);
int	_lcl_q_encode_text(LclBuffer *buf, char *ptr, int len, int *pos, char *charset);
int	_lcl_add_qp_decode(LclBuffer *out, char *ptr, int len);

/* lcl_bencode.c */
int	_lcl_b_encode_header_line(LclBuffer *buf, char *ptr, int len, char *charset);
int	_lcl_b_encode_text(LclBuffer *out, char *ptr, int len, int *pos, char *charset);
int	_lcl_add_b64_decode(LclBuffer *out, char *ptr, int len);

/* lcl_query_type.c */
LclContentInfo *_lct_get_query_content_type(LCTd lctd, LctNEAttribute form);
int	_lct_check_content_from_charset(LCTd lctd, char *ptr, size_t len, size_t *num, char *charset);

/* lcl_iconv.c */
iconv_t	_lcl_iconv_open(char *to_name, char *from_name);
void	_lcl_iconv_close(iconv_t i_conv);
size_t	_lcl_iconv(iconv_t i_conv, char **inbuf, size_t *inbytesleft, char **outbuf, size_t *outbytesleft);
