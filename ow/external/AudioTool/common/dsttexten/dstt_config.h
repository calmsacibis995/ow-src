/* Copyright (c) 1992 by Sun Microsystems, Inc. */

#ifndef _MULTIMEDIA_DSTT_CONFIG_H
#define	_MULTIMEDIA_DSTT_CONFIG_H

#ident	"@(#)dstt_config.h	1.2	93/02/22 SMI"

#include "dstt.h"

typedef	struct	msg
{
	char		*name;
	status_t	(*param)(Tt_message, status_t (*)(...));
	status_t	(*cb)(...);
	int		handle;
}Msg_t;

typedef	struct	message_set
{
	void			*key;
	Tt_pattern		pat;
	int			count;
	Msg_t			*list;
}Message_set_t;


typedef	struct	message_group
{
	Message_set_t		*(*find)(void *key, int create, struct message_group *group);
	void			*(*get_key)(Tt_message);
	void			(*rm)(void *);
	Tt_pattern		(*pattern)(void *);
	int			count;
	Message_set_t		**sets;
} Message_group_t;

extern	Message_group_t	*dstt_config_group(Msg_t *, int,
		Message_set_t *(*find)(void *, int, Message_group_t *),
		void *(get_key)(Tt_message),
		void (*rm)(void *),
		Tt_pattern (*pattern)(void *));

extern	Message_set_t *dstt_config_set(Msg_t *, int , Message_group_t *);

extern	int dstt_callback_register(Message_group_t *, void *, va_list *ap);

extern	int dstt_handle_register(Message_group_t *, void *, va_list *ap);

extern	Tt_message dstt_msg_receive(Tt_message);

#endif /* !_MULTIMEDIA_DSTT_CONFIG_H */
