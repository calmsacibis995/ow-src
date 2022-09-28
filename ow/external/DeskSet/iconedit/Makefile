#
# @(#)Makefile 1.36 90/11/21 SMI
#

SOURCES.G	= 
SOURCES.c	=	base.c \
			file.c \
			text.c \
			base_ui.c \
			file_ui.c \
			text_ui.c \
			select.c \
			line.c \
			box.c \
			ellipse.c \
			circle.c \
			draw.c \
			print.c \
			dragdrop.c \
			gcm.c \
			tooltalk.c \
			resources.c

SOURCES.h	=	base_gui.h \
			base.h \
			file_gui.h \
			text_gui.h \
			gcm.h \
			c_varieties.h \
			gdd.h \
			xpm.h \
			tooltalk.h \
			resources.h
SOURCES.l	=
SOURCES.sh	=
SOURCES.y	=
SOURCES.pkg	=
SOURCES.icon	=	box.icon16 \
			circle.icon16 \
			clock_rot.icon \
			counter_rot.icon \
			down_arrow.icon \
			ellipse.icon16 \
			eraser.icon \
			horzflip_arrow.icon \
			iconedit.icon \
			iconedit.mask \
			left_arrow.icon \
			line.icon16 \
			lucida.icon16 \
			point.icon16 \
			right_arrow.icon \
			select_box4.icon16 \
			swatch_17.icon16 \
			swatch_20.icon16 \
			swatch_25.icon16 \
			swatch_50.icon16 \
			swatch_75.icon16 \
			swatch_80.icon16 \
			swatch_83.icon16 \
			swatch_black.icon16 \
			swatch_root.icon16 \
			swatch_white.icon16 \
			up_arrow.icon \
			vertflip_arrow.icon \
			dupedoc_drag.icon \
			dupedoc_drop.icon 

SOURCES.map	=
SOURCES.man	=	iconedit.1
SOURCES.info	=	iconedit.info
PROGRAM		=	iconedit
PROJECTDIR	=	..
 
MANSECT		=	1

CPPFLAGS = -I../libdstt
LDFLAGS  = -L../libdstt
LDLIBS	 = -ldstt -ltt -lm

.KEEP_STATE:

default:	all

include ${PROJECTDIR}/Makefile.master.new

all:		$(PROGRAM)

clean:		master_clean

#
# For I18N string extraction (see Makefile.master)
#
TEXTDOMAIN	=	SUNW_DESKSET_ICONEDIT
 
install:	install_normal_bins install_info install_man install_icons \
		strings_single_domain
 
tar:
	echo You will want to have an empty directory that you are taring to!
	tar fc - . | rsh ${DESTMACH} \
		"cd ${DESTDIR}${SRCDIR};tar xpBf -"
 
.INIT:	$(SOURCES.icon) $(SOURCES.h)
