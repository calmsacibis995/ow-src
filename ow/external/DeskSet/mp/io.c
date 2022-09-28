
/*  @(#)io.c 3.7 94/01/20
 *
 *  Copyright (c) Steve Holden and Rich Burridge.
 *                All rights reserved.
 *
 *  Permission is given to distribute these sources, as long as the
 *  copyright messages are not removed, and no monies are exchanged.
 *
 *  No responsibility is taken for any errors inherent either
 *  to the comments or the code of this program, but if reported
 *  to me then an attempt will be made to fix them.
 */

#include <widec.h>
#include "mp.h"
#include "extern.h"


/* Emptyline returns true if its argument is empty or whitespace only */
         
bool
emptyline(str)
char *str ;
{
  while (*str)
    {
      if (!isspace(*str)) return(FALSE) ;
      str++ ;
    }
  return(TRUE) ;
}


#define MAXLEN 4096	/* Maximum input line length */

/*  Read an input line into nextline, setting end_of_file, end_of_page
 *  and end_of_line appropriately.
 */

void
readline()
{
  int c ;
  int i = 0 ;      /* Index into current line being read. */
  int len = 0 ;    /* Length of the current line. */
  static wchar_t wclinebuf[MAXLEN];
  static wchar_t *wcline = NULL;
  char line[MAXLEN];
  wchar_t wcbuf[MAXLINE];

  if (end_of_file) return ;
  end_of_page = end_of_line = FALSE ;

  if (wcline == NULL) {
    while ((c = getc(fp)) != EOF && c != '\n' && c != '\f')
      {
        if (c == '\t')
          {
            do
              {
                line[i++] = ' ' ;
                len++ ;
              }
            while (len % 8 != 0);
          }
        else
          { 
            line[i++] = c ;
            len++ ;
          }
        if (c == '\b')
          {
            len -= 2 ;
            i -= 2 ;
          }
      }
    line[i] = '\0' ;

    if (elm_if && c == '\f')
      {
        len-- ;
        i-- ;
      }

    switch (c)
      {
        case EOF  : if (i == 0) end_of_file = TRUE ;
                    else
                      { 
                        UNGETC(c, fp) ;
                      }
                    break ;
        case '\n' : 
                    break ;

/*  /usr/ucb/mail for some unknown reason, appends a bogus formfeed at
 *  the end of piped output. The next character is checked; if it's an
 *  EOF, then end_of_file is set, else the character is put back.
 */

        case '\f' : if ((c = getc(fp)) == EOF) end_of_file = TRUE ;
                    else UNGETC(c, fp) ;

                    end_of_page = TRUE ;
                    break ;
      }
    
   /*  save the line to a buffer in wide char format
    */
    mbstowcs(wclinebuf, line, strlen(line) + 1);
    wcline = wclinebuf;
  }

  if (wscol(wcline) <= llen) {
      wcstombs(line, wcline, MAXLINE);
      wcline = NULL;
      end_of_line = TRUE ;
  } else {
      for (i = 0, wcbuf[0] = '\0'; wscol(wcbuf) <= llen; i++) {
	wcbuf[i] = wcline[i];
	wcbuf[i+1] = '\0';
      }
      wcbuf[--i] = '\0';
      wcline += i;
      wcstombs(line, wcbuf, MAXLINE);
  }

  clen = strlen(line) + 1; 	/* Current line length (includes newline). */

  strcpy(nextline, euc_to_octal(line));
}
