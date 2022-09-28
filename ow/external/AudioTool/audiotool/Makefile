#
# @(#)Makefile	1.74	97/04/03 SMI
#

# Makefile for audiotool

BASEDIR		= ..
include ${BASEDIR}/Makefile.cfg


TARGETS		= subdirs cplusplusprog tt_types help man
TARGETS.install	= 


CPLUSPLUSPROG	= audiotool

SUBDIRS		= include \
		  icon \
		  config \
		  format \
		  loadsave \
		  segment

GSRCS		= atool.G
GHELP		= audiotool
DOMAIN		= SUNW_DESKSET_AUDIOTOOL
GFLAGS		+= -d $(DOMAIN)

TT_TYPES	= audiotool.types

PROGCSRCS	= \
		  atool.c \
		  atool_xv.c \
		  atool_ui.c \
		  audiotool.c \
		  atool_ctl.c \
		  atool_file.c \
		  atool_edit.c \
		  atool_sel_xv.c \
		  at_sel_tcp.c \
		  at_sel_repl.c \
		  at_sel_cvt.c \
		  atool_tt.c \
		  atool_dstt.c \
		  at_sel_atoms.c \
		  holdbutton.c \
		  atool_debug.c \
		  xv_system.c

SUNOS_CCSRCS	= devctl_c.cc
MARS_CCSRCS	= devctl_c.cc
SVR4_CCSRCS	= 
CCSRCS		= $($(TARG_SYS)_CCSRCS)

PROGCCSRCS	= Undolist.cc \
		  PlayRecord.cc \
		  undolist_c.cc \
		  ${CCSRCS}

PROGSRCS	= ${PROGCSRCS} ${PROGCCSRCS}

PROGHDRS	= Undolist.h \
		  PlayRecord.h \
		  undolist_c.h \
		  atool_sel_impl.h \
		  atool_sel.h \
		  version.h \
		  holdbutton.h \
		  atool_debug.h \
		  atool_ttce.h \
		  atool_panel.h \
		  atool_panel_impl.h \
		  atool_ui.h

PROGOBJS	= ${PROGCSRCS:%.c=%.o} ${PROGCCSRCS:%.cc=%.o}

ATOOL_MODULES	= segment config loadsave format
ATOOL_LIBS	= ${ATOOL_MODULES:%=%/${DIR.obj}/lib%.o}


# Used for pre-493 builds
DEVCTLDIR	= ${BASEDIR}/audiocontrol
DEVCTLINC	= -I${DEVCTLDIR}
DEVCTLMODULES	= playctl recordctl status
DEVCTLLIBS	= ${DEVCTLMODULES:%=${DEVCTLDIR}/%/${DIR.obj}/lib%.o} \
		  ${DEVCTLDIR}/${DIR.obj}/audioctl.o

SUNOS_INCS	= ${DEVCTLINC}
MARS_INCS	= ${DEVCTLINC}
SVR4_INCS	= 
INCS		= $($(TARG_SYS)_INCS)

SUNOS_DLIBS	= ${DEVCTLLIBS}
MARS_DLIBS	= ${DEVCTLLIBS}
SVR4_DLIBS	= 
DLIBS		= $($(TARG_SYS)_DLIBS)

DEPENDLIBS	= ${ATOOL_LIBS} ${AUDIOLIBS} ${LISTMGRLIBS} \
		  ${METERLIBS} ${DSTTLIBS} ${DLIBS}

LDLIBS		= ${GUIDELIBS} ${X11LIBS} ${DESKSETLIBS} \
		  ${ANSILIBS} ${SOCKETLIBS} ${SUNMATHLIB} ${INTLIBS} \
		  -norunpath -lm

INCLUDE		= -I. -I./include ${AUDIOINC} ${METERINC} ${LISTMGRINC} \
		  ${DSTTINC} ${INCS} ${X11INC} ${GUIDEINC}
DEFINES		= -DI18N_DOMAIN='"${DOMAIN}"' ${AUDIODEFS} -DOW_I18N


# I18N message file and help file stuff

# list of help files to concatenate to create application help file
HELP_FILES = atool.info config/config.info format/format.info \
		loadsave/fc_exten.info

# name of on-line handbook file, if any
HELP_HANDBOOK = handbooks/audio.handbook

# installable help file
INSTALL_HELP = audiotool.info

# list of dir's to get .po files from. these get concatenated
# when installed.
MESSAGE_FILE_DIRS = . config format loadsave segment

MAN		= audiotool.1

include ${BASEDIR}/Makefile.rules
