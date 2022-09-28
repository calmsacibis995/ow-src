#include <stdio.h>
#include <locale.h>
#include <errno.h>

#include "lcl.h"
#include "lcl_internal.h"
#include "lcl_mime_encode.h"


/* encode */

static char	HexChar[] = "0123456789ABCDEF";

static int
is_utf7(char *charset)
{
	if(charset && (strstr(charset, "UTF-7") || strstr(charset, "utf-7")))
		return 1;
	else
		return 0;
}

static int
need_quoted_printable(char *p, int len, char *charset)
{
	/* if encoding is UTF-7, need not to encode */
	/* but need to add Q encode tag */
	if(is_utf7(charset)){
		while(len > 0){
			if(*p == '+')
				return 1;
			*p++;
			len--;
		}
		return 0;
	}
	else{
		while(len > 0){
			if((*p < 32) && (*p != '\n') && (*p != '\t'))
				return 1;
			if(*p >= 127)
				return 1;
			*p++;
			len--;
		}
		return 0;
	}
}

static size_t
convert_iconv(char *to_name, char *from_name, char **from_ptr, size_t *from_bytes, char **to_ptr, size_t *to_bytes)
{
	iconv_t	i_conv;
	size_t	ret;
	typedef const char **cast_type;

	i_conv = iconv_open(to_name, from_name);
	if(i_conv == (iconv_t)-1)
		return -1;

	ret = iconv(i_conv, (cast_type)from_ptr, from_bytes, to_ptr, to_bytes);
	if(ret == -1){
		if(errno == E2BIG){
			iconv_close(i_conv);
			return 1;
		}
		else{
			iconv_close(i_conv);
			return -1;
		}
	}

	ret = iconv(i_conv, (cast_type)NULL, from_bytes, to_ptr, to_bytes);
	if(ret == -1){
		if(errno == E2BIG){
			iconv_close(i_conv);
			return 1;
		}
		else{
			iconv_close(i_conv);
			return -1;
		}
	}

	iconv_close(i_conv);
	return 0;
}

static char *
get_utf8(char *ptr, int len)
{
	char	*to_buf;
	size_t	to_len;

	to_len = len * 2;

	while(1){
		iconv_t	i_conv;
		size_t	ret;
		char	*from_ptr, *to_ptr;
		size_t	from_bytes, to_bytes;

		to_buf = (char *)malloc(to_len + 1);
		if(to_buf == (char *)NULL)
			return (char *)NULL;
		
		from_ptr = ptr, to_ptr = to_buf;
		from_bytes = len, to_bytes = to_len;
		ret = convert_iconv("UTF-8", "UTF-7", &from_ptr, &from_bytes, &to_ptr, &to_bytes);
		if(ret > 0){
			free(to_buf);
			to_len = to_len * 2;
			continue;
		}
		else if(ret == 0){
			*to_ptr = (char)NULL;
			return to_buf;
		}
		else{
			free(to_buf);
			return (char *)NULL;
		}
	}
}

static size_t
utf8_length_in_length(char *str, size_t str_len)
{
	size_t	len = 0;
	char	*ptr = str;
	while(1){
		int	ret = mblen(ptr, str_len - len);
		if(ret <= 0){
			if(len > 0)
				return len;
			else
				return -1;
		}
		else{
			if(len + ret >= str_len)
				return len;
			else{
				ptr -= ret;
				len += ret;
			}
		}
	}
}

static int
qp_encode_in_space_utf7(LclBuffer *buf, char **str, int *str_len, int *space)
{
	char	*tmp_buf;
	char	*from_ptr, *to_ptr;
	size_t	from_bytes, to_bytes;
	size_t	from_len, new_from_len;

	tmp_buf = (char *)malloc(*space);
	if(tmp_buf == (char *)NULL)
		return -1;
	from_len = *str_len;

	while(1){
		size_t	ret;

		from_ptr = *str; to_ptr = tmp_buf;
		from_bytes = from_len, to_bytes = *space;
		ret = convert_iconv("UTF-7", "UTF-8", &from_ptr, &from_bytes, &to_ptr, &to_bytes);
		if(ret > 0){
			from_len = utf8_length_in_length(*str, from_ptr - *str);
			if(from_len < 0){
				free(tmp_buf);
				return -1;
			}
			continue;
		}
		else if(ret == 0){
			size_t to_len = to_ptr - tmp_buf;
			if(_LclBuffer_add(buf, tmp_buf, to_len) != 0)
				return -1;
			free(tmp_buf);
			*str += from_len;
			*str_len -= from_len;
			*space -= to_len;
			return 0;
		}
		else{
			free(tmp_buf);
			return -1;
		}
	}
}

static int
qp_encode(LclBuffer *buf, char *str, int str_len)
{
	unsigned char	str_buf[3];
	int	buf_space;

	while(str_len){
		register unsigned char c = *((unsigned char *)str);
		if((c >= 127) || (c == '=') || (c < 32 && (c != '\n' && c != '\t'))){
			str_buf[0] = '=';
			str_buf[1] = HexChar[c >> 4];
			str_buf[2] = HexChar[c & 0x0f];
			if(_LclBuffer_add(buf, (char *)str_buf, 3) != 0)
				return -1;
		}
		else{
			char	c = '_';

			if(*str == ' '){
				if(_LclBuffer_add(buf, &c, 1) != 0)
					return -1;
			}
			else{
				if(_LclBuffer_add(buf, str, 1) != 0)
					return -1;
			}
		}
		str++;
		str_len--;
	}
	return 0;
}

static int
qp_encode_in_space(LclBuffer *buf, char **str, int *str_len, int *space)
{
	unsigned char	str_buf[3];

	while(*str_len){
		register unsigned char c = *((unsigned char *)(*str));
		if((c >= 127) || (c == '=') || (c < 32 && (c != '\n' && c != '\t'))){
			if(*space < 3)
				return 1;
			str_buf[0] = '=';
			str_buf[1] = HexChar[c >> 4];
			str_buf[2] = HexChar[c & 0x0f];
			if(_LclBuffer_add(buf, (char *)str_buf, 3) != 0)
				return -1;
			(*space) -= 3;
		}
		else{
			char	c = '_';

			if(*space < 1)
				return 1;

			if(**str == ' '){
				if(_LclBuffer_add(buf, &c, 1) != 0)
					return -1;
			}
			else{
				if(_LclBuffer_add(buf, *str, 1) != 0)
					return -1;
			}

			(*space)--;
		}
		(*str)++;
		(*str_len)--;
	}
	return 0;
}


static int
get_q_encode(LclBuffer *buf, char *in, int in_len, char *charset)
{
	char	str_buf[3];
	int	buf_len, qp_len;
	int	seq_len;
	char	*seq_ptr, *qp_ptr;

	if(in_len == 0)
		return 0;

	if((in == (char *)NULL) || (in_len < 0) || (charset == (char *)NULL))
		return -1;

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

	if(qp_encode(buf, in, in_len) != 0){
		return -1;
	}

	str_buf[0] = '?';
	str_buf[1] = '=';
/* no longer add white space
	str_buf[2] = ' ';
	if(_LclBuffer_add(buf, str_buf, 3) != 0)
*/
	if(_LclBuffer_add(buf, str_buf, 2) != 0)
		return -1;

	return 0;
}

static int
add_encode_text(LclBuffer *buf, char *ptr, int len, int *pos, char *charset)
{
	char	str_buf[3];
	int	buf_len, qp_len;
	int	seq_len;
	char	*seq_ptr, *qp_ptr;
	int	space, space_org;
	int	charsetLength;

	if((ptr == (char *)NULL) || (len <= 0) || (charset == (char *)NULL))
		return -1;

	charsetLength = strlen(charset);

	seq_len = 2 + charsetLength + 3 + 2;

	while(len){
		if(LclMaxHeaderLineLen - *pos <= seq_len){
			_LclBuffer_add(buf, LclSoftReturnStringList[0], LclSoftReturn0StringLength);
			*pos = LclAfterSoftReturnPos;
		}

		str_buf[0] = '=';
		str_buf[1] = '?';
		if(_LclBuffer_add(buf, str_buf, 2) != 0)
			return -1;

		if(_LclBuffer_add(buf, charset, charsetLength) != 0)
			return -1;

		str_buf[0] = '?';
		str_buf[1] = 'Q';
		str_buf[2] = '?';
		if(_LclBuffer_add(buf, str_buf, 3) != 0)
			return -1;

		space = LclMaxHeaderLineLen - *pos - seq_len;
		space_org = space;

		if(is_utf7(charset)){
			if(qp_encode_in_space_utf7(buf, &ptr, &len, &space) < 0)
				return -1;
		}
		else{
			if(qp_encode_in_space(buf, &ptr, &len, &space) < 0)
				return -1;
		}

		str_buf[0] = '?';
		str_buf[1] = '=';
		if(_LclBuffer_add(buf, str_buf, 2) != 0)
			return -1;

		*pos += space_org - space + seq_len;
	}
	return 0;
}

static int
add_qencode_text(LclBuffer *buf, char *ptr, int len, int *pos, char *charset)
{
	if(is_utf7(charset)){
		char	*tmp_buf;
		int	ret;
		tmp_buf = get_utf8(ptr, len);
		if(tmp_buf){
			ret = add_encode_text(buf, tmp_buf, strlen(tmp_buf), pos, charset);
			free(tmp_buf);
			return ret;
		}
		else
			return -1;
	}
	else
		return add_encode_text(buf, ptr, len, pos, charset);
}

typedef enum {NormalMode, QuotedMode, CommentMode, BracketMode} ParseMode;

static char	*DelimiterStrListMail[] = {
	"(",
	")",
	"<",
	">",
	"@",
	",",
	";",
	":",
	"\\",
	"\"",
	".",
	"[",
	"]",
	" ",
	"\t",
	"\n",
	(char *)NULL
};

static char	*DelimiterStrListQuotation[] = {
	"\"",
	(char *)NULL
};

static char	*DelimiterStrListLeftParenthesis[] = {
	"(",
	(char *)NULL
};

static char	*DelimiterStrListRightParenthesis[] = {
	")",
	(char *)NULL
};

static char	*DelimiterStrListParentheses[] = {
	"(",
	")",
	(char *)NULL
};

static char	*DelimiterStrListWhiteSpace[] = {
	" ",
	"\t",
	(char *)NULL
};

int
_lcl_q_encode_header_line(LclBuffer *buf, char *ptr, int len, char *charset)
{
	char	*token;
	int	token_len;
	int	ret, add_ret;
	ParseMode	parse_mode;
	int	comment_nest;

	parse_mode = NormalMode;
	comment_nest = 0;

	while(1){
		if(parse_mode == NormalMode){
			char	*encode_begin = ptr;
			char	*encode_end = ptr;
			/* if tokens which needs to be encoded are connected */
			/* with WhiteSpaces, they should be encoded together */
			while(1){
				ret = _lcl_next_token(&ptr, &len, &token, &token_len, DelimiterStrListMail);
				if(ret == 0){ /* token is text */
					if(need_quoted_printable(token, token_len, charset)){
						encode_end = token + token_len;
					}
					else{
						/* flush out */
						if(get_q_encode(buf, encode_begin, encode_end - encode_begin, charset) != 0)
								return -1;
						if(_LclBuffer_add(buf, encode_end, ptr - encode_end) != 0)
							return -1;
						break;
					}
				}
				if(ret > 0){ /* token is delimiter */
					if(_lcl_strncmp_with_string_list(token, token_len, DelimiterStrListWhiteSpace, 0)){
						if(encode_end == encode_begin){ /* if encoding string doesn't exist */
							if(_LclBuffer_add(buf, token, token_len) != 0)
								return -1;
							break;
						}
					}
					else{
						/* mode change? */
						if(_lcl_strncmp_with_string_list(token, token_len, DelimiterStrListQuotation, 0)){
							parse_mode = QuotedMode;
						}
						else if(_lcl_strncmp_with_string_list(token, token_len, DelimiterStrListLeftParenthesis, 0)){
							parse_mode = CommentMode;
							comment_nest = 1;
						}
						/* flush out */
						if(get_q_encode(buf, encode_begin, encode_end - encode_begin, charset) != 0)
								return -1;
						if(_LclBuffer_add(buf, encode_end, ptr - encode_end) != 0)
							return -1;
						break;
					}
				}
				if(ret < 0){ /* no more token */
					/* flush out */
					if(get_q_encode(buf, encode_begin, encode_end - encode_begin, charset) != 0)
							return -1;
					if(_LclBuffer_add(buf, encode_end, ptr - encode_end) != 0)
						return -1;
					return 0;
				}
			}
		}
		else{
			/* mode change? */
			if(parse_mode == QuotedMode){
				ret = _lcl_next_token(&ptr, &len, &token, &token_len, DelimiterStrListQuotation);
				if(ret > 0){
					if(_lcl_strncmp_with_string_list(token, token_len, DelimiterStrListQuotation, 0))
						parse_mode = NormalMode;
				}
			}
			else if(parse_mode == CommentMode){
				ret = _lcl_next_token(&ptr, &len, &token, &token_len, DelimiterStrListParentheses);
				if(ret > 0){
					if(_lcl_strncmp_with_string_list(token, token_len, DelimiterStrListLeftParenthesis, 0))
						comment_nest++;
					else if(_lcl_strncmp_with_string_list(token, token_len, DelimiterStrListRightParenthesis, 0)){
						comment_nest--;
						if(comment_nest <= 0)
							parse_mode = NormalMode;
					}
				}
			}
			else{
				ret = _lcl_next_token(&ptr, &len, &token, &token_len, DelimiterStrListMail);
			}
	
			/* flush out */
			if(ret > 0){
				if(_LclBuffer_add(buf, token, token_len) != 0)
					return -1;
			}
			if(ret == 0){
				if(need_quoted_printable(token, token_len, charset)){
					if(get_q_encode(buf, token, token_len, charset) != 0)
						return -1;
				}
				else{
					if(_LclBuffer_add(buf, token, token_len) != 0)
						return -1;
				}
			}

			/* no more */
			if(ret < 0)
				return 0;
	
		}
	}
}

int
_lcl_q_encode_text(LclBuffer *buf, char *ptr, int len, int *pos, char *charset)
{
	if(need_quoted_printable(ptr, len, charset))
		return add_qencode_text(buf, ptr, len, pos, charset);
	else
		return _LclBuffer_add(buf, ptr, len);
}


/* decode */

static char index_hex[128] = {
	-1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,
	-1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,
	-1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,
	0, 1, 2, 3,  4, 5, 6, 7,  8, 9,-1,-1, -1,-1,-1,-1,
	-1,10,11,12, 13,14,15,-1, -1,-1,-1,-1, -1,-1,-1,-1,
	-1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,
	-1,10,11,12, 13,14,15,-1, -1,-1,-1,-1, -1,-1,-1,-1,
	-1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1
};

int
_lcl_add_qp_decode(LclBuffer *out, char *ptr, int len)
{
	char	*buf, *buf_ptr;
	unsigned char	uc;

	buf = (char *)malloc(len);
	if(buf == (char *)NULL)
		return -1;
	buf_ptr = buf;

	while(len > 0){
		if((len >= 3) && (*ptr == '=')){
			ptr++;
			len--;
			if(*ptr > 0){
				uc = (index_hex[*ptr]) << 4;
				ptr++;
				len--;
				if(*ptr > 0){
					uc |= index_hex[*ptr];
					*buf_ptr++ = (char)uc;
					ptr++;
					len--;
				}
				else
					goto error_return;
			}
			else
				goto error_return;
		}
		else{
			if(*ptr == '_')
				*buf_ptr++ = ' ';
			else
				*buf_ptr++ = *ptr;
			ptr++;
			len--;
		}
	}

	if(_LclBuffer_add(out, buf, buf_ptr - buf) != 0)
		goto error_return;
	else
		return 0;

error_return:
	if(buf)
		free(buf);
	return -1;
}
