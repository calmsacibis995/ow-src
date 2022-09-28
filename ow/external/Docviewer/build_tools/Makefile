#
# @(#)Makefile	1.7 93/12/20 Copyright (c) 1991-1992 Sun Microsystems, Inc.
#

PROJECTDIR	=	..

TOOLS		=	uisschema	\
			cps		\
			cps.exe

default:	all

include		$(PROJECTDIR)/Makefile.master

all debug g purify:	$(TOOLS)

uisschema:	uisschema-$(MACH)
		$(RM) $@
		cp $? $@
		chmod 755 $@

cps:		cps-SVR4
		$(RM) $@
		cp $? $@
		chmod 755 $@

cps.exe:	cps.exe-$(MACH)
		$(RM) $@
		cp $? $@
		chmod 755 $@

install:

clean:		master_clean


.INIT:
