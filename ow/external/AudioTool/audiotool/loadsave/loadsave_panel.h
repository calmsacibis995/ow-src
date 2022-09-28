/* Copyright (c) 1993 by Sun Microsystems, Inc. */

#ifndef _AUDIOTOOL_LOADSAVE_PANEL_H
#define	_AUDIOTOOL_LOADSAVE_PANEL_H

#ident	"@(#)loadsave_panel.h	1.14	92/12/14 SMI"

#include <multimedia/audio_hdr.h>
#include "atool_types.h"

typedef enum {
	LS_None,
	LS_Open,
	LS_Save,
	LS_SaveAs,
	LS_Include,
} FilePanel_Type;

/* Define public interfaces */
extern ptr_t	FilePanel_Init(ptr_t,
		    int (*)(), int (*)(), int (*)(), int (*)());
extern void	FilePanel_Setfooter(ptr_t, FilePanel_Type, char*);
extern void	FilePanel_Defaultsavepath(ptr_t, char*);
extern void	FilePanel_Setcompresshdr(ptr_t, Audio_hdr*, int);
extern void	FilePanel_Setformat(ptr_t, Audio_hdr*, int);
extern void	FilePanel_Updatefilesize(ptr_t);
extern void	FilePanel_Setfilesize(ptr_t, double);
extern void	FilePanel_Rescan(ptr_t);
extern ptr_t	FilePanel_Getowner(ptr_t);
extern void	FilePanel_Show(ptr_t, FilePanel_Type);
extern void	FilePanel_Unshow(ptr_t);
extern void	FilePanel_Takedown(ptr_t, FilePanel_Type);
extern void	FilePanel_Busy(ptr_t);
extern void	FilePanel_Unbusy(ptr_t);

/*
 * These are used as format menu callbacks and must conform to
 * the interfaces defined by format/format.c
 */
extern int	FilePanel_Formatnotify(ptr_t, Audio_hdr*);
extern int	FilePanel_Cansave(ptr_t, Audio_hdr*);

#endif /* !_AUDIOTOOL_LOADSAVE_PANEL_H */
