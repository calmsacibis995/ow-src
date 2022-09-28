#
# Makefile for the Olit Programmer's Guide's Examples
#
# @(#)Makefile	1.6 93/01/26  SMI
#

SOURCEDIR       = ../../..
 
include ${SOURCEDIR}/config/Master.cfg

SUBDIRS		= app-defaults \
		  ch2 \
		  ch3 \
		  ch4 \
		  ch5 \
		  ch6 \
		  ch7 \
		  ch9 \
		  ch10 \
		  ch11 \
		  ch12 \
		  ch13 \
		  ch14 \
		  help

PUBLICSOURCE    = README Makefile.customer
 
MAKE.install.target             = install_publicsource
INSTALL.publicsource.dir        = ${DEST.source.olit.olitbook}

include ${RULESET.step}
