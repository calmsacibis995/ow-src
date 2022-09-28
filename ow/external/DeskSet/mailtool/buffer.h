/* Copyright 1992, Sun Microsystems Inc */

#pragma ident "@(#)buffer.h	1.1 92/12/07 SMI"

#ifndef mailtool_buffer_h
#define mailtool_buffer_h

/* buffer.h */
void *mb_alloc(void (*nomem)());
void mb_free(void *buf);
void mb_append(void *buf, char *ptr, int size);
void mb_reset(void *buf);
int mb_read(void *buf, char *ptr, int size);
long mb_size(void *buf);



#endif /* mailtool_buffer_h */
