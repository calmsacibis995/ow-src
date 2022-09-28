#
# @(#)Makefile	1.4	97/01/23 SMI
#

# Makefile for soundtool

BASEDIR		= ..
include ${BASEDIR}/Makefile.cfg

TARGETS		= program
TARGETS.install	= instsources instcustmakefile

PROGRAM		= soundtool

PROGSRCS	= soundtool.c
		  
PROGHDRS	= soundtool.h soundtool.icon

PROGOBJS	= ${PROGSRCS:%.c=%.o}

DEPENDLIBS	= ${AUDIOLIBS}
LDLIBS		= ${X11LIBS} ${INTLIBS} ${SUNMATHLIB} -lm

INCLUDE		= ${AUDIOINC} ${X11INC}
DEFINES		= ${AUDIODEFS}

SOURCES		= ${PROGSRCS} ${PROGHDRS}

DIR.install.program	= ${DEST.audio.demo.bin}
DIR.install.sources	= ${DEST.audio.demo.src.soundtool}

include ${BASEDIR}/Makefile.rules
