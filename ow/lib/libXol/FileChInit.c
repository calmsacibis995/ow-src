#pragma ident	"@(#)FileChInit.c	1.16    97/03/26 lib/libXol SMI"    /* OLIT	493	*/

/*
 *	Copyright (C) 1992  Sun Microsystems, Inc
 *			All rights reserved.
 *		Notice of copyright on this source code
 *		product does not indicate publication.
 *
 * RESTRICTED RIGHTS LEGEND: Use, duplication, or disclosure by
 * the U.S. Government is subject to restrictions as set forth
 * in subparagraph (c)(1)(ii) of the Rights in Technical Data
 * and Computer Software Clause at DFARS 252.227-7013 (Oct. 1988)
 * and FAR 52.227-19 (c) (June 1987).
 *
 *	Sun Microsystems, Inc., 2550 Garcia Avenue,
 *	Mountain View, California 94043.
 *
 */

/**************************************************************************
 *
 *	Initializetion Of The File Chooser Panel Widget
 *		
 **************************************************************************/


/**************************************************************************
 *
 *      Imported interfaces 
 *
 **************************************************************************/

#ifndef	DEBUG
#define	NDEBUG
#endif

#include <assert.h>		/* assert() */
#include <libintl.h>		/* dgettext() */
#include <string.h>
#include <sys/types.h>  	/* boolean_t */
#include <unistd.h>		/* geteuid(), getegid() */

#include <X11/Xlib.h>		/* XCreateBitmapFromData(), */
				/* XGetImage(), XFreePixmap(), */
				/* RootWindowOfScreen() */

#include <X11/IntrinsicP.h>
#include <X11/StringDefs.h>

#include <Xol/Datum.h>
#include <Xol/OlgxP.h>		/* OlgxGetValidScale() */
#include <Xol/OpenLookP.h>
#include <Xol/diags.h>		/* OlGetShell() */
#include <Xol/filenav.h>

/* Classes of components */
#include <Xol/Caption.h>
#include <Xol/FileChImpl.h>
#include <Xol/FileChP.h>
#include <Xol/Form.h>
#include <Xol/MenuButton.h>
#include <Xol/OblongButt.h>
#include <Xol/Ring.h>
#include <Xol/RubberTile.h>
#include <Xol/ScrollingP.h>
#include <Xol/StaticText.h>
#include <Xol/TextLine.h>

/* Glyphs */
#include <Xol/bitmaps/fcfolder.xbm>
#include <Xol/bitmaps/fcdoc.xbm>
#include <Xol/bitmaps/fcgoup.xbm>

#include <Xol/FileChInit.h>		/* interface of this implementation */


/**************************************************************************
 *
 *	Module Private Manifest Constants And Macros Definitions
 *
 **************************************************************************/

#define	NUM_PARAMETERS		(6)	/* must the number of items below */
#define	A			(0)	/* keyed to the GUI specification */
#define	B			(1)
#define	C			(2)
#define	D			(3)
#define	E			(4)
#define	F			(5)

/* Magic number */
#define MAX_CHILDREN		(8)

#ifdef	OLIT_SL_NOT_LIKE_XVIEW
/* Implementation-dependent border width of a list pane item */
#define	LIST_PANE_ITEM_BORDER		(1) /* in points, as hardwired in 4/93 */
#define	LIST_PANE_NUM_ITEM_BORDERS	(2)	/* as hardwired in 4/93 */
					/* (3) */
#endif	/* OLIT_SL_NOT_LIKE_XVIEW */		

/* Proportional (zoom) reduction factor to emulate XView spacing */
#define	XVIEW_FUDGE_FACTOR	(0.70)


/**************************************************************************
 *
 *	Forward Procedure Declarations Of Private Procedures
 *
 **************************************************************************/

						/* private procedures */

					/* top-down breadth-first order */

static void	verify_resource_values(FileChooserPart* my);
static void	copy_complex_resources(FileChooserPart* my);
static void	initialize_internal_fields(const Widget c_new);
static void	instantiate_components(const Widget c_new);
static void	manage_components(const Widget c_new);

static void	verify_last_document_name(FileChooserPart* my);
static void	verify_application_folders(FileChooserPart* my);
static void	verify_user_folders(FileChooserPart* my);

static void	copy_string_resources(FileChooserPart* my);
static void	copy_olstr_resources(FileChooserPart* my);

static void	compute_dimensions(const Widget c_new);
static void	load_glyphs(const Widget c_new);
static void	initialize_user_info(FileChooserPart* my);

static void	instantiate_goto_panel(const Widget c_new);
static void	instantiate_goto_menu_items(const Widget c_new);
static void	instantiate_folder_panel(const Widget c_new);
static void	instantiate_list_panel(const Widget c_new);
static void	instantiate_document_panel(const Widget c_new);
static void	instantiate_command_panel(const Widget c_new);

static OlGlyph	get_screen_glyph(const char* bits, const unsigned int width, 
	const unsigned int height, const Screen*const screenp);

						/* callback procedues */

static void	popup_cb(Widget wid, XtPointer client_data, XtPointer call_data);


/**************************************************************************
 *
 *      Implementation Of This Module's External Functions
 *
 **************************************************************************/


/**************************************************************************
 *
 *	Implementation Of Class Procedures
 *
 **************************************************************************/


/**************************************************************************
 *
 *  _OlFileChInitialize --
 * 
 **************************************************************************/

/*ARGSUSED2*/
void
_OlFileChInitialize(
	Widget			request,
	Widget			c_new,
	ArgList			args,		/* unused */
	Cardinal*		num_args	/* unused */
)
{
	const FileChooserWidget	new_fcw = (const FileChooserWidget)c_new;
	CorePart*		cp = &new_fcw->core;
	ManagerPart*		mp = &new_fcw->manager;
	FileChooserPart*	my = &new_fcw->file_chooser;

	_OlStrSetMode(my->text_format);

	copy_complex_resources(my);
	verify_resource_values(my);
	initialize_internal_fields(c_new);

	XtAddCallback(my->shell, XtNpopupCallback, popup_cb, (XtPointer)c_new);

	instantiate_components(c_new);
	manage_components(c_new);

	/*
	 * Verify that the window size is not zero.  (The Core Initialize()
	 * method does not do this.)
	 */
	if (0 == request->core.width)
		c_new->core.width = 100;
	if (0 == request->core.height)
		c_new->core.height = 100;
} /* end of _OlFileChInitialize() */


/**************************************************************************
 *
 *	Implemntation of External Private Procedure
 * 
 **************************************************************************/

/**************************************************************************
 *
 *  _OlVerifyOperationValue --
 * 
 **************************************************************************/

void
_OlVerifyOperationValue(OlDefine* operationp)
{

	if (	
		OL_OPEN		!= *operationp &&
		OL_SAVE		!= *operationp &&
		OL_SAVE_AS	!= *operationp &&
		OL_INCLUDE	!= *operationp &&
		OL_DO_COMMAND	!= *operationp
	) {
		*operationp = OL_OPEN;

		_OlReport(NULL, dgettext(OlMsgsDomain, 
			"_OlVerifyOperationValue():  unknown operation "
			"requested, \"open\" assumed.\n"));
	}
} /* end of _OlVerifyOperationValue() */


/**************************************************************************
 *
 *  _OlFileChVerifyFolderValue --
 * 
 **************************************************************************/

Boolean
_OlFileChVerifyFolderValue(String* folderp)
{
	String		path_bufp;
	String		new_folder;
	Boolean		ok = FALSE;
	
	if (NULL != (path_bufp = _OlFileNavExpandedPathName(*folderp))) {
		_OlStringConstruct(folderp, path_bufp);
		ok = TRUE;
	}

	return ok;
} /* end of _OlFileChVerifyFolderValue() */


/************************************************************************
 *
 *  _OlFileChUpdateCurrentFolderDisplay --
 *
 ************************************************************************/

void
_OlFileChUpdateCurrentFolderDisplay(const Widget wid, const char*const string)
{
	const FileChooserWidget	fcw = (const FileChooserWidget)wid;
	FileChooserPart*	my = &fcw->file_chooser;
	Widget			tlw = my->current_folder_widget;
	int			tail_cursor_position;

	assert(textLineWidgetClass == XtClass(tlw));

	if (NULL != string)
		XtVaSetValues(tlw, XtNstring, string, NULL);

	/*! work around 1110068 !*/
	tail_cursor_position = OlTLGetPosition(tlw, OL_END_LINE);

	XtVaSetValues(tlw, XtNcursorPosition, 0, NULL);

	XtVaSetValues(tlw, 
			XtNcursorPosition,	tail_cursor_position, 
		NULL);
	
	OlUpdateDisplay(tlw);
	/*! end wa 1110068 !*/

} /* end of _OlFileChUpdateCurrentFolderDisplay() */


/**************************************************************************
 *
 *  _OlFileChSetListPrompt --
 * 
 **************************************************************************/

void
_OlFileChSetListPrompt(const Widget c_new)
{
	const FileChooserWidget	new_fcw = (const FileChooserWidget)c_new;
	FileChooserPart*	my = &new_fcw->file_chooser;
	OlDefine		operation = my->operation;
	OlStr			list_prompt = my->list_prompt_string;
	/*! I18N !*/
	String			prompt;
	String			label;
	String			loc1;

	switch (operation) {
	case OL_OPEN:
		prompt = my->open_prompt_string;
		label = my->open_label;
		break;
	case OL_SAVE:
		prompt = my->folder_prompt_string;
		label = my->open_folder_label;
		break;
	case OL_SAVE_AS:
		prompt = my->folder_prompt_string;
		label = my->open_folder_label;
		break;
	case OL_INCLUDE:
		prompt = my->command_prompt_string;
		label = my->include_label;
		break;
	case OL_DO_COMMAND:
		prompt = my->command_prompt_string;
		label = my->command_label;
		break;
	}

	if (NULL == (loc1 = strchr(prompt, '%'))) {
		my->list_prompt_string = prompt;
	} else if (
		OL_OPEN		== operation ||
		OL_SAVE		== operation ||
		OL_SAVE_AS	== operation
	) {
		size_t		prompt_len = _OlStrlen(prompt);
		size_t		label_len = _OlStrlen(label);

		_OL_CALLOC(my->list_prompt_string, 
			prompt_len - 1 + label_len + 1, char);
				
		(void) strncpy(my->list_prompt_string, prompt, loc1 - prompt);
	
		(void) strcat(my->list_prompt_string, label);
		(void) strcat(my->list_prompt_string, loc1 + 1);
	} else if (
		OL_INCLUDE	== operation ||
		OL_DO_COMMAND	== operation
	) {
		String		loc2 = strchr(loc1 + 1, '%');
		size_t		prompt_len = _OlStrlen(prompt);
		size_t		label_len = _OlStrlen(label);
		size_t		open_folder_label_len = 
					_OlStrlen(my->open_folder_label);

		_OL_CALLOC(my->list_prompt_string, 
			prompt_len - 2 + open_folder_label_len + label_len + 1, 
			char);
				
		(void) strncpy(my->list_prompt_string, 	prompt, loc1 - prompt);
		(void) strcat(my->list_prompt_string, label);
		(void) strncat(my->list_prompt_string, loc1 + 1, loc2 - loc1 -1);
		(void) strcat(my->list_prompt_string, my->open_folder_label);
		(void) strcat(my->list_prompt_string, loc2 + 1);
	}

	 XtVaSetValues(my->list_prompt_widget, 
		XtNstring,	my->list_prompt_string, 
	NULL);

} /* end of _OlFileChSetListPrompt() */


/**************************************************************************
 *
 *  _OlFileChSetDocumentLabel --
 * 
 **************************************************************************/

void
_OlFileChSetDocumentLabel(const Widget c_new)
{
	const FileChooserWidget	new_fcw = (const FileChooserWidget)c_new;
	FileChooserPart*	my = &new_fcw->file_chooser;
	/*! I18N !*/
	String			label;

	switch (my->operation) {
	case OL_SAVE:
		label = my->save_label;
		break;
	case OL_SAVE_AS:
		label = my->save_as_label;
		break;
	case OL_DO_COMMAND:
		label = my->command_label;
		break;
	}

	/* 
	 * Derive the caption label by appending a colon to 
	 * the current operation's button's label.
	 */
	_OL_CALLOC(my->document_name_type_in_label, _OlStrlen(label) + 2, char);

	(void) strcpy(my->document_name_type_in_label, label);
	(void) strcat(my->document_name_type_in_label, ":");

	(void) XtVaSetValues(my->document_name_type_in_widget, 
			XtNcaptionLabel,	my->document_name_type_in_label,
		NULL);

} /* end of _OlFileChSetDocumentLabel() */


/**************************************************************************
 *
 *  _OlFileChSetDocumentString --
 * 
 **************************************************************************/

void
_OlFileChSetDocumentString(const Widget c_new)
{
	const FileChooserWidget	new_fcw = (const FileChooserWidget)c_new;
	FileChooserPart*	my = &new_fcw->file_chooser;
	OlDefine		operation = my->operation;
	/*! I18N !*/
	String			name;

	switch (operation) {
	case OL_SAVE:
		name = my->default_document_name;
		break;
	case OL_SAVE_AS:
		name = my->last_document_name;
		break;
	case OL_DO_COMMAND:
		name = my->default_document_name;
		break;
	}

	if (OL_SAVE_AS == my->operation) {
		/* Append the Save As suffix */
		if (NULL != name && 0 != strcmp("", name)) {
			size_t		name_len = _OlStrlen(name);
			size_t		suffix_len = 
					_OlStrlen(my->default_document_suffix);

			_OL_CALLOC(my->document_name_type_in_string, 
				name_len + suffix_len, char);

			(void) strcpy(my->document_name_type_in_string, name);

			(void) strcat(my->document_name_type_in_string, 
				my->default_document_suffix);
		} else
			my->document_name_type_in_string = name;
	} else
		my->document_name_type_in_string = name;

	/*! need to check for clobber by file save !*/
	
	(void) XtVaSetValues(my->document_name_type_in_widget, 
			XtNstring,	my->document_name_type_in_string,
		NULL);

} /* end of _OlFileChSetDocumentString() */


/************************************************************************
 *
 *      Implementation of this module's internal functions
 *
 ************************************************************************/


/**************************************************************************
 *
 *  verify_resource_values --
 * 
 **************************************************************************/

static void
verify_resource_values(FileChooserPart* my)
{

	_OlVerifyOperationValue(&my->operation);

	if (!_OlFileChVerifyFolderValue(&my->current_folder)) {
		String		path_bufp;

		_OlReport(NULL, dgettext(OlMsgsDomain, 
			"_OlFileChVerifyFolderValue():  invalid "
			"folder value %s.\n"
			"Using current working directory instead.\n"), 
			my->current_folder);

		if (NULL != (path_bufp = _OlFileNavExpandedPathName("."))) {
			XtFree(my->current_folder);
			_OlStringConstruct(&my->current_folder, path_bufp);
		} else
			_OlAbort(NULL, dgettext(OlMsgsDomain, 
				"_OlFileChVerifyFolderValue():  unable to "
				"determine the current working directory.\n"));
		}

	(void) _OlFileChVerifyFolderValue(&my->home_folder);
	
	verify_application_folders(my);
	verify_user_folders(my);
	
	if (my->history_folders_max_count < my->history_folders_min_count)
		my->history_folders_max_count = my->history_folders_min_count;

	if (OL_SAVE_AS == my->operation)
		verify_last_document_name(my);

} /* verify_resource_values() */


/**************************************************************************
 *
 *  copy_complex_resources --
 * 
 **************************************************************************/

static void
copy_complex_resources(FileChooserPart* my)
{

	copy_string_resources(my);
	copy_olstr_resources(my);

	/* other resources */
	/*!
	XtCallbackProc		filter_proc;
	OlFolderList		application_folders;
	OlFolderList		user_folders;
	OlFolderList		history_folders;
	OlComparisonFunc	comparison_func;
	WidgetClass		extension_class;
	!*/
} /* end of copy_complex_resources() */


/**************************************************************************
 *
 *  initialize_internal_fields --
 * 
 **************************************************************************/

static void
initialize_internal_fields(const Widget c_new)
{
	const FileChooserWidget	new_fcw = (const FileChooserWidget)c_new;
	FileChooserPart*	my = &new_fcw->file_chooser;
	Arg			args[2];

	my->shell = _OlGetShellOfWidget(c_new);

	compute_dimensions(c_new);

	my->list_prompt_string = NULL;
	my->document_name_type_in_label = NULL;
	my->document_name_type_in_string = NULL;

	my->home_folder_stat_bufferp = NULL;		/*!!*/
	my->current_folder_stat_bufferp = NULL;		/*!!*/
	
	XtSetArg(args[0], XtNshowMnemonics, &my->show_mnemonics);
	XtSetArg(args[1], XtNmnemonicPrefix, &my->mnemonic_modifiers);
	OlGetApplicationValues(c_new, args, XtNumber(args));

	my->filled = FALSE;
	my->chosen_item_node = NULL;
 
	my->tree = NULL;
	my->ring = NULL;

	load_glyphs(c_new);
	initialize_user_info(my);
} /* end of initialize_internal_fields() */


/**************************************************************************
 *
 *  instantiate_components --
 * 
 **************************************************************************/

static void
instantiate_components(const Widget c_new)
{
	const FileChooserWidget	new_fcw = (const FileChooserWidget)c_new;
	CorePart*		cp = &new_fcw->core;
	ManagerPart*		mp = &new_fcw->manager;
	FileChooserPart*	my = &new_fcw->file_chooser;
	OlDefine		operation = my->operation;

	instantiate_goto_panel(c_new);
	instantiate_goto_menu_items(c_new);

	instantiate_folder_panel(c_new);
	instantiate_list_panel(c_new);

	/* Additional controls for Save/Save As */
	if (
		OL_SAVE		== operation || 
		OL_SAVE_AS	== operation || 
		OL_DO_COMMAND	== operation
	)
		instantiate_document_panel(c_new);
	
	/* Optional extension container */
	if (NULL != my->extension_name)
		my->extension_widget = XtVaCreateWidget(my->extension_name,
			formWidgetClass /*!my->extension_class,!*/,  c_new,
				XtNweight, 	0,
				XtNspace,	3 * my->y_dimensions[B],
	
				XtNbackground,		cp->background_pixel,
				XtNbackgroundPixmap,	cp->background_pixmap,
				XtNborderColor,		cp->border_pixel,
				XtNborderPixmap,	cp->border_pixmap,
				XtNborderWidth,		cp->border_width,
				XtNtraversalOn,		mp->traversal_on,
				XtNinputFocusColor,	my->input_focus_color,
			NULL);

	instantiate_command_panel(c_new);

	my->buttom_spacer_c = XtVaCreateWidget("buttom_spacer_c", 
		coreWidgetClass, c_new,
			XtNweight,	0,

			XtNheight,	my->y_dimensions[A],
			XtNwidth,	1,
			XtNsensitive,	FALSE,
			XtNborderWidth,	0,
		NULL);

} /* end of instantiate_components() */


/**************************************************************************
 *
 *  manage_components --
 * 
 **************************************************************************/

static void
manage_components(const Widget c_new)
{
	const FileChooserWidget	new_fcw = (const FileChooserWidget)c_new;
	FileChooserPart*	my = &new_fcw->file_chooser;
	Widget			children[MAX_CHILDREN];
	Cardinal		num_children;
	OlDefine		operation = my->operation;

	XtManageChild(my->document_list_widget);

	num_children = 0;
	children[num_children++] = my->goto_prompt_widget;
	children[num_children++] = my->goto_button_widget;
	children[num_children++] = my->goto_type_in_widget;
	XtManageChildren(children, num_children);

	num_children = 0;
	children[num_children++] = my->current_folder_label_widget;
	children[num_children++] = my->current_folder_widget;
	children[num_children++] = my->list_prompt_widget;
	XtManageChildren(children, num_children);

	if (	
		OL_SAVE		== operation ||
		OL_SAVE_AS	== operation ||
		OL_DO_COMMAND	== operation
	) {
		XtManageChild(my->document_name_type_in_widget);
		XtManageChild(my->document_f);
	}

	num_children = 0;
	children[num_children++] = my->buttons_left_c;
	children[num_children++] = my->open_button_widget;

	if (OL_OPEN != operation)
		children[num_children++] = my->command_button_widget;

	children[num_children++] = my->cancel_button_widget;
	children[num_children++] = my->buttons_right_c;
	XtManageChildren(children, num_children);

	num_children = 0;
	children[num_children++] = my->goto_f;
	children[num_children++] = my->current_folder_f;
	
	if (	
		OL_SAVE		== operation ||
		OL_SAVE_AS	== operation ||
		OL_DO_COMMAND	== operation
	)
		children[num_children++] = my->document_f;
	
	if (NULL != my->extension_name)
		children[num_children++] = my->extension_widget;
	
	children[num_children++] = my->command_center_rt;
	children[num_children++] = my->buttom_spacer_c;
	XtManageChildren(children, num_children);
} /* end of manage_components() */


/**************************************************************************
 *
 *  verify_last_document_name --
 * 
 **************************************************************************/

static void
verify_last_document_name(FileChooserPart* my)
{

	if (OL_SAVE_AS == my->operation && NULL == my->last_document_name) {
		my->last_document_name = "";
		/*! who frees? !*/
	}
	
	/*! last_document_name access !*/
	/*! last_document_name clobber !*/

} /* end of verify_last_document_name() */


/**************************************************************************
 *
 *  verify_application_folders --
 * 
 **************************************************************************/

static void
verify_application_folders(FileChooserPart* my)
{

	/*!preprocess pathname goto folder names!*/
} /* end of verify_application_folders() */


/**************************************************************************
 *
 *  verify_user_folders --
 * 
 **************************************************************************/

static void
verify_user_folders(FileChooserPart* my)
{

	/*!preprocess pathname goto folder names!*/
} /* end of verify_user_folders() */


/**************************************************************************
 *
 *  copy_string_resources --
 * 
 **************************************************************************/

static void
copy_string_resources(FileChooserPart* my)
{

	_OlStringConstruct(&my->current_folder, my->current_folder);
	_OlStringConstruct(&my->filter_string, my->filter_string);
	_OlStringConstruct(&my->home_folder, my->home_folder);
	
	_OlStringConstruct(&my->goto_home_accelerator, 
		my->goto_home_accelerator);

	_OlStringConstruct(&my->go_up_one_folder_accelerator, 
		my->go_up_one_folder_accelerator);

	if (
		OL_SAVE_AS	== my->operation ||
		OL_DO_COMMAND	== my->operation
	)
		_OlStringConstruct(&my->last_document_name, 
			my->last_document_name);

	_OlStringConstruct(&my->extension_name, my->extension_name);

	_OlStringConstruct(&my->cancel_accelerator, my->cancel_accelerator);

	if (
		OL_SAVE		== my->operation ||
		OL_SAVE_AS	== my->operation ||
		OL_INCLUDE	== my->operation ||
		OL_DO_COMMAND	== my->operation
	)
		_OlStringConstruct(&my->open_folder_accelerator, 
			my->open_folder_accelerator);

	switch (my->operation) {

	case OL_OPEN:
		_OlStringConstruct(&my->open_accelerator, my->open_accelerator);
		break;

	case OL_SAVE:
		_OlStringConstruct(&my->save_accelerator, my->save_accelerator);
		break;

	case OL_SAVE_AS:
		_OlStringConstruct(&my->save_as_accelerator, 
			my->save_as_accelerator);
		break;

	case OL_INCLUDE:
		_OlStringConstruct(&my->include_accelerator, 
			my->include_accelerator);
		break;

	case OL_DO_COMMAND:
		_OlStringConstruct(&my->command_accelerator, 
			my->command_accelerator);
		break;
	}

} /* end of copy_string_resources() */


/**************************************************************************
 *
 *  copy_olstr_resources --
 *
 *	Make internal copies of all OlStr-type strings obtained 
 *	from the resource manager.
 * 
 **************************************************************************/

static void
copy_olstr_resources(FileChooserPart* my)
{

	/* OlStr */
	_OlStrConstruct(&my->goto_prompt_string, my->goto_prompt_string);
	_OlStrConstruct(&my->goto_label, my->goto_label);
	_OlStrConstruct(&my->goto_home_label, my->goto_home_label);

	_OlStrConstruct(&my->current_folder_label_string, 
		my->current_folder_label_string);

	_OlStrConstruct(&my->go_up_one_folder_label, my->go_up_one_folder_label);
	
	_OlStrConstruct(&my->cancel_label, my->cancel_label);

	if (	
		OL_SAVE		== my->operation ||
		OL_SAVE_AS	== my->operation || 
		OL_INCLUDE	== my->operation || 
		OL_DO_COMMAND	== my->operation
	)
		_OlStrConstruct(&my->open_folder_label, 
			my->open_folder_label);

	switch (my->operation) {

	case OL_OPEN:
		_OlStrConstruct(&my->open_label, my->open_label);
		break;

	case OL_SAVE:
		_OlStrConstruct(&my->save_label, my->save_label);
		break;

	case OL_SAVE_AS:
		_OlStrConstruct(&my->save_as_label, my->save_as_label);
		break;

	case OL_INCLUDE:
		_OlStrConstruct(&my->include_label, my->include_label);
		break;

	case OL_DO_COMMAND:
		_OlStrConstruct(&my->command_label, my->command_label);
		break;
	}

	if (	
		OL_SAVE		== my->operation ||
		OL_SAVE_AS	== my->operation || 
		OL_DO_COMMAND	== my->operation
	)
		_OlStrConstruct(&my->default_document_name, 
			my->default_document_name);

} /* end of copy_olstr_resources() */


/**************************************************************************
 *
 *  compute_dimensions -- Layout spacing
 * 
 **************************************************************************/

static void
compute_dimensions(const Widget c_new)
{
	const FileChooserWidget	new_fcw = (const FileChooserWidget)c_new;
	FileChooserPart*	my = &new_fcw->file_chooser;

	int			valid_scale;
	const Screen*const	screenp = XtScreen(c_new);
	int			i;
	int			scale_index;

	#define	NUM_SCALES	(4)	/* must the number of items below */
	const int		_10pt = 0;	/* small */
	const int		_12pt = 1;	/* medium */
	const int		_14pt = 2;	/* large */
	const int		_19pt = 3;	/* extra large */

	/* 
	 * From the 92/10/2 GUI functional specification
	 */
	const int		points[NUM_SCALES][NUM_PARAMETERS] = {
			/*	A	B	C	D	E	F */
	/* _10pt */	{	17,	8,	33,	225,	183,	258 },
	/* _12pt */	{	20,	10,	40,	270,	220,	310 },
	/* _14pt */	{	23,	12,	47,	315,	257,	362 },
	/* _19pt */	{	32,	16,	63,	428,	348,	491 },
	};
	#undef	NUM_SCALES

	static Dimension	x_pixels[NUM_PARAMETERS];
	static Dimension	y_pixels[NUM_PARAMETERS];

	valid_scale = OlgxGetValidScale(my->scale);
	my->scale = valid_scale;
	
	switch (my->scale) {
	case 10:
		scale_index = _10pt;
		break;
	case 12:
		scale_index = _12pt;
		break;
	case 14:
		scale_index = _14pt;
		break;
	case 19:
		scale_index = _19pt;
		break;
	default:
		/*! Notice !*/
		_OlReport(NULL, dgettext(OlMsgsDomain, "_OlFileChInit():  "
			"file chooser scale of %d points is not supported, "
			"using the default of 12 points instead.\n"), 
			my->scale);
		scale_index = _12pt;
		break;
	}

	for (i = 0; i < NUM_PARAMETERS; ++i) {
		double		dbl;

		dbl = OlScreenPointToPixel(OL_HORIZONTAL, 
			points[scale_index][i], screenp);
		dbl *= (double)XVIEW_FUDGE_FACTOR;

		x_pixels[i] = _OlRound(dbl);
	}

	for (i = 0; i < NUM_PARAMETERS; ++i) {
		double		dbl;

		dbl = OlScreenPointToPixel(OL_VERTICAL, 
			points[scale_index][i], screenp);
		dbl *= (double)XVIEW_FUDGE_FACTOR;

		y_pixels[i] = _OlRound(dbl);
	}

	my->x_dimensions = x_pixels;
	my->y_dimensions = y_pixels;

} /* end of compute_dimensions() */


/**************************************************************************
 *
 *  load_glyphs -- Load the glyphs, if needed
 * 
 *	The 92/10/2 GUI specification calls for a single glyph to be used 
 *	for all scales.
 *
 *	The glyphs should be dependent on resolution though.
 *
 **************************************************************************/

static void
load_glyphs(const Widget c_new)
{
	const FileChooserWidget	fcw = (const FileChooserWidget)c_new;
	FileChooserPart*	my = &fcw->file_chooser;
	const Screen*const	screenp = XtScreen(c_new);

	#define	GET_SCREEN_GLYPH(file_base_name, screenp) \
		get_screen_glyph((const char*)file_base_name##_bits, \
			(unsigned int)file_base_name##_width, \
			(unsigned int)file_base_name##_height, screenp)

	if (my->show_glyphs) {
		my->folder_glyph = GET_SCREEN_GLYPH(fcfolder, 	
			screenp);
		my->document_glyph = GET_SCREEN_GLYPH(fcdoc, 
			screenp);
		my->go_up_glyph = GET_SCREEN_GLYPH(fcgoup, 
			screenp);
	} else
		my->folder_glyph = my->document_glyph = 
			my->go_up_glyph = (OlGlyph)NULL;
	
	#undef	GET_SCREEN_GLYPH
} /* end of load_glyphs() */


/**************************************************************************
 *
 *  initialize_user_info --
 * 
 **************************************************************************/

static void
initialize_user_info(FileChooserPart* my)
{

	/* Initialize and cache the user id info */
	my->euid = geteuid();
	my->egid = getegid();
	my->groups = NULL;
	my->ngroups = 0;
} /* end of initialize_user_info() */


/**************************************************************************
 *
 *  instantiate_goto_panel --
 * 
 **************************************************************************/

static void
instantiate_goto_panel(const Widget c_new)
{
	const FileChooserWidget	new_fcw = (const FileChooserWidget)c_new;
	CorePart*		cp = &new_fcw->core;
	ManagerPart*		mp = &new_fcw->manager;
	FileChooserPart*	my = &new_fcw->file_chooser;
	Cardinal		request_prompt_width;

	my->goto_f = XtVaCreateWidget("goto_f", formWidgetClass, c_new,
			XtNweight,		0,
			XtNspace,		my->y_dimensions[B],

			XtNbackground,		cp->background_pixel,
			XtNbackgroundPixmap,	cp->background_pixmap,
			XtNborderColor,		cp->border_pixel,
			XtNborderPixmap,	cp->border_pixmap,
			XtNborderWidth,		cp->border_width,
			XtNtraversalOn,		mp->traversal_on,
			XtNinputFocusColor,	my->input_focus_color,
		NULL);

	request_prompt_width = my->x_dimensions[F] + 2 * my->x_dimensions[A];
 
	my->goto_prompt_widget = XtVaCreateWidget("goto_prompt", 
		staticTextWidgetClass, my->goto_f,
			XtNxOffset,		my->x_dimensions[A],
#ifdef	PROMPTS_DO_WRAP
			XtNxResizable,		TRUE,
#endif	/* PROMPTS_DO_WRAP */

			XtNtraversalOn,		FALSE,

#ifdef	PROMPTS_DO_WRAP
			XtNwidth,		request_prompt_width,
			XtNrecomputeSize,	FALSE,
#else	/* PROMPTS_DO_WRAP */
			XtNwrap,		FALSE,
#endif	/* PROMPTS_DO_WRAP */
			XtNgravity,		WestGravity,

			XtNselectable,		FALSE,
			XtNhSpace,		0,
			XtNvSpace,		0,
			XtNstring,		my->goto_prompt_string,

			XtNbackground,		cp->background_pixel,
			XtNbackgroundPixmap,	cp->background_pixmap,
			XtNborderColor,		cp->border_pixel,
			XtNborderPixmap,	cp->border_pixmap,
			XtNborderWidth,		cp->border_width,
			XtNfont,		my->font,
			XtNfontColor,		my->font_color,
			XtNforeground,		my->foreground,
			XtNinputFocusColor,	my->input_focus_color,
			XtNscale,		my->scale,
			XtNtextFormat,		my->text_format,
		NULL);

	my->goto_button_widget = XtVaCreateWidget("goto_button",
		menuButtonWidgetClass, my->goto_f,
			XtNyAddHeight,		TRUE,
			XtNyRefWidget,		my->goto_prompt_widget,
			XtNxOffset,		my->x_dimensions[A],
			XtNyOffset,		my->y_dimensions[B],

			XtNlabel,		my->goto_label,

			XtNbackground,		cp->background_pixel,
			XtNbackgroundPixmap,	cp->background_pixmap,
			XtNborderColor,		cp->border_pixel,
			XtNborderPixmap,	cp->border_pixmap,
			XtNborderWidth,		cp->border_width,
			XtNtraversalOn,		mp->traversal_on,
			XtNfont,		my->font,
			XtNfontColor,		my->font_color,
			XtNforeground,		my->foreground,
			XtNinputFocusColor,	my->input_focus_color,
			XtNscale,		my->scale,
			XtNtextFormat,		my->text_format,
		NULL);

	my->goto_type_in_widget = XtVaCreateWidget("goto_type_in",
		textLineWidgetClass, my->goto_f,
			XtNxAttachRight,	TRUE,
			XtNxResizable,		TRUE,
			XtNxAddWidth,		TRUE,
			XtNxRefWidget,		my->goto_button_widget,
			XtNyAddHeight,		TRUE,
			XtNyRefWidget,		my->goto_prompt_widget,
			XtNxOffset,		my->x_dimensions[B],
			XtNyOffset,		my->y_dimensions[B],
			XtNxAttachOffset,	my->x_dimensions[A],

			XtNbackground,		cp->background_pixel,
			XtNbackgroundPixmap,	cp->background_pixmap,
			XtNborderColor,		cp->border_pixel,
			XtNborderPixmap,	cp->border_pixmap,
			XtNborderWidth,		cp->border_width,
			XtNtraversalOn,		mp->traversal_on,
			XtNfont,		my->font,
			XtNfontColor,		my->font_color,
			XtNforeground,		my->foreground,
			XtNinputFocusColor,	my->input_focus_color,
			XtNscale,		my->scale,
			XtNtextFormat,		my->text_format,
		NULL);
	
	XtAddCallback(my->goto_button_widget, XtNconsumeEvent,
		_OlFileChGotoCB, (XtPointer)c_new);

	XtAddCallback(my->goto_type_in_widget, XtNcommitCallback,
		_OlFileChGotoTypeInCB, (XtPointer)c_new);
} /* end of instantiate_goto_panel() */


/**************************************************************************
 *
 *  instantiate_goto_menu_items --
 * 
 **************************************************************************/

static void
instantiate_goto_menu_items(Widget c_new)
{
	const FileChooserWidget	new_fcw = (const FileChooserWidget)c_new;
	CorePart*		cp = &new_fcw->core;
	ManagerPart*		mp = &new_fcw->manager;
	FileChooserPart*	my = &new_fcw->file_chooser;
	Widget			children[MAX_CHILDREN];
	Cardinal		num_children = 0;
	_OlDatumRec		datum_buf;
	_OlDatum		datum = &datum_buf;
	Cardinal		i;
	Cardinal		user_folders_count;
	Cardinal		application_folders_count;

	XtVaGetValues(my->goto_button_widget,
			XtNmenuPane,	&my->goto_menu_widget,
		NULL);

	my->goto_home_button_widget = XtVaCreateWidget("goto_home_button",
		oblongButtonWidgetClass, my->goto_menu_widget,
			XtNlabel,		my->goto_home_label,
			XtNmnemonic,		my->goto_home_mnemonic,
			XtNaccelerator,		my->goto_home_accelerator,
			XtNuserData, 		my->home_folder,

			XtNbackground,		cp->background_pixel,
			XtNbackgroundPixmap,	cp->background_pixmap,
			XtNborderColor,		cp->border_pixel,
			XtNborderPixmap,	cp->border_pixmap,
			XtNborderWidth,		cp->border_width,
			XtNtraversalOn,		mp->traversal_on,
			XtNfont,		my->font,
			XtNfontColor,		my->font_color,
			XtNforeground,		my->foreground,
			XtNinputFocusColor,	my->input_focus_color,
			XtNscale,		my->scale,
			XtNtextFormat,		my->text_format,
		NULL);

	children[num_children++] = my->goto_home_button_widget;

	XtAddCallback(my->goto_home_button_widget, XtNselect,
		_OlFileChGotoFolderCB, (XtPointer)c_new);
	

	#define	QUOTE(name)		#name

	/******************************************************************
	 *
	 * Create the menu entries for the user folders
	 * 
	 ******************************************************************/

	if (NULL != my->user_folders) {
		for (my->user_folders_count = 0; NULL != *my->user_folders; 
			my->user_folders_count++)	; /*NOOP*/
		_OL_CALLOC(my->user_folders_oba, my->user_folders_count + 1, 
			Widget);

		if (my->user_folders_count > 0) {
			my->user_folders_spacer_b = XtVaCreateWidget(
				"user_folders_spacer_b", buttonWidgetClass, 
				my->goto_menu_widget,
				XtNlabel,		"",
				XtNsensitive,		FALSE,
	
				XtNbackground,		cp->background_pixel,
				XtNbackgroundPixmap,	cp->background_pixmap,
				XtNtraversalOn,		FALSE,
				XtNinputFocusColor,	my->input_focus_color,
				XtNscale,		my->scale,
				NULL);
		
			children[num_children++] = my->user_folders_spacer_b;
	
			for (	i = 0;
				_OlFileChVerifyFolderValue(&my->user_folders[i]),
					i < my->user_folders_count; 
				i++
			) {
				my->user_folders_oba[i] = XtVaCreateWidget(
					"user_folders_ob"  QUOTE(i),
					oblongButtonWidgetClass,	
					my->goto_menu_widget,
				
				XtNlabel,		my->user_folders[i],
				XtNuserData,		my->user_folders[i],
										
				XtNbackground,	cp->background_pixel,
				XtNbackgroundPixmap,	cp->background_pixmap,
				XtNborderColor,		cp->border_pixel,
				XtNborderPixmap,	cp->border_pixmap,
				XtNborderWidth,		cp->border_width,
				XtNtraversalOn,		mp->traversal_on,
				XtNfont,		my->font,
				XtNfontColor,		my->font_color,
				XtNforeground,		my->foreground,
				XtNinputFocusColor,	my->input_focus_color,
				XtNscale,		my->scale,
				XtNtextFormat,		my->text_format,

					NULL);
		
				XtAddCallback(my->user_folders_oba[i], XtNselect,
					_OlFileChGotoFolderCB, (XtPointer)c_new);
		
				children[num_children++] = 
					my->user_folders_oba[i];	
			}
		}
	}	


	/******************************************************************
	 *
	 * Create the menu entries for the application folders
	 * 
	 ******************************************************************/

	if (NULL != my->application_folders) {
		for (	my->application_folders_count = 0; 
			NULL != *my->application_folders; 
			my->application_folders_count++
		)	
			; /*NOOP*/

		_OL_CALLOC(my->application_folders_oba, 
			my->application_folders_count + 1, Widget);

		if (my->application_folders_count > 0) {
			my->application_folders_spacer_b = XtVaCreateWidget(
				"application_folders_spacer_b", buttonWidgetClass, 
				my->goto_menu_widget,
				XtNlabel,		"",
				XtNsensitive,		FALSE,
	
				XtNbackground,		cp->background_pixel,
				XtNbackgroundPixmap,	cp->background_pixmap,
				XtNtraversalOn,		FALSE,
				XtNinputFocusColor,	my->input_focus_color,
				XtNscale,		my->scale,
				NULL);
		
			children[num_children++] = 
				my->application_folders_spacer_b;
	
			for (	i = 0;
				_OlFileChVerifyFolderValue(
						&my->application_folders[i]),
					i < my->application_folders_count; 
				i++
			) {
				my->application_folders_oba[i] = XtVaCreateWidget(
					"application_folders_ob"  QUOTE(i),
					oblongButtonWidgetClass,	
					my->goto_menu_widget,
				
				XtNlabel,	my->application_folders[i],
				XtNuserData,	my->application_folders[i],
										
				XtNbackground,	cp->background_pixel,
				XtNbackgroundPixmap,	cp->background_pixmap,
				XtNborderColor,		cp->border_pixel,
				XtNborderPixmap,	cp->border_pixmap,
				XtNborderWidth,		cp->border_width,
				XtNtraversalOn,		mp->traversal_on,
				XtNfont,		my->font,
				XtNfontColor,		my->font_color,
				XtNforeground,		my->foreground,
				XtNinputFocusColor,	my->input_focus_color,
				XtNscale,		my->scale,
				XtNtextFormat,		my->text_format,

					NULL);
		
				XtAddCallback(my->application_folders_oba[i], 
					XtNselect, _OlFileChGotoFolderCB, 
					(XtPointer)c_new);
		
				children[num_children++] = 
					my->application_folders_oba[i];	
			}
		}
	}	


	/******************************************************************
	 *
	 * Create the menu entries for the history folders
	 * 
	 ******************************************************************/

	if (my->history_folders_max_count > 0) {
	
		_OlRingConstruct((_OlRing*)&my->ring, 
			my->history_folders_max_count);
		
		datum->type = _OL_DATUM_TYPE_STRING;
		datum->content  = my->current_folder;
		_OlRingInsert((_OlRing*)&my->ring, datum);

		my->history_folders_spacer_b = XtVaCreateWidget(
			"history_folders_spacer_b", buttonWidgetClass, 
			my->goto_menu_widget,
				XtNlabel,		"",
				XtNsensitive,		FALSE,
	
				XtNbackground,		cp->background_pixel,
				XtNbackgroundPixmap,	cp->background_pixmap,
				XtNtraversalOn,		FALSE,
				XtNinputFocusColor,	my->input_focus_color,
				XtNscale,		my->scale,
			NULL);
	
		children[num_children++] = my->history_folders_spacer_b;

		_OL_CALLOC(my->history_folders_oba, 
			my->history_folders_max_count, Widget);

		for (i = 0; i < my->history_folders_max_count; i++) {
			my->history_folders_oba[i] = XtVaCreateWidget(
				"history_folders_ob"  QUOTE(i),
				oblongButtonWidgetClass, my->goto_menu_widget,
					XtNbackground,
							cp->background_pixel,
					XtNbackgroundPixmap,
							cp->background_pixmap,
					XtNborderColor,		cp->border_pixel,
					XtNborderPixmap,	cp->border_pixmap,
					XtNborderWidth,		cp->border_width,
					XtNtraversalOn,		mp->traversal_on,
					XtNfont,		my->font,
					XtNfontColor,		my->font_color,
					XtNforeground,		my->foreground,
					XtNinputFocusColor,	
							my->input_focus_color,
					XtNscale,		my->scale,
					XtNtextFormat,		my->text_format,
				NULL);

			XtAddCallback(my->history_folders_oba[i], XtNselect,
				_OlFileChGotoFolderCB, (XtPointer)c_new);
		}
		_OlFileChUpdateHistory(my);
	}

	#undef	QUOTE


	XtManageChildren(children, num_children);
} /* end of instantiate_goto_menu_items() */


/**************************************************************************
 *
 *  instantiate_folder_panel --
 * 
 **************************************************************************/

static void
instantiate_folder_panel(const Widget c_new)
{
	const FileChooserWidget	new_fcw = (const FileChooserWidget)c_new;
	CorePart*		cp = &new_fcw->core;
	ManagerPart*		mp = &new_fcw->manager;
	FileChooserPart*	my = &new_fcw->file_chooser;
	Cardinal		request_prompt_width;

	/* 
	 * Note: The current folder panel include the document list prompt.
	 * This accommodates the geometry change behavior of the list without
	 * incurring the cost of an additional container widget.
	 */

	my->current_folder_f = XtVaCreateWidget("current_folder_f", 
		formWidgetClass, c_new,
			XtNweight,		0,
			XtNspace,		my->y_dimensions[A],

			XtNbackground,		cp->background_pixel,
			XtNbackgroundPixmap,	cp->background_pixmap,
			XtNborderColor,		cp->border_pixel,
			XtNborderPixmap,	cp->border_pixmap,
			XtNborderWidth,		cp->border_width,
			XtNtraversalOn,		mp->traversal_on,
			XtNinputFocusColor,	my->input_focus_color,
		NULL);
	
	my->current_folder_label_widget = XtVaCreateWidget(
		"current_folder_label", captionWidgetClass, 
		my->current_folder_f,
			XtNxOffset,		2 * my->x_dimensions[A],

			XtNtraversalOn,		FALSE,
			XtNlabel,		my->current_folder_label_string,

			XtNbackground,		cp->background_pixel,
			XtNbackgroundPixmap,	cp->background_pixmap,
			XtNborderColor,		cp->border_pixel,
			XtNborderPixmap,	cp->border_pixmap,
			XtNborderWidth,		cp->border_width,
			XtNinputFocusColor,	my->input_focus_color,

	/*! derive the equivalent bold font !*/
	/*!
			XtNfont,		my->font,
	!*/	
			XtNfontColor,		my->font_color,
			XtNforeground,		my->foreground,
			XtNscale,		my->scale,
			XtNtextFormat,		my->text_format,
		NULL);


	my->current_folder_widget = XtVaCreateWidget("current_folder",
		textLineWidgetClass, my->current_folder_f,
			XtNxAttachRight,	TRUE,
			XtNxResizable,		TRUE,
			XtNyAddHeight,		TRUE,
			XtNyRefWidget,		my->current_folder_label_widget,
			XtNxOffset,		2 * my->x_dimensions[A],
			XtNxAttachOffset,	2 * my->x_dimensions[A],
			XtNyOffset,		my->y_dimensions[B],

			XtNtraversalOn,		FALSE,
			XtNeditType,		OL_TEXT_READ,
			XtNunderline, 		FALSE,
			
			XtNbackground,		cp->background_pixel,
			XtNbackgroundPixmap,	cp->background_pixmap,
			XtNborderColor,		cp->border_pixel,
			XtNborderPixmap,	cp->border_pixmap,
			XtNborderWidth,		cp->border_width,
			XtNfont,		my->font,
			XtNfontColor,		my->font_color,
			XtNforeground,		my->foreground,
			XtNinputFocusColor,	my->input_focus_color,
			XtNscale,		my->scale,
			XtNtextFormat,		my->text_format,
		NULL);

	_OlFileChUpdateCurrentFolderDisplay(c_new, my->current_folder);

	request_prompt_width = my->x_dimensions[F] + 2 * my->x_dimensions[A];
 
	my->list_prompt_widget = XtVaCreateWidget("list_prompt", 
		staticTextWidgetClass, my->current_folder_f,
			XtNxOffset,		2 * my->x_dimensions[A],
			XtNyRefWidget,		my->current_folder_widget,
			XtNyAddHeight,		TRUE,
			XtNyOffset,		my->y_dimensions[A],

			XtNwidth,		request_prompt_width,
/*
			XtNrecomputeSize,	FALSE,
*/
			XtNwrap,		FALSE,

			XtNgravity,		WestGravity,

			XtNselectable,		FALSE,
			XtNhSpace,		0,
			XtNvSpace,		0,

			XtNbackground,		cp->background_pixel,
			XtNbackgroundPixmap,	cp->background_pixmap,
			XtNborderColor,		cp->border_pixel,
			XtNborderPixmap,	cp->border_pixmap,
			XtNborderWidth,		cp->border_width,

			XtNtraversalOn,		FALSE,

			XtNfont,		my->font,
			XtNfontColor,		my->font_color,
			XtNforeground,		my->foreground,
			XtNinputFocusColor,	my->input_focus_color,
			XtNscale,		my->scale,
			XtNtextFormat,		my->text_format,
		NULL);

	_OlFileChSetListPrompt(c_new);
	
} /* end of instantiate_folder_panel() */


/**************************************************************************
 *
 *  instantiate_list_panel --
 * 
 **************************************************************************/

static void
instantiate_list_panel(const Widget c_new)
{
	const FileChooserWidget	new_fcw = (const FileChooserWidget)c_new;
	CorePart*		cp = &new_fcw->core;
	ManagerPart*		mp = &new_fcw->manager;
	FileChooserPart*	my = &new_fcw->file_chooser;
	Widget			list_pane;

#ifdef	OLIT_SL_NOT_LIKE_XVIEW
	Cardinal		request_item_height;
		
	/* 
	 * The file chooser GUI specifies the overall item height.
	 * Account for the all-around border provided by the 
	 * list pane around an item.
	 *
	 * This is both implementation-dependent and approximate.
	 */
	request_item_height = my->y_dimensions[A] -  
		OlScreenPointToPixel(OL_VERTICAL, LIST_PANE_ITEM_BORDER, 
			XtScreen(c_new)) * 2 * LIST_PANE_NUM_ITEM_BORDERS;
#endif	/* OLIT_SL_NOT_LIKE_XVIEW */

	my->document_list_widget = XtVaCreateWidget("document_list",
		scrollingListWidgetClass, c_new,
			XtNspace,		my->y_dimensions[B],

#ifdef	OLIT_SL_NOT_LIKE_XVIEW
			XtNitemHeight,		request_item_height,
#else
			XtNitemHeight,		my->y_dimensions[A],
#endif	/* OLIT_SL_NOT_LIKE_XVIEW */

			XtNposition,		OL_RIGHT,
			XtNprefMaxWidth,	my->x_dimensions[F],
			XtNscrollingListMode,	OL_EXCLUSIVE,

			XtNviewHeight,		my->list_visible_item_count >
				 		my->list_visible_item_min_count ?
						my->list_visible_item_count : 
						my->list_visible_item_min_count,
			
			XtNbackground,		cp->background_pixel,
			XtNbackgroundPixmap,	cp->background_pixmap,
			XtNborderColor,		cp->border_pixel,
			XtNborderPixmap,	cp->border_pixmap,
			XtNborderWidth,		cp->border_width,
			XtNtraversalOn,		mp->traversal_on,
			XtNfont,		my->font,
			XtNfontColor,		my->font_color,
			XtNforeground,		my->foreground,
			XtNinputFocusColor,	my->input_focus_color,
			XtNscale,		my->scale,
			XtNtextFormat,		my->text_format,
		NULL);

	XtVaSetValues(_OlListSBar(my->document_list_widget), 
			XtNxAttachOffset,	2 * my->x_dimensions[A],
		NULL);

	XtVaGetValues(my->document_list_widget, XtNlistPane, &list_pane, NULL);
	XtVaSetValues(list_pane, XtNxOffset, 2 * my->x_dimensions[A], NULL);

	/* _OlFileChListSelectCB() is called when an item is selected */
	XtAddCallback(my->document_list_widget, XtNitemCurrentCallback,
		_OlFileChItemCurrentCB, (XtPointer)c_new);

	XtAddCallback(my->document_list_widget, XtNmultiClickCallback,
		_OlFileChOpenCB, (XtPointer)c_new);

	XtAddEventHandler(list_pane, 
		EnterWindowMask | LeaveWindowMask | FocusChangeMask, 
		FALSE, _OlFileChListEventHandler, (XtPointer)c_new);

} /* end of instantiate_list_panel() */


/**************************************************************************
 *
 *  instantiate_document_panel --
 * 
 **************************************************************************/

static void
instantiate_document_panel(const Widget c_new)
{
	const FileChooserWidget	new_fcw = (const FileChooserWidget)c_new;
	CorePart*		cp = &new_fcw->core;
	ManagerPart*		mp = &new_fcw->manager;
	FileChooserPart*	my = &new_fcw->file_chooser;
	int			tail_cursor_position;

	my->document_f = XtVaCreateWidget("document_f",
		formWidgetClass, c_new,
			XtNweight,	0,
			XtNspace,	my->y_dimensions[B],

			XtNbackground,		cp->background_pixel,
			XtNbackgroundPixmap,	cp->background_pixmap,
			XtNborderColor,		cp->border_pixel,
			XtNborderPixmap,	cp->border_pixmap,
			XtNborderWidth,		cp->border_width,
			XtNtraversalOn,		mp->traversal_on,
			XtNinputFocusColor,	my->input_focus_color,
		NULL);

	my->document_name_type_in_widget = XtVaCreateWidget(
		"document_name_type_in", textLineWidgetClass, my->document_f,
			XtNxResizable,		TRUE,
			XtNxAttachRight,	TRUE,
			XtNxOffset,		my->x_dimensions[A],
			XtNxAttachOffset,	my->x_dimensions[A],

			XtNcaptionSpace,	my->x_dimensions[B],

			XtNbackground,		cp->background_pixel,
			XtNbackgroundPixmap,	cp->background_pixmap,
			XtNborderColor,		cp->border_pixel,
			XtNborderPixmap,	cp->border_pixmap,
			XtNborderWidth,		cp->border_width,
			XtNtraversalOn,		mp->traversal_on,
			XtNfont,		my->font,
			XtNfontColor,		my->font_color,
			XtNforeground,		my->foreground,
			XtNinputFocusColor,	my->input_focus_color,
			XtNscale,		my->scale,
			XtNtextFormat,		my->text_format,
		NULL);

	_OlFileChSetDocumentLabel(c_new);
	_OlFileChSetDocumentString(c_new);

	tail_cursor_position = OlTLGetPosition(my->document_name_type_in_widget, 
		OL_END_LINE);

	XtVaSetValues(my->document_name_type_in_widget, XtNcursorPosition, 
		tail_cursor_position, NULL);

	XtAddCallback(my->document_name_type_in_widget,	XtNcommitCallback,
		_OlFileChDocumentTypeInCB, (XtPointer)c_new);

	XtAddCallback(my->document_name_type_in_widget, XtNpostModifyCallback,
		_OlFileChDocumentTypeInCB, (XtPointer)c_new);
} /* end of instantiate_document_panel() */


/**************************************************************************
 *
 *  instantiate_command_panel --
 * 
 **************************************************************************/

static void
instantiate_command_panel(const Widget c_new)
{
	const FileChooserWidget	new_fcw = (const FileChooserWidget)c_new;
	CorePart*		cp = &new_fcw->core;
	ManagerPart*		mp = &new_fcw->manager;
	FileChooserPart*	my = &new_fcw->file_chooser;

	my->command_center_rt = XtVaCreateWidget("command_center_rt",
		rubberTileWidgetClass, c_new,
			XtNweight,		0,
			XtNorientation,		OL_HORIZONTAL,
			XtNspace,		my->y_dimensions[C],

			XtNbackground,		cp->background_pixel,
			XtNbackgroundPixmap,	cp->background_pixmap,
			XtNborderColor,		cp->border_pixel,
			XtNborderPixmap,	cp->border_pixmap,
			XtNborderWidth,		cp->border_width,
			XtNtraversalOn,		mp->traversal_on,
			XtNinputFocusColor,	my->input_focus_color,
		NULL);
	
	if (NULL != my->extension_name)
		XtVaSetValues(my->command_center_rt,
				XtNspace,	3 * my->y_dimensions[B],
			NULL);

	my->buttons_left_c = XtVaCreateWidget("buttons_left_c", coreWidgetClass, 
		my->command_center_rt,
			XtNborderWidth,		0,
			XtNsensitive,		FALSE,
		NULL);

	my->open_button_widget = XtVaCreateWidget("open_button",
		oblongButtonWidgetClass, my->command_center_rt,
			XtNweight,	0,

			XtNbackground,		cp->background_pixel,
			XtNbackgroundPixmap,	cp->background_pixmap,
			XtNborderColor,		cp->border_pixel,
			XtNborderPixmap,	cp->border_pixmap,
			XtNborderWidth,		cp->border_width,
			XtNtraversalOn,		mp->traversal_on,
			XtNfont,		my->font,
			XtNfontColor,		my->font_color,
			XtNforeground,		my->foreground,
			XtNinputFocusColor,	my->input_focus_color,
			XtNscale,		my->scale,
			XtNtextFormat,		my->text_format,
		NULL);

	XtAddCallback(my->open_button_widget, XtNselect, _OlFileChOpenCB,
		(XtPointer)c_new);

	my->cancel_button_widget = XtVaCreateWidget("cancel_button",
		oblongButtonWidgetClass, my->command_center_rt,
			XtNweight,		0,
			XtNspace,		my->x_dimensions[A],

			XtNlabel,		my->cancel_label,
			XtNmnemonic,		my->cancel_mnemonic,
			XtNaccelerator,		my->cancel_accelerator,
			
			XtNbackground,		cp->background_pixel,
			XtNbackgroundPixmap,	cp->background_pixmap,
			XtNborderColor,		cp->border_pixel,
			XtNborderPixmap,	cp->border_pixmap,
			XtNborderWidth,		cp->border_width,
			XtNtraversalOn,		mp->traversal_on,
			XtNfont,		my->font,
			XtNfontColor,		my->font_color,
			XtNforeground,		my->foreground,
			XtNinputFocusColor,	my->input_focus_color,
			XtNscale,		my->scale,
			XtNtextFormat,		my->text_format,
		NULL);

	XtAddCallback(my->cancel_button_widget, XtNselect, _OlFileChCancelCB,
		(XtPointer)c_new);

	/* Additional button for Save/Save As/Include/Do Command */
	if (OL_OPEN != my->operation) {
	    
		XtVaSetValues(my->open_button_widget,
				XtNlabel,	my->open_folder_label,
				XtNmnemonic,	my->open_folder_mnemonic,
				XtNaccelerator,	my->open_folder_accelerator,
			NULL);

		my->command_button_widget = XtVaCreateWidget("command_button",
			oblongButtonWidgetClass, my->command_center_rt,
				XtNweight,		0,
				XtNspace,		my->x_dimensions[A],
	
				XtNbackground,		cp->background_pixel,
				XtNbackgroundPixmap,	cp->background_pixmap,
				XtNborderColor,		cp->border_pixel,
				XtNborderPixmap,	cp->border_pixmap,
				XtNborderWidth,		cp->border_width,
				XtNtraversalOn,		mp->traversal_on,
				XtNfont,		my->font,
				XtNfontColor,		my->font_color,
				XtNforeground,		my->foreground,
				XtNinputFocusColor,	my->input_focus_color,
				XtNscale,		my->scale,
				XtNtextFormat,		my->text_format,
			NULL);

		switch (my->operation) {

		case OL_SAVE:
			XtVaSetValues(my->command_button_widget, 
					XtNlabel,	my->save_label,
					XtNmnemonic,	my->save_mnemonic,
					XtNaccelerator,	my->save_accelerator,
					XtNdefault,	TRUE, 
				NULL);

			if (_OlStrIsEmpty(my->default_document_name, 
					my->text_format))
				XtVaSetValues(my->command_button_widget, 
						XtNsensitive,	FALSE,
					NULL);
			break;
	
		case OL_SAVE_AS:
			XtVaSetValues(my->command_button_widget, 
					XtNlabel,	my->save_label,
					XtNmnemonic,	my->save_as_mnemonic,
					XtNaccelerator,	my->save_as_accelerator,
					XtNdefault,	TRUE, 
				NULL);
			if (_OlStrIsEmpty(my->last_document_name,
					my->text_format))
				XtVaSetValues(my->command_button_widget, 
						XtNsensitive,	FALSE,
					NULL);
			break;
	
		case OL_INCLUDE:
			XtVaSetValues(my->command_button_widget, 
					XtNlabel,	my->include_label,
					XtNmnemonic,	my->include_mnemonic,
					XtNaccelerator,	my->include_accelerator,
					XtNdefault,	TRUE,
					XtNsensitive,	FALSE,
				NULL);
			break;
	
		case OL_DO_COMMAND:
			XtVaSetValues(my->command_button_widget, 
					XtNlabel,	my->command_label,
					XtNmnemonic,	my->command_mnemonic,
					XtNaccelerator,	my->command_accelerator,
					XtNdefault,	TRUE, 
				NULL);
			break;
		}
	
		XtAddCallback(my->command_button_widget, XtNselect,
			_OlFileChCommandCB, (XtPointer)c_new);
	} else
		XtVaSetValues(my->open_button_widget,
				XtNlabel,	my->open_label,
				XtNmnemonic,	my->open_mnemonic,
				XtNaccelerator,	my->open_accelerator,
				XtNdefault, 	TRUE,
			NULL);
	
	my->buttons_right_c = XtVaCreateWidget("buttons_right_c", 
		coreWidgetClass, my->command_center_rt,
			XtNborderWidth,	0,
			XtNsensitive,	FALSE,
		NULL);

} /* end of instantiate_command_panel() */


/************************************************************************
 *
 *  get_screen_glyph --
 *
 ************************************************************************/

static OlGlyph
get_screen_glyph(const char* bits, const unsigned int width, 
	const unsigned int height, const Screen*const screenp)
{
	XImage*			imagep = (XImage*)NULL;
	Pixmap			pixmap;
	Display*		displayp = XDisplayOfScreen((Screen*)screenp);
	OlGlyphRec		glyph_buf;
	_OlDatumRec		datum_buf;
	_OlDatum		datum;

	pixmap = XCreateBitmapFromData(displayp, RootWindowOfScreen(screenp), 
		bits, width, height);

	imagep = XGetImage(displayp, pixmap, 0, 0, width, height, 1, XYPixmap);
	XFreePixmap(displayp, pixmap);
	/*
	 * Change the format since XPutImage() can only put images of depth 1 
	 * as Bitmap format for destinations that have depths greater than 1.
	 */
	imagep->format = XYBitmap;

	glyph_buf.type = OL_GLYPH_TYPE_XIMAGE;
	glyph_buf.rep = (OlGlyphRep)imagep;
	datum_buf.type = _OL_DATUM_TYPE_GLYPH;
	datum_buf.content = &glyph_buf;
	_OlDatumConstruct(&datum, &datum_buf);

	return ((OlGlyph)datum->content);
	
} /* end of get_screen_glyph() */


/**************************************************************************
 *
 *	Callback Procedures
 * 
 **************************************************************************/


/**************************************************************************
 *
 *  popup_cb --
 * 
 **************************************************************************/

/*ARGSUSERD2*/
static void
popup_cb(
	Widget		wid,
	XtPointer 	client_data, 
	XtPointer	call_data	/* unused*/
)
{
	const FileChooserWidget	fcw = (const FileChooserWidget)wid;
	FileChooserPart*	my = &fcw->file_chooser;

	_OlFileChFillList((Widget)client_data);
} /* end of popup_cb() */


/* end of FileChInit.c */
