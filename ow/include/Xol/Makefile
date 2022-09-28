#	@(#)Makefile	302.21	92/12/10 include/Xol SMI	/* OLIT	*/
#
# Makefile for OLIT includes
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


HEADERS			= ${HEADERS.noship} \
			  ${HEADERS.ship.obsolete} \
			  ${HEADERS.ship} \
			  bitmaps \
			  geometry

HEADERS.public		= ${HEADERS.ship.obsolete} \
			  ${HEADERS.ship}

#	|	|	| 123456789012
# All file lists are in sorted order
# The files below support ANSI C only
HEADERS.noship		= \
			  Converters.h \
			  Datum.h \
			  Error.h \
			  FCImplI.h \
			  FileChImpl.h \
			  FileChInit.h \
			  FontDB.h \
			  FontDBI.h \
			  OWFsetDB.h \
			  OWFsetDBI.h \
			  OlCursorsP.h \
			  OlDnDVCXI.h \
			  OlDnDVCXP.h \
			  OlI18nP.h \
			  OlIm.h \
			  OlImI.h \
			  OlStrMthdsI.h \
			  OlStringsTable.h \
			  OpenLookI.h \
			  ParseAccI.h \
			  Ring.h \
			  RingP.h \
			  RootShellP.h \
			  SBTree.h \
			  SBTreeP.h \
			  SuperCaretP.h \
			  TextDisp.h \
			  TextEPos.h \
			  TextLBuffI.h \
			  TextLPreEdI.h \
			  TextLineI.h \
			  TextUtil.h \
			  TextWrap.h \
			  Util.h \
			  diags.h \
			  filenav.h \
			  linkedList.h \
			  memutil.h \
			  strutil.h \
			  #


# Headers we continue to ship for source compatibility
HEADERS.ship.obsolete	= \
			  AbbrevStac.h \
			  ButtonStac.h \
			  #

# Headers shipped with the product
HEADERS.ship		= \
			  AbbrevMenP.h \
			  AbbrevMenu.h \
			  AcceleratP.h \
			  Arrow.h \
			  ArrowP.h \
			  BaseWindoP.h \
			  BaseWindow.h \
			  BulletinBP.h \
			  BulletinBo.h \
			  Button.h \
			  ButtonP.h \
			  Caption.h \
			  CaptionP.h \
			  Category.h \
			  CategoryP.h \
			  ChangeBar.h \
			  CheckBox.h \
			  CheckBoxP.h \
			  ControlArP.h \
			  ControlAre.h \
			  DrawArea.h \
			  DrawAreaP.h \
			  DropTarget.h \
			  DropTargetP.h \
			  Dynamic.h \
			  DynamicP.h \
			  EventObj.h \
			  EventObjP.h \
			  ExclusiveP.h \
			  Exclusives.h \
			  FButton.h \
			  FButtonP.h \
			  FCheckBox.h \
			  FCheckBoxP.h \
			  FExclusivP.h \
			  FExclusive.h \
			  FNonexcluP.h \
			  FNonexclus.h \
			  FileCh.h \
			  FileChP.h \
			  FileChSh.h \
			  FileChShP.h \
			  Flat.h \
			  FlatP.h \
			  Font.h \
			  FontCh.h \
			  FontChP.h \
			  FontChSh.h \
			  FontChShP.h \
			  FooterPanP.h \
			  FooterPane.h \
			  Form.h \
			  FormP.h \
			  Gauge.h \
			  GaugeP.h \
			  Help.h \
			  HelpP.h \
			  ListPane.h \
			  ListPaneP.h \
			  Mag.h \
			  MagP.h \
			  Manager.h \
			  ManagerP.h \
			  Margin.h \
			  Menu.h \
			  MenuButtoP.h \
			  MenuButton.h \
			  MenuP.h \
			  NonexclusP.h \
			  Nonexclusi.h \
			  Notice.h \
			  NoticeP.h \
			  NumericFiP.h \
			  NumericFie.h \
			  OblongButP.h \
			  OblongButt.h \
			  OlCEnv.h \
			  OlClients.h \
			  OlClientsP.h \
			  OlCursors.h \
			  OlDnDVCX.h \
			  OlImP.h \
			  OlReasons.h \
			  OlStrings.h \
			  OlgxP.h \
			  Oltextbuff.h \
			  OpenLook.h \
			  OpenLookP.h \
			  Pixmap.h \
			  PixmapP.h \
			  PopupWindP.h \
			  PopupWindo.h \
			  Primitive.h \
			  PrimitiveP.h \
			  Pushpin.h \
			  PushpinP.h \
			  RectButtoP.h \
			  RectButton.h \
			  RootShell.h \
			  RubberTilP.h \
			  RubberTile.h \
			  Scrollbar.h \
			  ScrollbarP.h \
			  ScrolledWP.h \
			  ScrolledWi.h \
			  ScrollingL.h \
			  ScrollingP.h \
			  Slider.h \
			  SliderP.h \
			  StaticTexP.h \
			  StaticText.h \
			  StringList \
			  Stub.h \
			  StubP.h \
			  SuperCaret.h \
			  TextEdit.h \
			  TextEditP.h \
			  TextField.h \
			  TextFieldP.h \
			  TextLine.h \
			  TextLineP.h \
			  VendorI.h \
			  array.h \
			  buffutil.h \
			  regexp.h \
			  textbuff.h \
			  txtbufCA.h \
			  txtbufCNA.h \
			  #

MAKE.clean.target	= clean_bitmaps \
			  clean_geometry

MAKE.clobber.target	= clean_bitmaps \
			  clean_geometry

MAKE.install.target	= install_h

INSTALL.headers.dir	= ${DEST.include.Xol}

include ${RULESET.include}

bitmaps:
	${RM} bitmaps
	${LN} ../Xolbitmaps bitmaps

geometry:
	${RM} geometry
	${LN} ../Xolgeometry geometry

clean_bitmaps:
	${RM} bitmaps

clean_geometry:
	${RM} geometry

OlStrings.h OlStringsTable.h ../../lib/libXol/OlStrings.c: strings.awk StringList
	${RM} OlStrings.h OlStringsTable.h ../../lib/libXol/OlStrings.c; \
	${AWK} -f strings.awk StringList

clean_derived:
	${RM} OlStrings.h OlStringsTable.h ../../lib/libXol/OlStrings.c
