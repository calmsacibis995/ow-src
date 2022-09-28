#
# @(#)Makefile	3.15 02/13/97 Copyright (c) 1987-1990 Sun Microsystems, Inc.
# All Rights Reserved
#

SOURCES.G       = 
SOURCES.c       = textedit.c \
		  dragdrop.c \
		  tooltalk.c \
		  textedit_dstt.c \
		  textedit_menu.c

SOURCES.h       = textedit.h
SOURCES.l       =
SOURCES.sh      =
SOURCES.y       =
SOURCES.pkg     =
SOURCES.icon    = 	busy.source.icon \
			dupedoc_drag.icon \
			dupedoc_drop.icon \
			source.icon \
			source.ptr.icon \
			textedit.icon \
			textedit.mask.icon

SOURCES.map     =
SOURCES.man     = textedit.1
SOURCES.info    = textedit.info
PROGRAM         = textedit
PROJECTDIR      = ..

MANSECT         = 1

# ToolTalk types file
SOURCES.types =	textedit.types

CPPFLAGS += -I../libdstt
LDFLAGS += -L../libdstt
LDLIBS += -ldstt -ltt

# Include rule set
include ../Makefile.master.new

all: $(SOURCES) $(TARGETS) $(LIBRARY) $(PROGRAM)

clean: master_clean

#
# For I18N string extraction (see Makefile.master)
#
TEXTDOMAIN   = SUNW_DESKSET_TEXTEDIT

install: install_normal_bins install_info install_man install_icons \
	install_tt_types strings_single_domain
 
tar:
	echo You will want to have an empty directory that you are taring to!
	tar fc - . | rsh ${DESTMACH} \
		"cd ${DESTDIR}${SRCDIR};tar xpBf -"
		
.INIT:	$(SOURCES.icon)
