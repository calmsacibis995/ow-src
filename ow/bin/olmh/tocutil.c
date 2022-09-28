#ident "@(#)tocutil.c	1.9 92/10/19"
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
 *			COPYRIGHT 1987, 1989
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

/* tocutil.c -- internal routines for toc stuff. */

#include <X11/Xos.h>
#include "xmh.h"
#include "toc.h"
#include "tocutil.h"
#include "tocintrnl.h"

#ifdef SYSV
#ifndef bzero
#define bzero(a,b) memset(a,0,b)
#endif
#ifndef index
#define index(a,b) strchr(a,b)
#endif
#endif

OlStrRep        text_format;

Toc TUMalloc(void)
{
    Toc toc;
    toc = XtNew(TocRec);
    bzero((char *)toc, (int) sizeof(TocRec));
    toc->msgs = (Msg *) NULL;
    toc->seqlist = (Sequence *) NULL;
    toc->validity = unknown;
    return toc;
}


/* Returns TRUE if the scan file for the given toc is out of date. */

int TUScanFileOutOfDate(Toc toc)
{
    return LastModifyDate(toc->path) > toc->lastreaddate;
}


void TUUnsetSelectedMessage(Scrn scrn)
{
    TextEditWidget ctx = (TextEditWidget) scrn->tocwidget;
    TextPosition start, end, cursor_position;

    OlTextEditGetCursorPosition(ctx, &start, &end, &cursor_position);
    OlTextEditSetCursorPosition(ctx, start, start, start);
}

void TUInvalidateToc(Toc toc, TextPosition position, int length)
{
    TextLocation end_loc;
    TextLocation start_loc;
    TextEditWidget ctx = (TextEditWidget) toc->scrn[0]->tocwidget;
    TextBuffer *textbuffer;
    OlTextBufferPtr mltextbuffer;

    XtVaGetValues((Widget)ctx, XtNtextFormat , &text_format, 0);
        if(text_format == OL_SB_STR_REP){
            textbuffer = OlTextEditTextBuffer(ctx);
            start_loc = LocationOfPosition(textbuffer, position);
            end_loc = LocationOfPosition(textbuffer, position + length);
            ReplaceBlockInTextBuffer(textbuffer, &start_loc,
                                                &end_loc, "", NULL, NULL);
        }
        else{
                mltextbuffer = (OlTextBufferPtr)OlTextEditOlTextBuffer(ctx);
                OlLocationOfPosition(mltextbuffer, position, &start_loc);
                OlLocationOfPosition(mltextbuffer, position + length, &end_loc);                OlReplaceBlockInTextBuffer(mltextbuffer, &start_loc,
                                                &end_loc, "", NULL, NULL);
        }
}


/* Make sure the sequence menu entries correspond exactly to the sequences 
 * for this toc.
 */

void TUCheckSequenceMenu(Toc toc)
{
    Scrn	scrn;
    register int i, n;
    Arg		query_args[2];
    char 	*name;
    int		j, numChildren;
    Widget	menu, item;
    Button	button;
    WidgetList	children;

    static XtCallbackRec callbacks[] = {
	{ DoSelectSequence,		(XtPointer) NULL},
	{ (XtCallbackProc) NULL,	(XtPointer) NULL},
    };
    static Arg  args[] = {
	{ XtNcallback,			(XtArgVal) callbacks},
	{ XtNleftMargin, 		(XtArgVal) 18},
    };

    for (j=0; j < toc->num_scrns; j++) {
	scrn = toc->scrn[j];

	/* Find the sequence menu and the number of entries in it. */

	name = MenuBoxButtons[XMH_SEQUENCE].button_name;
	button = BBoxFindButtonNamed(scrn->mainbuttons, name);
	menu = BBoxMenuOfButton(button);
	XtSetArg(query_args[0], XtNnumChildren, &numChildren);
	XtSetArg(query_args[1], XtNchildren, &children);
	XtGetValues(menu, query_args, (Cardinal) 2);
	n = MenuBoxButtons[XMH_SEQUENCE].num_entries;
	if (strcmp(XtName(children[0]), "menuLabel") == 0)
	    n++;

	/* Erase the current check mark. */

	for (i=(n-1); i < numChildren; i++) 
	    ToggleMenuItem(children[i], False);

	/* Delete any entries which should be deleted. */

	for (i=n; i < numChildren; i++)
	    if (! TocGetSeqNamed(toc, XtName(children[i])))
		XtDestroyWidget(children[i]);

	/* Create any entries which should be created. */
	
	callbacks[0].closure = (XtPointer) scrn;
#ifdef NOTDEF
	for (i=1; i < toc->numsequences; i++) 
	    if (! XtNameToWidget(menu, toc->seqlist[i]->name))
		XtCreateManagedWidget(toc->seqlist[i]->name, smeBSBObjectClass,
				      menu, args, XtNumber(args));
#endif

	/* Set the check mark. */

	name = toc->viewedseq->name;
	if ((item = XtNameToWidget(menu, name)) != NULL)
	    ToggleMenuItem(item, True);
    }
    TocSetSelectedSequence(toc, toc->viewedseq);
}


void TUScanFileForToc(Toc toc)
{
    Scrn scrn;
    char  **argv, str[100];
    if (toc) {
	TUGetFullFolderInfo(toc);
	if (toc->num_scrns) scrn = toc->scrn[0];
	else scrn = scrnList[0];

	(void) sprintf(str, dgettext(OlmhDomain,"Rescanning %s"),
						 toc->foldername);
	ChangeLabel(scrn->toclabel, str);

	argv = MakeArgv(4);
	argv[0] = "scan";
	argv[1] = TocMakeFolderName(toc);
	argv[2] = "-width";
	(void) sprintf(str, "%d", app_resources.toc_width);
	argv[3] = str;
	DoCommand(argv, (char *) NULL, toc->scanfile);
	XtFree(argv[1]);
	XtFree((char *) argv);

	toc->needslabelupdate = True;
	toc->validity = valid;
	toc->curmsg = NULL;	/* Get cur msg somehow! %%% */
    }
}



int TUGetMsgPosition(Toc toc, Msg msg)
{
    int msgid, h, l, m;
    char str[100];
    msgid = msg->msgid;
    l = 0;
    h = toc->nummsgs - 1;
    while (l < h - 1) {
	m = (l + h) / 2;
	if (toc->msgs[m]->msgid > msgid)
	    h = m;
	else
	    l = m;
    }
    if (toc->msgs[l] == msg) return l;
    if (toc->msgs[h] == msg) return h;
    (void) sprintf(str,dgettext(OlmhDomain,
		   "TUGetMsgPosition search failed! hi=%d, lo=%d, msgid=%d"),
		   h, l, msgid);
    Punt(str);
    return 0; /* Keep lint happy. */
}


void TUResetTocLabel(Scrn scrn)
{
    char str[500];
    Toc toc;
    if (scrn) {
	toc = scrn->toc;
	if (toc == NULL)
	    (void) strcpy(str, " ");
	else {
	    if (toc->stopupdate) {
		toc->needslabelupdate = TRUE;
		return;
	    }
	    (void) sprintf(str, "%s:%s", toc->foldername,
			   toc->viewedseq->name);
	    toc->needslabelupdate = FALSE;
	}
	ChangeLabel((Widget) scrn->toclabel, str);
    }
}


/* A major toc change has occured; redisplay it.  (This also should work even
   if we now have a new source to display stuff from.) */

void TURedisplayToc(Scrn scrn)
{
    Toc toc;
    Arg args[5];
    char        newpath[126];

/*    Widget source; */
    if (scrn != NULL && scrn->tocwidget != NULL) {
	toc = scrn->toc;
 	if (toc) {
	    if (toc->stopupdate) {
		toc->needsrepaint = TRUE;
		return;
	    }
	    OlTextEditUpdate((TextEditWidget)scrn->tocwidget, False);
/*
	    XtSetArg(args[0], XtNsource, (XtArgVal)toc->scanfile);
*/
                sprintf(newpath,"%s.euc",toc->scanfile);
                if(ConvertMailText(toc->scanfile, newpath, TRUE) == FALSE){
                        XtSetArg(args[0], XtNsource, (XtArgVal) toc->scanfile);
                }
                else{
                        XtSetArg(args[0], XtNsource, (XtArgVal) newpath);
                }
	    XtSetArg(args[1], XtNdisplayPosition, (XtArgVal)0);
	    XtSetArg(args[2], XtNcursorPosition, (XtArgVal)0);
	    XtSetArg(args[3], XtNselectStart, (XtArgVal)0);
	    XtSetArg(args[4], XtNselectEnd, (XtArgVal)0);
	    XtSetValues(scrn->tocwidget, args, XtNumber(args));
	    TocSetCurMsg(toc, TocGetCurMsg(toc));
	    OlTextEditUpdate((TextEditWidget)scrn->tocwidget, True);
	    TUCheckSequenceMenu(toc);
	    toc->needsrepaint = FALSE;
	}
    }
}

#ifdef NOTDEF
	    XawTextDisableRedisplay(scrn->tocwidget);
/* %%% dmc 8/14/89 
 *	    source = XawTextGetSource(scrn->tocwidget);
 *	    if (source != toc->source)
 */
		XawTextSetSource(scrn->tocwidget, toc->source,
				 (XawTextPosition) 0);
	    TocSetCurMsg(toc, TocGetCurMsg(toc));
	    XawTextEnableRedisplay(scrn->tocwidget);
	    TUCheckSequenceMenu(toc);
	    toc->needsrepaint = FALSE;
	} else {
	    XawTextSetSource(scrn->tocwidget, PNullSource, (XawTextPosition) 0);
	}
#endif

void TULoadSeqLists(Toc toc)
{
    Sequence seq;
    FILEPTR fid;
    char    str[500], *ptr, *ptr2, viewed[500], selected[500];
    int     i;
    if (toc->viewedseq) (void) strcpy(viewed, toc->viewedseq->name);
    else *viewed = 0;
    if (toc->selectseq) (void) strcpy(selected, toc->selectseq->name);
    else *selected = 0;
    for (i = 0; i < toc->numsequences; i++) {
	seq = toc->seqlist[i];
	XtFree((char *) seq->name);
	if (seq->mlist) FreeMsgList(seq->mlist);
	XtFree((char *)seq);
    }
    toc->numsequences = 1;
    toc->seqlist = (Sequence *) XtRealloc((char *) toc->seqlist,
					  (Cardinal) sizeof(Sequence));
    seq = toc->seqlist[0] = XtNew(SequenceRec);
    seq->name = XtNewString("all");
    seq->mlist = NULL;
    toc->viewedseq = seq;
    toc->selectseq = seq;
    (void) sprintf(str, "%s/.mh_sequences", toc->path);
    fid = myfopen(str, "r");
    if (fid) {
	while (ptr = ReadLine(fid)) {
	    ptr2 = index(ptr, ':');
	    if (ptr2) {
		*ptr2 = 0;
		if (strcmp(ptr, "all") != 0 &&
		    strcmp(ptr, "cur") != 0 &&
		    strcmp(ptr, "unseen") != 0) {
		    toc->numsequences++;
		    toc->seqlist = (Sequence *)
			XtRealloc((char *) toc->seqlist, (Cardinal)
				  toc->numsequences * sizeof(Sequence));
		    seq = toc->seqlist[toc->numsequences - 1] =
			XtNew(SequenceRec);
		    seq->name = XtNewString(ptr);
		    seq->mlist = StringToMsgList(toc, ptr2 + 1);
		    if (strcmp(seq->name, viewed) == 0) {
			toc->viewedseq = seq;
			*viewed = 0;
		    }
		    if (strcmp(seq->name, selected) == 0) {
			toc->selectseq = seq;
			*selected = 0;
		    }
		}
	    }
	}
	(void) myfclose(fid);
    }
}



/* Refigure what messages are visible. */

void TURefigureWhatsVisible(Toc toc)
{
    MsgList mlist;
    Msg msg, oldcurmsg;
    int     i, w, changed, newval, msgid;
    Sequence seq = toc->viewedseq;
    mlist = seq->mlist;
    oldcurmsg = toc->curmsg;
    TocSetCurMsg(toc, (Msg)NULL);
    w = 0;
    changed = FALSE;

    for (i = 0; i < toc->nummsgs; i++) {
	msg = toc->msgs[i];
	msgid = msg->msgid;
	while (mlist && mlist->msglist[w] && mlist->msglist[w]->msgid < msgid)
	    w++;
	newval = (!mlist
		  || (mlist->msglist[w] && mlist->msglist[w]->msgid == msgid));
	if (newval != msg->visible) {
	    changed = TRUE;
	    msg->visible = newval;
	}
    }
    if (changed) {
	TURefigureTocPositions(toc);
	if (oldcurmsg) {
	    if (!oldcurmsg->visible) {
		toc->curmsg = TocMsgAfter(toc, oldcurmsg);
		if (toc->curmsg == NULL)
		    toc->curmsg = TocMsgBefore(toc, oldcurmsg);
	    } else toc->curmsg = oldcurmsg;
	}
	for (i=0 ; i<toc->num_scrns ; i++)
	    TURedisplayToc(toc->scrn[i]);
    } else TocSetCurMsg(toc, oldcurmsg);
    for (i=0 ; i<toc->num_scrns ; i++)
	TUResetTocLabel(toc->scrn[i]);
}


/* (Re)load the toc from the scanfile.  If reloading, this makes efforts to
   keep the fates of msgs, and to keep msgs that are being edited.  Note that
   this routine must know of all places that msg ptrs are stored; it expects
   them to be kept only in tocs, in scrns, and in msg sequences. */

#define SeemsIdentical(msg1, msg2) ((msg1)->msgid == (msg2)->msgid &&	      \
				    ((msg1)->temporary || (msg2)->temporary ||\
				     strcmp((msg1)->buf, (msg2)->buf) == 0))

void TULoadTocFile(Toc toc)
{
    int maxmsgs, l, orignummsgs, i, j, origcurmsgid;
    FILEPTR fid;
    TextPosition position;
    char *ptr;
    Msg msg, curmsg;
    Msg *origmsgs;

    TocStopUpdate(toc);
    toc->lastreaddate = LastModifyDate(toc->scanfile);
    if (toc->curmsg) {
	origcurmsgid = toc->curmsg->msgid;
	TocSetCurMsg(toc, (Msg)NULL);
    } else origcurmsgid = 0;  /* The "default" current msg; 0 means none */
    fid = FOpenAndCheck(toc->scanfile, "r");
    maxmsgs = 10;
    orignummsgs = toc->nummsgs;
    toc->nummsgs = 0;
    origmsgs = toc->msgs;
    toc->msgs = (Msg *) XtMalloc((Cardinal) maxmsgs * sizeof(Msg));
    position = 0;
    i = 0;
    curmsg = NULL;
    while (ptr = ReadLineWithCR(fid)) {
	toc->msgs[toc->nummsgs++] = msg = XtNew(MsgRec);
	bzero((char *) msg, sizeof(MsgRec));
	l = strlen(ptr);
	msg->toc = toc;
	msg->position = position;
	msg->length = l;
	msg->buf = XtNewString(ptr);
	msg->msgid = atoi(ptr);
	if (msg->msgid == origcurmsgid)
	    curmsg = msg;
	msg->buf[MARKPOS] = ' ';
	position += l;
	msg->changed = FALSE;
	msg->fate = Fignore;
	msg->desttoc = NULL;
	msg->visible = TRUE;
	if (toc->nummsgs >= maxmsgs) {
	    maxmsgs += 100;
	    toc->msgs = (Msg *) XtRealloc((char *) toc->msgs,
					  (Cardinal) maxmsgs * sizeof(Msg));
	}
	while (i < orignummsgs && origmsgs[i]->msgid < msg->msgid) i++;
	if (i < orignummsgs) {
	    origmsgs[i]->buf[MARKPOS] = ' ';
	    if (SeemsIdentical(origmsgs[i], msg))
		MsgSetFate(msg, origmsgs[i]->fate, origmsgs[i]->desttoc);
	}
    }
    toc->length = toc->origlength = toc->lastPos = position;
    toc->msgs = (Msg *) XtRealloc((char *) toc->msgs,
				  (Cardinal) toc->nummsgs * sizeof(Msg));
    (void) myfclose(fid);
#ifdef NOTDEF
    if ( (toc->source == NULL) && ( toc->num_scrns > 0 ) ) {
        Arg args[1];

	XtSetArg(args[0], XtNtoc, toc);
	/* THIS WAS tocSourceWidgetClass */
	toc->source = XtCreateWidget("tocSource", textEditWidgetClass,
				     toc->scrn[0]->tocwidget,
				     args, (Cardinal) 1);
    }
#endif
    for (i=0 ; i<numScrns ; i++) {
	msg = scrnList[i]->msg;
	if (msg && msg->toc == toc) {
	    for (j=0 ; j<toc->nummsgs ; j++) {
		if (SeemsIdentical(toc->msgs[j], msg)) {
		    msg->position = toc->msgs[j]->position;
		    msg->visible = TRUE;
		    ptr = toc->msgs[j]->buf;
		    *(toc->msgs[j]) = *msg;
		    toc->msgs[j]->buf = ptr;
		    scrnList[i]->msg = toc->msgs[j];
		    break;
		}
	    }
	    if (j >= toc->nummsgs) {
		msg->temporary = FALSE;	/* Don't try to auto-delete msg. */
		MsgSetScrnForce(msg, (Scrn) NULL);
	    }
	}
    }
    for (i=0 ; i<orignummsgs ; i++)
	MsgFree(origmsgs[i]);
    XtFree((char *)origmsgs);
    TocSetCurMsg(toc, curmsg);
    TULoadSeqLists(toc);
    TocStartUpdate(toc);
}


void TUSaveTocFile(Toc toc)
{
    extern long lseek();
    Msg msg;
    int fid;
    int i;
#ifdef NOTDEF
    XawTextPosition position;
#endif
    int position;
    char c;
    if (toc->stopupdate) {
	toc->needscachesave = TRUE;
	return;
    }
    fid = -1;
    position = 0;
    for (i = 0; i < toc->nummsgs; i++) {
	msg = toc->msgs[i];
	if (fid < 0 && msg->changed) {
	    fid = myopen(toc->scanfile, O_RDWR, 0666);
	    (void) lseek(fid, (long)position, 0);
	}
	if (fid >= 0) {
	    c = msg->buf[MARKPOS];
	    msg->buf[MARKPOS] = ' ';
	    (void) write(fid, msg->buf, msg->length);
	    msg->buf[MARKPOS] = c;
	}
	position += msg->length;
    }
    if (fid < 0 && toc->length != toc->origlength)
	fid = myopen(toc->scanfile, O_RDWR, 0666);
    if (fid >= 0) {
	(void) ftruncate(fid, toc->length);
	toc->origlength = toc->length;
	(void) myclose(fid);
    }
    toc->needscachesave = FALSE;
    toc->lastreaddate = LastModifyDate(toc->scanfile);
}


void TUEnsureScanIsValidAndOpen(Toc toc)
{
    if (toc) {
	TUGetFullFolderInfo(toc);
	if (TUScanFileOutOfDate(toc)) {
	    if (toc->source) {
		XtFree((char *) toc->source);
		toc->source = NULL;
	    }
	    TUScanFileForToc(toc);
	}
	if (toc->source == NULL)
	    TULoadTocFile(toc);
	toc->validity = valid;
    }
}



/* Refigure all the positions, based on which lines are visible. */

void TURefigureTocPositions(Toc toc)
{
#ifdef NOTDEF
    int i;
    Msg msg;
    XawTextPosition position, length;
    position = length = 0;
    for (i=0; i<toc->nummsgs ; i++) {
	msg = toc->msgs[i];
	msg->position = position;
	if (msg->visible) position += msg->length;
	length += msg->length;
    }
    toc->lastPos = position;
    toc->length = length;
#endif
}



/* Make sure we've loaded ALL the folder info for this toc, including its
   path and sequence lists. */

void TUGetFullFolderInfo(Toc toc)
{
    char str[500];
    if (toc->path == NULL) {
	(void) sprintf(str, "%s/%s", app_resources.mail_path, toc->foldername);
	toc->path = XtNewString(str);
	(void) sprintf(str, "%s/.xmhcache", toc->path);
	toc->scanfile = XtNewString(str);
	toc->lastreaddate = LastModifyDate(toc->scanfile);
	if (TUScanFileOutOfDate(toc))
	    toc->validity = invalid;
	else {
	    toc->validity = valid;
	    TULoadTocFile(toc);
	}
    }
}

/* Append a message to the end of the toc.  It has the given scan line.  This
   routine will figure out the message number, and change the scan line
   accordingly. */

Msg TUAppendToc(Toc toc, char *ptr)
{
    char str[10];
    Msg msg;
    int msgid, i;

    TUGetFullFolderInfo(toc);
    if (toc->validity != valid)
	return NULL;
	    
    if (toc->nummsgs > 0)
	msgid = toc->msgs[toc->nummsgs - 1]->msgid + 1;
    else
	msgid = 1;
    (toc->nummsgs)++;
    toc->msgs = (Msg *) XtRealloc((char *) toc->msgs,
				  (Cardinal) toc->nummsgs * sizeof(Msg));
    toc->msgs[toc->nummsgs - 1] = msg = XtNew(MsgRec);
    bzero((char *) msg, (int) sizeof(MsgRec));
    msg->toc = toc;
    msg->buf = XtNewString(ptr);
    (void)sprintf(str, "%4d", msgid);
    for (i=0; i<4 ; i++) msg->buf[i] = str[i];
    msg->buf[MARKPOS] = ' ';
    msg->msgid = msgid;
    msg->position = toc->lastPos;
    msg->length = strlen(ptr);
    msg->changed = TRUE;
    msg->fate = Fignore;
    msg->desttoc = NULL;
    if (toc->viewedseq == toc->seqlist[0]) {
	msg->visible = TRUE;
	toc->lastPos += msg->length;
    }
    else
	msg->visible = FALSE;
    toc->length += msg->length;
#ifdef NOTDEF
    if (msg->visible) {
	TextEditWidget ctx = (TextEditWidget) toc->scrn[0]->tocwidget;
	TextBuffer *textbuffer = OlTextEditTextBuffer(ctx);
	TextLocation last_location = LastTextBufferLocation(textbuffer);
	String append_string = XtMalloc(msg->length + 1);
	sprintf(append_string, "\n%s", msg->buf);
	append_string[msg->length] = '\0';		/* zap the '\n' */

	ReplaceBlockInTextBuffer(textbuffer, &last_location, &last_location,
					append_string, NULL, NULL);
	XtFree(append_string);
    }


    if ( (msg->visible) && (toc->source != NULL) )
	TSourceInvalid(toc, msg->position, msg->length);
#endif
    TUSaveTocFile(toc);
    return msg;
}

