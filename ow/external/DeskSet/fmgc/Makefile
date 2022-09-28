#
# @(#)Makefile	@(#)Makefile	1.1.10/08/92 11 Aug 1992 Copyright (c) 1987-1990 Sun Microsystems, Inc.
# All Rights Reserved
#

SOURCES.c	= fmgc.c
SOURCES.man	= fmgc.1
PROGRAM		= fmgc
PROJECTDIR	= ..

MANSECT   	= 1

.KEEP_STATE:

default:	all

include ${PROJECTDIR}/Makefile.master.new

all:	$(PROGRAM)

clean:	master_clean

clobber: master_clobber

#
# For I18N string extraction (see Makefile.master)
#

TEXTDOMAIN   = fmgc.label fmgc.message fmgc.error

install: install_normal_bins install_man strings_multi_domain
