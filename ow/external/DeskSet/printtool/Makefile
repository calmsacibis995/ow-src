#
# @(#)Makefile	3.13 09/22/93 Copyright (c) 1987-1990 Sun Microsystems, Inc.
# All Rights Reserved
#

SOURCES.G	= 
SOURCES.c	= printtool_ui.c gdd.c printtool.c
SOURCES.h	= printtool.h printtool_ui.h
SOURCES.l	=
SOURCES.sh	=
SOURCES.y	=
SOURCES.pkg	=
SOURCES.icon	= print.icon print_mask.icon noprint.icon noprint_mask.icon
SOURCES.map	=
SOURCES.man	= printtool.1
SOURCES.info	= printtool.info
PROGRAM		= printtool
PROJECTDIR	= ..

MANSECT   	= 1

#
# Couple of extra def's so that Guide gets included.  GUIDEHOME has
# to be defined for this Makefile to work anyway.

CPPFLAGS	= -I$(GUIDEHOME)/include
LDFLAGS		= -L$(GUIDEHOME)/lib
LDLIBS		= -lguidexv -lce

.KEEP_STATE:

default:	all

include ${PROJECTDIR}/Makefile.master.new

TEXTDOMAIN   = SUNW_DESKSET_PRINTTOOL libguidexv

all:	$(PROGRAM)

clean:	master_clean

install: install_normal_bins install_info install_man \
	 strings_multi_domain
 
.INIT:	$(SOURCES.icon)
