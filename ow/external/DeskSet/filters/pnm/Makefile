#
# Makefile for pnm tools.
#
# Copyright (C) 1989, 1991 by Jef Poskanzer.
#
# Permission to use, copy, modify, and distribute this software and its
# documentation for any purpose and without fee is hereby granted, provided
# that the above copyright notice appear in all copies and that both that
# copyright notice and this permission notice appear in supporting
# documentation.  This software is provided "as is" without express or
# implied warranty.
#
#

SOURCES.G	= 
SOURCES.c	= 
SOURCES.h	= x10wd.h   x11wd.h
SOURCES.l	=
SOURCES.sh	=
SOURCES.y	=
SOURCES.pkg	=
SOURCES.icon	= 
SOURCES.map	=
SOURCES.man	=
SOURCES.info	= 
PROGRAM		= 

PROJECTDIR	= ../..

MANSECT   	= 1

.KEEP_STATE:

default:	all

include ${PROJECTDIR}/Makefile.master.new

all:
	@${MAKE} -f Makefile.xwdtopnm

#
# For I18N string extraction (see Makefile.master)
#
TEXTDOMAIN   = 

install: 
	@${MAKE} -f Makefile.xwdtopnm install
 
clean:  master_clobber

.INIT:	$(SOURCES.h) 

