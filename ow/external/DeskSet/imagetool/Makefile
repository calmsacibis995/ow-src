#
# @(#)Makefile 1.82 97/02/13 SMI
#
# ImageTool Makefile
# XIL 1.0 FCS required to run the imagetool in this nightly build.
#

SOURCES.c	= ce.c display.c dither.c dragdrop.c fileui.c gif.c gifcompr.c \
		  gifencod.c image.c imageinfoui.c imagetool.c imagetool_dps.c \
		  jfif.c menus.c misc.c open.c pageviewui.c paletteui.c photocd.c ppm.c \
		  print.c print_prev.c printprevui.c printui.c props.c \
		  propsui.c ps.c ps_optionsui.c rast.c save.c select.c state.c \
		  tiff.c tt.c ui_imagetool.c xv.c
SOURCES.h	= display.h image.h imagetool.h ppm.h props.h state.h tags.h \
		  ui_imagetool.h 
SOURCES.icon	= dupedoc_drag.icon dupedoc_drop.icon im.icon imask.icon \
		  rotate_r.icon rotate_l.icon pan.icon select.icon hflip.icon \
		  vflip.icon zoom.icon pan.cursor degrees.icon next_page.icon \
		  prev_page.icon
SOURCES.man	= imagetool.1
SOURCES.types	= imagetool.types
SOURCES.info	= imagetool.info
SOURCES.cps	= imagetool_ps.cps
SOURCES.psw	= imagetool.psw
PROGRAM		= imagetool 
PROJECTDIR	= ..

MANSECT   	= 1

XILHOME:sh	= echo ${XILHOME:-/opt/SUNWits/Graphics-sw/xil}

# When the build environment is set up correctly, we can remove the
#  comments from the following lines:
#
CPPFLAGS       += -DPHOTOCD -I../libtiff -I$(XILHOME)/include -I../libdstt
LDFLAGS        += -L../libtiff -L$(XILHOME)/lib -L../libdstt
LDLIBS          = -Bstatic -lcps -Bdynamic -ldstt -ltt -ltiff -lce \
		  -lxil -ldga -ldps -lm 

SVR4_CPS= PATH="$${PATH}:${DESTDIR}/bin:$${XW_HOME}/bin" ; export PATH; cps -o macros
XFP_CPS= PATH="$${PATH}:${DESTDIR}/bin:$${XW_HOME}/bin" ; export PATH; cps -o macros
SUNOS_CPS= PATH="$${PATH}:${DESTDIR}/bin" ; export PATH; cps -o macros
JLE_CPS= $(SUNDAEHOME)/bin/cps -o macros
CPS = $($(TARG_SYS)_CPS)

#
# Couple of extra def's so that Guide gets included.  GUIDEHOME has
# to be defined for this Makefile to work anyway.

.KEEP_STATE:

default:	all

include ${PROJECTDIR}/Makefile.master.new

CPPFLAGS	+= -I$(WIN_HOME)/include/X11 -I$(XW_HOME)/include/X11

#
# For our nightly builds, and for testing, you can set the environment
# variable ADD_RUN_PATH to be another library to be *pre-pended* to
# -R/usr/openwin/lib.  This variable *must* end in a ":" for the
# concatenation to work properly.  Kludge.
#
# Add the xil library to the LD_RUN_PATH.
#   Have to include the other paths from Makefile.master.new. 
#
LDFLAGS         += -R$(ADD_RUN_PATH)/usr/openwin/lib:/opt/SUNWits/Graphics-sw/xil/lib

all:	$(PROGRAM)
	@${MAKE} -f Makefile.pagecounter all

clean:	master_clean

#
# For I18N string extraction (see Makefile.master)
#
TEXTDOMAIN   = SUNW_DESKSET_IMAGETOOL_LABEL SUNW_DESKSET_IMAGETOOL_MSG SUNW_DESKSET_IMAGETOOL_ERR

install: install_normal_bins install_info install_man strings_multi_domain \
	 install_tt_types
	 @${MAKE} -f Makefile.pagecounter install
 
.INIT:	$(SOURCES.icon)
