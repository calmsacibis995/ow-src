/*	@(#)mail_dstt.h 1.3 93/06/15 Copyright 1992 Sun Micro		*/

/*
 * Copyright (c) 1990 Sun Microsystems, Inc.
 *
 * New Tool Talk interface routines for mailtool.
 *
 */

/*
 * Each TT message has client data. The following structure
 * references that data.
 */

typedef struct  list {
	char		*msg;		/* TT message identifer */
	Attach_list	*al;		/* attachment list */
	Attach_node	*node;		/* attachment node */
	Selection	sel;		/* Selection ID */
	int		use_tooltalk;	/* support for old protocol */
	char		*toolID;	/* TT recipient identifier */
	int		delete;		/* TT told to delete this */
}list_t; 

