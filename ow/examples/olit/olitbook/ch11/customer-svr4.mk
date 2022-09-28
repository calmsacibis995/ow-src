#
# @(#)customer-svr4.mk	1.6 93/01/26 SMI
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

CC = cc

COMMAND =       clipboard coloreditDnD controldata dragfile drawDnD \
		dropedit memo monitordata xtalk


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

all: ${COMMAND}

${COMMAND}: $$@.c $$@.o 
	$(CC) $(CFLAGS) $(CPPFLAGS) $@.o $(LDFLAGS) -o $@

clean:
	rm -f *.o ${COMMAND}

# end of Makefile.customer
#
