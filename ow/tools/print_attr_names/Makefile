##
#
# Makefile for the XView print_attr_names program
#
# @(#)Makefile	1.33 93/09/18
#
##

#
# Copyright (c) 1990 by Sun Microsystems, Inc.
#

SOURCEDIR		= ../..

include ${SOURCEDIR}/config/Master.cfg

COMMAND			= print_attr_names
DELIVERABLE		= #None
TOOL			= tool
SOURCES			= print_attr_names.c
OBJECTS			= ${SOURCES:.c=.o}
HEADERS			= #None
HELP			= #None

INCLUDE			= -I${SOURCEDIR}/include -I${XW_HOME}/include
DEFINES			= 
LIBRARIES		=
SYSLIBS			=
DERIVEDFILES		=

INSTALL.directory	=
INSTALL.tool.directory	= ${TOOL.bin}

include ${RULESET.command}

${OBJECTS.optimized} \
${OBJECTS.debug} \
${OBJECTS.gprof} \
${OBJECTS.tool.optimized} \
${OBJECTS.tool.debug} \
${OBJECTS.tool.gprof}: ${SOURCEDIR}/include/xview/drawable.h

${SOURCEDIR}/include/xview/drawable.h:
	(cd ${SOURCEDIR}/lib/libxview; make install_headers)

