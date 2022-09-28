#
#  Makefile for mp, the PostScript pretty printer.
#
#  @(#)Makefile	3.8 - 94/01/20
#
#  Copyright (c) Steve Holden and Rich Burridge.
#                All rights reserved.
#
#  Permission is given to distribute these sources, as long as the
#  copyright messages are not removed, and no monies are exchanged.
#
#  No responsibility is taken for any errors inherent either
#  to the comments or the code of this program, but if reported
#  to me then an attempt will be made to fix them.
#
#=====================================================================
#
#  It is possible to change the location of the mp prologue file
#  with the -p command line option. It can also be defined at compile
#  time, by uncommenting the macro definition below and setting
#  appropriately. If it's not present, then a sensible default value
#  is used.
#
PROLOGUE = -DPROLOGUE=\"/usr/openwin/share/xnews/client/mp\"
#---------------------------------------------------------------------
#  Mp will extract the user's gecos field, and try to make sense of what
#  it finds. It applies various rules to do this. If this field is too long,
#  then it needs to truncate sensibly. If it's extracted N "words" or has
#  found a comma, then it stops extracting. If an ampersand is found, this
#  is expanded to the users name in capitals.
#  This option allows you to change the number of "words" that mp extracts
#  from the gecos field. By default this is 3. If needed, then it should
#  be uncommented, and set accordingly.
#
#GECOSFIELDS     = -DGECOSFIELDS=3
#---------------------------------------------------------------------
#  Mp will extract upto a certain number of characters from the user's
#  passwd gecos field. By default, this is 18. This option allows you to
#  set this to something different. If needed, then it should be uncommented,
#  and set accordingly.
#
#GECOSLENGTH     = -DGECOSLENGTH=18
#---------------------------------------------------------------------
#  Not all machines have the index() string library function. If you
#  don't have this function then you should uncomment the NOINDEX
#  definition below.
#
#NOINDEX         = -DNOINDEX
#---------------------------------------------------------------------
#  If you are running on a Unix System V machine, then you should
#  uncomment the next definition.
#
#SYSV           = -DSYSV
#=====================================================================

SOURCES.c	= header.c io.c main.c misc.c print.c
SOURCES.h	= extern.h mp.h patchlevel.h
SOURCES.sh	= mailp

SOURCES.ps	= mp.pro.ps 	\
			mp.pro.l.ps	\
			mp.pro.alt.ps	\
			mp.pro.altl.ps	\
			mp.pro.ff.ps	\
			mp.pro.tm.ps	\
			mp.pro.ts.ps	\
			mp.common.ps	\
			mp.pro.fp.ps
SOURCES.man	= mp.1 	\
			mailp.1		\
			newsp.1		\
			digestp.1	\
			filep.1		\
			filofaxp.1	\
			franklinp.1	\
			timemanp.1	\
			timesysp.1

PROGRAM		= mp
PROJECTDIR	= ..

MAILPNAMES 	= newsp digestp filep filofaxp franklinp timemanp timesysp

INFOFILE 	= 
MANSECT  	= 1

CPPFLAGS	= $(GECOSFIELDS) $(GECOSLENGTH) $(NOINDEX) $(PROLOGUE) $(SYSV)

.KEEP_STATE:

default:	all

include ${PROJECTDIR}/Makefile.master.new

DESTDIR.ps	= $(DESTDIR)/share/xnews/client/mp
LDLIBS		= -lw

all:	$(PROGRAM)


install: install_normal_bins install_scripts install_links install_ps install_man

clean:		master_clean

clobber:	master_clobber

#install-a4: common-install
#	   @echo "      Adjusting for A4 paper"
#	   -@cd $(LIBDIR); \
#		cp mp.common.ps /tmp/tmp-pro; \
#		sed < /tmp/tmp-pro > $$pro \
#		   -e 's|^/fullwidth 8.5 inch def|/fullwidth 595 def|' \
#		   -e 's|^/fullheight 11 inch def|/fullheight 842 def|'; \
#		rm /tmp/tmp-pro

install_links:;
	   @echo Making mailp links
	   @cd $(DESTDIR.bin) ; \
	     for name in $(MAILPNAMES); do \
		echo -n $$name " "; \
		rm -f ./$$name; \
		ln -s mailp ./$$name; \
	     done
	   @echo
