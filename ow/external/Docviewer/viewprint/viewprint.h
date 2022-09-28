#ifndef	_TOOLS_VIEWPRINT_H_
#define	_TOOLS_VIEWPRINT_H_

#ident "@(#)viewprint.h	1.2 93/01/20 Copyright (c) 1990-1992 Sun Microsystems, Inc."

#include <doc/common.h>
#include <doc/document.h>
#include <doc/psdoc.h>
#include <doc/list.h>
#include <signal.h>


class	PSOUT {

    protected:

	FILE		*psfp;
	OBJECT_STATE	objstate;


    public:

	PSOUT(FILE *fp);

	virtual ~PSOUT();

	STATUS		WriteString(const char *string, ERRSTK &);
	STATUS		WriteBytes(const char *, size_t nbytes, ERRSTK &);

	virtual STATUS	Close() = 0;
};

class	PSOUTFILE : public PSOUT {

    private:

	STRING		filename;

	PSOUTFILE(const STRING &path, FILE *fp);


    public:

	~PSOUTFILE();

	static PSOUTFILE	*Open(const STRING &path, ERRSTK &err);
	STATUS			Close();
};

class	PSOUTLP : public PSOUT {

    private:

	STRING		printer;

#if	defined(_SIG_PF) || !defined(SVR4)
	SIG_PF		saveSig;
#else
	void (*saveSig)(int);	// saveSig is ptr to func (int) returning void
#undef	SIG_DFL
#undef	SIG_ERR
#undef	SIG_IGN

#define	SIG_DFL	(void(*)(int))0
#define	SIG_ERR	(void(*)(int))-1
#define	SIG_IGN	(void(*)(int))1

#endif

	PSOUTLP(const STRING &printer, FILE *fp);


    public:

	~PSOUTLP();

	static PSOUTLP	*Open(	const STRING	&printer,
				int		ncopies,
				const STRING	&job_title,
				ERRSTK		&err);
	STATUS		Close();
};


void	NewHandler();

STATUS
PrintDoc(PSOUT		&psout,
	 const DOCUMENT	*doc,
	 BOOL		revPages,
	 int		offset,
	 const STRING	&pageRange,
	 ERRSTK		&err);

STATUS
PrintFile(const STRING	&prfile,
	  const STRING	&dir,
	  int		offset,
	  BOOL		rev,
	  const STRING	&pageRange,
	  PSOUT		&psout,
	  ERRSTK	&err);

STATUS
PrintList(LIST<STRING>	&list,
	  const STRING	&dir,
	  int		offset,
	  BOOL		rev,
	  const STRING	&pageRange,
	  PSOUT		&psout,
	  ERRSTK	&err);

STATUS
PrintPages(PSDOC	&psdoc,
	   int		begPage,
	   int		endPage,
	   BOOL		reverse,
	   PSOUT	&psout,
	   ERRSTK	&err);

STATUS
SendPage(PSDOC		&psdoc,
	 int		page,
	 PSOUT		&psout,
	 ERRSTK		&err);

STATUS
SendDoc(const PSDOC	&psdoc,
	FILE *const	fptr);

void
SendSave(PSOUT &psout);

void
SendRestore(PSOUT &psout);

void	Usage();

#endif	/* _TOOLS_VIEWPRINT_H_ */
