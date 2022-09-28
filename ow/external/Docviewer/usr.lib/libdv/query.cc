#ident "@(#)query.cc	1.12 10/12/93 Copyright 1992 Sun Microsystems, Inc."


// This module contains the implementation for the semantic QUERY class.
#include <doc/query.h>
#include <doc/scopekey.h>
#include <doc/zonewght.h>
#include <ctype.h>
#include <xview/xview.h>

EXTERN_FUNCTION (int jmoranl, (char *dst, char *src, int is_mif));

#define FTLINT
#define FTHANSIC
extern "C" {
#include <ft/ftdefs.h>
}

// The following char control sequences or recognized by the Ful/Text 
// search and retrieval engine to induce boolean operations and 
// relevance ranking of the result list.
// The boolean AND and OR operators are terminated by and END_GROUP sequence.
//
static STRING	FTQUERY_AND       = "\\Ct ";
static STRING	FTQUERY_OR        = "\\Cu ";
static STRING	FTQUERY_END_GROUP = "\\C}";

// The KEY_ZONE sequence is used during scope-limited searches to
// match keywords in zone 67 .
// This convention requires the document indexing filters to supply 
// the appropriate keywords in order for the scoping mechanism to work.
// Any hits found in this zone will be given a weight of zero so that
// the ranking results will not be affected.
//
static STRING	FTQUERY_KEY_ZONE = "\\C67s \\C0$s";

// Document titles will be stored in zone 33.
// The TITLE_ZONES control sequence is used with titles-only
// searching to restrict searches to this zone.
//
static STRING	FTQUERY_TITLE_ZONES = "\\C33s ";

// Other query operators.
//
static STRING	FTQUERY_EXPAND_SUFFIX        = "\\C-w";
static STRING	FTQUERY_META_HYPHEN          = "\\C-~";
static STRING	FTQUERY_BEGIN_LITERAL_PHRASE = "\\Cp ";
static STRING	FTQUERY_BEGIN_PROXIMITY      = "\\Cy ";

const STRING
FTQUERY_ZONE(int zone)
{
	STRING	tmp;
	char	buf[20];

	sprintf(buf, "\\C%ds", zone);
	return(tmp = buf);
}

const STRING
FTQUERY_WEIGHT(int weight)
{
	STRING	tmp;
	char	buf[20];

	sprintf(buf, "\\C%d$s", weight);
	return(tmp = buf);
}

const STRING
FTQUERY_RANKING_ALGORITHM(int algorithm)
{
	STRING	tmp;
	char	buf[20];

	sprintf(buf, "\\C2:%d$v", algorithm);
	return(tmp = buf);
}


// Query constructor and destructor.
//
QUERY::QUERY() :
	ranking		(RANK_TF_IDF),
	max_docs	(30),
	titles_only	(BOOL_FALSE),
	scoped		(BOOL_FALSE),
	show_first	(BOOL_FALSE),
	operators	(BOOL_TRUE),
	sort_order	(SORT_BY_RANK)
{
	DbgFunc("QUERY::QUERY" << endl);
}

QUERY::~QUERY()
{
	DbgFunc("QUERY::~QUERY" << endl);
}

// Ful/Text query string construction routine.
//
void
QUERY::ComposeQuery(	LIST<SCOPEKEY*> &keys,
			LIST<ZONEWGHT*> &weights,
			STRING &query_string) const
{
	STRING	parsed_text;
	STRING	tmpstr;
	int	i;
	char	*input_lang;
 	char	tmp_text[1024];
 	STRING	jmor_text;

 	// why we can't replace the string in "text"?
 	jmor_text = text;

	// xv_default_server can be null if we're running via ab_admin
	if (xv_default_server == (Xv_server) NULL)
	   input_lang = setlocale (LC_CTYPE, NULL);
	else
 	   input_lang = (char *)xv_get(xv_default_server, XV_LC_INPUT_LANG);
 	if (input_lang && strlen(input_lang)>=2
 		&& !strncmp(input_lang, "ja", 2)) {
 	    jmoranl(tmp_text, (char *)~jmor_text, 0);
 	    jmor_text = tmp_text;
 	}


 	// Expand any embedded wildcarding sequences in the user typed
	// text first. e.g. hyphens, phrases, near groupings, and word
	// ending expansions.
	//
	if (operators)
		ParseQuery(jmor_text, parsed_text);
	else
		parsed_text = jmor_text;


	// Map ISO Latin-1 characters to FTCS.
	//
	MapISOStringToFTCS(parsed_text, tmpstr);
	parsed_text = tmpstr;


	// Fill in the specified ranking algorithm.
	//
	query_string = FTQUERY_RANKING_ALGORITHM(ranking) + "\n";


	// The user may select one of two searching strategy at
	// query invocation.  The scoped search restricts the search
	// to the documents which match a keyword list specified in
	// keys and the normal relevance ranking
	// specified in the weights parameter.
	//
	if (Scoped()) {
		// For scoped seaches the query string contains
		//	Select zone 67 
		//		OR the keywords 
		//      AND the results with the query string constructed 
		// 		in the ALL case.
		query_string += FTQUERY_AND + FTQUERY_KEY_ZONE + FTQUERY_OR;
		for (i = 0; i < keys.Count(); i++) {
			if (keys[i]->IsEnabled())
				query_string += keys[i]->Key() + " ";
		}
		query_string += FTQUERY_END_GROUP;
	}


	if ( ! TitlesOnly()) {
		// Searching in text and titles.
		// For each of the zones that will be searched 
		//	OR the numeric search results together
		//		Select the zone to be searched
		//		OR the terms together with the desired weight
		//
		query_string += FTQUERY_OR;
		for (i = 0; i < weights.Count(); i++) {
			if(weights[i]->Weight() == 0 )
				continue;
			query_string += "\n" +
				   FTQUERY_ZONE(weights[i]->Zone()) +
				   FTQUERY_WEIGHT(weights[i]->Weight()) +
				   FTQUERY_OR +
				   parsed_text +
				   FTQUERY_END_GROUP;
		}
		query_string += FTQUERY_END_GROUP ;
		query_string += "\n" ;

	} else {
		// For titles only searches weighting is not required, so
		//	Select the title zones and
		//		OR the terms together.
		//
		query_string += FTQUERY_TITLE_ZONES + FTQUERY_OR + parsed_text +
				FTQUERY_END_GROUP;
	}


	// Terminate the AND grouping for scoped searches.
	//
	if (Scoped()) {
		query_string += FTQUERY_END_GROUP;
	}


	DbgFunc("QUERY::ComposeQuery: " << text << endl);
}

// For debugging output a complete dump of the query object
// is available via the ostream operator friend function.
//
ostream & 
operator << (ostream &ostr, const QUERY &q) 
{
        ostr << "------------------ QUERY -----------"	<< endl;
        ostr << "Titles only:   " << q.titles_only	<< endl;
        ostr << "Ranking:       " << q.ranking		<< endl;
        ostr << "Max Docs:      " << q.max_docs		<< endl;
        ostr << "Show First:    " << q.show_first	<< endl;
        ostr << "Operators:     " << q.operators	<< endl;
        ostr << "Text:" << endl   << q.text		<< endl;
        ostr << "-------------- END QUERY -----------"	<< endl;

	return ostr ;
}

// Utility routine for converting embedding phrase, proximity,
// and suffix expansion operators to Ful/Text query syntax.
//
// Returns static STRING that will be overwritten on subsequent calls.
//
STATUS
ParseQuery(const STRING &inquery, STRING &outquery)
{
	BOOL		inphrase = BOOL_FALSE;
	int		balanced = 0;
	int		len, i;


	outquery = NULL_STRING;
	len = inquery.Length();

	for (i = 0; i < len; i++) {

		switch(inquery[i]) {

		case '\\':			// literal next character
			++i;
			// fall through...

		default:
			outquery += inquery.SubString(i, i);
			break;

		case '*':			// suffix expansion
			if (i > 0  &&  isalnum(inquery[i-1])) {
				outquery += FTQUERY_EXPAND_SUFFIX;
			}
			break ;

		case '-':			// Meta-hyphen enumeration
			if (i > 0      &&  isalnum(inquery[i-1])  &&
			    i < len-1  &&  isalnum(inquery[i+1])) {
				outquery += FTQUERY_META_HYPHEN;
			}
			break ;

		case '"':			// Double quoted phrase
			if (inphrase) {
				inphrase = BOOL_FALSE;
				outquery += FTQUERY_END_GROUP;
			} else {
				inphrase = BOOL_TRUE;
				outquery += FTQUERY_BEGIN_LITERAL_PHRASE;
			}
			break ;

		case '(':			// Parenthesized proximity
			++balanced;
			outquery += FTQUERY_BEGIN_PROXIMITY;
			break;

		case ')':
			if (balanced) {
				--balanced;
				outquery += FTQUERY_END_GROUP;
			}
			break ;
		}
	}

	while (balanced-- > 0)
		outquery += FTQUERY_END_GROUP;
	if (inphrase)
		outquery += FTQUERY_END_GROUP;


	return(STATUS_OK);
}

void
MapISOStringToFTCS(const STRING &iso_string,  STRING &ftcs_string)
{
	char		*ftcs_buf;	// buffer for xlated string
	int		len;		// length of FTCS buffer;


	if (iso_string == NULL_STRING) {
		ftcs_string = iso_string;
		return;
	}


	// Allocate buffer for the translation.
	// Since a given ISO character might be mapped into
	// an FTCS character sequence of one or 2 characters
	// (e.g., for handling accents), we'll make the buffer
	// 2x the length of the source string.
	// XXX for some reason this doesn't seem to be sufficient sometimes.
	// XXX so we'll make it 3x, plus throw in some slop.
	//
	len = 3 * iso_string.Length() + 50;
	ftcs_buf = new char [len+1];

#if	1
	//XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
	// We've defined our own version of "fttrlk()" in fttrtget.c.
	// It contains custom translation tables needed by "ftntstr()" below.
	// While we're linking against a shared "libft.so", we need
	// to reference fttrlk() explicitly or else the linker will
	// just use the libft version.
	// This section of code can be deleted once we go back to
	// statically linking "libft.a".
	//
	FTNTAB		fptab;
	fptab.nid = 'i';
	if (fttrlk(ftapih, &fptab) < 0) {
		int	err = fterrno;
		assert(0);
	}
	//XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
#endif	1

	// Have Ful/Text perform the translation based on the
	// ISO Latin translation table (table 'i') that we supplied.
	//
	if (ftntstr(iso_string, -1, ftcs_buf, len, 0, 'i')  <  0) {
		int	error = fterrno;
		DbgHigh("MapISOStringToFTCS: xlation failed: " << error<<endl);
		ftcs_string = iso_string;
	} else {
		ftcs_string = ftcs_buf;
	}

	delete ftcs_buf;

	DbgFunc("MapISOStringToFTCS: "		<< endl
			<< "\t"	<< iso_string	<< endl
			<< "\t"	<< ftcs_string	<< endl);
}

void
MapFTCSStringToISO(const STRING &ftcs_string,  STRING &iso_string)
{
	char		*iso_buf;	// buffer for xlated string
	int		len;		// length of ISO buffer;
	int		rc;


	if (ftcs_string == NULL_STRING) {
		iso_string = ftcs_string;
		return;
	}


	// Allocate buffer for the translation.
	// We'll make the buffer 3x as long as the source string,
	// just for kicks (see comment in "MapISOStringToFTCS()").
	//
	len = 3 * ftcs_string.Length() + 50;
	iso_buf = new char [len+1];


	// Have Ful/Text perform the translation based on the
	// ISO Latin translation table (table 'i') that we supplied.
	//
	if (ftntstr(ftcs_string, -1, iso_buf, len, FTFTCS, 'i')  <  0) {
		// translation failed - just return untranslated string
		int	error = fterrno;
		DbgHigh("MapFTCSStringToISO: xlation failed: " << error<<endl);
		iso_string = ftcs_string;
	} else {
		iso_string = iso_buf;
	}

	delete iso_buf;

	DbgFunc("MapFTCSStringToISO: "		<< endl
			<< "\t"	<< ftcs_string	<< endl
			<< "\t"	<< iso_string	<< endl);
}
