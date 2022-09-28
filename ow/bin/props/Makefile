# Makefile for the OpenWindows props program
#
# @(#)Makefile 1.16 93/05/11

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


SOURCEDIR		= ../..

include ${SOURCEDIR}/config/Master.cfg

COMMAND			= props
TOOL			=
DELIVERABLE		= deliverable
SOURCES			= props.c \
			  color.c \
			  fonts.c \
			  helpfile.c \
			  keyboard.c \
			  locale.c \
			  menu.c \
			  preview.c \
			  misc.c \
			  mouse.c \
			  wksplist.c \
			  wkspmenuentry.c \
			  wkspreadmenu.c \
			  wkspwritemenu.c \
			  wkspmenu.c \
			  OWFsetDB.c

OBJECTS                 = ${SOURCES:.c=.o}

HEADERS 		= props.h \
			  propsP.h \
			  color.h \
			  fonts.h \
			  preview.h \
			  helpfile.h \
			  wksplist.h \
			  wkspmenuentry.h \
			  wkspreadmenu.h \
			  wkspwritemenu.h \
			  OWFsetDB.h \
			  OWFsetDBI.h \
			  default.xbm \
			  empty.xbm

#CFLAGS		       += -Xc -v -xstrconst
CFLAGS		       += -Xc -v

DEFINES			= -DFUNCPROTO=0xff ${XCLIENTDEFINES}
INCLUDE                 = -I${SOURCEDIR}/include	\
			  -I${OPENWINHOME}/include	\
			  -I${XW_HOME}/include
LIBRARIES               = -lXol -lolgx -lXt -lXext ${XCLIENTLIBS}
SYSLIBS                 = -lm -lintl -lw

HELP			= props.info
APPDEFAULTS		= Props.ad 

INSTALL.directory       = ${DEST.bin}
INSTALL.tool.directory  =
INSTALL.help.dir	= ${DEST.help}
INSTALL.appdefaults.dir	= ${DEST.app-defaults}

MAKE.install.target	= install_help install_appdefaults

include ${RULESET.command}

${DIR.optimized}/OWFsetDB.o: OWFsetDB.c
	${CC} ${CPPFLAGS.debug} ${CPPFLAGS} ${CFLAGS} ${CFLAGS.debug} -c -o ${DIR.optimized}/OWFsetDB.o OWFsetDB.c

# This is for pmake
wkspmenu.o::	wkspmenu.c empty.xbm default.xbm

.KEEP_STATE:

