/* Copyright (c) 1993 by Sun Microsystems, Inc. */

#ifndef _AUDIOTOOL_FORMAT_PANEL_IMPL_H
#define	_AUDIOTOOL_FORMAT_PANEL_IMPL_H

#ident	"@(#)format_panel_impl.h	1.7	92/12/14 SMI"

#include "atool_types.h"
#include "listmgr.h"
#include "format_panel.h"

struct format_panel_data {
	ptr_t		owner;
	ptr_t		panel;
	List		*fmtlist;		/* format_list_entry objects */
	int		(*apply_proc)();	/* callback for apply button */
	ptr_t		cdata;			/* client data */
	int		panel_min_width;	/* store to shrink panel */
	int		frame_min_width;	/* store to shrink panel */
	int		frame_max_width;	/* store to grow panel */
	Audio_hdr	curhdr;		/* current settings (for reset) */
};

extern ptr_t	FormatPanel_INIT(ptr_t, ptr_t);
extern void	FormatPanel_SHOW(ptr_t);
extern void	FormatPanel_UNSHOW(ptr_t);
extern int	append_format_entry(List*, char*, int, int, int, int);
extern char	*find_format_hdr(struct format_panel_data*, Audio_hdr*);

#endif /* !_AUDIOTOOL_FORMAT_PANEL_IMPL_H */

