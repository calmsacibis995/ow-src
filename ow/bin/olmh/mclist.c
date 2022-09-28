/*
 * Copyright (c) 1991, Sun Microsystems, Inc.
 * Copyright (c) 1991, Nihon Sun Microsystems K.K.
 */

#pragma ident "@(#)mclist.c	1.4 95/02/22"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/param.h>
#include "mclist.h"

#define	CODETABLE "/usr/lib/locale/%s/LC_CTYPE/mailcodelist"

#define	min(a, b) ((a)>(b) ? (b) : (a))
#define	CNALLOC1(p1, n1) ((char *)XtMalloc(min((int)strlen(p1), n1)))
#define CNFREE1(p1) if (p1 != NULL) free(p1)



#pragma weak getmclist = _getmclist

struct mcent *
_getmclist(char *loc)
{
	FILE *fp;
	char linebuf[BUFSIZ];
	char *p, *q, *delp, *ep;
	struct mcent *chead, *cp;
	int i, n;
	char codetable[MAXPATHLEN];

	sprintf(codetable, CODETABLE, loc);

	if ((fp = fopen(codetable, "r")) == NULL)
		return (NULL);

	n = 0;
	while (fgets(linebuf, BUFSIZ, fp) != NULL)
		n++;

	if ((chead = (struct mcent *)XtCalloc(n + 1, sizeof(struct mcent))) 
								== NULL) {
		fclose(fp);
		return (NULL);
	}

	rewind(fp);
	delp = CN_DELMITERS;
	i = 0;
	while (fgets(linebuf, BUFSIZ, fp) != NULL) {
		p = strtok(linebuf, delp);
		cp = &chead[i];
		if((chead[i].cn_locale = (char *)XtNewString(p)) == NULL){
			freemclist(chead);
			fclose(fp);
			return (NULL);
		}

		p = strtok(NULL, delp);
		if((chead[i].cn_intcode = (char *)XtNewString(p)) == NULL){
			freemclist(chead);
			fclose(fp);
			return (NULL);
		}

		p = strtok(NULL, delp);
		if((chead[i].cn_extcode = (char *)XtNewString(p)) == NULL){
			freemclist(chead);
			fclose(fp);
			return (NULL);
		}

		p = strtok(NULL, delp);
		if (p == NULL) {
			freemclist(chead);
			fclose(fp);
			return (NULL);
		}
		chead[i].cn_flag = atoi(p);

		q = p + strlen(p) + 1;
		p = q + strspn(q, delp);
		if ((q = strpbrk(p, "\n")) != NULL)
			*q = '\0';
		if((chead[i].cn_comment = (char *)XtNewString(p)) == NULL){
			freemclist(chead);
			fclose(fp);
			return (NULL);
		}

		if (++i > n)
			break;
	}
	
	chead[n].cn_locale = NULL;
	fclose(fp);
	return (chead);
}


#pragma weak freemclist = _freemclist

void
_freemclist(struct mcent* chead)
{
	struct mcent *cp;
	int i;

	if(chead == NULL)
		return;
	i = 0;
	while (chead[i].cn_locale != NULL) {
		cp = &chead[i];
		CNFREE1(cp->cn_locale);
		CNFREE1(cp->cn_intcode);
		CNFREE1(cp->cn_extcode);
		CNFREE1(cp->cn_comment);
		i++;
	}

	free(chead);
}
