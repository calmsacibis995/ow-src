#	@(#)Makefile	1.33    93/12/20 bin SMI    /* OLIT	*/
#
# Makefile for the OpenWindows OLIT Sampler program
#

#
#        Copyright (C) 1986, 1992  Sun Microsystems, Inc
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

SOURCES			= olitsampler.c
SPOTHELP		= olitsampler.info
CUSTOMERMAKEFILE	= customer-svr4.mk

OBJECTS                 = ${SOURCES:.c=.o}

PUBLICSOURCE		= ${CUSTOMERMAKEFILE} \
			  ${SOURCES} \
			  ${APPDEFAULTS.ad} \
			  ${SPOTHELP}
			  
COMMAND:sh		= basename `pwd`
DELIVERABLE		= deliverable
TOOL			= # None

INCLUDE                 = -I${SOURCEDIR}/include -I${XW_HOME}/include
LIBRARIES		= -lXol -lolgx -lXt -lXext -lX11 
SYSLIBS                 = -lintl -lw -lgen

#   The SVr4/Solaris2 install doesn't have a `strip' option (and uses
#   `-s' for something completely different.)  If we really need to
#   strip this at this point, we'll have to override an appropriate
#   rule instead.
#
#BINSTALL        	= ${local.INSTALL} -s

INSTALL.directory       = ${DEST.demo}
INSTALL.appdef.dir	= ${DEST.app-defaults}
HELP			= ${SPOTHELP}
INSTALL.help.dir	= ${DEST.help}
INSTALL.publicsource.dir= ${DEST.source.olit}/${COMMAND}

MAKE.install.target	= install_help \
			  install_appdef \
			  install_publicsources \
			  strings

APPDEFAULTS		= Olitsampler
APPDEFAULTS.ad		= Olitsampler.ad

Olitsampler:


#
# Import OW build rules, but with local overrides
#

include ${RULESET.command}
include ${RULESET.include}


#
# Local rulesets
#

#
# Install application defaults file
#
#	in:	APPDEFAULTS
#		INSTALL.appdef.dir	where to install
#
install_appdef:	${APPDEFAULTS.ad}
	@if [ ! -d ${INSTALL.appdef.dir} ]; then \
		echo "created [" ${INSTALL.appdef.dir} "]"; \
		${DINSTALL} ${INSTALL.appdef.dir}; \
	fi; \
	cmp -s ${APPDEFAULTS.ad} ${INSTALL.appdef.dir}/${APPDEFAULTS} || { \
		${ECHO} \
	"${APPDEFAULTS.ad} 	=> ${INSTALL.appdef.dir}/${APPDEFAULTS}"; \
		${TINSTALL.bsd} ${APPDEFAULTS.ad} ${INSTALL.appdef.dir} ; \
		${TINSTALL.posix} -f ${INSTALL.appdef.dir}  ${APPDEFAULTS.ad} ; \
		${MV} ${INSTALL.appdef.dir}/${APPDEFAULTS.ad} ${INSTALL.appdef.dir}/${APPDEFAULTS} ; \
	};

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
		${MV} ${INSTALL.publicsource.dir}/$$file ${INSTALL.publicsource.dir}/Makefile ; \
		;; \
	      *) \
		${ECHO} "$$file 	=> ${INSTALL.publicsource.dir}"; \
		${TINSTALL.bsd} $$file ${INSTALL.publicsource.dir} ; \
		${TINSTALL.posix} -f ${INSTALL.publicsource.dir} $$file ; \
		;; \
	      esac; \
	  }; \
	done


${DIR.lint}/%.ln: ${DIR.lint} %.c ${HEADERS}
	-@${LINT.c} -u ${ln.flag} $< 2>&1 && mv ${<:.c=.ln} $@ \
		| tee ${@:.ln=.lo} 
	@if [ -s ${@:.ln=.lo} ]; then \
		${ECHO} Warning: $< has lint. \
			See `${PWDCMD}`/${@:.ln=.lo}.; \
	else \
		${RM} ${@:.ln=.lo}; \
	fi

clobber:: local_clobber

local_clobber:
	-@[ -d .sb ] && ${RMRECURSE} .sb
	${RM} tags

# Macros for string extraction
TEXTDOMAIN = SUNW_WST_OLITSAMPLER
# XGETTEXT = /usr/bin/xgettext -j -n -m "" -p${DEST.locale.msg_templates}

strings: ${SOURCES} ${HEADERS}
	-@echo "Extracting Strings:"
	@rm -f ${DEST.locale.msg_templates}/${TEXTDOMAIN}.po
	-@for i in ${SOURCES} ; do \
		echo "$$i"; \
		${CC} ${DEFINES} ${INCLUDE} -E -C $$i | ${XGETTEXT} -; \
	done


# end of Makefile
