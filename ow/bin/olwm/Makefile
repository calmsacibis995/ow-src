#
# Makefile for the OpenWindows OLWM Window Manager
#
# @(#)Makefile	26.55 94/04/06

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

COMMAND                 = olwm
TOOL			=
DELIVERABLE		= deliverable

CFILES.olwm.L3		=
CFILES.olwm.L4		= fontset.c

SOURCES			= atom.c client.c cmdstream.c debug.c defaults.c \
			  dsdm.c environ.c error.c evbind.c events.c group.c \
			  helpsend.c i18n.c info.c kbdfuncs.c list.c mem.c \
			  menuacc.c menu.c moveresize.c olwm.c notice.c \
			  properties.c resources.c screen.c services.c \
			  selection.c slave.c slots.c st.c states.c usermenu.c \
			  usleep.c win.c winbusy.c winbutton.c wincolor.c \
			  winframe.c wingframe.c winicon.c winipane.c \
			  winmenu.c winnofoc.c winpane.c winpinmenu.c \
			  winpush.c winresize.c winroot.c \
			  ${CFILES.olwm.${I18N.level}}

OBJECTS                 = ${SOURCES:.c=.o}
OTHEROBJECTS		=
HEADERS                 = cmdstream.h debug.h defaults.h dsdm.h environ.h \
			  error.h events.h globals.h group.h helpcmd.h \
			  i18n.h kbdfuncs.h list.h mem.h menu.h notice.h \
			  olcursor.h ollocale.h olwm.h resources.h \
			  properties.h screen.h selection.h slots.h st.h \
			  win.h \
			  ${BITMAPS}

BITMAPS			= iconimage.h iconmask.h
MISC			= .olwmmenu Readme

# To disable menu shadows (for servers that do not support SaveUnder)
# remove the -DSHADOW option from DEFINES.
DEFINES			= # -DSHADOW
DEFINES			= ${XCLIENTDEFINES} -DOW_I18N_L3 -DALLPLANES -DSHAPE

INCLUDE                 = -I${SOURCEDIR}/include	\
			  -I${OPENWINHOME}/include	\
			  -I${XW_HOME}/include
LIBRARIES               = -lolgx ${LIBS.i18nx} -lXext ${XCLIENTLIBS} \
				${I18NLIBS} ${INTLLIB} 
SYSLIBS			= -lw -R/usr/openwin/lib

SVR4-REORDER 		= relink	
MAPFILE			= mapfile
REORDERLDFLAGS		= -M ${MAPFILE}

# Macros for string extraction

TEXTDOMAIN              = olwm_messages

# Additional modifications for building a debugging version.
debug := DEFINES	+= -DFUNCPROTO=0xff

# Malloc-debugging version.  Compile a debugging version, but also add
# the debugging version of malloc and turn 
mdebug: debug
mdebug := OTHEROBJECTS	=   /usr/lib/debug/malloc.o
mdebug := DEFINES	+= -DMALLOCDEBUG=2


INSTALL.directory       = ${DEST.bin}
INSTALL.tool.directory  =
MAKE.install.target     = strings

include ${RULESET.command}

relink: ${MAPFILE}
	$(CC) $(CFLAGS) $(CPPFLAGS) $(LDFLAGS) ${REORDERLDFLAGS} -o ${COMMAND.optimized} \
		${OBJECTS.optimized} \
		${LIBS.link.optimized:sh} \
		${DYNAMIC} ${SYSLIBS} ${STDSYSLIBS}
	mcs -d ${COMMAND.optimized}
	
tags: ${SOURCES} ${HEADERS}
	ctags -tw ${SOURCES} ${HEADERS}

saber_src saber_obj 	:= CFLAGS	= -g
saber_src 		:= SABER_TARGET	= ${SOURCES}
saber_obj 		:= SABER_TARGET	= ${OBJECTS}
SABER_SUPPRESS		= saber.suppress

saber_src saber_obj: 	${SABER_SUPPRESS}
	#source ${SABER_SUPPRESS}
	#use . ${DIR.debug}
	#setopt load_flags ${INCLUDE}
	#setopt program_name olwm
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
		-c STRING_EXTRACTION -d${TEXTDOMAIN} - ; \
	done

.KEEP_STATE:
