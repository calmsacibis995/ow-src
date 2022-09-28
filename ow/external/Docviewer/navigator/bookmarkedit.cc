#ident "@(#)bookmarkedit.cc	1.34 01/14/94 Copyright 1992 Sun Microsystems, Inc."


#include "bookmarkedit.h"
#include <doc/bookmark.h>
#include "inputwin.h"
#include <locale.h>
#include <doc/notify.h>


static int	OBJ_KEY   = (int) xv_unique_key();


BOOKMARKEDIT::BOOKMARKEDIT(Xv_opaque parent) :
	bookmark		(NULL),
	frame			(NULL),
	panel			(NULL),
	editwin			(NULL),
	bmnotify		(NULL),
	event_handler		(NULL),
	event_client_data	(NULL)
{
	int	x, y;


	DbgFunc("BOOKMARKEDIT::BOOKMARKEDIT" << endl);


	// Create popup command frame for edit window.
	//
	frame = xv_create(parent, FRAME_CMD,
			XV_WIDTH,		xv_get(parent, XV_WIDTH),
			XV_HEIGHT,		200,		//XXX
			FRAME_SHOW_RESIZE_CORNER, TRUE,
			FRAME_DONE_PROC,	BOOKMARKEDIT::FrameDoneProc,
			XV_KEY_DATA,		OBJ_KEY,	this,
			XV_SHOW,		FALSE,
			NULL);

	if (frame == NULL)
		OutOfMemory();


	// Get the frame's command panel and whip it into shape.
	//
	if ((panel = xv_get(frame, FRAME_CMD_PANEL))  ==  NULL)
		OutOfMemory();

	xv_set(panel,
			WIN_DESIRED_WIDTH,	WIN_EXTEND_TO_EDGE,
			WIN_DESIRED_HEIGHT,	WIN_EXTEND_TO_EDGE,
			XV_KEY_DATA,		OBJ_KEY,	this,
			WIN_EVENT_PROC,		BOOKMARKEDIT::WinEventProc,
			XV_HELP_DATA,		BMNEW_HELP,
			NULL);


	// Create "Bookmark for:" field.
	//
	title_widget = xv_create(panel, PANEL_TEXT,
			PANEL_LABEL_STRING,	gettext("Bookmark for:"),
			PANEL_LABEL_BOLD,	TRUE,
			PANEL_VALUE_STORED_LENGTH, 200,
			PANEL_CLIENT_DATA,	(caddr_t)this,
			PANEL_NOTIFY_PROC,	BOOKMARKEDIT::TitleNotifyProc,
			XV_HELP_DATA,		BMNEW_TITLE_HELP,
			NULL);

	// Create "Comment:" label.
	//
	label_widget = xv_create(panel, PANEL_MESSAGE,
			PANEL_LABEL_STRING,	gettext("Comment:"),
			PANEL_LABEL_BOLD,	TRUE,
			NULL);


	if (title_widget == NULL  ||  label_widget == NULL)
		OutOfMemory();

	// Create comment window.
	//
	x  = xv_col(panel, 0),
	y  = (int) xv_get(label_widget, XV_Y);
	y += (int) xv_get(label_widget, XV_HEIGHT);

	editwin = new INPUTWIN(frame, x, y, 6);
	editwin->XvHelpData(BMNEW_COMMENT_HELP);
	editwin->XvSet(WIN_DESIRED_WIDTH, WIN_EXTEND_TO_EDGE);
	editwin->Show();


	// Create "Save" button.
	//
	save_button = xv_create(panel, PANEL_BUTTON,
			PANEL_LABEL_STRING,	gettext("Create"),
			PANEL_CLIENT_DATA,	(caddr_t)this,
			PANEL_NOTIFY_PROC,	BOOKMARKEDIT::ButtonEvent,
			XV_HELP_DATA,		BMNEW_CREATE_BUTTON_HELP,
			NULL);

	if (save_button == NULL)
		OutOfMemory();


	// Create message handler.
	//
	bmnotify = new NOTIFY(frame);
	bmnotify->Title(gettext("New Bookmark"));
	
	// Set minimum size to current size
	xv_set (frame, FRAME_MIN_SIZE, xv_get (frame, XV_WIDTH),
				       xv_get (frame, XV_HEIGHT),
		       NULL);

	// We're ready to roll...
	//
	objstate.MarkReady();
}

BOOKMARKEDIT::~BOOKMARKEDIT()
{
	DbgFunc("BOOKMARKEDIT::~BOOKMARKEDIT" << endl);

	if (frame)
		xv_destroy_safe(frame);
	if (editwin)
		delete(editwin);
	if (bmnotify)
		delete(bmnotify);
}

void
BOOKMARKEDIT::Edit(BOOKMARK *edit)
{
	assert(objstate.IsReady());
	assert(edit != NULL);
	DbgFunc("BOOKMARKEDIT::Edit" << endl);

	bookmark = edit;


	// Initialize "Title" field.
	//
	xv_set(title_widget,
		PANEL_VALUE,		~bookmark->Title(),
		NULL);


	// Initialize comment window.
	// Set frame's title bar.
	// Display correct 'action' button.
	//
	editwin->ClearText();


	// Display editing window.
	//
	xv_set(frame, XV_SHOW, TRUE, NULL);
	ResizeEvent();	// make sure all the widgets are in the right place
}

void
BOOKMARKEDIT::Done()
{
	assert(objstate.IsReady());
	assert(bookmark != NULL);
	DbgFunc("BOOKMARKEDIT::Done" << endl);


	// Get rid of current bookmark
	//
	xv_set(title_widget,
		PANEL_VALUE,		"",
		NULL);

	editwin->ClearText();

	xv_set(frame, FRAME_CMD_PIN_STATE, FRAME_CMD_PIN_OUT, NULL);
	bookmark = NULL;
}

void
BOOKMARKEDIT::SaveEvent()
{
	STRING	title;		// bookmark comment field
	STRING	comment;	// bookmark title field


	assert(objstate.IsReady());
	assert(bookmark != NULL);
	DbgFunc("BOOKMARKEDIT::SaveEvent" << endl);


	// Use the values from the panel items
	// to update the bookmark 'title' and 'comment' fields.
	//
	title = (const char *) xv_get(title_widget, PANEL_VALUE);
	comment = editwin->GetText(BM_MAXCOMMENT);

	bookmark->SetTitle(title);
	bookmark->SetAnnotation(comment);


	// Invoke handler for this event, if one has been registerd.
	//
	if (event_handler != NULL)
	 (*event_handler)(BMEDIT_SAVE_EVENT, (caddr_t)this, event_client_data);


	// Take the window down
	//
	Dismiss();
}

void
BOOKMARKEDIT::CancelEvent()
{
	assert(objstate.IsReady());
	assert(bookmark != NULL);
	DbgFunc("BOOKMARKEDIT::CancelEvent" << endl);


	// Invoke handler for this event, if one has been registerd.
	//
	if (event_handler != NULL){
		(*event_handler)(	BMEDIT_CANCEL_EVENT,
					(caddr_t)this,
					event_client_data);
	}


	// Take down the window (the pushpin is already out - that's why
	// we got called to begin with).
	//
	Dismiss();
}

void
BOOKMARKEDIT::ResizeEvent()
{
	int	x, y;		// x, y coordinates of 'Save' button
	int	height;		// height of comment window
	int	panel_width;
	int	panel_height;
	int	title_width;


	assert(objstate.IsReady());
	DbgFunc("BOOKMARKEDIT::ResizeEvent" << endl);


	panel_width  = (int)xv_get(panel, XV_WIDTH);
	panel_height = (int)xv_get(panel, XV_HEIGHT);

	// Position 'Save' button in middle of window at the bottom.
	//
	x  = panel_width - (int)xv_get(save_button, XV_WIDTH);
	x /= 2;
	y  = panel_height - PANEL_ITEM_Y_START;
	y -= (int)xv_get(save_button, XV_HEIGHT);

	if (x < 0) x = 0;		// watch out for really small window
	if (y < 0) y = 0;		// watch out for really small window
	xv_set(save_button, XV_X, x, XV_Y, y, NULL);


	// Adjust comment window so that it doesn't obscure 'Save' button.
	//
	height  = (int)xv_get(save_button, XV_Y)  -  (int)editwin->XvGet(XV_Y);
	height -= (int)xv_get(panel, WIN_ROW_GAP);
	if (height < 20) height = 20;	// watch out for really small window

	editwin->XvSet(XV_HEIGHT, height);


	// Adjust width of Title field.
	//
	title_width  = panel_width - 25;
	title_width -= (int)xv_get(title_widget, PANEL_VALUE_X);
	xv_set(title_widget, PANEL_VALUE_DISPLAY_WIDTH, title_width, XV_NULL);


	// Repaint panel.
	//
	panel_paint(panel, PANEL_CLEAR);
}

void
BOOKMARKEDIT::Dismiss()
{
	assert(objstate.IsReady());
	DbgFunc("BOOKMARKEDIT::Dismiss" << endl);

	xv_set(frame, XV_SHOW, FALSE, NULL);
}

void
BOOKMARKEDIT::ButtonEvent(Panel_item button, Event *)
{
	BOOKMARKEDIT	*bmedit;

	DbgFunc("BOOKMARKEDIT::ButtonEvent" << endl);

	bmedit = (BOOKMARKEDIT*) xv_get(button, PANEL_CLIENT_DATA);
	assert(bmedit != NULL);

	bmedit->SaveEvent();
}

// This event handler gets called when user explicitly takes out
// the pushpin.
//
void
BOOKMARKEDIT::FrameDoneProc(Frame frame)
{
	BOOKMARKEDIT	*bmedit;

	DbgFunc("BOOKMARKEDIT::FrameDoneProc" << endl);

	bmedit = (BOOKMARKEDIT *) xv_get(frame, XV_KEY_DATA, OBJ_KEY);
	assert(bmedit != NULL);

	bmedit->CancelEvent();
}

void
BOOKMARKEDIT::WinEventProc(Panel panel, Event *event)
{
	BOOKMARKEDIT	*bmedit;

	if (event_action(event) != WIN_RESIZE)
		return;

	DbgFunc("BOOKMARKEDIT::WinEventProc: resize event" << endl);
	EVENTLOG("resize bookmark edit window");

	bmedit = (BOOKMARKEDIT *) xv_get(panel, XV_KEY_DATA, OBJ_KEY);
	assert(bmedit != NULL);

	bmedit->ResizeEvent();
}

Panel_setting
BOOKMARKEDIT::TitleNotifyProc(Panel_item title, Event *)
{
	BOOKMARKEDIT	*bmedit = (BOOKMARKEDIT*) xv_get(title, PANEL_CLIENT_DATA);

	DbgFunc("BOOKMARKEDIT::TitleNotifyProc" << endl);
	assert(bmedit != NULL);

	// Advance cursor to comment editing window.
	//
	bmedit->editwin->XvSetFocus();

	return(PANEL_NONE);
}
