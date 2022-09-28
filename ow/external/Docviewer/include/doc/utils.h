#ifndef	_UTILS_H
#define	_UTILS_H

#ident "@(#)utils.h	1.7 93/02/24 Copyright 1992 Sun Microsystems, Inc."


#include <doc/common.h>


// Read a "logical line" from a file into "linestr".
// Handles arbitrarily long lines.
// A line beginning with "#" is treated as a comment and ignored.
// A line ending with "\" is concatenated with the subsequent line.
//
// Returns reference to "linestr", or NULL_STRING if no more lines.
//
const STRING	&GetLine(FILE *fp, STRING &linestr);

// Initialize specified text domain for "gettext()", etc.
//
void		InitTextDomain(const STRING &domain_name);

#endif	_UTILS_H
