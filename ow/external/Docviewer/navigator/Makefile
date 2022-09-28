# RE_SID: @(%)/export/build0/source/SOURCE+SCCS_S297_FCS/external/Docviewer/navigator/SCCS/s.Makefile 1.155 97/04/04 10:04:49 SMI
#
# @(#)Makefile	1.155 97/04/04 Copyright (c) 1992 by Sun Microsystems, Inc.
#

PROJECTDIR	=	..

SOURCES.cc	=	abbrowser.cc	\
			bookmarkedit.cc	\
			bookmarklist.cc	\
			bookmarkmgr.cc	\
			contents.cc	\
			hitlist.cc	\
			inputwin.cc	\
			location.cc	\
			main.cc		\
			modemgr.cc	\
			navigator.cc	\
			navutils.cc	\
			pthack.cc	\
			queryhist.cc	\
			queryprops.cc	\
			searchmgr.cc	\
			tocmgr.cc	\
			uimgr.cc	\
			winlist.cc	

SOURCES.h	=	abbrowser.h	\
			bookmarkedit.h	\
			bookmarklist.h	\
			bookmarkmgr.h	\
			contents.h	\
			hitlist.h	\
			inputwin.h	\
			location.h	\
			modemgr.h	\
			navigator.h	\
			queryhist.h	\
			queryprops.h	\
			searchmgr.h	\
			spothelp.h	\
			tocmgr.h	\
			uimgr.h		\
			winlist.h	\
			xview.h

SOURCES.icon	=	../include/images/ab_library.icon \
			../include/images/ab_library.mask.icon
SOURCES.info	=	navigator.info
SOURCES.types	=	navigator.types
SOURCES.man	=	navigator.1
MANSECT		=	1

CEDIR		=	${DESTDIR}/lib/cetables
AB_CE_ENTRY	=	../include/images/AB_ce_entry

PROGRAM		=	navigator

#
# For I18N string extraction (see Makefile.master)
#
TEXTDOMAIN	=	NAVIGATOR

#
# Creation date string that appears in the Viewer header
#
CREATION_DATE	=	`date +'%h %d 19%y'`

#
# Default target
#
default:	all

include		$(PROJECTDIR)/Makefile.master

#
# Add our tool specific flags
#
LDLIBS		+=	-ltt -lft -lisam -lw -norunpath

#
# Uncomment this line to get "Comments..." window
# CPPFLAGS	+=	-DUSER_COMMENTS

.KEEP_STATE:

all:		$(PROGRAM)

debug:		$(PROGRAM.g)

purify:		$(PROGRAM.pure)

install:	install_normal_bins install_icons install_info install_man \
		install_ce_db strings_help_doc_viewer_single_domain

install_ce_db:	
		@CEPATH=${CEDIR}; export CEPATH; \
		${CE_DB_MERGE} network -from_ascii $(AB_CE_ENTRY) > /dev/null

clean:		master_clean

pthack.cc:	$(PROJECTDIR)/pthack.cc
		@rm -f pthack.cc
		ln -s $?

FRC:

.INIT:		$(SOURCES.h) $(SOURCES.info) $(SOURCES.types) $(SOURCES.man) \
		$(SOURCES.icon) $(AB_CE_ENTRY)
