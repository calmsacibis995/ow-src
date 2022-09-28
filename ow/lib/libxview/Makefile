##
#
# Makefile for the XView Client Library
#
# @(#)Makefile	1.23 93/11/18
#
# Copyright (c) 1989 by Sun Microsystems, Inc.
#
##

SOURCEDIR		= ../..

include ${SOURCEDIR}/config/Master.cfg

CFLAGS += -D_TS_ERRNO

CURRDIR			= ${PWDCMD:sh}
PIC			= -PIC
LDOPTS			= -lintl ${LIBS.i18nx} -lc

LIBPARENT		= xview
TOOL			=
DELIVERABLE		= deliverable
STATIC			= static
SHARED			= shared
LIBMAJORVER		= 3
LIBMINORVER		= 1

INSTALL.directory	= ${DEST.lib}
MAKE.install.target	= strings

# Macros for string extraction

TEXTDOMAIN              = SUNW_WST_LIBXVIEW

SUBDIRS			= attr		\
			  base		\
			  canvas	\
			  color		\
			  cursor	\
			  defaults	\
			  dnd		\
			  file_chooser	\
			  filter	\
			  font		\
			  frame		\
			  fullscreen	\
			  help		\
			  icon		\
			  io_stream	\
			  menu		\
			  misc		\
			  notice	\
			  notify	\
			  openwin	\
			  panel		\
			  pw		\
			  rect		\
			  screen	\
			  scrollbar	\
			  sel		\
			  selection	\
			  server	\
			  server_image	\
			  string_utils	\
			  textsw	\
			  ttysw		\
			  win		\
			  window	\
			  wmgr		\
			  ${SUBDIR.sundae}

include ${RULESET.libparent}

# String extraction stuff

strings: misc/xv_version.h
	-@${AWK} '/XV_VERSION_NUMBER/ {print $$3}' < misc/xv_version.h >/tmp/xv_ver
	-@echo "${TEXTDOMAIN}_`cat /tmp/xv_ver`" > /tmp/xv_domain
	-@echo "Extracting Strings: `cat /tmp/xv_domain`"
	-@${RM} ${DEST.locale.msg_templates}/${TEXTDOMAIN}.po \
		${DEST.locale.msg_templates}/`cat /tmp/xv_domain`
	-@for i in ${SUBDIRS}; do \
		cd $$i; \
		${MAKE} $@; \
		cd ..; \
	done
	-@${SED} /\^domain/s/${TEXTDOMAIN}/\&_`cat /tmp/xv_ver`/ \
		< ${DEST.locale.msg_templates}/${TEXTDOMAIN}.po  \
		> ${DEST.locale.msg_templates}/`cat /tmp/xv_domain`.po
	-@${RM} /tmp/xv_ver /tmp/xv_domain \
		${DEST.locale.msg_templates}/${TEXTDOMAIN}.po
