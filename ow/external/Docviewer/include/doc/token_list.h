#ifndef	_TOKEN_LIST_H
#define	_TOKEN_LIST_H

#ident "@(#)token_list.h	1.5 06/11/93 Copyright 1990 Sun Microsystems, Inc."

#include <doc/common.h>
#include <doc/list.h>

class	TOKEN_LIST : public LIST<STRING> {

    private:

	char	separator;

	void	Parse(const STRING &token_string);

	BOOL 	empty_string_tok;


    public:

	TOKEN_LIST(const STRING &token_string, char sep);
	TOKEN_LIST(char sep);
	TOKEN_LIST(const STRING &token_string, char sep,
		   const BOOL empty_string_is_tok);
	TOKEN_LIST(char sep, const BOOL empty_string_is_tok);
	~TOKEN_LIST()		{ }

	// Assign a new token list string to this token list.
	//
	const TOKEN_LIST	&operator = (const STRING &token_string);

	friend ostream	&operator << (ostream &ostr, TOKEN_LIST &);
};

#endif	_TOKEN_LIST_H
