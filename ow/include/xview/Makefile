##
#
# Makefile for xview public headers
#
# The contents of this dir are symlinks pointing into the xview
# source tree under lib/libxview.  They are placed here when doing
# a make installtool within lib/libxview.
#
# @(#)Makefile	1.2 91/05/28
#
##

#
# Copyright (c) 1990 by Sun Microsystems, Inc.
#

SOURCEDIR	= ../..

include ${SOURCEDIR}/config/Master.cfg

MAKE.tool.target = install_headers
MAKE.installtool.target = install_headers
MAKE.all.target = install_headers

SUBDIRS		= 

include ${RULESET.step}

install_headers:
	cd ${SOURCEDIR}/lib/libxview; make install_headers
