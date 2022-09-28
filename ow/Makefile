#ident @(#)Makefile	1.35 93/12/17

#
# Top level Makefile for the X11/NeWS server and libraries
#

#   +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ 
#  
#                       PROPRIETARY NOTICE
#  
#           This source code is unpublished proprietary 
#           information constituting, or derived under 
#           license from AT&T's UNIX(r) System V. 
#  
#                      Copyright Notice 
#  
#           Notice of copyright on this source code 
#           product does not indicate publication. 
#  
#           Copyright (C) 1986,1990  Sun Microsystems, Inc
#  
#                     All rights reserved. 
#  
#   +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ 
#  
#                   RESTRICTED RIGHTS LEGEND: 
#   Use, duplication, or disclosure by the Government is subject 
#   to restrictions as set forth in subparagraph (c)(1)(ii) of 
#   the Rights in Technical Data and Computer Software clause at 
#   DFARS 52.227-7013 and in similar clauses in the FAR and NASA 
#   FAR Supplement. 
#  
#   +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ 

#

MAKE		= make
SOURCEDIR	= .
HEADERS		= 

include ${SOURCEDIR}/config/Templates/${OWCONFIG}.cfg

# Build order dependencies:

PASS0		= config tools/getbuildtype 
PASS2		= include tools
PASS3		= lib
PASS4		= bin
PASS5		= fonts help man locale
PASS6		= external
PASS7		= examples

ALLPASSES	= ${PASS0} \
		  ${PASS2} \
		  ${PASS3} \
		  ${PASS4} \
		  ${PASS5} \
		  ${PASS6} \
		  ${PASS7}

PASS1		= ${PASS0} \
		  ${PASS2} \
		  ${PASS3} \
		  ${PASS4}

SERVER		= ${SERVER_INCLUDES} ${SERVER_LIBS} bin/xnews
SERVER_INCLUDES	= include/Core \
		  include/NeWS \
		  include/X11 \
		  include/X11server \
		  include/Xau \
		  include/Xext \
		  include/Xinput \
		  include/config \
		  include/cscript \
		  include/dga \
		  include/folio \
		  include/folio.private \
		  include/graphics \
		  include/npsiSun \
		  include/nucleus \
		  include/palloc \
		  include/pixrect \
		  include/portable \
		  include/server \
		  include/sys
SERVER_LIBS	= lib/libcps \
		  lib/libpalloc \
        	  lib/libXau

XVIEW		= ${XVIEW_INCLUDES} ${XVIEW_LIBS} lib/libxview
XVIEW_INCLUDES	= include/X11 \
		  include/Xau \
		  include/olgx \
		  include/images \
		  include/bitmaps \
		  include/pixrect \
		  include/xview \
		  include/xview_private \
		  ${INCLUDE.sundae}
XVIEW_LIBS	= lib/libX11 \
		  lib/libXau \
		  lib/libolgx

pass0 	       := SUBDIRS= ${PASS0}
pass1 	       := SUBDIRS= ${PASS1}
pass2 	       := SUBDIRS= ${PASS2}
pass3 	       := SUBDIRS= ${PASS3}
pass4 	       := SUBDIRS= ${PASS4}
pass5 	       := SUBDIRS= ${PASS5}
pass6 	       := SUBDIRS= ${PASS6}
pass7 	       := SUBDIRS= ${PASS7}
clean 	       := SUBDIRS= ${ALLPASSES}
clobber        := SUBDIRS= ${ALLPASSES}
depopulate     := SUBDIRS= ${ALLPASSES}
populate       := SUBDIRS= ${ALLPASSES}
libs_gprof     := SUBDIRS= lib
libs_debug     := SUBDIRS= lib
libs_lint      := SUBDIRS= lib
server	       := SUBDIRS= ${SERVER}
xnews	       := SUBDIRS= bin/xnews
xnews_reorder  := SUBDIRS= bin/xnews/xnews
xnews_debug    := SUBDIRS= bin/xnews
xnews_gprof    := SUBDIRS= bin/xnews
xlib	       := SUBDIRS= lib/libX11
xlib_reorder   := SUBDIRS= lib/libX11
xview	       := SUBDIRS= ${XVIEW}
xview_sync     := SUBDIRS= ${XVIEW_INCLUDES}

pass0 	       := TARGET=  install
pass1 	       := TARGET=  installtool
pass2 	       := TARGET=  install
pass3 	       := TARGET=  all install
pass4 	       := TARGET=  install
pass5 	       := TARGET=  install
pass6 	       := TARGET=  install
pass7 	       := TARGET=  install
clean	       := TARGET=  clean
clobber	       := TARGET=  clobber
depopulate     := TARGET=  depopulate
populate       := TARGET=  populate
libs_gprof     := TARGET=  installgprof
libs_debug     := TARGET=  installdebug
libs_lint      := TARGET=  installlintlib
server	       := TARGET=  all
xnews	       := TARGET=  all
xnews_reorder  := TARGET=  reorder
xnews_debug    := TARGET=  installdebug
xnews_gprof    := TARGET=  installgprof
xlib	       := TARGET=  all
xlib_reorder   := TARGET=  install_reordered
xview	       := TARGET=  all
xview_sync     := TARGET=  populate

pass0::		step
pass1::		step
pass2::		step
pass3::		step
pass4::		step
pass5::		step
pass6::		step
pass7::		step
clean::	        step
clobber::       step
depopulate::    step
populate::      step
libs_gprof::	step
libs_debug::	step
libs_lint::	step
production::	step
server::	pass0 pass1 step
xnews::		step
xnews_reorder::	step
xnews_debug::	step
xnews_gprof::	step
xlib::		step
xlib_reorder::	step
xview::		pass0 step
xview_sync::	pass0 step

pass0+::	pass0 pass1+
pass1+::	pass1 pass2+
pass2+::	pass2 pass3+
pass3+::	pass3 pass4+
pass4+::	pass4 pass5+
pass5+::	pass5 pass6+
pass6+::	pass6 pass7+
pass7+::	pass7
all::		pass1+
nightly::	all nightly_stuff
galactic::	all galactic_stuff
setup::		pass0
installtool::	pass1
sync::		pass0 pass1 pass2

PWDcommand	= echo "====[${TARGET}]==[ "`date "+%T %D"`"]==["`pwd`" ]===="
DATECMD		= echo "====[ "`date "+%T %D"`" ]===="

SPECIAL_ENVIRONMENT=
xnews_reorder:=		SPECIAL_ENVIRONMENT=OWCONFIG=${OWCONFIG}-reorder
xlib_reorder:=		SPECIAL_ENVIRONMENT=OWCONFIG=${OWCONFIG}-reorder
xnews_gprof:=		SPECIAL_ENVIRONMENT=OWCONFIG=${OWCONFIG}-gprof
libs_gprof:=		SPECIAL_ENVIRONMENT=OWCONFIG=${OWCONFIG}-gprof
xnews_debug:=		SPECIAL_ENVIRONMENT=OWCONFIG=${OWCONFIG}-debug
libs_debug:=		SPECIAL_ENVIRONMENT=OWCONFIG=${OWCONFIG}-debug

step:  $${SUBDIRS} FRC
	@echo "====[<make ${TARGET}> start ]===="
	@${DATECMD}
	@for i in . ${SUBDIRS}; do \
	    ( \
		if [ "x$$i" != "x." ] ; then \
		    cd $$i; \
		    ${PWDcommand}; \
		    ${SPECIAL_ENVIRONMENT} ${MAKE} ${TARGET}; \
		fi \
	    ) \
	 done
	@${DATECMD}
	@echo "====[<make ${TARGET}> done  ]===="

FRC:

galactic_stuff: libs_gprof \
		libs_lint


nightly_stuff:	galactic_stuff \
		xnews_debug \
		xnews_gprof \
		libs_debug

date:
	@echo "${DATECMD:sh}"

huh:: date
	@echo ": OWCONFIG              =" ${OWCONFIG}
	@echo ": Source Directory      =" ${SOURCEDIR}
	@echo ": Defaults Database     =" ${CONFIG.DB}
	@echo ": Config token defined  =" ${CONFIG.XNeWS}
	@echo ": Host OS               =" ${CONFIG.OS}
	@echo ": Compiler version      =" ${CONFIG.Compiler}
	@echo ": Objects directory     =" ${DIR.targetarch}
	@echo ": Prototype install dir =" ${CONFIG.Destdir}

#
# Also in config/Rulesets/*/common
#
do_cmd: FRC
	-@for i in . ${ALLSUBDIRS}; do \
		if [ "x$$i" = "x." ]; then \
			${CMD} ; \
		else \
			( \
			  cd $$i;  echo "----["${PWDCMD:sh}"]----"; \
			  ${MAKE} do_cmd CMD='${CMD}'        \
			) ; \
		fi \
	done

FRC:

.KEEP_STATE:



#-#
#-#
#-#DELIV_pass1	= include tools lib
#-#DELIV_pass2	= bin modules fonts images etc man locale external examples
#-#
#-#ALLSUBDIRS	= ${DELIV_pass1} ${DELIV_pass2}
#-#XVIEW_INCLUDE_SUBDIRS	= X11 Xau olgx images bitmaps pixrect xview xview_private ${INCLUDE.sundae}
#-#
#-#all: setup installtool deliverable
#-#
#-#sync: setup do_include do_tools
#-#
#-#xview_sync:	setup do_xview_includes checkout_xview_headers
#-#
#-#setup: do_config
#-#
#-#galactic: setup installtool deliverable_pass1 install_pass1 \
#-#			    deliverable_pass2 install_pass2 galactic_stuff
#-#
#-#
#-#COMPILER.bin=compiler-bin
#-#compiler:
#-#	rm -rf ${COMPILER.bin}
#-#	mkdir ${COMPILER.bin}
#-#	ln -s /usr/local/lang/SC1.0/* ${COMPILER.bin}
#-#	rm -f ${COMPILER.bin}/CC
#-#	cp /usr/local/lang/SC1.0/CC ${COMPILER.bin}
#-#	rm -f ${COMPILER.bin}/libC.s?.?.?
#-#
#-#do_config: ${HEADERS}
#-#	@echo "Checking on the .cfg and .db files..."
#-#	@cd config; echo "----["`${PWDCMD}`"]----"; ${MAKE} all
#-#	@cd tools/getbuildtype; echo "----["`${PWDCMD}`"]----"; ${MAKE} installtool
#-#
#-#do_include:
#-#	@echo "Checking on the header files ..."
#-#	@cd include/config; echo "----["`${PWDCMD}`"]----"; ${MAKE} installtool
#-#	@cd include/portable; echo "----["`${PWDCMD}`"]----"; ${MAKE} installtool
#-#	@cd include/NeWS; echo "----["`${PWDCMD}`"]----"; ${MAKE} installtool
#-#	@cd tools/cps; echo "----["`${PWDCMD}`"]----"; ${MAKE} installtool
#-#	@cd include; echo "----["`${PWDCMD}`"]----"; ${MAKE} installtool
#-#
#-#do_xview_includes:
#-#	@for i in ${XVIEW_INCLUDE_SUBDIRS}; do \
#-#	(cd include/$$i; echo "----["`${PWDCMD}`"]----"; \
#-#	    ${MAKE} installtool \
#-#	) done
#-#
#-#checkout_xview_headers:
#-#	cd lib/libxview; make install_headers
#-#
#-#do_tools:
#-#	@echo "Checking on the build tools ..."
#-#	@cd tools; echo "----["`${PWDCMD}`"]----"; ${MAKE} installtool
#-#
#-#installtool:
#-#	@echo "Building and installing tools ..."
#-#	@for i in ${ALLSUBDIRS}; do \
#-#	(cd $$i; echo "----["`${PWDCMD}`"]----"; \
#-#	    ${MAKE} installtool \
#-#	) done
#-#
#-#deliverable:
#-#	@echo "Building deliverables ..."
#-#	@for i in ${ALLSUBDIRS}; do \
#-#	(cd $$i; echo "----["`${PWDCMD}`"]----"; \
#-#	    ${MAKE} deliverable \
#-#	) done
#-#
#-#deliverable_pass1:
#-#	@echo "Building deliverables ..."
#-#	@for i in ${DELIV_pass1}; do \
#-#	(cd $$i; echo "----["`${PWDCMD}`"]----"; \
#-#	    ${MAKE} deliverable \
#-#	) done
#-#
#-#deliverable_pass2:
#-#	@echo "Building deliverables ..."
#-#	@for i in ${DELIV_pass2}; do \
#-#	(cd $$i; echo "----["`${PWDCMD}`"]----"; \
#-#	    ${MAKE} deliverable \
#-#	) done
#-#
#-#populate:
#-#	@echo "Populating build tree"
#-#	@for i in ${ALLSUBDIRS}; do \
#-#	(cd $$i; echo "----["`${PWDCMD}`"]----"; \
#-#	    ${MAKE} populate \
#-#	) done
#-#
#-#install:	 date huh
#-#	@for i in ${ALLSUBDIRS}; do \
#-#	(cd $$i; echo "----["`${PWDCMD}`"]----"; \
#-#	    ${MAKE} install \
#-#	) done
#-#
#-#install_pass1:	 date huh
#-#	@for i in ${DELIV_pass1}; do \
#-#	(cd $$i; echo "----["`${PWDCMD}`"]----"; \
#-#	    ${MAKE} install \
#-#	) done
#-#
#-#install_pass2:	 date huh
#-#	@for i in ${DELIV_pass2}; do \
#-#	(cd $$i; echo "----["`${PWDCMD}`"]----"; \
#-#	    ${MAKE} install \
#-#	) done
#-#
#-#clean: date huh
#-#	@for i in ${ALLSUBDIRS}; do \
#-#	(cd $$i; echo "----["`${PWDCMD}`"]----"; \
#-#	    ${MAKE} clean \
#-#	) done
#-#
#-#clobber: date huh
#-#	@for i in ${ALLSUBDIRS}; do \
#-#	(cd $$i; echo "----["`${PWDCMD}`"]----"; \
#-#	    ${MAKE} clobber \
#-#	) done
#-#
