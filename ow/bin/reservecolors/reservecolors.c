#ifndef lint
static char sccsid[] = "@(#)reservecolors.c 1.8 91/07/15";
#endif

/*
** Copyright (c) 1990 by Sun Microsystems, Inc.
*/

#include <config/generic.h>
#include <portable/portable.h>

#include <stdio.h>
#include <assert.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>

/*
** Symbols
*/

#define RESERVECOLORS_PROP_NAME	"_SUN_RESERVECOLORS_XID"

#define CMAP_SIZE		256


/*
** Types
*/

typedef unsigned long Pixel;


/*
** Options 
*/

char	*display_name 	= NULL;		/* -display */
int	svmono		= 0;		/* -svmono */
int	invramp		= 0;		/* -invramp */
int	nokeep		= 0;		/* -nokeep */
int	discard		= 0;		/* -discard */

char *program = "reservecolors";


/*
** Global Data for Built-In Pixel specifications
*/

typedef struct {
	Pixel	pix;
	char	*color_name;
} Pixdesc;

Pixdesc svmonopd[] = {
	254,	"White",
	255,	"Black",
};

#define SVMONONUM 	(sizeof(svmonopd)/sizeof(Pixdesc))

Pixdesc invramppd[] = {
	2,	"#efefef",
	3,	"#e7e7e7",
	4,	"#dfdfdf",
	5,	"#d6d6d6",
	6,	"#cecece",
	7,	"#c6c6c6",
	8,	"#bebebe",
	9,	"#b5b5b5",
	10,	"#adadad",
	11,	"#a5a5a5",
	12,	"#9d9d9d",
	13,	"#949494",
	14,	"#8c8c8c",
	15,	"#848484",
	16,	"#7c7c7c",
	17,	"#737373",
	18,	"#6b6b6b",
	19,	"#636363",
	20,	"#5b5b5b",
	21,	"#525252",
	22,	"#4a4a4a",
	23,	"#424242",
	24,	"#3a3a3a",
	25,	"#313131",
	26,	"#292929",
	27,	"#212121",
	28,	"#191919",
	29,	"#101010",
	30,	"#080808",
	31,	"#000000",
};

#define INVRAMPNUM 	(sizeof(invramppd)/sizeof(Pixdesc))


/*VARARGS1*/
void
fatal_error (format, arg1, arg2, arg3, arg4)
char	*format;

{
	(void) fprintf(stderr, "%s: error: ", program);
	(void) fprintf(stderr, format, arg1, arg2, arg3, arg4);
	(void) fprintf(stderr, "\n");
	exit(1);
}

static void
usage ()

{
	fprintf(stderr, "usage: %s [-display name] [-svmono] [-invramp] [-nokeep] [-discard]\n", 
			program);
	exit(1);
}


/* 
** Parse arguments 
*/

void
process_arguments (argv)
char	**argv;

{
	register char	**a;

	for (a = argv; *a; a++) {
		if        (!strcmp(*a, "-display")) {
		    if (*++a)
			display_name = *a;
		    else {
			(void)fprintf(stderr, "%s: -display needs an argument\n", program);
			usage();
		    }
		} else if (!strcmp(*a, "-svmono")) {
			svmono = 1;
		} else if (!strcmp(*a, "-invramp")) {
			invramp = 1;
		} else if (!strcmp(*a, "-nokeep")) {
			nokeep = 1;
		} else if (!strcmp(*a, "-discard")) {
			discard = 1;
		} else {
			(void)fprintf(stderr, "%s: unrecognized option '%s'\n", program, *a);
			usage();
		}
	}
}


/*
** Helper function for resource_preserve.  
*/

static void
prop_append (dpy, w, name, type, format, data)
Display	*dpy;
Window	w;
char	*name;
Atom	type;
int	format;
int	data;

{
	/* intern the property name */
	Atom	atom = XInternAtom(dpy, name, 0);

	/* create or replace the property */
	XChangeProperty(dpy, w, atom, type, format, PropModeAppend, 
		&data, 1);
}


/*
** Sets the close-down mode of the client to 'RetainPermanent'
** so all client resources will be preserved after the client
** exits.  Creates or adds to a property on the default root window containing
** XIDs of past clients so that the resources of this client can later be killed.
*/

void
resource_preserve (dpy, propname)
Display	*dpy;
char	*propname;

{
	Window	w = DefaultRootWindow(dpy);

	/* create dummy resource */
	Pixmap	pm = XCreatePixmap(dpy, w, 1, 1, 1);
	
	/* create/append the property */
	prop_append(dpy, w, propname, XA_PIXMAP, 32, (int)pm);
	
	/* retain all client resources until explicitly killed */
	XSetCloseDownMode(dpy, RetainPermanent);
}


/*
** Flushes any resources previously retained by all previous 
** invocations of this program, if any exist.
*/

void
resource_discard (dpy, propname)
Display	*dpy;
char	*propname;

{
	Pixmap	*pm;
	Atom	actual_type;		
	int	format;
	int	nitems;
	int	bytes_after;
	register int i;

	/* intern the property name */
	Atom	atom = XInternAtom(dpy, propname, 0);

	/* look for existing resource allocation */
	if (XGetWindowProperty(dpy, DefaultRootWindow(dpy), atom, 0, -1,
		1/*delete*/, AnyPropertyType, &actual_type, &format, &nitems,
		&bytes_after, &pm) == Success && nitems > 0) 

		if (actual_type == XA_PIXMAP && format == 32 && bytes_after == 0) {
			/* blast them away */
			for (i = 0; i < nitems; i++) {
				XKillClient(dpy, pm[i]);
				XSync(dpy, 0);
			}
			XFree(pm);
			fprintf(stderr, "%s: colors from the last %d invocations discarded.\n",
					program, nitems);
		} else if (actual_type != None) {
		    (void)fprintf(stderr, "%s: warning: invalid format encountered for property %s\n",
				 propname, program);
		}
}



/* Helper function for get_pixels */

static int
pixdesc_compare (p1, p2)
Pixdesc	*p1, *p2;

{
	if (p1->pix < p2->pix)
		return -1;
	if (p1->pix > p2->pix)
		return 1;
	return 0;
}


/*
** Return pixel description array, either from built-ins or from stdin.
*/

void
get_pixels (pd, npd)
Pixdesc		**pd;
register int	*npd;

{
	static	Pixdesc	pixels[CMAP_SIZE], *p;
	int	seenpix[CMAP_SIZE];
	int	status;
	int	lineno = 1;
	char	buf[256];
	register int i;

	/* any abbreviation specified? */
	*pd = p = pixels;
	*npd = 0;
	if  (svmono) 
		for (i = 0; i < SVMONONUM; i++, p++) {
			*p = svmonopd[i];
			(*npd)++;
		}
	if (invramp) 
		for (i = 0; i < INVRAMPNUM; i++, p++) {
			*p = invramppd[i];
			(*npd)++;
		}

	/* if either selected, don't read from input */
	if (svmono || invramp)
		goto Sort;

	for (i = 0; i < CMAP_SIZE; i++)
		seenpix[i] = 0;

	/* read from standard input */
	while (1) {
		/* prompt if interactive */
		if (isatty(0)) printf("> ");
		status = scanf("%d %s", &p->pix, buf);
		if (status == EOF)
			break;
		if (status != 2)
			fatal_error("Invalid input line at line %d", lineno);
		if (p->pix >= CMAP_SIZE)
			fatal_error("pixel %d is invalid", p->pix);
		if (seenpix[p->pix])
			fatal_error("duplicate pixel %d", p->pix);
		if (!(p->color_name = (char *) malloc(strlen(buf)+1)))
			fatal_error("Out of memory\n");
		seenpix[p->pix] = 1;
		(void)strcpy(p->color_name, buf);
		(*npd)++;
		p++;
		lineno++;
	}

	if (isatty(0)) printf("\n");

	/* sort the pixels */
Sort:
	qsort(pixels, *npd, sizeof(Pixdesc), pixdesc_compare);
}



/*
** Return true if default visual of screen is dynamic.
*/

static int
dynamic_default_visual (screen)
Screen	*screen;

{
	int class = DefaultVisualOfScreen(screen)->class;

	if (class == DirectColor)
		fatal_error("DirectColor visual not yet supported");

	return (class == GrayScale 	||
	        class == PseudoColor);
}


static Visual *
get_dynamic_visual (dpy)
Display	*dpy;

{
	int		scridx = DefaultScreen(dpy);
	int		depth = DefaultDepth(dpy, scridx);
	XVisualInfo 	visi;

	/* search for PseudoColor */
	if (!XMatchVisualInfo(dpy, scridx, depth, PseudoColor, &visi))
		if (!XMatchVisualInfo(dpy, scridx, depth, GrayScale, &visi))
			return NULL;

	return visi.visual;
}


/*
** Get colormap to use, depending on whether we are keeping pixels
** or not.  If not keeping pixels, use PseudoColor if can find, or else
** GrayScale.  
**
** If keeping pixels, use the default colormap if it is dynamic.  If
** it is not dynamic, abort.
**
** REMIND: DirectColor not yet supported.
*/

Colormap
get_cmap (dpy)
Display	*dpy;

{
	if (nokeep) {
		Visual	*vis;
	  
		/* search for dynamic visual */
		if (!(vis = get_dynamic_visual(dpy)))
			fatal_error("cannot find dynamic visual\n");
		return XCreateColormap(dpy, DefaultRootWindow(dpy), vis, AllocNone);

	} else {

		/* pixels to be allocated in default colormap */
		if (!dynamic_default_visual(DefaultScreenOfDisplay(dpy))) 
			fatal_error("cannot be run when default visual is static");
		return DefaultColormapOfScreen(DefaultScreenOfDisplay(dpy));
	}
}



/*
** Determine which cells in the colormap are free and allocate them
** with placeholder cells.
*/

static void
cmap_get_avail (dpy, cmap, avail)
Display		*dpy;
Colormap	cmap;
int		avail[CMAP_SIZE];


{
	register int 	i, nalloc;
	int		totalpix;
	int		masks;		/* NOTUSED */
	Pixel		pixels[CMAP_SIZE];

	/* start out assuming all are read-only */
	totalpix = 1<<PlanesOfScreen(DefaultScreenOfDisplay(dpy));
	assert(totalpix == CMAP_SIZE);

	/*
	** Find the free cells by allocating them all RW.
	** We allocate by decreasing powers of 2 so the allocation
	** time will be proportional to the number of display planes
	** instead of the number of display cells.
	**
	** Mark the free cells we find.  These allocations will
	** be freed when the connection is closed.
	*/
	for (i=0; i < totalpix; i++)
		avail[i] = 0;
	for (nalloc=totalpix; nalloc > 0; nalloc >>= 1) {
	    if(!XAllocColorCells(dpy, cmap, 0, &masks, 0, 
			pixels, nalloc))
	        continue;
	    for (i=0; i<nalloc; i++) {
		if (avail[pixels[i]])
			fatal_error("fatal error: duplicate allocation");
	        avail[pixels[i]] = 1;
	    }
	}
	XSync(dpy, 0);
}


/*
** Clean up routine: deallocate remaining placeholder cells after 
** they're no longer needed.
*/

static void
cmap_free_avail (dpy, cmap, avail)
Display		*dpy;
Colormap	cmap;
int		avail[CMAP_SIZE];

{
	register int i;
	int	 tofree[CMAP_SIZE];
	int	 ntofree = 0;

	/* collect those which must be freed */
	for (i = 0; i < CMAP_SIZE; i++)
		if (avail[i]) 
			tofree[ntofree++] = i;

	/* free them in one request */
	XFreeColors(dpy, cmap, tofree, ntofree, 0);
}


/*
** Routine which does most of the work.
*/

void
reserve_pixels (dpy, cmap, pd, npd)
Display		*dpy;
Colormap	cmap;	
Pixdesc		*pd;
int		npd;

{
	int	avail[CMAP_SIZE];
	register Pixdesc *p;
	register int	 i;

	/* grab the server */
	XGrabServer(dpy);

	/* 
	** get availability status on colormap cells 
	** and reserve them with placeholder allocations.
	*/
	cmap_get_avail(dpy, cmap, avail);	

	/* see if all requested pixels are available */
	for (p = pd, i = 0; i < npd; p++, i++)
		if (!avail[p->pix]) {
			(void)fprintf(stderr, "%s: pixel %d is already allocated.\n", program, p->pix);
			(void)fprintf(stderr, "no pixels reserved.  program aborted.\n");
			exit(1);
		}

	/* 
	** Retain the RW cell of  every requested pixel and set to requested 
	** color.
	**
	** Note: pixel descriptions are assumed to be sorted by pixel 
	*/
	for (p = pd, i = 0; i < npd; p++, i++) {
		XColor	color;

		if (!XParseColor(dpy, cmap, p->color_name, &color))
			fatal_error("invalid color specification for pixel %d: %s", 
					p->pix, p->color_name);
		color.pixel = p->pix;
		XStoreColor(dpy, cmap, &color);
		avail[p->pix] = 0;
	}

	/* remaining placeholder pixels */
	cmap_free_avail(dpy, cmap, avail);

	/* release server grab */
	XUngrabServer(dpy);
}


static char	*dpyname;

/*ARGSUSED*/
static void 
disp_err_handler (dpy)
Display *dpy;	

{
	fatal_error("cannot open display \"%s\"", dpyname);
}


/*
** Open display and handle any errors.
*/

Display *
open_display (display_name)
char	*display_name;

{
	Display *dpy;

	/*
	**  Catch errors opening display so user doesn't
	**  get confusing 'XIOError: Broken Pipe' message.
	*/
	XSetIOErrorHandler(disp_err_handler); 

	if (display_name == NULL)
		display_name = "";
	dpyname = XDisplayName(display_name);
	if (!(dpy = XOpenDisplay(dpyname))) 
		return NULL;

	XSync(dpy, 0);
	XSetIOErrorHandler(NULL); 

	return dpy;
}



/*ARGSUSED*/
void
main (argc,argv)
int 	argc;
char    **argv;

{
	Display		*dpy = NULL;
	Colormap	cmap;
	Pixdesc		*pd;
	int		npd;

	/* Initialize error handling */
	program = argv[0];

	/* parse rest of arguments */
	process_arguments(++argv);

	/* open connection */
	if (!(dpy = open_display(display_name))) 
		fatal_error("cannot open display '%s'\n", 
				XDisplayName(display_name));
	
	/* discard old allocations from previous invocations, if specified */
	if (discard) {
		resource_discard(dpy, RESERVECOLORS_PROP_NAME);
		exit(0);
	}

	/* get pixel specifications */
	get_pixels(&pd, &npd);

	/* get colormap to use */
	cmap = get_cmap(dpy);

	/* do most of the work */
	reserve_pixels(dpy, cmap, pd, npd);

	/* preserve allocations after program exit */
	if (!nokeep) 
		resource_preserve(dpy, RESERVECOLORS_PROP_NAME);

	/* install into hardware colormap, if specified */
	if (nokeep) {
		XInstallColormap(dpy, cmap);
		XFreeColormap(dpy, cmap);
	}

	XCloseDisplay(dpy);
	exit(0);
}
