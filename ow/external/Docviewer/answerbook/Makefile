#
# @(#)Makefile	1.3 93/12/20 Copyright (c) 1992 by Sun Microsystems, Inc.
#

PROJECTDIR	=	..

SOURCES.sh	=	answerbook.sh

SOURCES.man	=	answerbook.1
MANSECT		=	1

SCRIPTS		=	answerbook

#
# Default target
#
default:	all

include		$(PROJECTDIR)/Makefile.master

all debug purify:	$(SCRIPTS)

.KEEP_STATE:

install:	install_scripts install_man

clean:		master_clean

FRC:

.INIT:		$(SOURCES.man)
