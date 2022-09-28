/* RE_SID: @(%)/export/build0/source/SOURCE+SCCS_S297_FCS/external/Docviewer/viewer/SCCS/s.docinfo.h 1.11 93/03/09 12:22:29 SMI */
#ifndef	_VIEWER_DOCINFO_H_
#define	_VIEWER_DOCINFO_H_

#ident "@(#)docinfo.h	1.11 93/03/09 Copyright 1990-1992 Sun Microsystems, Inc."

#include "common.h"
#include <xview/xview.h>


class	DOCINFO {
	
    private:

	Xv_opaque	parent;
	Xv_opaque	frame;
	Xv_opaque	panel;
	
	OBJECT_STATE	objstate;


    public:

	DOCINFO(Xv_opaque parent);
	~DOCINFO();

	// Return XView handle to popup window frame
	//
	Xv_opaque	FrameHandle() const
	{
		assert(frame);
		return(frame);
	}

	STATUS	Show(const DOCNAME &docname);
};

#endif	/* _VIEWER_DOCINFO_H_ */
