#
# @(#)Makefile	1.8 95/06/28 Copyright (c) 1992 by Sun Microsystems, Inc.
#

PROJECTDIR	=	../..
 
#
# The targets in this make file are:
#
#	compile: makes library and all executables, makes lint library
#
#	lint: runs lint on the library sources and all executables
#
#	install: installs deliverables in $(DESTDIR)
#
#	clean: removes *.o *.a core executables
#
#

SOURCES.c 	= isdatconv.c	 \
		  isfd.c isfab.c \
		  isopen.c 	 \
		  iserror.c      \
		  isparsname.c   \
		  isvars.c       \
		  isperm.c       \
		  isalloc.c      \
		  isbytearray.c  \
		  isclose.c 	 \
		  isbuild.c      \
		  iskeyvalid.c	 \
		  isamerror.c    \
		  isfcb.c 	 \
		  ispageio.c	 \
		  isfcbwatchfd.c \
		  ismngfcb.c 	 \
		  isamopen.c 	 \
		  isamwrite.c 	 \
		  iswrite.c	 \
		  isfixrec.c	 \
	  	  isdiskbufs2.c	 \
		  isfileio.c	 \
		  isdlink.c 	 \
		  isread.c	 \
		  isrewrec.c	 \
		  isdelrec.c	 \
		  isamdelrec.c   \
		  iswrrec.c	 \
		  isrewcurr.c	 \
		  isdelcurr.c    \
		  isamdelcurr.c	 \
		  isstart.c	 \
		  isaddindex.c   \
		  isamaddindex.c \
		  iskeyconv.c	 \
		  isfcbindex.c	 \
		  isindexconv.c	 \
		  issort.c	 \
		  iskeycalc.c	 \
		  iskeycmp.c	 \
		  iskeyaux.c	 \
		  isindfreel.c   \
		  isindexinfo.c	 \
		  isbtree.c	 \
		  isbsearch.c 	 \
		  isminmax.c	 \
		  isaddprimary.c \
		  isdel1key.c	 \
		  isadd1key.c	 \
		  ischange1key.c \
		  isdelete.c	 \
		  isamdelete.c   \
		  isdiskbufs2.c	 \
		  isamhooks.c	 \
		  iserase.c	 \
	 	  isrename.c	 \
		  isfname.c	 \
		  isstrec.c	 \
		  isldrec.c	 \
		  isvarrec.c	 \
		  issync.c	 \
		  isbtree2.c	 \
		  isbtree3.c	 \
		  isrepair.c	 \
		  issignals.c	 \
		  iscntl.c	 \
	          isapplmr.c	 \
		  isapplmw.c	 \
		  isversion.c	 \
		  isprimtorec.c

SOURCES.h	= isam.h	\
		  isam_impl.h


UISSOURCES.c	  = uisschema.c isschema.c
UISOBJS  = $(UISSOURCES.c:%c=$(ARCHDIR)/%o)

#
# make .c from .cc (for isschema.cc to isschema.c)
#
MV			= /bin/mv
.cc.c:
		$(MV) $< $@


PROGRAM		=

LIBRARY	        = libisam

LIBVERSION	= 

default:	all
include		$(PROJECTDIR)/Makefile.master

all:		$(LIBRARY.a) uisschema

# override the default of making $(ARCHDIR)/isschema.o from isschema.cc
$(ARCHDIR)/isschema.o:	isschemalex.c isschema.c
		$(CC) -c -O $(CCFLAGS) $(CPPFLAGS) isschema.c -o $@

# override standard LDLIBS for uisschema -- doesnt need -ldv nor -libjmor
LDLIBS		= $($(TARG_SYS)_LDLIBS) -ldeskset -lxview -lolgx -lX11
$(ARCHDIR)/uisschema-$(MACH):	$(UISOBJS)
		$(CC) $(CCFLAGS) $(UISOBJS) $(LDFLAGS) -L. -lisam $(LDLIBS) -o $@

uisschema:	$(ARCHDIR)/uisschema-$(MACH)
		$(RM) $@
		cp $(ARCHDIR)/uisschema-$(MACH) $@
		chmod 755 $@

debug:		$(LIBRARY.g)
purify:		$(LIBRARY.pure)
install:
clean:		master_clean
.INIT:		$(SOURCES.h) $(SOURCES.q)
