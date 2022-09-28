#
# @(#)Makefile 3.10 93/09/18 SMI
# Makefile - pageview

#
# Copyright (c) 1990 by Sun Microsystems Inc. - All Rights Reserved
#
#
#

SOURCES.c	= dragdrop.c edit.c emulator_dps.c file.c pageview.c parser.c \
		  print.c props.c tooltalk.c xv.c
SOURCES.cps	= emulator.cps
SOURCES.psw	= emulator.psw
SOURCES.h	= pageview.h patchlevel.h tags.h
SOURCES.types	= pageview.types
SOURCES.icon	= dupedoc_drag.icon dupedoc_drop.icon ps.icon psmask.icon
SOURCES.man	= pageview.1
SOURCES.info	= pageview.info
PROGRAM		= pageview
PROJECTDIR	= ..

MANSECT   	= 1

#
# Couple of extra def's so that Guide gets included.  GUIDEHOME has
# to be defined for this Makefile to work anyway.

#CPPFLAGS	= -I$(GUIDEHOME)/include
#LDFLAGS	= -L$(GUIDEHOME)/lib
CPPFLAGS	+= -I../libdstt 
LDFLAGS		+= -L../libdstt
LDLIBS		= -Bstatic -lcps -Bdynamic -ldstt -ltt -ldps -lm

SVR4_CPS= PATH="$${PATH}:${DESTDIR}/bin:$${XW_HOME}/bin" ; export PATH; cps -o macros
XFP_CPS= PATH="$${PATH}:${DESTDIR}/bin:$${XW_HOME}/bin" ; export PATH; cps -o macros
SUNOS_CPS= PATH="$${PATH}:${DESTDIR}/bin" ; export PATH; cps -o macros
JLE_CPS= $(SUNDAEHOME)/bin/cps -o macros
CPS = $($(TARG_SYS)_CPS)

.KEEP_STATE:

default:	all

include ${PROJECTDIR}/Makefile.master.new

CPPFLAGS	+= -I$(WIN_HOME)/include/X11 -I$(XW_HOME)/include/X11

all:	$(PROGRAM)
	@${MAKE} -f Makefile.fixframe all
	@${MAKE} -f Makefile.fixinterleaf all

clean:	master_clean

clobber: 	master_clobber

#
# For I18N string extraction (see Makefile.master)
#
TEXTDOMAIN   = SUNW_DESKSET_PAGEVIEW_LABEL SUNW_DESKSET_PAGEVIEW_MSG SUNW_DESKSET_PAGEVIEW_ERR

install: install_normal_bins install_info install_man strings_multi_domain \
	 install_tt_types
	 @${MAKE} -f Makefile.fixframe install
	 @${MAKE} -f Makefile.fixinterleaf install
 

.INIT:	$(SOURCES.icon)
