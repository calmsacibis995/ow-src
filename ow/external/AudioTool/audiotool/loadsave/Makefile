# Makefile for file chooser panel 
#
# @(#)Makefile 1.17 93/02/18
#


BASEDIR		= ../..
include ${BASEDIR}/Makefile.cfg


TARGETS		= objlib help

OBJLIB		= loadsave

SUNOS_GSRCS     = gfm.G
MARS_GSRCS	= gfm.G
SVR4_GSRCS	= fc_exten.G
GSRCS		= $($(TARG_SYS)_GSRCS)

GHELP		= audiotool
DOMAIN		= SUNW_DESKSET_AUDIOTOOL
GFLAGS		+= -d $(DOMAIN)

ICONS		= \
		  gfm_app.pr \
		  gfm_data.pr \
		  gfm_dotdot.pr \
		  gfm_folder.pr \
		  gfm_system.pr \
		  gfm_unknown.pr

SUNOS_GFMHDRS	= gfm.h gfm_ui.h $(ICONS)
MARS_GFMHDRS	= gfm.h gfm_ui.h $(ICONS)
SVR4_GFMHDRS	= fc_exten_ui.h audio_file_glyph.icon
GFMHDRS		= $($(TARG_SYS)_GFMHDRS)


SUNOS_GFMSRCS	= gfm.c gfm_load_dir.c gfm_ui.c
MARS_GFMSRCS	= gfm.c gfm_load_dir.c gfm_ui.c
SVR4_GFMSRCS	= fc_exten_ui.c
GFMSRCS		= $($(TARG_SYS)_GFMSRCS)

LIBSRCS		= \
		  loadsave.c \
		  loadsave_xv.c $(GFMSRCS)

LIBHDRS		= \
  		  loadsave_panel.h \
  		  loadsave_panel_impl.h $(GFMHDRS)

LIBOBJS		= ${LIBSRCS:%.c=%.o}

INCLUDE		= -I. -I.. -I../include ${AUDIOINC} ${GUIDEINC} ${X11INC}
DEFINES		= -DI18N_DOMAIN='"${DOMAIN}"'

include ${BASEDIR}/Makefile.rules
