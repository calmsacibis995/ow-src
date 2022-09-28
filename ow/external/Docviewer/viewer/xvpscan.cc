#ident "@(#)xvpscan.cc	1.26 11/02/94 Copyright 1990-1992 Sun Microsystems, Inc."

#include <xview/cms.h>
#include "common.h"
#include "xvpscan.h"
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <xview/canvas.h>
#include "dv_cps.h"
#include "xvpscan_cps.h"
#include "dpsdebug.h"
#define ps_Flush(p)                     (psio_setblockok((p)), psio_flush((p)))

extern  "C" {
        void    xv_help_show(Xv_opaque, char *, Event *);
};

#define DONE_TAG 2005

extern  STATUS  XvSleep(int secs, ERRSTK &err);

 Xv_opaque XVPSCAN::dataKey = 0;

 XVPSCAN::XVPSCAN() :
	psIn		(NULL),
	psOut		(NULL),
	pixmapWidth	(0),
	pixmapDepth	(0),
	pixmapHeight	(0),
 	psCan		(XV_NULL)
 {
 	DbgFunc("XVPSCAN::XVPSCAN:" << endl);
 }
 
 void
 XVPSCAN::ClearPage()
 {
	int hght = (int)xv_get(psCan, XV_HEIGHT);
	// Clear the canvas
        //
        ps_ClearCanvas(psIn);
	ps_resetmatrix(psIn, hght);
        ps_Flush(psOut);
 }
 
 void
 XVPSCAN::DrawLink(const PSLINK	 &link,
 		   const BBox	 &docbox,
 		   const float	 scale)
 {
        const Xv_window pwin    = canvas_paint_window(psCan);
        const Window    xwin    = (Window) xv_get(pwin, XV_XID);
        Display *const  dpy     = (Display *) xv_get(pwin, XV_DISPLAY);
	int		tag;
 
        const Rect &rect = ConvertBBoxToXRect(link.GetBBox(),
                                              docbox, scale);

 	DbgFunc("XVPSCAN::DrawLink");
 	DPSDbgFunc("Entered XVPSCAN::DrawLink");
	ChangeGCfunction();
	DrawBox(BOOL_FALSE, link.GetBBox());
	ChangeGCfunction();
	ps_waitcontext(psIn);
	ps_read_tag (&tag);
        if (pixmapDepth == 1)
                XCopyPlane(dpy, dataPage, xwin, gc, rect.r_left,
                        rect.r_top, rect.r_width, rect.r_height,
                        rect.r_left, rect.r_top, 1L);
        else
                XCopyArea(dpy, dataPage, xwin, gc, rect.r_left,
                        rect.r_top, rect.r_width, rect.r_height,
                        rect.r_left, rect.r_top);
        XSync(dpy, 0);
	DbgFunc("XVPSCAN::DrawLink:" << endl);
	DPSDbgFunc("Leaving XVPSCAN::DrawLink");
 }

 
 void
 XVPSCAN::DrawLinkBoxes(LIST<PSLINK*>		&links,
 		       const int		selected)
 {
 	register int	cntr;
	int hght = (int)xv_get(psCan, XV_HEIGHT);

 	DbgFunc("XVPSCAN::DrawLinkBoxes: entered" << endl);
 	// Draw boxes around the links except the currently selected
 	// link (if any)
 	//

	ps_resetmatrix (psIn, hght);
	PrepCanvas(displayBox, scaleVal);
 	for (cntr = 0; cntr < links.Count(); cntr++) 
 		if (cntr != selected) 
			DrawBox(BOOL_FALSE, links[cntr]->GetBBox());

}
 
 void
 XVPSCAN::DrawSelectedLink(const PSLINK	        &pslink,
 			  const BBox		&docbox,
 			  const float		scale)
 {

 	DPSDbgFunc("Entered XVPSCAN::DrawSelectedLink");   

	const Xv_window	pwin	= canvas_paint_window(psCan);
	const Window	xwin	= (Window) xv_get(pwin, XV_XID);
	Display *const	dpy	= (Display *) xv_get(pwin, XV_DISPLAY);
	ERRSTK		err;

	const Rect &rect = ConvertBBoxToXRect(pslink.GetBBox(), 
					      docbox, scale);
	ChangeGCfunction();
	XFillRectangle(dpy, dataPage, gc, rect.r_left, rect.r_top,
				rect.r_width, rect.r_height); 
	ChangeGCfunction();

	if (pixmapDepth == 1)
		XCopyPlane(dpy, dataPage, xwin, gc, rect.r_left, rect.r_top,
		       rect.r_width, rect.r_height,rect.r_left, rect.r_top, 1L);
	else
		XCopyArea(dpy, dataPage, xwin, gc, rect.r_left, rect.r_top,
		       rect.r_width, rect.r_height, rect.r_left, rect.r_top);

	XSync(dpy, 0);
        DPSDbgFunc("Leaving XVPSCAN::DrawSelectedLink");


 }


Rect&
XVPSCAN::ConvertBBoxToXRect(const BBox &linkbox, 
			   const BBox &docbox, 
			   const float mag)
{
  	static Rect rect;
 
 	// Convert the PostScript Bounding Box to an X11 rectangle
 
 	// Calculate r_left of rect
 	rect.r_left = (short) ((linkbox.ll_x - docbox.ll_x) * mag);

 	// Scale the height & width of rect
 	rect.r_height = (u_short) (BoxHght(linkbox) * mag) + 1;
 	rect.r_width = (u_short) (BoxWdth(linkbox) * mag) + 1;
 	// Calculate r_top of rect
 	rect.r_top =  (short) ((mag * (docbox.ur_y - linkbox.ur_y ))); 
 	DPSDbgFunc("Leaving XVPSCAN::BBoxToRect");
 	return (rect);
 }

 

 void
 XVPSCAN::EventProc(const Xv_Window	pwin,
 		     Event *const	event)
 {

 	DbgFunc("XVPSCAN::EventProc: entered" << endl);
 	DPSDbgFunc("Entered XVPSCAN::EventProc");
 	XVPSCAN *const	canObj	=
 		(XVPSCAN *) xv_get(pwin, XV_KEY_DATA, XVPSCAN::dataKey);
 
 	assert(canObj);
 	assert(canObj->objstate.IsReady());
 
 	if (event_is_up(event))
 		return;
 
 
 	switch (event_action(event)) {
 	case ACTION_MENU:
 		menu_show(canObj->canvasMenu, pwin, event, XV_NULL);
 		break;

	case ACTION_HELP:
		xv_help_show(pwin, VIEWER_CANVAS_HELP, event);
		break;

	case ACTION_SELECT:
		canObj->defaultEventProc(VE__SELECT,
					 (const void *) event,
					 (const void *) canObj->cdata);
		break; 

 	default:
		DbgFunc("XVPSCAN::EventProc..dafault event. Executing XVCANVAS::EventProc" << endl);

 		canObj->XVCANVAS::EventProc(pwin, event);
 	}
  	DPSDbgFunc("Leaving XVPSCAN::EventProc");
 	return;
 }


STATUS
XVPSCAN::CreateBkgrndStipple(ERRSTK &err)
{
 	const Xv_window pwin	= canvas_paint_window(psCan);
 	const Window	xwin	= (Window) xv_get(pwin, XV_XID);
 	Display *const	dpy	= XV_DISPLAY_FROM_WINDOW(pwin);
 	STATUS		status	= STATUS_OK;
 	int                 tileWidth = 16;
    	int                 tileHeight = 16;
    	unsigned char       tileBits [] = {
                                0x01, 0x01, 0x00, 0x00,
                                0x80, 0x80, 0x00, 0x00,
                                0x08, 0x08, 0x00, 0x00,
                                0x00, 0x00, 0x00, 0x00,
                                0x01, 0x01, 0x00, 0x00,
                                0x80, 0x80, 0x00, 0x00,
                                0x08, 0x08, 0x00, 0x00,
                                0x00, 0x00, 0x00, 0x00};

	if (!(bkgrndStipple = XCreateBitmapFromData(dpy, xwin, (char *)tileBits,
					tileWidth, tileHeight))){
		err.Init(gettext("Can't create stippled highlight pattern"));
                status = STATUS_FAILED;
	}
	return status;
}


STATUS
XVPSCAN::GetNeWSServer(Display* dpy)
{

	char newsserver[MAXHOSTNAMELEN + 1];
	char hostname[MAXHOSTNAMELEN + 1];
	char *xdisplay;
	char host[MAXHOSTNAMELEN + 1];
	u_int port;
	struct hostent *hp;


	xdisplay = DisplayString(dpy);
	if (xdisplay[0] == ':') {
		strcpy(host, hostname);
		sscanf(xdisplay, ":%u", &port);
	} else {
		sscanf(xdisplay, "%[^:]:%u", host, &port);
		if (strcmp(host, "unix") == NULL)
			strcpy(host, hostname);
       	}
	port += 2000;
	hp = gethostbyname(host);

	if (hp == (struct hostent *) NULL) {
		int i;
		char *x;
		char  addr[4];
   
		x = strtok (host, ".");
		addr [0] = (char) atoi (x);
		for (i = 1; i < 4; i++) {
			x = strtok (NULL, ".");
			addr [i] = (char) atoi (x);
		}
		hp = gethostbyaddr (addr, 4, AF_INET);
		if (hp == (struct hostent *) NULL)  {
			return STATUS_FAILED;
	     	}
          	strcpy (host, hp->h_name);
          }
	sprintf(newsserver, "%lu.%u;%s\n", 
		ntohl(*(u_long *) hp->h_addr), port, host);

	if ((psIn = ps_open_server(newsserver)) == NULL) {
                        return(STATUS_FAILED);
	}
	psOut = psio_getassoc(psIn);
	PostScript = psOut;
	PostScriptInput = psIn;
	endhostent();
	return (STATUS_OK);
}

	
   

 STATUS
 XVPSCAN::Init(const Frame	frame,
 		ERRSTK		&err)
 {
 	Display		*dpy		= NULL;
 	Xv_window	pwin		= XV_NULL;
 	STATUS		status		= STATUS_OK;
	int             visualClass;
	int             depth;
	Colormap	cmap;
	XColor		ccolor;
	unsigned long	gc_mask;
	XGCValues	gc_vals;


	
 	DPSDbgFunc("Entered XVPSCAN::Init");
 	DbgFunc("XVPSCAN::Init:" << endl);

 	assert(objstate.IsNotReady());
 	objstate.MarkGettingReady();
 
	if ((status =
	     VisualClass(frame, &visualClass, &depth, err)) != STATUS_OK) {
		err.Push(gettext("Cannot create XView canvas"));
	}
	else {
 		psCan = (Canvas) xv_create(frame,		CANVAS,
 			  CANVAS_AUTO_EXPAND,	FALSE,
 			  CANVAS_AUTO_SHRINK,	FALSE,
 			  CANVAS_REPAINT_PROC,	XVPSCAN::RepaintProc,
 			  CANVAS_RETAINED,	FALSE,
 			  CANVAS_X_PAINT_WINDOW,TRUE,
			  XV_VISUAL_CLASS,	visualClass,
 			  XV_NULL);

 		if (!psCan) {
 			err.Init(gettext("Could not create XView canvas"));
 			return (STATUS_FAILED);
 		}
      	}

 	pwin = (Xv_window) canvas_paint_window(psCan);
 	dpy  = (Display *) xv_get(pwin, XV_DISPLAY);

	if ((status = GetNeWSServer(dpy)) != STATUS_OK){ 
		err.Init(gettext("Could not connect to NeWS server"));
		return (status);
	}

	pixmapDepth = (int) xv_get(psCan, XV_DEPTH);

	
	dataKey = xv_unique_key();
	(void) xv_set(psCan,
		      XV_KEY_DATA, XVPSCAN::dataKey, (void *) this,
		      XV_NULL);

 	if ((status = XVCANVAS::Setup(frame, pwin, err)) != STATUS_OK) {
 		return (status);
 	}

	// Establish our package stack
	ps_init_connection(psIn);

	// Down-load some PS code see xvpscan_cps.cps for more info
	pscanvas_init_canvas(psIn);

	// Start up the print loop
	pscanvas_start_server_loop(psIn);

	ps_InitNeWS(psIn);
	ps_init_xvpscan(psIn);
	ps_dvSaveObject(psIn);
	ps_ShowPage(psIn);
	ps_Flush(psOut);
 
 	dataKey = xv_unique_key();
 	(void) xv_set(pwin,
 		      WIN_EVENT_PROC, XVPSCAN::EventProc,
 		      XV_KEY_DATA, XVPSCAN::dataKey, (void *) this,
 		      WIN_BIT_GRAVITY,	ForgetGravity,
 		      XV_NULL);
 
 	// Create DPS context
 	const Window xwin = (Window) xv_get(pwin, XV_XID);


	cmap = (Colormap) xv_get( xv_get (psCan, WIN_CMS), CMS_CMAP_ID);

	ccolor.red = ccolor.blue = ccolor.green = 65535;
	XAllocColor (dpy, cmap, &ccolor);
	gc_vals.background = ccolor.pixel;

	ccolor.red = ccolor.blue = ccolor.green = 0;
	XAllocColor (dpy, cmap, &ccolor);
	gc_vals.foreground = ccolor.pixel;
	
	gc_mask = GCForeground | GCBackground;

 	gc = XCreateGC(dpy, xwin, gc_mask, &gc_vals);

	
 	XSetFunction(dpy, gc, GXcopy);
 	XSetLineAttributes(dpy, gc, 4, LineSolid, CapRound, JoinRound);

	// The background gc setup

	ccolor.red = ccolor.blue = ccolor.green = (230 << 8 ) + 230;
	XAllocColor (dpy, cmap, &ccolor);
	gc_vals.background = ccolor.pixel;
	
 	if (CreateBkgrndStipple(err) == STATUS_FAILED) {
 	    notify->ShowErrs(err);
 	    return (STATUS_FAILED);
 	}

	gc_vals.stipple = bkgrndStipple;
	gc_vals.fill_style = FillOpaqueStippled;
	
	gc_mask = GCForeground | GCBackground | GCFillStyle | GCStipple;

	fillgc = XCreateGC( dpy, xwin, gc_mask, &gc_vals);

 	objstate.MarkReady();
 	DPSDbgFunc("Leaving XVPSCAN::Init");
 	return (status);
 }
 
 void
 XVPSCAN::RepaintProc(const Canvas		can,
 		       const Xv_Window		pwin,
 		       Display *const		dpy,
 		       const Window		xwin,
 		       Xv_xrectlist *const	xrects)
 {
 	DbgFunc("XVPSCAN::RepaintProc entered" << endl);
 	DPSDbgFunc("Entered XVPSCAN::RepaintProc");

 	XVPSCAN *const	canObj =
 		(XVPSCAN *) xv_get(pwin, XV_KEY_DATA, XVPSCAN::dataKey);
	int scroll_offset = canObj->sbar->GetViewStart();
	int canWidth = (int)xv_get(canObj->psCan, CANVAS_WIDTH);
	int canHght = (int)xv_get(canObj->psCan, CANVAS_HEIGHT);
 
 	DbgFunc("XVPSCAN::RepaintProc - entered" << endl);
 	assert(canObj);
 	assert(canObj->objstate.IsReady());
 	if (xrects) {
 	   	XSetClipRectangles(dpy, canObj->gc, 0, 0, xrects->rect_array,
 		xrects->count, Unsorted);
 	    	XSetClipRectangles(dpy, canObj->fillgc, 0, 0, xrects->rect_array,
 		xrects->count, Unsorted);
 	}
	if (canWidth > canObj->pixmapWidth)
		XFillRectangle(dpy, xwin, canObj->fillgc, canObj->pixmapWidth, 0, 
				canWidth - canObj->pixmapWidth, canHght);

	if (canHght > canObj->pixmapHeight)
		XFillRectangle(dpy, xwin, canObj->fillgc, 0, canObj->pixmapHeight, 
				canWidth, canHght - canObj->pixmapHeight);

	if (canObj->pixmapDepth == 1)
		XCopyPlane(dpy, canObj->dataPage, xwin, canObj->gc, 
				0, 0 + scroll_offset, 
				canObj->pixmapWidth, canObj->pixmapHeight - scroll_offset,
				0, 0 + scroll_offset, 1L);
	else {
		XCopyArea(dpy, canObj->dataPage, xwin, canObj->gc, 
				0, 0 + scroll_offset,
				canObj->pixmapWidth, canObj->pixmapHeight - scroll_offset,
		 		0, 0 + scroll_offset);
	}
	XSync(dpy, 0);

 	// if the GC clip was set above, then reset it to None
 	if (xrects) {
 		XGCValues gcVal;
 	    	gcVal.clip_mask = None;
 	    	XChangeGC(dpy, canObj->gc, GCClipMask, &gcVal);
		XFlushGC(dpy, canObj->gc);
 	    	XChangeGC(dpy, canObj->fillgc, GCClipMask, &gcVal);
		XFlushGC(dpy, canObj->fillgc);
 	}
 	DPSDbgFunc("Leaving XVPSCAN::RepaintProc");
 	return;
 }

 
void
XVPSCAN::UpdateCanvas()
{
 	const Xv_window pwin	= canvas_paint_window(psCan);
 	const Window	xwin	= (Window) xv_get(pwin, XV_XID);
 	Display *const	dpy	= XV_DISPLAY_FROM_WINDOW(pwin);
	int		tag;

	ps_waitcontext(psIn);
	ps_read_tag (&tag); 
	XClearArea(dpy, xwin, 0, 0, 0, 0, False);
	RepaintProc(NULL, pwin, dpy, xwin, NULL);
}


// enabling reverse video
void
XVPSCAN::ChangeGCfunction()
{
 	const Xv_window pwin	= canvas_paint_window(psCan);
 	const Window	xwin	= (Window) xv_get(pwin, XV_XID);
 	Display *const	dpy	= XV_DISPLAY_FROM_WINDOW(pwin);

	static int function = GXcopy;
	if (function == GXcopy) 
		function = GXxor;
	else
		function = GXcopy;
 	XSetFunction(dpy, gc, function);
	XSync(dpy, 0);
}

 void
 XVPSCAN::SetSize(const int wdth, const int hght)
 {
	static int firsttime = 1;
  	DPSDbgFunc("Entering XVPSCAN::SetSize");
 	DbgFunc("XVPSCAN::SetSize: desired(wdth, hght):\t"
 		<< "(" << wdth << ", " << hght << ")" << endl);
 
 	assert(objstate.IsReady());
 	assert(psCan);

	if ((wdth != (int)xv_get(psCan, CANVAS_WIDTH)) && 
		(hght != (int)xv_get(psCan, CANVAS_HEIGHT))) {

		(void) xv_set(psCan,
 		      		CANVAS_WIDTH,	wdth,
 			      	CANVAS_HEIGHT,	hght,
 		      		XV_NULL);
	}
	else if (wdth != (int)xv_get(psCan,  CANVAS_WIDTH)) {
		(void) xv_set(psCan,
 		      		CANVAS_WIDTH,	wdth,
 		      		XV_NULL);
			
	}
	else if (hght != (int) xv_get(psCan, CANVAS_HEIGHT)) {
		(void) xv_set(psCan,
 			      	CANVAS_HEIGHT,	hght,
				XV_NULL);
	}
	else
		return;
	if (firsttime){
		AdjustPixmap(wdth, hght);
		firsttime = 0;
	}
  	DPSDbgFunc("Leaving XVPSCAN::SetSize");
 	return;
 }
 

void
XVPSCAN::AdjustPixmap(int newpixmapWidth, int newpixmapHeight)
{
 	const Xv_window	pwin	= canvas_paint_window(psCan);
 	Display *const	dpy	= (Display *) xv_get(pwin, XV_DISPLAY);
	
        if (newpixmapWidth == 0)
                newpixmapWidth = (int) xv_get(psCan, CANVAS_WIDTH);
 
        if (newpixmapHeight == 0)
                newpixmapHeight = (int) xv_get(psCan, CANVAS_HEIGHT);


	if ((pixmapWidth != newpixmapWidth)||(pixmapHeight != newpixmapHeight)){
		if (dataPage)
			XFreePixmap(dpy, dataPage);

  	       dataPage = XCreatePixmap(dpy, RootWindow(dpy,DefaultScreen(dpy)),
				newpixmapWidth, newpixmapHeight, pixmapDepth);
		XSync(dpy, 0);

		pixmapWidth = newpixmapWidth;
		pixmapHeight = newpixmapHeight;
		ps_ChangeDrawable(psIn,dataPage);
		ClearPage();
	}
}


 STATUS
 XVPSCAN::DisplayData(const caddr_t start, const size_t nbytes)
 {
	DbgFunc("XVPSCAN::DisplayData: entered" << endl);
        assert(objstate.IsReady());
        STATUS status = ((psio_write((u_char *) start, sizeof(u_char), nbytes,
                              psOut)) ? STATUS_OK : STATUS_FAILED);
 
        if (status == STATUS_OK) {
                ps_Flush(psOut);
        }
	
        DPSDbgFunc("Leaving XVPSCAN::DisplayData");
        return(status);
 }


 
 STATUS
 XVPSCAN::DisplayPage(const float	scale,
 		     const BBox	        &box,
 		     const caddr_t	ptr,
 		     const size_t	nbytes)
 {
  	DPSDbgFunc("Entering XVPSCAN::DisplayPage");
 	ERRSTK		err;
 	STATUS		status	= STATUS_OK;

	// Save values to use for call to PrepCanvas from
	// DrawLinkBoxes
	//
	scaleVal = scale;
	displayBox = box;
        PrepCanvas(box, scale);
        status = DisplayData(ptr, nbytes);
        ps_ShowPage(psIn);
        ps_Flush(psOut);
        DPSDbgFunc("Leaving XVPSCAN::DisplayPage");
        return(status);
}



void
XVPSCAN::PrepCanvas(const BBox	   &displayBox,
		   const float	   scaleVal)
{
	int		origin_dx;
	int		origin_dy;
  	DPSDbgFunc("Entering XVPSCAN::PrepCanvas");
	DbgFunc("XVPSCAN::PrepCanvas: entered" << endl);


	origin_dy = ((int) xv_get(psCan, XV_HEIGHT)) -
		     rint(scaleVal*DocHght(displayBox));
	
	ps_XlateOrigin(psIn, 0, origin_dy);

	// Translate the origin to the lower left corner of the displayBox
	origin_dx = -displayBox.ll_x;
	origin_dy = -displayBox.ll_y;

	// Scale the canvas
	//
	ps_ScaleCanvas(psIn, scaleVal);

	// Translate the origin
	//
	ps_XlateOrigin(psIn, origin_dx, origin_dy);

	return;
}

void
XVPSCAN::DrawBox(const BOOL fill, const BBox &box)
{
	DbgFunc("XVPSCAN::DrawBox" << endl);

	DPSDbgFunc("Performing operation: gsave");

	ps_GSave(psIn);

        ps_DrawBox(psIn,
                   (int) ((fill == BOOL_TRUE) ? 1 : 0),
                   box.ll_x, box.ll_y, box.ur_x, box.ur_y);
        DPSDbgFunc("Performing operation: grestore");
        ps_GRestore(psIn);

        ps_Flush(psOut);

	return;
}


 XVPSCAN::~XVPSCAN()
 {
	XVPSCAN::dataKey = 0;
	psIn = psOut = NULL;
	delete sbar;
 }


void
XVPSCAN::ResetPostScript()
{
        ps_dvRestoreObject(psIn);
	ps_ChangeDrawable(psIn,dataPage);
        ps_dvSaveObject(psIn);
        ps_Flush(psOut);
}
