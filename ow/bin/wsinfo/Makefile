# @(#)Makefile	1.10 96/09/11 SMI
#
# Makefile - wsinfo
#
# Copyright (c) 1992 by Sun Microsystems Inc. - All Rights Reserved

SOURCEDIR               = ../..

include ${SOURCEDIR}/config/Master.cfg

COMMAND			= wsinfo
TOOL			=
DELIVERABLE		= deliverable
SOURCES                 = wsinfo.c helpfile.c
OBJECTS                 = ${SOURCES:.c=.o}
HEADERS                 = helpfile.h

CFLAGS		       += -v -xstrconst

DEFINES                 = ${XCLIENTDEFINES}
INCLUDE                 = -I${SOURCEDIR}/include	\
			  -I${OPENWINHOME}/include	\
			  -I${XW_HOME}/include
LIBRARIES		= -R/usr/openwin/lib -lXol -lXt -lolgx -lXext ${XCLIENTLIBS}
SYSLIBS			= -ladm

HELP			= wsinfo.info
APPDEFAULTS		= WSInfo.ad

INSTALL.directory       = ${DEST.bin}
INSTALL.tool.directory  =
INSTALL.help.dir	= ${DEST.help}
INSTALL.appdefaults.dir	= ${DEST.app-defaults}

MAKE.install.target	= install_help install_appdefaults

include ${RULESET.command}
