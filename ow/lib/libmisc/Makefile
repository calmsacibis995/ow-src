##
#
# Makefile for misc OpenWindows things
#
#	@(#)Makefile	23.32 95/07/28
##

#
# Copyright (c) 1990 by Sun Microsystems, Inc.
#

SOURCEDIR		= ../..

include ${SOURCEDIR}/config/Master.cfg

HEADERS			= ${FILES.lib.exe} \
			  ${FILES.share} \
			  ${FILES.lib}  \
			  ${FILES.xdm} \
			  ${FILES.xdm.exe}


MAKE.install.target	= inst_everything

SUBDIRS			= checkOW

FILES.lib.exe		= openwin-init openwin-sys
FILES.lib		= openwin-menu Xinitrc Xdefaults xman.help \
			  openwin-menu-programs openwin-menu-utilities \
			  openwin-menu-demo-fulldemo openwin-menu-demo-nodemo

FILES.xdm		= Xaccess Xresources Xservers Xsession \
				Xsetup_0 xdm-config 
FILES.xdm.exe		= GiveConsole StartOW TakeConsole Xsession xrdb.sh


include ${RULESET.step}

openwin-menu-utilities:	openwin-menu-utilities${TARGET_ARCH}
	    @cp -f openwin-menu-utilities${TARGET_ARCH} openwin-menu-utilities	
	
inst_everything: inst_xdm_exe inst_xdm inst_lib_exe inst_lib

inst_xdm_exe:	${FILES.xdm.exe}
	@for file in ${FILES.xdm.exe} ; do \
	  if cmp -s $$file ${DEST.lib}/xdm/$$file ; \
	    then : ; \
	    else \
		${ECHO} "$$file 	=> ${DEST.lib}"/xdm ; \
		${BINSTALL.bsd} $$file ${DEST.lib}/xdm ; \
		${BINSTALL.posix} -f ${DEST.lib}/xdm $$file ; \
	  fi \
	done

inst_xdm:	${FILES.xdm}
	@for file in ${FILES.xdm} ; do \
	  if cmp -s $$file ${DEST.lib}/xdm/$$file ; \
	    then : ; \
	    else \
		${ECHO} "$$file 	=> ${DEST.lib}"/xdm ; \
		${TINSTALL.bsd} $$file ${DEST.lib}/xdm ; \
		${TINSTALL.posix} -f ${DEST.lib}/xdm $$file ; \
	  fi \
	done

inst_lib_exe:	${FILES.lib.exe}
	@for file in ${FILES.lib.exe} ; do \
	  if cmp -s $$file ${DEST.lib}/$$file ; \
	    then : ; \
	    else \
		${ECHO} "$$file 	=> ${DEST.lib}" ; \
		${BINSTALL.bsd} $$file ${DEST.lib} ; \
		${BINSTALL.posix} -f ${DEST.lib} $$file ; \
	  fi \
	done


inst_lib:	${FILES.lib}
	@for file in ${FILES.lib} ; do \
	  if cmp -s $$file ${DEST.lib}/$$file ; \
	    then : ; \
	    else \
		${ECHO} "$$file 	=> ${DEST.lib}" ; \
		${TINSTALL.bsd} $$file ${DEST.lib} ; \
		${TINSTALL.posix} -f ${DEST.lib} $$file ; \
	  fi \
	done


inst_share:	${FILES.share}
	@for file in ${FILES.share} ; do \
	  if cmp -s $$file ${DEST.share}/$$file ; \
	    then : ; \
	    else \
		${ECHO} "$$file 	=> ${DEST.share}" ; \
		${TINSTALL.bsd} $$file ${DEST.share} ; \
		${TINSTALL.posix} -f ${DEST.share} $$file ; \
	  fi \
	done

