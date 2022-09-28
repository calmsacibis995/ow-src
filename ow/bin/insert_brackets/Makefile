
#
# Makefile for the OpenWindows insert_brackets program
#
# @(#)Makefile 23.7 92/01/23

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

COMMAND			= insert_brackets
DELIVERABLE		= deliverable
TOOL			= # None
SOURCES			= insert_brackets.c bracket_misc.c
OBJECTS                 = insert_brackets.o bracket_misc.o
HEADERS 		= 
DEFINES			= -DSTANDALONE ${XCLIENTDEFINES}
INCLUDE                 = -I${SOURCEDIR}/include
LIBRARIES               = 
SYSLIBS                 = 
DERIVEDFILES            =

INSTALL.directory       = ${DEST.bin}
INSTALL.tool.directory  =

include ${RULESET.command}
