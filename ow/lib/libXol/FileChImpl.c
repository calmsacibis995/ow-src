#pragma ident	"@(#)FileChImpl.c	1.23    97/03/26 lib/libXol SMI"    /* OLIT	493	*/

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

/************************************************************************
 *
 *	Implementation of the file chooser panel widget call backs
 *		and other internal functions
 *		
 ************************************************************************/


/************************************************************************
 *
 *	Imported Interfaces 
 *
 ************************************************************************/

#ifndef	DEBUG
#define	NDEBUG
#endif

#include <assert.h>		/* assert() */
#include <libintl.h>		/* dgettext() */
#include <stdio.h>
#include <string.h>
#include <sys/types.h>  	/* boolean_t */
#include <unistd.h>		/* getcwd() */
#include <widec.h>		/* wschr() */

extern int	strcasecmp(const char* s1, const char* s2);
				/* missing from string.h */

#include <X11/Xlib.h>		/* XDefineCursor() */

#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <X11/Shell.h>

#include <Xol/OlCursors.h>	/* OlGetBusyCursor(), OlGetStandardCursor() */
#include <Xol/Datum.h>
#include <Xol/FileChInit.h>
#include <Xol/FileChP.h>
#include <Xol/FileChShP.h>	/* _OlFileChShPopDown() */
#include <Xol/OpenLook.h>	/* OlGlyph, OlGlyphType, OlGlyphRep */
#include <Xol/OpenLookP.h>
#include <Xol/Ring.h>
#include <Xol/SBTree.h>
#include <Xol/ScrollingL.h>
#include <Xol/diags.h>
#include <Xol/filenav.h>
#include <Xol/TextLine.h>
#include <Xol/PopupWindo.h>	/* _OlPopupWiShPopDown() */

#include <Xol/FileChImpl.h>	/* interface of this implementation */


/************************************************************************
 *
 *	Forward procedure declarations of private procedures
 *
 ************************************************************************/

						/* private procedures */

static void		callback_input_document(
	const FileChooserWidget fcw, String document);

static void		callback_output_document(
	const FileChooserWidget fcw, String document);

static void		callback_cancel(const FileChooserWidget fcw);

static void		do_pre_fill_list(const Widget wid);
static void		do_post_fill_list(const Widget wid);
static void		do_go_up_item(FileChooserPart* my);

static void		fill_list_aux(const _OlDatum datum, void* fcpp, 
	void* arg2, void* arg3, void* arg4);

static void		fill_history_aux(const _OlDatum datum, void* fcpp, 
	void* arg2, void* arg3, void* arg4);

static void		extract_folder_n_doc(String goto_folder, 
	String*	the_folder, String* the_doc);

static char	*get_realpath (const char *input, char *output);
static char 	*filech_strtok (char *token_string, const char *sep_string);
static void 	_OlFileChChaseSymlinks (char *fname);

#ifdef	ENABLE_CRT102_1107044
static Boolean		select_given_doc(FileChooserPart* fcp, 
	String the_doc);
#endif	/* ENABLE_CRT102_1107044 */

static void		popdown_shell(const Widget shell, 
				const Boolean override_pushpin);


/************************************************************************
 *
 *	Implementation Of This Module's External Functions
 *
 ************************************************************************/


/**************************************************************************
 *
 *	Implementation Of Public Procedures
 *
 **************************************************************************/


/**************************************************************************
 *
 *  OlSortStrCaseDescending -- 
 * 
 **************************************************************************/

/*! Can these functions handle w_char? !*/
/*! is the notion of case defined in locale information? !*/
int
OlSortStrCaseDescending(const OlStr lstr, const OlStr rstr)
{

	return (-strcoll(lstr, rstr));
} /* end of OlSortStrCaseDescending() */


/**************************************************************************
 *
 *  OlSortStrNoCaseAscending -- 
 * 
 **************************************************************************/

int
OlSortStrNoCaseAscending(const OlStr lstr, const OlStr rstr)
{

	/*!strxfrm(lstr), strxfrm(rstr)!*/
	return (strcasecmp(lstr, rstr));
} /* end of OlSortStrNoCaseAscending() */


/**************************************************************************
 *
 *  OlSortStrNoCaseDescending -- 
 * 
 **************************************************************************/

int
OlSortStrNoCaseDescending(const OlStr lstr, const OlStr rstr)
{

	/*!strxfrm(lstr), strxfrm(rstr)!*/
	return (-strcasecmp(lstr, rstr));
} /* end of OlSortStrNoCaseDescending() */


/**************************************************************************
 *
 *	Implementation of callback procedures
 *
 **************************************************************************/


/**************************************************************************
 *
 *  _OlFileChGotoCB -- 
 * 
 **************************************************************************/

/*ARGSUSED*/
void
_OlFileChGotoCB(
	Widget		widget,		/* unused */
	XtPointer	client_data,
	XtPointer	call_data
)
{
	FileChooserWidget	fcw = (FileChooserWidget)client_data;
	FileChooserPart*	my = &fcw->file_chooser;
	OlVirtualEvent		ve = (OlVirtualEvent)call_data;

	assert(NULL != fcw && XtIsSubclass((Widget)fcw, fileChooserWidgetClass));

	if (OL_SELECT == ve->virtual_name) {
		String		string;
		
		XtVaGetValues(my->goto_type_in_widget, XtNstring, &string, NULL);
	
		if (!_OlStrIsEmpty(string, my->text_format)) {
			ve->consumed = TRUE;
			(void) OlActivateWidget(my->goto_type_in_widget, 
				OL_NEXTFIELD, NULL);
		}
	}
} /* end of _OlFileChGotoCB() */


/**************************************************************************
 *
 *  _OlFileChGotoTypeInCB -- 
 * 
 **************************************************************************/

/*! I18N !*/
void
_OlFileChGotoTypeInCB(Widget widget, XtPointer client_data, XtPointer call_data)
{
	static char			goto_folder[BIGGEST_PATH];

	FileChooserWidget		fcw = (FileChooserWidget)client_data;
	FileChooserPart*		my = &fcw->file_chooser;

	OlTLCommitCallbackStruct*	tlcd =
					(OlTLCommitCallbackStruct*)call_data;

	String				expanded_name;
	struct stat			stat_buf;
	
	assert(NULL != fcw && XtIsSubclass((Widget)fcw, fileChooserWidgetClass));

	GetToken();
	if (	
		!_OlStrIsEmpty(tlcd->buffer, my->text_format) &&
		!_OlStrIsEmpty(
			(expanded_name = 
				_OlFileNavExpandedPathName(tlcd->buffer)), 
			my->text_format)
	) {
		FileChooserPart *fcp = &fcw->file_chooser;

		XtVaSetValues(my->goto_button_widget, XtNbusy, TRUE, NULL);
		OlUpdateDisplay(my->goto_button_widget);

		if (_OlFileNavIsAbsolutePath(expanded_name))
			(void) strncpy(goto_folder, expanded_name, BIGGEST_PATH);
		else
			(void) snprintf(goto_folder, BIGGEST_PATH,
				"%s/%s", my->current_folder, expanded_name);

		/* Handle '.' and '..' occurences in folder name. */
		get_realpath (goto_folder, expanded_name);
		strncpy (goto_folder, expanded_name, BIGGEST_PATH);

		/* If we don't want the parent folder of a symbolic link,
		   continue until we reach a non-symbolic link node. */
		if (!fcp->follow_symlinks)
			_OlFileChChaseSymlinks (goto_folder);
				
		if (BAD_SYSCALL == stat(goto_folder, &stat_buf)) {
			/*! Notice !*/
			_OlReport("stat()", dgettext(OlMsgsDomain, 
				"_OlFileChGotoTypeInCB():  failed on %s.\n"),
				goto_folder);
		} else if (S_ISDIR(stat_buf.st_mode)) {
			_OlFileChCallbackOpenFolder(fcw, goto_folder);

			XtVaSetValues(widget, XtNstring, NULL, NULL);
			XtVaSetValues(my->goto_home_button_widget, 
					XtNdefault, TRUE, NULL);
		} else {
			/*! 
				if resolves to a document pathname, 
					accelerate to it
			!*/

			#ifdef	ENABLE_CRT102_1107044
			String			the_folder;
			String			the_doc;

			extract_folder_n_doc(goto_folder, &the_folder, &the_doc);

			if (NULL != the_folder) {
				_OlFileChCallbackOpenFolder(fcw, the_folder);
				XtFree(the_folder);
			}

			if (NULL != the_doc) {
				/* Select the_doc in the scrolling list */
				if (!select_given_doc(my, the_doc))
					/*! report error !*/
				XtFree(the_doc);
			}
			#endif	/* ENABLE_CRT102_1107044 */
		}

		XtVaSetValues(my->goto_button_widget, XtNbusy, FALSE, NULL);
		OlUpdateDisplay(my->goto_button_widget);
	}
	tlcd->valid = FALSE;

	if (OlCanAcceptFocus(my->document_list_widget, CurrentTime))
		OlCallAcceptFocus(my->document_list_widget, CurrentTime);
	else if (OL_OPEN != my->operation && OL_INCLUDE	!= my->operation)
		OlCallAcceptFocus(my->document_name_type_in_widget, CurrentTime);

	ReleaseToken();
} /* end of _OlFileChGotoTypeInCB() */


/**************************************************************************
 *
 *  _OlFileChGotoFolderCB -- 
 * 
 **************************************************************************/

/*ARGSUSED2*/
void
_OlFileChGotoFolderCB(
	Widget		widget,
	XtPointer	client_data,
	XtPointer	call_data	/* unused */
)
{
	FileChooserWidget	fcw = (FileChooserWidget)client_data;
	FileChooserPart*	my = &fcw->file_chooser;
	String			folder;

	assert(NULL != fcw && XtIsSubclass((Widget)fcw, fileChooserWidgetClass));
	
	XtVaGetValues(widget, XtNuserData, &folder, NULL);
	assert(NULL != folder);
	_OlFileChCallbackOpenFolder(fcw, folder);
} /* end of _OlFileChGotoFolderCB() */


/************************************************************************
 *
 *  _OlFileChItemCurrentCB -- 
 *
 ************************************************************************/

void
_OlFileChItemCurrentCB(
	Widget		widget,
	XtPointer	client_data,
	XtPointer	call_data
)
{
	FileChooserWidget	fcw = (FileChooserWidget)client_data;
	FileChooserPart*	my = &(fcw->file_chooser);
	OlSlistCallbackStruct*	cb_datap = (OlSlistCallbackStruct*)call_data;
	OlFileChListChoiceCallbackStruct
				fc_cdata = { OL_REASON_LIST_CHOICE, NULL };

	my->chosen_item_node = (OlFNavNode)OlSlistGetItemUserData(widget, 
		cb_datap->item);

	if (OL_INCLUDE == my->operation)
		if (my->chosen_item_node->is_folder) {
			XtVaSetValues(my->command_button_widget, XtNsensitive, 
				FALSE, NULL);
			XtVaSetValues(my->open_button_widget, XtNsensitive, 
				TRUE, NULL);
		} else {
			XtVaSetValues(my->command_button_widget, XtNsensitive, 
				TRUE, NULL);
			XtVaSetValues(my->open_button_widget, XtNsensitive, 
				FALSE, 	NULL);
		}


	fc_cdata.operation = my->operation;
	fc_cdata.current_folder = my->current_folder;

	fc_cdata.chosen_item = cb_datap->item;
	fc_cdata.chosern_item_pos = cb_datap->item_pos;
	fc_cdata.chosen_item_node = my->chosen_item_node;
	
	XtCallCallbackList((Widget)fcw, my->list_choice_callback, &fc_cdata);
} /* end of _OlFileChItemCurrentCB() */


/************************************************************************
 *
 *  _OlFileChListEventHandler -- 
 *
 ************************************************************************/

/*ARGSUSED3*/
void
_OlFileChListEventHandler(
	Widget		widget,
	XtPointer	client_data,
	XEvent*		event,
	Boolean*	continue_to_dispatch	/* unused */
)
{
	FileChooserWidget	fcw = (FileChooserWidget)client_data;
	FileChooserPart*	my = &fcw->file_chooser;
	Arg			args[2];

	assert(NULL != fcw && XtIsSubclass((Widget)fcw, fileChooserWidgetClass));
	
	switch (event->type) {
	case EnterNotify:
	/*FALLTHROUGH*/
	case FocusIn:
		XtSetArg(args[0], XtNshowMnemonics, OL_NONE);
		XtSetArg(args[1], XtNmnemonicPrefix, None);
		OlSetApplicationValues(widget, args, XtNumber(args));
		break;
	case LeaveNotify:
	/*FALLTHROUGH*/
	case FocusOut:
		XtSetArg(args[0], XtNshowMnemonics, my->show_mnemonics);
		XtSetArg(args[1], XtNmnemonicPrefix, my->mnemonic_modifiers);
		OlSetApplicationValues(widget, args, XtNumber(args));
		break;
	}
} /* end of _OlFileChListEventHandler() */


/**************************************************************************
 *
 *  _OlFileChDocumentTypeInCB -- 
 * 
 **************************************************************************/

void
_OlFileChDocumentTypeInCB(
	Widget		widget,
	XtPointer	client_data,
	XtPointer	call_data
)
{
	FileChooserWidget	fcw = (FileChooserWidget)client_data;
	FileChooserPart*	my = &fcw->file_chooser;
	OlAnyCallbackStruct*	any_cd = (OlAnyCallbackStruct*)call_data;

	assert(NULL != fcw && XtIsSubclass((Widget)fcw, fileChooserWidgetClass));
	
	switch (any_cd->reason) {
	case OL_REASON_POST_MODIFICATION:
		{
			OlTLPostModifyCallbackStruct*	tlpm_cd = 
				(OlTLPostModifyCallbackStruct*)any_cd;
				
			if (_OlStrIsEmpty(tlpm_cd->buffer, my->text_format))
				XtVaSetValues(my->command_button_widget,
						XtNsensitive,	FALSE,
					NULL);
			else
				XtVaSetValues(my->command_button_widget,
						XtNsensitive,	TRUE,
					NULL);
		}
		break;
	case OL_REASON_COMMIT:
		{
			OlTLCommitCallbackStruct*	tlcd = 
				(OlTLCommitCallbackStruct*)call_data;
	
			if (!_OlMouseless(widget) && !_OlStrIsEmpty(
					tlcd->buffer, my->text_format)) {
				callback_output_document(fcw, tlcd->buffer);
				popdown_shell(my->shell, FALSE);

				/*!
					if full-fledged pathname (and
					no access problems), split it
					into the requested_folder and
					requested_document fields and
					call save_cb.

					if a base name (and no access
					problems), set same fields and
					call same cb.
				!*/
			}
			tlcd->valid = FALSE;

			if (OlCanAcceptFocus(my->open_button_widget, 
					CurrentTime))
				OlCallAcceptFocus(my->open_button_widget, 
					CurrentTime);
			else
				OlCallAcceptFocus(my->goto_type_in_widget, 
					CurrentTime);
		}
		break;
	}
} /* end of _OlFileChDocumentTypeInCB() */


/**************************************************************************
 *
 *  _OlFileChOpenCB -- Callback procedure for the Open and Open Folder buttons
 * 
 **************************************************************************/

/*ARGSUSED*/
void
_OlFileChOpenCB(
	Widget		widget,		/* unused */
	XtPointer	client_data,
	XtPointer	call_data	/* unused */
)
{
	static char		parent_folder[BIGGEST_PATH];

	FileChooserWidget	fcw = (FileChooserWidget)client_data;
	FileChooserPart*	my = &fcw->file_chooser;
	int			reason = OL_REASON_ERROR;
	Boolean			reached_file_system_root = FALSE;
	OlSlistItemPtr*		item_ptr_array;
	OlFNavNode		parent_folder_node;

	GetToken();
	
	assert(NULL != fcw && XtIsSubclass((Widget)fcw, fileChooserWidgetClass));

	if (my == NULL || my->chosen_item_node == NULL ||
		my->chosen_item_node->name == NULL)
		return;

	if (0 == strcmp(my->chosen_item_node->name, 
			my->go_up_one_folder_label)) {
		/* 
		 * go up 
		 */
		if (0 == strcmp(my->current_folder, "/")) {
			reached_file_system_root = TRUE;
	
			XtVaGetValues(my->document_list_widget, XtNcurrentItems, 
				&item_ptr_array, NULL);
	
			parent_folder_node = (OlFNavNode)OlSlistGetItemUserData(
				my->document_list_widget, *item_ptr_array);
	
			(void) strcpy(parent_folder, "/");
			(void) strncat(parent_folder,
					parent_folder_node->name, BIGGEST_PATH-2);
		} else
			(void) strncpy(parent_folder, 
				_OlFileNavParentFolder(my->current_folder), BIGGEST_PATH);

		reason = OL_REASON_OPEN_FOLDER;
		_OlFileChCallbackOpenFolder(fcw, parent_folder);
	} else {
		switch (my->operation) {
		case OL_OPEN:
			reason = my->chosen_item_node->is_folder ? 
				OL_REASON_OPEN_FOLDER : 
				OL_REASON_INPUT_DOCUMENT;
			break;
		case OL_SAVE:
		/*FALLTHROUGH*/
		case OL_SAVE_AS:
			reason = OL_REASON_OPEN_FOLDER;
			break;
		case OL_INCLUDE:
			if (my->chosen_item_node->is_folder)
				reason = OL_REASON_OPEN_FOLDER;
			else {
				_OlFileChCommandCB(NULL, client_data, NULL);
				return;
			}
			break;
		case OL_DO_COMMAND:
		/*FALLTHROUGH*/
		default:
			_OlAbort(NULL, dgettext(OlMsgsDomain, 
				"_OlFileChOpenCB(): unsupported "
				"FileChooser operation %d.\n"), 
				my->operation);
			break;
		}
		
		switch (reason) {
		case OL_REASON_OPEN_FOLDER:
			_OlFileChCallbackOpenFolder(fcw, NULL);
			break;
		case OL_REASON_INPUT_DOCUMENT:
			callback_input_document(fcw, NULL);
			/* pop down on open document only */
			popdown_shell(my->shell, FALSE);
			break;
		default:
			_OlAbort(NULL, dgettext(OlMsgsDomain, 
				"_OlFileChOpenCB(): unsupported "
				"FileChooser reason %d.\n"), reason);
			break;
		}
	}
	
	ReleaseToken();
} /* end of _OlFileChOpenCB() */


/**************************************************************************
 *
 *  _OlFileChCommandCB -- Callback procedure for the command button
 * 
 **************************************************************************/

/*ARGSUSED*/
void
_OlFileChCommandCB(
	Widget		widget,		/* unused */
	XtPointer	client_data,
	XtPointer	call_data	/* unused */
)
{
	FileChooserWidget	fcw = (FileChooserWidget)client_data;
	FileChooserPart*	my = &fcw->file_chooser;
	OlStr			string;
	
	assert(NULL != fcw && XtIsSubclass((Widget)fcw, fileChooserWidgetClass));

	switch (my->operation) {
	case OL_INCLUDE:
		if (0 != strcmp(my->chosen_item_node->name, 
				my->go_up_one_folder_label)) {
			callback_input_document(fcw, NULL);
			popdown_shell(my->shell, FALSE);
		}
		break;

	case OL_SAVE:
	/*FALLTHROUGH*/
	case OL_SAVE_AS:
		XtVaGetValues(my->document_name_type_in_widget, XtNstring, 
			&string, NULL);

		callback_output_document(fcw, string);
		break;

	case OL_DO_COMMAND:
		/*! not supported yet !*/
		break;
	}	
} /* end of _OlFileChCommandCB() */


/**************************************************************************
 *
 *  _OlFileChCancelCB -- Callback procedure for the Cancel button
 * 
 **************************************************************************/

/*ARGSUSED*/
void
_OlFileChCancelCB(
	Widget		widget,		/* unused */
	XtPointer	client_data,
	XtPointer	call_data	/* unused */
)
{
	FileChooserWidget	fcw = (FileChooserWidget)client_data;
	FileChooserPart*	my = &fcw->file_chooser;
	
	assert(NULL != fcw && XtIsSubclass((Widget)fcw, fileChooserWidgetClass));

	callback_cancel(fcw);
	popdown_shell(my->shell, TRUE);
} /* end of _OlFileChCancelCB() */


/**************************************************************************
 *
 *	Implementation of Private procedures
 *
 **************************************************************************/


/************************************************************************
 *
 *  _OlFileChCallbackOpenFolder -- 
 *
 ************************************************************************/

void
_OlFileChCallbackOpenFolder(FileChooserWidget fcw, String folder)
{
	static char		request_folder_buf[BIGGEST_PATH] = { '\0' };

	FileChooserPart*		my = &fcw->file_chooser;
	OlFileChFolderCallbackStruct	fc_cdata = { OL_REASON_OPEN_FOLDER, 
						NULL };

	XtVaSetValues(my->open_button_widget, XtNbusy, TRUE, NULL);
	OlUpdateDisplay(my->open_button_widget);

	fc_cdata.operation = my->operation;
	fc_cdata.current_folder = my->current_folder;

	if (NULL == folder) {
		/*
		 * This open folder operation was performed through the 
		 * document list, excluding the go up item.
		 */
		 /*!I18N!*/
		if (0 == strcmp(my->current_folder, "/"))
			(void) snprintf(request_folder_buf, BIGGEST_PATH, "/%s", 
				my->chosen_item_node->name);
		else {
			(void) snprintf(request_folder_buf, BIGGEST_PATH, "%s/%s", 
				my->current_folder, my->chosen_item_node->name);

			/* If we don't want the parent folder of a symbolic
			   link, continue until we reach a non-symbolic link
			   node. */
			if (!my->follow_symlinks)
				_OlFileChChaseSymlinks (request_folder_buf);
		}

		fc_cdata.request_folder = request_folder_buf;
		fc_cdata.request_folder_node = my->chosen_item_node;
	} else {
		/* 
		 * perfomed through the goto button or type-in field,
		 * or through the go up item.
		 */
		fc_cdata.request_folder = folder;
		fc_cdata.request_folder_node = (OlFNavNode)NULL;
			/*! a partial node is available !*/
	}
	
	XtCallCallbackList((Widget)fcw, my->open_folder_callback, &fc_cdata);
			
	/*
	 * Did the application "veto" the folder change?
	 */
	if (NULL != fc_cdata.request_folder) {
		_OL_FREE(my->current_folder); 
		my->current_folder = XtNewString(fc_cdata.request_folder);

		if (_OlFileChFillList((const Widget)fcw)) {
			_OlFileChUpdateCurrentFolderDisplay((Widget)fcw, 
				my->current_folder);
			
			_OlFileChUpdateHistory(my);

			fc_cdata.reason = OL_REASON_FOLDER_OPENED;
			fc_cdata.current_folder = my->current_folder;
	
			XtCallCallbackList((Widget)fcw, 
				my->folder_opened_callback,
				(OlFileChGenericCallbackStruct*)&fc_cdata);
		}
	}

	XtVaSetValues(my->open_button_widget, XtNbusy, FALSE, NULL);
} /* end of _OlFileChCallbackOpenFolder() */


/************************************************************************
 *
 *  _OlFileChFillList -- Fill the scrolling list with the current folder
 *
 ************************************************************************/

Boolean
_OlFileChFillList(const Widget wid)
{
	FileChooserWidget	fcw = (FileChooserWidget)wid;
	FileChooserPart*	my = &fcw->file_chooser;
	_OlSBTree		prev_tree = (_OlSBTree)my->tree;
	Boolean			ok = FALSE;
	Arg			args[2];
	
	do_pre_fill_list(wid);
	my->tree = NULL;

	if (_OlFileNavVisitFolder(my->current_folder, my->euid, my->egid,
			&my->groups, &my->ngroups, my->operation, 
			my->hide_dot_files, my->show_inactive, my->show_glyphs, 
			my->filter_string, my->filter_proc, 
			my->comparison_func,
			(_OlSBTree*)&my->tree, &my->current_folder_stat_bufferp)
	) {
		OlSlistUpdateView(my->document_list_widget, FALSE);
		XtSetArg(args[0], XtNshowMnemonics, OL_NONE);
		XtSetArg(args[1], XtNmnemonicPrefix, None);
		OlSetApplicationValues(my->document_list_widget, args, 1);

		if (my->filled) {
			_OlSBTreeDestruct((_OlSBTree*)&prev_tree);
			OlSlistDeleteAllItems(my->document_list_widget);
		}

		do_go_up_item(my);
		
		_OlSBTreeTraverse((_OlSBTree)my->tree, fill_list_aux, (void*)my, 
			NULL, NULL, NULL);
	
		OlSlistUpdateView(my->document_list_widget, TRUE);	

		XtSetArg(args[0], XtNshowMnemonics, my->show_mnemonics);
		XtSetArg(args[1], XtNmnemonicPrefix, my->mnemonic_modifiers);
		OlSetApplicationValues(my->document_list_widget, args, 1);
	
		if (!my->filled)
			my->filled = TRUE;
		
		ok = TRUE;
	} else
		/*! Notice !*/
		_OlReport(NULL, dgettext(OlMsgsDomain, "_OlFileChFillList(): "
			"Permission to visit folder %s denied.\n"), 
			my->current_folder);

	do_post_fill_list(wid);
	
	if (OL_INCLUDE == my->operation || OL_DO_COMMAND == my->operation) {
		XtVaSetValues(my->command_button_widget, XtNsensitive, 
			FALSE, NULL);
		XtVaSetValues(my->open_button_widget, XtNsensitive, 
			TRUE, NULL);
	}

	return ok;
} /* end of _OlFileChFillList() */


/************************************************************************
 *
 *  _OlFileChUpdateHistory -- 
 *
 ************************************************************************/

void
_OlFileChUpdateHistory(FileChooserPart* my)
{

	if (0 != strcmp(my->current_folder, my->home_folder)) {
		_OlDatumRec	datum_buf;
		_OlDatum	datum = &datum_buf;

		datum->type = _OL_DATUM_TYPE_STRING;
		datum->content  = (void*)my->current_folder;
		
		_OlRingInsert((_OlRing*)&my->ring, datum);
		
		my->folder_index = 0;

		_OlRingTraverse((_OlRing)my->ring, 
			(_OlDatumTraversalFunc)fill_history_aux, 
			(void*)my, NULL, NULL, NULL);
	}
} /* end of _OlFileChUpdateHistory() */


/************************************************************************
 *
 *	Implementation Of This Module's Internal Functions
 *
 ************************************************************************/
 

/************************************************************************
 *
 *  callback_input_document -- 
 *
 ************************************************************************/

static void
callback_input_document(const FileChooserWidget fcw, String document)
{
	const FileChooserPart*		my = &fcw->file_chooser;
	OlFileChDocumentCallbackStruct	fc_cdata = { OL_REASON_INPUT_DOCUMENT, 
						NULL };
	
	fc_cdata.operation = my->operation;
	fc_cdata.current_folder = my->current_folder;

	/*!only if thru a list choice!*/
		fc_cdata.request_document_folder = fc_cdata.current_folder;
		fc_cdata.request_document_node = my->chosen_item_node;
		fc_cdata.request_document = 
			((OlFNavNode)fc_cdata.request_document_node)->name;
	/*! else fc_cdata.request_document = document !*/
	/*! add check to override !*/

	XtCallCallbackList((Widget)fcw, my->input_document_callback, &fc_cdata);
} /* end of callback_input_document() */


/************************************************************************
 *
 *  callback_output_document -- 
 *
 ************************************************************************/

static void
callback_output_document(const FileChooserWidget fcw, String document)
{
	const FileChooserPart*		my = &fcw->file_chooser;
	
	XtVaSetValues(my->command_button_widget, XtNbusy, TRUE, NULL);
	if (_OlStrIsEmpty(document, my->text_format))
		_OlReport(NULL, dgettext(OlMsgsDomain, 
			"_OlFileChCommandCB():  No document name "
			" provided.\n"));
	else {
		OlFileChDocumentCallbackStruct		fc_cdata = { 
			OL_REASON_OUTPUT_DOCUMENT, NULL };

		fc_cdata.operation = my->operation;
		fc_cdata.current_folder = my->current_folder;

		if (OL_DO_COMMAND != my->operation) {
			String		the_folder;
			String		the_document;

			if (_OlFileNavIsAbsolutePath(document)) {

				extract_folder_n_doc(document, &the_folder, 
					&the_document);

				fc_cdata.request_document_folder = the_folder;
				fc_cdata.request_document = the_document;
			} else if (_OlFileNavIsPath(document, my->text_format)) {
				char *path_buf;
				if (path_buf = malloc(BIGGEST_PATH)) {
					(void) snprintf(path_buf, BIGGEST_PATH, "%s/%s", my->current_folder, document);

					extract_folder_n_doc(path_buf, &the_folder, 
						&the_document);

					fc_cdata.request_document_folder = the_folder;
					fc_cdata.request_document = the_document;
					free(path_buf);
				}
			} else {
				fc_cdata.request_document = document;
				fc_cdata.request_document_folder = 
					fc_cdata.current_folder;
			}

			fc_cdata.request_document_node = NULL;
		} else {
			/*! not yet supported !*/

			/*!only if thru a list choice!*/
			fc_cdata.request_document_folder = 
				fc_cdata.current_folder;
			fc_cdata.request_document_node = my->chosen_item_node;
			fc_cdata.request_document = 
			    ((OlFNavNode)fc_cdata.request_document_node)->name;
			
			/*! else fc_cdata.request_document = document !*/
			/*! add check to override !*/
		}

		XtCallCallbackList((Widget)fcw, my->output_document_callback, 
			&fc_cdata);
	}
	XtVaSetValues(my->command_button_widget, XtNbusy, FALSE, NULL);
} /* end of callback_output_document() */


/************************************************************************
 *
 *  callback_cancel -- 
 *
 ************************************************************************/

static void
callback_cancel(const FileChooserWidget fcw)
{
	const FileChooserPart*		my = &fcw->file_chooser;
	OlFileChDocumentCallbackStruct	fc_cdata = { OL_REASON_CANCEL, NULL };
	
	fc_cdata.operation = my->operation;
	fc_cdata.current_folder = my->current_folder;

	XtCallCallbackList((Widget)fcw, my->cancel_callback, &fc_cdata);
} /* end of callback_cancel() */


/************************************************************************
 *
 *  do_pre_fill_list -- 
 *
 ************************************************************************/

static void
do_pre_fill_list(const Widget wid)
{
	const FileChooserWidget	fcw = (const FileChooserWidget)wid;
	FileChooserPart*	my = &fcw->file_chooser;
	Widget			shell = my->shell;

	if (XtIsRealized(shell)) {
		XDefineCursor(XtDisplay(shell), XtWindow(shell),
			OlGetBusyCursor(shell));

		XtVaSetValues(shell, XtNbusy, TRUE, NULL);
	}

	OlUpdateDisplay(shell);
} /* end of do_pre_fill_list() */


/************************************************************************
 *
 *  do_post_fill_list -- 
 *
 ************************************************************************/

static void
do_post_fill_list(const Widget wid)
{
	const FileChooserWidget	fcw = (const FileChooserWidget)wid;
	FileChooserPart*	my = &fcw->file_chooser;
	Widget			shell = my->shell;
	Widget			emanation_widget = XtParent(my->shell);

	if (XtIsRealized(shell)) { 
		XDefineCursor(XtDisplay(shell), XtWindow(shell),
			OlGetStandardCursor(shell));

		XtVaSetValues(shell, XtNbusy, FALSE, NULL);
	}

	OlUpdateDisplay(shell);
} /* end of do_post_fill_list() */


/************************************************************************
 *
 *  do_go_up_item -- 
 *
 ************************************************************************/

static void
do_go_up_item(FileChooserPart* my)
{
	static OlFNavNodeRec	parent_folder_node_buf;
	static OlFNavNode	parent_folder_node = &parent_folder_node_buf;
	static char		parent_folder[BIGGEST_PATH];

	_OlUserInfoRec		u_info_buf;
	_OlUserInfo		u_info = &u_info_buf;
	OlSlistItemAttrs	item_attributes;
	Boolean			reached_file_system_root = FALSE;

	/*! I18N !*/
	if (0 == strcmp(my->current_folder, "/")) {
		reached_file_system_root = TRUE;
		(void) strcpy(parent_folder, "/");
	} else
		(void) strncpy(parent_folder, 
			_OlFileNavParentFolder(my->current_folder), BIGGEST_PATH);

	parent_folder_node->name = my->go_up_one_folder_label;
	parent_folder_node->is_folder = B_TRUE;
	
	u_info->euid = my->euid;
	u_info->egid = my->egid;
	u_info->groups = my->groups;
	u_info->num_groups = my->ngroups;

	parent_folder_node->operational = _OlFileNavCanVisitFolder(
		parent_folder, &u_info_buf);
	
	parent_folder_node->filtered = B_TRUE;
	parent_folder_node->active = parent_folder_node->operational;
	parent_folder_node->glyph = my->go_up_glyph;
	parent_folder_node->sbufp = (OlStat)NULL;

	my->chosen_item_node = parent_folder_node;

	item_attributes.item_label = 
		(OlStr)XtNewString(my->go_up_one_folder_label);
	item_attributes.user_data = (XtPointer)&parent_folder_node_buf;
	item_attributes.flags = OlItemLabel | OlItemUserData | OlItemLabelType;
	
	if (my->show_glyphs) {
		item_attributes.label_type = OL_BOTH;
		item_attributes.item_image = (XImage*)my->go_up_glyph->rep;
		item_attributes.flags = item_attributes.flags | OlItemImage;
	} else
		item_attributes.label_type = OL_STRING;
	
	/* pseudo incremental search */
	if (my->allow_incremental_search)
		item_attributes.item_mnemonic = '.';	/*! I18N !*/
	else
		item_attributes.item_mnemonic = '\0';			
	item_attributes.flags = item_attributes.flags | OlItemMnemonic;

	if (reached_file_system_root) {
		item_attributes.item_sensitive = FALSE;
		item_attributes.item_current = FALSE;
		item_attributes.flags = item_attributes.flags | 
			OlItemSensitive | OlItemCurrent;
	}

	(void) OlSlistAddItem(my->document_list_widget, 
		&item_attributes, NULL);
} /* end of do_go_up_item() */


/************************************************************************
 *
 *  fill_list_aux -- tree traversal auxiliary function
 *
 ************************************************************************/

/*ARGSUSED2*/
static void
fill_list_aux(
	const _OlDatum	datum,
	void*		fcpp,
	void*		arg2,	/* unused */
	void*		arg3,	/* unused */
	void*		arg4	/* unused */
)
{
	FileChooserPart*	my = (FileChooserPart*)fcpp;
	OlSlistItemAttrs	item_attributes;
	OlFNavNode		node;
	OlGlyph			glyph;

	if (_OL_DATUM_TYPE_FNAVNODE != datum->type)
		_OlAbort(NULL, dgettext(OlMsgsDomain, "fill_list_aux(): Corrupt"
			"data.\n"));

	node = (OlFNavNode)datum->content;
	glyph = node->glyph;

	item_attributes.item_label = (OlStr)node->name;
	item_attributes.user_data = (XtPointer)datum->content;
	item_attributes.flags = OlItemLabel | OlItemUserData | OlItemLabelType;
	
	if (my->show_glyphs) {
		item_attributes.label_type = OL_BOTH;
		
		if (NULL == glyph->rep) {
			item_attributes.item_image = (XImage*)(node->is_folder ? 
				my->folder_glyph->rep : my->document_glyph->rep);
			item_attributes.flags = item_attributes.flags | 
				OlItemImage;
		} else if (OL_GLYPH_TYPE_XIMAGE == glyph->type) {
			item_attributes.item_image = (XImage*)&glyph->rep;
			item_attributes.flags = item_attributes.flags | 
				OlItemImage;
		} else
			_OlReport(NULL, dgettext(OlMsgsDomain, 
				"fill_list_aux():  unsupported glyph "
				"representation.\n"));
	} else
		item_attributes.label_type = OL_STRING;

	switch (my->operation) {

	case OL_OPEN:
	/*FALLTHROUGH*/
	case OL_INCLUDE:
		item_attributes.item_sensitive = (Boolean)(node->is_folder || 
			(!node->is_folder && node->filtered));
		break;

	case OL_SAVE:
	/*FALLTHROUGH*/
	case OL_SAVE_AS:
		item_attributes.item_sensitive = (Boolean)node->is_folder;
		break;

	case OL_DO_COMMAND:
		/*! TDB !*/
		break;

	default:
		_OlReport(NULL, dgettext(OlMsgsDomain, "fill_list_aux():  "
			"unsupported mode %d.\n"), my->operation);
		break;
	}
	item_attributes.flags = item_attributes.flags | OlItemSensitive;
	
	/* pseudo incremental search */
	if (my->allow_incremental_search)
		item_attributes.item_mnemonic = 
			*(String)item_attributes.item_label;	/*! I18N !*/
	else
		item_attributes.item_mnemonic = '\0';			
	item_attributes.flags = item_attributes.flags | OlItemMnemonic;
	
	(void) OlSlistAddItem(my->document_list_widget, &item_attributes, NULL);

} /* end of fill_list_aux() */


/************************************************************************
 *
 *  fill_history_aux -- folder history update auxiliary function
 *
 ************************************************************************/

/*ARGSUSED2*/
static void
fill_history_aux(
	const _OlDatum	datum,
	void*		fcpp,
	void*		index,
	void*		arg3,	/* unused */
	void*		arg4	/* unused */
)
{
	FileChooserPart*	my = (FileChooserPart*)fcpp;

	XtVaSetValues(my->history_folders_oba[my->folder_index],
		XtNlabel,	datum->content,
		XtNuserData,	datum->content,
	NULL);

	XtManageChild(my->history_folders_oba[my->folder_index++]);
} /* end of fill_history_aux() */


/************************************************************************
 *
 *  extract_folder_n_doc -- 
 *
 ************************************************************************/

static void
extract_folder_n_doc(String goto_folder, String* the_folder, String* the_doc)
{
	if (goto_folder == (String)NULL) {
		*the_folder = (String)NULL;
		*the_doc = (String)NULL;
	} else {
		String			full_p = XtNewString(goto_folder);
		int			l = strlen(full_p);
		int			i;

		for (i = l - 1; i >= 0; i--)
			if (full_p[i] == '/')
				break;
		if (i < 0) {
			*the_folder = (String)NULL;
			*the_doc = XtNewString(full_p);
		} else if (i == 0) {
			*the_folder = XtNewString("/");
			if (i < l - 1)
				*the_doc = XtNewString(&(full_p[i + 1]));
			else
				*the_doc = (String)NULL;
		} else {
			full_p[i] = '\0';
			*the_folder = XtNewString(full_p);
			if (i < l - 1)
				*the_doc = XtNewString(&(full_p[i + 1]));
			else
				*the_doc = (String)NULL;
		}
		XtFree((XtPointer)full_p);
	}
} /* end of extract_folder_n_doc() */


#ifdef	ENABLE_CRT102_1107044
/************************************************************************
 *
 *  select_given_doc -- 
 *
 ************************************************************************/

static Boolean
select_given_doc(FileChooserPart* fcp, String the_doc)
{
	Widget			swid = fcp->document_list_widget;
	OlSlistItemPtr*		tot_items;
	int			num_tot_items;
	int			i;
	int			prev_cur = -1;

	XtVaGetValues(swid,
			XtNscrollingListItems,	&tot_items,
			XtNnumItems,		&num_tot_items,
		NULL);

	for (i = 0; i < num_tot_items; i++) {
		if (OlSlistIsItemCurrent(swid, tot_items[i]))
			prev_cur = i;
		if (OlSlistGetItemSensitivity(swid, tot_items[i])) {
			String			it_label;

			it_label = OlSlistGetItemLabel(swid, tot_items[i]);
			if (it_label != (String)NULL && 
					!strcmp(it_label, the_doc))
				break;
		}
	}

	if (i != num_tot_items) {
		OlSlistUpdateView(swid, FALSE);
		if (prev_cur != -1)
			OlSlistMakeItemNotCurrent(swid, tot_items[prev_cur], 
				FALSE);

		OlSlistMakeItemCurrent(swid, tot_items[i], TRUE);
		OlSlistFirstViewableItem(swid, tot_items[i]);
		OlSlistUpdateView(swid, TRUE);

		return TRUE;
	}

	return FALSE;
} /* end of select_given_doc() */
#endif	/* ENABLE_CRT102_1107044 */


#ifdef	DEBUG
/************************************************************************
 *
 *  Self-test
 *
 ************************************************************************/


/************************************************************************
 *
 *	Forward Declaration Of Module Private Functions
 *
 ************************************************************************/

static void	construct_basewin(const Widget parent);

static void	open_folder_cb(Widget wid, XtPointer client_data, 
	XtPointer call_data);

static void	document_cb(Widget wid, XtPointer client_data, 
	XtPointer call_data);

static void	list_choice_cb(Widget wid, XtPointer client_data, 
	XtPointer call_data);

static void	generic_cb(Widget wid, XtPointer client_data, 
	XtPointer call_data);


/************************************************************************
 *
 *  construct_basewin -- Populate the top level panel
 *
 ************************************************************************/

static void
construct_basewin(const Widget parent)
{
	static XtCallbackRec	open_folder_cbr[] = {
		{ (XtCallbackProc)open_folder_cb, (XtPointer)NULL },
		{ NULL, NULL }
	};
	static XtCallbackRec	input_document_cbr[] = {
		{ (XtCallbackProc)document_cb, 
			(XtPointer)XtNinputDocumentCallback },
		{ NULL, NULL }
	};
	static XtCallbackRec	output_document_cbr[] = {
		{ (XtCallbackProc)document_cb, 
			(XtPointer)XtNoutputDocumentCallback },
		{ NULL, NULL }
	};
	static XtCallbackRec	list_choice_cbr[] = {
		{ (XtCallbackProc)list_choice_cb, (XtPointer)NULL },
		{ NULL, NULL }
	};
	static XtCallbackRec	folder_opened_cbr[] = {
		{ (XtCallbackProc)generic_cb, 
			(XtPointer)XtNfolderOpenedCallback },
		{ NULL, NULL }
	};
	static XtCallbackRec	cancel_cbr[] = {
		{ (XtCallbackProc)generic_cb, (XtPointer)XtNcancelCallback },
		{ NULL, NULL }
	};

	Widget 			test_fc;


	test_fc = XtVaCreateManagedWidget("test_fc", fileChooserWidgetClass, 
		parent, 
			XtNopenFolderCallback,		open_folder_cbr,
			XtNinputDocumentCallback,	input_document_cbr, 
			XtNoutputDocumentCallback,	output_document_cbr, 
			XtNlistChoiceCallback,		list_choice_cbr,
			XtNfolderOpenedCallback,	folder_opened_cbr, 
			XtNcancelCallback,		cancel_cbr,
		NULL);

} /* end of construct_basewin() */


/************************************************************************
 *
 *  open_folder_cb -- 
 *
 ************************************************************************/

/*ARGSUSED1*/
static void
open_folder_cb(
	Widget			wid,
	XtPointer		client_data,	/* unused */
	XtPointer		call_data
)
{
	const OlFileChFolderCallbackStruct* cb_datap = 
		(OlFileChFolderCallbackStruct*)call_data;

	(void) fprintf(stderr, 
		"\nopen_folder_cb() from %s/%s with data: \n"
		"reason =\t%d\n" 
		"extension =\t%d\n" 
		"operation =\t%d\n" 
		"current folder =\t%s\n",
		
		XtName(XtParent(wid)), XtName(wid), 
		cb_datap->reason,
		cb_datap->extension, 
		cb_datap->operation, 
		cb_datap->current_folder
	);

	(void) fprintf(stderr, 
		"request folder =\t%s\n" 
		"request folder node =\t", 
	
		cb_datap->request_folder
	);

	_OlFNavNodePrint(cb_datap->request_folder_node);
	(void) putchar('\n');

} /* end of open_folder_cb() */


/************************************************************************
 *
 *  document_cb -- 
 *
 ************************************************************************/

static void
document_cb(
	Widget			wid,
	XtPointer		client_data,
	XtPointer		call_data
)
{
	const OlFileChDocumentCallbackStruct* cb_datap = 
		(OlFileChDocumentCallbackStruct*)call_data;
	
	const char*const	cb_resource_name = (const char*const)client_data;

	(void) fprintf(stderr, 
		"\ndocument_cb() for XtN%s from %s/%s with data: \n"
		"reason =\t%d\n" 
		"extension =\t%d\n" 
		"operation =\t%d\n" 
		"current folder =\t%s\n",
		
		cb_resource_name, XtName(XtParent(wid)), XtName(wid), 
		cb_datap->reason,
		cb_datap->extension, 
		cb_datap->operation, 
		cb_datap->current_folder
	);

	switch (cb_datap->reason) {
	case OL_REASON_INPUT_DOCUMENT:
		(void) fprintf(stderr, 
			"request document folder =\t%s\n"
			"request document =\t%s\n"
			"request document node =\t", 
			
			cb_datap->request_document_folder,
			cb_datap->request_document
		);
		_OlFNavNodePrint(cb_datap->request_document_node);
		(void) putchar('\n');
		break;
	case OL_REASON_OUTPUT_DOCUMENT:
		if (OL_DO_COMMAND == cb_datap->operation) {
			(void) fprintf(stderr, 
				"request document folder =\t%s\n"
				"request document =\t%s\n"
				"request document node =\t", 
				
				cb_datap->request_document_folder,
				cb_datap->request_document
			);
			_OlFNavNodePrint(cb_datap->request_document_node);
			(void) putchar('\n');
		} else
			(void) fprintf(stderr, "request document =\t%s\n", 
				cb_datap->request_document);
		break;
	}
} /* end of document_cb() */


/************************************************************************
 *
 *  list_choice_cb -- 
 *
 ************************************************************************/

/*ARGSUSED1*/
static void
list_choice_cb(
	Widget			wid,
	XtPointer		client_data,	/* unused */
	XtPointer		call_data
)
{
	const OlFileChListChoiceCallbackStruct* cb_datap = 
		(OlFileChListChoiceCallbackStruct*)call_data;

	(void) fprintf(stderr, 
		"\nopen_folder_cb() from %s/%s with data: \n"
		"reason =\t%d\n" 
		"extension =\t%d\n" 
		"operation =\t%d\n"
		"current folder =\t%s\n",
		
		XtName(XtParent(wid)), XtName(wid), 
		cb_datap->reason,
		cb_datap->extension, 
		cb_datap->operation, 
		cb_datap->current_folder
	);

	(void) fprintf(stderr, 
		"chosen name =\t%s\n" 
		"chosen item node =\t", 
	
		cb_datap->chosen_item_node->name
	);

	_OlFNavNodePrint(cb_datap->chosen_item_node);
	(void) putchar('\n');
} /* end of list_choice_cb() */


/************************************************************************
 *
 *  generic_cb -- 
 *
 ************************************************************************/

static void
generic_cb(
	Widget			wid,
	XtPointer		client_data,
	XtPointer		call_data
)
{
	const OlFileChGenericCallbackStruct* cb_datap = 
		(OlFileChGenericCallbackStruct*)call_data;

	const char*const	cb_resource_name = (const char*const)client_data;

	(void) fprintf(stderr, 
		"\ngeneric_cb() for XtN%s from %s/%s with data: \n"
		"reason =\t%d\n"
		"operation =\t%d\n"
		"current folder =\t%s\n\n",
		
		cb_resource_name, XtName(XtParent(wid)), XtName(wid), 
		cb_datap->reason, 
		cb_datap->operation, 
		cb_datap->current_folder
	);

} /* end of generic_cb() */


/************************************************************************
 *
 *  _OlFileChooserTest
 *
 ************************************************************************/

void
_OlFileChTest(const int argc, const char* const argv[])
{
	const char* fallback_resources[] = { 
		"*Background:			gray",
		NULL
	};

	_OlMain(argc, argv, fallback_resources, construct_basewin);	
} /* end of _OlFileChooserTest() */

#endif	/* DEBUG */

static char *get_realpath (const char *file_name, char *resolved_name )
{
    extern char *	filech_strtok ();
    char *tok;

    /* intialize resolved_path */
    resolved_name[0] = '\0';


    /* easy case:  check for "/" */
    if ( (file_name [0] == '/') && (file_name [1] == '\0') ) {
	strcpy(resolved_name, "/");
	return resolved_name;
    }


    tok = filech_strtok(file_name, "/");
    while ( tok ) {
	if ( (tok [0] == '.') && (tok [1] == '\0') ) {
	    /* EMPTY */;
	} else if ( (tok [0] == '.') && (tok [1] == '.')
		&& (tok [2] == '\0') ) {
	    char *slash = strrchr(resolved_name, '/');

	    if ( !slash )
		(void) strcpy(resolved_name, "/");
	    else if ( slash != resolved_name )
		*slash = '\0';
	    else /* if ( slash == resolved_name) ) */
		resolved_name[1] = '\0';
	} else {
	    if ( !( (resolved_name [0] == '/')
		&& (resolved_name [1] == '\0') ) )
		(void) strcat(resolved_name, "/");
	    (void) strcat(resolved_name, tok);
	}
	tok = filech_strtok(NULL, "/");
    }

    return (resolved_name);
}

/*
 * Using our own version of strtok() so as not to interfere with
 * any other uses of strtok() in the application.
 * 
 */

static char *filech_strtok (char *token_string, const char *sep_string)
{
    char	*q, *r;
    static char	*save_pos;

    /*
     * If not first call, use saved pointer into string
     */
    if (token_string == NULL)  {
        token_string = save_pos;
    }

    /* 
     * If no more tokens left, return
     */
    if (token_string == 0)  {
        return(NULL);
    }

    /* 
     * skip leading separators 
     */
    q = token_string + strspn(token_string, sep_string);

    /* 
     * return if no tokens remaining 
     */
    if (*q == '\0')  {
        return(NULL);
    }

    /* 
     * move past token 
     */
    if((r = strpbrk(q, sep_string)) == NULL)  {
        /* 
	 * indicate this is last token 
	 */
        save_pos = 0;
    }
    else {
        *r = '\0';
        save_pos = r+1;
    }
    return(q);
}

static void _OlFileChChaseSymlinks (char *fname)
{
	struct stat st;
	int lsize;
	char *lbuf;

	if (lbuf = (char *) malloc(BIGGEST_PATH + 1)) {

		while ( ( (lstat (fname, &st) ) != -1) && (st.st_mode & S_IFLNK)
			&& ( (lsize = readlink (fname, (void *) lbuf,
			(size_t) BIGGEST_PATH) ) != -1) ) {

			/* readlink doesn't terminate the string. */
			lbuf [lsize] = '\0';
			if (_OlFileNavIsAbsolutePath (lbuf) )
				get_realpath (lbuf, fname);
			else {
				char *parent;

				if (parent = (char *) malloc(BIGGEST_PATH + 1)) {
					strncpy (parent, fname, BIGGEST_PATH);
					*strrchr (parent, '/') = '\0';
					snprintf (fname, BIGGEST_PATH, "%s/%s", parent, lbuf);
					get_realpath (fname, lbuf);
					strncpy (fname, lbuf, BIGGEST_PATH);
					free(parent);
				}
			}
		}
		free(lbuf);
	}
	return;
}


/************************************************************************
 *
 *  popdown_shell
 *
 ************************************************************************/
 
static void
popdown_shell(const Widget shell, const Boolean override_pushpin)
{
	if(XtIsSubclass(shell, popupWindowShellWidgetClass))
		_OlPopupWiShPopDown(shell, override_pushpin);
	else if (XtIsSubclass(shell, fileChooserShellWidgetClass))
		_OlFileChShPopDown(shell, override_pushpin);
}

/* end of FileChImpl.c */
