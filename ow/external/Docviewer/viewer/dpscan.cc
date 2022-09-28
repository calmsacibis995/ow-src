#ident "@(#)dpscan.cc	1.30 05/10/95 Copyright 1992 Sun Microsystems, Inc."

#include <xview/cms.h>
#include "common.h"
#include <DPS/psops.h>
#include "dpscan.h"
#include "dpsexterns.h"
#include "dvutils_dps.h"
#include "dvutils_dps.c"

#include "dpsdebug.h"

// External Functions
extern "C" {
        void ds_expand_pathname(char *str, char *path);
};

Xv_opaque DPSCAN::dataKey = 0;
XStandardColormap        *gray_cmap = NULL;
XStandardColormap        *rgb_cmap = NULL;

 DPSCAN::DPSCAN() :
	pixmapWidth	(0),
	pixmapDepth	(0),
	pixmapHeight	(0),
 	dpsCan		(XV_NULL),
	dataPage	(0)
 {
        DPSDbgFunc("Entered DPSCAN::DPSCAN");
 	DbgFunc("DPSCAN::DPSCAN:" << endl);
        DPSDbgFunc("Leaving DPSCAN::DPSCAN");
 }
 
 void
 DPSCAN::ClearPage()
 {
 	const Xv_window pwin	= canvas_paint_window(dpsCan);
 	const Window	xwin	= (Window) xv_get(pwin, XV_XID);
 	Display *const	dpy	= XV_DISPLAY_FROM_WINDOW(pwin);
	int hght = (int)xv_get(dpsCan, XV_HEIGHT);

        DPSDbgFunc("Entered DPSCAN::ClearPage");        
	if (ctxt != (DPSContext) NULL) {
		ps_ClearCanvas();
 		DPSPrintf(ctxt, "[ 1 0 0 -1 0 0 ] setmatrix\n");
		if (hght != 0) 
			PSsetXoffset(0, hght);
      	}
       	DPSDbgFunc("Leaving DPSCAN::ClearPage");        	
 }
 
 void
 DPSCAN::DrawLink(const PSLINK	 &link,
	 	  const BBox	&docbox,
		  const float	scale)
 {
	const Xv_window	pwin	= canvas_paint_window(dpsCan);
	const Window	xwin	= (Window) xv_get(pwin, XV_XID);
	Display *const	dpy	= (Display *) xv_get(pwin, XV_DISPLAY);

	const Rect &rect = ConvertBBoxToXRect(link.GetBBox(), 
					      docbox, scale);
 
 	DbgFunc("DPSCAN::DrawLink");
 	DPSDbgFunc("Entered DPSCAN::DrawLink");
	ChangeGCfunction();
	DrawBox(BOOL_FALSE, link.GetBBox());
	ChangeGCfunction();
	DPSWaitContext(ctxt);

	if (pixmapDepth == 1)
		XCopyPlane(dpy, dataPage, xwin, gc, rect.r_left, 
			rect.r_top, rect.r_width, rect.r_height,
			rect.r_left, rect.r_top, 1L);
	else
		XCopyArea(dpy, dataPage, xwin, gc, rect.r_left, 
			rect.r_top, rect.r_width, rect.r_height,
			rect.r_left, rect.r_top);

	XSync(dpy, 0);
	DbgFunc("DPSCAN::DrawLink:" << endl);
	DPSDbgFunc("Leaving DPSCAN::DrawLink");


 }
 
 void
 DPSCAN::DrawLinkBoxes(LIST<PSLINK*>		&links,
 		       const int		selected)
 {
 	register int	cntr;

 	DbgFunc("XVPSCAN::DrawLinkBoxes: entered" << endl);
 	DPSDbgFunc("Entered DPSCAN::DrawLinkBoxes");
 	// Draw boxes around the links except the currently selected
 	// link (if any)
 	//

	if (ctxt != (DPSContext) NULL) {

		DPSPrintf(ctxt, "1 setlinewidth\n");

 		for (cntr = 0; cntr < links.Count(); cntr++) {
 			if (cntr != selected) {
				DrawBox(BOOL_FALSE, links[cntr]->GetBBox());
 			}
 		}
      	}

 	DPSDbgFunc("Leaving DPSCAN::DrawLinkBoxes");
}
 
 void
 DPSCAN::DrawSelectedLink(const PSLINK	        &pslink,
 			  const BBox		&docbox,
 			  const float		scale)
 {

 	DPSDbgFunc("Entered DPSCAN::DrawSelectedLink");   

	const Xv_window	pwin	= canvas_paint_window(dpsCan);
	const Window	xwin	= (Window) xv_get(pwin, XV_XID);
	Display *const	dpy	= (Display *) xv_get(pwin, XV_DISPLAY);

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
		       rect.r_width, rect.r_height,rect.r_left, rect.r_top);
	XSync(dpy, 0);
        DPSDbgFunc("Leaving DPSCAN::DrawSelectedLink");


 }


Rect&
DPSCAN::ConvertBBoxToXRect(const BBox &linkbox, 
			   const BBox &docbox, 
			   const float mag)
{
  	static Rect rect;

        // Convert the PostScript Bounding Box to an X11 rectangle

        // Calculate r_left of rect                                
        rect.r_left = (short) ((linkbox.ll_x - docbox.ll_x) * mag);
 
        // Scale the height & width of rect
        rect.r_height = (u_short) (BoxHght(linkbox) * mag)+1;
        rect.r_width = (u_short) (BoxWdth(linkbox) * mag)+1;
        // Calculate r_top of rect
        rect.r_top =  (short) ((mag * (docbox.ur_y - linkbox.ur_y )));
        DPSDbgFunc("Leaving XVPSCAN::BBoxToRect");
        return (rect);
}
 
 void
 DPSCAN::EventProc(const Xv_Window	pwin,
 		     Event *const	event)
 {

 	DbgFunc("DPSCAN::EventProc: entered" << endl);
 	DPSDbgFunc("Entered DPSCAN::EventProc");
 	DPSCAN *const	canObj	=
 		(DPSCAN *) xv_get(pwin, XV_KEY_DATA, DPSCAN::dataKey);
 
 	assert(canObj);
 	assert(canObj->objstate.IsReady());
 
 	if (event_is_up(event))
 		return;
 
 
 	switch (event_action(event)) {
 	case ACTION_MENU:
 		menu_show(canObj->canvasMenu, pwin, event, XV_NULL);
 		break;

	case ACTION_SELECT:

		canObj->defaultEventProc(VE__SELECT,
					 (const void *) event,
					 (const void *) canObj->cdata);
		break; 

 	default:
		DbgFunc("DPSCAN::EventProc..dafault event. Executing XVCANVAS::EventProc" << endl);

 		canObj->XVCANVAS::EventProc(pwin, event);
 	}
  	DPSDbgFunc("Leaving DPSCAN::EventProc");
 	return;
 }



STATUS
DPSCAN::CreateBkgrndStipple(ERRSTK &err)
{
 	const Xv_window pwin	= canvas_paint_window(dpsCan);
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

void
dps_text_handler (DPSContext ctxt, char *buf, int count)
{
    char *tmp = (char*) malloc (count + 1);

    strncpy (tmp, buf, count);
    tmp[count] = '\0';
#ifdef DEBUG
//    if (verbose)
       fprintf (stderr, "%s", tmp);
#endif
    free (tmp);
}

void
dps_error_handler (DPSContext ctxt, DPSErrorCode error_code, 
		   long unsigned arg1, long unsigned arg2)
{
    char	*prefix = "%% [Error: ";
    char	*suffix = " ] %%\n";
    char	*infix = " Offending Command: ";
    char	*nameinfix = "User name too long; Name: ";
    char	*contextinfix = "Invalid context: ";
    char	*taginfix = "Unexpected wrap result tag: ";
    char	*typeinfix = "Unexpected wrap result type; tag: ";
    char 	*buf1;
    char	 buf2 [256];
    DPSBinObj	 ary;
    DPSBinObj	 elements;
    char 	*error_str;
    char	*error_name;
    int 	 error_count;
    int		 error_name_count;
    int  	 resync_flag;
    unsigned char tag;


    switch (error_code) {
       case dps_err_ps: 
    	    buf1 = (char *) arg1;
 	    ary = (DPSBinObj) (buf1 + DPS_HEADER_SIZE);

  	    elements = (DPSBinObj) (((char *) ary) + ary->val.arrayVal);
	    error_name = (char *) (((char *) ary) + elements [1].val.nameVal);
	    error_name_count = elements [1].length; 

	    error_str = (char *) (((char *) ary) + elements [2].val.nameVal);
	    error_count = elements [2].length;

	    resync_flag = elements [3].val.booleanVal;

	    dps_text_handler (ctxt, prefix, strlen (prefix));
	    dps_text_handler (ctxt, error_name, error_name_count);
	    dps_text_handler (ctxt, infix, strlen (infix));
	    dps_text_handler (ctxt, error_str, error_count);
	    dps_text_handler (ctxt, suffix, strlen (suffix));
	
	    break;

       case dps_err_nameTooLong:
	    buf1 = (char *) arg1;
	    dps_text_handler (ctxt, prefix, strlen (prefix));
	    dps_text_handler (ctxt, nameinfix, strlen (nameinfix));
	    dps_text_handler (ctxt, buf1, arg2);
	    dps_text_handler (ctxt, suffix, strlen (suffix));
	    break;
	     
       case dps_err_invalidContext:
	    sprintf (buf2, "%s %s %d %s", prefix, contextinfix, arg1,
		     suffix);
	    dps_text_handler (ctxt, buf2, strlen (buf2));
	    break;

       case dps_err_resultTagCheck:
	    tag = *((unsigned char *) arg1 + 1);
	    sprintf (buf2, "%s %s %d %s", prefix, taginfix, tag, 
		     suffix);
	    dps_text_handler (ctxt, buf2, strlen (buf2));
	    break;

       case dps_err_resultTypeCheck:
	    tag = *((unsigned char *) arg1 + 1);
	    sprintf (buf2, "%s %s %d %s", prefix, typeinfix, tag, 
		     suffix);
	    dps_text_handler (ctxt, buf2, strlen (buf2));
	    break;

       case dps_err_invalidAccess:
	    sprintf (buf2, "%s Invalid context access. %s", prefix, 
		     suffix);
	    dps_text_handler (ctxt, buf2, strlen (buf2));
	    break;

       case dps_err_encodingCheck:
	    sprintf (buf2, "%s Invalid name/program encoding: %d/%d. %s",
		     prefix, (int) arg1, (int) arg2, suffix);
	    dps_text_handler (ctxt, buf2, strlen (buf2));
	    break;
	   
       case dps_err_closedDisplay:
	    sprintf (buf2, "%s Broken display connection %d. %s",
		     prefix, (int) arg1, suffix);
	    dps_text_handler (ctxt, buf2, strlen (buf2));
	    break;
       
       case dps_err_deadContext:
	    sprintf (buf2, "%s Dead context 0x0%x. %s",
		     prefix, (int) arg1, suffix);
	    dps_text_handler (ctxt, buf2, strlen (buf2));
	    break;
       
       default:
	    buf1 = "Unknown error from DPS\n";
	    dps_text_handler (ctxt, buf1, strlen (buf1));
       
       }

    if (ctxt != (DPSContext) NULL) {
       DPSDestroyContext (ctxt);
       ctxt = (DPSContext) NULL;
       }
}

void
dps_status_handler (DPSContext aCtxt, int code)
{

/*
 * We only look for ZOMBIE events...
 */
#ifdef DEBUG
//    if (verbose) 
       fprintf (stderr, " Docviewer got ps status: %d\n", code);
#endif
    if (code == PSZOMBIE) {
       if (aCtxt != (DPSContext) NULL) {
          DPSDestroyContext (aCtxt);
          aCtxt = (DPSContext) NULL;
          }
       }
    else if (code == PSFROZEN)
       frozen++;
}

void
copy_default_cmap (Display *xdpy, Colormap cmap, int ncolors)
{
    int     i;
    XColor  colors[256];

    for (i = 0; i < ncolors; i++)
      colors[i].pixel = i;

    XQueryColors (xdpy, DefaultColormap (xdpy, DefaultScreen (xdpy)), colors,
                  ncolors);

    XStoreColors (xdpy, cmap, colors, ncolors);
}

void
make_std_colormaps (Xv_opaque owner, Xv_opaque obj, int vis_class)
{
    int		 status;
    Colormap	 canvas_cmap;
    int		 depth = xv_get (obj, XV_DEPTH);
    int		 win = xv_get (canvas_paint_window (obj), XV_XID);
    Display     *dpy = (Display *) xv_get (obj, XV_DISPLAY);
    int		 screen = DefaultScreen (dpy);
    XVisualInfo	 vinfo;

    canvas_cmap = (Colormap) xv_get (xv_get (obj, WIN_CMS), CMS_CMAP_ID);

    status = XMatchVisualInfo (dpy, screen, depth, vis_class, &vinfo);

/*
 * Only values of vis_class that are passed to this function are:
 * StaticGray (could be depth 1 or 8), StaticColor, TrueColor (24 bit only),
 * GrayScale or PseudoColor.
 */

    if (vis_class == StaticGray) {
       gray_cmap = (XStandardColormap *) 
			calloc (1, sizeof (XStandardColormap));
       gray_cmap->colormap = canvas_cmap;
       status = XDPSCreateStandardColormaps (dpy, win, vinfo.visual, 
					     0, 0, 0, 0, 
					     (XStandardColormap *) NULL, 
					     gray_cmap, False);
       }

/*
 * TrueColor, and StaticColor handled the same.
 */

    else if ((vis_class == TrueColor) || (vis_class == StaticColor)) {
       rgb_cmap = (XStandardColormap *) 
				calloc (1, sizeof (XStandardColormap));
       gray_cmap = (XStandardColormap *) 
				calloc (1, sizeof (XStandardColormap));
       rgb_cmap->colormap = canvas_cmap;
       gray_cmap->colormap = canvas_cmap;
       status = XDPSCreateStandardColormaps (dpy, win, vinfo.visual, 
					     0, 0, 0, 0, rgb_cmap, 
					     gray_cmap, False);
       }

/*
 * PseudoColor and GrayScale might need new colormaps created. Also, copy
 * values from the default colormap so that flashing is minimized.
 * 11/16/93 - note may be able to add DirectColor here also if
 *	      Adobe fixed their bug.
 */

    else {		/* vis is PseudoColor or GrayScale */

/*
 * First try and allocate in default colormap 
 */
       gray_cmap = (XStandardColormap *) 
			calloc (1, sizeof (XStandardColormap));
       gray_cmap->colormap = canvas_cmap;

       if ((vis_class == PseudoColor) || (vis_class == DirectColor)) {
          rgb_cmap = (XStandardColormap *) 
				calloc (1, sizeof (XStandardColormap));
          rgb_cmap->colormap = canvas_cmap;
   	  }

       status = XDPSCreateStandardColormaps (dpy, win, vinfo.visual, 
					     0, 0, 0, 0, rgb_cmap, 
					     gray_cmap, False);

/*
 * If we couldn't allocate the colors, then we have to create our own
 * colormap.
 */

       if (status != 1) { 
          unsigned long plane_masks [1];
          unsigned long pixels [40];
          Window **wins;
          Window *new_wins;
          int count = 0;
          int i;

/*
 * Check if the canvas_cmap is the Default... if so, then create a new
 * one. If not, then use it.
 */

          canvas_cmap = XCreateColormap (dpy, win, vinfo.visual, AllocNone);

          if (depth == 4) {
             status = XAllocColorCells (dpy, canvas_cmap, True, plane_masks,
                                        0, pixels, 6);
             copy_default_cmap (dpy, canvas_cmap, 6);
             }
          else if (DefaultDepth (dpy, screen) == 4) {
             status = XAllocColorCells (dpy, canvas_cmap, True, plane_masks,
                                        0, pixels, 16);
             copy_default_cmap (dpy, canvas_cmap, 16);
             }
          else if (depth == 8) {
             status = XAllocColorCells (dpy, canvas_cmap, True, plane_masks,
                                        0, pixels, 40);
             copy_default_cmap (dpy, canvas_cmap, 40);
             }

          gray_cmap = (XStandardColormap *) 
				calloc (1, sizeof (XStandardColormap));
          gray_cmap->colormap = canvas_cmap;

          if ((vis_class == PseudoColor) || (vis_class == DirectColor)) {
             rgb_cmap = (XStandardColormap *) 
				   calloc (1, sizeof (XStandardColormap));
             rgb_cmap->colormap = canvas_cmap;
   	     }

          status = XDPSCreateStandardColormaps (dpy, win, vinfo.visual, 
					        0, 0, 0, 0, rgb_cmap, 
						gray_cmap, False);

          XSetWindowColormap (dpy, win, canvas_cmap);

/*
 * Intall colormap property on the canvas paint window.
 */

          wins = (Window **) malloc (sizeof (Window *));
          XGetWMColormapWindows (dpy, xv_get (owner, XV_XID), wins, &count);
   
          new_wins = (Window *) malloc (sizeof (Window) * (count + 2));
          for (i = 0; i < count; i++)
              new_wins [i] = wins [0][i];

          new_wins [count++] = win;
	  new_wins [count++] = xv_get (owner, XV_XID);
          XSetWMColormapWindows (dpy, xv_get (owner, XV_XID), new_wins, count);
          }
       }
}

 STATUS
 DPSCAN::Init(const Frame	frame,
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
	Visual		*visual;

	
 	DPSDbgFunc("Entered DPSCAN::Init");
 	DbgFunc("DPSCAN::Init:" << endl);
 	assert(objstate.IsNotReady());
        enable_mask = PSZOMBIEMASK ;
        disable_mask = PSFROZENMASK | PSRUNNINGMASK | PSNEEDSINPUTMASK ;
        next_mask = 0;
 	objstate.MarkGettingReady();
 
	visualClass = xv_get (frame, XV_VISUAL_CLASS);
	visual = (Visual *) xv_get(frame, XV_VISUAL);
	depth = xv_get (frame, XV_DEPTH);
	
	// If 8 bit and vis class is DirectColor or TrueColor, force
	// to be PseudoColor (XSun doesn't seem to handle these visuals
	// very well).

	if (depth < 24) {
          if ((visualClass == DirectColor) || (visualClass == TrueColor))
             visualClass = PseudoColor;
          }

	// Until adobe fixes their DirectColor bug, use TrueColor instead.

        else if (visualClass == DirectColor)
           visualClass = TrueColor;

 	dpsCan = (Canvas) xv_create(frame,		CANVAS,
 			  CANVAS_AUTO_EXPAND,	FALSE,
 			  CANVAS_AUTO_SHRINK,	FALSE,
 			  CANVAS_REPAINT_PROC,	DPSCAN::RepaintProc,
 			  CANVAS_RETAINED,	FALSE,
 			  CANVAS_X_PAINT_WINDOW,TRUE,
			  XV_VISUAL,		visual,
			  XV_HELP_DATA, 	VIEWER_CANVAS_HELP,
 			  XV_NULL);

 	if (!dpsCan) {
 		err.Init(gettext("Could not create XView canvas"));
 		return (STATUS_FAILED);
 	}

	pixmapDepth = (int) xv_get(dpsCan, XV_DEPTH);

 	pwin = (Xv_window) canvas_paint_window(dpsCan);
 	dpy  = (Display *) xv_get(pwin, XV_DISPLAY);
	
	dataKey = xv_unique_key();
	(void) xv_set(dpsCan,
		      XV_KEY_DATA, DPSCAN::dataKey, (void *) this,
		      XV_NULL);

 	if ((status = XVCANVAS::Setup(frame, pwin, err)) != STATUS_OK) {
 		return (status);
 	}
 
 	dataKey = xv_unique_key();
 	(void) xv_set(pwin,
 		      WIN_EVENT_PROC, DPSCAN::EventProc,
 		      XV_KEY_DATA, DPSCAN::dataKey, (void *) this,
 		      WIN_BIT_GRAVITY,	ForgetGravity,
 		      XV_NULL);
 
	// Add dps colorcube/grayramp to colormap or use existing
 	// one.

	make_std_colormaps (frame, dpsCan, visualClass);

 	// Create DPS context
 	const Window xwin = (Window) xv_get(pwin, XV_XID);


	cmap = (Colormap) xv_get( xv_get (dpsCan, WIN_CMS), CMS_CMAP_ID);

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
	XSync (dpy, 0);

 	ctxt = XDPSCreateContext(dpy, xwin, gc, 0, 0, 0, gray_cmap, rgb_cmap, 0,
		(DPSTextProc) dps_text_handler,
		(DPSErrorProc) dps_error_handler, NULL);
 	if (ctxt == NULL) { /* order of message reverse coz err is a stack */
		err.Init(gettext("for further information."));
		err.Push(gettext("See printed AnswerBook documentation"));
		err.Push("");
		err.Push(gettext("Is Adobe Display PostScript running ?"));
		err.Push("");
 		err.Push(gettext("Could not create DPS context."));
 		return (STATUS_FAILED);
 	}
 	DPSSetContext(ctxt);
	XDPSSetStatusMask(ctxt, enable_mask, disable_mask, next_mask);
	XDPSRegisterStatusProc(ctxt, (XDPSStatusProc) dps_status_handler);

	// Find appropriate pixel value to use for xor-ing
        int dummy, colorInfo[12];
        PScurrentXgcdrawablecolor(&dummy, &dummy, &dummy, &dummy, colorInfo);
        int maxgrays = colorInfo[0];
        int graymult = colorInfo[1];
        int firstgray = colorInfo[2];
        unsigned long blackPix = (unsigned long) firstgray;
        unsigned long whitePix = (unsigned long) (firstgray
                + graymult * maxgrays);

        XSetForeground (dpy, gc, blackPix ^ whitePix);

 	// Initialize PS environment and start up a server loop
 	DPSPrintf(ctxt, "/showpage {} def\n");

/*
 * Check locale and if japanese, call the dps function to redefine
 * findfont and selecfont.
 */

	char *disp_lang = (char *) xv_get (frame, XV_LC_DISPLAY_LANG);
	if ((strcmp (disp_lang, "ja") == 0) || 
	    (strcmp (disp_lang, "japanese") == 0)) {
	   char font_prolog_file [1024];
	   FILE *fp_tmp;
	   ds_expand_pathname ("$OPENWINHOME/lib/locale/ja/viewer/fontalias.ps",
			       font_prolog_file);
	   if ( (fp_tmp = fopen (font_prolog_file, "r")) != (FILE *) NULL) {
	      char buf [2048];
	      char *result;
	      while (1) {
		 result = fgets (buf, 2048, fp_tmp);
		 if (result == (char *) NULL)
		    break;
		 DPSWritePostScript (ctxt, buf, strlen (buf));
		 }
	      fclose (fp_tmp);
	      }
	   }


 	// XXX Adobe Eclipse DPS has a non-unity scale factor
 	DPSPrintf(ctxt, "[ 1 0 0 -1 0 0] setmatrix\n");
        DPSPrintf(ctxt, "{{currentfile cvx exec} stopped { (%%PostScript Error Found: Continuing\n) print pop}{clientsync} ifelse} loop\n");	
 	objstate.MarkReady();
 	DPSDbgFunc("Leaving DPSCAN::Init");
 	return (status);
 }
 
 void
 DPSCAN::RepaintProc(const Canvas		can,
 		       const Xv_Window		pwin,
 		       Display *const		dpy,
 		       const Window		xwin,
 		       Xv_xrectlist *const	xrects)
 {
 	DbgFunc("DPSCAN::RepaintProc entered" << endl);
 	DPSDbgFunc("Entered DPSCAN::RepaintProc");

 	DPSCAN *const	canObj =
 		(DPSCAN *) xv_get(pwin, XV_KEY_DATA, DPSCAN::dataKey);
	int scroll_offset = canObj->sbar->GetViewStart();
	int canWidth = (int)xv_get(canObj->dpsCan, CANVAS_WIDTH);
	int canHght = (int)xv_get(canObj->dpsCan, CANVAS_HEIGHT);
 
 	DbgFunc("DPSCAN::RepaintProc - entered" << endl);
 	assert(canObj);
 	assert(canObj->objstate.IsReady());
	if ((canObj->ctxt) != (DPSContext) NULL) {
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
 	    		// force a ChangeGC request (because of a libdps bug)
 	    		XFlushGC(dpy, canObj->gc);
 	    		XChangeGC(dpy, canObj->fillgc, GCClipMask, &gcVal);
 	    		// force a ChangeGC request (because of a libdps bug)
 	    		XFlushGC(dpy, canObj->fillgc);
 		}
      	}
 	DPSDbgFunc("Leaving DPSCAN::RepaintProc");
 	return;
 }

 
void
DPSCAN::UpdateCanvas()
{
 	const Xv_window pwin	= canvas_paint_window(dpsCan);
 	const Window	xwin	= (Window) xv_get(pwin, XV_XID);
 	Display *const	dpy	= XV_DISPLAY_FROM_WINDOW(pwin);

	XClearArea(dpy, xwin, 0, 0, 0, 0, False);
	RepaintProc(NULL, pwin, dpy, xwin, NULL);
}


// enabling reverse video
void
DPSCAN::ChangeGCfunction()
{
 	const Xv_window pwin	= canvas_paint_window(dpsCan);
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
 DPSCAN::SetSize(const int wdth, const int hght)
 {
	static int firsttime = 1;
  	DPSDbgFunc("Entering DPSCAN::SetSize");
 	DbgFunc("DPSCAN::SetSize: desired(wdth, hght):\t"
 		<< "(" << wdth << ", " << hght << ")" << endl);
 
 	assert(objstate.IsReady());
 	assert(dpsCan);

	if (ctxt != (DPSContext) NULL) {
		if ((wdth != (int)xv_get(dpsCan, CANVAS_WIDTH)) && 
			(hght != (int)xv_get(dpsCan, CANVAS_HEIGHT))) {

			(void) xv_set(dpsCan,
 			      		CANVAS_WIDTH,	wdth,
 				      	CANVAS_HEIGHT,	hght,
 			      		XV_NULL);
		}
		else if (wdth != (int)xv_get(dpsCan,  CANVAS_WIDTH)) {
			(void) xv_set(dpsCan,
 			      		CANVAS_WIDTH,	wdth,
 			      		XV_NULL);
				
		}
		else if (hght != (int) xv_get(dpsCan, CANVAS_HEIGHT)) {
			(void) xv_set(dpsCan,
 				      	CANVAS_HEIGHT,	hght,
					XV_NULL);
		}
		else
			return;
		if (firsttime){
			AdjustPixmap(wdth, hght);
			firsttime = 0;
		}
      	}
  	DPSDbgFunc("Leaving DPSCAN::SetSize");
 	return;
 }
 

void
DPSCAN::AdjustPixmap(int newpixmapWidth, int newpixmapHeight)
{
 	const Xv_window	pwin	= canvas_paint_window(dpsCan);
 	Display *const	dpy	= (Display *) xv_get(pwin, XV_DISPLAY);

	if (newpixmapWidth == 0)
		newpixmapWidth = (int) xv_get(dpsCan, CANVAS_WIDTH);

	if (newpixmapHeight == 0)
		newpixmapHeight = (int) xv_get(dpsCan, CANVAS_HEIGHT);

	if ((pixmapWidth != newpixmapWidth)||(pixmapHeight != newpixmapHeight)){
		if (dataPage)
			XFreePixmap(dpy, dataPage);

  	       dataPage = XCreatePixmap(dpy, RootWindow(dpy,DefaultScreen(dpy)),
				newpixmapWidth, newpixmapHeight, pixmapDepth);
		XSync(dpy, 0);

		pixmapWidth = newpixmapWidth;
		pixmapHeight = newpixmapHeight;
		ps_ChangeDrawable((int)dataPage);
		ClearPage();
	}
}


 STATUS
 DPSCAN::DisplayData(const caddr_t start, const size_t nbytes)
 {
  	DPSDbgFunc("Entering DPSCAN::DisplayData");

	if ((ctxt == (DPSContext) NULL) || (nbytes <= 0)) {
	  return (STATUS_OK);
	}
	redefineSetScreen();
	defineFrameBuffer("framebuffer");
 	DPSWriteData(ctxt, (char *) start, (unsigned int) nbytes);
  	DPSDbgFunc("Leaving DPSCAN::DisplayData");
	DPSWaitContext(ctxt);
 	return (STATUS_OK);
 }
 
 STATUS
 DPSCAN::DisplayPage(const float	scale,
 		     const BBox	        &box,
 		     const caddr_t	ptr,
 		     const size_t	nbytes)
 {
  	DPSDbgFunc("Entering DPSCAN::DisplayPage");
 	ERRSTK		err;
 	STATUS		status	= STATUS_OK;

 	const Xv_window	pwin	= canvas_paint_window(dpsCan);

 	Display *const	dpy	= (Display *) xv_get(pwin, XV_DISPLAY);
 	const Window	xwin	= (Window) xv_get(pwin, XV_XID);
 
	const int	hght	= rint(BoxHght(box) * scale);
 	const int	wdth	= rint(BoxWdth(box) * scale);

 	DbgFunc("DPSCAN::DisplayPage:" << endl);
 	assert(objstate.IsReady());
 	assert(dpsCan);
 
 	if (xv_get(pwin, WIN_MAP) != TRUE) {
 		return(status);
 	}
 
 // XXX temporary fix for extra repaint bug in psviewer.cc
 if (scale == 0.0) return (status);
	if ((ctxt == (DPSContext) NULL) || (nbytes <= 0)) {
	  return (STATUS_OK);
	} 

	PrepCanvas(box, scale); 
	redefineSetScreen();
 	DPSWriteData(ctxt, (char *) ptr, (unsigned int) nbytes);
	DbgDPSOperation(WRITEDATA);
	ps_ShowPage();
	DbgDPSOperation(GRESTORE);
	DPSWaitContext(ctxt);
  	DPSDbgFunc("Leaving DPSCAN::DisplayPage");
 	return (status);
 }


void
DPSCAN::PrepCanvas(const BBox	   &displayBox,
		   const float	   scaleVal)
{
	int		origin_dx;
	int		origin_dy;
  	DPSDbgFunc("Entering DPSCAN::PrepCanvas");
	DbgFunc("DPSCAN::PrepCanvas: entered" << endl);


	origin_dy = ((int) xv_get(dpsCan, XV_HEIGHT)) -
		     rint(scaleVal*DocHght(displayBox));
	
	DPStranslate(ctxt, 0.0, origin_dy);

	// Translate the origin to the lower left corner of the displayBox
	origin_dx = -displayBox.ll_x;
	origin_dy = -displayBox.ll_y;

	// Scale the canvas
	//
	DPSscale(ctxt, scaleVal, scaleVal);
	DPSWaitContext(ctxt);

	// Translate the origin
	//
	DPStranslate(ctxt, origin_dx, origin_dy);
	DPSWaitContext(ctxt);
  	DPSDbgFunc("Leaving DPSCAN::PrepCanvas");

	return;
}

void
DPSCAN::DrawBox(const BOOL fill, const BBox &box)
{
	DbgFunc("DPSCAN::DrawBox" << endl);
        DPSDbgFunc("Entered DPSCAN::DrawBox");

	DPSDbgFunc("Performing operation: gsave");
	DPSgsave(ctxt);

	ps_DrawBox(0,
		   box.ll_x, box.ll_y, (box.ur_x - box.ll_x), (box.ur_y - box.ll_y));
	DPSDbgFunc("Performing operation: grestore");
	DPSgrestore(ctxt);
	DPSWaitContext(ctxt);

        DPSDbgFunc("Leaving DPSCAN::DrawBox");
	return;
}

 DPSCAN::~DPSCAN()
 {
	delete sbar;
 }

void
DPSCAN::ResetPostScript()
{
        DPSDbgFunc("Entered DPSCAN::ResetPostScript");  
	DPSDbgFunc("Performing operation: ps_dvRestoreObject");
	ps_dvRestoreObject();
 	ps_ChangeDrawable ((int)dataPage);
	DPSDbgFunc("Performing operation: ps_dvSaveObject");
	ps_dvSaveObject();
        DPSDbgFunc("Leaving DPSCAN::ResetPostScript");  
}



void
DPSCAN::SendBitmapOperator()
{
	DPSPrintf(ctxt, "/BITMAPCOLOR { /d 8 def gsave 	translate rotate scale /h exch def /w exch def /bitmapsave save def [ /Indexed /DeviceRGB 255 { dup red exch get 255 div exch dup green exch get 255 div exch blue exch get 255 div } ] setcolorspace << /ImageType 1 /Width w /Height h /BitsPerComponent d /Decode [0 255] /ImageMatrix [w 0 0 h neg 0 h] /DataSource currentfile /ASCIIHexDecode filter >> image bitmapsave restore grestore } bind def {/BITMAPCOLOR put}\n");
}

STRING*
DPSCAN::nameOperation(DPSOperation anOperation)
{
  STRING* nameStr = new STRING();
  switch (anOperation) {
  case ERASEPAGE:
    *nameStr = "ERSAEPAGE";
    break;
  case GSAVE:
    *nameStr = "GSAVE";
    break;
  case GRESTORE:
    *nameStr = "GRESTORE";
    break;
  case SAVECTX:
    *nameStr = "SAVECTX";
    break;
  case WAITCTX:
    *nameStr = "WAITCTX";
    break;
  case WRITEDATA:
    *nameStr = "WRITEDATA";
    break;
  case SCALE:
    *nameStr = "SCALE";
    break;
  case TRANSLATE:
    *nameStr = "TRANSLATE";
    break;
  default:
    cerr << "Unknown DPS Operation" << endl;
    break;
  }
  return nameStr;
}
void
DPSCAN::DbgDPSOperation(DPSOperation anOperation)
{
#ifdef DEBUG
  STRING* operationName = nameOperation(anOperation);
  cout << "Last operation performed: " << *operationName << endl;
  switch (anOperation) {
  case ERASEPAGE:
    operation = anOperation;
    erasePageCount++;
    cout << "Erase Page Operation Performed: " << erasePageCount << " times " << endl;
    break;
  case GSAVE:
    operation = anOperation;
    gsaveCount++;
    cout << "gsave Operation Performed: " << gsaveCount << " times " << endl;
    break;
  case GRESTORE:
    operation = anOperation;
    grestoreCount++;
    cout << "grestore Operation Performed: " << grestoreCount << " times " << endl;
    break;
  case SAVECTX:
    operation = anOperation;
    saveCtxCount++;
    cout << "Save ContextOperation Performed: " << saveCtxCount << " times " << endl;
    break;
  case WAITCTX:
    operation = anOperation;
    waitCtxCount++;
    cout << "Wait ContextOperation Performed: " << waitCtxCount << " times " << endl;
    break;
  case WRITEDATA:
    operation = anOperation;
    writeDataCount++;
    cout << "Write Data Operation Performed: " << writeDataCount << " times " << endl;    
    break;
  case SCALE:
    operation = anOperation;
    scaleCount++;
    cout << "Scale Operation Performed: " << scaleCount << " times " << endl;        
    break;
  case TRANSLATE:
    operation = anOperation;
    translateCount++;
    cout << "Translate Operation Performed: " << translateCount << " times " << endl;        
    break;
  default:
    cout << "Unknown DPS Operation " << endl;
    break;
  }
  delete operationName;
  return;
    

#endif
}

