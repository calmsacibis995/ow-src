# Makefile for LED meter panel item
#
# @(#)Makefile        1.9     92/11/11 SMI
#

BASEDIR		= ../..
include ${BASEDIR}/Makefile.cfg


TARGETS		= library

LIBRARY		= meter

LIBSRCS		= \
		  meter.c \
		  meter_xv.c

LIBHDRS		= meter.h \
		  meter_impl.h

LIBOBJS		= ${LIBSRCS:%.c=%.o}

INCLUDE		= -I. -I${BASEDIR}/include ${X11INC}
DEFINES		=

include ${BASEDIR}/Makefile.rules
