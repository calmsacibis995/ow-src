#ifndef lint
static char sccsid[] = "@(#)rash.c	3.2 07/12/93 Copyright 1987-1990 Sun Microsystem, Inc." ;
#endif

#include <stdio.h>
#include <math.h>

#ifdef SVR4
#include <pixrect/rasterfile.h>
#else
#include <rasterfile.h>
#endif "SVR4"

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/fcntl.h>
#include <sys/param.h>
#include <string.h>
#include <ctype.h>
#include <locale.h>
#include <time.h>
#include "rast.h"

extern FILE *pl_fopen_file();
extern char *getenv();
extern char *strdup();
extern char *malloc();

  
#define yes -1
#define no   0
#ifndef min
#define min(a,b) ((a)<(b)? (a): (b))
#endif
#ifndef max
#define max(a,b) ((a)<(b)? (b): (a))
#endif

extern long strtol ();
extern double strtod ();
void dump_mono_image(), dump_luminance_image(), dump_color_image();

char *progname;
void (*dump_fn)();
char line[1000];
FILE *input = stdin;
FILE *output = stdout;
FILE *wrapper;

/* 
 * These are controlled by command-line options 
 */
char  *ofn = NULL;		/* output filename (-p) */
char  *wfn = NULL;		/* wrapper filename (-w) */
char  *rfn = NULL;		/* input rasterfile */
int   grayscale = 0;		/* image is grayscale */
int   do_luminance = 0;		/* convert to mono (-m) */
int   center = 0, left = 0;     /* where to locate image (-l, -c) */
float px = 0.0, py = 0.0;	/* location of image wrt lower left (-l) */
				/* OR center (-c) */
float xscale = 0.0,		/* scale of image (-<n>, -S, -s, -H, -W) */
      yscale = 0.0;		/* if 0.0, output as large as possible. */
float rotation = 0.0;		/* rotation (-R) */
int   showpage = 1;		/* output a showpage (-n) */
int   integral_pixels = 0;	/* round to integral pixels (-i) */
int   landscape = 0;		/* print in landscape mode (-r) */
int   pix = 0;
int   epsf = 0;
float minx, miny, maxx, maxy;

#define gline() fgets (line, sizeof(line), wrapper)

#ifdef SVR4
/* Those two functions are not part of the ANSI-C string library.
 * !icequal yealds different results than strcasecmp.
 * [vmh - 4/3/91]
 * added to rash on 6/4/91 - will take out when they (strcasecmp
 * and strncasecmp) are put into libc.a (jup_alpha3 apparently).
 */

raise(c)
char c;
{
	if (islower(c)) return(toupper(c));
	return (c);
}

/*   Compares  thw  arguments  and  returns  an  integer
 *   greater  than,  equal to, or less than 0, according as s1 is
 *   lexicographically greater than, equal to, or less  than  s2.
 *   Ignore differences in case. Assume ASCII character set.
 */

int
strcasecmp(s1, s2)
char *s1, *s2;
{
        register int ch1, ch2;

        while ((ch1= raise(*s1)) == (ch2 = raise(*s2++)))
                if (*s1++ == '\0')
                        return(0);
        return (ch1 - ch2);

}

int
strncasecmp(s1, s2, n)
char *s1, *s2;
int n;
{
        register int ch1, ch2;
 
        while (--n >= 0 && (ch1 = raise(*s1)) == (ch2 = raise(*s2++)))
                if (*s1++ == '\0')
                        return(0);
        return (n < 0 ? 0 : ch1 - ch2);
}

#endif "SVR4"

void usage (str, arg) 
    char *str;
    int arg;
{
    if (str)
    {
	fprintf(stderr, "%s: ", progname);
	fprintf(stderr, str, arg);
    }

    fprintf (stderr, "Usage: %s [options] [raster-file]\n", progname);
    fprintf (stderr, "options:\n");
    fprintf (stderr, "\t-2          (scale to print raster at double size)\n");
    fprintf (stderr, "\t-<n>x<m>    (scale by (different) x and y scale factors)\n");
    fprintf (stderr, "\t-H height*  (match width to specified height.)\n");
    fprintf (stderr, "\t-R<number>  (rotation angle.).\n");
    fprintf (stderr, "\t-S width*   (match width to specified height.)\n");
    fprintf (stderr, "\t-W width*   (match height to specified width.)\n");
    fprintf (stderr, "\t-c x* y*    (locate image center vs page center)\n");
    fprintf (stderr, "\t-e          (produce EPSF.)\n");
    fprintf (stderr, "\t-g          (treat input image as grayscale.)\n");
    fprintf (stderr, "\t-i          (adjust scale to integral pixels.)\n");
    fprintf (stderr, "\t-l x* y*    (locate image llh corner vs page llh corner)\n");
    fprintf (stderr, "\t-m          (b/w grayscale output from color image.)\n");
    fprintf (stderr, "\t-n          (no showpage at end of file.)\n");
    fprintf (stderr, "\t-p filename (output PostScript to named file)\n");
    fprintf (stderr, "\t-r          (print in landscape mode)\n");
    fprintf (stderr, "\t-s w* h*    (output with specified width and height)\n");
    fprintf (stderr, "\t-w filename (specify wrapper file name)\n");
    fprintf (stderr, "\n");
    fprintf (stderr, "-<n>, -s, -S, -H and -W are mutually exclusive,\n");
    fprintf (stderr, "as are -l and -c.  -R implies -c.\n");
    fprintf (stderr, "\n");
    fprintf (stderr, "* By default, the unit of distance arguments is printer's points.\n");
    fprintf (stderr, "  You may also specify distances in inches or centimiters by\n");
    fprintf (stderr, "  appending 'in' or 'cm', e.g. -l 1.0in 2.0cm\n");
    fprintf (stderr, "\n");

    exit (13);
}


float 
num_arg(av, v, start_char) 
    char **av;
    int *v;
    int start_char;
{
    char *p;
    double n;

    if (!av[*v])
	usage("Missing numeric argument to %s option\n", av[*v - 1]);

    if (av[*v][start_char] == '\0')
    {
	++(*v);
	start_char = 0;
    }

    if (!av[*v])
	usage("Missing numeric argument to %s option\n", av[*v - 1]);

    n = strtod(av[*v] + start_char, &p);

    if (p == av[*v])
	usage("Missing numeric argument to %s option\n", av[*v - 1]);

    while (isspace(*p))
	++p;

    if (*p)
	usage("Bad numeric argument '%s'\n", av[*v]);
}

float size_arg(av, v, start_char)
    char **av;
    int *v;
    int start_char;
{
    char *p;
    double n;

    if (!av[*v])
	usage("Missing numeric argument to %s option\n", av[*v - 1]);

    if (av[*v][start_char] == '\0')
    {
	++(*v);
	start_char = 0;
    }

    if (!av[*v])
	usage("Missing numeric argument to %s option\n", av[*v - 1]);

    n = strtod(av[*v] + start_char, &p);
    if (p == av[*v])
	usage("Missing numeric argument to %s option\n", av[*v - 1]);

    while (isspace(*p))
	++p;

    /* 
     * rich berlin 1/15/91
     * bug--shouldn't do these comparisons if strlen(p) is zero! 
     */
    if ((strlen(p) == 0) || (strncasecmp(p,"pt",strlen(p)) == 0))
      {
	/* do nothing */
      }
    else
    if (strncasecmp(p,"inches",strlen(p)) == 0)
	n *= 72.0;
    else if (strncasecmp(p,"cm",strlen(p)) == 0)
	n *= 28.34646;
    else if (strncasecmp(p,"mm",strlen(p)) == 0)
	n *= 2.834646;
    else
	usage("Bad numeric argument '%s'\n", av[*v]);

    return(n);

}

main (ac, av)
     int   ac;
     char **av;
{
    char *l, *p;
    image_t *pf;
    unsigned char *cb;
    int po[2], pn[3], v, x, y, b = 1, lineb;
    long t;
    int numlines;

/*
 * Set locale, so arguments are read in correctly.
 */

    setlocale (LC_ALL, "");
    progname = av[0];
    for (v = 1; v < ac; v++) 
    {
	if (av[v][0] == '-') 
	{
	    switch (av[v][1]) 
	    {
		/* Options to match pssun */

	      case 'p':		/* output Postscript to named file */
		if (ofn != NULL)
		    usage("Can only specify output file name once\n", 0);
		
		if (av[v][2] == 0) 
		{
		    ofn = av[++v];
		    if (v == ac)
			usage("Missing filename argument to -p option\n", 0);
		} 
		else 
		{
		    ofn = av[v] + 2;
		}
		break;
		
	      case 'l':		/* position relative to Lower left corner */
		if (center || left)
		    usage("Can only specify position once\n", 0);

		left = 1;
		px = size_arg(av, &v, 2);
		++v;
		py  = size_arg(av, &v, 0);
		break;
		
	      case '2':		/* pssun just has -2 (scale factor 2), */
	      case '0':		/* the rest of these are additional */
	      case '1':
	      case '3':
	      case '4':
	      case '5':
	      case '6':
	      case '7':
	      case '8':
	      case '9':
		if ((xscale != 0.0) || (yscale != 0.0))
		    usage("Can only specify scale/size once\n", 0);
		{
		    float scale;
		    int i = 0;
		    char *p = strchr(av[v], 'x');
		    if (p)
		    {
			*p = '\0';
			scale = num_arg(av, &v, 1);
			*p = 'x';
		    }
		    else
			scale = num_arg(av, &v, 1);

		    xscale = scale;
		    if (p)
			yscale = num_arg(&p, &i, 1);
		    else
			yscale = scale;
		    pix = 1;
		}
		break;
		
	      case 's':		/* specify absolute Size of image */
		if ((xscale != 0.0) || (yscale != 0.0))
		    usage("Can only specify scale/size once\n", 0);
		xscale = size_arg(av, &v, 2);
		++v;
		yscale = size_arg(av, &v, 0);
		break;
		
	      case 'W':
	      case 'S':		/* specify x Size, calculate y to fit */
		if ((xscale != 0.0) || (yscale != 0.0))
		    usage("Can only specify scale/size once\n", 0);
		xscale = size_arg(av, &v, 2);
		yscale = 0.0;
		break;
		
	      case 'H':
		if ((xscale != 0.0) || (yscale != 0.0))
		    usage("Can only specify scale/size once\n", 0);
		yscale = size_arg(av, &v, 2);
		xscale = 0.0;
		break;
		
	      case 'r':		/* Rotate and translate (landscape mode) */
		landscape = 1;
		break;
		
	      case 'n':		/* No showpage at end */
		showpage = 0;
		break;
		
	      case 'R':		/* Rotate */
		rotation = num_arg(av, &v, 2);
		break;
		
	      case 'c':		/* position image relative to Center */
		if (left || center)
		    usage("Can only specify position once\n", 0);
		center = 1;
		px = size_arg(av, &v, 2);
		++v;
		py = size_arg(av, &v, 0);
		break;
		
	      case 'e':		/* produce EPSF */
		epsf = 1;
		if (wfn)
		{
		    fprintf(stderr, "Warning: wrapper filename and EPSF "); 
		    fprintf(stderr, "both specified.  (Output may not be\n");
		    fprintf(stderr, "legal EPSF.)\n"); 
		}
		else
		    wfn = "share/xnews/client/snapshot/epsf.rash";
		break;
		
	      case 'g':
		grayscale = 1;
		break;

	      case 'i':		/* Integral number of pixels */
		integral_pixels = 1;
		break;
		
	      case 'm':		/* produce Monochrome image */
		do_luminance = 1;
		break;
		
	      case 'w':		/* specify Wrapper file name */
		if (epsf)
		{
		    fprintf(stderr, "Warning: wrapper filename and EPSF "); 
		    fprintf(stderr, "both specified.  (Output may not be\n");
		    fprintf(stderr, "legal EPSF.)\n"); 
		}
		else if (wfn != NULL)
		    usage("Wrapper filename may only be specified once.\n", 0);

		if (av[v][2] == 0) {
		    wfn = av[++v];
		    if (v == ac)
			usage("Missing filename argument to -w option\n", 0);
		} else {
		    wfn = av[v] + 2;
		}
		break;
		default:
		usage(NULL, 0);
	    }
	} else {
	    if (rfn != NULL)
		usage("Can only specify one raster file name\n", 0);
	    rfn = av[v];
	}
    }
    
/*
 * Now set locale to C so values fprintf'ed to file contain decimal
 * point, not comma cause postscript doesn't understand commas in floating
 * point values.
 */

    setlocale (LC_NUMERIC, "C");
    if (rfn != NULL) {
	if ((input = fopen (rfn, "r")) == NULL) {
	    fprintf (stderr, "%s: Open failed for raster file \"%s\": ",
		     av[0], rfn);
	    perror(NULL);
	    exit(13);
	}
    }
    
    if (wfn == NULL)
	wfn = "share/xnews/client/snapshot/default.rash";

    {
	char *fn;
	if ((wrapper = pl_fopen_file (wfn, "r", NULL, &fn)) == NULL) {
	    fprintf (stderr, "%s: Open failed for wrapper file \"%s\": ",
		     progname, wfn);
	    perror(NULL);
	    exit(13);
	}
    }      
    
    if (ofn != NULL) {
	if ((output = fopen (ofn, "w")) == NULL) {
	    fprintf (stderr, "%s: Open failed for output file \"%s\": ",
		     progname, ofn);
	    perror(NULL);
	    exit(13);
	}
    }      

    if ((pf = rast_load (input)) == NULL)
    {
	fprintf (stderr, "Raster file read error.\n");
	exit (13);
    }

    if (epsf)
    {
	if ((pix || integral_pixels) || ((xscale == 0.0) && (yscale == 0.0)))
	    usage("Must specify ABSOLUTE size of EPSF image\n", 0);

	if (center)
	    usage("Centering not supported in EPSF\n", 0);

	if (xscale == 0.0)
	    xscale = (yscale * pf->width ) / (float) pf->height;
	else if (yscale == 0.0)
	    yscale = (xscale * pf->height ) / (float) pf->width;

	if (landscape)
	{
	    float tmp;
	    rotation += 90;
	    tmp = px; px = -py; py = tmp;
	    px += yscale;
	}

	{
	    float p1, p2, p3;

	    p1 = -yscale * sin(M_PI * rotation / 180);
	    p2 = xscale * cos(M_PI * rotation / 180);
	    p3 = p1 + p2;

	    minx = min(min(p1, p2), min(0,p3));
	    maxx = max(max(p1, p2), max(0,p3));

	    p1 = yscale * cos(M_PI * rotation / 180);
	    p2 = xscale * sin(M_PI * rotation / 180);
	    p3 = p1 + p2;

	    miny = min(min(p1, p2), min(0,p3));
	    maxy = max(max(p1, p2), max(0,p3));
	}

	minx += px; maxx += px;
	miny += py; maxy += py;
    }

    lineb = pf->bytes_per_line;

    switch (pf->depth)
    {
      case 1:
	dump_fn = dump_mono_image;
	break;

      case 8:
      case 24:
      case 32:
	if (do_luminance || grayscale)
	    dump_fn = dump_luminance_image;
	else
	    dump_fn = dump_color_image;
	break;
    }

    while ((l = gline()) != NULL) 
    {
	while ((p = strchr(l, '$')) != NULL) 
	{
	    *p = 0;
	    fputs (l, output);
	    switch (p[1]) {

	      case 'b':
		fprintf (output, "%d", pf->depth == 1 ? 1 : 8);
		break;

	      case 'c':
		/* 
		 * the default is center, so print true unless left was
		 * specified.
		 */
		fprintf (output, "%s", left ? "false" : "true");
		break;

	      case 'd':
		t = time(NULL);
		fprintf (output, "%.24s", ctime(&t));
		break;

	      case 'h':
		(*dump_fn)(pf);
		fprintf (output, "\n");
		break;

	      case 'i':
		fprintf (output, "%s", integral_pixels ? "true" : "false");
		break;

	      case 'l':
		fprintf (output, "%s", landscape ? "true" : "false");
		break;

	      case 'n':
		fprintf (output, "%c", (do_luminance || (pf->depth == 1)) ? '1' : '3');
		break;

	      case 'p':
		fprintf (output, "%s", pix ? "true" : "false");
		break;

	      case 'r':
		fprintf (output, "%f", rotation);
		break;

	      case 's':
		if (xscale == 0.0)
		    xscale = (yscale * pf->width) / (float) pf->height;
		else if (yscale == 0.0)
		    yscale = (xscale * pf->height) / (float) pf->width;

		++p;
		if (p[1] == 'x')
		    fprintf (output, "%f", xscale);
		else if (p[1] == 'y')
		    fprintf (output, "%f", yscale);
		else
		    fprintf (output, "$s%c", p[1]);
		break;

	      case 'x':
		fprintf (output, "%d", pf->width);
		break;

	      case 'y':
		fprintf (output, "%d", pf->height);
		break;

	      case 'B':
		fprintf (output, "%f %f %f %f", minx, miny, maxx, maxy);
		break;

	      case 'S':
		fprintf (output, "%s", showpage ? "showpage" : "");
		break;

	      case 'X':
		fprintf (output, "%f", px);
		break;

	      case 'Y':
		fprintf (output, "%f", py);
		break;

	      case '\0':
		fprintf (output, "$");
		p--;
		break;

	      default:
		fprintf (output, "$%c", p[1]);
		break;
	    }
	    l = p + 2;
	}
	fputs (l, output);
    }

    exit (0);
   
}  

void
dump_mono_image(pr)
    image_t *pr;
{
    int lineb = pr->bytes_per_line;
    register int x, b = 1, y;
    int mx = (7 + pr->width) / 8; /* pad to 8-bit boundary */
    int my = pr->height;
    register unsigned char *cb = pr->data + lineb * my;

    for (y = 0; y < my; y++) 
    {
	cb -= lineb;
	for (x = 0; x < mx; x++) 
	{
	    if (b++ & 31)
		fprintf (output, "%02x", cb[x] ^ 255);
	    else
		fprintf (output, "%02x\n", cb[x] ^ 255);
	}
    }
}

void
dump_luminance_image(color_pr)
    image_t *color_pr;
{
    image_t *pr = new_image ();
    pr->width = color_pr->width;
    pr->height = color_pr->height;
    pr->depth = 8;
    pr->data = (unsigned char *) ck_zmalloc((size_t) imagelen (pr->width,
							       pr->height,
							       pr->depth));

    if (color_pr->depth == 8)
    {
	if (grayscale)
	{
	    free_image (pr);
	    pr = color_pr;
	}
	else 
	    rgb8_to_y(color_pr, pr);
    }
/*
 * Note: we stopped using the pixrect library, and now just read
 * in the file. If the image is depth 24, we convert to 32 to be
 * more compatible with the rest of this program which always assumed
 * that the image was either depth 1, 8, or 32.
 */

    else if (color_pr->depth == 32) /* pr_load always converts 24-bit to xbgr? */
	xbgr_to_y(color_pr, pr);
    else
    {
	fprintf(stderr, "%s: Cannot print luminance from %d-bit pixrect\n",
		progname, color_pr->depth);
	exit(13);
    }

    {
        int lineb = linelen (pr->width, pr->depth);
	register int x, b = 1, y;
	int mx = pr->width;
	int my = pr->height;
	register unsigned char *cb = pr->data + lineb * my;

	for (y = 0; y < my; y++) 
	{
	    cb -= lineb;
	    for (x = 0; x < mx; x++) 
	    {
		if (b++ & 31)
		    fprintf (output, "%02x", cb[x]);
		else
		    fprintf (output, "%02x\n", cb[x]);
	    }
	}
    }
}

void
dump_color_image(pr)
    image_t *pr;
{
    int lineb = pr->bytes_per_line;
    register int x, b = 1, y;
    register unsigned char *cb;
    int mx = pr->width;
    int my = pr->height;

    if ((pr->depth != 32) && (pr->depth != 8))
    {
	fprintf(stderr, "%s: Cannot print color from %d-bit pixrect\n",
		progname, pr->depth);
	exit(13);
    }

    if (pr->depth == 32)
    {
	for (y = my; --y >= 0;) 
	{
	    cb = pr->data + lineb * y;
	    for (x = 0; x < mx; ++x, cb += 4)
	    {
		fprintf (output, "%02x%02x%02x", cb[3], cb[2], cb[1]);
		if ((b++ & 7) == 0)
		    putc('\n', output);
	    }
	}
    }
    else
    {
	for (y = my; --y >= 0;) 
	{
	    cb = pr->data + lineb * y;
	    for (x = 0; x < mx; ++x, ++cb)
	    {
		fprintf (output, "%02x%02x%02x", pr->red[*cb], 
			pr->green[*cb], pr->blue[*cb]);
		if ((b++ & 7) == 0)
		    putc('\n', output);
	    }
	}
    }
}

/*******************************************************
 *
 * Luminance conversion code provided by David Berry.
 *
 *******************************************************/

/*
 * Copyright (c) 1990 by Sun Microsystems, Inc.
 */


/*
 * Routines to conert 32 bit and 8-bit RGB to Y, luminance
 */

/* 32 bit XBGR pixel to red, green, blue values */
#define RED(v)		((v) & 0xFF)
#define GREEN(v)	(((v)>>8) & 0xFF)
#define BLUE(v)		(((v)>>16) & 0xFF)

/*
 * This set of tables converts RGB into Luma using
 * the standard RGB to Y weights.
 * 0.3R 0.59G 0.11B. 
 * This is quicker than multiplys
 */
static unsigned int lumar[] = {
	    0,   77,  153,  230,  306,  383,  459,  536,
	  612,  689,  765,  842,  919,  995, 1072, 1148,
	 1225, 1301, 1378, 1454, 1531, 1607, 1684, 1761,
	 1837, 1914, 1990, 2067, 2143, 2220, 2296, 2373,
	 2449, 2526, 2602, 2679, 2756, 2832, 2909, 2985,
	 3062, 3138, 3215, 3291, 3368, 3444, 3521, 3598,
	 3674, 3751, 3827, 3904, 3980, 4057, 4133, 4210,
	 4286, 4363, 4440, 4516, 4593, 4669, 4746, 4822,
	 4899, 4975, 5052, 5128, 5205, 5282, 5358, 5435,
	 5511, 5588, 5664, 5741, 5817, 5894, 5970, 6047,
	 6124, 6200, 6277, 6353, 6430, 6506, 6583, 6659,
	 6736, 6812, 6889, 6966, 7042, 7119, 7195, 7272,
	 7348, 7425, 7501, 7578, 7654, 7731, 7807, 7884,
	 7961, 8037, 8114, 8190, 8267, 8343, 8420, 8496,
	 8573, 8649, 8726, 8803, 8879, 8956, 9032, 9109,
	 9185, 9262, 9338, 9415, 9491, 9568, 9645, 9721,
	 9798, 9874, 9951,10027,10104,10180,10257,10333,
	10410,10487,10563,10640,10716,10793,10869,10946,
	11022,11099,11175,11252,11329,11405,11482,11558,
	11635,11711,11788,11864,11941,12017,12094,12170,
	12247,12324,12400,12477,12553,12630,12706,12783,
	12859,12936,13012,13089,13166,13242,13319,13395,
	13472,13548,13625,13701,13778,13854,13931,14008,
	14084,14161,14237,14314,14390,14467,14543,14620,
	14696,14773,14850,14926,15003,15079,15156,15232,
	15309,15385,15462,15538,15615,15692,15768,15845,
	15921,15998,16074,16151,16227,16304,16380,16457,
	16534,16610,16687,16763,16840,16916,16993,17069,
	17146,17222,17299,17375,17452,17529,17605,17682,
	17758,17835,17911,17988,18064,18141,18217,18294,
	18371,18447,18524,18600,18677,18753,18830,18906,
	18983,19059,19136,19213,19289,19366,19442,19519,} ;

static unsigned int lumag[] = {
	    0,  150,  301,  451,  601,  751,  902, 1052,
	 1202, 1352, 1503, 1653, 1803, 1954, 2104, 2254,
	 2404, 2555, 2705, 2855, 3005, 3156, 3306, 3456,
	 3607, 3757, 3907, 4057, 4208, 4358, 4508, 4658,
	 4809, 4959, 5109, 5260, 5410, 5560, 5710, 5861,
	 6011, 6161, 6311, 6462, 6612, 6762, 6913, 7063,
	 7213, 7363, 7514, 7664, 7814, 7964, 8115, 8265,
	 8415, 8566, 8716, 8866, 9016, 9167, 9317, 9467,
	 9617, 9768, 9918,10068,10218,10369,10519,10669,
	10820,10970,11120,11270,11421,11571,11721,11871,
	12022,12172,12322,12473,12623,12773,12923,13074,
	13224,13374,13524,13675,13825,13975,14126,14276,
	14426,14576,14727,14877,15027,15177,15328,15478,
	15628,15779,15929,16079,16229,16380,16530,16680,
	16830,16981,17131,17281,17432,17582,17732,17882,
	18033,18183,18333,18483,18634,18784,18934,19085,
	19235,19385,19535,19686,19836,19986,20136,20287,
	20437,20587,20738,20888,21038,21188,21339,21489,
	21639,21789,21940,22090,22240,22391,22541,22691,
	22841,22992,23142,23292,23442,23593,23743,23893,
	24044,24194,24344,24494,24645,24795,24945,25095,
	25246,25396,25546,25697,25847,25997,26147,26298,
	26448,26598,26748,26899,27049,27199,27350,27500,
	27650,27800,27951,28101,28251,28401,28552,28702,
	28852,29002,29153,29303,29453,29604,29754,29904,
	30054,30205,30355,30505,30655,30806,30956,31106,
	31257,31407,31557,31707,31858,32008,32158,32308,
	32459,32609,32759,32910,33060,33210,33360,33511,
	33661,33811,33961,34112,34262,34412,34563,34713,
	34863,35013,35164,35314,35464,35614,35765,35915,
	36065,36216,36366,36516,36666,36817,36967,37117,
	37267,37418,37568,37718,37869,38019,38169,38319,} ;

static unsigned int lumab[] = {
	    0,   29,   58,   88,  117,  146,  175,  204,
	  233,  263,  292,  321,  350,  379,  409,  438,
	  467,  496,  525,  554,  584,  613,  642,  671,
	  700,  730,  759,  788,  817,  846,  876,  905,
	  934,  963,  992, 1021, 1051, 1080, 1109, 1138,
	 1167, 1197, 1226, 1255, 1284, 1313, 1342, 1372,
	 1401, 1430, 1459, 1488, 1518, 1547, 1576, 1605,
	 1634, 1663, 1693, 1722, 1751, 1780, 1809, 1839,
	 1868, 1897, 1926, 1955, 1985, 2014, 2043, 2072,
	 2101, 2130, 2160, 2189, 2218, 2247, 2276, 2306,
	 2335, 2364, 2393, 2422, 2451, 2481, 2510, 2539,
	 2568, 2597, 2627, 2656, 2685, 2714, 2743, 2772,
	 2802, 2831, 2860, 2889, 2918, 2948, 2977, 3006,
	 3035, 3064, 3094, 3123, 3152, 3181, 3210, 3239,
	 3269, 3298, 3327, 3356, 3385, 3415, 3444, 3473,
	 3502, 3531, 3560, 3590, 3619, 3648, 3677, 3706,
	 3736, 3765, 3794, 3823, 3852, 3881, 3911, 3940,
	 3969, 3998, 4027, 4057, 4086, 4115, 4144, 4173,
	 4202, 4232, 4261, 4290, 4319, 4348, 4378, 4407,
	 4436, 4465, 4494, 4524, 4553, 4582, 4611, 4640,
	 4669, 4699, 4728, 4757, 4786, 4815, 4845, 4874,
	 4903, 4932, 4961, 4990, 5020, 5049, 5078, 5107,
	 5136, 5166, 5195, 5224, 5253, 5282, 5311, 5341,
	 5370, 5399, 5428, 5457, 5487, 5516, 5545, 5574,
	 5603, 5633, 5662, 5691, 5720, 5749, 5778, 5808,
	 5837, 5866, 5895, 5924, 5954, 5983, 6012, 6041,
	 6070, 6099, 6129, 6158, 6187, 6216, 6245, 6275,
	 6304, 6333, 6362, 6391, 6420, 6450, 6479, 6508,
	 6537, 6566, 6596, 6625, 6654, 6683, 6712, 6742,
	 6771, 6800, 6829, 6858, 6887, 6917, 6946, 6975,
	 7004, 7033, 7063, 7092, 7121, 7150, 7179, 7208,
	 7238, 7267, 7296, 7325, 7354, 7384, 7413, 7442,} ;

/*
 * Convert xbgr images to Y
 */
xbgr_to_y(src_pr, opr)
    image_t *src_pr, *opr;
{
    register int x, y;
    register u_char *data_out;
    register unsigned int *data_in;
    int in_inc, out_inc;

    if (!src_pr || !opr)
	return(-1);

    data_out = (u_char *) opr->data;
    out_inc = linelen (opr->width, opr->depth);

    data_in = (unsigned int *) src_pr->data;
    in_inc = src_pr->bytes_per_line/4;

    for(y = 0; y < src_pr->height; y++)  
    {
	for(x = 0; x < src_pr->width; x++)
	    data_out[x] = (lumar[RED(data_in[x])] + 
			   lumag[GREEN(data_in[x])] +
			   lumab[BLUE(data_in[x])]) >> 8;
	data_in += in_inc;	
	data_out += out_inc;
    }
    return(0);
}


/*
 * Convert 8-bit indexed colour to grayscale
 */
rgb8_to_y(src_pr, opr)
    image_t *src_pr;
    image_t *opr;
{
    register int x, y;
    register u_char *data_in, *data_out;
    int in_inc, out_inc;

    if (!src_pr || !opr || src_pr->cmapused == 0)
	return(-1);

    data_out = (u_char *) opr->data;
    out_inc = linelen (opr->width, opr->depth);

    data_in = (u_char *) src_pr->data;
    in_inc = src_pr->bytes_per_line;

    for(y = 0; y < src_pr->height; y++)  
    {
	for(x = 0; x < src_pr->width; x++)
	    data_out[x] = (lumar[src_pr->red[data_in[x]]] +
			   lumag[src_pr->green[data_in[x]]] +
			   lumab[src_pr->blue[data_in[x]]]) >> 8;
	data_out += out_inc;
	data_in += in_inc;
    }
    return(0);
}

FILE *pl_fopen_file(fn, type, path, new_path)
char *fn, *path, **new_path, *type;
{
    char *name, *full_path;
    char *pl_path;
    struct stat buf;
    register int i=0, length, j;
    FILE *fp;


    if ((pl_path = getenv("OPENWINHOME")) == (char *)NULL) {
        if (!path) {
            (void)fprintf(stderr, "pl_fopen_path: null OPENWINHOME\n");
            return(NULL);
        }
    }    

    name = malloc(200);
    full_path = malloc(MAXPATHLEN);

    length = strlen(fn);
    if (*fn == ' ')
        fn++;

    if (*fn == '/') {                   /* absolute */
        (void)strcpy(full_path, fn);
        goto done1;
    }
 
    while (*fn != (char)NULL)           /* get name */
        name[i++] = *fn++;
    name[i] = '\0';
       
 
    /* if there's user specified path, then use that in combo with
     * given filename
     */
    if (path) {
        (void)sprintf(full_path, "%s/%s", path, name);
        if ((fp = fopen(full_path, type)) == (FILE *)NULL)
            goto err_return;
        else
            goto good_return;
    }
    else {
        length = strlen(pl_path);
        i = 0;
 
        while (i < length) {
            j = 0;
            memset(full_path, 0, strlen(full_path));
            while (pl_path[i] != ':' && pl_path[i] != '\0')
                full_path[j++] = pl_path[i++];
 
            if (pl_path[i] == ':')
                i++;

            full_path[j] = '\0';
            (void)strcat(full_path, "/");
            (void)strcat(full_path, name);
done1:
                if ((fp = fopen(full_path, type)) == (FILE *)NULL) {
                    if (i == length)
                        goto err_return;
                }
                else
                    goto good_return;
        }
    }    

err_return:
        free(name);
        free(full_path);
        return(NULL);

good_return:
        *new_path = strdup(full_path);
        free(name);
        free(full_path);
        return(fp);

}

