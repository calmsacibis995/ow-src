#
# @(#)Makefile	1.4	96/02/20 SMI
#

# Makefile for audioplay

BASEDIR		= ..
include ${BASEDIR}/Makefile.cfg

TARGETS		= program
STRIP		= strip
DOMAIN		= SUNW_AUDIO_MESSAGES

PROGRAM		= audioplay

PROGSRCS	= audioplay.c
PROGHDRS	=

PROGOBJS	= ${PROGSRCS:%.c=%.o}

DEPENDLIBS	= ${AUDIOLIBS}

# XXX - need X11 lib's for this for 4.x
LDLIBS		= ${INTLIBS} -lm

INCLUDE		= ${AUDIOINC}
DEFINES		= -DI18N_DOMAIN='"${DOMAIN}"' ${AUDIODEFS}

DIR.install.program = ${DEST.audio.bin}

include ${BASEDIR}/Makefile.rules
