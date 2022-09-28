##
#
# Makefile for libXt application defaults files
#
# @(#)Makefile	1.11 94/05/02
##

#
# Copyright (c) 1990 by Sun Microsystems, Inc.
#

SOURCEDIR		= ../..

include ${SOURCEDIR}/config/Master.cfg

SUBDIRS			= play

INSTALL.help.dir	= ${DEST.help.handbooks} 

SOURCES			= ab.handbook.Z			\
			  audio.handbook.Z		\
			  binder.handbook.Z		\
			  calc.handbook.Z		\
			  clock.handbook.Z		\
			  cm.handbook.Z			\
			  desktop.intro.handbook.Z	\
			  filemgr.handbook.Z		\
			  iconedit.handbook.Z		\
			  imagetool.handbook.Z		\
			  mail.handbook.Z		\
			  nohelp.handbook.Z		\
			  perfmeter.handbook.Z		\
			  print.handbook.Z		\
			  question.handbook.Z		\
			  snapshot.handbook.Z		\
			  tape.handbook.Z		\
			  textedit.handbook.Z		\
			  top.toc.handbook.Z		\
			  tty.handbook.Z		\
			  workspace.handbook.Z		\
			  sample.txt

#HELP			= ${SOURCES:%=%.Z}
HELP			= ${SOURCES}

OBSOLETE		= pm.handbook			\
			  color.handbook		\
			  docviewer.handbook		\
			  dummy1.handbook		\
			  dummy2.handbook

MAKE.install.target	= install_help

include ${RULESET.step}

%.Z: %
	rm -f $@
	compress -cf $< > $@
