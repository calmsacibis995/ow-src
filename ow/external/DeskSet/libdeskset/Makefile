#
# @(#)Makefile	3.22 11/18/93 Copyright (c) 1992 Sun Microsystems, Inc.
#


SOURCES.c = \
	ds_listbx.c 	\
	ds_popup.c 	\
	ds_pathname.c 	\
	ds_item.c 	\
	ds_dblclick.c	\
	ds_colors.c 	\
	ds_bind.c	\
	ds_xlib.c	\
	XCrPFData.c 	\
	XRdPixF.c 	\
	XWrPixF.c 	\
	XRdPixD.c 	\
	XWrPixD.c 	\
	xpm.c		\
	ds_template.c	\
	ds_hostname.c  \
	ds_relname.c	\
	ds_print.c	\
	ds_verbose_malloc.c \
	ds_vendname.c	\
	dsll.c

SOURCES_SA.c =		\
	ds_relname.c

SOURCES.h = \
	ds_listbx.h 	\
	ds_popup.h 	\
	ds_item.h 	\
	ds_colors.h 	\
	ds_bind.h	\
	ds_xlib.h	\
	xpm.h 		\
	xpmP.h		\
	ds_verbose_malloc.h

LIBRARY = libdeskset
LIBVERSION = .0
PROJECTDIR = ..

default: all

include ../Makefile.master.new

# Libraries needed by libdeskset.so
SVR4_LIB_LDFLAGS        += -lc -lnsl -lX11 -lolgx -lxview
SVR4_CPPFLAGS		+= -D_TS_ERRNO

all:	$(LIBRARY.a) $(LIBRARY.sa) $(LIBRARY.so)

install: install_libs

clean:	master_clean

clobber:	master_clobber

.INIT: $(SOURCES.h)
