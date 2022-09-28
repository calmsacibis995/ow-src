/*
 * Copyright (c) 1991, Sun Microsystems, Inc.
 * Copyright (c) 1991, Nihon Sun Microsystems K.K.
 */

#pragma ident "@(#)mclist.h	1.3 95/02/22"

#define	CN_MAXLOCALELEN		 32
#define	CN_MAXINTCODELEN	 32
#define	CN_MAXEXTCODELEN	 32
#define	CN_MAXCOMMENTLEN	128

#define	CN_DELMITERS	" \t\n"

struct mcent {
	char *cn_locale;
	char *cn_intcode;
	char *cn_extcode;
	int   cn_flag;
	char *cn_comment;
};

extern struct mcent *getmclist();
extern void freemclist(struct mcent* cn);
