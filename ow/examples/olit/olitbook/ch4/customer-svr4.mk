#
# @(#)customer-svr4.mk	1.2 93/01/26 SMI
# User Makefile for the Olit Programmer's Guide's Examples 
# Requires SunOS 5.x (or later) Make(1).
#

#
# Environment set up
#

# If OPENWINHOME is not set, default to /usr/openwin for the location of 
# OpenWindows
OPENWINHOME:sh	= echo ${OPENWINHOME:-/usr/openwin}

#
# Source package description
#

COMMAND =       abbrev \
		bulletinboard \
		caption \
		checkbox \
		control \
		flfontchooser \
		fontchooser \
		footerpanel \
		form \
		gauge \
		help \
		help2 \
		help3 \
		menu \
		menubutton \
		notice \
		oblong \
		popupwindow \
		rectbutton \
		rubbertile \
		scrollbar \
		scrolledwindow \
		scrollinglist \
		slider \
		statictext \
		textedit \
		textfield \
		twoshells

LINKS   =	caption2 \
		control2 control3 \
		flfontchooser2 \
		fontchooser2 \
		form2 form3 form4 \
		gauge2 \
		menubutton2 \
		oblong2 \
		rectbutton2 \
		rubbertile2 \
		scrollbar2 \
		slider2 \
		statictext2 \
		textedit2 \
		textedit3


DEFINES		= -Dsun
		# -DDEBUG	if debugging output is desired

LIBRARIES	= -lXol -lXt -lX11

#
# Derived macros
#

INCLUDES	= -I${OPENWINHOME}/include
CPPFLAGS       += ${DEFINES} ${INCLUDES}

CFLAGS	       += -g
		# -g	for debugging

LIBDIRS		= -L$(OPENWINHOME)/lib
LDFLAGS		= $(LIBDIRS) $(LIBRARIES)


#
# Rule sets
#

all: ${COMMAND} ${LINKS}

${COMMAND}: $$@.c $$@.o 
	$(CC) $(CFLAGS) $(CPPFLAGS) $@.o $(LDFLAGS) -o $@

${LINKS}:
	ln -s `echo $@ | sed 's/[1-9]//'` $@

clean:
	rm -f *.o ${COMMAND} ${LINKS} core

# end of Makefile.customer
#
