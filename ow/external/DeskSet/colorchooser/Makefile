#
# @(#)Makefile	3.11 01/22/93
#
# Colorchooser Makefile
#
# Copyright (c) 1987-1990 Sun Microsystems, Inc.
# All Rights Reserved
#

SOURCES.G	= 
SOURCES.c	= colorchooser.c \
		  color_props.c \
		  color.c \
		  color_tt.c \
		  dragdrop.c
SOURCES.h	= colorchooser.h color.h color_tt.h
SOURCES.l	=
SOURCES.sh	=
SOURCES.y	=
SOURCES.pkg	=
SOURCES.icon	= preview.icon source.ptr.icon
SOURCES.map	=
SOURCES.man	= colorchooser.1
SOURCES.types   = colorchooser.types
SOURCES.info	= colorchooser.info
PROGRAM		= colorchooser
PROJECTDIR	= ..

MANSECT   	= 1

#
# Colorchooser specific flags
#
CPPFLAGS = -I../libdstt
LDFLAGS = -L../libdstt
LDLIBS = -ldstt -ltt

.KEEP_STATE:

default:	all

include ${PROJECTDIR}/Makefile.master.new

all:	$(PROGRAM)


clean:	master_clean

#
# For I18N string extraction (see Makefile.master)
#
TEXTDOMAIN   = SUNW_DESKSET_COLORCHOOSER

install: install_normal_bins install_info install_man \
	 install_tt_types strings_single_domain
 
.INIT:	$(SOURCES.icon)
