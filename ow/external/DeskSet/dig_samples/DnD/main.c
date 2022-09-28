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
#include <sys/systeminfo.h>
#include "dnd.h"

/* array of targets we want to use */
targets_t	mytargets[] =
{
	{TARGETS,		"TARGETS",			0},
	{FILE_NAME,		"FILE_NAME",			0},
	{STRING,		"STRING",			0},
	{LENGTH,		"LENGTH",			0},
	{SUN_AVAILABLE_TYPES,	"_SUN_AVAILABLE_TYPES",		0},
	{SUN_LOAD,		"_SUN_LOAD",			0},
	{SUN_DATA_LABEL,	"_SUN_DATA_LABEL",		0},
	{SUN_DRAGDROP_DONE,	"_SUN_DRAGDROP_DONE",		0},
	{TEXT,			"TEXT",				0},
	{SUN_SELECTION_END,	"_SUN_SELECTION_END",		0},
	{NAME,			"NAME",				0},
	{SUN_FILE_HOST_NAME,	"_SUN_FILE_HOST_NAME",		0},
	{SUN_ENUMERATION_COUNT,	"_SUN_ENUMERATION_COUNT",	0},
};

/* the number of targets */
int	num_targets = sizeof(mytargets)/sizeof(targets_t);

/* widgets we interact with */
Widget	drop_target;
Widget	textedit;
Widget	file;

char	*arg0;	/* program name */

/*
 * handle source & destination sides of dnd
 */
static void
DropTargetCB(Widget w, XtPointer clientData, XtPointer callData)
{
	OlDropTargetCallbackStruct	*cd;

	DP fprintf(stderr, "calling DropTargetCB\n");

	/* initalize the atom names */
	if(mytargets[0].atom == 0)
	{
		XrmValue	source, dest;
		int		i;

		dest.size = sizeof(Atom);

		for(i = 0; i < num_targets; i++)
		{
			source.size = strlen(mytargets[i].name)+1;
			source.addr = mytargets[i].name;
			dest.addr = (char *)&mytargets[i].atom;
			XtConvertAndStore(drop_target, XtRString,
				&source, XtRAtom, &dest);
		}
	}

	/* call the appropriate owner/requestor routines */
	cd = (OlDropTargetCallbackStruct *) callData;
	switch (cd->reason)
	{
	case OL_REASON_DND_OWNSELECTION: /* case when we do a drag */
		owner(cd->widget, cd->time);
		break;
	case OL_REASON_DND_TRIGGER: /* case when we detect a drop */
		requestor(cd->widget, cd->selection, cd->time);
	}
}

/* mark drop target as full if text is typed */
static void
text_modified(Widget w, XtPointer clientData, XtPointer callData)
{
	XtVaSetValues(drop_target, XtNfull, (XtArgVal)TRUE, NULL);
}

/* main initalization routine */
main(int argc, char **argv)
{
	XtAppContext    appContext;
	Widget	toplevel, base;
	Widget	control, scrolledwin;
	Widget	blank;

	/* save program name */
	arg0 = argv[0];

	/* initialize and build the GUI widgets */
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

	blank = XtVaCreateManagedWidget("blank",
			staticTextWidgetClass, 
			control,
			NULL);

	/* build the drop target and set its initial properties */
	drop_target = XtVaCreateManagedWidget("drop_target",
			dropTargetWidgetClass,
			control,
			XtNfull,
				(XtArgVal)FALSE,
			XtNdndPreviewHints,
				(XtArgVal)OlDnDSitePreviewDefaultSite,
			XtNdndMoveCursor,
				(XtArgVal)OlGetMoveDocDragCursor(toplevel),
			XtNdndCopyCursor,
				(XtArgVal)OlGetDupeDocDragCursor(toplevel),
			XtNdndAcceptCursor,
				(XtArgVal)OlGetDupeDocDropCursor(toplevel),
			XtNdndRejectCursor,
				(XtArgVal)OlGetDupeDocNoDropCursor(toplevel),
			NULL);


	file = XtVaCreateManagedWidget("file",
			staticTextWidgetClass, 
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

	/* let us know if we have text to drag-n-drop */
	XtAddCallback(textedit, XtNpostModifyNotification, text_modified, NULL);

	/* notifiy us of a drag or drop operation */
	XtAddCallback(drop_target, XtNownSelectionCallback, DropTargetCB, NULL);
	XtAddCallback(drop_target, XtNdndTriggerCallback,   DropTargetCB, NULL);

	/* realize the widget and start the notification loop */
	XtRealizeWidget(toplevel);
	XtAppMainLoop(appContext);

}

/* return the ascii name of a X Atom */
char *
get_atom_name(Atom atom)
{
	return(XGetAtomName(XtDisplay(drop_target), atom));
}

/* return the data and length of the text displayed */
Boolean
get_data(char **data, unsigned long *length)
{
	static	char	*content = NULL;

	if(content)
	{
		XtFree(content);
	}
	if (!OlTextEditCopyBuffer((TextEditWidget) textedit, &content))
	{
		OlWarning("get_data: error trying to copy textedit buffer\n");
		return(FALSE);
	}
	*data = content;
	*length = strlen(content)+1;
	return(TRUE);
}

/* return a data label for the data */
char *
get_data_label()
{
	static	char	buff[200];
	char	*c;
	String	filename;

	XtVaGetValues(file, XtNstring, &filename, NULL);
	c = strrchr(filename, '/');
	if(c)
	{
		strcpy(buff, c);
	}
	else
	{
		strcpy(buff, filename);
	}
	XtFree(filename);
	
	return(buff);
}

/* return the name of the application */
char *
get_name()
{
	char	*label = strrchr(arg0, '/');

	if(label)
	{
		return(label);
	}
	else
	{
		return(arg0);
	}
}

/* return the name of the file currently being edited if any */
char *
get_file_name()
{
	static	char	buff[200];
	String	filename;

	XtVaGetValues(file, XtNstring, &filename, NULL);
	strcpy(buff, filename);
	XtFree(filename);
	
	return(buff);
}

/* callback with the data from the selection */
dnd_load(Dnd_t *dnd)
{
	static	char	*sys = 0;

	/* fprintfs just to see ALL the data that came back */
	fprintf(stderr, "filename\t= %s\n",
		dnd->filename?dnd->filename:"(NULL)");

	fprintf(stderr, "data\t\t= %.50s%s\n",
		dnd->data?dnd->data:"(NULL)",
		((int)strlen(dnd->data)>50)?" . . .":"");

	fprintf(stderr, "length\t\t= %d\n",
		dnd->length);

	fprintf(stderr, "data_label\t= %s\n",
		dnd->data_label?dnd->data_label:"(NULL)");

	fprintf(stderr, "app_name\t= %s\n",
		dnd->app_name?dnd->app_name:"(NULL)");

	fprintf(stderr, "host_name\t= %s\n",
		dnd->host_name?dnd->host_name:"(NULL)");

	fprintf(stderr, "enum_count\t= %d\n",
		dnd->enum_count);

	/* displays the data returned */
	if(!sys)
	{
		char buff[100];

		if(sysinfo(SI_HOSTNAME, buff, 100) == -1)
		{
			return(FALSE);
		}
		sys = strdup(buff);
	}
	if(dnd->host_name && strcmp(dnd->host_name, sys) == 0 && dnd->filename)
	{
		XtVaSetValues(drop_target, XtNfull, (XtArgVal)TRUE, NULL);

		XtVaSetValues(textedit,
			XtNsourceType,	OL_DISK_SOURCE,
			XtNsource, dnd->filename,
			NULL);

		XtVaSetValues(file,
			XtNstring, dnd->filename,
			NULL);
	}
	else
	{
		XtVaSetValues(drop_target, XtNfull, (XtArgVal)TRUE, NULL);

		XtVaSetValues(textedit,
			XtNsourceType,	OL_STRING_SOURCE,
			XtNsource, dnd->data,
			NULL);

		XtVaSetValues(file,
			XtNstring, "",
			NULL);
	}
}
