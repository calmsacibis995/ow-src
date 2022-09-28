#
# Makefile for the OpenWindows OLWM Window Manager
#
# @(#)Makefile	26.12 90/11/12

#
# Copyright (c) 1990 by Sun Microsystems, Inc.
#

#	Sun design patents pending in the U.S. and foreign countries.
#	See LEGAL_NOTICE file for terms of the license.
#
# THIS FILE IS PROVIDED AS IS WITH NO WARRANTIES OF ANY KIND INCLUDING THE
# WARRANTIES OF DESIGN, MERCHANTIBILITY AND FITNESS FOR A PARTICULAR
# PURPOSE, OR ARISING FROM A COURSE OF DEALING, USAGE OR TRADE PRACTICE.
# 
# This file is provided with no support and without any obligation on the
# part of Sun Microsystems, Inc. to assist in its use, correction,
# modification or enhancement.
# 
# SUN MICROSYSTEMS, INC. SHALL HAVE NO LIABILITY WITH RESPECT TO THE
# INFRINGEMENT OF COPYRIGHTS, TRADE SECRETS OR ANY PATENTS BY THIS FILE
# OR ANY PART THEREOF.
# 
# In no event will Sun Microsystems, Inc. be liable for any lost revenue
# or profits or other special, indirect and consequential damages, even
# if Sun has been advised of the possibility of such damages.
# 
# Sun Microsystems, Inc.
# 2550 Garcia Avenue
# Mountain View, California  94043
#

SOURCEDIR		= ../..

include ${SOURCEDIR}/config/Master.cfg

COMMAND                 = owplaces
TOOL			=
DELIVERABLE		= deliverable
SOURCES                 = owplaces.c
OBJECTS                 = ${SOURCES:.c=.o}
OTHEROBJECTS		=
HEADERS                 = owplaces.h
DEFINES			= -DOW_I18N_L3 ${XCLIENTDEFINES}
MISC			= 

INCLUDE                 = -I${SOURCEDIR}/include	\
			  -I${OPENWINHOME}/include	\
			  -I${XW_HOME}/include
LIBRARIES               = -lXmu -lXt ${LIBS.i18nx} ${XCLIENTLIBS} \
			  ${I18NLIBS} ${INTLLIB} -lXau
SYSLIBS			=
debug := SYSLIBS	+= -lw

INSTALL.directory       = ${DEST.bin}
INSTALL.tool.directory  =

MAKE.install.target     = strings

TEXTDOMAIN		= ${COMMAND}
MSGUNIQ			= ${TOOL.bin}/msguniq

include ${RULESET.command}

saber_src saber_obj 	:= CFLAGS	= -g
saber_src 		:= SABER_TARGET	= ${SOURCES}
saber_obj 		:= SABER_TARGET	= ${OBJECTS}
SABER_SUPPRESS		= saber.suppress

saber_src saber_obj: 	${SABER_SUPPRESS}
	#source ${SABER_SUPPRESS}
	#use . ${DIR.debug}
	#setopt load_flags ${INCLUDE}
	#setopt program_name ${COMMAND}
	#load ${CFLAGS} ${CPPFLAGS} ${SABER_TARGET}
	#load ${LIBS.link.debug:sh} -lc

strings: ${SOURCES} ${HEADERS}
	@echo "Extracting Strings:"
	@${RM} ${DEST.locale.msg_templates}/${TEXTDOMAIN}.po
	@${RM} ${TEXTDOMAIN}.po
	@echo "domain \"${TEXTDOMAIN}\"" \
		>> ${DEST.locale.msg_templates}/${TEXTDOMAIN}.po
	-@for i in ${SOURCES} ; do \
	    echo "$$i"; \
	    ${CC} ${DEFINES} ${INCLUDE} -E -C $$i | ${XGETTEXT} \
		-c STRING_EXTRACTION -d${TEXTDOMAIN} -; \
	done; \

.KEEP_STATE:
