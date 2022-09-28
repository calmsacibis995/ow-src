#	@(#)Makefile	1.7	93/12/17 share/src/olit/oldials SMI	/* OLIT	*/
#
# Makefile for the OpenWindows oldials example program
#

#
#        Copyright (C) 1986,1992  Sun Microsystems, Inc
#			All rights reserved.
#		Notice of copyright on this source code 
#		product does not indicate publication. 
# 
#  RESTRICTED RIGHTS LEGEND: Use, duplication, or disclosure by
#  the U.S. Government is subject to restrictions as set forth
#  in subparagraph (c)(1)(ii) of the Rights in Technical Data
#  and Computer Software Clause at DFARS 252.227-7013 (Oct. 1988)
#  and FAR 52.227-19 (c) (June 1987).
# 
#	Sun Microsystems, Inc., 2550 Garcia Avenue,
#	Mountain View, California 94043.
#


SOURCEDIR		= ../../..

include ${SOURCEDIR}/config/Master.cfg

SOURCES			= oldials.c
	  
CUSTOMERMAKEFILE	= customer.mk

PUBLICSOURCE		= ${CUSTOMERMAKEFILE}	\
			  ${SOURCES}		

COMMAND:sh		= basename `pwd`
DELIVERABLE		= deliverable
TOOL			= # None
			  
INSTALL.publicsource.dir= ${DEST.source.olit}/${COMMAND}

MAKE.install.target	= \
			  install_publicsources
#
# Local rulesets
#

this_make_files_first_target:
	#EMPTY#

#
# Install source files
#
#	in:	PUBLICSOURCE
#		INSTALL.publicsource.dir	where to install
#
install_publicsources: ${PUBLICSOURCE}
	@if [ ! -d ${INSTALL.publicsource.dir} ]; then \
		echo "created [" ${INSTALL.publicsource.dir} "]"; \
		${DINSTALL} ${INSTALL.publicsource.dir}; \
	fi; \
	for file in ${PUBLICSOURCE}; do \
	  cmp -s $$file ${INSTALL.publicsource.dir}/$$file || { \
	      case "$$file" in \
	      customer*.mk) \
		${ECHO} \
		  "$$file 	=> ${INSTALL.publicsource.dir}/Makefile"; \
		${TINSTALL.bsd} $$file ${INSTALL.publicsource.dir} ; \
		${TINSTALL.posix} -f ${INSTALL.publicsource.dir} $$file ; \
		${MV} ${INSTALL.publicsource.dir}/$$file  ${INSTALL.publicsource.dir}/Makefile ; \
		;; \
	      *) \
		${ECHO} "$$file 	=> ${INSTALL.publicsource.dir}"; \
		${TINSTALL.bsd} $$file ${INSTALL.publicsource.dir} ; \
		${TINSTALL.posix} -f ${INSTALL.publicsource.dir} $$file ; \
		;; \
	      esac; \
	  }; \
	done


include ${RULESET.include}


