#pragma ident "@(#)externs.h	1.5 92/10/06"
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
 *
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose and without fee is hereby granted, provided
 * that the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of Digital Equipment Corporation not be
 * used in advertising or publicity pertaining to distribution of the software
 * without specific, written prior permission.
 */

	/* Action routines are declared in actions.h
	 * Functions which begin with `Do' are the corresponding callbacks.
         */

extern int errno;

	/* from command.c */

extern char *   DoCommandToFile         ( char **argv );
extern char *   DoCommandToString       ( char **argv );

        /* from compfuncs. */

extern void     DoResetCompose          ( Widget w, XtPointer client_data, XtPointer call_data );

        /* from folder.c */

extern void     DoClose                 ( Widget w, XtPointer client_data, XtPointer call_data );
extern void     DoComposeMessage        ( Widget w, XtPointer client_data, XtPointer call_data );
extern void     DoOpenFolder            ( Widget w, XtPointer client_data, XtPointer call_data );
extern void     DoOpenFolderInNewWindow ( Widget w, XtPointer client_data, XtPointer call_data );
extern void     DoCreateFolder          ( Widget w, XtPointer client_data, XtPointer call_data );
extern void     DoDeleteFolder          ( Widget w, XtPointer client_data, XtPointer call_data );
extern void     Push                    ( Stack *stack_ptr , char *data );
extern char *   Pop                     ( Stack *stack_ptr );
 
        /* from icon.c */
 
extern void     IconInit                ( void );
 
        /* from menu.c */
 
extern void     AttachMenuToButton      ( Button button, Widget w, char *menu_name );
/*
extern void     AddMenuEntry            ( Widget w, char *, ... );
*/
extern void     DoRememberMenuSelection ( Widget w, XtPointer client_data, XtPointer call_data );
extern void     SendMenuEntryEnableMsg  ( Button button, char *entry_name, int value);
extern void     ToggleMenuItem          ( Widget w, Boolean state);
 
        /* from msg.c */
 
extern Widget   CreateFileSource        ( Widget w, String filename, Boolean state);
 
        /* from popup.c */
 
extern void     DestroyPopup    ( Widget w, XtPointer client_data, XtPointer call_data );
extern void     PopupPrompt     ( String question, XtCallbackProc okayCallback);
extern void     PopupConfirm    ( Widget w, String, XtCallbackList, XtCallbackList );
extern void     PopupNotice     ( char *message, XtCallbackProc callback, XtPointer closure);
extern void     PopupError      ( String message);

	/* from screen.c */

extern void     EnableProperButtons     ( Scrn scrn );
extern Scrn     CreateNewScrn           ( ScrnKind );
extern Scrn     NewViewScrn             ( void );
extern Scrn     NewCompScrn             ( void );
extern void     ScreenSetAssocMsg       ( Scrn scrn, Msg msg);
extern void     DestroyScrn             ( Scrn scrn );
extern void     MapScrn                 ( Scrn scrn );
extern Scrn     ScrnFromWidget          ( Widget w );
          
        /* from tocfuncs.c */
 
extern Boolean  UserWantsAction         ( Widget w, Scrn scrn );
extern void     DoIncorporateNewMail    ( Widget w, XtPointer client_data, XtPointer call_data );
extern void     DoCommit                ( Widget w, XtPointer client_data, XtPointer call_data );
extern void     DoPack                  ( Widget w, XtPointer client_data, XtPointer call_data );
extern void     DoSort                  ( Widget w, XtPointer client_data, XtPointer call_data );
extern void     DoForceRescan           ( Widget w, XtPointer client_data, XtPointer call_data );
extern void     DoReverseReadOrder      ( Widget w, XtPointer client_data, XtPointer call_data );
extern void     DoNextView              ( Widget w, XtPointer client_data, XtPointer call_data );
extern void     DoPrevView              ( Widget w, XtPointer client_data, XtPointer call_data );
extern void     DoDelete                ( Widget w, XtPointer client_data, XtPointer call_data );
extern void     DoMove                  ( Widget w, XtPointer client_data, XtPointer call_data );
extern void     DoCopy                  ( Widget w, XtPointer client_data, XtPointer call_data );
extern void     DoUnmark                ( Widget w, XtPointer client_data, XtPointer call_data );
extern void     DoViewNew               ( Widget w, XtPointer client_data, XtPointer call_data );
extern void     DoReply                 ( Widget w, XtPointer client_data, XtPointer call_data );
extern void     DoForward               ( Widget w, XtPointer client_data, XtPointer call_data );
extern void     DoTocUseAsComp          ( Widget w, XtPointer client_data, XtPointer call_data );
extern void     DoPrint                 ( Widget w, XtPointer client_data, XtPointer call_data );
extern void     DoPickMessages          ( Widget w, XtPointer client_data, XtPointer call_data );
extern void     DoSelectSequence        ( Widget w, XtPointer client_data, XtPointer call_data );
extern void     DoOpenSeq               ( Widget w, XtPointer client_data, XtPointer call_data );
extern void     DoAddToSeq              ( Widget w, XtPointer client_data, XtPointer call_data );
extern void     DoRemoveFromSeq         ( Widget w, XtPointer client_data, XtPointer call_data );
extern void     DoDeleteSeq             ( Widget w, XtPointer client_data, XtPointer call_data );
 
	/* from util.c */

extern void     Punt                    ( char *str );
extern int      myopen                  ( char *str, int flags, int mode);
extern FILE *   myfopen                 ( char *path, char *mode );
extern int      myclose                 ( int fid );
extern int      myfclose                ( FILE *file );
extern char *   MakeNewTempFileName     ( void );
extern char **  MakeArgv                ( int n);
extern char **  ResizeArgv              ( char **str, int n);
extern FILEPTR  FOpenAndCheck           ( char *name, char *mode );
extern char *   ReadLine                ( FILE *file );
extern char *   ReadLineWithCR          ( FILE *file );
extern void     DeleteFileAndCheck      ( char *str );
extern void     CopyFileAndCheck        ( char *from, char *to );
extern void     RenameAndCheck          ( char *from, char *to );
extern char *   CreateGeometry          ( int gbits, int x, int y, int width, int height);
extern int      FileExists              ( char *str );
extern Boolean  IsSubfolder             ( char *str );
extern void     SetCurrentFolderName    ( Scrn scrn, char *str );
extern void     ChangeLabel             ( Widget w, char *str );
extern Widget   CreateTextSW    ( Scrn scrn, char *str, ArgList args, Cardinal num_args);
extern Widget   CreateTitleBar          ( Scrn scrn, char *str );
extern void     Feep                    ( void );
extern MsgList  CurMsgListOrCurMsg      ( Toc toc);
extern int      GetWidth                ( Widget w );
extern int      GetHeight               ( Widget w );
extern Toc      SelectedToc             ( Scrn scrn );
extern Toc      CurrentToc              ( Scrn scrn );
extern int      strncmpIgnoringCase();
extern void     StoreWindowName         ( Scrn scrn, char *str );
extern void     InitBusyCursor          ( Scrn scrn );
extern void     ShowBusyCursor          ( void );
extern void     UnshowBusyCursor        ( void );
extern void     SetCursorColor          ( Widget w, Cursor cursor, unsigned long foreground);
extern Boolean  ConvertMailText         (char *frompath, char *topath, Boolean ToEUC);
 
        /* from viewfuncs.c */
 
extern void     DoCloseView             ( Widget w, XtPointer client_data, XtPointer call_data );
extern void     DoViewReply             ( Widget w, XtPointer client_data, XtPointer call_data );
extern void     DoViewForward           ( Widget w, XtPointer client_data, XtPointer call_data );
extern void     DoViewUseAsComposition  ( Widget w, XtPointer client_data, XtPointer call_data );
extern void     DoEditView              ( Widget w, XtPointer client_data, XtPointer call_data );
extern void     DoSaveView              ( Widget w, XtPointer client_data, XtPointer call_data );
extern void     DoPrintView             ( Widget w, XtPointer client_data, XtPointer call_data );

