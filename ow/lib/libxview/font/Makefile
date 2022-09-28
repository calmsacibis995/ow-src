##
#
# Makefile for the font package of xview
#
# @(#)Makefile	20.14 93/11/09
#
##

#
# Copyright (c) 1990 by Sun Microsystems, Inc.
#

SOURCEDIR		= ../../..

include ${SOURCEDIR}/config/Master.cfg

CFLAGS += -D_TS_ERRNO

LIBPARENT		= xview
LIBPART			= font
TOOL			=
DELIVERABLE		= deliverable
STATIC			= static
SHARED			= shared
SOURCES			= font.c	\
			  font_set.c	\
			  font_get.c	\
			  font_x.c	\
			  pf_text.c	\
			  pf_ttext.c	\
			  pf.c
SOURCES.sa		= font_data.c
OBJECTS			= ${SOURCES:%.c=%.o}
OBJECTS.sa		= ${SOURCES.sa:%.c=%.o}
HEADERS			= ${HEADERS.public} ${HEADERS.private}
HEADERS.public		= font.h
HEADERS.private		= font_impl.h
DERIVEDFILES		=

INCLUDE			= -I${SOURCEDIR}/include -I${XW_HOME}/include

MAKE.install.target	= install_h

INSTALL.headers.dir	= ${DEST.include.xview}

PIC			= ${BIGPIC}

include ${RULESET.libchild}

