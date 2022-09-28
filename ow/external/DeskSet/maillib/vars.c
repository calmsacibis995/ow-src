/* @(#)vars.c	3.1 - 92/04/03 */

/*	Copyright (c) 1987, 1988, Sun Microsystems, Inc.  All Rights Reserved.
	Sun considers its source code as an unpublished, proprietary
	trade secret, and it is available only under strict license
	provisions.  This copyright notice is placed here only to protect
	Sun in the event the source is deemed a published work.  Dissassembly,
	decompilation, or other means of reducing the object code to human
	readable form is prohibited by the license agreement under which
	this code is provided to the user or company in possession of this
	copy.

	RESTRICTED RIGHTS LEGEND: Use, duplication, or disclosure by the 
	Government is subject to restrictions as set forth in subparagraph 
	(c)(1)(ii) of the Rights in Technical Data and Computer Software 
	clause at DFARS 52.227-7013 and in similar clauses in the FAR and 
	NASA FAR Supplement. */

/*
 * Mailtool - variable handling, stolen from Mail
 */

#include <stdio.h>
#include <sys/types.h>
#include "ck_strings.h"
#include "debug.h"

char	*calloc();
static	void vfree(), mt_puthash();

/*
 * Structure of a variable node.  All variables are
 * kept on a singly-linked list of these, rooted by
 * "variables"
 */
struct var {
	struct	var *v_link;		/* Forward link to next variable */
	char	*v_name;		/* The variable's name */
	caddr_t	v_value;		/* And it's current value */
};

#define	HSHSIZE	19
static struct	var *variables[HSHSIZE];	/* Pointer to active var list */

char	*mt_value();
static char	*vcopy();
static struct	var *lookup();

/*
 * Assign a value to a mail variable.
 */
void
mt_assign(name, val)
	char name[], val[];
{

	if (name[0]=='-')
		(void) mt_deassign(name+1);
	else if (name[0]=='n' && name[1]=='o')
		(void) mt_deassign(name+2);
	else if ((val[0] == 'n' || val[0] == 'N') &&
		 (val[1] == 'o' || val[1] == 'O') && val[2] == '\0') {
		(void) mt_deassign(name);
	} else mt_puthash(name, vcopy(val), variables);
}

/*
 * associate val with name in hasharray
 */
static void
mt_puthash(name, val, hasharray)
	char name[];
	caddr_t	val;
	struct	var *hasharray[];
{
	register struct var *vp;
	register int h;

	vp = lookup(name, hasharray);
	if (vp == (struct var *)NULL) {
		h = hash(name);
		vp = (struct var *) (calloc(sizeof *vp, 1));
		vp->v_name = vcopy(name);
		vp->v_link = hasharray[h];
		hasharray[h] = vp;
	} else
		vfree(vp->v_value);
	vp->v_value = val;
}

int
mt_deassign(s)
	register char *s;
{
	register struct var *vp, *vp2;
	register int h;

	if ((vp2 = lookup(s, variables)) == (struct var *)NULL) {
		return (1);
	}
	h = hash(s);
	if (vp2 == variables[h]) {
		variables[h] = variables[h]->v_link;
		vfree(vp2->v_name);
		vfree(vp2->v_value);
		cfree((char *)vp2);
		return (0);
	}
	for (vp = variables[h]; vp->v_link != vp2; vp = vp->v_link)
		;
	vp->v_link = vp2->v_link;
	vfree(vp2->v_name);
	vfree(vp2->v_value);
	cfree((char *)vp2);
	return (0);
}

/*
 * Free up a variable string.  We do not bother to allocate
 * strings whose value is "" since they are expected to be frequent.
 * Thus, we cannot free same!
 */
static void
vfree(cp)
	register char *cp;
{

	if (strcmp(cp, "") != 0)
		cfree(cp);
}

/*
 * Copy a variable value into permanent space.
 * Do not bother to alloc space for "".
 */
static char *
vcopy(str)
	char str[];
{

	if (strcmp(str, "") == 0)
		return ("");
	return (ck_strdup(str));
}

/*
 * Get the value of a variable and return it.
 * Look in the environment if its not available locally.
 */
char *
mt_value(name)
	char name[];
{
	register struct var *vp;
	register char *cp;
	extern char *getenv();

	if ((vp = lookup(name, variables)) == (struct var *)NULL)
		cp = getenv(name);
	else
		cp = vp->v_value;
	return (cp);
}

/*
 * Locate a variable and return its variable
 * node.
 */
static struct var *
lookup(name, hasharray)
	char name[];
	struct	var *hasharray[];

{
	register struct var *vp;
	register int h;

	h = hash(name);
	for (vp = hasharray[h]; vp != (struct var *)NULL; vp = vp->v_link)
		if (strcmp(vp->v_name, name) == 0)
			return (vp);
	return ((struct var *)NULL);
}

/*
 * Hash the passed string and return an index into
 * the variable or group hash table.
 */
static int
hash(name)
	char name[];
{
	register unsigned h;
	register char *cp;

	for (cp = name, h = 0; *cp; h = (h << 2) + *cp++)
		;
	return (h % HSHSIZE);
}
