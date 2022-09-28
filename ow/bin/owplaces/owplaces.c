/*
 *      (c) Copyright 1989, 1990 Sun Microsystems, Inc. Sun design patents
 *      pending in the U.S. and foreign countries. See LEGAL_NOTICE
 *      file for terms of the license.
 */

#ident "@(#)owplaces.c	1.23 95/05/17 owplaces.c SMI"

#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#ifdef SYSV
#include <string.h>
#include <netdb.h>	/* for MAXHOSTLEN */
#include <stropts.h>
#include <poll.h>
#else
#include <strings.h>
#ifndef __STDC__
#define strrchr rindex
#endif
#include <sys/param.h>	/* for MAXHOSTLEN */
#endif
#include <signal.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <X11/Xos.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <X11/cursorfont.h>
#include <X11/Xmu/WinUtil.h>
#include <X11/Xresource.h>
#include <X11/Xutil.h>

#include "owplaces.h"

typedef void	(*VoidFunc)();

/* -----------------------------------------------------------------------
 *	Globals
 * -----------------------------------------------------------------------*/
char	*ProgramName;

Display	*Dpy;
char	*DisplayName;

/*
 *	Output files
 */
FILE	*Output;
char	*OutputFile;
char	*BackupFile;
char	*TmpFile;

/*
 *	Basic program state variables
 */
Bool	InitScript = False;
Bool	PointerClient = False;
Bool	SingleScreen = True;
Bool	ToolWait = False;
Bool	Silent = False;
Bool	Ampersand = False;

/*
 *	all,local,remote,host client machine filtering
 */
enum _clientmachinefilter {
	ALL_MACHINES,
	LOCAL_MACHINES,
	REMOTE_MACHINES,
	SPECIFIED_HOST_MACHINES
} ClientMachineFilter = ALL_MACHINES;
char	*MatchHost;
char	LocalHost[MAXHOSTNAMELEN];

/*
 *	Timer for waiting for client response
 */
struct itimerval Timer = { {0,0}, {30,0} };	/* 30 seconds */
struct itimerval clrtimer = { {0,0}, {0,0} };	/* 0 seconds - clear timer */
Bool		 TimerExpired = False;
Bool		 DoTimer = True;

/*
 *	List of windows we're waiting for to save
 */
Window	SaveList[500];
int	SaveCount;

/*
 *	Non-predefined atoms
 */
Atom	WMProtocolsAtom;
Atom	SaveYourselfAtom;

/*
 *	Error msg buffer
 */
char	ErrBuf[256];

/* Declare/define some stuff mostly specific to localization */
static XrmDatabase	 XrmDB = NULL;
static char		*BasicLocale;
static XrmQuark		EarlyInstanceQ[4];
static XrmQuark		EarlyClassQ[4];

#define	MAX_NODES	500		/* Matches SaveList above */
typedef struct {
	Window		window;
	int		clientArgc;
	char		**clientArgv;
} WindowNode;

typedef struct {
	WindowNode	Nodes[MAX_NODES];
	int	 	Count;
} Clients;

static Clients		*Early;		/* Points to current screen */
static Clients		*Earlies;	/* For all screens */
static Clients		*Ordinary;	/* Points to current screen */
static Clients		*Ordinaries;	/* For all screens */

/* Forward declared functions */
static int		NodeCompare();
static Bool		CheckEarlyClass(char *name);
static Bool		CheckTightBind2(XrmDatabase *DB, XrmBindingList Binds,
					XrmQuarkList Qs,
					XrmRepresentation *Type,
					XrmValue *Value, XPointer Arg);
static void		GetLocaleDefaults();
static void		GetUserDefaults();
static void		GetLocale();

/* -----------------------------------------------------------------------
 *	Usage/Arg-Parsing Functions
 * -----------------------------------------------------------------------*/

/*
 *	Usage - print usage message
 */
void
Usage()
{
	if (Silent) {
		exit(USAGE_ERROR);
		/* NOTREACHED */
	}

	fprintf(stderr,LOCALIZE("Usage:\t%s [option ...]\n"),ProgramName);

#define USAGE(msg)		fprintf(stderr,"%s\n",LOCALIZE(msg))

USAGE("\twhere option is any of:");
USAGE("[ -display dpy ]       Select display to use");
USAGE("[ -single              Search just default screen [default]");
USAGE(" | -multi ]            Search all screens");
USAGE("[ -pointer ]           Print only the client clicked on");
USAGE("[ -timeout nsecs ]     How long to wait for client reply [30 sec]");
USAGE("[ -all                 Print clients on all hosts [default]");
USAGE(" | -local              Print local clients only");
USAGE(" | -remote             Print remote clients only");
USAGE(" | -host hostname ]    Print clients from hostname only");
USAGE("[ -script ]            Creates an initialization script");
USAGE("[ -output file ]       Specifies output file");
USAGE("[ -ampersand ]         Append an ampersand (&) to end of command");
USAGE("[ -tw ]                Prepend toolwait(1) in front of each command");
USAGE("[ -silent ]            Suppress error messages");

	exit(USAGE_ERROR);
	/* NOTREACHED */
}

/*
 *	MissingArg - print missing argument message
 */
MissingArg(arg)
	char	*arg;
{
	fprintf(stderr,LOCALIZE("Missing argument for %s\n"),arg);
	Usage();
}

/*
 *	SetTimeout
 */
SetTimeout(val)
	char	*val;
{
	int	nsecs;
	char	*ptr;

	nsecs = (int)strtol(val,&ptr,10);
	if (val == ptr) {
		fprintf(stderr,LOCALIZE("Bad timer argument \"%s\"\n"),val);
		Usage();
	}
	if (nsecs == 0)
		DoTimer = False;
	else
		Timer.it_value.tv_sec = nsecs;
}

/* -----------------------------------------------------------------------
 *	Main Function
 * -----------------------------------------------------------------------*/

/*
 *	main	
 */
/* ARGSUSED*/
main(argc,argv)
	int	argc;
	char	*argv[];
{
	char	**arg,*domain,*openwinDir,localePath[MAXPATHLEN];
	int	screen,firstScreen,lastScreen;
	int	CatchAlarm();
	int	i;


	ProgramName = strrchr(argv[0],'/');
	if (ProgramName)
		ProgramName++;		/* walk past slash */
	else
		ProgramName = argv[0];

	Output = stdout;

	/* Do some localization stuff */
	domain = ProgramName;
	if ((openwinDir = getenv("OPENWINHOME")) != 0)
		(void)strcpy(localePath,openwinDir);
	else
		(void)strcpy(localePath,"/usr/share");
	(void)strcat(localePath,"/lib/locale");
	(void)bindtextdomain(domain,localePath);
	textdomain(domain);

	if (gethostname(LocalHost,MAXHOSTNAMELEN) == -1) {
		perror("gethostname");
		Error(GENERAL_ERROR,LOCALIZE("Couldn't get host name"));
		/* NOTREACHED */
	}

	/*
 	 *	Parse the cmdline arguments into the program state
	 */
	for (arg = argv, arg++; *arg; arg++) {
		if (!strcmp(*arg,"-display") || !strcmp(*arg,"-d")) {
			if (! ++arg) MissingArg(*arg);
			DisplayName = *arg;
		} else if (!strcmp(*arg,"-single")) {
			SingleScreen = True;
		} else if (!strcmp(*arg,"-multi")) {
			SingleScreen = False;
		} else if (!strcmp(*arg,"-tw")) {
			ToolWait = True;
		} else if (!strcmp(*arg,"-all")) {
			ClientMachineFilter = ALL_MACHINES;
			PointerClient = False;
		} else if (!strcmp(*arg,"-local")) {
			ClientMachineFilter = LOCAL_MACHINES;
			PointerClient = False;
		} else if (!strcmp(*arg,"-remote")) {
			ClientMachineFilter = REMOTE_MACHINES;
			PointerClient = False;
		} else if (!strcmp(*arg,"-host")) {
			if (! ++arg) MissingArg(*arg);
			ClientMachineFilter = SPECIFIED_HOST_MACHINES;
			PointerClient = False;
			MatchHost = *arg;
		} else if (!strcmp(*arg,"-pointer")) {
			PointerClient = True;
			ClientMachineFilter = ALL_MACHINES;
		} else if (!strcmp(*arg,"-output") || !strcmp(*arg,"-o")) {
			if (! ++arg) MissingArg(*arg);
			OutputFile = *arg;
		} else if (!strcmp(*arg,"-ampersand")) {
			Ampersand = True;
		} else if (!strcmp(*arg,"-script")) {
			InitScript = True;
			Ampersand = True;
		} else if (!strcmp(*arg,"-timeout")) {
			if (! ++arg) MissingArg(*arg);
			SetTimeout(*arg);
		} else if (!strcmp(*arg,"-silent")) {
			Silent = True;
		} else {
			fprintf(stderr,LOCALIZE("Bad option \"%s\"\n"),*arg);
			Usage();
		}
	}

	/*
 	 *	Open the display
	 */
	if ((Dpy = XOpenDisplay(DisplayName)) == NULL) {
		sprintf(ErrBuf,LOCALIZE("Couldn't open display \"%s\""),
			XDisplayName(DisplayName));
		Error(DISPLAY_ERROR,ErrBuf);
		/* NOTREACHED */
	}

	/*
	 * Get the locale defaults 
	 */
	GetUserDefaults();
	GetLocale();
	GetLocaleDefaults();

	SaveYourselfAtom = XInternAtom(Dpy,"WM_SAVE_YOURSELF",False);
	WMProtocolsAtom = XInternAtom(Dpy,"WM_PROTOCOLS",False);

	if (OutputFile) 
		OpenFiles();

	if (InitScript)
		PrintScriptHeader();

	/*
	 *	If doing the client the user has clicked on
 	 */
	if (PointerClient) {
		/* Cheese up an "ordinary" client for use in routines below */
		Ordinaries = (Clients *)calloc(1, sizeof (Clients));
		Ordinary = Ordinaries;
		DoPointerClient();
		WaitForReplies();
		PrintClientCommand(Ordinary);

	/*
 	 *	Else do actual screen clients
	 */
	} else {
		if (SingleScreen) {
			firstScreen = DefaultScreen(Dpy);
			lastScreen = firstScreen + 1;
		} else {
			firstScreen = 0;
			lastScreen = ScreenCount(Dpy);
		}

		/*
		 * Check to see if there are any programs which
		 * will start "early". 
		 */
		EarlyInstanceQ[0] = XrmStringToQuark(ProgramName);
		EarlyInstanceQ[1] = XrmStringToQuark("startEarly");
		EarlyInstanceQ[2] = EarlyInstanceQ[3] = NULLQUARK;

		EarlyClassQ[0] = XrmStringToQuark("Owplaces");
		EarlyClassQ[1] = XrmStringToQuark("StartEarly");
		EarlyClassQ[2] = EarlyClassQ[3] = NULLQUARK;

		if (XrmEnumerateDatabase(XrmDB, EarlyInstanceQ, EarlyClassQ,
					 XrmEnumOneLevel,
					 CheckTightBind2, NULL) == True) {
			Earlies = (Clients *) calloc(lastScreen + 1,
						     sizeof (Clients));
			if (Earlies == NULL)
memerror:			Error(GENERAL_ERROR,
				      LOCALIZE("Memory allocation error"));
		} else
			Earlies = Early = NULL;
		Ordinaries = (Clients *) calloc(lastScreen + 1,
						sizeof (Clients));
		if (Ordinaries == NULL)
			goto memerror;

		/*
 		 *	Set up the ALARM handler
		 */
		(void)signal(SIGALRM,(VoidFunc)CatchAlarm);
		
		/*
 		 *	For each screen we're interested in,
		 *	print the clients for that screen, waiting for 
		 *	any replies to our save yourself message.
		 */
		for (screen = firstScreen; screen < lastScreen; screen++) {

			if (Earlies != NULL) 
				Early = &Earlies[screen];
			Ordinary = &Ordinaries[screen];

			DoClientsOfRoot(RootWindow(Dpy,screen));

			/* If configured to do so, crank up timer */
			if (DoTimer) 
				setitimer(ITIMER_REAL,&Timer,
						(struct itimerval *)NULL);

			/* Receive replies from clients */
			WaitForReplies();

			/* Turn off the timer if necessary */
			if(DoTimer) 
			    setitimer(ITIMER_REAL,&clrtimer,
						(struct itimerval *)NULL);

			if (TimerExpired) {
				if (TmpFile)
					unlink(TmpFile);
				Error(TIMEOUT_ERROR,LOCALIZE(
				"Timed out while waiting for client replies"));
				/* NOTREACHED */
			}
		}

		/* 
		 * Next, go through and print all of the accumulated lists.
		 * Write out the "early" entries first, then the regular
		 * entries.
		 */
		if (Earlies != NULL) {
			for (screen = firstScreen;
			     screen < lastScreen; screen++) {

				Early = &Earlies[screen];
				if (InitScript)
					PrintScriptScreenPrologue(screen);
				PrintClientCommand(Early);
				}
		}

		for (screen = firstScreen; screen < lastScreen; screen++) {

			Ordinary = &Ordinaries[screen];
			if (InitScript)
				PrintScriptScreenPrologue(screen);
			PrintClientCommand(Ordinary);
		}
	}

	if (OutputFile) 
		CloseFiles();

	XCloseDisplay(Dpy);

	exit(SUCCESS);
	/* NOTREACHED */
}

/* -----------------------------------------------------------------------
 *	Client Command Functions
 * -----------------------------------------------------------------------*/
/*
 *	DoPointerClient	- do the client that the user selects via the pointer
 */
DoPointerClient()
{
	Window	client = None;
	Window	root = DefaultRootWindow(Dpy);
	Cursor	cursor = XCreateFontCursor(Dpy,XC_crosshair); /* target shape */
	XEvent	event;
	int	status,buttons = 0;

	status = XGrabPointer(Dpy,root,False,ButtonPressMask|ButtonReleaseMask,
			GrabModeSync,GrabModeAsync,root,cursor,CurrentTime);

	if (status != GrabSuccess) {
		Error(DISPLAY_ERROR,LOCALIZE("Couldn't grab pointer"));
		/* NOTREACHED */
	}

	while (client == None || buttons != 0) {

		XAllowEvents(Dpy,SyncPointer,CurrentTime);
		XWindowEvent(Dpy,root,ButtonPressMask|ButtonReleaseMask,&event);

		switch (event.type) {
		/*
		 *	Use the window selected
		 */
		case ButtonPress:
			if (client == None) {
				client = event.xbutton.subwindow;
				if (client == None)
					client = root;
			}
			buttons++;
			break;
		/*
		 *	Ignore all - including any enqueued before the grab
		 */
		case ButtonRelease:
			if (buttons > 0)
				buttons--;
			break;
		}
	}

	XUngrabPointer(Dpy,CurrentTime);

	client = XmuClientWindow(Dpy,client);

	if (client != root) {
		SaveCount = 0;
		DoClient(client);
	}
}

/*
 *	DoClientsOfRoot	- do all clients of the root which match the spec
 */
DoClientsOfRoot(root)
	Window	root;
{
	Window	*children,client,dummy;
	int	i;
	unsigned int nchildren;

	if (!XQueryTree(Dpy,root,&dummy,&dummy,&children,&nchildren)) {
		return;
	}
	SaveCount = 0;
	for (i=0; i<nchildren; i++) {
		client = XmuClientWindow(Dpy,children[i]);
		if (client != None) {
			DoClient(client);
		}
	}
	XFree((char *)children);
}

/*
 *	DoClient	- Do a single client, filtering out any client that
 *			  doesn't match filter spec.  If the window supports
 *			  WM_SAVE_YOURSELF then ask for it, else use any
 *			  existing WM_COMMAND value;
 */
DoClient(client)
	Window	client;
{
	Bool	FilterClientOut();
	Bool	ClientHasSaveYourself();
	XClassHint	 ClassHint;

	if (FilterClientMachineOut(client))
		return;

	/*
	 * If the client is a special "early" client, put it in a different
	 * list from "ordinary" clients.
	 */
	if (Early != NULL) {
		ClassHint.res_name = NULL;
		ClassHint.res_class = NULL;
		if (XGetClassHint(Dpy, client, &ClassHint) != 0) {
			if (CheckEarlyClass(ClassHint.res_name) == True
			 || CheckEarlyClass(ClassHint.res_class) == True)
				Early->Nodes[Early->Count++].window = client;
			else
				Ordinary->Nodes[Ordinary->Count++].window=client;
			XFree(ClassHint.res_name);
			XFree(ClassHint.res_class);
		} else
			Ordinary->Nodes[Ordinary->Count++].window = client;
	} else
		Ordinary->Nodes[Ordinary->Count++].window = client;

	if (ClientHasSaveYourself(client)) 
		SendSaveYourself(client);
}

/*
 *	FilterClientMachineOut	- Filters out a client whose WM_CLIENT_MACHINE
 *			  property does not match the desired filter.
 *			  True means dont do client, False mean do client
 */
Bool
FilterClientMachineOut(client)
	Window	client;
{
	XTextProperty	prop;
	Bool		result = True;

	if (ClientMachineFilter == ALL_MACHINES) {
		return False;
	}
	if (!XGetWMClientMachine(Dpy,client,&prop)) {
		return True;
	}

	switch (ClientMachineFilter) {
	case LOCAL_MACHINES:		/* must have same as LocalHost */
		if (!strcmp(LocalHost,(char *)prop.value)) {
			result = False;
		}
		break;
	case REMOTE_MACHINES:		/* must be different than LocalHost */
		if (strcmp(LocalHost,(char *)prop.value)) {
			result = False;
		}
		break;
	case SPECIFIED_HOST_MACHINES:	/* must have same as MatchHost */
		if (!strcmp(MatchHost,(char *)prop.value)) {
			result = False;
		}
		break;
	}

	XFree((char *)prop.value);

	return result;
}

/*
 *	CheckEarlyClass		- returns True if a window is of the "early"
 *				  class -- primarily useful in asian 
 *				  localization.
 */
Bool
CheckEarlyClass(char	*name)
{
	XrmRepresentation	 Type;
	XrmValue		 Value;
	char			 string[200];
	char			*p1;
	char			*p2;


	if (name == NULL)
		return False;
	EarlyClassQ[2] = EarlyInstanceQ[2] = XrmStringToQuark(name);
	if (XrmQGetResource(XrmDB, EarlyInstanceQ, EarlyClassQ,
			    &Type, &Value) == True) {
		for (p1 = Value.addr, p2 = string; *p1; p1++, p2++) {
			if (p2 >= &string[sizeof(string) - 1])
				break;
			if (isupper(*p1))
				*p2 = tolower(*p1);
			else
				*p2 = *p1;
		}
		*p2 = '\0';

		if (strcmp(string, "yes") == 0
		 || strcmp(string, "on") == 0
		 || strcmp(string, "t") == 0
		 || strcmp(string, "true") == 0
		 || strcmp(string, "1") == 0)
			return True;
	}

	return False;
}

/*
 *	ClientHasSaveYourself	- returns whether the client has set the
 *				  WM_SAVE_YOURSELF property as one of its
 *				  WM_PROTOCOLS.
 */
Bool
ClientHasSaveYourself(client)
	Window	client;
{
	Atom	*protocols;
	int	i,count;
	Bool	result = False;

	if (!XGetWMProtocols(Dpy,client,&protocols,&count)) {
		return False;
	}
	for (i=0; i<count; i++) {
		if (protocols[i] == SaveYourselfAtom) {
			result = True;
			break;
		}
	}
	XFree((char *)protocols);

	return result;
}

/* -----------------------------------------------------------------------
 *	Event Functions
 * -----------------------------------------------------------------------*/

/*
 *	SendSaveYourself - send WM_SAVE_YOURSELF msg to the window,
 *			   and add that client to the list of waiters
 */
SendSaveYourself(client)
	Window	client;
{
	XEvent	clientEvent;

	clientEvent.xclient.type = ClientMessage;
	clientEvent.xclient.display = Dpy;
	clientEvent.xclient.message_type = WMProtocolsAtom;
	clientEvent.xclient.format = 32;
	clientEvent.xclient.window = client;
	clientEvent.xclient.data.l[0] = SaveYourselfAtom;
	clientEvent.xclient.data.l[1] = CurrentTime;

	XSendEvent(Dpy,client,False,NoEventMask,&clientEvent);

	SaveList[SaveCount++] = client;
	XSelectInput(Dpy,client,PropertyChangeMask|StructureNotifyMask);
}

/*
 *	GetNextEvent -- wait for the next event from the server,
 *			or a timeout signal.  Returns True if an event
 *		  	has been gotten or False if the timer has expired.
 */
Bool
GetNextEvent(event)
XEvent		*event;
{
	int	fd = ConnectionNumber(Dpy);
#ifdef SYSV
	struct	pollfd readfds;
#else
	fd_set	readfds;
#endif
	int	ready;

	if (XPending(Dpy) == 0) {

#ifdef SYSV
		readfds.fd = fd;
		readfds.events = POLLIN;

		ready = poll(&readfds,1,INFTIM);
#else
		FD_ZERO(&readfds);
		FD_SET(fd,&readfds);

		ready = select(fd+1,&readfds,NULL,NULL,NULL);
#endif
		if (ready < 0 || TimerExpired)
			return False;
	}

	XNextEvent(Dpy,event);

	return True;
}

/*
 *	WaitForReplies - wait for either timeout, changes of the property,
 *			 or windows going away.
 */
WaitForReplies()
{
	XEvent	event;

	while (!TimerExpired && SaveCount > 0 && GetNextEvent(&event)) {
		switch (event.type) {
		case PropertyNotify:
			if (event.xproperty.atom == XA_WM_COMMAND &&
			    event.xproperty.state == PropertyNewValue) {
				RemoveWaiter(event.xproperty.window);
			}
			break;
		case DestroyNotify:
			RemoveWaiter(event.xdestroywindow.window);
			break;
		default:
			break;
		}
	}
}

/*
 *	RemoveWaiter - if the client is in our list of windows then
 *		       remove it from the list.
 */
RemoveWaiter(client)
	Window	client;
{
	Window	tmp;
	int	i;

	for (i = 0; i < SaveCount; i++) {
		if (client == SaveList[i]) {
			--SaveCount;
			tmp = SaveList[SaveCount];
			SaveList[i] = SaveList[SaveCount];
			SaveList[SaveCount] = tmp;
			break;
		}
	}
}

/* -----------------------------------------------------------------------
 *	Printing Functions
 * -----------------------------------------------------------------------*/

/*
 *	PrintClientCommand - Prints the WM_COMMAND property/command
 */
PrintClientCommand(client)
	Clients	*client;
{
	int i,j;

 	/*
	 * Grab all of the commands for the display of interest
	 */
	for(i = 0; i < client->Count; ++i) 
	    	XGetCommand(Dpy,client->Nodes[i].window, 
			&client->Nodes[i].clientArgv, 
			&client->Nodes[i].clientArgc);

	/* Sort the list */
	qsort(&client->Nodes[0],client->Count,
		sizeof(WindowNode), NodeCompare);

	for(i = 0; i < client->Count; ++i) {
	    
	    	/* If we can't figure out to do with this client, ignore it */
	    	if(client->Nodes[i].clientArgc == 0)
			continue;
		
		/* If the toolwait option is turned on, then do it */
       	    	if (ToolWait)
        		fputs("toolwait ", Output);

		/*
 	 	 * For XView which puts all args into argv[0] - to prevent quoting
	 	 */
		if (client->Nodes[i].clientArgc == 1) {
			if (*client->Nodes[i].clientArgv[0] == '\0')
				continue;
			fputs(client->Nodes[i].clientArgv[0],Output);
			if (Ampersand && !ToolWait)
				fputs(" &",Output);
			fputc('\n',Output);
			continue;
		}
	
		for (j = 0; j < client->Nodes[i].clientArgc; j++) {
			PrintQuotedWord(client->Nodes[i].clientArgv[j]);
			fputc(' ',Output);
		}

		if (Ampersand && !ToolWait)
			fputc('&',Output);

		fputc('\n',Output);

		XFreeStringList(client->Nodes[i].clientArgv);
	}
}

/*
 *	PrintQuotedWord	- Print the word with the necessary quotes
 *			  Borrowed from xlsclients.c
 */
PrintQuotedWord(s)
	char	*s;
{
	register char	*cp;
	Bool		needQuote = False,inQuote = False;
	char		quoteChar = '\'',otherQuote = '"';

	if (*s == '\0') {
		fputs("''",Output);	
	}
	
	for (cp = s; *cp; cp++) {
		if (!((isascii(*cp) && isalnum(*cp)) ||
	              (*cp == '-' || *cp == '_' || *cp == '.' || *cp == '+' ||
		      *cp == '/' || *cp == '=' || *cp == ':' || *cp == ','))) {
			needQuote = True;
			break;
		}
	}
	inQuote = needQuote;
	if (needQuote) 
		fputc(quoteChar,Output);
	for (cp = s; *cp; cp++) {
		if (*cp == quoteChar) {
			if (inQuote)
				fputc(quoteChar,Output);
			fputc(otherQuote,Output);
			{
				char tmp = otherQuote;
				otherQuote = quoteChar;
				quoteChar = tmp;
			}
			inQuote = True;
		}
		fputc(*cp,Output);
	}
	if (inQuote)
		fputc(quoteChar,Output);
}

/*
 *	PrintScriptHeader
 */
PrintScriptHeader()
{
	char	*filename;

	if (OutputFile) {
		filename = strrchr(OutputFile,'/');
		if (filename)
			filename++;	/* walk past slash */
		else
			filename = OutputFile;
	} else {
		filename = ".openwin-init";
	}

	fputs("#!/bin/sh\n",Output);
	fprintf(Output,"# %s - OpenWindows initialization script.\n",filename);
	fputs("# WARNING: This file is automatically generated.\n",Output);
	fputs("#          Any changes you make here will be lost!\n",Output);
	fputs("export DISPLAY\n\n",Output);

	fputs("# Test for pathological case -- no $DISPLAY set\n", Output);
	fputs("if [ \"${DISPLAY}\" = \"\" ]; then\n", Output);
	fputs("\tDISPLAY=:0\n", Output);
	fputs("fi\n\n", Output);

	fputs("# Figure out the proper host/server number\n", Output);
	fputs("BASE=`echo $DISPLAY | sed -e 's/:.*//'`\n", Output);
	fputs("DISPLAYNO=`echo $DISPLAY | sed -e 's/.*://' -e 's/\\..*//'`\n",
		Output);
	fputs("BASEDISPLAY=${BASE}:${DISPLAYNO}\n\n",Output);

	fputs("SETDISPLAYSCREEN() {\n",Output);
	fputs("\tDISPLAY=${BASEDISPLAY}.$1\n",Output);
	fputs("\tif winsysck x11 ; then\n",Output);
	fputs("\t\t:\n",Output);
	fputs("\telse\n",Output);
	fputs("\t\techo No display available for screen $1\n",Output);
	fputs("\t\texit 1\n",Output);
	fputs("\tfi\n",Output);
	fputs("}\n",Output);

        if (ToolWait) {
           fputs("# Note: toolwait is a utility to control client startup.\n", Output);
           fputs("#       For more information, see the toolwait(1) man page.\n", Output);
        }

}

/*
 *	PrintScriptScreenPrologue
 */
PrintScriptScreenPrologue(screen)
	int	screen;
{
	static int last_screen = -1;

	/* This prevents unnecessary resetting the display screen */
	if (screen == last_screen)
		return;
	last_screen = screen;

	fputs("#\n",Output);
	fprintf(Output,"# Start clients on screen %d\n",screen);
	fputs("#\n",Output);
	fprintf(Output,"SETDISPLAYSCREEN %d\n",screen);
	fputs("#\n",Output);
}

/* -----------------------------------------------------------------------
 *	Misc Functions
 * -----------------------------------------------------------------------*/

/*
 *	OpenFiles - sets up filenames and opens the tmp file
 */
OpenFiles()
{
	int	len = strlen(OutputFile);
	char	*filename;
	struct	stat statbuf;

	if (OutputFile == NULL)
		return;

	if (((BackupFile = malloc(len+5)) == NULL) ||
	    ((TmpFile = malloc(len+5)) == NULL)) {
		Error(GENERAL_ERROR,LOCALIZE("Memory allocation error"));
	}
	strcpy(BackupFile,OutputFile);
	strcat(BackupFile,".BAK");
	strcpy(TmpFile,OutputFile);
	strcat(TmpFile,".TMP");

	if (InitScript)
		filename = TmpFile;
	else
		filename = OutputFile;

	if ((Output = fopen(filename,"w")) == NULL) {
		sprintf(ErrBuf,LOCALIZE("Couldn't open %s\n"),filename);
		Error(FILE_ERROR,ErrBuf);
	}

	if (InitScript) {
		(void)fstat(fileno(Output),&statbuf);
		(void)fchmod(fileno(Output),
			statbuf.st_mode | S_IXUSR | S_IXGRP | S_IXOTH);
	}
}

/*
 *	CloseFiles
 */
CloseFiles()
{
	fclose(Output);

	if (InitScript) {
		(void)unlink(BackupFile);
		(void)rename(OutputFile,BackupFile);
		(void)rename(TmpFile,OutputFile);
	}

	free(TmpFile);
	free(BackupFile);
}

/*
 *	CatchAlarm - signal handler for itimer expiration
 */
CatchAlarm()
{
	TimerExpired = True;
}

/*
 *	Error - error handler
 */
Error(exitCode,errMsg)
	int	exitCode;
	char	*errMsg;
{
	if (!Silent) {
		fprintf(stderr,LOCALIZE("%s: %s\n"),ProgramName,errMsg);
	}
	
	exit(exitCode);
	/* NOTREACHED */
}

/*
 * 	GetUserDefaults - Open a user's resource manager file. The resource
 *			  database can be referenced globally by the XrmDB
 *			  handle.
 */
static void
GetUserDefaults()
{
    XrmDatabase		 UserDB = NULL;
    XrmDatabase		 EnvDB = NULL;
    char		*XrdbStr;
    char		*HomeDir;
    char		*XenvFile;
    char		 FileName[MAXPATHLEN];


    HomeDir = getenv("HOME");

    if ((XrdbStr = XResourceManagerString(Dpy)) != NULL) {
	UserDB = XrmGetStringDatabase((char *) XrdbStr);
    } else {
	if (HomeDir != NULL) {
	    (void) sprintf(FileName, "%s/.Xdefaults", HomeDir);
	    UserDB = XrmGetFileDatabase(FileName);
	}
    }

    /*
     * I guess, we have to try XENVIRONMENT or ~/.Xdefaults-hostname.
     */
    if ((XenvFile = getenv("XENVIRONMENT")) == NULL) {
	if (HomeDir != NULL
	 && LocalHost[0] != 0) {
	    (void) sprintf(FileName, "%s/.Xdefaults-%s", HomeDir, LocalHost);
	    EnvDB = XrmGetFileDatabase(FileName);
	}
    } else
	EnvDB = XrmGetFileDatabase(XenvFile);

    if (EnvDB != NULL)
	XrmMergeDatabases(EnvDB, &UserDB);

    if (UserDB != NULL)
	XrmMergeDatabases(UserDB, &XrmDB);
}

/*
 * 	GetLocale - Get the preferred local to use. First, try getting it from
 *		    the user's XrmDB. If that fails, try using using 
 *		    setlocale() to figure out what localization to use.
 */
static void
GetLocale()
{
    XrmValue		 Value;
    char		*Type;
    char		 Instance[100];


    /*
     * Looking for Xrm first.
     */
    if (XrmGetResource(XrmDB, "OpenWindows.BasicLocale",
			      "OpenWindows.BasicLocale", &Type, &Value))
	BasicLocale = Value.addr;

    (void) sprintf(Instance, "%s.basicLocale", ProgramName);
    if (XrmGetResource(XrmDB, Instance, Instance, &Type, &Value))
	BasicLocale = Value.addr;

    /*
     * Now look for environment via setlocale to fill rest.
     */
    if (BasicLocale == NULL) {
	Type = setlocale(LC_CTYPE, "");
	if ((BasicLocale = malloc(strlen(Type) + 1)) == NULL) {
	    Error(GENERAL_ERROR, LOCALIZE("Memory allocation error"));
	}
	(void) strcpy(BasicLocale, Type);
    } else
	(void) setlocale(LC_ALL, BasicLocale);

    if (XSupportsLocale() != True) {
	(void) sprintf(ErrBuf, LOCALIZE("locale(%s) is not supported by Xlib"),
		       BasicLocale);
	Error(GENERAL_ERROR, ErrBuf);
    }
}

/*
 *	GetLocaleDefaults - Setup the locale-specific openwin defaults.
 */
static void
GetLocaleDefaults()
{
    XrmDatabase		 LocaleDB = NULL;
    char		*OpenwinHome;
    char		 FileName[MAXPATHLEN];

    
    if ((OpenwinHome = getenv("OPENWINHOME")) == NULL)
	OpenwinHome = "/usr/openwin";

    (void) sprintf(FileName, "%s/lib/locale/%s/app-defaults/Owplaces",
		   OpenwinHome, BasicLocale);

    LocaleDB = XrmGetFileDatabase(FileName);

    if (LocaleDB != NULL) {
	XrmMergeDatabases(XrmDB, &LocaleDB);
	XrmDB = LocaleDB;
    }
}

/*
 * 	CheckTightBind2 - Used in XrmEnumerateDatabase above.
 */
static Bool
CheckTightBind2(
    XrmDatabase		*DB,
    XrmBindingList	 Binds,
    XrmQuarkList	 Qs,
    XrmRepresentation	*Type,
    XrmValue		*Value,
    XPointer		 Arg)
{
	/*
	 * Only looking for the real tight bindings.
	 */
	if (Qs[0] == NULLQUARK || Qs[1] == NULLQUARK
	 || Binds[0] != XrmBindTightly || Binds[1] != XrmBindTightly)
	    return False;

	return True;
}

static int
NodeCompare(WindowNode *node1, WindowNode *node2)
{
    	/* If there are no arguments, push them to the back of the list */
    	if(node1->clientArgc == 0) 
	    	return ((node2->clientArgc == 0) ? 0 : -1);

	if(node2->clientArgc == 0)
		return 1;

	return(strcmp(node1->clientArgv[0], node2->clientArgv[0]));
}
