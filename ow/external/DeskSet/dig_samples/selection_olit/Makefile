SOURCES.pub	= Makefile.customer	\
		olit_sel.c

PUBLICDIR	= dig_samples/selection_olit

PROJECTDIR	= ../..

.KEEP_STATE:

include ${PROJECTDIR}/Makefile.master.new

CPPFLAGS	+= -I$(XW_HOME)/include

install:	install_public_source

.INIT:	$(SOURCES.pub)


