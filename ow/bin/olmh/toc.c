#pragma ident "@(#)toc.c	1.7 92/10/19"
/*
 *      Copyright (C) 1991  Sun Microsystems, Inc
 *                 All rights reserved.
 *       Notice of copyright on some portions of this source
 *       code product does not indicate publication.
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
 *
 *			  COPYRIGHT 1987
 *		   DIGITAL EQUIPMENT CORPORATION
 *		       MAYNARD, MASSACHUSETTS
 *			ALL RIGHTS RESERVED.
 *
 * THE INFORMATION IN THIS SOFTWARE IS SUBJECT TO CHANGE WITHOUT NOTICE AND
 * SHOULD NOT BE CONSTRUED AS A COMMITMENT BY DIGITAL EQUIPMENT CORPORATION.
 * DIGITAL MAKES NO REPRESENTATIONS ABOUT THE SUITABILITY OF THIS SOFTWARE FOR
 * ANY PURPOSE.  IT IS SUPPLIED "AS IS" WITHOUT EXPRESS OR IMPLIED WARRANTY.
 *
 * IF THE SOFTWARE IS MODIFIED IN A MANNER CREATING DERIVATIVE COPYRIGHT
 * RIGHTS, APPROPRIATE LEGENDS MAY BE PLACED ON THE DERIVATIVE WORK IN
 * ADDITION TO THAT SET FORTH ABOVE.
 *
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose and without fee is hereby granted, provided
 * that the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of Digital Equipment Corporation not be
 * used in advertising or publicity pertaining to distribution of the software
 * without specific, written prior permission.
 */

/* toc.c -- handle things in the toc widget. */

#include "xmh.h"
#include "tocintrnl.h"
#include "toc.h"
#include "tocutil.h"
#include <sys/stat.h>

#ifndef SVR4
#include <sys/dir.h>
#else
#include <sys/dirent.h>
#endif SVR4

OlStrRep        text_format;

#ifndef SVR4
static int IsDir(struct direct *ent)
#else /* SVR4 */
static int IsDir(struct dirent *ent)
#endif
{
    char str[500];
    struct stat buf;
#ifndef SVR4
    if (ent->d_name[0] == '.')
#else /* SVR4 */
    if (*(ent->d_name) == '.')
#endif	
	return FALSE;
    (void) sprintf(str, "%s/%s", app_resources.mail_path, ent->d_name);
    if (stat(str, &buf) /* failed */) return False;
    return (buf.st_mode & S_IFMT) == S_IFDIR;
}



#ifndef SVR4
static void MakeSureFolderExists(struct direct ***namelistptr, int *numfoldersptr, char *name)
#else /* SVR4 */
static void MakeSureFolderExists(char ***namelistptr, int *numfoldersptr, char *name)
#endif
{
    int i;
#ifndef SVR4
    extern alphasort();
#else /* SVR4 */
    extern ScanDir();
#endif    
    char str[200];
    for (i=0 ; i<*numfoldersptr ; i++)
#ifndef SVR4
	if (strcmp((*namelistptr)[i]->d_name, name) == 0) return;
#else /* SVR4 */
	if (strcmp((*namelistptr)[i], name) == 0) return;
#endif    
    (void) sprintf(str, "%s/%s", app_resources.mail_path, name);
    (void) mkdir(str, 0700);
#ifndef SVR4
    *numfoldersptr = scandir(app_resources.mail_path, namelistptr,
			     IsDir, alphasort);
#else /* SVR4 */
    *numfoldersptr = ScanDir(app_resources.mail_path, namelistptr,
			     IsDir);
#endif
    for (i=0 ; i<*numfoldersptr ; i++)
#ifndef SVR4
	if (strcmp((*namelistptr)[i]->d_name, name) == 0) return;
#else /* SVR4 */
	if (strcmp((*namelistptr)[i], name) == 0) return;
#endif    
    Punt(dgettext(OlmhDomain,"Can't create new mail folder!"));
}


#ifndef SVR4
static void MakeSureSubfolderExists(struct direct ***namelistptr, int *numfoldersptr, char *name)
#else /* SVR4 */
static void MakeSureSubfolderExists(char ***namelistptr, int *numfoldersptr, char *name)
#endif
{
    char folder[300];
    char subfolder_path[300];
    char *subfolder;
    struct stat buf;

    /* Make sure that the parent folder exists */

    subfolder = index( strcpy(folder, name), '/');
    *subfolder = '\0';
    subfolder++;
    MakeSureFolderExists(namelistptr, numfoldersptr, folder);
	
    /* The parent folder exists.  Make sure the subfolder exists. */

    (void) sprintf(subfolder_path, "%s/%s", app_resources.mail_path, name);
    if (stat(subfolder_path, &buf) /* failed */) {
	(void) mkdir(subfolder_path, 0700);
	if (stat(subfolder_path, &buf) /* failed */)
	    Punt(dgettext(OlmhDomain,"Can't create new xmh subfolder!"));
    }

    if ((buf.st_mode & S_IFMT) != S_IFDIR)
	Punt(dgettext(OlmhDomain,"Can't create new xmh subfolder!"));
}


static void LoadCheckFiles(void)
{
    FILE *fid;
    char str[1024], *ptr, *ptr2;
    int i;
    (void) sprintf(str, "%s/.xmhcheck", homeDir);
    fid = myfopen(str, "r");
    if (fid) {
	while (ptr = ReadLine(fid)) {
	    while (*ptr == ' ' || *ptr == '\t') ptr++;
	    ptr2 = ptr;
	    while (*ptr2 && *ptr2 != ' ' && *ptr2 != '\t') ptr2++;
	    if (*ptr2 == 0) continue;
	    *ptr2++ = 0;
	    while (*ptr2 == ' ' || *ptr2 == '\t') ptr2++;
	    if (*ptr2 == 0) continue;
	    for (i=0 ; i<numFolders ; i++) {
		if (strcmp(ptr, folderList[i]->foldername) == 0) {
		    folderList[i]->incfile = XtNewString(ptr2);
		    break;
		}
	    }
	}
	myfclose(fid);
    } else if (app_resources.initial_inc_file != NULL) {
        if (*app_resources.initial_inc_file != '\0')
	    InitialFolder->incfile = app_resources.initial_inc_file;
    } else {
	ptr = getenv("MAIL");
	if (ptr == NULL) ptr = getenv("mail");
	if (ptr == NULL) {
	    ptr = getenv("USER");
	    if (ptr) {
		(void) sprintf(str, "/usr/spool/mail/%s", ptr);
		ptr = str;
	    }
	}
	if (ptr)
	    InitialFolder->incfile = XtNewString(ptr);
    }
}
	    

/*	PUBLIC ROUTINES 	*/


/* Read in the list of folders. */

void TocInit(void)
{
    Toc toc;
#ifndef SVR4
    struct direct **namelist;
#else /* SVR4 */
    char **namelist;
#endif
    int i;
#ifndef SVR4
    extern alphasort();
    numFolders = scandir(app_resources.mail_path, &namelist, IsDir, alphasort);
#else /* SVR4 */
    extern ScanDir();
    numFolders = ScanDir(app_resources.mail_path, &namelist, IsDir);
#endif    
    if (numFolders < 0) {
	(void) mkdir(app_resources.mail_path, 0700);
#ifndef SVR4
	numFolders = scandir(app_resources.mail_path, &namelist, IsDir,
			     alphasort);
#else /* SVR4 */
	numFolders = ScanDir(app_resources.mail_path, &namelist, IsDir);
#endif	
	if (numFolders < 0)
	    Punt(dgettext(OlmhDomain,"Can't create or read mail directory!"));
    }
    if (IsSubfolder(app_resources.initial_folder_name))
	MakeSureSubfolderExists(&namelist, &numFolders,
				app_resources.initial_folder_name);
    else
	MakeSureFolderExists(&namelist, &numFolders,
			     app_resources.initial_folder_name);

    if (IsSubfolder(app_resources.drafts_folder_name))
	MakeSureSubfolderExists(&namelist, &numFolders,
				app_resources.drafts_folder_name);
    else
	MakeSureFolderExists(&namelist, &numFolders,
			     app_resources.drafts_folder_name);
    folderList = (Toc *) XtMalloc((Cardinal)numFolders * sizeof(Toc));
    for (i=0 ; i<numFolders ; i++) {
	toc = folderList[i] = TUMalloc();
#ifndef SVR4
	toc->foldername = XtNewString(namelist[i]->d_name);
#else /* SVR4 */
	toc->foldername = XtNewString(namelist[i]);
#endif
	free((char *)namelist[i]);
    }
    if (! (InitialFolder = TocGetNamed(app_resources.initial_folder_name)))
	InitialFolder = TocCreate(app_resources.initial_folder_name);

    if (! (DraftsFolder = TocGetNamed(app_resources.drafts_folder_name)))
	DraftsFolder = TocCreate(app_resources.drafts_folder_name);
    free((char *)namelist);
    if (app_resources.new_mail_check) LoadCheckFiles();
}



/* Create a toc and add a folder to the folderList.  */

Toc TocCreate(char *foldername)
{
    Toc		toc = TUMalloc();

    toc->foldername = XtNewString(foldername);
    folderList = (Toc *) XtRealloc((char *) folderList,
				   (unsigned) ++numFolders * sizeof(Toc));
    folderList[numFolders - 1] = toc;
    return toc;
}


/* Create a new folder with the given name. */

Toc TocCreateFolder(char *foldername)
{
    Toc toc;
    char str[500];
    if (TocGetNamed(foldername)) return NULL;
    (void) sprintf(str, "%s/%s", app_resources.mail_path, foldername);
    if (mkdir(str, 0700) < 0) return NULL;
    toc = TocCreate(foldername);
    return toc;
}



/* Check to see if what folders have new mail, and highlight their
   folderbuttons appropriately. */

void TocCheckForNewMail(void)
{
    Toc toc;
    Scrn scrn;
    int i, j, hasmail;
    static Arg arglist[] = {XtNiconPixmap, NULL};

    if (!app_resources.new_mail_check) return;

    for (i=0 ; i<numFolders ; i++) {
	toc = folderList[i];
	if (toc->incfile) {
	    hasmail =  (GetFileLength(toc->incfile) > 0);
	    if (hasmail != toc->mailpending) {

		toc->mailpending = hasmail;
		for (j=0 ; j<numScrns ; j++) {
		    scrn = scrnList[j];
		    if (scrn->kind == STtocAndView) {

			if (app_resources.mail_waiting_flag
			    && toc == InitialFolder) {
			    arglist[0].value = (XtArgVal)
				(hasmail ? NewMailPixmap : NoMailPixmap);
			    XtSetValues(scrn->parent,
					arglist, XtNumber(arglist));
			} 
			/* give visual indication of new mail waiting */
			
		    }
		}
	    }
	}
    }
}


/* Intended to support mutual exclusion on deleting folders, so that you
 * cannot have two confirm popups at the same time on the same folder.
 *
 * You can have confirm popups on different folders simultaneously.
 * However, I did not protect the user from popping up a delete confirm
 * popup on folder A, then popping up a delete confirm popup on folder
 * A/subA, then deleting A, then deleting A/subA -- which of course is 
 * already gone, and will cause xmh to Punt.
 *
 * TocClearDeletePending is a callback from the No confirmation button
 * of the confirm popup.
 */

Boolean TocTestAndSetDeletePending(Toc toc)
{
    Boolean flag;

    flag = toc->delete_pending;
    toc->delete_pending = True;
    return flag;
}

void TocClearDeletePending(Toc toc)
{
    toc->delete_pending = False;
}



/* Recursively delete an entire directory.  Nasty. */

static void NukeDirectory(char *path)
{
    char str[500];
    (void) sprintf(str, "rm -rf %s", path);
    (void) system(str);
}



/* Destroy the given folder. */

void TocDeleteFolder(Toc toc)
{
    Toc toc2;
    int i, j, w;
    if (toc == NULL) return;
    TUGetFullFolderInfo(toc);

    w = -1;
    for (i=0 ; i<numFolders ; i++) {
	toc2 = folderList[i];
	if (toc2 == toc)
	    w = i;
	else if (toc2->validity == valid)
	    for (j=0 ; j<toc2->nummsgs ; j++)
		if (toc2->msgs[j]->desttoc == toc)
		    MsgSetFate(toc2->msgs[j], Fignore, (Toc) NULL);
    }
    if (w < 0) Punt(dgettext(OlmhDomain,
			"Couldn't find it in TocDeleteFolder!"));
    NukeDirectory(toc->path);
    if (toc->validity == valid) {
	for (i=0 ; i<toc->nummsgs ; i++) {
	    MsgSetScrnForce(toc->msgs[i], (Scrn) NULL);
	    MsgFree(toc->msgs[i]);
	}
	XtFree((char *) toc->msgs);
    }
    XtFree((char *)toc);
    numFolders--;
    for (i=w ; i<numFolders ; i++) folderList[i] = folderList[i+1];
}


/*
 * Display the given toc in the given scrn.  If scrn is NULL, then remove the
 * toc from all scrns displaying it.
 */

void TocSetScrn(Toc toc, Scrn scrn)
{
    Arg args[5];
    Cardinal ac;
    int i;
    char        newpath[126];

    if (toc == NULL && scrn == NULL) return;
    if (scrn == NULL) {
	for (i=0 ; i<toc->num_scrns ; i++)
	    TocSetScrn((Toc) NULL, toc->scrn[i]);
	return;
    }
    if (scrn->toc == toc) return;
    if (scrn->toc != NULL) {
	for (i=0 ; i<scrn->toc->num_scrns ; i++)
	    if (scrn->toc->scrn[i] == scrn) break;
	if (i >= scrn->toc->num_scrns)
	    Punt(dgettext(OlmhDomain,"Couldn't find scrn in TocSetScrn!"));
	scrn->toc->scrn[i] = scrn->toc->scrn[--scrn->toc->num_scrns];
    }
    scrn->toc = toc;
    if (toc == NULL) {
	TUResetTocLabel(scrn);
	TURedisplayToc(scrn);
	StoreWindowName(scrn, progName);
    } else {
	toc->num_scrns++;
	toc->scrn = (Scrn *) XtRealloc((char *) toc->scrn,
				       (unsigned)toc->num_scrns*sizeof(Scrn));
	toc->scrn[toc->num_scrns - 1] = scrn;
	TUEnsureScanIsValidAndOpen(toc);
	TUResetTocLabel(scrn);
	if (app_resources.prefix_wm_and_icon_name) {
	    char wm_name[64];
	    int length = strlen(progName);
	    (void) strncpy(wm_name, progName, length);
	    (void) strncpy(wm_name + length , ": ", 2);
	    (void) strcpy(wm_name + length + 2, toc->foldername);
	    StoreWindowName(scrn, wm_name);
	}
	else
	    StoreWindowName(scrn, toc->foldername);
	TURedisplayToc(scrn);
	ac = 0;
        sprintf(newpath,"%s.euc",toc->scanfile);
        if(ConvertMailText(toc->scanfile, newpath, TRUE) == FALSE){
                XtSetArg(args[ac], XtNsource, (XtArgVal) toc->scanfile);++ac;
        }
        else{
                XtSetArg(args[ac], XtNsource, (XtArgVal) newpath);      ++ac;
        }
/*
	XtSetArg(args[ac], XtNsource, (XtArgVal) toc->scanfile);	++ac;
*/
	XtSetArg(args[ac], XtNsourceType, (XtArgVal) OL_DISK_SOURCE);	++ac;
	XtSetValues(scrn->tocwidget, args, ac);
	SetCurrentFolderName(scrn, toc->foldername);
    }
    EnableProperButtons(scrn);
}



/* Remove the given message from the toc.  Doesn't actually touch the file.
   Also note that it does not free the storage for the msg. */

void TocRemoveMsg(Toc toc, Msg msg)
{
    Msg newcurmsg;
    MsgList mlist;
    int i;
    if (toc->validity == unknown)
	TUGetFullFolderInfo(toc);
    if (toc->validity != valid)
	return;
    newcurmsg = TocMsgAfter(toc, msg);
    if (newcurmsg) newcurmsg->changed = TRUE;
    newcurmsg = toc->curmsg;
    if (msg == toc->curmsg) {
	newcurmsg = TocMsgAfter(toc, msg);
	if (newcurmsg == NULL) newcurmsg = TocMsgBefore(toc, msg);
	toc->curmsg = NULL;
    }
    toc->length -= msg->length;
    if (msg->visible) toc->lastPos -= msg->length;
    for(i = TUGetMsgPosition(toc, msg), toc->nummsgs--; i<toc->nummsgs ; i++) {
	toc->msgs[i] = toc->msgs[i+1];
	if (msg->visible) toc->msgs[i]->position -= msg->length;
    }
    for (i=0 ; i<toc->numsequences ; i++) {
	mlist = toc->seqlist[i]->mlist;
	if (mlist) DeleteMsgFromMsgList(mlist, msg);
    }

    if (msg->visible && toc->num_scrns > 0 && !toc->needsrepaint)
	TUInvalidateToc(toc, msg->position, msg->length);
#ifdef NOTDEF
	TSourceInvalid(toc, msg->position, -msg->length);
#endif
    TocSetCurMsg(toc, newcurmsg);
    TUSaveTocFile(toc);
}
    


void TocRecheckValidity(Toc toc)
{
    int i;
    if (toc && toc->validity == valid && TUScanFileOutOfDate(toc)) {
	if (app_resources.block_events_on_busy) ShowBusyCursor();

	TUScanFileForToc(toc);
	if (toc->source)
	    TULoadTocFile(toc);
	for (i=0 ; i<toc->num_scrns ; i++)
	    TURedisplayToc(toc->scrn[i]);

	if (app_resources.block_events_on_busy) UnshowBusyCursor();
    }
}


/* Set the current message. */

void TocSetCurMsg(Toc toc, Msg msg)
{
    Msg msg2;
    int i;
    if (toc->validity != valid) return;
    if (msg != toc->curmsg) {
	msg2 = toc->curmsg;
	toc->curmsg = msg;
	if (msg2)
	    MsgSetFate(msg2, msg2->fate, msg2->desttoc);
    }
    if (msg) {
	MsgSetFate(msg, msg->fate, msg->desttoc);
	if (toc->num_scrns) {
	    if (toc->stopupdate)
		toc->needsrepaint = TRUE;
	    else {
		for (i=0 ; i<toc->num_scrns ; i++)
		    OlTextEditSetCursorPosition((TextEditWidget)
						toc->scrn[i]->tocwidget,
						msg->position,
						msg->position,
						msg->position);
#ifdef NOTDEF
		    XawTextSetInsertionPoint(toc->scrn[i]->tocwidget,
						msg->position);
#endif
	    }
	}
    }
}

/* Return the current message. */

Msg TocGetCurMsg(Toc toc)
{
    return toc->curmsg;
}




/* Return the message after the given one.  (If none, return NULL.) */

Msg TocMsgAfter(Toc toc, Msg msg)
{
    int i;
    i = TUGetMsgPosition(toc, msg);
    do {
	i++;
	if (i >= toc->nummsgs)
	    return NULL;
    } while (!(toc->msgs[i]->visible));
    return toc->msgs[i];
}



/* Return the message before the given one.  (If none, return NULL.) */

Msg TocMsgBefore(Toc toc, Msg msg)
{
    int i;
    i = TUGetMsgPosition(toc, msg);
    do {
	i--;
	if (i < 0)
	    return NULL;
    } while (!(toc->msgs[i]->visible));
    return toc->msgs[i];
}



/* The caller KNOWS the toc's information is out of date; rescan it. */

void TocForceRescan(Toc toc)
{
    register int i;
    if (toc->num_scrns) {
	toc->viewedseq = toc->seqlist[0];
	for (i=0 ; i<toc->num_scrns ; i++)
	    TUResetTocLabel(toc->scrn[i]);
	TUScanFileForToc(toc);
	TULoadTocFile(toc);
	for (i=0 ; i<toc->num_scrns ; i++)
	    TURedisplayToc(toc->scrn[i]);
    } else {
	TUGetFullFolderInfo(toc);
	(void) unlink(toc->scanfile);
	toc->validity = invalid;
    }
}



/* The caller has just changed a sequence list.  Reread them from mh. */

void TocReloadSeqLists(Toc toc)
{
    int i;
    TocSetCacheValid(toc);
    TULoadSeqLists(toc);
    TURefigureWhatsVisible(toc);
    for (i=0 ; i<toc->num_scrns ; i++) {
	TUResetTocLabel(toc->scrn[i]);
	EnableProperButtons(toc->scrn[i]);
    }
}


/* Return TRUE if the toc has an interesting sequence. */

int TocHasSequences(Toc toc)
{
    return toc && toc->numsequences > 1;
}


/* Change which sequence is being viewed. */

void TocChangeViewedSeq(Toc toc, Sequence seq)
{
    int i;
    if (seq == NULL) seq = toc->viewedseq;
    toc->viewedseq = seq;
    TURefigureWhatsVisible(toc);

    for (i=0 ; i<toc->num_scrns ; i++) 
	TUResetTocLabel(toc->scrn[i]);
}


/* Return the sequence with the given name in the given toc. */

Sequence TocGetSeqNamed(Toc toc, char *name)
{
    register int i;
    if (name == NULL)
	return (Sequence) NULL;

    for (i=0 ; i<toc->numsequences ; i++)
	if (strcmp(toc->seqlist[i]->name, name) == 0)
	    return toc->seqlist[i];
    return (Sequence) NULL;
}


/* Return the sequence currently being viewed in the toc. */

Sequence TocViewedSequence(Toc toc)
{
    return toc->viewedseq;
}


/* Set the selected sequence in the toc */

void TocSetSelectedSequence(Toc toc, Sequence sequence)
{
    if (toc) 
	toc->selectseq = sequence;
}


/* Return the sequence currently selected */

Sequence TocSelectedSequence(Toc toc)
{
    if (toc) return (toc->selectseq);
    else return (Sequence) NULL;
}


/* Return the list of messages currently selected. */

/* #define SrcScan XawTextSourceScan */

MsgList TocCurMsgList(Toc toc)
{
    MsgList result;
    TextEditWidget ctx = (TextEditWidget) toc->scrn[0]->tocwidget;
    TextPosition start, end, cp;
    TextLocation location;
    TextBuffer *text_buffer;
    OlTextBufferPtr mltext_buffer;
    static int last_selmsg = -1;

    result = MakeNullMsgList();

    OlTextEditGetCursorPosition(ctx, &start, &end, &cp);
    if (start != end) {
	XtVaGetValues((Widget)ctx, XtNtextFormat , &text_format, 0);
        if(text_format == OL_SB_STR_REP){
                text_buffer = OlTextEditTextBuffer(ctx);
                location = LocationOfPosition(text_buffer, start);
        }
        else{
                mltext_buffer = (OlTextBufferPtr)OlTextEditOlTextBuffer(ctx);
                OlLocationOfPosition(mltext_buffer, start, &location);
        }

	AppendMsgList(result, toc->msgs[location.line]);
    }

    return result;
}


#ifdef NOTDEF
    MsgList result;
    XawTextPosition pos1, pos2;
    extern Msg MsgFromPosition();
    if (toc->num_scrns == NULL) return NULL;
    result = MakeNullMsgList();
    XawTextGetSelectionPos( toc->scrn[0]->tocwidget, &pos1, &pos2); /* %%% */
    if (pos1 < pos2) {
	pos1 = SrcScan(toc->source, pos1, XawstEOL, XawsdLeft, 1, FALSE);
	pos2 = SrcScan(toc->source, pos2, XawstPositions, XawsdLeft, 1, TRUE);
	pos2 = SrcScan(toc->source, pos2, XawstEOL, XawsdRight, 1, FALSE);
	while (pos1 < pos2) {
	    AppendMsgList(result, MsgFromPosition(toc, pos1, XawsdRight));
	    pos1 = SrcScan(toc->source, pos1, XawstEOL, XawsdRight, 1, TRUE);
	}
    }
    return result;
}
#endif



/* Unset the current selection. */

void TocUnsetSelection(Toc toc)
{
#ifdef NOTDEF
    if (toc->source)
        XawTextUnsetSelection(toc->scrn[0]->tocwidget);
#endif
}



/* Create a brand new, blank message. */

Msg TocMakeNewMsg(Toc toc)
{
    Msg msg;
    static int looping = False;
    TUEnsureScanIsValidAndOpen(toc);
    msg = TUAppendToc(toc, "####  empty\n");
    if (FileExists(MsgFileName(msg))) {
	if (looping++) Punt( dgettext(OlmhDomain,"Cannot correct scan file"));
        DEBUG1("**** FOLDER %s WAS INVALID!!!\n", toc->foldername)
	TocForceRescan(toc);
	return TocMakeNewMsg(toc); /* Try again.  Using recursion here is ugly,
				      but what the hack ... */
    }
    CopyFileAndCheck("/dev/null", MsgFileName(msg));
    looping = False;
    return msg;
}


/* Set things to not update cache or display until further notice. */

void TocStopUpdate(Toc toc)
{
#ifdef NOTDEF
    int i;
    for (i=0 ; i<toc->num_scrns ; i++)
	XawTextDisableRedisplay(toc->scrn[i]->tocwidget);
#endif
    toc->stopupdate++;
}


/* Start updating again, and do whatever updating has been queued. */

void TocStartUpdate(Toc toc)
{
    int i;
    if (toc->stopupdate && --(toc->stopupdate) == 0) {
	if (toc->needscachesave)
	    TUSaveTocFile(toc);
	for (i=0 ; i<toc->num_scrns ; i++) {
	    if (toc->needsrepaint) 
		TURedisplayToc(toc->scrn[i]);
	    if (toc->needslabelupdate)
		TUResetTocLabel(toc->scrn[i]);
	}
    }
#ifdef NOTDEF
    for (i=0 ; i<toc->num_scrns ; i++)
	XawTextEnableRedisplay(toc->scrn[i]->tocwidget);
#endif
}



/* Something has happened that could later convince us that our cache is out
   of date.  Make this not happen; our cache really *is* up-to-date. */

void TocSetCacheValid(Toc toc)
{
    TUSaveTocFile(toc);
}


/* Return the full folder pathname of the given toc, prefixed w/'+' */

char *TocMakeFolderName(Toc toc)
{
    char* name = XtMalloc((Cardinal) (strlen(toc->path) + 2) );
    (void)sprintf( name, "+%s", toc->path );
    return name;
}

char *TocName(Toc toc)
{
    return toc->foldername;
}



/* Given a foldername, return the corresponding toc. */

Toc TocGetNamed(char *name)
{
    int i;
    for (i=0; i<numFolders ; i++)
	if (strcmp(folderList[i]->foldername, name) == 0) return folderList[i];
    return NULL;
}



/* Throw out all changes to this toc, and close all views of msgs in it.
   Requires confirmation by the user. */

/*ARGSUSED*/
static void TocCataclysmOkay(Widget widget, XtPointer client_data, XtPointer call_data)
{
    Toc			toc = (Toc) client_data;
    register int	i;

    for (i=0; i < toc->nummsgs; i++)
	MsgSetFate(toc->msgs[i], Fignore, (Toc)NULL);

/* Doesn't make sense to have this MsgSetScrn for loop here. dmc. %%% */
    for (i=0; i < toc->nummsgs; i++)
	MsgSetScrn(toc->msgs[i], (Scrn) NULL, (XtCallbackList) NULL, 
		   (XtCallbackList) NULL);
}
	
int TocConfirmCataclysm(Toc toc, XtCallbackList confirms, XtCallbackList cancels)
{	
    register int	i;
    int			found = False;
    static XtCallbackRec yes_callbacks[] = {
	{TocCataclysmOkay,	(XtPointer) NULL},
	{(XtCallbackProc) NULL,	(XtPointer) NULL},
	{(XtCallbackProc) NULL,	(XtPointer) NULL}
    };

    if (toc == NULL) 
	return NULL;

    for (i=0 ; i<toc->nummsgs && !found ; i++)
	if (toc->msgs[i]->fate != Fignore) found = True;

    if (found) {
	char		str[300];
	Widget		tocwidget;
	int		i;

	(void)sprintf(str,dgettext(OlmhDomain,
		"Are you sure you want to remove all changes to %s?"),
		      toc->foldername);
	yes_callbacks[0].closure = (XtPointer) toc;
	yes_callbacks[1].callback = confirms[0].callback;
	yes_callbacks[1].closure = confirms[0].closure;

	tocwidget = NULL;
	for (i=0; i < toc->num_scrns; i++)
	    if (toc->scrn[i]->mapped) {
		tocwidget = toc->scrn[i]->tocwidget;
		break;
	    }

	PopupConfirm(tocwidget, str, yes_callbacks, cancels);
	return NEEDS_CONFIRMATION;
    }
    else {
/* Doesn't make sense to have this MsgSetFate for loop here. dmc. %%% */
	for (i=0 ; i<toc->nummsgs ; i++)
	    MsgSetFate(toc->msgs[i], Fignore, (Toc)NULL);

	for (i=0 ; i<toc->nummsgs ; i++)
	    if (MsgSetScrn(toc->msgs[i], (Scrn) NULL, confirms, cancels))
		return NEEDS_CONFIRMATION;
	return 0;
    }
}
    

/* Commit all the changes in this toc; all messages will meet their 'fate'. */

/*ARGSUSED*/
void TocCommitChanges(Widget widget, XtPointer client_data, XtPointer call_data)
{
    Toc toc = (Toc) client_data;
    Msg msg;
    int i, cur;
    char str[100], **argv;
    FateType curfate, fate; 
    Toc desttoc;
    Toc curdesttoc;
    XtCallbackRec	confirms[2];

    confirms[0].callback = TocCommitChanges;
    confirms[0].closure = (XtPointer) toc;
    confirms[1].callback = (XtCallbackProc) NULL;
    confirms[1].closure = (XtPointer) NULL;

    if (toc == NULL) return;
    for (i=0 ; i<toc->nummsgs ; i++) {
	msg = toc->msgs[i];
	fate = MsgGetFate(msg, (Toc *)NULL);
	if (fate != Fignore && fate != Fcopy)
	    if (MsgSetScrn(msg, (Scrn) NULL, confirms, (XtCallbackList) NULL)
		== NEEDS_CONFIRMATION)
	        return;
    }
    XFlush(XtDisplay(toc->scrn[0]->parent));
    for (i=0 ; i<numFolders ; i++)
	TocStopUpdate(folderList[i]);
    toc->haschanged = TRUE;
    if (app_resources.block_events_on_busy) ShowBusyCursor();

    do {
	curfate = Fignore;
	i = 0;
	while (i < toc->nummsgs) {
	    msg = toc->msgs[i];
	    fate = MsgGetFate(msg, &desttoc);
	    if (curfate == Fignore && fate != Fignore) {
		curfate = fate;
		argv = MakeArgv(2);
		switch (curfate) {
		  case Fdelete:
		    argv[0] = XtNewString("rmm");
		    argv[1] = TocMakeFolderName(toc);
		    cur = 2;
		    curdesttoc = NULL;
		    break;
		  case Fmove:
		  case Fcopy:
		    argv[0] = XtNewString("refile");
		    cur = 1;
		    curdesttoc = desttoc;
		    break;
		}
	    }
	    if (curfate != Fignore &&
		  curfate == fate && desttoc == curdesttoc) {
		argv = ResizeArgv(argv, cur + 1);
		(void) sprintf(str, "%d", MsgGetId(msg));
		argv[cur++] = XtNewString(str);
		MsgSetFate(msg, Fignore, (Toc)NULL);
		if (curdesttoc) {
		    (void) TUAppendToc(curdesttoc, MsgGetScanLine(msg));
		    curdesttoc->haschanged = TRUE;
		}
		if (curfate != Fcopy) {
		    TocRemoveMsg(toc, msg);
		    MsgFree(msg);
		    i--;
		}
		if (cur > 40)
		    break;	/* Do only 40 at a time, just to be safe. */
	    } 
	    i++;
	}
	if (curfate != Fignore) {
	    switch (curfate) {
	      case Fmove:
	      case Fcopy:
		argv = ResizeArgv(argv, cur + 4);
		argv[cur++] = XtNewString(curfate == Fmove ? "-nolink"
				       			   : "-link");
		argv[cur++] = XtNewString("-src");
		argv[cur++] = TocMakeFolderName(toc);
		argv[cur++] = TocMakeFolderName(curdesttoc);
		break;
	    }
	    if (app_resources.debug) {
		for (i = 0; i < cur; i++)
		    (void) fprintf(stderr, "%s ", argv[i]);
		(void) fprintf(stderr, "\n");
		(void) fflush(stderr);
	    }
	    DoCommand(argv, (char *) NULL, (char *) NULL);
	    for (i = 0; argv[i]; i++)
		XtFree((char *) argv[i]);
	    XtFree((char *) argv);
	}
    } while (curfate != Fignore);
    for (i=0 ; i<numFolders ; i++) {
	if (folderList[i]->haschanged) {
	    TocReloadSeqLists(folderList[i]);
	    folderList[i]->haschanged = FALSE;
	}
	TocStartUpdate(folderList[i]);
    }

    if (app_resources.block_events_on_busy) UnshowBusyCursor();
}



/* Return whether the given toc can incorporate mail. */

int TocCanIncorporate(Toc toc)
{
    return (toc && (toc == InitialFolder || toc->incfile));
}


/* Incorporate new messages into the given toc. */

void TocIncorporate(Toc toc)
{
    char **argv;
    char str[100], *file, *ptr;
    Msg msg, firstmessage;
    FILEPTR fid;

    argv = MakeArgv(toc->incfile ? 7 : 5);
    argv[0] = "inc";
    argv[1] = TocMakeFolderName(toc);
    argv[2] = "-width";
    (void) sprintf(str, "%d", app_resources.toc_width);
    argv[3] = str;
    if (toc->incfile) {
	argv[4] = "-file";
	argv[5] = toc->incfile;
	argv[6] = "-truncate";
    } else argv[4] = "-truncate";
    if (app_resources.block_events_on_busy) ShowBusyCursor();

    file = DoCommandToFile(argv);
    XtFree(argv[1]);
    XtFree((char *)argv);
    TUGetFullFolderInfo(toc);
    if (toc->validity == valid) {
	fid = FOpenAndCheck(file, "r");
	firstmessage = NULL;
	TocStopUpdate(toc);
	while (ptr = ReadLineWithCR(fid)) {
	    if (atoi(ptr) > 0) {
		msg = TUAppendToc(toc, ptr);
		if (firstmessage == NULL) firstmessage = msg;
	    }
	}
	if (firstmessage && firstmessage->visible) {
	    TocSetCurMsg(toc, firstmessage);
	}
	TocStartUpdate(toc);
	(void) myfclose(fid);
    }
    DeleteFileAndCheck(file);

    if (app_resources.block_events_on_busy) UnshowBusyCursor();
}


/* The given message has changed.  Rescan it and change the scanfile. */

void TocMsgChanged(Toc toc, Msg msg)
{
    char **argv, str[100], str2[10], *ptr;
    int length, delta, i;
    FateType fate;
    Toc desttoc;
    if (toc->validity != valid) return;
    fate = MsgGetFate(msg, &desttoc);
    MsgSetFate(msg, Fignore, (Toc) NULL);
    argv = MakeArgv(5);
    argv[0] = "scan";
    argv[1] = TocMakeFolderName(toc);
    (void) sprintf(str, "%d", msg->msgid);
    argv[2] = str;
    argv[3] = "-width";
    (void) sprintf(str2, "%d", app_resources.toc_width);
    argv[4] = str2;
    ptr = DoCommandToString(argv);
    XtFree(argv[1]);
    XtFree((char *) argv);
    if (strcmp(ptr, msg->buf) != 0) {
	length = strlen(ptr);
	delta = length - msg->length;
	XtFree(msg->buf);
	msg->buf = ptr;
	msg->length = length;
	toc->length += delta;
	if (msg->visible) {
	    if (delta != 0) {
		for (i=TUGetMsgPosition(toc, msg)+1; i<toc->nummsgs ; i++)
		    toc->msgs[i]->position += delta;
		toc->lastPos += delta;
	    }
	    for (i=0 ; i<toc->num_scrns ; i++)
		TURedisplayToc(toc->scrn[i]);
	}
	MsgSetFate(msg, fate, desttoc);
	TUSaveTocFile(toc);
    } else XtFree(ptr);
}



Msg TocMsgFromId(Toc toc, int msgid)
{
    int h, l, m;
    l = 0;
    h = toc->nummsgs - 1;
    if (h < 0) {
	if (app_resources.debug) {
	    char str[100];
	    (void)sprintf(str, dgettext(OlmhDomain,
			"Toc is empty! folder=%s\n"), toc->foldername);
	    DEBUG0( str )
	}
	return NULL;
    }
    while (l < h - 1) {
	m = (l + h) / 2;
	if (toc->msgs[m]->msgid > msgid)
	    h = m;
	else
	    l = m;
    }
    if (toc->msgs[l]->msgid == msgid) return toc->msgs[l];
    if (toc->msgs[h]->msgid == msgid) return toc->msgs[h];
    if (app_resources.debug) {
	char str[100];
	(void) sprintf(str,dgettext(OlmhDomain,
		      "TocMsgFromId search failed! hi=%d, lo=%d, msgid=%d\n"),
		      h, l, msgid);
	DEBUG0( str )
    }
    return NULL;
}

/* Sequence names are put on a stack which is specific to the folder. 
 * Sequence names are very volatile, so we make our own copies of the strings.
 */

/*ARGSUSED*/
void XmhPushSequence(Widget w, XEvent *event, String *params, Cardinal *count)
{
    Scrn	scrn = ScrnFromWidget(w);
    Toc		toc;
    int		i;

    if (! (toc = scrn->toc)) return;
    
    if (*count == 0) {
	if (toc->selectseq)
	    Push(&toc->sequence_stack, XtNewString(toc->selectseq->name));
    }
    else
	for (i=0; i < *count; i++) 
	    Push(&toc->sequence_stack, XtNewString(params[i]));
}

     /* Widget w;               any widget on the screen of interest */
/*ARGSUSED*/
void XmhPopSequence(Widget w, XEvent *event, String *params, Cardinal *count)
{
    Scrn	scrn = ScrnFromWidget(w);
    char	*seqname;
    Widget	sequenceMenu, selected, original;
    Button	button;
    Sequence	sequence;

    if ((seqname = Pop(&scrn->toc->sequence_stack)) != NULL) {

	button = BBoxFindButtonNamed(scrn->mainbuttons,
				     MenuBoxButtons[XMH_SEQUENCE].button_name);
	sequenceMenu = BBoxMenuOfButton(button);

#ifdef NOTDEF
	if (selected = XawSimpleMenuGetActiveEntry(sequenceMenu))
	    ToggleMenuItem(selected, False);
#endif

	if (original = XtNameToWidget(sequenceMenu, seqname)) {
	    ToggleMenuItem(original, True);
	    sequence = TocGetSeqNamed(scrn->toc, seqname);
	    TocSetSelectedSequence(scrn->toc, sequence);
	}
	XtFree(seqname);
    }
}
