#ifndef lint
static char sccsid[] = "@(#)24to8.c 23.3 90/05/07";
#endif

/*-
 * 24to8.c - Converts a 24 bit image to 8 bits using X/NeWS' colormap.
 *
 * Copyright (c) 1989 by Sun Microsystems, Inc.
 *
 * Author: Patrick Naughton
 * naughton@wind.sun.com
 *
 * This file is a product of Sun Microsystems, Inc. and is provided for
 * unrestricted use provided that this legend is included on all tape
 * media and as a part of the software program in whole or part.  Users
 * may copy or modify this file without charge, but are not authorized to
 * license or distribute it to anyone else except as part of a product
 * or program developed by the user.
 *
 * THIS FILE IS PROVIDED AS IS WITH NO WARRANTIES OF ANY KIND INCLUDING THE
 * WARRANTIES OF DESIGN, MERCHANTIBILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE, OR ARISING FROM A COURSE OF DEALING, USAGE OR TRADE PRACTICE.
 *
 * This file is provided with no support and without any obligation on the
 * part of Sun Microsystems, Inc. to assist in its use, correction,
 * modification or enhancement.
 *
 * SUN MICROSYSTEMS, INC. SHALL HAVE NO LIABILITY WITH RESPECT TO THE
 * INFRINGEMENT OF COPYRIGHTS, TRADE SECRETS OR ANY PATENTS BY THIS FILE
 * OR ANY PART THEREOF.
 *
 * In no event will Sun Microsystems, Inc. be liable for any lost revenue
 * or profits or other special, indirect and consequential damages, even
 * if Sun has been advised of the possibility of such damages.
 *
 * Sun Microsystems, Inc.
 * 2550 Garcia Avenue
 * Mountain View, California  94043
 *
 * Modification History
 *
 * 	DDJ	1/23/90		Dither to small color cube, by default.
 *				Added -large option.
 */

#include <stdio.h>
#include <sys/types.h>
#include <pixrect/pixrect_hs.h>

#define NRED_LARGE	5
#define NGREEN_LARGE	9
#define NBLUE_LARGE	5

#define NRED_SMALL	5
#define NGREEN_SMALL	5
#define NBLUE_SMALL	5

unsigned char red_map[256];
unsigned char green_map[256];
unsigned char blue_map[256];
unsigned char red_inverse[256];
unsigned char green_inverse[256];
unsigned char blue_inverse[256];
unsigned char gray_inverse[256];

void make_cube();

colormap_t  colormap = {1, 256, red_map, green_map, blue_map};

int	    cube_small = 1;
int         verbose = 0;
int         dummyinput = 0;
char       *pname;
char       *inf = NULL;
char       *outf = NULL;

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
    fprintf(stderr, "usage: %s -[vq] [-large] [-|rasterfile24] [rasterfile8]\n", pname);
    fprintf(stderr, "-v        verbose mode\n");
    fprintf(stderr, "-q        query? (show this message)\n");
    fprintf(stderr, "-large    dither into %dx%dx%d color cube space\n",
		NRED_LARGE, NGREEN_LARGE, NBLUE_LARGE);
    exit(1);
}


void
parseArgs(argc, argv)
    int         argc;
    char       *argv[];
{
    while (--argc > 0) {
	if ((++argv)[0][0] == '-') {
	    switch (argv[0][1]) {
	    case 'l':
		if (!strcmp(*argv, "-large"))
			cube_small = 0;
		else
			usage();
		break;
	    case 'v':
		verbose = 1;
		break;
	    case 'q':
		usage();
	    case '\0':
		if (inf == NULL)
		    dummyinput = 1;
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


#define error_distribute(color) \
    if (color > 255) error->color = color - 255, color = 255; \
    else if (color < 0) error->color = color, color = 0; \
    else error->color = 0

struct error {
    short       red,
                green,
                blue;
};

struct pixrect *
dither24to8(inpr)
    struct pixrect *inpr;
{
    register    x,
                y;
    struct error *errormap = (struct error *) calloc(sizeof(struct error),
						     inpr->pr_width + 2);
    struct pixrect *outpr = mem_create(inpr->pr_width, inpr->pr_height, 8);

    for (y = 0; y < inpr->pr_height; y++) {
	register struct error *error = errormap + 1;
	register u_char *src = (u_char *) mprd_addr(mpr_d(inpr), 0, y);
	register u_char *dst = (u_char *) mprd_addr(mpr_d(outpr), 0, y);

	for (x = 0; x < inpr->pr_width; x++) {
	    register    red,
	                green,
	                blue;
	    register    index;
	    register    T;

	    if (inpr->pr_depth == 32) {
		red = src[3] + error->red;
		green = src[2] + error->green;
		blue = src[1] + error->blue;
	    } else {
		red = src[2] + error->red;
		green = src[1] + error->green;
		blue = src[0] + error->blue;
	    }

	    error_distribute(red);
	    error_distribute(green);
	    error_distribute(blue);
	    index = red_inverse[red]
		+ green_inverse[green]
		+ blue_inverse[blue];
	    error->red -= red_map[index] - red;
	    error->green -= green_map[index] - green;
	    error->blue -= blue_map[index] - blue;
	    *dst++ = index;
	    T = (error->red * 3 + 4) / 8;
	    error[1].red += T;
	    error[0].red -= T + T;
	    error[-1].red += T;
	    T = (error->green * 3 + 4) / 8;
	    error[1].green += T;
	    error[0].green -= T + T;
	    error[-1].green += T;
	    T = (error->blue * 3 + 4) / 8;
	    error[1].blue += T;
	    error[0].blue -= T + T;
	    error[-1].blue += T;
	    error++;
	    if (inpr->pr_depth == 32) {
		src += 4;
	    } else {
		src += 3;
	    }
	}
    }
    free(errormap);
    return outpr;
}


main(argc, argv)
    int         argc;
    char      **argv;
{
    FILE       *fp;
    colormap_t  inmap;
    struct pixrect *inpr;
    struct pixrect *outpr;

    setbuf(stderr, NULL);
    pname = argv[0];

    parseArgs(argc, argv);

    if (dummyinput || inf == NULL) {
	inf = "Standard Input";
	fp = stdin;
    } else if ((fp = fopen(inf, "r")) == NULL)
	error("%s: %s couldn't be opened.\n", inf);

    if (verbose)
	fprintf(stderr, "Reading rasterfile from %s...", inf);

    inpr = pr_load(fp, &inmap);

    fclose(fp);

    if (inpr == NULL)
	error("%s: %s is not a raster file.\n", inf);

    if (inpr->pr_depth != 24 && inpr->pr_depth != 32)
	error("%s: %s is not 24 or 32 bits deep.\n", inf);

    if (verbose)
	fprintf(stderr, "done.\nConverting a %dx%d image...",
		inpr->pr_width, inpr->pr_height);

    /* generate the color cube on the fly */
    make_cube();

    outpr = dither24to8(inpr);

    if (verbose)
	fprintf(stderr, "done.\n");

    if (outf == NULL) {
	outf = "Standard Output";
	fp = stdout;
    } else {
	if (!(fp = fopen(outf, "w")))
	    error("%s: %s couldn't be opened for writing.\n", outf);
    }

    if (verbose)
	fprintf(stderr, "Writing rasterfile on %s...", outf);

    pr_dump(outpr, fp, &colormap, RT_STANDARD, 0);
    fclose(fp);

    if (verbose)
	fprintf(stderr, "done.\n");

    exit(0);
}


static void
compute_inverse (N, stride, offset, inverse)
    register unsigned char *inverse;
    register short N,
                stride;
{
    register short ipos;
    register    halfslot = 256 / N / 2;
    for (ipos = 0; ipos < 256; ipos++) {
	inverse[ipos] = (ipos * N - halfslot) / 256 * stride + offset;
    }
}


void
make_cube ()

{
    int	nred, ngreen, nblue;
    register    r,
                g,
                b,
                i;
    int         placed;
    int         interval;
    int 	cubebase;

    /* initialize unused with white/black */
    for (i = 0; i<255; i+=2) {
	red_map[i] = 255;
	green_map[i] = 255;
	blue_map[i] = 255;
	red_map[i+1] = 0;
	green_map[i+1] = 0;
	blue_map[i+1] = 0;
    }

    /* choose cube size */
    if (cube_small) {
	nred   = NRED_SMALL;
	ngreen = NGREEN_SMALL;
	nblue  = NBLUE_SMALL;
	if (verbose) fprintf(stderr, "Dithering to small color cube\n");
    } else {
	nred   = NRED_LARGE;
	ngreen = NGREEN_LARGE;
	nblue  = NBLUE_LARGE;
	if (verbose) fprintf(stderr, "Dithering to large color cube\n");
    }

    /* generate color cube */
    cubebase = 255-nred*ngreen*nblue;
    i = cubebase;
    for (g = 0; g < ngreen; g++)
	for (b = 0; b < nblue; b++)
	    for (r = 0; r < nred; r++) {
		red_map[i] = (r * 255) / (nred - 1);
		green_map[i] = (g * 255) / (ngreen - 1);
		blue_map[i] = (b * 255) / (nblue - 1);
		i++;
	    }

    /* calculate inverse of cube */
    compute_inverse(nred, 1, 0, red_inverse);
    compute_inverse(ngreen, nred * nblue, 0, green_inverse);
    compute_inverse(nblue, nred, cubebase, blue_inverse );
}




