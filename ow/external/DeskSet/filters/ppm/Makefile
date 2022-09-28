#
# Makefile for ppm tools.
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
SOURCES.h	= autocad.h   libppm.h    tga.h \
	          ilbm.h      xim.h
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
	@${MAKE} -f Makefile.gouldtoppm
	@${MAKE} -f Makefile.ilbmtoppm 
	@${MAKE} -f Makefile.imgtoppm 
	@${MAKE} -f Makefile.mtvtoppm 
	@${MAKE} -f Makefile.pcxtoppm 
	@${MAKE} -f Makefile.pgmtoppm 
	@${MAKE} -f Makefile.pi1toppm 
	@${MAKE} -f Makefile.picttoppm 
	@${MAKE} -f Makefile.pjtoppm 
	@${MAKE} -f Makefile.qrttoppm 
	@${MAKE} -f Makefile.rawtoppm 
	@${MAKE} -f Makefile.rgb3toppm 
	@${MAKE} -f Makefile.sldtoppm 
	@${MAKE} -f Makefile.spctoppm 
	@${MAKE} -f Makefile.sputoppm 
	@${MAKE} -f Makefile.tgatoppm 
	@${MAKE} -f Makefile.ximtoppm 
	@${MAKE} -f Makefile.xpmtoppm 
	@${MAKE} -f Makefile.yuvtoppm


#
# For I18N string extraction (see Makefile.master)
#
TEXTDOMAIN   = 

install: 
	@${MAKE} -f Makefile.gouldtoppm install
	@${MAKE} -f Makefile.ilbmtoppm install 
	@${MAKE} -f Makefile.imgtoppm install 
	@${MAKE} -f Makefile.mtvtoppm install 
	@${MAKE} -f Makefile.pcxtoppm install 
	@${MAKE} -f Makefile.pgmtoppm install 
	@${MAKE} -f Makefile.pi1toppm install 
	@${MAKE} -f Makefile.picttoppm install 
	@${MAKE} -f Makefile.pjtoppm install 
	@${MAKE} -f Makefile.qrttoppm install 
	@${MAKE} -f Makefile.rawtoppm install 
	@${MAKE} -f Makefile.rgb3toppm install 
	@${MAKE} -f Makefile.sldtoppm install 
	@${MAKE} -f Makefile.spctoppm install 
	@${MAKE} -f Makefile.sputoppm install 
	@${MAKE} -f Makefile.tgatoppm install 
	@${MAKE} -f Makefile.ximtoppm install 
	@${MAKE} -f Makefile.xpmtoppm install 
	@${MAKE} -f Makefile.yuvtoppm install

clean:  master_clobber

.INIT:	$(SOURCES.h) 

