##
#
# Makefile for the notice package of xview
#
# @(#)Makefile	20.19 93/11/09
#
##

#
# Copyright (c) 1990 by Sun Microsystems, Inc.
#

SOURCEDIR		= ../../..

include ${SOURCEDIR}/config/Master.cfg

CFLAGS += -D_TS_ERRNO

LIBPARENT		= xview
LIBPART			= notice
TOOL			=
DELIVERABLE		= deliverable
STATIC			= static
SHARED			= shared
SOURCES			= notice.c	\
			  notice_set.c	\
			  notice_get.c	\
			  notice_dstry.c	\
			  notice_pt.c	\
			  notice_data.c	\
			  notice_itm.c	\
			  notice_ol.c
SOURCES.sa		= notice_data.c
OBJECTS			= ${SOURCES:%.c=%.o}
OBJECTS.sa		= ${SOURCES.sa:%.c=%.o}
HEADERS			= ${HEADERS.public} ${HEADERS.private}
HEADERS.public		= notice.h	\
			  alert.h
HEADERS.private		= noticeimpl.h
DERIVEDFILES		=

INCLUDE			= -I${SOURCEDIR}/include -I${XW_HOME}/include

MAKE.install.target	= install_h

INSTALL.headers.dir	= ${DEST.include.xview}

PIC			= ${BIGPIC}

include ${RULESET.libchild}

