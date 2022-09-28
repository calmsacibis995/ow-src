#ifndef lint
#ifdef sccs
static char     sccsid[] = "@(#)seln.c 20.22 94/11/23";
#endif
#endif

/*
 *	(c) Copyright 1989 Sun Microsystems, Inc. Sun design patents 
 *	pending in the U.S. and foreign countries. See LEGAL NOTICE 
 *	file for terms of the license.
 */

#include <sys/types.h>
#include <sys/file.h>
#include <stdio.h>
#include <errno.h>
#include <xview_private/i18n_impl.h>
#include <xview/xview_xvin.h>
#include <xview/selection.h>
struct selection selnull;
static FILE	* sel_fptr = NULL;

char           *selection_filename();

/* ARGSUSED */
selection_set(sel, sel_write, sel_clear, window)
    struct selection *sel;
    int             (*sel_write) (), (*sel_clear) ();
    Xv_object       window;
/* Note: sel_clear not used yet */
{
    FILE           *file;
    int             firsttime = 1;

    (void) win_lockdata(window);
Open:
    if (sel_fptr != NULL ) {
	(void)fclose(sel_fptr);
	sel_fptr = NULL;
    }
	
    if ((file = fopen(selection_filename(), "w+")) == NULL) {
	if (firsttime) {
	    firsttime = 0;
	    if (unlink(selection_filename()) == 0)
		goto Open;
	}
	(void) win_unlockdata(window);
	(void) fprintf(stderr, 
		XV_MSG("%s would not open\n"), 
		selection_filename());
#ifdef SUNVIEW1			/******* BUG ************/
	(void) werror(-1, window);
#endif
	return;
    }
    (void) unlink(selection_filename());
    sel_fptr = file;
    /* Make so everyone can access */
    (void) fchmod(fileno(file), 0600);
    (void) fprintf(file, "TYPE=%ld, ITEMS=%ld, ITEMBYTES=%ld, PUBFLAGS=%lx, PRIVDATA=%lx\n",
		   sel->sel_type, sel->sel_items, sel->sel_itembytes,
		   sel->sel_pubflags, sel->sel_privdata);
    (*sel_write) (sel, file);
    (void) win_unlockdata(window);
}

selection_get(sel_read, window)
    int             (*sel_read) ();
    Xv_object       window;
{
    struct selection selection, *sel = &selection;
    FILE           *file;
    int             c;
    int             n;

    *sel = selnull;
    (void) win_lockdata(window);
    if ( sel_fptr == NULL ) {
	(void) win_unlockdata(window);
	if (errno == ENOENT)
	    return;		/* No selection available */
	(void) fprintf(stderr, 
		XV_MSG("%s would not open\n"), 
		selection_filename());
#ifdef SUNVIEW1			/******* BUG ************/
	(void) werror(-1, window);
#endif
	return;
    }
    file = sel_fptr;
    rewind(file);

    if ((c = getc(file)) == EOF) {
	goto Cleanup;		/* Selection has been cleared */
    } else
	(void) ungetc(c, file);
    if ((n = fscanf(file,
	 "TYPE=%ld, ITEMS=%ld, ITEMBYTES=%ld, PUBFLAGS=%lx, PRIVDATA=%lx%c",
		    &sel->sel_type, &sel->sel_items, &sel->sel_itembytes,
		    &sel->sel_pubflags, &sel->sel_privdata, &c)) != 6) {
	(void) win_unlockdata(window);
	(void) fprintf(stderr, 
		XV_MSG("%s not in correct format\n"),
		       selection_filename());
	(void) fprintf(stderr, "TYPE=%ld, ITEMS=%ld, ITEMBYTES=%ld, PUBFLAGS=%lx, PRIVDATA=%lx c=%c, n=%ld\n",
		       sel->sel_type, sel->sel_items, sel->sel_itembytes,
		       sel->sel_pubflags, sel->sel_privdata, c, n);
	goto Cleanup;
    }
    (*sel_read) (sel, file);
Cleanup:
    (void) win_unlockdata(window);
}

selection_clear(window)
    Xv_object       window;
{
    FILE           *file;

    (void) win_lockdata(window);
	
    if (sel_fptr != NULL) {
	(void)fclose(sel_fptr);
	sel_fptr = NULL;
    }
	
    if ((file = fopen(selection_filename(), "w+")) == NULL) {
	(void) win_unlockdata(window);
	(void) fprintf(stderr, 
		XV_MSG("%s would not open\n"), 
		selection_filename());
#ifdef SUNVIEW1			/******* BUG ************/
	(void) werror(-1, window);
#endif
	return;
    }
    (void) fclose(file);
    (void) unlink(selection_filename());
    (void) win_unlockdata(window);
}

char           *
selection_filename()
{
    char           *getenv();
    char           *name;

    if ((name = getenv("SELECTION_FILE")) == NULL)
	name = "/tmp/winselection";
    return (name);
}
