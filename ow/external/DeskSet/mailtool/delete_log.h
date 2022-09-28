/* Copyright 1992, Sun Microsystems Inc */

#pragma ident "@(#)delete_log.h	1.1 92/12/14 SMI"

/* delete_log.h */

#ifndef mailtool_delete_log_h
#define mailtool_delete_log_h

#include "../maillib/obj.h"
#include "header.h"

void mt_log_transaction(struct folder_obj *, struct msg *, int delete);
void mt_clear_transaction_log(void);
void mt_flush_transaction_log(void);
void mt_process_transaction_log(HeaderDataPtr);


#endif /* mailtool_delete_log_h */
