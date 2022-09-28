/* Copyright (c) 1992 by Sun Microsystems, Inc. */
#ident	"@(#)plistutils.c	1.3	92/10/11 SMI"

#ident	"$Id$ RAND"

#include <stdio.h>
#include <sys/param.h>
#include <sys/types.h>
#include <xview/xview.h>
#include <xview/panel.h>

#include "listmgr.h"

/* utilities for dealing with a panel list */


int get_selected_list_index (Panel_item list);

clear_panel_list(Panel_item plist)
{
    register char **alist;
    register i, j;
    
    int nitems = xv_get(plist, PANEL_LIST_NROWS);
    
    if (nitems) {
	alist = (char **) malloc(((2 * nitems)+1) * sizeof (char *));
    
	/* got to delete 'em backwards! */
	for(i=nitems-1, j=0; i >= 0; i--) {
	    alist[j++]= (char*) PANEL_LIST_DELETE;
	    alist[j++]= (char*) i;
	}
	alist[j] = (char *)NULL;
	xv_set(plist, ATTR_LIST, alist, NULL);
    
	free(alist);
    }
}

int
replace_selected_list_item(Panel_item list, char *s)
{
    int nobjs;
    register i;

    nobjs = (int) xv_get(list, PANEL_LIST_NROWS);
    for (i = 0; i < nobjs; i++)
	if (xv_get(list, PANEL_LIST_SELECTED, i))
	  {
	      xv_set(list, PANEL_LIST_STRING, i, s, NULL);
	      break;
	  }
}

int
replace_list_item(Panel_item list, char *s, int i)
{
    int nobjs;

    if (i < (int) xv_get(list, PANEL_LIST_NROWS)) {
	xv_set(list, PANEL_LIST_STRING, i, s, NULL);
    }
}

delete_selected_list_item(Panel_item list)
{
    int nobjs;
    register i;
	
    nobjs = (int) xv_get(list, PANEL_LIST_NROWS);
    for (i = 0; i < nobjs; i++)
	if (xv_get(list, PANEL_LIST_SELECTED, i))
	  {
	      xv_set(list, 
		     PANEL_LIST_SELECT, i, FALSE,
		     PANEL_LIST_DELETE, i, 
		     0);
	      break;
	  }
}

delete_list_item(Panel_item list, char *s)
{
    int nobjs;
    register i;
	
    nobjs = (int) xv_get(list, PANEL_LIST_NROWS);
    for (i = 0; i < nobjs; i++) {
	if (strcmp((char*)xv_get(list, PANEL_LIST_STRING, i), s) == 0) {
	    xv_set(list, 
		   PANEL_LIST_SELECT, i, FALSE,
		   PANEL_LIST_DELETE, i, 
		   0);
	    break;
	}
    }
}

int
append_list_item(Panel_item list, char *s)
{
    register i;
	
    i = (int) xv_get(list, PANEL_LIST_NROWS);
    xv_set(list, PANEL_LIST_STRING, i, s,  NULL);
    return (i);
}

int
insert_list_item(Panel_item list, char *s, int where)
{
    int i, pos;
	
    switch(where) {
      case I_TOP:
	pos = 0;
	break;
      case I_BEFORE:
	pos = get_selected_list_index(list);
	if (pos <= 0) {
	    pos = 0;
	} else {
	    /* pos--; */
	}
	break;
      case I_AFTER:
	pos = get_selected_list_index(list);
	i = (int) xv_get(list, PANEL_LIST_NROWS);
	if (pos >= i) {
	    pos = i;
	} else {
	    pos++;
	}
	break;
      default:
      case I_BOTTOM:
	pos = (int) xv_get(list, PANEL_LIST_NROWS);
	break;
    }
    xv_set(list, 
	   PANEL_LIST_INSERT, pos, 
	   PANEL_LIST_STRING, pos, s,
	   NULL);
    return (pos);
}

char *get_selected_list_item(Panel_item list)
{
    
    int nobjs;
    register i;
	
    nobjs = (int) xv_get(list, PANEL_LIST_NROWS);
    for (i = 0; i < nobjs; i++)
	if (xv_get(list, PANEL_LIST_SELECTED, i))
	    return (char*) xv_get(list, PANEL_LIST_STRING, i);
    return NULL;
}



int get_selected_list_index(Panel_item list)
{
    
    int nobjs;
    register i;
	
    nobjs = (int) xv_get(list, PANEL_LIST_NROWS);
    for (i = 0; i < nobjs; i++)
	if (xv_get(list, PANEL_LIST_SELECTED, i))
	    return (i);
    return (-1);
}

int
set_selected_list_index(Panel_item list, int index)
{
    
    int i;
	
    if ((i = get_selected_list_index(list)) != -1) {
	xv_set(list, PANEL_LIST_SELECT, i, FALSE, NULL);
    }

    xv_set(list, PANEL_LIST_SELECT, index, TRUE, NULL);
}

int
unselect_list_item(Panel_item list)
{
    int i;
	
    if ((i = get_selected_list_index(list)) != -1) {
	xv_set(list, PANEL_LIST_SELECT, i, FALSE, NULL);
    }
}

int
find_list_item(Panel_item list, char *s)
{
    int nobjs;
    register i;
	
    nobjs = (int) xv_get(list, PANEL_LIST_NROWS);
    for (i = 0; i < nobjs; i++) {
	if (strcmp((char*)xv_get(list, PANEL_LIST_STRING, i), s) == 0) {
	    return (i);
	}
    }
    return (-1);
}

load_panel_list(Panel_item plist, char **slist, int nitems)
{
    register char **alist;
    register i, j;
    
    alist = (char **) malloc(((3 * nitems)+1) * sizeof (char *));
    
    /* got to delete 'em backwards! */
    for(i=0, j=0; i < nitems; i++) {
	alist[j++]= (char*) PANEL_LIST_STRING;
	alist[j++]= (char*) i;
	alist[j++]= (char*) slist[i];
    }
    alist[j] = (char *)NULL;
    xv_set(plist, ATTR_LIST, alist, NULL);
    
    free(alist);
}

/* this does it the slow way.... */
load_panel_list_from_file(Panel_item plist, char *fname, int skip)
                     
                
             			/* # lines to skip before loading (hack) */
{
    int i;
    FILE *fp;
    char buf[BUFSIZ];
    char *cp;

    if ((fp = fopen(fname, "r")) == NULL) {
	fprintf(stderr,"load_panel_list: can't open file %s\n", fname);
	perror("fopen");
	return;
    }

    /* skip lines */
    while ((skip-- > 0) && fgets(buf, BUFSIZ, fp)) ;

    /* read & load */
    for(i=0; cp = fgets(buf, BUFSIZ, fp); i++) {
	xv_set(plist, PANEL_LIST_STRING, i, cp, NULL);
    }
    fclose(fp);
}


/*
 * Local Variables:
 * eval: (set-c-style 'IJS)
 * End:
 */
