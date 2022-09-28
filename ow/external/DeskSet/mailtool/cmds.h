/* Copyright 1992, Sun Microsystems Inc */

#pragma ident "@(#)cmds.h	1.3 93/02/11 SMI"


/* cmds.h */

#ifndef mailtool_cmds_h
#define mailtool_cmds_h


#include <xview/xview.h>
#include "header.h"


void mt_commit_proc_hd(struct header_data *);
void mt_commit_proc(Menu, Menu_item);
void mt_comp_proc(Menu, Menu_item);
void mt_copy_proc(Menu, Menu_item);
void mt_copyshelf_proc(Menu, Menu_item);
void mt_cut_proc(Menu, Menu_item);
void mt_do_deliver(Menu, Menu_item);
void mt_del_proc_hd(HeaderDataPtr);
void mt_del_proc(Menu, Menu_item, Event *);
void mt_done_proc_hd(HeaderDataPtr);
void mt_del_trash(HeaderDataPtr, char *trash_name);
void mt_include_proc(Menu, Menu_item);
void mt_next_proc(Menu, Menu_item);
void mt_new_mail_proc(Menu, Menu_item);
void mt_prev_proc(Menu, Menu_item);
void mt_view_message(HeaderDataPtr, int abbrev_headers);
void mt_print_proc(Menu, Menu_item);
void mt_reply_proc(Menu, Menu_item);
void mt_save_proc(Menu, Menu_item);
void mt_sort_proc(Menu, Menu_item);
void mt_undel_last(Menu, Menu_item);
void mt_do_undel(HeaderDataPtr, struct msg *);
void mt_new_folder(HeaderDataPtr, char *file, int quietly, int incorporate,
	int no_views, int addtohistorylist);
struct msg *mt_any_selected(HeaderDataPtr);
void mt_set_new_folder(HeaderDataPtr);
int mt_attachment_in_msg(struct msg *msg);
void mt_expand_foldername(char *file, char *full_path);
void mt_getfolderdir(char *path);
int mt_check_and_crate(char *path, int directory, Frame, int confirm);
void		mt_do_del(struct header_data *hd, Msg_attr flag);
void		mt_do_save(struct header_data *hd, int del, Msg_attr flag,
			char *file);

#ifdef PEM
mt_decrypt_PEM_message(HeaderDataPtr);
void mt_do_deliver_pem(Menu, Menu_item);
#endif PEM

#ifdef MULTIPLE_FOLDERS
void mt_set_new_folder1(HeaderDataPtr);
#endif



#endif /* mailtool_cmds_h */
