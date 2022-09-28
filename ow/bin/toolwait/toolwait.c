#include <X11/Xlib.h>
#include <X11/X.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/time.h>
#include <signal.h>
#ifdef SYSV
#include <unistd.h>
#endif

#define TRUE 1
#define FALSE 0
#define EVMASK (SubstructureNotifyMask|ExposureMask|VisibilityChangeMask|StructureNotifyMask)

static Display * dpy;
extern int errno;

Usage(str)
char * str;
{
   fprintf(stderr, "%s\n", str);
   fprintf(stderr, "Usage: toolwait [options] command \n");
   fprintf(stderr, "options:\n");
   fprintf(stderr, "   -timeout [timeout in seconds]\n");
   fprintf(stderr, "   -diplay display-name\n");
   fprintf(stderr, "   -help (This Message)\n");
   exit(1);
}

myalarm()
{
   exit(0);
}

child_handler()
{
  int status;

  wait(&status);
  exit(status);
}

start_tool(tool)
char ** tool;
{
  pid_t pid;

  if ((pid = fork()) == 0) {
     /*  Child  */
     execvp(tool[0], tool);
     fprintf(stderr, "toolwait - Could not start %s: ", tool[0]);
     perror(NULL);
     exit(1);
  } else {
     /*  toolwait  */
     if (pid == -1) {
        fprintf(stderr, "toolwait - Could not start %s: ", tool[0]);
        perror(NULL);
        exit(1);
     }
     signal(SIGCHLD, (void(*)())child_handler);
  }   
}
 
main(argc, argv)
int argc;
char ** argv;

{
 char * display;
 int i, command, timeout, screen;
 extern int toolwait_error();
 XEvent ev;

 timeout = 15;
 display = (char *) getenv("DISPLAY");
 if ( display == NULL ) display = "None";
 command = FALSE;

 if (argc < 2) 
    Usage("Too few arguements");

 for (i=1; i<argc; i++) {
     char * s = argv[i];

     if (!strncmp("-timeout", s, strlen("-timeout")))
        if (++i < argc)
           timeout = atoi(argv[i]);
        else
           Usage("Timeout is a value in seconds");
     else if (!strncmp("-display", s, strlen("-display")))
        if (++i < argc)
           display = argv[i];
        else
           Usage("display is a valid display name");
     else if (!strncmp("-help", s, strlen("-help"))) {
        Usage("");
	}
     else if (!strncmp("--", s, strlen("--"))) {
           i++;
	   if (argv[i] != NULL)
              command = TRUE;
           break;
        }
     else {
        command = TRUE;
        break;
        }
 }

 if (!command)
    Usage("Must specify a command");

 signal(SIGALRM, (void (*)())myalarm);
 alarm(timeout);

 if((dpy = XOpenDisplay(display)) == NULL) {
   fprintf(stderr, "%s: Couldn't open display: %s\n", argv[0], display);
   fprintf(stderr, "Trying to start %s anyway.\n", argv[i]);
   start_tool(&argv[i]);
   exit(1);
 }

 XSetErrorHandler(toolwait_error);

 for (screen=0; screen < ScreenCount(dpy); screen++) {
     XSelectInput(dpy, RootWindow(dpy, screen), EVMASK);
 }

 start_tool(&argv[i]);

 while (1) {
    XNextEvent(dpy, &ev);

    if (ev.type == CreateNotify)
       if (ev.xcreatewindow.override_redirect == True) 
           exit(0);
    if (ev.type == MapNotify) 
       exit(0);
 }
}

int toolwait_error(dpy, err)
Display * dpy;
XErrorEvent * err;

{
 exit(1);
}
