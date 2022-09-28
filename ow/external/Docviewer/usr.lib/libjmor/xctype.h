/*
 * This is the extension of wctype.h which defines international
 * classification functions for multi byte character.
 *
 * If you want to use multi byte character classification 
 * for your language,additional functions can be defined here.
 *
 * This file is automatically included by <jctype.h>.
 */

#ident  "@(#)xctype.h 1.1 93/01/27 SMI; JFP;"
/*	from AT&T JAE 2.1	*/

#ifndef _XCTYPE_H
#define _XCTYPE_H
#include <widec.h>

/* Beginning of Japanese-specific definitions */
#define isjis(c)	_iswctype((c), _E9)
#define isj1bytekana(c) _iswctype((c), _E14)
#define isjhankana(c)	isj1bytekana(c)

#define isjparen(c)	_iswctype((c), _E10)
#define	isjhira(c)	_iswctype((c), _E11)
#define isjkata(c)	_iswctype((c), _E12)
#define isjgreek(c) 	_iswctype((c), _E13)
#define isjrussian(c)   _iswctype((c), _E15)
#define isjline(c)	_iswctype((c), _E16)	/* Ruled line symbols */
						/* i.e. Kei-sen. */
#define isjunit(c)	_iswctype((c), _E17)
#define isjsci(c)	_iswctype((c), _E18)	/* Scientific symbols.*/
#define isjgen(c)	_iswctype((c), _E19)	/* General symbols.*/
#define isjspecial(c)	_iswctype((c), _E13 | _E15)

#define isjspace(c)	_iswctype((c), _S)	/* JIS space. */
#define isjdigit(c)	_iswctype((c), _N)	/* JIS numeric. */
#define isjpunct(c)	_iswctype((c), _E20)
#define isjupper(c)	_iswctype((c), _U)
#define isjlower(c)	_iswctype((c), _L)
#define isjkanji(c)	_iswctype((c), _E2)
#define isjalpha(c)	_iswctype((c), _E3)

#define toujis(c)	((c) & (WCHAR_CSMASK | WCHAR_S_MASK | \
			(WCHAR_S_MASK << WCHAR_SHIFT) | \
			(WCHAR_S_MASK << (WCHAR_SHIFT * 2))))

#define _tojlower(c)	((c) + 0x0020)
#define _tojupper(c)	((c) - 0x0020)

/* 0xa1BC is Japanese 'CHOUON KIGOU'. */

#define _CHOUON_KIGOU	(WCHAR_CSMASK | ((WCHAR_S_MASK & 0xa1) << WCHAR_SHIFT) \
			| (WCHAR_S_MASK & 0xbc))
#define _tojhira(c)	((c) == _CHOUON_KIGOU ? (c) : (c) - (0x01 << WCHAR_SHIFT))
#define _tojkata(c)	((c) == _CHOUON_KIGOU ? (c) : (c) + (0x01 << WCHAR_SHIFT))

#define tojupper(c)	(((c) > 127) ? (( _iswctype((c),_L) ?  \
				_trwctype((c),_L) : (c))) : (c))

#define tojlower(c)	(((c) > 127) ? (( _iswctype((c),_U) ?  \
				_trwctype((c),_U) : (c))) : (c))

extern wchar_t _kana_to_jis[];
extern wchar_t _ascii_to_jis[];
#define _atojis(c)	(((c) & WCHAR_CSMASK ) ? \
	_kana_to_jis[((c) & WCHAR_S_MASK) - '!'] : _ascii_to_jis[(c) - '!'] )

extern atojis(), kutentojis(), tojhira(), tojkata();
/* end of Japanese-specific definitions */

#endif /*!_XCTYPE_H*/
