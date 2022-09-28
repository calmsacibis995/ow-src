#ident "@(#)comments.cc	1.7 06/11/93 Copyright 1992 Sun Microsystems, Inc."


#include "comments.h"
#include "inputwin.h"
#include <doc/notify.h>
#include <locale.h>


typedef enum {
	CATEGORY_EOU	= 0,
	CATEGORY_BUG	= 1,
	CATEGORY_DOC	= 2,
	CATEGORY_OTHER	= 3,
} CATEGORY;


typedef enum {
	COMMENTS_SEND_EVENT	= 14500,
	COMMENTS_RESIZE_EVENT	= 14501,
} COMMENTS_EVENT;


static const STRING	mail_cmd("/usr/lib/sendmail -t");
static const STRING	mail_to("docviewer-comments@East.Sun.COM");


static int
Max(int x, int y)
{
	return(x > y ? x : y);
}

void		WidgetEvent(Panel_item widget, Event *);
Notify_value	PanelEvent(Panel, Event *, Notify_arg, Notify_event_type);


COMMENTS::COMMENTS(Xv_opaque parent)
{
	int		margin_x, margin_y;
	int		category_x, category_y;
	int		subject_x, subject_y;
	int		cc_x, cc_y;
	int		text_x, text_y;
	int		button_x, button_y;
	int		min_width, min_height;
	STRING		bug_label, other_label, eou_label, doc_label;


	assert(parent != NULL);
	DbgFunc("COMMENTS::COMMENTS" << endl);
	objstate.MarkGettingReady();


	margin_x = 10;
	margin_y = 10;


	// Create popup frame; get popup's default panel.
	//
	frame = xv_create(parent, FRAME_CMD,
			FRAME_LABEL, gettext("Comments about the Navigator and Viewer"),
			FRAME_SHOW_RESIZE_CORNER,	TRUE,
			FRAME_SHOW_FOOTER,		TRUE,
			NULL);
	if (frame == NULL)
		OutOfMemory();

	panel = xv_get(frame, FRAME_CMD_PANEL);
	if (panel == NULL)
		OutOfMemory();

	xv_set(panel,	PANEL_LAYOUT,		PANEL_VERTICAL,
			WIN_CLIENT_DATA,	(caddr_t) this,
			XV_NULL);


	// Capture panel resize events.
	//
	notify_interpose_event_func(	panel,
					(Notify_func)PanelEvent,
					NOTIFY_IMMEDIATE);


	eou_label   = gettext("Ease of Use");
	bug_label   = gettext("Bug");
	doc_label   = gettext("Documentation");
	other_label = gettext("Other");
	category_x  = margin_x;
	category_y  = margin_y;
	category_toggle = xv_create(panel, PANEL_CHOICE,
			PANEL_LAYOUT,		PANEL_HORIZONTAL,
			XV_X,			category_x,
			XV_Y,			category_y,
			PANEL_LABEL_STRING,	gettext("Comment on:"),
			PANEL_CHOICE_STRING,	CATEGORY_EOU,   ~eou_label,
			PANEL_CHOICE_STRING,	CATEGORY_BUG,   ~bug_label,
			PANEL_CHOICE_STRING,	CATEGORY_DOC,   ~doc_label,
			PANEL_CHOICE_STRING,	CATEGORY_OTHER, ~other_label,
//XXX			XV_HELP_DATA,		COMMENTS_CATEGORY_TOGGLE_HELP,
			XV_NULL);


	// Create "Subject:" line (text widget).
	//
	subject_x = margin_x;
	subject_y = category_y + (int) xv_get(category_toggle, XV_HEIGHT) + 10;
	subject_line = xv_create(panel, PANEL_TEXT,
			XV_X,			subject_x,
			XV_Y,			subject_y,
			PANEL_LABEL_STRING,	gettext("Subject:"),
			PANEL_LABEL_BOLD,	TRUE,
			PANEL_VALUE_DISPLAY_LENGTH, 40,
			PANEL_VALUE_STORED_LENGTH, 80,
//XXX			XV_HELP_DATA,		COMMENTS_SUBJECT_LINE_HELP,
			XV_NULL);


	// Create "Cc:" line (text widget).
	//
	cc_x = margin_x;
	cc_y = subject_y + (int) xv_get(subject_line, XV_HEIGHT) + 10;
	cc_line = xv_create(panel, PANEL_TEXT,
			XV_X,			cc_x,
			XV_Y,			cc_y,
			PANEL_LABEL_STRING,	gettext("Cc:"),
			PANEL_LABEL_BOLD,	TRUE,
			PANEL_VALUE_DISPLAY_LENGTH, 40,
			PANEL_VALUE_STORED_LENGTH, 80,
//XXX			XV_HELP_DATA,		COMMENTS_CC_LINE_HELP,
			XV_NULL);


	// Line up "Comment", "Subject", and "Cc" widgets.
	//
	cc_x = Max((int) xv_get(category_toggle, PANEL_VALUE_X),
		   (int) xv_get(subject_line,    PANEL_VALUE_X));
	cc_x = Max((int) xv_get(cc_line,         PANEL_VALUE_X), cc_x);
	xv_set(category_toggle, PANEL_VALUE_X, cc_x, XV_NULL);
	xv_set(subject_line,    PANEL_VALUE_X, cc_x, XV_NULL);
	xv_set(cc_line,         PANEL_VALUE_X, cc_x, XV_NULL);


	// Create the comment text window.
	//
	text_x = margin_x;
	text_y = cc_y + (int) xv_get(cc_line, XV_HEIGHT) + 10;
	text_win = new INPUTWIN(frame, text_x, text_y, 7);
//XXX	text_win->XvHelpData(COMMENTS_TEXT_WINDOW_HELP);


	// Create the "Send Comment" button.
	//
	button_y = text_y + (int) xv_get(text_win->XvHandle(), XV_HEIGHT) + 10;
	send_button = xv_create(panel, PANEL_BUTTON,
			XV_Y,			button_y,
			PANEL_LABEL_STRING,	gettext("Send Comments"),
			PANEL_NOTIFY_PROC,	WidgetEvent,
			PANEL_CLIENT_DATA,	COMMENTS_SEND_EVENT,
//XXX			XV_HELP_DATA,		COMMENTS_SEND_BUTTON_HELP,
			XV_NULL);


	// Shrink panel and frame to fit the widgets.
	//
	window_fit(panel);
	window_fit(frame);


	// Set minimum frame size.
	//
	min_width  = (int) xv_get(category_toggle, XV_WIDTH)  +  20;
	min_height = (int) xv_get(panel, XV_HEIGHT) -
		     (int) xv_get(text_win->XvHandle(), XV_HEIGHT) / 2;
	SetMinFrameSize(frame, min_width, min_height);


	// Invoke Resizer to tidy up widget layout.
	//
	Resize();


	// Create message handler.
	//
	notify = new NOTIFY(frame);


	// We're ready to roll...
	//
	objstate.MarkReady();
}

COMMENTS:: ~COMMENTS()                     
{
	DbgFunc("COMMENTS::~COMMENTS" << endl);
}

// Display query history popup.
//
void
COMMENTS::Show()
{
	assert(objstate.IsReady());
	DbgFunc("COMMENTS::Show" << endl);
	EVENTLOG("navigator info");

	xv_set(frame, XV_SHOW, TRUE, XV_NULL);
	xv_set(text_win->XvHandle(), XV_SHOW, TRUE, XV_NULL);
}

STATUS
COMMENTS::SendComments()
{
	STRING	category;
	STRING	subject;
	STRING	cc;
	STRING	comments;
	STRING	mailfile;
	STRING	osinfo;
	char	osbuf[100];
	FILE	*mailfp;
	FILE	*unamefp;


	assert(objstate.IsReady());
	DbgFunc("COMMENTS::SendComments" << endl);


	subject  = (char *) xv_get(subject_line, PANEL_VALUE);
	cc       = (char *) xv_get(cc_line,      PANEL_VALUE);
	comments = text_win->GetText();

	subject  = STRING::CleanUp(subject);
	cc       = STRING::CleanUp(cc);
	comments = STRING::CleanUp(comments);


	// Validate input.
	//
	if (subject == NULL_STRING) {
		notify->Alert(gettext("Please fill in the \"Subject:\" line\nthen hit the \"Send Comments\" button again"));
		return(STATUS_FAILED);
	} else if (comments == NULL_STRING) {
		notify->Alert(gettext("Please type your comments in the window provided\nthen hit the \"Send Comments\" button again"));
		return(STATUS_FAILED);
	}


	switch ((int) xv_get(category_toggle, PANEL_VALUE)) {
	case CATEGORY_EOU:
		category = "EOU";
		break;
	case CATEGORY_BUG:
		category = "BUG";
		break;
	case CATEGORY_DOC:
		category = "DOC";
		break;
	case CATEGORY_OTHER:
		category = "OTHER";
		break;
	default:
		assert(0);
	}


	// Get important OS info.
	//
	if ((unamefp = popen("uname -a", "r"))  !=  NULL) {
		osinfo = fgets(osbuf, 99, unamefp);
		osinfo = STRING::CleanUp(osinfo);
		pclose(unamefp);
	}


	// Send the comments to the DocViewer folks using "sendmail".
	//
	if ((mailfp = popen(mail_cmd, "w"))  ==  NULL) {
		notify->Alert(gettext(
			"Sorry, unable to mail your comments.\n%s: %s"),
			~mail_cmd, SysErrMsg(errno));
		return(STATUS_FAILED);
	}

	fprintf(mailfp, "To: %s\n", ~mail_to);
	fprintf(mailfp, "Subject: Important DocViewer customer comments\n");
	if (cc != NULL_STRING)
		fprintf(mailfp, "Cc: %s\n", ~cc);
	fprintf(mailfp, "\n");
	fprintf(mailfp, "Navigator version: %s (%s)\n",
		~navigator_version, creation_date);
	if (osinfo != NULL_STRING)
		fprintf(mailfp, "OS version: %s\n", ~osinfo);
	fprintf(mailfp, "Comments: %s (%s)\n", ~subject, ~category);
	fprintf(mailfp, "%s\n", ~comments);
	pclose(mailfp);


	return(STATUS_OK);
}

void
COMMENTS::Resize()
{
	int	text_width, text_height;
	int	button_x, button_y;
	int	button_width, button_height;
	int	panel_width, panel_height;


	assert(objstate.IsReady());
	DbgFunc("COMMENTS::Resize" << endl);


	panel_width  = (int) xv_get(panel, XV_WIDTH);
	panel_height = (int) xv_get(panel, XV_HEIGHT);
	

	// Center the "Send Comments" button at bottom of panel.
	//
	button_width  = (int) xv_get(send_button, XV_WIDTH);
	button_height = (int) xv_get(send_button, XV_HEIGHT);
	button_x      = (panel_width - button_width) / 2;
	button_y      = panel_height - button_height - 10;
	xv_set(send_button, XV_X, button_x, XV_Y, button_y, XV_NULL);


	// Fit Comment window in between "Cc:" line and "Send Comments" button.
	// Make it as wide as the panel.
	//
	text_width  = panel_width - 20;
	text_width  = Max(text_width, 20);
	text_height = button_y - (int) xv_get(text_win->XvHandle(), XV_Y) - 20;
	text_height = Max(text_height, 20);
	xv_set(text_win->XvHandle(),
		XV_WIDTH,	text_width,
		XV_HEIGHT,	text_height,
		XV_NULL);
}

// Handle Comments events.
//
void
COMMENTS::EventHandler(int event, caddr_t /*event_obj*/)
{
	assert(objstate.IsReady());
	DbgFunc("COMMENTS::EventHandler" << endl);


	switch (event) {

	case COMMENTS_RESIZE_EVENT:
		Resize();
		break;

	case COMMENTS_SEND_EVENT:
		notify->Busy(gettext("Mailing your comments to the DocViewer developers . . ."));
		if (SendComments() != STATUS_OK) {
			// There was a problem sending the comments.
			// By setting button's PANEL_NOTIFY_STATUS to XV_ERROR,
			// we prevent XView from taking down the popup
			// so that the user can resolve the problem.
			//
			xv_set(send_button,
				PANEL_NOTIFY_STATUS, XV_ERROR,
				NULL);
		}
		notify->Done();
		break;

	default:
		assert(0);
		break;
	}
}

void
WidgetEvent(Panel_item widget, Event *)
{
	Panel		panel;
	COMMENTS	*comments;
	int		event;

	panel = (Panel) xv_get(widget, XV_OWNER);
	assert(panel != NULL);
	comments = (COMMENTS *) xv_get(panel, WIN_CLIENT_DATA);
	assert(comments != NULL);
	event = (int) xv_get(widget, PANEL_CLIENT_DATA);

	comments->EventHandler(event, NULL);
}

Notify_value
PanelEvent(Panel panel, Event *event, Notify_arg arg, Notify_event_type type)
{
	COMMENTS	*comments;
	

	if (event_action(event) != WIN_RESIZE) {
		return(notify_next_event_func(panel, (Notify_event)event, 
						arg, type));
	}


	DbgFunc("PanelEvent: resize" << endl);
	EVENTLOG("resize comments window");

	comments = (COMMENTS *) xv_get(panel, WIN_CLIENT_DATA);
	assert(comments != NULL);
		
	comments->EventHandler(COMMENTS_RESIZE_EVENT, NULL);

	return(NOTIFY_DONE);
}
