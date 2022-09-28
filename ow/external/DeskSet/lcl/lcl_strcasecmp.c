/*
 * Copyright (c) 1996, Sun Microsystems, Inc.
 * All rights reserved.
 */
 
#pragma ident	"@(#)lcl_lcl_strcasecmp.c 1.2	96/06/27 SMI"

/*//////////////////////////////////////////////////////////////////////*/
#include <ctype.h>
#include <memory.h>

lcl_strncasecmp(a,b,n)
	register char *a,*b;
	register int n;
{	register char ac,bc;
	register int i;

	for( i = 1; i < n; i++ ){
		ac = *a++;
		bc = *b++;

		if(ac == 0){
			if(bc == 0)
				return 0;
			else	return -1;
		}else
		if(bc == 0)
			return 1;
		else
		if(ac != bc){
			if(islower(ac)) ac = toupper(ac);
			if(islower(bc)) bc = toupper(bc);
			if( ac != bc )
				return ac - bc;
		}
	}
	if(islower(*a)) ac = toupper(*a); else ac = *a;
	if(islower(*b)) bc = toupper(*b); else bc = *b;
	return ac - bc;
}

lcl_strcasecmp(a,b)
	register char *a,*b;
{

	register int len_a;
	register int len_b;
	register int i;
	int *upper_a,*upper_b; 
	int *str_pt_a,*str_pt_b;
	int value;

	len_a=strlen(a);
	len_b=strlen(b);	

	if (len_a==len_b){
	       	if (_toupper(*a) == _toupper(*b)){
		   upper_a=(int *)malloc(sizeof(int)*len_a);
		   upper_b=(int *)malloc(sizeof(int)*len_a);
		   str_pt_a=upper_a;
		   str_pt_b=upper_b;
		   for (i=0;i<len_a;i++){
		      *upper_a=_toupper(*a++);
		      *upper_b=_toupper(*b++);
		       upper_a++;
		       upper_b++;
		     
		    }
		    value=memcmp(str_pt_a,str_pt_b,len_a);
		    free(str_pt_a);
		    free(str_pt_b);
		    return value;	
		}

	}
	else if (len_a > len_b)
	        return 1;
	        else return -1;	

}
