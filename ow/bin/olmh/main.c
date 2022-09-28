#pragma ident "@(#)main.c	1.6 92/10/06"
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

#define MAIN 1			/* Makes global.h actually declare vars */
#include "xmh.h"

static XtIntervalId timerid;
static unsigned long interval;
XtAppContext    app;

/* NeedToCheckScans() gets called every five minutes, by default.
 * The frequency of its invocations is can be changed by the resource
 * Xmh*CheckFrequency, whose default is one minute.  NeedToCheckScans()
 * will be called once for every five check frequency intervals.
 */

static void NeedToCheckScans(void)
{
    int i;
    DEBUG0("[magic toc check ...")
    for (i = 0; i < numScrns; i++) {
	if (scrnList[i]->toc)
	    TocRecheckValidity(scrnList[i]->toc);
	if (scrnList[i]->msg)
	    TocRecheckValidity(MsgGetToc(scrnList[i]->msg));
    }
    DEBUG0(" done]\n")
}


/*ARGSUSED*/
static void CheckMail(XtPointer client_data, XtIntervalId *id)
{
    static int count = 0;
    register int i;
    timerid = XtAppAddTimeOut(app, interval, CheckMail, (XtPointer) NULL);
    if (app_resources.new_mail_check) {
	DEBUG0("(Checking for new mail...")
	TocCheckForNewMail();
	DEBUG0(" done)\n")
    }
    if (!subProcessRunning && (count++ % 5 == 0)) {
	NeedToCheckScans();
	if (app_resources.make_checkpoints) {
	    DEBUG0("(Checkpointing...")
	    for (i=0 ; i<numScrns ; i++)
		if (scrnList[i]->msg) 
		    MsgCheckPoint(scrnList[i]->msg);
	    DEBUG0(" done)\n")
	}
    }
}

/* Main loop. */

void
main(unsigned int argc, char **argv)
{
    InitializeWorld(argc, argv);
    if (app_resources.new_mail_check)
	TocCheckForNewMail();
    subProcessRunning = False;

    if (app_resources.check_frequency > 0) {
	interval = app_resources.check_frequency * 60000;
	timerid = XtAppAddTimeOut( app, interval, CheckMail, (XtPointer) NULL);
    }

    lastInput.win = -1;		/* nothing mapped yet */
    for (;;) {
	XEvent ev;
	XtAppNextEvent( app , &ev );
	if (ev.type == KeyPress) {
	    lastInput.win = ev.xany.window;
	    lastInput.x = ev.xkey.x_root;
	    lastInput.y = ev.xkey.y_root;
	} else if (ev.type == ButtonPress) {
	    lastInput.win = ev.xany.window;
	    lastInput.x = ev.xbutton.x_root;
	    lastInput.y = ev.xbutton.y_root;
	}
	XtDispatchEvent( &ev );
    }
}
