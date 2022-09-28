/* RE_SID: @(%)/export/build0/source/SOURCE+SCCS_S297_FCS/external/Docviewer/viewer/SCCS/s.viewer.h 1.48 95/02/14 17:24:40 SMI */
#ifndef	_VIEWER_H_
#define	_VIEWER_H_

#ident "@(#)viewer.h	1.48 02/14/95 Copyright 1989 Sun Microsystems, Inc."

#include "common.h"
#include "hist_stack.h"
#include "ttmgr.h"
#include "uimgr.h"
#include <doc/docname.h>
#include "docfinder.h"
#include "docinfo.h"
#include "magval.h"

class	TTMGR;
class	UIMGR;

class	VIEWER {
	private:

	STRING			helppath;
	TTMGR		       *ttmgr;
	OBJECT_STATE		objstate;

	STRING			argv_procid; // TTalk procid passed in arg list
	STRING			argv_docname; // docname passed in arg list
	STRING			argv_viewfile; // view file passed in arg list

	static Xv_opaque	_dataKey;

	static Notify_value	DestroyProc(const Notify_client	client,
					    const Destroy_status dstatus);

	static Notify_value	SignalProc(const Notify_client	client,
					   const int		sig,
					   const Notify_signal_mode when);

	static TtReplyStatus	MessageProc(void		*client_data,
					    const Tt_message	msg);

	STATUS			ParseArgs(const int	argc,
					  char *const	*argv,
					  STRING	&newshost,
			      		  STRING	&preferred_language,
			      		  STRING	&card_catalog,
					  ERRSTK	&err);

    	void			DisplayError( ERRSTK &err);
    
	BOOL			UserSetSize();

	STATUS			WhereAmI(DOCNAME	&docname,
					 ERRSTK		&err);

	void			SetHelpPath();

	STATUS			InitUI(int		*argc,
				       char		**argv,
				       const ViewerType	vtype,
				       STRING		&preferred_language,
			      	       STRING		&card_catalog,
				       ERRSTK		&err);

	protected:

	int		currPage;	// current page
	DOCNAME		currDocName;	// name of current document object
	float		defaultMag;
	DOCFINDER	*docfinder;	// translate page # to document object
	DOCINFO		*docinfo;	// Page Info object
	MAGVAL		mag;
	STRING		modeLineStr;
	ViewerType	vtype;

	HIST_STACK	*goback;		// LIFO stack for goback info
	UIMGR		*uimgr;
	STRING                  serverType;     
	                             // Is this a News server or a DPS server?

	VIEWER(const ViewerType vtype);

	// Event handler for ViewerEvents. This method is declared
	// "static" because it is used as a callback in an environment
	// where its class is not known.

	static void		EventProc(const ViewerEvent	event,
					  const void	       *ptrToEventObj,
					  void		       *cdata);

	STATUS			ExecuteLink(const DVLINK	&dvlink,
					    ERRSTK		&err);

	virtual	int		GetCurrPage()	const		= 0;
	virtual	const STRING	&GetDocName()	const		= 0;
	virtual	int		GetNumPages()	const		= 0;

	// Return the DisplayBox for this doc
	virtual const BBox	&DocDisplayBox() const		= 0;

	virtual STATUS		GoBack();

	// Document positioning
	virtual STATUS		GotoFirstPage()			= 0;
	virtual STATUS		GotoLastPage()			= 0;
	virtual STATUS		GotoNextPage();
	virtual	STATUS		GotoPage(const int)		= 0;
	virtual STATUS		GotoPrevPage();
	virtual STATUS		GotoNextDoc();
	virtual STATUS		GotoPrevDoc();

	virtual void		HideLinks()			= 0;

	// Load a new document
	virtual	STATUS		LoadDocument(const DOCNAME &docname,
					     const STRING  &filename,
					     const STRING  &path,
					     ERRSTK &) = 0;

	// clear stack proc
	STATUS			ClearStack();
	
	// Repaint proc
	virtual STATUS		Repaint()			= 0;

	// Resize proc
	virtual void		ResizeProc();

	// Set current LDA object (document)
	virtual void		SetCurrDoc(const DOCNAME &)	= 0;

	virtual const DOCNAME	&GetCurrDoc() const		= 0;

	void			GetCurrDocPath(STRING &currDocDir);

	STATUS			GetDocOnPage(int	page,
					     DOCNAME	&docname,
					     ERRSTK	&err);

	virtual	STATUS		SetSelection(Event *const)	= 0;
	virtual STATUS		ExecuteSelection()		= 0;

	// Display links
	virtual void		ShowLinks()			= 0;

	// Describe document we are currently viewing
	void			ShowPageInfo(int);

	// Show printing information for document we are currently viewing
	void			ShowPrintInfo(int);

	// Magnification related stuff
	void		Larger();
	STATUS		Magnify(const float mag);
	void		Smaller();
	void		StdSize();

	const STRING		 &MakeModeLine();

	public:

	virtual			~VIEWER();

	STATUS			Init(int	*argcPtr,
				     char	**argv,
				     ERRSTK	&err);

	STATUS		Start(const STRING	&linkText,
			      const STRING	&preferred_language,
			      const STRING	&card_catalog,
			      ERRSTK		&err);
	void            SetServerType(STRING whatType) {serverType = whatType;}
	UIMGR*          GetUiMgrAddress() {return uimgr;}
};

#endif	/* _VIEWER_H_ */

