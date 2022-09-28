#
# @(#)Makefile	1.21 93/12/20 Copyright (c) 1989-1992 by Sun Microsystems, Inc.
#

PROJECTDIR	=	..

SOURCES.h	=	isam.h	\
			ndbm.h

DIRS		=	doc ft images libdeskset NeWS xvps

#
# Default target
#
default:	all

include		$(PROJECTDIR)/Makefile.master

all install clean:	$(DIRS)
	-@for i in ${DIRS} ; do 					\
		echo "---------- $@ing $$i ----------"; 		\
		cd $$i;							\
		$(MAKE) $@;						\
		cd ..;							\
	done

debug g purify:	all

.KEEP_STATE:

FRC:

.INIT:		$(SOURCES.h)

