#
# @(#)Makefile	1.6	97/01/23 SMI
#

# Makefile for gaintool

BASEDIR		= ..
include ${BASEDIR}/Makefile.cfg

TARGETS		= program

TARGETS.install	= instinfo instsources instcustmakefile

PROGRAM		= gaintool

GSRCS		= gaintool.G

PROGSRCS	= gaintool.c gaintool_ui.c
		  
PROGHDRS	= gaintool_ui.h gaintool.icon gaintool.mask.icon

PROGOBJS	= ${PROGSRCS:%.c=%.o}

INFO		= gaintool.info

DEPENDLIBS	= ${AUDIOLIBS}
LDLIBS		= ${GUIDELIBS} ${X11LIBS} ${INTLIBS} ${SUNMATHLIB}

INCLUDE		= ${AUDIOINC} ${X11INC} ${GUIDEINC}
DEFINES		= ${AUDIODEFS}

SOURCES		= ${GSRCS} ${PROGSRCS} ${PROGHDRS} ${INFO}

DIR.install.info	= ${DEST.audio.demo.help}
DIR.install.program	= ${DEST.audio.demo.bin}
DIR.install.sources	= ${DEST.audio.demo.src.gaintool}

include ${BASEDIR}/Makefile.rules
