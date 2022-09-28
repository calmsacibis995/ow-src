#pragma ident	"@(#)FileCh.c	1.16    97/03/26 lib/libXol SMI"    /* OLIT	493	*/

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
 *	Implementation of the file chooser panel widget.
 *		
 **************************************************************************/


/**************************************************************************
 *
 *      Imported interfaces 
 *
 **************************************************************************/

#include <errno.h>      	/* errno */
#include <libintl.h>		/* dgettext() */
#include <limits.h>		/* MAX_PATH */
#include <string.h>

#ifndef	MAX_PATH
	#include <sys/param.h>	/* MAXPATHLEN */
#endif	/* MAX_PATH */

#include <sys/types.h>  	/* boolean_t */
#include <unistd.h>  		/* getcwd() */

#include <X11/IntrinsicP.h>
#include <X11/StringDefs.h>

#include <Xol/Datum.h>
#include <Xol/FileChImpl.h>
#include <Xol/FileChInit.h>
#include <Xol/Form.h>
#include <Xol/OpenLookP.h>
#include <Xol/Ring.h>
#include <Xol/RootShellP.h>	/* _OlGetScale() */
#include <Xol/SBTree.h>
#include <Xol/diags.h>
#include <Xol/filenav.h>

#include <Xol/FileChP.h>	/* interface of this implementation */


/**************************************************************************
 *
 *	Module private manifest constants and macros definitions
 *
 **************************************************************************/

/* Magic numbers */
#ifdef	MAX_PATH
	#define	BIGGEST_PATH	(MAX_PATH + 1)
#else
	#define	BIGGEST_PATH	(MAXPATHLEN)
#endif	/* MAX_PATH */


/**************************************************************************
 *
 * Forward procedure declarations listed by category:
 *
 *		1. action procedures
 *		2. class procedures
 *		3. callback procedures
 *		4. private procedures
 *
 **************************************************************************/

					/* class procedures/methods */

static Boolean	AcceptFocus(Widget wid, Time* timep);
static void	ClassInitialize(void);
static void	Destroy(Widget widget);

static void	Realize(Widget wid, Mask* value_maskp, 
	XSetWindowAttributes* attributesp);

static void	Redisplay(Widget wid, XEvent* xeventp, Region region);
static void	Resize(Widget wid);
static Widget	RegisterFocus(Widget wid);

static Boolean	SetValues(Widget oldw, Widget request, Widget neww,
	ArgList args, Cardinal* num_argsp);

					/* internal functions */

static void	set_default_scale(Widget wid, int offset, XrmValue* value);


/**************************************************************************
 *
 *  Define the resource list associated with the widget instance
 *
 **************************************************************************/


#define OFFSET(member)	XtOffsetOf(FileChooserRec, file_chooser.member)

static XtResource	resources[] =
{

/* I18N */

	{ XtNtextFormat, XtCTextFormat, XtROlStrRep,
		sizeof (OlStrRep), OFFSET(text_format),
		XtRCallProc, (XtPointer)_OlGetDefaultTextFormat },

/* State */

	{ XtNfont, XtCFont, XtROlFont, 
    		sizeof (OlFont), OFFSET(font), 
		XtRString, XtDefaultFont },

	{ XtNfontColor, XtCFontColor, XtRPixel, 
		sizeof (Pixel), OFFSET(font_color), 
		XtRString, XtDefaultForeground }, 

	{ XtNforeground, XtCForeground, XtRPixel, 
		sizeof (Pixel), OFFSET(foreground),
		XtRString, XtDefaultForeground, },

    	{ XtNinputFocusColor, XtCInputFocusColor, XtRPixel,
    		sizeof (Pixel), OFFSET(input_focus_color),
    		XtRCallProc, (XtPointer)_OlGetDefaultFocusColor },

	{ XtNscale, XtCScale, XtROlScale,
		sizeof (int), OFFSET(scale), 
		XtRCallProc, (XtPointer)set_default_scale },

	{ XtNoperation, XtCOperation, XtROlDefine,
		sizeof (OlDefine), OFFSET(operation),
		XtRImmediate, (XtPointer)OL_OPEN },

	{ XtNcurrentFolder, XtCFolderName, XtRString,
		sizeof (String), OFFSET(current_folder),
		XtRString, "." },

	{ XtNlastDocumentName, XtCDocumentName, XtRString,
		sizeof (String), OFFSET(last_document_name),
		XtRImmediate, (XtPointer)NULL },

	{ XtNlistVisibleItemMinCount, XtCVisibleItemMinCount, XtRCardinal,
		sizeof (Cardinal), OFFSET(list_visible_item_min_count),
		XtRImmediate, (XtPointer)3 },

	{ XtNlistVisibleItemCount, XtCVisibleItemCount, XtRCardinal,
		sizeof (Cardinal), OFFSET(list_visible_item_count),
		XtRImmediate, (XtPointer)10 },

	{ XtNfollowSymlinks, XtCBooleanDefault, XtRBoolean,
		sizeof (Boolean), OFFSET(follow_symlinks),
		XtRImmediate, (XtPointer)TRUE },

	{ XtNshowGlyphs, XtCBooleanDefault, XtRBoolean,
		sizeof (Boolean), OFFSET(show_glyphs),
		XtRImmediate, (XtPointer)TRUE },

	{ XtNnoTypeInAcceleration, XtCBooleanDefault, XtRBoolean,
		sizeof (Boolean), OFFSET(no_type_in_acceleration),
		XtRImmediate, (XtPointer)FALSE },

	{ XtNallowIncrementalSearch, XtCBooleanDefault, XtRBoolean,
		sizeof (Boolean), OFFSET(allow_incremental_search),
		XtRImmediate, (XtPointer)FALSE },

/* Standard Callbacks */

	{ XtNopenFolderCallback, XtCCallback, XtRCallback,
		sizeof (XtCallbackList), OFFSET(open_folder_callback),
		XtRImmediate, (XtPointer)NULL },

	{ XtNinputDocumentCallback, XtCCallback, XtRCallback,
		sizeof (XtCallbackList), OFFSET(input_document_callback),
		XtRImmediate, (XtPointer)NULL },

	{ XtNoutputDocumentCallback, XtCCallback, XtRCallback,
		sizeof (XtCallbackList), OFFSET(output_document_callback),
		XtRImmediate, (XtPointer)NULL },

/* Filtering */

	{ XtNfilterString, XtCFilterString, XtRString,
		sizeof (String), OFFSET(filter_string),
		XtRImmediate, (XtPointer)NULL },

	{ XtNfilterProc, XtCFilterProc, XtRCallback,
		sizeof (XtCallbackProc), OFFSET(filter_proc),
		XtRImmediate, (XtPointer)NULL },

	{ XtNshowInactive, XtCBooleanDefault, XtRBoolean,
		sizeof (Boolean), OFFSET(show_inactive),
		XtRImmediate, (XtPointer)TRUE },

	{ XtNhideDotFiles, XtCBooleanDefault, XtRBoolean,
		sizeof (Boolean), OFFSET(hide_dot_files),
		XtRImmediate, (XtPointer)TRUE },

/* Go to */

	{ XtNhomeFolder, XtCHomeFolder, XtRString,
		sizeof (String), OFFSET(home_folder),
		XtRString, "~" },

	{ XtNapplicationFoldersMaxCount, XtCFolderMaxCount, XtRCardinal,
		sizeof (Cardinal), OFFSET(application_folders_max_count),
		XtRImmediate, (XtPointer)5 },

	{ XtNapplicationFolders, XtCFolders, XtROlFolderList,
		sizeof (OlFolderList), OFFSET(application_folders),
		XtRImmediate, (XtPointer)NULL },

	{ XtNuserFoldersMaxCount, XtCFolderMaxCount, XtRCardinal,
		sizeof (Cardinal), OFFSET(user_folders_max_count),
		XtRImmediate, (XtPointer)5 },

	{ XtNuserFolders, XtCFolders, XtROlFolderList,
		sizeof (OlFolderList), OFFSET(user_folders),
		XtRImmediate, (XtPointer)NULL },

	{ XtNhistoryFoldersMinCount, XtCFolderMinCount, XtRCardinal,
		sizeof (Cardinal), OFFSET(history_folders_min_count),
		XtRImmediate, (XtPointer)3 },

	{ XtNhistoryFoldersMaxCount, XtCFolderMaxCount, XtRCardinal,
		sizeof (Cardinal), OFFSET(history_folders_max_count),
		XtRImmediate, (XtPointer)15 },

	{ XtNhistoryFolders, XtCFolders, XtROlFolderList,
		sizeof (OlFolderList), OFFSET(history_folders),
		XtRImmediate, (XtPointer)NULL },

/* Sorting */

	{ XtNcomparisonFunc, XtCComparisonFunc, XtROlComparisonFunc,
		sizeof (OlComparisonFunc), OFFSET(comparison_func),
		XtRImmediate, (XtPointer)OlSortStrNoCaseAscending },

/* Path Name Processing */

	{ XtNexpandTilde, XtCBooleanDefault, XtRBoolean,
		sizeof (Boolean), OFFSET(expand_tilde),
		XtRImmediate, (XtPointer)TRUE },

	{ XtNsubstituteShellVariables, XtCBooleanDefault, XtRBoolean,
		sizeof (Boolean), OFFSET(substitute_shell_variables),
		XtRImmediate, (XtPointer)TRUE },

/* Accelerators */

	{ XtNgotoHomeAccelerator, XtCAccelerator, XtRString,
		sizeof (String), OFFSET(goto_home_accelerator),
		XtRImmediate, (XtPointer)NULL },

	{ XtNgoUpOneFolderAccelerator, XtCAccelerator, XtRString,
		sizeof (String), OFFSET(go_up_one_folder_accelerator),
		XtRImmediate, (XtPointer)NULL },

	{ XtNcancelAccelerator, XtCAccelerator, XtRString,
		sizeof (String), OFFSET(cancel_accelerator),
		XtRImmediate, (XtPointer)NULL },

	{ XtNopenAccelerator, XtCAccelerator, XtRString,
		sizeof (String), OFFSET(open_accelerator),
		XtRImmediate, (XtPointer)NULL },

	{ XtNopenFolderAccelerator, XtCAccelerator, XtRString,
		sizeof (String), OFFSET(open_folder_accelerator),
		XtRImmediate, (XtPointer)NULL },

	{ XtNsaveAccelerator, XtCAccelerator, XtRString,
		sizeof (String), OFFSET(save_accelerator),
		XtRImmediate, (XtPointer)NULL },

	{ XtNsaveAsAccelerator, XtCAccelerator, XtRString,
		sizeof (String), OFFSET(save_as_accelerator),
		XtRImmediate, (XtPointer)NULL },

	{ XtNincludeAccelerator, XtCAccelerator, XtRString,
		sizeof (String), OFFSET(include_accelerator),
		XtRImmediate, (XtPointer)NULL },

	{ XtNcommandAccelerator, XtCAccelerator, XtRString,
		sizeof (String), OFFSET(command_accelerator),
		XtRImmediate, (XtPointer)NULL },

/* Mnemonics */

	{ XtNgotoHomeMnemonic, XtCMnemonic, OlRChar,
		sizeof (char), OFFSET(goto_home_mnemonic),
		XtRImmediate, (XtPointer)'\0' },

	{ XtNcancelMnemonic, XtCMnemonic, OlRChar,
		sizeof (char), OFFSET(cancel_mnemonic),
		XtRImmediate, (XtPointer)'\0' },

	{ XtNopenMnemonic, XtCMnemonic, OlRChar,
		sizeof (char), OFFSET(open_mnemonic),
		XtRImmediate, (XtPointer)'\0' },

	{ XtNopenFolderMnemonic, XtCMnemonic, OlRChar,
		sizeof (char), OFFSET(open_folder_mnemonic),
		XtRImmediate, (XtPointer)'\0' },

	{ XtNsaveMnemonic, XtCMnemonic, OlRChar,
		sizeof (char), OFFSET(save_mnemonic),
		XtRImmediate, (XtPointer)'\0' },

	{ XtNsaveAsMnemonic, XtCMnemonic, OlRChar,
		sizeof (char), OFFSET(save_as_mnemonic),
		XtRImmediate, (XtPointer)'\0' },

	{ XtNincludeMnemonic, XtCMnemonic, OlRChar,
		sizeof (char), OFFSET(include_mnemonic),
		XtRImmediate, (XtPointer)'\0' },

	{ XtNcommandMnemonic, XtCMnemonic, OlRChar,
		sizeof (char), OFFSET(command_mnemonic),
		XtRImmediate, (XtPointer)'\0' },

/* Extension Container */

	{ XtNextensionName, XtCExtensionName, XtRString,
		sizeof (String), OFFSET(extension_name),
		XtRImmediate, (XtPointer)NULL },

	{ XtNextensionClass, XtCExtensionClass, XtRWidgetClass,
		sizeof (WidgetClass), OFFSET(extension_class),
		XtRImmediate, (XtPointer)&formWidgetClass },

	{ XtNextensionWidget, XtCExtensionWidget, XtRWidget,
		sizeof (Widget), OFFSET(extension_widget),
		XtRImmediate, (XtPointer)NULL },

/* Component Access */

	{ XtNgotoPromptWidget, XtCComponentWidget, XtRWidget,
		sizeof (Widget), OFFSET(goto_prompt_widget),
		XtRImmediate, (XtPointer)NULL },

	{ XtNgotoButtonWidget, XtCComponentWidget, XtRWidget,
		sizeof (Widget), OFFSET(goto_button_widget),
		XtRImmediate, (XtPointer)NULL },

	{ XtNgotoMenuWidget, XtCComponentWidget, XtRWidget,
		sizeof (Widget), OFFSET(goto_menu_widget),
		XtRImmediate, (XtPointer)NULL },

	{ XtNgotoHomeButtonWidget, XtCComponentWidget, XtRWidget,
		sizeof (Widget), OFFSET(goto_home_button_widget),
		XtRImmediate, (XtPointer)NULL },

	{ XtNgotoTypeInWidget, XtCComponentWidget, XtRWidget,
		sizeof (Widget), OFFSET(goto_type_in_widget),
		XtRImmediate, (XtPointer)NULL },

	{ XtNcurrentFolderLabelWidget, XtCComponentWidget, XtRWidget,
		sizeof (Widget), OFFSET(current_folder_label_widget),
		XtRImmediate, (XtPointer)NULL },

	{ XtNcurrentFolderWidget, XtCComponentWidget, XtRWidget,
		sizeof (Widget), OFFSET(current_folder_widget),
		XtRImmediate, (XtPointer)NULL },

	{ XtNlistPromptWidget, XtCComponentWidget, XtRWidget,
		sizeof (Widget), OFFSET(list_prompt_widget),
		XtRImmediate, (XtPointer)NULL },

	{ XtNdocumentListWidget, XtCComponentWidget, XtRWidget,
		sizeof (Widget), OFFSET(document_list_widget),
		XtRImmediate, (XtPointer)NULL },

	{ XtNdocumentNameTypeInWidget, XtCComponentWidget, XtRWidget,
		sizeof (Widget), OFFSET(document_name_type_in_widget),
		XtRImmediate, (XtPointer)NULL },

	{ XtNopenButtonWidget, XtCComponentWidget, XtRWidget,
		sizeof (Widget), OFFSET(open_button_widget),
		XtRImmediate, (XtPointer)NULL },

	{ XtNcommandButtonWidget, XtCComponentWidget, XtRWidget,
		sizeof (Widget), OFFSET(command_button_widget),
		XtRImmediate, (XtPointer)NULL },

	{ XtNcancelButtonWidget, XtCComponentWidget, XtRWidget,
		sizeof (Widget), OFFSET(cancel_button_widget),
		XtRImmediate, (XtPointer)NULL },

/* Extensibility Callbacks */

	{ XtNlistChoiceCallback, XtCCallback, XtRCallback,
		sizeof (XtCallbackList), OFFSET(list_choice_callback),
		XtRImmediate, (XtPointer)NULL },

	{ XtNfolderOpenedCallback, XtCCallback, XtRCallback,
		sizeof (XtCallbackList), OFFSET(folder_opened_callback),
		XtRImmediate, (XtPointer)NULL },

	{ XtNcancelCallback, XtCCallback, XtRCallback,
		sizeof (XtCallbackList), OFFSET(cancel_callback),
		XtRImmediate, (XtPointer)NULL },

/* Labels */

	#ifndef	XGETTEXT
		#define	I18N(string)	(XtPointer)string
	#else
		#define	I18N(string)	dgettext(OlMsgsDomain, string)
	#endif	/* XGETTEXT */

	{ XtNgotoPromptString, XtCPromptString, XtROlStr,
		sizeof (OlStr), OFFSET(goto_prompt_string),
		XtRLocaleString, I18N(
			"Type in the path to the folder and press Return.") },

	{ XtNgotoLabel, XtCLabel, XtROlStr,
		sizeof (OlStr), OFFSET(goto_label),
		XtRLocaleString, I18N("Go To:") },

	{ XtNgotoHomeLabel, XtCLabel, XtROlStr,
		sizeof (OlStr), OFFSET(goto_home_label),
		XtRLocaleString, I18N("Home") },

	{ XtNcurrentFolderLabelString, XtCLabel, XtROlStr,
		sizeof (OlStr), OFFSET(current_folder_label_string),
		XtRLocaleString, I18N("Current Folder:") },

	{ XtNopenPromptString, XtCPromptString, XtROlStr,
		sizeof (OlStr), OFFSET(open_prompt_string),
		XtRLocaleString, I18N(
			"Select a document or folder and click %.") },

	{ XtNfolderPromptString, XtCPromptString, XtROlStr,
		sizeof (OlStr), OFFSET(folder_prompt_string),
		XtRLocaleString, I18N(
			"Select a folder and click %.") },

	{ XtNdocumentPromptString, XtCPromptString, XtROlStr,
		sizeof (OlStr), OFFSET(document_prompt_string),
		XtRLocaleString, I18N(
			"Select a document and click %.") },

	{ XtNcommandPromptString, XtCPromptString, XtROlStr,
		sizeof (OlStr), OFFSET(command_prompt_string),
		XtRLocaleString, I18N(
		"Select a document and click % or a folder and click %.") },

	{ XtNgoUpOneFolderLabel, XtCLabel, XtROlStr,
		sizeof (OlStr), OFFSET(go_up_one_folder_label),
		XtRLocaleString, I18N("...Go up one folder...") },

	{ XtNopenLabel, XtCLabel, XtROlStr,
		sizeof (OlStr), OFFSET(open_label),
		XtRLocaleString, I18N("Open") },

	{ XtNopenFolderLabel, XtCLabel, XtROlStr,
		sizeof (OlStr), OFFSET(open_folder_label),
		XtRLocaleString, I18N("Open Folder") },

	{ XtNcancelLabel, XtCLabel, XtROlStr,
		sizeof (OlStr), OFFSET(cancel_label),
		XtRLocaleString, I18N("Cancel") },

	{ XtNsaveLabel, XtCLabel, XtROlStr,
		sizeof (OlStr), OFFSET(save_label),
		XtRLocaleString, I18N("Save") },

	{ XtNsaveAsLabel, XtCLabel, XtROlStr,
		sizeof (OlStr), OFFSET(save_as_label),
		XtRLocaleString, I18N("Save As") },

	{ XtNincludeLabel, XtCLabel, XtROlStr,
		sizeof (OlStr), OFFSET(include_label),
		XtRLocaleString, I18N("Include") },

	{ XtNcommandLabel, XtCLabel, XtROlStr,
		sizeof (OlStr), OFFSET(command_label),
		XtRLocaleString, I18N("Command") },


	{ XtNdefaultDocumentName, XtCDefaultDocumentName, XtROlStr,
		sizeof (OlStr), OFFSET(default_document_name),
		XtRLocaleString, I18N("Untitled1") },

	{ XtNdefaultDocumentSuffix, XtCDefaultDocumentSuffix, XtROlStr,
		sizeof (OlStr), OFFSET(default_document_suffix),
		XtRLocaleString, I18N(".1") },

	#undef I18N

/* Private Resources */
/*
	{ XtNshowMnemonics, XtCShowAccelerators, XtROlDefine, 
		sizeof (OlDefine), Offset(display.show_mnemonics), 
		XtRImmediate, (XtPointer)OL_NONE },

	{ XtNmnemonicPrefix, XtCMnemonicPrefix, XtRModifiers, 
		sizeof (Modifiers), Offset(display.mnemonic_modifiers), 
		XtRString, (XtPointer)"None" },
*/
}; /* end of resources */

#undef OFFSET

/* Dynamic resources */

#define BYTE_OFFSET	XtOffsetOf(FileChooserRec, \
				file_chooser.dynamic_resources_flags)
static _OlDynResource dynamic_resources[] = {
	{ 	
		{ XtNforeground, XtCForeground, XtRPixel,
			sizeof (Pixel), 0,
	 		XtRString, XtDefaultForeground },
	 	BYTE_OFFSET,
	 	OL_FILECH_FOREGROUND, NULL
	 },
	 
	{
		{ XtNfontColor, XtCFontColor, XtRPixel,
			sizeof(Pixel), 0,
	 		XtRString, XtDefaultForeground },
	 	BYTE_OFFSET,
	 	OL_FILECH_FONTCOLOR, NULL
	 }
}; /* end of dynamic_resources */

#undef BYTE_OFFSET


/**************************************************************************
 *
 * Define class record structure to be initialized at compile time
 *
 **************************************************************************/

FileChooserClassRec fileChooserClassRec = {

	{	/* core_class fields */

		/* superclass */	(WidgetClass)&rubberTileClassRec,
		/* class_name */	"FileChooser",
		/* size */		sizeof (FileChooserRec),
		/* Class initializer */	ClassInitialize,
		/* class_part_initialize */
					NULL,
		/* Class initialized? */
					FALSE,
		/* initialize */	_OlFileChInitialize,
		/* initialize_hook */	NULL,
		/* realize */		Realize,
		/* actions */		NULL,
		/* num_actions */	0,
		/* resources */		resources,
		/* resource_count */	XtNumber(resources),
		/* xrm_class */		NULLQUARK,
		/* compress_motion */	TRUE,
		/* compress_exposure */	XtExposeCompressMaximal,
		/* compress_enterleave */
					TRUE,
		/* visible_interest */	FALSE,
		/* destroy */		Destroy,
		/* resize */		Resize,
		/* expose */		Redisplay,
		/* set_values */	SetValues,
		/* set_values_hook */	NULL,
		/* set_values_almost */	XtInheritSetValuesAlmost,
		/* get_values_hook */	NULL,
		/* accept_focus */	AcceptFocus,
		/* intrinsics version */
					XtVersion,
		/* callback offsets */	NULL,
		/* tm_table */		XtInheritTranslations,
		/* query_geometry */	NULL,
		/* display_accelerator */
					NULL,
		/* extension */		NULL,

	}, {	/* composite_class fields */

		/* geometry_manager */	XtInheritGeometryManager,
		/* change_managed */	XtInheritChangeManaged,
		/* insert_child */	XtInheritInsertChild,
		/* delete_child */	XtInheritDeleteChild,
		/* extension */		NULL

	}, {	/* constraint_class fields */

		/* subresources */	NULL,
		/* subresources_count */
					0,
		/* constraint_size */	sizeof(FileChooserConstraintRec),
		/* initialize */	NULL,
		/* destroy */		NULL,
		/* set_values */	NULL,
		/* extension */		NULL

	}, {	/* manager_class fields */

		/* highlight_handler */	NULL,
		/* reserved */		NULL,
		/* reserved */		NULL,
		/* traversal_handler */	XtInheritTraversalHandler,
		/* activate */		XtInheritActivateFunc,
		/* event_procs */	NULL,
		/* num_event_procs */	0,
		/* register_focus */	RegisterFocus,
		/* reserved */		NULL,
		/* version */		OlVersion,
		/* extension */		NULL,
		/* dyn_data */		{ dynamic_resources,
						 XtNumber(dynamic_resources) },
		/* transparent_proc */	XtInheritTransparentProc,

	}, {	/* rubber_tile_class fields */

		/* no_class_fields */	NULL

	}, {	/* file_chooser_class fields */

		/* extension */		NULL
	}
}; /* end of fileChooserClassRec */


/**************************************************************************
 *
 *	Public Widget Class Definition Of The Widget Class Record
 *
 **************************************************************************/

WidgetClass	fileChooserWidgetClass = (WidgetClass)&fileChooserClassRec;


/**************************************************************************
 *
 *	Class Procedures
 *
 **************************************************************************/


/**************************************************************************
 *
 *  AcceptFocus --
 *
 *	Pass along request to the proper component
 * 
 **************************************************************************/

static Boolean
AcceptFocus(Widget wid, Time* timep)
{
	FileChooserWidget	fcw = (FileChooserWidget)wid;
	FileChooserPart*	my = &(fcw->file_chooser);
	Widget			initial_focus_widget;

	if (OL_OPEN || OL_INCLUDE == my->operation)
		initial_focus_widget = my->goto_type_in_widget;
	else
		initial_focus_widget = my->document_name_type_in_widget;

	return (OlCallAcceptFocus(initial_focus_widget, *timep));

} /* end of AcceptFocus() */


/**************************************************************************
 *
 *  ClassInitialize --  Register OlDefine string values
 * 
 **************************************************************************/

static void
ClassInitialize(void)
{

	_OlAddOlDefineType("open", OL_OPEN);
	_OlAddOlDefineType("save", OL_SAVE);
	_OlAddOlDefineType("save_as", OL_SAVE_AS);
	_OlAddOlDefineType("include", OL_INCLUDE);
	_OlAddOlDefineType("do_command", OL_DO_COMMAND);

} /* end of ClassInitialize() */


/**************************************************************************
 *
 *  Destroy --
 * 
 **************************************************************************/

static void
Destroy(Widget widget)
{
	FileChooserWidget	fcw = (FileChooserWidget)widget;
	FileChooserPart*	my = &(fcw->file_chooser);
	OlDefine		operation = my->operation;

	_OlStringDestruct(&my->current_folder);
	_OlStringDestruct(&my->filter_string);
	_OlStringDestruct(&my->home_folder);
	
	_OlStringDestruct(&my->goto_home_accelerator);
	_OlStringDestruct(&my->go_up_one_folder_accelerator);

	_OlStringDestruct(&my->extension_name);

	_OlStrDestruct(&my->goto_prompt_string);
	_OlStrDestruct(&my->goto_label);
	_OlStrDestruct(&my->goto_home_label);
	
	_OlStrDestruct(&my->current_folder_label_string);
	
	_OlStrDestruct(&my->list_prompt_string);
	_OlStrDestruct(&my->go_up_one_folder_label);

	_OlStrDestruct(&my->document_name_type_in_label);
	_OlStrDestruct(&my->document_name_type_in_string);

	_OlStrDestruct(&my->cancel_label);
	_OlStringDestruct(&my->cancel_accelerator);

	XtRemoveAllCallbacks(widget, XtNcancelCallback);

	XtRemoveAllCallbacks(widget, XtNopenFolderCallback);
	XtRemoveAllCallbacks(widget, XtNfolderOpenedCallback);

	XtRemoveAllCallbacks(widget, XtNlistChoiceCallback);

	if (
		OL_SAVE		== operation ||
		OL_SAVE_AS	== operation ||
		OL_INCLUDE	== operation ||
		OL_DO_COMMAND	== operation
	) {
		_OlStringDestruct(&my->open_folder_accelerator);
		_OlStrDestruct(&my->open_folder_label);
	}

	if (
		OL_SAVE		== operation ||
		OL_SAVE_AS	== operation ||
		OL_DO_COMMAND	== operation
	)
		XtRemoveAllCallbacks(widget, XtNoutputDocumentCallback);

	if (
		OL_OPEN		== operation ||
		OL_INCLUDE	== operation
	)
		XtRemoveAllCallbacks(widget, XtNinputDocumentCallback);

	switch (operation) {

	case OL_OPEN:
		_OlStringDestruct(&my->open_accelerator);
		_OlStrDestruct(&my->open_label);

		_OlStrDestruct(&my->open_prompt_string);
		break;

	case OL_SAVE:
		_OlStrDestruct(&my->default_document_name);

		_OlStringDestruct(&my->save_accelerator);
		_OlStrDestruct(&my->save_label);

		_OlStrDestruct(&my->folder_prompt_string);
		_OlStrDestruct(&my->document_name_type_in_label);
		break;

	case OL_SAVE_AS:
		_OlStringDestruct(&my->last_document_name);
		_OlStrDestruct(&my->default_document_suffix);
		
		_OlStringDestruct(&my->save_as_accelerator);
		_OlStrDestruct(&my->save_as_label);
		
		_OlStrDestruct(&my->folder_prompt_string);
		_OlStrDestruct(&my->document_name_type_in_label);
		_OlStrDestruct(&my->document_name_type_in_string);
		break;

	case OL_INCLUDE:
		_OlStringDestruct(&my->include_accelerator);
		_OlStrDestruct(&my->include_label);

		_OlStrDestruct(&my->command_prompt_string);
		break;

	case OL_DO_COMMAND:
		_OlStringDestruct(&my->command_accelerator);
		_OlStrDestruct(&my->command_label);

		_OlStrDestruct(&my->command_prompt_string);
		_OlStrDestruct(&my->document_name_type_in_label);
		break;
	}


	_OL_FREE(my->user_folders);
	_OL_FREE(my->user_folders_oba);
	_OL_FREE(my->application_folders);
	_OL_FREE(my->application_folders_oba);
	_OL_FREE(my->history_folders_oba);

	/*!
	XtCallbackProc		filter_proc;
	OlComparisonFunc	comparison_func;
	WidgetClass		extension_class;
	!*/

	_OlFNavNodeDestruct(&my->chosen_item_node);
	_OlStatDestruct(&my->home_folder_stat_bufferp);
	_OlStatDestruct(&my->current_folder_stat_bufferp);

	_OlSBTreeDestruct((_OlSBTree*)&my->tree);
	_OlRingDestruct((_OlRing*)&my->ring);

	_OlGlyphDestruct(&my->folder_glyph);
	_OlGlyphDestruct(&my->document_glyph);
	_OlGlyphDestruct(&my->go_up_glyph);

	_OL_FREE(my->groups);

} /* end of Destroy() */


/**************************************************************************
 *
 *  Realize --
 * 
 **************************************************************************/

static void
Realize(Widget wid, Mask* value_maskp, XSetWindowAttributes* attributesp)
{
	XtRealizeProc		super_realize;

	super_realize = XtSuperclass(wid)->core_class.realize;

	if ((XtRealizeProc)NULL != super_realize) {
		(*super_realize)(wid, value_maskp, attributesp);
	} else
		_OlReport(NULL, dgettext(OlMsgsDomain, "Realize():  "
			"FileChooser's superclass has no realize procedure."));
} /* end of Realize() */


/**************************************************************************
 *
 *  Redisplay --
 * 
 **************************************************************************/

/*ARGSUSED1*/
static void
Redisplay(
	Widget		wid, 
	XEvent*		xeventp,	/* unused */
	Region		region		/* unused */
)
{
	const FileChooserWidget	fcw = (const FileChooserWidget)wid;
	FileChooserPart*	my = &fcw->file_chooser;

	#ifdef	REFRESH
	/* Populating the list is delayed to first (and every) mapping */
	_OlFileChFillList(wid, FALSE);
	#endif	/* REFRESH */

} /* end of Redisplay() */


/**************************************************************************
 *
 * Resize -- 
 * 
 **************************************************************************/

static void
Resize(Widget wid)
{
	FileChooserWidget	fcw = (FileChooserWidget)wid;
	FileChooserPart*	my = &fcw->file_chooser;
	XtWidgetProc		super_resize;
	int			num_visible_items;

	XtVaGetValues(my->document_list_widget,
			XtNviewHeight,		&num_visible_items, 
		NULL);
	
	if (num_visible_items < my->list_visible_item_min_count)
		XtVaSetValues(my->document_list_widget,
			XtNviewHeight,		my->list_visible_item_min_count, 
		NULL);

	super_resize = XtSuperclass(wid)->core_class.resize;

	if ((XtWidgetProc)NULL != super_resize) {
		(*super_resize)(wid);
	} else
		_OlReport(NULL, dgettext(OlMsgsDomain, "Resize():  "
			"FileChooser's superclass has no resize procedure."));

	_OlFileChUpdateCurrentFolderDisplay(wid, NULL);

} /* end of Resize() */


/**************************************************************************
 *
 *  RegisterFocus -- return widget to register on Shell
 * 
 **************************************************************************/

static Widget
RegisterFocus(Widget wid)
{
	FileChooserWidget	fcw = (FileChooserWidget)wid;
	FileChooserPart*	my = &(fcw->file_chooser);
	Widget			initial_focus_widget;

	if (OL_OPEN || OL_INCLUDE == my->operation)
		initial_focus_widget = my->goto_type_in_widget;
	else
		initial_focus_widget = my->document_name_type_in_widget;

	return initial_focus_widget;
} /* end of RegisterFocus() */


/**************************************************************************
 *
 *  SetValues --
 * 
 **************************************************************************/

/*ARGSUSED1*/
static Boolean
SetValues(
	Widget		oldw,
	Widget		request,	/* unused */
	Widget		neww,
	ArgList		args,
	Cardinal*	num_argsp
)
{
	FileChooserWidget	old_fcw = (FileChooserWidget)oldw;
	CorePart*		old_core = &old_fcw->core;
	ManagerPart*		old_manager = &old_fcw->manager;
	FileChooserPart*	old = &old_fcw->file_chooser;
	FileChooserPart*	old_file_chooser = &old_fcw->file_chooser;

	FileChooserWidget	new_fcw = (FileChooserWidget)neww;
	CorePart*		new_core = &new_fcw->core;
	ManagerPart*		new_manager = &new_fcw->manager;
	FileChooserPart*	new = &new_fcw->file_chooser;
	FileChooserPart*	new_file_chooser = &new_fcw->file_chooser;

	OlDefine		operation = old->operation;


	#define HAS_CHANGED(member) \
		(old->member != new->member)


	/******************************************************************
	 *
	 *  Enforce G and IG Resources --
	 * 
	 ******************************************************************/

	#define DEFEAT_CHANGE(member) \
		if (HAS_CHANGED(member)) { \
			new->member = old->member; \
			_OlReport(NULL, \
				dgettext(OlMsgsDomain, "SetValues():  " \
					"ignored attempt to set the value of " \
					"an initialization time resource " \
					#member ".\n")); \
		}

	/* IG */
	DEFEAT_CHANGE(text_format);
	DEFEAT_CHANGE(scale);
	DEFEAT_CHANGE(operation);
	DEFEAT_CHANGE(list_visible_item_min_count);
	DEFEAT_CHANGE(list_visible_item_count);
	
	DEFEAT_CHANGE(application_folders_max_count);
	DEFEAT_CHANGE(user_folders_max_count);
	DEFEAT_CHANGE(history_folders_min_count);
	DEFEAT_CHANGE(history_folders_max_count);
	DEFEAT_CHANGE(history_folders);			/* G */
	
	DEFEAT_CHANGE(extension_name);	/*!ptr!*/
	DEFEAT_CHANGE(extension_class);
	DEFEAT_CHANGE(extension_widget);

	DEFEAT_CHANGE(goto_prompt_widget);
	DEFEAT_CHANGE(goto_button_widget);
	DEFEAT_CHANGE(goto_menu_widget);
	DEFEAT_CHANGE(goto_home_button_widget);
	DEFEAT_CHANGE(goto_type_in_widget);
	
	DEFEAT_CHANGE(current_folder_label_widget);
	DEFEAT_CHANGE(current_folder_widget);
	DEFEAT_CHANGE(list_prompt_widget);

	DEFEAT_CHANGE(document_list_widget);

	if (	
		OL_SAVE		== operation ||
		OL_SAVE_AS	== operation ||
		OL_DO_COMMAND	== operation
	) 
		DEFEAT_CHANGE(document_name_type_in_widget);

	
	DEFEAT_CHANGE(open_button_widget);
	DEFEAT_CHANGE(cancel_button_widget);

	if (OL_OPEN != operation) 
		DEFEAT_CHANGE(command_button_widget);

	if (	
		OL_SAVE		!= operation &&
		OL_DO_COMMAND	!= operation
	) 
		DEFEAT_CHANGE(default_document_name);	/*!ptr!*/

	if (	
		OL_SAVE_AS	!= operation
	) 
		DEFEAT_CHANGE(default_document_suffix);	/*!ptr!*/


	/******************************************************************
	 *
	 *  Dispatch Resource Changes To The Approriate Components --
	 * 
	 ******************************************************************/

	#define MGR_HAS_CHANGED(member) \
		(old_mgr->member != new_mgr->member)

	/******************************************************************
	 *
	 *  Dispatch Core and Manager Resource Changes --
	 * 
	 ******************************************************************/

	#define HAS_CHANGED_IN_PART(member, part) \
		(old_##part->member != new_##part->member)

	#define	SET_RES(part, resource, field, widget_field) \
		XtVaSetValues(new->widget_field, \
			resource,		new_##part->field, \
		NULL)

	#define	UPDATE_RESOURCE(prt, res, fld) \
		if (HAS_CHANGED_IN_PART(fld, prt)) { \
			SET_RES(prt, res, fld, goto_f); \
			SET_RES(prt, res, fld, goto_prompt_widget); \
			SET_RES(prt, res, fld, goto_button_widget); \
			SET_RES(prt, res, fld, goto_type_in_widget); \
			SET_RES(prt, res, fld, goto_home_button_widget); \
			SET_RES(prt, res, fld, current_folder_f); \
			SET_RES(prt, res, fld, current_folder_label_widget); \
			SET_RES(prt, res, fld, current_folder_widget); \
			SET_RES(prt, res, fld, list_prompt_widget); \
			SET_RES(prt, res, fld, document_list_widget); \
			\
		        if ( \
		                OL_SAVE         == operation || \
		                OL_SAVE_AS      == operation || \
		                OL_DO_COMMAND   == operation \
		        ) { \
				SET_RES(prt, res, fld, document_f); \
				SET_RES(prt, res, fld, \
					document_name_type_in_widget); \
			} \
			\
			if (NULL != old->extension_name) \
				SET_RES(prt, res, fld, extension_widget); \
			\
			SET_RES(prt, res, fld, command_center_rt); \
			SET_RES(prt, res, fld, open_button_widget); \
			SET_RES(prt, res, fld, cancel_button_widget); \
			\
			if (OL_OPEN != operation) \
				SET_RES(prt, res, fld, command_button_widget); \
		}
	
	UPDATE_RESOURCE(core, XtNbackground, background_pixel);
	UPDATE_RESOURCE(core, XtNbackgroundPixmap, background_pixmap);
	UPDATE_RESOURCE(core, XtNborderColor, border_pixel);
	UPDATE_RESOURCE(core, XtNborderPixmap, border_pixmap);
	UPDATE_RESOURCE(core, XtNborderWidth, border_width);

	UPDATE_RESOURCE(file_chooser, XtNinputFocusColor, input_focus_color);

	#define	SET_MGR_RES(widget_field) \
		SET_RES(manager, XtNtraversalOn, traversal_on, widget_field);

	if (HAS_CHANGED_IN_PART(traversal_on, manager)) {
		SET_MGR_RES(goto_f);
		SET_MGR_RES(goto_button_widget);
		SET_MGR_RES(goto_type_in_widget);
		SET_MGR_RES(goto_home_button_widget);
		SET_MGR_RES(current_folder_f);
		SET_MGR_RES(document_list_widget);

	        if (
	                OL_SAVE         == operation ||
	                OL_SAVE_AS      == operation ||
	                OL_DO_COMMAND   == operation
	        ) {
			SET_MGR_RES(document_f);
			SET_MGR_RES(document_name_type_in_widget);
		}

		if (NULL != old->extension_name)
			SET_MGR_RES(extension_widget);

		SET_MGR_RES(command_center_rt);
		SET_MGR_RES(open_button_widget);
		SET_MGR_RES(cancel_button_widget);

		if (OL_OPEN != operation)
			SET_MGR_RES(command_button_widget);
	}
	
	#undef	HAS_CHANGED_IN_PART
	#undef	SET_RES
	#undef	UPDATE_RESOURCE
	#undef	SET_MGR_RES

	/******************************************************************
	 *
	 *  Dispatch FileChooser Resource Changes --
	 * 
	 ******************************************************************/

	/******************************************************************
	 *
	 *  State Resources Changes --
	 * 
	 ******************************************************************/

	if (
		HAS_CHANGED(show_inactive) ||
		HAS_CHANGED(hide_dot_files) ||
		HAS_CHANGED(show_glyphs)
	)
		_OlFileChFillList(neww);

	/******************************************************************
	 *
	 *  Label/String Resources Changes --
	 * 
	 ******************************************************************/

  	#define	CHANGE_REQUESTED(resource) \
		_OlInArgList(resource, args, *num_argsp)

     	/*
	 * Change internal copies of all OlStr-type strings obtained from the
	 * resource manager.
	 */
	#define	CHANGE_OLSTR(member) \
		{ \
			 _OlStrConstruct(&new->member, old->member); \
			_OlStrDestruct(&old->member); \
		}
	
	#define	UPDATE_LABEL(resource, field, widget_field) \
		if (CHANGE_REQUESTED(resource)) { \
			CHANGE_OLSTR(field); \
			XtVaSetValues(new->widget_field, \
				XtNstring,	new->field, \
			NULL); \
		}

	UPDATE_LABEL(XtNgotoPromptString, goto_prompt_string, goto_prompt_widget);
	UPDATE_LABEL(XtNgotoLabel, goto_label, goto_button_widget);
	UPDATE_LABEL(XtNgotoHomeLabel, goto_home_label, goto_home_button_widget);

	UPDATE_LABEL(XtNcurrentFolderLabelString, current_folder_label_string, 
		current_folder_label_widget);

	#undef	UPDATE_LABEL

	if (CHANGE_REQUESTED(XtNopenPromptString))
		if (OL_OPEN == new->operation) {
			CHANGE_OLSTR(open_prompt_string);
			_OlFileChSetListPrompt(neww);
		} else
			_OlReport(NULL, dgettext(OlMsgsDomain, "SetValues():  "
				"ignored request to set the value of a "
				"resource that is not relevant to the current "
				"file chooser operation.\n"));

	if (CHANGE_REQUESTED(XtNfolderPromptString))
		if (
			OL_SAVE == new->operation || 
			OL_SAVE_AS == new->operation
		) {
			CHANGE_OLSTR(folder_prompt_string);
			 _OlFileChSetListPrompt(neww);
		} else
			_OlReport(NULL, dgettext(OlMsgsDomain, "SetValues():  "
				"ignored request to set the value of a "
				"resource that is not relevant to the current "
				"file chooser operation.\n"));

	if (CHANGE_REQUESTED(XtNdocumentPromptString))
		if (
			OL_SAVE		== new->operation || 
			OL_SAVE_AS	== new->operation
		) {
			CHANGE_OLSTR(document_prompt_string);
		} else
			_OlReport(NULL, dgettext(OlMsgsDomain, "SetValues():  "
				"ignored request to set the value of a "
				"resource that is not relevant to the current "
				"file chooser operation.\n"));

	if (CHANGE_REQUESTED(XtNcommandPromptString))
		if (	
			OL_INCLUDE	== new->operation ||
			OL_DO_COMMAND	== new->operation
		) {
			CHANGE_OLSTR(command_prompt_string);
			_OlFileChSetListPrompt(neww);
		} else
			_OlReport(NULL, dgettext(OlMsgsDomain, "SetValues():  "
				"ignored request to set the value of a "
				"resource that is not relevant to the current "
				"file chooser operation.\n"));

	if (CHANGE_REQUESTED(XtNgoUpOneFolderLabel)) 
		CHANGE_OLSTR(go_up_one_folder_label);

	if (CHANGE_REQUESTED(XtNcancelLabel)) {
		CHANGE_OLSTR(cancel_label);
		XtVaSetValues(new->cancel_button_widget, 
			XtNlabel,	new->cancel_label, 
		NULL);
	}

	if (CHANGE_REQUESTED(XtNopenLabel))
		if (OL_OPEN == new->operation) {
			CHANGE_OLSTR(open_label);
			XtVaSetValues(new->open_button_widget, 
				XtNlabel,	new->open_label, 
			NULL);
		} else
			_OlReport(NULL, dgettext(OlMsgsDomain, "SetValues():  "
				"ignored request to set the value of a "
				"resource that is not relevant to the current "
				"file chooser operation.\n"));

	if (CHANGE_REQUESTED(XtNopenFolderLabel))
		if (OL_OPEN != new->operation) {
			CHANGE_OLSTR(open_folder_label);
			XtVaSetValues(new->open_button_widget, 
				XtNlabel,	new->open_folder_label, 
			NULL);
		} else
			_OlReport(NULL, dgettext(OlMsgsDomain, "SetValues():  "
				"ignored request to set the value of a "
				"resource that is not relevant to the current "
				"file chooser operation.\n"));

	if (CHANGE_REQUESTED(XtNsaveLabel))
		if (OL_SAVE == new->operation) {
			CHANGE_OLSTR(save_label);
			XtVaSetValues(new->command_button_widget, 
				XtNlabel,	new->save_label,
			NULL);
			_OlFileChSetDocumentLabel(neww);
		} else
			_OlReport(NULL, dgettext(OlMsgsDomain, "SetValues():  "
				"ignored request to set the value of a "
				"resource that is not relevant to the current "
				"file chooser operation.\n"));

	if (CHANGE_REQUESTED(XtNsaveAsLabel))
		if (OL_SAVE_AS == new->operation) {
			CHANGE_OLSTR(save_as_label);
			XtVaSetValues(new->command_button_widget, 
				XtNlabel,	new->save_as_label,
			NULL);
			_OlFileChSetDocumentLabel(neww);
		} else
			_OlReport(NULL, dgettext(OlMsgsDomain, "SetValues():  "
				"ignored request to set the value of a "
				"resource that is not relevant to the current "
				"file chooser operation.\n"));

	if (CHANGE_REQUESTED(XtNincludeLabel))
		if (OL_INCLUDE == new->operation) {
			CHANGE_OLSTR(include_label);
			XtVaSetValues(new->command_button_widget, 
				XtNlabel,	new->include_label, 
			NULL);
		} else
			_OlReport(NULL, dgettext(OlMsgsDomain, "SetValues():  "
				"ignored request to set the value of a "
				"resource that is not relevant to the current "
				"file chooser operation.\n"));

	if (CHANGE_REQUESTED(XtNcommandLabel))
		if (OL_DO_COMMAND == new->operation) {
			CHANGE_OLSTR(command_label);
			XtVaSetValues(new->command_button_widget, 
				XtNlabel,	new->command_label, 
			NULL);
			_OlFileChSetDocumentLabel(neww);
		} else
			_OlReport(NULL, dgettext(OlMsgsDomain, "SetValues():  "
				"ignored request to set the value of a "
				"resource that is not relevant to the current "
				"file chooser operation.\n"));

	if (	
		CHANGE_REQUESTED(XtNdefaultDocumentName) || 
		CHANGE_REQUESTED(XtNdefaultDocumentSuffix)
	)
		if (	
			OL_SAVE		== new->operation || 
			OL_SAVE_AS	== new->operation || 
			OL_DO_COMMAND	== new->operation
		) {
			_OlFileChSetDocumentString(neww);

			XtVaSetValues(new->command_button_widget, 
					XtNsensitive,	
						_OlStrIsEmpty(
						new->default_document_name, 
						new->text_format) ? FALSE : TRUE,
				NULL);
		} else
			_OlReport(NULL, dgettext(OlMsgsDomain, "SetValues():  "
				"value of a resource that is not relevant to "
				"the current file chooser operation was "
				"set.\n"));

	if (CHANGE_REQUESTED(XtNlastDocumentName))
		if (OL_SAVE_AS == new->operation) {
			_OlFileChSetDocumentString(neww);

			XtVaSetValues(new->command_button_widget, 
					XtNsensitive,	
						_OlStrIsEmpty(
						new->last_document_name, 
						new->text_format) ? FALSE : TRUE,
				NULL);
		} else
			_OlReport(NULL, dgettext(OlMsgsDomain, "SetValues():  "
				"value of a resource that is not relevant to "
				"the current file chooser operation was "
				"set.\n"));


	/******************************************************************
	 *
	 *  Accelerator Resources Changes --
	 * 
	 ******************************************************************/

	if (CHANGE_REQUESTED(XtNgotoHomeAccelerator))
		XtVaSetValues(new->goto_home_button_widget, 
			XtNaccelerator,		new->goto_home_accelerator, 
		NULL);

	/*! XtNgoUpOneFolderAccelerator !*/

	if (CHANGE_REQUESTED(XtNcancelAccelerator))
		XtVaSetValues(new->cancel_button_widget, 
			XtNaccelerator,		new->cancel_accelerator, 
		NULL);

	if (CHANGE_REQUESTED(XtNopenAccelerator))
		if (OL_OPEN == new->operation)
			XtVaSetValues(new->open_button_widget, 
				XtNaccelerator,	new->open_accelerator,
			NULL);
		else
			_OlReport(NULL, dgettext(OlMsgsDomain, "SetValues():  "
				"ignored request to set the value of a "
				"resource that is not relevant to the current "
				"file chooser operation.\n"));

	if (CHANGE_REQUESTED(XtNopenFolderAccelerator))
		if (OL_OPEN != new->operation)
			XtVaSetValues(new->open_button_widget, 
				XtNaccelerator,	new->open_folder_accelerator,
			NULL);
		else
			_OlReport(NULL, dgettext(OlMsgsDomain, "SetValues():  "
				"ignored request to set the value of a "
				"resource that is not relevant to the current "
				"file chooser operation.\n"));

	if (CHANGE_REQUESTED(XtNsaveAccelerator))
		if (OL_SAVE == new->operation)
			XtVaSetValues(new->command_button_widget, 
				XtNaccelerator,	new->save_accelerator,
			NULL);
		else
			_OlReport(NULL, dgettext(OlMsgsDomain, "SetValues():  "
				"ignored request to set the value of a "
				"resource that is not relevant to the current "
				"file chooser operation.\n"));

	if (CHANGE_REQUESTED(XtNsaveAsAccelerator))
		if (OL_SAVE_AS == new->operation)
			XtVaSetValues(new->command_button_widget, 
				XtNaccelerator,	new->save_as_accelerator,
			NULL);
		else
			_OlReport(NULL, dgettext(OlMsgsDomain, "SetValues():  "
				"ignored request to set the value of a "
				"resource that is not relevant to the current "
				"file chooser operation.\n"));

	if (CHANGE_REQUESTED(XtNincludeAccelerator))
		if (OL_INCLUDE == new->operation)
			XtVaSetValues(new->command_button_widget, 
				XtNaccelerator,	new->include_accelerator, 
			NULL);
		else
			_OlReport(NULL, dgettext(OlMsgsDomain, "SetValues():  "
				"ignored request to set the value of a "
				"resource that is not relevant to the current "
				"file chooser operation.\n"));

	if (CHANGE_REQUESTED(XtNcommandAccelerator))
		if (OL_DO_COMMAND == new->operation)
			XtVaSetValues(new->command_button_widget, 
				XtNaccelerator,	new->command_accelerator, 
			NULL);
		else
			_OlReport(NULL, dgettext(OlMsgsDomain, "SetValues():  "
				"ignored request to set the value of a "
				"resource that is not relevant to the current "
				"file chooser operation.\n"));

	/******************************************************************
	 *
	 *  Mnemonic Resources Changes --
	 * 
	 ******************************************************************/

	if (CHANGE_REQUESTED(XtNgotoHomeMnemonic))
		XtVaSetValues(new->goto_home_button_widget, 
			XtNmnemonic,		new->goto_home_mnemonic, 
		NULL);

	if (CHANGE_REQUESTED(XtNcancelMnemonic))
		XtVaSetValues(new->cancel_button_widget, 
			XtNmnemonic,		new->cancel_mnemonic,
		NULL);

	if (CHANGE_REQUESTED(XtNopenMnemonic))
		if (OL_OPEN == new->operation)
			XtVaSetValues(new->open_button_widget, 
				XtNmnemonic,	new->open_mnemonic,
			NULL);
		else
			_OlReport(NULL, dgettext(OlMsgsDomain, "SetValues():  "
				"ignored request to set the value of a "
				"resource that is not relevant to the current "
				"file chooser operation.\n"));

	if (CHANGE_REQUESTED(XtNopenFolderMnemonic))
		if (OL_OPEN != new->operation)
			XtVaSetValues(new->open_button_widget, 
				XtNmnemonic,	new->open_folder_mnemonic,
			NULL);
		else
			_OlReport(NULL, dgettext(OlMsgsDomain, "SetValues():  "
				"ignored request to set the value of a "
				"resource that is not relevant to the current "
				"file chooser operation.\n"));

	if (CHANGE_REQUESTED(XtNsaveMnemonic))
		if (OL_SAVE == new->operation)
			XtVaSetValues(new->command_button_widget, 
				XtNmnemonic,	new->save_mnemonic,
			NULL);
		else
			_OlReport(NULL, dgettext(OlMsgsDomain, "SetValues():  "
				"ignored request to set the value of a "
				"resource that is not relevant to the current "
				"file chooser operation.\n"));

	if (CHANGE_REQUESTED(XtNsaveAsMnemonic))
		if (OL_SAVE_AS == new->operation)
			XtVaSetValues(new->command_button_widget, 
				XtNmnemonic,	new->save_as_mnemonic,
			NULL);
		else
			_OlReport(NULL, dgettext(OlMsgsDomain, "SetValues():  "
				"ignored request to set the value of a "
				"resource that is not relevant to the current "
				"file chooser operation.\n"));

	if (CHANGE_REQUESTED(XtNincludeMnemonic))
		if (OL_INCLUDE == new->operation)
			XtVaSetValues(new->command_button_widget, 
				XtNmnemonic,	new->include_mnemonic,
			NULL);
		else
			_OlReport(NULL, dgettext(OlMsgsDomain, "SetValues():  "
				"ignored request to set the value of a "
				"resource that is not relevant to the current "
				"file chooser operation.\n"));

	if (CHANGE_REQUESTED(XtNcommandMnemonic))
		if (OL_DO_COMMAND == new->operation)
			XtVaSetValues(new->command_button_widget, 
				XtNmnemonic,	new->command_mnemonic,
			NULL);
		else
			_OlReport(NULL, dgettext(OlMsgsDomain, "SetValues():  "
				"ignored request to set the value of a "
				"resource that is not relevant to the current "
				"file chooser operation.\n"));

	/* if current_folder has been reset refresh the list content */
	if (CHANGE_REQUESTED(XtNcurrentFolder)) {
		if (!_OlFileChVerifyFolderValue(&new->current_folder)) {
			String		path_bufp;

			_OlReport(NULL, dgettext(OlMsgsDomain, 
				"_OlFileChVerifyFolderValue():  invalid "
				"folder value %s.\n"
				"Using current working directory instead.\n"), 
				new->current_folder);
	
			if (NULL != 
				(path_bufp = _OlFileNavExpandedPathName("."))) {
				_OlStringConstruct(&new->current_folder, 
					path_bufp);
			} else
				_OlAbort(NULL, dgettext(OlMsgsDomain, 
					"_OlFileChVerifyFolderValue():  unable "
					"to determine the current working "
					"directory.\n"));
			}
		_OlFileChCallbackOpenFolder(new_fcw, new->current_folder);
	}

	if (CHANGE_REQUESTED(XtNhomeFolder))
		(void) _OlFileChVerifyFolderValue(&new->home_folder);
	/*!
	XtNfilterString
	XtNfilterProc
	!*/

	/*! 
	XtNapplicationFolders
	XtNuserFolders
	verify_application_folders(new);
	verify_user_folders(new);
	::: preprocess pathname goto folder names, converter?
	!*/

	return TRUE;

	#undef	HAS_CHANGED
	#undef	DEFEAT_CHANGE
	#undef	CHANGE_REQUESTED
	#undef	CHANGE_OLSTR
} /* end of SetValues() */


/**************************************************************************
 *
 *	Implemenation of Module-Private Functions
 * 
 **************************************************************************/

/**************************************************************************
 *
 *  set_default_scale -- 
 * 
 **************************************************************************/

/*ARGSUSED1*/
static void
set_default_scale(
	Widget		wid,
	int		offset,		/* unused */
	XrmValue*	value
)
{
	static int		def;
	Widget			ssw = _OlGetScreenShellOfWidget(wid);

	if ((Widget)NULL != ssw)
		XtVaGetValues(ssw, XtNscale, &def, NULL);
	else
		def = OL_DEFAULT_POINT_SIZE;

	value->size = sizeof (int);
	value->addr = (XtPointer)&def;
} /* end of set_default_scale() */


/* end of FileCh.c */
