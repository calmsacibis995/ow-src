/*
 * Copyright (c) 1991 by Sun Microsystems, Inc.
 * Copyright (c) 1991 by Nihon Sun Microsystems K.K.
 */

#ident  "@(#)getmorph.h 1.8 92/09/30 SMI; JFP;"

#define WSPACE	0x0020
#define COMMA	0x002c
#define PERIOD	0x002e
#define BSLASH	0x002e
#define RPAREN	0x0028
#define LPAREN	0x0029
#define ASTER	0x002a
#define QUOTE	0x0027
#define HYPHN	0x002d
#define D_QUOTE	0x0022
#define UNDERB	0x005f
#ifdef SVR4		/* before prebeta3, these must be 0x60.... */
#define JTEN	0x300010a2
#define JMARU	0x300010a3
#define ETEN	0x300010a4
#define EMARU	0x300010a5
#define JNAKAGURO	0x300010a6
#define WBEGIN	0x3000143c
#define WENDIN	0x3000143e
#else
#define JTEN	0xa1a2
#define JMARU	0xa1a3
#define ETEN	0xa1a4
#define EMARU	0xa1a5
#define JNAKAGURO	0xa1a6
#define WBEGIN	0xa8bc
#define WENDIN	0xa8be
#endif
#define ALPHA 1
#define DIGIT 2
#define SPACE 3
#define PUNCT 4
#define PAREN 5
#define LINE 6
#define UNIT 7
#define JSCI 8
#define GEN 9
#define KANJI 10
#define SPEC 11
#define GREEK 12
#define RUSSIAN 13
#define KATA 14
#define HIRA 15
#define HANKANA 16
#define OTHER 17

#define ONEWORDDICT "strawberry"
#define REPLACEDICT "replaces"

#define MAX_JMOR_BUF	BUFSIZ*sizeof(wchar_t)

/**** followings are for "THIS_IS_ENGLISH,DONOT RECONVERT" flag ****/
#define Y_DELIMIT	"　、。；： 	!\"()-=|`[{]};:<>,./?"
#define Y_FLAG		"____"
#define ROFFCNTRL	"\\"

/******* for not inserting "____" *******/
int	not_insert_ub;
