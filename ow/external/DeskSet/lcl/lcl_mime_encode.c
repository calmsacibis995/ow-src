#include <stdio.h>
#include <locale.h>

#include "lcl.h"
#include "lcl_internal.h"


/********** Define of line length and CRLF **********/

/* line length without CRLF */
/* refered throuth extern in lcl_mime_encode.h */
const int	LclMaxHeaderLineLen = 75;

static char	LclCRLFString[] = "\n";
static int	LclCRLFStringLength = 1;

/* use LclSoftReturnStringList[0] when insersion */
/* refered throuth extern in lcl_mime_encode.h */
char	*LclSoftReturnStringList[] = {
	"\n ",
	"\n\t",
	(char *)NULL
};
int LclSoftReturn0StringLength = 2;

/* refered through extern in lcl_mime_encode.h */
const int	LclAfterSoftReturnPos = 1;



/********** mime header parsing **********/

static char	*NotTextFieldStrList[] = {
	"Return-path",
	"Received",
	"Reply-To",
	"From",
	"Sender",
	"Resent-Reply-To",
	"Resent-From",
	"Resent-Sender",
	"Date",
	"Resent-Date",
	"To",
	"Resent-To",
	"cc",
	"Resent-cc",
	"bcc",
	"Resent-bcc",
	"Message-ID",
	"Resent-Message-ID",
	"In-Reply-To",
	"References",
	"Keywords",
	"Encrypted",
	"Content-Type",
	"Content-Transfer-Encoding",
	"Content-MD5",
	(char *)NULL
};

static char	*DelimiterStrListFieldname[] = {
	": ",
	":",
	(char *)NULL
};

static char	*StrListEncodeBegin[] = {
	"=?",
	(char *)NULL
};

/* \n should be substituted if LF/CR is valid */
static char	*StrListEncodeEnd[] = {
	" ",
	"\t",
	"\n",
	(char *)NULL
};

static char	*StrListRemoveWhenEncodeEnd[] = {
/* according to RFC1522, space should not be removed
	" ",
*/
	"\n ",
	"\n\t",
	(char *)NULL
};

typedef struct _FieldList FieldList;
static struct _FieldList{
	char	*field;
	int	length;
	FieldList	*next;
};


/* general functions */

int
_lcl_next_token(char **p, int *len, char **token, int *token_len, char **delimiter_str)
{
	*token = (char *)NULL;
	*token_len = 0;

	while(*len > 0){
		int	delimiter_len;
		delimiter_len = _lcl_strncmp_with_string_list(*p, *len, delimiter_str, 0);
		if(delimiter_len > 0){
			if(*token){
				*token_len = *p - *token;
				return 0;
			}
			else{
				*token = *p;
				*token_len = delimiter_len;
				*len -= delimiter_len;
				*p += delimiter_len;
				return 1;
			}
		}
		else{
			if(*token == (char *)NULL)
				*token = *p;
			(*len)--;
			(*p)++;
			if(*len == 0){
				*token_len = *p - *token;
				return 0;
			}
		}
	}
	return -1;
}

int
_lcl_strncmp_with_string_list(char *ptr, int len, char **str_list, int case_flag)
{
        while(*str_list){
                int cmp_len = strlen(*str_list);
                if(len >= cmp_len){
                        if(case_flag){
                                if(!strncasecmp(ptr, *str_list,cmp_len))
                                        return cmp_len;
                        }
                        else{
                                if(!strncmp(ptr, *str_list, cmp_len))
                                        return cmp_len;
                        }
                }
                str_list++;
        }
        return 0;
}


/* encode */

static int
encode_mail_mode(LclBuffer *buf, char *ptr, int len, int *pos, LclMailEncoding encoding, char *charset)
{
	switch(encoding){
		case LclQPEncoding:
			return _lcl_q_encode_header_line(buf, ptr, len, charset);
			break;
		case LclBase64Encoding:
			return _lcl_b_encode_header_line(buf, ptr, len, charset);
		default:
			return -1; 
			break;
	}
}

static int
encode_text_mode(LclBuffer *buf, char *ptr, int len, int *pos, LclMailEncoding encoding, char *charset)
{
	switch(encoding){
		case LclQPEncoding:
			return _lcl_q_encode_text(buf, ptr, len, pos, charset);
			break;
		case LclBase64Encoding:
			return _lcl_b_encode_text(buf, ptr, len, pos, charset);
		default:
			return -1; 
			break;
	}
}

static char *
encode_one_line(char *header, int length, LclMailEncoding encoding, char *charset)
{
	char	*ptr;
	int	len;
	int	ret;
	LclBuffer	*buf;
	char	*encoded_buf;
	int	pos;
	char	*field_name, *delimiter;
	int	field_name_len, delimiter_len;


	buf = _LclBuffer_create(length * 4);
	if(buf == (LclBuffer *)NULL)
		return (char *)NULL;

	ptr = header;
	len = length;
	pos = 0;

	/* check the first token */
	if(_lcl_next_token(&ptr, &len, &field_name, &field_name_len, DelimiterStrListFieldname) == 0){
		if(_lcl_next_token(&ptr, &len, &delimiter, &delimiter_len, DelimiterStrListFieldname) > 0){
			if(_LclBuffer_add(buf, field_name, field_name_len) != 0){
				_LclBuffer_destroy(buf);
				return (char *)NULL;
			}
			pos += field_name_len;
			if(_LclBuffer_add(buf, delimiter, delimiter_len) != 0){
				_LclBuffer_destroy(buf);
				return (char *)NULL;
			}
			pos += delimiter_len;

			/* if this field is not "text" */
			if(_lcl_strncmp_with_string_list(field_name, field_name_len, NotTextFieldStrList, 1))
				ret = encode_mail_mode(buf, ptr, len, &pos, encoding, charset);
			/* this field is "text" */
			else
				ret = encode_text_mode(buf, ptr, len, &pos, encoding, charset);
		}
		else
			ret = encode_text_mode(buf, header, length, &pos, encoding, charset);
	}
	else
		ret = encode_text_mode(buf, header, length, &pos, encoding, charset);

	if(ret){
		_LclBuffer_destroy(buf);
		return (char *)NULL;
	}
	else{
		encoded_buf = _LclBuffer_get_string(buf);
		_LclBuffer_destroy(buf);
		return encoded_buf;
	}
}


static int
get_next_field(char **p, int *len, char **field)
{
	LclBuffer	*buf;

	*field = (char *)NULL;

	buf = _LclBuffer_create(256);
	if(buf == (LclBuffer *)NULL)
		return -1;

	while(*len > 0){
		int comp_len = _lcl_strncmp_with_string_list(*p, *len, LclSoftReturnStringList, 0);
		if(comp_len){
			*p += comp_len;
			*len -= comp_len;
		}
		else if((*len >= LclCRLFStringLength) && !strncmp((*p), LclCRLFString, LclCRLFStringLength)){
			*p += LclCRLFStringLength;
			*len -= LclCRLFStringLength;
			break;
		}
		else{
			if(_LclBuffer_add(buf, *p, 1) != 0){
				_LclBuffer_destroy(buf);
				return -1;
			}
			(*p)++;
			(*len)--;
		}
	}

	*field = _LclBuffer_get_string(buf);
	_LclBuffer_destroy(buf);

	return 0;
}

static int
add_new_field(FieldList **list, char *new_field, int new_field_len)
{
	FieldList	*new_list, *ptr, *prev_ptr;

	new_list = (FieldList *)malloc(sizeof(struct _FieldList));
	if(new_list == (FieldList *)NULL)
		return -1;
	new_list->field = new_field;
	new_list->length = new_field_len;
	new_list->next = (FieldList *)NULL;

	if(*list == (FieldList *)NULL){
		*list = new_list;
		return 0;
	}
	else{
		prev_ptr = *list;
		ptr = (*list)->next;
		while(ptr){
			prev_ptr = ptr;
			ptr = ptr->next;
		}
		prev_ptr->next = new_list;
		return 0;
	}
}

static void
destroy_field_list(FieldList *list)
{
	FieldList	*ptr, *prev_ptr;

	ptr = list;
	while(ptr){
		prev_ptr = ptr;
		ptr = ptr->next;
		if(prev_ptr->field)
			free(prev_ptr->field);
		free(prev_ptr);
	}
}

static char *
make_new_header(FieldList *list)
{
	FieldList	*list_ptr;
	int	total_len;
	char	*new_header, *ptr;

	total_len = 0;

	list_ptr = list;
	while(list_ptr){
		total_len += list_ptr->length + LclCRLFStringLength;
		list_ptr = list_ptr->next;
	}

	new_header = (char *)malloc(total_len + 1);
	if(new_header == (char *)NULL)
		return (char *)NULL;

	ptr = new_header;
	list_ptr = list;
	while(list_ptr){
		memcpy(ptr, list_ptr->field, list_ptr->length);
		ptr += list_ptr->length;
		memcpy(ptr, LclCRLFString, LclCRLFStringLength);
		ptr += LclCRLFStringLength;
		list_ptr = list_ptr->next;
	}
	*ptr = (char)0;

	return new_header;
}
		
char *
_lcl_mime_encode_header(char *header, int header_len, LclMailEncoding encoding, char *charset)
{
	FieldList	*field_list;
	char	*new_header;

	field_list = (FieldList *)NULL;
	while(1){
		char	*field, *new_field;

		if(get_next_field(&header, &header_len, &field) != 0){
			destroy_field_list(field_list);
			return (char *)NULL;
		}
		if(field == (char *)NULL)
			break;

		new_field = encode_one_line(field, strlen(field), encoding, charset);
		free(field);

		if(new_field){
			if(add_new_field(&field_list, new_field, strlen(new_field))){
				destroy_field_list(field_list);
				return (char *)NULL;
			}
		}
		else{
			destroy_field_list(field_list);
			return (char *)NULL;
		}
	}

	new_header = make_new_header(field_list);
	destroy_field_list(field_list);

	return new_header;
}


/* decode */

static int
encode_check(char *ptr, int len)
{
	char	*org_ptr = ptr;
	int	question_num = 0;

	if(_lcl_strncmp_with_string_list(ptr, len, StrListEncodeBegin, 0) <= 0)
		return 0;

	ptr += 2;
	len -= 2;
	while(len){
		if(*ptr == '?')
			question_num++;
		ptr++;
		len--;
		if(question_num >= 3)
			break;
	}
	if(len <= 0)
		return 0;

	if(*ptr != '=')
		return 0;
	ptr++;
	len--;

/* According to RFC1342, encoded-word should be followed by SPACE or newline.
   But RFC1522 obsoletes this condition.
	if(len > 0){
		if(_lcl_strncmp_with_string_list(ptr, len, StrListEncodeEnd, 0) <= 0)
			return 0;
	}
*/
	return ptr - org_ptr;
}

static int
get_next_decode_segment(char **ptr, int *len, char **token, int *token_len)
{
	int	encode_len;
	int	comp_len;

	*token = (char *)NULL; 
	while(*len > 0){
		encode_len = encode_check(*ptr, *len);
		if(encode_len > 0){
			if(*token == (char *)NULL){
				*token = *ptr;
				*token_len = encode_len;
				*ptr += encode_len;
				*len -= encode_len;
				return 1;
			}
			else{
				*token_len = *ptr - *token;
				return 0;
			}
		}
/*
		comp_len = _lcl_strncmp_with_string_list(*ptr, *len, LclSoftReturnStringList, 0);
		if(comp_len > 0){
			if(*token == (char *)NULL){
				*token = *ptr;
				*token_len = comp_len;
				*ptr += comp_len;
				*len -= comp_len;
				return 0;
			}
			else{
				*token_len = *ptr - *token;
				return 0;
			}	
		}
*/
		if(*token == (char *)NULL)
			*token = *ptr;
		(*ptr)++;
		(*len)--;
	}
	if(*token == (char *)NULL)
		return -1;

	*token_len = *ptr - *token;
	return 0;
}

static int
get_next_decode_token(char **ptr, int *len, char **token, int *token_len)
{
	if((*len > 0) && (**ptr == '?')){
		(*ptr)++;
		(*len)--;
	}

	*token = *ptr;
	while(*len > 0){
		if(**ptr == '?'){
			*token_len = *ptr - *token;
			return 1;
		}
		else{
			(*ptr)++;
			(*len)--;
		}
	}
	return -1;
}

static int
decode_segment(LclBuffer *buf, char *ptr, int len, char **charset)
{
	LclMailEncoding	encoding;
	char	*token;
	int	token_len;
	
	*charset == (char *)NULL;

	if((len < 1) || (*ptr != '='))
		return -1;
	ptr++;
	len--;

	if((len < 1) || (*ptr != '?'))
		return -1;
	ptr++;
	len--;

	if (get_next_decode_token(&ptr, &len, &token, &token_len) > 0){
		*charset = (char *)malloc(token_len + 1);
		if(*charset == (char *)NULL)
			return -1;
		memcpy(*charset, token, token_len);
		(*charset)[token_len] = (char)0;
	}
	else
		return -1;
	if (get_next_decode_token(&ptr, &len, &token, &token_len) > 0){
		if((token[0] == 'Q') || (token[0] == 'q'))
			encoding = LclQPEncoding;
		else if((token[0] == 'B') || (token[0] == 'b'))
			encoding = LclBase64Encoding;
		else
			goto error_return;
	}
	else
		goto error_return;

	if (get_next_decode_token(&ptr, &len, &token, &token_len) > 0){
		switch(encoding){
			case LclQPEncoding:
				if(_lcl_add_qp_decode(buf, token, token_len) != 0)
					goto error_return;
				break;
			case LclBase64Encoding:
				if(_lcl_add_b64_decode(buf, token, token_len) != 0)
					goto error_return;
				break;
			default:
				goto error_return;
				break;
		}
	}
	else
		goto error_return;

	return 0;

error_return:
	if(*charset){
		free(*charset);
		*charset = (char *)NULL;
	}
	return -1;
}

static int
check_linear_white_space(char *ptr, int len)
{
	while(len > 0){
		if((len >= LclCRLFStringLength) && !strncmp(ptr, LclCRLFString, LclCRLFStringLength)){
			ptr += LclCRLFStringLength;
			len -= LclCRLFStringLength;
		}
		else if((*ptr == ' ') || (*ptr == '\t')){
			ptr++;
			len--;
		}
		else
			return 0;
	}
	return 1;
}

char *
_lcl_mime_decode_header(char *header, int length, char **charset)
{
	LclBuffer	*buf;
	char	*ptr;
	int	len;
	int	ret;
	int	comp_len;
	char	*decode_buf;
	char	*segment_charset;

	*charset = (char *)NULL;

	buf = _LclBuffer_create(length);
	if(buf == (LclBuffer *)NULL)
		return (char *)NULL;

	while(length > 0){
		ret = get_next_decode_segment(&header, &length, &ptr, &len);
		if(ret > 0){
			char	*segment_charset = (char *)NULL;
			if (decode_segment(buf, ptr, len, &segment_charset) != 0){
				if(_LclBuffer_add(buf, ptr, len) != 0){
					_LclBuffer_destroy(buf);
					return (char *)NULL;
				}
			}
			if(segment_charset){
				if(*charset)
					free(*charset);
				*charset = segment_charset;
			}
/*
			comp_len = _lcl_strncmp_with_string_list(header, length, StrListRemoveWhenEncodeEnd, 0);
			if(comp_len > 0){
				header += comp_len;
				length -= comp_len;
			}
*/
		}
		if(ret == 0){
			if(!check_linear_white_space(ptr, len)){
				if(_LclBuffer_add(buf, ptr, len) != 0){
					_LclBuffer_destroy(buf);
					return (char *)NULL;
				}
			}
		}
		if(ret < 0)
			break;
	}
	decode_buf = _LclBuffer_get_string(buf);
	_LclBuffer_destroy(buf);
	return decode_buf;
}


/********** not used in this release **********/

#ifdef AAA
static char	*AddressFieldStrList[] = {
	"Return-path",
	"Reply-To",
	"From",
	"Sender",
	"Resent-Reply-To",
	"Resent-From",
	"Resent-Sender",
	"To",
	"Resent-To",
	"cc",
	"Resent-cc",
	"bcc",
	"Resent-bcc",
	(char *)NULL
};
#endif

#ifdef AAA
static char	*DelimiterStrListText[] = {
	"\n",
	(char *)NULL
};
#endif

#ifdef AAA
static char	*DelimiterStrListAddress[] = {
	"(",
	")",
	",",
	"\"",
	"[",
	"]",
	(char *)NULL
};
#endif

#ifdef AAA
static char	*DelimiterStrListRootAddress[] = {
	"(",
	")",
	"\"",
	"<",
	(char *)NULL
};
#endif


#ifdef AAA
/* if add ",", although addr-spec is not still be folded, */
/*   address field may be foled before ",". */
/*   That is not recommended in RFC822. */
static char	*DelimiterStrListComment[] = {
	"(",
	")",
	" ",
	"\t",
/*	",",	*/
	(char *)NULL
};
#endif

#ifdef AAA
static char	*DelimiterStrListLeftSquareBracket[] = {
	"[",
	(char *)NULL
};
#endif

#ifdef AAA
static char	*DelimiterStrListRightSquareBracket[] = {
	"]",
	(char *)NULL
};
#endif

#ifdef AAA
static char	*DelimiterStrListLeftAngleBracket[] = {
	"<",
	(char *)NULL
};
#endif

#ifdef AAA
static char	*DelimiterStrListComma[] = {
	",",
	(char *)NULL
};
#endif

#ifdef AAA
static char	*QuotedPrintableNonEncodeStrList[] = {
	" ",
	"\t",
	"\n",
	(char *)NULL
};
#endif

#ifdef AAA
static char	*DelimiterStrListFolding[] = {
	" ",
	"\t",
	"\n",
	(char *)NULL
};
#endif

#ifdef AAA
typedef enum {FoldAlwaysNo, FoldNo, FoldAtMost, FoldAnywhere} FoldMode;
#endif


#ifdef AAA
static int
add_qencode_with_folding(LclBuffer *buf, char *in, int in_len, int *pos, char *charset, FoldMode fold_mode)
{
	char	str_buf[3];
	int	buf_len, qp_len;
	int	seq_len;
	char	*seq_ptr, *qp_ptr;
	int	space, space_org;

	if((in == (char *)NULL) || (in_len <= 0) || (charset == (char *)NULL))
		return -1;

	seq_len = 2 + strlen(charset) + 3 + 2;

	while(in_len){
		if((fold_mode == FoldAtMost) || (fold_mode == FoldAnywhere)){
			if(LclMaxHeaderLineLen - *pos <= seq_len){
				_LclBuffer_add(buf, LclSoftReturnStringList[0], LclSoftReturn0StringLength);
				*pos = LclAfterSoftReturnPos;
			}
		}

		str_buf[0] = '=';
		str_buf[1] = '?';
		if(_LclBuffer_add(buf, str_buf, 2) != 0)
			return -1;

		if(_LclBuffer_add(buf, charset, strlen(charset)) != 0)
			return -1;

		str_buf[0] = '?';
		str_buf[1] = 'Q';
		str_buf[2] = '?';
		if(_LclBuffer_add(buf, str_buf, 3) != 0)
			return -1;

		if((fold_mode == FoldAtMost) || (fold_mode == FoldAnywhere))
			space = LclMaxHeaderLineLen - *pos - seq_len;
		else
			space = in_len * 4;
		space_org = space;

		if(qp_encode_in_space(buf, &in, &in_len, &space) < 0)
			return -1;

		str_buf[0] = '?';
		str_buf[1] = '=';
		if(_LclBuffer_add(buf, str_buf, 2) != 0)
			return -1;

		*pos += space_org - space + seq_len;
	}
	return 0;
}
#endif

#ifdef AAA
static void
print_token(char *token, int token_len)
{
	char	*buf, *qp_buf;

	buf = (char *)malloc(token_len + 1);
	memcpy(buf, token, token_len);
	buf[token_len] = (char)0;
	fprintf(stdout, "token = %s\n", buf);
	free(buf);
}
#endif

#ifdef AAA
static void
print_field(char *field, int field_len)
{
	char	*buf, *qp_buf;

	buf = (char *)malloc(field_len + 1);
	memcpy(buf, field, field_len);
	buf[field_len] = (char)0;
	fprintf(stdout, "field = %s\n", buf);
	free(buf);
}
#endif

#ifdef AAA
int
add_element_with_folding(LclBuffer *buf, char *ptr, int len, int *pos, FoldMode fold_mode)
{
	char	*org_ptr, *token, *line_ptr;
	int 	org_len, token_len, line_len;

	if((fold_mode != FoldAtMost) && (fold_mode != FoldAnywhere)){
		if(_LclBuffer_add(buf, ptr, len) != 0)
			return -1;
		*pos += len;
		return 0;
	}

	while(len > 0){
		if((*pos > 0) && (*pos + len > LclMaxHeaderLineLen)){
			if(_LclBuffer_add(buf, LclSoftReturnStringList[0], LclSoftReturn0StringLength) != 0)
				return -1;
			*pos = LclAfterSoftReturnPos;
		}
		if((fold_mode == FoldAnywhere) && (*pos + len > LclMaxHeaderLineLen))
			line_len = LclMaxHeaderLineLen - 1;
		else
			line_len = len;
		if(_LclBuffer_add(buf, ptr, line_len) != 0)
			return -1;
		ptr += line_len;
		len -= line_len;
		*pos +=line_len ;
	}
	return 0;
}
#endif
		
#ifdef AAA
int
add_text_with_folding(LclBuffer *buf, char *ptr, int len, int *pos, FoldMode fold_mode)
{
	char	*org_ptr, *token, *line_ptr;
	int 	org_len, token_len, line_len;

	if((fold_mode != FoldAtMost) && (fold_mode != FoldAnywhere)){
		if(_LclBuffer_add(buf, ptr, len) != 0)
			return -1;
		*pos += len;
		return 0;
	}

	while(len){
		line_ptr = ptr;
		if(*pos + len > LclMaxHeaderLineLen){
			line_len = 0;
			while(1){
				org_ptr = ptr;
				org_len = len;
				if(next_token(&ptr, &len, &token, &token_len, DelimiterStrListFolding) >= 0){
					if(*pos + line_len + token_len > LclMaxHeaderLineLen){
						if((line_len == 0) && (*pos <= 1)){
							token_len = LclMaxHeaderLineLen - 1;
							ptr = org_ptr + token_len;
							len = org_len - token_len;
							line_len = token_len;
							break;
						}
						else{
							ptr = org_ptr;
							len = org_len;
							break;
						}
					}
					else{
						line_len += token_len;
					}
				}
				else
					break;
			}
		}
		else{
			line_len = len;
			ptr += len;
			len = 0;
		}

		if(line_len == 0){
			if(_LclBuffer_add(buf, LclSoftReturnStringList[0], LclSoftReturn0StringLength) != 0)
				return -1;
			*pos = LclAfterSoftReturnPos;
		}
		else{
			if(_LclBuffer_add(buf, line_ptr, line_len) != 0)
				return -1;
			*pos += line_len;
		}
	}
	return 0;
}
#endif
		
#ifdef AAA
static int
encode_text(LclBuffer *buf, char *ptr, int len, int *pos, char *charset, FoldMode fold_mode)
{
	char	*token;
	int	token_len;
	int	ret, add_ret;
	ParseMode	parse_mode;
	int	comment_nest;

	parse_mode = NormalMode;
	comment_nest = 0;

	while(1){
		ret = next_token(&ptr, &len, &token, &token_len, DelimiterStrListText);
		if(ret > 0){
#ifdef Folding
			add_ret = add_text_with_folding(buf, token, token_len, pos, FoldAnywhere);
#else
			add_ret = add_text_with_folding(buf, token, token_len, pos, FoldAlwaysNo);
#endif
			if(add_ret){
				_LclBuffer_destroy(buf);
				return -1;
			}
		}
		if(ret == 0){
			if(need_quoted_printable(token, token_len)){
#ifdef Folding
				add_ret = add_qencode_with_folding(buf, token, token_len, pos, charset, FoldAnywhere);
#else
				add_ret = add_qencode_with_folding(buf, token, token_len, pos, charset, FoldAlwaysNo);
#endif
				if(add_ret){
					_LclBuffer_destroy(buf);
					return -1;
				}
			}
			else{
#ifdef Folding
				add_ret = add_text_with_folding(buf, token, token_len, pos, FoldAnywhere);
#else
				add_ret = add_text_with_folding(buf, token, token_len, pos, FoldAlwaysNo);
#endif
				if(add_ret){
					_LclBuffer_destroy(buf);
					return -1;
				}
			}
		}
		if(ret < 0)
			return 0;
	}
}
#endif

#ifdef AAA
static int
encode_with_separator(LclBuffer *buf, char *ptr, int len, int *pos, char *charset, FoldMode fold_mode)
{
	char	*token;
	int	token_len;
	int	ret, add_ret;
	ParseMode	parse_mode;
	int	comment_nest;

	parse_mode = NormalMode;
	comment_nest = 0;

	while(1){
		switch(parse_mode){
		    case NormalMode:
			ret = next_token(&ptr, &len, &token, &token_len, DelimiterStrListMail);
			if(ret > 0){
				if(strncmp_with_string_list(token, token_len, DelimiterStrListQuotation, 0))
					parse_mode = QuotedMode;
				else if(strncmp_with_string_list(token, token_len, DelimiterStrListLeftParenthesis, 0)){
					parse_mode = CommentMode;
					comment_nest = 1;
				}
			}
			break;
		    case QuotedMode:
			ret = next_token(&ptr, &len, &token, &token_len, DelimiterStrListQuotation);
			if(ret > 0){
				if(strncmp_with_string_list(token, token_len, DelimiterStrListQuotation, 0))
					parse_mode = NormalMode;
			}
			break;
		    case CommentMode:
			ret = next_token(&ptr, &len, &token, &token_len, DelimiterStrListParentheses);
			if(ret > 0){
				if(strncmp_with_string_list(token, token_len, DelimiterStrListLeftParenthesis, 0))
					comment_nest++;
				else if(strncmp_with_string_list(token, token_len, DelimiterStrListRightParenthesis, 0)){
					comment_nest--;
					if(comment_nest <= 0)
						parse_mode = NormalMode;
				}
			}
			break;
		    default:
			ret = next_token(&ptr, &len, &token, &token_len, DelimiterStrListMail);
			break;
		}

		if(ret > 0){
#ifdef Folding
			add_ret = add_element_with_folding(buf, token, token_len, pos, FoldAtMost);
#else
			add_ret = add_element_with_folding(buf, token, token_len, pos, FoldAlwaysNo);
#endif
			if(add_ret){
				_LclBuffer_destroy(buf);
				return -1;
			}
		}
		if(ret == 0){
			if(need_quoted_printable(token, token_len)){
#ifdef Folding
				add_ret = add_qencode_with_folding(buf, token, token_len, pos, charset, FoldAtMost);
#else
				add_ret = add_qencode_with_folding(buf, token, token_len, pos, charset, FoldAlwaysNo);
#endif
				if(add_ret){
					_LclBuffer_destroy(buf);
					return -1;
				}
			}
			else{
#ifdef Folding
				add_ret = add_element_with_folding(buf, token, token_len, pos, FoldAtMost);
#else
				add_ret = add_element_with_folding(buf, token, token_len, pos, FoldAlwaysNo);
#endif
				if(add_ret){
					_LclBuffer_destroy(buf);
					return -1;
				}
			}
		}
		if(ret < 0)
			return 0;
	}
}
#endif

#ifdef AAA
static char *
encode_header_logical_line(char *header, int length, char *charset)
{
	char	*ptr;
	int	len;
	char	*token;
	int	token_len;
	int	ret;
	LclBuffer	*buf;
	char	*encoded_buf;
	ParseMode	parse_mode;
	int	text_mode;
	int	comment_nest;
	int	pos;
	int	fold_moe;


	buf = _LclBuffer_create(length * 4);
	if(buf == (LclBuffer *)NULL)
		return (char *)NULL;


	text_mode = 0;
	parse_mode = NormalMode;
	comment_nest = 0;

	ptr = header;
	len = length;
	pos = 0;

	/* check the first token */
	if(next_token(&ptr, &len, &token, &token_len, DelimiterStrListFieldname) >= 0){
		/* if this field is not "text" */
		if(strncmp_with_string_list(token, token_len, NotTextFieldStrList, 1)){
			if(_LclBuffer_add(buf, token, token_len) != 0){
				_LclBuffer_destroy(buf);
				return (char *)NULL;
			}
			text_mode = 0;
		}
		/* this field is "text" */
		else{
			/* the first token consists of only ascii */
			/* then this may be field name */
			if(!need_quoted_printable(token, token_len)){
				char	*org_ptr;
				int	org_len;
				if(_LclBuffer_add(buf, token, token_len) != 0){
					_LclBuffer_destroy(buf);
					return (char *)NULL;
				}
				org_ptr = ptr;
				org_len = len;
				/* if delimiter ":" is next, */
				/* it shoud not be encoded */
				if(next_token(&ptr, &len, &token, &token_len, DelimiterStrListFieldname) > 0){
					if(_LclBuffer_add(buf, token, token_len) != 0){
						_LclBuffer_destroy(buf);
						return (char *)NULL;
					}
				}
				/* it turns to be not field name */
				else{
					ptr = org_ptr;
					len = org_len;
				}
			}

			/* the first token contains non-ascii */
			/* then this may not be field name */
			/* all the field should be encoded as "text" */
			else{
				ptr = header;
				len = length;
			}

			text_mode = 1;
		}
	}
	else{
		_LclBuffer_destroy(buf);
		return (char *)NULL;
	}


	while(1){
		if(text_mode)
			ret = next_token(&ptr, &len, &token, &token_len, DelimiterStrListText);
		else{
			switch(parse_mode){
			    case NormalMode:
				ret = next_token(&ptr, &len, &token, &token_len, DelimiterStrListMail);
				if(ret > 0){
					if(strncmp_with_string_list(token, token_len, DelimiterStrListQuotation, 0))
						parse_mode = QuotedMode;
					else if(strncmp_with_string_list(token, token_len, DelimiterStrListLeftParenthesis, 0)){
						parse_mode = CommentMode;
						comment_nest = 1;
					}
				}
				break;
			    case QuotedMode:
				ret = next_token(&ptr, &len, &token, &token_len, DelimiterStrListQuotation);
				if(ret > 0){
					if(strncmp_with_string_list(token, token_len, DelimiterStrListQuotation, 0))
						parse_mode = NormalMode;
				}
				break;
			    case CommentMode:
				ret = next_token(&ptr, &len, &token, &token_len, DelimiterStrListParentheses);
				if(ret > 0){
					if(strncmp_with_string_list(token, token_len, DelimiterStrListLeftParenthesis, 0))
						comment_nest++;
					else if(strncmp_with_string_list(token, token_len, DelimiterStrListRightParenthesis, 0)){
						comment_nest--;
						if(comment_nest <= 0)
							parse_mode = NormalMode;
					}
				}
				break;
			    default:
				ret = next_token(&ptr, &len, &token, &token_len, DelimiterStrListMail);
				break;
			}
		}
		if(ret > 0){
			if(_LclBuffer_add(buf, token, token_len) != 0){
				_LclBuffer_destroy(buf);
				return (char *)NULL;
			}
		}
		if(ret == 0){
			if(need_quoted_printable(token, token_len)){
				if(get_q_encode(buf, token, token_len, charset) != 0){
					_LclBuffer_destroy(buf);
					return (char *)NULL;
				}
			}
			else{
				if(_LclBuffer_add(buf, token, token_len) != 0){
					_LclBuffer_destroy(buf);
					return (char *)NULL;
				}
			}
		}
		if(ret < 0)
			break;
	}

	encoded_buf = _LclBuffer_get_string(buf);
	_LclBuffer_destroy(buf);
	return encoded_buf;
}
#endif

#ifdef AAA
static char *
encode_header_line_with_folding(char *header, int length, char *charset)
{
	char	*ptr;
	int	len;
	char	*token;
	int	token_len;
	int	ret;
	LclBuffer	*buf;
	char	*encoded_buf;
	ParseMode	parse_mode;
	int	text_mode;
	int	comment_nest;
	int	pos;
	int	fold_moe;


	buf = _LclBuffer_create(length * 4);
	if(buf == (LclBuffer *)NULL)
		return (char *)NULL;


	text_mode = 0;
	parse_mode = NormalMode;
	comment_nest = 0;

	ptr = header;
	len = length;
	pos = 0;

	/* check the first token */
	if(next_token(&ptr, &len, &token, &token_len, DelimiterStrListFieldname) >= 0){
		/* if this field is  "address" */
		if(strncmp_with_string_list(token, token_len, AddressFieldStrList, 1)){
			if(encode_address_line(buf, ptr, len) < 0){
				_LclBuffer_destroy(buf);
				return (char *)NULL;
			}
			else
				goto return_point;
		}
		/* if this field is not "text" */
		if(strncmp_with_string_list(token, token_len, NotTextFieldStrList, 1)){
			if(add_text_with_folding(buf, token, token_len, &pos) != 0){
				_LclBuffer_destroy(buf);
				return (char *)NULL;
			}
			text_mode = 0;
		}
		/* this field is "text" */
		else{
			/* the first token consists of only ascii */
			/* then this may be field name */
			if(!need_quoted_printable(token, token_len)){
				char	*org_ptr;
				int	org_len;
				if(add_text_with_folding(buf, token, token_len, &pos) != 0){
					_LclBuffer_destroy(buf);
					return (char *)NULL;
				}
				org_ptr = ptr;
				org_len = len;
				/* if delimiter ":" is next, */
				/* it shoud not be encoded */
				if(next_token(&ptr, &len, &token, &token_len, DelimiterStrListFieldname) > 0){
					if(add_text_with_folding(buf, token, token_len, &pos) != 0){
						_LclBuffer_destroy(buf);
						return (char *)NULL;
					}
				}
				/* it turns to be not field name */
				else{
					ptr = org_ptr;
					len = org_len;
				}
			}

			/* the first token contains non-ascii */
			/* then this may not be field name */
			/* all the field should be encoded as "text" */
			else{
				ptr = header;
				len = length;
			}

			text_mode = 1;
		}
	}
	else{
		_LclBuffer_destroy(buf);
		return (char *)NULL;
	}


	while(1){
		if(text_mode)
			ret = next_token(&ptr, &len, &token, &token_len, DelimiterStrListText);
		else{
			switch(parse_mode){
			    case NormalMode:
				ret = next_token(&ptr, &len, &token, &token_len, DelimiterStrListMail);
				if(ret > 0){
					if(strncmp_with_string_list(token, token_len, DelimiterStrListQuotation, 0))
						parse_mode = QuotedMode;
					else if(strncmp_with_string_list(token, token_len, DelimiterStrListLeftParenthesis, 0)){
						parse_mode = CommentMode;
						comment_nest = 1;
					}
				}
				break;
			    case QuotedMode:
				ret = next_token(&ptr, &len, &token, &token_len, DelimiterStrListQuotation);
				if(ret > 0){
					if(strncmp_with_string_list(token, token_len, DelimiterStrListQuotation, 0))
						parse_mode = NormalMode;
				}
				break;
			    case CommentMode:
				ret = next_token(&ptr, &len, &token, &token_len, DelimiterStrListParentheses);
				if(ret > 0){
					if(strncmp_with_string_list(token, token_len, DelimiterStrListLeftParenthesis, 0))
						comment_nest++;
					else if(strncmp_with_string_list(token, token_len, DelimiterStrListRightParenthesis, 0)){
						comment_nest--;
						if(comment_nest <= 0)
							parse_mode = NormalMode;
					}
				}
				break;
			    default:
				ret = next_token(&ptr, &len, &token, &token_len, DelimiterStrListMail);
				break;
			}
		}
		if(ret > 0){
			if(add_text_with_folding(buf, token, token_len, &pos) != 0){
				_LclBuffer_destroy(buf);
				return (char *)NULL;
			}
		}
		if(ret == 0){
			if(need_quoted_printable(token, token_len)){
				if(add_qencode_with_folding(buf, token, token_len, &pos, charset) != 0){
					_LclBuffer_destroy(buf);
					return (char *)NULL;
				}
			}
			else{
				if(add_text_with_folding(buf, token, token_len, &pos) != 0){
					_LclBuffer_destroy(buf);
					return (char *)NULL;
				}
			}
		}
		if(ret < 0)
			break;
	}

return_point:
	encoded_buf = _LclBuffer_get_string(buf);
	_LclBuffer_destroy(buf);
	return encoded_buf;
}
#endif


#ifdef AAA
int
get_next_address_token(char **ptr, int *len, char **addr, int *addr_len)
{
	ParseMode	parse_mode;
	int	comment_nest;
	char	*token;
	int	token_len;
	int	ret;

	parse_mode = NormalMode;
	comment_nest = 0;

	*addr = *ptr;
	*addr_len = 0;

	while(1){
		switch(parse_mode){
		    case NormalMode:
			ret = next_token(ptr, len, &token, &token_len, DelimiterStrListAddress);
			if(ret > 0){
				if(strncmp_with_string_list(token, token_len, DelimiterStrListQuotation, 0))
					parse_mode = QuotedMode;
				else if(strncmp_with_string_list(token, token_len, DelimiterStrListLeftSquareBracket, 0)){
					parse_mode = BracketMode;
				}
				else if(strncmp_with_string_list(token, token_len, DelimiterStrListLeftParenthesis, 0)){
					parse_mode = CommentMode;
					comment_nest = 1;
				}
			}
			break;
		    case QuotedMode:
			ret = next_token(ptr, len, &token, &token_len, DelimiterStrListQuotation);
			if(ret > 0){
				if(strncmp_with_string_list(token, token_len, DelimiterStrListQuotation, 0))
					parse_mode = NormalMode;
			}
			break;
		    case BracketMode:
			ret = next_token(ptr, len, &token, &token_len, DelimiterStrListRightSquareBracket);
			if(ret > 0){
				if(strncmp_with_string_list(token, token_len, DelimiterStrListQuotation, 0))
					parse_mode = NormalMode;
			}
			break;
		    case CommentMode:
			ret = next_token(ptr, len, &token, &token_len, DelimiterStrListParentheses);
			if(ret > 0){
				if(strncmp_with_string_list(token, token_len, DelimiterStrListLeftParenthesis, 0))
					comment_nest++;
				else if(strncmp_with_string_list(token, token_len, DelimiterStrListRightParenthesis, 0)){
					comment_nest--;
					if(comment_nest <= 0)
						parse_mode = NormalMode;
				}
			}
			break;
		    default:
			ret = next_token(ptr, len, &token, &token_len, DelimiterStrListMail);
			break;
		}
		
		if((ret > 0) && (parse_mode == NormalMode)){
			if(strncmp_with_string_list(token, token_len, DelimiterStrListComma, 0)){
				*addr_len = *ptr - *addr;
				return 0;
			}
		}
		if(ret < 0){
			*addr_len = *ptr - *addr;
			return 0;
		}
	}
}
#endif

#ifdef AAA
int
get_phrase_token_from_address(char **ptr, int *len, char **phrase, int *phrase_len)
{
	ParseMode	parse_mode;
	int	comment_nest;
	char	*token;
	int	token_len;
	int	ret;

	parse_mode = NormalMode;
	comment_nest = 0;

	*phrase = *ptr;
	*phrase_len = *len;

	while(1){
		switch(parse_mode){
		    case NormalMode:
			ret = next_token(ptr, len, &token, &token_len, DelimiterStrListRootAddress);
			if(ret > 0){
				if(strncmp_with_string_list(token, token_len, DelimiterStrListLeftParenthesis, 0)){
					parse_mode = CommentMode;
					comment_nest = 1;
				}
				else if(strncmp_with_string_list(token, token_len, DelimiterStrListQuotation, 0))
					parse_mode = QuotedMode;
				else if(strncmp_with_string_list(token, token_len, DelimiterStrListLeftAngleBracket, 0)){
					(*ptr)--;
					(*len)++;
					*phrase_len = *ptr - *phrase;
					if(*phrase_len)
						return 1;
					else
						return 0;
				}
			}
			break;
		    case CommentMode:
			ret = next_token(ptr, len, &token, &token_len, DelimiterStrListParentheses);
			if(ret > 0){
				if(strncmp_with_string_list(token, token_len, DelimiterStrListLeftParenthesis, 0))
					comment_nest++;
				else if(strncmp_with_string_list(token, token_len, DelimiterStrListRightParenthesis, 0)){
					comment_nest--;
					if(comment_nest <= 0)
						parse_mode = NormalMode;
				}
			}
			break;
		    case QuotedMode:
			ret = next_token(ptr, len, &token, &token_len, DelimiterStrListQuotation);
			if(ret > 0){
				if(strncmp_with_string_list(token, token_len, DelimiterStrListQuotation, 0))
					parse_mode = NormalMode;
			}
			break;
		}

		if(ret < 0){
			*ptr = *phrase;
			*len = *phrase_len;
			*phrase_len = 0;
			return -1;
		}
	}
}
#endif
		
#ifdef AAA
int
get_precomment_from_address(char **ptr, int *len, char **comment, int *comment_len)
{
	ParseMode	parse_mode;
	int	comment_nest;
	char	*token, *org_ptr;
	int	token_len, org_len;
	int	ret;

	parse_mode = NormalMode;
	comment_nest = 0;

	*comment = *ptr;
	*comment_len = *len;

	while(1){
		switch(parse_mode){
		    case NormalMode:
			org_ptr = *ptr;
			org_len = *len;
			ret = next_token(ptr, len, &token, &token_len, DelimiterStrListComment);
			if(ret > 0){
				if(strncmp_with_string_list(token, token_len, DelimiterStrListLeftParenthesis, 0)){
					parse_mode = CommentMode;
					comment_nest = 1;
				}
			}
			else{
				*ptr = org_ptr;
				*len = org_len;
				*comment_len = *ptr - *comment;
				if(*comment_len)
					return 1;
				else
					return 0;
			}
			break;
		    case CommentMode:
			ret = next_token(ptr, len, &token, &token_len, DelimiterStrListParentheses);
			if(ret > 0){
				if(strncmp_with_string_list(token, token_len, DelimiterStrListLeftParenthesis, 0))
					comment_nest++;
				else if(strncmp_with_string_list(token, token_len, DelimiterStrListRightParenthesis, 0)){
					comment_nest--;
					if(comment_nest <= 0)
						parse_mode = NormalMode;
				}
			}
			if(ret < 0){
				*ptr = *comment;
				*len = *comment_len;
				*comment_len = 0;
				return 1;
			}
			break;
		}
	}
}
#endif
		
#ifdef AAA
int
get_addrspec_from_address(char **ptr, int *len, char **addr, int *addr_len)
{
	ParseMode	parse_mode;
	int	comment_nest;
	char	*token, *comment_ptr;
	int	token_len, org_len;
	int	ret;

	parse_mode = NormalMode;
	comment_nest = 0;

	*addr = *ptr;
	org_len = *len;
	comment_ptr = (char *)NULL;

	while(1){
		switch(parse_mode){
		    case NormalMode:
			ret = next_token(ptr, len, &token, &token_len, DelimiterStrListComment);
			if(ret > 0){
				if(comment_ptr == (char *)NULL)
					comment_ptr = *ptr;
				if(strncmp_with_string_list(token, token_len, DelimiterStrListLeftParenthesis, 0)){
					parse_mode = CommentMode;
					comment_nest = 1;
				}
			}
			if(ret == 0){
				comment_ptr = (char *)NULL;
			}
			if(ret < 0){
				if(comment_ptr){
					*addr_len = comment_ptr - *addr;
					*ptr = comment_ptr;
					*len = org_len - *addr_len;
					return 1;
				}
				else{
					*addr_len = org_len;
					return 1;
				}	
			}
			break;
		    case CommentMode:
			ret = next_token(ptr, len, &token, &token_len, DelimiterStrListParentheses);
			if(ret > 0){
				if(strncmp_with_string_list(token, token_len, DelimiterStrListLeftParenthesis, 0))
					comment_nest++;
				else if(strncmp_with_string_list(token, token_len, DelimiterStrListRightParenthesis, 0)){
					comment_nest--;
					if(comment_nest <= 0)
						parse_mode = NormalMode;
				}
			}
			if(ret < 0){
				*addr_len = org_len;
				return 1;
			}
			break;
		}
	}
}
#endif

#ifdef AAA
int
encode_address_line(LclBuffer *buf, char *ptr, int len, int *pos, LclMailEncoding encoding, char *charset)
{
	char	*addr;
	int	addr_len;
	char	*token;
	int	token_len;

	while(len){
		if (get_next_address_token(&ptr, &len, &addr, &addr_len) == 0){
			if(get_phrase_token_from_address(&addr, &addr_len, &token, &token_len) > 0){
/*
				fprintf(stdout, "phrase:\n");
				print_token(token, token_len);
*/
				if(encode_mail_mode(buf, token, token_len, pos, encoding, charset, FoldAlwaysNo))
					return -1;
			}
			if(get_precomment_from_address(&addr, &addr_len, &token, &token_len) > 0){
/*
				fprintf(stdout, "pre-comment:\n");
				print_token(token, token_len);
*/
				if(encode_mail_mode(buf, token, token_len, pos, encoding, charset, FoldAlwaysNo))
					return -1;
			}
			if(get_addrspec_from_address(&addr, &addr_len, &token, &token_len) > 0){
/*
				fprintf(stdout, "addr-spec:\n");
				print_token(token, token_len);
*/
				if(encode_mail_mode(buf, token, token_len, pos, encoding, charset, FoldAlwaysNo))
/*
				if(add_element_with_folding(buf, token, token_len, pos, FoldAtMost))
*/
					return -1;
			}
			if(addr_len){
/*
				fprintf(stdout, "post-comment:\n");
				print_token(addr, addr_len);
*/
				if(encode_mail_mode(buf, addr, addr_len, pos, encoding, charset, FoldAlwaysNo))
					return -1;
			}
		}
	}
	return 0;
}
#endif

#ifdef AAA
char *
_lcl_mime_remove_soft_return(char *header, int length)
{
	LclBuffer	*buf;
	char	*ptr;
	int	len;
	int	ret;
	int	comp_len;
	char	*decode_buf;

	buf = _LclBuffer_create(length);
	if(buf == (LclBuffer *)NULL)
		return (char *)NULL;

	while(length > 0){
		comp_len = strncmp_with_string_list(header, length, LclSoftReturnStringList, 0);
		if(comp_len > 0){
			header += comp_len;
			length -= comp_len;
		}
		else{
			if(_LclBuffer_add(buf, header, 1) != 0){
				_LclBuffer_destroy(buf);
				return (char *)NULL;
			}
			header++;
			length--;
		}
	}
	decode_buf = _LclBuffer_get_string(buf);
	_LclBuffer_destroy(buf);
	return decode_buf;
}
#endif

#ifdef AAA
int
_lcl_encode_ascii_text(LclBuffer *buf, char *ptr, int len, int *pos)
{
	int	line_char_num;

	if((ptr == (char *)NULL) || (len <= 0))
		return -1;

	while(len){
		if(LclMaxHeaderLineLen - *pos <= 0){
			if(_LclBuffer_add(buf, LclSoftReturnStringList[0], LclSoftReturn0StringLength) != 0)
				return -1;
			*pos = LclAfterSoftReturnPos;
		}

		line_char_num = LclMaxHeaderLineLen - *pos;
		if(line_char_num > len)
			line_char_num = len;

		if(_LclBuffer_add(buf, ptr, line_char_num) != 0)
			return -1;

		*pos += line_char_num;
		ptr += line_char_num;
		len -= line_char_num;
	}
	return 0;
}
#endif
