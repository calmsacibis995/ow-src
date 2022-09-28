#
# @(#)Makefile   3.20 - 93/11/10
#

YFLAGS		=	-dv

SOURCES.c	= ce_main.c ce_update_api.c ce_allocate.c master_db.c \
		  ce_defns_xdr.c ce_db_rw.c ce_search.c ce_utils.c

SOURCES.h	= ce_defns.h ce.h ce_err.h fns.h ce_int.h
SOURCES.public.h= ce.h ce_err.h

SOURCES.misc	= cetables.ascii

LIBRARY		= libce
LIBVERSION	= .0
PROJECTDIR	= ..

CMDS		= ce_db_build ce_db_merge
MGRS		= tns_mgr.so fns_mgr.so
TABLES		= cetables

.KEEP_STATE:

default:	all

include ${PROJECTDIR}/Makefile.master.new

# Libraries needed by libce.so
SVR4_LIB_LDFLAGS      += -lc -lnsl -ldl
SVR4_CPPFLAGS += -D_TS_ERRNO

# This is where the cetables and namespace managers go
TABLES_DESTDIR	= ${DESTDIR}/lib/cetables
DESTDIR.public.h= ${DESTDIR.include}/desktop

all:		$(LIBRARY.a) $(LIBRARY.so) .WAIT lexlib .WAIT $(MGRS) $(CMDS) .WAIT cetables

clean:		master_clean
		rm -f cetables* ce_yacc.c ce_lex.c y.* $(CMDS) $(MGRS) libce.* libcelex.a

#
# For I18N string extraction (see DeskSet's Makefile.master)
#
TEXTDOMAIN   	  = SUNW_DESKSET_CE

cetables.tmp: 	cetables.ascii
		$(CC) -D$(TARG_SYS) -Xs -C -undef -E cetables.ascii | egrep -v "^#" | egrep -v "^//" >cetables.tmp

cetables:	cetables.tmp
		rm -f $@ $@.lck
		LD_LIBRARY_PATH=".:$(WIN_HOME)/lib:$(XW_HOME)/lib:$(LD_LIBRARY_PATH)"; export LD_LIBRARY_PATH; \
		./ce_db_build network -from_ascii cetables.tmp -db_file $@

lexlib:
		${MAKE} -f Makefile.lexer

ce_db_merge:
		${MAKE} -f Makefile.ce_db_merge

ce_db_build:
		${MAKE} -f Makefile.ce_db_build

tns_mgr.so:
		${MAKE} -f Makefile.tns_mgr.so

fns_mgr.so:
		${MAKE} -f Makefile.fns_mgr.so

install:	install_libs install_cetables strings_single_domain \
			install_public_h
		-@if [ ! -d $(TABLES_DESTDIR) ]; then \
			mkdir -p $(TABLES_DESTDIR); \
		fi; \
		for i in $(MGRS); do \
			rm -f $(TABLES_DESTDIR)/$$i; \
			$(INSTALL) -m 755 -f $(TABLES_DESTDIR) $$i; \
		done
		-@for i in $(CMDS);  do \
			${MAKE} -f Makefile.$$i install;  \
		done

		
install_cetables:	cetables
		rm -f $(TABLES_DESTDIR)/cetables
		-@if [ ! -d $(TABLES_DESTDIR) ]; then \
			mkdir -p $(TABLES_DESTDIR); \
		fi; \
		if [ -f cetables ]; then \
			$(INSTALL) -m 644 -f $(TABLES_DESTDIR) cetables; \
		fi
		
.INIT:		$(SOURCES.h)




