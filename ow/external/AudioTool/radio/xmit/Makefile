#
# @(#)Makefile	1.5	97/01/23 SMI
#

# Makefile for xmit

BASEDIR		= ../..
include ${BASEDIR}/Makefile.cfg

TARGETS		= program
TARGETS.install	= instinfo instsources instcustmakefile

PROGRAM		= xmit

GSRCS		= xmit.G

PROGSRCS	= xmit.c xmit_ui.c
		  
PROGHDRS	= \
		  xmit_ui.h \
		  xmit_off.icon \
		  xmit_off.mask.icon \
		  xmit_on.icon \
		  xmit_on.mask.icon

PROGOBJS	= ${PROGSRCS:%.c=%.o}

DEPENDLIBS	= ${RADIOLIBS} ${AUDIOLIBS}
LDLIBS		= ${X11LIBS} ${GUIDELIBS} ${SOCKETLIBS} ${INTLIBS} ${SUNMATHLIB} -lm

INCLUDE		= ${AUDIOINC} ${RADIOINC} ${X11INC} ${GUIDEINC}
DEFINES		= ${AUDIODEFS}

INFO		= xmit.info
SOURCES		= ${PROGSRCS} ${PROGHDRS} ${GSRCS} ${INFO}

DIR.install.program 	= ${DEST.audio.demo.bin}
DIR.install.info	= ${DEST.audio.demo.help}
DIR.install.sources	= ${DEST.audio.demo.src.radio.xmit}

include ${BASEDIR}/Makefile.rules
