# RE_SID: @(%)/export/build0/source/SOURCE+SCCS_S297_FCS/external/Docviewer/viewopen/SCCS/s.Makefile 1.17 97/04/04 09:51:11 SMI 
#
# @(#)Makefile	1.17 97/04/04  Copyright (c) 1992 by Sun Microsystems, Inc.
#

#
# Note: "viewopen" is no longer distributed as part of DocViewer.
# It is obviated by the new DocViewer ToolTalk stuff.
#

PROJECTDIR	=	..

SOURCES.cc	=	viewopen.cc	\
			pthack.cc

SOURCES.h	=	

#SOURCES.man	=	helpopen.1 viewopen.1
SOURCES.man	=	helpopen.1
MANSECT		=	1

PROGRAM		=	helpopen

#
# For I18N string extraction (see Makefile.master)
#
TEXTDOMAIN	=	HELPOPEN

#
# Default target
#
default:	all

include		$(PROJECTDIR)/Makefile.master

#
# Add our tool specific flags
# (XXX - we shouldn't need "-lft")
#
LDLIBS		+=	-lisam -ltt -lft -lw -norunpath

all:		$(PROGRAM)

debug:		$(PROGRAM.g)

purify:		$(PROGRAM.pure)

.KEEP_STATE:

install:	install_normal_bins install_man strings_help_doc_viewer_single_domain

clean:		master_clean

FRC:

.INIT:		$(SOURCES.man) $(SOURCES.h)
