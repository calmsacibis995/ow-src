##
#
# Makefile for the ttysw package of xview
#
# @(#)Makefile	20.23 94/03/02
#
##

#
# Copyright (c) 1990 by Sun Microsystems, Inc.
#

SOURCEDIR		= ../../..

include ${SOURCEDIR}/config/Master.cfg

CFLAGS += -D_TS_ERRNO -DINTERACTIVE

LIBPARENT		= xview
LIBPART			= ttysw
TOOL			=
DELIVERABLE		= deliverable
STATIC			= static
SHARED			= shared

CFILES.ttysw.L3		=
CFILES.ttysw.L4		= tty_cb.c

SOURCES			= cim_change.c	\
			  cim_size.c	\
			  csr_change.c	\
			  csr_init.c	\
			  ttyansi.c	\
			  tty_gtty.c	\
			  tty_init.c	\
			  tty_ntfy.c	\
			  tty_stty.c	\
			  ttytl.c	\
			  ttyselect.c	\
			  tty_main.c	\
			  tty_menu.c	\
			  tty_mapkey.c	\
			  tty.c	\
			  tty_modes.c	\
			  tty_es.c	\
			  term_ntfy.c	\
			  tty_compat.c	\
			  termsw.c	\
			  tty_newtxt.c	\
			  ${CFILES.ttysw.${I18N.level}}
SOURCES.sa		= tty_data.c	\
			  term_data.c
OBJECTS			= ${SOURCES:%.c=%.o}
OBJECTS.sa		= ${SOURCES.sa:%.c=%.o}
HEADERS			= ${HEADERS.public} ${HEADERS.private}
HEADERS.public		= ttysw.h	\
			  tty.h	\
			  termsw.h
HEADERS.private		= charimage.h	\
			  charscreen.h	\
			  ttyansi.h	\
			  tty_impl.h	\
			  term_impl.h
DERIVEDFILES		=

INCLUDE			= -I${SOURCEDIR}/include -I${XW_HOME}/include

MAKE.install.target	= install_h

INSTALL.headers.dir	= ${DEST.include.xview}

PIC			= ${BIGPIC}

include ${RULESET.libchild}
