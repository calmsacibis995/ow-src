#
# @(#)Makefile	3.6 01/22/93 
#
# Tapetool Makefile
#
# Copyright (c) 1987-1990 Sun Microsystems, Inc.
# All Rights Reserved
#

SOURCES.G	= 
SOURCES.c	= tapetool.c dragdrop.c
SOURCES.h	= 
SOURCES.l	=
SOURCES.sh	=
SOURCES.y	=
SOURCES.pkg	=
SOURCES.icon	= app_list.icon \
		  box.icon \
		  doc_list.icon \
		  folder_list.icon \
        	  tapetool.full.icon \
		  tapetool.icon \
		  tapetool.mask.icon \
		  source.icon \
        	  busy.source.icon

SOURCES.map	=
SOURCES.man	= tapetool.1
SOURCES.info	= tapetool.info
PROGRAM		= tapetool
PROJECTDIR	= ..

MANSECT   	= 1

#
# Couple of extra def's so that Guide gets included.  GUIDEHOME has
# to be defined for this Makefile to work anyway.

#CPPFLAGS	= -I$(GUIDEHOME)/include
#LDFLAGS	= -L$(GUIDEHOME)/lib
#LDLIBS		= -lm -lguidexv

.KEEP_STATE:

default:	all

include ${PROJECTDIR}/Makefile.master.new

all:	$(PROGRAM)

clean:	master_clean

#
# For I18N string extraction (see Makefile.master)
#
TEXTDOMAIN   = SUNW_DESKSET_TAPETOOL

install: install_normal_bins install_info install_man \
	 strings_single_domain
 
.INIT:	$(SOURCES.icon)
