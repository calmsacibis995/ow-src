SOURCES.G	= format_floppy.G
SOURCES.c	= format_floppy_ui.c core.c xview.c
SOURCES.h	= format_floppy.h format_floppy_ui.h
SOURCES.l	=
SOURCES.sh	=
SOURCES.y	=
SOURCES.pkg	=
SOURCES.icon	= 
SOURCES.map	=
SOURCES.man	= 
SOURCES.info	= format_floppy.info
PROGRAM		= format_floppy
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

TEXTDOMAIN   = SUNW_DESKSET_DS_FORMAT

all:		$(PROGRAM)

clean:		master_clean

clobber:	master_clobber

install: install_normal_bins install_info \
	strings_multi_domain

.INIT:	$(SOURCES.icon)
