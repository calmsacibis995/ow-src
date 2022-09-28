/* Copyright 1992, Sun Microsystems Inc */

#pragma ident "@(#)mail.h	1.2 93/04/23 SMI"

#ifndef MAILTOOL_MAIL_H
#define MAILTOOL_MAIL_H

#include <xview/textsw.h>
#include "header.h"

extern char *mt_mailbox;	/* file name of user's mailbox */



int mt_idle_mail(HeaderDataPtr);
void mt_stop_mail(int doabort);
int mt_check_mail_box(HeaderDataPtr, int start_up);
char *makeheader(struct msg *m);
void mt_get_headers(HeaderDataPtr, struct msg *start_msg, int change_current);
void mt_get_folder(HeaderDataPtr);
char **mt_get_folder_list(char *path_string, char *fdir_name, char **strtabp, int *acp, int dir_first);
void mt_reply_msg(struct msg *m, Textsw textsw, int all, int orig, struct reply_panel_data *ptr);
void mt_include_msg(struct msg *m, Textsw textsw, int intented, struct reply_panel_data *rpd);
int mt_insert_textsw(char *buffer, int len, Textsw textsw);
int mt_copy_msg(HeaderDataPtr, struct msg *m, char *file, int give_warn);
void mt_print_textsw(HeaderDataPtr, struct msg *m, Textsw, int ignore);
void mt_error_handle(HeaderDataPtr);
void mt_del_msg(struct msg *m);
void mt_truncate_current_folder(HeaderDataPtr);
void mt_undel_msg(struct msg *m);
int mt_set_folder(HeaderDataPtr, char *path, int *good_switch_p);
int mt_incorporate_mail(HeaderDataPtr);
struct msg *mt_next_msg(HeaderDataPtr, struct msg *msg, int consider_selection);
void mt_set_state_no_mail(HeaderDataPtr, int check_mbox);
int get_time(struct msg *m);
struct msg *FIRST_NDMSG(struct folder_obj *folder);
struct msg *LAST_NDMSG(struct folder_obj *folder);
struct msg *NEXT_NDMSG(struct msg *msg);
struct msg *PREV_NDMSG(struct msg *msg);
void CLEAR_MSGS(HeaderDataPtr);
struct msg *NUM_TO_MSG(HeaderDataPtr, int msg_num);
struct msg *LINE_TO_MSG(HeaderDataPtr, int line_num);
void nomem(void);
void mt_text_insert(Textsw, char *data, int len);
void mt_text_clear_error(Textsw);




#endif /* MAILTOOL_MAIL_H */
