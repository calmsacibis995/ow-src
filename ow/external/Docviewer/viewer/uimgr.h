/* RE_SID: @(%)/export/build0/source/SOURCE+SCCS_S297_FCS/external/Docviewer/viewer/SCCS/s.uimgr.h 1.31 93/12/20 16:00:38 SMI */
#ifndef	_DOCVIEWER_UIMGR_H
#define	_DOCVIEWER_UIMGR_H

#ident "@(#)uimgr.h	1.31 12/20/93 Copyright 1992 Sun Microsystems, Inc."

#include "common.h"
#include "xvcanvas.h"
#include "xvpanel.h"
#include "xvsubwin.h"
#include "printwin.h"
#include <doc/ps_link.h>
#include <xview/xview.h>
#include <xview/rect.h>
#include <NeWS/psmacros.h>

class   VIEWER;
class	UIMGR {

    private:
	// XView objects
	Frame		frame;
	Window		focus_winid;
	int		revert_to;
	int		got_focus;

	// UI related C++ objects
	XVCANVAS	*canvas;
	XVPANEL		*panel;
	MAGWIN		*magwin;
	PRINTWIN	*printwin;
	PSVIEWER         *parentViewer;
	// Stores the string DISPLAY=<displayhostname>
	STRING		envDisplayVar;

	// Current state of this object.
	//
	OBJECT_STATE	objstate;
	STRING          serverType;

	// Client of this object can register an event proc to handle
	// events that this object cannot handle
	//
	ViewerEventProc	eventProc;
	void	       *cdata;
	ViewerEvent	resize_event;	// type of event during resize

	static Xv_opaque _dataKey;

	STATUS		CreateBaseFrame(const ViewerType	vtype,
					ERRSTK			&err);

	STATUS		CreateIcon(const ViewerType		vtype,
				   ERRSTK			&err);

	STATUS		CreateStipple(ERRSTK			&err);

	static void	FrameEventProc(const Xv_window		win,
				       Event *const		ev,
				       const Notify_arg		/* unused */);

    public:
	
	// UIMGR constuctor, destructor
	//
	UIMGR();
	~UIMGR();

	void		ClearPage()
	{
		canvas->ClearPage();
	}

	void 		AdjustPixmap(int newpixmapWidth, int newpixmapHeight)
	{
			canvas->AdjustPixmap(newpixmapWidth, newpixmapHeight);
	}

	void		UpdateCanvas()
	{
			canvas->UpdateCanvas();
	}

	void		CustomMag(const BBox			&bbox,
				  const int			magValue);

	STATUS		PrintDocument(const DOCNAME &docname,
				      const int	    currViewPage,
				      ERRSTK &err)
	{
		DbgFunc("UIMGR::PrintDocument: entered" << endl);
		return (printwin->PrintDocument(abgroup, docname,
					currViewPage, err));
	}

	void		DisableGoBack(const BOOL bool)
	{
		assert(panel);
		assert(objstate.IsReady());
		panel->DisableGoBack(bool);
	}

	void		DrawSelectedLink(const PSLINK	&link,
					 const BBox	&docbox,
					 const float	mag)
	{
		DbgFunc("UIMGR::DrawSelectedLink:" << endl);
		assert(canvas);
		canvas->DrawSelectedLink(link, docbox, mag);
	}

	void	DrawLinkBoxes(LIST<PSLINK*>		&links,
			      const int			selected)
       {
	       canvas->DrawLinkBoxes(links, selected);
       }

	void		DrawLink(const PSLINK		&link,
				 const BBox		&docbox,
				 const float		mag)
	{
		DbgFunc("UIMGR::DrawLink:" << endl);
		assert(canvas);
		canvas->DrawLink(link, docbox, mag);
	}
				       
	void		EnableFollowLinkItem(const BOOL bool)
	{
		panel->EnableFollowLinkItem(bool);
	}

	void		ParseArgs(int *argcPtr, char **argv);

	STATUS		Init(const ViewerType	vtype,

			     const STRING	&owserver_host,
			     ERRSTK		&err);

	// Create printwin
	void		CreatePrintWin ();

	// Return XView handle to main window frame
	//
	Xv_opaque	FrameHandle() const
	{
		assert(frame);
		assert(objstate.IsReady());
		return(frame);
	}

	Xv_opaque	vScrollbarHandle() const
	{
		return (canvas->vScrollbarHandle());
	}

	Xv_opaque	hScrollbarHandle() const
	{
		return (canvas->hScrollbarHandle());
	}

	Xv_opaque	PrintwinHandle() const
	{
		if (printwin == NULL)
		   return ((Xv_opaque) NULL);
		else
		   return (printwin->FrameHandle());
	}

	void		FitFrame(const float mag, const BBox &box);

	// Minimum allowable width
	//
	int		MinWidth() const
	{
		assert(panel);
		assert(objstate.IsReady());
		return(panel->MinWidth());
	}

	STATUS		SendData(const caddr_t start, const size_t nbytes)
	{
		return(canvas->DisplayData(start, nbytes));
	}

	STATUS		DisplayPage(const float		scaleVal,
				    const BBox		&displayBox,
				    const caddr_t	start,
				    const size_t	nbytes)
	{
		return(canvas->DisplayPage(scaleVal, displayBox,
					   start, nbytes));
	}

	void		ResetPostScript()
	{
		canvas->ResetPostScript();
	}

	void		InitFrame(const float mag, const BBox &box);

	// Set client's event handler
	void		SetEventProc(const ViewerEventProc	ptrToFunc,
				     const void		       *client_data);

	void		SetResizeEvent(ViewerEvent event)
	{
		resize_event = event;
	}

	ViewerEvent	GetResizeEvent()
	{
		return (resize_event);
	}

        void            CheckScrollbars ()
        {        
                        Xv_opaque       can = canvas->CanvasHandle (); 
                        if ( (int) xv_get (can, XV_WIDTH) >= 
                                (int) xv_get (can, CANVAS_WIDTH))
                           xv_set (hScrollbarHandle (),
                                SCROLLBAR_VIEW_START, 0,
                                XV_NULL);

                        if ( (int) xv_get (can, XV_HEIGHT) >= 
                                (int) xv_get (can, CANVAS_HEIGHT)) 
                           xv_set(vScrollbarHandle(),
                                SCROLLBAR_VIEW_START, 0,
                                XV_NULL);
        } 

        void            ResetScrollbars()
        {
                        xv_set(vScrollbarHandle(),
                                SCROLLBAR_VIEW_START, 0,
                                XV_NULL);

                        xv_set(hScrollbarHandle(),
                                SCROLLBAR_VIEW_START, 0,
                                XV_NULL);
        }

	void		SetFrameLimits();


	void		SetMinSize(const int			wdth,
				   const int			hght);

       void		SetViewerSize(const int			wdth,
				      const int			hght);

	void		SetSize(const int		docwdth,
				 const int		dochght);

        void            DeterminePSServer();

        XVCANVAS*       CreateCanvas();

        void            SendBitmapOperator() {canvas->SendBitmapOperator();}

	void		GetInputFocus();

	void		SetInputFocus();

	const STRING	&GetServerType() const	{ return(serverType); }
};

#ifdef	DEBUG
extern ostream& operator << (ostream &ostr, Event *const event);
#endif	/* DEBUG */
#endif	/* _DOCVIEWER_UIMGR_H */
