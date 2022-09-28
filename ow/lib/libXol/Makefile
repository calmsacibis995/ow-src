#	@(#)Makefile	302.42	97/03/26 lib/libXol SMI	/* OLIT	*/
#
# Makefile for the OLIT (libXol) client library
#

#
#        Copyright (C) 1986,1991  Sun Microsystems, Inc
#                    All rights reserved.
#          Notice of copyright on this source code 
#          product does not indicate publication. 
# 
# RESTRICTED RIGHTS LEGEND: Use, duplication, or disclosure by 
# the U.S. Government is subject to restrictions as set forth 
# in subparagraph (c)(1)(ii) of the Rights in Technical Data
# and Computer Software Clause at DFARS 252.227-7013 (Oct. 1988) 
# and FAR 52.227-19 (c) (June 1987).
#
#    Sun Microsystems, Inc., 2550 Garcia Avenue,
#    Mountain View, California 94043.
#


SOURCEDIR		= ../..

include ${SOURCEDIR}/config/Master.cfg

LIBRARY			= Xol
LIBRARY.tool		=
LIBRARY.deliverable	= ${LIBRARY}
TOOL			=
DELIVERABLE		= deliverable
STATIC			= static
SHARED			= shared
LIBMAJORVER		= 3
LIBMINORVER		= 1
OBJECTS			= ${SOOBJS}
OBJECTS.sa		= ${SAOBJS}
SOURCES			= ${OBJECTS:%.o=%.c}
FILES.lib		= 
DEFINES			= -Dsun -D_OL_BUILD -DINCLUDE_ALLOCA_H -DNO_ALLOCA
LDOPTS			= -L${XW_HOME}/lib -lolgx -L${XW_HOME}/lib -lXext
PIC			= ${BIGPIC}
HEADERS			= ${FILES.lib}
INCLUDE			= -I${SOURCEDIR}/include	\
			  -I${OPENWINHOME}/include	\
			  -I${XW_HOME}/include		\
			  -I/export/build/WORKSPACES_XW494_HAND_OFF/include
INSTALL.directory	= ${DEST.lib}
INSTALL.lintlib.dir	= ${DEST.lib.lint}
INSTALL.tool.directory	=
MAPFILE			= mapfile.xol

# i386
# Redefine CPPFLAGS.OS to exclude -Usun since libXol need -Dsun 
# to compile. This won't affect Sparc Solaris. CPPFLAGS.OS-sun4-svr4 and
# CPPFLAGS.OS-sparc are listed for user's reference.
#CPPFLAGS.OS-sparc     = -DSUN4SVR4 -DSVR4 -DSYSV -DDNETSVR4
#CPPFLAGS.OS-sun4-svr4 = -DSUN4SVR4 -DSVR4 -DSYSV -DDNETSVR4
CPPFLAGS.OS-i386-svr4    = ${CPPFLAGS.OS.COMMON-i386-svr4}
CPPFLAGS.OS	= ${CPPFLAGS.OS${TARGET_ARCH}}

MAKE.install.target	= strings

# Turn off the pure-text check - it appears to cause bogus errors against
# the referenced libraries (libolgx and libXext) even though they pass the
# test themselves...
#
PURETEXT		=

#	|	|	| 123456789012
# Sorted list of object files
SOOBJS 			= \
			  AbbrevMenu.o \
			  Accelerate.o \
			  Action.o \
			  Applic.o \
			  Arrow.o \
			  BaseWindow.o \
			  BulletinBo.o \
			  Button.o \
			  Caption.o \
			  Category.o \
			  ChangeBar.o \
			  CheckBox.o \
			  ControlAre.o \
			  Converters.o \
			  Copy.o \
			  CvtColor.o \
			  Datum.o \
			  DragNDrop.o \
			  DrawArea.o \
			  DropTarget.o \
			  DynResProc.o \
			  Dynamic.o \
			  Error.o \
			  EventObj.o \
			  Exclusives.o \
			  Extension.o \
			  FButton.o \
			  FCImpl.o \
			  FCheckBox.o \
			  FExclusive.o \
			  FNonexclus.o \
			  FileCh.o \
			  FileChImpl.o \
			  FileChInit.o \
			  FileChSh.o \
			  Flat.o \
			  FlatCvt.o \
			  FlatExpand.o \
			  FlatPublic.o \
			  FlatState.o \
			  FontCh.o \
			  FontChSh.o \
			  FontDB.o \
			  FooterPane.o \
			  Form.o \
			  Help.o \
			  List.o \
			  ListPane.o \
			  Mag.o \
			  Manager.o \
			  Margin.o \
			  MaskArgs.o \
			  Menu.o \
			  MenuButton.o \
			  Nonexclusi.o \
			  Notice.o \
			  NumericField.o \
			  OWFsetDB.o \
			  OblongButt.o \
			  OlCommon.o \
			  OlCursors.o \
			  OlDnDVCX.o \
			  OlGetFont.o \
			  OlGetRes.o \
			  OlIm.o \
			  OlStrMthds.o \
			  OlStrings.o \
			  OlgxAttr.o \
			  OlgxButton.o \
			  OlgxInit.o \
			  OlgxSize.o \
			  Oltextbuff.o \
			  OpenLook.o \
			  Packed.o \
			  ParseAcc.o \
			  Pixmap.o \
			  PopupWindo.o \
			  Primitive.o \
			  Pushpin.o \
			  RectButton.o \
			  Ring.o \
			  RootShell.o \
			  RubberTile.o \
			  SBTree.o \
			  Scrollbar.o \
			  ScrolledWi.o \
			  Slider.o \
			  StaticText.o \
			  Stub.o \
			  SuperCaret.o \
			  TextDisp.o \
			  TextEPos.o \
			  TextEdit.o \
			  TextField.o \
			  TextLPreEd.o \
			  TextLine.o \
			  TextLineBuff.o \
			  TextPreEdit.o \
			  TextUtil.o \
			  TextWrap.o \
			  Traversal.o \
			  Vendor.o \
			  array.o \
			  buffutil.o \
			  diags.o \
			  filenav.o \
			  linkedList.o \
			  memutil.o \
			  regexp.o \
			  strutil.o \
			  textbuff.o \
			  #

include ${RULESET.library}

${DIR.optimized}/OWFsetDB.o: OWFsetDB.c
	${CC} ${CPPFLAGS.debug} ${CPPFLAGS} ${CFLAGS} ${CFLAGS.debug} -c -o ${DIR.optimized}/OWFsetDB.o OWFsetDB.c
 
${DIR.shared}/OWFsetDB.o: OWFsetDB.c
	${CC} ${CPPFLAGS.debug} ${CPPFLAGS} ${CFLAGS} ${CFLAGS.debug} ${PIC} -DDYNAMICLIB -c -o ${DIR.shared}/OWFsetDB.o OWFsetDB.c
 

CFLAGS += -D_TS_ERRNO

# Macros for string extraction
TEXTDOMAIN = SUNW_WST_LIBXOL
 
strings: ${SOURCES} ${HEADERS}
	-@echo "Extracting Strings:"
	@${RM} ${DEST.locale.msg_templates}/${TEXTDOMAIN}.po
	-@for i in ${SOURCES} ; do \
		echo "$$i"; \
		${CC} -DXGETTEXT ${DEFINES} ${INCLUDE} -E -C $$i | \
			${XGETTEXT} - ; \
	done
