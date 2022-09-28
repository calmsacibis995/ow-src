##
#
# Makefile for SV1 -> XView conversion program
#
# @(#)Makefile	1.15 92/05/12
#
##

#
# Copyright (c) 1990 by Sun Microsystems, Inc.
#

SOURCEDIR		= ../..

include ${SOURCEDIR}/config/Master.cfg

SEDFILES		= .full1.sed		\
			  .full2.sed		\
			  .minimal1.sed 	\
			  .minimal2.sed		\
			  .minimal3.sed 	\
			  .minimal4.sed 	\
			  .inter1.sed		\
			  .inter_min2.sed

SCRIPT			= convert_to_xview ${SEDFILES}
FILE			= 
MANPAGE			= convert_to_xview.1

MAKE.install.target	= install_script install_man
INSTALL.manpage.dir	= ${DEST.man1}
INSTALL.script.dir	= ${DEST.xview}
INSTALL.file.dir	= ${DEST.xview}

include ${RULESET.include}

