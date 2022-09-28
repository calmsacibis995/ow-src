#ident "@(#)printwin.cc	1.13 07/10/95 Copyright 1989 Sun Microsystems, Inc."

#include "printwin.h"
#include "winlist.h"
#include "ds_popup.h"
#include <doc/abgroup.h>
#include <doc/document.h>
#include <doc/notify.h>
#include <doc/pathname.h>
#include <doc/token_list.h>
#include <doc/utils.h>
#include <fcntl.h>
#include <ctype.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <locale.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>


typedef enum {
	DEST_PRINTER=0,
	DEST_FILE=1
} DESTINATION;


typedef enum {
	FIRST_PAGE_FIRST=0,
	LAST_PAGE_FIRST=1
} PAGE_ORDER;


extern void		NewHandler();


// PRINTWIN widget event handler.
//
static void		PrintWidgetEvent(Panel_item widget, Event *event);

// Handle input event in "Total Pages" widget.
//
static Panel_setting	CopyEvent(Panel_item, Event *);

static STATUS		CheckDestFile(const STRING &, NOTIFY *);
static Notify_value	Wait3Handler(	Notify_client,
					int,
					int *,
					struct rusage *);

static Notify_value	PanelEvent(	Panel			panel,
					Event			*event,
					Notify_arg		arg,
					Notify_event_type	type);


inline int
min(int a, int b)
{
	if (a < b)
		return a;
	else
		return b;
}

inline int
max(int a, int b)
{
	if (a > b)
		return a;
	else
		return b;
}

// PRINTWIN initializer.
//
STATUS
PRINTWIN::Init(const Xv_opaque	parent_frame,
	       ERRSTK		&err)
{
	int		x_margin, y_margin;
	char		*path;
	STRING		printdest,
			printer,
			file;
	STATUS		status;
	XSizeHints	hints;
	int		set_input_method = 1;


	objstate.MarkGettingReady();

	if ((status = SUBWIN::Init(parent_frame, set_input_method, err)) != STATUS_OK)
		return (status);

	xv_set(frame, FRAME_SHOW_RESIZE_CORNER, TRUE, NULL);


	// Create error message handler.
	//
	notify = new NOTIFY(frame);
	notify->Title(gettext("AnswerBook Viewer: Print"));
	notify->Mode(" ");	//XXX hack to make status msgs go away


	// Get panel from pop-up frame.
	//
	xv_set(panel,
		        PANEL_LAYOUT,		PANEL_VERTICAL,
			WIN_CLIENT_DATA,	(caddr_t) this,
			XV_HELP_DATA,		PRINT_HELP,
		        NULL);

	notify_interpose_event_func(	panel,
					(Notify_func)PanelEvent, 
					NOTIFY_IMMEDIATE);


	x_margin = 10;
	y_margin = 10;


	// Set up the scrolling list of things to select:
	//
	choices_list = new WINLIST(panel, x_margin, y_margin);

	// Set the number of rows to 2 initially. Because of the widgets that
	// overlay each other when the base frames size is determined it would
	// be too big, and when the number of rows is calcualted, there would
	// be too many. By setting it to 2 we end up with 6.
	//
	choices_list->SetViewNumRows(2);
	xv_set(choices_list->XvHandle(),
			PANEL_LAYOUT,		PANEL_VERTICAL,
			XV_HELP_DATA,		PRINT_LIST_HELP,
			XV_SHOW,		TRUE,
			PANEL_LABEL_STRING,	gettext("Print Choices:"),
			NULL);

	// Register event handler for selection list events.
	//
	choices_list->SetEventHandler(PRINTWIN::DispatchEvent, (caddr_t)this);


	// Create "Number of Copies" widget.
	//
	copies_widget = xv_create(panel, PANEL_NUMERIC_TEXT,
		        PANEL_LAYOUT,		PANEL_HORIZONTAL,
		        PANEL_VALUE_DISPLAY_LENGTH, 4,
		        PANEL_VALUE_STORED_LENGTH,  4,
		        PANEL_MIN_VALUE,	1,
		        PANEL_MAX_VALUE,	50,
		        PANEL_VALUE,		1,
		        PANEL_LABEL_STRING,	gettext("Copies:"),
			PANEL_CLIENT_DATA,	PRINT_COPIES_EVENT,
			PANEL_NOTIFY_LEVEL,	PANEL_ALL,
			PANEL_NOTIFY_PROC,	PrintWidgetEvent,
			XV_HELP_DATA,		PRINT_COPIES_HELP,
		        NULL);

	// Create "Total pages" widget.
	//
	totals_widget = xv_create(panel, PANEL_TEXT,
		        PANEL_LAYOUT,		PANEL_HORIZONTAL,
		        PANEL_VALUE_DISPLAY_LENGTH, 5,
		        PANEL_VALUE_STORED_LENGTH, 5,
		        PANEL_READ_ONLY,	TRUE,
			PANEL_VALUE_UNDERLINED,	FALSE,
		        PANEL_LABEL_STRING,   gettext("Total Pages to Print:"),
			XV_HELP_DATA,		PRINT_TOTAL_PAGES_HELP,
		        NULL);


	printdest = gettext("Destination:");
	printer   = gettext("Printer");
	file      = gettext("File");
	dest_widget = xv_create(panel, PANEL_CHOICE,
		        PANEL_LAYOUT,		PANEL_HORIZONTAL,
		        PANEL_CHOICE_NROWS,	1,
		        PANEL_LABEL_STRING,	~printdest,
		 	PANEL_CHOICE_STRINGS,
				~printer,
				~file,
				NULL,
			PANEL_CLIENT_DATA,	(caddr_t)this,
			PANEL_VALUE,		DEST_PRINTER,
			PANEL_CLIENT_DATA,	PRINT_DESTINATION_EVENT,
		        PANEL_NOTIFY_PROC,	PrintWidgetEvent,
			XV_HELP_DATA,		PRINT_DESTINATION_HELP,
		        NULL);

	// If Helios, turn off above option

	if (getenv ("HELIOS_STARTED") != (char *) NULL)
		xv_set (dest_widget, PANEL_INACTIVE, TRUE, NULL);

	// The following widgets are only displayed when we're
	// printing to a printer.
	// There are other widgets that appear only when we're
	// printing to a file.
	//
	printer_menu = xv_create(panel, PANEL_CHOICE_STACK,
			PANEL_LAYOUT,		PANEL_HORIZONTAL,
			PANEL_LABEL_STRING,	gettext("Printer:"),
			XV_HELP_DATA,		PRINT_PRINTER_NAME_HELP,
			NULL);
	
	print_order = xv_create(panel, PANEL_CHOICE,
			PANEL_LAYOUT,		PANEL_HORIZONTAL,
			PANEL_CHOICE_NROWS,	1,
			PANEL_LABEL_STRING,	gettext("Print Order:"),
			PANEL_CHOICE_STRINGS,
				gettext("First Page First"),
				gettext("Last Page First"),
				NULL,
			XV_HELP_DATA,		PRINT_PAGE_ORDER_HELP,
			NULL);


	// "Print to file" widgets.
	//
	if ((path = getenv("HOME"))  ==  NULL)
		path = "/tmp";
	path_widget = xv_create(panel, PANEL_TEXT,
			PANEL_LAYOUT,		PANEL_HORIZONTAL,
			PANEL_VALUE_DISPLAY_LENGTH,	30,
			PANEL_VALUE_STORED_LENGTH,	80,
			PANEL_VALUE,		path,
			PANEL_READ_ONLY,	FALSE,
			PANEL_LABEL_STRING,	gettext("Directory:"),
			XV_HELP_DATA,		PRINT_PATHNAME_HELP,
			NULL);

	// layer 2
	file_widget = xv_create(panel, PANEL_TEXT,
			PANEL_LAYOUT,		PANEL_HORIZONTAL,
			PANEL_VALUE_DISPLAY_LENGTH,	30,
			PANEL_VALUE_STORED_LENGTH,	80,
			PANEL_READ_ONLY,	FALSE,
			PANEL_LABEL_STRING,	gettext("File:"),
			XV_HELP_DATA,		PRINT_FILENAME_HELP,
			NULL);


	// "Print" button.
	//
	print_button  = (Panel_item)xv_create(panel, PANEL_BUTTON,
			PANEL_LABEL_STRING,	gettext("Print"),
			PANEL_NOTIFY_PROC,	PrintWidgetEvent,
			PANEL_CLIENT_DATA,	PRINT_BUTTON_EVENT,
			XV_HELP_DATA,		PRINT_BUTTON_HELP,
			NULL);


	// Fit the panel and frame to the widgets,
	// set the minimum frame size, then arrange the widgets
	// properly on the panel.
	//
	window_fit(panel);
	window_fit(frame);

	// Set minimum frame size for resizing.
	//
	hints.flags      = PMinSize;
	hints.min_width  = (int)xv_get(frame, XV_WIDTH);
	hints.min_height = (int)xv_get(frame, XV_HEIGHT);
	XSetWMNormalHints(	(Display *) xv_get(frame, XV_DISPLAY),
				(Window)    xv_get(frame, XV_XID),
				&hints);

	UpdatePrintMode();
	Resize();
	ds_position_popup(parent_frame, frame, DS_POPUP_LOR);


	// We're ready to roll...
	//
	objstate.MarkReady();

	return (STATUS_OK);
}

// PRINTWIN destructor.
//
PRINTWIN::~PRINTWIN()
{
	DbgFunc("PRINTWIN::PRINTWIN" << endl);


	if (choices_list != NULL)
		delete(choices_list);

	if (notify != NULL)
		delete(notify);
}

// Show the FRAME_CMD popup
//
STATUS
PRINTWIN::PrintDocument(ABGROUP		*abgroup_arg,
		        const DOCNAME	&docname,
			const int	currViewPage,
			ERRSTK		&err)
{
	DOCUMENT	*doc;
	STRING		title;
	STRING		indent;
	int		last;
	int		i;
	static int	print_list_created = FALSE;


	assert(objstate.IsReady());
	abgroup = abgroup_arg;
	assert(abgroup != NULL);
	assert(notify != NULL);
	DbgFunc("PRINTWIN::PrintDocument: " << docname << endl);


	// Clear lists.
	//
	choices_list->Clear();
	doc_list.Clear();


	// Look up document...
	//
	if ((doc = abgroup->LookUpDoc(docname, 0, err))  ==  NULL)
		return(STATUS_FAILED);

	assert(doc->Range().IsValid());

	// Save page number of currently viewed page
	//
	viewPage = currViewPage;


	// ... and each of its ancestors in turn.
	//
	for ( ; doc != NULL; doc = doc->GetParent(err)) {
		doc_list.Insert(doc, 0);
	}
	

	// Delete docs in the list that don't have titles.
	//
	i = 0;
	while (i < doc_list.Count()) {
		if (doc_list[i]->Title() == NULL_STRING  ||
		    doc_list[i]->Range().IsValid() == BOOL_FALSE)
			doc_list.Delete(i--);
		i++;
	}


	// Add document and its ancestors to the display list.
	//
	for (i = 0; i < doc_list.Count(); i++) {

		title = indent + doc_list[i]->Title();
		if (doc_list[i]->Label() != NULL_STRING)
			title += " (" + doc_list[i]->Label() + ")";
		choices_list->InsertEntry(i, title);

		indent += "     ";
	}

	// Last entry should always be "Current Viewer Page"
	//
	title = indent + gettext("Current Viewer Page");
	choices_list->InsertEntry(i, title);


	last = choices_list->NumEntries() - 1;
	choices_list->SetSelection(last);
	DocSelectEvent();


	// Show print window.
	//
	xv_set(frame, XV_SHOW, TRUE, NULL);

	if (print_list_created == FALSE) {

	   // Create printer list.
	   //
	   notify->Busy(gettext("Making list of available printers..."));
	   InitPrinterList();
	   notify->Done();
	   print_list_created = TRUE;
	   }


	return(STATUS_OK);
}

// Dispatch events to the event handler.
//
void
PRINTWIN::DispatchEvent(int event, caddr_t event_obj, caddr_t client_data)
{
	PRINTWIN	*pw = (PRINTWIN *)client_data;

	assert(pw != NULL);
	pw->EventHandler(event, event_obj);
}

// Handle Query Props events.
//
void
PRINTWIN::EventHandler(int event, caddr_t /*event_obj*/)
{
	assert(objstate.IsReady());
	DbgFunc("PRINTWIN::EventHandler: " << event << endl);


	switch (event) {

	case PRINT_DESTINATION_EVENT:
		UpdatePrintMode();
		break;

	case PRINT_BUTTON_EVENT:
		DoPrint();
		break;

	case PRINT_DONE_EVENT:
		notify->Done();
		break;

	case PRINT_FAILED_EVENT:
		notify->Done();
		notify->Alert(gettext("Can't print document"));
		break;

	case PRINT_RESIZE_EVENT:
		Resize();
		break;

	case WINLIST_SELECT_EVENT:
		DocSelectEvent();
		break;

	case WINLIST_EXECUTE_EVENT:
		// ignore - we're not interested
		break;

	case PRINT_COPIES_EVENT:
		UpdateTotalPages();
		break;

	default:
		break;
	}
}

// Notify callback function for Print Window widgets.
//
void
PrintWidgetEvent(Panel_item widget, Event *)
{
	Xv_opaque	panel;
	caddr_t		printwin;
	int		event;

	DbgFunc("PrintWidgetEvent" << endl);

	event    = (int)       xv_get(widget, PANEL_CLIENT_DATA);
	panel    = (Xv_opaque) xv_get(widget, XV_OWNER);
	printwin = (caddr_t)   xv_get(panel,  WIN_CLIENT_DATA);

	PRINTWIN::DispatchEvent(event, NULL, printwin);
}

// Pass info to viewprint utility to print the selection
//
void
PRINTWIN::DoPrint()
{
	STRING		docname;	// object name of doc we're printing
	STRING		printer;
	STRING		directory;	// value of "Path:" field
	STRING		file;		// value of "File:" field
	STRING		fullpath;	// directory + file
	STRING		copies;		// number of copies to print
	STRING		cclist;		// list of card catalogs to search
	const char	*argv[16];	// viewprint argument list
	int		argc=0;		// arg list count
	char		buf[99];
	DESTINATION	dest;
	PAGE_ORDER	order;
	pid_t		pid;		// process id of forked "viewprint"
	int		n;


	assert(objstate.IsReady());
	assert(abgroup != NULL);
	DbgFunc("PRINTWIN::DoPrint" << endl);


	// If we're printing lots of pages, make sure the user
	// really wants to do this.
	//
	if (CheckNumPages() != STATUS_OK)
		return;


	// Create argument list for print utility 'viewprint'.
	// viewprint's command line looks like:
	//	viewprint -c<card_catalogs> -P<printer> [-#<copies>] \
	//		[-p<page range>] [-R] SectionToPrint
	// or
	//	viewprint -c<card_catalogs> -f<destfile> [-#<copies>] \
	//		[-p<page range>] [-R] SectionToPrint
	//


	// The initial program name/cmd.
	//
	argv[argc++] = "viewprint";


	// Get #copies user wants to print.
	//
	argv[argc++] = "-#";
	sprintf(buf, "%d", (int) xv_get(copies_widget, PANEL_VALUE));
	copies = buf;
	argv[argc++] = copies;

	
	// Specify list of card catalogs to use in finding Bookshelf
	// containing the document to print.
	//
	abgroup->GetCardCatalogs().GetPaths(cclist);
	if (cclist == NULL_STRING) {
		assert(0);
	} else {
		argv[argc++] = "-c";
		argv[argc++] = cclist;
	}


	// Print to printer or file?
	//
	dest = (DESTINATION) xv_get(dest_widget, PANEL_VALUE);
	if (dest == DEST_PRINTER) { // valid for printer and order

		// Verify there are some printers.
		//
		if (printer_list.IsEmpty()) {

			notify->Alert(gettext(
			   "No printers (contact your System Administrator)"));

			// By setting button's PANEL_NOTIFY_STATUS to XV_ERROR,
			// we prevent XView from taking down the print window
			// so that the user can resolve the problem.
			//
			(void) xv_set(print_button,
					PANEL_NOTIFY_STATUS,	XV_ERROR,
					NULL);
			return;
		}

		// Send output to printer.
		// Get printer choice, page order.
		//
		argv[argc++] = "-P";

		int printer = (int) xv_get(printer_menu, PANEL_VALUE);
		argv[argc++] = printer_list[printer];

		order = (PAGE_ORDER) xv_get(print_order, PANEL_VALUE);
		if (order == LAST_PAGE_FIRST)	// print last page first
			argv[argc++] = "-R";

	} else {

		// Send output to file.
		// Get path name.
		//
		directory = (char *) xv_get(path_widget, PANEL_VALUE);
		file      = (char *) xv_get(file_widget, PANEL_VALUE);

		// Make sure directory is terminated by '/'.
		//
		if (directory != NULL_STRING	&&
		    directory != ""		&&
		    directory[directory.Length()-1] != '/')
			directory += "/";
		fullpath = directory + file;
//XXX		fullpath = GetAbsolutePath(fullpath);
		STRING tmpstr(fullpath);
		PATHNAME::Expand(tmpstr, fullpath);

		// Verify output file is accessible.
		//
		if (CheckDestFile(fullpath, notify)  !=  STATUS_OK) {

			// By setting button's PANEL_NOTIFY_STATUS to XV_ERROR,
			// we prevent XView from taking down the print window
			// so that the user can resolve the problem.
			//
			(void) xv_set(print_button,
					PANEL_NOTIFY_STATUS,	XV_ERROR,
					NULL);
			return;
		}


		argv[argc++] = "-f";
		argv[argc++] = fullpath;
	}


	// Get document selection.
	//
	n = choices_list->GetSelection();
	assert(n >= 0);

	// Last selection is always "Current Viewer Page".  If selected,
	// set argument for page to print.
	//
	if (n == (choices_list->NumEntries() - 1)) {
		n--;
		argv[argc++] = "-p";
		sprintf(buf, "%d", viewPage);
		argv[argc++] = buf;
	}

	// Document name.
	//
	docname = doc_list[n]->Name().NameToString();
	argv[argc++] = docname;

	
	// Terminate arg list.
	//
	argv[argc] = NULL;
	

	DbgMed("PRINTWIN::DoPrint: viewprint ");
	for (argc = 0; argv[argc] != NULL; argc++) {
		DbgMed(" " << argv[argc]);
	}
	DbgMed(endl);


	// Set the frame to busy, unset in wait3 routine.
	//
	notify->Busy(gettext("Printing document . . ."));


	// Invoke viewprint.
	//
	switch (pid = fork()) {
	case -1:
		// Fork failed...
		//
		notify->Done();
		notify->Alert(gettext("Can't print document: %s"),
			SysErrMsg(errno));
		return;
	case 0:				// this is the child of the fork
		execvp(argv[0], (char * const *)argv);
		// execlp never returns, so if we get here, SCREAM LOUDLY
		_exit(2);		// just to make sure it be dead
		break;
	default:
		notify_set_wait3_func(	(Notify_client)this,
					(Notify_func)Wait3Handler,
					(int)pid);
		break;
	}
}

STATUS
PRINTWIN::CheckNumPages()
{
	int		num_copies;
	int		total_pages;	// total pages to print
	BOOL		continue_print;
	STRING		yes_string;
	STRING		no_string;
	int		n;


	assert(objstate.IsReady());
	DbgFunc("PRINTWIN::CheckNumPages" << endl);


	num_copies =  (int) xv_get(copies_widget, PANEL_VALUE);
	n = choices_list->GetSelection();
	if (n == (choices_list->NumEntries() - 1))
		total_pages = num_copies;
	else
		total_pages = num_copies * doc_list[n]->Range().NumPages();


	if (total_pages <= 25)
		return(STATUS_OK);


	yes_string = gettext("Yes");
	no_string = gettext("No");

	continue_print = notify->AlertPrompt(	~yes_string,
						~no_string,
				gettext("Printing %d pages - continue?"),
						total_pages);

	if ( ! continue_print) {
		// By setting button's PANEL_NOTIFY_STATUS to XV_ERROR,
		// we prevent XView from taking down the print window
		// so that the user can resolve the problem.
		//
		xv_set(print_button, PANEL_NOTIFY_STATUS, XV_ERROR, NULL);
		notify->Warning(gettext("Print request cancelled"));
		return(STATUS_FAILED);
	}

	return(STATUS_OK);
}

void
PRINTWIN::UpdatePrintMode()
{
	assert(objstate.IsReady());
	DbgFunc("PRINTWIN::UpdatePrintMode" << endl);


	switch ((int) xv_get(dest_widget, PANEL_VALUE)) {

	case DEST_PRINTER:
		xv_set(copies_widget, PANEL_INACTIVE, FALSE, NULL);
		xv_set(path_widget,  XV_SHOW, FALSE, NULL);
		xv_set(file_widget,  XV_SHOW, FALSE, NULL);
		xv_set(printer_menu, XV_SHOW, TRUE,  NULL);
		xv_set(print_order,  XV_SHOW, TRUE,  NULL);
		break;

	case DEST_FILE:
		xv_set(copies_widget, PANEL_INACTIVE, TRUE, NULL);
		xv_set(printer_menu, XV_SHOW, FALSE, NULL);
		xv_set(print_order,  XV_SHOW, FALSE, NULL);
		xv_set(path_widget,  XV_SHOW, TRUE,  NULL);
		xv_set(file_widget,  XV_SHOW, TRUE,  NULL);
		break;

	default:
		assert(0);
		break;
	}

	UpdateTotalPages();
}

void
PRINTWIN::Resize()
{
	int	panel_width;
	int	panel_height;
	int	button_x, button_y;
	int	order_x, order_y;
	int	printer_x, printer_y;
	int	copies_x, copies_y;
	int	dest_x, dest_y;
	int	choices_width;
	int	choices_height;
	int	total_x, total_y;
	int	path_x, path_y;
	int	file_x, file_y;
	int	margin_x, margin_y;


	assert(objstate.IsReady());
	assert(panel != NULL);
	DbgFunc("PRINTWIN::Resize" << endl);


	panel_width  = (int) xv_get(panel, XV_WIDTH);
	panel_height = (int) xv_get(panel, XV_HEIGHT);

	margin_x     = 10;
	margin_y     = 10;


	// Center "Print" button at bottom of panel.
	//
	button_x  = panel_width / 2;
	button_x -= (int) xv_get(print_button, XV_WIDTH) / 2;
	button_y  = panel_height - 10;
	button_y -= (int) xv_get(print_button, XV_HEIGHT);
	xv_set(print_button, XV_X, button_x, XV_Y, button_y, XV_NULL);


	// Position "Print Order" widget 20 pixels above "Print" button.
	//
	order_x  = margin_x;
	order_y  = button_y - 20;
	order_y -= (int) xv_get(print_order, XV_HEIGHT);
	xv_set(print_order, XV_X, order_x, XV_Y, order_y, XV_NULL);


	// Position "File" widget coincident with the "Print Order" widget
	//
	file_x = (int) xv_get(print_order, PANEL_LABEL_X);
	file_y = (int) xv_get(print_order, PANEL_LABEL_Y);
	xv_set(file_widget, PANEL_LABEL_X, file_x, PANEL_LABEL_Y, file_y, 0);


	// Position "Printers" menu 10 pixels above "Print Order" widget.
	//
	printer_x  = margin_x;
	printer_y  = order_y - 10;
	printer_y -= (int) xv_get(printer_menu, XV_HEIGHT);
	xv_set(printer_menu, XV_X, printer_x, XV_Y, printer_y, XV_NULL);


	// Position "Directory" widget coincident with the "Printers" menu.
	//
	path_x = (int) xv_get(printer_menu, PANEL_LABEL_X);
	path_y = (int) xv_get(printer_menu, PANEL_LABEL_Y);
	xv_set(path_widget, PANEL_LABEL_X, path_x, PANEL_LABEL_Y, path_y, 0);


	// Position "Print to:" widget 10 pixels above "Printer" menu.
	//
	dest_x  = margin_x;
	dest_y  = printer_y - 10;
	dest_y -= (int) xv_get(dest_widget, XV_HEIGHT);
	xv_set(dest_widget, XV_X, dest_x, XV_Y, dest_y, XV_NULL);


	// Position "Copies" widget 10 pixels above "Print to:" menu.
	//
	copies_x  = margin_x;
	copies_y  = dest_y - 10;
	copies_y -= (int) xv_get(copies_widget, XV_HEIGHT);
	xv_set(copies_widget, XV_X, copies_x, XV_Y, copies_y, XV_NULL);


	// Position "Print Choices" list 10 pixels above "Copies" widget
	// and make it as wide as the panel.
	//
	choices_width  = panel_width - margin_x;
	choices_height = copies_y - margin_y - 20;
	choices_list->Fit(choices_width, choices_height);


	// Position "Total Pages" widget to the right of the "Copies" widget.
	//
	total_x  = copies_x + 10;
	total_x += (int) xv_get(copies_widget, XV_WIDTH);
	total_y  = copies_y;
	xv_set(totals_widget, XV_X, total_x, XV_Y, total_y, 0);


	// We must redraw widgets here or else the panel will be a mess...
	//
	panel_paint(panel, PANEL_CLEAR);
}

STATUS
CheckDestFile(const STRING &file, NOTIFY *notify)
{
	STRING	alert_msg;
	BOOL	cancel;
	STRING	cancel_msg;
	int	exist_fd;
	int	fd;
	STRING	overwrite_msg;


	// Make sure file name is non-null.
	//
	if (file == NULL_STRING  ||  file == "") {
		notify->Alert(gettext("Please specify the directory and file to which to print."));
		return(STATUS_FAILED);
	}

	exist_fd = open(file, O_RDWR|O_CREAT|O_EXCL, 0644);
	close( exist_fd );

	// Check access permissions.
	//
	if ((fd = open(file, O_RDWR|O_CREAT, 0644))  <  0) {

		switch (errno) {

		case EISDIR:
			notify->Alert(gettext(
"\"%s\" is a directory.\nPlease specify the name of a file within that directory."),
				~file);
			break;
		case ENOENT:
			notify->Alert(gettext(
"Non-existent directory path for\n\"%s\"\nPlease verify that the directory exists, or specify another directory."),
				~file);
			break;
		case EACCES:
			notify->Alert(gettext(
"Permission denied for file\n\"%s\"\nPlease check permissions or specify another destination."),
				~file);
			break;
		default:
			notify->Alert(gettext(
				"Unable to print to file\n\"%s\"\n%s."),
				~file, strerror (errno));
			break;
		}

		return(STATUS_FAILED);
	}

	close(fd);

	if (exist_fd < 0) {
		cancel_msg = gettext("Cancel");
		overwrite_msg = gettext("Overwrite");
		alert_msg = gettext("File \"%s\" already exists.");
		cancel = notify->AlertPrompt(cancel_msg, overwrite_msg,
					     alert_msg, ~file);

		if ( cancel ) {
			return(STATUS_FAILED);
		}
		else {
			return(STATUS_OK);
		}
	}
	else {
		return(STATUS_OK);
	}
}

// Wait3Handler - Handle the event; let the method do the work.
//
Notify_value
Wait3Handler(Notify_client client_data, int, int *status, struct rusage *)
{
	PRINTWIN_EVENT	event;

	if (WIFEXITED(*status)  &&  WEXITSTATUS(*status) != 0) {
		event = PRINT_FAILED_EVENT;
	} else {
		event = PRINT_DONE_EVENT;
	}

	PRINTWIN::DispatchEvent(event, NULL, (caddr_t)client_data);

	return(NOTIFY_DONE);
}

// Update "Total Pages", "# Copies" widgets.
//
void
PRINTWIN::DocSelectEvent()
{
	assert(objstate.IsReady());
	DbgFunc("PRINTWIN::DocSelectEvent" << endl);

	UpdateCopies();
	UpdateTotalPages();
}

// Update "Total Pages" widget.
//
void
PRINTWIN::UpdateTotalPages()
{
	DESTINATION	dest;
	int	total_pages;
	char	total_buf[10];
	int	num_copies;
	int	n;

	assert(objstate.IsReady());
	DbgFunc("PRINTWIN::UpdateTotalPages" << endl);

	n = choices_list->GetSelection();

	// Has choice been made yet
	if (n >= 0) {

		// Print to printer or file?
		//
		dest = (DESTINATION) xv_get(dest_widget, PANEL_VALUE);

		if (dest == DEST_PRINTER) { // valid for printer and order
			num_copies =  (int) xv_get(copies_widget, PANEL_VALUE);
		}
		else {
			num_copies = 1;
		}

		if (n == (choices_list->NumEntries() - 1))
			total_pages = num_copies;
		else
			total_pages = num_copies * doc_list[n]->Range().NumPages();
		sprintf(total_buf, "%d", total_pages);
		xv_set(totals_widget, PANEL_VALUE, total_buf, NULL);
	}
}

// Update "# Copies" widget.
//
void
PRINTWIN::UpdateCopies()
{
	int	num_copies;
	int	min_copies, max_copies;

	assert(objstate.IsReady());
	DbgFunc("PRINTWIN::UpdateCopies" << endl);

	// Get number of copies user wants to print.
	// Make sure they're not exceeding the maximum number.
	//
	num_copies = (int) xv_get(copies_widget, PANEL_VALUE);
	min_copies = (int) xv_get(copies_widget, PANEL_MIN_VALUE);
	max_copies = (int) xv_get(copies_widget, PANEL_MAX_VALUE);
	num_copies = max(num_copies, min_copies);
	num_copies = min(num_copies, max_copies);
	xv_set(copies_widget, PANEL_VALUE, num_copies, NULL);
}

// CopyEvent - update the 'total pages to print' field
//
Panel_setting
CopyEvent(Panel_item copies_widget, Event *event)
{
	caddr_t		printwin;
	int		num_copies;
	int		min_copies, max_copies;
	Panel		panel;
	Panel_setting	setting;


	// "Down" events appear to be extraneous, so we'll ignore them.
	//
	if (event_is_down(event))
		return(PANEL_INSERT);


	num_copies = (int) xv_get(copies_widget, PANEL_VALUE);
	min_copies = (int) xv_get(copies_widget, PANEL_MIN_VALUE);
	max_copies = (int) xv_get(copies_widget, PANEL_MAX_VALUE);


	if ( ! event_is_ascii(event)) {

		// User clicked increment/decrement button.
		//
		setting = PANEL_INSERT;

	} else if (num_copies >= min_copies  &&  num_copies <= max_copies) {

		// User typed an ascii digit, current value is within range.
		//
		setting = PANEL_INSERT;

	} else {

		// User typed an ascii digit, current value is out of range.
		// Adjust current value to within range, and throw away
		// the digit.
		//
		if (num_copies > max_copies)
			num_copies = max_copies;
		else
			num_copies = min_copies;
		xv_set(copies_widget,       PANEL_VALUE, num_copies, NULL);
		setting = PANEL_NONE;
	}


	panel    = (Xv_opaque) xv_get(copies_widget, XV_OWNER);
	printwin = (caddr_t)   xv_get(panel, WIN_CLIENT_DATA);
	PRINTWIN::DispatchEvent(PRINT_COPIES_EVENT, NULL, printwin);

	return(setting);
}

void
PRINTWIN::InitPrinterList()
{ 	
	FILE		*lpstatfp;
	char		buf[BUFSIZ];
	STRING		printer;
	STRING		lp_printer;
	STRING		env_printer;
	TOKEN_LIST	default_msg(':');
	TOKEN_LIST	list_msg(' ');
	int		lp_printer_index = 0;
	int		env_printer_index = 0;
	ERRSTK		err;
	int		i;
	int             index;
	

	assert(objstate.IsReady());
	DbgFunc("PRINTWIN::InitPrinterList" << endl);


	printer_list.Clear();
//XXX	printer_menu.Clear();	//XXXXXXXXXXXXXXXXXX


	// Get the printer specified in the environment (if any).
	//
	env_printer = getenv("PRINTER");

	

	// Ask "lpstat" about the system's default printer (if any).
	// The output of "lpstat -d" looks like this:
	//
	//	"system default destination: <print_name>"
	//
	// NOTE: the "LC_ALL=C; export LC_ALL" stuff is needed to insure
	// we get the output in English.
	//
	if ((lpstatfp = popen("LC_ALL=C; export LC_ALL; lpstat -d", "r"))  !=  NULL) {

		if (fgets(buf, BUFSIZ, lpstatfp)  !=  NULL) {
			default_msg = buf;
			if (default_msg.Count() == 2)
				lp_printer = default_msg[1];
		}

		pclose(lpstatfp);
	}
	

	// Ask "lpstat" for a list of available printers.
	// We assume the format of lpstat output is:
	//
	// system for foo: <printer server name>
        // system for bar: <printer server name>
	//
	//  or if there are local printers
	// then the format is:
	//
	// device for foo: <printer device path>
	//
	// 
	//
	// NOTE: the "LC_ALL=C; export LC_ALL" stuff is needed to insure
	// we get the output in English.
	//
	if ((lpstatfp = popen("LC_ALL=C; export LC_ALL; lpstat -v all", "r"))  ==  NULL) {
		err.Init(gettext("The \"lpstat\" command failed: %s"),
			SysErrMsg(errno));
		err.Push(gettext("Can't create list of available printers"));
		notify->ShowErrs(err);
	} else {

		while (fgets(buf, BUFSIZ, lpstatfp)  !=  NULL) {
			list_msg = buf;
			if (list_msg.Count() >= 3 && 
			(list_msg[0] == "system" || list_msg[0] == "device")){
			  index = list_msg[2].Index(':');
			  printer = list_msg[2].SubString(0, index-1);
			  printer_list.Add(printer);
			  }
		}
		pclose(lpstatfp);
	}


	// Decide on default printer.
	//
	if (env_printer != NULL_STRING)
		default_printer = env_printer;
	else if (lp_printer != NULL_STRING)
		default_printer = lp_printer;
	else
		default_printer = NULL_STRING;


	// Initialize Printer menu.
	//

	for (i = 0; i < printer_list.Count(); i++) {

		xv_set(printer_menu,
			PANEL_CHOICE_STRING,	i, ~printer_list[i],
			NULL);

		if (printer_list[i] == default_printer) {
			xv_set(printer_menu,
				PANEL_DEFAULT_VALUE,	i,
				PANEL_VALUE,		i,
				NULL);
		}
	}

	if (printer_list.Count() == 0) {
		xv_set(printer_menu,
			PANEL_CHOICE_STRING,	0, gettext("(No Printers)"),
//XXX			PANEL_INACTIVE,		TRUE,
			XV_NULL);
	}
}

Notify_value
PanelEvent(Panel panel, Event *event, Notify_arg arg, 
		   Notify_event_type type)
{
	caddr_t	printwin;

	if (event_action(event) != WIN_RESIZE) {

		return(notify_next_event_func(	panel,
						(Notify_event) event, 
						arg,
						type));
	}

	DbgFunc("PRINTWIN::PanelEvent: resize" << endl);

	printwin = (caddr_t) xv_get(panel, WIN_CLIENT_DATA);
	PRINTWIN::DispatchEvent(PRINT_RESIZE_EVENT, NULL, printwin);

	return(NOTIFY_DONE);
}
