#define	NTYPES	4		/* Number of operand types */

#define	LONG	0
#define	STR	1
#define	ABYTE	2
#define	SHORT	3

#define	NOPS	5		/* Number of operators */

#define	EQ	0
#define	GT	1
#define	LT	2
#define	STRC	3		/* ...string compare */
#define	ANY	4
#define	SUB	64		/* ...or'ed in */

#define MAXBIND                 128


typedef struct
{
	long off;                       /* Byte offset into file */
	char type;                      /* String, byte, short, int... */
	long mask;                      /* If non-zero, mask value */
	char opcode;                    /* Test magic value */
	union {
		long    num;            /* if a numeric type */
		char    *str;           /* if a string type */
	} value;
	char *str;                      /* Magic number description */
}
B_MAGIC;

typedef struct
{
	char *buf;                      /* Malloced buffer */
	char *pattern;                  /* Filename regular expression */
	B_MAGIC *magic;                /* Or magic number description */
	char *application;              /* Object's action */
	char *iconfile;                 /* Icon filename */
	Pixrect *icon;                  /* Icon */
	char *print_method;		/* Print method */
	char *rgb;			/* RGB value */
	u_char color;                   /* Icon foreground color */
	u_char image;			/* View contents? */
	char *doc_id;
	char *filter;
}
B_BIND;
