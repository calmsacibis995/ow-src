#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <X11/Xatom.h>
#include <Xol/OpenLook.h>
#include <Xol/DropTarget.h>
#include <sys/systeminfo.h>
#include <stdio.h>
#include "dnd.h"

/* these 13 routines just get the requested data and places gives
 * it to the selection service when requested.
*/ 
static Boolean
convert_sun_file_host_name(
	Atom *type,
	XtPointer *value,
	unsigned long *length,
	int *format)
{
	static	char	*sys = 0;

	DP fprintf(stderr, "calling convert_sun_file_host_name\n");
	if(!sys)
	{
		char buff[100];

		if(sysinfo(SI_HOSTNAME, buff, 100) == -1)
		{
			return(FALSE);
		}
		sys = strdup(buff);
	}
	*type = XA_STRING;
	*value = sys;
	*length = strlen(sys)+1;
	*format = 8;
	return(TRUE);
}

static Boolean
convert_string(
	Atom *type,
	XtPointer *value,
	unsigned long *length,
	int *format)
{
	DP fprintf(stderr, "calling convert_string\n");
	*type = XA_STRING;
	get_data(value, length);
	*format = 8;
	return(TRUE);
}

static Boolean
convert_name(
	Atom *type,
	XtPointer *value,
	unsigned long *length,
	int *format)
{
	DP fprintf(stderr, "calling convert_name\n");
	*type = XA_STRING;
	*value = (XtPointer)get_name();
	*length = (unsigned long)strlen(*value)+1;
	*format = 8;
	return(TRUE);
}

static Boolean
convert_sun_available_types(
	Atom *type,
	XtPointer *value,
	unsigned long *length,
	int *format)
{
	static	Atom	av_type = XA_STRING;

	DP fprintf(stderr, "calling convert_sun_available_types\n");
	*type = XA_ATOM;
	*value = (XtPointer)&av_type;
	*length = 1;
	*format = 32;
	return(TRUE);
}

static Boolean
convert_length(
	Atom *type,
	XtPointer *value,
	unsigned long *length,
	int *format)
{
	char		*val;
	unsigned long	len;

	DP fprintf(stderr, "calling convert_length\n");
	*type = XA_INTEGER;
	get_data((char **)&val, &len);
	*length = 1;
	*value = (XtPointer)&len;
	*format = 32;
	return(TRUE);
}

static Boolean
convert_text(
	Atom *type,
	XtPointer *value,
	unsigned long *length,
	int *format)
{
	DP fprintf(stderr, "calling convert_text\n");
	*type = XA_STRING;
	get_data(value, length);
	*format = 8;
	return(TRUE);
}

static Boolean
convert_sun_dragdrop_done(
	Atom *type,
	XtPointer *value,
	unsigned long *length,
	int *format)
{
	DP fprintf(stderr, "calling convert_sun_dragdrop_done\n");
	*type = XA_INTEGER;
	*length = (unsigned long)0;
	*value = (XtPointer)0;
	*format = 32;
	return(TRUE);
}

static Boolean
convert_targets(
	Atom *type,
	XtPointer *value,
	unsigned long *length,
	int *format)
{
	static	Atom	*targets;

	DP fprintf(stderr, "calling convert_targets\n");
	if(!targets)
	{
		int	i;

		targets = (Atom *)malloc(num_targets*sizeof(Atom));
		for(i = 0; i < num_targets; i++)
		{
			targets[i] = mytargets[i].atom;
		}
	}
	*type = XA_ATOM;
	*value = (XtPointer)targets;
	*length = num_targets;
	*format = 32;
	return(TRUE);
}

static Boolean
convert_sun_selection_end(
	Atom *type,
	XtPointer *value,
	unsigned long *length,
	int *format)
{
	DP fprintf(stderr, "calling convert_sun_selection_end\n");
	*type = XA_INTEGER;
	*length = (unsigned long)0;
	*value = (XtPointer)0;
	*format = 32;
	return(TRUE);
}

static Boolean
convert_sun_enumeration_count(
	Atom *type,
	XtPointer *value,
	unsigned long *length,
	int *format)
{
	static	int	count = 1;

	DP fprintf(stderr, "calling convert_sun_enumeration_count\n");
	*type = XA_INTEGER;
	*length = 1;
	*value = (XtPointer)&count;
	*format = 32;
	return(TRUE);

}

static Boolean
convert_file_name(
	Atom *type,
	XtPointer *value,
	unsigned long *length,
	int *format)
{
	DP fprintf(stderr, "calling convert_file_name\n");
	*type = XA_STRING;
	*value = (XtPointer)get_file_name();
	*length = (unsigned long )strlen(*value)+1;
	*format = 8;
	return(TRUE);
}

static Boolean
convert_sun_data_label(
	Atom *type,
	XtPointer *value,
	unsigned long *length,
	int *format)
{
	DP fprintf(stderr, "calling convert_sun_data_label\n");
	*type = XA_STRING;
	*value = (XtPointer)get_data_label();
	*length = strlen(*value)+1;
	*format = 8;
	return(TRUE);
}

static Boolean
convert_sun_load(
	Atom *type,
	XtPointer *value,
	unsigned long *length,
	int *format)
{
	DP fprintf(stderr, "calling convert_sun_load\n");
	*type = XA_INTEGER;
	*length = (unsigned long)0;
	*value = (XtPointer)0;
	*format = 32;
	return(TRUE);
}

/* this routine gets called when ever a conversion is requested */
static Boolean
ConvertSelection(
	Widget w,
	Atom *selection,
	Atom *atom,
	Atom *type,
	XtPointer *value,
	unsigned long *length,
	int *format)
{
	atom_t	this_atom;
	Boolean	results = FALSE;
	int	i;

	DP fprintf(stderr, "calling ConvertSelection\n");

	/* find out which atom is requested */
	for(i = 0; i < num_targets; i++)
	{
		if(*atom == mytargets[i].atom)
		{
			this_atom = mytargets[i].type;
			break;
		}
	}
	if(i == num_targets)
	{
		this_atom = UNKNOWN;
		DP fprintf(stderr, "atom requested 0x%X\n", *atom);
		DP fprintf(stderr, "unknown atom requested '%s'\n", 
			get_atom_name(*atom));
	}
	else
	{
		DP fprintf(stderr, "ConvertSelection called for %s\n", 
			mytargets[i].name);
	}

	/* call the appropriate convert proc */
	switch(this_atom)
	{
	case	TARGETS:
		results = convert_targets(type, value, length, format);
		break;
	case	FILE_NAME:
		results = convert_file_name(type, value, length, format);
		break;
	case	STRING:
		results = convert_string(type, value, length, format);
		break;
	case	LENGTH:
		results = convert_length(type, value, length, format);
		break;
	case	SUN_AVAILABLE_TYPES:
		results = convert_sun_available_types(type, value,
				length, format);
		break;
	case	SUN_LOAD:
		results = convert_sun_load(type, value, length, format);
		break;
	case	SUN_DATA_LABEL:
		results = convert_sun_data_label(type, value, length, format);
		break;
	case	SUN_DRAGDROP_DONE:
		results = convert_sun_dragdrop_done(type, value,
				length, format);
		break;
	case	TEXT:
		results = convert_text(type, value, length, format);
		break;
	case	SUN_SELECTION_END:
		results = convert_sun_selection_end(type, value,
				length, format);
		break;
	case	NAME:
		results = convert_name(type, value, length, format);
		break;
	case	SUN_FILE_HOST_NAME:
		results = convert_sun_file_host_name(type, value,
				length, format);
		break;
	case	SUN_ENUMERATION_COUNT:
		results = convert_sun_enumeration_count(type, value,
				length, format);
		break;
	default:
		return(FALSE);
	}
	return(results);
}

/* check the state of the request etc. */
static void
TransactionState(
	Widget w,
	Atom selection,
	OlDnDTransactionState state,
	Time timestamp,
	XtPointer clientData)
{
	DP fprintf(stderr, "calling TransactionState\n");
	switch (state)
	{
	case OlDnDTransactionDone:
	case OlDnDTransactionRequestorError:
	case OlDnDTransactionRequestorWindowDeath:
		/*
		 * some sort of failure occured or we are done, give up
		 * selection we own. Note: we could have done the disowning
		 * of selection when we got SELECTION_END, but we chose to do
		 * it here...
		 */
		OlDnDDisownSelection(w,
				     selection,
				     XtLastTimestampProcessed(XtDisplay(w)));
		OlDnDFreeTransientAtom(w, selection);
		break;
	case OlDnDTransactionBegins:
	case OlDnDTransactionEnds:
		break;
	}
}

/* starts the drag operation */ 
void
owner(Widget widget, Time time)
{
	Atom	atom;

	DP fprintf(stderr, "calling owner\n");

	/* allocate and own selection.  Register a convert proc */
	atom = OlDnDAllocTransientAtom(widget);

	XtVaSetValues(widget, XtNselectionAtom, atom, NULL);

	OlDnDOwnSelection(widget, atom, time,
			  ConvertSelection,
			  (XtLoseSelectionProc) NULL,
			  (XtSelectionDoneProc) NULL,
			  TransactionState,
			  NULL);
}
