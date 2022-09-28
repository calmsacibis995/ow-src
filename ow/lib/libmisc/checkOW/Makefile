# @(#)Makefile	1.2 91/07/19 lib/libmisc/checkOW SMI
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
#

SOURCEDIR		= ../../..

include ${SOURCEDIR}/config/Master.cfg

COMMAND                 = checkOW
DELIVERABLE             = deliverable
TOOL                    = # None
SOURCES                 = checkOW.c
OBJECTS                 = ${SOURCES:.c=.o}
HEADERS                 =
DEFINES                 = ${XCLIENTDEFINES}
INCLUDE                 = -I${SOURCEDIR}/include
LIBRARIES               = ${XCLIENTLIBS:-lX11=}
SYSLIBS			=
DERIVEDFILES		=

INSTALL.directory       = ${DEST.lib}
INSTALL.tool.directory  =

include ${RULESET.command}
