#
# @(#)Makefile	1.22	97/01/23 SMI
#

# Makefile for audio platform tools

# Set default target
default: all

BASEDIR		= .

MAKEFILE.cfg = Makefile.cfg
MAKEFILE.rules = Makefile.rules

include ${MAKEFILE.cfg}

TARGETS		= subdirs

# AUXFILES	= filter_messages.sh

# NOTE: Ordering of the subdirs DOES matter!

# share stuff
SHARED_SUBDIRS	= include \
		  $(GUIDESRC) \
		  libAudio

# audiotool (OW) only builds
ATOOL_SUBDIRS	= common \
		  audiocontrol \
		  audiotool

# audio command line and demo builds only
CMD_SUBDIRS	= audioplay \
		  audiorecord \
		  audioconvert

DEMO_SUBDIRS	= libaudio \
		  soundtool \
		  radio \
		  sounds \
		  gaintool \
		  man \
		  misc

AUDIO_SUBDIRS	= ${CMD_SUBDIRS} \
		  ${DEMO_SUBDIRS}


SUBDIRS		= ${SHARED_SUBDIRS} \
		  ${ATOOL_SUBDIRS} \
		  ${AUDIO_SUBDIRS}

tools:		all
tools:= 	SUBDIRS = ${SHARED_SUBDIRS} ${ATOOL_SUBDIRS}
insttools:	install
insttools:= 	SUBDIRS = ${SHARED_SUBDIRS} ${ATOOL_SUBDIRS}

cmds:		all
cmds:= 		SUBDIRS = ${SHARED_SUBDIRS} ${CMD_SUBDIRS}
instcmds:	install
instcmds:= 	SUBDIRS = ${SHARED_SUBDIRS} ${CMD_SUBDIRS}

demos:		all
demos:=		SUBDIRS = ${SHARED_SUBDIRS} ${DEMO_SUBDIRS}
instdemos:	install
instdemos:=	SUBDIRS = ${SHARED_SUBDIRS} ${DEMO_SUBDIRS}

audio:		all
audio:=		SUBDIRS = ${SHARED_SUBDIRS} ${AUDIO_SUBDIRS}
instaudio:	install
instaudio:=	SUBDIRS = ${SHARED_SUBDIRS} ${AUDIO_SUBDIRS}

include ${MAKEFILE.rules}

tell sccstell:
	-@find . -name 'p.*' -print | grep "SCCS/p"
