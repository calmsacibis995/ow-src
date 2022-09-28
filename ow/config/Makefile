##
#
# Makefile for the Configuration system
#
# @(#)Makefile	1.3 92/02/02
#
##

#
# Copyright (c) 1990 by Sun Microsystems, Inc.
#


PWDCMD=pwd

SUBDIRS		= Templates OS Compilers Rulesets


clean clobber::

populate tool installtool deliverable all install::	Master.cfg
	@for i in ${SUBDIRS}; do \
	(cd $$i; echo "----["`${PWDCMD}`"]----"; \
	    ${MAKE} all \
	) done


