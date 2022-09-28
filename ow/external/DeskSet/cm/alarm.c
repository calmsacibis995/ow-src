#ifndef lint
static  char sccsid[] = "@(#)alarm.c 3.14 94/05/06 Copyr 1991 Sun Microsystems, Inc.";
#endif

#include <stdio.h>

#include <fcntl.h>
#include <rpc/rpc.h>
#include <sys/file.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <errno.h>
#ifdef __STDC__
#include <stdarg.h>
#endif /* __STDC__ */

#ifdef SVR4
#include <sys/kbd.h>
#include <sys/kbio.h>
#else
#include <sundev/kbd.h>
#include <sundev/kbio.h>
#endif SVR4

#include <xview/xview.h>
#include <xview/canvas.h>
#include <xview/panel.h>
#include <xview/textsw.h>
#include <xview/scrollbar.h>
#include <xview/wmgr.h>
#include <xview/font.h>
#include <xview/svrimage.h>
#include <xview/cms.h>
#include "util.h"
#include "appt.h"
#include "graphics.h"
#include "table.h"
#include "calendar.h"
#include "datefield.h"
#include "props.h"
#include "postup.h"
#include "mail.h"

int kbdfd;
extern void reset_timer();

static 
slp(x)
{
	struct timeval tv;
	tv.tv_sec = x/64;
	tv.tv_usec = (x % 64) * (1000000/64);
	(void) select(32, (fd_set *)NULL, (fd_set *)NULL, (fd_set *)NULL, &tv);
}

/* ARGSUSED */
extern Notify_value 
flash_it(client, arg)
	Notify_client client; int arg;
{
	int w, h, i, j;
	Calendar *c;

	c = (Calendar *) xv_get(client, WIN_CLIENT_DATA, 0);

	if (xv_get(c->frame, FRAME_CLOSED)) {
		Server_image si;
		XContext *xc;

		si = (Server_image) icon_get(c->icon, ICON_IMAGE);

		/*
		 * Should save this so we don't have to keep creating it
		 * and throwing it away
		 */
		xc = gr_create_xcontext(c->icon, si, gr_mono);

		w = (int) icon_get(c->icon, ICON_WIDTH);
		h = (int) icon_get(c->icon, ICON_HEIGHT);
		for (i=0; i < 4; i++) {
			for (j=0; j< 24; j++) {
			gr_invert_box(xc, 0, 0, w, h);
			icon_set(c->icon, ICON_IMAGE, si, 0);
			gr_invert_box(xc, 0, 0, w, h);
			icon_set(c->icon, ICON_IMAGE, si, 0);
			slp(1);
			}
		}

		gr_free_xcontext(xc);
	}
	else {
		w = (int) xv_get(c->canvas, XV_WIDTH, 0);
		h = (int) xv_get(c->canvas, XV_HEIGHT, 0);
		for (i=0; i < 3; i++) {
			for (j=0; j<10; j++) {
				gr_invert_box(c->xcontext, 0, 0, w, h);
				gr_invert_box(c->xcontext, 0, 0, w, h);
				slp(2);
			}
		}
	}
	return (NOTIFY_DONE);
}

/* beeps is used to beep the screen. jff  */

static void
beeps (client, duration)
	Notify_client	client;
	int duration;
{
	XContext* xc =
		((Calendar *)window_get(client, WIN_CLIENT_DATA))->xcontext;

	XBell(xc->display, 50);
	slp (duration);
}

/* VARARGS0 */
extern void
#ifdef __STDC__
beep_it(Notify_client client, ...)
#else /* __STDC__ */
beep_it(client, va_alist)
	Notify_client	client;
	va_dcl	
#endif /* __STDC__ */
{
	int p;
	va_list ap;

#ifdef __STDC__
        va_start(ap, client);
#else /* __STDC__ */
	va_start (ap);
#endif /* __STDC__ */
	for (;;) {
		p = va_arg (ap, int);
		if (p==0) break;
		beeps (client, p);
		p = va_arg (ap, int);
		if (p==0) break;
	}
	va_end (ap);
}

/* ARGSUSED */
extern Notify_value
ring_it(client, arg)
        Notify_client client; int arg;
{
	(void) beep_it(client,  5, 4, 5, 4, 5, 4, 5, 4, 5, 0);
	return (NOTIFY_DONE);
}

/* ARGSUSED */
extern Notify_value
open_it(client, arg)
	Notify_client client; int arg;
{
	Calendar *c = (Calendar *) xv_get (client, WIN_CLIENT_DATA, 0);
	Reminder *r = (Reminder *)c->view->next_alarm;
	Appt	*a;
	Uid 	id;
	
	id.next = NULL;
	id.appt_id = r->appt_id;
	table_lookup(c->calname, &id, &a);

	if (a != NULL) {
		(void)postup_show_proc(c->frame, a);
		destroy_appt(a);
	}
	return (NOTIFY_DONE);
}

static void
compose(a)
	Appt *a;
{
	int hr=0, pm=0;
	FILE *f;
	char buf[50];
	Lines *lines=NULL, *l=NULL;
	Boolean found = false;
	Props *p = (Props*)calendar->properties;

	(void) cm_strcpy(cm_mailfile, "/tmp/.calmailXXXXXX");
	(void) mktemp(cm_mailfile);
	if ((f = fopen(cm_mailfile, "w")) == NULL) {
		/* alert */
		return;
	}
	if (a->attr != NULL) {
		struct Attribute *item = a->attr;
		while (item != NULL) {
			if (strcmp(item->attr, "ml")==0) {
				(void) fprintf (f, "To: %s\n",
					item->clientdata);
				found = true;
				break;
			}
			item = item->next;
		}
	}
	if (!found)
		(void) fprintf (f, "To: %s\n", a->author);

	lines = text_to_lines(a->what, 5);
	if (lines != NULL)
		(void) fprintf(f, "Subject: Reminder- %s\n\n", lines->s);
	else
		(void) fprintf(f, "Subject: Reminder- \n\n");

	(void) fprintf(f, "\t** Calendar Appointment **\n\n");	
	format_tick(a->appt_id.tick, p->ordering_VAL, p->separator_VAL, buf);
	(void) fprintf(f, "\tDate:\t%s\n", buf);
	hr = hour(a->appt_id.tick);
	if (p->default_disp_VAL == hour12 && !adjust_hour(&hr))
		pm = 1;
	if (a->tag->showtime && !magic_time(a->appt_id.tick))
		if (p->default_disp_VAL == hour12)
			(void) fprintf(f, "\tStart:\t%2d:%02d %s\n", hr,
				minute(a->appt_id.tick), pm?"pm":"am");
		else
			(void) fprintf(f, "\tStart:\t%02d%02d \n", hr,
				minute(a->appt_id.tick));
	else
		(void) fprintf(f, "\tStart:\t\n");
	pm = 0;
	hr = hour(a->appt_id.tick+a->duration);
	if (p->default_disp_VAL == hour12 && !adjust_hour(&hr))
		pm = 1;
	if (a->tag->showtime && !magic_time(a->appt_id.tick))
		if (p->default_disp_VAL == hour12)
			(void) fprintf(f, "\tEnd:\t%2d:%02d %s\n", hr,
			minute(a->appt_id.tick+a->duration), pm?"pm":"am");
		else
			(void) fprintf(f, "\tEnd:\t%02d%02d \n", hr,
			minute(a->appt_id.tick+a->duration));
	else
		(void) fprintf(f, "\tEnd:\t\n");

	l = lines;
	(void) fprintf(f, "\tWhat:\t");
	if (l != NULL) {
		(void) fprintf(f, "%s\n", l->s);
		l = l->next;
	}
	while(l != NULL) {
		(void) fprintf(f, "\t\t%s\n", l->s);
		l = l->next;
	}
	if (lines != NULL)
		destroy_lines(lines);

	(void) fclose(f);
}
	
/* ARGSUSED */
extern Notify_value 
mail_it(client, arg) 
	Notify_client client; int arg;
{
	Calendar *c = (Calendar *) xv_get (client, WIN_CLIENT_DATA, 0);
	Reminder *r = (Reminder *)c->view->next_alarm;
	Uid     id;
	Appt	*a;
	FILE    *f;
         
        id.next = NULL;
        id.appt_id = r->appt_id;
	table_lookup(c->calname, &id, &a);

	if (a==NULL) 
		return(NOTIFY_DONE);

	compose(a);
	f = fopen(cm_mailfile, "r");
	unlink(cm_mailfile);

	if (vfork()==0) {
		dup2(fileno(f), 0);
#ifdef SVR4
                execlp(MAIL_BINARY_NAME, MAIL_BINARY_NAME, "-t", 0);
		                              /* try using $PATH first */
		if (errno == ENOENT) {
		    execlp(MAIL_FIXED_PATH, MAIL_FIXED_PATH, "-t", 0);
                                              /* failed, try hard-coded path */
                }
#else
                execlp("Mail", "Mail", "-t", 0);
#endif "SVR4"
		_exit(-1);
	}
	destroy_appt(a);
	fclose(f);
	return (NOTIFY_DONE);
}



#ifdef SCRIPTS
static void
background_it(command)

char	*command;

{
        int j;   
	int	err_fd;

        if (fork() == 0) {

		/* create a new process group for the child, so 
		   quitting the parent will not destroy it. */

#ifdef SVR4
		setpgid(getpid(), 0);
#else
		setpgrp(getpid(), 0);
#endif "SVR4"

        	/* Map stderr and stdout to console so we can 
		   report errors */

        	if ((err_fd = open("/dev/console" , O_WRONLY, 00666)) == -1)
                	_exit(-1);

                if (dup2(err_fd, 1) == -1 || dup2(err_fd, 2) == -1)
                        (void)fprintf(stderr, "cm: dup2 failed trying to execute: %s\n", command);

		/* close all open file descriptors for the child process */

#ifdef SVR4
		for (j = (int)sysconf(_SC_OPEN_MAX); j > 2; j--)
#else
                for (j = getdtablesize(); j > 2; j--)
#endif "SVR4"
                        (void)close(j);

		/* run the shell to do the command */
	
		(void) execl("/bin/sh", "sh", "-c", command, (char *)0);

                _exit(127);
        }

	/* do not wait for exit status */
}
#endif /* SCRIPTS */


/* ARGSUSED */
/* SCRIPTS
extern Notify_value
run_it(client, arg)
	Notify_client client; int arg;
{
	Calendar *c = (Calendar *) xv_get (client, WIN_CLIENT_DATA, 0);
	Reminder *r = (Reminder *)c->view->next_alarm;
	Uid	id;
	Appt	*a;

	id.next = NULL;
	id.appt_id = r->appt_id;
	table_lookup(c->calname, &id, &a);
	if (a==NULL) 
		return(NOTIFY_DONE);

	if (a->script != NULL) {
#if 0
		(void) system(a->script);
#endif
		(void) background_it(a->script);
	}
	destroy_appt(a);
	return (NOTIFY_DONE);
}

static Boolean
has_access(c, access, name)
	Calendar        *c;
        int            access;
        char            *name;
{
	char    *list_name, *login_name;
	Access_Entry    *list=NULL;
	Stat	status; 

	login_name = cm_target2name(name); 
	if (strcmp(cm_target2name(c->user), login_name) == 0) {
		free(login_name);
		return true;
	}
	free(login_name);
	status = table_get_access(c->user, &list);
	if (status != status_ok) {
		if (status == status_denied)
			xv_set(c->frame, FRAME_LEFT_FOOTER,
			"Access Denied for Running Script.", NULL);
		return false;
	}
        while (list != NULL) {
		list_name = cm_target2name(list->who);
                if (strcmp(login_name, list_name) == 0 || 
			strcmp(list_name, WORLD) == 0) {
			free(list_name);
			if (list->access_type & access)
				return true;
		}
		else 
			free(list_name);
		list = list->next;
        }
        return false;
}
*/
	
extern Notify_value
reminder_driver(client, arg)
	Notify_client client; int arg;
{
	char *action;
	Calendar *c;
	Reminder *save_r, *r;

	c = (Calendar *) xv_get(client, WIN_CLIENT_DATA);
		
	save_r = r = (Reminder *) c->view->next_alarm;
	if (r==NULL) {
		reset_timer();
		return(NOTIFY_DONE);
	}

	while(r != NULL) {
		action = r->attr.attr;
		if (strcmp(action, "bp")==0) 
			(void) ring_it(client, arg);
		else if (strcmp(action, "fl")==0) 
			(void) flash_it(client, arg);
		else if (strcmp(action, "op")==0) 
			(void) open_it(client, arg);
		else if (strcmp(action, "ml")==0) 
				(void) mail_it(client, arg);
		r = r->next;
		c->view->next_alarm = (caddr_t)r;
	}
	c->view->next_alarm = (caddr_t)save_r;
	reset_timer();
	return(NOTIFY_DONE);
}

