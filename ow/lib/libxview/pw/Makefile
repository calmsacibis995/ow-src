##
#
# Makefile for the pw package of xview
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
LIBPART			= pw
TOOL			=
DELIVERABLE		= deliverable
STATIC			= static
SHARED			= shared
SOURCES			= pw_btchrop.c	\
			  pw_cms.c	\
			  pw_get.c	\
			  pw_line.c	\
			  pw_plygon2.c	\
			  pw_plyline.c	\
			  pw_plypt.c	\
			  pw_put.c	\
			  pw_read.c	\
			  pw_traprop.c	\
			  pw_vector.c	\
			  xv_olgx.c	\
			  xv_text.c	\
			  xv_stencil.c	\
			  xv_rop.c	\
			  mem.c		\
			  mem_ops.c
SOURCES.sa		= 
OBJECTS			= ${SOURCES:%.c=%.o}
OBJECTS.sa		= ${SOURCES.sa:%.c=%.o}
HEADERS			= ${HEADERS.public} ${HEADERS.private}
HEADERS.public		= pixwin.h
HEADERS.private		= pw_impl.h	\
			  xv_color.h
DERIVEDFILES		=

INCLUDE			= -I${SOURCEDIR}/include -I${XW_HOME}/include

MAKE.install.target	= install_h

INSTALL.headers.dir	= ${DEST.include.xview}

PIC			= ${BIGPIC}

include ${RULESET.libchild}

