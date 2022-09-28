#ident @(#)Makefile	23.14 94/03/29

#
# Makefile for OpenWindows Man pages
#

#
# Copyright (c) 1990 by Sun Microsystems, Inc.
#

SOURCEDIR		= ..

include ${SOURCEDIR}/config/Master.cfg

SUBDIRS			= man1 man6 man7
SCRIPT			= makewhatis

MAKE.install.target	= install_script .WAIT run_makewhatis
INSTALL.script.dir	= ${DEST.man}

include ${RULESET.step}

##
# NOTE: This is done AFTER the SUBDIRS ade installed
##
run_makewhatis: makewhatis
	${DEST.man}/makewhatis ${DEST.man}

