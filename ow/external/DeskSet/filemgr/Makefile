#
# @(#)Makefile 1.38 94/01/07 SMI
#
# Requires GUIDEHOME to be set.
#

SOURCES.G	= 

SOURCES.c	= filemgr_ui.c	 	\
			assoc.c		\
			display.c	\
			dragdrop.c	\
			file.c		\
			functions.c 	\
			isubs.c		\
			main.c 		\
			text.c		\
			various.c 	\
			xview.c

SOURCES.h	= defs.h 		\
			filemgr_ui.h	\
			fm.h 		\
			xdefs.h		\
			patchlevel.h 	\
			tree.h 		\
			../ce/ce.h 	\
			../ce/ce_err.h

SOURCES.l	=
SOURCES.sh	=
SOURCES.y	=
SOURCES.pkg	=
SOURCES.map	= mapfile

IDIR		= images
SOURCES.icon	= $(IDIR)/FileMgr.icon  $(IDIR)/FileMgr_mask.icon      \
	   $(IDIR)/app_list.icon        $(IDIR)/app_listI.icon         \
	   $(IDIR)/application.icon     $(IDIR)/applicationI.icon      \
	   $(IDIR)/broken_list.icon     $(IDIR)/brokenlink.icon        \
	   $(IDIR)/cd.icon              $(IDIR)/cd.mask.icon           \
	   $(IDIR)/col_pi.icon                                         \
	   $(IDIR)/diskette.icon        $(IDIR)/diskette.mask.icon     \
	   $(IDIR)/doc_list.icon        $(IDIR)/doc_listI.icon         \
	   $(IDIR)/document.icon        $(IDIR)/documentI.icon         \
	   $(IDIR)/dos.icon             $(IDIR)/dos.mask.icon          \
	   $(IDIR)/fol_acc_copy.icon    $(IDIR)/fol_acc_move.icon      \
	   $(IDIR)/fol_drag_copy.icon   $(IDIR)/fol_drag_move.icon     \
	   $(IDIR)/fols_acc_copy.icon   $(IDIR)/fols_acc_move.icon     \
	   $(IDIR)/fols_drag_copy.icon  $(IDIR)/fols_drag_move.icon    \
	   $(IDIR)/hotspot.cursor				       \
	   $(IDIR)/lock.icon					       \
	   $(IDIR)/open_folder.icon     $(IDIR)/open_folder.mask.icon  \
	   $(IDIR)/panning.cursor                                      \
	   $(IDIR)/readonly.icon	$(IDIR)/row_pi.icon            \
	   $(IDIR)/small_cd.icon	$(IDIR)/small_cd.mask.icon     \
	   $(IDIR)/small_dos.icon       $(IDIR)/small_dos.mask.icon    \
	   $(IDIR)/small_floppy.icon	$(IDIR)/small_floppy.mask.icon \
	   $(IDIR)/small_waste.icon	$(IDIR)/small_waste.mask.icon  \
	   $(IDIR)/sys_list.icon        $(IDIR)/sys_listI.icon         \
	   $(IDIR)/system.icon          $(IDIR)/systemI.icon           \
	   $(IDIR)/waste-empty.icon     $(IDIR)/waste-empty_mask.icon  \
	   $(IDIR)/waste-full.icon      $(IDIR)/waste-full_mask.icon	\
	   $(IDIR)/folder_list.icon	$(IDIR)/tree.icon		\
	   $(IDIR)/tree_mask.icon	$(IDIR)/folder_listI.icon

SOURCES.man	= filemgr.1	\
		format_floppy.1

PROGRAM		= filemgr
PROJECTDIR	= ..

SOURCES.info 	= filemgr.info
MANSECT  	= 1

# GUIDEHOME has to be defined.

CPPFLAGS	= -I$(GUIDEHOME)/include
LDFLAGS		= -L$(GUIDEHOME)/lib -L$(PROJECTDIR)/libdeskset

.KEEP_STATE:

default:	all

all	:=	LINK.c	+= -M $(SOURCES.map.arch)

include ${PROJECTDIR}/Makefile.master.new

TEXTDOMAIN   = SUNW_DESKSET_FM_ERR SUNW_DESKSET_FM_LABEL SUNW_DESKSET_FM_MSG

all:	$(PROGRAM)

# Here I set LDLIBS based on the system I'm on -- adding -lelf for 5.x.

SUNOS_LDLIBS	+= -lguidexv -lce
SVR4_LDLIBS	+= -lguidexv -lce -lelf -lvolmgt

#----- conditional macros to setup the variaout targets ------------------


install: install_normal_bins install_info install_man install_icons strings_multi_domain

clean:	master_clean

clobber:	master_clobber

g		:= CFLAGS = -DCE
disk 		:= CFLAGS = -DCE -DCD_SUPPORT -DDISKETTE_SUPPORT

.INIT: $(SOURCES.icon)
