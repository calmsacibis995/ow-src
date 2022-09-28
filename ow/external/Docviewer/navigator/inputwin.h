#ifndef	_INPUTWIN_H
#define	_INPUTWIN_H

#ident "@(#)inputwin.h	1.23 06/11/93 Copyright 1990-1992 Sun Microsystems, Inc."



#include "navigator.h"
#include "xview.h"
#include <xview/textsw.h>


#define	MAX_INPUT_SIZE	MAX_LINE_LEN


// Generic Input Window class definition.
//
class	INPUTWIN {

    protected:

	// Text subwindow containing queries.
	//
	Textsw		textsw;

	// Actual contents of query text subwindow.
	//
	STRING		textsw_contents;

	// Current state of this object.
	//
	OBJECT_STATE	objstate;


    public:
	
	// INPUTWIN constructor, destructor.
	//
	INPUTWIN(Xv_opaque parent, int x, int y, int rows);
	~INPUTWIN();

	void		Show(BOOL show=BOOL_TRUE); // Display the window

	STRING		&GetText(int n=MAX_INPUT_SIZE);	// Get the text
	void		SetText(const STRING &);	// Set the text
	void		AppendText(const STRING &);	// Add text to end
	void		ClearText();		// Clear window contents
	BOOL		TextReadOnly(BOOL rdonly=BOOL_TRUE);
	BOOL		TextModified();


	// Provide low-level access to XView functions:
	// attribute set/get routines, spot help, and XView object handle.
	//
	Xv_opaque	XvHandle()	{ return((Xv_opaque)textsw); }

	int		XvGet(int a)	{ return((int)xv_get(textsw, a)); }

	void		XvSet(int a, int v)
			{ xv_set(textsw, a, v, NULL); }

	Xv_opaque	XvViewWindow()
			{ return(xv_get(textsw, OPENWIN_NTH_VIEW, 0)); }

	void		XvHelpData(const char *help)
			{ xv_set(XvViewWindow(), XV_HELP_DATA, help, NULL); }

	void		XvSetFocus()
			{ xv_set(XvViewWindow(), WIN_SET_FOCUS, NULL); }
};

#endif	_INPUTWIN_H
