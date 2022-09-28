/* Copyright (c) 1993 by Sun Microsystems, Inc. */

#ifndef _AUDIOTOOL_LOADSAVE_PANEL_IMPL_H
#define	_AUDIOTOOL_LOADSAVE_PANEL_IMPL_H

#ident	"@(#)loadsave_panel_impl.h	1.21	92/12/14 SMI"

#include <sys/param.h>
#include <multimedia/audio_hdr.h>
#include "atool_types.h"
#include "loadsave_panel.h"

/* XXX - use GFM code 'till real file chooser is available */
#ifdef PRE_493
#define	USE_GFM
#endif

struct file_panel_data {
	ptr_t			owner;	/* handle to owner of file panels */

	/*
	 * XXX - If USE_GFM, then only the open_panel is used,
	 *       and reused for each function.
	 */
	ptr_t			open_panel;
	ptr_t			saveas_panel;
	ptr_t			include_panel;
	ptr_t			audio_file_glyph;
	int			busy;		/* load/save in process? */
	double			filesize;	/* length of current data */

	char			opendir[MAXPATHLEN+1];
	char			savedir[MAXPATHLEN+1];

	/* call backs for buttons, etc. */
	int			(*open_proc)();
	int			(*save_proc)();
	int			(*include_proc)();
	int			(*filt_proc)();


	Audio_hdr		save_hdr;   /* hdr that describes save fmt */
	Audio_hdr		cur_hdr;    /* hdr that describes loaded fmt */
	int			compression;/* compression encoding */
};

/* XXX - ??? */
/* struct to map between audio hdr encodings and UI object setting values */
struct file_panel_encodings {
	int			encoding;
	int			setting;
};

/* Define private interfaces */
extern int	FilePanel_INIT(ptr_t, ptr_t);
extern void	FilePanel_SETFILESIZESTR(ptr_t, char*);
extern void	FilePanel_SHOW(ptr_t, FilePanel_Type);
extern int	FilePanel_SETFORMAT(ptr_t, Audio_hdr*);
extern void	FilePanel_SETCOMPRESSION(ptr_t, int);
extern char*	FilePanel_GETDIRECTORY(ptr_t, FilePanel_Type);
extern void	FilePanel_SETLEFTFOOTER(ptr_t, FilePanel_Type, char*);
extern void	FilePanel_TAKEDOWN(ptr_t, FilePanel_Type);
extern int	FilePanel_ISMAPPED(ptr_t, FilePanel_Type);
extern void	FilePanel_ALLBUSY(ptr_t, int);
extern void	FilePanel_ALLRESCAN(ptr_t);
extern void	FilePanel_ALLUNSHOW(ptr_t);

#endif /* !_AUDIOTOOL_LOADSAVE_PANEL_IMPL_H */
