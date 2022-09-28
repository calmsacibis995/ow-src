#
# @(#)Makefile 1.15 91/07/08 SMI
#

SOURCEDIR	= ../..

include ${SOURCEDIR}/config/Master.cfg

SUBDIRS		= canvas \
		  color \
		  cursor \
		  defaults \
		  dnd \
		  extensions \
		  fonts \
		  frames \
		  icons \
		  menus \
		  misc \
		  notice \
		  notifier \
		  panels \
		  scrollbar \
		  selection \
		  seln_svc \
		  sv_compat \
		  textsw \
		  ttysw

PUBLICSOURCE	= Makefile.customer

MAKE.install.target		= install_publicsource
INSTALL.publicsource.dir	= ${DEST.source.xview.examples}

include ${RULESET.step}

