#include <iconv.h>
#include <errno.h>

#include "lcl.h"
#include "lcl_internal.h"

static int
is_printable_ascii(unsigned char uc)
{
	if((uc >= 0x20) && (uc <= 0x7e))
		return 1;
	if((uc == 0x09) || (uc == 0x0d) || (uc == 0x0a))
		return 1;
	else
		return 0;
}

static int
check_null_character(char *ptr, size_t len)
{
	while(len > 0){
		if(*ptr == (char)0)
			return 1;
		ptr++;
		len--;
	}
	return 0;
}

static int
check_ascii_string(char *ptr, size_t len, size_t *num)
{
	unsigned char	*uc = (unsigned char *)ptr;
	*num = 0;

	while(*num < len){
		if((*uc < 0x20) || (*uc > 0x7f)){
			if(!is_printable_ascii(*uc))
				return 0;
		}
		uc++;
		(*num)++;
	}
	return 1;
}

static int
check_7bit_string(char *ptr, size_t len, size_t *num)
{
	unsigned char	*uc = (unsigned char *)ptr;
	*num = 0;

	while(*num < len){
		if(*uc & 0x80)
			return 0;
		uc++;
		(*num)++;
	}
	return 1;
}

static int
check_iso9496_string(char *ptr, size_t len, size_t *num)
{
	unsigned char	*uc = (unsigned char *)ptr;
	*num = 0;

	while(*num < len){
		if(*uc < 0x20){
			if(!is_printable_ascii(*uc))
				return 0;
		}
		if((*uc >= 0x7f) && (*uc < 0xa0))
			return 0;
		uc++;
		(*num)++;
	}
	return 1;
}

static int
check_iso94Ext_string(char *ptr, size_t len, size_t *num)
{
	unsigned char	*uc = (unsigned char *)ptr;
	*num = 0;

	while(*num < len){
		if(*uc < 0x20){
			if(!is_printable_ascii(*uc))
				return 0;
		}
		uc++;
		(*num)++;
	}
	return 1;
}

static int
check_multibyte_string(char *ptr, size_t len, size_t *num)
{
	int	c_len;

	*num = 0;
	while(*num < len){
		c_len = mblen(ptr, len - *num);
		if(c_len <= 0)
			return 0;
		else{
			ptr += c_len;
			*num -= c_len;
		}
	}
	return 1;
}

#ifdef AAA
static char *
guess_iso8859_format(LCTd lctd, char *ptr, size_t len)
{
	LclCharsetLinkedList	*display_list, *outgoing_list;

	display_list = _lct_get_default_string_linkedlist_attribute_with_type(lctd, LctNTaggedText, LctNDisplayForm, LctNBodyCharsetList);
	if(display_list == (LclCharsetLinkedList *)NULL)
		return (char *)NULL;

	outgoing_list = _lct_get_default_string_linkedlist_attribute_with_type(lctd, LctNTaggedText, LctNOutGoingStreamForm, LctNBodyCharsetList);
	if(outgoing_list == (LclCharsetLinkedList *)NULL)
		return (char *)NULL;

	if(strcmp(display_list->name, outgoing_list->name))
		return (char *)NULL;

	if(_lct_get_iso8859_format_info(lctd, display_list->name) == (char *)NULL)
		return (char *)NULL;

	if(iso8859_format(ptr, len))
		return display_list->name;
	else
		return (char *)NULL;
}
	
static int
check_convert(LclIconvInfo *iconv_info, char *ptr, size_t len, int reverse_flag)
{
	iconv_t	i_conv;
	size_t	ret;
	char	*from_buf, *to_buf;
	size_t	from_len, to_len;
	char	*from_name, *to_name;
	char	*from_ptr, *to_ptr;
	size_t	from_bytes, to_bytes;

	if(reverse_flag){
		from_name = iconv_info->atom[iconv_info->atom_num - 1].to;
		to_name = iconv_info->atom[iconv_info->atom_num - 1].from;
	}
	else{
		from_name = iconv_info->atom[0].from;
		to_name = iconv_info->atom[0].to;
	}
	i_conv = iconv_open(to_name, from_name);
	if(i_conv == (iconv_t)-1)
		return -1;

	from_buf = ptr, from_len = len;
	to_len = len;
	while(1){
		to_buf = (char *)malloc(to_len);
		if(to_buf == (char *)NULL){
			iconv_close(i_conv);
			return -1;
		}
		from_ptr = from_buf, from_bytes = from_len;
		to_ptr = to_buf, to_bytes = to_len;
		ret = iconv(i_conv, (const char **)(&from_ptr), &from_bytes, &to_ptr, &to_bytes);
		free(to_buf);
		if((ret == -1) && (errno == E2BIG)){
			to_len = to_len * 2;
			continue;
		}
		else
			break;
	}
	iconv_close(i_conv);

	if(from_bytes == 0)
		return 1;
	else
		return -1;
}

static char *
guess_from_iconv(LCTd lctd, char *type, char *form, char *attr, char *ptr, size_t len)
{
	LclCharsetLinkedList	*target_charset;
	LclIconvInfo	*iconv_info;
	char	*charset = (char *)NULL;

	if(lctd->lcld->iconv_info == (LclIconvInfo *)NULL)
		return (char *)NULL;

	target_charset = _lct_get_default_string_linkedlist_attribute_with_type(lctd, type, form, attr);
	if(target_charset == (LclCharsetLinkedList *)NULL)
		return (char *)NULL;

	while(target_charset){
		iconv_info = lctd->lcld->iconv_info;
		while(iconv_info){
			/* check from unknown charset to target charset */
			if(!strcmp(iconv_info->to_encoding, target_charset->name)){
				if(check_convert(iconv_info, ptr, len, 0) > 0){
					charset = iconv_info->from_encoding;
					return charset;
				}
			}
			else if((iconv_info->direction == LclTypeBothway) && !strcmp(iconv_info->from_encoding, target_charset->name)){
				if(check_convert(iconv_info, ptr, len, 1) > 0){
					charset = iconv_info->to_encoding;
					return charset;
				}
			}

			/* check from target charset to unknown charset */
			if(!strcmp(iconv_info->from_encoding, target_charset->name)){
				if(check_convert(iconv_info, ptr, len, 0) > 0){
					charset = target_charset->name;
					return charset;
				}
			}
			else if((iconv_info->direction == LclTypeBothway) && !strcmp(iconv_info->to_encoding, target_charset->name)){
				if(check_convert(iconv_info, ptr, len, 1) > 0){
					charset = target_charset->name;
					return charset;
				}
			}

			iconv_info = iconv_info->next;
		}
	}

	return charset;
}
#endif


static LclContentInfo *
make_content_info(LCTd lctd, LctNEAttribute form, LclContentType type, char *charset)
{
	LclCharsetLinkedList *list;
	LclContentInfo	*info;

	info = (LclContentInfo *)malloc(sizeof(LclContentInfo));
	if(info == (LclContentInfo *)NULL)
		return (LclContentInfo *)NULL;

	info->type = type;
	info->charset_list = (LclCharsetList *)NULL;

	if(charset){
		list = _lcl_create_one_charset_linked_list(charset);
		if(list){
			info->charset_list = _lct_create_context_charset_list(lctd, form, list);
			_lcl_destroy_charset_linked_list(list);
		}
	}

	return info;
}
	
LclContentInfo *
_lct_get_query_content_type(LCTd lctd, LctNEAttribute form)
{
	char	*ptr = lctd->contents->buf;
	size_t	len = lctd->contents->buf_len;
	char	*charset;
	size_t	num;

	if((ptr == (char *)NULL) || (len <= 0))
		return make_content_info(lctd, form, LclContentUnknown, (char *)NULL);

	/* check null character */
	if(check_null_character(ptr, len))
		return make_content_info(lctd, form, LclContentBinary, (char *)NULL);
	
	/* check ascii */
	if(check_ascii_string(ptr, len, &num)){
		charset = _lct_get_charsetname_from_format(lctd, LclCsFormatASCII);
		if(charset)
			return  make_content_info(lctd, form, LclContentText, charset);
	}

	/* check 7bit */
	charset = _lct_get_charsetname_from_format(lctd, LclCsFormat7bit);
	if(charset){
		if(check_7bit_string(ptr, len, &num))
			return  make_content_info(lctd, form, LclContentText, charset);
	}

	/* check to see if correct Multibyte string format */
	charset = _lct_get_charsetname_from_format(lctd, LclCsFormatMBString);
	if(charset){
		if(check_multibyte_string(ptr, len, &num))
			return  make_content_info(lctd, form, LclContentText, charset);
	}

	/* check ISO 94/96 */
	charset = _lct_get_charsetname_from_format(lctd, LclCsFormatISO9496);
	if(charset){
		if(check_iso9496_string(ptr, len, &num))
			return  make_content_info(lctd, form, LclContentText, charset);
	}

	/* check ISO 94 + Ext */
	charset = _lct_get_charsetname_from_format(lctd, LclCsFormatISO94Ext);
	if(charset){
		if(check_iso94Ext_string(ptr, len, &num))
			return  make_content_info(lctd, form, LclContentText, charset);
	}

#ifdef AAA
	/* check iconv */
	charset = guess_from_iconv(lctd, LctNTaggedText, LctNOutGoingStreamForm, LctNBodyCharsetList, ptr, len);
	if(charset)
		return  make_content_info(lctd, form, LclContentText, charset);

	/* check iso8859-x format */
	charset = guess_iso8859_format(lctd, ptr, len);
	if(charset)
		return  make_content_info(lctd, form, LclContentText, charset);
#endif

	/* unknown */
	return make_content_info(lctd, form, LclContentUnknown, (char *)NULL);
}

/* check if the content is in the range
	return	-1	unknown
		0	false
		1	true
*/
int
_lct_check_content_from_charset(LCTd lctd, char *ptr, size_t len, size_t *num, char *charset)
{
	LclFormat	format;

	if((ptr == (char *)NULL) || (charset == (char *)NULL))
		return -1;

	format = _lct_get_format_from_charsetname(lctd, charset);

	if(format == LclCsFormatASCII)
		return check_ascii_string(ptr, len, num);
	else if(format == LclCsFormat7bit)
		return (check_7bit_string(ptr, len, num));
	else if(format == LclCsFormatMBString)
		return (check_multibyte_string(ptr, len, num));
	else if(format == LclCsFormatISO9496)
		return (check_iso9496_string(ptr, len, num));
	else if(format == LclCsFormatISO94Ext)
		return (check_iso94Ext_string(ptr, len, num));
	else
		return -1;
}

void
lcl_destroy_content_info(LclContentInfo *info)
{
	if(info){
		if(info->charset_list)
			lcl_destroy_charset_list(info->charset_list);
		free(info);
	}
}

