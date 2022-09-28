# RE_SID: @(%)/export/build0/source/SOURCE+SCCS_S297_FCS/external/Docviewer/viewer/SCCS/s.Makefile 1.164 97/04/04 09:59:15 SMI
#
# @(#)Makefile	1.164 97/04/04 Copyright (c) 1992 by Sun Microsystems, Inc.
#

PROJECTDIR	=	..

SOURCES.cc	=	docfinder.cc	\
			docinfo.cc	\
			dpscan.cc	\
			dpsdebug.cc	\
			hist_stack.cc	\
			main.cc		\
			printwin.cc	\
			psviewer.cc	\
			pthack.cc	\
			ttmgr.cc	\
			uimgr.cc	\
			utils.cc	\
			viewer.cc	\
			winlist.cc	\
			xvcanvas.cc	\
			xvpscan.cc	\
			xvpanel.cc	\
			xvscrollbar.cc	\
			xvsubwin.cc	

SOURCES.h	=	common.h	\
			docfinder.h	\
			docinfo.h	\
			dpscan.h	\
			dpsdebug.h	\
		        dpsexterns.h	\
			dv_ow_servernum.h\
			hist_stack.h	\
			magval.h	\
			printwin.h	\
			psviewer.h	\
			spothelp.h	\
			ttmgr.h		\
			uimgr.h		\
			viewer.h	\
			winlist.h	\
			xvcanvas.h	\
			xvpanel.h	\
			xvpscan.h	\
			xvscrollbar.h	\
			xvsubwin.h	


SOURCES.c	=	dv_ow_servernum_clnt.c \
#			dvutils_dps.c \
			purify.c


SOURCES.cps     = 	dv_cps.cps	 \
			xvpscan_cps.cps
SOURCES.psw     =       dvutils.psw
SOURCES.icon	=	next_page.icon prev_page.icon
SOURCES.info	=	helpviewer.info viewer.info
SOURCES.types	=	helpviewer.types docviewer.types
SOURCES.man	=	docviewer.1 helpviewer.1
MANSECT		=	1

PROGRAM		=	docviewer

#
# For I18N string extraction (see Makefile.master)
#
TEXTDOMAIN	=	DOCVIEWER

#
# Creation date string that appears in the Viewer header
#
CREATION_DATE	=	`date +'%h %d 19%y'`

#
# Default target
#
default:	dvutils_dps.h all


include		$(PROJECTDIR)/Makefile.master

#
# Add our tool specific flags
#
LDLIBS		+=	-ltt -lft -lisam -lcps -lw -ldps -norunpath
LDLIBS		+=	-L /opt/SUNWspro/lib -L /opt/SUNWspro/SC2.0.1 -lm
CPSFLAGS	+=	-f psfile
CPPFLAGS	+=      -I$(OPENWINHOME)/include/X11 -I$(XW_HOME)/include/X11
all:		dvutils_dps.h $(PROGRAM)

debug:		dvutils_dps.h $(PROGRAM.g)

purify:		dvutils_dps.h $(PROGRAM.pure)

.KEEP_STATE:

install:	install_normal_bins install_info install_man \
		install_helpviewer_link strings_help_doc_viewer_single_domain

clean:		master_clean

#
# HelpViewer is simply a symbolic link to the DocViewer binary.
#
install_helpviewer_link:
		@echo "Creating symbolic link for HelpViewer"
		cd $(DESTDIR.bin) && \
		rm -f helpviewer  && \
		ln -s docviewer helpviewer

pthack.cc:	$(PROJECTDIR)/pthack.cc
		@rm -f pthack.cc
		ln -s $?

RPCGEN		=	/bin/rpcgen

dv_ow_servernum_clnt.c:	dv_ow_servernum.x dv_ow_servernum.h
		$(RPCGEN) -l -C -o $@ dv_ow_servernum.x

dv_ow_servernum.h:	dv_ow_servernum.x
		$(RPCGEN) -h -C -o $@ dv_ow_servernum.x

FRC:

.INIT:		$(SOURCES.h) $(SOURCES.info) $(SOURCES.types) \
		$(SOURCES.man) $(SOURCES.icon)
