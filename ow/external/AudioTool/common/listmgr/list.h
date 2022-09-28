/* Copyright (c) 1992 by Sun Microsystems, Inc. */

#ifndef _LIST_H
#define _LIST_H

#ident	"@(#)list.h	1.5	92/10/14 SMI"

#include <stdio.h>
#include <ctype.h>
#include <sys/types.h>

/* linked list type */

typedef struct _link
{
	char 	     *label;		/* label (how it'll appear in list) */
	caddr_t	     dp;		/* list data */
	struct _link *next,*prev;	/* pointer to next link */
} Link;

typedef struct _list
{
    Link *head;			/* pointer to head of list */
    Link *tail;			/* pointer to tail of list */
    Link *lastlink;		/* last link visited */
    int lastind;		/* last index visted */
    int num;			/* number of elems currently in list */
    int maxlinks;		/* number of links alloc'd */
    int (*cmpfcn)();		/* compare fcn */
    caddr_t (*encfcn)();	/* encode fcn */
    caddr_t (*decfcn)();	/* decode fcn */
    int (*prtfcn)();		/* print fcn */
    caddr_t (*getlabel)();	/* get label */
    caddr_t (*cpyfcn)();	/* copy data */
    int (*delfcn)();		/* delete fcn */
    caddr_t (*newfcn)();	/* create new object */
} List;

/* initialize new linked list */
extern List *NewList(int 	(*cmpfcn) (/* ??? */),
		     caddr_t	(*decfcn) (/* ??? */),
		     caddr_t	(*getlabel) (/* ??? */),
		     caddr_t	(*encfcn) (/* ??? */),
		     int	(*prtfcn) (/* ??? */),
		     caddr_t	(*cpyfcn) (/* ??? */),
		     int	(*delfcn) (/* ??? */),
		     caddr_t	(*newfcn) (/* ??? */));
extern Link *NewLink(List *list); /* returns an initialized ptr to link */
extern Link *GetLink(List *list, int num); /* get n'th link from list */
extern Link *FindLink(List *list, char *s); /* find n'th link given label */
extern List *CopyList(List *list); /* copy an entire list */
/* return a copy of the link data ptr */
extern caddr_t CopyLinkData(List *list, Link *link); 
/* add link to list */
extern int AppendLink(List *list, char *label, caddr_t dp); 
extern int ClearList(List *list);		/* init the list */
extern int DeleteLink(List *list, int num);	/* delete a link */
extern void DumpList(List *list); /* print all lines in list to stdout */
/* append to list from file */
extern int AppendListFromFile(List *list, char *fname, int aflag); 
/* write the list to a file */
extern int WriteListToFile(List *list, char *fname);
extern int ReplaceLink();	/* replace a link */
extern int ChangeLinkLabel(List *list, Link *lp, char *label);

extern char *getfld(char *s, char c, char *buf, int len);

/* macro to help us along with reading list files w/fields */
#define GETSFIELD(s, fld, buf, len)  buf[0]='\0'; s=getfld(s,':',buf,len);\
    if (buf[0]) fld = strdup(buf); else fld = NULL;

#define GETIFIELD(s, fld, buf, len)  buf[0]='\0'; s=getfld(s,':',buf,len);\
    if (buf[0]) fld = atoi(buf); else fld = 0;

#endif /* _LIST_H */

