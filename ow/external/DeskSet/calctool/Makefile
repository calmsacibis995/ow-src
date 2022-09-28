#
#
#  Makefile for calctool, a calculator program.
#
#  @(#)Makefile 1.10 93/09/22
#



SOURCES.G	= calctool.G
SOURCES.c	= calctool_ui.c 	\
			calctool.c 	\
			display.c 	\
			get.c 		\
			functions.c 	\
			graphics.c	\
			mp.c		\
			text.c		\
			xview.c
SOURCES.h	= calctool.h color.h extern.h patchlevel.h calctool_ui.h
SOURCES.l	=
SOURCES.sh	=
SOURCES.y	=
SOURCES.pkg	=
IDIR		= images
SOURCES.icon	= $(IDIR)/button.color.invert.icon \
		  $(IDIR)/button.color.normal.icon \
		  $(IDIR)/button.color.stencil.icon \
		  $(IDIR)/button.mono.invert.icon \
		  $(IDIR)/button.mono.normal.icon \
		  $(IDIR)/button.mono.stencil.icon \
	  	  $(IDIR)/calctool.icon \
		  $(IDIR)/calctool.mask.icon \
		  $(IDIR)/grey.icon \
		  $(IDIR)/menu.normal.icon \
		  $(IDIR)/menu.invert.icon \
		  $(IDIR)/menu.stencil.icon
SOURCES.misc	=

SOURCES.man	= calctool.1
SOURCES.info	= calctool.info

PROGRAM		= calctool
PROJECTDIR	= ..

MANSECT   	= 1

#
# Couple of extra def's so that Guide gets included.  GUIDEHOME has
# to be defined for this Makefile to work anyway.

CPPFLAGS	= -I$(GUIDEHOME)/include
LDFLAGS		= -L$(GUIDEHOME)/lib -L$(PROJECTDIR)/libdeskset
LDLIBS		= -lm -lguidexv

.KEEP_STATE:

default:	all

include ${PROJECTDIR}/Makefile.master.new

TEXTDOMAIN   = SUNW_DESKSET_CALCTOOL_LABEL SUNW_DESKSET_CALCTOOL_MSG SUNW_DESKSET_CALCTOOL_ERR

all:		$(PROGRAM)

clean:		master_clean

install:	install_normal_bins install_info install_man install_icons \
		strings_multi_domain

.INIT:		$(SOURCES.icon)
