# @(#)Makefile	1.7 93/09/18 vkbd SMI
#
#         Copyright (C) 1991  Sun Microsystems, Inc
#                   All rights reserved.
#           Notice of copyright on this source code
#           product does not indicate publication.
# 
# RESTRICTED RIGHTS LEGEND: Use, duplication, or disclosure by 
# the U.S. Government is subject to restrictions as set forth 
# in subparagraph (c)(1)(ii) of the Rights in Technical Data
# and Computer Software Clause at DFARS 252.227-7013 (Oct. 1988) 
# and FAR 52.227-19 (c) (June 1987).
#
# Sun Microsystems, Inc.
# 2550 Garcia Avenue
# Mountain View, California 94043

SOURCEDIR		= ../..

include ${SOURCEDIR}/config/Master.cfg

COMMAND			= vkbd
DELIVERABLE		= deliverable
TOOL			= #None
#SOURCES		= vkbd.c vkbd_data.c util.c kcode_data.c
SOURCES			= vkbd.c vkbd_data.c kcode_data.c
OBJECTS			= ${SOURCES:.c=.o}
HEADERS			= vkbd.h

DEFINES			= -DSTANDALONE ${XCLIENTDEFINES}
INCLUDE			= -I${SOURCEDIR}/include -I${XW_HOME}/include
LIBRARIES		= -lxview -lolgx ${XCLIENTLIBS}
HELP			= vkbd.info

MAKE.install.target     = install_help
INSTALL.help.dir        = ${DEST.help}
INSTALL.directory	= ${DEST.xview}

include ${RULESET.command}
