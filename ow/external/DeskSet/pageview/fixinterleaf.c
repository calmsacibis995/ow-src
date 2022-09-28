#ifndef lint
static char *sccsid = "@(#)fixinterleaf.c 3.1 92/04/03";
#endif

/*
 * Copyright (c) 1990 - Sun Microsystems Inc.
 */

/*
 * fixinterleaf.c
 *
 * This program converts Interleaf PostScript files to follow the
 * PostScript Document Structuring Conventions described in Appendix C
 * of the Red Book.  PageView, psrev and other programs that manipulate
 * PostScript documents will not work with unaltered Interleaf
 * PostScript files.
 *
 * This program keeps track of the glyph definitions in Interleaf.
 * It eats the glyph definitions that occur between pages and then
 * inserts the proper glyph definitions in the Setup section after
 * the Prolog.  It also inserts the proper %%Page comments between
 * each page, and it properly numbers the pages from back to front.
 *
 */

#include <stdio.h>

#define FALSE 0
#define TRUE 1

#define eq(a,b) (!strncmp(a, b, strlen(b)))

char       *numbernames[] = {
    "one", "two", "three", "four", "five",
    "six", "seven", "eight", "nine", "ten",
    "eleven", "twelve", "thirteen", "fourteen",
    "fifteen", "sixteen", "seventeen", "eighteen", "nineteen",
    "twenty", "thirty", "fourty", "fifty",
    "sixty", "seventy", "eighty", "ninety",
};

void
pnum(f, num)
    FILE       *f;
    int         num;
{
    int         h = num / 100;
    int         r = (num - h * 100);
    int         t = r / 10;
    int         o = (r - t * 10);

    if (h) {
	fprintf(f, "%s hundred", numbernames[h - 1]);
	if (r)
	    fprintf(f, " and ");
	else
	    return;
    }
    if (r <= 20)
	fprintf(f, "%s", numbernames[r - 1]);
    else {
	fprintf(f, "%s", numbernames[17 + t]);
	if (o)
	    fprintf(f, "-%s", numbernames[o - 1]);
    }
}

main(argc, argv)
    int         argc;
    char      **argv;
{
    FILE       *pfin = stdin;
    FILE       *pfout = stdout;
    char        buf[1024];
    int         inpage;
    int         pages;

    switch (argc) {
    case 3:
	pfout = fopen(argv[2], "w");
	if (!pfout) {
	    fprintf(stderr, "%s: couldn't create %s.\n", argv[0], argv[2]);
	    exit(1);
	}
    case 2:
	pfin = fopen(argv[1], "r");
	if (!pfin) {
	    fprintf(stderr, "%s: %s not found.\n", argv[0], argv[1]);
	    exit(1);
	}
	break;
    default:
	fprintf(stderr, "Usage: %s inputfile [ > output ]\n", argv[0]);
	exit(1);
    }

    inpage = FALSE;
    pages = 0;
    fputs("%!PS-Adobe-2.0\n", pfout);
    while (1) {
	buf[0] = 0;
	fgets(buf, sizeof(buf), pfin);
	if (buf[0] == 0)
	    break;

	if (eq(buf, "bop")) {
	    inpage = TRUE;
	    pages++;
	    continue;
	}
	if (!inpage) {
	    if (eq(buf, "%  Copyright 1987 Interleaf, Inc.")) {
		fputs(buf, pfout);
		fputs("% fixed by the 'fixinterleaf' filter.\n", pfout);
		continue;
	    }
	    if (eq(buf, "%%EndProlog")) {
		fputs("%%EndProlog\n", pfout);
		fputs("%%BeginSetup\n", pfout);
		continue;
	    }
	    if (eq(buf, "%%Endcomments")) {
		fputs("%%EndComments\n", pfout);
		continue;
	    }
	    if (eq(buf, "initialstate restore")) {
		fputs("%%EndSetup\n", pfout);
		break;
	    }
	    if (!eq(buf, "%!"))
		fputs(buf, pfout);
	}
	if (eq(buf, "eop")) {
	    inpage = FALSE;
	    continue;
	}
    }

    fseek(pfin, 0, 0);

    inpage = FALSE;
    while (1) {
	buf[0] = 0;
	fgets(buf, sizeof(buf), pfin);
	if (buf[0] == 0)
	    break;
	if (eq(buf, "initialstate restore")) {
	    fputs("%%Trailer\n", pfout);
	    fputs("initialstate restore\n", pfout);
	    fputs("headerpage showpage\n", pfout);
	    break;
	}
	if (eq(buf, "bop")) {
	    fprintf(pfout, "%%%%Page: %d \"", pages);
	    pnum(pfout, pages);
	    fprintf(pfout, "\"\n");
	    pages--;
	    inpage = TRUE;
	}
	if (inpage)
	    fputs(buf, pfout);
	if (eq(buf, "eop"))
	    inpage = FALSE;
    }
    return 0;
}
