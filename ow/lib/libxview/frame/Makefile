##
#
# Makefile for the frame package of xview
#
# @(#)Makefile	20.24 93/11/09
#
##

#
# Copyright (c) 1990 by Sun Microsystems, Inc.
#

SOURCEDIR		= ../../..

include ${SOURCEDIR}/config/Master.cfg

CFLAGS += -D_TS_ERRNO

LIBPARENT		= xview
LIBPART			= frame
TOOL			=
DELIVERABLE		= deliverable
STATIC			= static
SHARED			= shared
SOURCES			= frame.c	\
			  frame_init.c	\
			  fm_get.c	\
			  fm_set.c	\
			  fm_destroy.c	\
			  fm_layout.c	\
			  fm_cmdline.c	\
			  fm_display.c	\
			  fm_win.c	\
			  fm_geom.c	\
			  fm_input.c	\
			  fm_bdry.c	\
			  frame_sw.c	\
			  fm_rescale.c	\
			  frame_base.c	\
			  fmbs_set.c	\
			  fmbs_get.c	\
			  fmbs_dsty.c	\
			  frame_cmd.c	\
			  fmcmd_set.c	\
			  fmcmd_get.c	\
			  fmcmd_dsty.c	\
			  frame_help.c	\
			  fmhlp_set.c	\
			  fmhlp_get.c	\
			  fm_rodata.c	\
			  fmhlp_dsty.c
SOURCES.sa		= fm_data.c
OBJECTS			= ${SOURCES:%.c=%.o}
OBJECTS.sa		= ${SOURCES.sa:%.c=%.o}
HEADERS			= ${HEADERS.public} ${HEADERS.private}
HEADERS.public		= frame.h
HEADERS.private		= fm_impl.h	\
			  frame_base.h	\
			  frame_cmd.h	\
			  frame_help.h
DERIVEDFILES		=

INCLUDE			= -I${SOURCEDIR}/include -I${XW_HOME}/include

MAKE.install.target	= install_h

INSTALL.headers.dir	= ${DEST.include.xview}

PIC			= ${BIGPIC}

include ${RULESET.libchild}
${DIR.shared}/fm_rodata.o :=    CONSTDATA = ${f.CONSTDATA}
