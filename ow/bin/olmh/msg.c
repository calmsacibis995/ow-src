#pragma ident "@(#)msg.c	1.11 93/01/15"
/*
 *      Copyright (C) 1991  Sun Microsystems, Inc
 *		 All rights reserved.
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
 *
 *		       COPYRIGHT 1987, 1989
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

/* msgs.c -- handle operations on messages. */

#ifdef NOTDEF
#include <X11/Xaw/Cardinals.h>
#endif

#include "xmh.h"
#include "tocintrnl.h"

static int SetScrn(Msg msg, Scrn scrn, Boolean force, XtCallbackList confirms, XtCallbackList cancels);

static Boolean OlmhSaveMessage(Msg msg);
OlStrRep text_format;

/*	Function Name: SetEditable
 *	Description: Sets the editable flag for this message.
 *	Arguments: msg - the message.
 *		 edit - set editable to this.
 *	Returns: none
 */

static void
SetEditable(Msg msg, Boolean edit)
{
  Arg args[1];

#ifdef NOTDEF
  if (edit)
    XtSetArg(args[0], XtNeditType, XawtextEdit);
  else
    XtSetArg(args[0], XtNeditType, XawtextRead);

  XtSetValues(msg->source, args, ONE);
#endif
}

/*	Function Name: IsEditable
 *	Description: Returns true if this is an editable message.
 *	Arguments: msg - the message to edit.
 *	Returns: TRUE if editable.
 */

static Boolean
IsEditable(Msg msg)
{
#ifdef NOTDEF
  Arg args[1];
  XawTextEditType type;

  XtSetArg(args[0], XtNeditType, &type);
  XtGetValues(msg->source, args, ONE);

  return(type == XawtextEdit);
#endif
}

/* Return the user-viewable name of the given message. */

static char *NameOfMsg(Msg msg)
{
    static char result[100];
    (void) sprintf(result, "%s:%d", msg->toc->foldername, msg->msgid);
    return result;
}


/* Update the message titlebar in the given scrn. */

static void ResetMsgLabel(Scrn scrn)
{
    Msg msg;
    char str[200];
    if (scrn) {
 	msg = scrn->msg;
	if (msg == NULL) (void) strcpy(str, app_resources.banner);
	else {
	    (void) strcpy(str, NameOfMsg(msg));
	    switch (msg->fate) {
	      case Fdelete:
		(void) strcat(str, dgettext(OlmhDomain," -> *Delete*"));
 		break;
	      case Fcopy:
	      case Fmove:
		(void) strcat(str, " -> ");
		(void) strcat(str, msg->desttoc->foldername);
		if (msg->fate == Fcopy)
		    (void) strcat(str, dgettext(OlmhDomain," (Copy)"));
		break;
	    }
	    if (msg->temporary) (void)strcat(str, 
				dgettext(OlmhDomain," [Temporary]"));
	}
	ChangeLabel((Widget) scrn->viewlabel, str);
    }
}


/* A major msg change has occured; redisplay it.  (This also should
work even if we now have a new source to display stuff from.)  This
routine arranges to hide boring headers, and also will set the text
insertion point to the proper place if this is a composition and we're
viewing it for the first time. */

static void RedisplayMsg(Scrn scrn)
{
    Msg msg;
    TextPosition startPos;
    TextLine current_line, last_line;
    TextBuffer *text_buffer;
    OlTextBufferPtr mltext_buffer;
    Arg args[2];
    int length;
    char *str;
    if (scrn) {
	msg = scrn->msg;
	if (msg) {
	    startPos = 0;
	    if (app_resources.hide_boring_headers && scrn->kind != STcomp) {
#ifdef NOTDEF
		lastPos = XawTextSourceScan(msg->source, (XawTextPosition) 0,
					    XawstAll, XawsdRight, 1, FALSE);
 		while (startPos < lastPos) {
		    nextPos = startPos;
		    length = 0;
		    while (length < 8 && nextPos < lastPos) {
			nextPos = XawTextSourceRead(msg->source, nextPos,
						    &block, 8 - length);
			(void) strncpy(str + length, block.ptr, block.length);
 			length += block.length;
		    }
		    if (length == 8) {
			if (strncmp(str, "From:", 5) == 0 ||
			    strncmp(str, "To:", 3) == 0 ||
			    strncmp(str, "Date:", 5) == 0 ||
			    strncmp(str, "Subject:", 8) == 0) break;
		    }
		    startPos = XawTextSourceScan(msg->source, startPos,
						XawstEOL, XawsdRight, 1, TRUE);
 		}
#endif
		XtVaGetValues((Widget)(scrn->viewwidget),
					XtNtextFormat , &text_format, 0);
		if(text_format == OL_SB_STR_REP){
			text_buffer = OlTextEditTextBuffer((TextEditWidget)
							scrn->viewwidget);
			last_line = LastTextBufferLine(text_buffer);
			current_line = 0;
			while (current_line < last_line) {
			    str = GetTextBufferLine(text_buffer, current_line);
			    if (strncmp(str, "From:", 5) == 0 ||
				    strncmp(str, "To:", 3) == 0 ||
				    strncmp(str, "Date:", 5) == 0 ||
				    strncmp(str, "Subject:", 8) == 0) {
				XtFree(str);
				break;
			    }
			    XtFree(str);
			    ++current_line;
			}

			if (current_line == last_line)
			    startPos = 0;
			else
			    startPos = PositionOfLine(text_buffer,current_line);
#ifdef NOTDEF
		    + LengthOfTextBufferLine(text_buffer, current_line);
#endif
		XtSetArg(args[0], XtNdisplayPosition, (XtArgVal) startPos);
		XtSetArg(args[1], XtNcursorPosition, (XtArgVal) startPos);
		XtSetValues(scrn->viewwidget, args, 2);

		OlTextEditUpdate((TextEditWidget)scrn->viewwidget, True);
#ifdef NOTDEF
		OlTextEditSetCursorPosition(scrn->viewwidget,
						startPos, startPos, startPos);
#endif

		}
		else{
			mltext_buffer = (OlTextBufferPtr)OlTextEditOlTextBuffer(					(TextEditWidget) scrn->viewwidget);
			last_line = OlLastTextBufferLine(mltext_buffer);
			current_line = 0;
			while (current_line < last_line) {
			    str = OlGetTextBufferLine(mltext_buffer,
								current_line);
			    if (strncmp(str, "From:", 5) == 0 ||
				    strncmp(str, "To:", 3) == 0 ||
				    strncmp(str, "Date:", 5) == 0 ||
				    strncmp(str, "Subject:", 8) == 0) {
				XtFree(str);
				break;
			    }
			    XtFree(str);
			    ++current_line;
			}
 
			if (current_line == last_line)
			    startPos = 0;
			else
			    startPos = OlPositionOfLine(mltext_buffer,
								current_line);
 
		XtSetArg(args[0], XtNdisplayPosition, (XtArgVal) startPos);
		XtSetArg(args[1], XtNcursorPosition, (XtArgVal) startPos);
		XtSetValues(scrn->viewwidget, args, 2);
		OlTextEditUpdate((TextEditWidget)scrn->viewwidget, True);
		}
	    }
	    else {
		/*
		 * This covers the case where hideBoringHeaders is False -
		 * we need to see the message!
		 */
		if (msg->startPos > 0) {
		    OlTextEditSetCursorPosition((TextEditWidget)
				scrn->viewwidget, msg->startPos,
				msg->startPos, msg->startPos);
		    msg->startPos = 0; /* Start in magic place only once. */
		}
		OlTextEditUpdate((TextEditWidget)scrn->viewwidget, True);
	    }
#ifdef NOTDEF
		if (startPos >= lastPos) startPos = 0;
	    }
	    XawTextSetSource(scrn->viewwidget, msg->source, startPos);
	    if (msg->startPos > 0) {
		XawTextSetInsertionPoint(scrn->viewwidget, msg->startPos);
		msg->startPos = 0; /* Start in magic place only once. */
	    }
	else {
	    XawTextSetSource(scrn->viewwidget, PNullSource,
			     (XawTextPosition)0);
#endif
 	}
    }
}


static char tempDraftFile[100] = "";

/* Temporarily move the draftfile somewhere else, so we can exec an mh
   command that affects it. */

static void TempMoveDraft(void)
{
    char *ptr;
    if (FileExists(draftFile)) {
	do {
	    ptr = MakeNewTempFileName();
	    (void) strcpy(tempDraftFile, draftFile);
	    (void) strcpy(rindex(tempDraftFile, '/'), rindex(ptr, '/'));
	} while (FileExists(tempDraftFile));
	RenameAndCheck(draftFile, tempDraftFile);
    }
}



/* Restore the draftfile from its temporary hiding place. */

static void RestoreDraft(void)
{
    if (*tempDraftFile) {
	RenameAndCheck(tempDraftFile, draftFile);
	*tempDraftFile = 0;
    }
}



/* Public routines */



char *MsgEUCFileName(Msg msg)
{
    static char result[500];
    (void) sprintf(result, "%s/.%d.euc", msg->toc->path, msg->msgid);
    return result;
}

/* Given a message, return the corresponding filename. */

char *MsgFileName(Msg msg)
{
    static char result[500];
    (void) sprintf(result, "%s/%d", msg->toc->path, msg->msgid);
    return result;
}

static Boolean
OlmhSaveMessage(Msg msg)
{
    TextEditWidget ctx = (TextEditWidget)msg->scrn[0]->viewwidget;
    char *buffer;
    int fd;

    XtVaGetValues((Widget)ctx, XtNtextFormat , &text_format, 0);
    if (!OlTextEditCopyBuffer(ctx, &buffer))
	return False;

	if(text_format == OL_SB_STR_REP){
	    if (((fd = creat(MsgFileName(msg), 0666)) == -1) ||
		(write(fd, buffer, strlen(buffer)) == -1))
		return False;
	}
	else{
	    if (((fd = creat(MsgEUCFileName(msg), 0666)) == -1) ||
		(write(fd, buffer, strlen(buffer)) == -1))
		return False;
	}

    if (close(fd) == -1)
	return False;

    XtFree(buffer);

    return True;
}


/* Save any changes to a message.  Also calls the toc routine to update the
   scanline for this msg.  Returns True if saved, false otherwise. */

MsgSaveChanges(Msg msg)
{
    int i;
    TextEditWidget ctx;
    int	 ret;
    TextBuffer *text_buffer;
    OlTextBufferPtr mltext_buffer;

	ctx = (TextEditWidget)msg->scrn[0]->viewwidget;
	XtVaGetValues((Widget)ctx, XtNtextFormat , &text_format, 0);
	if(text_format == OL_SB_STR_REP){
		text_buffer = OlTextEditTextBuffer(ctx);
	}
	else{

		mltext_buffer = (OlTextBufferPtr)OlTextEditOlTextBuffer(ctx);
	}
 
#ifdef NOTDEF
    if (msg->source) {	  /* source not used in Olmh */
	if (XawAsciiSave(msg->source)) {
#endif
	/* if (OlmhSaveMessage(msg)) { */
	if(text_format == OL_SB_STR_REP){
		ret = SaveTextBuffer(text_buffer, MsgFileName(msg));
	}
	else{
		ret = OlSaveTextBuffer(mltext_buffer, MsgEUCFileName(msg));
		if(ConvertMailText(MsgEUCFileName(msg),
					MsgFileName(msg), FALSE) == FALSE){
			;
		}
	}

	if (ret == SAVE_SUCCESS) {
	    for (i=0 ; i<msg->num_scrns ; i++)
		EnableProperButtons(msg->scrn[i]);
	    if (!msg->temporary)
		TocMsgChanged(msg->toc, msg);
	    return True;
	}
	else {
	    char str[256];
	    (void) sprintf(str, dgettext(OlmhDomain,
			"Cannot save changes to \"%s/%d\"!"),
			   msg->toc->foldername, msg->msgid);
	    PopupError(str);
	    return False;
	}
#ifdef NOTDEF
    }
    Feep();
    return False;
#endif
}


/*
 * Show the given message in the given scrn.  If a message is changed, and we
 * are removing it from any scrn, then ask for confirmation first.  If the
 * scrn was showing a temporary msg that is not being shown in any other scrn,
 * it is deleted.  If scrn is NULL, then remove the message from every scrn
 * that's showing it.
 */


/*ARGSUSED*/
static void ConfirmedNoScrn(Widget widget, XtPointer client_data, XtPointer call_data)
{
    Msg		msg = (Msg) client_data;
    register int i;

    for (i=msg->num_scrns - 1 ; i >= 0 ; i--)
	SetScrn((Msg)NULL, msg->scrn[i], TRUE, (XtCallbackList) NULL,
		(XtCallbackList) NULL);
}


static void RemoveMsgConfirmed(Scrn scrn)
{
#ifdef NOTDEF
    if (scrn->kind == STtocAndView && MsgChanged(scrn->msg)) {
	Arg	args[1];
	XtSetArg(args[0], XtNtranslations, scrn->read_translations);
	XtSetValues(scrn->viewwidget, args, (Cardinal) 1);
    }
    scrn->msg->scrn[0] = NULL;
    scrn->msg->num_scrns = 0;
    XawTextSetSource(scrn->viewwidget, PNullSource, (XawTextPosition) 0);
    XtDestroyWidget(scrn->msg->source);
    scrn->msg->source = NULL;
#endif

    /* OlTextEditClearBuffer(scrn->viewwidget); */

    scrn->msg->scrn[0] = NULL;
    scrn->msg->num_scrns = 0;

    if (scrn->msg->temporary) {
	FILE *fp;

	(void) unlink(MsgFileName(scrn->msg));
	if( (fp = myfopen(MsgEUCFileName(scrn->msg), "r")) != NULL){
		myfclose(fp);
		(void) unlink(MsgEUCFileName(scrn->msg));
	}
	TocRemoveMsg(scrn->msg->toc, scrn->msg);
	MsgFree(scrn->msg);
    }		
}


static void SetScrnNewMsg(Msg msg, Scrn scrn)
{
    Arg args[2];
    Cardinal ac;
    char	newpath[126];

    scrn->msg = msg;
    if (msg == NULL) {
	OlTextEditClearBuffer((TextEditWidget)scrn->viewwidget);
	ResetMsgLabel(scrn);
	EnableProperButtons(scrn);
	if (scrn->kind != STtocAndView)
	    StoreWindowName(scrn, progName);
    }
    else {
	msg->num_scrns++;
	msg->scrn = (Scrn *) XtRealloc((char *)msg->scrn,
				       (unsigned) sizeof(Scrn)*msg->num_scrns);
	msg->scrn[msg->num_scrns - 1] = scrn;

	/*
	 * If this isn't a compose scrn, prohibit screen update until the
	 * display position is recalculated in RedisplayMsg
	 */
	if (scrn->kind != STcomp)
	    OlTextEditUpdate((TextEditWidget)scrn->viewwidget, False);

	OlTextEditSetCursorPosition((TextEditWidget)scrn->viewwidget, 0, 0, 0);
	ac = 0;
	if(ConvertMailText(MsgFileName(msg), MsgEUCFileName(msg), TRUE)
								== FALSE){
		XtSetArg(args[ac], XtNsource, (XtArgVal) MsgFileName(msg));++ac;	}
	else{
		XtSetArg(args[ac], XtNsource,
				(XtArgVal) MsgEUCFileName(msg));      ++ac;
	}
/*
	XtSetArg(args[ac], XtNsource, (XtArgVal) MsgFileName(msg));++ac;
*/
	XtSetArg(args[ac], XtNsourceType, (XtArgVal) OL_DISK_SOURCE);   ++ac;
	XtSetValues(scrn->viewwidget, args, ac);

	ResetMsgLabel(scrn);
	RedisplayMsg(scrn);
	EnableProperButtons(scrn);
	if (scrn->kind != STtocAndView)
	    StoreWindowName(scrn, NameOfMsg(msg));
    }
}


#ifdef NOTDEF
   scrn->msg = msg;
   if (msg == NULL) {
	XawTextSetSource(scrn->viewwidget, PNullSource, (XawTextPosition) 0);
	ResetMsgLabel(scrn);
	EnableProperButtons(scrn);
	if (scrn->kind != STtocAndView)
	    StoreWindowName(scrn, progName);
    } else {
	msg->num_scrns++;
	msg->scrn = (Scrn *) XtRealloc((char *)msg->scrn,
				       (unsigned) sizeof(Scrn)*msg->num_scrns);
	msg->scrn[msg->num_scrns - 1] = scrn;
	if ((msg->source == NULL) || (msg->toc == DraftsFolder))
	    msg->source = CreateFileSource(scrn->viewwidget, MsgFileName(msg),
					   scrn->kind == STcomp);
	ResetMsgLabel(scrn);
	/* RedisplayMsg(scrn); */
	EnableProperButtons(scrn);
	if (scrn->kind != STtocAndView)
	    StoreWindowName(scrn, NameOfMsg(msg));
    }
}
#endif

typedef struct _MsgAndScrn {
    Msg		msg;
    Scrn	scrn;
} MsgAndScrnRec, *MsgAndScrn;

/*ARGSUSED*/
static void ConfirmedWithScrn(Widget widget, XtPointer client_data, XtPointer call_data)
{
    MsgAndScrn	mas = (MsgAndScrn) client_data;
    RemoveMsgConfirmed(mas->scrn);
    SetScrnNewMsg(mas->msg, mas->scrn);
    XtFree((char *) mas);
}
    
    
/* Boolean      force;		   if true, force msg set scrn */
/* XtCallbackList       confirms;	callbacks upon confirmation */
/* XtCallbackList       cancels;	 callbacks upon cancellation */

static int SetScrn(Msg msg, Scrn scrn, Boolean force, XtCallbackList confirms, XtCallbackList cancels)
{
    Widget viewwidget;
    TextBuffer *text_buffer;
    OlTextBufferPtr mltext_buffer;
    Boolean     modified;
    register int i, num_scrns;
    static XtCallbackRec yes_callbacks[] = {
	{(XtCallbackProc) NULL,	(XtPointer) NULL},
	{(XtCallbackProc) NULL,	(XtPointer) NULL},
	{(XtCallbackProc) NULL,	(XtPointer) NULL}
    };

    if (scrn == NULL) {
	if (msg == NULL || msg->num_scrns == 0) return 0;
	viewwidget = msg->scrn[0]->viewwidget;

	XtVaGetValues(viewwidget, XtNtextFormat , &text_format, 0);
	if(text_format == OL_SB_STR_REP){
		text_buffer = OlTextEditTextBuffer((TextEditWidget)viewwidget);
		modified = TextBufferModified(text_buffer);
	}
	else{
		mltext_buffer =(OlTextBufferPtr)OlTextEditOlTextBuffer(
						(TextEditWidget)viewwidget);
		modified = OlIsTextBufferModified(mltext_buffer);
	}
	if (!force && modified) {
#ifdef NOTDEF
	if (!force && XawAsciiSourceChanged(msg->source)) {
#endif
	    char str[100];
	    (void) sprintf(str,dgettext(OlmhDomain,
			   "Are you sure you want to remove changes to %s?"),
			   NameOfMsg(msg));

	    yes_callbacks[0].callback = ConfirmedNoScrn;
	    yes_callbacks[0].closure = (XtPointer) msg;
	    yes_callbacks[1].callback = confirms[0].callback;
	    yes_callbacks[1].closure = confirms[0].closure;

	    PopupConfirm((Widget) NULL, str, yes_callbacks, cancels);
	    return NEEDS_CONFIRMATION;
	}
	ConfirmedNoScrn((Widget)NULL, (XtPointer) msg, (XtPointer) NULL);
	return 0;
    }
    if (scrn->msg == msg) return 0;
    if (scrn->msg) {
	num_scrns = scrn->msg->num_scrns;
	for (i=0 ; i<num_scrns ; i++)
	    if (scrn->msg->scrn[i] == scrn) break;
	if (i >= num_scrns) Punt(dgettext(OlmhDomain,
				"Couldn't find scrn in SetScrn!"));
	if (num_scrns > 1)
	    scrn->msg->scrn[i] = scrn->msg->scrn[--(scrn->msg->num_scrns)];
	else {
	    viewwidget = scrn->viewwidget;
	    XtVaGetValues(viewwidget, XtNtextFormat , &text_format, 0);
	    if(text_format == OL_SB_STR_REP){
		text_buffer = OlTextEditTextBuffer((TextEditWidget)viewwidget);
		modified = TextBufferModified(text_buffer);
	    }
	    else{
		mltext_buffer = (OlTextBufferPtr)OlTextEditOlTextBuffer(
						(TextEditWidget)viewwidget);
		modified = OlIsTextBufferModified(mltext_buffer);
	    }
	    if (!force && modified){
#ifdef NOTDEF
	    if (!force && XawAsciiSourceChanged(scrn->msg->source)) {
#endif
		char		str[100];
		MsgAndScrn	cb_data;

		cb_data = XtNew(MsgAndScrnRec);
		cb_data->msg = msg;
		cb_data->scrn = scrn;
		(void)sprintf(str,dgettext(OlmhDomain,
			      "Are you sure you want to remove changes to %s?"),
			      NameOfMsg(scrn->msg));
		yes_callbacks[0].callback = ConfirmedWithScrn;
		yes_callbacks[0].closure = (XtPointer) cb_data;
		yes_callbacks[1].callback = confirms[0].callback;
		yes_callbacks[1].closure = confirms[0].closure;
		PopupConfirm(scrn->viewwidget, str, yes_callbacks, cancels);
		return NEEDS_CONFIRMATION;
	    }
	    RemoveMsgConfirmed(scrn);
	}
    }
    SetScrnNewMsg(msg, scrn);
    return 0;
}



/* Associate the given msg and scrn, asking for confirmation if necessary. */

int MsgSetScrn(Msg msg, Scrn scrn, XtCallbackList confirms, XtCallbackList cancels)
{
    return SetScrn(msg, scrn, FALSE, confirms, cancels);
}


/* Same as above, but with the extra information that the message is actually
   a composition.  (Nothing currently takes advantage of that extra fact.) */

void MsgSetScrnForComp(Msg msg, Scrn scrn)
{
    (void) SetScrn(msg, scrn, FALSE, (XtCallbackList) NULL, 
		   (XtCallbackList) NULL);
}


/* Associate the given msg and scrn, even if it means losing some unsaved
   changes. */

void MsgSetScrnForce(Msg msg, Scrn scrn)
{
    (void) SetScrn(msg, scrn, TRUE, (XtCallbackList) NULL,
		   (XtCallbackList) NULL);
}



/* Set the fate of the given message. */

void MsgSetFate(Msg msg, FateType fate, Toc desttoc)
{
    Toc toc = msg->toc;
    TextLocation location;
    TextBuffer *textbuffer;
    OlTextBufferPtr mltextbuffer;
#ifdef NOTDEF
    XawTextBlock block;
#endif
    int i, mark;
    msg->fate = fate;
    msg->desttoc = desttoc;
    if (fate == Fignore && msg == msg->toc->curmsg)
	mark = '+';
    else {
	switch (fate) {
	    case Fignore:	mark = ' '; break;
	    case Fcopy:		mark = 'C'; break;
	    case Fmove:		mark = '^'; break;
	    case Fdelete:	mark = 'D'; break;
	}
    }
#ifdef NOTDEF
    block.firstPos = 0;
    block.format = FMT8BIT;
    block.length = 1;
#endif
    if (toc->stopupdate)
	toc->needsrepaint = TRUE;
    if (toc->num_scrns && msg->visible && !toc->needsrepaint &&
	    mark != msg->buf[MARKPOS]) {
	    XtVaGetValues(msg->toc->scrn[0]->tocwidget, XtNtextFormat ,
							&text_format, 0);
	    if(text_format == OL_SB_STR_REP){
		textbuffer = OlTextEditTextBuffer(
				(TextEditWidget)msg->toc->scrn[0]->tocwidget);
		location = LocationOfPosition(textbuffer,
					msg->position + MARKPOS);
		ReplaceCharInTextBuffer(textbuffer, &location,
							mark, NULL, NULL);
	    }
	    else{
		char mb_mark[2] = {0,0};
		
		mltextbuffer = (OlTextBufferPtr)OlTextEditOlTextBuffer(
				(TextEditWidget)msg->toc->scrn[0]->tocwidget);
		OlLocationOfPosition(mltextbuffer,
					msg->position + MARKPOS, &location);
		mb_mark[0] = (char)mark;
		OlReplaceCharInTextBuffer(mltextbuffer, &location,
						(OlStr)mb_mark, NULL, NULL);
	    }
	msg->buf[MARKPOS] = mark;
    }
#ifdef NOTDEF
	(void)XawTextReplace(msg->toc->scrn[0]->tocwidget, /*%%%SourceReplace*/
			    msg->position + MARKPOS,
			    msg->position + MARKPOS + 1, &block);
#endif
    else
	msg->buf[MARKPOS] = mark;
    for (i=0 ; i<msg->num_scrns ; i++)
	ResetMsgLabel(msg->scrn[i]);
}



/* Get the fate of this message. */

FateType MsgGetFate(Msg msg, Toc *toc)
{
    if (toc) *toc = msg->desttoc;
    return msg->fate;
}


/* Make this a temporary message. */

void MsgSetTemporary(Msg msg)
{
    int i;
    msg->temporary = TRUE;
    for (i=0 ; i<msg->num_scrns ; i++)
	ResetMsgLabel(msg->scrn[i]);
}


/* Make this a permanent message. */

void MsgSetPermanent(Msg msg)
{
    int i;
    msg->temporary = FALSE;
    for (i=0 ; i<msg->num_scrns ; i++)
	ResetMsgLabel(msg->scrn[i]);
}



/* Return the id# of this message. */

int MsgGetId(Msg msg)
{
    return msg->msgid;
}


/* Return the scanline for this message. */

char *MsgGetScanLine(Msg msg)
{
    return msg->buf;
}



/* Return the toc this message is in. */

Toc MsgGetToc(Msg msg)
{
    return msg->toc;
}


/* Set the reapable flag for this msg. */

void MsgSetReapable(Msg msg)
{
    int i;
    msg->reapable = TRUE;
    for (i=0 ; i<msg->num_scrns ; i++)
	EnableProperButtons(msg->scrn[i]);
}



/* Clear the reapable flag for this msg. */

void MsgClearReapable(Msg msg)
{
    int i;
    msg->reapable = FALSE;
    for (i=0 ; i<msg->num_scrns ; i++)
	EnableProperButtons(msg->scrn[i]);
}


/* Get the reapable value for this msg.  Returns TRUE iff the reapable flag
   is set AND no changes have been made. */

int MsgGetReapable(Msg msg)
{
    return msg == NULL || (msg->reapable);
    /* && !MsgChanged(msg)); */		/* jsc - taken this out for now! */
#ifdef NOTDEF
			   (msg->source == NULL ||
			    !XawAsciiSourceChanged(msg->source)));
#endif
}


/* Make it possible to edit the given msg. */
void MsgSetEditable(Msg msg)
{
    int i;
    if (msg && msg->source) {
	SetEditable(msg, TRUE);
	for (i=0 ; i<msg->num_scrns ; i++)
	    EnableProperButtons(msg->scrn[i]);
    }
}



/* Turn off editing for the given msg. */

void MsgClearEditable(Msg msg)
{
    int i;
    if (msg && msg->source) {
	SetEditable(msg, FALSE);
	for (i=0 ; i<msg->num_scrns ; i++)
	    EnableProperButtons(msg->scrn[i]);
    }
}



/* Get whether the msg is editable. */

int MsgGetEditable(Msg msg)
{
    Widget ctx;
    Arg args[1];
    OlEditMode editable;

    if (!msg)
	return False;
    ctx = msg->scrn[0]->viewwidget;
    XtSetArg(args[0], XtNeditType, &editable);
    XtGetValues(ctx, args, 1);
    return (editable == OL_TEXT_EDIT);
#ifdef NOTDEF
    return msg && msg->source && IsEditable(msg);
#endif
}


/* Get whether the msg has changed since last saved. */

int MsgChanged(Msg msg)
{
    Widget ctx;
    TextBuffer *text_buffer;
    OlTextBufferPtr mltext_buffer;
    Boolean     modified;

    if (!msg)
	return False;
    ctx = msg->scrn[0]->viewwidget;
    XtVaGetValues(ctx, XtNtextFormat , &text_format, 0);
    if(text_format == OL_SB_STR_REP){
	    text_buffer = OlTextEditTextBuffer((TextEditWidget)ctx);
	    modified = TextBufferModified(text_buffer);
    }
    else{
	    mltext_buffer = (OlTextBufferPtr)OlTextEditOlTextBuffer(
							(TextEditWidget)ctx);
	    modified = OlIsTextBufferModified(mltext_buffer);
    }
    return (modified);
#ifdef NOTDEF
    return msg && msg->source && XawAsciiSourceChanged(msg->source);
#endif
}

/* Call the given function when the msg changes. */

void 
MsgSetCallOnChange(Msg msg, void (*func)(), XtPointer param)
{
  Arg args[1];
  static XtCallbackRec cb[] = { {NULL, NULL}, {NULL, NULL} };

  if (func != NULL) {
    cb[0].callback = func;
    cb[0].closure = param;
    XtSetArg(args[0], XtNpostModifyNotification, cb);
  }
  else
    XtSetArg(args[0], XtNpostModifyNotification, NULL);

  XtSetValues(msg->scrn[0]->viewwidget, args, (Cardinal) 1);
}

/* Send (i.e., mail) the given message as is.  First break it up into lines,
   and copy it to a new file in the process.  The new file is one of 10
   possible draft files; we rotate amoung the 10 so that the user can have up
   to 10 messages being sent at once.  (Using a file in /tmp is a bad idea
   because these files never actually get deleted, but renamed with some
   prefix.  Also, these should stay in an area private to the user for
   security.) */

void MsgSend(Msg msg)
{
    FILEPTR from;
    FILEPTR to;
    int     p, c, l, inheader, sendwidth, sendbreakwidth;
    char   *ptr, *ptr2, **argv, str[100];
    static sendcount = -1;
    (void) MsgSaveChanges(msg);
    from = FOpenAndCheck(MsgFileName(msg), "r");
    sendcount = (sendcount + 1) % 10;
    (void) sprintf(str, "%s%d", xmhDraftFile, sendcount);
    to = FOpenAndCheck(str, "w");
    sendwidth = app_resources.send_line_width;
    sendbreakwidth = app_resources.break_send_line_width;
    inheader = TRUE;
    while (ptr = ReadLine(from)) {
	if (inheader) {
	    if (strncmpIgnoringCase(ptr, "sendwidth:", 10) == 0) {
		if (atoi(ptr+10) > 0) sendwidth = atoi(ptr+10);
		continue;
	    }
	    if (strncmpIgnoringCase(ptr, "sendbreakwidth:", 15) == 0) {
		if (atoi(ptr+15) > 0) sendbreakwidth = atoi(ptr+15);
		continue;
	    }
	    for (l = 0, ptr2 = ptr ; *ptr2 && !l ; ptr2++)
		l = (*ptr2 != ' ' && *ptr2 != '\t' && *ptr != '-');
	    if (l) {
		(void) fprintf(to, "%s\n", ptr);
		continue;
	    }
	    inheader = FALSE;
	    if (sendbreakwidth < sendwidth) sendbreakwidth = sendwidth;
	}
	do {
	    for (p = c = l = 0, ptr2 = ptr;
		 *ptr2 && c < sendbreakwidth;
		 p++, ptr2++) {
		 if (*ptr2 == ' ' && c < sendwidth)
		     l = p;
		 if (*ptr2 == '\t') {
		     if (c < sendwidth) l = p;
		     c += 8 - (c % 8);
		 }
		 else
		 c++;
	     }
	    if (c < sendbreakwidth) {
		(void) fprintf(to, "%s\n", ptr);
		*ptr = 0;
	    }
	    else
		if (l) {
		    ptr[l] = 0;
		    (void) fprintf(to, "%s\n", ptr);
		    ptr += l + 1;
		}
		else {
		    for (c = 0; c < sendwidth; ) {
			if (*ptr == '\t') c += 8 - (c % 8);
			else c++;
			(void) fputc(*ptr++, to);
		    }
		    (void) fputc('\n', to);
		}
	} while (*ptr);
    }
    (void) myfclose(from);
    (void) myfclose(to);
    argv = MakeArgv(3);
    argv[0] = "send";
    argv[1] = "-push";
    argv[2] = str;
    DoCommand(argv, (char *) NULL, (char *) NULL);
    XtFree((char *) argv);
}


/* Make the msg into the form for a generic composition.  Set msg->startPos
   so that the text insertion point will be placed at the end of the first
   line (which is usually the "To:" field). */

void MsgLoadComposition(Msg msg)
{
    static char *blankcomp = NULL; /* Array containing comp template */
    static int compsize = 0;
#ifdef NOTDEF
    static XawTextPosition startPos;
#endif
    static TextPosition startPos;
    char *file, **argv;
    int fid;
    if (blankcomp == NULL) {
	file = MakeNewTempFileName();
	argv = MakeArgv(5);
	argv[0] = "comp";
	argv[1] = "-file";
	argv[2] = file;
	argv[3] = "-nowhatnowproc";
	argv[4] = "-nodraftfolder";
	DoCommand(argv, (char *) NULL, (char *) NULL);
	XtFree((char *) argv);
	compsize = GetFileLength(file);
	if (compsize > 0) {
	    blankcomp = XtMalloc((Cardinal) compsize);
	    fid = myopen(file, O_RDONLY, 0666);
	    if (compsize != read(fid, blankcomp, compsize))
		Punt(dgettext(OlmhDomain,
			"Error reading in MsgLoadComposition!"));
	    (void) myclose(fid);
	    DeleteFileAndCheck(file);
	} else {
 	    blankcomp = "To: \n--------\n";
 	    compsize = strlen(blankcomp);
 	}
	startPos = index(blankcomp, '\n') - blankcomp;
    }
    fid = myopen(MsgFileName(msg), O_WRONLY | O_TRUNC | O_CREAT, 0666);
    if (compsize != write(fid, blankcomp, compsize))
	Punt(dgettext(OlmhDomain,"Error writing in MsgLoadComposition!"));
    (void) myclose(fid);
    TocSetCacheValid(msg->toc);
    msg->startPos = startPos;
}



/* Load a msg with a template of a reply to frommsg.  Set msg->startPos so
   that the text insertion point will be placed at the beginning of the
   message body. */

void MsgLoadReply(Msg msg, Msg frommsg)
{
    char **argv;
    char str[100];
    TempMoveDraft();
    argv = MakeArgv(5);
    argv[0] = "repl";
    argv[1] = TocMakeFolderName(frommsg->toc);
    (void) sprintf(str, "%d", frommsg->msgid);
    argv[2] = str;
    argv[3] = "-nowhatnowproc";
    argv[4] = "-nodraftfolder";
    DoCommand(argv, (char *) NULL, (char *) NULL);
    XtFree(argv[1]);
    XtFree((char*)argv);
    RenameAndCheck(draftFile, MsgFileName(msg));
    RestoreDraft();
    TocSetCacheValid(frommsg->toc); /* If -anno is set, this keeps us from
				       rescanning folder. */
    TocSetCacheValid(msg->toc);
#ifdef NOTDEF
    I've reduced this length by 1, so as not to overrun in the
    OLIT TextEdit widget. Adding a blank line to replcomps will allow
    the user to get better positioning
    msg->startPos = GetFileLength(MsgFileName(msg));
#endif
    msg->startPos = GetFileLength(MsgFileName(msg)) - 1;
}



/* Load a msg with a template of forwarding a list of messages.  Set 
   msg->startPos so that the text insertion point will be placed at the end
   of the first line (which is usually a "To:" field). */

void MsgLoadForward(Scrn scrn, Msg msg, MsgList mlist)
{
    char  **argv, str[100], *ptr;
    FILE  *fid;
    int     i;
    TempMoveDraft();
    argv = MakeArgv(4 + mlist->nummsgs);
    argv[0] = "forw";
    argv[1] = TocMakeFolderName(mlist->msglist[0]->toc);
    for (i = 0; i < mlist->nummsgs; i++) {
	(void) sprintf(str, "%d", mlist->msglist[i]->msgid);
	argv[2 + i] = XtNewString(str);
    }
    argv[2 + i] = "-nowhatnowproc";
    argv[3 + i] = "-nodraftfolder";
    DoCommand(argv, (char *) NULL, (char *) NULL);
    for (i = 1; i < 2 + mlist->nummsgs; i++)
	XtFree((char *) argv[i]);
    XtFree((char *) argv);
    RenameAndCheck(draftFile, MsgFileName(msg));
    RestoreDraft();
    TocSetCacheValid(msg->toc);

    if (fid = myfopen(MsgFileName(msg), "r")) {
	ptr = ReadLine(fid);
	msg->startPos = strlen(ptr);
	myfclose(fid);
    }
#ifdef NOTDEF
    msg->source = CreateFileSource(scrn->viewlabel, MsgFileName(msg), True);
    msg->startPos = XawTextSourceScan(msg->source, (XawTextPosition) 0, 
				      XawstEOL, XawsdRight, 1, False);
#endif
}


/* Load msg with a copy of frommsg. */

void MsgLoadCopy(Msg msg, Msg frommsg)
{
    char str[500];
    (void)strcpy(str, MsgFileName(msg));
    CopyFileAndCheck(MsgFileName(frommsg), str);
    TocSetCacheValid(msg->toc);
}

/* Checkpoint the given message. */

void MsgCheckPoint(Msg msg)
{
#ifdef NOTDEF
    char file[BUFSIZ];

    if ( (msg == NULL) || (msg->source == NULL)) 
	return;

    sprintf(file, "%s.CKP", MsgFileName(msg));
    XawAsciiSaveAsFile(msg->source, file);
    TocSetCacheValid(msg->toc);
#endif
}

/* Free the storage being used by the given msg. */

void MsgFree(Msg msg)
{
    XtFree(msg->buf);
    XtFree((char *)msg);
}

/* Insert the associated message, if any, filtering it first */

/*ARGSUSED*/
void XmhInsert(Widget w, XEvent *event, String *params, Cardinal *num_params)
{
    Scrn scrn = ScrnFromWidget(w);
    Msg msg = scrn->msg;
    TextPosition start_pos, end_pos, cp;
    TextLocation start_loc, end_loc;
    String ptr;
    TextBuffer *text_buffer;
    OlTextBufferPtr mltext_buffer;

	XtVaGetValues(scrn->viewwidget, XtNtextFormat , &text_format, 0);
	if(text_format == OL_SB_STR_REP){
		text_buffer = OlTextEditTextBuffer((TextEditWidget)
							scrn->viewwidget);
	}
	else{
		mltext_buffer = (OlTextBufferPtr)OlTextEditOlTextBuffer((TextEditWidget)
							scrn->viewwidget);
	}

    if (msg == NULL || scrn->assocmsg == NULL) return;

    if (app_resources.insert_filter != NULL) {
	char command[1024];
	char *argv[4];
	argv[0] = "/bin/sh";
	argv[1] = "-c";
	sprintf(command, "%s %s", app_resources.insert_filter,
		MsgFileName(scrn->assocmsg));
	argv[2] = command;
	argv[3] = 0;
	ptr = DoCommandToString(argv);
    }
    else {
	/* default filter is equivalent to 'echo "<filename>"' */
	ptr = XtNewString(MsgFileName(scrn->assocmsg));
    }
    OlTextEditGetCursorPosition((TextEditWidget)scrn->viewwidget,
					&start_pos, &end_pos, &cp);
    if(text_format == OL_SB_STR_REP){
	    start_loc = LocationOfPosition(text_buffer, start_pos);
	    end_loc = LocationOfPosition(text_buffer, end_pos);
	    ReplaceBlockInTextBuffer(text_buffer, &start_loc, &end_loc,
							ptr, NULL, NULL);
    }
    else{
	    OlLocationOfPosition(mltext_buffer, start_pos, &start_loc);
	    OlLocationOfPosition(mltext_buffer, end_pos, &end_loc);
	    OlReplaceBlockInTextBuffer(mltext_buffer, &start_loc, &end_loc,
							ptr, NULL, NULL);
    }

#ifdef NOTDEF
    pos = XawTextGetInsertionPoint(scrn->viewwidget);
    if (XawTextReplace(scrn->viewwidget, pos, pos, &block) != XawEditDone)
	PopupError(dgettext(OlmhDomain,"Insertion failed!"));
    XtFree(block.ptr);
#endif
}

/*	Function Name: CreateFileSource
 *	Description: Creates an AsciiSource for a file. 
 *	Arguments: w - the widget to create the source for.
 *		 filename - the file to assign to this source.
 *		 edit - if TRUE then this disk source is editable.
 *	Returns: the source.
 */

Widget
CreateFileSource(Widget w, String filename, Boolean edit)
{
#ifdef NOTDEF
  Arg arglist[10];
  Cardinal num_args = 0;

  XtSetArg(arglist[num_args], XtNtype, XawAsciiFile);  num_args++;
  XtSetArg(arglist[num_args], XtNstring, filename);    num_args++;
  if (edit) 
      XtSetArg(arglist[num_args], XtNeditType, XawtextEdit);
  else
      XtSetArg(arglist[num_args], XtNeditType, XawtextRead);
  num_args++;

  return(XtCreateWidget("textSource", asciiSrcObjectClass, w, 
			arglist, num_args));
#endif
}

