##
#
# Makefile for the win package of xview
#
# @(#)Makefile	20.28 93/11/09
#
##

#
# Copyright (c) 1990 by Sun Microsystems, Inc.
#

SOURCEDIR		= ../../..

include ${SOURCEDIR}/config/Master.cfg

CFLAGS += -D_TS_ERRNO

LIBPARENT		= xview
LIBPART			= win
TOOL			=
DELIVERABLE		= deliverable
STATIC			= static
SHARED			= shared
SOURCES			= win_cursor.c	\
			  win_env.c	\
			  win_geom.c	\
			  win_global.c	\
			  win_cntral.c	\
			  win_input.c	\
			  win_treeop.c	\
			  win_bell.c	\
			  win_damage.c	\
			  win_compat.c	\
			  win_keymap.c
SOURCES.sa		= 
OBJECTS			= ${SOURCES:%.c=%.o}
OBJECTS.sa		= ${SOURCES.sa:%.c=%.o}
HEADERS			= ${HEADERS.public} ${HEADERS.private}
HEADERS.public		= win_enum.h	\
			  win_env.h	\
			  win_event.h	\
			  win_input.h	\
			  win_notify.h	\
			  win_screen.h	\
			  win_struct.h
HEADERS.private		= win_info.h	\
			  win_keymap.h
DERIVEDFILES		=

INCLUDE			= -I${SOURCEDIR}/include -I${XW_HOME}/include

MAKE.install.target	= install_h

INSTALL.headers.dir	= ${DEST.include.xview}

PIC			= ${BIGPIC}

include ${RULESET.libchild}
