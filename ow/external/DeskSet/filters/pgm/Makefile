#
# Makefile for pgm tools.
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
SOURCES.h	= dithers.h
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
	@${MAKE} -f Makefile.fitstopgm    
	@${MAKE} -f Makefile.hipstopgm    
	@${MAKE} -f Makefile.psidtopgm
	@${MAKE} -f Makefile.fstopgm      
	@${MAKE} -f Makefile.lispmtopgm   
	@${MAKE} -f Makefile.rawtopgm
#
# For I18N string extraction (see Makefile.master)
#
TEXTDOMAIN   = 

install: 
	 @${MAKE} -f Makefile.fitstopgm install 
	 @${MAKE} -f Makefile.hipstopgm install
	 @${MAKE} -f Makefile.psidtopgm install 
	 @${MAKE} -f Makefile.fstopgm install 
	 @${MAKE} -f Makefile.lispmtopgm install 
	 @${MAKE} -f Makefile.rawtopgm install

clean:  master_clobber

.INIT:	$(SOURCES.h) 

