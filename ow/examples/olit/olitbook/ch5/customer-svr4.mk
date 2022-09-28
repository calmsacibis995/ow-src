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

TM_COMMAND =    tracker1 \
                tracker2 \
                tracker3 \
                tracker4

COMMAND =       select \
                xclock \
                xbc \
                stopwatch

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

all: ${COMMAND} ${TM_COMMAND}

${COMMAND}: $$@.c $$@.o 
	$(CC) $(CFLAGS) $(CPPFLAGS) $@.o $(LDFLAGS) -o $@

${TM_COMMAND}: $$@.c $$@.o mousetracks.c mousetracks.o 
	$(CC) $(CFLAGS) $(CPPFLAGS) $@.o mousetracks.o $(LDFLAGS) -o $@

clean:
	rm -f *.o ${COMMAND} ${TM_COMMAND}

# end of Makefile.customer
#
