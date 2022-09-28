##
#
# Makefile for the panel package of xview
#
# @(#)Makefile	20.30 93/11/09
#
##

#
# Copyright (c) 1990 by Sun Microsystems, Inc.
#

SOURCEDIR		= ../../..

include ${SOURCEDIR}/config/Master.cfg

CFLAGS += -D_TS_ERRNO

LIBPARENT		= xview
LIBPART			= panel
TOOL			=
DELIVERABLE		= deliverable
STATIC			= static
SHARED			= shared

CFILES.panel.L3		=
CFILES.panel.L4		= p_imcb.c

SOURCES			= item.c	\
			  item_get.c	\
			  item_set.c	\
			  p_ambtn.c	\
			  p_btn.c	\
			  p_choice.c	\
			  p_compat.c	\
			  p_drop.c	\
			  p_event.c	\
			  p_gauge.c	\
			  p_get.c	\
			  p_image.c	\
			  p_list.c	\
			  p_mlinetxt.c	\
			  p_msg.c	\
			  p_num_txt.c	\
			  p_paint.c	\
			  p_public.c	\
			  p_scroll.c	\
			  p_select.c	\
			  p_set.c	\
			  p_slider.c	\
			  p_txt.c	\
			  p_utl.c	\
			  p_view.c	\
			  panel.c	\
			  ${CFILES.panel.${I18N.level}}
SOURCES.sa		= item_data.c	\
			  p_amb_data.c	\
			  p_btn_data.c	\
			  p_chcedata.c	\
			  p_drp_data.c	\
			  p_lst_data.c	\
			  p_mltxdata.c	\
			  p_msg_data.c	\
			  p_ntx_data.c	\
			  p_txt_data.c	\
			  p_utl_data.c	\
			  panel_data.c
OBJECTS			= ${SOURCES:%.c=%.o}
OBJECTS.sa		= ${SOURCES.sa:%.c=%.o}
HEADERS			= ${HEADERS.public} ${HEADERS.private}
HEADERS.public		= panel.h
HEADERS.private		= panel_impl.h	\
			  item_impl.h	\
			  p_lst_impl.h
DERIVEDFILES		=

INCLUDE			= -I${SOURCEDIR}/include -I${XW_HOME}/include

MAKE.install.target	= install_h

INSTALL.headers.dir	= ${DEST.include.xview}

PIC			= ${BIGPIC}

include ${RULESET.libchild}

