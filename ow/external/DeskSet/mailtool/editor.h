/*	@(#)editor.h 3.1 IEI SMI	*/

/*
 * Copyright (c) 1991 by Sun Microsystems, Inc.
 */

/*
 * Interface file for editor.c
 */
extern int	mt_start_editor();
extern void	mt_stop_editor();

#define	MT_DEFAULT_EDITOR	"vi"
#define MT_DEFAULT_TMP		"/tmp"
#define MT_DEFAULT_SHELL	"sh"
