##
#
# Makefile for the window package of xview
#
# @(#)Makefile	20.17 93/11/09
#
##

#
# Copyright (c) 1990 by Sun Microsystems, Inc.
#

SOURCEDIR		= ../../..

include ${SOURCEDIR}/config/Master.cfg

CFLAGS += -D_TS_ERRNO

LIBPARENT		= xview
LIBPART			= window
TOOL			=
DELIVERABLE		= deliverable
STATIC			= static
SHARED			= shared
SOURCES			= window.c	\
			  window_get.c	\
			  window_set.c	\
			  windowlayt.c	\
			  windowutil.c	\
			  window_cms.c	\
			  windowcmpt.c	\
			  windowdrop.c
SOURCES.sa		= windowdata.c
OBJECTS			= ${SOURCES:%.c=%.o}
OBJECTS.sa		= ${SOURCES.sa:%.c=%.o}
HEADERS			= ${HEADERS.public} ${HEADERS.private}
HEADERS.public		= window.h
HEADERS.private		= windowimpl.h
DERIVEDFILES		=

INCLUDE			= -I${SOURCEDIR}/include -I${XW_HOME}/include

MAKE.install.target	= install_h

INSTALL.headers.dir	= ${DEST.include.xview}

PIC			= ${BIGPIC}

include ${RULESET.libchild}
