/* Copyright (c) 1993 by Sun Microsystems, Inc. */
#ident	"@(#)atool_file.c	1.65	93/03/03 SMI"

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <memory.h>
#include <sys/types.h>
#include <sys/file.h>
#include <sys/fcntl.h>
#include <sys/stat.h>
#include <stropts.h>
#include <math.h>

#include <multimedia/libaudio.h>

#include "atool_panel_impl.h"
#include "atool_i18n.h"
#include "atool_debug.h"
#include "loadsave/loadsave_panel.h"
#include "segment/segment_canvas.h"

long		save_timer = 25000;	/* usec for load/save timer */

/*
 * Audiotool file routines
 */

/*
 * Get the filename component of a path.
 * Note that this returns a pointer to someplace in the given string.
 */
static char*
path2file(
	char			*path)
{
	char			*file;

	if (path == NULL)		/* paranoid check */
		return ("");

	/* find start of file name, by looking for last slash */
	file = strrchr(path, '/');
	if (file++ == NULL) {
		file = path;
	}
	return (file);
}

/*
 * Return Audio Panel's current pathname in fpath.
 */
void
AudPanel_Getpath(
	ptr_t			iap,	/* audio tool data */
	char			*fpath,	/* file path name */
	int			len)	/* length of buffer */
{
	struct atool_panel_data	*ap = (struct atool_panel_data *)iap;

	if (ap->cpath)
		(void) strncpy(fpath, ap->cpath, len);
	else
		*fpath = '\0';
}

/*
 * Set the Audio Panel's current path and file name.
 * The path will be used for default saves.
 * The name will be the filename string displayed in the panel label.
 */
void
AudPanel_Setpath(
	ptr_t			iap,
	char			*path)
{
	struct atool_panel_data	*ap = (struct atool_panel_data *)iap;

	if (path != NULL) {
		/* save names */
		(void) strncpy(ap->cpath, path, MAXPATHLEN);
		(void) strncpy(ap->cfile, path2file(path), MAXPATHLEN);
	} else {
		ap->cpath[0] = NULL;
		ap->cfile[0] = NULL;
	}
}

/* Set the pathname of the file being saved */
static void
set_savepath(
	ptr_t			iap,
	char			*path)
{
	struct atool_panel_data	*ap = (struct atool_panel_data *)iap;

	(void) strncpy(ap->savepath, path, MAXPATHLEN);
}

/* Set the pathname of the audioconvert working file */
static void
set_tmppath(
	ptr_t			iap,
	char			*path)
{
	struct atool_panel_data	*ap = (struct atool_panel_data *)iap;

	(void) strncpy(ap->tmppath, path, MAXPATHLEN);
}

/* Save enough current info to restore the current state on load/save error */
static void
store_prev(
	ptr_t		iap)
{
	struct atool_panel_data	*ap = (struct atool_panel_data *)iap;

	/* Save current filename, modify flag, format (in case of error) */
	(void) strncpy(ap->prevpath, ap->cpath, MAXPATHLEN);
	ap->prevmod = ap->modified;

	memcpy((void*)&(ap->prevhdr), (void*)&(ap->save_hdr),
	    sizeof (Audio_hdr));
}

/* Clear out save info */
clear_prev(
	ptr_t		iap)
{
	struct atool_panel_data	*ap = (struct atool_panel_data *)iap;

	/* Just to be on the safe side, try to unlink the tmpfile */
	if (ap->tmppath[0] != '\0') {
		(void) unlink(ap->tmppath);
		ap->tmppath[0] = '\0';
	}
	ap->savepath[0] = '\0';
	ap->prevpath[0] = '\0';

	AudPanel_STOP_TIMER(ap);
	ap->tstate = Stopped;
	ap->async_op = LS_None;

	/* Unbusy the tool, but leave play controls disabled if not playable */
	AudPanel_AsyncUnbusy(ap);
	AudPanel_Unbusy(ap);
}

/* Restore saved info */
static void
restore_prev(
	ptr_t		iap)
{
	struct atool_panel_data	*ap = (struct atool_panel_data *)iap;

	/* Restore name and modified flag, updating the namestripe */
	AudPanel_Setpath(ap, ap->prevpath);
	if (ap->prevmod) {
		(void) AudPanel_Modify(ap);
	} else {
		(void) AudPanel_Unmodify(ap);
	}
	memcpy((void*)&(ap->save_hdr), (void*)&(ap->prevhdr),
	    sizeof (Audio_hdr));
	clear_prev(ap);
}

/* Take down the operation panel if unpinned */
static void
panel_takedown(
	ptr_t		iap)
{
	struct atool_panel_data	*ap = (struct atool_panel_data *)iap;

	if (ap->async_op != LS_None) {
		FilePanel_Takedown(AudPanel_FilePanel_hndl(ap), ap->async_op);
		ap->async_op = LS_None;
	}
}

/* Return TRUE if the specified path is an audio file; fill in the header */
int
atool_isaudiofile(
	char		*path,
	Audio_hdr	*hdrp)
{
	Audio_hdr	hdr;
	int		fd;
	int		err;
	unsigned char	buf[sizeof (Audio_filehdr)];
	unsigned	isize;

	/* Open the file (set O_NONBLOCK in case the name refers to a device) */
	fd = open(path, O_RDONLY | O_NONBLOCK);
	if (fd < 0) {
		return (FALSE);
	}

	/* Read the header (but not the text info) */
	err = read(fd, (char *)buf, sizeof (buf));
	(void) close(fd);

	if (err != sizeof (buf))
		return (FALSE);		/* catch short files ... */

	if (audio_decode_filehdr(buf, &hdr, &isize) != AUDIO_SUCCESS)
		return (FALSE);

	/* Return the file header, if a pointer was supplied */
	if (hdrp) {
		memcpy((char*)hdrp, (char*)&hdr, sizeof (Audio_hdr));
	}
	return (TRUE);
}

/*
 * Verify that we have a valid file path to read from.
 * Check to see if file exists, and there is read permission.
 *
 * Return:	TRUE, if directory and file are ok to write.
 */
int
verify_readfile(
	ptr_t			iap,
	char			*apath,
	Audio_hdr		*hdrp)
{
	struct atool_panel_data	*ap;		/* audio data pointer */
	int			err;		/* error code */
	struct stat		fstat;		/* file stats */ 
	int			encoding;

	ap = (struct atool_panel_data *)iap;

	/* check if current path exists */
	if ((err = stat(apath, &fstat)) < 0) {
		AudPanel_Errormessage(ap, MGET("File not found"));
		return (FALSE);

	} else if (S_ISDIR(fstat.st_mode)) {
		AudPanel_Errormessage(ap, MGET("Filename not specified"));
		return (FALSE);

	} else if (access(apath, R_OK) < 0) {
		/* do not have read permission */
		AudPanel_Errormessage(ap, MGET("File read access denied"));
		return (FALSE);

	} else if (!S_ISREG(fstat.st_mode) ||
	    (!atool_isaudiofile(apath, hdrp)) ) {
		/* file exists, but not an audio file */
		AudPanel_Errormessage(ap, MGET("Not an audio file"));
		return (FALSE);
	}
	/* Return the encoding in *hdrp so we know what to save it back as */
	return (TRUE);
}

/*
 * Verify that we have a valid directory.
 * Check to see if file exists.  If so, check write permission, and
 * post a warning alert that we're overwriting a file, if alert is TRUE.
 *
 * Return:	TRUE, if the file is ok to rewrite.
 */
verify_writefile(
	ptr_t			iap,
	char			*apath,
	int			alert)
{
	struct atool_panel_data	*ap;		/* audio data pointer */
	int			err;		/* error code */
	char			msg[MAXPATHLEN+BUFSIZ];
	struct stat		fstat;		/* file stats */ 

	ap = (struct atool_panel_data *)iap;

	/* check if current path exists */
	if (stat(apath, &fstat) < 0) {
		return (TRUE);

	} else if (S_ISDIR(fstat.st_mode)) {
		/* directory specified */
		AudPanel_Errormessage(ap, MGET("Filename not specified"));
		return (FALSE);

	/* If readonly, the File Chooser already asked about going ahead */
#ifdef PRE_493
	} else if (access(apath, W_OK) < 0) {
		/* do not have write permission */
		AudPanel_Errormessage(ap, MGET("File write access denied"));
		return (FALSE);
#endif
	}

#ifndef PRE_493
	return (TRUE);

#else
	/* XXX - not needed w/S493 file_chooser */
	if (S_ISREG(fstat.st_mode) && audio_isaudiofile(apath)) {
		if (alert) {
			sprintf(msg, MGET(
			    "A file named \"%s\" already exists.\n"
			    "Do you want to overwrite the existing file?"),
			    ap->cfile);
			/* Display popup below */
		} else {
			/* expert mode, do you feel lucky */
			return (TRUE);
		}

	} else {
		/* file exists, but not a regular file */
		sprintf(msg, MGET("\"%s\" is not an audio file.\n"
		    "Do you want to overwrite the existing file?"),
		    ap->cfile);
		/* Display popup below */
	}
	return (AudPanel_Choicenotice(ap, msg,
	    MGET("Cancel"),
	    MGET("Overwrite existing file"), NULL) == 2);
#endif
}

/*
 * Verify that we an empty or non-existant file that can be written to.
 * In this case, audiotool will use the filename but load no data.
 * When the file is saved it'll be stored in this file.
 *
 * Return:	TRUE, if file is empty (or non-existent) & ok to write.
 */
int
verify_emptyfile(
	ptr_t			iap,
	char			*path)
{
	struct atool_panel_data	*ap;		/* audio data pointer */
	struct stat		st;
	int			err;
	char			dir[MAXPATHLEN+1];
	char			*cp;

	ap = (struct atool_panel_data *)iap;

	if ((err = stat(path, &st)) < 0) {
		if (errno == ENOENT) {
			/* file doesn't exist, make sure we can create it */
			strncpy(dir, path, MAXPATHLEN);
			if (cp = strrchr(dir, '/')) {
				*cp = NULL;
				if (access(dir, W_OK) == 0) {
					DBGOUT((1, "no file dir %s writable\n",
					    dir));
					return (TRUE);
				}
			} else { /* no dir part, check "." */
				if (access(".", W_OK) == 0) {
					DBGOUT((1, "no file '.' writable\n"));
					return (TRUE);
				}
			}
			/* no access, return FALSE */
			DBGOUT((1, "no file, can't write to dir %s\n",
				cp ? dir : "."));
			return (FALSE);
		} else {
			/* another stat error, return FALSE */
			DBGOUT((1, "stat error %d on file %s\n",
				errno, path));
			return (FALSE);
		}
	}

	/* it exists. see if it's 0 length & writable */
	if (S_ISREG(st.st_mode) && (st.st_size == 0) && 
	    (access(path, W_OK) == 0)) {
		DBGOUT((1, "0 length file %s\n", path));
		return (TRUE);
	}
	return (FALSE);
}

/*
 * Verify that the specified directory filesystem has enough space
 * available to store the given filesize in the specified format.
 * Make sure the directory is writeable first!
 * Return FALSE if not.
 */
int
verify_freespace(
	char		*dir,
	double		len,
	Audio_hdr	*hdrp)
{
	long		kbytes;
	long		space;

	kbytes = audio_getsize(len, hdrp);
	if (kbytes < 0)
		return (TRUE);		/* we got bigger problems */
	space = getfreespace(dir);
	if (space <= kbytes)		/* if marginal, play it safe */
		return (FALSE);
	return (TRUE);
}

/*
 * Called before unloading data to make sure it is ok to do so.
 * Returns FALSE if unload is cancelled.
 */
int
AudPanel_VerifyUnload(
	ptr_t			iap)
{
	struct atool_panel_data	*ap;
	char buf[MAXPATHLEN+BUFSIZ];

	ap = (struct atool_panel_data *)iap;

	/* Don't bother confirming if expert mode or unmodified */
	if (ap->Confirm_clear && ap->modified) {
		sprintf(buf, MGET(
		    "%s%s%s is modified.  You may:\n"
		    "Continue, discarding the edited data, or\n"
		    "Cancel this operation."),
		    ap->cfile[0] != '\0' ? MGET("\"") : MGET("The audio data"),
		    ap->cfile[0] != '\0' ? ap->cfile : "",
		    ap->cfile[0] != '\0' ? MGET("\"") : "");

		return (AudPanel_Choicenotice(ap, buf,
		    MGET("Continue"), MGET("Cancel"), NULL) == 1);
	}
	return (TRUE);
}

/*
 * Unload current data from the tool.
 * If modified data, request user confirmation (unless expert mode).
 * If force flag is TRUE, break any tooltalk links.
 * If newhdr is not null, set the new format.
 * Returns FALSE if cancelled by user confirmation.
 */
AudPanel_Unloadfile(
	ptr_t			iap,
	int			force,
	Audio_hdr		*newhdr)
{
	struct atool_panel_data	*ap = (struct atool_panel_data *)iap;

	AudPanel_Stop(ap);

	/* check for existing edits */
	if (!AudPanel_VerifyUnload(ap))
		return (FALSE);

	/* reset the undo list */
	list_destroy(ap->lp);
	list_init(&(ap->lp));

	/* Update display */
	clear_graph(ap);

	/*
	 * don't change to unloaded state if we're in compose mode. this
	 * way save back's & "Done" will work.
	 */
	if ((ap->compose == FALSE) || force) {
		ap->tstate = Unloaded;
		ap->cpath[0] = NULL;
		ap->cfile[0] = NULL;
		ap->compression = AUDIO_ENCODING_NONE; /* no compression */

		/* break DnD/Seln/Tooltalk link */
		(void) AudPanel_Breaklink(ap);
	}
	AudPanel_AsyncUnbusy(ap);

	/* If a non-null audio header was supplied, set the data format */
	if (newhdr != NULL) {
		if (newhdr != &ap->rec_hdr) {
			memcpy((void*)&ap->rec_hdr, (void*)newhdr,
			    sizeof (Audio_hdr));
		}
		/* Try to set the device format, too */
		if (ap->ctldev != NULL)
			(void) audio_setformat(ap->ctldev, &ap->rec_hdr);

		/* Set the default save format */
		FilePanel_Setformat(ap->subp.file,
		    &ap->rec_hdr, ap->compression);

		/* Set the format in the footer */
		AudPanel_Message(ap, NULL);
	} else {
		/* If no format supplied, explicitly clear the footer */
		AudPanel_Message(ap, "");
	}

	/* AudPanel_Unmodify calls AudPanel_Changestate() to update panel */
	(void) AudPanel_Unmodify(ap);
	return (TRUE);
}

/*
 * This is what actually makes the call to open a file and set it
 * into the undolist.  It is called after all file conversions are complete.
 * Returns TRUE if success.
 */
int
do_loadfile(
	struct atool_panel_data	*ap,
	char			*fpath,
	int			reload)
{
	Audio_hdr		hdr;
	int			err;		/* audio error */
	char			msg[MAXPATHLEN+BUFSIZ];
	Undolist		newlp;

	/* Set a reasonable message */
	sprintf(msg, "%s %s...", MGET("Loading"), ap->cfile);
	AudPanel_Message(ap, msg);
	AudPanel_Busy(ap);
	ap->tstate = Loading;

	/* Open and create an edit list for the file */
	list_init(&newlp);
	if ((err = list_newfile(newlp, fpath)) != AUDIO_SUCCESS) {
		switch (err) {
		case AUDIO_ERR_BADHDR:
		case AUDIO_ERR_BADFILEHDR:
		case AUDIO_ERR_ENCODING:
			AudPanel_Errormessage(ap, MGET("Not an audio file"));
			break;
		case AUDIO_UNIXERROR:
		default:
			AudPanel_Errormessage(ap, MGET("Error reading file"));
			break;
		}
		list_destroy(newlp);

		/* Error loading file....restore the original name */
		restore_prev(ap);
		return (FALSE);
	}

	/* Throw away the old undo list and set the one */
	list_destroy(ap->lp);
	ap->lp = newlp;

#ifdef notdef
	/*
	 * Break any existing links.
	 * Don't do this when re-loading a file.
	 * There usually wouldn't be one to break,
	 * but if there ever is, we don't want to break it anyway.
	 */
	if (!reload)
	/* XXX - for now, Save/SaveAs always breaks the link */
#endif
	{
		AudPanel_Breaklink(ap);

		/* Set save panel default directory */
		if (ap->cpath[0] != '\0')
			FilePanel_DefaultSavepath(AudPanel_FilePanel_hndl(ap),
			    ap->cpath);
	}

	/* Just in case the file was converted on the list_newfile() call */
	list_getfilehdr(ap->lp, 0.0, AUDIO_UNKNOWN_TIME, &(ap->rec_hdr));

	/* Try to set the device format to match the file */
	if (ap->ctldev != NULL)
		(void) audio_setformat(ap->ctldev, &ap->rec_hdr);

	/*
	 * If the file has a title, it did not convert substantially.
	 * In this case, set the default saveback format.
	 * Otherwise, set the default format to whatever we converted to.
	 */
	FilePanel_Setformat(ap->subp.file, &ap->rec_hdr,
	    ap->cpath[0] != '\0' ? ap->save_hdr.encoding : AUDIO_ENCODING_NONE);

	/* Update state and take down the initating panel if unpinned */
	ap->modified = FALSE;
	panel_takedown(ap);
	clear_prev(ap);

	/* Redraw graph and unbusy the tool */
	update_graph(ap, 0.0, AUDIO_UNKNOWN_TIME);
	if (!ap->empty) {
		AudPanel_Setinsert(ap, 0.0);
	}

	if (ap->loadplay) {
		AudPanel_Play(ap);
	}
	return (TRUE);
}

/* Clean up after a load */
void
finish_loadfile(
	ptr_t			apd,
	int			err)
{
	struct atool_panel_data	*ap = (struct atool_panel_data *)apd;

	/* noop if already called */
	if (ap->tmppath[0] == '\0')
		return;

	if (ap->convpid != -1)
		ap->convpid = -1;

	if (err != AUDIO_SUCCESS) {
		if (err == AUDIO_ERR_NOEFFECT) {
			AudPanel_Message(ap, MGET("Load cancelled"));
		} else {
			AudPanel_Errormessage(ap,
			    MGET("Error converting file"));
		}
		restore_prev(ap);
	} else {
		(void) do_loadfile(ap, ap->tmppath, TRUE);
	}
	/* unlink the tmp file */
	(void) unlink(ap->tmppath);
	ap->tmppath[0] = '\0';
}

/* Called when load is finished */
void
finish_convert_load(
	ptr_t			apd,
	int			status)
{
	struct atool_panel_data	*ap = (struct atool_panel_data *)apd;

	finish_loadfile(apd, status == 0 ? AUDIO_SUCCESS : AUDIO_UNIXERROR);
}

/* Find a filesystem with sufficient space, and return a tmpfile name */
static char*
find_tempdir(
	struct atool_panel_data	*ap,
	char			*fpath,		/* target file or NULL */
	Audio_hdr		*tohdr,
	double			len)
{
	char			sdir[MAXPATHLEN+BUFSIZ];
	char			*cp;

	/*
	 * Look for a writeable directory with enough space for the file.
	 * Try:  tempfiledir : /tmp : . : $HOME
	 */
	strncpy(sdir, ap->recpath, MAXPATHLEN);
	if ((access_dir(sdir) < 0) || !verify_freespace(sdir, len, tohdr)) {
		strncpy(sdir, DEF_TEMP_DIR, MAXPATHLEN);
	}
	if ((access_dir(sdir) < 0) || !verify_freespace(sdir, len, tohdr)) {
		if (fpath != NULL) {
			strncpy(sdir, fpath, MAXPATHLEN);
			cp = path2file(fpath);
			if (cp == fpath)
				strcpy(sdir, ".");	/* no dir, use "." */
			else
				sdir[cp - fpath - 1] = '\0';	/* get dir */
		}
	}
	if ((access_dir(sdir) < 0) || !verify_freespace(sdir, len, tohdr)) {
		strcpy(sdir, ".");
	}
	if ((access_dir(sdir) < 0) || !verify_freespace(sdir, len, tohdr)) {
		cp = getenv("HOME");
		if (cp != NULL)
			strcpy(sdir, cp);
	}
	if ((access_dir(sdir) < 0) || !verify_freespace(sdir, len, tohdr)) {
		return (NULL);
	}
	return (tempnam(sdir, "audio"));
}

/* Initiate a file conversion for loading */
int
convert_and_load(
	struct atool_panel_data	*ap,
	char			*fpath,
	Audio_hdr		*tohdr,
	double			len,
	void			(*callback)())
{
	char			buf[MAXPATHLEN*3];
	char			sdir[MAXPATHLEN+BUFSIZ];
	char			*dirpath;
	char			*tpath;
	char			*cp;
	int			pid;

	/* Look for a writeable directory with enough space for the file */
	tpath = find_tempdir(ap, fpath, tohdr, len);
	if (tpath == NULL) {
		AudPanel_Alert(ap, MGET(
		    "There is not enough space in the Temp file directory to\n"
		    "perform the format conversion required "
		    "to load this file,\n"
		    "and an alternate directory could not be located.\n"
		    "You may either delete files to recover storage space,\n"
		    "or edit the Properties to set the Temp file directory\n"
		    "to an alternate filesystem."));
		goto error;
	}

	/* Store the name of the tmp file being loaded */
	set_tmppath(ap, tpath);
	(void) free(tpath);

	/* Find index to start of file name */
	cp = path2file(fpath);

	sprintf(buf, MGET("%s %s...  [<STOP> to cancel]"), ap->busymsg, cp);
	AudPanel_Message(ap, buf);

	sprintf(buf, "audioconvert -f '%s' -o %s %s",
	    audio_printhdr(tohdr), ap->tmppath, fpath);
	if (atool_exec(buf, callback, ap, &pid) == -1) {
		AudPanel_Errormessage(ap, MGET("Cannot convert file"));
		goto error;
	}
	ap->convpid = pid;
	ap->tstate = Loading;
	AudPanel_AsyncBusy(ap);
	return (TRUE);

error:
	/* Error loading file....restore the original name */
	restore_prev(ap);
	return (FALSE);
}

/*
 * Handle the load (or reload) of a named file.
 * If the file requires conversion, initiate it.
 * If the file is ready to be loaded as is, call do_loadfile().
 * Returns:
 *	 1: load succeeded
 *	 0: load failed
 *	-1: async convert-and-load started
 *	-2: load cancelled
 */
int
start_loadfile(
	struct atool_panel_data	*ap,
	char			*fpath,
	int			reload,
	int			autoplay)
{
	Audio_hdr		hdr;
	Audio_hdr		convhdr;	/* hdr if converted */
	int			canplay;
	int			mustconvert;
	int			result;
	struct stat		fstat;
	double			len;

	/* make sure current file is ok to read */
	if (!verify_readfile(ap, fpath, &hdr))
		return (0);

	/* Save play flag in case of async load */
	ap->loadplay = autoplay;

	/* Save the format of the file, for future File->Save operations */
	memcpy((char*)&ap->save_hdr, (char*)&hdr, sizeof (Audio_hdr));

	/*
	 * Check whether the file is playable.
	 * If not playable as is, can it be simply decompressed?
	 * If not, try to find the simplest conversion to make it playable.
	 *
	 * XXX - This hard-wired ordering of tests to find a playable format
	 *       should really be in an API routine.
	 */
	memcpy((char*)&convhdr, (char*)&hdr, sizeof (Audio_hdr));
	mustconvert = FALSE;
	ap->compression = AUDIO_ENCODING_NONE;
	canplay = AudPanel_Canplay(ap, &hdr);
	if (!canplay) {
		switch (hdr.encoding) {
		/* A-law and u-law don't count as compression types */
		case AUDIO_ENCODING_ALAW:
		case AUDIO_ENCODING_ULAW:
		case AUDIO_ENCODING_LINEAR:
		case AUDIO_ENCODING_FLOAT:
			break;

		case AUDIO_ENCODING_G721:
		case AUDIO_ENCODING_G723:
		default:
			convhdr.encoding = AUDIO_ENCODING_ULAW;
			convhdr.samples_per_unit = 1;
			convhdr.bytes_per_unit = 1;
			ap->compression = hdr.encoding;
			break;
		}
		/* Try again with uncompressed format */
		canplay = AudPanel_Canplay(ap, &convhdr);
	}
	if (!canplay) {
		/* Try again as 16-bit linear */
		convhdr.encoding = AUDIO_ENCODING_LINEAR;
		convhdr.bytes_per_unit = 2;
		canplay = AudPanel_Canplay(ap, &convhdr);

		/* Flag as a conversion if this is not a simple decompress */
		mustconvert = (ap->compression == AUDIO_ENCODING_NONE);
	}
	if (!canplay) {
		/* Try again as 16-bit linear, mono */
		convhdr.channels = 1;
		canplay = AudPanel_Canplay(ap, &convhdr);
		mustconvert = TRUE;
	}
	if (!canplay) {
		/* Try again as 8-bit ulaw, mono */
		convhdr.encoding = AUDIO_ENCODING_ULAW;
		convhdr.bytes_per_unit = 1;
		canplay = AudPanel_Canplay(ap, &convhdr);
	}
	if (!canplay) {
		/* Try again as 8-bit ulaw, 8 kHz, mono */
		convhdr.sample_rate = 8000;
		canplay = AudPanel_Canplay(ap, &convhdr);
	}

	/* Set the filename field to file we're loading */
	AudPanel_Setpath(ap, fpath);

	if (!canplay || mustconvert) {
		if (!canplay) {
			/* Hopefully, this won't ever happen */
			result = AudPanel_Choicenotice(ap, MGET(
			    "Cannot convert to a playable audio data format."),
			    MGET("Continue"), MGET("Cancel"), NULL);
			result++;	/* to match the query below */

		} else if (reload) {
			result = AudPanel_Choicenotice(ap, MGET(
			    "The saved file format is not playable.  You may:\n"
			    "Convert back to a playable format or\n"
			    "Cancel the reload."),
			    MGET("Convert"), MGET("Cancel"), NULL);
			if (result == 2)
				result = 3;	/* to match the query below */
		} else {
			result = AudPanel_Choicenotice(ap, MGET(
			    "The audio file format is incompatible with the "
			    "audio device.  You may:\n"
			    "Convert to a playable format "
			    "and load as a new file,\n"
			    "Load the file as is "
			    "(play and record will be disabled), or\n"
			    "Cancel this operation."),
			    MGET("Convert"), 
			    MGET("Load"), 
			    MGET("Cancel"));
		}
		switch (result) {
		case 1: /* convert */
			/* Mark the converted file as 'untitled' */
			AudPanel_Setpath(ap, NULL);
			break;

		case 2:	/* load as is - restore the header */
			memcpy((char*)&convhdr, (char*)&hdr,
			    sizeof (Audio_hdr));
			break;

		case 3:	/* cancel */
			restore_prev(ap);
			return (2);
		}
	}

	/* Set unmodifed and update the namestripe */
	(void) AudPanel_Unmodify(ap);

	/* If a conversion is necessary, start it and return */
	if (audio_cmp_hdr(&hdr, &convhdr) != 0) {
		/* Figure out what the busy message is */
		ap->busymsg = mustconvert ?
		    MGET("Converting") : MGET("Decompressing");

		/* Estimate(!) the size of the file we're converting */
		if (stat(fpath, &fstat) >= 0) {
			len = audio_bytes_to_time(fstat.st_size, &hdr);
		} else {
			len = 0.;		/* skip freespace checks */
		}

		if (convert_and_load(ap, fpath, &convhdr, len,
		    finish_convert_load)) {
			return (-1);	/* conversion started */
		} else {
			return (0);
		}
	}
	return (do_loadfile(ap, fpath, reload) ? 1 : 0);
}

/*
 * Load the tool with a file.  If a file name is passed use it, and update
 * the file panel, otherwise get the name from the file panel.
 * The playflag controls whether to force playing when loaded.
 * Return FALSE if load fails.
 */
int
AudPanel_Loadfile(
	ptr_t			iap,
	char			*fpath,
	int			playflag)
{
	struct atool_panel_data	*ap;
	int			err;
	int			canplay;

	ap = (struct atool_panel_data *)iap;

	switch (ap->tstate) {
	case Loading:
		return (FALSE);		/* no-op */
	default:
	case Saving:
	case Playing:
	case Recording:
		/* stop recording before loading file */
		AudPanel_Stop(ap);
	case Unloaded:
	case Stopped:
		break;
	}

	/* check if file was modified & ask for veto */
	if (!AudPanel_VerifyUnload(ap))
		return (FALSE);

	/* Save the current info, in case of error */
	store_prev(ap);

	/* 
	 * If we have an empty or non-existent file, unload the data,
	 * set the default save header to whatever is current (no compression),
	 * and set path so new data can be saved to named file.
	 */
	if (verify_emptyfile(ap, fpath)) {
		/* force unload and unlink */
		(void) AudPanel_Unmodify(ap);
		AudPanel_Unloadfile(ap, TRUE, &ap->rec_hdr);

		memcpy((char*)&ap->save_hdr, (char*)&ap->rec_hdr,
		    sizeof (Audio_hdr));
		ap->tstate = Stopped;
		AudPanel_Setpath(ap, fpath);
		AudPanel_Changestate(ap);
		return (TRUE);
	}

	/* do the actual load */
	switch (start_loadfile(ap, fpath, FALSE, ap->Autoplay_load ||
	    playflag)) {
	case 1:			/* load succeeded */
		break;

	case -1:		/* async convert and load started */
		return (FALSE);	/* don't take down panel until done */

	case 0:			/* load failed */
	case -2:		/* load cancelled */
		return (FALSE);
	}
	return (TRUE);
}

/*
 * Insert a file at the insertion point.  If fpath is NULL, use the name
 * from the file box.
 */
AudPanel_Insertfile(
	ptr_t			iap,
	char			*fpath)
{
	struct atool_panel_data	*ap;	/* audio data pointer */
	int			err;	/* audio error */
	double			start;	/* start of select (seconds) */
	double			end;	/* end of selection (seconds) */
	Audio_Object		io;	/* insert object */
	double			io_secs;/* length of audio file */
	Audio_hdr		hdr;
	char			msg[MAXPATHLEN+BUFSIZ];

	ap = (struct atool_panel_data *)iap;

	AudPanel_Stop(ap);
	if (fpath == NULL)
		return;

	/* make sure current file is ok to read */
	if (!verify_readfile(ap, fpath, &hdr)) {
		return (FALSE);
	}

	/* busy message */
	/* XXX - this should be handled by async convert code */
	sprintf(msg, "%s %s...", AudPanel_Canplay(ap, &hdr) ?
	    MGET("Including") : MGET("Decompressing"), path2file(fpath));
	AudPanel_Message(ap, msg);

	if (AudPanel_Caninsert(ap, hdr) != TRUE) {
		AudPanel_Errormessage(ap, MGET("Incompatible audio format"));
		return (FALSE);
	}

	/* Open the input file readonly */
	if ((err = audio_openfile(fpath, &io)) != AUDIO_SUCCESS) {
		switch (err) {
		case AUDIO_ERR_BADHDR:
		case AUDIO_ERR_BADFILEHDR:
		case AUDIO_ERR_ENCODING:
			AudPanel_Errormessage(ap,
			    MGET("Incompatible audio format"));
			break;
		case AUDIO_UNIXERROR:
		default:
			AudPanel_Errormessage(ap, MGET("Error reading file"));
			break;
		}
		return (FALSE);
	}

	/*
	 * If selection exists, replace selection with insert file, 
	 * otherwise insert from the pointer position. 
	 */
	if (SegCanvas_Selection(ap->subp.segment)) {
		/* copy selection range */
		start = ap->start;
		end = ap->end;

		/* copy selection to the shelf */
		AudPanel_Putshelf(ap, start, end);
	} else {
		/* set start to insert pointer */
		start = ap->insert;

		/* tell list_replace no selection to cut */
		end = start;
	}

	/* insert file into list */
	/* XXX - should decompress into a temporary file and load that! */
	err = list_replace(ap->lp, start, end, io);

	/* set pointer to end of insertion */
	if (!err) {
		io_secs = audio_getlength(io);
	} else {
		io_secs = 0.;
	}

	/* free our handle to the data (list_replace implicitly refs io) */
	audio_dereference(io);

	/* if tool was unloaded, insert puts it in a stopped state */
	if (ap->tstate == Unloaded) 
		ap->tstate = Stopped;

	/* Update display */
	update_graph(ap, 0.0, AUDIO_UNKNOWN_TIME);
	if (!ap->empty) {
		(void) AudPanel_Modify(ap);
		AudPanel_Setinsert(ap, start + io_secs);
	}

	if (err) {
		AudPanel_Errormessage(ap, MGET("Incompatible audio format"));
		return (FALSE);
	}
	return (TRUE);
}


/* Called when Open button is hit */
int
Open_proc(
	ptr_t			ifp,
	char			*path)
{
	struct atool_panel_data	*ap;

	ap = AudPanel_KEYDATA(FilePanel_Getowner(ifp));
	ap->async_op = LS_Open;
	return (AudPanel_Loadfile(ap, path, FALSE));
}

/*
 * Called when Include button is hit.
 */
int
Include_proc(
	ptr_t			ifp,
	char			*path)
{
	struct atool_panel_data	*ap;

	ap = AudPanel_KEYDATA(FilePanel_Getowner(ifp));
	switch (ap->tstate) {
	default:
	case Loading:
	case Saving:
		return (FALSE);
	case Playing:
	case Recording:
	case Unloaded:
	case Stopped:
		/* AudPanel_Insertfile() stops activity */
		break;
	}
	ap->async_op = LS_Include;
	return (AudPanel_Insertfile(ap, path));
}

/* Cancel an active load */
void
AudPanel_Cancelload(
	ptr_t			iap)
{
	struct atool_panel_data	*ap = (struct atool_panel_data *)iap;

	if (ap->convpid != -1) {
		kill(ap->convpid, SIGINT);
		DBGOUT((1, "Cancel Load conversion: killing process %d\n",
		    ap->convpid));
	}
	finish_loadfile(iap, AUDIO_ERR_NOEFFECT);
}


/* 	Callbacks from the Audio Display Canvas */


/* Called from the Audio Display Canvas whenever the insertion point is set */
Set_insert_proc(
	ptr_t			iap)
{
	struct atool_panel_data	*ap;
	double			newpos;

	ap = AudPanel_KEYDATA(SegCanvas_Getowner(iap));

	/* Can't insert in empty file */
	if (ap->empty)
		return;
	switch (ap->tstate) {
	default:
	case Recording:
	case Unloaded:
	case Loading:
	case Saving:
		return;
	case Playing:
#ifdef notdef
		/* If playing, keep going if pointer is within selection */
		/* XXX - this needs segment code changes to work */
		newpos = SegCanvas_Getinsert(iap);
		if (!SegCanvas_Selection(ap->subp.segment) ||
		    ((newpos >= ap->start) && (newpos < ap->end))) {
			/* Keep playing by simulating a seek */
			newpos -= list_playposition(ap->lp);
			(void) AudPanel_Seek(iap, newpos);
			return;
		}
#endif
	case Stopped:
		break;
	}
	AudPanel_Stop(ap);

	/* display size of file (overwrite selection range display if any) */
	AudPanel_Displaysize(ap);

	/* update pointer value and display */
	AudPanel_Setinsert(ap, SegCanvas_Getinsert(iap));
}

/*
 * Is it ok to insert this data with the currently loaded data? 
 * Either the data has to match exactly or it must be easily converted.
 */
int
AudPanel_Caninsert(
	ptr_t			iap,
	Audio_hdr		hdr)
{
	struct atool_panel_data	*ap;	/* audio tool panel data  */

	ap = (struct atool_panel_data *) iap;

	if (audio_cmp_hdr(&(ap->rec_hdr), &hdr) == 0)
		return (TRUE);

	/* XXX - this is a kludge.  should use the CanConvert() method */
	if ((ap->rec_hdr.sample_rate == hdr.sample_rate) &&
	    (ap->rec_hdr.channels == hdr.channels)) {
		switch (hdr.encoding) {
		case AUDIO_ENCODING_G721:
			if ((hdr.bytes_per_unit != 1) || (hdr.channels != 1))
				break;
			return (TRUE);
		case AUDIO_ENCODING_G723:
			if ((hdr.bytes_per_unit != 3) || (hdr.channels != 1))
				break;
			return (TRUE);
		}
	}
	return (FALSE);
}

/* 
 * If being called upon "Include", set curfmtonly to check if file is
 * the same type as the currently loaded audio file.
 * Returns: 1 (TRUE) if audio file is ok, 0 (FALSE) if not, and
 * -1 (TRUE) if audio file with the wrong format.
 */
int
File_filter_proc(
	ptr_t			ifp,
	char			*path,
	int			curfmtonly)
{
	struct atool_panel_data	*ap;
	Audio_hdr		curhdr;
	Audio_hdr		hdr;
	int			fd;
	struct stat		fstat;
	unsigned char		buf[sizeof (Audio_filehdr)];
	unsigned		isize;

	if ((stat(path, &fstat) < 0) || !S_ISREG(fstat.st_mode))
		return (0);
	if (!atool_isaudiofile(path, &hdr)) {
		return (0);
	}
	if (!curfmtonly) {
		/* any audio file is ok */
		return (1);
	} else {
		ap = AudPanel_KEYDATA(FilePanel_Getowner(ifp));
		if (AudPanel_Caninsert(ap, hdr))
			return (1);
		else
			return (-1);
	}
}

/* Start copying an audio object back to the linked application */
static void
start_saveback(
	struct atool_panel_data	*ap,
	Audio_Object		aobj,
	int			done)
{
#ifndef PRE_493
	if (ap->link_msg != NULL) {
		gtt_Saveback(ap, aobj, done);
	} else
#endif
	{
		/* Save back thru the X selection service */
		Audio_SAVEBACK(ap->panel, aobj, done);
	}
}

/* Called when compressed file is ready for saveback */
static void
finish_convert_saveback(
	ptr_t			iap)
{
	Audio_Object		aobj;
	struct atool_panel_data	*ap = (struct atool_panel_data *)iap;

	if (audio_openfile(ap->tmppath, &aobj) != AUDIO_SUCCESS) {
		AudPanel_Errormessage(ap, MGET("Cannot save file"));
		restore_prev(ap);
		return;
	}
	start_saveback(ap, aobj, ap->donepending);
	clear_prev(ap);
}

/* Called upon completion of write */
void
finish_writefile(
	ptr_t			iap,
	int			status)
{
	int			err;
	char			buf[MAXPATHLEN+30];
	int			rescan = TRUE;
	int			convert;
	struct atool_panel_data	*ap = (struct atool_panel_data *)iap;

	/* Noop if already called */
	if (ap->outfile == NULL) {
		return;
	}
	AudPanel_STOP_TIMER(ap);

	if (ap->outfile) {
		/* close and free up outfile object */
		audio_dereference(ap->outfile);
		ap->outfile = NULL;
	}
	convert = (ap->convpid != -1);
	ap->convpid = -1;

	if (status != AUDIO_SUCCESS) {
		if (status == AUDIO_ERR_NOEFFECT) {
			AudPanel_Message(ap, MGET("Save cancelled"));
		} else if (convert) {
			AudPanel_Errormessage(ap, MGET("Cannot convert file"));
		} else {
			AudPanel_Errormessage(ap, MGET("Cannot write file"));
		}
		goto save_error;
	} 

	/* If finishing a saveback, send the temp file and remove it */
	if (ap->saveback) {
		finish_convert_saveback(ap);
		return;
	}

	/* Rename new file (rename() removes old one) */
	if (rename(ap->tmppath, ap->savepath) < 0) {
		AudPanel_Errormessage(ap, MGET("Cannot rename file"));
		goto save_error;
	}
	ap->tmppath[0] = '\0';

	/* See if cancelled recently
	if (ap->tstate != Saving) {
		AudPanel_Message(ap, MGET("Reload cancelled"));
		goto save_error;
	}

	/* If name changed, and load succeeds, update file choosers soon */
	if (strcmp(ap->savepath, ap->cpath) == 0) {
		rescan = FALSE;
	} else {
		rescan = TRUE;
	}
	status = start_loadfile(ap, ap->savepath, TRUE, FALSE);

	/* Get the name into the file panels.  This is *SLOW* */
	if (rescan) {
		FilePanel_Rescan(AudPanel_FilePanel_hndl(ap));
	}
	switch (status) {
	case 1:			/* load succeeded */
		break;
	case -1:		/* async convert and load started */
		return;

	case 0:			/* load failed */
		sprintf(buf, MGET("Reload of %s failed"),
		    path2file(ap->savepath));
		AudPanel_Errormessage(ap, buf);

	case -2:		/* load cancelled */
save_error:	restore_prev(ap);
		return;
	}
	/* Take down the initating panel if unpinned */
	panel_takedown(ap);
	clear_prev(ap);
	return;
}

/* Handler to keep data moving out to a pipe */
int
do_async_writefile(
	ptr_t			iap)
{
static char			*savefmt_str = NULL;	/* Cache translation */
	char			buf[BUFSIZ];
	double			length;
	double			pcnt;
	int			err;
	struct atool_panel_data	*ap = (struct atool_panel_data *)iap;

	if (ap->tstate != Saving)
		return;

	/* first do a write */
	switch (err = list_async_writefile(ap->lp,
	    ap->outfile, &(ap->frompos), &(ap->topos))) {
	case AUDIO_EOF:
		/* 
		 * if there's a convert process, don't finish here,
		 * finish when the process dies (in the callback for
		 * the SIGCHILD).
		 */
		if (ap->convpid == -1) {
			finish_writefile(ap, AUDIO_SUCCESS);
		} else {
			/* make sure the pipe shuts down! */
			AudPanel_STOP_TIMER(ap);
			ap->tstate = Stopped;
			audio_close(ap->outfile);
		}
		break;
	case AUDIO_SUCCESS:
		/* how much has been written */
		length = list_getlength(ap->lp);
		pcnt = 100.0 * (ap->frompos / length);

		if (!savefmt_str) {
			savefmt_str = strdup(
			    MGET("Saving (%d%%)  [<STOP> to cancel]"));
		}
		sprintf(buf, savefmt_str, (int)irint(pcnt));
		AudPanel_Message(ap, buf);

		FilePanel_Setfilesize(ap->subp.file, length);
		break;
	default:
		/* XXX - handle error */
		finish_writefile(ap, err);
		break;
	}
	return (err);
}

/* called when child audioconvert process exits */
void
finish_convert_save(
	ptr_t			iap,
	int			status)
{
	struct atool_panel_data	*ap = (struct atool_panel_data *)iap;

	/* make sure this doesn't get called more then once ... */
	if (ap->convpid != -1) {
		finish_writefile(iap,
		    (status == 0) ? AUDIO_SUCCESS : AUDIO_UNIXERROR);
	}
}

/* Create a pipe to audioconvert to convert data to save format */
static int
create_convert_pipe(
	struct atool_panel_data *ap,
	char			*path,
	char			*convstr,
	ptr_t			*ofp,
	void			(*callback)())
{
	char			cmdbuf[MAXPATHLEN*2];
	int			pipefd;
	int			pid;

	/* Quote the encoding string and hand it to audioconvert */
	sprintf(cmdbuf, "audioconvert -f \"%s\" -o %s", convstr, path);

	pipefd = atool_pipe(cmdbuf, callback, (caddr_t)ap, &pid);
	if (pipefd == XV_ERROR) {
		return (AUDIO_UNIXERROR);
	}
	/* Save pid of convert process to wait for completion */
	ap->convpid = pid;

	/*
	 * We've got an fd to the audioconvert pipe.
	 * Now turn this in to an audio pipe object.
	 */
	return (list_createpipe(ap->lp, pipefd, path, ofp));
}

/* Save the file back to a linked application */
int
Audio_Saveback(
	struct atool_panel_data	*ap,
	int			done,
	Audio_hdr		*hdrp)		/* target audio format */
{
	int			err;		/* audio error */
	Audio_Object		aobj;
	Audio_hdr		hdr;
	ptr_t			ofp = NULL;	/* output file object */
	char			*tpath;
	char			*conv;
	char			buf[MAXPATHLEN+100];	/* file name */

	/* XXX - for now, Save always takes down panel */
	done = TRUE;

	sprintf(buf, MGET("Saving back to %s"), ap->link_app);
	AudPanel_Message(ap, buf);
	ap->saveback = TRUE;

	/* Check if we're changing format on save */
	if (hdrp == NULL)
		hdrp = &ap->save_hdr;
	list_getfilehdr(ap->lp, 0.0, AUDIO_UNKNOWN_TIME, &hdr);
	if (audio_cmp_hdr(hdrp, &hdr) == 0) {
		/* Start copying out the data */
save_asis:
		restore_prev(ap);
		list_copy(ap->lp, 0., AUDIO_UNKNOWN_TIME, &aobj);
		start_saveback(ap, aobj, done);
	} else {

		/* Use audioconvert to save to a temporary file */

		/* Look for a directory with enough space for the file */
		tpath = find_tempdir(ap, NULL, hdrp, list_getlength(ap->lp));
		if (tpath == NULL) {
			switch (AudPanel_Choicenotice(ap, MGET(
	    "There is not enough space in the Temp file directory to perform\n"
	    "the format conversion required to save this file.  You may:\n"
	    "Continue saving the uncompressed data, or\n"
	    "Cancel the saveback operation."),
			    MGET("Continue"), MGET("Cancel"), NULL)) {
			case 2:
save_error:
				restore_prev(ap);
				return (FALSE);
			default:
				goto save_asis;
			}
		}

		/* Store the name of the tmp file being loaded */
		set_tmppath(ap, tpath);
		(void) free(tpath);

		/* Get the format conversion string and start audioconvert */
		if ((tpath = audio_printhdr(hdrp)) == NULL) {
			/* Cannot convert....save as is */
			goto save_asis;
		}
		err = create_convert_pipe(ap, ap->tmppath, tpath, &ofp,
		    finish_convert_save);
		(void) free(tpath);
		if (err != AUDIO_SUCCESS) {
			/* Cannot convert....save as is */
			goto save_asis;
		}

		/* Save output object ptr and rewind to start of audio data */
		ap->outfile = ofp;
		(void) list_rewind(ap->lp);
		ap->donepending = done;

		/* put up initial Saving message and busy the tool */
		AudPanel_Message(ap, MGET("Saving (0%)  [<STOP> to cancel]"));
		AudPanel_AsyncBusy(ap);

		ap->frompos = 0.0;
		ap->topos = 0.0;
		ap->tstate = Saving;

		/* start I/O on first call */
		schedule_async_handler(ap);

		/* get timer rolling (will first do write) */
		AudPanel_START_TIMER(ap, save_timer);
	}
	return (TRUE);
}


/* Save the file to the specified filename and reload it, if possible. */
int
AudPanel_Savefile(
	ptr_t			iap,
	char			*spath,
	int			saveasflag,
	int			doneflag,
	Audio_hdr		*hdrp)		/* target audio format */
{
	int			err;		/* audio error */
	char			*tpath;		/* temp path to save to */
	char			dir[MAXPATHLEN+1]; 	/* directory name */
	char			buf[MAXPATHLEN+100];	/* file name */
	Audio_hdr		hdr;		/* current audio format */
	ptr_t			ofp = NULL;	/* output file object */
	int			cflag = FALSE;	/* is conversion required? */
	char			*str;
	struct atool_panel_data	*ap = (struct atool_panel_data *)iap;

	/* react to current state */
	switch (ap->tstate) {
	case Saving:
		return (FALSE);		/* no-op */
	default:
	case Playing:
	case Recording:
	case Loading:
		/* stop recording before saving file */
		AudPanel_Stop(ap);
	case Unloaded:
	case Stopped:
		break;
	}

	if (ap->empty) {
		AudPanel_Errormessage(ap, MGET("Nothing to save"));
		return (FALSE);
	}

	/* Save the current info, in case of error */
	store_prev(ap);
	ap->convpid = -1;
	ap->saveback = FALSE;

	if (saveasflag) {
		/* verify access, post alert if file exists */
		if (!verify_writefile(ap, spath, TRUE)) {
			return (FALSE);
		}
	} else if (ap->link_app[0]) {
		/* Save back if linked */
		return (Audio_Saveback(ap, doneflag, hdrp));
	}

	/* Copy out the directory part of save path */
	if (tpath = strrchr(spath, '/')) {
		strncpy(dir, spath, tpath - spath);
		dir[tpath - spath] = '\0';	/* terminate string */
	} else {
		strcpy(dir, "."); 		/* no dir, use "." */
	}

	/* Make sure dir is writeable, if not, post notice */
	if (access_dir(dir) == -1) {
		/* XXX - should check errno and be more specific */
		AudPanel_Errormessage(ap,
		    MGET("Directory write access denied"));
		return (FALSE);
	}

	/* Create a temp name to write file to */
	if ((tpath = tempnam(dir, "audio")) == NULL) {
		AudPanel_Errormessage(ap,
		    MGET("Could not create output file"));
		return (FALSE);
	}

	/* Store the names of the files being written */
	set_savepath(ap, spath);
	set_tmppath(ap, tpath);
	(void) free(tpath);

	/*
	 * Check if we're changing format on save.
	 * If so, create a pipe to audioconvert,
	 * otherwise create a file and write to it.
	 */
	list_getfilehdr(ap->lp, 0.0, AUDIO_UNKNOWN_TIME, &hdr);
	if (hdrp && (audio_cmp_hdr(hdrp, &hdr) != 0)) {
		/* Make sure there is enough room for the save */
		if (!verify_freespace(dir, list_getlength(ap->lp), hdrp)) {
space_error:
			AudPanel_Alert(ap, MGET(
			    "There is not enough space available to "
			    "perform the Save operation.\n"
			    "You may either delete files to recover storage "
			    "space,\n"
			    "or save the file to an alternate directory "
			    "on another filesystem."));
			goto save_error;
		}

		/*
		 * Check to make sure the conversion is remotely possible.
		 * If a foreign type was loaded without conversion,
		 * then this could fail here.
		 */
		if ((tpath = audio_printhdr(hdrp)) == NULL) {
			switch (AudPanel_Choicenotice(ap, MGET(
			    "The current audio format cannot be converted as "
			    "requested.  You may:\n"
			    "Continue saving without format conversion, or\n"
			    "Cancel the save operation."),
			    MGET("Continue"), MGET("Cancel"), NULL)) {
			case 2:
save_error:
				restore_prev(ap);
				return (FALSE);
			default:
				goto save_asis;
			}
		}

		/* Conversion is ok, but check for mono->stereo */
		if ((hdr.channels ==1) && (hdrp->channels > 1)) {
			str = audio_print_channels(hdrp);
			(void) sprintf(buf, MGET(
			    "You have selected a Save Format that will "
			    "result in a conversion from mono to %s.\n"
			    "This will produce a larger output file due to "
			    "the unnecessary duplication of data.  You may:\n"
			    "Continue saving in the specified format, or\n"
			    "Cancel the save operation."), str);
			(void) free(str);
			switch (AudPanel_Choicenotice(ap, buf,
			    MGET("Continue"), MGET("Cancel"), NULL)) {
			case 2:
				goto save_error;
			default:
				break;
			}
		}
		/* If error, a message will be displayed */
		cflag = TRUE;
		err = create_convert_pipe(ap, ap->tmppath, tpath, &ofp,
		    finish_convert_save);
		(void) free(tpath);
	} else {
		/* No conversion, just write the current type */
save_asis:
		/* First make sure there is room */
		if (!verify_freespace(dir, list_getlength(ap->lp), &hdr))
			goto space_error;
		err = list_createfile(ap->tmppath, &hdr, &ofp);
	}
	switch (err) {
	case AUDIO_SUCCESS:
		break;

	case AUDIO_ERR_NOEFFECT:
		AudPanel_Errormessage(ap, MGET("Nothing to save"));
		goto save_error;
	default:
		if (cflag) {
			AudPanel_Errormessage(ap, MGET("Cannot convert file"));
		} else {
			AudPanel_Errormessage(ap, MGET("Cannot write file"));
		}
		goto save_error;
	}

	/* Save pointer to output object and rewind to start of audio data */
	ap->outfile = ofp;
	(void) list_rewind(ap->lp);

	/* put up initial Saving message and busy the tool */
	AudPanel_Message(ap, MGET("Saving (0%)  [<STOP> to cancel]"));
	AudPanel_AsyncBusy(ap);

	DBGOUT((2, "starting file save ...\n"));
	ap->frompos = 0.0;
	ap->topos = 0.0;
	ap->tstate = Saving;

	/* start I/O on first call */
	schedule_async_handler(ap);

	/* get timer rolling (will first do write) */
	AudPanel_START_TIMER(ap, save_timer);
	return (TRUE);
}

/* Called when Save As button is hit */
int
Save_proc(
	ptr_t			ifp,
	char			*path,
	int			saveasflag,
	Audio_hdr		*hdrp)
{
	struct atool_panel_data	*ap;	/* audio panel data  */

	ap = AudPanel_KEYDATA(FilePanel_Getowner(ifp));

	/* No save panel, so don't take down save as panel */
	ap->async_op = saveasflag ? LS_SaveAs : LS_None;
	(void) AudPanel_Savefile(ap, path, saveasflag, FALSE, hdrp);

	/*
	 * Errors keep panel up, and the rest of the save is asynchronous
	 * so always return FALSE (keep panels mapped).
	 */
	return (FALSE);
}

/* Cancel an active save */
AudPanel_Cancelsave(
	ptr_t		iap)
{
	struct atool_panel_data	*ap = (struct atool_panel_data *)iap;

	if (ap->convpid != -1) {
		kill(ap->convpid, SIGINT);
		DBGOUT((1, "Cancel Save conversion: killing process %d\n",
		    ap->convpid));
	}
	finish_writefile(iap, AUDIO_ERR_NOEFFECT);
}
