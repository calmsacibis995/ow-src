#ifndef	_NEWS_PARSE_H
#define _NEWS_PARSE_H


#ident "@(#)parse.h	1.2 06/11/93 NEWS SMI"


/*
 * parse.h - Definitions for the Postscript parser.
 * Copyright (c) 1991 by Sun Microsystems, Inc.
 */

#define INIT_PARSE_BUF_SIZE 64

struct parse_state {
    int p_arrayi;
    int p_stringi;
    int p_currenti;
    int p_state;
    int p_action;
    int p_magic;
    int p_count;
    int p_bufsize;
    unsigned char p_buffer[INIT_PARSE_BUF_SIZE];
};

#endif /* _NEWS_PARSE_H */
