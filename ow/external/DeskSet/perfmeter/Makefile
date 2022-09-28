#
# @(#)Makefile 1.11 93/09/22 SMI
#
#
# Expects GUIDEHOME to be set to a Guide heirarchy that
# includes bin-SVR4 & bin-SUNOS directories; e.g.,
# /net/jans/export/DeskSet/ow/V3.2/guide
#

SOURCES.G	= 
SOURCES.c	= perfmeter_ui.c	\
			perfmeter.c	\
			clockhands.c	\
			text.c		\
			xview.c
SOURCES.h	= clockhands.h patchlevel.h perfmeter.h perfmeter_ui.h
SOURCES.l	=
SOURCES.sh	=
SOURCES.y	=
SOURCES.pkg	=
IDIR		= images
SOURCES.icon	= $(IDIR)/dead.icon           \
		   $(IDIR)/rspeedometer.icon   \
		   $(IDIR)/sick.icon           \
		   $(IDIR)/perfmeter.icon      \
		   $(IDIR)/perfmeter.mask.icon
SOURCES.misc	=
SOURCES.man	= perfmeter.1
SOURCES.info  	= perfmeter.info
PROGRAM		= perfmeter
PROJECTDIR	= ..

MANSECT   	= 1

#
# Couple of extra def's so that Guide gets included.  GUIDEHOME has
# to be defined for this Makefile to work anyway.

CPPFLAGS	= -I$(GUIDEHOME)/include
LDFLAGS		= -L$(GUIDEHOME)/lib -L$(PROJECTDIR)/libdeskset
LDLIBS		= -lguidexv -lrpcsvc

.KEEP_STATE:

default:	all

include ${PROJECTDIR}/Makefile.master.new

TEXTDOMAIN   = SUNW_DESKSET_PERFMETER_LABEL SUNW_DESKSET_PERFMETER_MSG SUNW_DESKSET_PERFMETER_ERR


all:		$(PROGRAM)

clean:		master_clean

install:	install_normal_bins install_info install_man install_icons strings_multi_domain

.INIT:		$(SOURCES.icon)

