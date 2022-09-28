/*
 * Copyright (c) 1996, Sun Microsystems, Inc.
 * All rights reserved.
 */
 
#pragma ident	"@(#)lcl_strstr.c 1.2	96/06/27 SMI"

/*//////////////////////////////////////////////////////////////////////*/
char *
strstr(s1,s2)
char *s1,*s2;
{
    char *p1;
    int len;
    
    if( *s2 == 0 )
	return s1;
    
    len = strlen(s2);
    for( p1 = s1; *p1; p1 ++ )
	if( *p1 == *s2 && strncmp(p1,s2,len)==0 ) 
	    return p1;
    return 0;
}
char *
strcasestr(s1,s2)
char *s1,*s2;
{
    char *p1;
    
    if( *s2 == 0 )
	return s1;
    
	for( p1 = s1; *p1; p1 ++ )
	    if( lcl_strncasecmp(p1,s2)==0 ) 
		return p1;
    return 0;
}
