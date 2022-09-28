#
# @(#)Makefile	1.17 09/22/93 Copyright (c) 1987-1990 Sun Microsystems, Inc.
# All Rights Reserved
#

SOURCES.G	= 
SOURCES.c	= clockui.c clock.c text.c xview.c
SOURCES.h	= clock.h patchlevel.h clockui.h
SOURCES.l	=
SOURCES.sh	=
SOURCES.y	=
SOURCES.pkg	=
SOURCES.icon	= clock.xbm clock.rom.xbm clock.icon
SOURCES.map	=
SOURCES.man	= clock.1
SOURCES.info	= clock.info
PROGRAM		= clock
PROJECTDIR	= ..

MANSECT   	= 1

#
# Couple of extra def's so that Guide gets included.  GUIDEHOME has
# to be defined for this Makefile to work anyway.

CPPFLAGS	= -I$(GUIDEHOME)/include
LDFLAGS		= -L$(GUIDEHOME)/lib
LDLIBS		= -lm -lguidexv

.KEEP_STATE:

default:	all

include ${PROJECTDIR}/Makefile.master.new


TEXTDOMAIN   = SUNW_DESKSET_CLOCK_LABEL SUNW_DESKSET_CLOCK_MSG SUNW_DESKSET_CLOCK_ERR

all:	$(PROGRAM)

clean:	master_clean

clobber: 	master_clobber

install: install_normal_bins install_info install_man install_icons \
	 strings_multi_domain
 
.INIT:	$(SOURCES.icon)
