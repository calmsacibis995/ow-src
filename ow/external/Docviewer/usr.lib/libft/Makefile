#
# @(#)Makefile	1.17 93/12/20 Copyright (c) 1991-1992 Sun Microsystems, Inc.
#

PROJECTDIR	=	../..

#LIBRARY	=	libdv
LIBVERSION	=

LIBFT		= libft.a
LIBFTZ		= $(LIBFT)-$(MACH).Z

default:	all

include		$(PROJECTDIR)/Makefile.master

all debug g purify:	$(LIBFT)

$(LIBFT):	$(LIBFTZ)
		rm -f $@
		zcat $(LIBFTZ) > $@
		chmod +w $@
		$(RANLIB) $@
		chmod -w $@

#install:	 install_libs
install:

clean:		master_clean

.INIT:
