#ifndef	_NAVIGATOR_H
#define	_NAVIGATOR_H

#ident "@(#)navigator.h	1.16 01/20/94 Copyright 1990-1992 Sun Microsystems, Inc."


#include <doc/common.h>
#include <doc/cardcats.h>
#include <xview/xview.h>

// Forward declarations.
//
class	LOGGER;
class	TT_VIEW_DRIVER;
class	UIMGR;
class	BOOKSHELF;


class	NAVIGATOR {

    private:

	Xv_opaque	main_frame;	// main window frame
	UIMGR		*uimgr;		// UI manager
	TT_VIEW_DRIVER	*ttmgr;		// interface to ToolTalk
	CARDCATS	cardcats;	// card catalogs for locating AnswerBks
#ifdef	LOG
	LOGGER		*logger;	// event logger
#endif	LOG
	OBJECT_STATE	objstate;	// current state of this object.

	//Initialize frame for main Navigator window.
	//
	void		InitMainFrame(int argc, char **argv);


    public:

	NAVIGATOR();
	~NAVIGATOR();

	// Initialize Navigator.
	//
	STATUS		Init(int *argc_ptr, char **argv, ERRSTK &err);

	// Set the preferred viewing language for this session.
	//
	void		SetPreferredLanguage(const STRING &lang);

	// Enter XView event loop.
	//
	void		EnterEventLoop();

	// Public access methods for various NAVIGATOR private members.
	//
	TT_VIEW_DRIVER	*GetTTMgr() const;
	UIMGR		*GetUIMgr() const;
	Xv_opaque	GetMainFrame() const;
	CARDCATS	&GetCardCatalogs()	{ return(cardcats); }
	void		ViewFile (const STRING &filename, ERRSTK &err);
#ifdef	LOG
	LOGGER		*GetLogger() const;
#endif	LOG
};

// Call this routine when we run out of memory.
//
void	OutOfMemory();

// The main navigator object is globally available.
//
extern NAVIGATOR	*navigator;

// The current bookshelf is the only other major global object.
//
extern BOOKSHELF	*bookshelf;
extern STRING		bookshelf_path;

// Navigator version.
//
extern const STRING	navigator_version;

#endif	/* _NAVIGATOR_H */
