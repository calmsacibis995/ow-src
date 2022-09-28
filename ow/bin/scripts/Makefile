#ident @(#)Makefile	1.16 93/09/29

#
# Makefile for the OpenWindows scripts 
#

#
# Copyright (c) 1990 by Sun Microsystems, Inc.
#

SOURCEDIR		= ../..

include ${SOURCEDIR}/config/Master.cfg

#####
#
# REMIND - Why is this here? - JMP
#
OPTARCH-sun4-svr4       =sun4-svr4
OPTARCH                 =${OPTARCH${TARGET_ARCH}}
#
#####

SCRIPT			= 		\
			  openwin	\
			  svenv		\
			  #

INSTALL.script.dir	= ${DEST.bin}

MAKE.install.target	= install_script

include ${RULESET.include}
