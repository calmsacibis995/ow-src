#ident "@(#)uimgr.cc	1.44 02/14/95 Copyright 1992 Sun Microsystems, Inc."

#include "uimgr.h"
#include <sys/utsname.h>
#include "dpscan.h"
#include "xvpscan.h"
#include "viewer.h"
#include <X11/Xutil.h>

const int MAXHOSTNAME=100;
#ifndef	ABS
#define	ABS(x)	(((x) < 0) ? -(x) : (x))
#endif
static u_short	dvIconBits[] = {
#include <images/dv.icon>
};

static u_short	dvMaskBits[] = {
#include <images/dvmask.icon>
};

static u_short	hvIconBits[] = {
#include <images/hv.icon>
};

static u_short	hvMaskBits[] = {
#include <images/hvmask.icon>
};

static const STRING NeWSServer = "NeWS";
static const STRING DPSServer  = "DPS";

Xv_opaque UIMGR::_dataKey = 0;

static XSizeHints *
GetWMHints(Display	*const	dpy,
	   const Window		xid);

UIMGR::UIMGR() :
	frame		(XV_NULL),
	got_focus	(0),
	canvas		(NULL),
	cdata		(NULL),
	eventProc	(NULL),
	magwin		(NULL),
	printwin	(NULL),
	panel		(NULL),
	resize_event	(VE__NULL_EVENT)
{
	DbgFunc("UIMGR::UIMGR: entered" << endl);
	envDisplayVar = "DISPLAY=";
}


STATUS
UIMGR::CreateBaseFrame(const ViewerType vtype, ERRSTK & /* err */)
{
	Icon		icon;
	u_short	       *iconBits;
	Server_image	iconImage;
	char	       *iconLabel;
	u_short	       *maskBits;
	Server_image	maskImage;
	STRING		label;
	extern void	NewHandler();
	extern STRING	docviewer_version;


	DbgFunc("UIMGR::CreateFrame entered" << endl);


	// Decide whether to create a HelpViewer icon or
	// a DocViewer icon.
	//
	if (vtype == HELPVIEWER) {
		iconBits	= hvIconBits;
		maskBits	= hvMaskBits;
		iconLabel	= gettext("More Help");
		label		= gettext("Help Viewer v");
	} else {
		iconBits	= dvIconBits;
		maskBits	= dvMaskBits;
		iconLabel	= gettext("Viewer");
		label		= gettext("AnswerBook Viewer v");
	}

	label += docviewer_version;


	// Create the icon.
	//
	iconImage = xv_create(XV_NULL, SERVER_IMAGE,
			      XV_WIDTH, 64,
			      XV_HEIGHT, 64,
			      SERVER_IMAGE_BITS, iconBits,
			      XV_NULL);

	maskImage = xv_create(XV_NULL, SERVER_IMAGE,
			      XV_WIDTH, 64,
			      XV_HEIGHT, 64,
			      SERVER_IMAGE_BITS, maskBits,
			      XV_NULL);
	if (iconImage == XV_NULL  ||  maskImage == XV_NULL)
		NewHandler();

	icon = xv_create(XV_NULL, ICON,
			 ICON_IMAGE, iconImage,
			 ICON_MASK_IMAGE, maskImage,
			 ICON_LABEL, iconLabel,
			 XV_HELP_DATA, VIEWER_ICON_HELP,
			 XV_NULL);

	if (icon == XV_NULL)
		NewHandler();


	_dataKey = xv_unique_key();


	// Create base frame.
	// Note that by setting the FRAME_ICON here in "xv_create()",
	// rather than later via "xv_set()", the default Viewer icon
	// can be overridden from the command line (i.e., "-icon_image <foo>").
	//
	frame = xv_create(XV_NULL,			FRAME,
			  FRAME_LABEL,			~label,
			  FRAME_NO_CONFIRM,		TRUE,
			  FRAME_ICON,			icon,
			  WIN_COLLAPSE_EXPOSURES,	TRUE,
			  WIN_USE_IM,			FALSE,
			  XV_SHOW,			FALSE,
			  XV_HELP_DATA,			VIEWER_FRAME_HELP,
			  XV_KEY_DATA,			_dataKey, (void *)this,
			  WIN_EVENT_PROC,		FrameEventProc,
			  XV_NULL);

	if (frame == XV_NULL)
		NewHandler();


	return(STATUS_OK);
}

STATUS
UIMGR::CreateIcon(const ViewerType /*vtype*/, ERRSTK & /*err*/)
{
	assert(0);
	return(STATUS_FAILED);
}

void
UIMGR::CustomMag(const BBox	&bbox,
		 const int	currentMagValue)
{
	ERRSTK		err;
	STATUS		status = STATUS_OK;
	const float	docWdth		= (float) ABS(DocWdth(bbox));

	DbgFunc("UIMGR::CustomMag: entered" << endl);

	assert(docWdth);

	if (magwin == NULL) {
	
	// Create and initialize magnification popup
	//
	   magwin = new MAGWIN();

	   if ((status == STATUS_OK) &&
	       (status = magwin->Init(frame, err)) != STATUS_OK) {
		   err.Push(gettext("Can not initialize magnification popup"));
	      }  
           magwin->SetEventProc(eventProc, cdata);
	   }

	magwin->Show(ToPercent(panel->MinWidth()/docWdth) + 1, currentMagValue);

	return;
}


void
UIMGR::FrameEventProc(const Xv_window		frame,
		      Event *const		event,
		      const Notify_arg		/* unused */)
{
	DbgFunc("UIMGR::FrameEventProc:\n" << event << endl);

	static int framewdth = 0;
	static int framehght = 0;
	int currwdth = (int) xv_get(frame, XV_WIDTH);
	int currhght = (int) xv_get(frame, XV_HEIGHT);

	switch (event_action(event)) {

	case WIN_RESIZE:
		if (event_xevent(event)->xconfigure.send_event	&&
		    xv_get(frame, XV_SHOW)) {
			UIMGR *const uimgr = (UIMGR *)
				xv_get(frame, XV_KEY_DATA, _dataKey);

			if (uimgr && uimgr->eventProc) {

				if ((framewdth != currwdth) || 
				     (framehght != currhght)){

					if (uimgr->GetResizeEvent() == VE__NULL_EVENT)
						uimgr->SetResizeEvent(VE__RESIZE);

					uimgr->eventProc(VE__RESIZE,
						 (void *) event,
						 (void *) uimgr->cdata);
					framewdth = currwdth;
					framehght = currhght;
				}
			}
		}
		break;

	default:
		break;
	}

	return;
}


void
UIMGR::DeterminePSServer()
{
	
  STRING displayhost = DisplayString((Display *)xv_get(frame, XV_DISPLAY));
  STRING defaulthost = ":0.0";

  if (displayhost == defaulthost){
  	struct utsname* localhost;
  	localhost = new(struct utsname);
  	if (uname(localhost) >= 0){
		displayhost = localhost->nodename;
		displayhost += defaulthost;
	free (localhost);
	}
  }
  envDisplayVar += displayhost; 
  // a static is required by putenv
  static char buf[MAXHOSTNAME];
  strcpy(buf, ~envDisplayVar);
  putenv(buf);
  PSFILE* testFile = ps_open_PostScript();
  if (testFile == NULL) {
    serverType = DPSServer;
  }
  else {
    serverType = NeWSServer;
    ps_close_PostScript();
  }
    
    
}		      


XVCANVAS*
UIMGR::CreateCanvas()
{
	XVCANVAS* aCanvas;

	if (serverType == DPSServer) 
		aCanvas = new DPSCAN();
	else 
		aCanvas = new XVPSCAN();

    	return aCanvas;
}


STATUS
UIMGR::Init(const ViewerType	vtype,
	    const STRING	&newshost,
	    ERRSTK		&err)
{
	STATUS		status;

	DbgFunc("UIMGR::UIMGR: Init" << endl);
	assert(objstate.IsNotReady());

	// Create & initialize window frame
	//
	if ((status = CreateBaseFrame(vtype, err)) != STATUS_OK) {
		err.Push(gettext("Cannot create XView base frame"));
	}

	// Determine whether we have to talk to a NeWS server or not:

	DeterminePSServer();


	// Create the panel
	//
	if (status == STATUS_OK) {
		// Create & initialize control panel
		panel = new XVPANEL();

		if ((status = panel->Init(frame, vtype, err)) != STATUS_OK) {
			err.Push(gettext("Can not initialize control panel"));
		}
	}

	// Create and initialize magnification popup
	// 10/14/93 joew - Now delay this until the user asks for this
	//magwin = new MAGWIN();

	//if ((status == STATUS_OK) &&
	//    (status = magwin->Init(frame, err)) != STATUS_OK) {
	//	err.Push(gettext("Can not initialize magnification popup"));
	//}


	// Create and initialize Print popup
	// 10/14/93 joew - Now delay this until the user asks for this
	//printwin = new PRINTWIN();

	//if ((status == STATUS_OK) &&
	//    (status = printwin->Init(frame, err)) != STATUS_OK) {
	//	err.Push(gettext("Can not initialize print popup"));
	//}



	// Create and initialize canvas
	//

	canvas = CreateCanvas();

	/* canvas->SetServerType(serverType); */

	if ((status = canvas->Init(frame, err)) != STATUS_OK) {
		err.Push(gettext("Could not initialize canvas."));
		objstate.MarkUnusable();
	}
	else {
		(void) xv_set(canvas->CanvasHandle(),
			      XV_Y,		panel->Height(),
			      XV_NULL);
		objstate.MarkReady();
	}

	return (status);
}

void
UIMGR::CreatePrintWin ()
{
	STATUS          status = STATUS_OK;
	ERRSTK          err;

	// Create and initialize Print popup
	//

	printwin = new PRINTWIN();

	if ((status == STATUS_OK) &&
	    (status = printwin->Init(frame, err)) != STATUS_OK) {
	        err.Push(gettext("Can not initialize print popup"));
	}
}

void
UIMGR::FitFrame(const float mag, const BBox &box)
{
	const short	wdth = (short) rint(DocWdth(box) * mag);
	const short	hght = (short) rint(DocHght(box) * mag);
	DbgFunc("UIMGR::FitFrame: entered" << endl);
	assert(objstate.IsReady());

	SetViewerSize(
		(wdth == 0)
			? wdth
			: wdth + (int)xv_get(vScrollbarHandle(), XV_WIDTH),
		(hght == 0)
			? hght 
			: hght + (int)xv_get(hScrollbarHandle(), XV_HEIGHT));
	SetFrameLimits();

	return;
}

void
UIMGR::ParseArgs(int *argcPtr, char **argv)
{
	DbgFunc("UIMGR::ParseArgs:" << endl);

	(void) xv_init(XV_INIT_ARGC_PTR_ARGV, argcPtr, argv,
		       XV_USE_LOCALE, TRUE,
		       0);

	return;
}

void
UIMGR::SetSize(const int docwdth, const int dochght)
{
	int frame_width = (int)xv_get(frame, XV_WIDTH);
	int frame_height = (int)xv_get(frame, XV_HEIGHT);
	int canvas_width = 0, canvas_height = 0;

	DbgFunc("UIMGR::SetSize: entered" << endl);
	assert(frame);
	assert(docwdth > 0);
	frame_width  -= (int)xv_get(vScrollbarHandle(), XV_WIDTH);
	frame_height -= panel->Height();
frame_height -= (int) xv_get (hScrollbarHandle(), XV_HEIGHT);
	
	if (frame_width > docwdth)
		canvas_width = frame_width;
	else
		canvas_width = docwdth;
	if (frame_height > dochght)
		canvas_height = frame_height;
	else
		canvas_height = dochght;

	canvas->SetSize(canvas_width, canvas_height);
	return;
}

void
UIMGR::SetFrameLimits()
{
	int		minHght;
	int		minWdth		= MinWidth();

	minHght = panel->Height() + 50;

	// Include scrollbar width in minimum frame width.
	minWdth += (int)xv_get(vScrollbarHandle(), XV_WIDTH);

	// Include scrollbar height in minimum frame height.
	minHght += (int)xv_get(hScrollbarHandle(), XV_HEIGHT);

	// Set the new limits on the frame
	SetMinSize(minWdth, minHght);
}

void
UIMGR::SetEventProc(const ViewerEventProc	func,
		    const void			*data)
{
	DbgFunc("UIMGR::SetEventProc: entered" << endl);
	//assert(canvas && panel && magwin);
	assert(canvas && panel);

	// this object does not handle any viewer events - so,
	// pass on our clients' event proc

	eventProc	= func;
	cdata		= (void *) data;

	canvas->SetEventProc(func, data);
	panel->SetEventProc(func, data);
	// magwin->SetEventProc(func, data); - delay till we need it now


	// The following code will tell the display server type,
	// i.e., NeWS or DPS to the viewer class
	// and the scrollbar class.


	((VIEWER*) cdata)->SetServerType(serverType);
        
}

void
UIMGR::SetMinSize(const int wdth, const int hght)
{
	Display		*dpy	= (Display *)xv_get(frame, XV_DISPLAY);
	Window		xid	= (Window) xv_get(frame, XV_XID);
	XSizeHints	*hints	= GetWMHints(dpy, xid);

	DbgFunc("UIMGR::SetMinSize: entered" << endl);

	if (hints) {

		// Set the minimum size for this frame
		hints->flags |= PMinSize;
		hints->min_width = wdth;
		hints->min_height = hght;

		XSetWMNormalHints(dpy, xid, hints);

		XFree((char *) hints);
	}

	return;
}

void
UIMGR::SetViewerSize(const int		wdth,
		     const int		hght)
{
	int       currHght        = (int) xv_get(frame, XV_HEIGHT);
	int       currWdth        = (int) xv_get(frame, XV_WIDTH);

	// The arguments `wdth' and `hght' are with respect to the
	// document, taking into account the magnification factor.
	// The actual frame height needs to account for the control
	// panel.
	//
	
	int	Hght		= hght + panel->Height();

	// And also the Scrollbar width of the horizontal scrollbar 
	// in the case of the dps canvas

	Rect		*rect;
	rect = (Rect *) xv_get(frame, WIN_SCREEN_RECT);

	if (Hght > (rect->r_height - 75)) {
            	Hght = rect->r_height - 75;
	}

	DbgFunc("UIMGR::SetViewerSize: current size:\t("
		<< currWdth << ", " << currHght << ")" << endl);

	DbgFunc("UIMGR::SetViewerSize: desired size:\t("
		<< wdth << ", " << Hght << ")" << endl);

	assert(objstate.IsReady());

	if (wdth && Hght && (currWdth != wdth) && (currHght != Hght)) {
		(void) xv_set(frame,
			      XV_HEIGHT,		Hght,
			      XV_WIDTH,			wdth,
			      XV_NULL);
	}
	else if (wdth && (currWdth != wdth)) {
		(void) xv_set(frame,
			      XV_WIDTH,			wdth,
			      XV_NULL);
	}
	else if (Hght && (currHght != Hght)) {
		(void) xv_set(frame,
			      XV_HEIGHT,		Hght,
			      XV_NULL);
	}
	DbgMed("UIMGR::SetViewerSize: new: " << wdth << ", " << Hght <<
	       endl);

	return;
}


UIMGR::~UIMGR()
{
	DbgFunc("UIMGR::UIMGR" << endl);

	if (canvas)
		delete canvas;

	if (panel)
		delete panel;

	if (magwin)
		delete magwin;

	if (printwin)
		delete printwin;
}

XSizeHints *
GetWMHints(Display	*const	dpy,
	   const Window		xid)
{
	XSizeHints     *hints;
	u_long		usrDef	= NULL;

	DbgFunc("uimgr.cc:GetWMHints: entered" << endl);
	assert(dpy && xid);

	if (!(hints = XAllocSizeHints())) {
		DbgHigh("UIMGR::ClearAspectRatio: XAllocSizeHints failed\n");
	}

	if (hints && !XGetWMNormalHints(dpy, xid, hints, (long *) &usrDef)) {
		DbgHigh("UIMGR::ClearAspectRatio: " <<
			"XGetWMNormalHints failed" << endl);
		XFree((char *) hints);
		hints = NULL;
	}

	return(hints);
}


void
UIMGR::GetInputFocus()
{
	Display *const dpy = (Display *)xv_get(frame, XV_DISPLAY);

	XGetInputFocus(dpy, &focus_winid, &revert_to);
	got_focus = 1;
}

void
UIMGR::SetInputFocus()
{
	Display *const dpy = (Display *)xv_get(frame, XV_DISPLAY);

	if(got_focus){
		XSetInputFocus(dpy, focus_winid, revert_to, CurrentTime);
		got_focus = 0;
	}
}

	

#ifdef	DEBUG
ostream &
operator << (ostream &ostr, Event *const event)
{
	ostr << "\tevent->ie_code\t= " << event->ie_code << endl;
	ostr << "\tevent->action\t= " << event_action(event) << endl;
	ostr << "\tevent->ie_shiftmask\t= " << event_shiftmask(event) << endl;
	ostr << "\tevent->ie_locx\t= " << event_x(event) << endl;
	ostr << "\tevent->ie_locy\t= " << event_y(event) << endl;
	ostr << "\tevent->ie_time\t= " << event_time(event).tv_sec;
	ostr << " " << event_time(event).tv_usec;

	return(ostr);
}
#endif	/* DEBUG */
