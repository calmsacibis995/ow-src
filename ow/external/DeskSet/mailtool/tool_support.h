/*      @(#)tool_support.h 1.3 IEI SMI      */

/*	Copyright (c) 1987, 1988, Sun Microsystems, Inc.  All Rights Reserved.
	Sun considers its source code as an unpublished, proprietary
	trade secret, and it is available only under strict license
	provisions.  This copyright notice is placed here only to protect
	Sun in the event the source is deemed a published work.  Dissassembly,
	decompilation, or other means of reducing the object code to human
	readable form is prohibited by the license agreement under which
	this code is provided to the user or company in possession of this
	copy.

	RESTRICTED RIGHTS LEGEND: Use, duplication, or disclosure by the 
	Government is subject to restrictions as set forth in subparagraph 
	(c)(1)(ii) of the Rights in Technical Data and Computer Software 
	clause at DFARS 52.227-7013 and in similar clauses in the FAR and 
	NASA FAR Supplement. */


/* tool_support.h */

#ifndef MAILTOOL_TOOL_SUPPORT_H
#define MAILTOOL_TOOL_SUPPORT_H

#include <xview/xview.h>
#include <xview/canvas.h>
#include <xview/sel_svc.h>
#include "header.h"

extern void force_repaint_from_line(int line, int force_clear, HeaderDataPtr hd);
extern void force_repaint_on_line(int line, HeaderDataPtr hd);
extern void mt_shift_nlines(int line, int n, HeaderDataPtr hd);
extern void mt_build_cursors(void);
extern void mt_load_headers(HeaderDataPtr hd);
extern void mt_append_headers(int make_visible, struct msg *msg, HeaderDataPtr hd);
extern void mt_update_headersw(struct msg *m, HeaderDataPtr hd);
extern void mt_save_curmsgs(HeaderDataPtr hd);
extern void mt_save_msg(struct view_window_list *vwl);
extern void mt_update_msgsw(struct msg *m, int force_view, int ask_save, int make_visible, HeaderDataPtr hd);
extern void mt_scroll_header(Xv_opaque win, int action);
extern void mt_resize_canvas(HeaderDataPtr hd);
extern void mt_update_folder_status(HeaderDataPtr hd) ;
extern void mt_announce_mail(void);
extern int mt_vs_confirm3(Frame frame, int optional, char *button1, char *button2, char *button3, char *fmt, ...);

extern int mt_vs_confirm(Frame frame, int optional, char *button1, char *button2, char *fmt, ...);
extern void mt_vs_exit(Frame frame, char *fmt, ...);
extern void mt_vs_warn(Frame frame, char *fmt, ...);
extern void mt_nomail_warning(HeaderDataPtr hd);
extern int mt_has_attachments(struct msg *m);
extern void mt_draw_glyph(struct msg *m, HeaderDataPtr hd, int x, int y);
extern void mt_redo_glyphs(struct msg *beginning, HeaderDataPtr hd);
extern void mt_clear_selected_glyphs(HeaderDataPtr hd);
extern void mt_toggle_glyph(int line, HeaderDataPtr hd);
extern void mt_header_seln_func_proc(char *client_data, Seln_function_buffer *args);
extern void mt_frame_msg(Frame frame, int beep, char *string);
extern Notify_value mt_canvas_interpose_proc(Canvas canvas, Event *event, Notify_arg arg, Notify_event_type type);
extern void mt_label_frame(Frame frame, char *string);
extern void mt_path_to_folder(char *path, char *new_folder);
extern int make_selection(HeaderDataPtr hd, Seln_rank rank);
extern int acquire_selection(Seln_rank rank);
extern void mt_lose_selections(HeaderDataPtr hd);
extern void mt_busy(Frame frame, int busy, char *message, int all_frames);
extern void mt_abort(char *msg, int arg1, int arg2, int arg3, int arg4);
extern char * mt_strip_leading_blanks(char *s);
extern char * mt_strip_trailing_blanks(char *s);
extern Menu mt_create_bogus_menu(HeaderDataPtr hd);
extern Menu_item mt_create_bogus_menu_item(HeaderDataPtr hd);




#endif	/* MAILTOOL_TOOL_SUPPORT_H */
