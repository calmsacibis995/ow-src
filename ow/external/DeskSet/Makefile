#
# @(#)Makefile	3.25 02/22/93 Copyright (c) 1992 Sun Microsystems, Inc.
# All Rights Reserved
#

LIBS	=		\
	libdeskset	\
	libdstt		\
	icons		\
	ce		\
	libtiff

TOOLS	=		\
	binder		\
	calctool	\
	clock		\
	cm		\
	colorchooser	\
	dig_samples	\
	ds_server_init	\
	filemgr		\
	fmgc		\
	filters		\
	ff.core		\
	format_floppy	\
	iconedit	\
	imagetool	\
	mailprint	\
	mailtool	\
	mp		\
	pageview	\
	perfmeter	\
	printtool	\
	snapshot	\
	tapetool	\
	textedit


DESTDIR:sh = echo ${DESTDIR}

#
# Get the version of OpenWindows we are building with
#
OWVERSION:sh = \
	if [ -f ../../../config/OWversion ]; then \
		grep OpenWindows ../../../config/OWversion; \
	fi

.KEEP_STATE:

.NO_PARALLEL: 

all_install:	all_install_libs tools install_tools
all:		libs tools 
install:	install_libs install_tools
clean:		clean_libs clean_tools

libs:	$(LIBS)
	-@for i in ${LIBS} ; do 	\
		echo "---------- Building $$i ----------"; 		\
		cd $$i;				\
		${MAKE} all;			\
		cd ..;				\
	done

install_libs: destdir
	-@for i in ${LIBS} ; do 	\
		echo "---------- Installing $$i ----------"; 		\
		cd $$i;				\
		${MAKE} "DESTDIR=${DESTDIR}" "OWVERSION=${OWVERSION}" install; \
		cd ..;				\
	done

all_install_libs: $(LIBS) destdir
	-@for i in ${LIBS} ; do		\
		cd $$i;                         \
		echo "---------- Building $$i ----------";              \
		${MAKE} all;                    \
		echo "---------- Installing $$i ----------"; 		\
		${MAKE} "DESTDIR=${DESTDIR}" "OWVERSION=${OWVERSION}" install; \
		cd ..; 				\
	done


clean_libs: $(LIBS)
	-@for i in ${LIBS} ; do 	\
		echo "---------- Cleaning $$i ----------"; 		\
		cd $$i;				\
		${MAKE} clean;			\
		cd ..;				\
	done


tools:	$(TOOLS)
	-@for i in ${TOOLS} ; do 	\
		echo "---------- Building $$i ----------"; 		\
		cd $$i;				\
		${MAKE} all;				\
		cd ..;				\
	done

install_tools: destdir
	-@for i in ${TOOLS} ; do 	\
		echo "---------- Installing $$i ----------"; 		\
		cd $$i;				\
		${MAKE} "DESTDIR=${DESTDIR}" "OWVERSION=${OWVERSION}" install; \
		cd ..;				\
	done

all_install_tools: $(TOOLS) destdir
	-@for i in ${TOOLS} ; do		\
		cd $$i;                         \
		echo "---------- Building $$i ----------";              \
		${MAKE} all;                    \
		echo "---------- Installing $$i ----------"; 		\
		${MAKE} "DESTDIR=${DESTDIR}" "OWVERSION=${OWVERSION}" install; \
		cd ..;  			\
	done


clean_tools: $(TOOLS)
	-@for i in ${TOOLS} ; do 	\
		echo "---------- Cleaning $$i ----------"; 		\
		cd $$i;				\
		${MAKE} clean;			\
		cd ..;				\
	done

destdir: create-installdir
	@echo "---------- Creating $$DESTDIR ----------"; \
	if [ $(DESTDIR) ]; then \
		create-installdir $(DESTDIR); \
	else \
		echo "You must set DESTDIR to perform an install"; \
		exit 1; \
	fi;
	
