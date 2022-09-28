#ifndef lint
static  char sccsid[] = "@(#)token.c 3.1 92/04/03 Copyr 1991 Sun Microsystems, Inc.";
#endif

#include <sys/types.h>
#include <string.h>
#include "util.h"
#include "token.h"
#define INITIAL_LENGTH 100

	
int
invalid_char(h, temp, expected)
	Token h;
	char *temp;
	char expected;
{
	h->stop = h->get(h);
	temp = strncat(temp, &h->stop, 1);
	return(h->stop != expected);
}


zero_data(data)
	Standard_filter data;
{
	if(data != NULL) data[0] = 0;
}

int
skip(h, data, filter, in_class)	
	Token h;
	Standard_filter data;
	Filter_proc filter;
	Boolean in_class;
{
	if(data!=NULL) zero_data(data);
	while((h->stop=h->get(h)) != NULL 
			&& filter(h->stop, data) == in_class);
}

Boolean
is_boolean(h)
	Token h;
{
	char *temp;

	temp =(char *)ckalloc(6);
	skip(h, NULL, whitespace_filter, true);
	temp = strncat(temp, &h->stop, 1);
	switch(h->stop) {
	  case 't':
	  	if(invalid_char(h, temp, 'r') || invalid_char(h, temp, 'u') ||
	       	invalid_char(h, temp, 'e')) {
			free(temp);
			temp = NULL;
			return(false);  /* syntax error, should return error */
		}
		free(temp);
		temp = NULL;
		return(true);
	  case 'f':
	  	if(invalid_char(h, temp, 'a') || invalid_char(h, temp, 'l') ||
	       	invalid_char(h, temp, 's') || invalid_char(h, temp, 'e')) {
			free(temp);
			temp = NULL;
	     		return(false);  /* syntax error, should return error */
		}
		free(temp);
		temp = NULL;
		return(true);
	  default:
		free(temp);
		temp = NULL;
	  	return(false);  /* syntax error, should return error */
	} 
}


/* ARGSUSED */
Boolean
whitespace_filter(c, data)		/* Filter_proc */
	char c; Standard_filter data;
{
	switch(c) {
	 case ' ':
	 case '\n':
	 case '\t':
	 	return(true);
	 default:
	 	return(false);
	}
}

Boolean
non_whitespace_filter(c, data)		/* Filter_proc */
	char c; Standard_filter data;
{
	if(c != NULL && !(whitespace_filter(c, data)))
		return(true);
	return(false);
}


/* ARGSUSED */
Boolean
alphabetic_filter(c, data)		/* Filter_proc */
	char c; Standard_filter data;
{
	if((c >= 'a' && c <= 'z') ||(c >= 'A' && c <= 'Z'))
		return(true);
	return(false);
}	


/* ARGSUSED */
Boolean
numeric_filter(c, data)		/* Filter_proc */
	char c; Standard_filter data;
{
	if(c >= '0' && c <= '9')
		return(true);
	return(false);
}

Boolean
alphanumeric_filter(c, data)		/* Filter_proc */
	char c; Standard_filter data;
{
	if (alphabetic_filter(c, data) || numeric_filter(c, data)) 
		return(true);
	return(false);
}

/* ARGSUSED */
Boolean
number_filter(c, data)		/* Filter_proc */
	char c; Standard_filter data;
{
	if((c >= '0' && c <= '9') || c == '+' || c == '-' || c == '.')
		return(true);
	return(false);
}

Boolean 
time_filter(c, data)		/* Filter_proc */
	char c; Standard_filter data;
{
	if(numeric_filter(c, data) || (c==':') ||
	   (c=='a') ||(c=='p') ||(c=='m') ||(c=='.'))
		return(true);
	return(false);
}

Boolean
date_filter(c, data)		/* Filter_proc */
	char c; Standard_filter data;
{
	if (alphanumeric_filter(c, data) ||
		(c=='/') ||
		(c=='.') ||
		(c=='-') ||
		(c==','))
		return(true);
	return(false);
} 

/* ARGSUSED */
Boolean
line_filter(c, data)		/* Filter_proc */
	char c; Standard_filter data;
{
	switch(c) {
	  case '\n':
		return(false);
	  case NULL:
		return(false);
	  default:
		return(true);
	}
}

/* ARGSUSED */
Boolean
eol_filter(c, data)			/* Filter_proc */
	char c; Standard_filter data;
{
	switch(c) {
	  case '\n':
		return(true);
	  case NULL:
		return(true);
	  default:
		return(false);
	}
}

Boolean
login_filter(c, data)		/* Filter_proc */
	char c; Standard_filter data;
{
	if(alphanumeric_filter(c, data) || (c=='_') || (c=='-'))
		return(true);
	switch(c) {
	  case '@':
		return(false);
	  case '\n':
		return(false);
	  case NULL:
		return(false);
	  default:
		return(false);
	}
}

Boolean
delimited(c, data)	/* Filter_proc */
	char c; Standard_filter data;
	/* first non-blank char is the delimiter;
	   must be used with skip=non_token		*/
{
	char *delimiter =(char *)data;
	if(data == NULL)
		return(false); /* should be an error */
	if(data[0] != 0) {
		if(c != delimiter[0])
			return(true);
		return(false);
	}
	if((!whitespace_filter(c, NULL)) &&(c != NULL))
		delimiter[0] = c;
	return(false);
}

long
number(h, radix)
	Token h; int radix;
{
	char *s;
	long result;

	s = filtered(h, NULL, number_filter, nothing);
	if (s==NULL) return(TOKEN_ERROR);
	result =(long)strtol(s,(char **)NULL, radix);
	return(result);
}
	

long
decimal(h) 
	Token h;
{
	long i;
	i = number(h, 10);
	return(i);
}


char
quote(c)		/* Quote_proc */
	char c;
{
	switch(c) {
	  case '\'':   
	  case '\"':
	  	return(c);
	  default:
	  	return(NON_QUOTE);
	  }
}


char 
brackets(c)		/* Quote_proc */
	char c;
{
	switch(c) {
	  case '(':
		return(')');
	  case '[':
	  	return(']');
	  case '{':
	  	return('}');
	  case '<':
	  	return('>');
	  default:
	  	return(NON_BRACKET);
	}
}


int
quote_filter(c, close_quote)
	char c; char close_quote;
{
	if(c == NULL)
		return(TOKEN_ERROR);
	return(c != close_quote);
}


char *
filtered(h, data, filter, skip)
	Token h; Standard_filter data; 
	Filter_proc filter; Skip skip; 
{
	char *value=NULL;
	int  max = INITIAL_LENGTH;

	if(h == NULL)
		return(NULL);
	if(filter == NULL)
		filter = non_whitespace_filter;
	zero_data(data);
	for(;;) {
		if((h ->stop = h->get(h)) == NULL)
			return(NULL);
		if(skip == white_space && whitespace_filter(h->stop, NULL))
			continue;
		if(filter(h->stop, data))
			break;
		if(skip == nothing || skip == white_space)
			return(NULL);
	}
	value =(char *) ckalloc(max);
	for(;;) {
		value = strncat(value, &h->stop, 1);
		if(cm_strlen(value) == (max-1)) {  /* string bounds fault */
			max = cm_strlen(value) + max;
			value = (char *) realloc(value, max);
		}
		h->stop = h->get(h);
		if (h->stop == NULL ||
			!filter(h->stop, data) )
				break;
	}
	return(value);
}

  
char *
maybe_quoted(h, data, filter, is_quote, skip, temporary)
	Token h; Standard_filter data; Filter_proc filter;
	Quote_proc is_quote; Skip skip; Boolean temporary;
{
	char *old	= NULL;
	char *temp	= NULL;
	char *value	= NULL;
	int  max	= INITIAL_LENGTH;
	Filter_proc apply_filter = filter;
	char close_quote;

	if(h == NULL)
		return(NULL);
	if(is_quote == NULL) is_quote = quote;
	if(filter == NULL) filter = non_whitespace_filter;
	zero_data(data);
	for(;;) {
		if((h->stop = h->get(h)) == NULL)
			return(value);
		if(skip == white_space && whitespace_filter(h->stop, NULL))
			continue;
		if((close_quote = is_quote(h->stop)) != NON_QUOTE) {
			if((h->stop = h->get(h)) == close_quote) {  /* literal */
				h->stop = h->get(h);
				if(h->stop == close_quote)
					break; 
				return(value);
			}
			else {
				if(h->stop == NULL)
					return((char*)TOKEN_ERROR);  /* unterminated */
				apply_filter = (Filter_proc)quote_filter;
				break;
	      		}
	    	}
	  	if(filter(h->stop, data))
			break;
	  	if(skip == nothing || skip == white_space)
			break;
	}
	value =(char *) ckalloc(max);	
	while(h->stop != NULL) {
		value = strncat(value, &h->stop, 1);
		if(cm_strlen(value) == max) {  
			max = cm_strlen(value) + max;
			temp =(char *) ckalloc(max);
			cm_strcpy(temp, value);
			free(value);
			value = temp;
			temp = NULL;
	  	}
		if(!(apply_filter(h->stop = h->get(h), close_quote))) {
			if(apply_filter == (Filter_proc)quote_filter) {
				h->stop = h->get(h);
				if(h->stop == close_quote)
					continue;
	      		}
		break;
		}
	}	
	if(!temporary) {
		old = value;
		value = cm_strdup(old);
		free(old);
		old = NULL;
		return(value);
	}
	return(NULL);
}



item(h, value)
	Token h; char *value;
{
	int max = INITIAL_LENGTH;

	value =(char *) ckalloc(max); 	
	skip(h, NULL, (Filter_proc)white_space, true);
	for(;;) {
		if(whitespace_filter(h->stop, NULL) || h->stop == NULL)
			break;
		value = strncat(value, &h->stop, 1);
		if(cm_strlen(value) == max) {  
	  		max = cm_strlen(value) + max;
	  		value = (char *) realloc( value, max);
	  	}	    
	  	h->stop = h->get(h);
	}
	if(cm_strlen(value) == 0) {
		free(value);
		value = NULL;
	}
}

/* Coercions & Utilities */
	
typedef struct {
	Token_object token;	/* token object */
	char *s;
	int index;
} StrObject, *Str;

char
default_string_get(h)
	Token h;
{
	char a;
	Str str =(Str)h;
 
	if(str->index < cm_strlen(str->s)) {
		a = str->s[str->index];
		str->index++;
		return(a);
	}
	return(NULL);
}
	  

/*	Must call this before the default_string_get is called
	to encapsulate the string properly			*/

Token
string_to_handle(s, offset)
	char *s; int offset;
{
	Str str = (Str) ckalloc(sizeof(StrObject));
	str->s = cm_strdup(s);
	str->index = offset;
	str->token.get = default_string_get;
	str->token.stop = NULL;	
	return(&str->token);

}

void
destroy_handle(t)
	Token t;
{
	Str str;

	if (t==NULL) return;
	str = (Str) t;
	if (str->s != NULL) {
		free(str->s);
		str->s=NULL;
	}
	free(str);
	str=NULL;
}
