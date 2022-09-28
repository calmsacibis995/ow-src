#
# Makefile for pbm tools.
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
SOURCES.h	= bitreverse.h  g3.h   macp.h 	  \
		  cmuwm.h       mgr.h  
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
	@${MAKE} -f Makefile.atktopbm     
	@${MAKE} -f Makefile.gemtopbm     
	@${MAKE} -f Makefile.pi3topbm
	@${MAKE} -f Makefile.brushtopbm   
	@${MAKE} -f Makefile.icontopbm    
	@${MAKE} -f Makefile.xbmtopbm
	@${MAKE} -f Makefile.cmuwmtopbm   
	@${MAKE} -f Makefile.macptopbm    
	@${MAKE} -f Makefile.ybmtopbm
	@${MAKE} -f Makefile.g3topbm      
	@${MAKE} -f Makefile.mgrtopbm
#
# For I18N string extraction (see Makefile.master)
#
TEXTDOMAIN   = 

install: 
	@${MAKE} -f Makefile.atktopbm install     
	@${MAKE} -f Makefile.gemtopbm install     
	@${MAKE} -f Makefile.pi3topbm install
	@${MAKE} -f Makefile.brushtopbm install   
	@${MAKE} -f Makefile.icontopbm install    
	@${MAKE} -f Makefile.xbmtopbm install
	@${MAKE} -f Makefile.cmuwmtopbm install   
	@${MAKE} -f Makefile.macptopbm install    
	@${MAKE} -f Makefile.ybmtopbm install
	@${MAKE} -f Makefile.g3topbm install      
	@${MAKE} -f Makefile.mgrtopbm install
 
clean:  master_clobber

.INIT:	$(SOURCES.h) 

