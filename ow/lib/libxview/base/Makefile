##
#
# Makefile for the base package of xview
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
LIBPART			= base
TOOL			=
DELIVERABLE		= deliverable
STATIC			= static
SHARED			= shared
SOURCES			= xv.c		\
			  xv_init.c	\
			  xv_init_x.c	\
			  generic.c	\
			  drawable.c	\
			  xv_debug.c	\
			  xv_parse.c	\
			  xv_list.c	\
			  xv_error.c
SOURCES.sa		= base_data.c	\
			  xv_usage.c
OBJECTS			= ${SOURCES:%.c=%.o}
OBJECTS.sa		= ${SOURCES.sa:%.c=%.o}
HEADERS			= ${HEADERS.public} ${HEADERS.private}
HEADERS.public		= base.h	\
			  generic.h	\
			  pkg_public.h	\
			  pkg.h	\
			  xview_xvin.h	\
			  drawable.h	\
			  xv_error.h
HEADERS.private		= xv_debug.h	\
			  xv_list.h	\
			  draw_impl.h	\
			  gen_impl.h
DERIVEDFILES		=

INCLUDE			= -I${SOURCEDIR}/include -I${XW_HOME}/include

MAKE.install.target	= install_h

INSTALL.headers.dir	= ${DEST.include.xview}

PIC			= ${BIGPIC}

include ${RULESET.libchild}

