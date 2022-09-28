##
#
# Makefile for the dnd package of xview
#
# @(#)Makefile	1.5 93/11/09
#
##

#
# Copyright (c) 1990 by Sun Microsystems, Inc.
#

SOURCEDIR		= ../../..

include ${SOURCEDIR}/config/Master.cfg

CFLAGS += -D_TS_ERRNO

LIBPARENT		= xview
LIBPART			= dnd
TOOL			=
DELIVERABLE		= deliverable
STATIC			= static
SHARED			= shared
SOURCES			= site.c	\
			  site_pblc.c	\
			  dnd_decode.c	\
			  dnd.c		\
			  dnd_pblc.c	\
			  dndutil.c	\
			  dnd_dsdm.c
SOURCES.sa		= dnd_data.c	\
			  site_data.c
OBJECTS			= ${SOURCES:%.c=%.o}
OBJECTS.sa		= ${SOURCES.sa:%.c=%.o}
HEADERS			= ${HEADERS.public} ${HEADERS.private}
HEADERS.public		= dragdrop.h
HEADERS.private		= site_impl.h	\
			  dndimpl.h
DERIVEDFILES		=

INCLUDE			= -I${SOURCEDIR}/include -I${XW_HOME}/include

MAKE.install.target	= install_h

INSTALL.headers.dir	= ${DEST.include.xview}

PIC			= ${BIGPIC}

include ${RULESET.libchild}

