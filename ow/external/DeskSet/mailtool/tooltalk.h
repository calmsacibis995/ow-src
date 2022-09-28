/*	@(#)tooltalk.h 3.1 IEI SMI	*/

/*
 * Copyright (c) 1990 by Sun Microsystems, Inc.
 */

#ifndef _mt_tooltalk_h
#define _mt_tooltalk_h

/*
 * Routines to send tooltalk messages
 */
Tt_message mt_send_tt_launch(), mt_send_tt_dispatch_data(), mt_send_tt_expose();

void mt_send_tt_move(), mt_send_tt_quit(), mt_send_tt_hide();

int	mt_start_tt_init();
void	mt_complete_tt_init();
void	mt_quit_tt();
   
extern int      mt_tt_running;          /* TRUE if tool talk is running */

#endif /* !_mt_tooltalk_h */

