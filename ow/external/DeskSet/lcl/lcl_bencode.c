#include <stdio.h>
#include <locale.h>

#include "lcl.h"
#include "lcl_internal.h"
#include "lcl_mime_encode.h"

typedef enum {EscSI, EscSO, EscSS2, EscSS3, EscDesigSO, EscDesigSS2, EscDesigSS3, EscUnknown} EscType;

typedef struct{
	char	*string;
	EscType	type;
	int	char_len;
	int	ascii;
}EscInfo;

static EscInfo ISO_2022_JP_def[] = {
	{"\033(B",	EscSI,	1,	1},
	{"\033(J",	EscSI,	1,	1},
	{"\033$@",	EscSO,	2,	0},
	{"\033$B",	EscSO,	2,	0},
	{(char *)NULL,	0,	0,	0},
};

static EscInfo ISO_2022_CN_def[] = {
	{"\017",	EscSI,		0,	0},
	{"\016",	EscSO,		0,	0},
	{"\033\116",	EscSS2,		2,	0},
	{"\033\117",	EscSS3,		2,	0},
	{"\033$)A",	EscDesigSO,	2,	0},
	{"\033$)G",	EscDesigSO,	2,	0},
	{"\033$)H",	EscDesigSO,	2,	0}, /* for ISO-2022-7 */
	{"\033$)I",	EscDesigSO,	2,	0}, /* for ISO-2022-7 */
	{"\033$*H",	EscDesigSS2,	2,	0},
	{"\033$+I",	EscDesigSS3,	2,	0},
	{(char *)NULL,	0,		0,	0},
};

typedef struct{
	char	*charset;
	EscInfo	*info;
	int	initial_shift;
}CharsetEscInfo;

static CharsetEscInfo	ISO_2022_def[] = {
	{"ISO-2022-JP",	ISO_2022_JP_def,	1},
	{"ISO-2022-CN",	ISO_2022_CN_def,	0},
	{"ISO-2022-CN-EXT",	ISO_2022_CN_def,	0},
	{"eucKR",	0,			0},
	{(char *)NULL,	0,			0},
};

static EscInfo *
get_esc_SI_info(EscInfo *list)
{
	while(list){
		if((list->type == EscSI) || (list->ascii))
			return list;
		else
			list++;
	}
	return (EscInfo *)NULL;
}

static EscInfo *
get_esc_SO_info(EscInfo *list)
{
	while(list){
		if(list->type == EscSO)
			return list;
		else
			list++;
	}
	return (EscInfo *)NULL;
}

static EscInfo *
get_esc_info(char *charset)
{
	CharsetEscInfo	*cs_info;
	EscInfo	*esc_info;

	if(charset == (char *)NULL)
		return (EscInfo *)NULL;

	cs_info = ISO_2022_def;
	esc_info = (EscInfo *)NULL;
	while(cs_info->charset){
		if(!strcmp(charset, cs_info->charset)){
			esc_info = cs_info->info;
			break;
		}
		cs_info++;
	}
	return esc_info;
}

static int
get_initial_shift(char *charset)
{
	CharsetEscInfo	*cs_info;

	if(charset == (char *)NULL)
		return 0;

	cs_info = ISO_2022_def;
	while(cs_info->charset){
		if(!strcmp(charset, cs_info->charset)){
			return cs_info->initial_shift;
		}
		cs_info++;
	}
	return 0;
}

static EscInfo *
get_cur_desig_SO_info(char *charset)
{
	EscInfo *list = get_esc_info(charset);
	while(list){
		if(list->type == EscDesigSO)
			return list;
		else
			list++;
	}
	return (EscInfo *)NULL;
}

static EscInfo *
get_cur_desig_SS2_info(char *charset)
{
	EscInfo *list = get_esc_info(charset);
	while(list){
		if(list->type == EscDesigSS2)
			return list;
		else
			list++;
	}
	return (EscInfo *)NULL;
}

static EscInfo *
get_cur_desig_SS3_info(char *charset)
{
	EscInfo *list = get_esc_info(charset);
	while(list){
		if(list->type == EscDesigSS3)
			return list;
		else
			list++;
	}
	return (EscInfo *)NULL;
}

static char Base64Index[] =
	"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

static int
b64_encode(LclBuffer *out, char *str, int str_len)
{
	int	buf_space;
	unsigned char	*ptr;
	unsigned char	c1, c2, c3;
	unsigned char	buf[4];
	int	total_len = 0;

	ptr = (unsigned char *)str;
	while(str_len > 0){
		if(str_len >= 3){
			c1 = *ptr++;
			c2 = *ptr++;
			c3 = *ptr++;
			buf[0] = Base64Index[c1 >> 2];
			buf[1] = Base64Index[((c1 & 0x3) << 4) | ((int)(c2 & 0xf0) >> 4)];
			buf[2] = Base64Index[((c2 & 0xf) << 2) | ((int)(c3 & 0xc0) >> 6)];
			buf[3] = Base64Index[c3 & 0x3f];
			str_len -= 3;
		}
		else if(str_len == 2){
			c1 = *ptr++;
			c2 = *ptr++;
			buf[0] = Base64Index[c1 >> 2];
			buf[1] = Base64Index[((c1 & 0x3) << 4) | ((int)(c2 & 0xf0) >> 4)];
			buf[2] = Base64Index[(c2 & 0xf) << 2];
			buf[3] = '=';
			str_len -= 2;
		}
		else if(str_len == 1){
			c1 = *ptr++;
			buf[0] = Base64Index[c1 >> 2];
			buf[1] = Base64Index[(c1 & 0x3) << 4];
			buf[2] = '=';
			buf[3] = '=';
			str_len--;
		}
		else
			break;

		if (_LclBuffer_add(out, (char *)(&buf), 4) != 0)
			return -1;
		total_len += 4;
	}
	return total_len;
}

static int
b_encode(LclBuffer *out, char *str, int str_len, char *charset){
	int	c1, c2, c3;
	char	buf[3];

	if(str_len == 0)
		return 0;

	if(charset == (char *)NULL)
		return -1;

	buf[0] = '=';
	buf[1] = '?';
	if(_LclBuffer_add(out, buf, 2) != 0)
		return -1;

	if(_LclBuffer_add(out, charset, strlen(charset)) != 0)
		return -1;

	buf[0] = '?';
	buf[1] = 'B';
	buf[2] = '?';
	if(_LclBuffer_add(out, buf, 3) != 0)
		return -1;

	if(b64_encode(out, str, str_len) < 0)
		return -1;

	buf[0] = '?';
	buf[1] = '=';
/* no longer needed
	buf[2] = ' ';
	if(_LclBuffer_add(out, buf, 3) != 0)
*/
	if(_LclBuffer_add(out, buf, 2) != 0)
		return -1;

	return 0;
}

static int
add_b_encode(LclBuffer *out, char *str, int str_len, int *pos, char *charset)
{
	char	buf[3];
	int	len;
	int	charsetLength = strlen(charset);

	buf[0] = '=';
	buf[1] = '?';
	if(_LclBuffer_add(out, buf, 2) != 0)
		return -1;
	*pos += 2;

	if(_LclBuffer_add(out, charset, charsetLength) != 0)
		return -1;
	*pos += charsetLength;

	buf[0] = '?';
	buf[1] = 'B';
	buf[2] = '?';
	if(_LclBuffer_add(out, buf, 3) != 0)
		return -1;
	*pos += 3;

	len = b64_encode(out, str, str_len);
	if(len < 0)
		return -1;
	*pos += len;

	buf[0] = '?';
	buf[1] = '=';
	if(_LclBuffer_add(out, buf, 2) != 0)
		return -1;
	*pos += 2;

	return 0;
}

static EscInfo *
strncmp_with_esc_list(char *ptr, int len, EscInfo *esc_list)
{
	while(esc_list->string){
		int cmp_len = strlen(esc_list->string);
		if(len >= cmp_len){
			if(!strncmp(ptr, esc_list->string, cmp_len))
				return esc_list;
		}
		esc_list++;
	}
	return (EscInfo *)NULL;
}

static EscInfo *
get_escape_segment(char **ptr, int *len, char **token, int *token_len, EscInfo *esc_list)
{
	int	encode_mode;
	EscInfo	*ret_info;
	int retinfostringLength;

	*token = (char *)NULL;
	*token_len = 0;

	ret_info = strncmp_with_esc_list(*ptr, *len, esc_list);
	if(ret_info){
	    retinfostringLength = strlen(ret_info->string);
		switch(ret_info->type){
			case EscSS2:
			case EscSS3:
				*ptr += retinfostringLength;
				*len -= retinfostringLength;
				*token = *ptr;
				*ptr += ret_info->char_len;
				*len -= ret_info->char_len;
				*token_len = *ptr - *token;
				if(*len < 0)
					return (EscInfo *)-1;
				else
					return ret_info;
				break;
			default:
				*ptr += retinfostringLength;
				*len -= retinfostringLength;
				return ret_info;
				break;
		}	
	}
	else{
		*token = *ptr;
		while(*len > 0){
			EscInfo *esc_info;
			esc_info = strncmp_with_esc_list(*ptr, *len, esc_list);
			if(esc_info)
				break;
			else{
				(*ptr)++;
				(*len)--;
			}
		}
		*token_len = *ptr - *token;
		return ret_info;
	}
}

static int
b_encode_text_JP(LclBuffer *out, char *ptr, int len, int *pos, char *charset)
{
	char	buf[100];
	int	buf_pos;
	char	*str;
	int	str_len;
	int	seq_len;
	int	line_space;
	EscInfo	*esc_list;
	EscInfo	*esc_SI;

	int	SO_mode;
	int	SO_turn_on;
	int	desig_SO_on;
	EscInfo	*cur_desig_SO;

	if((ptr == (char *)NULL) || (len <= 0) || (charset == (char *)NULL))
		return -1;

	SO_mode = 0;
	SO_turn_on = 0;
	desig_SO_on = 0;
	cur_desig_SO = (EscInfo *)NULL;

	esc_list = get_esc_info(charset);
	esc_SI = get_esc_SI_info(esc_list);

	seq_len = 2 + strlen(charset) + 3 + 2;

	line_space = (LclMaxHeaderLineLen - *pos - seq_len) / 4 * 3;
	buf_pos = 0;

	while(len){
		EscInfo	*cur_esc;
		EscType	esc_type;
		int	req_space, char_num, line_char_space, line_char_num;

		cur_esc = get_escape_segment(&ptr, &len, &str, &str_len, esc_list);
		if(cur_esc == (EscInfo *)-1)
			return -1;

		if(cur_esc == (EscInfo *)NULL){
			if(SO_mode)
				esc_type = EscSO;
			else
				esc_type = EscSI;
		}
		else
			esc_type = cur_esc->type;

		switch(esc_type){
			case EscSI:
				if(cur_esc){
				    int curescstringLength = strlen(cur_esc->string);
					memcpy(buf + buf_pos, cur_esc->string, curescstringLength);
					buf_pos += curescstringLength;
					SO_turn_on = 0;
					desig_SO_on = 0;
					SO_mode = 0;
				}
				while(str_len){
					if (line_space - buf_pos <= 0){
						if(add_b_encode(out, buf, buf_pos, pos, charset) != 0)
								return -1;
						if (_LclBuffer_add(out, LclSoftReturnStringList[0], LclSoftReturn0StringLength) != 0)
							return -1;
						*pos = LclAfterSoftReturnPos;
						line_space = (LclMaxHeaderLineLen - *pos - seq_len) / 4 * 3;
						buf_pos = 0;

						SO_turn_on = 0;
						desig_SO_on = 0;
					}
					line_char_num = line_space - buf_pos;
					if(line_char_num > str_len)
						line_char_num = str_len;
					memcpy(buf + buf_pos, str, line_char_num);
					buf_pos += line_char_num;
					str += line_char_num;
					str_len -= line_char_num;
				}
				break;
			case EscSO:
				if(cur_esc){
					if(cur_esc != cur_desig_SO)
						desig_SO_on = 0;
					cur_desig_SO = cur_esc;
					SO_mode = 1;
				}
				char_num = str_len / cur_desig_SO->char_len;
				while(char_num > 0){
					req_space = strlen(esc_SI->string);
					req_space += cur_desig_SO->char_len;
					if(!desig_SO_on)
						req_space += strlen(cur_desig_SO->string);
					if (line_space - buf_pos < req_space){
						if(SO_turn_on){
							memcpy(buf + buf_pos, esc_SI->string, strlen(esc_SI->string));
							buf_pos += strlen(esc_SI->string);
						}
						if(buf_pos > 0){
							if(add_b_encode(out, buf, buf_pos, pos, charset) != 0)
								return -1;
						}
						if (_LclBuffer_add(out, LclSoftReturnStringList[0], LclSoftReturn0StringLength) != 0)
							return -1;
						*pos = LclAfterSoftReturnPos;
						line_space = (LclMaxHeaderLineLen - *pos - seq_len) / 4 * 3;
						buf_pos = 0;

						SO_turn_on = 0;
						desig_SO_on = 0;
					}
					if(!desig_SO_on){
						memcpy(buf + buf_pos, cur_desig_SO->string, strlen(cur_desig_SO->string));
						buf_pos += strlen(cur_desig_SO->string);
						desig_SO_on = 1;
						SO_turn_on = 1;
					}
					line_char_space = line_space - buf_pos - strlen(esc_SI->string);
					line_char_num = line_char_space / cur_desig_SO->char_len;
					if(line_char_num > char_num)
						line_char_num = char_num;
					memcpy(buf + buf_pos, str, cur_desig_SO->char_len * line_char_num);
					buf_pos += cur_desig_SO->char_len * line_char_num;
					str += cur_desig_SO->char_len * line_char_num;
					char_num -= line_char_num;
				}
				break;
		}
	}
	if(SO_turn_on){
		memcpy(buf + buf_pos, esc_SI->string, strlen(esc_SI->string));
		buf_pos += strlen(esc_SI->string);
	}
	if(buf_pos){
		if(add_b_encode(out, buf, buf_pos, pos, charset) != 0)
			return -1;
	}
	return 0;
}

static int
b_encode_text_CN(LclBuffer *out, char *ptr, int len, int *pos, char *charset)
{
	char	buf[100];
	int	buf_pos = 0;
	char	*str;
	int	str_len;
	int	seq_len;
	int	line_space;
	EscInfo	*esc_list;
	EscInfo	*esc_SI;
	EscInfo	*esc_SO;

	int	SO_mode;
	int	SO_turn_on;
	int	desig_SO_on;
	int	desig_SS2_on;
	int	desig_SS3_on;
	EscInfo	*cur_desig_SO;
	EscInfo	*cur_desig_SS2;
	EscInfo	*cur_desig_SS3;

	if((ptr == (char *)NULL) || (len <= 0) || (charset == (char *)NULL))
		return -1;

	SO_mode = 0;
	SO_turn_on = 0;
	desig_SO_on = 0;
	desig_SS2_on = 0;
	desig_SS3_on = 0;
	cur_desig_SO = get_cur_desig_SO_info(charset);
	cur_desig_SS2 = get_cur_desig_SS2_info(charset);
	cur_desig_SS3 = get_cur_desig_SS3_info(charset);

	esc_list = get_esc_info(charset);
	esc_SI = get_esc_SI_info(esc_list);
	esc_SO = get_esc_SO_info(esc_list);

	seq_len = 2 + strlen(charset) + 3 + 2;
	line_space = (LclMaxHeaderLineLen - *pos - seq_len) / 4 * 3;

	while(len){
		EscInfo	*cur_esc;
		EscType	esc_type;
		int	req_space, char_num, line_char_space, line_char_num;

		cur_esc = get_escape_segment(&ptr, &len, &str, &str_len, esc_list);
		if(cur_esc == (EscInfo *)-1)
			return -1;

		if(cur_esc == (EscInfo *)NULL){
			if(SO_mode)
				esc_type = EscSO;
			else
				esc_type = EscSI;
		}
		else{
			esc_type = cur_esc->type;
#ifdef AAA
			/* added for iconv bug */
			if((esc_type == EscSI) && !SO_turn_on)
				SO_turn_on = 1;
#endif
		}

		switch(esc_type){
			case EscSI:
				SO_mode = 0;
				if(SO_turn_on){
					memcpy(buf + buf_pos, esc_SI->string, strlen(esc_SI->string));
					buf_pos += strlen(esc_SI->string);
					SO_turn_on = 0;	
				}
				while(str_len){
					if (line_space - buf_pos <= 0){
						if(add_b_encode(out, buf, buf_pos, pos, charset) != 0)
								return -1;
						if (_LclBuffer_add(out, LclSoftReturnStringList[0], LclSoftReturn0StringLength) != 0)
							return -1;
						*pos = LclAfterSoftReturnPos;
						line_space = (LclMaxHeaderLineLen - *pos - seq_len) / 4 * 3;
						buf_pos = 0;

						SO_turn_on = 0;
						desig_SO_on = 0;
						desig_SS2_on = 0;
						desig_SS3_on = 0;
					}
					line_char_num = line_space - buf_pos;
					if(line_char_num > str_len)
						line_char_num = str_len;
					memcpy(buf + buf_pos, str, line_char_num);
					buf_pos += line_char_num;
					str += line_char_num;
					str_len -= line_char_num;
				}
				break;
			case EscDesigSO:
/* comment out for iconv bug
				if(cur_desig_SO != cur_esc)
*/
					desig_SO_on = 0;
				cur_desig_SO = cur_esc;
				break;
			case EscDesigSS2:
/* comment out for iconv bug
				if(cur_desig_SS2 != cur_esc)
*/
					desig_SS2_on = 0;
				cur_desig_SS2 = cur_esc;
				break;
			case EscDesigSS3:
/* comment out for iconv bug
				if(cur_desig_SS3 != cur_esc)
*/
					desig_SS3_on = 0;
				cur_desig_SS3 = cur_esc;
				break;
			case EscSS2:
				req_space = strlen(cur_esc->string);
				req_space += cur_esc->char_len;
				if(!desig_SS2_on)
					req_space += strlen(cur_desig_SS2->string);
				if(SO_turn_on)
					req_space += strlen(esc_SI->string);
				if(line_space - buf_pos < req_space){
					if(SO_turn_on){
						memcpy(buf + buf_pos, esc_SI->string, strlen(esc_SI->string));
						buf_pos += strlen(esc_SI->string);
					}
					if(buf_pos > 0){
						if(add_b_encode(out, buf, buf_pos, pos, charset) != 0)
							return -1;
					}
					if (_LclBuffer_add(out, LclSoftReturnStringList[0], LclSoftReturn0StringLength) != 0)
						return -1;
					*pos = LclAfterSoftReturnPos;
					line_space = (LclMaxHeaderLineLen - *pos - seq_len) / 4 * 3;
					buf_pos = 0;

					SO_turn_on = 0;
					desig_SO_on = 0;
					desig_SS2_on = 0;
					desig_SS3_on = 0;
				}
					
				if(!desig_SS2_on){
					memcpy(buf + buf_pos, cur_desig_SS2->string, strlen(cur_desig_SS2->string));
					buf_pos += strlen(cur_desig_SS2->string);
					desig_SS2_on = 1;
				}
				memcpy(buf + buf_pos, cur_esc->string, strlen(cur_esc->string));
				buf_pos += strlen(cur_esc->string);

				memcpy(buf + buf_pos, str, cur_desig_SS2->char_len);
				buf_pos += cur_esc->char_len;

				break;
			case EscSS3:
				req_space = strlen(cur_esc->string);
				req_space += cur_esc->char_len;
				if(!desig_SS3_on)
					req_space += strlen(cur_desig_SS3->string);
				if(SO_turn_on)
					req_space += strlen(esc_SI->string);
				if(line_space - buf_pos < req_space){
					if(SO_turn_on){
						memcpy(buf + buf_pos, esc_SI->string, strlen(esc_SI->string));
						buf_pos += strlen(esc_SI->string);
					}
					if(buf_pos > 0){
						if(add_b_encode(out, buf, buf_pos, pos, charset) != 0)
							return -1;
					}
					if (_LclBuffer_add(out, LclSoftReturnStringList[0], LclSoftReturn0StringLength) != 0)
						return -1;
					*pos = LclAfterSoftReturnPos;
					line_space = (LclMaxHeaderLineLen - *pos - seq_len) / 4 * 3;
					buf_pos = 0;

					SO_turn_on = 0;
					desig_SO_on = 0;
					desig_SS2_on = 0;
					desig_SS3_on = 0;
				}
					
				if(!desig_SS3_on){
					memcpy(buf + buf_pos, cur_desig_SS3->string, strlen(cur_desig_SS3->string));
					buf_pos += strlen(cur_desig_SS3->string);
					desig_SS3_on = 1;
				}
				memcpy(buf + buf_pos, cur_esc->string, strlen(cur_esc->string));
				buf_pos += strlen(cur_esc->string);

				memcpy(buf + buf_pos, str, cur_desig_SS3->char_len);
				buf_pos += cur_esc->char_len;

				break;
			case EscSO:
				SO_mode = 1;
				char_num = str_len / cur_desig_SO->char_len;
				while(char_num > 0){
					req_space = strlen(esc_SI->string);
					req_space += cur_desig_SO->char_len;
					if(!SO_turn_on)
						req_space += strlen(esc_SO->string);
					if(!desig_SO_on)
						req_space += strlen(cur_desig_SO->string);
					if (line_space - buf_pos < req_space){
						if(SO_turn_on){
							memcpy(buf + buf_pos, esc_SI->string, strlen(esc_SI->string));
							buf_pos += strlen(esc_SI->string);
						}
						if(buf_pos > 0){
							if(add_b_encode(out, buf, buf_pos, pos, charset) != 0)
								return -1;
						}
						if (_LclBuffer_add(out, LclSoftReturnStringList[0], LclSoftReturn0StringLength) != 0)
							return -1;
						*pos = LclAfterSoftReturnPos;
						line_space = (LclMaxHeaderLineLen - *pos - seq_len) / 4 * 3;
						buf_pos = 0;

						SO_turn_on = 0;
						desig_SO_on = 0;
						desig_SS2_on = 0;
						desig_SS3_on = 0;
					}
					if(!desig_SO_on){
						memcpy(buf + buf_pos, cur_desig_SO->string, strlen(cur_desig_SO->string));
						buf_pos += strlen(cur_desig_SO->string);
						desig_SO_on = 1;
					}
					if(!SO_turn_on){
						memcpy(buf + buf_pos, esc_SO->string, strlen(esc_SO->string));
						buf_pos += strlen(esc_SO->string);
						SO_turn_on = 1;
					}
					line_char_space = line_space - buf_pos - strlen(esc_SI->string);
					line_char_num = line_char_space / cur_desig_SO->char_len;
					if(line_char_num > char_num)
						line_char_num = char_num;
					memcpy(buf + buf_pos, str, cur_desig_SO->char_len * line_char_num);
					buf_pos += cur_desig_SO->char_len * line_char_num;
					str += cur_desig_SO->char_len * line_char_num;
					char_num -= line_char_num;
				}
				break;
		}
	}
	if(SO_turn_on){
		memcpy(buf + buf_pos, esc_SI->string, strlen(esc_SI->string));
		buf_pos += strlen(esc_SI->string);
		SO_turn_on = 0;
	}
	if(buf_pos){
		if(add_b_encode(out, buf, buf_pos, pos, charset) != 0)
			return -1;
	}
	return 0;
}

static int
b_encode_text_euc(LclBuffer *out, char *ptr, int len, int *pos, char *charset)
{
	char	*encode_ptr;
	int	encode_len;
	int	line_space;
	int	seq_len;

	seq_len = 2 + strlen(charset) + 3 + 2;

	while(len){
		if((LclMaxHeaderLineLen - *pos - seq_len) < 4){
			if (_LclBuffer_add(out, LclSoftReturnStringList[0], LclSoftReturn0StringLength) != 0)
				return -1;
			*pos = LclAfterSoftReturnPos;
		}
		line_space = (LclMaxHeaderLineLen - *pos - seq_len) / 4 * 3;
		encode_ptr = ptr;
		if(line_space < len){
			encode_len = 0;
			while(encode_len < line_space - 1){
				if(*ptr & 0x80){
					ptr += 2;
					encode_len += 2;
				}
				else{
					ptr++;
					encode_len++;
				}
			}
			if(encode_len == line_space -1){
				if(!(*ptr & 0x80)){
					ptr++;
					encode_len++;
				}
			}
			len -= encode_len;	
		}
		else{
			encode_len = len;
			ptr += encode_len;
			len -= encode_len;
		}
		if(add_b_encode(out, encode_ptr, encode_len, pos, charset) != 0)
			return -1;
	}
	return 0;
}

static int
next_segment_with_8bit(char **p, int *len, char **token, int *token_len)
{
	int	bit8_flag;

	*token = (char *)NULL;
	*token_len = 0;

	if(*len > 0){
		*token = *p;
		if((**p) & 0x80)
			bit8_flag = 1;
		else
			bit8_flag = 0;
		(*p)++;
		(*len)--;
	}
	else
		return -1;

	while(*len > 0){
		if((**p) & 0x80){
			if(!bit8_flag){
				*token_len = *p - *token;
				return 0;
			}
		}
		else{
			if(bit8_flag){
				*token_len = *p - *token;
				return 1;
			}
		}
		(*p)++;
		(*len)--;
	}

	*token_len = *p - *token;
	if(bit8_flag)
		return 1;
	else
		return 0;
}

static int
next_segment_with_esc(char **p, int *len, char **token, int *token_len, EscInfo *esc_list, int *SO_mode)
{
	int	encode_mode;
	*token = (char *)NULL;
	*token_len = 0;

	while(*len > 0){
		EscInfo	*esc_info;
		esc_info = strncmp_with_esc_list(*p, *len, esc_list);
		if(esc_info){
			switch(esc_info->type){
			    case EscSI:
				if(*token == (char *)NULL)
					*token = *p;
				*len -= strlen(esc_info->string);
				*p += strlen(esc_info->string);
				*token_len = *p - *token;
				*SO_mode = 0;
				return 1;
				break;
			    case EscSO:
				if(*token == (char *)NULL)
					*token = *p;
				else if(encode_mode == 0){
					*token_len = *p - *token;
					return 0;
				}
				*len -= strlen(esc_info->string);
				*p += strlen(esc_info->string);
				encode_mode = 1;
				*SO_mode = 1;
				break;
			    case EscSS2:
			    case EscSS3:
				if(*token == (char *)NULL)
					*token = *p;
				else if(encode_mode == 0){
					*token_len = *p - *token;
					return 0;
				}
				*len -= strlen(esc_info->string);
				*p += strlen(esc_info->string);
				*len -= esc_info->char_len;
				*p += esc_info->char_len;
				if(*len < 0)
					return -1;
				encode_mode = 1;
				break;
			    case EscDesigSO:
			    case EscDesigSS2:
			    case EscDesigSS3:
				if(*SO_mode && (esc_info->ascii)){
					if(*token == (char *)NULL)
						*token = *p;
					else if(encode_mode == 0){
						*token_len = *p - *token;
						return 0;
					}
					*len -= strlen(esc_info->string);
					*p += strlen(esc_info->string);
					*token_len = *p - *token;
					return 1;
				}
				else{
					if(*token == (char *)NULL)
						*token = *p;
					else if(encode_mode == 0){
						*token_len = *p - *token;
						return 0;
					}
					*len -= strlen(esc_info->string);
					*p += strlen(esc_info->string);
					encode_mode = 1;
				}
				break;
			    default:
				break;
			}
		}
		else{
			if(*token == (char *)NULL){
				encode_mode = 0;
				*token = *p;
			}
			(*len)--;
			(*p)++;
		}
	}
	if(*token == (char *)NULL)
		return -1;

	*token_len = *p - *token;
	if(encode_mode)
		return 1;
	else
		return 0;
}

static int
check_white_space(char *ptr, int len)
{
        while(len > 0){
                if((*ptr == ' ') || (*ptr == '\t')){
                        ptr++;
                        len--;
                }
                else
                        return 0;
        }
        return 1;
}

int
_lcl_b_encode_header_line(LclBuffer *buf, char *ptr, int len, char *charset)
{
	EscInfo	*esc_info;
	LclBuffer	*out;
	char	*token;
	int	token_len;
	int	ret;
	int	SO_mode;

	if(charset == (char *)NULL)
		return -1;

	esc_info = get_esc_info(charset);
	SO_mode = get_initial_shift(charset);

	while(1){	
		char	*encode_begin = ptr;
		char	*encode_end = ptr;
		while(1){
			if(esc_info)
				ret = next_segment_with_esc(&ptr, &len, &token, &token_len, esc_info, &SO_mode);
			else
				ret = next_segment_with_8bit(&ptr, &len, &token, &token_len);
			if(ret > 0){ /* token should be encoded */
				encode_end = token + token_len;
			}
			if(ret == 0){ /* token need not to be encoded */
				if(check_white_space(token, token_len)){
					if(encode_end == encode_begin){
						if(_LclBuffer_add(buf, token, token_len) != 0)
							return -1;
						break;
					}
				}
				else{
					if(b_encode(buf, encode_begin, encode_end - encode_begin, charset) != 0)
						return -1;
					if(_LclBuffer_add(buf, encode_end, ptr - encode_end) != 0)
						return -1;
					break;
				}
			}
			if(ret < 0){ /* no more token */
				if(b_encode(buf, encode_begin, encode_end - encode_begin, charset) != 0)
					return -1;
				if(_LclBuffer_add(buf, encode_end, ptr - encode_end) != 0)
					return -1;
				return 0;
			}
		}
	}
}

int
_lcl_b_encode_text(LclBuffer *out, char *ptr, int len, int *pos, char *charset)
{
	EscInfo	*esc_info;
	int	SO_mode;
	char	*tmp_ptr, *tmp_token;
	int	tmp_len, tmp_token_len;
	int	ret;

	esc_info = get_esc_info(charset);
	tmp_ptr = ptr;
	tmp_len = len;
	SO_mode = 0;

	if(esc_info)
		ret = next_segment_with_esc(&tmp_ptr, &tmp_len, &tmp_token, &tmp_token_len, esc_info, &SO_mode);
	else
		ret = next_segment_with_8bit(&tmp_ptr, &tmp_len, &tmp_token, &tmp_token_len);

	if((ret > 0)|| (tmp_len > 0)){
		if(esc_info){
			if(!lcl_strcasecmp(charset, "ISO-2022-JP"))
				return b_encode_text_JP(out, ptr, len, pos, charset);
			else
				return b_encode_text_CN(out, ptr, len, pos, charset);
		}
		else
			return b_encode_text_euc(out, ptr, len, pos, charset);
	}
	else
		return _LclBuffer_add(out, ptr, len);
}


/* decode */

static char index_b64[128] = {
	-1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,
	-1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,
	-1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,62, -1,-1,-1,63,
	52,53,54,55, 56,57,58,59, 60,61,-1,-1, -1,-1,-1,-1,
	-1, 0, 1, 2,  3, 4, 5, 6,  7, 8, 9,10, 11,12,13,14,
	15,16,17,18, 19,20,21,22, 23,24,25,-1, -1,-1,-1,-1,
	-1,26,27,28, 29,30,31,32, 33,34,35,36, 37,38,39,40,
	41,42,43,44, 45,46,47,48, 49,50,51,-1, -1,-1,-1,-1
};

int
_lcl_add_b64_decode(LclBuffer *out, char *ptr, int len)
{
	unsigned char	*buf, *buf_ptr;
	unsigned char	c1, c2, c3, c4;
	int	unit;
	int	pad_char;
	int	ret;

	buf = (unsigned char *)malloc(len);
	if(buf == (unsigned char *)NULL)
		return -1;
	buf_ptr = buf;

	unit = len / 4;
	pad_char = 0;
	while(unit){
		if((*ptr >= 0) && (index_b64[*ptr] >= 0))
			c1 = index_b64[*ptr++];
		else
			goto error_return;

		if((*ptr >= 0) && (index_b64[*ptr] >= 0))
			c2 = index_b64[*ptr++];
		else
			goto error_return;

		if((*ptr >= 0) && (index_b64[*ptr] >= 0))
			c3 = index_b64[*ptr++];
		else if((*ptr == '=') && (unit == 1)){
			c3 = 0;
			pad_char++;
			ptr++;
		}
		else
			goto error_return;

		if((*ptr >= 0) && (index_b64[*ptr] >= 0))
			c4 = index_b64[*ptr++];
		else if((*ptr == '=') && (unit == 1)){
			c4 = 0;
			pad_char++;
			ptr++;
		}
		else
			goto error_return;

		*buf_ptr++ = (c1 << 2) | ((int)(c2 & 0x30) >> 4);
		*buf_ptr++ = ((c2 & 0xf) << 4) | ((int)(c3 & 0x3c) >> 2);
		*buf_ptr++ = ((c3 & 0x03) << 6) | c4;

		unit--;
	}

	if(_LclBuffer_add(out, (char *)buf, buf_ptr - buf - pad_char) != 0){
		goto error_return;
	}
	else{
		if(buf)
			free(buf);
		return 0;
	}

error_return:
	if(buf)
		free(buf);
	return -1;
}
