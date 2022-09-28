/* Copyright (c) 1993 by Sun Microsystems, Inc. */
#ident	"@(#)atool.c	1.143	93/03/03 SMI"

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/signal.h>
#include <sys/types.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <stropts.h>
#include <string.h>
#include <math.h>

/* XXX - Workaround for bug 1119267 */
#ifndef SUNOS41
#include <sys/statvfs.h>
#endif

#include <multimedia/libaudio.h>
#include <multimedia/AudioDetect.h>

#include "atool_panel_impl.h"
#include "atool_i18n.h"
#include "atool_debug.h"
#include "loadsave/loadsave_panel.h"
#include "segment/segment_canvas.h"
#include "config/config_panel.h"


/* save the program name ... */
char *Progname;

static void	AudPanel_DisablePlayrec(ptr_t);

/*
 * Create an Audio Tool Panel
 */
ptr_t
AudPanel_Init(ptr_t owner, int *argcp, char **argv)
{
	struct atool_panel_data		*ap;
	int				i;	/* loop index */
	int				pflag = FALSE;	/* play loaded file? */
	char	 			**args;	/* scan ahead of argv */
	int				nargs;
	static char 			*envstr = "TMPDIR=";
	char				*devname = NULL;

	/*
	 * do first scan of arguments to find things we at this stage
	 * (i.e. device name). another pass is made in AudPanel_INIT()
	 * for xview related stuff (and xv_init()), and below to snarf
	 * up file names.
	 */
	for (args = argv, nargs = *argcp; nargs && args && *args; nargs--) {
		if (!strcmp(*args, "-d")) {
			devname = args[1];
			scrunch_args(argcp, argv, args, 2);
		} else {
			args++;
		}
	}

	/* Init the status panel window */
	if ((ap = (struct atool_panel_data *)calloc(1, sizeof (*ap))) == NULL)
		return (NULL);


	/* override user's TMPDIR setting so tempnam will work properly */
	putenv(envstr);

	/* save a ptr of the basename of argv[0] */
	if ((Progname = strrchr(argv[0], '/')) == NULL) {
		Progname = argv[0];
	} else {
		Progname++;
	}

	/*
	 * Open the control device and get its name.
	 * XXX - Only when linking audio panels in directly:
	 *       this must be done before AudPanel_INIT() is called so the
	 *       device name is known when initing the play/record ctl panels. 
	 */
	sigio_block(ap);
	if (audio_openctldev(devname, &ap->ctldev) == AUDIO_SUCCESS) {
		ap->devname = audio_getname(ap->ctldev);
	} else {
		ap->devname = devname;
	}

	ap->panel = AudPanel_INIT(owner, (ptr_t)ap, argcp, argv);
	sigio_unblock(ap);

	ap->owner = owner;
	ap->shelf = NULL;
	ap->modified = FALSE;
	ap->cfile[0] = NULL;

	ap->link_app[0] = NULL;
	ap->compose = FALSE;
#ifndef PRE_493
	ap->link_msg = NULL;
#endif

	/* something went wrong */
	if (ap->panel == NULL) {
		(void) free((ptr_t)ap);
		return (NULL);
	}

	/* initialize audio list pointer */
	list_init(&(ap->lp));

	/* init temp record file name, in case user default is invalid */
	ap->recpath = NULL;

	/* XXX - set audio header API should support header initialization */
	ap->rec_hdr.sample_rate = 8000;
	ap->rec_hdr.samples_per_unit = 1;
	ap->rec_hdr.bytes_per_unit = 1;
	ap->rec_hdr.channels = 1;
	ap->rec_hdr.encoding = AUDIO_ENCODING_ULAW;

	/* Initialize default Save format */
	memcpy((char*)&ap->save_hdr, (char*)&ap->rec_hdr, sizeof (Audio_hdr));

	/* init state variable */
	ap->pointer_cleared = FALSE;

	/* Load state from $HOME/.audiorc */
	ConfigPanel_SetDefaults(ap->subp.config);
	FormatPanel_Readdefaults(ap->subp.format);

	/* set format and file panels to init values */
	FilePanel_Setformat(ap->subp.file, &ap->rec_hdr, AUDIO_ENCODING_NONE);

	/* XXX - set defaults for things that are NOT customizable */
	SegCanvas_SETINCREMENTTIME(ap->subp.segment, 1.0);
	SegCanvas_Hashon(ap->subp.segment, 1);

	/* set tool state */
	ap->tstate = Unloaded;
	ap->Play_speed = NORMAL_SPEED;
	ap->insert = 0.0;
	ap->start = 0.0;
	ap->end = 0.0;
	ap->async_op = LS_None;
	ap->convpid = -1;

	/* Make sure the audio device exists */
	if (ap->ctldev == NULL) {
		char	buf[3 * MAXPATHLEN];

		(void) sprintf(buf, MGET(
		    "The audio device cannot be located.\n"
		    "Check that %s and %sctl\n"
		    "exist and are linked to valid device files.\n\n"
		    "You may continue now, but playing and recording "
		    "will be disabled."),
		    ap->devname ? ap->devname : "/dev/audio",
		    ap->devname ? ap->devname : "/dev/audio");

		if (AudPanel_Choicenotice(ap, buf,
		    MGET("Quit"), MGET("Continue"), NULL) == 1)
			exit(1);
		AudPanel_DisablePlayrec(ap);
	}

	/* set state and paint everything properly */
	clear_graph(ap);

        /* 
	 * Complete tool talk initialization. needs to be done here 
	 * after audiotool is mostly initialized
	 */

        if ((ap->tt_type == TT_V3compat) || (ap->tt_type == TT_Both)) {
		complete_tt_init(ap->panel);
	}

#ifndef PRE_493
        if ((ap->tt_type == TT_MediaExchange) || (ap->tt_type == TT_Both)) {
		atool_dstt_start(ap->panel);
	}
#endif

	/* scan for any remaining non-toolkit related arg's */
	for (args = argv, nargs = *argcp; nargs && args && *args; nargs--) {
		if (!strcmp(*args, "-p")) {
			pflag = TRUE;
			scrunch_args(argcp, argv, args, 1);
		} else {
			args++;
		}
	}

	if (*argcp == 2) {
		/* load a single file */
		(void) AudPanel_Loadfile(ap, argv[1], pflag);
	} else {
		/* insert multiple files, creating a new file with no name */
		for (i = 1; i < *argcp; i++) {
			/* insert audio files listed */
			AudPanel_Insertfile(ap, argv[i]);

			/* set insert position to end of file */
			ap->insert = list_getlength(ap->lp);
		}
		if (pflag) {
			AudPanel_Play(ap);
		}
	}
	return ((ptr_t) ap);
}

/* Return the File Panel window handle */
ptr_t
AudPanel_FilePanel_hndl(ptr_t atd)
{
	struct atool_panel_data		*ap;

	ap = (struct atool_panel_data *)atd;
	return (ap->subp.file);
}

/* Return the Format Panel window handle */
ptr_t
AudPanel_FormatPanel_hndl(ptr_t atd)
{
	struct atool_panel_data		*ap;

	ap = (struct atool_panel_data *)atd;
	return (ap->subp.format);
}

/* Return the Audio Tool Config Panel window handle */
ptr_t
AudPanel_ConfigPanel_hndl(ptr_t atd)
{
	struct atool_panel_data		*ap;

	ap = (struct atool_panel_data *)atd;
	return (ap->subp.config);
}


/* Return the Audio Tool Panel window handle */
ptr_t
AudPanel_Gethandle(ptr_t atd)
{
	struct atool_panel_data		*ap;

	ap = (struct atool_panel_data *)atd;
	return ((ptr_t)ap->panel);
}

/* Return the owner of the Audio Tool Panel */
ptr_t
AudPanel_Getowner(ptr_t atd)
{
	struct atool_panel_data		*ap;

	ap = (struct atool_panel_data *)atd;
	return ((ptr_t)ap->owner);
}

/* Pop up the Audio Tool Panel */
void
AudPanel_Show(ptr_t atd)
{
	struct atool_panel_data		*ap;

	ap = (struct atool_panel_data *)atd;

	/* Make sure the panel info is current */
	AudPanel_SHOW(ap->panel);
}

/* Dismiss the Audio Tool Panel */
void
AudPanel_Unshow(ptr_t atd)
{
	struct atool_panel_data		*ap;

	ap = (struct atool_panel_data *)atd;

	/* hide any panels that are up */
	/* XXX - workaround for XView bugid: 1100412 */
	ConfigPanel_Unshow(ap->subp.config);
	FilePanel_Unshow(ap->subp.file);
	FormatPanel_Unshow(ap->subp.format);

#ifdef PRE_493
	PlayctlPanel_Unshow(ap->subp.playctl);
	ReclevelPanel_Unshow(ap->subp.reclevel);
#endif

	AudPanel_UNSHOW(ap->panel);
}

/* Dismiss the Audio Tool Panel - clear the data first. */
void
AudPanel_Hide(ptr_t atd)
{
	struct atool_panel_data		*ap;

	ap = (struct atool_panel_data *)atd;

	/* force unlink if in compose mode */
	AudPanel_Unloadfile(ap, TRUE, &ap->rec_hdr);
	AudPanel_Unshow(ap);
}

/* really quit audiotool. if running tooltalk, just hide for a while. */
void
AudPanel_Quit(ptr_t atd)
{
	struct atool_panel_data		*ap;

	ap = (struct atool_panel_data *)atd;

	/* force a real quit */
	ap->tt_type = TT_None;
	ap->tt_started = FALSE;
	xv_destroy_safe((Xv_opaque)ap->panel);
}

/* Set Busy Status on Audio Tool panel */
void
AudPanel_Busy(ptr_t atd)
{
	struct atool_panel_data *ap = (struct atool_panel_data *)atd;

	AudPanel_BUSY(ap->panel);
}

/* Clear Busy Status on Audio Tool panel */
void
AudPanel_Unbusy(ptr_t atd)
{
	struct atool_panel_data *ap = (struct atool_panel_data *)atd;

	AudPanel_UNBUSY(ap->panel);
}

/* Disable play controls */
static void
AudPanel_DisablePlayrec(ptr_t atd)
{
	struct atool_panel_data *ap = (struct atool_panel_data *)atd;

	if (ap->noplay)		/* already disabled */
		return;
	ap->noplay = TRUE;
	AudPanel_ACTIVATEPLAYREC(ap->panel, FALSE);
	AudPanel_ACTIVATESEARCH(ap->panel, FALSE);
}

/* Enable play controls, only if the current format is playable */
static void
AudPanel_EnablePlayrec(ptr_t atd)
{
	struct atool_panel_data *ap = (struct atool_panel_data *)atd;

	if ((ap->ctldev == NULL) || !AudPanel_Canplay(atd, &ap->rec_hdr)) {
		AudPanel_DisablePlayrec(atd);
		return;
	}
	if (!ap->noplay)	/* already enabled */
		return;
	ap->noplay = FALSE;
	AudPanel_ACTIVATEPLAYREC(ap->panel, TRUE);
	AudPanel_ACTIVATESEARCH(ap->panel, TRUE);
}

/* Disable file/edit controls */
static void
AudPanel_DisableControls(ptr_t atd)
{
	struct atool_panel_data *ap = (struct atool_panel_data *)atd;

	if (ap->EditLock)
		return;
	ap->EditLock = TRUE;
	AudPanel_ACTIVATEBUTTONS(ap->panel, FALSE);
	FilePanel_Busy(ap->subp.file);
}

/* Enable file/edit controls */
static void
AudPanel_EnableControls(ptr_t atd)
{
	struct atool_panel_data *ap = (struct atool_panel_data *)atd;

	if (!ap->EditLock)
		return;
	ap->EditLock = FALSE;
	FilePanel_Unbusy(ap->subp.file);
	AudPanel_ACTIVATEBUTTONS(ap->panel, TRUE);
}

/* Set most of Audio Tool busy, but leave active for STOP */
void
AudPanel_AsyncBusy(ptr_t atd)
{
	struct atool_panel_data	*ap = (struct atool_panel_data *)atd;

	AudPanel_DisablePlayrec(atd);
	AudPanel_DisableControls(atd);
}

/* Restore state and enable controls */
void
AudPanel_AsyncUnbusy(ptr_t atd)
{
	struct atool_panel_data	*ap = (struct atool_panel_data *)atd;

	AudPanel_EnableControls(atd);
	AudPanel_EnablePlayrec(atd);
}

/* Ring bell to indicate an error occurred */
void
AudPanel_Errorbell(ptr_t atd)
{
	struct atool_panel_data	*ap = (struct atool_panel_data *)atd;

	AudPanel_ERRORBELL(ap->panel);
}

/* Show audiotool message (for state changes and errors) */
void
AudPanel_Errormessage(ptr_t atd, char *msg)
{
	struct atool_panel_data	*ap = (struct atool_panel_data *)atd;

	AudPanel_SETLEFTFOOTER(ap->panel, msg);
	AudPanel_Errorbell(ap);
}

/* Show audiotool message (for state changes and errors) */
void
AudPanel_Message(ptr_t atd, char *msg)
{
	struct atool_panel_data	*ap = (struct atool_panel_data *)atd;

	if (msg == NULL)
		AudPanel_Showtypeinfo(atd);
	else
		AudPanel_SETLEFTFOOTER(ap->panel, msg);
}

/* Display a popup with 1 choice. */
void
AudPanel_Alert(
	ptr_t		atd,
	char		*msg)
{
	struct atool_panel_data	*ap = (struct atool_panel_data *)atd;

	AudPanel_CHOICENOTICE(ap->panel, msg, MGET("Continue"), NULL, NULL);
}

/* Display a popup with up to 3 choices.  Returns user selection (1-3). */
int
AudPanel_Choicenotice(
	ptr_t		atd,
	char		*msg,
	char		*b1,
	char		*b2,
	char		*b3)
{
	struct atool_panel_data	*ap = (struct atool_panel_data *)atd;

	return (AudPanel_CHOICENOTICE(ap->panel, msg, b1, b2, b3));
}

/* 
 * Show audio encoding info in left footer of base frame
 */
void
AudPanel_Showtypeinfo(ptr_t atd)
{
	struct atool_panel_data	*ap = (struct atool_panel_data *)atd;

	AudPanel_SHOWTYPEINFO(ap->panel, 
	    FormatPanel_Getformatname(ap->subp.format, &ap->rec_hdr));
}

/*
 * Display size current file & selection. Calls AudPanel_Sizemsg with
 * current values....
 */
void
AudPanel_Displaysize(ptr_t atd)
{
	struct atool_panel_data		*ap;
	char				str[100];
	char				ftime[40], stime[40];

	ap = (struct atool_panel_data *)atd;

	if ((ap->tstate == Unloaded) || ap->empty) {
		/* If no data, size is zero */
		AudPanel_Sizemsg(ap, 0.0, 0.0);
		FilePanel_Setfilesize(ap->subp.file, 0.0);

	} else {
		/* If selection, put the selection length in brackets */
		AudPanel_Sizemsg(ap, list_getlength(ap->lp),
		    SegCanvas_Selection(ap->subp.segment) ?
		    (ap->end - ap->start) : 0.0);
		FilePanel_Setfilesize(ap->subp.file, list_getlength(ap->lp));
	}
}


/* Display size of file and selection in the right footer */
void
AudPanel_Sizemsg(ptr_t atd, double flength, double slength)
{
	struct atool_panel_data		*ap;
	char				str[100];
	char				ftime[40], stime[40];

	ap = (struct atool_panel_data *)atd;

	/*
	 * if file size is 0, or no file loaded, clear the msg
	 */
	if ((flength == 0.0) || (ap->tstate == Unloaded) ||
	    (ap->empty && (ap->tstate != Recording))) {
		AudPanel_SETRIGHTFOOTER(ap->panel,"");
	} else {
		if (slength > 0.0) {
			sprintf(str, ap->Lengthsel_string,
				audio_secs_to_str(flength, ftime, 1),
				audio_secs_to_str(slength, stime, 1));
		} else {
			/* a lousy guess at how many spaces to pad ... */
			sprintf(str, ap->Length_string,
				audio_secs_to_str(flength, ftime, 1));
		}
		AudPanel_SETRIGHTFOOTER(ap->panel, str);
	}
}

/* Display current file name in the frame label, with the modified flag */
void
AudPanel_Namestripe(ptr_t atd)
{
	struct atool_panel_data	*ap;
	char			msg[BUFSIZ];
	char			from[MAXPATHLEN+BUFSIZ];

	/* the rest of the info */
	ap = (struct atool_panel_data *)atd;
	if ((ap->link_app[0] == NULL) &&
	    (*ap->cfile == NULL) && (ap->tstate == Unloaded)) {
		(void) sprintf(msg, "%s %s",
		    MGET("Audio Tool"), audiotool_release());
	} else {
		/* Set link app string separately for i18n */
		from[0] = '\0';
		if (ap->link_app[0])
			(void) sprintf(from, MGET(" From %s "), ap->link_app);

		(void) sprintf(msg, "%s %s - %s%s%s",
		    MGET("Audio Tool"),
		    audiotool_release(),       
		    *ap->cfile ? ap->cfile : MGET("Untitled"),
		    from,
		    ap->modified ? MGET("[Modified]") : "");
	}
	AudPanel_SETLABEL(ap->panel, msg);

	AudPanel_SETICONLABEL(ap->panel, 
	    *ap->cfile ? ap->cfile : ((ap->tstate == Unloaded) ? 
	    MGET("Audio Tool") : MGET("Untitled")));
}

/*
 * sets name of application audiotool is linked to. if a link name
 * is specified, create the include button, otherwise destroy it.
 * if going in to "compose" mode (cflag set), make sure all data
 * is cleared and proper state is set....
 */
void
AudPanel_Setlink(ptr_t atd, char *name, int cflag)
{
	struct atool_panel_data *ap;

	ap = (struct atool_panel_data*)atd;

	if (!(name && *name))	/* must have an app name to link ... */
		return;

	/* if going in to compose mode, make sure all data is cleared */
	if (cflag) {
		AudPanel_Unloadfile(ap, TRUE, &ap->rec_hdr);
		ap->compose = TRUE;
	}
	/* do this after AudPanel_Unloadfile (or else it's a NOOP!) */
	strncpy(ap->link_app, name, MAXPATHLEN);

	AudPanel_Changestate(ap);
}

/* Returns the name of the link application, or NULL if not linked */
char*
AudPanel_Getlink(ptr_t atd)
{
	struct atool_panel_data *ap = (struct atool_panel_data*)atd;

	if (ap->link_app[0] != '\0')
		return (ap->link_app);
	return (NULL);
}

/* Breaks link to application created by DnD LOAD (if one exists) */
void
AudPanel_Breaklink(ptr_t atd)
{
	struct atool_panel_data *ap = (struct atool_panel_data*)atd;

	/* only break link if one is established */
	if (ap->link_app[0]) {
		ap->link_app[0] = NULL;
		/* no more file name if we break the link (?) */
		ap->compose = FALSE;
		AudPanel_DESTROY_DONE_BUTTON(ap->panel);

		/*
		 * notify link app that link has been broken (convert 
		 * SELN_END on load atom).
		 */
		discard_load(ap->panel);
#ifndef PRE_493
		gtt_Breaklink(ap);
#endif

		AudPanel_Changestate(ap);
	}
}

/* return TRUE if iconified */
int
AudPanel_Isicon(ptr_t atd)
{
	struct atool_panel_data		*ap;

	ap = (struct atool_panel_data *)atd;
	return (AudPanel_ISICON(ap->panel));
}

/* return TRUE if we have modified data */
int
AudPanel_Ismodified(ptr_t atd)
{
	struct atool_panel_data *ap;

	ap = (struct atool_panel_data*)atd;

	return (ap->modified);
}

/* return TRUE if we're in compose mode */
int
AudPanel_Iscompose(ptr_t atd)
{
	struct atool_panel_data *ap;

	ap = (struct atool_panel_data*)atd;
	return (ap->compose);
}

/*
 * set file to be unmodified.  Update name stripe, if neccessary.
 * Return TRUE if change was made.
 */
int
AudPanel_Unmodify(
	struct atool_panel_data	*ap)
{
	int			change;

	change = ap->modified;
	if (change) {
		ap->modified = FALSE;
	}
	AudPanel_Changestate(ap);
	return (change);
}

/*
 * set file to be modified.  Update name stripe, if neccessary
 * Return TRUE if change was made.
 */
int
AudPanel_Modify(
	struct atool_panel_data	*ap)
{
	int			change;

	change = !ap->modified;
	if (change) {
		ap->modified = TRUE;
	}
	AudPanel_Changestate(ap);
	return (change);
}

/* called when insert_selection is successfull */
void
AudPanel_Finishsaveback(
	ptr_t			atd,
	int			cflag)
{
	struct atool_panel_data *ap = (struct atool_panel_data*)atd;

	/*
	 * In all cases, data becomes unmodified.
	 * If cflag is set, destroy done button, break link, and clear data.
	 */
	AudPanel_Message(ap, MGET("Save complete"));
	AudPanel_RESET_DONE_BUTTON(ap->panel);
	(void) AudPanel_Unmodify(ap);

	if (cflag) {
		/* force unlink */
		AudPanel_Unloadfile(ap, TRUE, &ap->rec_hdr);
		AudPanel_DESTROY_DONE_BUTTON(ap->panel);
		/* XXX - app should send TT hide msg -- not for V3. */
		AudPanel_Unshow(ap);
	}
}

/* 
 * cleanup before exit (flush audio buffer, etc.). if cflag is TRUE,
 * ask for confirmation before cleaning up. returns FALSE if cleanup
 * is cancelled by user, TRUE otherwise.
 */
int
AudPanel_Cleanup(
	ptr_t			atd,
	int			cflag)
{
	struct atool_panel_data *ap;

	ap = (struct atool_panel_data *)atd;
	if (!cflag) {
		/* Set to unmodified so we don't get prompted */
		AudPanel_Unmodify(ap);
	}

	/* This will stop play/record, unload data, and break links */
	if (!AudPanel_Unloadfile(ap, TRUE, NULL))
		return (FALSE);

	/* Release clipboard, free the shelf */
	Audio_RELEASE_CLIPBOARD(ap->panel);

	return (TRUE);
}

/*
 * examines various objects to determine state of tool and update
 * labels and menus accordingly. call this after edits are made,
 * selections are made, etc.
 */
void
AudPanel_Changestate(ptr_t atd)
{
	struct atool_panel_data *ap;

	ap = (struct atool_panel_data *)atd;

	/* first, update title bar */
	AudPanel_Namestripe(ap);

	if ((ap->tstate == Unloaded) || (ap->tstate == Stopped))
	    AudPanel_SETLEVEL(ap->panel, 0.0, FALSE); /* reset level meter */

	if ((ap->tstate == Unloaded) ||
	    (ap->empty && (ap->tstate != Recording)))
		AudPanel_POINTERLABEL(ap, "");
	else
		AudPanel_POINTERLABEL(ap, ap->Pointer_string);
		
	/* set file menu apropriately */
	AudPanel_SETFILEMENU(ap->panel, 
	     (!ap->empty && ap->modified),		/* save */
	     !ap->empty,				/* save as */
	     !ap->compose);				/* can load */

	AudPanel_SETEDITMENU(ap->panel, 
			     SegCanvas_Selection(ap->subp.segment),
			     (int) AudPanel_Getshelf(ap),
			     AudPanel_Canundo(ap), 
			     AudPanel_Canredo(ap));

	Audio_SetDropsiteFull(ap->panel, !ap->empty);
	AudPanel_Displaysize(ap);
}	

/* These are the callbacks for the config panel */
void
Config_autoplay_load_proc(ptr_t iap, int val)
{
	struct atool_panel_data	*ap;	/* audio panel data  */

	ap = AudPanel_KEYDATA(ConfigPanel_Getowner(iap));
	ap->Autoplay_load = val;
}

void
Config_autoplay_sel_proc(ptr_t iap, int val)
{
	struct atool_panel_data	*ap;	/* audio panel data  */

	ap = AudPanel_KEYDATA(ConfigPanel_Getowner(iap));
	ap->Autoplay_select = val;
}

void
Config_confirm_proc(ptr_t iap, int val)
{
	struct atool_panel_data	*ap;	/* audio panel data  */

	ap = AudPanel_KEYDATA(ConfigPanel_Getowner(iap));
	ap->Confirm_clear = val;
}

void
Config_apply_proc(ptr_t iap)
{
	struct atool_panel_data	*ap;	/* audio panel data  */

	ap = AudPanel_KEYDATA(ConfigPanel_Getowner(iap));
	update_graph(ap, 0.0, AUDIO_UNKNOWN_TIME);
}

/* turn on/off silence detection */
void
Config_silence_proc(ptr_t iap, int val)
{
	struct atool_panel_data	*ap;	/* audio panel data  */

	ap = AudPanel_KEYDATA(ConfigPanel_Getowner(iap));
	ap->Detect_silence = val;
}

/* set threshold values for silence detection algorithm */
void
Config_threshold_proc(ptr_t iap, 
	double min_silence, 
	double min_sound,
	double thresh_scale,
	double noise_ratio)
{
	struct atool_panel_data	*ap; /* audio panel data  */

	ap = AudPanel_KEYDATA(ConfigPanel_Getowner(iap));
	DBGOUT((1, "setting thresholds: %.2f, %.2f, %.2f, %.2f\n",
		min_silence, min_sound, thresh_scale, noise_ratio));
	list_configdetect(ap->lp, DETECT_MINIMUM_SILENCE, min_silence);
	list_configdetect(ap->lp, DETECT_MINIMUM_SOUND, min_sound);
	list_configdetect(ap->lp, DETECT_THRESHOLD_SCALE, thresh_scale);
	list_configdetect(ap->lp, DETECT_NOISE_RATIO, noise_ratio);
}

/* XXX - Workaround for bug 1119267: check if a directory can be written */
int
access_dir(
	char			*dir)
{
	int			f;
	char			buf[2 * MAXPATHLEN];
#ifndef SUNOS41
	struct statvfs	fst;

	if ((statvfs(dir, &fst) < 0) || (fst.f_flag & ST_RDONLY))
		return (-1);
#endif /* SVR4 */

	if (access(dir, W_OK) < 0)
		return (-1);

	/* XXX - try to create a dummy file */
	sprintf(buf, "%s/..TmP..", dir);
	if ((f = open(buf, O_RDWR | O_CREAT, 0666)) >= 0) {
		close(f);
		unlink(buf);
		return (0);
	}
	return (-1);
}


/* set temp dir for record */
void
Config_tempdir_proc(
	ptr_t			iap,
	char			*dir)
{
	struct stat		st;
	char 			sbuf[BUFSIZ]; /* for msg */
	struct atool_panel_data	*ap;

	ap = AudPanel_KEYDATA(ConfigPanel_Getowner(iap));

	/* Validate the specified directory */
	if ((stat(dir, &st) < 0) || !S_ISDIR(st.st_mode) ||
	    (access_dir(dir) < 0)) {

		/* It's no good: see if the previous one will work */
		if ((ap->recpath == NULL) || (stat(ap->recpath, &st) < 0) ||
		    !S_ISDIR(st.st_mode) || (access_dir(ap->recpath) < 0)) {

			/* Nope: try /tmp explicitly */
			if (ap->recpath != NULL)
				(void) free(ap->recpath);
			ap->recpath = strdup(DEF_TEMP_DIR);
		}

		if ((stat(ap->recpath, &st) < 0) || !S_ISDIR(st.st_mode) ||
		    (access_dir(ap->recpath) < 0)) {

			sprintf(sbuf, MGET(
			    "The specified Temp file directory "
			    "is unwriteable\n"
			    "and no valid alternative was found.\n"
			    "Please reset this field, or make the %s "
			    "directory writeable."),
			    ap->recpath);
			AudPanel_Alert(ap, sbuf);
		} else {
			sprintf(sbuf, MGET(
			    "The specified Temp file directory "
			    "is unwriteable.\n"
			    "%s will be used instead."),
			    ap->recpath);
			AudPanel_Alert(ap, sbuf);
			ConfigPanel_SetTempdir(iap, ap->recpath);
		}
	} else {
		DBGOUT((1, "set tmp dir to %s\n", dir));
		if (ap->recpath != NULL)
			(void) free(ap->recpath);
		ap->recpath = strdup(dir);
	}
}

AudPanel_Setdefault(ptr_t win, char *name, ptr_t val)
{
	struct atool_panel_data	*ap;	/* audio panel data  */

	ap = AudPanel_KEYDATA(win);
	ConfigPanel_Setdefault(ap->subp.config, name, val);
}

ptr_t
AudPanel_Getdefault(ptr_t win, char *name)
{
	struct atool_panel_data	*ap;	/* audio panel data  */

	ap = AudPanel_KEYDATA(win);
	return ((ptr_t)ConfigPanel_Getdefault(ap->subp.config, name));
}

AudPanel_Writedefaults(ptr_t win)
{
	struct atool_panel_data	*ap;	/* audio panel data  */

	ap = AudPanel_KEYDATA(win);
	ConfigPanel_Writedefaults(ap->subp.config);
}
