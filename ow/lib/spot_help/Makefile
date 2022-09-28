# @(#)Makefile	1.16 93/12/17 spot_help SMI
#
#         Copyright (C) 1991  Sun Microsystems, Inc
#                   All rights reserved.
#           Notice of copyright on this source code
#           product does not indicate publication.
# 
# RESTRICTED RIGHTS LEGEND: Use, duplication, or disclosure by 
# the U.S. Government is subject to restrictions as set forth 
# in subparagraph (c)(1)(ii) of the Rights in Technical Data
# and Computer Software Clause at DFARS 252.227-7013 (Oct. 1988) 
# and FAR 52.227-19 (c) (June 1987).
#
# Sun Microsystems, Inc.
# 2550 Garcia Avenue
# Mountain View, California 94043
#

SOURCEDIR	= ../..

include ${SOURCEDIR}/config/Master.cfg

DERIVEDFILES		= attr_names
SUPPORT			= ttyswrc
INSTALL.support.dir	= ${DEST.lib}

HELP			= xview.info	\
			  textsw.info	\
			  ttysw.info	\
			  tnt.info	\
			  attr_names

INSTALL.help.dir	= ${DEST.help}

MAKE.install.target	= install_support install_help

install_support: ${INSTALL.support.dir} ${SUPPORT}
	@for i in ${SUPPORT}  ; do \
	  if cmp -s $$i ${INSTALL.support.dir}/.$$i ; \
	    then : ; \
	    else \
		${ECHO} "$$i	=> ${INSTALL.support.dir}/.$$i" ; \
		${TINSTALL.bsd} $$i ${INSTALL.support.dir} ; \
		${TINSTALL.posix} -f ${INSTALL.support.dir} $$i ; \
		${MV} ${INSTALL.support.dir}/$$i ${INSTALL.support.dir}/.$$i ; \
	    fi; \
	 done

include ${RULESET.include}

attr_names: ${TOOL.bin}/print_attr_names
	${TOOL.bin}/print_attr_names
