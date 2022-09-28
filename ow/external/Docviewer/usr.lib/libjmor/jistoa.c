/*
 * Copyright (c) 1991, Sun Microsystems, Inc.
 * Copyright (c) 1991, Nihon Sun Microsystems K.K.
 */

#ident	"@(#)jistoa.c	1.1	93/01/27	SMI"
 
#include <widec.h>
#define MAX_KANA	63
#define MAX_ALPHA	95

extern wchar_t	_kana_to_jis();
extern wchar_t	_ascii_to_jis();

jistoa(c)
int     c;
{
        int     i;
        for (i=0;i<MAX_KANA;i++)
                if (c == _kana_to_jis(i))
                        return (i + 0x21 + WCHAR_CS2);
        for (i=0;i<MAX_ALPHA;i++)
                if (c == _ascii_to_jis(i))
                        return (i + 0x20);
        return (c);
}
