SOURCEDIR               = ../../..

include ${SOURCEDIR}/config/Master.cfg

OLFONTS.bdf = olcursor.bdf \
	olgl10.bdf olgl12.bdf olgl14.bdf olgl16.bdf \
	olgl19.bdf olgl20.bdf olgl24.bdf

FONTS.bdf = ${OLFONTS.bdf}
FONTS.source = ${OLFONTS.bdf}
FONTS.alias = fonts.alias.req

FONT.install.targets =	install.FONTS.bdf  \
			install.FONTS.alias .WAIT \
			install.FONTS.dir .WAIT \
			install.FONTS.source
MAKE.install.target = ${FONT.install.targets} .WAIT install_h
INSTALL.fonts.dir = ${DEST.font.misc}
INSTALL.fontsource.dir = ${DEST.source.fonts.misc}

CURSORHEADER.gen        = mkolcursorh.sh
CURSORHEADER            = olcursor.h
HEADERS.public		= ${CURSORHEADER}

${CURSORHEADER}: olcursor.bdf ${CURSORHEADER.gen}
	rm -f ${@}
	sh ./${CURSORHEADER.gen}

INSTALL.headers.dir	= ${DEST.include.X11}

include ${RULESET.font}
