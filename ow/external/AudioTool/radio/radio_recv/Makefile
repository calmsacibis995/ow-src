#
# @(#)Makefile	1.2	92/06/25 SMI
#

# Makefile for radio_recv

BASEDIR		= ../..
include ${BASEDIR}/Makefile.cfg

TARGETS		= program
TARGETS.install	= instsources instcustmakefile

PROGRAM		= radio_recv

PROGSRCS	= radio_recv.c
		  
PROGHDRS	= 

PROGOBJS	= ${PROGSRCS:%.c=%.o}

DEPENDLIBS	= ${RADIOLIBS} ${AUDIOLIBS}
LDLIBS		= ${SOCKETLIBS} ${INTLIBS} -lm

INCLUDE		= ${AUDIOINC} ${RADIOINC}
DEFINES		= ${AUDIODEFS}

SOURCES		= ${PROGSRCS} ${PROGHDRS}

DIR.install.program 	= ${DEST.audio.demo.bin}
DIR.install.sources	= ${DEST.audio.demo.src.radio.radio_recv}

include ${BASEDIR}/Makefile.rules
