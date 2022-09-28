# @(#)Makefile 3.55 94/03/03 Copyr 1991 Sun Microsystems, Inc.
#
#       cm Makefile
#

SOURCES.c = \
	alarm.c \
	alist.c \
	blist.c \
	browser.c \
	calendarA.c \
	common.c \
	cm_tt.c \
	dayglance.c \
	datefield.c \
	editor.c \
	dnd.c \
	find.c \
	fv_cal.c \
	gdd.c \
	goto.c \
	graphics.c \
	mail.c \
	monthglance.c \
	postup.c \
	props.c \
	ps_graphics.c \
	repeat.c \
	select.c \
	tempbr.c \
	todo.c \
	weekglance.c \
	yearglance.c \
	cm_i18n.c \
	format.c \
	namesvc.c \
	defnamesvc.c

SOURCES.h = \
	appt.h \
	cm_tt.h \
	util.h \
	timeops.h \
	agent.h \
	select.h \
	graphics.h \
	editor.h \
	mail.h \
	alarm.h \
	props.h \
	dayglance.h \
	datefield.h \
	weekglance.h \
	yearglance.h \
	blist.h \
	browser.h \
	token.h \
	alist.h \
	todo.h \
	calendar.h \
	find.h \
	goto.h \
	postup.h \
	sched.h \
	tempbr.h \
	table.h \
	abbcache.h \
	gettext.h \
	cm_i18n.h \
	format.h \
	defnamesvc.h \
	namesvc.h \
	common.h \
	ps_graphics.h \
	misc.h \
	monthglance.h \
	repeat.h

SOURCES.icon = \
	postup.icon \
	busy.icon \
	compose.icon \
	compose.mask.icon \
	cmgr.icon \
	cmgrmask.icon \
	accept.cursor \
	drag.cursor

SOURCES.libcm = \
	abbcache.c \
	agent.c \
	agent_xdr.c \
	appt.c \
	convert2-4.c \
	convert4-2.c \
	convert3-4.c \
	convert4-3.c \
	timeops.c \
	table.c \
	getdate.c \
	misc.c \
	rtable2_clnt.c \
	rtable3_clnt.c \
	rtable4_clnt.c \
	rtable2_xdr.c \
	rtable3_xdr.c \
	rtable4_xdr.c \
	util.c

PROGRAM 	= cm
LIBRARY.cm 	= libcm.a

SOURCES.info	= cm.info
SOURCES.tpl	= calendar.tpl
SOURCES.map 	= mapfile
SOURCES.man	= cm.1 rpc.cmsd.1 cm_insert.1 cm_delete.1 cm_lookup.1
MANSECT		= 1
BINS		= rpc.cmsd cm_lookup cm_insert cm_delete libcm.a ae
PROJECTDIR 	= ..
CPPFLAGS	+= -I$(PROJECTDIR)/libdstt -I$(GUIDEHOME)/include
LDFLAGS		+= -L$(PROJECTDIR)/libdstt -L$(GUIDEHOME)/lib 
LDLIBS		+= -ldstt -ltt -lguidexv -L. -lcm

.KEEP_STATE:

$(PROGRAM)	:= LINK.c += -M $(SOURCES.map.arch)

default: 	all

include $(PROJECTDIR)/Makefile.master.new

TEXTDOMAIN   = SUNW_DESKSET_CM_ERR SUNW_DESKSET_CM_LABEL SUNW_DESKSET_CM_MSG libguidexv

# These are all of the targets for CM and ae.
# You can make libcm, libcm.a, libcm-g, libcm.a-g, all, all-g,
# cm_delete, cm_delete-g, cm_insert, cm_insert-g, 
# cm_lookup,  cm_lookup-g, cm, cm-g, ae and ae-g

ae:
	${MAKE} -f Makefile.ae all

ae-g:
	${MAKE} -f Makefile.ae ae-g

libcm:
	${MAKE} -f Makefile.libcm all

libcm-g:
	${MAKE} -f Makefile.libcm libcm.a-g

libcm.a-g:
	${MAKE} -f Makefile.libcm libcm.a-g


$(ARCHDIR)/$(PROGRAM): $(ARCHDIR)/$(LIBRARY.cm)

$(ARCHDIR.g)/$(PROGRAM.g): $(ARCHDIR.g)/$(LIBRARY.cm)

$(ARCHDIR.xf)/$(PROGRAM.xf): $(ARCHDIR.xf)/$(LIBRARY.cm)

$(ARCHDIR)/$(LIBRARY.cm): 
	${MAKE} -f Makefile.libcm all

$(ARCHDIR.g)/$(LIBRARY.cm): 
	${MAKE} -f Makefile.libcm libcm.a-g

rpc.cmsd : 
	${MAKE} -f Makefile.rpc.cmsd all

rpc.cmsd-g : 
	${MAKE} -f Makefile.rpc.cmsd rpc.cmsd-g

cm_lookup : 
	${MAKE} -f Makefile.libcm all
	${MAKE} -f Makefile.cm_lookup all

cm_lookup-g : 
	${MAKE} -f Makefile.libcm libcm.a-g
	${MAKE} -f Makefile.cm_lookup cm_lookup-g

cm_insert : 
	${MAKE} -f Makefile.libcm all
	${MAKE} -f Makefile.cm_insert all

cm_insert-g : 
	${MAKE} -f Makefile.libcm libcm.a-g
	${MAKE} -f Makefile.cm_insert cm_insert-g

cm_delete : 
	${MAKE} -f Makefile.libcm all
	${MAKE} -f Makefile.cm_delete all

cm_delete-g : 
	${MAKE} -f Makefile.libcm libcm.a-g
	${MAKE} -f Makefile.cm_delete cm_delete-g

all:
	${MAKE} -f Makefile.libcm all
	${MAKE} $(PROGRAM) 
	${MAKE} -f Makefile.rpc.cmsd all
	${MAKE} -f Makefile.cm_delete all
	${MAKE} -f Makefile.cm_insert all
	${MAKE} -f Makefile.cm_lookup all
	${MAKE} -f Makefile.ae all

all-g:
	${MAKE} -f Makefile.libcm libcm.a-g
	${MAKE} $(PROGRAM)-g
	${MAKE} -f Makefile.rpc.cmsd rpc.cmsd-g
	${MAKE} -f Makefile.cm_delete cm_delete-g
	${MAKE} -f Makefile.cm_insert cm_insert-g
	${MAKE} -f Makefile.cm_lookup cm_lookup-g
	${MAKE} -f Makefile.ae ae-g
	
clean:	master_clean
	rm -f getdate.c ${BINS} *.pg  *_xdr* *_clnt* \
	agent.h rtable2.h rtable3.h rtable4.h appt.h rtable2_tbl.i \
	rtable3_tbl.i rtable4_tbl.i parser.h parser.c \
	*_ui.c *_ui.h

clobber: master_clobber
	rm -f getdate.c ${BINS} *.pg  *_xdr* *_clnt* \
	agent.h rtable2.h rtable3.h rtable4.h appt.h rtable2_tbl.i \
	rtable3_tbl.i rtable4_tbl.i parser.h parser.c \
	*_ui.c *_ui.h

install: install_normal_bins install_tpl install_man install_info strings_multi_domain strings_append_domain
	${MAKE} -f Makefile.rpc.cmsd install
	${MAKE} -f Makefile.cm_delete install
	${MAKE} -f Makefile.cm_insert install
	${MAKE} -f Makefile.cm_lookup install
	${MAKE} -f Makefile.ae install

agent.h : agent.x
	rpcgen -h agent.x -o agent.h

agent_xdr.c : agent.x rtable4.x
	rpcgen -c agent.x -o agent_xdr.tmp
	cat agent_xdr.tmp | awk '{ print }  /rpc\.h/ { printf ("#include \"rtable4.h\"\n"); }' > agent_xdr.c
	rm agent_xdr.tmp

agent_xdr.o : agent_xdr.c rtable4.x
	$(COMPILE.c) agent_xdr.c

appt.h : rtable4.x
	rpcgen -h rtable4.x -o appt.h

strings_append_domain: $(SOURCES.libcm)
	-@echo "Extracting strings:"; \
	for i in $(SOURCES.libcm) ; do \
		echo "$$i"; \
		$(COMPILE.c) -E -C -DXGETTEXT $$i | $(XGETTEXT) $(XGETTEXT_OPTS);\
	done

