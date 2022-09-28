#
# @(#)Makefile	1.2	92/06/25 SMI
#

#
# Makefile for libradio
#

BASEDIR		= ../..
include ${BASEDIR}/Makefile.cfg


TARGETS		= library
TARGETS.install	= instsources instcustmakefile

LIBRARY		= radio

LIBSRCS		= \
		  agc.c \
		  netbroadcast.c \
		  radio_network.c \
		  radio_rcfile.c \
		  radio_subr.c

LIBHDRS		= \
		  audio_agc.h \
		  netbroadcast.h \
		  radio.h \
		  radio_network.h


LIBOBJS		= ${LIBSRCS:%.c=%.o}

INCLUDE		= ${AUDIOINC} -I.
DEFINES		= 

SOURCES		= ${LIBSRCS} ${LIBHDRS}

# DIR.install.library 	= ${DEST.audio.demo.lib}
DIR.install.sources	= ${DEST.audio.demo.src.radio.libradio}

include ${BASEDIR}/Makefile.rules
