#
# @(#)Makefile 3.17 - 97/01/31 SMI; Copyright (c) 1992 by Sun Microsystems, Inc.
#


SOURCES.c = \
	ck_free.c \
	ck_malloc.c \
	ck_strdup.c \
	ck_zmalloc.c \
	contype.c \
	msg.c \
	folder.c \
	headers.c \
	vars.c \
	submission.c \
	attach.c \
	tools.c \
	global.c \
	hash.c \
	misc.c \
	lockf.c \
	encode.c \
	base64.c \
	quoted.c \
	buffer.c \
	charset.c \
	enconv.c

SOURCES.h = assert.h \
	ck_strings.h \
	debug.h \
	global.h \
	hash.h \
	obj.h \
	submission.h \
	folder.h \
	msg.h \
	attach.h \
	buffer.h \
	headers.h \
	misc.h \
	bool.h \
	charset.h \
	enconv.h

LIBRARY = libmail
PROJECTDIR = ..

CEDIR= $(PROJECTDIR)/ce
MYLIBS += -L$(CEDIR) -lce -ldl -ltt -L$(OPENWINHOME)/lib -lX11 -L../lcl/$(ARCHDIR) -llcl

default: all

include ../Makefile.master.new

lcllib:
	cd ../lcl; $(MAKE) all

all: lcllib $(LIBRARY.a)

#
# For I18N string extraction
#
TEXTDOMAIN   = SUNW_DESKSET_MAILLIB

# set the BROWSER environment variable to "-sb" to compile with the source
# browser.  The V2 flag is for reading V2 mailtool enclosure.
CPPFLAGS += -DV2 -DSENDMAIL -DLINE_COUNT ${DBX} ${MAIL_DEBUG}

#
#uncomment this to use RFC-MIME format
#
CPPFLAGS += -DRFC_MIME

#
# uncomment this to use Tool-Talk locking
#
CPPFLAGS += -DTOOLTALK_LOCK -I../lcl

$(ARCHDIR.g)/% := CPPFLAGS += -DDEBUG

$(ARCHDIR)/test: $(ARCHDIR)/test.o $(ARCHDIR)/libmail.a
	$(LINK.c) -o $@ $(ARCHDIR)/test.o $(ARCHDIR)/libmail.a $(MYLIBS) $(LIBS)

clean : master_clean

install: strings_multi_domain

# boiler plate
.KEEP_STATE:
.INIT:	lcllib $(SOURCES.h)
