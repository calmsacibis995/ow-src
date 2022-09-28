#ident "@(#)docinfo.cc	1.30 94/02/22 Copyright 1990 Sun Microsystems, Inc."

#include <doc/abgroup.h>
#include <doc/book.h>
#include <doc/document.h>
#include "docinfo.h"
#ifdef SVR4
#include <locale.h>
#endif
#include <xview/frame.h>
#include <xview/panel.h>

DOCINFO::DOCINFO(Xv_opaque handle) :
	parent(handle),
	panel(XV_NULL)
{
	Xv_opaque	cmdpanel;	// panel for this command frame


	assert(parent != NULL);
	DbgFunc("DOCINFO::DOCINFO" << endl);


	frame = xv_create(parent, FRAME_CMD,
			XV_LABEL,		gettext("Page Info"),
			FRAME_SHOW_RESIZE_CORNER,	TRUE,
			NULL);

	if (frame == NULL) {
//XXX		OutOfMemory();
		return;
	}


	// Get the command panel and make it go away.
	// I think there's an XView bug the prevents us from
	// xv_destroying the panel, so we'll do the next best thing ...
	//
	if ((cmdpanel = xv_get(frame, FRAME_CMD_PANEL))  ==  NULL) {
//XXX		OutOfMemory();
		return;
	}

	xv_set(cmdpanel,
			XV_WIDTH,	1,
			XV_HEIGHT,	1,
			XV_SHOW,	FALSE,
			NULL);


	// We're ready to roll...
	//
	objstate.MarkReady();
}

DOCINFO::~DOCINFO()
{
	DbgFunc("DOCINFO::~DOCINFO" << endl);
}

STATUS
DOCINFO::Show(const DOCNAME &docname)
{
	LIST<STRING>	titles;		// list of document titles
	DOCUMENT	*doc, *next;	// document objects
	STRING		title;		// temp title string
	STRING		indent;		// amount by which title is indented
	Panel_item	info_label;
	int		y, yincr;
	Panel		oldpanel;
	BOOL		set_min_size = BOOL_FALSE;
	ERRSTK		err;


	assert(objstate.IsReady());
	assert(abgroup != NULL);
	assert(docname.IsValid());
	DbgFunc("DOCINFO::Show: " << docname << endl);


	// Add titles of document and its parents to title list.
	//
	doc = abgroup->LookUpDoc(docname, 0, err);
	for ( ; doc != NULL; doc = next) {

		if (doc->Title() != NULL_STRING) {

			title = doc->Title();
			if (doc->Label() != NULL_STRING)
				title += " (" + doc->Label() + ")";

			titles.Add(title);
		}

		next = doc->GetParent(err);
		delete(doc);
	}

	if (titles.Count() == 0)
		return(STATUS_FAILED);


	// Create a new panel.
	// Stash away the old one for later disposal.
	//
	oldpanel = panel;
	panel = xv_create(frame, PANEL,
			  XV_X,		0,
			  XV_Y,		0,
			  PANEL_LAYOUT,	PANEL_VERTICAL,
			  XV_HELP_DATA,	PAGEINFO_PANEL_HELP,
			  NULL);

	if (panel == NULL)
		return(STATUS_FAILED);


	// Create Page Info label.
	//
	info_label = xv_create(panel, PANEL_MESSAGE,
			XV_X,			xv_col(panel, 0),
			XV_Y,			xv_row(panel, 0),
			PANEL_LABEL_STRING,  gettext("The current page is:"),
			PANEL_LABEL_BOLD,	TRUE,
			NULL);


	// Create indented list of document and its ancestors.
	//
	indent = "   ";
	yincr  = (int) xv_get(info_label, XV_HEIGHT);
	yincr += yincr/4;
	y      = (int) xv_get(info_label, XV_Y)  +  2 * yincr;

	for (int i = 0; i < titles.Count(); i++) {

		title = indent + titles[titles.Count()-i-1];

		xv_create(panel, PANEL_MESSAGE,
			XV_X,			xv_col(panel, 0),
			XV_Y,			y + i*yincr,
			PANEL_LABEL_STRING,	(const char *) title,
			NULL);

		indent += "   ";
	}


	// For the first time, fit panel in the width dimension and
	// set height to some nominal value that will (at a minimum)
	// encompass all doc titles to be displayed.
	// After the first, we simply maintain the previous
	// dimensions, which may have been set by a user's resize.
	// Also set the min size to the current size.
	//
	if (oldpanel == NULL) {
		window_fit_width(panel);
		xv_set(panel, XV_HEIGHT, y + i*yincr + xv_rows(panel, 1),
			NULL);
		set_min_size = BOOL_TRUE;
	} else
		xv_set(panel,
			XV_WIDTH, (int)xv_get(oldpanel, XV_WIDTH),
			XV_HEIGHT, (int)xv_get(oldpanel, XV_HEIGHT),
			NULL);

	// Get rid of old panel.
	//
	if (oldpanel != NULL) {
		xv_set(oldpanel, XV_SHOW, FALSE, NULL);
		xv_destroy_safe(oldpanel);
	}

	// Show Page Info window.
	//
	window_fit(frame);
	if (set_min_size == BOOL_TRUE)
		xv_set(frame, FRAME_MIN_SIZE, xv_get (frame, XV_WIDTH),
				      xv_get (frame, XV_HEIGHT), NULL);

	if (xv_get (frame, XV_SHOW) != TRUE)
	   xv_set(frame, XV_SHOW, TRUE, NULL);


	return(STATUS_OK);
}
