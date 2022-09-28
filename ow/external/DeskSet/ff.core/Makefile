SOURCES.c	= ff.core.c
PROGRAM		= ff.core
PROJECTDIR	= ..

MANSECT   	= 1

include ${PROJECTDIR}/Makefile.master.new

CPPFLAGS	= 
LDFLAGS		= 
LDLIBS		= 

.KEEP_STATE:

default:	all

all:		$(PROGRAM)

clean:		master_clean

clobber: 	master_clobber

install_setuid:: $(ARCHDIR)/$(PROGRAM)
	-@echo "$(ARCHDIR)/$(PROGRAM): Replacing .comment section"; \
	mcs -d $(ARCHDIR)/$(PROGRAM); \
	mcs -a "$(PROGRAM) $(COMMENT_STRING)" $(ARCHDIR)/$(PROGRAM); \
	rm -f $(DESTDIR.bin)/$(PROGRAM); \
	$(INSTALL) -m 6555 -u root -f $(DESTDIR.bin) $(PROGRAM)

install: install_setuid
