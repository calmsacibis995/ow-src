# RE_SID: @(%)/export/build0/source/SOURCE+SCCS_S297_FCS/external/Docviewer/admin/SCCS/s.Makefile 1.12 97/04/04 09:39:15 SMI
#
# @(#)Makefile	1.12 97/04/04 Copyright (c) 1992 by Sun Microsystems, Inc.
#

PROJECTDIR	=	..

SOURCES.cc	=	cardcatadmin.cc	\
			cardcat_work.cc	\
			pthack.cc

SOURCES.h	=	cardcat_work.h

SOURCES.man	=	ab_admin.1 ab_cardcatalog.4 ab_library.4

PROGRAM		=	ab_admin

#
# For I18N string extraction (see Makefile.master)
#
TEXTDOMAIN	=	ADMIN

#
# Default target
#
default:	all

include		$(PROJECTDIR)/Makefile.master

#
# Add our tool specific flags
# (XXX - we shouldn't need "-lft")
#
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
