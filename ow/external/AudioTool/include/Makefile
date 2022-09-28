#
# @(#)Makefile	1.7	93/01/07 SMI
#

#
# Makefile for audio tools - include files
#

BASEDIR		= ..
include ${BASEDIR}/Makefile.cfg

TARGETS		= subdirs

SUBDIRS		= sun

# this will insure that the headers get checked out
LIBHDRS		= \
		  audio_i18n.h \
		  audio_types.h \
		  ds_colors.h \
		  ds_popup.h \
		  ds_tooltalk.h

# this .INIT makes sure right symlinks are created ...
.INIT: multimedia c_varieties.h

include ${BASEDIR}/Makefile.rules

multimedia: FRC
	@if [ ! -h multimedia -a ! -d multimedia ] ; then \
		echo "creating sym-link: multimedia -> ../libAudio"; \
		ln -s ../libAudio multimedia; \
	fi

c_varieties.h: FRC
	@if [ ! -h c_varieties.h ] ; then \
	echo "creating sym-link: c_varieties.h -> ../libAudio/audio_types.h"; \
	ln -s ../libAudio/audio_types.h c_varieties.h; \
	fi
FRC:
