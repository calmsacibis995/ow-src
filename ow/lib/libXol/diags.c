#pragma ident	"@(#)diags.c	1.9    93/02/10 lib/libXol SMI"    /* OLIT	493	*/

/************************************************************************
 *
 *      Implementation of the diagnostics module
 *
 ************************************************************************/

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
 *      Imported Interfaces
 *
 ************************************************************************/

#include <ctype.h>
#include <errno.h>		/* perror(), errno */
#include <stdarg.h>		/* va_list, va_start(), va_end() */
#include <stdio.h>		/* vfprintf(), stderr, NULL */
#include <stdlib.h>		/* free(), exit() */
#include <string.h>
#include <sys/types.h>		/* boolean_t */
#include <widec.h>

#include <X11/IntrinsicP.h>
#include <X11/StringDefs.h>
#include <X11/CompositeP.h>
#include <X11/CoreP.h>

#include <Xol/OpenLook.h>	/* OlError() */
#include <Xol/OpenLookP.h>	/* OlMsgsDomain */

#include <Xol/diags.h>		/* Interface of this module */


/************************************************************************
 *
 *      Supplemental External Data Declarations
 *
 ************************************************************************/

extern int	sys_nerr;


/************************************************************************
 *
 *      Module Private Macro Definitions
 *
 ************************************************************************/

#define	MESSAGE_BUFFER_SIZE		(2048)
#define	MAX_INSTANCE_PATH_LENGTH	(1024)
#define	MAX_NUM_INSTANCES		(256)
#define	MAX_RESOURCE_NAME		(128)

#ifdef	DEBUG
	#define	MESSAGE(fmt) \
		{ \
			va_list		args; \
			\
			if (NULL != fmt) { \
				va_start(args, fmt); \
				(void) vfprintf(stderr, fmt, args); \
				va_end(args); \
			} \
		}
#else
	#define	MESSAGE(fmt)	/*NOOP*/
#endif


#define	PERROR(func_name) \
	{ \
		if ((errno > 0) && (errno < sys_nerr))	perror((func_name)); \
		errno = 0;	/* must be cleared */ \
	}


/************************************************************************
 *
 *      Implementation of this module's public functions
 *
 ************************************************************************/


/************************************************************************
 *
 *      _OlMessage -- Report to standard output
 *
 ****************************procedure*header******************************/

/*PRINTFLIKE1*/
void
_OlMessage(const char*const fmt, ...)
{

	MESSAGE(fmt);
} /* end of _OlMessage() */


/************************************************************************
 *
 *      _OlReport -- Warn of function failure
 *
 ****************************procedure*header******************************/

/*PRINTFLIKE2*/
void
_OlReport(const char*const func_name, const char*const fmt, ...)
{

	MESSAGE(fmt);
	PERROR(func_name);
} /* end of _OlReport() */


/************************************************************************
 *
 *      _OlAbort -- Abort after function failure
 *
 ****************************procedure*header******************************/

/*PRINTFLIKE2*/
void
_OlAbort(const char *const func_name, const char *const fmt, ...)
{
	static char		message_buffer[MESSAGE_BUFFER_SIZE];

	va_list			args;

	GetToken();

	if (NULL != fmt) {
		va_start(args, fmt);
		(void) vsprintf(message_buffer, fmt, args);
		va_end(args);
	}

	PERROR(func_name);
	
	ReleaseToken();	/*! ?? !*/
	OlError(message_buffer);
	/*NOTREACHED*/
} /* end of _OlAbort() */


/************************************************************************
 *
 *  _OlInArgList() -- 
 * 
 *	Does the named resource exist in the argument list?
 *
 ************************************************************************/

Boolean
_OlInArgList(const char*const resource_name, const ArgList args, 
	const Cardinal num_args)
{
	Boolean		found = FALSE;
	int		i = 0;

	while (i < num_args && !found)
			found = (0 == strcmp(args[i++].name, resource_name));

	return found;
} /* end of _OlInArgList() */


/************************************************************************
 *
 *      _OlStrIsEmpty --
 *
 ************************************************************************/

Boolean
_OlStrIsEmpty(const OlStr olstr, const OlStrRep text_format)
{
	Boolean			empty = (NULL == olstr);
	
	if (!empty)
		switch (text_format) {
		case OL_SB_STR_REP:
		/*FALLTHROUGH*/
		case OL_MB_STR_REP:
			empty = (0 == strcmp((const char*)olstr, ""));
			break;
		case OL_WC_STR_REP:
			empty = (0 == wscmp((wchar_t*)olstr, L""));
			break;
		}
	
	return empty;
} /* end of _OlStrIsEmpty() */


/************************************************************************
 *
 *      _OlRootWidget -- 
 *
 ************************************************************************/

const Widget 
_OlRootWidget(const Widget widget)
{
	Widget			w;
	Widget			prevw;

	/* trace widget's ancestry */
	for (w = widget; NULL != w; prevw = w, w = w->core.parent)
		/*NOOP*/;

	return prevw;
} /* end of _OlRootWidget() */


#ifdef	DEBUG

/************************************************************************
 *
 *      debug only functions
 *
 ************************************************************************/

/*! I18N !*/

/************************************************************************
 *
 *      _OlWidgetPrintInstanceTree -- 
 *
 ************************************************************************/

void 
_OlWidgetPrintInstanceTree(const Widget widget)
{

	(void) printf("%-40s %s\n", _OlWidgetInstanceHierarchy(widget),
		_OlWidgetClassOfAncestory(widget));

	if (XtIsComposite(widget)) {
		CompositeRec*		compw = (CompositeRec*)widget;
		int			i;

		for (i = 0; i < compw->composite.num_children; ++i) {
			_OlWidgetPrintInstanceTree(compw->composite.children[i]);
		}
	}
}


/************************************************************************
 *
 *      _OlWidgetInstanceHierarchy -- 
 *
 ************************************************************************/

const String
_OlWidgetInstanceHierarchy(const Widget widget)
{
	static char		instance_path[MAX_INSTANCE_PATH_LENGTH];

	Widget			ancestry[MAX_NUM_INSTANCES];
	CoreWidget		w;
	int			num_ancestors = 0;
	int			i;

	GetToken();

	/* trace widget's ancestry */
	for (num_ancestors = 0, w = (CoreWidget)widget; NULL != w; 
			++num_ancestors, w = w->core.parent)
		ancestry[num_ancestors] = w;

	/* traverse ancestry backwards concatenating names */
	for (*instance_path = '\0', i = num_ancestors - 1; i > 0; --i)
		(void) strcat(strcat(instance_path, ancestry[i]->core.name), 
			".");

	(void) strcat(instance_path, ancestry[0]->core.name);

	ReleaseToken();
	return instance_path;
}


/************************************************************************
 *
 *      _OlWidgetClassOfAncestory -- 
 *
 ************************************************************************/

const String
_OlWidgetClassOfAncestory(const Widget widget)
{
	static char		class_of_instance_path[
					MAX_INSTANCE_PATH_LENGTH] = { '\0' };
	
	Widget			ancestry[MAX_NUM_INSTANCES];
	Widget			w;
	int			num_ancestors;
	int			i;

	GetToken();

	/* trace widget's ancestry */
	for (num_ancestors = 0, w = (CoreWidget)widget; NULL != w; 
			++num_ancestors, w = w->core.parent)
		ancestry[num_ancestors] = w;

	/* traverse ancestry backwards concatenating names */
	class_of_instance_path[0] = '\0';
	for (*class_of_instance_path = '\0', i = num_ancestors - 1; i > 0; --i)
		(void) strcat(strcat(class_of_instance_path, 
			ancestry[i]->core.widget_class->core_class.class_name), 
			".");

	(void) strcat(class_of_instance_path, 
		ancestry[0]->core.widget_class->core_class.class_name);

	ReleaseToken();
	return class_of_instance_path;
}


/************************************************************************
 *
 *      _OlWidgetResourceValue -- 
 *
 ************************************************************************/

String
_OlWidgetResourceValue(const Widget widget, const char* const resource_name)
{
	static char		resource_class_name[MAX_RESOURCE_NAME];
	static char		output_buf[MESSAGE_BUFFER_SIZE];

	char			instance_spec[MAX_INSTANCE_PATH_LENGTH + 
						MAX_RESOURCE_NAME + 1];

	char			class_spec[MAX_INSTANCE_PATH_LENGTH + 
						MAX_RESOURCE_NAME + 1];

	char			value_buf[MESSAGE_BUFFER_SIZE];
	
	XrmValue		rmvalue;
	String			resource_type;
	Display*		dpy = XtDisplay(widget);
	XrmDatabase		rmdb = dpy->db;
	boolean_t		found;

	GetToken();

	(void) strcat(strcat(strcpy(instance_spec, 
		_OlWidgetInstanceHierarchy(widget)), "."), resource_name);
	
	(void) strcpy(resource_class_name, resource_name);
	if (islower(resource_class_name[0]))
		resource_class_name[0] = toupper(resource_class_name[0]);

	(void) strcat(strcat(strcpy(class_spec, 
		_OlWidgetClassOfAncestory(widget)), "."), resource_class_name);

	found = XrmGetResource(rmdb, instance_spec, class_spec,
		&resource_type, &rmvalue);

	*output_buf = '\0';
	if (found) {
		
		if (0 == strcmp("String", resource_type)) {
			sprintf(output_buf, "resource manager: (%s)", 
				resource_type);
			strncpy(value_buf, rmvalue.addr, rmvalue.size);
			value_buf[rmvalue.size] = '\0';
			strcat(output_buf, " ");
			strcat(output_buf, value_buf);
		} else if (0 == strcmp(resource_type, "Dimension") || 
				0 == strcmp(resource_type, "Int") || 
				0 == strcmp(resource_type, "Position")) {
			sprintf(output_buf, "resource manager: (%s)%d", 
				resource_type, *rmvalue.addr);
		} else {
			sprintf(output_buf, "resource manager: (%s)(length = "
				"%d)", resource_type, rmvalue.size);
		}
	}

	ReleaseToken();
	return output_buf;
}


/************************************************************************
 *
 *      _OlWidgetOfName -- 
 *
 ************************************************************************/

const Widget
_OlWidgetOfName(const Widget widget, const char* const widget_name)
{
	XrmQuark		xrm_name = XrmStringToQuark(widget_name);
	Widget			result = NULL;

	/*
	 * Compare quarks, not strings. First, it should prove a little
	 * faster, second, it works for gadget children (the string version
	 * of the name isn't in core for gadgets).
	 */

	if (xrm_name == widget->core.xrm_name)
		result = widget;
	else if (XtIsComposite(widget)) {

		CompositeRec*		compw = (CompositeRec*)widget;
		WidgetList		wlist = compw->composite.children;
		Cardinal		wnum = compw->composite.num_children;
		boolean_t		found;
		int			i;

		/* Breadth-first search */

		for (found = B_FALSE, i = 0; 
				!found && i < wnum;
				++i) 
			if (xrm_name == wlist[i]->core.xrm_name) {
				result = wlist[i];
				found = B_TRUE;
			}
		
		if (!found)
			for (found = B_FALSE, i = 0; !found && i < wnum; ++i)
		if (NULL != (result = _OlWidgetOfName(wlist[i], widget_name))) {
					found = B_TRUE;
				}
	}

	return result;
}

		
/************************************************************************
 *
 *      _OlMain --
 *
 ************************************************************************/

void
_OlMain(const int argc, const char* const argv[], 
	const char** fallback_resources, XtWidgetProc construct_basewin)
{
	XtAppContext		app_context;
	Widget			basewin;	/* top level widget */

	/* Make sure the syscall error handler is reset */
        errno = 0;
	
	(void) OlToolkitInitialize((XtPointer)NULL); 
	
	basewin = XtVaAppInitialize(&app_context, (String)NULL, 
		(XrmOptionDescList)NULL, 0, (int*)&argc, (String*)argv, 
		(String*)fallback_resources, NULL);

	(*construct_basewin)(basewin);

	XtRealizeWidget(basewin);

	XtAppMainLoop(app_context);
	/*NOTREACHED*/
	exit(EXIT_FAILURE);
	/*NOTREACHED*/
} /* end of _OlMain() */


/************************************************************************
 *
 *      self-test
 *
 ************************************************************************/


#include <X11/Shell.h>

#include <Xol/ControlAre.h>
#include <Xol/OblongButt.h>


#define MAX_CHILDREN	(16)

#define	CREATE_WIDGET(root_name, class, parent_name) \
	root_name = XtCreateWidget( \
				#root_name, \
				class ## WidgetClass, \
				parent_name, \
				NULL, 0 \
			)


static void		diags_test(void);
static void		construct_basewin(Widget parent);

static void		print_instance_tree_test(const int argc, 
	const char* const argv[]);


/************************************************************************
 *
 *      diags_test -- 
 *
 ************************************************************************/

static void
diags_test(void)
{
	boolean_t	ok = B_TRUE;
	String*		sp;
	void*		vp;
	int*		ip;
	
	errno = 0;

	_OlMessage("This is a message.\n");
	_OlReport(NULL, "This is warning no. %d.  Yes, no. "
		"%d.\n", 1, 1);
	_OL_WHERE();
	
	_OlReport(NULL, "Should be 1:  %d\n", ok);
	
	_OL_MALLOC(sp, String);
	_OL_FREE(sp);
	
	_OL_MALLOCN(vp, 100);
	_OL_FREE(vp);

	_OL_CALLOC(ip, 300, int);
	_OL_FREE(ip);
	
	/* _OlAbort(NULL, "We are aborting for the %s time\n", "first"); */

} /* end of diags_test() */


/************************************************************************
 *
 *      construct_basewin -- Populate the top level panel
 *
 ************************************************************************/

static void
construct_basewin(Widget parent)
{
	Widget          
		panel_ca,
			one_ob,
			two_ob
		;

	#define	SHOW_ROOT(wid) \
		(void) printf("Root widget of %s is %s.\n", XtName(wid), \
			XtName(_OlRootWidget(wid)))

	#define	OUTPUT(topw, name) \
		(void) printf("Widget named %s found to be %s :=)\n", name,  \
			XtName(_OlWidgetOfName(topw, name)))
	
	#define	SHOW_IHIER(wid) \
		(void) printf("%s:\t%s\n", #wid, \
			_OlWidgetInstanceHierarchy(wid))
	
	#define	SHOW_CHIER(wid) \
		(void) printf("%s:\t%s\n", #wid, \
			_OlWidgetClassOfAncestory(wid))
	
	/* Instantiate the widget tree */
	CREATE_WIDGET(panel_ca, controlArea, parent);

	CREATE_WIDGET(one_ob, oblongButton, panel_ca);

	two_ob = XtVaCreateWidget("two_ob", oblongButtonWidgetClass, 
		panel_ca, 
			XtNlabel, 	"Test Button",
		NULL);

	/* Manage all children by their parents */
	{
		Widget		children[MAX_CHILDREN];
		Cardinal	num_children;
	
		num_children = 0;
		children[num_children++] = one_ob;
		children[num_children++] = two_ob;
		XtManageChildren(children, num_children);
		
		XtManageChild(panel_ca);
	}
	
	SHOW_ROOT(two_ob);
	SHOW_ROOT(panel_ca);
	SHOW_ROOT(parent);

	OUTPUT(parent, XtName(parent));
	OUTPUT(parent, "panel_ca");
	OUTPUT(parent, "one_ob");
	
	SHOW_IHIER(parent);
	SHOW_IHIER(panel_ca);
	SHOW_IHIER(one_ob);

	SHOW_CHIER(parent);
	SHOW_CHIER(panel_ca);
	SHOW_CHIER(one_ob);

	_OlWidgetPrintInstanceTree(parent);
	
	(void) printf("=> %s\n", _OlWidgetResourceValue(two_ob, XtNbackground));
	(void) printf("=> %s\n", _OlWidgetResourceValue(two_ob, 
		XtNinputFocusColor));
	(void) printf("=> %s\n", _OlWidgetResourceValue(two_ob, XtNsupercaret));

	#undef	SHOW_ROOT
	#undef	OUTPUT
} /* end of construct_basewin() */


/************************************************************************
 *
 *  _OlDiagsTest --
 *
 ****************************procedure*header******************************/

void
_OlDiagsTest(const int argc, const char* const argv[])
{
	const char* fallback_resources[] = { 
		"*Background:		gray",
		"*title:		Test through _OlMain()",
		NULL
	};

	diags_test();
	_OlMain(argc, argv, fallback_resources, construct_basewin);
} /* end of _OlDiagsTest() */


#endif	/* DEBUG */


/* end of diags.c */
