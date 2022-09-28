#
# Makefile for the OpenWindows olmh program
#
# @(#)Makefile	1.19 95/02/22

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

COMMAND			= olmh
DELIVERABLE		= deliverable
TOOL			= # None

SOURCES			= bbox.c	\
			  command.c	\
			  compfuncs.c	\
			  folder.c	\
			  icon.c	\
			  init.c	\
			  main.c	\
			  menu.c	\
			  mlist.c	\
			  msg.c		\
			  pick.c	\
			  popup.c	\
			  screen.c	\
			  toc.c		\
			  tocfuncs.c	\
			  tocutil.c	\
			  util.c	\
			  viewfuncs.c	\
			  ScanDir.c	\
			  enconv.c	\
			  mclist.c

#			  tsource.c
#			  tsource.h
#			  tsourceP.h


OBJECTS                 = ${SOURCES:.c=.o}
HEADERS 		= actions.h	\
			  bbox.h	\
			  bboxint.h	\
			  externs.h	\
			  globals.h	\
			  mlist.h	\
			  msg.h		\
			  toc.h		\
			  tocintrnl.h	\
			  tocutil.h	\
			  xmh.h		\
			  newmail.bit	\
			  nomail.bit	\
			  enconv.h	\
			  mclist.h

DEFINES			= -DSYSV -DSVR4 ${XCLIENTDEFINES}
INCLUDE                 = -I${SOURCEDIR}/include -I${XW_HOME}/include
LIBRARIES               = -lXol -lolgx -lXt -lXext -lX11
SYSLIBS                 = -lnsl -lintl -lw -ldl
DERIVEDFILES            =

INSTALL.directory       = ${DEST.demo}
INSTALL.tool.directory  =
MAKE.install.target	= strings

include ${RULESET.command}

# Macros for string extraction
TEXTDOMAIN = SUNW_WST_OLMH
# XGETTEXT = /usr/bin/xgettext -j -n -m "" -p${DEST.locale.msg_templates}

strings: ${SOURCES} ${HEADERS}
	-@echo "Extracting Strings:"
	@${RM} ${DEST.locale.msg_templates}/${TEXTDOMAIN}.po \
		${DEST.locale.msg_templates}/messages.po
	@touch ${DEST.locale.msg_templates}/${TEXTDOMAIN}.po
	-@for i in ${SOURCES} ; do \
		echo "$$i"; \
		${CC} ${DEFINES} ${INCLUDE} -E -C $$i | ${XGETTEXT} -; \
	done

