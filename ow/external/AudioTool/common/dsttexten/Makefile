#
# Makefile for extensions to deskset tooltalk (dstt) library
#
# @(#)Makefile	1.1	92/10/30 SMI

BASEDIR		= ../..
include ${BASEDIR}/Makefile.cfg


TARGETS		= library 

LIBRARY		= dsttexten

LIBSRCS		= dstt_audioctl.c dstt_audioctl_parse.c dstt_audioctl_request.c
LIBHDRS		= dstt.h dstt_audio.h dstt_config.h dstt_vtype.h
LIBOBJS		= ${LIBSRCS:%.c=%.o}

INCLUDE		= -I. -I${BASEDIR}/include ${X11INC}
DEFINES		=

include ${BASEDIR}/Makefile.rules
