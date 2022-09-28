# Makefile for demo libaudio
#
# @(#)Makefile	1.4	92/11/12 SMI
#
#

BASEDIR		= ..
include ${BASEDIR}/Makefile.cfg

TARGETS		= library files
TARGETS.install	= inst-library

LIBRARY		= audio

LIBSRCS		= filehdr.c \
		  device_ctl.c \
		  hdr_misc.c \
		  g721.c \
		  g723.c \
		  g72x_tables.c \
		  g711.c \
		  cosmult.c


LIBHDRS		=

LIBOBJS		= ${LIBSRCS:%.c=%.o}

INCLUDE		= ${AUDIOINC} -I../libAudio
DEFINES		= 

%.c: ../libAudio/%.c
	@if [ -f $@ ] ; then rm -f $@ ; fi; 
	@echo linking $< '->' $@
	@ln -s $< $@

INSTFILES	=  archdep.h libaudio.h audio_errno.h audio_hdr.h \
		   audio_filehdr.h audio_device.h audio_encode.h \
		   audio_types.h

FILES		= ${INSTFILES:%=../libAudio/%}

DIR.install.library	= ${DEST.audio.demo.lib}
DIR.install.files	= ${DEST.audio.demo.include.multimedia}

include ${BASEDIR}/Makefile.rules
