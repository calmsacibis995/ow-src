/*	static  char sccsid[] = "@(#)token.h 3.1 92/04/03 Copyr 1989 Sun Micro";

	token.h
*/

#define NULL 0
#define NON_BRACKET NULL
#define NON_QUOTE NULL
#define TOKEN_ERROR -1

typedef char(*Getchar_proc)(/* Token h */);

typedef struct object {
	Getchar_proc get;
	char stop;		
} Token_object, *Token;


typedef enum {nothing, white_space, non_token} Skip;

typedef caddr_t Standard_filter[2];

typedef Standard_filter *State_ptr;

typedef Boolean(*Filter_proc)(/* char c, Standard_filter data */);

typedef char(*Quote_proc)(/* char c */);

extern Boolean is_boolean(/* Token h */);

extern long number(/* Token h, int radix */);

extern long decimal(/* Token h */);

extern item(/* Token h, char *value */);

extern caddr_t filtered(/* Token h, caddr_t data, Filter_proc filter,
	Skip skip */);

extern int skip(/* Token h, caddr_t data, Filter_proc filter,
	int in_class */);

extern caddr_t maybe_quoted(/* Token h, caddr_t data, Filter_proc filter,
	Quote_proc quote, Skip skip, char *value */);

extern Boolean time_filter(/* char c, Standard_filter data */);

extern Boolean date_filter(/* char c, Standard_filter data */);

extern Boolean alphabetic_filter(/* char c, Standard_filter data */);

extern Boolean alphanumeric_filter(/* char c, Standard_filter data */);

extern Boolean delimited_filter(/* char c, Standard_filter data*/);

extern Boolean line_filter(/* char c, Standard_filter data */);

extern Boolean eol_filter(/* char c, Standard_filter data */);

extern Boolean non_whitespace_filter(/* char c, Standard_filter data */);

extern Boolean numeric_filter(/* char c, Standard_filter data */);

extern Boolean login_filter(/* char c, Standard_filter data */);

extern Boolean name_filter(/* char c, Standard_filter data */);

extern Boolean whitespace_filter(/* char c, Standard_filter data */);

extern char brackets(/* char c */);

extern char quote(/* char c */);

extern Token string_to_handle(/* char *s, int offset */);

extern void destroy_handle(/* Token t */);




