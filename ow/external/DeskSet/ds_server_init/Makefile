#
# @(#)Makefile	3.7 01/22/93
#
# ds_server_init Makefile
#
# Copyright (c) 1987-1990 Sun Microsystems, Inc.
# All Rights Reserved
#

SOURCES.G	= 
SOURCES.c	= ds_server_init.c
SOURCES.h	= 
SOURCES.l	=
SOURCES.sh	=
SOURCES.y	=
SOURCES.pkg	=
SOURCES.icon	= 
SOURCES.map	=
SOURCES.man	= ds_server_init.1
SOURCES.info	= 
PROGRAM		= ds_server_init
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
TEXTDOMAIN   = SUNW_DESKSET_DS_SERVER_INIT

install_colors:
	

#
# DESTDIR defined by Makefile.master
#
DATA_FILE    = ds_colors.txt
DATA_DIR     = $(DESTDIR)/share/xnews/client/ds_server_init

install_color_file: $(DATA_FILE)
	$(INSTALL) -d $(DATA_DIR)
	rm -f $(DATA_DIR)/$(DATA_FILE)
	$(INSTALL) -m 444 -f $(DATA_DIR) $(DATA_FILE)

install: install_normal_bins install_man strings_single_domain install_color_file
 
.INIT:	$(SOURCES.icon)
