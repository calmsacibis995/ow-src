##
#
# Makefile for the OpenWindows locale customization files
#
# @(#)Makefile 1.9 94/03/11
#
##

#   +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ 
#  
#                       PROPRIETARY NOTICE
#  
#           This source code is unpublished proprietary 
#           information constituting, or derived under 
#           license from AT&T's UNIX(r) System V. 
#  
#                      Copyright Notice 
#  
#           Notice of copyright on this source code 
#           product does not indicate publication. 
#  
#           Copyright (C) 1986,1990  Sun Microsystems, Inc
#  
#                     All rights reserved. 
#  
#   +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ 
#  
#                   RESTRICTED RIGHTS LEGEND: 
#   Use, duplication, or disclosure by the Government is subject 
#   to restrictions as set forth in subparagraph (c)(1)(ii) of 
#   the Rights in Technical Data and Computer Software clause at 
#   DFARS 52.227-7013 and in similar clauses in the FAR and NASA 
#   FAR Supplement. 
#  
#   +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++ 


SOURCEDIR	= ..

include ${SOURCEDIR}/config/Master.cfg

SUBDIRS		= C \
		  templates \
		#

NOTBUILT	=

MAKE.install.target = # notice
notice:
	@echo "WARNING: The following locales are not being built:"
	@for file in ${NOTBUILT} ; do \
		echo "	" $$file; \
	done

include ${RULESET.step}

