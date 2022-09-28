#ifndef	_XOL_OLI18NP_H
#define	_XOL_OLI18NP_H

#pragma	ident	"@(#)OlI18nP.h	302.1	92/03/26 include/Xol SMI"	/* OLIT	*/

/*
 *	Copyright (C) 1986,1991  Sun Microsystems, Inc
 *			All rights reserved.
 *		Notice of copyright on this source code
 *		product does not indicate publication.
 *
 * RESTRICTED RIGHTS LEGEND: Use, duplication, or disclosure by
 * the U.S. Government is subject to restrictions as set forth
 * in subparagraph (c)(1)(ii) of the Rights in Technical Data
 * and Computer Software Clause at DFARS 252.227-7013 (Oct. 1988)
 * and FAR 52.227-19 (c) (June 1987).
 *
 *	Sun Microsystems, Inc., 2550 Garcia Avenue,
 *	Mountain View, California 94043.
 *
 */


#ifdef	__cplusplus
extern "C" {
#endif


/*
 * PLUS:	the separator between modifier names in accelerator text
 * LBRA:	left bracket, the character that begins a string KeySym
 * RBRA:	right bracket, the character that ends a string KeySym
 * S_SEPS:	the character(s) that separate modifiers in a string
 */
#define PLUS		('+')
#define MINUS		('-')
#define LPAR		('(')
#define RPAR		(')')
#define LBRA		('<')
#define RBRA		('>')
#define SEP		(',')
#define DEF_SEPS	(",")
#define MOD_SEPS	("!~ ")
#define S_SEPS		(" ")
#define WORD_SEPS	(" \t\n") /* Delimiters for words in a resource file. */
#define DECPOINT	('.')
#define DIGIT(x)	((long)((x) - '0'))


#ifdef	__cplusplus
}
#endif


#endif	/* _XOL_OLI18NP_H */
