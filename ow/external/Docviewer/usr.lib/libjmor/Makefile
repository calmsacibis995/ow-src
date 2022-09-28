#
# Copyright (c) 1992 by Sun Microsystems, Inc.
# Copyright (c) 1992 by Nihon Sun Microsystems K.K.
#
#ident  "@(#)Makefile 1.10 93/01/29 SMI; JFP; JLE"

PROJECTDIR	=	../..

SOURCES.cc	=

SOURCES.c	=	getmorph.c convl.c convtab.c jistoa.c

SOURCES.q	=

SOURCES.h	=	getmorph.h xctype.h

LIBRARY		=	libjmor
LIBVERSION	=

CPPFLAGS	+=	-DFRM_FILE -DIN_NAV

default:	all

include		$(PROJECTDIR)/Makefile.master

all:		$(LIBRARY.a)

debug:		$(LIBRARY.g)

purify:		$(LIBRARY.pure)

#install:	 install_libs
install:

clean:		master_clean

.INIT:		$(SOURCES.h) $(SOURCES.q)

