#
# @(#)Makefile	3.12 01/22/93 
# 
# Binder Makefile 
#
# Copyright (c) 1987-1990 Sun Microsystems, Inc.
# All Rights Reserved
#

SOURCES.G	= 
SOURCES.c	= xv.c binder_tt.c binder_ce.c props.c dragdrop.c binder.c
SOURCES.h	= binder.h xv.h binder_tt.h binder_ce.h props.h
SOURCES.l	=
SOURCES.sh	=
SOURCES.y	=
SOURCES.pkg	=
SOURCES.icon	= binder.icon binder.mask.icon
SOURCES.map	=
SOURCES.man	= binder.1
SOURCES.info	= binder.info
PROGRAM		= binder 
PROJECTDIR	= ..

MANSECT   	= 1

#
# Binder specific flags
#
CPPFLAGS = -DTOOLTALK -I../libdstt
LDFLAGS  = -L../libdstt
LDLIBS   = -lce -ldstt -ltt


.KEEP_STATE:

default:	all

include ${PROJECTDIR}/Makefile.master.new

all:	$(PROGRAM)

clean:	master_clean

#
# For I18N string extraction (see Makefile.master)
#
TEXTDOMAIN   = SUNW_DESKSET_BINDER

install: install_normal_bins install_info install_man \
	 strings_single_domain
 
.INIT:	$(SOURCES.icon) $(SOURCES.h)
