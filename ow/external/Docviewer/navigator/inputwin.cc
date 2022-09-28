#ident "@(#)inputwin.cc	1.29 01/14/94 Copyright 1990-1992 Sun Microsystems, Inc."


// This module provides the implementation of the BMWIN class.

#define	_OTHER_TEXTSW_FUNCTIONS	//XXX see <xview/textsw.h> for clues
#include "inputwin.h"
#include <fcntl.h>


INPUTWIN::INPUTWIN(Xv_opaque frame, int x, int y, int rows)
{
	assert(frame != NULL);
	DbgFunc("INPUTWIN::INPUTWIN" << endl);


	// Create text input widget.
	//
	textsw = xv_create(frame, TEXTSW,
			XV_X,			x,
			XV_Y,			y,
			WIN_ROWS,		rows,
			TEXTSW_IGNORE_LIMIT,	TEXTSW_INFINITY,
			XV_FONT,		xv_get(frame, XV_FONT),
			XV_SHOW,		FALSE,
			NULL);

	if (textsw == NULL)
		OutOfMemory();


	// We're ready to roll...
	//
	objstate.MarkReady();
}

INPUTWIN::~INPUTWIN()
{
	DbgFunc("INPUTWIN::~INPUTWIN" << endl);
}
	
void
INPUTWIN::Show(BOOL show)
{
	DbgFunc("INPUTWIN::Show" << endl);
	assert(objstate.IsReady());
	xv_set(textsw, XV_SHOW, show, NULL);
}

// Get contents of text input window, up to 'nchars' characters
// (including terminating NULL character).
//
STRING &
INPUTWIN::GetText(int nchars)
{
	char		*buf;
	Textsw_index	nextpos;


	DbgFunc("INPUTWIN::GetText" << endl);
	assert(objstate.IsReady());


	// Allocate input buffer.
	//
	if ((buf = new char[nchars])  ==  NULL) {
		cerr << "out of memory" << endl;
		return(textsw_contents = "");
	}


	// Fetch textsw contents.
	//
	nextpos = (Textsw_index)
		xv_get(textsw, TEXTSW_CONTENTS, 0, buf, nchars);

	if (nextpos < 0)
		nextpos = 0;

	buf[nextpos] = '\0';
	textsw_contents = buf;


	// Clean up after ourselves.
	//
	delete(buf);


	return(textsw_contents);
}

// Make textsw readonly (or not)
//
BOOL
INPUTWIN::TextReadOnly(BOOL rdonly)
{
	BOOL	currstate;

	DbgFunc("INPUTWIN::TextReadOnly" << endl);
	assert(objstate.IsReady());

	currstate = (BOOL) xv_get(textsw, TEXTSW_BROWSING);
	xv_set(textsw, TEXTSW_BROWSING, (int)rdonly, NULL);

	return(currstate);
}

// See if contents of textsw have been modified since last SetText().
//
BOOL
INPUTWIN::TextModified()
{
	DbgFunc("INPUTWIN::TextModified" << endl);
	assert(objstate.IsReady());

	return(BOOL_TRUE);	//XXX
}

void
INPUTWIN::SetText(const STRING &str)
{
	BOOL	rdonly;

	DbgFunc("INPUTWIN::SetText: " << str << endl);
	assert(objstate.IsReady());

	rdonly = TextReadOnly(BOOL_FALSE);

	textsw_reset(textsw, 0, 0);	// discard all edits

//XXX	dumps core if contents == NULL, in spite of what O'Reilly says
//XXX	xv_set(textsw, TEXTSW_CONTENTS, (const char *)str, NULL);
	xv_set(textsw,
		TEXTSW_CONTENTS,	(str == NULL_STRING ? "" : (const char *)str),
		NULL);
	textsw_normalize_view(textsw, (Textsw_index)0);

	TextReadOnly(rdonly);
}

void
INPUTWIN::AppendText(const STRING &str)
{
	BOOL		rdonly;
	Textsw_index	where;
	char		*tmp;	// textsw_insert expects non-const char *

	DbgFunc("INPUTWIN::AppendText: " << str << endl);
	assert(objstate.IsReady());

	if (str == NULL_STRING)
		return;

	rdonly = TextReadOnly(BOOL_FALSE);

	xv_set(textsw, TEXTSW_INSERTION_POINT, TEXTSW_INFINITY, NULL);
	where = (Textsw_index) xv_get(textsw, TEXTSW_INSERTION_POINT);
	tmp = strdup(str);
	textsw_insert(textsw, tmp, strlen(str));
	free(tmp);
	textsw_normalize_view(textsw, (Textsw_index)0);

	TextReadOnly(rdonly);
}

void
INPUTWIN::ClearText()
{
	SetText(NULL_STRING);
}
