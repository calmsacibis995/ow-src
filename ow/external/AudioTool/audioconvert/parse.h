/* Copyright (c) 1992 by Sun Microsystems, Inc. */

#ifndef _AUDIOCONVERT_PARSE_H
#define	_AUDIOCONVERT_PARSE_H

#ident	"@(#)parse.h	1.6	92/12/05 SMI"

typedef enum {
	K_NULL = 0, K_ENCODING = 1, K_FORMAT, K_RATE, K_CHANNELS, 
	K_OFFSET, K_INFO, K_AMBIG = -1
} keyword_type;

typedef enum {
	F_RAW = 0, F_SUN = 1, F_AIFF, F_UNKNOWN = -1
} format_type;

struct keyword_table {
	char*		name;
	keyword_type	type;
};

extern int	parse_format(char*, AudioHdr&, format_type&, off_t&);

#endif /* !_AUDIOCONVERT_PARSE_H */
