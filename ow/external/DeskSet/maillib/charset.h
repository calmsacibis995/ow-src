
/* @(#)charset.h	3.4 - 94/07/27 */

/* Copyright (C) 1992, Sun Microsystems Inc.  All Rights Reserved */

/* charset.h */


#ifndef	__charset_h__
#define	__charset_h__

#include "msg.h"
#include "attach.h"

struct __charset_methods {
	void (*cs_init)();
	void *(*cs_translation_required)(struct msg *m, char **intcode);
	int (*cs_copy)(void *iconvinfo, int (*func)(), char *buf, int len, 
		int param);
	int (*cs_copy2)(void *iconvinfo, int (*func)(), char *buf, int len, 
		int param);
	int (*cs_msg_label)(struct msg *m, void **info);
};

extern struct __charset_methods cs_methods;

#endif __charset_h__

