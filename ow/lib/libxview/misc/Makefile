##
#
# Makefile for the misc package of xview
#
# @(#)Makefile	1.20 93/11/09
#
##

#
# Copyright (c) 1990 by Sun Microsystems, Inc.
#

SOURCEDIR		= ../../..

include ${SOURCEDIR}/config/Master.cfg

CFLAGS += -D_TS_ERRNO

LIBPARENT		= xview
LIBPART			= misc
TOOL			=
DELIVERABLE		= deliverable
STATIC			= static
SHARED			= shared

CFILES.misc.L3		=
CFILES.misc.L4		= convdup.c     \
                          wckind.c      \
                          xv_i18n.c     \
                          xv_path.c

HFILES.misc.L3		=
HFILES.misc.L4		=


SOURCES			= demorandom.c	\
			  getlogindr.c	\
			  bitmask.c	\
			  ${I18N_CFILES.misc} \
			  hashfn.c	\
			  expandname.c	\
			  expandpath.c	\
			  drop.c	\
			  db_conv.c	\
			  quark.c	\
			  setlocale.c	\
			  windowloop.c	\
			  xv_casecmp.c	\
			  xv_deaf.c	\
			  xv_win_lp.c	\
			  xv_util.c	\
			  xv_version.c  \
			  xv_slots.c  \
			  ${CFILES.misc.${I18N.level}}
SOURCES.sa		= 
OBJECTS			= ${SOURCES:%.c=%.o}
OBJECTS.sa		= ${SOURCES.sa:%.c=%.o}
HEADERS			= ${HEADERS.public} ${HEADERS.private}
HEADERS.public		= cms_grays.h	\
			  cms_mono.h	\
			  cmsrainbow.h	\
			  cms_rgb.h	\
			  sun.h		\
			  window_hs.h	\
			  expandname.h	\
			  xv_c_types.h	\
			  db.h		\
			  attrol.h	\
			  xv_i18n.h	\
			  xv_version.h  \
			  xview.h
HEADERS.private		= bitmask.h	\
			  hashfn.h	\
			  i18n_impl.h	\
			  ${I18N_HFILES.misc} \
			  portable.h	\
			  xv_slots.h	\
			  ${HFILES.misc.${I18N.level}}
DEFINES			= -DOS_HAS_LOCALE -DOS_HAS_MMAP
DERIVEDFILES		=

INCLUDE			= -I${SOURCEDIR}/include -I${XW_HOME}/include

MAKE.install.target	= install_h

INSTALL.headers.dir	= ${DEST.include.xview}

PIC			= ${BIGPIC}

include ${RULESET.libchild}

