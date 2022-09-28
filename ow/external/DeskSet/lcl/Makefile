# @(#)Makefile 1.3 - 97/01/30; Copyright (c) 1997 by Sun Microsystems, Inc.

include ../Makefile.master.new

  TARGETS = liblcl.a


  PUBLIC_HDR = lcl.h
  PRIVATE_HDR = config.h \
		lcl_cs_info.h \
		lcl_internal.h \
		lcl_str_stdio.h \
		lcl_db.h \
		lcl_locale_db.h \
		lcl_types.h \
		lcl_args.h \
		lcl_db_def.h \
		lcl_str.h \
		lcl_mime_encode.h
   HDRS = $(PUBLIC_HDR) $(PRIVATE_HDR)

     MIMESRCS = lcl_str_stdio.c \
		lcl_str_codes.c \
		lcl_parse_header.c \
		lcl_strstr.c \
		lcl_strcasecmp.c \
		lcl_uue.c \
		lcl_endecode.c \
		lcl_mimehead.c

    ICONVSRCS = lcl_iconv_tw_to_iso.c \
		lcl_iconv_iso_to_tw.c

      CNSSRCS = cns11643_to_iso2022-CN.c \
		iso2022-CN_to_cns11643.c
         SRCS = $(MIMESRCS) \
		$(ICONVSRCS) \
		$(CNSSRCS) \
		lcl.c \
		lcl_db.c \
		lcl_str.c \
		lcl_args.c \
		lcl_cs_info.c \
		lcl_segment.c \
		lcl_attr.c \
		lcl_convert.c \
		lcl_ascii.c \
		lcl_create.c \
		lcl_buf.c \
		lcl_mime_encode.c \
		lcl_qencode.c \
		lcl_bencode.c \
		lcl_query_type.c \
		lcl_iconv.c

      DEFSRCS = ja.def \
		big5.def	\
		en_US_UTF8.def	\
		iso8859_1.def	\
		iso8859_2.def	\
		iso8859_4.def	\
		iso8859_5.def	\
		iso8859_7.def	\
		iso8859_9.def	\
		ja_PCK.def	\
		ko.def	\
		ko_UTF8.def	\
		ru.def	\
		zh.def	\
		zh_TW.def

     MIMEOBJS = $(ARCHDIR)/lcl_str_stdio.o \
                $(ARCHDIR)/lcl_str_codes.o \
                $(ARCHDIR)/lcl_parse_header.o \
                $(ARCHDIR)/lcl_strstr.o \
                $(ARCHDIR)/lcl_strcasecmp.o \
                $(ARCHDIR)/lcl_uue.o \
                $(ARCHDIR)/lcl_endecode.o \
                $(ARCHDIR)/lcl_mimehead.o
 
    ICONVOBJS = $(ARCHDIR)/lcl_iconv_tw_to_iso.o \
                $(ARCHDIR)/lcl_iconv_iso_to_tw.o
 
         OBJS = $(MIMEOBJS) \
                $(ICONVOBJS) \
                $(ARCHDIR)/lcl.o \
                $(ARCHDIR)/lcl_db.o \
                $(ARCHDIR)/lcl_str.o \
                $(ARCHDIR)/lcl_args.o \
                $(ARCHDIR)/lcl_cs_info.o \
                $(ARCHDIR)/lcl_segment.o \
                $(ARCHDIR)/lcl_attr.o \
                $(ARCHDIR)/lcl_convert.o \
                $(ARCHDIR)/lcl_ascii.o \
                $(ARCHDIR)/lcl_create.o \
                $(ARCHDIR)/lcl_buf.o \
                $(ARCHDIR)/lcl_mime_encode.o \
                $(ARCHDIR)/lcl_qencode.o \
                $(ARCHDIR)/lcl_bencode.o \
                $(ARCHDIR)/lcl_query_type.o \
                $(ARCHDIR)/lcl_iconv.o



##########################################################################
# COMPILIER FLAGS
##########################################################################
 
         CCOPTIONS = -Xc -mt -DSYSV -DSVR4
         DEFINES = -DNOT_X_ENV -DPARSE -DLCL_LIB
         CFLAGS = $(CDEBUGFLAGS) $(CCOPTIONS) $(INCLUDES) $(DEFINES)



default:	all

all: setup $(HDRS) $(SRCS) $(DEFSRCS) $(TARGETS)
 

$(TARGETS): $(OBJS)
	ar cr $(ARCHDIR)/$@ $(OBJS)

$(ARCHDIR)/%.o: %.c
#	$(CC) -c -O $(CFLAGS) -o $@ $<
	$(CC) -c $(CFLAGS) -o $@ $<


setup:
	-@if [ -d $(ARCHDIR) ]; then set +x; \
	else (set -x; /usr/bin/mkdir -p $(ARCHDIR)); fi
 

clean:
	$(RM) $(OBJS) $(TARGETS) core

