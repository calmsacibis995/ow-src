/*****************************************************************************
*
* olitDnD.c: 
* 	Simple OLIT TextEdit program which demonstrates use of the
*	V3 Drag&Drop protocol using the DropTarget widget.  This
*	program demonstrates the D&D handshake protocol required to
*	integrate D&D with the DeskSet.
*
******************************************************************************
*
* This file is a product of Sun Microsystems, Inc. and is provided for
* unrestricted use provided that this legend is included on all tape
* media and as a part of the software program in whole or part.
* Users may copy, modify or distribute this file at will.
*
* THIS FILE IS PROVIDED AS IS WITH NO WARRANTIES OF ANY KIND INCLUDING
* THE WARRANTIES OF DESIGN, MERCHANTIBILITY AND FITNESS FOR A PARTICULAR
* PURPOSE, OR ARISING FROM A COURSE OF DEALING, USAGE OR TRADE PRACTICE.
*
* This file is provided with no support and without any obligation on the
* part of Sun Microsystems, Inc. to assist in its use, correction,
* modification or enhancement.
*
* SUN MICROSYSTEMS, INC. SHALL HAVE NO LIABILITY WITH RESPECT TO THE
* INFRINGEMENT OF COPYRIGHTS, TRADE SECRETS OR ANY PATENTS BY THIS FILE
* OR ANY PART THEREOF.
*
* In no event will Sun Microsystems, Inc. be liable for any lost revenue
* or profits or other special, indirect and consequential damages, even
* if Sun has been advised of the possibility of such damages.
*
* Sun Microsystems, Inc.
* 2550 Garcia Avenue
* Mountain View, California  94043
*
* Copyright (C) 1992,1993 by Sun Microsystems. All rights reserved.
*
****************************************************************************/

#include <stdio.h>
#include <sys/systeminfo.h>

#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <X11/Shell.h> 
#include <X11/Xatom.h>

#include <Xol/OpenLook.h> 
#include <Xol/Caption.h>
#include <Xol/ControlAre.h>
#include <Xol/DropTarget.h>
#include <Xol/RubberTile.h>
#include <Xol/ScrolledWi.h> 
#include <Xol/StaticText.h>
#include <Xol/TextEdit.h>
#include <Xol/TextField.h>

/****************************************************
 * Utility Routines & Callbacks
 ****************************************************/
static void	DisplayData(char *text);
static Boolean 	GetData(char **textPtr, int *length);

static void	TextModifiedCB(Widget w, XtPointer clientData, XtPointer callData);
static void	DropTargetCB(Widget w, XtPointer clientData, XtPointer callData);

/*************************************************
 * Stuff for handling DESTINATION side of DnD :
 **************************************************/
/*
 * Define a Selection State Mask: 
 * Mask is used to determine where we are in the DnD handshaking 
 * protocol.
 */
static unsigned long selection_mask;
 
#define SELECTION_CLEAR         0
#define SELECTION_1             (1L<<0)
#define SELECTION_2             (1L<<1)
#define SELECTION_3             (1L<<2)
#define SELECTION_4             (1L<<3)
#define SELECTION_5             (1L<<4)
#define SELECTION_6             (1L<<5)
#define SELECTION_PASS_1        (SELECTION_1|SELECTION_2|SELECTION_3|SELECTION_4|SELECTION_5)
#define SELECTION_PASS_2        (SELECTION_PASS_1|SELECTION_6)

static void 	GetSelection(Widget w, XtPointer clientData, Atom *selection,
			         Atom *type, XtPointer value, unsigned long *length,
			         int *format);
static void	GetTextSelection(Widget w, XtPointer clientData, Atom *selection,
                               	 Atom *type, XtPointer value, unsigned long *length,
                                 int *format);

/************************************************************
 * Stuff to handle SOURCE side of DnD 
 *************************************************************/
static Atom *targets;            /* array of target atoms we support */
/*
 * Define TARGETS our application supports (also supported by DeskSet)
 */
static char *target_types[] = {
        "TARGETS","FILE_NAME","STRING","LENGTH","_SUN_AVAILABLE_TYPES",
        "_SUN_LOAD","_SUN_DATA_LABEL","_SUN_DRAGDROP_DONE", "TEXT",
        "_SUN_SELECTION_END","NAME","_SUN_FILE_HOST_NAME","_SUN_ENUMERATION_COUNT"};
 
static enum { TARGETS, FILE_NAME, STRING, LENGTH, AVAILABLE_TYPES,
        LOAD, DATA_LABEL, DRAGDROP_DONE, TEXT, SELECTION_END,
        NAME, FILE_HOST_NAME, ENUMERATION_COUNT };

static Boolean  ConvertSelection(Widget w, Atom *selection, Atom *target,
				 Atom *type, XtPointer *value,
				 unsigned long *length, int *format);
static void	TransactionState(Widget w, Atom selection,
			         OlDnDTransactionState state, Time timestamp,
			         XtPointer clientData);


/************************************************************
 * Define a few Globals for simplicity
 ************************************************************/
static Widget  	filenameField, dropTarget, textEdit, statusText;

static char *fileName;		/* filename of dropped data file */
static char *hostName;		/* name of host where file was dropped from */
static char *sourceName;	/* name of app that drop originated from */
static char *pname;		/* our program name */


main(int argc, char **argv)
{
    Widget toplevel, tile, upperControls, caption, scrollWin;
    XtAppContext appContext;
    int i;

    /* Parse/Store our program name
     */
    if (pname = strrchr(argv[0],'/'))
        pname++;
    else
    	pname = argv[0];

    /* Initialize OLIT
     */
    OlToolkitInitialize((XtPointer)NULL);
    toplevel = XtAppInitialize(&appContext, "OlitDnD", (XrmOptionDescList)NULL,
                0, &argc, argv, (String *) NULL, 
                (ArgList) NULL, 0);

    tile = XtVaCreateManagedWidget("popup_tile",
		rubberTileWidgetClass,
		toplevel,
		XtNorientation,	(XtArgVal)OL_VERTICAL,
		NULL);

    /* Add Upper panel area with TextField and DropTarget
     */
    upperControls = XtVaCreateManagedWidget("upper_controls",
		rubberTileWidgetClass,
		tile,
		XtNorientation,	(XtArgVal)OL_HORIZONTAL,
		XtNweight,	(XtArgVal)0,
		NULL);

    caption = XtVaCreateManagedWidget("filename_caption",
		captionWidgetClass,
		upperControls,
		XtNweight,	(XtArgVal)1,
		XtNlabel,	(XtArgVal)"Save to File:",
		NULL);

    /* note: to simplify program, we won't really save files */
    filenameField = XtVaCreateManagedWidget("filename_field",
		textFieldWidgetClass,
		caption,
		XtNstring,	(XtArgVal)"/tmp/dummy_file",
		XtNcharsVisible,(XtArgVal)40,
		NULL);

    dropTarget = XtVaCreateManagedWidget("drop_target",
		dropTargetWidgetClass,
		upperControls,
		XtNweight,		(XtArgVal)0,
		XtNfull,		(XtArgVal)FALSE,
		XtNdndPreviewHints,(XtArgVal)OlDnDSitePreviewDefaultSite,
	    	XtNdndMoveCursor,
			  (XtArgVal)OlGetMoveDocDragCursor(toplevel),
	    	XtNdndCopyCursor,
			  (XtArgVal)OlGetDupeDocDragCursor(toplevel),
	    	XtNdndAcceptCursor,
			  (XtArgVal)OlGetDupeDocDropCursor(toplevel),
	    	XtNdndRejectCursor,
			  (XtArgVal)OlGetDupeDocNoDropCursor(toplevel),
	    	NULL);

    /* Add callbacks for both SOURCE & DESTINATION sides of DnD
     * for DropTarget
     */
    XtAddCallback(dropTarget, XtNownSelectionCallback, DropTargetCB, NULL);
    XtAddCallback(dropTarget, XtNdndTriggerCallback,   DropTargetCB, NULL);

    /* Store list of target atoms we support for handshake protocol.
     */
    targets = (Atom *)XtMalloc(XtNumber(target_types) * sizeof(Atom));

    for (i=0; i<XtNumber(target_types); i++) {
	XrmValue source, dest;
      	source.size = strlen(target_types[i])+1;
      	source.addr = target_types[i];
      	dest.size = sizeof(Atom);
      	dest.addr = (char *)&targets[i];
      	XtConvertAndStore(dropTarget,XtRString,&source,XtRAtom,&dest);
    }

    /* Add main TextEdit area
     */
    scrollWin = XtVaCreateManagedWidget("scrollwin",
		scrolledWindowWidgetClass,
		tile,
		XtNweight,	(XtArgVal)1,
		NULL);

    textEdit = XtVaCreateManagedWidget("textedit",
		textEditWidgetClass,
		scrollWin,
		NULL);

    XtAddCallback(textEdit, XtNpostModifyNotification, TextModifiedCB, NULL);

    /* Add Lower Status area
     */
    statusText = XtVaCreateManagedWidget("status_text",
		staticTextWidgetClass,
		tile,
		XtNstring,	(XtArgVal)"\nNo text loaded",
		XtNweight,	(XtArgVal)0,
		XtNgravity,	(XtArgVal)SouthWestGravity,
		NULL);

    /* Realize and enter mainloop
     */
    XtRealizeWidget(toplevel);

    XtAppMainLoop(appContext);

} /* main */

/*
 * TextModifiedCB: mark DropTarget as full if text is typed
 */
static void
TextModifiedCB(Widget w, XtPointer clientData, XtPointer callData)
{
    XtVaSetValues(dropTarget, XtNfull, (XtArgVal)TRUE, NULL);
    XtVaSetValues(statusText, XtNstring, (XtArgVal)"\nText modified", NULL);

} /* TextModifiedCB */


/*
 * DropTargetCB: Handle SOURCE & DESTINATION sides of DnD
 */
static void
DropTargetCB(Widget w, XtPointer clientData, XtPointer callData)
{
    OlDropTargetCallbackStruct *cd = (OlDropTargetCallbackStruct *)callData;

    switch (cd->reason) {
    	case OL_REASON_DND_OWNSELECTION: { /* case when we do a drag */

            /* allocate and own selection.  Register a convert proc */
	    Atom atom = OlDnDAllocTransientAtom(cd->widget);
	    XtVaSetValues(cd->widget, XtNselectionAtom, atom, NULL);

	    OlDnDOwnSelection(cd->widget,atom,cd->time,
			  ConvertSelection,
			  (XtLoseSelectionProc)NULL, 
			  (XtSelectionDoneProc)NULL,
			  TransactionState,
			  NULL);
        }
        break;
        case OL_REASON_DND_TRIGGER: { /* case when we detect a drop */
	    /* put into an array a series of questions in the form of atoms
             * that source understands.  Then ask selection to deliver the
             * questions.  We also register a function to handle the answers
             */
	    int i;
	    static Atom sun_targets[5];
	    sun_targets[0] = targets[TARGETS];
	    sun_targets[1] = targets[FILE_HOST_NAME];
	    sun_targets[2] = targets[NAME];
	    sun_targets[3] = targets[FILE_NAME];
	    sun_targets[4] = targets[DATA_LABEL];
            selection_mask = SELECTION_CLEAR;     /* Clear the selection mask */
            /* Set the target atom array as the clientData so we can tell
	     * which atom GetSelection is being called for.
             */
	    printf("DEST: Requesting Conversion of targets:\n");
	    for (i=0; i < XtNumber(sun_targets); i++)
                printf("\tTarget[%d] = %s\n",i,XGetAtomName(XtDisplay(w),sun_targets[i]));
		
	    XtGetSelectionValues(cd->widget,cd->selection,sun_targets, XtNumber(sun_targets),
			     GetSelection,(XtPointer*)sun_targets,cd->time);
        }
        break;
        default:
            break;

    } /* switch */

} /* DropTargetCB */

/*
 * GetSelection: DESTINATION: Get value of converted selections
 */
static void GetSelection(Widget w,XtPointer clientData,Atom *selection,
			 Atom *type,XtPointer value,unsigned long *length,
			 int *format)
{
    register int i;
    Boolean error = False;
    Atom target = (Atom)clientData;

    /* if we get an answer for our DRAGDONE_DONE and SELECTION_END we just
     * we end our session. We free data and call OlDnDDragNDropDone().
     */
    if (target == targets[DRAGDROP_DONE] || target == targets[SELECTION_END]) {
	printf("DEST: got %s target\n", target == targets[DRAGDROP_DONE] ?
			"_SUN_DRAGDROP_DONE" : "_SUN_SELECTION_END");

      	OlDnDDragNDropDone(w,
		         *selection, 
		         XtLastTimestampProcessed(XtDisplay(w)),
		         NULL, NULL);
      	if (value)
  	    XtFree(value);
      	return;
    }
    /* if we get answer to our request for TARGETS, just print out all the
     * targets to stdout and set the mask to indicate that we received our answer
     */ 
    if (target == targets[TARGETS]) {
      	Atom *target_atoms = (Atom*)value;
     	Boolean found = False;
	printf("DEST: Got TARGETS target:\n");

      	for (i=0; i<*length; i++)
            printf("\tTarget[%d] = %s\n",i,XGetAtomName(XtDisplay(w),target_atoms[i]));

        selection_mask |= SELECTION_1;

      	/* Check to see if we support any of the TARGETS returned */
      	for(i=0; i<*length; i++) {
            if (target_atoms[i] == targets[STRING] ||
		   target_atoms[i] == targets[TEXT]) {
                found = True;
                break;
            }
        }

        if (!found)
	    error = True;
     }
    else if (target == targets[FILE_HOST_NAME]) {
	printf("DEST: got FILE_HOST_NAME target: %s\n", *length? value : "NULL");

      	selection_mask |= SELECTION_2;
	if (hostName)
	    XtFree(hostName);
	hostName = *length? (char*)XtNewString((char*)value) : (char*)0;

    } else if (target == targets[NAME]) {
	printf("DEST: got NAME target: %s\n", *length? value : "NULL");

      	selection_mask |= SELECTION_3;
	if (sourceName)
	    XtFree(sourceName);
	sourceName = *length? (char*)XtNewString((char*)value) : (char*)0;

    } else if (target == targets[FILE_NAME]) {
	printf("DEST: got FILE_NAME target: %s\n", *length? value : "NULL");

      	if (fileName)
	    XtFree(fileName);
      	fileName = *length ? (char*)XtNewString((char*)value) : (char*)0;
      	selection_mask |= SELECTION_4;
    }
    /* 
     * we are really interested in FILE_NAME.  Since Mailtool chooses to send
     * the filename using DATA_LABEL, we have to ask for this target as well.
     * we check to see if filename was received earlier by checking selection
     * mask corresponding to FILE_NAME.  If not, set it.
     */
    else if (target == targets[DATA_LABEL]) {
	printf("DEST: got DATA_LABEL target: %s\n", *length? value : "NULL");

      	if (!fileName && selection_mask & SELECTION_4)
      	    fileName = *length ? (char*)XtNewString((char*)value) : (char*)0;
      	selection_mask |= SELECTION_5;

    } else if (target == targets[LOAD]) {
	printf("DEST: got LOAD target\n");

      	selection_mask |= SELECTION_6;
    }
    /*
     * We got through First set of questions. We need to ask for LOAD to 
     * be converted.  This will tell the owner of selection that we are ready
     * to load data
     */ 
    if (selection_mask == SELECTION_PASS_1) {
       	Time Ctime = XtLastTimestampProcessed(XtDisplay(w));
	printf("DEST: Requesting conversion of _SUN_LOAD target\n");

       	XtGetSelectionValue(w,
                           *selection,
			   targets[LOAD],
			   GetSelection,
			   (XtPointer)targets[LOAD],
			   Ctime);
    }
    /* if we get to pass through to Second pass, we will ask the owner of
     * selection to send data so that we can load it.  We ask for selection
     * STRING to be converted since we can handle it. 
     */
    else if (selection_mask == SELECTION_PASS_2) {
	printf("DEST: Requesting conversion of STRING (get the Data!)\n");

       	XtGetSelectionValue(w,
                           *selection,
			   targets[STRING],
			   GetTextSelection,
			   (XtPointer)NULL,
			   XtLastTimestampProcessed(XtDisplay(w)));
    }

    if (value)
        XtFree(value);

    /* if we encounted error, just end DnD */
    if (error) {
      	OlDnDDragNDropDone(w,
       		         *selection, 
		         XtLastTimestampProcessed(XtDisplay(w)),
		         NULL,NULL);
    
    }
} /* GetSelection */

static void
GetTextSelection(Widget w, XtPointer clientData, Atom *selection,
                 Atom *type, XtPointer value, unsigned long *length,
                 int *format)
{
    Time Ctime;
    char *fileData;
    int fileSize;
 
    /* if data we received has non zero length then store data and size in
     * our global variables.  Also set the dropsite to full so that we can
     * source drag from it.
     */  
    if (*length) {
        /* Set the new file data */
        fileData = value;
        fileSize = (unsigned int) *length;

        DisplayData(fileData);
        XtFree(value);
	XtVaSetValues(dropTarget, XtNfull, TRUE, NULL);
 
    /* received empty file, just do cleanup and deactivate dropsite */
    } else {
        if (fileName) {
            XtFree(fileName);
            fileName = (char*)0;
        }
        /* Deactivate the DropTarget for Drag's */
        XtVaSetValues(dropTarget,XtNfull,False,NULL);
        XtVaSetValues(statusText, XtNstring, "\nDropped file was empty", NULL); 
	OlTextEditClearBuffer((TextEditWidget)textEdit);
    }
    /*
     * we have received the text.  Let the source know that we are done
     * so that it can do clean up and disown its selection
     */  
    Ctime = XtLastTimestampProcessed(XtDisplay(w));
    XtGetSelectionValue(w,
                      *selection,
                      targets[DRAGDROP_DONE],
                      GetSelection,
                      (XtPointer)targets[DRAGDROP_DONE],
                      Ctime);
    XtGetSelectionValue(w,
                      *selection,
                      targets[SELECTION_END],
                      GetSelection,
                      (XtPointer)targets[SELECTION_END],
                      Ctime);
 
} /* GetTextSelection */

/*
 * DisplayData: DESTINATION: load text into TextEdit widget
 */
static void
DisplayData(char *text)
{
    char statusMsg[120];

    /* Build and Display Drop status message
     */  
    if (fileName)
        strcpy(statusMsg, "File ");
    else
	strcpy(statusMsg, "Data ");

    if (fileName && hostName) {
        strcat(statusMsg,hostName);
        strcat(statusMsg,":");
    }   
    if (fileName)
        strcat(statusMsg,fileName);

    strcat(statusMsg, "\n");

    if (sourceName) {
	strcat(statusMsg, "dropped from ");
	strcat(statusMsg, sourceName);
    }
 
    XtVaSetValues(statusText,
                XtNstring, (XtArgVal)statusMsg,
                NULL);

    XtVaSetValues(textEdit,
                XtNsourceType,     (XtArgVal)OL_STRING_SOURCE,
                XtNsource,         (XtArgVal)text,
                XtNdisplayPosition,(XtArgVal)0,
                XtNcursorPosition, (XtArgVal)0,
                XtNselectStart,    (XtArgVal)0,
                XtNselectEnd,      (XtArgVal)0,
                NULL);

} /* DisplayData */

/*
 * ConvertSelection: SOURCE: convert selections requested by destination
 */
static Boolean 
ConvertSelection(Widget w, Atom *selection, Atom *target,
		 Atom *type, XtPointer *value,
		 unsigned long *length, int *format)
{
    Display *display = XtDisplay(w);

    /* destination asked us to convert on TARGETS.  We need to provide a list
     * of targets we support by filling in passed parameters
     */
    if(*target == targets[TARGETS]) {
      	register int i;
      	Atom *target_list = (Atom*)XtMalloc(XtNumber(target_types)*sizeof(Atom));
	printf("SOURCE: Converting TARGETS target..\n");

      	/* put the list of targets we support in a array and assign to value */
      	for (i=0; i<XtNumber(target_types); i++) {
	    target_list[i] = targets[i];
            printf("\tTarget[%d] = %s\n",i,XGetAtomName(XtDisplay(w),target_list[i]));
	}

        *type = XA_ATOM;
        *value = (XtPointer)target_list;
        *length = (unsigned long)XtNumber(target_types);
        *format = 32;
        return TRUE;
    }
    /* destination asked us to convert on FILE_NAME
     */
    if (*target == targets[FILE_NAME]) {
        int size;
      	String filename = OlTextFieldGetString((TextFieldWidget)filenameField, &size);
	printf("SOURCE: Converting FILE_NAME target: %s\n", size? filename : "NULL");

        *type = XA_STRING;
        *length = (unsigned long)size;
        *value = (XtPointer)filename;
        *format = 8;
        return TRUE;
    }
    /* destination asked us to convert on STRING.  We need to provide the
     * the current data stored in textEdit.
     */
    else if (*target == targets[STRING] || *target == targets[TEXT]) {
        char *data;
        int size;
	printf("SOURCE: Converting STRING target\n");
        if (GetData(&data, &size) && (size > 0)) {
	    *type = XA_STRING;
	    *length = (unsigned long)size;
	    *value = (XtPointer)data; 
	    *format = 8;
	    return TRUE;
	} else
	    return FALSE;
    }
    /* destination asked us to convert on FILE_HOST_NAME.  We need to
     * provide the hostname where the file resides to destination.  
     * note: to simpify this program, we arn't doing any file-storing of
     * the data, so this isn't really valid information
     */
    else if (*target == targets[FILE_HOST_NAME]) {
      	char hostname[256];

      	if (sysinfo(SI_HOSTNAME,hostname,256) > 0) {
	    printf("SOURCE: Converting _SUN_FILE_HOST_NAME target: %s\n", hostname);

            *type = XA_STRING;
            *length = (unsigned long)strlen(hostname);
            *value = (XtPointer)XtNewString(hostname);
            *format = 8;
            return TRUE;
        } else
            return FALSE;
    }
    /* destination asked us to convert on AVAILABLE_TYPES.  We need to provide
     * the various file types we support to destination.  This lets the
     * destination ask us for file type that it feels comfortable handling.
     * However, we only support STRING.
     */
    else if (*target == targets[AVAILABLE_TYPES]) {
        Atom *atoms;
	printf("SOURCE: Converting _SUN_AVAILABLE_TYPES target: STRING\n");

        atoms = (Atom*)XtMalloc(sizeof(Atom));
        atoms[0] = targets[STRING];
        *type = XA_ATOM;
        *length = (unsigned long)1;
        *value = (XtPointer)atoms;
        *format = 32;
        return TRUE;
    }
    /* destination asked us to convert on NAME.  We need to provide
     * our application name to destination.  
     */
    else if (*target == targets[NAME]) {
	printf("SOURCE: Converting NAME target: %s\n", pname);

        *type = XA_STRING;
        *length = (unsigned long)strlen(pname);
        *value = (XtPointer)XtNewString(pname);
        *format = 8;
        return TRUE;
    }
    /* destination asked us to convert on ENUMERATION_COUNT.  We need to provide 
     * number of selections we are sending to destination. 
     */
    else if (*target == targets[ENUMERATION_COUNT]) {
        int *buffer;
	printf("SOURCE: Converting _SUN_ENUMERATION_COUNT target: 1\n");
      
        buffer = (int*) XtMalloc(sizeof(int));
        *buffer = 1;
        *type = XA_INTEGER;
        *length = (unsigned long)1;
        *value = (XtPointer)buffer;
        *format = 32;
        return TRUE;
    }
    /* destination asked us to convert on LENGTH.  We need to provide
     * size of data we have dropped on destination.  
     */
    else if (*target == targets[LENGTH]) {
      	int *buffer = (int*)XtMalloc(sizeof(int));
	char *data;
	if (!GetData(&data, buffer))
	    return FALSE;
	printf("SOURCE: Converting LENGTH target: %d\n", *buffer);

	XtFree(data); /* don't need the data here */
      
      	*type = XA_INTEGER;
      	*length = (unsigned long)1;
      	*value = (XtPointer)buffer;
     	*format = 32;
      	return TRUE;
    }
    /* destination asked us to convert on DRAGDONE_DONE and SELECTION_END.  
     * This is tell us that destination has received information correctly. 
     * We could do cleanup here.
     */
    else if(*target == targets[DRAGDROP_DONE] ||
          *target == targets[SELECTION_END]) {
	printf("SOURCE: Converting %s\n", *target == targets[DRAGDROP_DONE]?
			"_SUN_DRAGDROP_DONE" : "_SUN_SELECTION_END");

        *type = OlInternAtom(display,"NULL");
        *length = (unsigned long)0;
        *value = (XtPointer)0;
        *format = 32;
        return TRUE;
    }

    /* we return FALSE if we do not support request atom since we can not do
     * any converts on it.
     */
    return FALSE;

} /* ConvertSelection */ 

/*
 * GetData: SOURCE: Get the text & length from the TextEdit widget
 */
Boolean
GetData(char **textPtr, int *length)
{
   if (!OlTextEditCopyBuffer((TextEditWidget)textEdit, textPtr)) {
        OlWarning("getData: error trying to copy textedit buffer\n");
        return(FALSE);
   }
 
   *length = strlen(*textPtr);
   return(TRUE);
 
} /* GetData */

/*
 * TransactionState: SOURCE: called to indicate stages of DnD
 */
static void 
TransactionState(Widget w,Atom selection,
			     OlDnDTransactionState state,Time timestamp,
			     XtPointer clientData)
{
    switch (state) {
    	case OlDnDTransactionDone:
    	case OlDnDTransactionRequestorError:
    	case OlDnDTransactionRequestorWindowDeath:
    	/* some sort of failure occured or we are done, give up selection we own.
     	 * Note: we could have done the disowning of selection when we got
     	 * SELECTION_END, but we chose to do it here...
     	 */
      	    OlDnDDisownSelection(w,
			   selection, 
                           XtLastTimestampProcessed(XtDisplay(w)));
            OlDnDFreeTransientAtom(w,selection);
      	    break;
    	case OlDnDTransactionBegins:
    	case OlDnDTransactionEnds:
      	    break;
    }
} /* TransactionState */
