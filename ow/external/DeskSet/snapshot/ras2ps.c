#ifndef lint
static  char sccsid[] = "@(#)ras2ps.c 3.2 93/05/26 SMI";
#endif

/*-
 * ras2ps.c - Converts a Sun Rasterfile to a Color PostScript file.
 *
 * Copyright (c) 1989 by Sun Microsystems, Inc.
 *
 * Author: Patrick J. Naughton
 * naughton@wind.sun.com
 *
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose and without fee is hereby granted,
 * provided that the above copyright notice appear in all copies and that
 * both that copyright notice and this permission notice appear in
 * supporting documentation.
 *
 * This file is provided AS IS with no warranties of any kind.  The author
 * shall have no liability with respect to the infringement of copyrights,
 * trade secrets or any patents by this file or any part thereof.  In no
 * event will the author be liable for any lost revenue or profits or
 * other special, indirect and consequential damages.
 *
 * Comments and additions should be sent to the author:
 *
 *                     Patrick J. Naughton
 *                     Sun Microsystems
 *                     2550 Garcia Ave, MS 14-40
 *                     Mountain View, CA 94043
 *                     (415) 336-1080
 *
 * Revision History:
 * 05-May-93: (joew) fixed couple of bugs.
 * 04-Jun-91: Joe Warzecha removed pixrect dependencies.
 * 20-Jun-90: Hala Abdalla added capabilities for scaling width and height
 *            independent of each other.
 * 27-Nov-89: added -n for dumb EPSF includers that don't override showpage.
 *	      386i hexout() fix.
 *	      Added dump32().
 * 13-Sep-89: Updated EPSF version, added %%Page: comment, removed copies.
 * 02-Feb-88: Fixed mono string buffering bug, changed float 2f to 3f.
 * 22-Nov-88: Fixed filename processing.
 * 03-Nov-88: Put under SCCS control.
 * 17-Oct-88: Fixed BoundingBox / scale and various others nits.
 * 13-Oct-88: Fixed width/height bug.  Added rotate.
 * 13-Sep-88: Fixed bpsl bug.
 * 03-Sep-88: Changed default size of output to 300 dpi.
 * 12-Aug-88: Added stdin/stdout capability.
 * 11-Aug-88: Added Encapsulated PostScript Comments. (BoundingBox)
 * 10-Aug-88: Added support for 1 bit deep images.
 * 09-Aug-88: Added landscape option. Allow standard input.
 * 02-Aug-88: Clean up...
 * 27-Jul-88: Added colormapped rasterfile support.
 * 26-Jul-88: Updated to use libpixrect to fix 386i byteswapping problems.
 * 24-Jul-88: Written.
 *
 * Description:
 * This program takes a Sun rasterfile and writes a PostScript file to stdout.
 *
 */

#include <stdio.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/time.h>
#include <unistd.h>
#include "rast.h"

#define True 1
#define False 0
#define Intensity(A) (u_char) (0.11 * A[0] + 0.59 * A[1] + 0.30 * A[2])

int         Version = 2;
int         Revision = 10;
char       *pname;
double      xorig = 0.25;
double      yorig = 0.25;
double      xscale = 1.0;
double      yscale = 1.0;
double      width = 0.0;
double      height = 0.0;
double      angle = 0.0;
int         graydepth = 8;
int         invert = False;
int         color = False;
int         landscape = False;
int         verbose = False;
int         dummyinput = False;
int         showpage = True;
char       *inf = NULL;
char       *outf = NULL;
FILE       *fp,
           *ofp;
int         endline = 40;
int         colormap;
int         prDepth,
            prWidth,
            prHeight;
int         buffersize;
int         bpsl;
u_char     *Image;
u_char      pixel[3];
u_char      mask;
double      atof();
struct timeval tv;

void
error(s1, s2)
    char       *s1,
               *s2;
{
    fprintf(stderr, s1, pname, s2);
    exit(1);
}


void
usage()
{
    fprintf(stderr, "usage: %s -[xyXYwhrniClvq] [-|rasterfile] [psfile]\n",
	    pname);
    fprintf(stderr, "-x# -y#   origin in inches from lower left corner\n");
    fprintf(stderr, "-X# -Y#   scale of image\n");
    fprintf(stderr, "-w# -h#   width/height of image, in inches\n");
    fprintf(stderr, "-r#       rotate image\n");
    fprintf(stderr, "-i        invert image\n");
    fprintf(stderr, "-C        produce Color PostScript\n");
    fprintf(stderr, "-l        landscape mode\n");
    fprintf(stderr, "-n        don't include showpage\n\n");
    fprintf(stderr, "-v        verbose mode\n");
    fprintf(stderr, "-q        query? (show this message)\n");
    exit(1);
}


double
strarg(s)
    char       *s;
{
    if (s[2] == 0)
	error("%s: Switch '%s' needs a numeric argument.\n", s);
    return (atof(s + 2));
}

void
parseArgs(argc, argv)
    int         argc;
    char       *argv[];
{
    while (--argc > 0) {
	if ((++argv)[0][0] == '-') {
	    switch (argv[0][1]) {
	    case 'x':
		xorig = strarg(argv[0]);
		break;
	    case 'y':
		yorig = strarg(argv[0]);
		break;
	    case 'X':
		xscale = strarg(argv[0]);
		if (xscale <= 0.0)
		    error("%s: x-scale must be > 0.\n", NULL);
		break;
	    case 'Y':
		yscale = strarg(argv[0]);
		if (yscale <= 0.0)
		    error("%s: y-scale must be > 0.\n", NULL);
		break;
	    case 'w':
		width = strarg(argv[0]);
		if (width < 0.0)
		    error("%s: width must be > 0.\n", NULL);
		break;
	    case 'h':
		height = strarg(argv[0]);
		if (height < 0.0)
		    error("%s: height must be > 0.\n", NULL);
		break;
	    case 'g':
		graydepth = strarg(argv[0]);
		if ((graydepth != 2) && (graydepth != 4) && (graydepth != 8))
		    error("%s: graydepth must be 2, 4 or 8.\n", NULL);
		break;
	    case 'r':
		angle = strarg(argv[0]);
		break;
	    case 'i':
		invert = True;
		break;
	    case 'C':
		color = True;
		endline = 10;
		break;
	    case 'l':
		landscape = True;
		break;
	    case 'n':
		showpage = False;
		break;
	    case 'v':
		verbose = True;
		break;
	    case 'q':
		usage();
		exit(1);
		break;
	    case '\0':
		if (inf == NULL)
		    dummyinput = True;
		else
		    usage();
		break;
	    default:
		fprintf(stderr, "%s: illegal option -%c.\n",
			pname, argv[0][1]);
		usage();
	    }
	} else if (inf == NULL && !dummyinput) {
	    inf = argv[0];
	} else if (outf == NULL)
	    outf = argv[0];
	else
	    usage();
    }
}


void
hexout(ch)
    register u_char ch;
{
#ifdef i386
    static char *hex = "084C2A6E195D3B7F";
    putc(hex[ch & 0xf], ofp);
    putc(hex[ch >> 4], ofp);
#else
    static char *hex = "0123456789ABCDEF";
    putc(hex[ch >> 4], ofp);
    putc(hex[ch & 0xf], ofp);
#endif				/* i386 */
}


void
dump1()
{
    register    i,
                j,
                l;
    register u_char *p = Image;

    l = 0;
    mask ^= 0xff;
    for (i = 0; i < prHeight; i++) {
	for (j = 0; j < (prWidth + 7) / 8; j++) {
	    if (l++ % 40 == 0)
		fprintf(ofp, "\n");
	    hexout(p[j] ^ mask);
	}
	p += bpsl;
    }
}


void
dump8(pr)
image_t *pr;
{
    register    i,
                j,
                k,
                l;
    register u_char index;
    u_char     *p = Image;

    l = 0;
    if (color) {
	for (i = 0; i < prHeight; i++) {
	    for (j = 0; j < prWidth; j++) {
		if (l++ % endline == 0)
		    fprintf(ofp, "\n");
		index = p[j];
		hexout(pr->red[index] ^ mask);
		hexout(pr->green[index] ^ mask);
		hexout(pr->blue[index] ^ mask);
	    }
	    p += bpsl;
	}
    } else {
	u_char      outbyte;

	switch (graydepth) {
	case 2:
	    for (i = 0; i < prHeight; i++) {
		for (j = 0; j < prWidth; j += 4) {
		    if (l++ % endline == 0)
			fprintf(ofp, "\n");
		    outbyte = 0;
		    for (k = 0; k < 4; k++) {
			index = p[j + k];
			pixel[0] = pr->red[index] ^ mask;
			pixel[1] = pr->green[index] ^ mask;
			pixel[2] = pr->blue[index] ^ mask;
			outbyte |= (Intensity(pixel) & 0xc0) >> k * 2;
		    }
		    hexout(outbyte);
		}
		p += bpsl;
	    }
	    break;
	case 4:
	    for (i = 0; i < prHeight; i++) {
		for (j = 0; j < prWidth; j += 2) {
		    if (l++ % endline == 0)
			fprintf(ofp, "\n");
		    outbyte = 0;
		    for (k = 0; k < 2; k++) {
			index = p[j + k];
			pixel[0] = pr->red[index] ^ mask;
			pixel[1] = pr->green[index] ^ mask;
			pixel[2] = pr->blue[index] ^ mask;
			outbyte |= (Intensity(pixel) & 0xf0) >> k * 2;
		    }
		    hexout(outbyte);
		}
		p += bpsl;
	    }
	    break;
	case 8:
	    for (i = 0; i < prHeight; i++) {
		for (j = 0; j < prWidth; j++) {
		    if (l++ % endline == 0)
			fprintf(ofp, "\n");
		    index = p[j];
		    pixel[0] = pr->red[index] ^ mask;
		    pixel[1] = pr->green[index] ^ mask;
		    pixel[2] = pr->blue[index] ^ mask;
		    hexout(Intensity(pixel));
		}
		p += bpsl;
	    }
	    break;
	}
    }
}


void
dump24()
{
    register int i,
                j,
                k;
    register u_char rgb;
    u_char     *p = Image;

    for (i = 0; i < prHeight; i++) {
	for (j = 0; j < prWidth; j++) {
	    if (!((i * prWidth + j) % endline))
		fprintf(ofp, "\n");
	    for (k = 0; k < 3; k++) {
		rgb = p[j * 3 + k] ^ mask;
		if (color)
		    hexout(rgb);
		else
		    pixel[k] = rgb;
	    }
	    if (!color)
		hexout(Intensity(pixel));
	}
	p += bpsl;
    }
}


void
dump32()
{
    register int i,
                j,
                k;
    register u_char rgb;
    u_char     *p = Image;

    for (i = 0; i < prHeight; i++) {
	for (j = 0; j < prWidth; j++) {
	    if (!((i * prWidth + j) % endline))
		fprintf(ofp, "\n");
	    for (k = 0; k < 3; k++) {
		rgb = p[j * 4 + k] ^ mask;
		if (color)
		    hexout(rgb);
		else
		    pixel[k] = rgb;
	    }
	    if (!color)
		hexout(Intensity(pixel));
	}
	p += bpsl;
    }
}


main(argc, argv)
    int         argc;
    char      **argv;
{
    register int i,
                j;
    image_t	*pr;
    char	*who;
    double      dimscale;
    double      xdimscale, ydimscale;
    double      rawwidth,
                rawheight;

    setbuf(stderr, NULL);
    pname = argv[0];
    fp = NULL;

    parseArgs(argc, argv);

    if (dummyinput || inf == NULL) {
	inf = "Standard Input";
	fp = stdin;
    } else if ((fp = fopen(inf, "r")) == NULL)
	error("%s: %s couldn't be opened.\n", inf);

    if (verbose)
	fprintf(stderr, "Reading rasterfile from %s...", inf);

    pr = rast_load (fp);
    if (pr == NULL)
	error("%s: %s is not a raster file.\n", inf);

    if (verbose)
	fprintf(stderr, "done.\n");

    mask = ((invert) ? 0xff : 0);
    colormap = (pr->cmapused != 0);
    prDepth = pr->depth;
    prWidth = pr->width;
    prHeight = pr->height;

    if (verbose) {
	fprintf(stderr, "%s colormap was found, ", (colormap ? "A" : "No"));
	fprintf(stderr, "file is %d bit%s deep.\n",
		prDepth, (prDepth == 1) ? "" : "s");
    }
    if ((colormap && prDepth == 1)
	    || (!colormap && prDepth == 8)
	    || (colormap && prDepth == 24))
	error("%s: Strange combination of colormap and depth!\n", NULL);

    rawwidth = prWidth * xscale;
    rawheight = prHeight * yscale;

    if ((width == 0.0) && (height == 0.0))
        xdimscale = ydimscale = 1.0 / 72.0;
    else if ((height == 0.0) && (width != 0.0)) 
      /* && ((width / rawwidth) <= (height / rawheight)))*/
      xdimscale = ydimscale = width / rawwidth;
    else if ((height != 0.0) && (width != 0.0))
      {
	xdimscale = width / rawwidth;
	ydimscale = height / rawheight;
      }
    else
      xdimscale = ydimscale = height / rawheight;

    buffersize = (prDepth == 1) ? ((prWidth + 7) / 8)
	: ((color) ? prWidth * 3 : prWidth * graydepth / 8);

    if (color && prDepth == 1) {
	color = False;
	if (verbose)
	    fprintf(stderr, "Color output of a 1 bit image???\n");
    }
    gettimeofday(&tv, (struct timezone *) NULL);

    if (outf == NULL) {
	outf = "Standard Output";
	ofp = stdout;
    } else {
	if (!(ofp = fopen(outf, "w")))
	    error("%s: %s couldn't be opened for writing.\n", outf);
    }

    if (verbose)
	fprintf(stderr, "Writing %s PostScript to %s...",
		((color) ? "Color" : "Standard"), outf);

    fprintf(ofp, "%%!PS-Adobe-2.0 EPSF-2.0\n");
    fprintf(ofp, "%%%%Creator: %s v%d.%02d\n",
	    pname, Version, Revision);
    fprintf(ofp, "%%%%Title: %d x %d Sun Rasterfile: %s\n",
	    prWidth, prHeight, inf);
    who = cuserid ((char *) NULL);
    if (who == (char *) NULL)
       who = "Unknown";
    fprintf(ofp, "%%%%For: %s\n", who);
    fprintf(ofp, "%%%%CreationDate: %s", asctime(localtime(&tv.tv_sec)));
    fprintf(ofp, "%%%%Pages: 1\n");
    if (color)
	fprintf(ofp, "%%%%Requirements: colorprinter\n");

    fprintf(ofp, "%%%%BoundingBox: %.3f %.3f %.3f %.3f\n",
	    (72.0 * xorig),
	    (72.0 * yorig),
	    (72.0 * (xorig + rawwidth * xdimscale)),
	    (72.0 * (yorig + rawheight * ydimscale)));
    fprintf(ofp, "%%%%EndComments\n\n");

    fprintf(ofp, "/bufstr %d string def\n", buffersize);

    fprintf(ofp, "\n%%%%EndProlog\n");

    fprintf(ofp, "%%%%Page: 1 1\n");

    fprintf(ofp, "\ngsave\n");

    if (verbose)
	fprintf(stderr, "%s mode...",
		((landscape) ? "landscape" : "portrait"));
    if (landscape)
	fprintf(ofp, "612 0 translate 90 rotate\n");

    if (xorig != 0 || yorig != 0)
	fprintf(ofp, "%.3f %.3f translate\n", xorig * 72, yorig * 72);

    if (angle != 0.0)
	fprintf(ofp, "%.3f rotate\n", angle);

    fprintf(ofp, "%.3f %.3f scale\n\n",
	    rawwidth * xdimscale * 72.0,
	    rawheight * ydimscale * 72.0);

    fprintf(ofp, "%d %d %d\n",
	    prWidth, prHeight, ((prDepth == 1) ? 1 : graydepth));
    fprintf(ofp, "[%d 0 0 -%d 0 %d]\n",
	    prWidth, prHeight, prHeight);
    fprintf(ofp, "{currentfile bufstr readhexstring pop} bind\n");

    fprintf(ofp, "%simage\n", ((color) ? "false 3 color" : ""));

    Image = (u_char *) pr->data;
    bpsl = linelen (prWidth, prDepth);

    switch (prDepth) {
    case 1:
	dump1();
	break;
    case 8:
	dump8(pr);
	break;
    case 24:
	dump24();
	break;
    case 32:
	dump32();
	break;
    }
    fprintf(ofp, "\ngrestore\n");
    if (showpage)
	fprintf(ofp, "\nshowpage\n");
    fprintf(ofp, "\n%%%%Trailer\n");

    if (verbose)
	fprintf(stderr, "done.\n");

    exit(0);
}

