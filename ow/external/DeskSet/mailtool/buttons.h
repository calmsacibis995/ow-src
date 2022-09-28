/* Copyright 1992, Sun Microsystems Inc */

#pragma ident "@(#)buttons.h	1.1 92/12/07 SMI"

#ifndef mailtool_buttons_h
#define mailtool_buttons_h

#include "header.h"

/* buttons.h */

typedef void (*ButtonCallback)(void *callarg, int x, int y);


void commit_buttons(HeaderDataPtr);
void bind_button(HeaderDataPtr, int row, int col, char *label, char *name);
void bind_button_user(HeaderDataPtr, int row, int col, char *label,
	char *name, int user);
int define_sys_button(HeaderDataPtr, char *name, void (*notify)(), void *menu,
	char *helpid, void *client_data);
int get_button_at_position(int row, int col, char **label, char **name);
int user_button(char **argv);
void set_button_callback(HeaderDataPtr, int row, int col,
	ButtonCallback callback, void *callarg);




#endif /* mailtool_buttons_h */
