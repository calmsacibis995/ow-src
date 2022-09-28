#include <iconv.h>

extern void *_lcl_tw_to_iso_icv_open();
extern void *_lcl_iso_to_tw_icv_open();

typedef enum {TW_TO_ISO, ISO_TO_TW} _lcl_iconv_type;

typedef struct _lcl_iconv_struct {
	_lcl_iconv_type	type;
	void	*state;
} _lcl_iconv_t;

iconv_t
_lcl_iconv_open(char *to_name, char *from_name)
{
	_lcl_iconv_type	type;
	void	*state;
	_lcl_iconv_t	*conv_t;

	if(!strcmp(to_name, "zh_TW-iso2022-CN-EXT") && !strcmp(from_name, "zh_TW-euc")){
		type = TW_TO_ISO;
		state = _lcl_tw_to_iso_icv_open();
	}
	else if(!strcmp(from_name, "zh_TW-iso2022-CN-EXT") && !strcmp(to_name, "zh_TW-euc")){
		type = ISO_TO_TW;
		state = _lcl_iso_to_tw_icv_open();
	}
	else
		return (iconv_t)-1;
	
	if(state == (void *)-1)
		return (iconv_t)-1;

	conv_t = (_lcl_iconv_t *)malloc(sizeof(_lcl_iconv_t));
	if(conv_t == (_lcl_iconv_t *)NULL)
		return (iconv_t)-1;
	
	conv_t->type = type;
	conv_t->state = state;

	return (iconv_t)conv_t;
}

void
_lcl_iconv_close(iconv_t i_conv)
{
	_lcl_iconv_t	*conv_t = (_lcl_iconv_t *)i_conv;

	switch(conv_t->type){
		case TW_TO_ISO:
			_lcl_tw_to_iso_icv_close(conv_t->state);
			break;
		case ISO_TO_TW:
			_lcl_iso_to_tw_icv_close(conv_t->state);
			break;
	}
	free(conv_t);
}

size_t
_lcl_iconv(iconv_t i_conv, char **inbuf, size_t *inbytesleft, char **outbuf, size_t *outbytesleft)
{
	_lcl_iconv_t	*conv_t = (_lcl_iconv_t *)i_conv;

	switch(conv_t->type){
		case TW_TO_ISO:
			return _lcl_tw_to_iso_icv_iconv(conv_t->state, inbuf, inbytesleft, outbuf, outbytesleft);
			break;
		case ISO_TO_TW:
			return _lcl_iso_to_tw_icv_iconv(conv_t->state, inbuf, inbytesleft, outbuf, outbytesleft);
			break;
		default:
			return -1;
	}
}
