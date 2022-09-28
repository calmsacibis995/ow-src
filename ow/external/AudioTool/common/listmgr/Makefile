#
# Makefile for panel list manager library
#
# @(#)Makefile	1.2	92/10/23 SMI

BASEDIR		= ../..
include ${BASEDIR}/Makefile.cfg


TARGETS		= library 

LIBRARY		= listmgr

LIBSRCS		= listmgr.c list.c plistutils.c 
LIBHDRS		= list.h listmgr.h
LIBOBJS		= ${LIBSRCS:%.c=%.o}

INCLUDE		= -I. -I${BASEDIR}/include ${X11INC} ${GUIDEINC}
DEFINES		=

include ${BASEDIR}/Makefile.rules
