#
# @(#)Makefile	1.2	92/06/25 SMI
#

# Makefile for radio_xmit

BASEDIR		= ../..
include ${BASEDIR}/Makefile.cfg

TARGETS		= program
TARGETS.install	= instsources instcustmakefile

PROGRAM		= radio_xmit

PROGSRCS	= radio_xmit.c
		  
PROGHDRS	= 

PROGOBJS	= ${PROGSRCS:%.c=%.o}

DEPENDLIBS	= ${RADIOLIBS} ${AUDIOLIBS}
LDLIBS		= ${SOCKETLIBS} ${INTLIBS} -lm

INCLUDE		= ${AUDIOINC} ${RADIOINC}
DEFINES		= ${AUDIODEFS}

SOURCES		= ${PROGSRCS} ${PROGHDRS}

DIR.install.program 	= ${DEST.audio.demo.bin}
DIR.install.sources	= ${DEST.audio.demo.src.radio.radio_xmit}

include ${BASEDIR}/Makefile.rules
