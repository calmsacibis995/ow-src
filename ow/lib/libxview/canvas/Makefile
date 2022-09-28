##
#
# Makefile for the canvas package of xview
#
# @(#)Makefile	20.22 93/11/09
#
##

#
# Copyright (c) 1990 by Sun Microsystems, Inc.
#

SOURCEDIR		= ../../..

include ${SOURCEDIR}/config/Master.cfg

CFLAGS += -D_TS_ERRNO

LIBPARENT		= xview
LIBPART			= canvas
TOOL			=
DELIVERABLE		= deliverable
STATIC			= static
SHARED			= shared

CFILES.canvas.L3	=
CFILES.canvas.L4	= cnvs_cb.c

SOURCES			= canvas.c	\
			  cnvs_set.c	\
			  cnvs_get.c	\
			  cnvs_scrol.c	\
			  cnvs_resze.c	\
			  cnvs_input.c	\
			  cnvs_view.c	\
			  ${CFILES.canvas.${I18N.level}}
SOURCES.sa		= cnvs_data.c
OBJECTS			= ${SOURCES:%.c=%.o}
OBJECTS.sa		= ${SOURCES.sa:%.c=%.o}
HEADERS			= ${HEADERS.public} ${HEADERS.private}
HEADERS.public		= canvas.h
HEADERS.private		= cnvs_impl.h
DERIVEDFILES		=

INCLUDE			= -I${SOURCEDIR}/include -I${XW_HOME}/include

MAKE.install.target	= install_h

INSTALL.headers.dir	= ${DEST.include.xview}

PIC			= ${BIGPIC}

include ${RULESET.libchild}

