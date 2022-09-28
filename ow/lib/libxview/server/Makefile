##
#
# Makefile for the server package of xview
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
LIBPART			= server
TOOL			=
DEFINES			= -DOS_HAS_LOCALE
DELIVERABLE		= deliverable
STATIC			= static
SHARED			= shared
SOURCES			= svr_x.c	\
			  svr_get.c	\
			  svr_set.c	\
			  svr_atom.c    \
			  svr_parse.c    \
			  svr_rodata.c
SOURCES.sa		= server.c	\
			  svr_data.c
OBJECTS			= ${SOURCES:%.c=%.o}
OBJECTS.sa		= ${SOURCES.sa:%.c=%.o}
HEADERS			= ${HEADERS.public} ${HEADERS.private}
HEADERS.public		= server.h
HEADERS.private		= svr_impl.h	\
			  svr_atom.h	\
			  svr_kmdata.h
DERIVEDFILES		=

INCLUDE			= -I${SOURCEDIR}/include -I${XW_HOME}/include

MAKE.install.target	= install_h

INSTALL.headers.dir	= ${DEST.include.xview}

PIC			= ${BIGPIC}

include ${RULESET.libchild}
${DIR.shared}/svr_rodata.o :=    CONSTDATA = ${f.CONSTDATA}
