# RE_SID: @(%)/export/build0/source/SOURCE+SCCS_S297_FCS/external/Docviewer/SCCS/s.Makefile 1.56 95/07/13 10:15:40 SMI
#
# @(#)Makefile	1.56 07/13/95 Copyright 1989-1992 Sun Microsystems, Inc.
#

#
# The following directories are needed to build the standard DocViewer product.
#
PROD_DIRS	=	build_tools	\
			include		\
			usr.lib		\
			lib		\
			answerbook	\
			admin		\
			navigator	\
			viewer		\
			viewopen	\
			viewprint

#
# The following directories are needed to build the AnswerBook
# Publishing tools.  These tools are used internally, and are not
# part of the standard DocViewer product.
#
TOOL_DIRS	=	book2ascii	\
			bookgen		\
			bookdump	\
			docinfo

DIRS		=	$(PROD_DIRS) $(TOOL_DIRS)

.INIT:		Makefile.master

.KEEP_STATE:

all:	$(PROD_DIRS)
	if [ -f  build_tools/cps.exe ] ; then rm -f build_tools/cps.exe ; fi ;
	if [ -f  build_tools/uisschema ] ; then rm -f build_tools/uisschema ; fi ;
	if [ -f  usr.lib/libft/libft.a ] ; then rm -f usr.lib/libft/libft.a; fi ;
	if [ -f  usr.lib/libdv/libdv.a ] ; then rm -f usr.lib/libdv/libdv.a; fi ;
	if [ -f  usr.lib/libisam/libisam.a ] ; then rm -f usr.lib/libisam/libisam.a; fi ;
	if [ -f  usr.lib/libjmor/libjmor.a ] ; then rm -f usr.lib/libjmor/libjmor.a; fi ;
	if [ -f  usr.lib/libcps/libcps.a ] ; then rm -f usr.lib/libcps/libcps.a; fi ;
	-@for i in ${PROD_DIRS} ; do 					\
		echo "---------- Building $$i ----------"; 		\
		cd $$i;							\
		$(MAKE) all;						\
		cd ..;							\
	done

tools:	$(TOOL_DIRS)
	-@for i in ${TOOL_DIRS} ; do 					\
		echo "---------- Building $$i ----------"; 		\
		cd $$i;							\
		$(MAKE) all;						\
		cd ..;							\
	done

install:	$(PROD_DIRS)
	-@for i in ${PROD_DIRS} ; do 					\
		echo "---------- Installing $$i ----------"; 		\
		cd $$i;							\
		$(MAKE) install;					\
		cd ..;							\
	done

install_tools:	$(TOOL_DIRS)
	-@for i in ${TOOL_DIRS} ; do 					\
		echo "---------- Installing $$i ----------"; 		\
		cd $$i;							\
		$(MAKE) install;					\
		cd ..;							\
	done

clean:	$(DIRS)
	-@for i in ${DIRS} ; do 					\
		echo "---------- Cleaning $$i ----------"; 		\
		cd $$i;							\
		$(MAKE) clean;						\
		cd ..;							\
	done

debug:	$(PROD_DIRS)
	if [ -f  build_tools/cps.exe ] ; then rm -f build_tools/cps.exe ; fi ;
	if [ -f  build_tools/uisschema ] ; then rm -f build_tools/uisschema ; fi ;
	if [ -f  usr.lib/libft/libft.a ] ; then rm -f usr.lib/libft/libft.a; fi ;
	if [ -f  usr.lib/libdv/libdv.a ] ; then rm -f usr.lib/libdv/libdv.a; fi ;
	if [ -f  usr.lib/libisam/libisam.a ] ; then rm -f usr.lib/libisam/libisam.a; fi ;
	if [ -f  usr.lib/libjmor/libjmor.a ] ; then rm -f usr.lib/libjmor/libjmor.a; fi ;
	-@for i in ${PROD_DIRS} ; do 					\
		echo "---------- Building (debug)  $$i ----------";	\
		cd $$i;							\
		$(MAKE) debug;						\
		cd ..;							\
	done

purify:	$(PROD_DIRS)
	if [ -f  build_tools/cps.exe ] ; then rm -f build_tools/cps.exe ; fi ;
	if [ -f  build_tools/uisschema ] ; then rm -f build_tools/uisschema ; fi ;
	if [ -f  usr.lib/libft/libft.a ] ; then rm -f usr.lib/libft/libft.a; fi ;
	if [ -f  usr.lib/libdv/libdv.a ] ; then rm -f usr.lib/libdv/libdv.a; fi ;
	if [ -f  usr.lib/libisam/libisam.a ] ; then rm -f usr.lib/libisam/libisam.a; fi ;
	if [ -f  usr.lib/libjmor/libjmor.a ] ; then rm -f usr.lib/libjmor/libjmor.a; fi ;
	-@for i in ${PROD_DIRS} ; do 					\
		echo "---------- Building (purify)  $$i ----------";	\
		cd $$i;							\
		$(MAKE) purify;						\
		cd ..;							\
	done
