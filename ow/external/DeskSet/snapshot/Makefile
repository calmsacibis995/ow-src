#
# @(#)Makefile	3.6 11/22/96 Sun Microsystems, Inc.
#
#       Snapshot Makefile
#

SOURCES.c	= ck_zmalloc.c dither.c dragdrop.c gif.c multivis.c prlib.c \
		  rect.c snapshot.c text.c xview.c 
SOURCES.h	= multivis.h patchlevel.h snapshot.h xdefs.h
SOURCES.icon	= dupedoc_drag.icon dupedoc_drop.icon snapshot.icon \
		  snapshot.mask.icon
SOURCES.man	= snapshot.1
SOURCES.info	= snapshot.info
PROGRAM		= snapshot
PROJECTDIR	= ..

MANSECT   	= 1

#
# Couple of extra def's so that Guide gets included.  GUIDEHOME has
# to be defined for this Makefile to work anyway.

CPPFLAGS	+= -I../libdstt
LDFLAGS		+= -L../libdstt
LDLIBS		+= -ldstt -ltt -lm

.KEEP_STATE:

default:	all

include ${PROJECTDIR}/Makefile.master.new

all:	$(PROGRAM)
	$(MAKE) -f Makefile.ras2ps all
	$(MAKE) -f Makefile.rash all

clean:	master_clean

#
# For I18N string extraction (see Makefile.master)
#
TEXTDOMAIN   = SUNW_DESKSET_SNAPSHOT_LABEL SUNW_DESKSET_SNAPSHOT_MSG SUNW_DESKSET_SNAPSHOT_ERR

install: install_normal_bins install_info install_man strings_multi_domain
	 $(MAKE) -f Makefile.ras2ps install
	 $(MAKE) -f Makefile.rash install
 
.INIT:	$(SOURCES.icon)
