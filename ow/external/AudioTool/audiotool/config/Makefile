#
# @(#)Makefile	1.14	93/01/15 SMI
#

BASEDIR		= ../..
include ${BASEDIR}/Makefile.cfg


TARGETS		= objlib help

OBJLIB		= config

GSRCS		= config.G
GHELP		= audiotool
DOMAIN		= SUNW_DESKSET_AUDIOTOOL
GFLAGS		+= -d $(DOMAIN)

LIBSRCS		= \
		  defaults.c \
		  xv_defaults.c \
		  silent_params.c \
		  config.c \
		  config_xv.c \
		  config_ui.c

LIBHDRS		= defaults.h \
		  xv_defaults.h \
		  silent_params.h \
		  config_panel.h \
		  config_panel_impl.h \
		  config_ui.h

LIBOBJS		= ${LIBSRCS:%.c=%.o}

INCLUDE		= -I. -I.. -I../include ${AUDIOINC} ${GUIDEINC} ${X11INC}
DEFINES		= -DI18N_DOMAIN='"${DOMAIN}"'

include ${BASEDIR}/Makefile.rules
