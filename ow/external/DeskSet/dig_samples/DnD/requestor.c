#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <Xol/OpenLook.h>
#include <Xol/DropTarget.h>
#include <stdio.h>
#include "dnd.h"

/* forward pointers */
static	void	check_state();
static	void	make_request();


#define	SET_FLAG(i, flag)	(flag |= 1<<i)
#define	FLAG_SET(i, flag)	(flag & 1<<i)

struct	load	/* structure to keep track of the current state */
{
	unsigned long	req_flag;	/* current request */
	unsigned long	rec_flag;	/* values received back */
	unsigned long	err_flag;	/* errors received back */
	unsigned long	seen_flag;	/* targets seen so far */
	unsigned long	targets;	/* targets suppored by other tool */
	char	*filename;		/* contents of filename selection */
	char	*data;			/* contents of string/text selection */
	int	length;			/* contents of length selection */
	int	num_avail_types;	/* Number of available types */
	Atom	*avail_types;		/* contents of available types sel. */
	char	*data_label;		/* contents of data label selection */
	char	*app_name;		/* contents of name selection */
	char	*host_name;		/* contents of host name selection */
	int	enum_count;		/* contents of the enumerate count sel*/
	Widget	widget;			/* drop target widget */
	Atom	selection;		/* current selection item */
	Time	time;			/* time stamp */
} state;

/* strdup type of routine that takes care of the length of the string */
static char	*
save_str(char *str, int len)
{
	char	*new;

	if(!str || len <= 0)
	{
		return(NULL);
	}
	new = (char *)malloc((len+1));
	strncpy(new, str, len);
	new[len] = '\0';
}

/* the next set of 13 routines each take the data passed by a 
 * selection request and saves the data in the state structure.
*/
static Boolean
load_sun_file_host_name(
	Atom *type,
	XtPointer value,
	unsigned long *length,
	int *format)
{
	DP fprintf(stderr, "calling load_sun_file_host_name\n");
	state.host_name = save_str(value, *length);
	return(TRUE);
}

static Boolean
load_string(
	Atom *type,
	XtPointer value,
	unsigned long *length,
	int *format)
{
	DP fprintf(stderr, "calling load_string\n");
	state.data = save_str(value, *length);
	return(TRUE);
}

static Boolean
load_name(
	Atom *type,
	XtPointer value,
	unsigned long *length,
	int *format)
{
	DP fprintf(stderr, "calling load_name\n");
	state.app_name = save_str(value, *length);
	return(TRUE);
}

static Boolean
load_sun_available_types(
	Atom *type,
	XtPointer value,
	unsigned long *length,
	int *format)
{
	DP fprintf(stderr, "calling load_sun_available_types\n");
	state.avail_types = (Atom *)malloc(*length*sizeof(Atom));
	state.num_avail_types = (int)*length;
	memcpy(state.avail_types, value, (int)(*length*sizeof(Atom)));
	return(TRUE);
}

static Boolean
load_length(
	Atom *type,
	XtPointer value,
	unsigned long *length,
	int *format)
{
	unsigned long	*len;

	DP fprintf(stderr, "calling load_length\n");
	len = (unsigned long *)value;
	state.length = (int)len[0];
	return(TRUE);
}

static Boolean
load_text(
	Atom *type,
	XtPointer value,
	unsigned long *length,
	int *format)
{
	DP fprintf(stderr, "calling load_text\n");
	state.data = save_str(value, *length);
	return(TRUE);
}

static Boolean
load_sun_dragdrop_done(
	Atom *type,
	XtPointer value,
	unsigned long *length,
	int *format)
{
	DP fprintf(stderr, "calling load_sun_dragdrop_done\n");
	return(TRUE);
}

static Boolean
load_targets(
	Atom *type,
	XtPointer value,
	unsigned long *length,
	int *format)
{
	int	i,j;
	Atom	*list;

	DP fprintf(stderr, "calling load_targets\n");
	list = (Atom *)value;
	while(list && *list)
	{
		for(i = 0; i < num_targets; i++)
		{
			if(*list == mytargets[i].atom)
			{
				SET_FLAG(mytargets[i].type, state.targets);
				break;
			}
		}
		list++;
	}
	return(TRUE);
}

static Boolean
load_sun_selection_end(
	Atom *type,
	XtPointer value,
	unsigned long *length,
	int *format)
{
	DP fprintf(stderr, "calling load_sun_selection_end\n");
	OlDnDDragNDropDone(state.widget,
		         state.selection, 
		         state.time,
		         NULL, NULL);
	return(TRUE);
}

static Boolean
load_sun_enumeration_count(
	Atom *type,
	XtPointer value,
	unsigned long *length,
	int *format)
{
	unsigned long	*len;

	DP fprintf(stderr, "calling load_sun_enumeration_count\n");
	len = (unsigned long *)value;
	state.enum_count = (int)len[0];
	return(TRUE);
}

static Boolean
load_file_name(
	Atom *type,
	XtPointer value,
	unsigned long *length,
	int *format)
{
	DP fprintf(stderr, "calling load_file_name\n");
	state.filename = save_str(value, *length);
	return(TRUE);
}

static Boolean
load_sun_data_label(
	Atom *type,
	XtPointer value,
	unsigned long *length,
	int *format)
{
	DP fprintf(stderr, "calling load_sun_data_label\n");
	state.data_label = save_str(value, *length);
	return(TRUE);
}

static Boolean
load_sun_load(
	Atom *type,
	XtPointer value,
	unsigned long *length,
	int *format)
{
	DP fprintf(stderr, "calling load_sun_load\n");
	return(TRUE);
}

/* a dubugging routine to list out the names of a list of atoms */
static char *
list_types(Atom *list, int num)
{
	static	char	buff[1000];
	int	i;

	buff[0] = '\0';
	for(i = 0; i < num; i++)
	{
		strcat(buff, (char *)get_atom_name(list[i]));
		strcat(buff, ",");
	}
	if(buff[0] == '\0')
	{
		strcat(buff, "NULL");
	}
	else
	{
		buff[strlen(buff)-1] = '\0';
	}
	return(buff);
}

/* debbugging routine to list out the names of atoms marked in a flag */
static char *
list_flags(unsigned long flags)
{
	static	char	buff[1000];
	int	i;

	buff[0] = '\0';
	for(i = 0; i < num_targets; i++)
	{
		if(FLAG_SET(mytargets[i].type, flags))
		{
			strcat(buff, mytargets[i].name);
			strcat(buff, ",");
		}
	}
	if(buff[0] == '\0')
	{
		strcat(buff, "NULL");
	}
	else
	{
		buff[strlen(buff)-1] = '\0';
	}
	return(buff);
}

/* debugging routine to print out the state structure */
static void
print_state()
{
	fprintf(stderr, "\t--------------------\n");
	fprintf(stderr, "\treq_flag\t= 0x%04X\n\t\t%s\n",
		state.req_flag,
		list_flags(state.req_flag));

	fprintf(stderr, "\trec_flag\t= 0x%04X\n\t\t%s\n",
		state.rec_flag,
		list_flags(state.rec_flag));

	fprintf(stderr, "\terr_flag\t= 0x%04X\n\t\t%s\n",
		state.err_flag,
		list_flags(state.err_flag));

	fprintf(stderr, "\tseen_flag\t= 0x%04X\n\t\t%s\n",
		state.seen_flag,
		list_flags(state.seen_flag));

	fprintf(stderr, "\ttargets\t\t= 0x%04X\n\t\t%s\n",
		state.targets,
		list_flags(state.targets));

	fprintf(stderr, "\tfilename\t= %s\n",
		state.filename?state.filename:"(NULL)");

	fprintf(stderr, "\tdata\t\t= %s\n",
		state.data?state.data:"(NULL)");

	fprintf(stderr, "\tlength\t\t= %d\n",
		state.length);

	fprintf(stderr, "\tnum_avail_types\t= %d\n",
		state.num_avail_types);

	fprintf(stderr, "\tavail_types\t= %s\n",
		list_types(state.avail_types, state.num_avail_types));

	fprintf(stderr, "\tdata_label\t= %s\n",
		state.data_label?state.data_label:"(NULL)");

	fprintf(stderr, "\tapp_name\t= %s\n",
		state.app_name?state.app_name:"(NULL)");

	fprintf(stderr, "\thost_name\t= %s\n",
		state.host_name?state.host_name:"(NULL)");

	fprintf(stderr, "\tenum_count\t= %d\n",
		state.enum_count);
	fprintf(stderr, "\t--------------------\n");
}

/* called each time we get a selection back */
static void
GetSelection(
	Widget w,
	XtPointer clientData,
	Atom *selection,
	Atom *type,
	XtPointer value,
	unsigned long *length,
	int *format)
{
	atom_t	this_atom;
	int	results = FALSE;
	Atom	target = (Atom) clientData;
	int	i;

	DP fprintf(stderr, "calling GetSelection\n");

	/* find out which selection came back */
	for(i = 0; i < num_targets; i++)
	{
		if(target == mytargets[i].atom)
		{
			this_atom = mytargets[i].type;
			break;
		}
	}

	/* check for errors */
	if (length == 0)
	{
		DP fprintf(stderr, "ERROR\n");
		SET_FLAG((int)this_atom, state.err_flag);
		return;
	}
	if(i == num_targets)
	{
		this_atom = UNKNOWN;
		fprintf(stderr, "atom requested 0x%X\n", target);
		fprintf(stderr, "unknown atom requested '%s'\n", 
			get_atom_name(target));
		return;
	}
	else
	{
		DP fprintf(stderr, "ConvertSelection called for %s\n",
				mytargets[i].name);
	}

	/* call the appropriate procedure to load the info */
	switch(this_atom)
	{
	case	TARGETS:
		results = load_targets(type, value, length, format);
		break;
	case	FILE_NAME:
		results = load_file_name(type, value, length, format);
		break;
	case	STRING:
		results = load_string(type, value, length, format);
		break;
	case	LENGTH:
		results = load_length(type, value, length, format);
		break;
	case	SUN_AVAILABLE_TYPES:
		results = load_sun_available_types(type, value,
				length, format);
		break;
	case	SUN_LOAD:
		results = load_sun_load(type, value, length, format);
		break;
	case	SUN_DATA_LABEL:
		results = load_sun_data_label(type, value, length, format);
		break;
	case	SUN_DRAGDROP_DONE:
		results = load_sun_dragdrop_done(type, value,
				length, format);
		break;
	case	TEXT:
		results = load_text(type, value, length, format);
		break;
	case	SUN_SELECTION_END:
		results = load_sun_selection_end(type, value,
				length, format);
		break;
	case	NAME:
		results = load_name(type, value, length, format);
		break;
	case	SUN_FILE_HOST_NAME:
		results = load_sun_file_host_name(type, value,
				length, format);
		break;
	case	SUN_ENUMERATION_COUNT:
		results = load_sun_enumeration_count(type, value,
				length, format);
		break;
	}

	/* if the load was succesfull */
	if(results)
	{
		/* mark the received flag */
		SET_FLAG((int)this_atom, state.rec_flag);
	}
	else
	{
		/* mark the error flag */
		SET_FLAG((int)this_atom, state.err_flag);
	}
	DP fprintf(stderr, "GetSelection call\n%04X\n%04X\n%04X\n",
			state.req_flag,
			state.rec_flag,
			state.err_flag);

	/* if we got all our requests back check the state to
	 * find out what to do next
	 */

	if(state.req_flag == state.rec_flag|state.err_flag)
	{
		check_state();
	}
}

/* initialize the state structure (We're begining a new drop */
static void
init_state()
{
	state.req_flag = 0;
	state.rec_flag = 0;
	state.err_flag = 0;
	state.seen_flag = 0;

	state.targets = 0;

	if(state.filename)
	{
		free(state.filename);
	}
	state.filename = 0;

	if(state.data)
	{
		free(state.data);
	}
	state.data = 0;

	state.length = -1;

	if(state.avail_types)
	{
		free(state.avail_types);
	}
	state.avail_types = 0;
	state.num_avail_types = 0;

	if(state.data_label)
	{
		free(state.data_label);
	}
	state.data_label = 0;

	if(state.app_name)
	{
		free(state.app_name);
	}
	state.app_name = 0;

	if(state.host_name)
	{
		free(state.host_name);
	}
	state.host_name = 0;

	state.enum_count = -1;
}

/* request the targest specified in the request flag */
static	void
make_request()
{
	static	Atom	*requests = 0;
	int	num_request = 0;
	int	i;

	/* make an array that will hold the requests */
	if(!requests)
	{
		requests = (Atom *)malloc(num_targets*sizeof(Atom));
	}

	/* check each target to see if we request it */
	for(i = 0; i < num_targets; i++)
	{
		if(FLAG_SET(mytargets[i].type, state.req_flag))
		{
			requests[num_request] = mytargets[i].atom;
			num_request++;
		}
	}
	requests[num_request] = 0;

	/* ask for the list of targets */
	XtGetSelectionValues(state.widget,
			state.selection,
			requests,
			num_request,
			GetSelection,
			requests,
			state.time);
}

/* we've been droped on */
void
requestor(Widget widget, Atom selection, Time time)
{
	/*
	 * put into an array a series of questions in the
	 * form of atoms that source understands.  Then ask
	 * selection to deliver the questions.  We also
	 * register a function to handle the answers
	 */

	DP fprintf(stderr, "calling requestor\n");

	/* initialize the state and save the calling info */
	init_state();
	state.widget = widget;
	state.selection = selection;
	state.time = time;

	/* request the target list */
	SET_FLAG((int)TARGETS, state.req_flag);

	make_request();
}

/* part of the request have been completed so lets see if there
 * is anything else we should do 
*/
static void
check_state()
{
	int	tmp = 0;


	DP fprintf(stderr, "Before\n");

	/* save those targets we've seen */
	state.seen_flag |= state.rec_flag;

	DP print_state();

	/* clear the request, received and error flags */
	state.req_flag = 0;
	state.rec_flag = 0;
	state.err_flag = 0;

	/* check if this is the first request */
	SET_FLAG((int)TARGETS, tmp);
	if(tmp == state.seen_flag)
	{
		/* request the info for those we know about */
		if(FLAG_SET(FILE_NAME, state.targets))
			SET_FLAG((int)FILE_NAME, state.req_flag);
	
		if(FLAG_SET(LENGTH, state.targets))
			SET_FLAG((int)LENGTH, state.req_flag);
	
		if(FLAG_SET(SUN_AVAILABLE_TYPES, state.targets))
			SET_FLAG((int)SUN_AVAILABLE_TYPES, state.req_flag);
	
		if(FLAG_SET(SUN_DATA_LABEL, state.targets))
			SET_FLAG((int)SUN_DATA_LABEL, state.req_flag);
	
		if(FLAG_SET(NAME, state.targets))
			SET_FLAG((int)NAME, state.req_flag);
	
		if(FLAG_SET(SUN_FILE_HOST_NAME, state.targets))
			SET_FLAG((int)SUN_FILE_HOST_NAME, state.req_flag);
	
		if(FLAG_SET(SUN_ENUMERATION_COUNT, state.targets))
			SET_FLAG((int)SUN_ENUMERATION_COUNT, state.req_flag);


		if(FLAG_SET(STRING, state.targets))
		{
			SET_FLAG((int)STRING, state.req_flag);
		}
		else if(FLAG_SET(TEXT, state.targets))
		{
			SET_FLAG((int)TEXT, state.req_flag);
		}
	}
	else if(!FLAG_SET(SUN_DRAGDROP_DONE, state.seen_flag))
	{
		/* since we haven't seen the end info then request it */
		SET_FLAG((int)SUN_DRAGDROP_DONE, state.req_flag);
		SET_FLAG((int)SUN_SELECTION_END, state.req_flag);
	}
	else
	{
		Dnd_t	user_data;

		/* fill in the user info structure and display it */ 
		user_data.filename = state.filename;
		user_data.data = state.data;
		user_data.length = state.length;
		user_data.data_label = state.data_label;
		user_data.app_name = state.app_name;
		user_data.host_name = state.host_name;
		user_data.enum_count = state.enum_count;

		dnd_load(&user_data);
		init_state();
		return;
	}
	DP fprintf(stderr, "After\n");
	DP print_state();
	make_request();
}
