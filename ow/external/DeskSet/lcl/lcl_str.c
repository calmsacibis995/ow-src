/*
 * Copyright (c) 1996, Sun Microsystems, Inc.
 * All rights reserved.
 */
 
#pragma ident	"@(#)lcl_str.c 1.1	97/01/28 SMI"

#include <string.h>

char *
_LclCreateStr(register char *str)
{
	register char *ptr;

	if(str == (char *)NULL)
		return (char *)NULL;

	ptr = (char *)malloc(strlen(str) + 1);
	if(ptr != (char *)NULL)
		strcpy(ptr, str);
	return ptr;
}

char *
_LclCreateStrn(char *str, int len)
{
	char	*ptr;

	ptr = (char *)malloc(len + 1);
	if(ptr != (char *)NULL){
		strncpy(ptr, str, len);
		ptr[len] = (char)NULL;
	}
	return ptr;
}
