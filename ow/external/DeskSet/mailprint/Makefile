#
# @(#)Makefile 3.6 - 97/02/13 SMI; Copyright (c) 1992 by Sun Microsystems, Inc.
#

SOURCES.c = \
	mailprint.c \
	readmsg.c

SOURCES.man	= mailprint.1
PROGRAM		= mailprint
MANSECT		= 1
PROJECTDIR	= ..

#
# Default target
#
default:	all

include $(PROJECTDIR)/Makefile.master.new

all: $(PROGRAM)

#
# Need name of directory the .o's are in so we can load with the
# appropriate libmail.a
#
$(ARCHDIR)/% := SUBDIR_NAME = $(ARCHDIR)
$(ARCHDIR.g)/% := SUBDIR_NAME = $(ARCHDIR.g)
$(ARCHDIR.xf)/% := SUBDIR_NAME = $(ARCHDIR.xf)

MAILLIBDIR= $(PROJECTDIR)/maillib/$(SUBDIR_NAME)

#
# Add our tool specific flags
#
LDFLAGS += -L$(MAILLIBDIR) -L../libdstt -L../lcl/$(ARCHDIR)
LDLIBS  += -lmail -ldstt -ltt -lce -llcl
$(ARCHDIR.g)/$(PROGRAM.g) := CPPFLAGS += -DDEBUG -I../libdstt

.KEEP_STATE:

install: install_normal_bins install_man

clean: master_clean

#
# this is all needed to build the mail library
#
$(ARCHDIR)/$(PROGRAM): ../maillib/$(ARCHDIR)/libmail.a
$(ARCHDIR.g)/$(PROGRAM.g): ../maillib/$(ARCHDIR.g)/libmail.a
$(ARCHDIR.xf)/$(PROGRAM.xf): ../maillib/$(ARCHDIR.xf)/libmail.a

../maillib/$(ARCHDIR)/libmail.a: FRC
	cd ../maillib ; $(MAKE) $(ARCHDIR)/libmail.a
../maillib/$(ARCHDIR.g)/libmail.a: FRC
	cd ../maillib ; $(MAKE) $(ARCHDIR.g)/libmail.a
../maillib/$(ARCHDIR.xf)/libmail.a: FRC
	cd ../maillib ; $(MAKE) $(ARCHDIR.xf)/libmail.a

FRC:
.INIT: MAILHDRS

MAILHDRS:
	cd ../maillib; $(MAKE) .INIT
