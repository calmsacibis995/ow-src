#pragma ident "@(#)wsinfo.c	1.10 96/06/19 SMI"

/* Copyright */

/*-------------------------------------------------------------------------
 *  Workstation Information
 *-------------------------------------------------------------------------*/

#include <ctype.h>              /* general headers */
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include <sys/systeminfo.h>     /* sysinfo(2) */

#include <netdb.h>              /* gethostbyname() & inet_ntoa() */
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>


#include <sys/stat.h>           /* swapctl */
#include <sys/swap.h>

#include <X11/Xlib.h>           /* X11 & OLIT */
#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <X11/Shell.h>
#include <Xol/OpenLook.h>
#include <Xol/ControlAre.h>
#include <Xol/Caption.h>
#include <Xol/StaticText.h>
#include <Xol/Gauge.h>
#include <Xol/ScrollingL.h>

#include "helpfile.h"


strconst    HELP_FILE = "wsinfo.info";


/*-------------------------------------------------------------------------
 *  Workstation Information Data
 *-------------------------------------------------------------------------*/

typedef struct _WSGlobals {
    unsigned long   timeDelay;
    String          strOpenWindows;
    String          strRelease;
    String          strVersion;
    String          strUnavailable;
    String          strMegabyteFmt;
    String          strHelpFailFmt;
} WSGlobals, *WSGlobalsP;

WSGlobals   wsGlobals;

#define RESOFFSET(m)    XtOffset(WSGlobalsP, m)

static XtResource wsResources[] = {
    { "timeDelay", "TimeDelay", XtRInt, sizeof(int),
      RESOFFSET(timeDelay), XtRImmediate, "0"
    },
    { "strOpenWindows", "StrOpenWindows", XtRString, sizeof(String),
      RESOFFSET(strOpenWindows), XtRImmediate, "OpenWindows"
    },
    { "strRelease", "StrRelease", XtRString, sizeof(String),
      RESOFFSET(strRelease), XtRImmediate, "Release"
    },
    { "strVersion", "StrVersion", XtRString, sizeof(String),
      RESOFFSET(strVersion), XtRImmediate, "Version"
    },
    { "strUnavailable", "StrUnavailable", XtRString, sizeof(String),
      RESOFFSET(strUnavailable), XtRImmediate, "Unavailable"
    },
    { "strMegabyteFmt", "StrMegabyteFmt", XtRString, sizeof(String),
      RESOFFSET(strMegabyteFmt), XtRImmediate, "%ld Megabytes"
    },
    { "strHelpFailFmt", "StrHelpFailFmt", XtRString, sizeof(String),
      RESOFFSET(strHelpFailFmt), XtRImmediate, "Help Unavailable For %s"
    }
};

static XtAppContext     appContext;
static Display      *   dpy;
static int              scr;
static Widget           topLevel;

static long             systemMemory;


/*-------------------------------------------------------------------------
 *  Misc System Info Routines
 *-------------------------------------------------------------------------*/

/*
 * SysInfo - wrapper around sysinfo(2) call, guaranteed to return
 * a string even on failure.  Note the return value is a pointer
 * to static memory so subsequent calls to SysInfo() overwrite it.
 */
static char *
SysInfo(
    int         command
)
{
    static char SIbuffer[257];
    long        ret;


    ret = sysinfo(command, SIbuffer, sizeof SIbuffer);

    if ((ret == -1) || (strlen(SIbuffer) == 0)) {
        sprintf(SIbuffer, wsGlobals.strUnavailable);
    }

    return SIbuffer;
}


/*-------------------------------------------------------------------------
 *  Help File Support
 *-------------------------------------------------------------------------*/

static void
IndirectHelp(
    OlDefine     idType,
    XtPointer    id,
    Cardinal     srcX,
    Cardinal     srcY,
    OlDefine    *srcType,
    XtPointer   *src
)
{
    char        *key;
    FILE        *fp;
    char         helpErr[128];
    char        *moreHelp;
    char        *helpLine;
    char        *helpBuf = NULL;
    int          pos,
                 len;


    *srcType = OL_STRING_SOURCE;

    key = XtName(id);

    sprintf(helpErr, wsGlobals.strHelpFailFmt, key);

    fp = HelpFindFile(HELP_FILE, NULL);
    if (fp == NULL) {
        *src = XtNewString(helpErr);
        return;
    }

    if (!HelpSearchFile(fp, key, &moreHelp)) {
        fclose(fp);
        *src = XtNewString(helpErr);
        return;
    }

    for (helpLine = HelpGetText(fp), pos = 0;
         helpLine != NULL;
         helpLine = HelpGetText(fp))
    {
        len = strlen(helpLine);

        helpBuf = XtRealloc(helpBuf, pos + len + 1);
        strcpy(&helpBuf[pos], helpLine);

        pos += len;
    }

    fclose(fp);

    *src = helpBuf;
}


/*-------------------------------------------------------------------------
 *  Widget Creation Routines
 *-------------------------------------------------------------------------*/

/*
 * CreateCaptionTextCombo - creates a Caption widget with a StaticText
 * widget -- sets the string of the StaticText to the passed textValue.
 */
Widget
CreateCaptionTextCombo(
    Display *dpy,
    Widget   parent,
    char    *widgetName,
    char    *textValue
)
{
    Widget   caption,
             text;


    caption = XtVaCreateManagedWidget(
        widgetName, captionWidgetClass,
        parent,
        NULL);

    text = XtVaCreateManagedWidget(
        "text",
        staticTextWidgetClass,
        caption,
        XtNstring,  textValue,
        NULL);

    OlRegisterHelp(
        OL_WIDGET_HELP, (XtPointer)caption, NULL,
        OL_INDIRECT_SOURCE, (XtPointer)IndirectHelp);

    return text;
}


/*
 * CreateCaptionGaugeCombo - creates a Caption widget with a Gauge
 * widget -- sets XtNstring on the static-text widget to textValue.
 */
Widget
CreateCaptionGaugeCombo(
    Display *dpy,
    Widget   parent,
    char    *widgetName,
    int      sliderValue
)
{
    Widget   caption,
             controlarea,
             gauge,
             text;


    caption = XtVaCreateManagedWidget(
        widgetName, captionWidgetClass, parent, NULL);

    controlarea = XtVaCreateManagedWidget(
        "slidercontrol", controlAreaWidgetClass, caption, NULL);

    gauge = XtVaCreateManagedWidget(
        "gauge", gaugeWidgetClass,
        controlarea,
        XtNorientation, OL_HORIZONTAL,
        XtNsliderValue, sliderValue,
        NULL);

    text = XtVaCreateManagedWidget(
        "text", staticTextWidgetClass, controlarea, NULL);

    OlRegisterHelp(
        OL_WIDGET_HELP, (XtPointer)caption, NULL,
        OL_INDIRECT_SOURCE, (XtPointer)IndirectHelp);

    return gauge;
}


/*
 * CreateCaptionListCombo - creates a Caption widget with a controlarea
 * that contains a ScrollingList and a StaticText widget.
 */
Widget
CreateCaptionListCombo(
    Display *dpy,
    Widget   parent,
    char    *widgetName
)
{
    Widget   caption,
             controlarea,
             scroll,
             text;


    caption = XtVaCreateManagedWidget(
        widgetName, captionWidgetClass, parent, NULL);

    controlarea = XtVaCreateManagedWidget(
        "listcontrol", controlAreaWidgetClass, caption, NULL);

    scroll = XtVaCreateManagedWidget(
        "list", scrollingListWidgetClass, controlarea, NULL);

    text = XtVaCreateManagedWidget(
        "text", staticTextWidgetClass, controlarea, NULL);

    OlRegisterHelp(
        OL_WIDGET_HELP, (XtPointer)caption, NULL,
        OL_INDIRECT_SOURCE, (XtPointer)IndirectHelp);

    return scroll;
}


/*
 * CreateSpacerWidget - creates a Caption widget with no
 * label to act as a space separator in a control area.
 */
Widget
CreateSpacerWidget(
    Display *dpy,
    Widget   parent
)
{
    Widget  caption;


    caption = XtVaCreateManagedWidget(
        "spacer", captionWidgetClass, parent, XtNlabel, "", NULL);

    return caption;
}


/*-------------------------------------------------------------------------
 *  Workstation Name
 *-------------------------------------------------------------------------*/

static void
DoWorkstationName(
    Display *dpy,
    Widget   parent
)
{
    char    *name;


    name = SysInfo(SI_HOSTNAME);

    (void) CreateCaptionTextCombo(dpy, parent, "workstationName", name);
}


/*-------------------------------------------------------------------------
 *  Workstation Type
 *-------------------------------------------------------------------------*/

static void
DoWorkstationType(
    Display     *dpy,
    Widget       parent
)
{
    long         hostid;
    int          idType;
    char         resTypeName[64];
    char         resTypeClass[64];
    char        *rtype = NULL;
    XrmValue     value;
    XrmDatabase  db = XtDatabase(dpy);
    Bool         ok;
    char         platform[256];


    strcpy(platform, SysInfo(SI_PLATFORM));
    if (strcmp(platform, wsGlobals.strUnavailable) == 0) {

        /* SI_PLATFORM is not supported on this OS. */

        /*
         * If the host is older then an SS10/SS5 we can use its hostid
         * to get platform info from our resource file. The first two
         * digits of hostids on newer platforms are not unique per platform.
         */
         
		platform[0] = '\0'; /* Throw away strUnavailable and be ready to only
							 * use arch and mach from below if this platform
							 * look up fails.
							 */

        hostid  = atol(SysInfo(SI_HW_SERIAL));
        idType  = hostid >> 24;

        sprintf(resTypeName, "*systemType%x", idType);
        strcpy(resTypeClass, resTypeName);
        resTypeClass[1] = toupper(resTypeName[1]);

        ok = XrmGetResource(db, resTypeName, resTypeClass, &rtype, &value);

        if (ok && (strcmp(rtype, "String") == 0)) {
            strcpy(platform, (char *)(value.addr));
            strcat(platform, "; ");
        }
    } else {
        strcat(platform, "; ");
    }

    strcat(platform, SysInfo(SI_ARCHITECTURE));
    strcat(platform, "; ");
    strcat(platform, SysInfo(SI_MACHINE));

    (void) CreateCaptionTextCombo(dpy, parent, "workstationType", platform);
}


/*-------------------------------------------------------------------------
 *  Host ID
 *-------------------------------------------------------------------------*/

static void
DoHostID(
    Display *dpy,
    Widget   parent
)
{
    long     hostid;
    char     id[64];


    hostid = atol(SysInfo(SI_HW_SERIAL));

    sprintf(id, "%x",hostid);

    (void) CreateCaptionTextCombo(dpy, parent, "hostID", id);
}


/*-------------------------------------------------------------------------
 *  Internet Address
 *-------------------------------------------------------------------------*/

static char *
GetInternetAddress(
    char            *hostname
)
{
    struct hostent  *he;
    struct in_addr   in;


    he = gethostbyname(hostname);

    if (he == NULL) {
        return NULL;
    }
    
    (void) memcpy(&in, he->h_addr, he->h_length);

    return inet_ntoa(in);
}


static void
DoInternetAddress(
    Display *dpy,
    Widget   parent
)
{
    char    *ia;


    ia = GetInternetAddress(SysInfo(SI_HOSTNAME));

    if (ia == NULL) {
        ia = wsGlobals.strUnavailable;
    }
    
    (void) CreateCaptionTextCombo(dpy, parent, "internetAddress", ia);
}


/*-------------------------------------------------------------------------
 *  Network Domain
 *-------------------------------------------------------------------------*/

static void
DoNetworkDomain(
    Display *dpy,
    Widget   parent
)
{
    char    *ndom;


    ndom = SysInfo(SI_SRPC_DOMAIN);

    (void) CreateCaptionTextCombo(dpy, parent, "networkDomain", ndom);
}


/*-------------------------------------------------------------------------
 *  Physical Memory
 *-------------------------------------------------------------------------*/

static Bool
GetPhysicalMemoryInfo(
    unsigned long   *phyMem
)
{
    long             pagesize,
                     pagecount;


    pagecount = sysconf(_SC_PHYS_PAGES);
    pagesize  = sysconf(_SC_PAGESIZE);

    if ((pagecount != -1) && (pagesize != -1)) {
        /*
         * As of Solaris 2.4 up to 5GB of physical memory is supported.
         * Immediately dividing pagecount by 1024 guarantees that
         * intermediate calculations will not overflow a long.
         * However it does cause the calculation to round down by 4Mb.
         * Also on PCs, pagecount can be slightly off due to historic
         * architecture issues, so add 640K to prevent rounding errors.
		 * Bug 1210836
         */

        /* In megabytes. */

        /* Add 640K in case we are on a PC */
        if (pagesize > 0)
                pagecount += (0xA0000 / pagesize);

        /* Prevent long overflow without rounding to nearest 4Mb */
        *phyMem = ((pagecount >> 6) * (pagesize >> 4)) >> 10;

    } else {
        /*  
         * For compatibility with older libc's try the interface
         * accidentally published in the 3rd. ed. of the SVID.
         */
        extern long     sysmem(void);

        *phyMem = sysmem();

        if (*phyMem == -1) {
            return False;
        } else {
            *phyMem = *phyMem / (1024 * 1024);
        }
    }

    return True;
}


static void
DoPhysicalMemory(
    Display         *dpy,
    Widget           parent
)
{
    unsigned long    pMem;
    char             pmStr[128];

    if (GetPhysicalMemoryInfo(&pMem)) {
        sprintf(pmStr, wsGlobals.strMegabyteFmt, pMem);
    } else {
        strcpy(pmStr, wsGlobals.strUnavailable);
    }

    (void) CreateCaptionTextCombo(dpy, parent, "physicalMemory", pmStr);
}


/*-------------------------------------------------------------------------
 *  Virtual Memory
 *-------------------------------------------------------------------------*/

static Bool
GetVirtualMemoryInfo(
    unsigned long   *total,
    unsigned long   *used
)
{
    struct anoninfo  ai;
    long             pagesize;


    pagesize = sysconf(_SC_PAGESIZE);
    if (pagesize == -1) {
        return False;
    }

    if (swapctl(SC_AINFO, &ai) == -1) {
        return False;
    }
    
    *total  = ai.ani_max;
    *used   = ai.ani_resv; 

    /* Convert to megabytes */
    
    /*
     * Immediately dividing by 1024 gives us a fighting chance that
     * intermediate calculations will not overflow an unsigned long.
     *
     * Long double constants are used because there is no guarantee
     * that total or used will be even multiples of 1024.
     */

    *total  =  ((*total / 1024.0L) * pagesize) / 1024.0L;
    *used   =  ((*used  / 1024.0L) * pagesize) / 1024.0L;

    return True;
}


Widget  virtualMemoryAvailWidget = NULL;
Widget  virtualMemoryUsedWidget  = NULL;

static void
UpdateVirtualMemory(
    XtPointer           cliData,
    XtIntervalId       *id
)
{
    Boolean             goBusy = (Boolean)cliData;
    unsigned long       vMem,
                        vUsed;
static unsigned long    vMemOld,
                        vUsedOld;
    char                vmStr[128];


    if (goBusy) {
        XtVaSetValues(topLevel, XtNbusy, TRUE, NULL);
    }
    
    if (GetVirtualMemoryInfo(&vMem, &vUsed)) {
        sprintf(vmStr, wsGlobals.strMegabyteFmt, vMem);
        vUsed = (float)vUsed / (float)vMem * 100;
    } else {
        strcpy(vmStr, wsGlobals.strUnavailable);
        vUsed = 0;
    }

    if ((virtualMemoryAvailWidget != NULL) && (vMem != vMemOld)) {
        XtVaSetValues(
            virtualMemoryAvailWidget,
            XtNstring,    vmStr,
            NULL);
        vMemOld = vMem;
    }

    if ((virtualMemoryUsedWidget != NULL) && vUsed != vUsedOld) {
        XtVaSetValues(
            virtualMemoryUsedWidget,
            XtNsliderValue,   vUsed,
            NULL);
        vUsedOld = vUsed;
    }

    if (wsGlobals.timeDelay > 0) {
        (void) XtAppAddTimeOut(
            appContext, wsGlobals.timeDelay, 
            UpdateVirtualMemory, (XtPointer)FALSE);
    }

    if (goBusy) {
        XtVaSetValues(topLevel, XtNbusy, FALSE, NULL);
    }
}


static void
DoVirtualMemory(
    Display *dpy,
    Widget   parent
)
{
    virtualMemoryAvailWidget = CreateCaptionTextCombo(
        dpy, parent, "virtualMemAvail", "");

    virtualMemoryUsedWidget = CreateCaptionGaugeCombo(
        dpy, parent, "virtualMemUsed", 0);

    UpdateVirtualMemory(FALSE, NULL);
}


/*-------------------------------------------------------------------------
 *  Operation System Release
 *-------------------------------------------------------------------------*/

static void
DoOperatingSystemRelease(
    Display *dpy,
    Widget   parent
)
{
    char     os[256];


    strcpy(os, SysInfo(SI_SYSNAME));
    strcat(os, " ");
    strcat(os, wsGlobals.strRelease);
    strcat(os, " ");
    strcat(os, SysInfo(SI_RELEASE));
    strcat(os, " ");
    strcat(os, SysInfo(SI_VERSION));

    (void) CreateCaptionTextCombo(dpy, parent, "osRelease", os);
}


/*-------------------------------------------------------------------------
 *  Window System Release
 *-------------------------------------------------------------------------*/

static void
DoWindowSystemRelease(
    Display *dpy,
    Widget   parent
)
{
    char    *vendor = NULL;
    char     version[32];
    char     ws[256];
    char     major,
             minor,
             dotdot;


    if (strstr(ServerVendor(dpy), "Sun") != NULL) {
        vendor = wsGlobals.strOpenWindows;

        sprintf(version, "%d", VendorRelease(dpy));

        major   = version[0];
        minor   = version[1];
        dotdot  = version[2];
    
        if (dotdot != '0') {
            sprintf(version, "%c.%c.%c", major, minor, dotdot);
        } else {
            sprintf(version, "%c.%c",    major, minor);
        }
    } else {
        vendor = ServerVendor(dpy);

        sprintf(version, "%d", VendorRelease(dpy));
    }

    sprintf(ws, "%s %s %s", vendor, wsGlobals.strVersion, version);

    (void) CreateCaptionTextCombo(dpy, parent, "wsRelease", ws);
}


/*-------------------------------------------------------------------------
 *  Misc OLIT routines
 *-------------------------------------------------------------------------*/

/*
 * InitResources - use OpenWindows.WindowColor as *Background
 */
static void
InitResources(
    Display     *dpy
)
{
    XrmDatabase  db  = XtDatabase(dpy);
    char        *rtype;
    XrmValue     value;
    Bool         ok;

    if (db == NULL) {
        return;
    }
    
    ok = XrmGetResource(
        db,"OpenWindows.windowColor", "OpenWindows.WindowColor", &rtype,&value);

    if (ok) {
        XrmPutResource(&db, "*Background",      rtype, &value);
        XrmPutResource(&db, "*WindowColor",     rtype, &value);
        XrmPutResource(&db, "*InputFocusColor", rtype, &value);
    }
}


/*
 * Quit - quit the app when we receive a WM_PROTOCOLS
 * clientmessage of WM_DELETE_WINDOW (in OLIT-speak).
 */
static void
Quit(
    Widget               wid,
    XtPointer            clientData,
    XtPointer            callData
)
{
    OlWMProtocolVerify  *pv       = (OlWMProtocolVerify *)callData;
    Widget               topLevel = (Widget)clientData;


    if (pv->msgtype == OL_WM_DELETE_WINDOW) {
        XtUnrealizeWidget(topLevel);
        exit(0);
    }
}


/*-------------------------------------------------------------------------
 *  main
 *-------------------------------------------------------------------------*/

int
main(
    int          argc,
    char        *argv[]
)
{
    Widget       container;

    const char  *displayName = NULL;


    /* Initialize the Toolkit. */

    OlToolkitInitialize(NULL);
    OlSetDefaultTextFormat(OL_MB_STR_REP);
    XtToolkitInitialize();

    appContext = XtCreateApplicationContext();

    dpy = XtOpenDisplay(
        appContext, NULL, "wsinfo", "WSInfo",
        NULL, (Cardinal)0,
        &argc, (String *)argv);
    if (dpy == NULL) {
        displayName = getenv("DISPLAY");
        if (displayName == NULL) {
            fprintf(stderr, "The DISPLAY environment variable is not set.\n");
            fprintf(stderr, "See the X man page, X(1), for details.\n");
        } else {
            fprintf(stderr, "Your DISPLAY environment variable is set to: "
                "\"%s\".\n", displayName);
            fprintf(stderr, "I was unable to connect to this display.\n");
            fprintf(stderr, "See the X man page, X(1), for details.\n");
        }
        exit(1);
    }

    scr = DefaultScreen(dpy);

    /*
     * Initialize Resources.
     * Create top-level shell.
     * Get application resources.
     * Create a container for info items.
     * Add a Quit callback.
     */

    InitResources(dpy);

    topLevel = XtAppCreateShell(
        "wsinfo", "WSInfo", 
        applicationShellWidgetClass,
        dpy, NULL, (Cardinal)0);

    OlRegisterHelp(
        OL_WIDGET_HELP, (XtPointer)topLevel, NULL,
        OL_INDIRECT_SOURCE, (XtPointer)IndirectHelp);

    XtGetApplicationResources(
        topLevel, &wsGlobals, wsResources, XtNumber(wsResources), NULL, 0);

    container = XtVaCreateManagedWidget(
        "controls", controlAreaWidgetClass, topLevel, NULL);

    OlAddCallback(topLevel, (String)XtNwmProtocol, Quit, (XtPointer)topLevel);

    /* Create the various info items. */

    DoWorkstationName(dpy, container);
    DoWorkstationType(dpy, container);

    (void) CreateSpacerWidget(dpy, container);

    DoHostID(dpy, container);
    DoInternetAddress(dpy, container);
    DoNetworkDomain(dpy, container);

    (void) CreateSpacerWidget(dpy, container);

    DoPhysicalMemory(dpy, container);
    DoVirtualMemory(dpy, container);

    (void) CreateSpacerWidget(dpy, container);

    DoOperatingSystemRelease(dpy, container);
    DoWindowSystemRelease(dpy, container);

    /* Show top-level & start processing input events. */

    XtRealizeWidget(topLevel);
    XtAppMainLoop(appContext);

    return 0;
}
