#ident "@(#)utils.cc	1.35 11/15/96 Copyright 1989 Sun Microsystems, Inc."

#include <sys/param.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "common.h"
#include <xview/xview.h>
#include <X11/Xlib.h>
#include <doc/string.h>

/*
 * We have to undef all the macros defined in varargs.h (va_alist, va_arg,
 * va_dcl, va_end, va_list, and va_start) because we want to use the defines
 * in stdarg.h and XView wants to use varargs.h and the two of them
 * (varargs.h and stdargs.h) won't play together.
 */

#ifdef _sys_varargs_h
#undef va_alist
#undef va_arg
#undef va_dcl
#undef va_end
#undef va_list
#undef va_start
#endif /* _sys_varargs_h */

#include <stdarg.h>

// Local (to this file) static variables
static STRING	savedCmd;

typedef struct rusage struct_rusage ;
static Notify_value	WaitHandler(Notify_client,
				    int,
				    int *,
				    struct_rusage *);

void
BoxToRect(const BBox &box, Rect &rect)
{
	DbgFunc("utils.cc:BoxToRect: entered" << endl);

	rect.r_left	= box.ll_x;
	rect.r_top	= box.ur_y;
	rect.r_width	= DocWdth(box);
	rect.r_height	= DocHght(box);

	return;
}

void
NewHandler()
{
	console.Message(gettext("out of memory"));
	exit(ENOMEM);
}

#ifdef DEBUG
void
PrintRect(const char *str, const Xv_opaque xvobj)
{
	Rect		rect;
	Rect	       *ptr	= &rect;
	const Xv_pkg   *type	= (Xv_pkg *) xv_get(xvobj, XV_TYPE);

	if (type == FRAME_CLASS) {
		frame_get_rect(xvobj, ptr);
	}
	else {
		ptr = (Rect *) xv_get(xvobj, XV_RECT);
	}

	if (!ptr) {
		DbgHigh("PrintRect: rect is null!" << endl);
	}
	else {
		
		DbgMed(str << ": r_left = " << ptr->r_left << ", r_top = "
		       << ptr->r_top << ", r_width = " << ptr->r_width
		       << ", r_height = " << ptr->r_height << endl);
	}

	return;
}
#endif /* DEBUG */

/*
 * We cannot use the libc version of system() with the Notifier, so I am
 * rolling my own
 */
STATUS
System(const STRING &cmd_str, const STRING &dir)
{
	// Local variables

	pid_t			pid;

	static int	dummy_client_obj = 0;
	const Notify_client	dummyPtr = (Notify_client) &dummy_client_obj;

	if ((pid = fork()) == NULL) {
		DbgMed("utils.cc:System: changing dir to \""
		       << dir << "\"" << endl);

		if (dir != NULL_STRING)
			(void) chdir(dir);

		execl("/bin/sh", "sh", "-c", (const char *) cmd_str, NULL);
		console.Perror(gettext("shell command failed '%s'"),
			       ~cmd_str);
		_exit(0x7f);
	}
	else if (pid < 0) {
		console.Perror(gettext("can't fork shell command"));
		return(STATUS_FAILED);
	}
	else {
		savedCmd = cmd_str;
		(void) notify_set_waitpid_func(dummyPtr,
					       (Notify_func) WaitHandler,
					       (int) pid);
	}
	return(STATUS_OK);
}

static Notify_value
WaitHandler(Notify_client	/* clnt */,
	    int			/* pid */,
	    int			*status,
	    struct_rusage	* /* rusage */)
{
	if (WIFEXITED(*status)) {
		if (WEXITSTATUS(*status) != 0) {
			console.Message(
				gettext("command exited with status %d: '%s'"),
				WEXITSTATUS(*status),
				savedCmd);
		}
		return(NOTIFY_DONE);

	} else {
		return(NOTIFY_IGNORED);
	}
}
