#
# @(#)Makefile	1.5	96/02/29 SMI
#

# Makefile for radio

BASEDIR		= ../..
include ${BASEDIR}/Makefile.cfg

TARGETS		= program
TARGETS.install	= instinfo instsources instcustmakefile

PROGRAM		= radio

GSRCS		= radio.G

PROGSRCS	= radio.c radio_ui.c
		  
PROGHDRS	= \
		  radio_ui.h \
		  radio_off.icon \
		  radio_off.mask.icon \
		  radio_on.icon \
		  radio_on.mask.icon

PROGOBJS	= ${PROGSRCS:%.c=%.o}

DEPENDLIBS	= ${RADIOLIBS} ${AUDIOLIBS}
LDLIBS		= ${X11LIBS} ${GUIDELIBS} ${SOCKETLIBS} ${INTLIBS} -lm

INCLUDE		= ${AUDIOINC} ${RADIOINC} ${X11INC} ${GUIDEINC}
DEFINES		= ${AUDIODEFS}

INFO		= radio.info
SOURCES		= ${PROGSRCS} ${PROGHDRS} ${GSRCS} ${INFO}

DIR.install.program 	= ${DEST.audio.demo.bin}
DIR.install.info	= ${DEST.audio.demo.help}
DIR.install.sources	= ${DEST.audio.demo.src.radio.radio}

include ${BASEDIR}/Makefile.rules
