/*
 * Copyright (c) 1991 by Sun Microsystems, Inc.
 * Copyright (c) 1991 by Nihon Sun Microsystems K.K.
 */

#ident  "@(#)convl.c 1.18 93/10/12 SMI; JFP;"

/*********************************************************************
 *	convtojc is to make Japanese character from a hex
 *	representation such as a4a2.
 *
 ********************************************************************/

#include <stdio.h>
#include <locale.h>
#include <widec.h>
#include <wctype.h>
#include "xctype.h"
#include "getmorph.h"

convtojc(dst, src)
char *dst;
char *src;
{
	char	buf[BUFSIZ];
	wchar_t	wbuf[BUFSIZ];
	wchar_t	wdst[BUFSIZ];
	wchar_t	wsrc[BUFSIZ];
	int	i, j, len;
	int	com, per;

	com = 0x002c;
	per = 0x002e;
	memset(buf, 0, BUFSIZ);
	memset(wbuf, 0, BUFSIZ);
	memset(wdst, 0, BUFSIZ);
	memset(wsrc, 0, BUFSIZ);

	/** if there is any japanese char in src, it must be already conved.
	    so, do not convert it.. **/
	mbstowcs(wsrc, src, strlen(src));
	len=wslen(wsrc);
	for (i=0;i<len;i++) {
	    if (isjis(wsrc[i])) {
		memcpy(dst, src, BUFSIZ);
		return(1);
	    }
	}

	convtojc_sub(buf, src);
	mbstowcs(wbuf, buf, strlen(buf));
	len = wslen(wbuf);

	for (i=0, j=0 ;i<len; i++) {
	    if (iswspace(wbuf[i]) && !iswascii(wbuf[i+1]) &&
		(i!=0 && !iswascii(wbuf[i-1]))) {
		/*** not to delete space in english sentence "this is ..."
		 *** do nothing.
		 ***/

	    } else if (iswspace(wbuf[i]) && (i!=0 && iswpunct(wbuf[i-1]))) {
		/*** not to delete space in punctuations
		 *** like "(abcde) AIUEO"
		 ***/
		wdst[j]=wbuf[i];
		j++;

	    } else if (wbuf[i]==HYPHN && isjis(wbuf[i+1])) {
		/*** convert hyphen to NAKAGURO ***/
		wdst[j]=JNAKAGURO;
		j++;

	    } else if (wbuf[i]==com) {
		wdst[j]=JTEN;
		j++;
	    } else if (wbuf[i]==per && !iswdigit(wbuf[i+1]) &&
			(i+1 ==len || !iswascii(wbuf[i+1]))) {
		/*** handling not to convert "1.1", "a.out", etc. ***/
		wdst[j]=JMARU;
		j++;
	    } else {
		wdst[j]=wbuf[i];
		j++;
	    }
	}
	wcstombs(dst, wdst, BUFSIZ);
}


/**************************************************************************
 *	convtojc_sub
 *		real function which convert hex to J-char.
 *		Rules:
 *		    1. it must be constructed from {a,b,c,d,e,f, 0-9}.
 *		    2. the value must NOT be in the following range because
 *		       there is no Japanese character in these ranges.
 *				0xa8c1 =< word =< 0xb0a0
 *				0xcfd4 =< word =< 0xd0a0
 *				word >= 0xf4a5
 *		New rule is added 2/28/92
 *		    * when the word starts with FLAG "____",
 *		      it means "this is an english word, do not
 *		      convert to japanese char.
 *************************************************************************/
convtojc_sub(dst, src)
char *dst;
char *src;
{
	char	buf[BUFSIZ];
	char	tmp[128];
#ifdef SVR4
	unsigned char	tmp2[3];
	unsigned short	lolo;
#else
	wchar_t	tmp2[128];
	wchar_t	lolo;
#endif
	wchar_t	wbuf[128];
	int	i,j;
	int	len;
	int	found_yuta=0;

	memset(buf, 0, BUFSIZ);
	memset(wbuf, 0, 128);
	len=strlen(src);
	for(i=0;i<len;i++) {
	    memset(tmp, 0, 128);

	    /* if it starts with "____", skip to next delimiter */
	    if ((src[i]=='_')&&(src[i+1]=='_')&&
		(src[i+2]=='_')&&(src[i+3]=='_')) {
		found_yuta=1;
		i+=4;		/* <- skip to the real head */
		while (found_yuta) {	/* copy until delimiter is found */
		    buf[strlen(buf)] = src[i];
		    i++;
		    if (strchr(Y_DELIMIT, src[i]))
			found_yuta = 0;
		}
	    /* Or, if first char is upper, also skip to the next. */
	    } else if (isupper(src[i])) {
		found_yuta=1;
		while (found_yuta) {	/* copy until delimiter is found */
		    buf[strlen(buf)] = src[i];
		    i++;
		    if (strchr(Y_DELIMIT, src[i]))
			found_yuta = 0;
		}
	    }

				/** "face" is conved... ouch!! **/
	    for(j=0;j<4;j++) {
		tmp[j]=src[i+j];
		if(((src[i+j]>='a')&&(src[i+j]<='f'))||
		    ((src[i+j]>='0')&&(src[i+j]<='9'))) {
		} else {
		    strncpy(&buf[strlen(buf)], tmp, j+1);
		    i+=j;
		    j=3;
		    continue;
		}
		if(j==3) {
#ifdef SVR4
		    lolo=(unsigned short)strtol(tmp, (char **)NULL, 16);
#else
		    lolo=(wchar_t)strtol(tmp, (char **)NULL, 16);
#endif
		    if ((tmp[0]>='0')&&(tmp[0]<='9')) {/* start with 0-9 ?*/
			strcpy(&buf[strlen(buf)], tmp, j);
			i+=j;
		    } else if (lolo && is_it_kanjiable(lolo)) {
			tmp2[0] = (unsigned char) (lolo/256);
			tmp2[1] = (unsigned char) lolo;
			tmp2[2] = 0;
/***
			memset(wbuf, 0, 128);
			wbuf[0]=lolo;
			memset(tmp2, 0, 128);
			wcstombs(tmp2, wbuf, sizeof(wchar_t));
***/
			strcat(buf, tmp2);
			i+=j;
		    }else{
			strcpy(&buf[strlen(buf)], tmp, j);
			i+=j;
		    }
		}
	    }
	}
	memset(dst, 0, BUFSIZ);
	strcpy(dst, buf);
}

is_it_kanjiable(word)
#ifdef SVR4
unsigned short	word;
#else
wchar_t word;
#endif
{
	if (((word>=0xa8c1)&&(0xb0a0>=word))||
	    ((word>=0xcfd4)&&(0xd0a0>=word))||
	    (word>=0xf4a5)) {
	    return(0);
	} else {
	    return(1);
	}
}
