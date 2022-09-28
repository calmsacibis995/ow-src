/* Copyright (c) 1993 by Sun Microsystems, Inc. */
#ident	"@(#)atool_ctl.c	1.77	93/03/10 SMI"

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <sys/signal.h>
#include <sys/types.h>
#include <sys/file.h>
#include <sys/fcntl.h>
#include <sys/stat.h>
#include <stropts.h>
#include <math.h>

#include "segment/segment_canvas.h"

#include <multimedia/libaudio.h>
#include <multimedia/AudioDetect.h>
#include <multimedia/audio_device.h>

#include "atool_types.h"
#include "atool_debug.h"
#include "atool_panel_impl.h"
#include "atool_panel.h"
#include "atool_i18n.h"


/* Flag waiting for audio output device */
unsigned	output_waiting = FALSE;
unsigned	input_waiting = FALSE;

long		playrec_timer = 50000; /* usec for play/record timer */

#define	RECORD_SPACE_PAD	(10.)	/* seconds to pad temp file space */

/* local fcn's */

double confine_pointer(struct atool_panel_data*	ap, double secs);

/*
 * Reset play pointer to beginning, set insertion point to beginning,
 * clear selection.
 */
void
AudPanel_Reset(ptr_t iap)
{
	struct atool_panel_data	*ap = (struct atool_panel_data *)iap;

	if (ap->empty)
		return;

	AudPanel_Stop(ap);
	AudPanel_Setinsert(ap, 0.0);
	AudPanel_Changestate(ap);
}

/*
 * update the pointer display image, position in seconds, and
 * confine pointer to selection, or if no selection to file length
 */
AudPanel_Setpointer(
	ptr_t			iap,		/* audio tool data */
	double			secs)		/* time in seconds */
{
	struct atool_panel_data	*ap = (struct atool_panel_data *)iap;

	/* update pointer image and value */
	SegCanvas_Setpointer(ap->subp.segment, secs);
}

/*
 * Given a position, calculate the amplitude of the current sample
 * and set level meter accordingly
 */
void
AudPanel_Setplaylevel(
	ptr_t			iap,
	double			pos)
{
	struct atool_panel_data	*ap;	/* audio data pointer */
	AudioLevel		lev;

	ap = (struct atool_panel_data *)iap;
	sigio_block(ap);
	lev = list_playlevel(ap->lp, pos);
	sigio_unblock(ap);
	AudPanel_SETLEVEL(ap->panel, lev.level, lev.clip);
}

/*
 * Given a position, calculate the amplitude of the current sample
 * and set level meter accordingly
 */
void
AudPanel_Setrecordlevel(
	ptr_t			iap,
	double			pos)
{
	struct atool_panel_data	*ap;	/* audio data pointer */
	AudioLevel		lev;

	ap = (struct atool_panel_data *)iap;
	sigio_block(ap);
	lev = list_recordlevel(ap->lp, pos);
	sigio_unblock(ap);
	AudPanel_SETLEVEL(ap->panel, lev.level, lev.clip);
}

/*
 * Move the play pointer by offset secs.
 */
double
AudPanel_Seek(ptr_t iap, double offset)
{
	struct atool_panel_data		*ap;	/* audio data pointer */
	double				cpos;	/* current position */
	double				npos;	/* new position */

	ap = (struct atool_panel_data *)iap;

	/* bad states */
	switch (ap->tstate) {
	case Unloaded:
	case Saving:
	case Loading:
	case Recording:
		/* DO NOTHING */
		return (0.);
	}

	/* no effect */
	if (ap->empty || (offset == 0.0))
		return (0.);

	/* get the current play position */
	cpos = (ap->tstate == Playing) ? list_playposition(ap->lp) : ap->insert;

	/* move pointer by offset (keep confined to selection) */
	npos = confine_pointer(ap, cpos + offset);
	AudPanel_Setpointer(ap, npos);

	/* if new position comes up short, we ran into the end so stop */
	if (SECS_LT(npos, (cpos + offset)))
		AudPanel_Stop(ap);

	if (ap->tstate == Playing)
		audio_play(ap, ap->insert, ap->Play_speed);

	/* return the actual offset */
	return (npos - cpos);
}


/*
 * Move the play pointer to the next marker.  If forward is true, move
 * the pointer forward, otherwise move it backwards.
 */
AudPanel_Findmarker(ptr_t iap, int forward)
	       				    	/* audio tool data */
	   				         /* search forward if TRUE */
{
	struct atool_panel_data		*ap;	/* audio data pointer */
	double				ppos;	/* play position */
	double				npos;	/* new position */


	ap = (struct atool_panel_data *)iap;

	/* bad states */
	switch (ap->tstate) {
	case Unloaded:
	case Saving:
	case Loading:
	case Recording:
		/* DO NOTHING */
		return;
	}
	if (ap->empty)
		return;

	/* get the current pointer position */
	ppos = (ap->tstate == Playing) ? list_playposition(ap->lp) : ap->insert;

	/* use current pointer position to find nearest marker */
	npos = SegCanvas_Findsound(ap->subp.segment, ppos, forward);

	/* XXX need a better way to do reverse */
	if (!forward) {
		/* if new value very close to play position */
		if (SECS_LT(ppos - npos, .5)) {
			/* jump back one more */
			npos = SegCanvas_Findsound(ap->subp.segment, 
			    npos, forward);
		}
	}

	/* make sure new value is within selection or file */
	npos = confine_pointer(ap, npos);

	if (ap->tstate == Playing)
		audio_play(ap, npos, ap->Play_speed);
	else
		AudPanel_Setpointer(ap, npos);
}

/*
 * Change the Play speed
 */
void
AudPanel_Playspeed(ptr_t iap, double speed)
	       				    	/* audio tool data */
{
	struct atool_panel_data		*ap;	/* audio data pointer */

	ap = (struct atool_panel_data *)iap;

	ap->Play_speed = speed;

	/* reset speed */
	if (ap->tstate == Playing) {
		audio_play(ap, AUDIO_UNKNOWN_TIME, speed);
	} 
}



/*
 * Start playing audio from the specified position.  If play position is
 * UNKNOWN, use the current play position.
 *	Return -1 if audio is not going to play
 */
int
audio_play(ptr_t iap, double pos, double speed)
	       				    	/* audio tool data */
	       				    	/* speed to play at */
{
	struct atool_panel_data		*ap;	/* audio data pointer */
	int				err;	/* audio error */
	double				start;	/* start of play range */
	double				end;	/* end of play range */

	ap = (struct atool_panel_data *)iap;

	/* there's nothing to play! */
	if ((ap->tstate == Unloaded) || ap->empty || ap->noplay)
		return (-1);

	/* set play range  */
	if (SegCanvas_Selection(ap->subp.segment)) {
		start = ap->start;
		end = ap->end;
	} else {
		start = 0.0; 
		end = list_getlength(ap->lp);
	}

#ifdef notdef
	/* verify play position in within play range */
	if (SECS_LT(pos, ap->start) ||
	   SECS_GT(pos, ap->end)) {
		 /* XXX Bug somewhere if we get here!  */
		 pos = ap->start;
	}
#endif
		
	sigio_block(ap);

	/* Play region from start to end */
	ap->Play_speed = speed;
	err = list_playstart(ap->lp, ap->devname, start, end, pos, speed);
	DBGOUT((1,
	    "Playstart left = %.2f\tright = %.2f\tpos = %.8f\tspeed = %.2f\n",
	    start, end, (pos == AUDIO_UNKNOWN_TIME) ? -5.0 : pos, speed));

	sigio_unblock(ap);
	switch (err) {
	case AUDIO_ERR_DEVICEBUSY:
		if (ap->ctldev == NULL) {
			AudPanel_Errormessage(ap, MGET("Audio output is busy"));
			break;
		}
		AudPanel_Errormessage(ap, 
		    MGET("Audio output is busy...waiting..."));
		output_waiting = TRUE;
		audio_setplaywaiting(ap->ctldev);
							/* fall through */
	case 0:			/* play was started; kick the data copy loop */
		schedule_async_handler(ap);
	case AUDIO_EOF:		/* all data was queued already */
		return (0);

	case AUDIO_ERR_FORMATLOCK:
	case AUDIO_ERR_BADHDR:
	case AUDIO_ERR_BADFILEHDR:
	case AUDIO_ERR_ENCODING:
		AudPanel_Errormessage(ap, 
		    MGET("Cannot set audio format"));
		break;

	case AUDIO_ERR_NOTDEVICE:
	case AUDIO_UNIXERROR:
	default:
		AudPanel_Errormessage(ap, MGET("Cannot open audio device"));
		break;
	}
	return (-1);
}


/*
 * Play current file, or selection of file.  Reset set pointer to 
 * start if it's at the end of the selection of file.
 */
AudPanel_Play(ptr_t iap)
	       				    	/* audio tool data */
{
	struct atool_panel_data		*ap;	/* audio data pointer */
	int				err;	/* audio error */

	ap = (struct atool_panel_data *)iap;

	switch (ap->tstate) {
	case Recording:
		/* stop recording */
		AudPanel_Stop(ap);

		/* set end of selection to end of recording */
		ap->end = ap->insert;

		/* select previously recorded data */
		SegCanvas_Setselect(ap->subp.segment, ap->start, ap->end);
		break;
	case Playing:
		/* already playing, do nothing */
		return;
	case Saving:
	case Loading:
	case Unloaded:
		return;
	case Stopped:
		break;
	}

	/* 
	 * Reset play pointer to start of selection, or file if
	 * it's currently at the end of selection or file and
	 * we're not about to play in reverse.
	 */
	if (SegCanvas_Selection(ap->subp.segment)) {
		if (SECS_LT(ap->insert, ap->start) || 
		    (!SECS_LT(ap->insert, ap->end))) {
			/* 
			 * if play position is outside of or at the end 
			 * of selection, set it to start of selection 
			 */
			if (ap->Play_speed >= 0.)
				ap->insert = ap->start;
		}
	} else {
		
		/* if pointer >= to end of the file, set it to beginning */
		if (!SECS_LT(ap->insert, list_getlength(ap->lp))) {
			if (ap->Play_speed >= 0.)
				ap->insert = 0.0;
		}
	}

	sigio_block(ap);

	/* start playing the audio */
	if (audio_play(ap, ap->insert, ap->Play_speed) < 0)
		goto playerr;

	/* start animation */
	AudPanel_START_TIMER(ap, playrec_timer);

	/* switch to the stop button */
	AudPanel_SETPLAYBUTTON(ap->panel,
	    ap->stop_play_button_image, ap->Stop_string);

	/* change state */
	ap->tstate = Playing;
	AudPanel_Changestate(ap);
playerr:
	sigio_unblock(ap);
}

/*
 * Record into current file at position pointer.  If no current file,
 * record into an unnamed file.
 */
AudPanel_Record(ptr_t iap)
{
	struct atool_panel_data	*ap;		/* audio data pointer */
	int			err;	
	double			start;		/* start of selection */
	double			end;		/* end of selection */
	double			space;

	ap = (struct atool_panel_data *)iap;

	input_waiting = FALSE;
	if (ap->noplay)		/* quit now if play controls are disabled */
		return;

	switch (ap->tstate) {
	case Saving:
	case Loading:
		return;
	case Recording:	
		/* stop playing */
		AudPanel_Stop(ap);
		return;
	case Unloaded:
		/* set start to zero */
		start = 0.0;
		end = 0.0;
		AudPanel_Setpointer(ap, start);
		break;
	case Playing:
		/* stop playing */
		AudPanel_Stop(ap);
		/* FALL THROUGH AND START RECORD */
	case Stopped:
		/* Record starts by cutting the selection */
		if (SegCanvas_Selection(ap->subp.segment)) {
			/* copy selection range */
			start = ap->start;
			end = ap->end;

			/* First, copy cut selection to the shelf */
			AudPanel_Putshelf(ap, start, end);
		} else {
			/* If no selection, record inserts at the current pos */
			start = ap->insert;
			end = start;
		}
		break;
	}

	space = audio_bytes_to_time((1024 * getfreespace(ap->recpath)),
	    &ap->rec_hdr) - RECORD_SPACE_PAD;
	if (space <= 3.) {
		AudPanel_Errormessage(ap, MGET("Temp directory full"));
		return;
	}

	/* block SIGPOLL */
	sigio_block(ap);

	/*
	 * Start recording...
	 * Note that ap->rec_hdr is set when user does a New or Open
	 */
	err = list_recordstart(ap->lp, ap->devname, ap->recpath, &(ap->rec_hdr),
	    start, end);
	if (err != 0) {
		switch (err) {
		case AUDIO_ERR_DEVICEBUSY:
			if (ap->ctldev != NULL) {
				/* Try setting flag and wait a short time */
				audio_setrecordwaiting(ap->ctldev);
				input_waiting = TRUE;
				AudPanel_START_TIMER(ap, playrec_timer);
			}
			AudPanel_Errormessage(ap, MGET("Audio input is busy"));
			break;

		case AUDIO_ERR_FORMATLOCK:
			AudPanel_Errormessage(ap, 
			    MGET("Cannot set audio format"));
			break;

		case AUDIO_ERR_NOTDEVICE:
			AudPanel_Errormessage(ap,
			    MGET("Cannot open audio device"));
			break;

		case AUDIO_UNIXERROR:
		default:
			AudPanel_Errormessage(ap,
			    MGET("Could not create output file"));
			break;
		}
		sigio_unblock(ap);
		return;
	}
	/* kick the copy routine to start moving data */
	schedule_async_handler(ap);

	/* unblock SIGPOLL */
	sigio_unblock(ap);

	/* notify user */
	/* XXX - add format string? */
	AudPanel_Message(ap, MGET("Recording..."));

	/* save start of new recording */
	AudPanel_Setpointer(ap, start);
	ap->start = start;

	/* make forward and reverse buttons inactive */
	AudPanel_ACTIVATESEARCH(ap->panel, FALSE);

	/* switch the record button to the stop button */
	AudPanel_SETRECORDBUTTON(ap->panel, ap->stop_record_button_image,
	    ap->Stop_string);

	/* flag file as modified */
	(void) AudPanel_Modify(ap);

	/* change state */
	ap->tstate = Recording;
	AudPanel_Changestate(ap);

	/* start timer to get screen updates */
	AudPanel_START_TIMER(ap, playrec_timer);
}

/*
 * set insert point - this is equivalent to the user picking an
 * insert point with the mouse, which basically means the selection
 * is cleared, and the pointer moved.
 * (But with the precision of a double, instead of a converted pixel)
 */
AudPanel_Setinsert(ptr_t iap, double time)
	       				    	/* audio tool data */
	      				     	/* time to reset pointer to */
{
	struct atool_panel_data		*ap;	/* audio data pointer */

	ap = (struct atool_panel_data *)iap;

	/* move pointer to new position */
	AudPanel_Setpointer(ap, time);

	/* insert clears the selection by definition */
	AudPanel_Clearselect(ap);
}


/*
 * Given time make sure it falls with in selection range, or if no
 * selection make sure it falls within the file range. 
 * Return confined time.
 */
double
confine_pointer(
	struct atool_panel_data*	ap,
	double 				secs)
{
	if (SegCanvas_Selection(ap->subp.segment)) {
		/* if selection, confine new position to be within selection */ 
		if (SECS_LT(secs, ap->start))
			secs = ap->start;
		else if (!SECS_LT(secs, ap->end))
			secs = ap->end;
	} else {
		/* confine pointer to file range */
		if (SECS_LT(secs, 0.0)) {
			secs = 0.0;
		} else if (secs > list_getlength(ap->lp)) {
			secs = list_getlength(ap->lp);
		}
	}
	return (secs);
}

/*
 * Stop any audio activity.
 */
audio_stop(ptr_t iap)
	       				    	/* audio tool data */
{
	struct atool_panel_data		*ap;	/* audio data pointer */

	ap = (struct atool_panel_data *)iap;

	switch (ap->tstate) {
	case Unloaded:
	case Stopped:
	case Saving:
	case Loading:
		/* tool is not doing device I/O */
		return;
	case Playing:
		/* stop the audio output */
		list_flush(ap->lp);
		break;
	}
	list_stop(ap->lp);
}


/*
 * Stop current activity
 */
AudPanel_Stop(
	ptr_t			iap)
	       				    	/* audio tool data */
{
	struct atool_panel_data	*ap;		/* audio data pointer */
	Atool_state		state;		/* state before stopping */
	double			start;		/* start of selection */
	double			endpt;		/* end of play */

	ap = (struct atool_panel_data *)iap;
	/* check for a no-op */
	if ((ap->tstate == Unloaded) || (ap->tstate == Stopped))
		return;

	sigio_block(ap);

	/* save the state, since audio_stop will clobber it */
	state = ap->tstate;

	/* Display the audio format */
	AudPanel_Showtypeinfo(ap);

	/* stop animation */
	AudPanel_STOP_TIMER(ap);

	/* stop the audio output */
	audio_stop(ap);

	switch (state) {
	case Playing:
		if (output_waiting) {
			output_waiting = FALSE;
		} else {
			/* make sure pointer is exactly at current position */
			endpt = list_playposition(ap->lp);
			AudPanel_Setpointer(ap, endpt);
			update_pointer_time(iap, endpt);
		}

		/* switch stop to the play button */
		AudPanel_SETPLAYBUTTON(ap->panel,
		    ap->play_button_image, ap->Play_string);
		break;

	case Recording:
		/* make forward and reverse buttons active */
		AudPanel_ACTIVATESEARCH(ap->panel, TRUE);

		/* switch the stop button to the record button */
		AudPanel_SETRECORDBUTTON(ap->panel,
		    ap->record_button_image, ap->Rec_string);

		/* Update display to reflect changes */
		update_graph(ap, 0.0, AUDIO_UNKNOWN_TIME);
		AudPanel_Setpointer(ap,
		    ap->insert + list_recordposition(ap->lp));

		/* save start of selection, but clear the selection */
		start = ap->start;
		AudPanel_Clearselect(ap);
		ap->start = start;
		break;
	case Loading:
		/* stop current load activity, etc. */
		AudPanel_Cancelload(ap);
		break;
	case Saving:
		/* stop current save activity, etc. */
		AudPanel_Cancelsave(ap);
		break;
	case Unloaded:
	case Stopped:
		/* stop waiting for audio record access */
		if (input_waiting) {
			input_waiting = FALSE;
		}
	default:
		break;
	}

	/* changed state */
	ap->tstate = Stopped;

	/* reset level meter to 0 */
	AudPanel_Changestate(ap);
	sigio_unblock(ap);
}


/* Update the pointer time value of audiotool */
update_pointer_time(ptr_t iap, double secs)
	       				    	/* audio tool data */
	      				     	/* time in seconds */
{
	struct atool_panel_data		*ap;	/* audio data pointer */
	char				time_str[AUDIO_MAX_TIMEVAL];

	ap = (struct atool_panel_data *)iap;

	/* update insert value */
	ap->insert = secs;

	/* convert time to string */
	(void) audio_secs_to_str(secs, time_str, 1);

	/* update time display */
	AudPanel_POINTERTIME(ap, time_str);
}

/* Called from the Audio Display Canvas whenever the pointer position changes */
Pointer_proc(ptr_t spd)
	                                        /* segment data handle */
{
	struct atool_panel_data		*ap;	/* audio tool panel data  */

	ap = AudPanel_KEYDATA(SegCanvas_Getowner(spd));

	/* update pointer value and display */
	update_pointer_time(ap, SegCanvas_Getpointer(spd));
}

/* Update the cursor time value of audiotool */
update_cursor_time(
	ptr_t			iap,		/* audio tool data */
	double			secs)		/* time in seconds */
{
	struct atool_panel_data	*ap;		/* audio data pointer */
	char			time_str[AUDIO_MAX_TIMEVAL];

	ap = (struct atool_panel_data *)iap;

	/* convert time to string */
	(void) audio_secs_to_str(secs, time_str, 1);

	/* update time display */
	AudPanel_CURSORTIME(ap, time_str);
}


/* Called from the Audio Display Canvas whenever the cursor position changes */
Cursor_proc(ptr_t spd)
	                        	/* segment data handle */
{
	struct atool_panel_data		*ap;	/* audio tool panel data  */

	ap = AudPanel_KEYDATA(SegCanvas_Getowner(spd));

	/* update time value label */
	update_cursor_time(ap, SegCanvas_Getcursor(spd));
}

/*
 * Called from the Audio Display Canvas whenever the mouse enters
 * the audio canvas
 */
enter_audio_canvas(ptr_t spd)
	                        	/* segment data handle */
{
	struct atool_panel_data		*ap;	/* audio tool panel data  */

	ap = AudPanel_KEYDATA(SegCanvas_Getowner(spd));

	AudPanel_CURSORLABEL(ap, ap->Cursor_string);
	update_cursor_time(ap, SegCanvas_Getcursor(spd));
}

/*
 * Called from the Audio Display Canvas whenever the mouse exits
 * the audio canvas
 */
exit_audio_canvas(ptr_t spd)
	                        	/* segment data handle */
{
	struct atool_panel_data		*ap;	/* audio tool panel data  */

	ap = AudPanel_KEYDATA(SegCanvas_Getowner(spd));

	/* blank out time cursor display */
	AudPanel_CURSORLABEL(ap, "");
	AudPanel_CURSORTIME(ap, "");
}


/* Lock out SIGIO async handler */
void
sigio_block(struct atool_panel_data *ap)
	                       		    	/* audio tool panel data  */
{

        if (ap->Async == 0)
#ifdef SUNOS41
                ap->Save_sigmask = sigblock(sigmask(SIGIO));
#else
                ap->Save_sigmask = sighold(SIGIO);
#endif

        /* Increment Async in case of multiple calls */
        ap->Async++;
}

/* Unblock SIGIO async handler */
void
sigio_unblock(struct atool_panel_data *ap)
	                       		    	/* audio tool panel data  */
{
        if (--(ap->Async) == 0)
#ifdef SUNOS41
                (void) sigsetmask(ap->Save_sigmask);
#else
                (void) sigprocmask(SIG_SETMASK,
				   (const sigset_t*)&(ap->Save_sigmask),
				   NULL);
#endif
}


/*
 * Signal handler entered asynchronously (!) when packets arrive.
 * Returns AUDIO_SUCCESS or an error code.
 * Return == AUDIO_ERR_INTERRUPTED, if a synchronous event should be scheduled.
 */
int
async_handler(
	ptr_t				iap)
{
	struct atool_panel_data		*ap;	/* audio data pointer */
	int				err;

	ap = (struct atool_panel_data *)iap;
	err = AUDIO_SUCCESS;

	/* by definition SIGIO is blocked, so bump sigio_block() counter */
	ap->Async++;

	switch (ap->tstate) {
	case Playing:
	case Recording:
		if (!output_waiting) {
			/* get/send more data to/from device */
			err = list_async(ap->lp);
		}
		break;
	case Loading:
	case Saving:
		break;
	default:
	case Unloaded:
	case Stopped:
		/* Ignore signal */
		DBGOUT((3,"got async event in stopped/unloaded state\n"));
	}
	ap->Async--;
	return (err);
}

/*
 * Synchronous SIGIO handler.  
 * Locks out SIGIO, then calls async routine.
 */
int
sync_handler(
	ptr_t			iap)
{
static int			overflowmsgseen = FALSE;
	int			err;
	int			resched;

	resched = FALSE;
	sigio_block(iap);
	err = async_handler(iap);
	sigio_unblock(iap);

	switch (err) {
	default:
	case AUDIO_SUCCESS:
		break;
	case AUDIO_ERR_DEVOVERFLOW:
		AudPanel_Stop(iap);
		AudPanel_Errormessage(iap, MGET("Record data overflow"));

		/* Pop up an alert only once per run */
		if (!overflowmsgseen) {
			overflowmsgseen = TRUE;
			AudPanel_Alert(iap, MGET(
	    "A data overflow occurred during the record operation\n"
	    "and the last few seconds of data may be corrupted.\n"
	    "This is likely to occur if the Temp file directory is set\n"
	    "to a directory on a slow disk or remote file system,\n"
	    "or if the file server or network is busy.  You may:\n"
	    "retry the record operation,\n"
	    "set the audio format to a lower data rate, or\n"
	    "edit the Properties to change the Temp file directory.\n\n"
	    "Undo the last operation to discard the data just recorded.\n"
			    ));
		}
	case AUDIO_ERR_INTERRUPTED:
		resched = TRUE;
		break;
	}
	return (resched);
}

/*
 * Synchronous handler for screen updates.
 * Does things that would interfere with X server.
 */
update_handler(
	ptr_t				iap)
{
	struct atool_panel_data		*ap;	/* audio data pointer */
	double				pos;	/* pointer position */
	double				space;	/* time remaining */

	ap = (struct atool_panel_data *)iap;

	switch (ap->tstate) {
	case Playing:
		/* if waiting for output device */
		if (output_waiting) {
			/* if the device is still busy, do nothing */
			if ((ap->ctldev == NULL) ||
			    audio_getplayopen(ap->ctldev))
				break;

			/* try to open the device again */
			output_waiting = FALSE;
			audio_play(ap, ap->insert, ap->Play_speed);
			if (output_waiting)
				break;
			AudPanel_Showtypeinfo(ap);
		}

		/* if play ran out, reset button, state, and close device */
		if (list_eof(ap->lp)) {
			AudPanel_Stop(iap);
			break;
		}

		/* update LED's and animate pointer */
		pos = list_playposition(ap->lp);
		AudPanel_Setplaylevel(ap, pos);
		AudPanel_Setpointer(ap, pos);

		break;

	case Recording:
		/* update file size & recorded length */
		pos = list_recordposition(ap->lp);
		AudPanel_Sizemsg(ap, list_getlength(ap->lp), pos);

		/* if record ran out, reset button, state, and close device */
		if (list_eof(ap->lp)) {
			AudPanel_Stop(iap);
			break;
		}

		/* update LED's and animate pointer */
		blink_pointer(ap, ap->insert);
		AudPanel_Setrecordlevel(ap,
		    ap->insert + list_recordbufposition(ap->lp));

		/* check free space remaining */
		space = audio_bytes_to_time(
		    (1024 * getfreespace(ap->recpath)), &ap->rec_hdr) -
		    RECORD_SPACE_PAD;
		if (space <= 0.) {
			AudPanel_Stop(ap);
			AudPanel_Errormessage(ap, MGET("Temp directory full"));
		} else if (space < 60.45) {
			char	msg[BUFSIZ];

			(void) sprintf(msg, ap->Space_string, (int) space);
			AudPanel_Message(ap, msg);
		}

		break;
	case Saving:
		/* Here's where we actually do the data writes */
		(void) do_async_writefile(ap);
		break;
	case Loading:
		/* Update filesystem freespace message */
		FilePanel_Updatefilesize(ap->subp.file);
		break;
	case Unloaded:
	case Stopped:
		/* If waiting to record, try again.  If still busy, cancel */
		if (input_waiting) {
			AudPanel_Record(iap);
			if (input_waiting) {
				AudPanel_STOP_TIMER(ap);
				input_waiting = FALSE;
			}
		}
	default:
		/* Ignore signal */
		break;
	}
}

/*
 * Blink the pointer. If it's on turn if off, and visa versa
 */
blink_pointer(ptr_t iap, double pos)
{
	struct atool_panel_data		*ap;	/* audio data pointer */

	ap = (struct atool_panel_data *)iap;

	if (!ap->pointer_cleared) {
		SegCanvas_Clearpointer(ap->subp.segment);
		ap->pointer_cleared = TRUE;
	} else {
		SegCanvas_Setpointer(ap->subp.segment, pos);
		ap->pointer_cleared = FALSE;
	}
}

/* Print the detection list to stdout */
Print_detect(ptr_t iap)
	       				    	/* audio tool data */
{
	struct atool_panel_data		*ap;	/* audio data pointer */

	ap = (struct atool_panel_data *)iap;

	print_detect(ap->dp); 
}

/* Clear the bar graph and pointer display */
clear_graph(ptr_t iap)
{
	struct atool_panel_data		*ap = (struct atool_panel_data *)iap;

	ap->start = 0.;
	ap->end = 0.;
	ap->insert = 0.;
	ap->empty = TRUE;
	ap->dp = NULL;

	/* clear file size footer */
	AudPanel_Sizemsg(ap, 0.0, 0.0);

	/* clear the pointer time display */
	AudPanel_POINTERLABEL(ap, "");
	AudPanel_POINTERTIME(ap, "");

	/* Display the audio format */
	AudPanel_Showtypeinfo(ap);

	/* clear bar graph */
	SegCanvas_Clearfile(ap->subp.segment);

	/* Display the audio format and update controls, etc */
	AudPanel_Showtypeinfo(ap);
	AudPanel_Changestate(ap);

	/* unbusy the tool (may have been busied prior to entry) */
	AudPanel_Unbusy(ap);
}

/* Update the display list of the bar graph, then redisplay it */
/* XXX - by the way, updates the entire tool state display, too.
 *       this should be moved to a generic panel routine.
 */
update_graph(
	ptr_t			iap,		/* audio tool data */
	double			st,		/* start of damage area */
	double			end)		/* end of damage area */
{
static	AudioDetectPts		onesound[2];	/* detect points  */
	int			err;		/* audio error */
	double			len;		/* length of audio data */
	struct atool_panel_data	*ap = (struct atool_panel_data *)iap;

	len = list_getlength(ap->lp);
	ap->empty = (len == 0.);
	if (ap->empty) {
		clear_graph(iap);
		return;
	}

	/* only do silence detection on low (voice-ish) quality audio */
	if (ap->Detect_silence && (ap->tstate != Unloaded) && !ap->empty &&
	    (ap->rec_hdr.sample_rate <= 16000)) {
		AudPanel_Message(ap, MGET("Detecting silence..."));
		AudPanel_Busy(ap);

		/* find silent sections XXX what kind of errors happen here? */
		if (list_getdetect(ap->lp, &(ap->dp)) == AUDIO_SUCCESS) {
			/* Print_detect(ap); */
		} 
	} else {
		/* create detect points for one sound segment */
		onesound[0].pos = 0.0;
		onesound[0].type = DETECT_SOUND;
		onesound[1].pos = len;
		onesound[1].type = DETECT_EOF;
		ap->dp = onesound;
	}

	/* update display list*/
	SegCanvas_Updatelist(ap->subp.segment, ap->dp);

	/* update the pointer display */
	SegCanvas_Setpointer(ap->subp.segment, ap->insert);

	/* Display bar graph */
	draw_graph(ap);

	/* Display the audio format and update controls, etc */
	AudPanel_Showtypeinfo(ap);
	AudPanel_Changestate(ap);

	/* unbusy the tool (may have been busied prior to entry) */
	AudPanel_Unbusy(ap);
}

/* Draw the bar graph using current display list */
draw_graph(ptr_t iap)
	       				    	/* audio tool data */
{
	struct atool_panel_data		*ap;	/* audio data pointer */

	ap = (struct atool_panel_data *)iap;

	/* Display bar graph */
	SegCanvas_Displayfile(ap->subp.segment);

	/* update the file size */
	AudPanel_Displaysize(ap);
}


/*
 * Determine if the given file is playable on the audio device.
 * If not, the play/record buttons will be disabled and only editing allowed.
 */
int
AudPanel_Canplay(
	ptr_t			dp,
	Audio_hdr		*hdrp)
{
	struct atool_panel_data *ap = (struct atool_panel_data *)dp;

	/* If no audio device, allow load of reasonable files */
	if (ap->ctldev == NULL) {
		if ((hdrp->channels > 2) || (hdrp->channels < 1) ||
		    (hdrp->sample_rate > 48000) || (hdrp->sample_rate < 1000))
			return (FALSE);

		/* XXX - need an API routine to check for compressions */
		switch (hdrp->encoding) {
		case AUDIO_ENCODING_G721:
		case AUDIO_ENCODING_G723:
		default:
			return (FALSE);

		case AUDIO_ENCODING_ULAW:
		case AUDIO_ENCODING_ALAW:
		case AUDIO_ENCODING_LINEAR:
			return (TRUE);
		}
	}
	return (audio_checkformat(ap->ctldev, hdrp));
}
