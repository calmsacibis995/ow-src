#
# @(#)Makefile 3.31 - 97/02/13 SMI; Copyright (c) 1992 by Sun Microsystems, Inc.
#

SOURCES.c = \
	main.c \
	tool.c \
	destruct.c \
	tool_support.c \
	create_panels.c \
	selection.c \
	selreply.c \
	cmds.c \
	delete_log.c \
	msort.c \
	generate_menus.c \
	reply.c \
	editor.c \
	attach_panel.c \
	attach_canvas.c \
	attach_list.c \
	attach_ce.c \
	mail.c \
	subr.c \
	props.c \
	header_selection.c \
	view.c \
	undelete.c \
	vacation_ifc.c \
	template.c \
	folder_menu.c \
	graphics.c \
	instrument.c \
	filelist.c \
	buttons.c \
	tooltalk.c \
	tooltalk_send.c \
	buffer.c \
	mailtool_dstt.c \
	mailcmds.c lex.c cmdtab.c \
	list.c

SOURCES.icon = \
	compose.icon \
	empty_letter.icon \
	nomail.icon \
	nomail.mask.icon \
	emptymail.icon \
	dead.icon \
	mail.icon \
	reply.icon \
	copyletter.cursor \
	copyletters.cursor \
	moveletter.cursor \
	moveletters.cursor \
	document.cursor \
	compose.mask.icon \
	dead.mask.icon \
	empty_letter.mask.icon \
	mail.mask.icon \
	emptymail.mask.icon \
	reply.mask.icon \
	document.icon \
	application.icon \
	folder.glyph \
	mbox.glyph

SOURCES.h = \
	attach.h \
	buffer.h \
	buttons.h \
	cmds.h \
	create_panels.h \
	debug.h \
	def.h \
	delete_log.h \
	destruct.h \
	editor.h \
	glob.h \
	glob_mail.h \
	graphics.h \
	header.h \
	instrument.h \
	list.h \
	local.h \
	mail.h \
	mail_dstt.h \
	mail_version.h \
	main.h \
	mle.h \
	select.h \
	tool.h \
	tool_support.h \
	tooltalk.h 


SOURCES.info	= mailtool.info
SOURCES.man	= mailtool.1
SOURCES.map	= mapfile
SOURCES.types	= mailtool.types
SOURCES.misc	= makeversion.sh

PROGRAM		= mailtool
MANSECT		= 1

#
# For I18N string extraction (see Makefile.master)
#
TEXTDOMAIN   = SUNW_DESKSET_MAILTOOL

PROJECTDIR=..

#
# Default target
#
default:	all

#
# Add our tool specific flags
#

LDFLAGS += -L$(MAILLIBDIR) -L../libdstt/ -L../libdeskset/ -L../lcl/$(ARCHDIR)
LDLIBS  += -lmail -lm -lce -ldstt -ltt -llcl
all	:=	LINK.c  += -M $(SOURCES.map.arch)

include $(PROJECTDIR)/Makefile.master.new

all: $(ARCHDIR)/$(PROGRAM)

#
# Need name of directory the .o's are in so we can load with the
# appropriate libmail.a
#
$(ARCHDIR)/% := SUBDIR_NAME = $(ARCHDIR)
$(ARCHDIR.g)/% := SUBDIR_NAME = $(ARCHDIR.g)
$(ARCHDIR.xf)/% := SUBDIR_NAME = $(ARCHDIR.xf)
$(ARCHDIR.xa)/% := SUBDIR_NAME = $(ARCHDIR.xa)

MAILLIBDIR= $(PROJECTDIR)/maillib/$(SUBDIR_NAME)

#
# Add more of our tool specific flags
#
$(ARCHDIR.g)/$(PROGRAM.g) := CPPFLAGS += -DDEBUG

#
# Uncomment these lines to access the local aliases in compose window
#CPPFLAGS += -DCOMPOSE_ADDR -I../libdstt

#
# Uncomment this line and the above 2 lines to get tooltalk locking
#
CPPFLAGS += -DTOOLTALK_LOCK -I../libdstt -I../lcl

#
# Uncomment this line to get context search in Find.
#
#CPPFLAGS += -DCONTEXT_SEARCH -I../libdstt

#
# a way to add extra stuff to the compile line, specifically the src browser
#
CPPFLAGS += $(SBROWSER) 

#
# 9 Jan 90, katin <-- blame me for what follows...
#
# actually make the mailtool program.  We do a horrible, but terribly
# useful hack after compiling the program -- we get the compile machine
# and date, and then try and assign this info to a string within
# mailtool.  We use mcs to assign the string.
#
# the only place this is used is in the output of the "mailtool -v" command.
#
POST_LINK = sh makeversion.sh $@

install: install_normal_bins install_info install_man install_icons \
		install_tt_types strings_single_domain install_maillib

install_maillib:
	cd ../maillib; $(MAKE) install

clean: master_clean

#
# this is all needed to build the mail library
#
$(ARCHDIR)/$(PROGRAM): ../maillib/$(ARCHDIR)/libmail.a
$(ARCHDIR.g)/$(PROGRAM.g): ../maillib/$(ARCHDIR.g)/libmail.a
$(ARCHDIR.xf)/$(PROGRAM.xf): ../maillib/$(ARCHDIR.xf)/libmail.a
$(ARCHDIR.xa)/$(PROGRAM.xa): ../maillib/$(ARCHDIR.xa)/libmail.a

../maillib/$(ARCHDIR)/libmail.a: FRC
	cd ../maillib ; $(MAKE) $(ARCHDIR)/libmail.a
../maillib/$(ARCHDIR.g)/libmail.a: FRC
	cd ../maillib ; $(MAKE) $(ARCHDIR.g)/libmail.a
../maillib/$(ARCHDIR.xf)/libmail.a: FRC
	cd ../maillib ; $(MAKE) $(ARCHDIR.xf)/libmail.a
../maillib/$(ARCHDIR.xa)/libmail.a: FRC
	cd ../maillib ; $(MAKE) $(ARCHDIR.xa)/libmail.a

FRC:
.INIT: $(SOURCES.h) $(SOURCES.icon) MAILHDRS

MAILHDRS:
	cd ../maillib; $(MAKE) .INIT

.PARALLEL:
