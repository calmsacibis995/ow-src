#
# @(#)Makefile	1.11	95/04/28 SMI
#

# Makefile for playctl panel of audioctl

BASEDIR		= ../..
include ${BASEDIR}/Makefile.cfg

TARGETS		= objlib help

OBJLIB		= playctl

GSRCS		= playctl.G
GHELP		= audiocontrol
DOMAIN		= SUNW_DESKSET_AUDIOCONTROL
GFLAGS		+= -d $(DOMAIN)

LIBSRCS	= playctl.cc playctl_xv.cc playctl_ui.cc
		  
LIBHDRS	= playctl_ui.h playctl.h playctl_xv.h playctl.icon playctl.mask.icon

LIBOBJS	= ${LIBSRCS:%.cc=%.o}

INCLUDE		= -I.. ${AUDIOINC} \
		  -I/opt/SUNWspro/SC4.0/include/cc \
		  ${X11INC} \
		  ${GUIDEINC}
DEFINES		= -DI18N_DOMAIN='"${DOMAIN}"' ${AUDIODEFS} -DOW_I18N

include ${BASEDIR}/Makefile.rules
