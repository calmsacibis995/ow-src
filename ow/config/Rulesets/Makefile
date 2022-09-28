#

# Copyright

#
#	@(#)Makefile	1.1 90/07/23
#

PWDCMD  = pwd
SUBDIRS	= SunPro Simple

tool installtool deliverable all:
	@for i in ${SUBDIRS}; do \
	(cd $$i; echo "----["`${PWDCMD}`"]----"; \
	    ${MAKE} all \
	) done


