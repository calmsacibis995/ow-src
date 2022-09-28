##
#
# Makefile for Core (server) includes
#
# @(#)Makefile	23.67 94/12/09
#
##

#
# Copyright (c) 1990 by Sun Microsystems, Inc.
#

SOURCEDIR		= ../..

include ${SOURCEDIR}/config/Master.cfg

MANPAGE		= ${OW_MANPAGES} \
		  ${XVIEW_MANPAGES} \
		  ${I18N_MANPAGES}

XVIEW_MANPAGES	= cmdtool.1		\
		  convert_to_xview.1	\
		  convert_to_Xdefaults.1	\
		  shelltool.1
		  

OW_MANPAGES = 		\
	24to8.1		\
	dsdm.1		\
	locale_env.1	\
	olwm.1 		\
	olwmslave.1	\
	openwin.1	\
	owplaces.1	\
	props.1		\
	textedit_filters.1 \
	toolwait.1	\
	vkbd.1		\
	xv_get_sel.1	\
	#

MAKE.install.target	= install_man
INSTALL.manpage.dir	= ${DEST.man1}

include ${RULESET.include}
