##
#
# Makefile for the file_chooser packages of xview
#
# @(#)Makefile	1.4 93/11/09#
##

#
# Copyright (c) 1992 by Sun Microsystems, Inc.
#

SOURCEDIR		= ../../..

include ${SOURCEDIR}/config/Master.cfg

CFLAGS += -D_TS_ERRNO


LIBPARENT		= xview
LIBPART			= file_chooser
TOOL			=
DELIVERABLE		= deliverable
STATIC			= static
SHARED			= shared
SOURCES			= file_chsr.c	\
			fc_layout.c 	\
			file_list.c 	\
			path.c		\
			xv_path_util.c	\
			hist_list.c	\
			hist_menu.c
SOURCES.sa		= fchsr_data.c	\
			flist_data.c	\
			path_data.c	\
			hist_data.c
OBJECTS			= ${SOURCES:%.c=%.o}
OBJECTS.sa		= ${SOURCES.sa:%.c=%.o}
HEADERS			= ${HEADERS.public} ${HEADERS.private}
HEADERS.public		= file_chsr.h	\
			file_list.h 	\
			path.h 		\
			hist.h
HEADERS.private		= fchsr_impl.h	\
			flist_impl.h	\
			xv_path_util.h	\
			path_impl.h	\
			hist_impl.h
DERIVEDFILES		=

INCLUDE			= -I${SOURCEDIR}/include -I${XW_HOME}/include

MAKE.install.target	= install_h

INSTALL.headers.dir	= ${DEST.include.xview}

PIC			= ${BIGPIC}

include ${RULESET.libchild}

