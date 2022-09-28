#
# Makefile for the OLIT periodic table demo
#
# @(#)Makefile	1.17 93/09/18

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

COMMAND			= olittable
DELIVERABLE		= deliverable
TOOL			= # None

SOURCES			= find_info_file.c \
			  table.main.c	\
			  table.widgets.c

OBJECTS                 = ${SOURCES:.c=.o}
HEADERS 		= mainhelp.h	\
			  table.h	\
			  widgetdata.h	\
			  widgethelp.h \
			  canvas.bitmap \
			  const.bitmap \
			  flat.bitmap \
			  flatmsg.bitmap \
			  icon.bitmap \
			  map.bitmap \
			  mapinvert.bitmap \
			  open.bitmap \
			  prim.bitmap \
			  shell.bitmap

DEFINES			= -DSYSV ${XCLIENTDEFINES}
INCLUDE                 = -I${SOURCEDIR}/include -I${XW_HOME}/include
LIBRARIES		= -lXol -lolgx -lXt -lXext -lX11 
SYSLIBS                 = -lintl -lw

HELP			= Ab-G.txt \
			  Bb.txt \
			  Ca.txt \
			  Cb.txt \
			  Cp.txt \
			  Da.txt \
			  Dt.txt \
			  Ex.txt \
			  F.txt \
			  Fc.txt \
			  Fe.txt \
			  Fic.txt \
			  Fn.txt \
			  Fnc.txt \
			  Fp.txt \
			  Ga.txt \
			  M.txt \
			  Mb-G.txt \
			  Mb.txt \
			  N.txt \
			  Ne.txt \
			  Nf.txt \
			  Ob-G.txt \
			  Pw.txt \
			  Rb.txt \
			  Rt.txt \
			  S.txt \
			  Sb.txt \
			  Sd.txt \
			  Sl.txt \
			  St.txt \
			  Sw.txt \
			  Te.txt \
			  Tf.txt \
			  Tl.txt \
			  motif.map.txt \
			  motifmap.txt \
			  writeup.txt

MAKE.install.target	= install_help \
			  strings

INSTALL.help.dir	= ${DEST.help.olittable}
INSTALL.directory       = ${DEST.demo}
INSTALL.tool.directory  =

include ${RULESET.command}

# Macros for string extraction
TEXTDOMAIN = SUNW_WST_OLITTABLE
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

