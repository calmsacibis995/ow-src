#
# @(#)Makefile	1.4 93/09/18
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

COMMAND                 = dsdm
TOOL			=
DELIVERABLE		= deliverable
SOURCES                 = dsdm.c
OBJECTS                 = ${SOURCES:.c=.o}
OTHERSOURCES		= site.c source.c
OTHERPROGS		= ${OTHERSOURCES:.c=}
HEADERS                 = 

INCLUDE                 = -I${SOURCEDIR}/include -I${XW_HOME}/include
LIBRARIES               = ${XCLIENTLIBS} -lXau
SYSLIBS			=

INSTALL.directory       = ${DEST.bin}
INSTALL.tool.directory  =

include ${RULESET.command}

other: ${OTHERPROGS}

other-debug: ${OTHERPROGS}

other-debug		:= CFLAGS = -g

.KEEP_STATE:
