/* Copyright (c) 1991 by Sun Microsystems, Inc. */
#ident	"@(#)atool_debug.c	1.1	91/07/10 SMI"

#include <stdio.h>
#include <stdarg.h>

#include "atool_debug.h"

static int debug_level = 0;

/* debugging code */

void
set_debug_level(int level)
{
	debug_level = level;
}

void
_dbgprintf(int level, char *format, ...)
{
    va_list ap;

    if (level > debug_level)
	return;

    va_start(ap, format);

    _doprnt(format, ap, stderr);
    va_end(ap);
}

