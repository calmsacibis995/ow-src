#	@(#)customer-svr4.mk	1.8    93/02/08 bin SMI    /* OLIT	*/
#
# Solaris 2.1 User Makefile for the OpenWindows OLIT Sampler sources
#

#
# 	Copyright (C) 1989, 1993  Sun Microsystems, Inc
# 			All rights reserved.
#		Notice of copyright on this source code
# 		product does not indicate publication.
# 
# RESTRICTED RIGHTS LEGEND: Use, duplication, or disclosure by
# the U.S. Government is subject to restrictions as set forth
# in subparagraph (c)(1)(ii) of the Rights in Technical Data
# and Computer Software Clause at DFARS 252.227-7013 (Oct. 1988)
# and FAR 52.227-19 (c) (June 1987).
#
# This file is a product of Sun Microsystems, Inc. and is
# provided for unrestricted use provided that this legend is
# included on all media and as a part of the software
# program in whole or part.  Users may copy or modify this
# file without charge, but are not authorized to license or
# distribute it to anyone else except as part of a product or
# program developed by the user.
# 
# THIS FILE IS PROVIDED AS IS WITH NO WARRANTIES OF ANY KIND
# INCLUDING THE WARRANTIES OF DESIGN, MERCHANTIBILITY AND
# FITNESS FOR A PARTICULAR PURPOSE, OR ARISING FROM A COURSE
# OF DEALING, USAGE OR TRADE PRACTICE.
# 
# This file is provided with no support and without any
# obligation on the part of Sun Microsystems, Inc. to assist
# in its use, correction, modification or enhancement.
# 
# SUN MICROSYSTEMS, INC. SHALL HAVE NO LIABILITY WITH RESPECT
# TO THE INFRINGEMENT OF COPYRIGHTS, TRADE SECRETS OR ANY
# PATENTS BY THIS FILE OR ANY PART THEREOF.
# 
# In no event will Sun Microsystems, Inc. be liable for any
# lost revenue or profits or other special, indirect and
# consequential damages, even if Sun has been advised of the
# possibility of such damages.
# 
#	Sun Microsystems, Inc., 2550 Garcia Avenue,
#	Mountain View, California 94043.
#


#
# Source package description
#
COMMAND		= olitsampler
SOURCES.c	= olitsampler.c


#
# Environment set up
#

# If OPENWINHOME is not set, default to /opt/openwin for the location of 
# OpenWindows Version 3
OPENWINHOME$(OPENWINHOME) = /usr/openwin

DEFINES		= 
		# -DDEBUG	if debugging

CFLAGS	       = -X c -v -O
		# -g		if debugging

LIBRARIES	= -lXol -lXt -lX11 -lintl -lgen


#
# Derived macros
#
SOURCES		= $(SOURCES.c)
OBJECTS		= $(SOURCES:.c=.o)
INCLUDES	= -I$(OPENWINHOME)/include
CPPFLAGS       += $(DEFINES) $(INCLUDES)
LIBDIRS		= -L$(OPENWINHOME)/lib
LDFLAGS		= $(LIBDIRS) $(LIBRARIES)


#
# Rule sets
#
.KEEP_STATE:

all: $(COMMAND)

$(COMMAND): $(OBJECTS)
	$(CC) $(CFLAGS) $(CPPFLAGS) $(OBJECTS) $(LDFLAGS) -o $@

clean:
	$(RM) -r core $(OBJECTS) $(COMMAND) .make.state .sb
