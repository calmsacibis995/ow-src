#
# @(#)Makefile	1.41 95/06/28 Copyright (c) 1989-1992 by Sun Microsystems, Inc.
#

PROJECTDIR	=	..


LIBSRCDIR	=	$(PROJECTDIR)/usr.lib
LIB_LINKS	=	libdv.a		\
			libft.a		\
			libisam.a	\
			libjmor.a	\
			libcps.a

#
# Default target
#
default:	all

include		$(PROJECTDIR)/Makefile.master

all debug g purify:	$(LIB_LINKS)

$(LIB_LINKS):
		rm -f $@
		?ln -s $(LIBSRCDIR)/$(@:%.a=%)/$@ $@

install:

clean:
		$(RM) $(LIB_LINKS)


.KEEP_STATE:

FRC:

.INIT:
