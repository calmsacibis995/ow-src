#
# @(#)Makefile	1.8	95/04/28 SMI
#

# Makefile for recordctl panel of audiocontrol

BASEDIR		= ../..
include ${BASEDIR}/Makefile.cfg

TARGETS		= objlib help

OBJLIB		= recordctl

GSRCS		= recordctl.G
GHELP		= audiocontrol
DOMAIN		= SUNW_DESKSET_AUDIOCONTROL
GFLAGS		+= -d $(DOMAIN)

LIBSRCS	= recordctl.cc recordctl_xv.cc recordctl_ui.cc
		  
LIBHDRS	= recordctl_ui.h recordctl.h recordctl_xv.h

LIBOBJS	= ${LIBSRCS:%.cc=%.o}

INCLUDE		= -I.. -I${BASEDIR}/common ${AUDIOINC} \
		  -I/opt/SUNWspro/SC4.0/include/cc \
		  ${X11INC} \
		  ${GUIDEINC}
DEFINES		= -DI18N_DOMAIN='"${DOMAIN}"' ${AUDIODEFS} -DOW_I18N

include ${BASEDIR}/Makefile.rules
