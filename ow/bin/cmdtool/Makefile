##
#
# Makefile for the XView cmdtool & shelltool
#
# @(#)Makefile	1.28 94/04/06
#
##

#
# Copyright (c) 1990 by Sun Microsystems, Inc.
#

SOURCEDIR		= ../..

include ${SOURCEDIR}/config/Master.cfg

COMMAND			= cmdtool
DELIVERABLE		= deliverable
TOOL			= #None
SOURCES			= cmdtool.c
OBJECTS			= ${SOURCES:.c=.o}
HEADERS			= #None
HELP			= #None

INCLUDE			= -I${SOURCEDIR}/include -I${XW_HOME}/include
DEFINES			= ${XCLIENTDEFINES}
LIBRARIES		= -lxview -lolgx ${LIBS.i18nx} ${LIB.mle} \
			  ${XCLIENTLIBS} ${INTLLIB}
SYSLIBS			= #None
DERIVEDFILES		=

INSTALL.directory	= ${DEST.xview}

MAKE.install.target	= linktoshelltool strings

# Macros for string extraction

TEXTDOMAIN              = SUNW_WST_CMDTOOL
strings                := DEFINES += -DXGETTEXT

include ${RULESET.command}

linktoshelltool: 
	@echo "${INSTALL.directory}/shelltool	=> cmdtool"
	@${CD} ${INSTALL.directory}; \
	 ${RM} shelltool; \
	 ${LN} ./cmdtool shelltool

strings: ${SOURCES} ${HEADERS} 
	-@echo "Extracting Strings:"
	@${RM} ${DEST.locale.msg_templates}/${TEXTDOMAIN}.po
	-@for i in ${SOURCES} ; do \
	    echo "$$i"; \
	    ${CC} ${DEFINES} ${INCLUDE} -E -C $$i | ${XGETTEXT} - ; \
	done
