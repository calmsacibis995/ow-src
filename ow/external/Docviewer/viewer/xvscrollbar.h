#ident "@(#)xvscrollbar.h	1.20 11/11/93 Copyright 1989-1992 Sun Microsystems, Inc."

#ifndef	_XVSCROLLBAR_H_
#define	_XVSCROLLBAR_H_

#include <xview/xview.h>
#include <xview/openwin.h>
#include <xview/scrollbar.h>

class DPSCAN;
class PSVIEWER;
class UIMGR;
// Scrollbar class
class	XVSCROLLBAR {
private:
	Xv_opaque	can;
	Scrollbar	v_scrollBar;		// vertical  Scroll bar
	Scrollbar	h_scrollBar;		// horizontal Scroll bar
	static Xv_opaque _dataKey;


public:
	XVSCROLLBAR(Xv_opaque can);

	~XVSCROLLBAR()
	{
		if ((int) xv_get(v_scrollBar, SCROLLBAR_VIEW_START))
			Reset();

		(void) xv_destroy_safe(v_scrollBar);
	}

	STATUS		Init();
	int		GetViewStart();

	BOOL		IsInited()
	{
		return((v_scrollBar) ? BOOL_TRUE : BOOL_FALSE);
	}

	void		Reset()
	{
		assert(v_scrollBar != NULL);
		(void) xv_set(v_scrollBar,
			SCROLLBAR_VIEW_START, 0,
			XV_SHOW, FALSE,
			XV_NULL);
	}

	void		Set()
	{
		assert(v_scrollBar != NULL);
		(void) xv_set(v_scrollBar, XV_SHOW, TRUE, XV_NULL);
	}

	// Return XView handle to scrollbar attached to canvas
	//
	Xv_opaque	vScrollbarHandle() const
	{
		assert(v_scrollBar);
		return(v_scrollBar);
	}

	Xv_opaque	hScrollbarHandle() const
	{
		assert(h_scrollBar);
		return(h_scrollBar);
	}
};
#endif	/* _XVSCROLLBAR_H_ */
