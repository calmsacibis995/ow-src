#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <Xol/OpenLook.h>
#include <Xol/TextEdit.h>
#include <Xol/ScrolledWi.h>
#include <Xol/RubberTile.h>
#include <Xol/MenuButton.h>
#include <Xol/OblongButt.h>
#include <Xol/StaticText.h>
#include <Xol/DropTarget.h>
#include <Xol/Exclusives.h>
#include <Xol/RectButton.h>

#define	TEXT_UNMODIFIED	FALSE
#define TEXT_MODIFIED	TRUE

static	Widget	toplevel, base, textedit;
static	char	*saved_text = NULL;

/* calback to hide the window */
hide_frame()
{
	XtPopdown(base);
}

/* calback to expose the window */
show_frame()
{
	XtPopup(base, XtGrabNone);
}

/* calback to place the data in the text window */
display_data(char *text)
{
	XtVaSetValues(textedit, 
		XtNsourceType,		(XtArgVal)OL_STRING_SOURCE,
		XtNsource,		(XtArgVal)text,
		XtNuserData,		(XtArgVal)TEXT_UNMODIFIED,
		XtNdisplayPosition,	(XtArgVal)0,
		XtNcursorPosition,	(XtArgVal)0,
		XtNselectStart,		(XtArgVal)0,
		XtNselectEnd,		(XtArgVal)0,
		NULL);

	if (saved_text != NULL)
	{
		XtFree(saved_text);
	}

	OlTextEditCopyBuffer((TextEditWidget)textedit, &saved_text);
}

/* calback to get the data from the text window */
Boolean
get_data(char **text, int *len)
{
	if (!OlTextEditCopyBuffer((TextEditWidget)textedit, text))
	{
		OlWarning("getData: error trying to copy textedit buffer\n");
		return(FALSE);
	}

	*len = strlen(*text);
	return(TRUE);

}

/* calback to check if the data in the text window is modified */
Boolean
data_is_modified(void)
{
	int	text_state;

	XtVaGetValues(textedit, XtNuserData, &text_state, NULL);

	return(text_state);
}

/* calback to restore the data of the text window to its last unmodified 
 * state
 */
restore_data()
{
	if(data_is_modified())
	{
		display_data(saved_text);
	}
}

/* calback to clear the data in the text window */
clear_data()
{
	OlTextEditClearBuffer((TextEditWidget)textedit);
}

/* calback to quit this application */
void
quit(void)
{
	quit_tt();
	exit(0);
}

/* Save button calback to save back this data  */
static void
saveTextCB(Widget w, XtPointer client_data, XtPointer callData)
{
	save_tt();
}

/* Quit button calback to quit  */
static void
quitTextCB(Widget w, XtPointer client_data, XtPointer callData)
{
	quit_tt();
	exit(0);
}

/* calback to called when we get a tooltalk message  */
static void
handleMessageCB(Widget w, XtPointer client_data, XtPointer callData)
{
	handle_tt_message();
}

/* main initialization routine */
main(int argc, char **argv)
{
	XtAppContext    appContext;
	Widget	control, scrolledwin;
	Widget	save_btn, quit_btn;
	Widget	blank;
	int	fd;

	/* check if we were started by tooltalk */
	check_tt_startup(&argc, &argv);

	/* initialize and build the widgets for the application */
	OlToolkitInitialize((XtPointer) NULL);
	toplevel = XtAppInitialize(&appContext, "AsciiEdit",
				(XrmOptionDescList) NULL,
				0, &argc, argv, (String *) NULL,
				(ArgList) NULL, 0);

	base = XtVaCreateManagedWidget("base",
			rubberTileWidgetClass,
			toplevel,
			NULL);

	control = XtVaCreateManagedWidget("control",
			rubberTileWidgetClass,
			base,
			NULL);

	save_btn = XtVaCreateManagedWidget("save",
			oblongButtonWidgetClass,
			control,
			NULL);

	blank = XtVaCreateManagedWidget("blank",
			staticTextWidgetClass, 
			control,
			NULL);

	quit_btn = XtVaCreateManagedWidget("quit",
			oblongButtonWidgetClass,
			control,
			NULL);

	scrolledwin = XtVaCreateManagedWidget("scrolledwin", 
			scrolledWindowWidgetClass,
			base,
			NULL);

	textedit = XtVaCreateManagedWidget("textedit", 
			textEditWidgetClass,
			scrolledwin,
			NULL);

	/* add the callbacks for the save and quit buttons */
	XtAddCallback(save_btn, XtNselect, saveTextCB, NULL);
	XtAddCallback(quit_btn, XtNselect, quitTextCB, NULL);

	/* start to handle the tooltalk messages and set the callback
	   when we get messages in
	 */
	fd = start_handling_messages();
	XtAppAddInput(appContext, fd, XtInputReadMask, handleMessageCB, NULL);

	/* realize the widgets and start the notification process */
	XtRealizeWidget(toplevel);
	XtAppMainLoop(appContext);

}
