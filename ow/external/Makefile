# @(#)Makefile	1.34 94/09/30 SMI

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

#	Makefile for external packages

#	Since none of these packages use the primary OW build
#	environment, anything we pass down that is relative to
#	the build tree must be an absolute pathname.
#
PWD:sh			= pwd
SOURCEDIR		= ${PWD}/..
OWVersionFile		= ${SOURCEDIR}/config/OWversion

include ${SOURCEDIR}/config/Master.cfg

AUDIOTOOLDIR		= AudioTool
DESKSETDIR		= DeskSet
DOCVIEWERDIR		= Docviewer
TOOLTALKDIR		= ToolTalk/tt

#   All of the `external' components seem to rely on having DESTDIR
#   and/or OPENWINHOME set.  DESTDIR will come from either the environment
#   or command-line make macro specification.
ENVIRONMENT		= DESTDIR=$$DESTDIR \
			  XW_HOME=$$XW_HOME \
			  OPENWINHOME=$$DESTDIR \
			  OWVersion="`grep OpenWindows ${OWVersionFile}` `date +'%e %B %Y'`"

#   The `MAKE=${MAKEsequential}' is duplicated in each component-specific 
#   environment in the expectation that they will become ParallelMake safe
#   at some point after the bulk of the OW tree switches over.  As each
#   component becomes PMake safe, that line will be removed from its 
#   environment.

#   Targets using DevGuide need to know where to find the binaries, and
#   those binaries need to be able to find the OpenWindows libraries, even
#   if OpenWindows isn't installed on the build machine.
#
GUIDEenvironment	= ${ENVIRONMENT}				\
			  GUIDEHOME=$${GUIDEHOME:-/opt/SUNWguide}	\
			  LD_LIBRARY_PATH=$${DESTDIR}/lib \
			  MAKE=${MAKEsequential}
			  
#   DeskSet also needs to have TARGET_SYS set.  Nobody seems to know
#   what `XFP' stands for, but it is the value for the I18N L4 stuff.
#
DESKSETenvironment	= ${GUIDEenvironment} TARGET_SYS=XFP \
			  MAKE=${MAKEsequential}

#   DocViewer needs to have TARG_SYS set.  They never picked up the
#   DeskSet XFP mods though...
DOCVIEWERenvironment	= ${GUIDEenvironment} CPS=${CPS} \
			  MAKE=${MAKEsequential}

#   AudioTool
AUDIOTOOLenvironment	= ${GUIDEenvironment} CPS=${CPS} \
			  GUIDEINC=$${GUDEHOME:-/opt/SUNWguide}/include \
			  MAKE=${MAKEsequential}



# ToolTalk binaries depend on the Classing Engine which is made as part of 
# DeskSet. Unfortunately, DeskSet in turn depends on ToolTalk.
# It would be best if Classing Engine could be made separately, but
# until then, make ToolTalk library first, then DeskSet libraries,
# then ToolTalk binaries, finally the DeskSet binaries

MAKE.install.target	= deskset_libs		.WAIT \
			  deskset_installtools	      \
			  install_docviewer 	      \
			  install_audiotool

#######  don't build tooltalk and docviewer #######
### should only be used on incremental builds ###
#MAKE.install.target    =  deskset_libs		.WAIT \
#			  deskset_installtools	      \
#			  install_audiotool

include ${RULESET.include}

tooltalk_lib::
	@cd ${TOOLTALKDIR}; \
	echo "----["`${PWDCMD}`"]----"; \
	${ENVIRONMENT} ${MAKEsequential} ttinstall;
	@cd ${TOOLTALKDIR}/mini_isam; \
	echo "----["`${PWDCMD}`"]----"; \
	${ENVIRONMENT} ${MAKEsequential} isam.h;
	@cd ${TOOLTALKDIR}/lib; \
	echo "----["`${PWDCMD}`"]----"; \
	${ENVIRONMENT} ${MAKEsequential} proto-O; \
	${ENVIRONMENT} ${MAKEsequential} install-O

tooltalk_bin::
	@cd ${TOOLTALKDIR}; \
	echo "----["`${PWDCMD}`"]----"; \
	${ENVIRONMENT} ${MAKEsequential} product


deskset_installtools::
	@echo "====== building deskset ======";
	@cd ${DESKSETDIR}; \
	echo "----["`${PWDCMD}`"]----"; \
	${DESKSETenvironment} ${MAKEsequential} tools install_tools;
	@echo "====== end of deskset ======"

deskset_libs::
	@echo "====== building deskset libs ======";
	@cd ${DESKSETDIR}; \
	echo "----["`${PWDCMD}`"]----"; \
	${DESKSETenvironment} ${MAKEsequential} all_install_libs;
	@echo "====== end of deskset libs ======"



install_docviewer::
	@cd ${DOCVIEWERDIR}; \
	echo "----["`${PWDCMD}`"]----"; \
	${DOCVIEWERenvironment} ${MAKEsequential} all install \
		TARG_SYS=SVR4 \
		INSTALLDIR="$$DESTDIR"

install_audiotool::
	@cd ${AUDIOTOOLDIR}; \
	echo "----["`${PWDCMD}`"]----"; \
	${AUDIOTOOLenvironment} ${MAKEsequential} all install \
		TARG_SYS=SVR4 \
		INSTALLDIR="$$DESTDIR"

