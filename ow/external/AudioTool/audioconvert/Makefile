#
# @(#)Makefile	1.6	97/04/03 SMI
#

# Makefile for audioconvert

BASEDIR		= ..
include ${BASEDIR}/Makefile.cfg

TARGETS		= cplusplusprog
STRIP		= strip
DOMAIN		= SUNW_AUDIO_MESSAGES

# XXX - This is here to ensure that the SUNW_AUDIO message file gets built
# only in the IMPS consolidation.  There are better ways to do this.
MESSAGE_FILE_DIRS = ../libAudio ../audioplay ../audiorecord .

CPLUSPLUSPROG	= audioconvert

PROGSRCS	= \
		  convert.cc \
		  file.cc \
		  main.cc \
		  parse.cc

PROGHDRS	= convert.h parse.h

PROGOBJS	= ${PROGSRCS:%.cc=%.o}

DEPENDLIBS	= ${AUDIOLIBS}

LDLIBS		= ${INTLIBS} ${SUNMATHLIB} -lm -norunpath

INCLUDE		= ${AUDIOINC}
DEFINES		= -DI18N_DOMAIN='"${DOMAIN}"' ${AUDIODEFS}

DIR.install.program = ${DEST.audio.bin}

include ${BASEDIR}/Makefile.rules
