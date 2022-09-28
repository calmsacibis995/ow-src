#ifndef lint
static char *sccsid = "@(#)fixframe.c 3.1 92/04/03";
#endif

/*
 * Copyright (c) 1990 - Sun Microsystems Inc.
 */

/*
 * fixframe.c
 *
 * This program converts FrameMaker 2.0 PostScript files to follow the
 * PostScript Document Structuring Conventions described in Appendix C
 * of the Red Book.  PageView, psrev and other programs that manipulate
 * PostScript documents will not work with unaltered FrameMaker 2.0
 * PostScript files.
 *
 * This program keeps track of the font definitions in FrameMaker 2.0.
 * It eats the font definitions that occur between pages and then
 * inserts the proper font definitions after the %%Page: comment for
 * each page.
 *
 */

#include <stdio.h>

#define FALSE 0
#define TRUE 1

#define MAXFONTS 100
char       *fontarray[MAXFONTS];
int	    verbose = FALSE;

printFonts(pf)
    FILE       *pf;
{
    int         i;
    for (i = 0; i < MAXFONTS; i++)
	if (fontarray[i] != (char *) 0) {
	    fputs(fontarray[i], pf);
	}
}

addFont(i, string)
    int         i;
    char       *string;
{
    if (fontarray[i] != (char *) 0) {
	if (verbose)
	    fprintf(stderr, "redefine font %d: %s\n", i, fontarray[i]);
	free(fontarray[i]);
    }
    fontarray[i] = (char *) malloc(strlen(string) + 1);
    strcpy(fontarray[i], string);
}


Usage(argc, argv)
    int         argc;
    char      **argv;
{

    fprintf(stderr, "Usage: %s [ < input ] [ > output ]\n", argv[0]);
    exit(1);
}

main(argc, argv)
    int         argc;
    char      **argv;
{
    FILE       *pfin = stdin;
    FILE       *pfout = stdout;
    char        buf[1024];
    int         limbo;
    int         dummy;
    int		end_setup_found = FALSE;

    if (argc != 1)
	Usage(argc, argv);

    memset(fontarray, 0, sizeof(fontarray));
    limbo = FALSE;
    while (1) {
	buf[0] = 0;
	fgets(buf, sizeof(buf), pfin);
	if (buf[0] == 0)
	    break;

	if (limbo == FALSE)
	    fputs(buf, pfout);

	if (0 == strncmp(buf, "%%EndComments", 13)) {
	    fputs("% This file has been fixed by the 'fixframe' filter\n",
		  pfout);
	}
	if ((limbo == TRUE) && (buf[0] != '%')) {
	    int         num;
	    if (1 == sscanf(buf, "%d", &num)) {
		addFont(num, buf);
	    } else if (verbose)
		fprintf(stderr, "Bad font definition: %s\n", buf);
	}
	if ((0 == strncmp(buf, "%%EndSetup", 10)) && 
	    (end_setup_found == FALSE) ) {
	    limbo = TRUE;
	    end_setup_found = TRUE;
	    }
	if (0 == strncmp(buf, "%%Page:", 7)) {
	    limbo = FALSE;
	    fputs(buf, pfout);
	    printFonts(pfout);
	}
	if (0 == strncmp(buf, "%%EndPage:", 10))
	    limbo = TRUE;
	if ((0 == strncmp(buf, "%%Trailer", 9)) && (limbo == TRUE)) {
            limbo = FALSE;
            fputs(buf, pfout);
        }

    }
    exit(0);
}
