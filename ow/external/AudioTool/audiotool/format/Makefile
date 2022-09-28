# Makefile for file chooser panel 
#
# @(#)Makefile 1.7 93/01/15
#


BASEDIR		= ../..
include ${BASEDIR}/Makefile.cfg


TARGETS		= objlib help

OBJLIB		= format

GSRCS		= format.G
GHELP		= audiotool
DOMAIN		= SUNW_DESKSET_AUDIOTOOL
GFLAGS		+= -d $(DOMAIN)

LIBSRCS		= format.c format_xv.c format_ui.c

LIBHDRS		= format_panel.h format_panel_impl.h format_ui.h

LIBOBJS		= ${LIBSRCS:%.c=%.o}

INCLUDE		= -I. -I.. -I../config -I../include ${LISTMGRINC} ${AUDIOINC} \
		  ${GUIDEINC} ${X11INC}
DEFINES		= -DI18N_DOMAIN='"${DOMAIN}"'

include ${BASEDIR}/Makefile.rules
