#
# @(#)Makefile	1.31 95/07/13 Copyright (c) 1989-1992 by Sun Microsystems, Inc.
#

PROJECTDIR	=	..

DIRS		= libdv libft libjmor libisam libcps

#
# Default target
#
default:	all

include		$(PROJECTDIR)/Makefile.master

all install clean debug g purify:	$(DIRS)
	-@for i in ${DIRS} ; do 					\
		echo "---------- $@ing $$i ----------"; 		\
		cd $$i;							\
		$(MAKE) $@;						\
		cd ..;							\
	done

.KEEP_STATE:

FRC:

.INIT:
