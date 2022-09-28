##
#
# Makefile for the X11/NeWS executables
#
# @(#)Makefile	23.90 94/07/28 bin/Makefile SMI
#
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

#
# The Xsun server should be built first so failures in other parts of the build 
# don't break server builds.
#
SUBDIRS		= 			\
		scripts			\
		${OW_COMMANDS}		\
		${OLIT_DEMOS}		\
		${XVIEW_COMMANDS}	\
		${XVIEW_DEMOS}

MAKE.install.target = notice
notice:
	@echo "WARNING: The following commands are not being built:"
	@for file in ${NOTBUILT} ; do \
		echo "	" $$file; \
	done

OW_COMMANDS   = 24to8		\
		mae		\
		dsdm		\
		olwm		\
		olwmslave	\
		owplaces	\
		props		\
		reservecolors	\
		wsinfo		\
		capitalize	\
		insert_brackets	\
		locale_env	\
		remove_brackets	\
		shift_lines	\
		align_equals	\
		toolwait	


OLIT_DEMOS    = olmh		\
		olbiff		\
		olittable	\
		olitsampler	

XVIEW_COMMANDS= cmdtool			\
		convert_to_Xdefaults	\
		convert_to_xview	\
		vkbd			\
		xv_sel_svc

XVIEW_DEMOS   = realxfishdb		\
		spider


NOTBUILT      = \
		print_attr_names \
		xvbench \
		xvbenchmark

include ${RULESET.step}

