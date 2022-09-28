#ident "@(#)xvscrollbar.cc	1.22 05/10/95 Copyright 1989-1992 Sun Microsystems, Inc."

#include "common.h"
#include <math.h>
#include "xvscrollbar.h"
#include "dpscan.h"
#include "psviewer.h"

Xv_opaque XVSCROLLBAR::_dataKey = 0;

XVSCROLLBAR::XVSCROLLBAR(Xv_opaque canArg) :
	v_scrollBar	(NULL),
	h_scrollBar	(NULL)
{
	can		= canArg;
}

STATUS
XVSCROLLBAR::Init()
{
	STATUS		status		= STATUS_OK;
        Visual          *visual = (Visual *)xv_get(can, XV_VISUAL);



	v_scrollBar = (Scrollbar)
		xv_create(can, 	SCROLLBAR,
			SCROLLBAR_DIRECTION,  SCROLLBAR_VERTICAL,
			XV_VISUAL, visual,
			NULL);
	if (!v_scrollBar)
		status = STATUS_FAILED;

	h_scrollBar = (Scrollbar)
		xv_create(can, SCROLLBAR,
			SCROLLBAR_DIRECTION, SCROLLBAR_HORIZONTAL,
			XV_VISUAL, visual,
			NULL);

	if (!h_scrollBar)
		status = STATUS_FAILED;

	return(status);
}


int
XVSCROLLBAR::GetViewStart()
{

	assert(v_scrollBar != NULL);
	return((int) xv_get(v_scrollBar, SCROLLBAR_VIEW_START) *
		(int) xv_get(v_scrollBar, SCROLLBAR_PIXELS_PER_UNIT));
}

