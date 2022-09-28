##
#
# Makefile for the sel package of xview
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
LIBPART			= sel
TOOL			=
DELIVERABLE		= deliverable
STATIC			= static
SHARED			= shared
SOURCES			= seln.c	\
			  sel_appl.c	\
			  sel_clnt.c	\
			  sel_common.c	\
			  sel_policy.c	\
			  sel_agent.c	\
			  sel_compat.c
SOURCES.sa		= seln_data.c
OBJECTS			= ${SOURCES:%.c=%.o}
OBJECTS.sa		= ${SOURCES.sa:%.c=%.o}
HEADERS			= ${HEADERS.public} ${HEADERS.private}
HEADERS.public		= selection.h	\
			  sel_attrs.h	\
			  sel_svc.h	\
			  seln.h	\
			  sel_compat.h
HEADERS.private		= seln_impl.h
DERIVEDFILES		=

INCLUDE			= -I${SOURCEDIR}/include -I${XW_HOME}/include

MAKE.install.target	= install_h

INSTALL.headers.dir	= ${DEST.include.xview}

PIC			= ${BIGPIC}

include ${RULESET.libchild}
