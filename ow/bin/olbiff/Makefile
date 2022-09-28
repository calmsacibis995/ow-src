#	@(#)Makefile	1.18	95/02/22 bin/olbiff SMI	/* OLIT	*/
#
# Makefile for the OpenWindows olbiff program
#

#
#        Copyright (C) 1992, 1993  Sun Microsystems, Inc
#			All rights reserved.
#		Notice of copyright on this source code 
#		product does not indicate publication. 
# 
# RESTRICTED RIGHTS LEGEND: Use, duplication, or disclosure by 
# the U.S. Government is subject to restrictions as set forth 
# in subparagraph (c)(1)(ii) of the Rights in Technical Data
# and Computer Software Clause at DFARS 252.227-7013 (Oct. 1988) 
# and FAR 52.227-19 (c) (June 1987).
#
#	Sun Microsystems, Inc., 2550 Garcia Avenue,
#	Mountain View, California 94043.
#


SOURCEDIR		= ../..

include ${SOURCEDIR}/config/Master.cfg

SOURCES			= olbiff.c enconv.c mclist.c

OBJECTS                 = ${SOURCES:.c=.o}
COMMAND:sh		= basename `pwd`
DELIVERABLE		= deliverable
TOOL			= # None

HEADERS 		= mclist.h enconv.h
DEFINES			= -DSYSV ${XCLIENTDEFINES}
INCLUDE                 = -I${SOURCEDIR}/include -I${XW_HOME}/include
LIBRARIES		= -lXol -lolgx -lXt -lXext -lX11 
SYSLIBS                 = -lintl -lw -lmail -ldl

INSTALL.directory       = ${DEST.demo}

MAKE.install.target	= strings


#
# Import OW build rules, but with local overrides
#

include ${RULESET.command}


#
# Local rulesets
#

clobber:: local_clobber

local_clobber:
	-@[ -d .sb ] && ${RMRECURSE} .sb
	${RM} tags

# Macros for string extraction
TEXTDOMAIN = SUNW_WST_OLBIFF
# XGETTEXT = /usr/bin/xgettext -j -n -m "" -p${DEST.locale.msg_templates}

strings: ${SOURCES} ${HEADERS}
	-@echo "Extracting Strings:"
	@${RM} ${DEST.locale.msg_templates}/${TEXTDOMAIN}.po \
		${DEST.locale.msg_templates}/messages.po
	@touch ${DEST.locale.msg_templates}/${TEXTDOMAIN}.po
	-@for i in ${SOURCES} ; do \
		echo "$$i"; \
		${CC} ${DEFINES} ${INCLUDE} -E -C $$i | ${XGETTEXT} -c STRING_EXTRACTION -; \
	done


# end of Makefile
