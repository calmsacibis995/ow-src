/* RE_SID: @(%)/export/build0/source/SOURCE+SCCS_S297_FCS/external/Docviewer/viewer/SCCS/s.xvcanvas.h 1.85 93/11/11 18:29:28 SMI */
#ifndef	_VIEWER_XVCANVAS_H_
#define	_VIEWER_XVCANVAS_H_

#ident "@(#)xvcanvas.h	1.85 11/11/93 Copyright 1992 Sun Microsystems, Inc."

#include <stdio.h>
#include <xview/xview.h>
#include <xview/xv_xrect.h>
#include <xview/rect.h>
#include <X11/Xutil.h>
#include "xvscrollbar.h"
#include <doc/ps_link.h>

// DocViewer XView Canvas class.
class	XVCANVAS {
	
	protected:

	Menu		canvasMenu;
	XVSCROLLBAR	*sbar;			// Scrollbar
	Xv_opaque	sbar_bg;	// handle for scrollbar background
					// panel, used when scrollbar is off

	// Client of this object can register an event proc to handle
	// events that this object cannot handle
	//
	ViewerEventProc	defaultEventProc;
	void	       *cdata;


	OBJECT_STATE	objstate;

	static Xv_opaque	dataKey;
	static Xv_opaque	canvasKey;
	STRING                  serverType; // A NeWS server or DPS server?
	// Private Functions
	STATUS			CreateMenu(const Frame		frame,
					   ERRSTK		&err);

	static void		EventProc(const Xv_Window	win,
					  Event *const		event);


	XVisualInfo		*VisualInfo(Display *const	dpy,
					    const int		screen,
					    const int		depth,
					    int			*nvisuals);

	STATUS			Handle4BitVisual(Display *const	dpy,
						 int		*visualClass,
						 int		*depth,
						 ERRSTK		&err);

	STATUS			VisualClass(const Frame		frame,
					    int			*visualClass,
					    int			*depth,
					    ERRSTK		&err);

	static void		MenuActionProc(Menu		menu,
					       Menu_item	menuItem);

	STATUS			Setup(const Frame		frame,
				      const Xv_window		win,
				      ERRSTK			&err);

	XVCANVAS();

	public:

	virtual ~XVCANVAS();

	// Virtual members to be defined by subclass
	//

	virtual void	ClearPage() = 0;



	// Send PostScript document prolog to PostScript server.
	//
	virtual STATUS	DisplayData(const caddr_t	start,
				    const size_t	nbytes) = 0;

	// Send PostScript document page to PostScript server.
	//
	virtual STATUS	DisplayPage(const float		scaleVal,
				    const BBox		&displayBox,
				    const caddr_t	ptr,
				    const size_t	nbytes) = 0;

	virtual void	DrawLink(const PSLINK		&link,
				 const BBox		&docbox,
				 const float		mag) = 0;

	virtual void	DrawLinkBoxes(LIST<PSLINK*>	&links,
				      const int		selected) = 0;

	virtual void	DrawSelectedLink(const PSLINK	&link,
					 const BBox	&docbox,
					 const float	mag) = 0;

	virtual STATUS	Init(const Frame		frame,
			     ERRSTK			&err) = 0;

	virtual void	AdjustPixmap(int, int) =0;
	virtual void	UpdateCanvas() =0;
	virtual void	ResetPostScript()=0;
        virtual void 	SendBitmapOperator()=0;

	// Set client's event handler
	void		SetEventProc(const ViewerEventProc ptrToFunc,
				     const void		*client_data)
	{
		defaultEventProc	= ptrToFunc;
		cdata			= (void *) client_data;
	}

	virtual void		SetSize(const int	width,
					const int	height) = 0;


	virtual Xv_window CanvasHandle() const = 0;

	Xv_opaque       vScrollbarHandle() const 
	{ 
		return (sbar->vScrollbarHandle());   
	}

	Xv_opaque       hScrollbarHandle() const 
	{ 
		return (sbar->hScrollbarHandle());   
	}

	Xv_opaque       ScrollbarBgHandle() const 
	{ 
		return (sbar_bg);   
	}
   
        XVSCROLLBAR* GetScrollBar() {return sbar;}

	void         SetServerType(STRING whatType) {serverType = whatType;}

};

#ifdef	DEBUG
extern ostream &operator << (ostream &, Event const* );
#endif	/* DEBUG */

#endif	/* _VIEWER_XVCANVAS_H_ */

