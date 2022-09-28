#
# @(#)Makefile	1.9 05/10/93 Copyright (c) 1992 Sun Microsystems, Inc.
#


SOURCES.c = \
	ds_tooltalk.c		\
	dstt_general.c		\
	dstt_desktop_request.c	\
	dstt_notice_request.c	\
	dstt_prop_request.c	\
	dstt_desktop_parse.c	\
	dstt_notice_parse.c	\
	dstt_prop_parse.c	\
	dstt_xview_cb.c		\
	dstt_xview_desktop.c	\
	dstt_notices.c		\
	dstt_props.c		\
	dstt_set_status.c	\
	dstt_oldtt.c		\
	dstt_media.c		\
	dstt_display.c		\
	dstt_edit.c		\
	dstt_editor_general.c	\
	dstt_open.c		\
	dstt_paste.c		\
	dstt_close.c		\
	dstt_job.c

SOURCES_SA.c =

SOURCES.h = \
	ds_tooltalk.h	\
	dstt.h		\
	dstt_config.h	\
	dstt_vtype.h

LIBRARY = libdstt
LIBVERSION = .0
PROJECTDIR = ..

default: all

include ../Makefile.master.new

# Libraries needed by libdstt.so
SVR4_LIB_LDFLAGS      += -lc -ltt -lX11 -lolgx -lxview -ldeskset

all:	$(LIBRARY.a) $(LIBRARY.sa) $(LIBRARY.so)

install: install_libs

clean:	master_clean

.INIT: $(SOURCES.h)
