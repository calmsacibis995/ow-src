#ident "@(#)queryhist.cc	1.32 01/14/94 Copyright 1990-1992 Sun Microsystems, Inc."


#include "queryhist.h"
#include "inputwin.h"
#include <locale.h>

QUERYHIST::QUERYHIST(Xv_opaque parent) :
	nqueries	(0)
{ 
	Panel	panel;


	DbgFunc("QUERYHIST::QUERYHIST" << endl);


	frame = xv_create(parent, FRAME_CMD,
			FRAME_LABEL,
			   gettext("AnswerBook Navigator: Previous Searches"),
			FRAME_SHOW_RESIZE_CORNER,	TRUE,
			NULL);

	if (frame == NULL)
		OutOfMemory();


	// We don't need panel.  Get rid of it.
	//
	if ((panel = xv_get(frame, FRAME_CMD_PANEL))  !=  NULL) {
		xv_set(panel, XV_SHOW, FALSE, NULL);
		//XXX xv_destroy(panel);  Should we do this???
	}


	histwin = new INPUTWIN(frame, 0, 0, 8);

	histwin->XvSet(WIN_ROWS, 8);
	histwin->XvSet(WIN_COLUMNS, 50);
	histwin->XvSet(XV_SHOW, TRUE);
	histwin->XvHelpData(SEARCH_HISTORY_HELP);

	window_fit(frame);

	histwin->XvSet(WIN_DESIRED_WIDTH, WIN_EXTEND_TO_EDGE);
	histwin->XvSet(WIN_DESIRED_HEIGHT, WIN_EXTEND_TO_EDGE);

	//Set the minimum size of the pop up to current size.
	xv_set (frame, FRAME_MIN_SIZE, xv_get (frame, XV_WIDTH),
				       xv_get (frame, XV_HEIGHT),
			NULL);

	// We're ready to roll...
	//
	objstate.MarkReady();
}

QUERYHIST:: ~QUERYHIST()                     
{
	DbgFunc("QUERYHIST::~QUERYHIST" << endl);
	delete histwin;	/* for purify */
}

// Display query history popup.
//
void
QUERYHIST::Show()
{
	assert(objstate.IsReady());
	DbgFunc("QUERYHIST::Show" << endl);
	EVENTLOG("search history");

	xv_set(frame, XV_SHOW, TRUE, NULL);
}

// Add the query text to the query history list.
//
void
QUERYHIST::AddQuery(const STRING &query)
{
	STRING	format;		// query string formatted for display purposes

	assert(objstate.IsReady());
	DbgFunc("QUERYHIST::AddQuery: " << query << endl);

	if ((format = STRING::CleanUp(query))  ==  NULL_STRING)
		return;

	histwin->AppendText(format + "\n\n");
}
