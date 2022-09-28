#
# @(#)Makefile	1.100 97/07/08 Copyright (c) 1992 by Sun Microsystems, Inc.
#

PROJECTDIR	=	../..

SOURCES.cc	=	abclient.cc	\
			abinfo.cc	\
			abgroup.cc	\
			abname.cc	\
			attrlist.cc	\
			book.cc		\
			bookmark.cc	\
			bookname.cc	\
			bookshelf.cc	\
			cardcat.cc	\
			cardcats.cc	\
			console.cc	\
			dbmbook.cc	\
			dbmdoc.cc	\
			docname.cc	\
			document.cc	\
			dvlink.cc	\
			errstk.cc	\
			isambook.cc	\
			isamdoc.cc	\
			itimer.cc	\
			notify.cc	\
			pathlist.cc	\
			pathname.cc	\
			pgrange.cc	\
			psdoc.cc	\
			ps_link.cc	\
			query.cc	\
			scopekey.cc	\
			searchdoc.cc	\
			searcher.cc	\
			string.cc	\
			token_list.cc	\
			tooltalk.cc	\
			tt_view_driver.cc \
			utils.cc	\
			xvutils.cc	\
			xxdoc.cc	\
			zonewght.cc

SOURCES.c	=	\
			ndbm.c

SOURCES.q	=	isamschema.q

SOURCES.h	=	isamrec.h dvlocale.h

LIBRARY		=	libdv
LIBVERSION	=

default:	all

include		$(PROJECTDIR)/Makefile.master

all:		$(LIBRARY.a)

debug:		$(LIBRARY.g)

purify:		$(LIBRARY.pure)

install:	strings_multi_domain

#install:	
clean:		master_clean

.INIT:		$(SOURCES.h) $(SOURCES.q)
