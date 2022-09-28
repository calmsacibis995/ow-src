/* Copyright (c) 1993 by Sun Microsystems, Inc. */

#ifndef _AUDIOTOOL_ATOOL_TTCE_H
#define	_AUDIOTOOL_ATOOL_TTCE_H

#ident	"@(#)atool_ttce.h	1.2	92/12/14 SMI"

/* tooltalk & classing engine functions */

#include <xview/xview.h>

/* # of sec's to sleep before really quitting */
#define TT_SLEEP_TIME (30 * 60)

extern int start_tt_init(char *app_string, int fetch_msg,
	int argc, char **argv);

extern void complete_tt_init(Xv_opaque frame);
extern void quit_tt(void);
extern void set_tt_quit_timer(int flag);
extern Xv_opaque get_ce_icon(char *file_type);

#endif /* !_AUDIOTOOL_ATOOL_TTCE_H */
