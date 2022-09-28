# Makefile for the XView selection service
#
# @(#)Makefile	1.7 93/09/18

# Copyright (c) 1990 by Sun Microsystems, Inc.

SOURCEDIR		= ../..

include ${SOURCEDIR}/config/Master.cfg

COMMAND			= xv_get_sel
DELIVERABLE		= deliverable
TOOL			= #None
SOURCES			= xv_get_sel.c		\
			  sel_writable_data.c
OBJECTS			= ${SOURCES:.c=.o}
HEADERS			= seln_svc.h
HELP			=
DERIVEDFILES		= sel_writable_data.c	\
			  seln_svc.h

DEFINES			= -DSTANDALONE ${XCLIENTDEFINES}
INCLUDE			= -I${SOURCEDIR}/include		\
			  -I${SOURCEDIR}/lib/libxview/sel	\
			  -I${XW_HOME}/include
LIBRARIES		= -lxview -lolgx -lXau ${XCLIENTLIBS}

INSTALL.directory	= ${DEST.xview}

include ${RULESET.command}
