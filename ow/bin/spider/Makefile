# @(#)Makefile	1.17 93/09/18

# Makefile for the spider XView demo

# Copyright (c) 1990 by Sun Microsystems, Inc.

SOURCEDIR		= ../..
include ${SOURCEDIR}/config/Master.cfg

COMMAND			= spider
DELIVERABLE		= deliverable
TOOL			= #None
SOURCES			= events.c	\
			  gfx.c		\
			  movelog.c	\
			  spider.c	\
			  util.c	\
			  version.c	\
			  windows.c	\
			  xv_stubs.c	\
			  xv_ui.c
OBJECTS			= ${SOURCES:.c=.o}
HEADERS			= copyright.h	\
			  defs.h	\
			  globals.h	\
			  xv_ui.h	\
			  ${PUBLIC_IMAGES}
DEFINES			= -DXVIEW -DHELPDIR="\"${DOCDIR}\"" -DDATE="\"93/09/18\"" \
			  ${XCLIENTDEFINES}
INCLUDE			= -I${SOURCEDIR}/include -I${XW_HOME}/include
LIBRARIES		= -lxview -lolgx -lXau ${XCLIENTLIBS}
SYSLIBS			= #None

INSTALL.directory	= ${DEST.demo}

MAKE.install.target	= install_publicsource	\
			  install_man	\
			  install_help	\
			  install_file	\
			  remind

DOCDIR			= /usr/openwin/lib/help/spider
INSTALL.publicsource.dir= ${DEST.source.spider}
INSTALL.manpage.dir	= ${DEST.man6}
INSTALL.help.dir	= ${DEST.help}
INSTALL.file.dir	= ${DEST.help.spider}

MANPAGE			= spider.6
HELP			= spider.info 
FILE			= doc.controls	\
			  doc.examples	\
			  doc.intro	\
			  doc.misc	\
			  doc.rules	\
			  doc.summary

# Note: Makefile.customer gets installed in sourcedir as Makefile.

PUBLIC_MISC		= Makefile.customer README Imakefile

PUBLIC_IMAGES		= face.bm rank.bm spider.bm suit.bm

PUBLICSOURCE		= ${SOURCES} ${HEADERS} ${HELP} ${MANPAGE} \
			  ${FILE} ${PUBLIC_MISC} ${PUBLIC_IMAGES}


remind:
	@echo "REMIND: ${COMMAND} has a hardcoded directory -D'd"
	@echo "        DOCDIR	= ${DOCDIR}"

include ${RULESET.command}

