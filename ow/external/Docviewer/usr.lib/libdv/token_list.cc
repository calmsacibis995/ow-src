#ident "@(#)token_list.cc	1.6 11/19/93 Copyright 1992 Sun Microsystems, Inc."


#include <doc/token_list.h>


TOKEN_LIST::TOKEN_LIST(const STRING &tokstr, char sep) :
	separator(sep)
{
	assert(separator != '\0');

	empty_string_tok = BOOL_FALSE;
	
	Parse(tokstr);

	DbgFunc("TOKEN_LIST::TOKEN_LIST: " << *this << endl);
}

TOKEN_LIST::TOKEN_LIST(char sep) :
	separator(sep)
{
	assert(separator != '\0');
	DbgFunc("TOKEN_LIST::TOKEN_LIST: " << *this << endl);

	empty_string_tok = BOOL_FALSE;
	
}

TOKEN_LIST::TOKEN_LIST(const STRING &tokstr, char sep,
		       const BOOL empty_string_is_tok) :
	separator(sep),
	empty_string_tok (empty_string_is_tok)
{
	assert(separator != '\0');

	Parse(tokstr);

	DbgFunc("TOKEN_LIST::TOKEN_LIST: " << *this << endl);
}

TOKEN_LIST::TOKEN_LIST(char sep,
		       const BOOL empty_string_is_tok) :
	separator(sep),
	empty_string_tok (empty_string_is_tok)

{
	assert(separator != '\0');
	DbgFunc("TOKEN_LIST::TOKEN_LIST: " << *this << endl);
}

const TOKEN_LIST &
TOKEN_LIST::operator = (const STRING &tokstr)
{
	Clear();
	Parse(tokstr);

	DbgFunc("TOKEN_LIST::operator= : " << *this << endl);
	return(*this);
}

void
TOKEN_LIST::Parse(const STRING &tokstr)
{
	STRING	token;
	STRING 	*orig_tok;
	int	token_start;
	int	len;
	int	i;
	STRING  *empty_string;
	BOOL 	sep_seen = BOOL_FALSE;
	BOOL	emp_str_used = BOOL_FALSE;

	empty_string = new STRING ("");
	
	
	len = tokstr.Length();

	token_start = 0;
	for (i = 0; i <= len; i++) {

		if (len > 0)
			if (tokstr[i] == separator)
				sep_seen = BOOL_TRUE;
		
		if (i < len  &&  tokstr[i] != separator)
			continue;

		// empty strings have no length
		if (token_start == i)	{
			
			// empty strings should be ok as tokens *and*
			// we should have seen at least one
			// separator char in the string - to account
			// for degenerate cases "", NULL_STRING,
			if (empty_string_tok == BOOL_TRUE &&
			    sep_seen == BOOL_TRUE) {
				this->Add (*empty_string);
				emp_str_used = BOOL_TRUE;
			}
		}
		
		else	{
			
			if (token_start < i) {
				BOOL orig_tok_used = BOOL_FALSE;

				token = tokstr.SubString(token_start, i-1);
				orig_tok = new STRING (token);
				token = STRING::CleanUp(token);

				// if empty strings are not tokens, don't
				// add them. If they are, add them
				// Note: CleanUp turns "   " into NULL_STRING
				if (token != NULL_STRING)
					this->Add(token);
				else	{
					
					if (empty_string_tok == BOOL_TRUE) {
						this->Add (*orig_tok);
						orig_tok_used = BOOL_TRUE;
					}
				}

			if (orig_tok_used == BOOL_FALSE)
				delete (orig_tok);	/* to test purify */
				
		
			}
		}
		

		token_start = i + 1;
	}


	if (emp_str_used == BOOL_FALSE)
		delete (empty_string); 	/* to test purify */

	DbgFunc("TOKEN_LIST::Parse: " << tokstr << " => " << *this << endl);
}

ostream &
operator << (ostream &ostr, TOKEN_LIST &tokens)
{
	int	count = tokens.Count();
	int	i;

	for (i = 0; i < count; i++) {
		ostr << tokens[i];
		if (i < count-1)
			ostr << tokens.separator;
	}

	return(ostr);
}
