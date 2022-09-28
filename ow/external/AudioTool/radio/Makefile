#
# @(#)Makefile	1.2	92/06/25 SMI
#

# Makefile for audio platform tools

BASEDIR		= ..
include ${BASEDIR}/Makefile.cfg

TARGETS		= subdirs

# to install Makefile.customer -> Makefile in src dir
TARGETS.install	= instcustmakefile

SUBDIRS		= libradio \
		  radio_xmit \
		  radio_recv \
		  radio \
		  xmit

DIR.install.sources = ${DEST.audio.demo.src.radio}

include ${BASEDIR}/Makefile.rules
