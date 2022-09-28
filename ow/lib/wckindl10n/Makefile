##
#
# Makefile for the iso_8859_1 locale version of the wckind.so loadable
# module
#
# @(#)Makefile 1.1 93/05/14 ; SMI
#
##

#
# Copyright (c) 1993 by Sun Microsystems, Inc.
#

SOURCEDIR		= ../..

include ${SOURCEDIR}/config/Master.cfg

LIBRARY			= wckind
LIBRARY.tool		= ${LIBRARY}
LIBRARY.deliverable	= ${LIBRARY}
TOOL			= 
DELIVERABLE		= deliverable
STATIC			= 
SHARED			= shared
LIBMAJORVER		= 0
LIBMINORVER		= 0
SOURCES			= wckind.c

SOURCES.sa		=
OBJECTS			= ${SOURCES:%.c=%.o}
OBJECTS.sa		= ${SOURCES.sa:%.c=%.o}
HEADERS			= ${HEADERS.public} ${HEADERS.private}
HEADERS.public		=
HEADERS.private		= 
DERIVEDFILES		=

INCLUDE			= -I${SOURCEDIR}/include

INSTALL.directory	= ${DEST.locale.iso_8859_1.libs}
INSTALL.lintlib.dir	= 
INSTALL.tool.directory	= 

LIBPREFIX = 

include ${RULESET.library}

