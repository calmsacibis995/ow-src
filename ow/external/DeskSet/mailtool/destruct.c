#ifndef lint
static 	char sccsid[] = "@(#)destruct.c 3.3 94/05/04 Copyr 1992 Sun Micro";
#endif

/*
 * Copyright (c) 1992 by Sun Microsystems, Inc.
 */

/*
 * Routines which implement mailtool's self destruct capability. 
 *
 * When mailtool comes up via tooltalk it stays unmapped, so that
 * requests for a compose window (from calendar manager for example)
 * appear to only bring up a compose window (and not an entire mailtool).
 * When the compose window goes away mailtool is not visible, but is still
 * running.  We want to keep it running for a while so that subsequent tooltalk
 * requests are handled quickly, but we don't want it to stay up forever.
 *
 * So mailtool stays in this hidden state for a period of time, after which
 * it exits.
 */

#include <xview/xview.h>
#include <xview/panel.h>
#include <xview/text.h>
#include <xview/font.h>
#include <xview/svrimage.h>
#include <xview/cms.h>
#include <xview/notice.h>
#include <xview/notify.h>

#include "tool.h"
#include "glob.h"

#define DEBUG_FLAG mt_debugging
extern int DEBUG_FLAG;
#include "debug.h"

/*
 * itimer callback to destroy mailtool
 */
Notify_value
mt_self_destruct(client, which)

	Notify_client	client;
	int		which;
{
	struct  header_data	*hd;

	DP(("mt_self_destruct()\n"));

	hd = mt_get_header_data(mt_frame);

	/*
	 * Only do destruction if mailtool is hidden
	 */
	if (mt_mailtool_hidden(hd)) {
		mt_stop_self_destruct();
		DP(("\tDestroying baseframe %d\n", mt_frame));
		(void)xv_destroy_safe(mt_frame);
	}

	return NOTIFY_DONE;
}

/*
 * Start mailtool's self destruct sequence. Mailtool will
 * terminate after "interval" seconds.
 *
 * Returns TRUE if the self destruct sequence is started, else FALSE.
 */
int
mt_start_self_destruct(interval) 

	int	interval;	/* In seconds */

{
	struct itimerval	itv = { {0, 0}, {0, 0} };
	static long		destruct_client;

	DP(("mt_start_self_destruct(): interval = %d seconds\n", interval));

	itv.it_value.tv_sec = interval;
	itv.it_interval.tv_sec = interval;

	(void)notify_set_itimer_func((Notify_client)&destruct_client,
				mt_self_destruct, ITIMER_REAL, &itv, 0);

	if (interval == 0)
		return FALSE;
	else
		return TRUE;
}

/*
 * Stop mailtool self destruct sequence
 *
 * Returns TRUE if the self destruct sequence is terminated, else FALSE.
 */
int
mt_stop_self_destruct() 

{
	DP(("mt_stop_self_destruct()\n"));
	return !mt_start_self_destruct(0);
}

/*
 * Check if mailtool is hidden. When mailtool comes up via tooltalk it
 * stays unmapped.  This allows clients to request compose windows without
 * the mailtool baseframe comming up. 
 *
 * Returns TRUE if mailtool is completely unmapped, else FALSE.
 */
int
mt_mailtool_hidden(hd)

	struct header_data	*hd;

{
	struct reply_panel_data	*rpd;

	if ((int)xv_get(hd->hd_frame, XV_SHOW, TRUE)) {
		return FALSE;
	}

	for (rpd = MT_RPD_LIST(hd);rpd != NULL; rpd = rpd->next_ptr) {
		if (xv_get(rpd->frame, XV_SHOW))
			return FALSE;
	}

	return TRUE;
}
