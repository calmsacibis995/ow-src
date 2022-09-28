/* @(#)assert.h	3.1 - 92/04/03 */

# ifdef DEBUG
# ifdef __STDC__
# define ASSERT(ex)	{if (!(ex)){(void)fprintf(stderr,"Assertion failed: file \"%s\", line %d (%s)\n", __FILE__, __LINE__, #ex );exit(1);}}
# else
# define ASSERT(ex)	{if (!(ex)){(void)fprintf(stderr,"Assertion failed: file \"%s\", line %d (%s)\n", __FILE__, __LINE__, "ex" );exit(1);}}
# endif
# else
# define ASSERT(ex)
# endif
