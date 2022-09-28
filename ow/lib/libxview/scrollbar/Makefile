##
#
# Makefile for the scrollbar package of xview
#
# @(#)Makefile	1.23 93/11/09
#
##

#
# Copyright (c) 1990 by Sun Microsystems, Inc.
#

SOURCEDIR		= ../../..

include ${SOURCEDIR}/config/Master.cfg

CFLAGS += -D_TS_ERRNO

LIBPARENT		= xview
LIBPART			= scrollbar
TOOL			=
DELIVERABLE		= deliverable
STATIC			= static
SHARED			= shared
SOURCES			= sb_compat.c	\
			  sb_destroy.c	\
			  sb_get.c	\
			  sb_pos.c	\
			  sb_set.c	\
			  sb_event.c	\
			  sb_paint.c	\
			  sb_scroll.c	\
			  sb.c
SOURCES.sa		= sb_data.c
OBJECTS			= ${SOURCES:%.c=%.o}
OBJECTS.sa		= ${SOURCES.sa:%.c=%.o}
HEADERS			= ${HEADERS.public} ${HEADERS.private}
HEADERS.public		= scrollbar.h
HEADERS.private		= sb_impl.h
DERIVEDFILES		=

INCLUDE			= -I${SOURCEDIR}/include -I${XW_HOME}/include

MAKE.install.target	= install_h

INSTALL.headers.dir	= ${DEST.include.xview}

PIC			= ${BIGPIC}

include ${RULESET.libchild}
