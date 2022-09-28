# RE_SID: @(%)/export/build0/source/SOURCE+SCCS_S297_FCS/external/Docviewer/viewprint/SCCS/s.Makefile 1.12 97/04/11 10:33:07 SMI 
#
# @(#)Makefile	1.12 97/04/11  Copyright (c) 1992 by Sun Microsystems, Inc.
#

PROJECTDIR	=	..

SOURCES.cc	=	viewprint.cc	\
			pthack.cc

SOURCES.h	=	viewprint.h

SOURCES.man	=	viewprint.1
MANSECT		=	1

PROGRAM		=	viewprint

#
# For I18N string extraction (see Makefile.master)
#
TEXTDOMAIN	=	VIEWPRINT

#
# Default target
#
default:	all

include		$(PROJECTDIR)/Makefile.master

#
# Add our tool specific flags
# (XXX - we shouldn't need "-ltt": the libdeskset seems to be pulling it in)
# (XXX - we shouldn't need "-lft" either)
#
#LDLIBS		+=	-lisam -ltt -lft
LDLIBS		+=	-lisam -lft -lw -norunpath

all:		$(PROGRAM)

debug:		$(PROGRAM.g)

purify:		$(PROGRAM.pure)

.KEEP_STATE:

install:	install_normal_bins install_man strings_help_doc_viewer_single_domain

clean:		master_clean

pthack.cc:	$(PROJECTDIR)/pthack.cc
		@rm -f pthack.cc
		ln -s $?

FRC:

.INIT:		$(SOURCES.man) $(SOURCES.h)
