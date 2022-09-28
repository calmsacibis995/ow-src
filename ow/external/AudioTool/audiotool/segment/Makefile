# Makefile for segment library
#
# @(#)Makefile        1.15     93/01/15 SMI
#

BASEDIR		= ../..
include ${BASEDIR}/Makefile.cfg

DOMAIN		= SUNW_DESKSET_AUDIOTOOL

TARGETS		= objlib help

OBJLIB		= segment

LIBSRCS		= \
		  segment.c \
		  segment_xv.c

LIBHDRS		= segment_canvas.h \
		  segment_canvas_impl.h \
		  basic_cursor.pr \
		  xhair_cursor.pr

LIBOBJS		= ${LIBSRCS:%.c=%.o}

INCLUDE		= -I.. -I../include ${GUIDEINC} ${AUDIOINC} ${X11INC}
DEFINES		= -DI18N_DOMAIN='"${DOMAIN}"'

include ${BASEDIR}/Makefile.rules
