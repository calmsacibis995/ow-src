# Makefile for getbuildtype and processlibs
#
# @(#)Makefile	1.24 92/11/02

#  ----------------------------------------------------------------- 
#         Copyright (C) 1986,1990  Sun Microsystems, Inc
#                     All rights reserved. 
#           Notice of copyright on this source code 
#           product does not indicate publication. 
#  
#                   RESTRICTED RIGHTS LEGEND: 
#  Use, duplication, or disclosure by the Government is subject 
#  to restrictions as set forth in subparagraph (c)(1)(ii) of 
#  the Rights in Technical Data and Computer Software clause at 
#  DFARS 52.227-7013 and in similar clauses in the FAR and NASA 
#  FAR Supplement. 
#  ----------------------------------------------------------------- 

# This Makefile does not use the flexable ruleset based
# build process because of the chicken/egg problem....
#
# The Ruleset-based build process uses the getbuildtype program,
# so it is obvious that you can't use that process to build this command.

SOURCEDIR	= ../..
include ${SOURCEDIR}/config/Master.cfg

COMMAND			= getbuildtype
DELIVERABLE		=
TOOL			= tool
SOURCES			= getbuildtype.c database.c debugger.c define.c hash.c
OBJECTS			= ${SOURCES:.c=.o}
HEADERS			= database.h debugger.h define.h hashtable.h
DEFINES			=
DERIVEDFILES		= testrun
LIBRARIES		=
INSTALL.tool.directory	= ${TOOL.bin}

CFLAGS			= -Xa -O4

#
# These targets implement the "Ruleset" actions
#
all:: ${COMMAND}
debug:: ${COMMAND}
gprof:: ${COMMAND}
deliverable:: ${COMMAND}

galactic:: install
tool :: all
installtool:: install

.PARALLEL:	${COMMAND} ${OBJECTS}

${COMMAND} : ${OBJECTS}
	${CC} -o ${COMMAND} ${OBJECTS}

${OBJECTS} : ${HEADERS}

install::	${INSTALL.tool.directory}/${COMMAND}

${INSTALL.tool.directory}/${COMMAND} : ${COMMAND}
	@if [ ! -d ${INSTALL.tool.directory} ] ; then \
		mkdir -p ${INSTALL.tool.directory}; \
	fi
	@${ECHO} ${COMMAND} "      =>" ${INSTALL.tool.directory}
	@cp ${COMMAND} ${INSTALL.tool.directory}
	@chmod 755  ${INSTALL.tool.directory}/${COMMAND} 

installdebug:: install
installgprof:: install

clean::
	@${RM} ${OBJECTS} core

clobber:: clean
	@${RM} ${COMMAND}

#
# End of "Ruleset"
#

TESTS		=test-bopt	\
		 test-bdebug	\
		 test-bgprof	\
		 test-bvariants	\
		 test-b-	\
		 test-b		\
		 test-lopt-l	\
		 test-ldebug-l	\
		 test-lgprof-l	\
		 test-lvariants-l	\
		 test-lopt-	\
		 test-ldebug-	\
		 test-lgprof-	\
		 test-lvariants-	\
		 test-lopt	\
		 test-ldebug	\
		 test-lgprof	\
		 test-lvariants	\
		 test-n	\
		 test-nopt	\
		 test-ndebug	\
		 test-ngprof	\
		 test-nvariants	\
		 test-bopt+	\
		 test-bdebug+	\
		 test-bgprof+	\
		 test-bvariants+	\
		 test-n+	\
		 test-nopt+	\
		 test-ndebug+	\
		 test-ngprof+	\
		 test-nvariants+	\
		 test-bvlopt	\
		 test-bvldebug	\
		 test-bvlgprof	\
		 test-bvlmacro	\
		 test-bvlmacroA	\
		 test-bvlmacroF

GETBT=${DIR.targetarch}/getbuildtype -x 1 -g -f test.db -O Obj-sun4
GETLIB=${DIR.targetarch}/getbuildtype -x 1 -p -f test.db -O Obj-sun4 -n xnews
GETLINT=${DIR.targetarch}/getbuildtype -x 1 -L -f test.db -O Obj-sun4 -n xnews

runtests:	test.db testreference
	@( \
	echo ================================ ; \
	echo Date produced: `date`;		\
	echo SCCSID: \%\W\% \%\E\%;		\
	echo ================================ ; \
	echo ;					\
	echo ================================ ; \
	echo Testing build methods ; 		\
	echo ================================ ; \
	for token in ${TESTS} ; do 		\
		fgrep  $$token\	 test.db ; 	\
		echo $$token is built: \\c ; 	\
		${GETBT} -n $$token ; 		\
		echo =-=-=-=-=-=-=-=-=-= ; 	\
	done ; 					\
	echo ================================ ; \
	echo Testing library location lookups ; \
	echo ================================ ; \
	for token in ${TESTS} ; do 		\
		fgrep  $$token\	 test.db ; 	\
		echo $$token is linked: \\c ; 	\
		${GETLIB} -d opt -- -l$$token ; \
		echo ; 				\
		${GETLIB} -d gprof -- -l$$token ; \
		echo ; 				\
		echo =-=-=-=-=-=-=-=-=-= ; 	\
	done ; 					\
	echo ================================ ; \
	echo Testing lint library lookups ;	\
	echo ================================ ; \
	for token in ${TESTS} ; do 		\
		echo $$token is linted: \\c ; 	\
		${GETLINT} -d opt -- -l$$token ; \
		echo ; 				\
	done ; 					\
	) > testrun
	@if [ -f testreference ] ; then diff testreference testrun ; \
	else echo "No Test Results Reference to compare against" ; \
	     echo "See the file   testrun   for the current test output" ;\
	fi

