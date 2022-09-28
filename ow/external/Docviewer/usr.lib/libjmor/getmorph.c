/*
 * Copyright (c) 1991 by Sun Microsystems, Inc.
 * Copyright (c) 1991 by Nihon Sun Microsystems K.K.
 */

#ident  "@(#)getmorph.c 1.28 94/09/06 SMI; JFP;"

/*********************************************************************
 *	These are routines which convert a Japanese sentences
 *	to English one.
 *	Each Japanese character is converted to its hex representation.
 *
 ********************************************************************/

#include <stdio.h>
#include <locale.h>
#include <widec.h>
#include <wctype.h>
#include "xctype.h"

#include "getmorph.h"

jmoranl(dst, src, is_it_mif)
char	*dst;
char	*src;
int	is_it_mif;
{
	wchar_t wsrc[BUFSIZ];
	wchar_t wdst[BUFSIZ];
	int	kekka;

	memset(wsrc, 0, BUFSIZ);
	memset(wdst, 0, BUFSIZ);
	mbstowcs(wsrc, src, BUFSIZ);
	mbstowcs(wdst, dst, BUFSIZ);
	kekka = jmor_analysis(wdst, wsrc, is_it_mif);
	if (kekka < 0)
	    return(kekka);
	memset(dst, 0, BUFSIZ);
	kekka = (int)wcstombs(dst, wdst, BUFSIZ);
	return(kekka);
}

/*********************************************************************
 * jmor_analysis
 *	morphological analysis for DV
 ********************************************************************/
jmor_analysis(dst, src, is_mif_file)
wchar_t	*dst;
wchar_t	*src;
int	is_mif_file;
{
	wchar_t	wbuf[BUFSIZ];
	wchar_t	src2[BUFSIZ];
	wchar_t	middle[BUFSIZ];
	wchar_t	middle2[BUFSIZ];
	wchar_t	wbuf2[BUFSIZ];
	wchar_t	wbuf3[BUFSIZ];
	wchar_t	wbuf4[BUFSIZ];
	wchar_t	wbuf5[BUFSIZ];
	int	in_navigator_flag=0;

	if (dst==NULL||src==NULL)
	    return(-1);

	memset(src2, 0, BUFSIZ);
	memset(middle, 0, BUFSIZ);
	memset(middle2, 0, BUFSIZ);
	memset(wbuf, 0, BUFSIZ);
	memset(wbuf2, 0, BUFSIZ);
	memset(wbuf3, 0, BUFSIZ);
	memset(wbuf4, 0, BUFSIZ);
	memset(wbuf5, 0, BUFSIZ);

#ifdef IN_NAV
	in_navigator_flag=1;	/* do not insert flag when it's in Nav. */
#else
	in_navigator_flag=0;
#endif
	insert_space(wbuf, src);

	if (in_navigator_flag || not_insert_ub) {
	    wscpy(middle2, wbuf);
	} else {
	    insert_4alnum_flag(middle2, wbuf);
	}

	eng_zen2han(wbuf2, middle2);
	delete_needless(wbuf3, wbuf2);
	replace_num(wbuf4, wbuf3);
	wspace_erase(wbuf5, wbuf4, is_mif_file);
	memset((char *)dst, 0, BUFSIZ);
	wscpy(dst, wbuf5);
#ifdef DEBUG
 	printf("insert flag -> [%ws]\n", src2);
 	printf("insert space -> [%ws]\n", wbuf);
 	printf("replace j2e -> [%ws]\n", middle2);
 	printf("eng zen2han  -> [%ws]\n", wbuf2);
 	printf("\ndelete need  -> [%ws]\n", wbuf3);
	printf("replace num  -> [%ws]\n", wbuf4);
#endif
	return(1);
}

/*********************************************************************
 * is_it_needed
 *      judge str[0] is needed or not
 *	current reguration for needed
 *		1. a word does not consist of hiragana only
 *		2. a word starts with the character "WBEGIN"
 *		3. a kanji word whose length is more than 2
 ********************************************************************/
is_it_needed(str)
wchar_t	*str;
{
#ifdef IN_NAV
	int	kekka;
	if (!isjhira(*str)||(*str == WBEGIN))
	    kekka=1;
	else if (isjkanji(*str)) {
	    if (wslen(str)>1)
		kekka=1;
	    else
		kekka=0;
	} else
	    kekka=0;
	return(kekka);
#else
	return(1);
#endif
}

/*********************************************************************
 * delete_needless
 *      delete needless word from src
 ********************************************************************/
delete_needless(dst, src)
wchar_t *dst;
wchar_t *src;
{
	wchar_t	wtmp[BUFSIZ];

	memset(wtmp, 0, BUFSIZ);
	memset(dst, 0, wslen(dst)*sizeof(wchar_t));
	while (*src) {
	    if (iswspace(*src) || (*src == WENDIN)) {
		if (wslen(wtmp) && is_it_needed(wtmp)) {
		    if (wtmp[0] == WBEGIN)
			wscat(dst, &wtmp[1]);
		    else
			wscat(dst, wtmp);
		    if (iswspace(*src))
			dst[wslen(dst)] = WSPACE;

		} else if ((wslen(dst) && !iswspace(dst[wslen(dst)-1])) ||
			   (!wslen(dst) && iswspace(*src))) {
		    dst[wslen(dst)] = WSPACE;
		}

		memset(wtmp, 0, BUFSIZ);

	    } else {
		wtmp[wslen(wtmp)] = *src;
	    }
	    *src++;
	}
	if (wslen(wtmp) && is_it_needed(wtmp)) {
	    if (wslen(dst) && !iswspace(dst[wslen(dst)-1]))
		dst[wslen(dst)] = WSPACE;
	    wscat(dst, wtmp);
	}
}

/*********************************************************************
 * insert_space
 *      insert white space between different type of chars
 ********************************************************************/
insert_space(dst, src)
wchar_t *dst;
wchar_t *src;
{
	int	i,current_char,before_char;
	int	len;
	int	donot_flag;

	donot_flag=0;
	current_char=before_char=0;
	len=wslen(src);
	memset(dst, 0, wslen(dst)*sizeof(wchar_t));
	for(i=0; i<len; i++) {
	    if (wslen(dst) >= (BUFSIZ/sizeof(wchar_t))-4) {
/********** When string might be over 1024, cut src.
		fprintf(stderr,
		    "String becomes over 1024 bytes, cut after %s\n", &src[i]);
**********/
		return(0);
	    }
	    if (src[i] == WBEGIN) {
		dst[wslen(dst)] = WSPACE;
		donot_flag=1;
	    } else if (src[i] == WENDIN)
		donot_flag=0;
	    else if (donot_flag==0) {
		before_char=current_char;
		current_char=get_char_type(src[i]);
		if (before_char!=current_char &&
		    before_char!=0 &&
		    /* current_char!=PUNCT && */

				/*** current/before must not be space ***/
		    src[i]!=WSPACE &&
		    (i>0 && src[i-1]!=WSPACE) &&

				/*** for "____SUNOS", etc. ***/
		    !(i>0 && src[i-1]==UNDERB &&
			(current_char==ALPHA || current_char==DIGIT))) {
		    dst[wslen(dst)] = WSPACE;
		}
	    }
	    dst[wslen(dst)] = src[i];
	}
}

/*********************************************************************
 * eng_zen2han
 *      replace ZENKAKU english to HANKAKU english
 ********************************************************************/
eng_zen2han(dst, src)
wchar_t *dst;
wchar_t *src;
{
	wchar_t *wptr;

	wptr = src;
	memset(dst, 0, wslen(dst)*sizeof(wchar_t));
	while(*wptr){
	    if(*wptr == (wchar_t)JTEN) {
		*wptr = ETEN;
	    } else if(*wptr == (wchar_t)JMARU) {
		*wptr = EMARU;
	    }
	    if(isjdigit(*wptr) || isjalpha(*wptr) ||
		isjspecial(*wptr) || iswspace(*wptr)) {
		dst[wslen(dst)] = jistoa(*wptr);

	    } else {
		wsncat(dst, wptr, 1);
/*		dst[wslen(dst)] = *wptr; */
	    }
	*wptr++;
	}
}

/*********************************************************************
 * replace_num
 *	replace the characters to the hex number which they stand for.
 *	we had better not use "wchar_t" and "wsprintf"..
 *	because the value is different between SunOS and SVR4
 *		-> rewrite for handling only "char/unsigned short".
 ********************************************************************/
replace_num(dst, src)
wchar_t *dst;
wchar_t *src;
{
	wchar_t wbuf[BUFSIZ];
	wchar_t *wptr;

	unsigned char tmp[128];

	wptr=src;
	memset(dst, 0, wslen(dst)*sizeof(wchar_t));
	while(*wptr) {
/********** when dst may be over 1024, cut after them **************/
	    if (wslen(dst)>=BUFSIZ) {
/******
		fprintf(stderr, "Over 1024! cut after %s\n", wptr);
*****/
		return(0);
	    }
	    memset(wbuf, 0, BUFSIZ);
	    if (isjis(*wptr)) {
		wctomb(tmp, *wptr);
		wsprintf(wbuf, "%x%x", tmp[0], tmp[1]);
		wscat(dst, wbuf);
	    } else {
		wsprintf(wbuf, "%c", *wptr);
		if (wslen(wbuf))	/** no code for such as "nakamaru" **/
		    wscat(dst, wbuf);
		else if (*(wptr+1)==WSPACE) /* not to insert two spaces */
		    *wptr++;
	    }
	*wptr++;
	}
}

/*********************************************************************
 * wspace_erase
 *	erase exessive spaces
 ********************************************************************/
wspace_erase(dst, src, is_it_mif)
wchar_t	*dst;
wchar_t *src;
int	is_it_mif;
{
	int	len;
	int	i;
	wchar_t	tmp[1024];
	wchar_t	y_flag[128];
	int	quote_flag=0;

	len=wslen(src);
	memset(tmp, 0, 1024);
	memset(y_flag, NULL, 128);
	memset(dst, 0, len*sizeof(wchar_t));

	mbstowcs(y_flag, Y_FLAG, strlen(Y_FLAG));

	for(i=0;i<len;i++) {
		/** in case of "____ 1234" **/
	    if ((!wsncmp(&src[i], y_flag, 4)) && src[i+4]==WSPACE) {
		wscpy(tmp, &src[i+5]);
		memset(&src[i+4], NULL, wslen(&src[i+4])*sizeof(wchar_t));
		wscpy(&src[i+4], tmp);
		memset(tmp, NULL, 1024);
		/** in case of ") " **/
	    } else if (src[i]==RPAREN||src[i]==HYPHN) {
		if (src[i+1]==WSPACE) {
		    wscpy(tmp, &src[i+2]);
		    memset(&src[i+1], NULL, wslen(&src[i+1])*sizeof(wchar_t));
		    wscpy(&src[i+1], tmp);
		    memset(tmp, 0, 1024);
		}
		/** in case of "\" abcde \"" **/
	    } else if ((src[i]==QUOTE||src[i]==D_QUOTE)&&quote_flag==0) {
		quote_flag=1;
		if (src[i+1]==WSPACE) {
		    wscpy(tmp, &src[i+2]);
		    memset(&src[i+1], NULL, wslen(&src[i+1])*sizeof(wchar_t));
		    wscpy(&src[i+1], tmp);
		    memset(tmp, 0, 1024);
		}
		/** in case of " ," **/
	    } else if (src[i]==WSPACE) {
		if (src[i+1]==COMMA||src[i+1]==PERIOD||src[i+1]==ASTER||
			src[i+1]==LPAREN||src[i+1]==HYPHN||src[i+1]==WSPACE||
			(i>0 && src[i-1]==HYPHN)) /* in case " - " */
		{
		    wscpy(tmp, &src[i+1]);
		    memset(&src[i], NULL, wslen(&src[i])*sizeof(wchar_t));
		    wscpy(&src[i], tmp);
		    memset(tmp, 0, 1024);
		} else if ((src[i+1]==QUOTE||src[i+1]==D_QUOTE)&&quote_flag) {
		    quote_flag=0;
		    wscpy(tmp, &src[i+1]);
		    memset(&src[i], NULL, wslen(&src[i])*sizeof(wchar_t));
		    wscpy(&src[i], tmp);
		    memset(tmp, 0, 1024);
		} else if (src[i+1]==QUOTE && is_it_mif) {
		    wscpy(tmp, &src[i+1]);
		    memset(&src[i], NULL, wslen(&src[i])*sizeof(wchar_t));
		    wscpy(&src[i], tmp);
		    memset(tmp, 0, 1024);
		}
	    }
	}
	wscpy(dst, src);
}

/*********************************************************************
 * get_char_type
 *	return character type.... ALPHA, DIGIT, etc.
 ********************************************************************/
get_char_type(wc)
wchar_t	wc;
{
	int	current_char;
	
	    if (isjalpha(wc)) {
		current_char=ALPHA;
	    } else 
	    if (isjkata(wc)) {
		current_char=KATA;
	    } else 
	    if (isjhira(wc)) {
		current_char=HIRA;
	    } else 
	    if (isjdigit(wc)) {
		current_char=DIGIT;
	    } else 
	    if (isjspace(wc)) {
		current_char=SPACE;
	    } else 
	    if (isjpunct(wc)) {
		current_char=PUNCT;
	    } else 
	    if (isjparen(wc)) {
		current_char=PAREN;
	    } else 
	    if (isjline(wc)) {
		current_char=LINE;
	    } else 
	    if (isjunit(wc)) {
		current_char=UNIT;
	    } else 
	    if (isjsci(wc)) {
		current_char=JSCI;
	    } else 
	    if (isjgen(wc)) {
		current_char=GEN;
	    } else 
	    if (isjkanji(wc)) {
		current_char=KANJI;
	    } else 
	    if (isjspecial(wc)) {
		current_char=SPEC;
	    } else 
	    if (isjgreek(wc)) {
		current_char=GREEK;
	    } else 
	    if (isjrussian(wc)) {
		current_char=RUSSIAN;
	    } else 
	    if (isjhankana(wc)) {
		current_char=HANKANA;
	    } else {
		current_char=OTHER;
	    }
	return (current_char);
}

/*********************************************************************
 *	insert_4alnum_flag
 *		Add "THIS_IS_ENGLISH_WORD,DONOT RECONVERT TO J-CHAR"
 *		flag before the word if it is over 4 letters 
 *		alphabet/digit.
 ********************************************************************/
insert_4alnum_flag(dst, src)
wchar_t	*dst;
wchar_t	*src;
{
	wchar_t	*wptr;
	int	not_yflag_yet;
	int	ar_num;

	wchar_t y_flg[128];
	wchar_t y_dlmt[128];
	wchar_t roffcntrl[128];

	mbstowcs(y_flg, Y_FLAG, strlen(Y_FLAG));
	mbstowcs(y_dlmt, Y_DELIMIT, strlen(Y_DELIMIT));
	mbstowcs(roffcntrl, ROFFCNTRL, strlen(ROFFCNTRL));

	/**** initialize flags and destination buffer ****/
	not_yflag_yet = 0;
	memset(dst, 0, BUFSIZ);

	wptr=src;
	while(*wptr) {
		/** insert Y_FLAG if word is over_4_alpha_or_digit **/
	    if ((ar_num=is_over4_alpha_num(wptr)) && !not_yflag_yet) {
		not_yflag_yet = 1;
		wscat(dst, y_flg);
		wsncat(dst, wptr, ar_num-1);   /* ar_num is a number of word */
		wptr+=(ar_num-1); /* for the last char will be put in dst
					later. see "HERE HERE" */

		/** set flag 0 when delimiters appear **/
	    } else if (wschr(y_dlmt, *wptr)) {
		not_yflag_yet = 0;

		/** donot insert y_flg after troff control chars ("\", etc) **/
	    } else if (wschr(roffcntrl, *wptr)) {
		not_yflag_yet = 1;
	    } else if (not_yflag_yet) {
		not_yflag_yet = 0;
	    }
		dst[wslen(dst)] = *wptr;	/* HERE HERE */
		wptr++;
	}
#ifdef DEBUG
	printf("%ws\n", dst);
#endif
}

/*********************************************************************
 * is_over4_alpha_num
 *			judge over 4 letters alphabet/digit word or not
 *			if YES, returns the number of the word,
 *			otherwise returns 0.
 ********************************************************************/
int
is_over4_alpha_num(str)
wchar_t	*str;
{
	int	i, len;
	len = wslen(str);

	for (i=0;i<len;i++) {
	    if (!iswalnum(str[i]) && str[i]!=UNDERB)
		break;
	}
	if (i>=4)
	    return(i);
	else
	    return(0);
}

