#ident "@(#)utils.cc	1.3 06/11/93 Copyright 1992 Sun Microsystems, Inc."


#include <locale.h>
#include "navigator.h"


void
OutOfMemory()
{
	fprintf(stderr, gettext("Out of memory - exiting\n"));
	exit(1);
}
