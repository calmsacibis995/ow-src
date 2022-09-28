/* RE_SID: @(%)/export/build0/source/SOURCE+SCCS_S297_FCS/external/Docviewer/viewer/SCCS/s.ttmgr.h 1.8 93/02/10 13:38:41 SMI */
#ifndef	_DOCVIEWER_TTMGR_H_
#define	_DOCVIEWER_TTMGR_H_
#ident "@(#)ttmgr.h	1.8 02/10/93 Copyright 1992 Sun Microsystems, Inc."

#include <doc/common.h>
#include <doc/tooltalk.h>
#include <doc/docname.h>

class TTMGR : public TOOLTALK {

	private:

	STRING	navigator_procid;
	STRING	program_name;


	public:

	// Navigator's ToolTalk name.
	//
	static	const STRING	tt_navigator_name;

	TTMGR()
	{
		DbgFunc("TTMGR::TTMGR: entered" << endl);
		navigator_procid = NULL_STRING;
	}

	virtual ~TTMGR();

	STATUS		Init(const int argc, char **argv, ERRSTK &err);
	
	STATUS		CreateWhereAmIReply(const Tt_message	msg,
					    const DOCNAME	&whereami,
					    ERRSTK		&err);
	
	STATUS		CreateWhatFileReply(const Tt_message	msg,
					    const STRING	&filename,
					    const int		view_page,
					    ERRSTK		&err);
	
	STATUS		CreatePingReply(const Tt_message	msg,
					const STRING		&hostname,
					const STRING		&display,
					const STRING		&viewer_name,
					ERRSTK			&err);
	STRING		GetRealDisplay(const STRING &display,
				       const STRING &hostname);

	STATUS		SendIAmYourViewerMsg(STRING	destination_procid,
					     STRING	status_string,
					     STRING	our_procid,
					     ERRSTK	&err);

	STATUS		SendDepartingMsg(STRING		destination_procid,
					 ERRSTK		&err);
};

#endif	/* _DOCVIEWER_TTMGR_H_ */
