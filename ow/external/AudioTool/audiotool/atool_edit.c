/* Copyright (c) 1993 by Sun Microsystems, Inc. */
#ident	"@(#)atool_edit.c	1.38	93/02/17 SMI"

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <stropts.h>
#include <math.h>

#include <multimedia/libaudio.h>
#include "segment/segment_canvas.h"

#include "atool_types.h"
#include "atool_panel_impl.h"
#include "atool_panel.h"
#include "atool_i18n.h"
#include "atool_debug.h"

/* save a ptr to the last Edit function called for the Again function */
static void (*again_fcn)(ptr_t) = NULL;

/* Check if Undo is possible (used for menu item handlers to determine
 * if Undo item should be set inactive).
 */

AudPanel_Canundo(ptr_t atd)
{
	struct atool_panel_data		*ap;	/* audio data pointer */

	ap = (struct atool_panel_data *)atd;
	return (list_canundo(ap->lp));
}

/* Check if Redo is possible (used for menu item handlers to determine
 * if Redo item should be set inactive).
 */

AudPanel_Canredo(ptr_t atd)
	       				    	/* audio tool data */
{
	struct atool_panel_data		*ap;	/* audio data pointer */

	ap = (struct atool_panel_data *)atd;
	return (list_canredo(ap->lp));
}

/*
 * Undo the last edit (cut, copy, paste, insert).
 */
void
AudPanel_Undo(ptr_t atd)
{
	struct atool_panel_data		*ap;	/* audio data pointer */

	ap = (struct atool_panel_data *)atd;
	if (ap->EditLock)
		return;
	again_fcn = AudPanel_Undo;	/* set the again function */
	AudPanel_Stop(ap);
	/*
	 * If undo is successful and another undo is possible, do nothing.
	 * If we've done the last possible undo, set state to unmodified.
	 * If they hit undo in unmodified mode, print a message.
	 */
	if (list_undo(ap->lp)) {
		/* there are no more edits left to undo */
		if (!AudPanel_Unmodify(ap)) {
			/* there were no edits previously */
			AudPanel_Errormessage(ap, MGET("No edits to undo"));
			return;
		}
	}

	/* reset pointer since it's position is meaningless */
	ap->insert = 0.0;

	/* clear selection because range is meaning less */
	AudPanel_Clearselect(ap);

	/* Update display to reflect changes */
	update_graph(ap, 0.0, AUDIO_UNKNOWN_TIME);
}

/*
 * Undo the all edits (cut, copy, paste, insert).
 */
void
AudPanel_Undoall(ptr_t atd)
{
	struct atool_panel_data		*ap;	/* audio data pointer */

	ap = (struct atool_panel_data *)atd;
	if (ap->EditLock)
		return;
	again_fcn = AudPanel_Undoall;
	AudPanel_Stop(ap);
	/*
	 * If undo all is successful, set state to unmodified.
	 * If they hit undo in unmodified mode, print a message.
	 */
	if (list_undoall(ap->lp)) {
		/* there were no edits previously */
		AudPanel_Errormessage(ap, MGET("No edits to undo"));
		return;
	}
	(void) AudPanel_Unmodify(ap); 

	/* reset pointer since it's position is meaningless */
	ap->insert = 0.0;

	/* clear selection because range is meaning less */
	AudPanel_Clearselect(ap);

	/* Update display to reflect changes */
	update_graph(ap, 0.0, AUDIO_UNKNOWN_TIME);
}

/*
 * Undo the last undo.
 */
void
AudPanel_Redo(ptr_t atd)
{
	struct atool_panel_data		*ap;	/* audio data pointer */

	ap = (struct atool_panel_data *)atd;
	if (ap->EditLock)
		return;
	again_fcn = AudPanel_Redo;
	AudPanel_Stop(ap);

	if (list_redo(ap->lp)) {
		/* redo was unsuccessful */
		AudPanel_Errormessage(ap, MGET("No edits to redo"));
		return;
	} else {
		/* redo was successful */
		(void) AudPanel_Modify(ap);
	}

	/* reset pointer since it's position is meaningless */
	ap->insert = 0.0;

	/* clear selection because range is meaning less */
	AudPanel_Clearselect(ap);

	/* Update display to reflect changes */
	update_graph(ap, 0.0, AUDIO_UNKNOWN_TIME);
}

/*
 * Undo the last series of undos.
 */
void
AudPanel_Redoall(ptr_t atd)
{
	struct atool_panel_data		*ap;	/* audio data pointer */

	ap = (struct atool_panel_data *)atd;
	if (ap->EditLock)
		return;
	again_fcn = AudPanel_Redoall;
	AudPanel_Stop(ap);

	if (list_redoall(ap->lp)) {
		/* redo was unsuccessful */
		AudPanel_Errormessage(ap, MGET("No edits to redo"));
		return;
	} else {
		/* redo succeeded */
		(void) AudPanel_Modify(ap);
	}

	/* reset pointer since it's position is meaningless */
	ap->insert = 0.0;

	/* clear selection because range is meaning less */
	AudPanel_Clearselect(ap);

	/* Update display to reflect changes */
	update_graph(ap, 0.0, AUDIO_UNKNOWN_TIME);
}

/*
 * Copy selection to the shelf
 */
AudPanel_Putshelf(ptr_t atd, double st, double end)
	       			    	/* audio tool data */
	      			   	/* start of selection */
	      			    	/* end of selection */
{
	struct atool_panel_data		*ap;	/* audio data pointer */
	Audio_Object		svshelf;

	ap = (struct atool_panel_data *)atd;

	/* Save the current shelf, in case of error */
	svshelf = ap->shelf;

	/* Copy the selected region to the shelf */
	if (list_copy(ap->lp, st, end, &(ap->shelf)) == AUDIO_SUCCESS) {
		/* If there was something on the shelf, throw it away */
		if (svshelf != NULL)
			audio_dereference(svshelf);
	}

	/* assert ownership of window system clipboard */
	Audio_OWN_CLIPBOARD(ap->panel);

	AudPanel_Changestate(ap);
}

/*
 * Return a ptr to an audio object that contains the current selection
 * Args: 
 * ptr_t atd	audio tool data 
 * int all	all or just selection?
 * int shelf	get data from shelf (or use seln)?
 */
Audio_Object
AudPanel_GetSelData(
	ptr_t			atd,
	int			all,
	int			shelf)
{
	Audio_Object		svshelf;
	double			start;
	double			end;
	struct atool_panel_data	*ap = (struct atool_panel_data *)atd;

	if (shelf) {
		/* returns ptr to shelf object */
		if (!(svshelf = AudPanel_Getshelf(ap))) {
			DBGOUT((1, "** error: can't get copy of shelf\n"));
			return (NULL);
		}
		/* 
		 * since seln code deref's audio object, we must
		 * reference it here.
		 */
		audio_reference(svshelf);
		return (svshelf);
	}

	/* if there's no selection, zilch ... */
	if (!all && !SegCanvas_Selection(ap->subp.segment)) {
		DBGOUT((1, "** error: no selection to copy\n"));
		return (NULL);
	}

	if (all) {
		start = 0.0;
		end = AUDIO_UNKNOWN_TIME;
	} else {
		start = ap->start;
		end = ap->end;
	}

	/* Copy the selected region to the shelf */
	if (list_copy(ap->lp, start, end, &svshelf) != AUDIO_SUCCESS) {
		DBGOUT((1, "** error: can't copy selection\n"));
		return (NULL);
	}

	return (svshelf);
}

/*
 * Clear current audio selection.
 * Callers must call update_graph() or AudPanel_Changestate()
 * soon after this to make sure the changes appear on the screen.
 */
void
AudPanel_Clearselect(ptr_t atd)
	       			    	/* audio tool data */
{
	struct atool_panel_data		*ap;	/* audio data pointer */

	ap = (struct atool_panel_data *)atd;

	SegCanvas_Clearselect(ap->subp.segment);
	ap->start = ap->insert;
	ap->end = ap->insert;
}

/*
 * Get selection from the selection shelf
 */
Audio_Object
AudPanel_Getshelf(ptr_t atd)
	       				    	/* audio tool data */
{
	struct atool_panel_data		*ap;	/* audio data pointer */

	ap = (struct atool_panel_data *)atd;

	/* get data from server shelf XXX */

	/* Return current shelf data */
	return (ap->shelf);
}

/*
 * dereference the shelf (we've lost ownership of it)
 */
void
AudPanel_Releaseshelf(ptr_t atd)
	       				    	/* audio tool data */
{
	struct atool_panel_data		*ap;	/* audio data pointer */

	ap = (struct atool_panel_data *)atd;

	if (ap->shelf) {
		audio_dereference(ap->shelf);
		ap->shelf = NULL;
	}
}



/* Copy the selection to the internal, and server shelves */
void
AudPanel_Copy(
	ptr_t			atd)
{
	struct atool_panel_data	*ap = (struct atool_panel_data *)atd;

	again_fcn = AudPanel_Copy;

	if (ap->tstate == Recording) {
		DBGOUT((1, "Can't copy while recording\n"));
		return;
	}
	if (SegCanvas_Selection(ap->subp.segment)) {
		/* copy selection to the shelf */
		AudPanel_Putshelf(ap, ap->start, ap->end);
	} else {
		/* if there's no selection, print an error */
		AudPanel_Errormessage(ap, MGET("No selection to copy"));
	}
}

/* Copy the selection to a pipe (doing a drag) */
int
AudPanel_SendSelection(
	ptr_t			atd,
	Audio_Object		asel,
	int			fd)
{
	int			err;
	struct atool_panel_data	*ap = (struct atool_panel_data *)atd;

	if (asel) {
		/* copy selection to the pipe */
		err = audio_write_pipe(asel, fd);
	} else {
		/* if there's no selection, print an error */
		DBGOUT((1, "No selection to send\n"));
		err = AUDIO_ERR_NOEFFECT;
	}
	if (err != AUDIO_SUCCESS) {
		DBGOUT((1, "audio error: %s\n", audio_errmsg(err)));
	}
	return (err);
}

/* Cut silent ends from current file */
void
AudPanel_Deletesilentends(ptr_t atd)
{
	struct atool_panel_data		*ap;	/* audio data pointer */
	int				err;

	ap = (struct atool_panel_data *)atd;

	if (ap->EditLock || ap->empty || ap->tstate == Unloaded)
		return;

	if (ap->Detect_silence) {
		AudPanel_Stop(ap);
		again_fcn = AudPanel_Deletesilentends;
		err = list_trimends(ap->lp, ap->dp);
	} else {
		err = AUDIO_ERR_NOEFFECT;
	}
	switch (err) {
	case AUDIO_SUCCESS:
		(void) AudPanel_Modify(ap);

		/* reset pointer since it's position is meaningless */
		ap->insert = 0.0;

		/* clear selection because range is meaning less */
		AudPanel_Clearselect(ap);

		/* Update display to reflect changes */
		update_graph(ap, ap->start, ap->end);
		break;
	case AUDIO_ERR_NOEFFECT:
		AudPanel_Errormessage(ap, MGET("No silence to delete"));
		break;
	default:
		AudPanel_Errormessage(ap, MGET("Error deleting silence"));
		break;
	}
}

/* Cut silent ends from current file */
void
AudPanel_Deleteallsilence(ptr_t atd)
{
	struct atool_panel_data		*ap;	/* audio data pointer */
	int				err;

	ap = (struct atool_panel_data *)atd;

	if (ap->EditLock || ap->empty || ap->tstate == Unloaded)
		return;

	if (ap->Detect_silence) {
		AudPanel_Stop(ap);
		again_fcn = AudPanel_Deleteallsilence;
		err = list_trimsilence(ap->lp, ap->dp);
	} else {
		err = AUDIO_ERR_NOEFFECT;
	}
	switch (err) {
	case AUDIO_SUCCESS:
		(void) AudPanel_Modify(ap);

		/* reset pointer since it's position is meaningless */
		ap->insert = 0.0;

		/* clear selection because range is meaning less */
		AudPanel_Clearselect(ap);

		/* Update display to reflect changes */
		update_graph(ap, ap->start, ap->end);
		break;
	case AUDIO_ERR_NOEFFECT:
		AudPanel_Errormessage(ap, MGET("No silence to delete"));
		break;
	default:
		AudPanel_Errormessage(ap, MGET("Error deleting silence"));
		break;
	}
}


/* Delete unselected parts from current file */
void
AudPanel_Deleteunselected(ptr_t atd)
{
	struct atool_panel_data		*ap;	/* audio data pointer */

	ap = (struct atool_panel_data *)atd;

	if (ap->EditLock || ap->empty || ap->tstate == Unloaded)
		return;

	again_fcn = AudPanel_Deleteunselected;

	AudPanel_Stop(ap);

	/* If no selection, delete everything */
	if (!SegCanvas_Selection(ap->subp.segment)) {
		(void) AudPanel_Docut(atd, FALSE, TRUE);
		return;
	}

	/* cut unselected portions from the current edit list */
	(void) list_prune(ap->lp, ap->start, ap->end);
	(void) AudPanel_Modify(ap);

	/* reset pointer since it's position is meaningless */
	ap->insert = 0.0;

	/* clear selection because range is meaning less */
	AudPanel_Clearselect(ap);

	/* Update display to reflect changes */
	update_graph(ap, ap->start, ap->end);
}

/*
 * Perform cut/delete operation
 * Return FALSE if no action taken.
 */
int
AudPanel_Docut(ptr_t atd, int saveflag, int allflag)
{
	double			start;
	double			end;
	struct atool_panel_data	*ap = (struct atool_panel_data *)atd;

	if (ap->EditLock)
		return;
	AudPanel_Stop(ap);

	if (allflag) {
		start = 0.0;
		end = AUDIO_UNKNOWN_TIME;
	} else {
		start = ap->start;
		end = ap->end;
	}

	if (!(SegCanvas_Selection(ap->subp.segment) || allflag)) {
		return (FALSE);
	}

	/* copy selection to the shelf - for Cut only */
	if (saveflag) {
		AudPanel_Putshelf(ap, start, end);
	}

	/* cut selection from the current edit list */
	(void) list_replace(ap->lp, start, end, (Audio_Object *)NULL);
	(void) AudPanel_Modify(ap);

	/* clear selection, and set pointer to old start of selection */
	ap->insert = start;		/* gets displayed by update_graph */
	AudPanel_Clearselect(ap);

	if (allflag) {
		clear_graph(ap);

		/* change tool state */
		ap->insert = 0.0;
		ap->start = 0.0;
		ap->end = 0.0;
		ap->empty = TRUE;
	} else {
		/* Update display to reflect changes */
		update_graph(ap, start, end);
	}
	return (TRUE);
}

/* Cut range of selection from current file */
void
AudPanel_Cut(ptr_t atd)
{
	again_fcn = AudPanel_Cut;

	if (!AudPanel_Docut(atd, TRUE, FALSE))
		AudPanel_Errormessage(atd, MGET("No selection to cut"));
}

/*
 * Delete range of selection from current file (like cut, don't save).
 */
void
AudPanel_Delete(ptr_t atd)
{
	again_fcn = AudPanel_Delete;

	if (!AudPanel_Docut(atd, FALSE, FALSE))
		AudPanel_Errormessage(atd, MGET("No selection to delete"));
}

/* Delete all data from current file (like cut, don't save) */
void
AudPanel_Clear(ptr_t atd)
{
	again_fcn = AudPanel_Clear;

	(void) AudPanel_Docut(atd, FALSE, TRUE);
}

/*
 * Insert shelf data at insertion point, or replace current selection with
 * shelf data.
 */
void
AudPanel_Paste(ptr_t atd)
{
	struct atool_panel_data		*ap;	/* audio data pointer */
	double				start;	/* start of selection */
	double				end;	/* end of selection */
	Audio_Object			shelf;	/* shelf data */
	double				p_secs;/* length of pasted data */
	int				err;

	/* 
	 * Get selection and replace it with shelf data.
	 * If there was no selection, this is an insert.
	 * If there was a selection, replace the shelf after cut and insert.
	 */
	ap = (struct atool_panel_data *)atd;
	if (ap->EditLock)
		return;

	again_fcn = AudPanel_Paste;

	switch (ap->tstate) {
	case Recording:
		AudPanel_Errormessage(ap, MGET(
			"Can't paste while recording"));
		return;
	case Playing:
		/* stop activity */
		AudPanel_Stop(ap);
		break;
	case Unloaded:
	case Stopped:
		break;
	}

	/* if TRUE, will get clipboard via seln svc, so just return ... */
	if (Audio_GET_CLIPBOARD(ap->panel))
		return;

	/* get data from selection shelf and ensure it doesn't go away */
	shelf = AudPanel_Getshelf(ap);
	if (shelf == NULL) {
		/* If no shelf, paste is a no-op */
		AudPanel_Errormessage(ap, MGET("Nothing to paste"));
		return;
	}
	audio_reference(shelf);

	/* if there's no selection */
	if (!SegCanvas_Selection(ap->subp.segment)) {
		/* set insert point to current pointer position */
		start = ap->insert;
		end = start;
	} else {
		/* save selection range */
		start = ap->start;
		end = ap->end;
	}

	err = list_replace(ap->lp, start, end, shelf);
	p_secs = audio_getlength(shelf);

	/* Now the shelf can go away */
	audio_dereference(shelf);

	/* Update display to reflect changes */
	update_graph(ap, start, start + p_secs);
	if (err) {
		AudPanel_Errormessage(ap, MGET("Paste failed"));
		return;
	}

	/* if tool was unloaded, paste puts it in stopped state */
	if (ap->tstate == Unloaded) {
		ap->tstate = Stopped;
	}

	/* set pointer position to end of pasted data */
	AudPanel_Setinsert(ap, start + p_secs);
	(void) AudPanel_Modify(ap);
}

/*
 * Insert dropped data by reading AudioBuffer at insertion point, 
 * or replace current selection with data from AudioBuffer.
 * XXX - Compressed data should be decompressed using audioconvert
 */
int
AudPanel_InsertFromBufferList(ptr_t atd,    	/* audio tool data */
	Audio_Object	*abuflist,		/* list 'o buf's */
	int 		nbufs,			/* # bufs to insert */
	int 		rflag,      		/* replace all? */
	int 		sflag,			/* select after insert? */
	int		pflag)			/* auto play if load? */
{
	struct atool_panel_data		*ap;	/* audio data pointer */
	double				start;	/* start of selection */
	double				end;	/* end of selection */
	double				p_secs;	/* length of pasted data */
	Audio_Object			shelf;	/* shelf data */
	int				err;
	int				i;
	Audio_hdr			hdr;
	int				iscompressed;
	char 				msg[BUFSIZ];

	/* 
	 * Get selection and replace it with shelf data.
	 * If there was no selection, this is an insert.
	 * If there was a selection, replace the shelf after cut and insert.
	 */
	ap = (struct atool_panel_data *)atd;

	if (ap->EditLock)
		return (FALSE);

	AudPanel_Stop(ap);
	if (!abuflist) {
		DBGOUT((1, "Buffer list empty, nothing to insert"));
		return (FALSE);
	}
	    
	/*
	 * If we're reloading everything, try to unload (user may cancel).
	 * Don't break the ToolTalk link, if any.
	 */
	if (rflag) {
		if (!AudPanel_Unloadfile(ap, FALSE, NULL))
		    return (FALSE);
	}

	/* if there's no selection */
	if (!SegCanvas_Selection(ap->subp.segment)) {
		/* set insert point to current pointer position */
		start = ap->insert;
		end = start;
	} else {
		/* save selection range */
		start = ap->start;
		end = ap->end;
		AudPanel_Clearselect(ap);
		AudPanel_Changestate(ap);
	}

	/*
	 * check all buffers and see if we have compressed data - so we
	 * can put up the appropriate message. also, if the first file
	 * is compressed and we're doing a load, the default will 
	 * be to save as compressed.
	 * XXX - we need a better way to check and do the conversion
	 * XXX - data mismatch checked???
	 */
	ap->compression = AUDIO_ENCODING_NONE;
	for (i = 0, iscompressed = 0; i < nbufs; i++) {
		if (audio_getfilehdr(abuflist[i], &hdr) == AUDIO_SUCCESS) {
			switch (hdr.encoding) {
			case AUDIO_ENCODING_G721:
				if ((i == 0) && rflag)
					ap->compression = AUDIO_ENCODING_G721;
				iscompressed = TRUE;
				break;
			case AUDIO_ENCODING_G723:
				if ((i == 0) && rflag)
					ap->compression = AUDIO_ENCODING_G723;
				iscompressed = TRUE;
				break;
			default:
				break;
			}
		}
	}
	sprintf(msg, iscompressed ? MGET("Decompressing selection") :
	    rflag ? MGET("Loading selection") : MGET("Including selection"));
	AudPanel_Message(ap, msg);

	/* insert all the buffers back to back */
	for (p_secs = i = 0; i < nbufs; i++) {
		if (abuflist[i]) {
			audio_reference(abuflist[i]);
			if (rflag && (i == 0)) {
				err = list_insert(ap->lp, abuflist[i], 0.);
			} else {
				err = list_replace(ap->lp, start + p_secs, 
				    i ? (start + p_secs) : end, abuflist[i]);
			}
			if (err) {
				DBGOUT((1, "Selection transfer failed\n"));
			}
			p_secs += audio_getlength(abuflist[i]);
			audio_dereference(abuflist[i]);
		}
	}
	/* set pointer to end of pasted data */
	ap->insert = rflag ? 0 : (start + p_secs);

	/* if tool was unloaded, paste puts it in stopped state */
	if (ap->tstate == Unloaded) {
		ap->tstate = Stopped;
	}

	/* If load, init the headers in the state structure */
	if (rflag) {
		list_getfilehdr(ap->lp, 0.0, AUDIO_UNKNOWN_TIME, &ap->rec_hdr);
		memcpy((char*)&ap->save_hdr, (char*)&hdr, sizeof (Audio_hdr));
		FilePanel_Setformat(ap->subp.file, &ap->rec_hdr,
		    ap->compression);
	}

	/* Update display to reflect changes */
	update_graph(ap, start, start + p_secs);

	/* Make sure we've really got something before updating */
	if (!ap->empty) {
		if (!rflag)
			(void) AudPanel_Modify(ap);	/* modified, untitled */

		/* set selection if this is a drop */
		if (sflag) {
			ap->start = start;
			ap->end = start + p_secs;
			SegCanvas_Setselect(ap->subp.segment,
			    ap->start, ap->end);
		}
		if (rflag && (ap->Autoplay_load || pflag)) {
			AudPanel_Play(ap);
		} else if (sflag) {
			Select_proc(ap->subp.segment); /* play seln */
		}
	}
	return (TRUE);
}

/* select all audio data */
void
AudPanel_Selectall(ptr_t atd)
{
	struct atool_panel_data		*ap;	/* audio data pointer */

	ap = (struct atool_panel_data *)atd;

	if (ap->empty || (ap->tstate == Unloaded) || (ap->tstate == Recording))
		return;

	again_fcn = AudPanel_Selectall; /* set the again function */

	AudPanel_Stop(ap);
	ap->start = 0.0;
	ap->end = list_getlength(ap->lp);

	SegCanvas_Setselect(ap->subp.segment, ap->start, ap->end);
	AudPanel_Changestate(ap);		/* update menus, etc. */
	Select_proc(ap->subp.segment);		/* play selection if enabled */
}

/*
 * Insert dropped data be reading pipe at insertion point, 
 * or replace current selection with data from pipe
 *
 * XXX - this code should be merged with the paste code!!!
 * for now keep it seperate so we can debug.
 * XXX - also, need to make newly inserted data selected if
 * this was the result of a drop.
 * XXX - Compressed data should be decompressed using audioconvert
 */
int
AudPanel_InsertFromPipe(
	ptr_t			atd,		/* audio tool data */
	int			fd,		/* fd of pipe to read */
	int			rflag,		/* replace all audio? */
	int			sflag)		/* select after insert? */
{
	struct atool_panel_data	*ap;		/* audio data pointer */
	double			start;		/* start of selection */
	double			end;		/* end of selection */
	double			p_secs;		/* length of pasted data */
	Audio_Object		shelf;		/* shelf data */
	Audio_hdr		hdr;
	int			err;

	/* 
	 * Get selection and replace it with shelf data.
	 * If there was no selection, this is an insert.
	 * If there was a selection, replace the shelf after cut and insert.
	 */
	ap = (struct atool_panel_data *)atd;
	if (ap->EditLock)
		return (AUDIO_ERR_NOEFFECT);

	AudPanel_Stop(ap);

	/*
	 * If we're reloading everything, try to unload (don't unlink)
	 * and return if the user cancels it
	 */
	if (rflag) {
		if (!AudPanel_Unloadfile(ap, FALSE, NULL))
			return (AUDIO_NOERROR);
	}

	/* snarf data from pipe */
	err = audio_read_pipe(fd, &shelf);

	/* bail out on errors */
	if (err) {
		AudPanel_Errormessage(ap, MGET("Data transfer failed"));
		return (err);
	}

	/* if there's no selection */
	if (!SegCanvas_Selection(ap->subp.segment)) {
		/* set insert point to current pointer position */
		start = ap->insert;
		end = start;
	} else {
		/* save selection range */
		start = ap->start;
		end = ap->end;
		AudPanel_Clearselect(ap);
	}

	(void) audio_getfilehdr(shelf, &hdr);
	if (rflag) {
		err = list_insert(ap->lp, shelf, 0.);
	} else {
		err = list_replace(ap->lp, start, end, shelf);
	}
	p_secs = audio_getlength(shelf);

	/* dereference shelf so it gets freed */
	audio_dereference(shelf);

	/* XXX - If error, assume it was data conversion */
	if (err) {
		update_graph(ap, start, start + p_secs);
		AudPanel_Errormessage(ap, MGET("Data conversion failed"));
		return (err);
	}

	/* set pointer to end of pasted data */
	ap->insert = start + p_secs;

	/* if tool was unloaded, paste puts it in stopped state */
	if (ap->tstate == Unloaded) {
		ap->tstate = Stopped;
	}

	/* If load, init the headers in the state structure */
	if (rflag) {
		list_getfilehdr(ap->lp, 0.0, AUDIO_UNKNOWN_TIME, &ap->rec_hdr);
		memcpy((char*)&ap->save_hdr, (char*)&hdr, sizeof (Audio_hdr));
		ap->compression = AUDIO_ENCODING_NONE;
		switch (ap->save_hdr.encoding) {
		case AUDIO_ENCODING_G721:
			ap->compression = AUDIO_ENCODING_G721;
			break;
		case AUDIO_ENCODING_G723:
			ap->compression = AUDIO_ENCODING_G723;
			break;
		}
		FilePanel_Setformat(ap->subp.file, &ap->rec_hdr,
		    ap->compression);
	}

	/* Update display to reflect changes */
	update_graph(ap, start, start + p_secs);

	/* Make sure we've really got something before updating */
	if (!ap->empty) {
		if (!rflag)
			(void) AudPanel_Modify(ap);	/* modified, untitled */

		/*
		 * set selection if this is a drop. need to consider
		 * the length of the previous selection and new length
		 * after replace.
		 */
		if (sflag) {
			ap->start = start;
			ap->end = start + p_secs;
			SegCanvas_Setselect(ap->subp.segment,
			    ap->start, ap->end);
		}

		/* XXX - broken for multi-file load. should not do this here */
		if (rflag && ap->Autoplay_load) {
			AudPanel_Play(ap);
		} 
	}
	return (AUDIO_SUCCESS);
}

void
AudPanel_Again(ptr_t atd)
{
	struct atool_panel_data		*ap;	/* audio data pointer */

	ap = (struct atool_panel_data *)atd;

	if (again_fcn)
	    (*again_fcn)(ap);
}

/*
 * Called from the Audio Display Canvas while rubberbanding a selection.
 * Note:  Selection does not officially exist yet, just the image of the
 *	  selection exists in the user's mind (XXX?).
 *	  In other words, no data is saved on a shelf.
 */
Rubberband_select_proc(ptr_t spd)
	                        	/* segment data handle */
{
	struct atool_panel_data		*ap;	/* audio tool panel data  */
	double				ostart;	/* old start of selection */
	double				oend;	/* old end of selection */
	double				npos;	/* new play position */

	ap = AudPanel_KEYDATA(SegCanvas_Getowner(spd));

	/* can't adjust a selection in these states */
	if ((ap->tstate == Unloaded) || (ap->tstate == Recording) || ap->empty)
		return;

	/* save old and new selection ranges */
	ostart = ap->start;
	oend = ap->end;

	/* get new play range */
	(void) SegCanvas_Getselect(spd, &(ap->start), &(ap->end));

	/* if old start equals new start we're adjusting the end of selection */
	ap->adjust_end = SECS_EQUAL(ostart, ap->start);

	/*
	 * Constrain the pointer to the selection.  If adjusting the
	 * start/left side of selection, move pointer to the start of
	 * the selection.
	 * NOTE: SegCanvas_Getinsert is used because the pointer position 
	 * is not updated for us.  So we need to get insert position and 
	 * move the pointer ourselves.
	 */
	if (!ap->adjust_end) {
		/* adjusting left/start of the selection, move pointer there */
		AudPanel_Stop(ap);
		AudPanel_Setpointer(ap, ap->start);
	} else {
		/* don't let pointer beyond end of selection */
		if (ap->end < SegCanvas_Getinsert(spd)) {
			AudPanel_Stop(ap);
			AudPanel_Setpointer(ap, ap->end);
		}

	}

	/* display size of selection */
	AudPanel_Displaysize(ap);

	/* set new range using list_playstart */
	if (ap->tstate == Playing) {
		audio_play(ap, AUDIO_UNKNOWN_TIME, ap->Play_speed);
	}
}

/* Called from the Audio Display Canvas whenever a new selection is made */
Select_proc(ptr_t spd)
	                        	/* segment data handle */
{
	struct atool_panel_data	*ap;	/* audio tool panel data  */
	double			len;		/* length of audio */
	double			auto_st;	/* auto play start pos */
	double			auto_end;	/* auto play end pos */
	double			insert;		/* new insert pos */

	ap = AudPanel_KEYDATA(SegCanvas_Getowner(spd));

	/* can't adjust a selection in these states */
	if ((ap->tstate == Unloaded) || (ap->tstate == Recording) || ap->empty)
		return;

	/* init insert */
	insert = AUDIO_UNKNOWN_TIME;

	AudPanel_Changestate(ap);

	if ((ap->tstate != Playing) && (!ap->Autoplay_select))
		return;

	/* XXX hardwire auto edit feed back time to 1 secs */
#define	EDIT_CUE	1.0

	len = list_getlength(ap->lp);
	auto_st = ((len - ap->start) < EDIT_CUE) ? len : (ap->start + EDIT_CUE);
	auto_end = (ap->end < EDIT_CUE) ? 0.0 : (ap->end - EDIT_CUE);

	if ((ap->Autoplay_select) &&
	    (ap->adjust_end) && (auto_end < SegCanvas_Getinsert(spd))) {
		/* 
		 * if we adjusted the right side and it is somewhat close to 
		 * the pointer, assume user is trying to find exact 
		 * end of selection, help by setting insert to just before end
		 */
		insert = auto_end;
	}

	/* 
	 * if already playing, stop audio and reset pointer to posibly "better" 
	 * position. if stoppped, auto play on select is on so start
	 * play at current position
	 */
	if (ap->tstate == Playing) {
		audio_play(ap, insert, ap->Play_speed);
	} else {
		/* only set pointer if variable "insert" was modified */
		if (!SECS_EQUAL(insert, AUDIO_UNKNOWN_TIME)) {
			AudPanel_Setpointer(ap, insert); 
		}
		AudPanel_Play(ap);
	}
	return;
}

