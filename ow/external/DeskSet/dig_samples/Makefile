SAMPLES	= 	DnD 	\
		ce1 	\
		ce2 	\
		dnd_olit	\
		dnd_xview1	\
		dnd_xview2	\
		selection_olit 	\
		selection_xview \
		tooltalk_simple \
		Tooltalk

SOURCES.pub	= README

PROJECTDIR	= ..
PUBLICDIR	= dig_samples

.KEEP_STATE:

include ${PROJECTDIR}/Makefile.master.new

all:		samples

install:	install_public_source install_samples

samples:	$(SAMPLES)
		-@for i in ${SAMPLES} ; do	\
			cd $$i;			\
			${MAKE} -f Makefile.customer CPPFLAGS=-I$(XW_HOME)/include LDFLAGS=-L$(XW_HOME)/lib ;	\
			cd ..;			\
		done

install_samples:	$(SAMPLES)
		-@for i in ${SAMPLES} ; do 	\
			cd $$i;			\
			${MAKE} install;	\
			cd ..;			\
		done


.INIT:	$(SOURCES.pub)
