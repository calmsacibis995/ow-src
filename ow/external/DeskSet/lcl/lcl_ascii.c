/*
 * Copyright (c) 1996, Sun Microsystems, Inc.
 * All rights reserved.
 */
 
#pragma ident	"@(#)lcl_ascii.c 1.6	96/10/10 SMI"

#include <sys/types.h>

char *
_lcl_convert_to_ascii(char *buf, size_t len)
{
	size_t	i;
	unsigned char	*u_buf;
	boolean_t	is_ascii = B_TRUE;
	char	*new_buf;

	u_buf = (unsigned char *)buf;
	for(i = 0; i < len; i++){
		if(u_buf[i] & 0x80){
			is_ascii = B_FALSE;
			break;
		}
	}

	if(is_ascii == B_FALSE)
		return (char *)NULL;

	new_buf = (char *)malloc(len + 1);
	if(new_buf == (char *)NULL)
		return (char *)NULL;

	strncpy(new_buf, buf, len);
	return new_buf;
}

boolean_t
_lcl_check_ascii(char *buf, size_t len)
{
	size_t	i;
	unsigned char	*u_buf;

	u_buf = (unsigned char *)buf;
	for(i = 0; i < len; i++){
		if(u_buf[i] & 0x80){
			return B_FALSE;
		}
	}
	return B_TRUE;
}

boolean_t
_lcl_check_printable_ascii(char *buf, size_t len)
{
	unsigned char	*uc = (unsigned char *)buf;

	while(len > 0){
		if((*uc < 0x20) || (*uc > 0x7f)){
			if((*uc != 0x09) && (*uc != 0x0d) && (*uc != 0x0a))
				return B_FALSE;
		}
		uc++;
		len--;
	}
	return B_TRUE;
}
