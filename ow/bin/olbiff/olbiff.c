#pragma ident "@(#)olbiff.c	1.13 95/02/22"
/*
 *      Copyright (C) 1990, 1991 Sun Microsystems, Inc
 *                 All rights reserved.
 *       Notice of copyright on this source code 
 *       product does not indicate publication. 
 *
 * RESTRICTED RIGHTS LEGEND: Use, duplication, or disclosure by 
 * the U.S. Government is subject to restrictions as set forth 
 * in subparagraph (c)(1)(ii) of the Rights in Technical Data
 * and Computer Software Clause at DFARS 252.227-7013 (Oct. 1988) 
 * and FAR 52.227-19 (c) (June 1987).
 * 
 *   Sun Microsystems, Inc., 2550 Garcia Avenue,
 *   Mountain View, California 94043.
 */ 
/*
 *  Name:        olbiff
 *
 *  Module description:
 *
 *      olbiff is an OLIT program which monitors the system for any new mail
 *	messages. When a new mail message arrives, olbiff will display the
 *	author and subject as a listbox item inside a popup notice widget.
 *	There is also an acknowledge button which can be used to dismiss the
 *	popup and clear the listbox.
 *	This program conforms to the OPENLOOK specification.
 *
 *  Author:     John S. Cooper          Sun Microsystems Inc.
 *
 *  Date:       March 8th 1991
 */

#include <X11/Xlib.h>
#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <Xol/OpenLook.h>
#include <Xol/ControlAre.h>
#include <Xol/OblongButt.h>
#include <Xol/Notice.h>
#include <Xol/ScrollingL.h>
#include <stdio.h>
#include <stdlib.h>
#include <libintl.h>
#include "enconv.h"
#include "mclist.h"

#ifdef SYSV
#include <string.h>
#include <signal.h>
#include <maillock.h>
#else
#include <strings.h>
#include <sys/file.h>
#endif

#include <unistd.h>
#include <pwd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

#define OlbiffDomain	"SUNW_WST_OLBIFF"
#define BUFSIZE		1000
#define NUMITEMS	20

static void getMailFile(void);
static void timer(XtPointer client_data,XtIntervalId *id);
static void popup(String *string_list);
static void createPopup(void);
static void noticeCB(Widget w,XtPointer client_data, XtPointer call_data);
static void addListItem(String string);
static Boolean getFromAndSubject(char buffer[],FILE *mail,char **msg);
static void error(String msg);
static void usage(void);
static void (*deleteFunc)();
static void (*touchFunc)();
static void (*viewFunc)();
static OlListToken (*addFunc)();
static Boolean checkNewMail(void);
static String *findWhoFrom(void);
#ifdef SYSV
static void clear_lock(int signal);
#endif /* SYSV */

Widget toplevel, emanate, noticeShell, noticeText, scrollingList;
unsigned long fileSize = 0;
OlListItem *items;
OlListToken *tokens;
int num_items;
int cur_item = 0;


typedef struct {
	String mail_file;
	int interval;
	int show;
	int subject_len;
	int from_len;
	Boolean bell;
	Boolean center;
	Boolean help;
	Boolean history;
} ApplicationData, *ApplicationDataPtr;

static XrmOptionDescRec options[] = {
	{ "-file",	".mailFile",	XrmoptionSepArg,	NULL },
	{ "-interval",	".interval",	XrmoptionSepArg,	NULL },
	{ "-show",	".show",	XrmoptionSepArg,	NULL },
	{ "-sl",	".subjectLen",	XrmoptionSepArg,	NULL },
	{ "-fl",	".fromLen",	XrmoptionSepArg,	NULL },
	{ "-bell",	".bell",	XrmoptionNoArg,		"true" },
	{ "-center",	".center",	XrmoptionNoArg,		"true" },
	{ "-help",	".help",	XrmoptionNoArg,		"true" },
	{ "-history",	".history",	XrmoptionNoArg,		"true" }
};

#define offset(F)	XtOffsetOf(ApplicationData, F)
static XtResource resources[] = {
	{ "mailFile", "MailFile", XtRString, sizeof(String),
	  offset(mail_file), XtRString, (XtPointer)NULL },
	{ "interval", "Interval", XtRInt, sizeof(int),
	  offset(interval), XtRImmediate, (XtPointer) 30},
	{ "show", "Show", XtRInt, sizeof(int),
	  offset(show), XtRImmediate, (XtPointer) 3},
	{ "subjectLen", "SubjectLen", XtRInt, sizeof(int),
	  offset(subject_len), XtRImmediate, (XtPointer) 40},
	{ "fromLen", "FromLen", XtRInt, sizeof(int),
	  offset(from_len), XtRImmediate, (XtPointer) 40},
	{ "bell", "Bell", XtRBoolean, sizeof(Boolean),
	  offset(bell), XtRImmediate, (XtPointer) False},
	{ "center", "Center", XtRBoolean, sizeof(Boolean),
	  offset(center), XtRImmediate, (XtPointer) False},
	{ "help", "Help", XtRBoolean, sizeof(Boolean),
	  offset(help), XtRImmediate, (XtPointer) False},
	{ "history", "History", XtRBoolean, sizeof(Boolean),
	  offset(history), XtRImmediate, (XtPointer) False}
};
#undef offset

ApplicationData app_data;

char	lc_messages[126];
char	*lc_string;
char	username[40];
XtAppContext	app;

void
main(int argc,char **argv)
{

	char		*dirname;
	char		*localename;
	char		*pwdpath;
	char		domainpath[126];
	char		mofile[126];
	int		fd;
	void		(*prev)(int);

        if((dirname = getenv("OPENWINHOME")) == NULL){
                (void)sprintf((char *)lc_messages,"%s",
                        dgettext(OlbiffDomain, "Please set OPENWINHOME"));
                lc_string = (char *)XtMalloc(strlen(lc_messages)+1);
                strcpy(lc_string,lc_messages);
                fprintf(stderr,"%s",lc_string);
                exit(1);
        }
        (void)sprintf(domainpath,"%s/lib/locale",dirname);
        if((localename = getenv("LANG")) != NULL){
                (void)sprintf(mofile, "%s/lib/locale/%s/LC_MESSAGES/%s.mo",
                                dirname,localename,OlbiffDomain);
        }
        else{
                (void)sprintf(mofile, "%s/lib/locale/C/LC_MESSAGES/%s.mo",
                                dirname,OlbiffDomain);
        }

        if( (fd = open(mofile,O_RDONLY)) != -1){
                close(fd);
                (void)bindtextdomain(OlbiffDomain,domainpath);
        }
        else{
                pwdpath = getenv("PWD");
                (void)bindtextdomain(OlbiffDomain,pwdpath);
        }

	OlToolkitInitialize(NULL);
	OlSetDefaultTextFormat(OL_MB_STR_REP);
	toplevel = XtAppInitialize(&app, "Olbiff", options,
		XtNumber(options), &argc, argv, NULL, NULL, 0);

	XtGetApplicationResources(toplevel, &app_data, resources,
				  XtNumber(resources), NULL, 0);

	if (app_data.help || argc > 1) {
	    usage();
	    exit(0);
	}

	getMailFile();

	if (!app_data.center) {
	    Arg args[2];
	    Cardinal ac = 0;

	    XtSetArg(args[ac], XtNwidth, (XtArgVal) 1);			++ac;
	    XtSetArg(args[ac], XtNheight, (XtArgVal) 1);		++ac;
	    emanate = XtCreateManagedWidget("emanate", widgetClass,
						toplevel, args, ac);

	    XtSetMappedWhenManaged(toplevel, False);
	    XtRealizeWidget(toplevel);
	}

        num_items = NUMITEMS;
        items = (OlListItem *) XtMalloc(sizeof(OlListItem) * num_items);
        tokens = (OlListToken *) XtMalloc(sizeof(OlListToken) * num_items);

#ifdef SYSV
	/* need to remove the spool file lock on process termination */
	prev = signal(SIGHUP, clear_lock);
	prev = signal(SIGINT, clear_lock);
	prev = signal(SIGQUIT, clear_lock);
	prev = signal(SIGTERM, clear_lock);
#endif /* SYSV */

	XtAppAddTimeOut(app, 0, timer, (XtPointer)NULL);

	XtAppMainLoop(app);
}


static void
getMailFile()
{
	struct passwd *pw;
	extern struct passwd *getpwuid(uid_t uid);
	struct stat status;
	char buffer[BUFSIZE];
	char *ptr;

#ifdef SYSV
	/*
	 * Store the username in the global variable, to avoid
	 * having to find it in each call to checkNewMail()
	 */
	if (!(ptr = cuserid(username)))
	    error(dgettext(OlbiffDomain,
			   "getMailFile - couldn't get username\n"));
#endif /* SYSV */

	if (app_data.mail_file == NULL) {
	    if ((ptr = getenv("MAIL")) == NULL)
		ptr = getenv("mail");
	    if (ptr != NULL)	/* got it! */
		app_data.mail_file = XtNewString(ptr);
	    else {
		pw = getpwuid(getuid());
		if (pw != NULL) {
#ifdef SYSV
		    (void) sprintf(buffer, "/var/mail/%s", pw->pw_name);
#else
		    (void) sprintf(buffer, "/usr/spool/mail/%s", pw->pw_name);
#endif /* SYSV */
		    app_data.mail_file = XtNewString(buffer);
		}
		else
		    error(dgettext(OlbiffDomain,
		"Couldn't find MAIL directory\nTry olbiff -file <mail_file>"));
	    }
	}
}

static void
timer(XtPointer client_data, XtIntervalId *id)
{
	if (!checkNewMail()) {
	    XtAppAddTimeOut(app , app_data.interval * 1000, timer, 
							(XtPointer)NULL);
	}
}

static Boolean
checkNewMail()
{
        FILE *mail;
	struct stat status;

#ifndef SYSV
	if ((mail = fopen(app_data.mail_file, "r")) == NULL)
	    return False;

	/*
	 * Attempt to lock the file in case a mail reader is
	 * currently modifying it. This call blocks until the
	 * lock can be obtained
	 */
	if ((flock(fileno(mail), LOCK_EX)) != 0)
	    fprintf(stderr, dgettext(OlbiffDomain,
				     "flock failed, errno %d\n"), errno);
#else /* SYSV */
	switch (maillock(username, 2)) {
	case L_SUCCESS:
	    break;
	case L_MAXTRYS:
	    return False;
	case L_ERROR:
	    fprintf(stderr, dgettext(OlbiffDomain,
				     "maillock failed, errno %d\n"), errno);
	    /* fall through */
	default:
	    exit(1);
	}

#endif /* SYSV */

	/*
	 * Some mailers remove the spool file, so we must allow
	 * for this, i.e. not assume an error.
	 */
	if (access(app_data.mail_file, R_OK) != 0)
	    if (errno == ENOENT) {
#ifndef SYSV
		flock(fileno(mail), LOCK_UN);
		fclose(mail);
#else /* SYSV */
		mailunlock();
#endif /* SYSV */
		return False;
	    }
	    else
		error(dgettext(OlbiffDomain,
			       "checkNewMail - Cannot access mail_file"));

	if (stat(app_data.mail_file, &status) != 0)
	    error(dgettext(OlbiffDomain,
			   "checkNewMail - Cannot stat mail_file"));

	if (status.st_size == fileSize ||
			(status.st_size == 0 && app_data.history)) {
#ifndef SYSV
	    flock(fileno(mail), LOCK_UN);
	    fclose(mail);
#else /* SYSV */
	    mailunlock();
#endif /* SYSV */	    
	    return False;
	}

	if (status.st_size == 0 && noticeShell && XtIsRealized(noticeShell)) {
	    XtPopdown(noticeShell);
#ifndef SYSV
	    flock(fileno(mail), LOCK_UN);
	    fclose(mail);
#else /* SYSV */
	    mailunlock();
#endif /* SYSV */
	    return False;
	}

	popup(findWhoFrom());
	fileSize = status.st_size;
#ifndef SYSV
	flock(fileno(mail), LOCK_UN);
	fclose(mail);
#else /* SYSV */
	mailunlock();
#endif /* SYSV */
	return True;
}

static String *
findWhoFrom()
{
	char buffer[BUFSIZE];
	char **string_list, **next_slot;
	static char previousTag[BUFSIZE] = { '\0' };
	FILE *mail = fopen(app_data.mail_file, "r");
	int num_msgs;
	int num_slots;
	static int count = 0;

	++count;
	if (mail == NULL) {
	    error(dgettext(OlbiffDomain,"unknown mail sender"));
	}

	fgets(buffer, BUFSIZE, mail);
	if (strcmp(buffer, previousTag) == 0) {
#ifdef DEBUG
	    printf("Assuming mail file has grown!\n");
#endif
	}
	else {
#ifdef DEBUG
	    printf("Ahh - assuming inc + NEW mail!\n");
	    printf("...resetting fileSize to 0\n");
#endif
	    strcpy(previousTag, buffer);
#ifdef DEBUG
	    printf("previousTag: %s\n", previousTag);
#endif
            /* If history not wanted, clear any existing listbox items */
            if (!app_data.history) {
	        noticeCB(NULL, NULL, NULL);
            }
	    fileSize = 0;	/* mail is different, so rescan from start */
	}

	if (fseek(mail, fileSize, 0) == -1) {
	    fclose(mail);
	    error(dgettext(OlbiffDomain,"fseek failed"));
	}

	num_msgs = 0;
	num_slots = 5;

	string_list = (char **) XtMalloc(sizeof(char *) * num_slots);
	next_slot = string_list;

#ifdef DEBUG
	printf("fileSize is %d\n", fileSize);
#endif
	while (fgets(buffer, BUFSIZE, mail) != NULL) {
#ifdef DEBUG
	    printf("%s\n", buffer);
#endif
	    if ((strncmp(buffer, "From ", strlen("From ")) == 0) &&
		getFromAndSubject(buffer, mail, next_slot)) {
		++num_msgs;
		++next_slot;
#ifdef DEBUG
		printf("num_msgs is %d\n", num_msgs);
#endif
		if (num_msgs == num_slots) {
		    string_list = (char **) XtRealloc((char *)string_list,
					sizeof(char *) * num_slots * 2);
		    next_slot = string_list + num_slots;
		    num_slots *= 2;
		}
	    }
	}
#ifdef DEBUG
	fflush(stdout);
#endif
	if (num_msgs == 0) {
	    error(dgettext(OlbiffDomain,"No messages found in findWhoFrom()"));
	}

	*next_slot = NULL;
	fclose(mail);
	return (string_list);
}

static Boolean
getFromAndSubject(char buffer[], FILE *mail, char **msg)
{
    char from[BUFSIZE];
    char subject[BUFSIZE];
    Boolean gotFromLine = False;
    short fromLen = strlen("From: ");
    short subjectLen = strlen("Subject: ");

    subject[0] = '\0';

    for (;;) {
	if (fgets(buffer, BUFSIZE, mail) == NULL) {
	    fclose(mail);
	    error(dgettext(OlbiffDomain,"couldn't find a From line"));
	}

	if (buffer[0] == '\n')
	    break;
	
	if (strncmp(buffer, "From: ", fromLen) == 0) {
                buffer[strlen(buffer) - 1] = '\0';      /* Replace \n */
                if ((int)(strlen(buffer) - fromLen) > app_data.from_len)
                    buffer[fromLen + app_data.from_len] = '\0';
                (void) sprintf(from, "%s", &buffer[fromLen]);

		gotFromLine = True;	/* set flag to say we're 'OK' */
	}
	else if (strncmp(buffer, "Subject: ", subjectLen) == 0) {
            buffer[strlen(buffer) - 1] = '\0';
            if ((int)(strlen(buffer) - subjectLen) > app_data.subject_len)
                buffer[subjectLen + app_data.subject_len] = '\0';
            (void) sprintf(subject, " :  %s", &buffer[subjectLen]);
        }
    }

    if (!gotFromLine) {
	/*
	 * This probably is an included message under SVR4
	 * (the mailbox format changed such that included "From"
	 * lines don't get changed to ">From")
	 * This makes it more difficult to distinguish messages;
	 * in some cases, included messages may be interpreted as
	 * separate messages. MH on Solaris2.x suffers the same
	 * problem, although sending mail through slocal appears
	 * to be a good work around.
	 */
	return False;
#if 0
        fclose(mail);
        error(dgettext(OlbiffDomain,"Couldn't get \"From: \" line before \\n"));
#endif
    }
    *msg = XtMalloc(strlen(from) + strlen(subject) + 1);
    strcpy((char *)*msg, from);
    strcat((char *)*msg, subject);

    return True;
}


static void
popup(String *string_list)
{
    Arg args[10];
    Cardinal ac = 0;
    String *current_ptr = string_list;

    if (noticeShell == NULL) createPopup();

    while (*current_ptr)
        addListItem(*current_ptr++);
    XtFree((char *)string_list);
    XtAppAddTimeOut(app, app_data.interval * 1000, timer, (XtPointer)NULL);
    XtPopup(noticeShell, XtGrabNone);
}

char            ibuf[BUFSIZ] , obuf[BUFSIZ];

static Boolean
ConvertFunc(String fromstring, String tostring)
{
        enconv_t        cd;
        char            *ip , *op;
        size_t          ileft , oleft;
        char            *lang;
        struct mcent    *chead;
        struct mcent    *cp;

        if((lang = getenv("LANG")) != NULL){
                chead = getmclist(lang);
        }

        if(lang == NULL || chead == NULL){
                return(FALSE);
        }

        if(!((cp = &chead[0]) != NULL && strcmp(cp->cn_locale,lang) == 0)){
                fprintf(stderr,dgettext(OlbiffDomain,
        "/usr/lib/locale/%s/LC_CTYPE/mailcodetab is not correct ."),lang);
                freemclist(chead);
                return(FALSE);
        }

        if((cd = enconv_open( cp->cn_intcode, cp->cn_extcode)) == (enconv_t)-1){                fprintf(stderr,dgettext(OlbiffDomain,
                                "Conversion for mail text failed .\n"));
                freemclist(chead);
                return(FALSE);
        }
 
        ip = fromstring;
        ileft = strlen(fromstring);
        op = obuf;
        oleft = BUFSIZ;
        if(enconv(cd , &ip , &ileft , &op , &oleft) < 0){
                fprintf(stderr,dgettext(OlbiffDomain,
                                "Conversion for mail text failed .\n"));
                freemclist(chead);
                return(FALSE);
        }
        tostring = obuf;
        return(TRUE);
}

static void
addListItem(String string)
{
	String  newstring ;

	newstring = string;
        if(ConvertFunc(string , newstring) == TRUE)
            items[cur_item].label = newstring;
        else
            items[cur_item].label = string;

    items[cur_item].label_type = OL_STRING;
    items[cur_item].mnemonic = '\0';
    items[cur_item].attr = 0;
    tokens[cur_item] = (*addFunc)(scrollingList, 0, 0, items[cur_item]);

    /* Make this latest addition visible */

    if (XtIsRealized(noticeShell)) {
        XRaiseWindow(XtDisplay(noticeShell), XtWindow(noticeShell));
        (*viewFunc)(scrollingList, tokens[cur_item]);
    }

    if (app_data.bell)
        XBell(XtDisplay(noticeShell), 0);

    ++cur_item;
    if (cur_item == num_items) {
	num_items *= 2;
	items = (OlListItem *) XtRealloc((char *)items,
					sizeof(OlListItem) * num_items);
	tokens = (OlListToken *) XtRealloc((char *)tokens,
					sizeof(OlListToken) * num_items);
#ifdef DEBUG
	printf("addListItem() - realloc'ing items & tokens\n");
#endif
    }
}

static void
createPopup()
{
    Widget noticeBox, widget;
    Arg args[5], noticeArgs[2];
    Cardinal ac;

    noticeShell = XtCreatePopupShell("noticeShell", noticeShellWidgetClass,
					app_data.center ? toplevel : emanate,
					NULL, 0);

    XtSetArg(noticeArgs[0], XtNtextArea, (XtArgVal)&noticeText);
    XtSetArg(noticeArgs[1], XtNcontrolArea, (XtArgVal)&noticeBox);
    XtGetValues(noticeShell, noticeArgs, XtNumber(noticeArgs));

    ac = 0;
	(void)sprintf((char *)lc_messages,
				dgettext(OlbiffDomain, "New Mail"));
	lc_string = (char *)XtMalloc(strlen(lc_messages)+1);
	strcpy(lc_string,lc_messages);
    XtSetArg(args[ac], XtNstring, (XtArgVal)lc_string);		++ac;
    XtSetValues(noticeText, args, ac);

    ac = 0;
    XtSetArg(args[ac], XtNviewHeight, (XtArgVal)app_data.show);		++ac;
    XtSetArg(args[ac], XtNtraversalOn, (XtArgVal)False);		++ac;
    scrollingList = XtCreateManagedWidget("list", scrollingListWidgetClass,
					  noticeBox, args, ac);

    ac = 0;
    XtSetArg(args[ac], XtNapplAddItem, (XtArgVal) &addFunc);		++ac;
    XtSetArg(args[ac], XtNapplDeleteItem, (XtArgVal) &deleteFunc);	++ac;
    XtSetArg(args[ac], XtNapplViewItem, (XtArgVal) &viewFunc);		++ac;
    XtSetArg(args[ac], XtNapplTouchItem, (XtArgVal) &touchFunc);	++ac;
    XtGetValues(scrollingList, args, ac);

	(void)sprintf((char *)lc_messages,
				dgettext(OlbiffDomain, "Acknowledge"));
	lc_string = (char *)XtMalloc(strlen(lc_messages)+1);
	strcpy(lc_string,lc_messages);
    widget = XtVaCreateManagedWidget("Acknowledge", oblongButtonWidgetClass,
				noticeBox, 
				XtNlabel ,	lc_string,
				NULL, 0);
    XtAddCallback(noticeShell, XtNpopdownCallback, noticeCB, NULL);
}

static void
noticeCB(Widget w, XtPointer client_data, XtPointer call_data)
{
    int i;
    String string;

    for (i = 0; i < cur_item; i++) {
	string = (OlListItemPointer(tokens[i]))->label;
#ifdef TEMP
	(*touchFunc)(scrollingList, tokens[i]);
#endif
	(*deleteFunc)(scrollingList, tokens[i]);
	XtFree(string);
	/* XtFree((OlListItemPointer(tokens[i]))->label); */
    }
    cur_item = 0;
}

#ifdef SYSV
static void
clear_lock(int signal)
{

#ifdef DEBUG
    printf("Unlocking mail file on signal %d\n", signal);
    fflush(stdout);
#endif /* DEBUG */
    mailunlock();
    exit(1);
}
#endif /* SYSV */

static void
error(String msg)
{
#define APP_PREFIX "Olbiff: "

    fprintf(stderr, "%s%s\n", APP_PREFIX, msg);
    exit(1);
}

static void
usage()
{
    fprintf(stderr,dgettext(OlbiffDomain,"usage: olbiff [ -options ... ]\n"));
    fprintf(stderr,dgettext(OlbiffDomain,"where options include:\n"));
    fprintf(stderr,dgettext(OlbiffDomain,
				"\t-show n\t\tDisplay n lines in ListBox\n"));
    fprintf(stderr,dgettext(OlbiffDomain,
				"\t-interval n\tCheck mail every n seconds\n"));
    fprintf(stderr,dgettext(OlbiffDomain,
		"\t-fl n\t\tDisplay no more than n characters for name\n"));
    fprintf(stderr,dgettext(OlbiffDomain,
		"\t-sl n\t\tDisplay no more than n characters in subject\n"));
    fprintf(stderr,dgettext(OlbiffDomain,
		"\t-bell\t\tBeep the bell when each new message arrives\n"));
    fprintf(stderr,dgettext(OlbiffDomain,
			"\t-help\t\tDisplay this help message\n"));
/* STRING_EXTRACTION -
    * The popup flag refers to the olbiff window which is a popup shell
    * (it can't be iconified).  If the popup flag is set the olbiff popup (i.e.
    * window) is displayed centered on the screen.  If not set it will be
    * displayed at the upper left hand corner.  
    */
    fprintf(stderr,dgettext(OlbiffDomain,
			"\t-center\t\tCenter the popup on the screen\n"));
/* STRING_EXTRACTION -
    * The history flag controls whether the list of items displays previously
    * unacknowledged mail messages when the list is refreshed.

    * Some mailers (esp. MH) truncate the spool file to zero length when new
    * mail is read (`inc' means `incorporate new mail' in MH parlance).
    * Olbiff notices this, and then pops itself down. When more mail arrives,
    * olbiff pops up once again and displays the author and subject of the
    * *new* mail only. If the `-history' option is given, olbiff continues to
    * display the information about all the previous messages too.
*/
    fprintf(stderr,dgettext(OlbiffDomain,
		"\t-history\tMaintain the list of items beyond mail incs\n\t\t\tuntil acknowledge button is pressed\n"));
}
