#
# @(#)Makefile	1.2 97/05/30
#

ARCH=`uname -p`

all: SUNWoladd

SUNWoladd: pkginfo prototype
	for dir in data binary ; \
	do \
	    cd $$dir ; \
	    make     ; \
	    cd ..    ; \
	done
	pkgmk -o -a ${ARCH} -f prototype -b `pwd` -d `pwd`

pkginfo:
	#sccs get pkginfo.tmpl
	sed -e 's/\"//g' pkginfo.tmpl | sed -e '/^ARCH=/d' > pkginfo

prototype:
	#sccs get prototype
