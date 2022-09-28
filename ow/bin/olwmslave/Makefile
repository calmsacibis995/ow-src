#
# Makefile for the OpenWindows olwmslave program
#
# @(#)Makefile	1.25 04/06/94

#
# Copyright (c) 1990 by Sun Microsystems, Inc.
#

#
# This file is a product of Sun Microsystems, Inc. and is provided for
# unrestricted use provided that this legend is included on all tape
# media and as a part of the software program in whole or part.  Users
# may copy or modify this file without charge, but are not authorized to
# license or distribute it to anyone else except as part of a product
# or program developed by the user.
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

COMMAND                 = olwmslave
TOOL			=
DELIVERABLE		= deliverable
SOURCES                 = cmdstream.c \
			  help_file.c \
			  helprecv.c \
			  helpwin.c \
			  mem.c \
			  olwmslave.c \
			  propsrecv.c \
			  propswin.c
OBJECTS                 = ${SOURCES:.c=.o}

HFILES.olwmslave.L3	=
HFILES.olwmslave.L4	= i18n.h

HEADERS                 = cmdstream.h \
			  helpcmd.h \
			  mem.h \
			  olwmslave.h \
			  propscmd.h \
			  ${HFILES.olwmslave.${I18N.level}}

HELP			= olwm.info workspace.info window.info
DEFINES                 = -DOLWMSLAVE -DOW_I18N_L3 ${XCLIENTDEFINES}
INCLUDE                 = -I${SOURCEDIR}/include	\
			  -I${OPENWINHOME}/include	\
			  -I${XW_HOME}/include
LIBRARIES               = -lxview -lolgx ${LIBS.i18nx} ${XCLIENTLIBS} \
				${INTLLIB} -lXau 
SYSLIBS                 = -lm
debug := SYSLIBS	+= -lw

INSTALL.directory       = ${DEST.bin}
INSTALL.tool.directory  =
INSTALL.help.dir	= ${DEST.help}

MAKE.install.target	= install_help strings

# Macros for string extraction

TEXTDOMAIN              = olwmslave

#SHARED_SRC		= mem.c mem.h
#SHARED_DIR		= ../olwm
#${SHARED_SRC}		:= SCCSFLAGS += -d${SHARED_SRC}

include ${RULESET.command}

saber_src saber_obj 	:= CFLAGS	= -g
saber_src 		:= SABER_TARGET = ${SOURCES}
saber_obj 		:= SABER_TARGET = ${OBJECTS}
LIBRARY			= -L${OPENWINHOME}/lib -L${XVIEWHOME}/lib
SABER_SUPPRESS		= saber.suppress

saber_src saber_obj:	${SABER_SUPPRESS}
	#source ${SABER_SUPPRESS}
	#use . ${DIR.targetarch}/debug
	#setopt load_flags ${INCLUDE}
	#load ${INCLUDE} ${CFLAGS} ${SABER_TARGET}
	#load ${LIBRARY} ${LIBRARIES} ${SYSLIBS}

strings: ${SOURCES} ${HEADERS} 
	@echo "Extracting Strings:"
	@${RM} ${DEST.locale.msg_templates}/${TEXTDOMAIN}.po
	@${RM} ${TEXTDOMAIN}.po
	@echo "domain \"${TEXTDOMAIN}\"" \
		>> ${DEST.locale.msg_templates}/${TEXTDOMAIN}.po
	-@for i in ${SOURCES} ; do \
	    echo "$$i"; \
	    ${CC} ${DEFINES} ${INCLUDE} -E -C $$i | ${XGETTEXT} \
		-d${TEXTDOMAIN} - ; \
	done
