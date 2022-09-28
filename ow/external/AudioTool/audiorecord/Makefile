#
# @(#)Makefile	1.5	97/01/23 SMI
#

# Makefile for audiorecord

BASEDIR		= ..
include ${BASEDIR}/Makefile.cfg

TARGETS		= program
STRIP		= strip
DOMAIN		= SUNW_AUDIO_MESSAGES

PROGRAM		= audiorecord

PROGSRCS	= audiorecord.c
PROGHDRS	=

PROGOBJS	= ${PROGSRCS:%.c=%.o}

DEPENDLIBS	= ${AUDIOLIBS}

# XXX - need X11 lib's for this for 4.x
LDLIBS		= ${INTLIBS} ${SUNMATHLIB} -lm

INCLUDE		= ${AUDIOINC}
DEFINES		= -DI18N_DOMAIN='"${DOMAIN}"' ${AUDIODEFS}

DIR.install.program = ${DEST.audio.bin}

include ${BASEDIR}/Makefile.rules
