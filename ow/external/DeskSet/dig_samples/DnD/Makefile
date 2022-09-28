SOURCES.pub	= Makefile.customer	\
		Resources		\
		dnd.h			\
		main.c			\
		owner.c			\
		requestor.c

PUBLICDIR	= dig_samples/DnD

PROJECTDIR	= ../..

.KEEP_STATE:

include ${PROJECTDIR}/Makefile.master.new

install:	install_public_source

.INIT:	$(SOURCES.pub)

