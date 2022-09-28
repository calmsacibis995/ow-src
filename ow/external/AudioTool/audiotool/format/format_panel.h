/* Copyright (c) 1993 by Sun Microsystems, Inc. */

#ifndef _AUDIOTOOL_FORMAT_PANEL_H
#define	_AUDIOTOOL_FORMAT_PANEL_H

#ident	"@(#)format_panel.h	1.8	93/01/07 SMI"

#include <multimedia/libaudio.h>
#include "atool_types.h"

struct format_list_entry {
	char		*label;	/* format label */
	Audio_hdr	hdr;	/* audio hdr describing format */
	int		readonly; /* readonly ("hardwired") entry? */
};

/* Define public interfaces */
extern ptr_t FormatPanel_Init(ptr_t owner);
extern void FormatPanel_Show(ptr_t dp, PFUNCI apply_proc, ptr_t cdata,
			     char *banner);
extern int FormatPanel_Setformat(ptr_t dp, Audio_hdr*);
extern char *FormatPanel_Getformatname(ptr_t dp, Audio_hdr*);

#endif /* !_AUDIOTOOL_FORMAT_PANEL_H */
