/* Copyright (c) 1992 by Sun Microsystems, Inc. */
#ident	"@(#)list.c	1.6	92/10/23 SMI"

#include "list.h"

/* NEWLIST: Return a pointer a new list structure. */

#include <string.h>
#include <ctype.h>

List *
NewList(int (*cmpfcn) (/* ??? */),	/* compare fcn */
	caddr_t	(*decfcn) (/* ??? */),	/* decode fcn */
	caddr_t	(*getlabel) (/* ??? */), /* get data label (char*) from dp */
	caddr_t	(*encfcn) (/* ??? */),	/* encode fcn */
	int	(*prtfcn) (/* ??? */),	/* print fcn */
	caddr_t	(*cpyfcn) (/* ??? */),	/* copy fcn */
	int	(*delfcn) (/* ??? */),	/* delete fcn */
	caddr_t	(*newfcn) (/* ??? */))	/* create a new object */
{
    List *list;

    list = (List *) calloc (1, sizeof (List));

    list->cmpfcn = cmpfcn;
    list->encfcn = encfcn;
    list->getlabel = getlabel;
    list->decfcn = decfcn;
    list->prtfcn = prtfcn;
    list->cpyfcn = cpyfcn;
    list->delfcn = delfcn;
    list->newfcn = newfcn;

    return list;
}

int AppendLink(List *list, char *label, caddr_t dp)
{
    if (list->num == 0)	{
	    if (!list->head) {
#ifdef DEBUGxx
		    fprintf(stderr,"Creating new Head link\n");
#endif
		    if (!(list->tail = list->head = NewLink(list)))
			    return -1;
	    }
    } else {
	    /* add onto tail (assume we have one!) */
	    if (!list->tail->next) {
		    /* no next ptr, yet */
#ifdef DEBUGxx
		    fprintf(stderr,"Adding new tail link\n");
#endif
		    if (!(list->tail->next = NewLink(list))) {
			    return -1;
		    }
	    }
	    list->tail->next->prev = list->tail;
	    list->tail = list->tail->next; /* move to next link */
    }
    
    list->tail->dp = dp;
    if (list->tail->label) {
	    free(list->tail->label);
    }
    list->tail->label = strdup(label);

    list->num++;

    return 0;
}

/*
 * insert a link at the specified position (i.e. before the one currently at
 * that position).
 */
int InsertLink(List *list, char *label, caddr_t dp, int pos)
{
	Link *l, *nl;

	if (!(l = GetLink(list, pos))) {
		return (AppendLink(list, label, dp));
	}

	if (!(nl = NewLink(list))) {
		return (-1);
	}

	if (l->prev) {
		l->prev->next = nl;
		nl->next = l;
		nl->prev = l->prev;
		l->prev = nl;
	} else {
		nl->prev = NULL;
		nl->next = l;
		l->prev = nl;
		list->head = nl;
	}

	nl->dp = dp;
	nl->label = strdup(label);

	list->num++;

	list->lastlink = nl;
	list->lastind = pos;

	return pos;
}

List *
CopyList(List *list)
{
	List *nlist;
	Link *link;
	int i;
	caddr_t dp;

	if (!list) {
		return(NULL);
	}

	if (!list->cpyfcn) {
		fprintf(stderr, "list: no cpyfcn, can't copy list\n");
		return(NULL);
	}

	nlist = NewList(list->cmpfcn, list->decfcn, list->getlabel,
			list->encfcn, list->prtfcn, list->cpyfcn,
			list->delfcn, list->newfcn);

	for (i=0, link=list->head; (i<list->num) && link; 
	     link=link->next, i++) {
		if (dp = list->cpyfcn(list, link->dp)) {
			AppendLink(nlist, list->getlabel(nlist, dp), dp);
		}
	}
	return(nlist);
}

/* return a copy of the link data */
caddr_t
CopyLinkData(List *list, Link *link)
{
	if (!list->cpyfcn)
		return(NULL);
	return (list->cpyfcn(list, link->dp));
}

/* 
 * CLEARLIST: Clear a list for re-use. free all data associated w/list, but
 * don't clear list since we can re-use the link ptr's 
 */

int ClearList(List *list)
{
	Link *link;
	int i;

	if (!list) {
		return -1;
	}

	if (list->delfcn) {
		for(i=0, link=list->head; link && (i < list->num);
		    link = link->next, i++) {
			if (link->dp) {
				list->delfcn(list, link->dp);
			}
			link->dp = NULL;
		}
	}

	list->num = 0;		/* zero out the counter */
	list->tail = list->head; /* tail points to head */
	list->lastlink = NULL;
	list->lastind = 0;
	return 0;
}

int DeleteList(List *list)
{
	Link *link, *nlink;
	int i;

	if (!list) {
		return -1;
	}

	for(i=0, link=list->head; link && (i < list->maxlinks); i++) {
		if (list->delfcn && link->dp) {
			list->delfcn(list, link->dp);
		}
		if (link->label) {
			free(link->label);
		}
		nlink = link->next;
		free(link);
		link = nlink;
	}
	free(list);
	return 0;
}

/*
 * Get N'th element from a list (kinda inefficient), returns
 * pointer to link, or NULL if none there.
 */

Link *
GetLink(List *list, int num)
{
    register Link *l;
    register i;
    int backward = 0;		/* if we search backward */

    if (num >= list->num)
      return NULL;

    
    /* first check easy cases */

    if (list->lastlink && (list->lastind == num))
      {
#ifdef STATS
	  printf("found link %d as lastlink\n", num);
#endif
	  return list->lastlink;
      }
    
    if (num == 0)
      {
#ifdef STATS
	  printf("found link %d as head\n", num);
#endif
	  return list->head;
      }
    
    if (num == (list->num-1))
      {
#ifdef STATS
	  printf("found link %d as tail\n", num);
#endif
	  return list->tail;
      }
    
    /* figure out starting point */

    if (list->lastind && list->lastlink)
      {
	  if (num < list->lastind)
	     {
		 i = ((list->lastind - num) > num) ? 0 : list->lastind;
		 backward = i;
		 l = (backward ? list->lastlink : list->head);
#ifdef STATS
		 printf("link %d - searching %s from %d (%s) (%d iter's)\n",
			num, backward ? "backward" : "forward", i,
			l == list->lastlink ? "lastlink" : "head",
			backward ? (i - num) : (num - i));
#endif
	     }
	  else    
	    {
		i = (( list->num-1 - num) > (num - list->lastind) 
		     ? list->lastind : list->num-1);
		backward = (i == (list->num-1));
		l = (backward ? list->tail : list->lastlink);
#ifdef STATS
		 printf("link %d - searching %s from %d (%s) (%d iter's)\n",
			num, backward ? "backward" : "forward", i, 
			l == list->lastlink ? "lastlink" : "tail",
			backward ? (i - num) : (num - i));

#endif
	    }
      }
    else
      {
	  i = (num < (list->num / 2)) ? 0 : (list->num - 1);
	  backward = i;
	  l = (backward ? list->tail : list->head);
#ifdef STATS
	  printf("link %d - searching %s from %d (%s) (%d iterations)\n",
		 num, backward ? "backward" : "forward", i,
		 l == list->head ? "head" : "tail",
		 backward ? (i - num) : (num - i));
#endif
      }

    /* now that we figured out where to start and which direction to go... */

    if (backward) {
	    for (; i > num; i--, l = l->prev ) {
		    continue;
	    }
    } else {
	    for (; i < num; i++, l = l->next ) {
		    continue;
	    }
    }
    
    list->lastind = i;
    list->lastlink = l;

    return (l);
}

/* NEWLINK: Allocate storage for a new link, and initialize its
   values to NULL */

Link *
NewLink(List *list)
{
    Link *l;
    
    if (!(l = (Link *) malloc (sizeof(Link)))) {
	    fprintf(stderr,"*Fatal* malloc() failed in NewLink()");
	    exit(1);
    }

    list->maxlinks++;		/* increment max no. of links */

#ifdef VERBOSE
    printf("Adding New Link, count = %d.\n", list->maxlinks);
#endif

    l->next = NULL;
    l->prev = NULL;
    l->dp = NULL;
    l->label = NULL;

    return l;
}


/* delete a link from the list. return -1 upon failure. */

int DeleteLink(List *list, int num)
{
    register Link *l, *nl, *pl;
    
    if (!(l = GetLink(list, num))) /* ain't in there! */
	return -1;

    if (nl = l->next) {
	    nl->prev = l->prev;
    }
    if (pl = l->prev) {
	    pl->next = nl;
    }
    if (list->head == l) {
	    list->head = nl;
    } else if (list->tail == l) {
	    list->tail = pl;
    }
    if (list->lastlink == l) {	/* if we deleted last link visited */
	    if (nl) {		/* if there's a next link, set to that */
		    list->lastlink = nl;
	    } else {
		    list->lastlink = pl; /* if not, set to prev & decrement */
		    list->lastind--;
	    }
    } else if (num < list->lastind) { /* if we deleted before last, decr */
	    list->lastind--;
    }

    list->num--;
    list->maxlinks--;

    /* free up data associated w/link */
    if (l->dp && list->delfcn) {
	    list->delfcn(list, l->dp);
    }
    if (l->label) {
	    free(l->label);
    }

    free(l);			/* free link */

    return 0;
}

/* find a link given its label */

Link *
FindLink(List *list, char *s)
{
	int i;
	Link *link;

	/* doesn't support this function ... */
	if (!(list && s && *s)) {
		return(NULL);
	}

#ifdef notdef
	if (!list->getlabel) {
		return (NULL);
	}
#endif

	for (i=0, link = list->head; link && (i < list->num); 
	     link = link->next, i++) {
		if (link->label && (strcmp(link->label, s) == 0)) {
			list->lastind = i;
			list->lastlink = link;
			return(link);
		}
	}
	return(NULL);
}

/* DUMPLIST: Dump linked list, for debugging. */

void DumpList(List *list)
{
	register Link *l;
	register i;
    
	for (l = list->head, i=0; i < list->num; i++) {
		if (list->prtfcn) {
			list->prtfcn(list, l->dp);
			l = l->next;
		}
	}
}

int AppendListFromFile(List *list, char *fname, int aflag)
{
    
    FILE *fopen(), *fp;
    char buf[BUFSIZ];
    char *cp;
    caddr_t dp;

    if ((fp = fopen(fname, "r"))==NULL) {
	    fprintf(stderr,"can't open file %s\n", fname);
	    return -1;
    }
    
    if (!aflag) {
	    ClearList(list);
    }
    
    while (fgets(buf, BUFSIZ, fp)) {
	    if (list->decfcn) {
		    dp = list->decfcn(list, buf);
		    if (list->getlabel) {
			    cp = list->getlabel(list, dp);
		    }
		    if (AppendLink(list, cp, dp) == -1)
			    return -1;
	    }
    }

    fclose(fp);
    return 0;
}

int WriteListToFile(List *list, char *fname)
{
	FILE *fp, *fopen();
	register Link *lp;
	char *cp;

	if (!list->encfcn) {
		fprintf(stderr,"WriteListToFile: no ecnode function\n");
		return(-1);
	}

	if (!(fp=fopen(fname, "w"))) {
		fprintf(stderr,"WriteListToFile: can't write file %s\n", 
			fname);
		return (-1);
	}
	for (lp = list->head; lp; lp = lp->next) {
		if (cp = list->encfcn(list, lp->dp)) {
			fputs(cp, fp);
			/* check if there's a newline at the end of 
			 * the line, if not, write one to the file
			 */
			if (cp[strlen(cp)-1] != '\n') {
				fputc('\n', fp);
			}
		}
	}

	fclose(fp);

	return 0;
}

ReplaceLink()
{
}

int
ChangeLinkLabel(List *list, Link *lp, char *label)
{
	if (!lp) {
		return(-1);
	}

	if (lp->label) {
		if (strcmp(lp->label, label) != 0) {
			free(lp->label);
			lp->label = strdup(label);
		}
	} else {
		lp->label = strdup(label);
	}

	return(0);
}


/* 
 * utility to get next field of a string. s is the source string, delim
 * is the field delimiter character, dest is the destination string, and
 * len is the max length of the destination string.
 */

char *
getfld(char *s, char delim, char *dest, int len)
{
	register char *t;
	register i;

	if (!(s && dest)) {
		return (NULL);
	}

	for (t=dest,i=0 ;s && *s && (*s != delim) && (*s != '\n')&& (i<len);
	     i++) {
		*t++ = *s++;
	}

	if (i == len) { 	/* go to end of field */
		while (*s && s && (*s != delim) && (*s != '\n')) {
			s++;
		}
	}

	*t = '\0';

	/* if we're at the end of the string, return NULL ... */
	if (!*s) {
		return(NULL);
	} else {
		return (++s);
	}
}


/* END */

