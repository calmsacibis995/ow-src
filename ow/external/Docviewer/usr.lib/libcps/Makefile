#
#ident "@(#)Makefile	1.5 12/20/93 Copyright 1991-1992 Sun Microsystems, Inc."
#

PROJECTDIR	=	../..

#LIBRARY	=	libcps
LIBVERSION	=

LIBCPS		= libcps.a
LIBCPSZ		= $(LIBCPS)-$(MACH).Z

default:	all

include		$(PROJECTDIR)/Makefile.master

all debug g purify:	$(LIBCPS)

$(LIBCPS):	$(LIBCPSZ)
		rm -f $@
		zcat $(LIBCPSZ) > $@
		chmod +w $@
		$(RANLIB) $@
		chmod -w $@

#install:	 install_libs
install:

clean:		master_clean

.INIT:
