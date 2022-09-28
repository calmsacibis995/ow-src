#
# @(#)Makefile	1.18	97/04/03 SMI
#

# Makefile for audiocontrol

BASEDIR		= ..
include ${BASEDIR}/Makefile.cfg

TARGETS		= subdirs cplusplusprog tt_types help man

CPLUSPLUSPROG	= audiocontrol

SUBDIRS		= playctl recordctl status

LIBHDRS		= audiocontrol.h audioctl.h

PROGSRCS	= audiocontrol.cc audioctl.cc audioctl_dstt.cc
		  
PROGOBJS	= ${PROGSRCS:%.cc=%.o}

GHELP		= audiocontrol
DOMAIN		= SUNW_DESKSET_AUDIOCONTROL
GFLAGS		+= -d $(DOMAIN)

TT_TYPES	= audiocontrol.types

ACTL_MODULES	= playctl recordctl status
ACTL_LIBS	= ${ACTL_MODULES:%=%/${DIR.obj}/lib%.o}

DEPENDLIBS	= ${ACTL_LIBS} ${AUDIOLIBS} ${METERLIBS} ${DSTTLIBS}

LDLIBS		= ${GUIDELIBS} ${X11LIBS} ${DESKSETLIBS} \
		  ${SUNMATHLIB} ${INTLIBS} -lm -norunpath

INCLUDE		= -I. ${AUDIOINC} ${DSTTINC} ${X11INC} ${GUIDEINC}
DEFINES		= -DI18N_DOMAIN='"${DOMAIN}"' ${AUDIODEFS}

HELP_FILES	= playctl/playctl.info recordctl/recordctl.info \
		  status/status.info

INSTALL_HELP	= audiocontrol.info

MESSAGE_FILE_DIRS = . playctl recordctl status

MAN		= audiocontrol.1

include ${BASEDIR}/Makefile.rules
