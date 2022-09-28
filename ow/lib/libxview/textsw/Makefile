##
#
# Makefile for the text package of xview
#
# @(#)Makefile	20.28 93/11/09
#
##

#
# Copyright (c) 1990 by Sun Microsystems, Inc.
#

SOURCEDIR		= ../../..

include ${SOURCEDIR}/config/Master.cfg

CFLAGS += -D_TS_ERRNO

LIBPARENT		= xview
LIBPART			= textsw
TOOL			=
DELIVERABLE		= deliverable
STATIC			= static
SHARED			= shared

CFILES.textsw.L4	= txt_cb.c convpos.c
CFILES.textsw.L3	=
HFILES.textsw.L4	= convpos.h
HFILES.textsw.L3	=


SOURCES			= txt_once.c	\
			  txt_input.c	\
			  txt_edit.c	\
			  txt_menu.c	\
			  txt_disp.c	\
			  txt_event.c	\
			  txt_find.c	\
			  txt_file.c	\
			  txt_sel.c	\
			  txt_selsvc.c	\
			  txt_again.c	\
			  txt_getkey.c	\
			  txt_putkey.c	\
			  txt_filter.c	\
			  text.c	\
			  txt_attr.c	\
			  txt_dbx.c	\
			  txt_tool.c	\
			  txt_scroll.c	\
			  txt_field.c	\
			  txt_caret.c	\
			  txt_search.c	\
			  txt_e_menu.c	\
			  txt_move.c	\
			  txt_view.c	\
			  txt_popup.c	\
			  txt_store.c	\
			  txt_match.c	\
			  txt_load.c	\
			  txt_line.c	\
			  txt_incl.c	\
			  es_file.c	\
			  es_mem.c	\
			  es_util.c	\
			  es_attr.c	\
			  ps_impl.c	\
			  ev_display.c	\
			  ev_op_bdry.c	\
			  ev_edit.c	\
			  ev_once.c	\
			  ev_attr.c	\
			  ev_field.c	\
			  ev_update.c	\
			  es_cp_file.c	\
			  ei_text.c	\
			  ei_attr.c	\
			  finger_tbl.c	\
			  ${CFILES.textsw.${I18N.level}}
SOURCES.sa		= txt_data.c
OBJECTS			= ${SOURCES:%.c=%.o}
OBJECTS.sa		= ${SOURCES.sa:%.c=%.o}
HEADERS			= ${HEADERS.public} ${HEADERS.private}
HEADERS.public		= textsw.h text.h
HEADERS.private		= ei.h	\
			  es.h	\
			  ev.h	\
			  ev_impl.h	\
			  primal.h	\
			  ps_impl.h	\
			  finger_tbl.h	\
			  txt_impl.h	\
			  txt_18impl.h	\
			  ${HFILES.textsw.${I18N.level}}
DERIVEDFILES		=

INCLUDE			= -I${SOURCEDIR}/include -I${XW_HOME}/include

MAKE.install.target	= install_h

INSTALL.headers.dir	= ${DEST.include.xview}

PIC			= ${BIGPIC}

include ${RULESET.libchild}
