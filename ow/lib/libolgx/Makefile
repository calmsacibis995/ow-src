##
#
# Makefile for the olgx library
#
# @(#)Makefile	1.28 93/11/16
#
##

#
# Copyright (c) 1990 by Sun Microsystems, Inc.
#

SOURCEDIR		= ../..

include ${SOURCEDIR}/config/Master.cfg

CFLAGS += -D_TS_ERRNO

LIBRARY			= olgx
LIBRARY.tool		= ${LIBRARY}
LIBRARY.deliverable	= ${LIBRARY}
TOOL			= tool
DELIVERABLE		= deliverable
STATIC			= static
SHARED			= shared
LIBMAJORVER		= 3
LIBMINORVER		= 1
SOURCES			= ol_button.c	\
			  ol_color.c	\
			  ol_draw.c	\
			  ol_init.c	\
			  ol_misc.c	\
			  ol_sb.c	\
			  ol_slider.c
MAPFILE			= mapfile.libolgx
SVR4-REORDER		= ${MAPFILE}
SOURCES.sa		=
OBJECTS			= ${SOURCES:%.c=%.o}
OBJECTS.sa		= ${SOURCES.sa:%.c=%.o}
HEADERS			= ${HEADERS.public} ${HEADERS.private}
HEADERS.public		=
HEADERS.private		= busy.h grey.h olgx_impl.h
DERIVEDFILES		=

INCLUDE			= -I${SOURCEDIR}/include -I${XW_HOME}/include

INSTALL.directory	= ${DEST.lib}
INSTALL.lintlib.dir	= ${DEST.lib.lint}
INSTALL.tool.directory	= ${DEST.lib}


include ${RULESET.library}

