#pragma ident "@(#)realxfishdb.c	1.13 93/09/02	SMI"

/*

  *	Original Author Unknow.

  *	8/10/88 - Ported from X10 to X11R3 by:

		Jonathan Greenblatt (jonnyg@rover.umd.edu)

  *	Cleaned up by Dave Lemke (lemke@sun.com)

  *	Ported to monocrome by Jonathan Greenblatt (jonnyg@rover.umd.edu)

  * 	Added touch of reality by Bruce McIntyre (bmac@Eng.Sun.COM)

  TODO:

	Parameter parsing needs to be redone.

*/

#ifndef HPUX
#include <sys/time.h>
#else
#include <time.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <string.h>
#include <ctype.h>
#include <sys/types.h>
#include <X11/extensions/multibuf.h>

/* constants are based on rand(3C) returning an integer between 0 and 32767 */

/*
 * Sun raster file stuff
 */

union {
    char unf[32]; /* unformated */
    struct {
        unsigned char magic[4];   /* magic number */
        unsigned char width[4];   /* width of image in pixels */
        unsigned char height[4];  /* height of image in pixels */
        unsigned char depth[4];   /* depth of each pixel */
        unsigned char length[4];  /* length of the image in bytes */
        unsigned char type[4];    /* format of file */
        unsigned char maptype[4]; /* type of colormap */
        unsigned char maplen[4];  /* length of colormap in bytes */
    } fc; /* formated */
    struct {
        unsigned long magic;   /* magic number */
        unsigned long width;   /* width of image in pixels */
        unsigned long height;  /* height of image in pixels */
        unsigned long depth;   /* depth of each pixel */
        unsigned long length;  /* length of the image in bytes */
        unsigned long type;    /* format of file */
        unsigned long maptype; /* type of colormap */
        unsigned long maplen;  /* length of colormap in bytes */
    } fi; /* formated */
} sr_header;

struct {
    unsigned long length;
    unsigned char red[256];
    unsigned char green[256];
    unsigned char blue[256];
} sr_cmap;


/*
 * Following the header is the colormap (unless maplen is zero) then
 * the image.  Each row of the image is rounded to 2 bytes.
 */

#define RMAGICNUMBER 0x59a66a95 /* magic number of this file type */

/*
 * These are the possible file formats
 */

#define ROLD       0 /* old format, see /usr/include/rasterfile.h */
#define RSTANDARD  1 /* standard format */
#define RRLENCODED 2 /* run length encoding to compress the image */

/* 
 * These are the possible colormap types.  If it's in RGB format,
 * the map is made up of three byte arrays (red, green, then blue)
 * that are each 1/3 of the colormap length.
 */

#define RNOMAP  0 /* no colormap follows the header */
#define RRGBMAP 1 /* rgb colormap */
#define RRAWMAP 2 /* raw colormap; good luck */

#define RESC 128 /* run-length encoding escape character */

	Multibuffer buffer[2];
	XColor default_colors[256];
 	int intensity[256];
	char color_match[256];
	int draw_buffnum = 0;
	int mbevbase, mberrbase;
     	int fwidth[32];
	int fheight[32];
	int fnums[32] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
        int maxcolors = 0;
        int use_props = 0;
	int maxwidth,
	    maxheight;
	unsigned int pixelscopied;

	int RAND_I_1_16,
	    RAND_I_1_4,
	    RAND_I_1_2,
	    RAND_I_3_4;

	float RAND_F_1_8,
	      RAND_F_MAX;

unsigned char xbBits[9][8] = {
  {0x00 },
  {0xff },
  {0xff,0xff },
  {0xff,0xff,0xff },
  {0x06,0x0b,0x09,0x06 },
  {0x0e,0x17,0x13,0x11,0x0e },
  {0x1e,0x2d,0x27,0x21,0x21,0x1e },
  {0x1c,0x2a,0x4d,0x47,0x41,0x22,0x1c },
  {0x3c,0x4e,0x8b,0x87,0x81,0x81,0x42,0x3c }
};


/* typedefs for bubble and fish structures, also caddr_t (not used in X.h) */
typedef struct {
    int         x,
                y,
                s,
                i;
}           bubble;
typedef struct {
    int         n,
		prevx,
		prevy,
		x,
                y,
    		w,
		h,
                d,
                i;
}           fish;
typedef unsigned char *caddrt;


/* bubble increment and yes check tables */
int         binc[] = {0, 64, 56, 48, 40, 32, 24, 16, 8};
char       *yess[] = {"yes", "Yes", "YES", "on", "On", "ON"};


char       *pname,		/* program name from argv[0] */
            sname[128],		/* host:display specification */
            cname[128],		/* colorname specification */
	    clienthome[128],	/* client's home */
	    fishhome[128],	/* fish home directory */
	    fishfile[128];	/* used for file name construction */

int         seasickness = 0,	/* wave flag */
	    wavex = 0,		/* current position of wave */
	    waveinc = 64,	/* current increment of wave */
	    waveoffset = 1,	/* current offset of wave */
	    blimit = 0,		/* bubble limit */
            flimit = 2,		/* fish limit */
  	    dbl_bufrd = 0,	/* single or double buffered mode */
	    quadro = 0,		/* special double buffer mode */
            pmode = 1,		/* pop mode, (1 for lower, 0 for raise) */
  	    perf = 0,		/* high performance mode switch */
  	    timed = 0,		/* timer switch */
            width,		/* width of initial window in pixels */
            height,		/* height of initial window in pixels */
            screen;		/* Default screen of this display */
	    
int 	    Init_B,
	    *cmap;
				/* Initialize bubbles with random y value */
double      rate = 1.0,		/* update interval in seconds */
            smooth = 1.0;	/* smoothness increment multiplier */
bubble     *binfo;		/* bubble info structures, allocated 
				 * dynamically  */
fish       *finfo;		/* fish info structures, allocated dynamically */
Display    *Dpy;
XImage	   *xback;		/* background image */
XImage     *xfish[64];		/* fish pixmaps (0 is left-fish, 1 is
				 * right-fish) */
XImage     *xfishand[64];	/* fish boundary pixmaps (0 is left-fish, 1 is
				 * right-fish) */
Pixmap	    pback = 0;
Pixmap      pfish[64] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
Pixmap      pfishand[64] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
Pixmap	    pfishmerge[32] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};

Pixmap      xbubbles[9];	/* bubbles bitmaps (1 to 8, by size in pixels)*/
Window      wid;		/* aqaurium window */
unsigned long white, black,bcolor;
Colormap    colormap;
unsigned    int pixcolorcnt[256];
GC          gc,
            bgc;


/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
Output desired error message and exit.
* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
void 
msgdie(message)
    char       *message;
{
    fprintf(stderr, "%s: %s\n", pname, message);
    exit(1);
}


/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
Set up program defaults, get X defaults, parse command line using getopts.
* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
void 
parse(argc, argv)
    int         argc;
    char      **argv;
{
    int         c,
                i;
    char       *p;
    extern int  optind;
    extern char *optarg;
    extern double atof();

    pname = argv[0];
    strcpy(sname, getenv("DISPLAY"));
    strcpy(cname, "AquaMarine");

    if ((p = XGetDefault(Dpy, pname, "BubbleLimit")) != NULL)
	blimit = atoi(p);
    if ((p = XGetDefault(Dpy, pname, "Color")) != NULL)
	strcpy(cname, p);
    if ((p = XGetDefault(Dpy, pname, "FishLimit")) != NULL)
	flimit = atoi(p);
    if ((p = XGetDefault(Dpy, pname, "IncMult")) != NULL)
	smooth = atof(p);
    if ((p = XGetDefault(Dpy, pname, "Rate")) != NULL)
	rate = atof(p);
    if ((p = XGetDefault(Dpy, pname, "Secure")) != NULL)
	for (i = 0; i < 6; i++)
	    if (strcmp(p, yess[i]) == 0)
		pmode = 0;

    while ((c = getopt(argc, argv, "b:c:f:i:r:spdqt")) != EOF) {
	switch (c) {
	case 'b':
	    blimit = atoi(optarg);
	    break;
	case 'c':
	    strcpy(cname, optarg);
	    break;
	case 'f':
	    flimit = atoi(optarg);
	    break;
	case 'i':
	    smooth = atof(optarg);
	    break;
	case 'r':
	    rate = atof(optarg);
	    break;
	case 's':
	    pmode = 0;
	    break;
	case 'd':
	    dbl_bufrd = 1;
	    break;
	case 'q':
	    quadro = 1;
	    dbl_bufrd = 1;
	    break;
	case 'p':
	    perf = 1;
	    break;
	case 't':
	    timed = 1;
	    break;
	case '?':
	    fprintf(stderr, "usage: %s [-b limit][-c color][-f limit][-i mult][-r rate][-d][-q][-p][-t][-s][host:display]\n", pname);
	    exit(1);
	}
    }

    if (optind < argc)
	strcpy(sname, argv[optind]);
}

putfish(f)
    fish       *f;
{
    int 	i;
    XGCValues	gcv;

    i = (f->d - 1) + (f->n << 1);

    if (i < 0) i = 0;

    if (dbl_bufrd) {
	if ((f->prevx != -32768) && quadro)
	    XCopyArea(Dpy, pback, buffer[draw_buffnum], gc, f->prevx, f->prevy, f->w, f->h, f->prevx, f->prevy);
	if (timed)
	    pixelscopied += (f->w * f->h);
	f->prevx = f->x;
	f->prevy = f->y;
	gcv.function = GXand;
	XChangeGC(Dpy, gc, GCFunction, &gcv);
	XCopyArea(Dpy, pfishand[i], buffer[draw_buffnum], gc, 0, 0, f->w, f->h, f->x, f->y);
	if (timed)
	    pixelscopied += (f->w * f->h);
	gcv.function = GXor;
	XChangeGC(Dpy, gc, GCFunction, &gcv);
	XCopyArea(Dpy, pfish[i], buffer[draw_buffnum], gc, 0, 0, f->w, f->h, f->x, f->y);
	if (timed)
	    pixelscopied += (f->w * f->h);
	gcv.function = GXcopy;
	XChangeGC(Dpy, gc, GCFunction, &gcv);
    } else {
	XCopyArea(Dpy,pback,pfishmerge[f->n],gc,f->x,f->y,f->w,f->h,0,0);
	if (timed)
	    pixelscopied += (f->w * f->h);
	gcv.function = GXand;
	XChangeGC(Dpy, gc, GCFunction, &gcv);
	XCopyArea(Dpy,pfishand[i],pfishmerge[f->n],gc, 0, 0, f->w, f->h, 0, 0);
	if (timed)
	    pixelscopied += (f->w * f->h);
	gcv.function = GXor;
	XChangeGC(Dpy, gc, GCFunction, &gcv);
	XCopyArea(Dpy,pfish[i],pfishmerge[f->n], gc, 0, 0, f->w, f->h, 0, 0);
	if (timed)
	    pixelscopied += (f->w * f->h);
	gcv.function = GXcopy;
	XChangeGC(Dpy, gc, GCFunction, &gcv);
	XCopyArea(Dpy,pfishmerge[f->n],buffer[draw_buffnum], gc, 0, 0, f->w, f->h, f->x, f->y);
	if (timed)
	    pixelscopied += (f->w * f->h);
    }
}

putbubble(b, s, c)
    bubble     *b;
    int         s;
    unsigned long c;
{
    XGCValues   gcv;

    gcv.foreground = c;
    gcv.clip_mask = xbubbles[s];
    gcv.clip_x_origin = b->x;
    gcv.clip_y_origin = b->y;
    XChangeGC(Dpy, bgc, GCClipMask | GCClipXOrigin |
	      GCClipYOrigin, &gcv);
    XFillRectangle(Dpy, buffer[draw_buffnum], bgc, b->x, b->y, s, s);

}


/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
Load Sun Raster file.
* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
char *sr_load(fp)
FILE	*fp;
{
register int	i;
register char *p, *pixels;

	for (i = 0; i < 32; i++)
	    if ((sr_header.unf[i] = getc(fp)) == EOF)
		return(0);

	/* Make the raster header information is in proper byte order. */
    sr_header.fi.magic = htonl( sr_header.fi.magic); 
    sr_header.fi.width = htonl(sr_header.fi.width);
    sr_header.fi.height = htonl(sr_header.fi.height);
    sr_header.fi.depth = htonl(sr_header.fi.depth);
    sr_header.fi.length = htonl(sr_header.fi.length);
    sr_header.fi.type = htonl(sr_header.fi.type);
    sr_header.fi.maptype = htonl(sr_header.fi.maptype);
    sr_header.fi.maplen = htonl(sr_header.fi.maplen);

	if (sr_header.fi.magic != RMAGICNUMBER) {
	    printf("\nBad magic number in raster image file.");
	    return(0);
	}

	if (sr_header.fi.type != RSTANDARD) {
	    printf("\nUnsupported raster image file type.");
	    return(0);
	}

	if (sr_header.fi.maptype != RRGBMAP) {
	    printf("\nUnsupported raster image file colormap type.");
	    return(0);
	}

	if (sr_header.fi.maplen == 0) {
	    printf("\nRaster image file colormap is empty.");
	    return(0);
	}

	sr_cmap.length = sr_header.fi.maplen / 3;

	for (i = 0; i < sr_cmap.length; i++)
	    sr_cmap.red[i] = getc(fp);

	for (i = 0; i < sr_cmap.length; i++)
	    sr_cmap.green[i] = getc(fp);

	for (i = 0; i < sr_cmap.length; i++)
	    sr_cmap.blue[i] = getc(fp);

	if ((pixels = malloc(sr_header.fi.length)) == NULL) {
	    printf("\nNot enough memory to load raster image file.");
	    return(0);
	}	    	

	p = pixels;

	for (i = 0; i < sr_header.fi.length; i++)
	    *p++ = getc(fp);

	return(pixels);
}


/* Compare two colors with a fuzz factor */
static int
clrdiff(color1, color2, fuzz)
  XColor *color1, *color2;
  int fuzz;
{
    if ((abs(color1->red - color2->red) < fuzz)
	  && (abs(color1->green - color2->green) < fuzz)
	  && (abs(color1->blue - color2->blue) < fuzz))
	return 0;		      /* Nearly the same color */
    else
	return 1;		      /* Different */
}

/* Searches default color map for closely matching color */
static int
clrclose(color)
  XColor *color;
{
  register int i, j, color_intensity, matchcount = 0;

    color_intensity = ((color->red / 256) * 0.30) +
		       ((color->green / 256) * 0.59) + 	
		       ((color->blue / 256) * 0.11);

    for (i = 0; i < 256; i++)
	if ((abs(color->red - default_colors[i].red) > 1*256) ||
	    (abs(color->green - default_colors[i].green) > 0) ||
	    (abs(color->blue - default_colors[i].blue) > 3*256))
	    color_match[i] = 0;
	else {
	    color_match[i] = 1;
	    j = i;
	    matchcount++;
	}
	
    if (matchcount) {
	    i = 0;
	    while ((matchcount > 0) && (i < 256)) {
		if (color_match[i]) {
		    matchcount--;
		    if (abs(color_intensity - intensity[i]) <
			abs(color_intensity - intensity[j]))
			    j = i;
		}
		i++; 
	    }
	    color->pixel = default_colors[j].pixel;
	    color->red = default_colors[j].red;
	    color->green = default_colors[j].green;
	    color->blue = default_colors[j].blue;
	    return 1;
    }
    return 0;
}


/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
Initialize colormap for background color and required fish colors.
The fish colors are coded in xfishy.c as a trio of tables.
* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
void 
init_colormap()
{
FILE	*fp, *fopen();
register int x, y;
unsigned char *pixel;
char *pixels;
register caddrt p,
                q,
		bpixels;
register int 	i,
                j,
		k,
		black,
		result,
		linebytes;
XColor      	hdef,
                edef,
		black_color;

    colormap = XDefaultColormap(Dpy, screen);

    black = BlackPixel(Dpy, screen);

    black_color.red = 0;
    black_color.green = 0;
    black_color.blue = 0;
    
    if (colormap == NULL)
	return;

    for (i = 0; i < 256; i++)
	default_colors[i].pixel = i;

    XQueryColors(Dpy, colormap, default_colors, 256);

    for (i = 0; i < 256; i++)
	intensity[i] = ((default_colors[i].red / 256) * 0.30) +
		       ((default_colors[i].green / 256) * 0.59) + 	
		       ((default_colors[i].blue / 256) * 0.11);

    cmap = (int *) malloc(256 * sizeof(int));

    for (i = 0; i < 256; i++) pixcolorcnt[i] = 0;

    fishfile[0] = 0;
    strcat(fishfile, fishhome);
    strcat(fishfile, "/b2rot.im8");

    if (!(fp = fopen(fishfile, "r"))) {
	printf("Couldn't open %s.  Try setting FISHHOME.\n", fishfile);
	exit(1);
    }

    if (!(pixels = sr_load(fp))) {
	printf("Couldn't read background raster image file.\n");
	exit(1);
    }
	
    linebytes = (sr_header.fi.width + 1) & 0xFFFFFFFE;

    fclose(fp);

    if (sr_header.fi.depth != 8)
	printf("Non-depth 8 image.\n");

    bpixels = (caddrt) malloc(width * height);

    p = bpixels;

    i = sr_header.fi.height - height;
    if (i < 0) i = 0;
    j = 200;
    if (width > 1280) j = 0;

    for (y = i; y < i + height; y++) {
	pixel = (unsigned char *) pixels + (y * linebytes);
	for (x = j; x < j + width; x++) {
	    pixcolorcnt[pixel[x]] += 1;
	    *p++ = pixel[x];   
	}
    }

    free(pixels);

    for (i = 255; i >= 0; i--) {
	if (pixcolorcnt[i] != 0) {
	    hdef.red = ((unsigned char)sr_cmap.red[i])*256;
	    hdef.green = ((unsigned char)sr_cmap.green[i])*256;
	    hdef.blue = ((unsigned char)sr_cmap.blue[i])*256;
	    if (clrdiff(&hdef,&black_color,85*256)) {
	        if (!clrclose(&hdef)) {
		    result = XAllocColor(Dpy, colormap, &hdef);
		    if ((result == 0) && (maxcolors < 55)) {
		       printf("\nCouldn't allocate enough colors to produce good images.\n");
		       exit(1);
		    }
		    if (result != 0)
		       maxcolors++;
		} else {
		    XAllocColor(Dpy, colormap, &hdef);
	        }
	    }	    
	    else hdef.pixel = black;
	    cmap[i] = hdef.pixel;
	}
    }
    bcolor = cmap[maxcolors - 2];

    cmap[0] = cmap[29];
/*  cmap[255] = cmap[29];
*/
    printf("\nNumber of colors allocated for background image are %i ",maxcolors);
    
    p = bpixels;

    j = width * height;

    for (i = 0; i < j; i++) {
	*p = cmap[*p];
	p++;
    }

    xback = XCreateImage(Dpy, DefaultVisual(Dpy, screen), 8,
	    ZPixmap, 0, bpixels, width, height, 8, width);

}


/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
Attempt to allocate new color in color map
* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
void 
alloc_newcolor(index, red, green, blue)
int index, red, green, blue;
{
register int 	result;
XColor      	hdef, black_color;

    black_color.red = 0;
    black_color.green = 0;
    black_color.blue = 0;
    
    hdef.red = red * 256;
    hdef.green = green * 256;
    hdef.blue = blue * 256;
    result = XAllocColor(Dpy, colormap, &hdef);
    if ((result == 0) && (maxcolors < 55)) {
	printf("\nCouldn't allocate enough colors to produce good images.\n");
	exit(1);
    }
    pixcolorcnt[index] += 1;

    if (result != 0) {
        maxcolors++;
        cmap[index] = hdef.pixel;
    } else {
	if (clrdiff(&hdef,&black_color,85*256)) {
	    if (clrclose(&hdef))
		cmap[index] = hdef.pixel;
	    else {
		register int i, j, color_intensity;

		color_intensity = ((hdef.red / 256) * 0.30) +
			   ((hdef.green / 256) * 0.59) + 	
			   ((hdef.blue / 256) * 0.11);
		j = 255;
		i = 255;
		while (i > 0) {
		    if (abs(color_intensity - intensity[i]) <
			abs(color_intensity - intensity[j]))
			    j = i;
		    i--;
		}
		cmap[index] = default_colors[j].pixel;
	    }
	} else cmap[index] = black; 
    }
}


/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
Calibrate the pixmaps and bimaps.  The left-fish data is coded in xfishy.c,
this is transformed to create the right-fish.  The eight bubbles are coded
in xfishy.c as a two dimensional array.
* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
void 
init_pixmap()
{
FILE	*fp, *fopen();
register int x, y;
unsigned char *pixel;
char *pixels;
register caddrt p,
                q;
caddrt		fpixels[64],
		apixels[64];
register int 	i,
                j,
		k,
		fnum,
		maxfish,
		linebytes;

    for (i = 0; i < 64; i++) { fpixels[i] = NULL; apixels[i] = NULL; }

    maxwidth = maxheight = 0;

    maxfish = flimit;

    if (maxfish > 32) maxfish = 32;

    for (k = 0; k < maxfish; k++) {

	if (use_props) fnum = fnums[k];
	else fnum = k;

	fishfile[0] = 0;
	strcat(fishfile, fishhome);

	switch(fnum) {
	    case 0:
		strcat(fishfile, "/f12_tus.im8");
	    break;

	    case 1:
		strcat(fishfile, "/f8_tus.im8");
	    break;

	    case 2:
		strcat(fishfile, "/f25_tus.im8");
	    break;

	    case 3:
		strcat(fishfile, "/f7_tus.im8");
	    break;

	    case 4:
		strcat(fishfile, "/f15_tus.im8");
	    break;

	    case 5:
		strcat(fishfile, "/f11_tus.im8");
	    break;

	    case 6:
		strcat(fishfile, "/f10_tus.im8");
	    break;

	    case 7:
		strcat(fishfile, "/f21_tus.im8");
	    break;

	    case 8:
		strcat(fishfile, "/f6_tus.im8");
	    break;

	    case 9:
		strcat(fishfile, "/f2_tus.im8");
	    break;

	    case 10:
		strcat(fishfile, "/f16_tus.im8");
	    break;

	    case 11:
		strcat(fishfile, "/f9_tus.im8");
	    break;

	    case 12:
		strcat(fishfile, "/f28_tus.im8");
	    break;

	    case 13:
		strcat(fishfile, "/f17_tus.im8");
	    break;

	    case 14:
		strcat(fishfile, "/f36_tus.im8");
	    break;

	    case 15:
		strcat(fishfile, "/f23_tus.im8");
	    break;

	    case 16:
		strcat(fishfile, "/f24_tus.im8");
	    break;

	    case 17:
		strcat(fishfile, "/f26_tus.im8");
	    break;

	    case 18:
		strcat(fishfile, "/f27_tus.im8");
	    break;

	    case 19:
		strcat(fishfile, "/f29_tus.im8");
	    break;

	    case 20:
		strcat(fishfile, "/f30_tus.im8");
	    break;

	    case 21:
		strcat(fishfile, "/f31_tus.im8");
	    break;

	    case 22:
		strcat(fishfile, "/f32_tus.im8");
	    break;

	    case 23:
		strcat(fishfile, "/f34_tus.im8");
	    break;

	    case 24:
		strcat(fishfile, "/f35_tus.im8");
	    break;

	    case 25:
		strcat(fishfile, "/f37_tus.im8");
	    break;

	    case 26:
		strcat(fishfile, "/f38_tus.im8");
	    break;

	    case 27:
		strcat(fishfile, "/f39_tus.im8");
	    break;

	    case 28:
		strcat(fishfile, "/f40_tus.im8");
	    break;

	    case 29:
		strcat(fishfile, "/f41_tus.im8");
	    break;

	    case 30:
		strcat(fishfile, "/f42_tus.im8");
	    break;

	    case 31:
		strcat(fishfile, "/f43_tus.im8");
	    break;

	    case 32:
		strcat(fishfile, "/f46_tus.im8");
	    break;

	    case 33:
		strcat(fishfile, "/f48_tus.im8");
	    break;

	    case 34:
		strcat(fishfile, "/f49_tus.im8");
	    break;

	    case 35:
		strcat(fishfile, "/f50_tus.im8");
	    break;

	    case 36:
		strcat(fishfile, "/f51_tus.im8");
	    break;

	    case 37:
		strcat(fishfile, "/f52_tus.im8");
	    break;

	    case 38:
		strcat(fishfile, "/f53_tus.im8");
	    break;

	    case 39:
		strcat(fishfile, "/swimmer_tus.im8");
	    break;
	}


	if (!(fp = fopen(fishfile, "r"))) {
	    printf("Couldn't open %s.  Try setting FISHHOME.\n", fishfile);
	    exit(1);
	}
	if (!(pixels = sr_load(fp))) {
		printf("Couldn't read raster image file.\n");
		exit(1);
	}

	linebytes = (sr_header.fi.width + 1) & 0xFFFFFFFE;

	fclose(fp);

	if (sr_header.fi.depth != 8)
		printf("Non-depth 8 image.\n");

	fwidth[k] = sr_header.fi.width;
	fheight[k] = sr_header.fi.height;

        if (fwidth[k] > maxwidth) maxwidth = fwidth[k];
        if (fheight[k] > maxheight) maxheight = fheight[k];

	fpixels[k << 1] = (caddrt) malloc(fwidth[k] * fheight[k]);
	apixels[k << 1] = (caddrt) malloc(fwidth[k] * fheight[k]);

	p = fpixels[k << 1];

	for (y = 0; y < fheight[k]; y++) {
	    pixel = (unsigned char *) pixels + (y * linebytes);
	    for (x = 0; x < fwidth[k]; x++) {
		i = pixel[x];
		if (pixcolorcnt[i] == 0)
	            alloc_newcolor(i,
			sr_cmap.red[i], sr_cmap.green[i], sr_cmap.blue[i]);
	    	if (i == 255)
	        *p++ = 0;
		else *p++ = cmap[i];   
	    }
	}


	fpixels[(k << 1) + 1] = (caddrt) malloc(fwidth[k] * fheight[k]);
	apixels[(k << 1) + 1] = (caddrt) malloc(fwidth[k] * fheight[k]);

	for (i = 0; i < fheight[k]; i++) {
	    p = fpixels[k << 1] + i * fwidth[k];
	    q = fpixels[(k << 1) + 1] + (i + 1) * fwidth[k] - 1;
	    for (j = 0; j < fwidth[k]; j++)
		*q-- = *p++;
	    }


	p = apixels[k << 1];
	q = fpixels[k << 1];

	for (y = 0; y < fheight[k]; y++) {
	    for (x = 0; x < fwidth[k]; x++) {
	    	if (*q++ == 0)
	        *p++ = 255;
		else *p++ = 0;   
	    }
	}

	p = apixels[(k << 1) + 1];
	q = fpixels[(k << 1) + 1];

	for (y = 0; y < fheight[k]; y++) {
	    for (x = 0; x < fwidth[k]; x++) {
	    	if (*q++ == 0)
	        *p++ = 255;
		else *p++ = 0;   
	    }
	}

	free(pixels);

        xfish[k << 1] = XCreateImage(Dpy, DefaultVisual(Dpy, screen), 8,
	    ZPixmap,0,fpixels[k << 1],fwidth[k],fheight[k],8,fwidth[k]);
        xfish[(k << 1) + 1] = XCreateImage(Dpy, DefaultVisual(Dpy, screen), 8,
	    ZPixmap,0,fpixels[(k << 1) + 1],fwidth[k],fheight[k],8,fwidth[k]);
        xfishand[k << 1] = XCreateImage(Dpy, DefaultVisual(Dpy, screen), 8,
	    ZPixmap,0,apixels[k << 1],fwidth[k],fheight[k],8,fwidth[k]);
        xfishand[(k << 1) + 1] = XCreateImage(Dpy,DefaultVisual(Dpy, screen),8,
	    ZPixmap,0,apixels[(k << 1) + 1],fwidth[k],fheight[k],8,fwidth[k]);

	j = DisplayPlanes(Dpy, screen);

	pfish[k << 1] = XCreatePixmap(Dpy, wid, fwidth[k], fheight[k], j);
	pfish[(k << 1) + 1] = XCreatePixmap(Dpy, wid, fwidth[k], fheight[k],j);
	pfishand[k << 1] = XCreatePixmap(Dpy, wid, fwidth[k], fheight[k], j);
	pfishand[(k << 1) + 1] = XCreatePixmap(Dpy,wid,fwidth[k],fheight[k],j);
	if (!dbl_bufrd)
	    pfishmerge[k] = XCreatePixmap(Dpy, wid, fwidth[k], fheight[k], j);

	if (pfish[k << 1])
	   XPutImage(Dpy, pfish[k << 1], gc, xfish[k << 1], 0, 0, 0, 0, fwidth[k], fheight[k]);
	if (pfish[(k << 1) + 1])
	   XPutImage(Dpy, pfish[(k << 1) + 1], gc, xfish[(k << 1) + 1], 0, 0, 0, 0, fwidth[k], fheight[k]);
	if (pfishand[k << 1])
	   XPutImage(Dpy, pfishand[k << 1], gc, xfishand[k << 1], 0, 0, 0, 0, fwidth[k], fheight[k]);
	if (pfishand[(k << 1) + 1])
	   XPutImage(Dpy, pfishand[(k << 1) + 1], gc, xfishand[(k << 1) + 1], 0, 0, 0, 0, fwidth[k], fheight[k]);

	if (pfish[k << 1])
		XDestroyImage(xfish[k << 1]);
	if (pfish[(k << 1) + 1])
		XDestroyImage(xfish[(k << 1) + 1]);
	if (pfishand[k << 1])
		XDestroyImage(xfishand[k << 1]);
	if (pfishand[(k << 1) + 1])
		XDestroyImage(xfishand[(k << 1) + 1]);

    }

    for (i = 1; i <= 8; i++)
	xbubbles[i] = XCreateBitmapFromData(Dpy, buffer[draw_buffnum], xbBits[i], i, i);

    printf("\nTotal number of colors allocated are %i\n", maxcolors);
}


/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
Toggle secure mode on receipt of signal
* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
toggle_secure()
{
    pmode = !pmode;
    if (pmode)
	XLowerWindow(Dpy, wid);
    else
	XRaiseWindow(Dpy, wid);
    XFlush(Dpy);
}


/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
Initialize signal so that SIGUSR1 causes secure mode to toggle.
* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
void 
init_signals()
{
#ifndef	SYSV
    struct sigvec vec;

    vec.sv_handler = toggle_secure;
    vec.sv_mask = 0;
    vec.sv_onstack = 0;

#ifndef HPUX
    sigvec(SIGUSR1, &vec, &vec);
#else
    sigvector(SIGUSR1, &vec, &vec);
#endif
#endif	/* SYSV */
}


/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
Variety of initialization calls, including getting the window up and running.
* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
void 
initialize()
{
    XWindowAttributes winfo;
    XSetWindowAttributes attr;
    XGCValues   vals;
    XSizeHints  xsh;
    register int i, j, numbuffers;

    XGetWindowAttributes(Dpy, DefaultRootWindow(Dpy), &winfo);
    width = winfo.width;
    height = winfo.height;

    attr.override_redirect = True;
    attr.event_mask = ExposureMask | EnterWindowMask;

    wid = XCreateWindow(Dpy, DefaultRootWindow(Dpy),
	1, 1, width - 2, height - 2, 0,
	CopyFromParent, CopyFromParent, CopyFromParent, 
	CWOverrideRedirect | CWEventMask, &attr);

    if (!wid)
	msgdie("XCreateWindow failed");

    vals.foreground = vals.background = 29;
    vals.graphics_exposures = False;
    gc = XCreateGC(Dpy, wid, GCForeground | GCBackground | GCGraphicsExposures,
	&vals);
    vals.foreground = vals.background = 255;
    vals.function = GXxor;
    bgc = XCreateGC(Dpy, wid, GCForeground | GCBackground |
		    GCGraphicsExposures | GCFunction, &vals);

    buffer[0] = wid;

    if (dbl_bufrd) {
        if (!XmbufQueryExtension(Dpy, &mbevbase, &mberrbase)) {

    	    printf("\nMultibuffering extension not present in server.");
	    quadro = 0;
	    dbl_bufrd = 0;
	}
 	numbuffers = XmbufCreateBuffers(Dpy, wid, 2,
				   MultibufferUpdateActionUndefined,
				   MultibufferUpdateHintFrequent,
				   buffer);
    	if (numbuffers != 2) {
    	    printf("\nCouldn't create enough buffers.");
	    quadro = 0;
	    dbl_bufrd = 0;
	}
    }

    init_colormap();

    j = DisplayPlanes(Dpy, screen);

    pback = XCreatePixmap(Dpy, wid, width, height, j);

    init_pixmap();
    init_signals();

    XStoreName(Dpy, wid, pname);

    xsh.flags = USSize | USPosition | PPosition | PSize;
    xsh.x = xsh.y = 0;
    xsh.width = width;
    xsh.height = height;
    XSetNormalHints(Dpy, wid, &xsh);

    XMapWindow(Dpy, wid);

    binfo = (bubble *) malloc(blimit * sizeof(bubble));
    finfo = (fish *) malloc(flimit * sizeof(fish));

    for (i = 0; i < flimit; i++) {
        finfo[i].w = fwidth[i & 31];
        finfo[i].h = fheight[i & 31];
	finfo[i].n = i & 31;
    }

    if (pback) {
	XPutImage(Dpy, pback, gc, xback, 0, 0, 0, 0, width, height);
	XDestroyImage(xback);
    }

    if (dbl_bufrd) draw_buffnum = 1;

    XCopyArea(Dpy, pback, buffer[draw_buffnum], gc, 0, 0, width, height, 0, 0);

}


/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
Create a new bubble.  Placement along the x axis is random, as is the size of
the bubble.  Increment value is determined by speed.
* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
void 
new_bubble(b0)
    bubble     *b0;
{
    register int s;
    register bubble *b = b0;

    b->x = width * (rand() / RAND_F_MAX);
    if (Init_B)
	b->y = (height / 16) * (rand() / RAND_I_1_16 + 1) - 1;
    else
	b->y = height - 1;
    b->s = s = 1.0 + rand() / RAND_F_1_8;
    if ((b->i = smooth * height / (float) binc[s]) == 0)
	b->i = 1;
    putbubble(b, s, bcolor);
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
Erase old bubbles, move and draw new bubbles.  Random left-right factor
can move bubble one size-unit in either direction.
* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
void 
step_bubbles()
{
    register int i,
                j,
                s;
    register bubble *b;

    for (i = 0; i < blimit; i++) {
	b = &binfo[i];
	s = b->s;
	/* clear */
	if (b->y > 0)
	    putbubble(b, s, cmap[0]);
	if ((b->y -= b->i) > 0) {
	    j = rand();
	    if (j < RAND_I_1_4)
		b->x -= s;
	    else if (j > RAND_I_3_4)
		b->x += s;
	    putbubble(b, s, bcolor);
	} else {
	    if (rand() < RAND_I_1_4)
		new_bubble(b);
	}
    }
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
Fish collision detection.  The specified fish is checked against all other
fish for overlap.  The xt parameter specifies the x axis tolerance for overlap.
* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
int 
collide_fish(f0, xt)
    fish       *f0;
    int         xt;
{
    int         i,
                j;
    register fish *f = f0;

    for (i = 0; i < flimit; i++) {
	if (&finfo[i] != f) {
	    j = finfo[i].y - f->y;
	    if ((j > -maxheight) && (j < maxheight)) {
		j = finfo[i].x - f->x;
		if ((j > -xt) && (j < xt))
		    return (1);
	    }
	}
    }
    return (0);
}


/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
Create a new fish.   Placement along the y axis is random, as is the side
from which the fish appears.  Direction is determined from side.  Increment
is also random.
* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
void 
new_fish(f0)
    fish       *f0;
{
    int         i,
                collide;
    fish       *f = f0;

    f->prevx = -32768;
    for (i = 0, collide = 1; (i < 32) && (collide); i++) {
	f->y = (height - maxheight) * (rand() / RAND_F_MAX);
	if (perf) {
	    f->i = smooth * width / (8.0 * (1.0 + 0x7FFFFFFF / RAND_F_1_8));
	} else {
	    if ((f->i =
		smooth * width / (8.0 * (1.0 + rand() / RAND_F_1_8))) == 0)
	    f->i = 1;
	}
	if (rand() < RAND_I_1_2) {
	    f->d = 1;
	    f->x = width;
	} else {
	    f->d = 2;
	    f->x = -maxwidth;
	}

	collide = collide_fish(f, 2 * maxwidth);
    }

    if (!collide)
	putfish(f);
    else {
	f->d = 0;
	f->x = rand();
    }
}


/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
Move all the fish.  Clearing old fish is accomplished by masking only the
exposed areas of the old fish.  Random up-down factor can move fish 1/4 a
fish height in either direction, if no collisions are caused.
* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
void 
move_fish()
{
    register int i,
                j,
                x,
                y,
                ofx,
                ofy,
                done;
    register fish *f;

    for (i = 0; i < flimit; i++) {
	f = &finfo[i];
	if (f->d) {
	    ofx = f->x;
	    ofy = f->y;

	    if (f->d == 1) {
		done = ((f->x -= f->i) < -maxwidth);
		x = f->x + f->w;
	    } else if (f->d == 2) {
		done = ((f->x += f->i) > width);
		x = f->x - f->i;
	    }
	    if (!collide_fish(f, maxwidth)) {
		if (!done) {
		    j = rand();
		    if (j < RAND_I_1_4)
			y = f->i / 4;
		    else if (j > RAND_I_3_4)
			y = f->i / -4;
		    else
			y = 0;
		    if (y) {
			f->y += y;
			if (collide_fish(f, maxwidth)) {
			    f->y -= y;
			    y = 0;
			} else {
			    if (y > 0) {
				j = f->y - y;
			    } else {
				j = f->y + f->h;
				y *= -1;
			    }
			}
		    }
		    putfish(f);
		    if ((!dbl_bufrd)) {
		        XCopyArea(Dpy, pback, buffer[draw_buffnum], gc, x, ofy, f->i, f->h, x, ofy);
			if (timed)
	    		    pixelscopied += (f->i * f->h);
		        if (y) {
		            XCopyArea(Dpy, pback, buffer[draw_buffnum], gc, ofx, j, f->w, y, ofx, j);
			    if (timed)
	    		        pixelscopied += (f->w * y);
			}			  
		    }		    
		} else {
		    if ((!dbl_bufrd) || quadro) {
		       XCopyArea(Dpy, pback, buffer[draw_buffnum], gc, x, f->y, f->i, f->h, x, f->y);
		       if (timed)
	    		   pixelscopied += (f->i * f->h);
		    }
		    new_fish(f);
		}
	    } else {
		if ((f->d = 3 - f->d) == 1) {
		    f->x = f->x - 2 * f->i;
		    x = f->x + f->w;
		} else {
		    f->x = f->x + 2 * f->i;
		    x = f->x - f->i;
		}
		putfish(f);
		if ((!dbl_bufrd)) {
		    XCopyArea(Dpy, pback, buffer[draw_buffnum], gc, x, f->y, f->i, f->h, x, f->y);
		    if (timed)
	    		pixelscopied += (f->i * f->h);
		}
	    }
	} else {
	    new_fish(f);
	}
    }
}


/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
Higher-resolution sleep
* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
void 
high_res_sleep(seconds)
    double      seconds;
{
    int         fds = 0;
    struct timeval timeout;

    timeout.tv_sec = seconds;
    timeout.tv_usec = (seconds - timeout.tv_sec) * 1000000.0;
    select(0, &fds, &fds, &fds, &timeout);
}


/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
void 
main(argc, argv)
    int         argc;
    char      **argv;
{
    FILE	*fp, *fpopen();
    int         i, j, cyclecount;
    union {
      XEvent       ev;
      XExposeEvent ex;
    } ev;

    struct 	itimerval t1, t2;
    double 	etime, ftime;
    char	*fhp;
    int 	initframecount = 40;
    double	framecount = 10.0;
    double	framedelay = 10.0;
    int rand_range = 0;

    cyclecount = 100;
    pixelscopied = 0;

    if ((Dpy = XOpenDisplay("")) == 0)
	msgdie("XOpenDisplay failed");

    screen = DefaultScreen(Dpy);

    white = WhitePixel(Dpy, screen);
    black = BlackPixel(Dpy, screen);
    parse(argc, argv);

    clienthome[0] = 0;
    fishhome[0] = 0;

    strcpy(clienthome, getenv("HOME"));

    strcat(clienthome,"/.fishrc");

    fhp = (char *)getenv("FISHHOME");

    if (fhp) strcpy(fishhome, fhp);
    else {
       fhp = (char *)getenv("OPENWINHOME");
       fishhome[0] = 0;
       strcpy(fishhome, fhp);
       strcat(fishhome, "/share/images/fish");
       fishfile[0] = 0;
       strcat(fishfile, fishhome);
       strcat(fishfile, "/b2rot.im8");
       if (!(fp = fopen(fishfile, "r"))) {
           fishhome[0] = 0;
	   strcpy(fishhome, "./fish");
       } else fclose(fp);
    }


    if (fp = fopen(clienthome, "r")) {
        fscanf(fp,"%i",&j);
	dbl_bufrd = 0;
	quadro = 0;
	seasickness = 0;
	if ((j & 0x3) > 0) dbl_bufrd = 1;
	if ((j & 0x3) > 1) quadro = 1;
	if ((j & 0xC) > 0) seasickness = 1;
        fscanf(fp,"%i",&j);
	rate = j / 100.0;
        fscanf(fp,"%i",&j);
	smooth = j / 100.0;
	i = -1;
	while ((++i < 32) && (fscanf(fp,"%i",&j) != EOF)) fnums[i] = j;
	flimit = i;
	fclose(fp);
	use_props = 1;
    }

    initialize();

    srand((unsigned) getpid());

    /* try to figure out which rand() function is being used */

    i = rand();
    if (i > 32767)
	rand_range = 1;	
    i = rand();
    if (i > 32767)
	rand_range = 1;	
    i = rand();
    if (i > 32767)
	rand_range = 1;	
    i = rand();
    if (i > 32767)
	rand_range = 1;	
    i = rand();
    if (i > 32767)
	rand_range = 1;	

    if (rand_range) {
	RAND_I_1_16 = 134217728;
	RAND_F_1_8 = 268435455.875;
	RAND_I_1_4 = 536870911;
	RAND_I_1_2 = 1073741823;
	RAND_I_3_4 = 1610612735;
	RAND_F_MAX = 2147483647.0;
    } else {
	RAND_I_1_16 = 2048;
	RAND_F_1_8 = 4096.0;
	RAND_I_1_4 = 8096;
	RAND_I_1_2 = 16384;
	RAND_I_3_4 = 24575;
	RAND_F_MAX = 32767.0;
    }

    Init_B = 1;
    for (i = 0; i < blimit; i++)
	new_bubble(&binfo[i]);
    for (i = 0; i < flimit; i++)
	new_fish(&finfo[i]);
    if (pmode)
	XLowerWindow(Dpy, wid);
    else
	XRaiseWindow(Dpy, wid);
    XFlush(Dpy);

    Init_B = 0;

    t1.it_value.tv_sec = 2000000;
    t1.it_value.tv_usec = 0;
    t1.it_interval.tv_sec = 2000000;
    t1.it_interval.tv_usec = 0;
    if (setitimer(ITIMER_REAL, &t1, &t2))
	    printf("\nSetitimer problem");

    for (;;) {
	if (XPending(Dpy)) {
	    XNextEvent(Dpy, &ev);
	    if (ev.ex.type == Expose) {
   	        XCopyArea(Dpy,pback,buffer[draw_buffnum], gc, ev.ex.x, ev.ex.y,
                    ev.ex.width, ev.ex.height, ev.ex.x, ev.ex.y);
		if (timed)
		    pixelscopied += (ev.ex.width * ev.ex.height);
	    }
	    if (ev.ex.type == EnterNotify) {
	        XInstallColormap(Dpy, colormap);
	    }
	}

	if (rate > 0.0) high_res_sleep(rate);

	if (seasickness) {
	    if ((framecount -= 1.0) <= 0.0) {
		framecount += framedelay;
   	        XCopyArea(Dpy,buffer[draw_buffnum],buffer[draw_buffnum], gc,
		    wavex, 0, 128, height, wavex + waveoffset, 0);
		if (timed)
		    pixelscopied += (128 * height);
		wavex += waveinc;
		if (wavex > width) {
		    waveinc = -64;
		    waveoffset = -1;
		}
		if (wavex < 0) {
		    waveinc = 64;
		    waveoffset = 1;
		}
	    }	
	}

	move_fish();

	if (blimit > 0) step_bubbles();

	if (dbl_bufrd) {
	    XmbufDisplayBuffers(Dpy,1,&buffer[draw_buffnum],0,0);
	    if (timed) 
		pixelscopied += (width * height);
	    draw_buffnum = 1 - draw_buffnum;
	    if (!quadro) {
   	        XCopyArea(Dpy,pback,buffer[draw_buffnum], gc, 0, 0,
                    width, height, 0, 0);
	        if (timed) 
		    pixelscopied += (width * height);
	    }
	}
	  
	if (pmode)
	    XLowerWindow(Dpy, wid);
	else
	    XRaiseWindow(Dpy, wid);
	
	if (seasickness && (initframecount > 0)) {
	    --initframecount;
	    if (initframecount == 10) {
	        getitimer(ITIMER_REAL, &t2);
	        etime = (double) (t1.it_value.tv_sec - t2.it_value.tv_sec) +
	        ((double)(t1.it_value.tv_usec-t2.it_value.tv_usec)/1000000.0);
		ftime = (double) etime / 30.0;
	        t1.it_value.tv_sec = 2000000;
	        t1.it_value.tv_usec = 0;
	        t1.it_interval.tv_sec = 2000000;
	        t1.it_interval.tv_usec = 0;
	        if (setitimer(ITIMER_REAL, &t1, &t2))
		    printf("\nSetitimer problem");
	    }
	    if (initframecount == 0) {
	        cyclecount = 100;
	        getitimer(ITIMER_REAL, &t2);
	        etime = (double) (t1.it_value.tv_sec - t2.it_value.tv_sec) +
	        ((double)(t1.it_value.tv_usec-t2.it_value.tv_usec)/1000000.0);
		ftime = (double) etime / 10.0;
		framedelay = (double) 0.20 / ftime;
		if (framedelay < 1.0) framedelay = 1.0;
		framecount = framedelay;
	        pixelscopied = 0;
	        t1.it_value.tv_sec = 2000000;
	        t1.it_value.tv_usec = 0;
	        t1.it_interval.tv_sec = 2000000;
	        t1.it_interval.tv_usec = 0;
	        if (setitimer(ITIMER_REAL, &t1, &t2))
		    printf("\nSetitimer problem");
	    }
	}
	
	if (timed && (--cyclecount == 0)) {
	    cyclecount = 100;
	    getitimer(ITIMER_REAL, &t2);
	    etime = (double) (t1.it_value.tv_sec - t2.it_value.tv_sec) +
	     ((double) (t1.it_value.tv_usec - t2.it_value.tv_usec) /1000000.0);

	    printf("\nPixels copied per second = %8.2f",(pixelscopied/etime));
	    pixelscopied = 0;
	    t1.it_value.tv_sec = 2000000;
	    t1.it_value.tv_usec = 0;
	    t1.it_interval.tv_sec = 2000000;
	    t1.it_interval.tv_usec = 0;
	    if (setitimer(ITIMER_REAL, &t1, &t2))
		printf("\nSetitimer problem");
	}
	

    }
}

