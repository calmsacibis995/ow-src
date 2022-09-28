#
# @(#)Makefile	3.6 13 Aug 1992 
# 
# Makefile for pbmplus tools.
#
# Copyright (C) 1989, 1991 by Jef Poskanzer.
#
# Permission to use, copy, modify, and distribute this software and its
# documentation for any purpose and without fee is hereby granted, provided
# that the above copyright notice appear in all copies and that both that
# copyright notice and this permission notice appear in supporting
# documentation.  This software is provided "as is" without express or
# implied warranty.

# CONFIGURE: gcc makes things go faster on some machines, but not everyone
# has it.  Warning: do not use gcc's -finline-functions or -fstrength-reduce
# flags, they can produce incorrect code.  (This is with gcc versions 1.35,
# 1.36, and 1.37, later versions may fix these bugs.)  Also, on some systems
# gcc can't compile pnmconvol - dunno why.  And on some systems you can't
# use the -ansi flag, it gives compilation errors in <math.h>.

SOURCES.G	= 
SOURCES.c	= 
SOURCES.h	= pbmplus.h version.h
SOURCES.l	=
SOURCES.sh	=
SOURCES.y	=
SOURCES.pkg	=
SOURCES.icon	= 
SOURCES.map	=
SOURCES.man	= 
SOURCES.info	= 
PROGRAM		=
PROJECTDIR	= ..

MANSECT   	= 

#
# Couple of extra def's so that Guide gets included.  GUIDEHOME has
# to be defined for this Makefile to work anyway.

#CPPFLAGS	= -I$(GUIDEHOME)/include
#LDFLAGS	= -L$(GUIDEHOME)/lib
#LDLIBS		= -lm -lguidexv

.KEEP_STATE:

default:	all

include ${PROJECTDIR}/Makefile.master.new

SUBDIRS = pbm pgm ppm pnm

all: 	
	cd pbm; ${MAKE} -f Makefile.pbmlib
	cd pgm; ${MAKE} -f Makefile.pgmlib
	cd ppm; ${MAKE} -f Makefile.ppmlib
	cd pnm; ${MAKE} -f Makefile.pnmlib
	cd pbm; ${MAKE} 
	cd pgm; ${MAKE}
	cd ppm; ${MAKE}
	cd pnm; ${MAKE}

clean:	master_clobber
	rm -f *.shar *.shar? art.*
	for i in $(SUBDIRS) ; do \
	    ( echo $$i ; cd $$i ; ${MAKE} clean ); \
	done
#
# For I18N string extraction (see Makefile.master)
#
TEXTDOMAIN   = 

install: 
	for i in $(SUBDIRS) ; do \
	    ( echo $$i ; cd $$i ; ${MAKE} install ); \
	done

.INIT:	$(SOURCES.h)

