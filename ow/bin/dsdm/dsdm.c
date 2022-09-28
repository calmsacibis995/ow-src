/*
 * @(#)dsdm.c	1.8 91/09/03
 * Drop Site Database Manager for drag'n'drop.
 *
 * Master algorithm:
 *
 * Start with visible region as whole screen.
 * For each top-level window, do
 * (0) flatten its interest rectangles
 * (1) intersect interest rects with the top-level window
 * (2) intersect them with the visible region
 * (3) append them to the master list
 * (4) subtract this top-level frame from the visible region
 */

#include <config/generic.h>
#include <portable/portable.h>

#include <stdio.h>
#include <string.h>

#include <X11/Xproto.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>

/*
 * Use DPRINTF to write debugging messages.  Place all arguments in an
 * extra pair of parentheses, like so:
 *	DPRINTF(("%s: some error occurred (code=%d)\n", ProgramName, err));
 */

#ifdef    DEBUG
#define DPRINTF(args) (void) printf args
#else  /* DEBUG */
#define DPRINTF(a)
#endif /* DEBUG */

/*
 * Round-trip metering.  Meter the XGetGeometry, XGetWindowAttributes, 
 * XGetWindowProperty, XQueryTree, and XTranslateCoordinates calls.  Calls to 
 * them in the code must be coded as _XWhatever((arg, arg, ...)).
 */

#ifdef RTMETER

int _rt_count;
#define INC_RT_COUNT ++_rt_count ,

#else

#define INC_RT_COUNT

#endif /* RTMETER */


#define _XGetGeometry(args) (\
	 INC_RT_COUNT \
	 XGetGeometry args)

#define _XGetWindowAttributes(args) (\
	 INC_RT_COUNT \
	 XGetWindowAttributes args)

#define _XGetWindowProperty(args) (\
	 INC_RT_COUNT \
         XGetWindowProperty args)

#define _XQueryTree(args) (\
	 INC_RT_COUNT \
	 XQueryTree args)

#define _XTranslateCoordinates(args) (\
	 INC_RT_COUNT \
	 XTranslateCoordinates args)


/*
 * Globals.
 */

#define INTEREST_MAX (1024L*1024L)

#define FIND_CONTINUE ((Window) 0L)
#define FIND_STOP     ((Window) 1L)

char *ProgramName;
Atom ATOM_DRAGDROP_DSDM;
Atom ATOM_DRAGDROP_INTEREST;
Atom ATOM_WM_STATE;
Atom ATOM_SITE_RECTS;


int (*DefaultErrorHandler)();


typedef struct _site {
    int screen;
    unsigned long site_id;
    Window window_id;
    unsigned long flags;
    Region region;
    struct _site *next;
} site_t;

site_t *MasterSiteList = NULL;
site_t **NextSite;
int SitesFound = 0;


Bool SearchChildren();

/*
 * Region stuff.  Stolen from region.h.
 */

typedef struct _box {
    short x1, x2, y1, y2;
} BOX;

typedef struct {
    long size;
    long numRects;
    BOX *rects;
    BOX extents;
} REGION;


#define REGION_NUMRECTS(r) (((REGION *)(r))->numRects)


/*
 * Get the interest property from this window.  If a valid interest property
 * was found, a pointer to the data is returned.  This data must be freed with
 * XFree().  If no valid property is found, NULL is returned.
 */
void *
GetInterestProperty(dpy, win, nitems)
    Display *dpy;
    Window win;
    unsigned long *nitems;
{
    Status s;
    Atom acttype;
    unsigned long remain;
    int actfmt;
    void *data;

    s = _XGetWindowProperty((dpy, win, ATOM_DRAGDROP_INTEREST, 0L,
			     INTEREST_MAX, False, ATOM_DRAGDROP_INTEREST,
			     &acttype, &actfmt, nitems, &remain,
			     (unsigned char **) &data));

    if (s != Success)
	return NULL;

    if (acttype == None)
	/* property does not exist */
	return NULL;

    if (acttype != ATOM_DRAGDROP_INTEREST) {
	fputs("dsdm: interest property has wrong type\n", stderr);
	return NULL;
    }

    if (actfmt != 32) {
	fputs("dsdm: interest property has wrong format\n", stderr);
	XFree(data);
	return NULL;
    }

    if (remain > 0) {
	/* XXX didn't read it all, punt */
	fputs("dsdm: interest property too long\n", stderr);
	XFree(data);
	return NULL;
    }
    return data;
}


/*
 * Check to see if window win is a top-level window, that is, if it is a
 * viewable, InputOutput window that has a drop interest or WM_STATE property
 * on it.  If either property is found, return True.  Additionally, if True is
 * returned, psite will be set to point to the drop interest property data if 
 * that property is found, or NULL if not.  If neither property is found, all 
 * children of this window are searched.
 */
Bool
FindRecursively(dpy, root, win, pwin, psite, plen, px, py)
    Display *dpy;
    Window root, win, *pwin;
    void **psite;
    unsigned long *plen;
    int *px, *py;
{
    XWindowAttributes attr;
    Window junk;
    Atom acttype;
    int actfmt;
    unsigned long nitems;
    unsigned long remain;
    void *data;
    Status s;
    
    if (_XGetWindowAttributes((dpy, win, &attr)) == 0) {
	fprintf(stderr, "%s: XGetWindowAttributes failed for window 0x%08x\n",
		ProgramName, win);
	return False;
    }

    if (attr.depth == 0 || attr.class == InputOnly ||
	    attr.map_state != IsViewable) {
	return False;
    }

    data = GetInterestProperty(dpy, win, &nitems);
    if (data != NULL) {
	if (!_XTranslateCoordinates((dpy, win, root, 0, 0, px, py, &junk))) {
	    fprintf(stderr, "%s: window 0x%08x isn't on the same root!\n",
		    ProgramName, win);
	    XFree(data);
	    return False;
	}
	*psite = (void *) data;
	*plen = nitems;
	*pwin = win;
	DPRINTF(("%s: found top-level window 0x%08x with an interest\n",
		 ProgramName, win));
	return True;
    }
	
    s = _XGetWindowProperty((dpy, win, ATOM_WM_STATE, 0L, 1,
			     False, ATOM_WM_STATE, &acttype, &actfmt,
			     &nitems, &remain, (unsigned char **) &data));

    if (s != Success)	/* couldn't find the window */
	return False;

    if (acttype == ATOM_WM_STATE) {
	/* found it! */
	DPRINTF(("%s: found top-level window 0x%08x with no interest\n",
		 ProgramName, win));
	XFree(data);
	*psite = NULL;
	*plen = 0;
	return True;
    }

    return(SearchChildren(dpy, root, win, pwin, psite, plen, px, py));
}



/*
 * Look through all the children of window win for a top-level window.
 */
Bool
SearchChildren(dpy, root, win, pwin, psite, plen, px, py)
    Display *dpy;
    Window root, win, *pwin;
    void **psite;
    unsigned long *plen;
    int *px, *py;
{
    Window junk;
    Window *children;
    int nchildren;
    int i;

    if (_XQueryTree((dpy, win, &junk, &junk, &children,
		     (unsigned int *) &nchildren)) == 0)
	return False;

    for (i=nchildren-1; i>=0; --i) {
	if (FindRecursively(dpy, root, children[i], pwin, psite, plen, px, py))
	{
	    XFree((char *) children);
	    return True;
	}
    }
    XFree((char *) children);
    return False;
}


/*
 * Create and return a region that contains a given rectangle.
 */
Region
MakeRegionFromRect(x, y, w, h)
    int x, y;
    unsigned int w, h;
{
    XRectangle r;
    Region reg;

    r.x = x;
    r.y = y;
    r.width = w;
    r.height = h;
    reg = XCreateRegion();
    XUnionRectWithRegion(&r, reg, reg);
    return reg;
}


/*
 * Create and return a region that contains the geometry of the window.
 * The region returned must be destroyed with XDestroyRegion().  The offset 
 * parameter indicates whether the window's geometry should be offset by its
 * (x,y) location w.r.t. its parent.  If it is false, the region's upper left 
 * corner is at (0,0).
 */
Region
GetWindowRegion(dpy, win, offset)
    Display *dpy;
    Window win;
    Bool offset;
{
    Window wjunk;
    int x, y;
    unsigned int width, height, junk;

    if (0 == _XGetGeometry((dpy, win, &wjunk, &x, &y, &width, &height,
			    &junk, &junk))) {
	fprintf(stderr, "%s: XGetGeometry failed on window 0x%08x\n",
		ProgramName, win);
	return XCreateRegion();
    }
    return MakeRegionFromRect(offset ? x : 0, offset ? y: 0, width, height);
}    


/*
 * Subtract the area of a window from the current visible region.
 */
void
SubtractWindowFromVisibleRegion(dpy, win, visrgn)
    Display *dpy;
    Window win;
    Region visrgn;
{
    Region winrgn = GetWindowRegion(dpy, win, True);
    XSubtractRegion(visrgn, winrgn, visrgn);
    XDestroyRegion(winrgn);
}


#define DRAGDROP_VERSION	0
#define INTEREST_RECT		0
#define INTEREST_WINDOW		1

#define NEXTWORD(dest) do {						\
	    if (++cur >= datalen) {					\
		fprintf(stderr,						\
			"%s: drop interest data too short on 0x%08x\n",	\
			ProgramName, win);				\
		if (region != NULL)					\
		    XDestroyRegion(region);				\
		if (toprgn != NULL)					\
		    XDestroyRegion(toprgn);				\
		return;							\
	    }								\
	    (dest) = array[cur];					\
	} while (0)


void
ProcessInterestProperty(dpy, win, screen, data, datalen, visrgn, xoff, yoff)
    Display *dpy;
    Window win;
    int screen;
    void *data;
    unsigned long datalen;
    Region visrgn;
    int xoff, yoff;
{
    unsigned long *array = data;
    int cur = 0;
    int i, j, nsites;
    Window wid;
    Window wjunk;
    Window areawin;
    unsigned long sid;
    int areatype;
    int nrects;
    unsigned long flags;
    Region region = NULL;
    Region toprgn = NULL;
    XRectangle rect;
    site_t *site;
    int x, y;
    unsigned int width, height, border, ujunk;
    int junk;

    if (array[cur] != DRAGDROP_VERSION) {
	fprintf(stderr,
		"%s: unknown drop interest property version (%d) on 0x%08x\n",
		ProgramName, array[cur], win);
	return;
    }

    toprgn = GetWindowRegion(dpy, win, False);

    NEXTWORD(nsites);
    for (i=0; i<nsites; ++i) {
	NEXTWORD(wid);
	NEXTWORD(sid);
	NEXTWORD(flags);
	NEXTWORD(areatype);
	switch (areatype) {
	case INTEREST_RECT:
	    region = XCreateRegion();
	    NEXTWORD(nrects);
	    for (j=0; j<nrects; ++j) {
		NEXTWORD(rect.x);
		NEXTWORD(rect.y);
		NEXTWORD(rect.width);
		NEXTWORD(rect.height);
		XUnionRectWithRegion(&rect, region, region);
	    }
	    break;
	case INTEREST_WINDOW:
	    region = XCreateRegion();
	    NEXTWORD(nrects);
	    for (j=0; j<nrects; ++j) {
		NEXTWORD(areawin);
		/* REMIND need to make sure areawin isn't bogus */
		if (0 == _XGetGeometry((dpy, areawin, &wjunk, &junk, &junk,
					&width, &height, &border, &ujunk))) {
		    fprintf(stderr,
			    "%s: XGetGeometry failed on window 0x%08x\n",
			    ProgramName, win);
		    return;
		}
		(void) _XTranslateCoordinates((dpy, areawin, win, 0, 0,
					       &x, &y, &wjunk));
		rect.x = x - border;
		rect.y = y - border;
		rect.width = width + border;
		rect.height = height + border;
		XUnionRectWithRegion(&rect, region, region);
	    }
	    break;
	default:
	    fprintf(stderr,
		    "%s: unknown site area type on window 0x%08x\n",
		    ProgramName, win);
	    XDestroyRegion(toprgn);
	    return;
	}
	XIntersectRegion(region, toprgn, region);
	XOffsetRegion(region, xoff, yoff);
	XIntersectRegion(region, visrgn, region);
	site = (site_t *) malloc(sizeof(site_t));
	/* XXX check for site == NULL */
	site->screen = screen;
	site->site_id = sid;
	site->window_id = wid;
	site->flags = flags;
	site->region = region;
	site->next = NULL;
	(*NextSite) = site;
	NextSite = &site->next;
	++SitesFound;
	region = NULL;
    }
    XDestroyRegion(toprgn);
}


/*
 * For the root window of each screen, get the list of children.  For each 
 * child, get its drop forwarding information and find the top-level window 
 * underneath that child, and get the top-level window's drop site 
 * information.  Add the top-level window's site information and the site 
 * forwarding information to the site database.
 */
void
FindDropSites(dpy)
    Display *dpy;
{
    int s, i, nchildren;
    Window root, junk, *children, topwin;
    void *sitedata;
    Region visrgn, framergn, toprgn;
    XWindowAttributes attr;
    unsigned long datalen;
    int xoff, yoff;
    void *fwdsitedata;
    unsigned long fwdlen;
    Bool foundtoplevel;

#ifdef RTMETER
    _rt_count = 0;
#endif

    for (s=0; s<ScreenCount(dpy); ++s) {

	/* Find the virtual root here, if necessary. */
	root = RootWindow(dpy, s);
	visrgn = GetWindowRegion(dpy, root, False);

	if (_XQueryTree((dpy, root, &junk, &junk, &children,
			 (unsigned int *) &nchildren)) == 0)
	{
	    fprintf(stderr, "%s: XQueryTree failed on root window 0x%08x\n",
		    ProgramName, root);
	    continue;
	}

	/*
	 * For each mapped, InputOutput, child-of-root window, look for a drop
	 * interest property.  This will be a forwarding interest property
	 * placed by the window manager.  Then, find the top-level window
	 * underneath this window and process its interest property.
	 */

	for (i=nchildren-1; i>=0; --i) {
	    if (_XGetWindowAttributes((dpy, children[i], &attr)) == 0) {
		fprintf(stderr,
			"%s: XGetWindowAttributes failed for window 0x%08x\n",
			ProgramName, children[i]);
		continue;
	    }

	    if (attr.depth == 0 || attr.class == InputOnly ||
		attr.map_state != IsViewable) {
		    continue;
	    }

	    fwdsitedata = GetInterestProperty(dpy, children[i], &fwdlen);

	    foundtoplevel = SearchChildren(dpy, root, children[i], &topwin,
					   &sitedata, &datalen, &xoff, &yoff);
	    if (foundtoplevel && sitedata != NULL) {
		/* we found a valid drop interest */
		ProcessInterestProperty(dpy, topwin, s, sitedata,
					datalen, visrgn, xoff, yoff);
		XFree(sitedata);
		if (fwdsitedata != NULL) {
		    framergn = MakeRegionFromRect(attr.x, attr.y,
						  attr.width, attr.height);
		    XIntersectRegion(framergn, visrgn, framergn);
		    toprgn = GetWindowRegion(dpy, topwin, False);
		    XOffsetRegion(toprgn, xoff, yoff);
		    XSubtractRegion(framergn, toprgn, framergn);
		    ProcessInterestProperty(dpy, children[i], s, fwdsitedata,
					    fwdlen, framergn, attr.x, attr.y);
		    XDestroyRegion(framergn);
		    XDestroyRegion(toprgn);
		    XFree(fwdsitedata);
		}
	    } else {
		if (fwdsitedata != NULL) {
		    ProcessInterestProperty(dpy, children[i], s, fwdsitedata,
					    fwdlen, visrgn, attr.x, attr.y);
		    XFree(fwdsitedata);
		}
	    }

	    SubtractWindowFromVisibleRegion(dpy, children[i], visrgn);
	}
	XDestroyRegion(visrgn);
	XFree((char *) children);
    }

#ifdef RTMETER
    printf("roundtrips = %d\n", _rt_count);
#endif
}


void
FreeDropSites()
{
    site_t *next, *temp;

    next = MasterSiteList;
    while (next != NULL) {
	XDestroyRegion(next->region);
	temp = next->next;
	free(next);
	next = temp;
    }
    MasterSiteList = NULL;
    SitesFound = 0;
    NextSite = &MasterSiteList;
}


/*
 * Write a property containing site rectangle information.  The format 
 * consists of zero or more blocks of 8 words, as follows:
 *	8k+0	screen number
 *	8k+1	site id
 *	8k+2	window id
 *	8k+3	x
 *	8k+4	y
 *	8k+5	width
 *	8k+6	height
 *	8k+7	flags
 */
void
WriteSiteRectList(dpy, win, prop)
    Display *dpy;
    Window win;
    Atom prop;
{
    unsigned long *cur;
    unsigned long *array;
    site_t *site;
    int numrects = 0;
    REGION *region;
    BOX *box, *last;

    site = MasterSiteList;
    while (site != NULL) {
	numrects += REGION_NUMRECTS(site->region);
	site = site->next;
    }

    /* XXX beware of malloc(0) */
    array = (unsigned long *) malloc(8*numrects*sizeof(int));
    cur = array;
    site = MasterSiteList;
    while (site != NULL) {
	region = (REGION *) site->region;
	box = region->rects;
	last = box + region->numRects;
	for ( ; box < last ; ++box) {
	    *cur++ = site->screen;
	    *cur++ = site->site_id;
	    *cur++ = site->window_id;
	    *cur++ = box->x1;
	    *cur++ = box->y1;
	    *cur++ = box->x2 - box->x1;
	    *cur++ = box->y2 - box->y1;
	    *cur++ = site->flags;
	}
	site = site->next;
    }

    XChangeProperty(dpy, win, prop, XA_INTEGER, 32, PropModeReplace,
		    (unsigned char *)array, cur - array);
    free(array);
}


/*
 * Ignore BadWindow and BadDrawable errors on a variety of requests.  These
 * errors often occur if the requester window goes away after requesting the
 * site database.  REMIND: more robust error handling is called for.
 */
int
ErrorHandler(dpy, error)
    Display *dpy;
    XErrorEvent *error;
{
    if (    (error->error_code == BadWindow ||
	     error->error_code == BadDrawable) &&
	    (error->request_code == X_GetWindowAttributes ||
	     error->request_code == X_GetGeometry ||
	     error->request_code == X_QueryTree ||
	     error->request_code == X_ChangeProperty ||
	     error->request_code == X_GetProperty ||
	     error->request_code == X_SendEvent)) {
	DPRINTF(("ignored BadWindow error on request %d\n",
		 error->request_code));
	return 0;
    }

    fputs("dsdm: ", stderr);
    (*DefaultErrorHandler)(dpy, error);
    exit(1);
    /*NOTREACHED*/
}


main(argc, argv)
    int argc;
    char **argv;
{
    Display *dpy;
    Window selwin, win;
    XSetWindowAttributes attr;
    XEvent e;
    XEvent reply;
    int xflag = 0;
    int nullflag = 0;

    ProgramName = strrchr(argv[0], '/');
    if (ProgramName == NULL) {
	ProgramName = argv[0];
    } else {
	++ProgramName;
    }

    dpy = XOpenDisplay(NULL);
    if (dpy == NULL) {
	fprintf(stderr, "%s: can't open display %s\n",
		ProgramName, XDisplayString(NULL));
	exit(1);
    }

    DefaultErrorHandler = XSetErrorHandler(ErrorHandler);

    ATOM_DRAGDROP_DSDM = XInternAtom(dpy, "_SUN_DRAGDROP_DSDM", False);
    ATOM_DRAGDROP_INTEREST = XInternAtom(dpy, "_SUN_DRAGDROP_INTEREST", False);
    ATOM_SITE_RECTS = XInternAtom(dpy, "_SUN_DRAGDROP_SITE_RECTS", False);
    ATOM_WM_STATE = XInternAtom(dpy, "WM_STATE", False);

    attr.event_mask = PropertyChangeMask;
    selwin = XCreateWindow(dpy, DefaultRootWindow(dpy), 0, 0, 1, 1, 0, 0,
			   InputOnly, CopyFromParent,
			   CWEventMask, &attr);

    if (argc > 1 && 0 == strcmp(argv[1], "-x"))
	xflag = 1;

    if (argc > 1 && 0 == strcmp(argv[1], "-null"))
	nullflag = 1;

    XGrabServer(dpy);
    win = XGetSelectionOwner(dpy, ATOM_DRAGDROP_DSDM);

    if (xflag) {
	if (win == None) {
	    fprintf(stderr, "%s: no DSDM is running\n", ProgramName);
	    exit(1);
	}
	/* Clear any DSDM selection to force the running DSDM to exit */
	XSetSelectionOwner(dpy, ATOM_DRAGDROP_DSDM, None, CurrentTime);
	XFlush(dpy);
	exit(0);
    }

    if (win != None) {
	fprintf(stderr,
		"%s: another DSDM is already running\n",
		ProgramName);
	exit(1);
    }

    XSetSelectionOwner(dpy, ATOM_DRAGDROP_DSDM, selwin, CurrentTime);
    /* no need to get owner per ICCCM, because we have the server grabbed */
    XUngrabServer(dpy);
    XFlush(dpy);

    while (1) {
	XNextEvent(dpy, &e);
	switch (e.type) {
	case SelectionRequest:
	    if (e.xselectionrequest.selection != ATOM_DRAGDROP_DSDM) {
		DPRINTF(("%s: got SelectionRequest on wrong selection?\n",
			 ProgramName));
		break;
	    }
	    if (e.xselectionrequest.owner != selwin) {
		fprintf(stderr, "%s: got SelectionRequest on wrong window?\n",
			ProgramName);
		break;
	    }
		
	    reply.xselection.type = SelectionNotify;
	    reply.xselection.requestor = e.xselectionrequest.requestor;
	    reply.xselection.selection = ATOM_DRAGDROP_DSDM;
	    reply.xselection.target = e.xselectionrequest.target;
	    reply.xselection.time = e.xselectionrequest.time;

	    if (e.xselectionrequest.target != ATOM_SITE_RECTS) {
		fprintf(stderr,
			"%s: got SelectionRequest for unknown target\n",
			ProgramName);
		reply.xselection.property = None;
	    } else {
		DPRINTF(("%s: got SelectionRequest event OK\n", ProgramName));
		if (!nullflag) {
		    FreeDropSites();
		    FindDropSites(dpy);
		}
		WriteSiteRectList(dpy, e.xselectionrequest.requestor,
				  e.xselectionrequest.property);
		reply.xselection.property =
		    e.xselectionrequest.property;
	    }

	    (void) XSendEvent(dpy, reply.xselection.requestor, False, 0,
			      &reply);
	    break;
	case SelectionClear:
	    exit(0);
	}
    }
}
