/* Copyright (c) 1993 by Sun Microsystems, Inc. */
#ident  "@(#)loadsave.c	1.35	92/12/14 SMI"

#include <stdio.h>
#include <sys/types.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <errno.h>
#include <string.h>

#include "undolist_c.h"
#include "loadsave_panel_impl.h"
#include "loadsave_panel.h"
#include "atool_i18n.h"


/* concatenate the directory and file names to make one full path name */
static void
concat_path(
	char		*path,
	char		*dir,
	char		*file)
{
	if (*dir == '\0')
		/* just copy the file name */
		sprintf(path, "%s", file);
	else
		/* create a full path name */
		sprintf(path, "%s/%s", dir, file);
}

/* 
 * parse the path name into a directory and a file.
 * if there are no directory dividers (/), then assume it's
 * just a file name and return a null directory
 */
static void
parse_path(
	char		*path,
	char		*dir,
	char		*file)
{
	int		dir_exists;	/* true if directory component exists */
	char		*p;

	p = path;
	/* look for a directory separator (/) */
	while ((*p != '\0') && (*p != '/')) p++;

	if (*p == '\0') {
		/* directory component does not exist */
		*dir = '\0';
		(void) strcpy(file, path);
	} else {
		/* directory component exists, find start of file name */
		p = path;
		p += strlen(path) - 1;
		while (*p != '/') p--;

		/* copy file name */
		(void) strcpy(file, p+1);

		/* copy dir name */
		(void) strncpy(dir, path, p - path);
		/* strncpy does not null terminate */
		dir[p - path] = '\0';
	}
}


/* Create the File Panels */
ptr_t
FilePanel_Init(
	ptr_t			owner,
	int			(*open_proc)(),
	int			(*save_proc)(),
	int			(*include_proc)(),
	int			(*filt_proc)())
{
	struct file_panel_data	*fp;

	/* allocate storage for file panel data */
	if ((fp = (struct file_panel_data *)calloc(1, sizeof (*fp))) == NULL)
		return (NULL);

	fp->owner = owner;

	/* init call backs */
	fp->open_proc = open_proc;
	fp->save_proc = save_proc;
	fp->include_proc = include_proc;
	fp->filt_proc = filt_proc;

	/* Init the file panel window */
	FilePanel_INIT(owner, (ptr_t)fp);
	return ((ptr_t) fp);
}


/* Set the left lower footer of the panel */
void
FilePanel_Setfooter(
	ptr_t			ifp,
	FilePanel_Type		which,
	char			*str)
{
	FilePanel_SETLEFTFOOTER(ifp, which, str);
}

/*
 * Set the default save panel directory
 * This only takes effect if the save panel has never been mapped
 */
void
FilePanel_DefaultSavepath(
	ptr_t			ifp,
	char			*path)
{
	struct file_panel_data	*fp;
	struct stat		ps;
	char			dir[MAXPATHLEN+1];
	char			file[MAXPATHLEN+1];

	fp = (struct file_panel_data *)ifp;

	/* if full path is a directory, set it */
	if ((stat(path, &ps) == 0) && S_ISDIR(ps.st_mode)) {
		(void) strcpy(fp->savedir, path);
	} 

	/* parse path into a directory and a file name */
	parse_path(path, dir, file);

	/* verify it's a directory */
	if ((stat(dir, &ps) == 0) && S_ISDIR(ps.st_mode)) {
		(void) strcpy(fp->savedir, dir);
	}
}

/* Store the current format and update the format panel */
static void
FilePanel_Setsaveformat(
	ptr_t			ifp,
	Audio_hdr		*hdrp,
	int			compression)
{
	struct file_panel_data	*fp = (struct file_panel_data*)ifp;
	Audio_hdr		chdr;

	memcpy(&(fp->save_hdr), hdrp, sizeof (Audio_hdr));

	if (FilePanel_SETFORMAT(fp, hdrp) == TRUE) {
		/* Also set format panel to these settings */
		FormatPanel_Setformat(AudPanel_FormatPanel_hndl(
		    AudPanel_KEYDATA(fp->owner)), hdrp);
	}

	/* make sure we can do this compression, otherwise reset to none */
	if ((compression != AUDIO_ENCODING_NONE) &&
	    (FilePanel_Setcompresshdr(fp, &chdr, compression),
	    !FilePanel_Cansave(fp, &chdr))) {
		fp->compression = AUDIO_ENCODING_NONE;
	} else {
		fp->compression = compression;
	}
	FilePanel_SETCOMPRESSION(fp, fp->compression);

	/* re-calculate size of loaded file for display */
	FilePanel_Updatefilesize(ifp);
}

/* Convert the save header to use the current compression */
void
FilePanel_Setcompresshdr(
	ptr_t			ifp,
	Audio_hdr		*hdrp,
	int			compression)
{
	struct file_panel_data	*fp = (struct file_panel_data*)ifp;
	
	memcpy(hdrp, &(fp->save_hdr), sizeof (Audio_hdr));

	/* XXX - should have an API routine for this stuff */
	switch (compression) {
	case AUDIO_ENCODING_G721:
		hdrp->bytes_per_unit = 1;
		hdrp->samples_per_unit = 2;
		hdrp->encoding = AUDIO_ENCODING_G721;
		break;
	case AUDIO_ENCODING_G723:
		hdrp->bytes_per_unit = 3;
		hdrp->samples_per_unit = 8;
		hdrp->encoding = AUDIO_ENCODING_G723;
		break;
	case AUDIO_ENCODING_NONE:
	default:
		break;
	}
}

/* Set current and save format (happens when a new file is loaded) */
void
FilePanel_Setformat(
	ptr_t			ifp,
	Audio_hdr		*hdrp,
	int			compression)
{
	struct file_panel_data	*fp = (struct file_panel_data*)ifp;

	memcpy(&(fp->cur_hdr), hdrp, sizeof (Audio_hdr));
	FilePanel_Setsaveformat(fp, hdrp, compression);
}

/* called from the format menu notify proc */
int
FilePanel_Formatnotify(
	ptr_t			ifp,
	Audio_hdr		*hdrp)
{
	struct file_panel_data	*fp = (struct file_panel_data*)ifp;

	/* first make sure we can actually save in this format */
	if (FilePanel_Cansave(fp, hdrp) != TRUE) {
		AudPanel_Alert(AudPanel_KEYDATA(fp->owner),
		  MGET("Cannot convert and save to this format."));
		return (FALSE);
	}
	/* Store the format and update the panel */
	FilePanel_Setsaveformat(ifp, hdrp, fp->compression);
	return (TRUE);
}

/* Update the Save panel with the current file size and free space */
void
FilePanel_Updatefilesize(
	ptr_t			ifp)
{
static char			*sizemsg = NULL;
static char			*romsg = NULL;
	struct file_panel_data	*fp = (struct file_panel_data*)ifp;
	long			fsize;
	long			freespace;
	char			*dir;
	char			buf[BUFSIZ];
	char			*s1 = NULL;
	char			*s2 = NULL;
	Audio_hdr		hdr;	/* copy of hdr w/compression */

	/* Don't bother if not created or mapped yet */
	if (!FilePanel_ISMAPPED(ifp, LS_SaveAs))
		return;

	if (sizemsg == NULL) {
		sizemsg = strdup(MGET("%s Mbytes  (%s Mb Available)"));
		romsg = strdup(MGET("%s Mbytes  (Read-Only Directory)"));
	}

	/* Always set according to "save as" format */
	FilePanel_Setcompresshdr(fp, &hdr, fp->compression);
	fsize = audio_getsize(fp->filesize, &hdr);
	s1 = audio_printkbytes(fsize);

	/* Get current directory free space */
	dir = FilePanel_GETDIRECTORY(ifp, LS_SaveAs);
	freespace = getfreespace(dir);

	/* oops - bad dir, don't display space */
	if (freespace != -1) {
		s2 = audio_printkbytes(freespace);
		sprintf(buf, sizemsg, s1, s2);
	} else {
		sprintf(buf, romsg, s1);
	}
	FilePanel_SETFILESIZESTR(fp, buf);

	if (s1)
		free(s1);
	if (s2)
		free(s2);
}

/* 
 * Takes length of current file in time.
 * Displays file size on save panel for currently loaded type.
 */
void
FilePanel_Setfilesize(
	ptr_t			ifp,
	double			len)
{
	struct file_panel_data	*fp = (struct file_panel_data *)ifp;

	/* Save current file size */
	fp->filesize = len;
	FilePanel_Updatefilesize(ifp);
}

/* Rescan directory of load file chooser */
void
FilePanel_Rescan(ptr_t ifp)
{
	FilePanel_ALLRESCAN(ifp);
}

/* Return the File Panel owner handle */
ptr_t
FilePanel_Getowner(ptr_t ifp)
{
	struct file_panel_data	*fp = (struct file_panel_data *)ifp;

	return ((ptr_t)fp->owner);
}

/* Pop up the File Panel */
void
FilePanel_Show(
	ptr_t			ifp,
	FilePanel_Type		which)
{
	/* Make sure the panel info is current */
	FilePanel_Setfooter(ifp, which, "");
	FilePanel_SHOW(ifp, which);
	FilePanel_Updatefilesize(ifp);
}

/* Dismiss the File Panel */
void
FilePanel_Unshow(
	ptr_t			ifp)
{
	FilePanel_ALLUNSHOW(ifp);
}

/* Take down the specified panel if it is unpinned */
void
FilePanel_Takedown(
	ptr_t			ifp,
	FilePanel_Type		which)
{
	FilePanel_TAKEDOWN(ifp, which);
}

/* 
 * compare the current header with the passed in header and determine
 * if this conversion is possible.
 */
int
FilePanel_Cansave(
	ptr_t		ifp,
	Audio_hdr	*hdrp)
{
	/* XXX - what *can't* we convert to? */
	return (TRUE);
}

void
FilePanel_Busy(
	ptr_t		ifp)
{
	FilePanel_ALLBUSY(ifp, TRUE);
}

void
FilePanel_Unbusy(
	ptr_t		ifp)
{
	FilePanel_ALLBUSY(ifp, FALSE);
}
