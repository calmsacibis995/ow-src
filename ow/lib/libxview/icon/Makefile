##
#
# Makefile for the icon package of xview
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
LIBPART			= icon
TOOL			=
DELIVERABLE		= deliverable
STATIC			= static
SHARED			= shared
SOURCES			= icon.c	\
			  icon_obj.c	\
			  icon_load.c	\
			  icon_input.c
SOURCES.sa		= icon_data.c
OBJECTS			= ${SOURCES:%.c=%.o}
OBJECTS.sa		= ${SOURCES.sa:%.c=%.o}
HEADERS			= ${HEADERS.public} ${HEADERS.private}
HEADERS.public		= icon.h	\
			  icon_load.h
HEADERS.private		= icon_impl.h
DERIVEDFILES		=

INCLUDE			= -I${SOURCEDIR}/include -I${XW_HOME}/include

MAKE.install.target	= install_h

INSTALL.headers.dir	= ${DEST.include.xview}

PIC			= ${BIGPIC}

include ${RULESET.libchild}

