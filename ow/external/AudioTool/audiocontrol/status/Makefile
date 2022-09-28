#
# @(#)Makefile	1.7	93/01/15 SMI
#

# Makefile for status panel of audioctl

BASEDIR		= ../..
include ${BASEDIR}/Makefile.cfg

TARGETS		= objlib help

OBJLIB		= status

GSRCS		= status.G
GHELP		= audiocontrol
DOMAIN		= SUNW_DESKSET_AUDIOCONTROL
GFLAGS		+= -d $(DOMAIN)

LIBSRCS	= status.cc status_xv.cc status_ui.cc
		  
LIBHDRS	= status_ui.h status.h status_xv.h

LIBOBJS	= ${LIBSRCS:%.cc=%.o}

INCLUDE		= -I.. ${AUDIOINC} \
		  ${X11INC} \
		  ${GUIDEINC}
DEFINES		= -DI18N_DOMAIN='"${DOMAIN}"' ${AUDIODEFS}

include ${BASEDIR}/Makefile.rules
