/*
  handcoded lexer to get rid of slow yylook routines
  produced by lex.
  */


#include <ctype.h>
#include <fcntl.h>
#include <rpc/rpc.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <ctype.h>

#include "rtable4.h"
#include "util.h"
#include "parser.h"

int externNumberVal;
char *externQuotedString;
Period externPeriod;
Appt_Status externApptStatus;
Tag externTag;
Privacy_Level externPrivacy;

extern char *pgname;

int yyylineno;

static int len;
static caddr_t start_of_mmapped_area;
static caddr_t current_ptr;
static caddr_t end;

#define input_char()	((end == current_ptr)?0:(*(current_ptr++)))
#define unput_char()    (((current_ptr>start_of_mmapped_area)?(current_ptr--):0))
			  
#define ALPHA  	1
#define DIGIT  	2
#define IGNORE 	3
#define SENDIT 	4
#define EOFILE 	5
#define QUOTE  	6
#define COMMENT	7
#define NEWLINE	8
#define DASH  	9

/* 
  the following token entry allows us to combine the
  hash table, token id and action on token recog 
  together.
  */

typedef struct token_data {
  const char * name;		/* lexical token representation */
  short first_token;		/* index into tokens.  points to
				   first token that hashes to this spot.
				   subsequent tokens may be found by following
				   next_token field IN first_token. */
  short	 next_token;
  void * info_to_change;	/* ptr to 4 byte value to change when token 
				   is found.  Ignored if NULL */
  int token_value;		/* value to set above to.  */
  int return_value;		/* value to return from yyylex when this token
				   is found */
} token_data_t;

static u_char parse_buffer[BUFSIZ*3];
static u_char initial_mask[255];
static u_int  sendit_value[255];

/*
  some macros to save my fingers
  */

#define PVAL(a) { #a,-1, -1, &externPeriod.period,a,PERIODVAL}
#define AVAL(a) { #a,-1, -1, &externApptStatus,a,APPTSTATUSVAL}
#define TVAL(a) { #a,-1, -1, &externTag.tag,a,TAGSVAL }
#define SVAL(a) { #a,-1, -1, &externPrivacy,a,PRIVACYVAL}

#define EMP(a,b) {#a,-1, -1, NULL,NULL,b}

token_data_t tokens[] = {
  PVAL(single),
  PVAL(daily),
  PVAL(weekly),
  PVAL(biweekly),
  PVAL(monthly),
  PVAL(yearly),
  PVAL(nthWeekday),
  PVAL(everyNthDay),
  PVAL(everyNthWeek),
  PVAL(everyNthMonth),
  PVAL(monThruFri),
  PVAL(monWedFri),
  PVAL(tueThur),
  PVAL(daysOfWeek),

  AVAL(active),
  AVAL(pendingAdd),
  AVAL(pendingDelete),
  AVAL(committed),
  AVAL(cancelled),
  AVAL(completed),

  TVAL(appointment),
  TVAL(reminder),
  TVAL(otherTag),
  TVAL(holiday),
  TVAL(toDo),

  SVAL(public),
  SVAL(private),
  SVAL(semiprivate),

  EMP(Version, VERSION),
  EMP(duration,DURATION),
  EMP(period,PERIOD),
  EMP(nth,NTH),
  EMP(enddate,ENDDATE),
  EMP(ntimes,NTIMES),
  EMP(what,WHAT),
  EMP(details,DETAILS),
  EMP(mailto,MAILTO),
  EMP(exceptions,EXCEPTION),
  EMP(attributes,ATTRIBUTES),
  EMP(author,AUTHOR),
  EMP(key,KEY),
  EMP(read,READ),
  EMP(write,WRITE),
  EMP(delete,DELETE),
  EMP(exec,EXEC),
  EMP(apptstat,APPTSTATUS),
  EMP(tags,TAGS),
  EMP(privacy,PRIVACY),
  EMP(add,ADD),
  EMP(remove,REMOVE),
  EMP(access,ACCESS),
  EMP(deny,DENY)};

#define NUMTOKES (sizeof(tokens)/sizeof(tokens[0]))

yyylex()
{
  char * strescapes();

  static int first_time=1;

  if(first_time) {
    init();
    first_time = 0;
  }

  while(1) {
    register u_char * ptr = parse_buffer;
    register u_char c;

    switch(initial_mask[ *ptr = input_char()]) {

    case IGNORE:
      continue;
      break;

    case NEWLINE:
      yyylineno++;
      break;

    case SENDIT:
      *(ptr+1) = 0;
      return(sendit_value[*ptr]);
      break;
      
    case DASH:
      /* make sure next input is a number */
      if( initial_mask[*(++ptr)=input_char()] != DIGIT) {
         fprintf(stderr, "%s: Unsupported char %c (ascii %d) found in callog file\n",
              pgname, *ptr, *ptr);
         return(0);
      }
      /* fall through */

    case DIGIT:
      while(initial_mask[*(++ptr)=input_char()] == DIGIT)
	;

      unput_char();

      *ptr = NULL;

      externNumberVal=atoi(parse_buffer); 
      return(NUMBER);
      break;

    case ALPHA:
      {
	register int bucket;
	register token_data_t * t;

	while(initial_mask[*(++ptr)=input_char()] == ALPHA)
	  ;

	unput_char();

	*ptr = NULL;

	bucket = tokens[hash_string(parse_buffer)].first_token;

	while(strcmp(tokens[bucket].name, (char*)parse_buffer) != 0)
	  if((bucket = tokens[bucket].next_token) == -1) /* end of chain */ {
	    fprintf(stderr, "%s: cannot lex %s in callog file\n", 
		    pgname, parse_buffer);
	    return(0);
	  }
	t = tokens + bucket;

	if(t->info_to_change)
	  *((int *) t->info_to_change) = t->token_value;
	return(t->return_value);
	break;
      }

    case QUOTE:       /* note that code removes leading and trailing quotes
		                                */
      while(1) {
	switch(*ptr = input_char())
	  {
	  case '\\':
	    *++ptr = input_char(); /* load next char in any case */
		ptr++; 
	    break;
	  case '"':
	    *ptr = 0;
	    strescapes((u_char *) parse_buffer); /* process any escape sequences */
	    externQuotedString = strdup((char *)parse_buffer);
	    return(QUOTEDSTRING);
	    break;
	  case EOFILE:
	  case '\n':
	  case '\r':
	    fprintf(stderr, "%s: missing matching \" in callog file\n", pgname);
	    return(0);
	  default:
	    ptr++;
	  }
      }
      
      
      
    case EOFILE:
      return(0);
      break;

    case COMMENT:
      {
	int i;
	for(i=0;i<3;i++) /* look for 4 **** as comment lead-in */
	  if(initial_mask[*(++ptr)=input_char()]!= COMMENT) {
	    *(++ptr) = 0;
	    fprintf(stderr, "%s: cannot lex %s in callog file\n",
		    pgname, parse_buffer);
	    return(0);
	  }

	while(input_char() != '\n') /* eat up rest of comment */
	  ;
        yyylineno++; /* since we eat the newline here.... */

	continue;
      }


    default:
      fprintf(stderr, "%s: Unsupported char %c (ascii %d) found in callog file\n",
	      pgname, *ptr, *ptr);
      return(0);
      break;
    }
  }
}

extern int
yywrap(f)
FILE* f;
{
  fclose (f);
  f = NULL;
  munmap(start_of_mmapped_area, len);
  return (1);
}

extern int
setinput (f)
FILE* f;
{
  struct stat buff;

#if 0
  if (f)
    yywrap(); /* perhaps after an error we did't close */
#endif
  
  if(fstat(fileno(f), & buff) < 0) {
    perror("fstat");
    return(-1);
  }
  
  if((start_of_mmapped_area = 
      mmap(NULL,  
	   len = buff.st_size,
	   PROT_READ,
	   MAP_PRIVATE,
	   fileno(f),
	   0)) == (caddr_t) -1) { /* mmap failed??? */
	perror("mmap");
	return(-1);
      }

  madvise(start_of_mmapped_area,
	  len,
	  MADV_SEQUENTIAL);

  end = start_of_mmapped_area + len;
  current_ptr = start_of_mmapped_area;
  yyylineno = 1;	
  
  return(0);
}

/*
  load parser masks & build
  token hash tables...
  we could make this staticly,
  inititialized, but it would
  be harder to maintain.
  */

/*
static u_char initial_mask[255];
static u_int  sendit_value[255];
*/

static init()
{
  int i;

  for(i=0;i<255;i++)
    if(isascii(i)) {
      if(isalpha(i))
	initial_mask[i] = ALPHA;
      else if(isdigit(i))
	initial_mask[i] = DIGIT;
      else if(isspace(i))
	initial_mask[i] = IGNORE;
      else {
	switch(i) {
	case '-':
	  initial_mask[i] = DASH;
	  break;
	case '*':
	  initial_mask[i] = COMMENT;
	  break;
	case ' ':
	  initial_mask[i] = IGNORE;
	  break;
	case '"':
	  initial_mask[i] = QUOTE;
	  break;
	case '(':
	  initial_mask[i] = SENDIT;
	  sendit_value[i] = OPENPAREN;
	  break;
	case ')':
	  initial_mask[i] = SENDIT;
	  sendit_value[i] = CLOSEPAREN;
	  break;
	case ',':
	  initial_mask[i] = SENDIT;
	  sendit_value[i] = COMMA;
	  break;
	case ':':
	  initial_mask[i] = SENDIT;
	  sendit_value[i] = COLON;
	  break;
	}
      }
    }
    else if(isspace(i))
      initial_mask[i] = IGNORE;

  initial_mask[0] = EOFILE;
  initial_mask['\n'] = NEWLINE;

  /*
    build token hash table
    */

  for(i=0;i<NUMTOKES;i++) {
    int bucket = hash_string(tokens[i].name);
    
    if(tokens[bucket].first_token == -1)
      tokens[bucket].first_token = i;
    else {
      int j = 0;
      bucket = tokens[bucket].first_token;
      while(tokens[bucket].next_token != -1)
	j++, bucket = tokens[bucket].next_token;
      tokens[bucket].next_token = i;
    }
  }
}


static int hash_string(register char * s)
{
  register unsigned result = 0;
  register int sum;
  
  while(sum = *s++)
    result = (result << 4) + sum;
  
  return(result % NUMTOKES);
}

/*---------------------------------------------------------------------------
  Strescapes performs escape character processing in a manner similar to the C
  compiler.  It processes a character string passed to it and performs the
  following substitutions in place:
  
  \\    ->   \
  \"    ->   "
  \e    ->   033
  \t    ->   011
  \n    ->   newline
  \0nn  ->   0nn
  \r    ->   cr
  
  A single \ in front of other characters is ignored(&removed).  As with
  most string routines, strescapes returns it's argument.  Note that
  inserting a \0 will end the string at that location, but the remainder of
  the string will be processed.
  
  ---------------------------------------------------------------------------*/


static char * strescapes(s)
     char * s;
{
  register char * in=s, * out=s;
  
  while(*in)
    {
      if(*in == '\\')
	switch(*(in+1))
	  {
	  case 'e':  /* an escape character */
	    *out++ = '\033';
	    in += 2;
	    break;
	    
	  case 't':  /* a tab character */
	    *out++ = '\t';
	    in += 2;
	    break;
	    
	  case 'n':  /* a newline */
	    *out++ = '\n';
	    in += 2;
	    break;
	  case 'r':  /* a carriage return */
	    *out++ = '\r';
	    in += 2;
	    break;
	  case '\\': /* a backslash */
	    *out++ = '\\';
	    in += 2;
	    break;
	  case '0':  /* a octal constant */
	    {
	      register int i,result;
	      in++;
	      for(result=i=0; (i<=3) && (*in >='0') && (*in <= '7') ; )
		{
		  result <<= 3;
		  result += (*in++) - '0';
		}
	      *out++ = result & 0377;
	      break;
	    }
	    
	  default:  /* not used as escape.... make it disappear */
	    in++;   /* this also handles nulls */
	    break;
	  }
      else
	*out++ = * in++;
    }
  *out='\0';
  return(s);
}


#ifdef TEST

main(int argc, char ** argv)
{
  FILE * f;
  int d;

  if(argc < 2) {
    printf("usage: %s filename\n", argv[0]);
    exit(1);
  }

  if((f = fopen(argv[1], "r")) == NULL) {
    perror("fopen");
    exit(2);
  }

  if(setinput(f) < 0) {
    exit(3);
  }

  while(d=yyylex()) {
    if(argc == 2)
      printf("received %d, text <%s>\n", d, parse_buffer);
  }

  exit(0);
}
    
#endif  
