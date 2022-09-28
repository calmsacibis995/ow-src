/*
 * Copyright (c) 1996, Sun Microsystems, Inc.
 * All rights reserved.
 */
 
#pragma ident	"@(#)lcl_mimehead.c 1.5	96/08/02 SMI"

/*////////////////////////////////////////////////////////////////////////
Copyright (c) 1992 Electrotechnical Laboratry (ETL)

Permission to use, copy, modify, and distribute this material for any
purpose and without fee is hereby granted, provided that the above
copyright notice and this permission notice appear in all copies, and
that the name of ETL not be used in advertising or publicity pertaining
to this material without the specific, prior written permission of an
authorized representative of ETL.
ETL MAKES NO REPRESENTATIONS ABOUT THE ACCURACY OR SUITABILITY OF THIS
MATERIAL FOR ANY PURPOSE.  IT IS PROVIDED "AS IS", WITHOUT ANY EXPRESS
OR IMPLIED WARRANTIES.
/////////////////////////////////////////////////////////////////////////
Content-Type: program/C; charset=US-ASCII
Program:      mimehead.c (MIME header encoder/decoder)
Author:       Yutaka Sato <ysato@etl.go.jp>
Description:
    MIME PartII (RFC1522) encoder/decoder for multibyte ISO-2022 charsets
    -----------------------------------------------------------------------
    EN_FGETC ->[ Encode ->encode_word <= encode_one ]->EN_FPUTC ->EN_FPUTC1
               [        ->noencode_word             ]
   
    DE_FGETC ->[ Decode ->decord_word <= scan_eword ]
               [                      -> disp_word  ]
               [        ->nodecode_word             ]->DE_FPUTC ->DE_FPUTC1
    -----------------------------------------------------------------------
History:
        920515	extracted from mmsclient.c
        920516	added MIME header encoder/decoder for ISO-2022-JP
	920930	added switch to ASCII at the end of splitted encoded-text
	930826	added DecodeOverwrite()
	930915	added LWSP_CHAR -> liner-white-space translation in EN_FPUTC()
	930925	moved unfolding operation from DE_FGETC() to DE_FPUTC()
	930925	removed LWSP BETWEEN encoded word rather than AFTER
	930925	added folding in ascii text "noencode_word"
	930925	added output flush on FORMFEED code
	930927	removed generation of empty encoded-text in encode-word
	930927	removed folding in noencode_word except preceeded encode-word
	931007	fixed the DE_FPUTC bug which eats the spaces at the top of body
	931022	fixed the tmpHeaderEncode which skips message body
	931029	infinite loop on folding to avoid encode_word()<MIN_ENCODEDLEN
	931103	decoded E-W come always terminate in ASCII at the end of line
	931105	finally fixed the "infinite loop on folding.." in encode_word()
	931116	extracted from mmsencode.c
	931227	added parameter decl. for DEC-Alpha(Thanks to <ko@soum.co.jp>)
	931228	changed type ch->int to detect EOF at disp_word on RS6000/AIX
	940219	experimental encoding rule "B-1.0" (ISO-2022-JP local)
	940222	escaped 'trigraph sequence replacement' of '?\?' in sprintf()
 1.1	940223	encode LWSP between ewords
 1.2	940224	passing CHAR with charset to DE_FPUTC(mainly for encoded space)
 1.2.1  940521  preserve line terminator (LF or CRLF) <anaka@mrit.mei.co.jp>
Bugs:
	Any linear-white-space between encoded-words should be ignored
	Should support any charsets & encodings in ISO-2022 or ISO-8859
	960620  adoption to lcl library.
///////////////////////////////////////////////////////////////////////*/

#include <stdio.h>
#include "lcl_str_stdio.h"
FILE *tmpfile();
FILE *str_fopen();
char *strchr(),*getenv();
#define MAX_LNSIZE	1024
typedef char MsgLine[MAX_LNSIZE];

#define DEBUG(a)
/*
#define DEBUG(a)	a
*/


#define ES_NONE	0
#define ES_IN	1	/* type-A encoding (not supported yet...) */
#define ES_OUT	2	/* type-B encoding */

int MIME_SPACE_ENCODING = ES_OUT;

/*//////////////////////////////////////////////////////////////////////*/

#define MAXCOL			72
#define DISPCOLS		80

#define SPACECTL_LENG		4
#define SWCODE_LENG		4 /*length(encoded(charset sw ESC seq))*/
#define MIN_ENCODEDLEN		4 /* minimum length of encoded text(base64)*/

#define ENCODE_BEGIN		"=?"
#define CHARSET_DONE		'?'
#define ENCODING_DONE		'?'
#define ENCODE_DONE		"?="

#define LF			'\n'
#define NL			'\n'
#define CR			'\r'
#define TAB			'\t'
#define SPACE			' '
#define FORMFEED		'\f'

#define LWSP_CHAR(ch)		(ch == TAB || ch == SPACE)
#define FOLD_CHAR		SPACE
#define SPECIALS		"()<>@,;:\\\".[]"
#define DELIMITER(ch)		(ch==LF || LWSP_CHAR(ch) || strchr(SPECIALS,ch))
#define IS_PRE_DELIMITER(ch)	DELIMITER(ch)
#define IS_POST_DELIMITER(ch)	(ch==EOF || DELIMITER(ch))

#define NLNL			0x80000001
#define XC_EN_FOLD		0x80000002
#define XC_DE_UNFOLD		0x80000003
#define XC_DE_CATENATE		0x80000004
#define XC_DE_EWORD_WAS_PUT	0x80000005
#define XC_DE_FORMFEED		0x80000006
#define XC_DE_DEL_LWSP		0x80000007
#define XC_DE_IGN_LWSP		0x80000008
#define XC_DE_OFLUSH		0x80000009

#define ENCODE_NONE		 0
#define ENCODE_BASE64		"B"
#define	ENCODE_QP		"Q"

/*
 * ISO-2022 LOCAL
 */
#define DELSP_PRE	1	/* delete prefixed LWSP */
#define DELSP_POST	2	/* delete postfixed LWSP */
#define DELSP_BOTH	3	/* delete bothside LWSP */

/*//////////////////////////////////////////////////////////////////////*/
/*	character sets
 */

/*
 *	ISO-2022 character set switch sequences
 */
#define ESC			033
#define GOTO_1BCODE		'('
#define GOTO_2BCODE		'$'

/*
 *	basic charset
 */
typedef struct {
	char	 iso2022[8];
	char	*name;
	int	 delspace;	/* as indication code for space deletion */
} Charset;


#define B_US_ASCII	1
#define B_JP_ASCII	2
#define B_JP_KANJI1	3
#define B_JP_KANJI2	4

static Charset BasicCharsets[] = {
	{"",       "UNKNOWN"	},
	{"\033(B", "US_ASCII"	},
	{"\033(J", "JISX0201-R", DELSP_BOTH},
	{"\033$@", "JISX0208-1", DELSP_PRE },
	{"\033$B", "JISX0208-2", DELSP_POST},
	0
};
#define CODESW_SEQ(bset)	BasicCharsets[bset].iso2022
#define GOTO_ASCII_SEQ		BasicCharsets[B_US_ASCII].iso2022
#define GOTO_ASCII_SEQ_LEN	strlen(GOTO_ASCII_SEQ)

static char *DELSP_SEQUENCE[8] = {
	0,
	BasicCharsets[B_JP_KANJI1].iso2022,
	BasicCharsets[B_JP_KANJI2].iso2022,
	BasicCharsets[B_JP_ASCII].iso2022,
	0
};
#define DELSP_OP(bset)		BasicCharsets[bset].delspace
#define DELSP_SEQ(delop)	DELSP_SEQUENCE[delop]


/*
 *	MIME charset (may include encoding system and several charsets)
 */
char M_US_ASCII[]	= "US-ASCII";
char M_ISO_8859_8[]	= "ISO-8859-8";
char M_ISO_2022_JP[]	= "ISO-2022-JP";

typedef struct {
	int	local;
	char	codesw;
	char   *mcharset;
	int	basic_charset;
	char   *encoding;
} MimeCharset;

static MimeCharset Codes1[16] = {
	{1, 'B', M_US_ASCII,	B_US_ASCII,  ENCODE_NONE	},
	{1, 'J', M_US_ASCII,	B_JP_ASCII,  ENCODE_NONE	},
	0
};
static MimeCharset Codes2[16] = {
	{1, '@', M_ISO_2022_JP,	B_JP_KANJI1, ENCODE_BASE64	},
	{1, 'B', M_ISO_2022_JP,	B_JP_KANJI2, ENCODE_BASE64	},
	0
};

MIME_localCharset(mcharset)
	char *mcharset;
{	int csi;
	char *cs;

	for(csi = 0; cs = Codes1[csi].mcharset; csi++)
		if( lcl_strcasecmp(cs,mcharset) == 0 )
			return Codes1[csi].local;

	for(csi = 0; cs = Codes2[csi].mcharset; csi++)
		if( lcl_strcasecmp(cs,mcharset) == 0 )
			return Codes2[csi].local;
	return 0;
}

/*//////////////////////////////////////////////////////////////////////*/
/*
 */
typedef struct {
	int	c_ch;		/* character code value */
	char*	c_mcharset;	/* MIME charset */
	int	c_bcharset;	/* basic charset */
} CHAR;
static CHAR NULL_CHAR = { 0,  M_US_ASCII, B_US_ASCII };

/*
 *	ROUND ROBBIN BUFFER
 */
#define RRBUFF_SIZE 8
typedef struct {
	CHAR	b_BUFF[RRBUFF_SIZE];
	int	b_putx;
	int	b_getx;
} RRBUFF;
static RRBUFF NULL_RRBUFF = {0};

typedef struct {
	FILE	*in_file;
	int	 in_column;
	char	*in_mcharset;		/* current MIME charset */
	int	 in_bcharset;		/* current basic charset */
	int	 in_bcharset_got;	/* bcharset was got explicitly */
	char	*in_encoding;		/* B or Q */
	int	 in_prevch;		/* EN_FGETC() local */
	RRBUFF	 in_BUFF;		/* EN_FGETC() local */
	RRBUFF	 in_PUSHED;		/* EN_UNGETC() -> EN_FGETC() */

	FILE	*out_file;
	int	 out_column;
	int	 out_lastputch;		/* EN_FPUTC() -> encode_word() */
	int	 out_whichASCII;	/* disp_word() -> DE_FPUTC(),C1() */
	int	 out_enLWSP;		/* EN_FPUTC() local */
	CHAR	 out_prevCHAR;		/* EN_FPUTC1() local */
	CHAR	 out_deLWSP[4];		/* DE_FPUTCX() local */
	int	 out_prev_bcharset;	/* DE_FPUTC1X() local */

	union { int all; struct { unsigned int
		 MIMEencoded :1,	/* I: this field is MIME encoded */
		 end_CRLF    :1,	/* I: line terminates with CRLF */
		 eat_SPACE   :2,	/* I: decoder space eraser */
		 ext_SPACE   :1,	/* O: external space encoding */
		 unfolding   :1,	/* O: unfold decoder output */
		 ign_SPACE   :1,	/* O: ignore postfix space */
		 gen_SPACE   :1,	/* O: just after space was generated */
		 after_eword :1;	/* O: just after eword has put */
	} mode; } io_MODES;
} INOUT;
#define ENCODE_EXT	io_MODES.mode.ext_SPACE

static INOUT_init(io,in,out)
	INOUT *io;
	FILE *in,*out;
{
	io->in_file           = in;
	io->in_column         = 0;
	io->in_mcharset       = M_US_ASCII;
	io->in_bcharset       = B_US_ASCII;
	io->in_bcharset_got   = 0;
	io->in_encoding       = ENCODE_NONE;
	io->in_prevch         = EOF;
	io->in_BUFF           = NULL_RRBUFF;
	io->in_PUSHED         = NULL_RRBUFF;

	io->out_file          = out;
	io->out_column        = 0;
	io->out_lastputch     = 0;
	io->out_whichASCII    = B_US_ASCII;
	io->out_enLWSP        = 0;
	io->out_prevCHAR      = NULL_CHAR;
	io->out_deLWSP[0]     = NULL_CHAR;
	io->out_prev_bcharset = B_US_ASCII;

	io->io_MODES.all      = 0;
	io->ENCODE_EXT        = MIME_SPACE_ENCODING == ES_OUT;
}

#define in_CODESW_SEQ(io)	CODESW_SEQ(io->in_bcharset)
#define MIME_ENCODED	io_MODES.mode.MIMEencoded
#define EAT_SPACE	io_MODES.mode.eat_SPACE
#define UNFOLD_LINE	io_MODES.mode.unfolding
#define IGN_POST_SPACE	io_MODES.mode.ign_SPACE
#define SPACE_WAS_GEN	io_MODES.mode.gen_SPACE
#define EWORD_WAS_PUT	io_MODES.mode.after_eword


#define NEXT_RRBUFF(BP) (\
	((RRBUFF_SIZE <= ++(BP)->b_putx) ? ((BP)->b_putx = 0):0), \
	&(BP)->b_BUFF[(BP)->b_putx] \
	)

#define PUT_RRBUFF(BP,CH) (\
	/* must check full here */ \
		((RRBUFF_SIZE <= ++(BP)->b_putx) ? ((BP)->b_putx = 0):0), \
		((BP)->b_BUFF[(BP)->b_putx] = *CH) \
	)

#define GET_RRBUFF(BP,CH) (\
	((BP)->b_putx == (BP)->b_getx) ? 0 : (\
		((RRBUFF_SIZE <= ++(BP)->b_getx) ? ((BP)->b_getx = 0):0), \
		(*CH = (BP)->b_BUFF[(BP)->b_getx]), \
		&(BP)->b_BUFF[(BP)->b_getx] \
	))


static int end_CRLF;
static NLfgetc(in)
	FILE *in;
{	int ch;

	ch = fgetc(in);
	if( ch == CR ){
		ch = fgetc(in);
		if( ch == LF ){
			end_CRLF = 1;
			ch = NL;
		}else{
			if( ch != EOF )
				ungetc(ch,in);
			ch = CR;
		}
	}else
	if( ch == LF ){
		end_CRLF = 0;
		ch = NL;
	}
	return ch;
}
static NLfputc(ch,out)
	FILE *out;
{
	if( ch == NL && end_CRLF )
		fputc(CR,out);
	fputc(ch,out);
}

static EN_UNGETC(CH,io)
	INOUT *io;
	CHAR *CH;
{
	PUT_RRBUFF(&io->in_PUSHED,CH);
}

static CHAR *EN_FGETC(io)
	INOUT *io;
{	int ch;
	MimeCharset *csw;
	int ci;
	char *mcharset;
	FILE *infile = io->in_file;
	CHAR *CH;
	RRBUFF *BP;

	BP = &io->in_BUFF;
	CH = NEXT_RRBUFF(BP);

	BP = &io->in_PUSHED;
	if( GET_RRBUFF(BP,CH) ){
		ch = CH->c_ch;
		goto EXIT;
	}

	*CH = NULL_CHAR;
GET1:
	ch = NLfgetc(infile);
GOT1:
	if( ch != ESC )
		goto exit;

	/* got ESC character */
	if( (ch = fgetc(infile)) == EOF )
		goto exit;

	if( io->in_prevch == NL )
		if( LWSP_CHAR(ch) )
			goto GET1;

	switch( ch ){
		default:	goto exit;
		case GOTO_1BCODE: csw = Codes1; break;
		case GOTO_2BCODE: csw = Codes2; break;
	}
	if( (ch = fgetc(infile)) == EOF )
		goto exit;

	for( ci = 0; mcharset = csw[ci].mcharset; ci++ ){
		if( ch == csw[ci].codesw ){
			io->in_mcharset = mcharset;
			io->in_encoding = csw[ci].encoding;

			if( io->in_column == 0 && io->in_bcharset_got )
				io->EAT_SPACE = DELSP_OP(io->in_bcharset);

			io->in_bcharset_got = 1;
			io->in_bcharset = csw[ci].basic_charset;
			break;
		}
	}

	ch = NLfgetc(infile);
	if( ch == ESC )
		goto GOT1;
exit:
	CH->c_ch = ch;
	CH->c_mcharset = io->in_mcharset;
	CH->c_bcharset = io->in_bcharset;

EXIT:
	io->in_prevch = ch;
	io->in_column++;
	if( ch == NL || ch == NLNL ){
		io->in_column = 0;
		io->EAT_SPACE = 0;
		io->MIME_ENCODED = 0;
	}

	return CH;
}

static ew_overhead(charset,encoding)
	char *charset,*encoding;
{	MsgLine overhead;

	sprintf(overhead,"=?%s?%s?x?= ",charset,encoding);
	return strlen(overhead) - 1;
}

static EN_FPUTC0(ch,io)
	INOUT *io;
{
	if( ch == NL ){
		io->SPACE_WAS_GEN = 0;
		io->out_column = 0;
	}else	io->out_column += 1;
	NLfputc(ch,io->out_file);
}

/*
 *	extra folding before a lengthy encoded-word
 *	put =?charset?encoding? at the beginning of non-ASCII
 *	put SPACE before it if the previous char is not DELIMITER
 *
 *	put ?= at the end of non-ASCII
 *	put SPACE after it if the next char is not DELIMITER
 */
static EN_FPUTC1(ch,io,charset,encoding)
	INOUT *io;
	char *charset,*encoding;
{	char *cp;
	MsgLine line;

	if( charset != io->out_prevCHAR.c_mcharset ){

		/* AT THE END OF A ENCODED WORD */
		if( io->out_prevCHAR.c_mcharset != M_US_ASCII ){
			for( cp = ENCODE_DONE; *cp; cp++ )
				EN_FPUTC0(*cp,io);

			if( !DELIMITER(ch) ){
				EN_FPUTC0(SPACE,io);
				io->SPACE_WAS_GEN = 1;
			}
		}

		/* AT THE BEGINNING OF A ENCODED WORD */
		if( charset != M_US_ASCII ){
			int reqlen,remlen;

			if( !DELIMITER(io->out_prevCHAR.c_ch) )
				EN_FPUTC0(SPACE,io);

			reqlen = ew_overhead(charset,encoding);
			remlen = MAXCOL - (io->out_column + reqlen);

			if( (remlen-SWCODE_LENG) < MIN_ENCODEDLEN ){
				EN_FPUTC0(NL,io);
				EN_FPUTC0(FOLD_CHAR,io);
			}
			sprintf(line,"=?%s?%s?",charset,encoding);
			for( cp = line; *cp; cp++ )
				EN_FPUTC0(*cp,io);
			io->MIME_ENCODED = 1;
		}
	}

	if( ch != EOF ){
		if( ch != NL ){
			/* split at LWSP_CHAR ... */
			if( !encoding )
			if( io->MIME_ENCODED )
			if( MAXCOL <= io->out_column )
			if( LWSP_CHAR(ch) )
				EN_FPUTC0(NL,io);
		}
		EN_FPUTC0(ch,io);
	}
	io->out_prevCHAR.c_mcharset = charset;
	io->out_prevCHAR.c_ch = ch;
}

#define PENDING_LWSP out_enLWSP

static EN_FPUTC(ch,io,charset,encoding)
	INOUT *io;
	char *charset,*encoding;
{	int lwsp;

	if( (ch & 0xFF) == ch )
		io->out_lastputch = ch;

	if( ch == XC_EN_FOLD ){
		if( lwsp = io->PENDING_LWSP )
			io->PENDING_LWSP = 0;
		else{
			lwsp = SPACE;
			io->SPACE_WAS_GEN = 1;
		}
		EN_FPUTC1(NL,io,M_US_ASCII,ENCODE_NONE);
		EN_FPUTC1(lwsp,io,M_US_ASCII,ENCODE_NONE);
	}else{
		if( lwsp = io->PENDING_LWSP ){
			EN_FPUTC1(lwsp,io,M_US_ASCII,ENCODE_NONE);
			io->PENDING_LWSP = 0;
		}
		if(LWSP_CHAR(ch)&& charset==M_US_ASCII&& encoding==ENCODE_NONE)
			io->PENDING_LWSP = ch;
		else	EN_FPUTC1(ch,io,charset,encoding);
	}
}

/*
 *	PASS THROUGH AN ASCII WORD
 */
static noencode_word(io)
	INOUT *io;
{	CHAR *CH,*NCH;
	int ch,inx;
	int canbe_folded;
	MsgLine line;

	canbe_folded = io->MIME_ENCODED;
	for(inx = 0; inx <= MAXCOL; inx++){
		CH = EN_FGETC(io);
		ch = CH->c_ch;

		if( io->in_mcharset != M_US_ASCII ){
			EN_UNGETC(CH,io);
			break;
		}
		if( ch == EOF )
			break;
		if( ch == NL ){
			line[inx++] = NL;
			NCH = EN_FGETC(io);
			switch( NCH->c_ch ){
			    case NL:  ch = NLNL; break;
			    case EOF: break;
			    default: EN_UNGETC(NCH,io); break;
			}
			break;
		}
/*
if( canbe_folded )
if( DELIMITER(ch) )
*/
/* might be harmful for tools don't treat unfolding properly */
		if( LWSP_CHAR(ch) )
		{
			line[inx++] = ch;
			break;
		}
		line[inx] = ch;
	}
	line[inx] = 0;

	if( line[0] != NL )
	if( canbe_folded )/* safety for non-MIMEr like inews/Cnews */
	if( MAXCOL+2 < io->out_column+inx )
		EN_FPUTC(XC_EN_FOLD,io,M_US_ASCII,ENCODE_NONE);

	{	int ch,ci;
		for( ci = 0; ch = line[ci]; ci++ )
			EN_FPUTC(ch,io,M_US_ASCII,ENCODE_NONE);
	}
	return ch;
}
static encode_one(encoding,ins,ilen,outs,osize)
	char *encoding,*ins,*outs;
{	int len;

	if( lcl_strcasecmp(encoding,ENCODE_QP) == 0 )
		len = str_toqp(ins,ilen,outs,osize);
	else
	if( lcl_strcasecmp(encoding,ENCODE_BASE64) == 0 )
		len = str_to64(ins,ilen,outs,osize);
	else{
		strncpy(outs,ins,ilen);
		len = ilen;
	}
	outs[len] = 0;
	return len;
}

static encode_word(io)
	INOUT *io;
{	char *charset;	/* charset of this encoded-word */
	char *encoding;	/* encoding of this encoded-word */
	MsgLine ins,outs;
	int inx,outx,prefold;
	int char_bytes,nchar,reqlen,remlen,outlen;
	char ch,encoded_ch;
	int prech = 0;
	int postch = 0;
	int delop = 0;
	CHAR *CH;

	charset = io->in_mcharset;
	char_bytes = 2;
	encoding = io->in_encoding;
	reqlen = ew_overhead(charset,encoding);

	/*
	 *	firstly, add the code switch sequence in a encoded format
	 */
	strcpy(ins,in_CODESW_SEQ(io));
	inx = strlen(ins);

	if( io->ENCODE_EXT ){
		strcat(ins,in_CODESW_SEQ(io));
		outlen = encode_one(encoding,ins,strlen(ins),outs,sizeof(outs));
	}else	outlen = encode_one(encoding,ins,inx,outs,sizeof(outs));

	/*
	 *	if remaining length is not enough, fold the line
	 */
	remlen = MAXCOL - (io->out_column + reqlen);
	if( (remlen-outlen) <= MIN_ENCODEDLEN ){
		remlen = MAXCOL - (1 + reqlen);
		prefold = 1;
	}else	prefold = 0;

	/*
	 *	scan a word to be encoded expanding byte by byte.
	 *	every encoded-texts end with the switch to M_US_ASCII
	 */
	for(nchar = 0; ;nchar++){
		strcpy(&ins[inx],GOTO_ASCII_SEQ);
		outlen = encode_one(encoding,ins,inx+GOTO_ASCII_SEQ_LEN,
			 outs,sizeof(outs));

		CH = EN_FGETC(io);
		ch = CH->c_ch;
		if( ch == EOF || ch == NL || CH->c_mcharset != charset ){
			if( io->in_mcharset == M_US_ASCII )
				strcpy(&ins[inx],in_CODESW_SEQ(io));
				/* ASCII family like JIS-X0201-Roman */


			/* ENCODE A LWSP BETWEEN ENCODED-WORDS */
			/*if( 4 <= (remlen-outlen) )*/
			if( LWSP_CHAR(ch) ){
				CHAR *NCH;

				NCH = EN_FGETC(io);
				if( NCH->c_mcharset == charset ){
					EN_UNGETC(NCH,io);
					inx += strlen(&ins[inx]);
					ins[inx++] = ch;
					ins[inx] = 0;
					if( 12 <= (remlen-outlen) ){
/* TO BE CATENATED */
strcpy(&ins[inx],CODESW_SEQ(NCH->c_bcharset));
inx = strlen(ins);
continue;
					}
/* TO BE SPLITTED */
postch = ch;
break;
				}
				EN_UNGETC(NCH,io);
				EN_UNGETC(CH,io);
				break;
			}
			if( ch != EOF )
				EN_UNGETC(CH,io);

			postch = ch;
			break;
		}
		if( nchar % char_bytes == 0 && remlen <= outlen ){
			EN_UNGETC(CH,io);
			break;
		}

		ins[inx++] = ch;
		ins[inx] = 0;
	}
	inx += strlen(&ins[inx]);

	if( nchar == 0 )
		return ch;

	if( prefold )
		EN_FPUTC(XC_EN_FOLD,io,M_US_ASCII,ENCODE_NONE);

	/*
	 *	output the scanned word
	 */
	if( io->ENCODE_EXT ){ /* external space encoding for ISO-2022-JP */
		delop = 0;
		prech = io->out_lastputch;

		/* if pre-SPACE will be inserted... X-), or was inserted */
		if( !IS_PRE_DELIMITER(prech) || io->SPACE_WAS_GEN && !prefold )
			delop |= DELSP_PRE;
		io->SPACE_WAS_GEN = 0;

		/* if post SPACE will be inserted... */
		if( !IS_POST_DELIMITER(postch) && postch != 0 )
			delop |= DELSP_POST;

		if( delop ){
			MsgLine tmp;

			strcpy(tmp,ins);
			strcpy(ins,DELSP_SEQ(delop));
			strcat(ins,tmp);
			inx = strlen(ins);
		}
	}

	outlen = encode_one(encoding,ins,inx,outs,sizeof(outs));
	for(outx = 0; outx < outlen; outx++){
		encoded_ch = outs[outx];
		if( encoded_ch == NL )
			continue;
		EN_FPUTC(encoded_ch,io,charset,encoding);
	}
	return ch;
}
/* it may be desirable to fold before an encoded-word, which length is
 *  shorter than MAXCOL, but will be splitted in current line.  */


MIME_headerEncode0(in,out)
	FILE *in,*out;
{	char *ip,*op;
	INOUT iob,*io = &iob;
	CHAR *CH;
	int ch;

	INOUT_init(io,in,out);
	for(;;){
		CH = EN_FGETC(io);
		EN_UNGETC(CH,io);

		ch = CH->c_ch;
		if( CH->c_mcharset == M_US_ASCII ){
			ch = noencode_word(io);
			if( ch == EOF )
				break;
			if( ch == NLNL )
				break;
		}else{
			for(;;){
				ch = encode_word(io);
				if( io->in_mcharset == M_US_ASCII )
					break;
#ifdef AAA
				/* temporary */
				if( ch == EOF )
					break;
#endif
			}
			if( ch == EOF )
				break;
		}
	}
	if( ch == EOF )
		EN_FPUTC(ch,io,M_US_ASCII,ENCODE_NONE);
	return ch;
}
MIME_headerEncode(in,out)
	FILE *in,*out;
{	int ch;
	MsgLine line;

	ch = MIME_headerEncode0(in,out);
	if( ch != EOF ){
		NLfputc(NL,out);
		while( fgets(line,sizeof(line),in) != NULL )
			fputs(line,out);
	}
}


/*
 *	FINAL OUTPUT WITH ISO-2022 CHARACTER SET SWITCH SEQUENCE
 */
static DE_FPUTC1X(CH,Out)
	CHAR *CH;
	INOUT *Out;
{	FILE *out;
	int cset;
	int ch;

	out = Out->out_file;
	ch = CH->c_ch;
	cset = CH->c_bcharset;

	if( cset != Out->out_prev_bcharset ){
		fputs(CODESW_SEQ(cset),out);
		Out->out_prev_bcharset = cset;
	}
	Out->EWORD_WAS_PUT = 0;

	switch( ch ){
		case EOF:			return(0);
		case XC_DE_OFLUSH:		return(0);
		case NL: Out->out_column = 0;	break;
		default: Out->out_column++;	break;
	}
	NLfputc(ch,out);
}

/*
 *	PUT ASCII (or CONTROL) CHARACTER IN A CURRENT ASCII FAMILY
 */
static DE_FPUTC1(ch,io)
	INOUT *io;
{	CHAR CH;

	CH.c_bcharset = io->out_whichASCII;
	CH.c_ch = ch;
	DE_FPUTC1X(&CH,io);
}

/*
 *	PUT CHARACTER CONTROLLING "LWSP" AND UNFOLDING
 */
#define CLEAR_LWSP(io)	(io->out_deLWSP[0].c_ch = 0)

static DE_FPUTCX(CH,io)
	CHAR *CH;
	INOUT *io;
{	int ch;
	CHAR PCH;

	if( io == 0 )
		return(0);

	ch = CH->c_ch;

	if( ch == XC_DE_DEL_LWSP ){
		CLEAR_LWSP(io);
		return(0);
	}
	if( ch == XC_DE_IGN_LWSP ){
		io->IGN_POST_SPACE = 1;
		return(0);
	}
	if( io->IGN_POST_SPACE ){
		if( LWSP_CHAR(ch) )
			return(0);

		if( (ch & 0xFF) == ch )
			io->IGN_POST_SPACE = 0;
	}
	if( ch == XC_DE_EWORD_WAS_PUT ){
		io->EWORD_WAS_PUT = 1;
		return(0);
	}
	if( ch == XC_DE_CATENATE ){
		/* REMOVE PENDING SPACE IF EXISTS */
		if( ! io->EWORD_WAS_PUT ){
			PCH = io->out_deLWSP[0];
			if( PCH.c_ch == NL ){
				/* discard the NEWLINE */
				PCH = io->out_deLWSP[1];
			}
			if( PCH.c_ch )
				DE_FPUTC1X(&PCH,io);
		}
		CLEAR_LWSP(io);
		return(0);
	}
	if( ch == XC_DE_UNFOLD ){
		if( io->out_deLWSP[0].c_ch == NL ){
			PCH = io->out_deLWSP[1];
			if( PCH.c_ch ){
				DE_FPUTC1X(&PCH,io);
				CLEAR_LWSP(io);
			}
		}
		return(0);
	}
	if( ch == XC_DE_FORMFEED ){
		PCH = io->out_deLWSP[0];
		if( PCH.c_ch ){
			DE_FPUTC1X(&PCH,io);
			PCH = io->out_deLWSP[1];
			if( PCH.c_ch )
				DE_FPUTC1X(&PCH,io);
			CLEAR_LWSP(io);
		}
		DE_FPUTC1(FORMFEED,io);
		DE_FPUTC1(NL,io);
		fflush(io->out_file);
		return(0);
	}

	/* FLUSH LWSP */
	PCH = io->out_deLWSP[0];
	if( PCH.c_ch ){
		if( PCH.c_ch == NL ){
			if( LWSP_CHAR(ch) ){ /* linear-white-space */
				io->out_deLWSP[1] = *CH;
				io->out_deLWSP[2].c_ch = 0;
				return(0);
			}
			DE_FPUTC1(NL,io);
			PCH = io->out_deLWSP[1];
			if( PCH.c_ch )
				DE_FPUTC1X(&PCH,io);
		}else	DE_FPUTC1X(&PCH,io);
		CLEAR_LWSP(io);
	}

	/* ENBUFFER LWSP */
	if( io->UNFOLD_LINE ){
/*
		if( ch == NL || io->EWORD_WAS_PUT && LWSP_CHAR(ch)){
*/
		if( ch == NL || LWSP_CHAR(ch)){
			io->out_deLWSP[0] = *CH;
			io->out_deLWSP[1].c_ch = 0;
			return(0);
		}
	}
	DE_FPUTC1X(CH,io);
}

/*
 *	PUT ASCII (or CONTROL) CHARACTER IN A CURRENT ASCII FAMILY
 */
static DE_FPUTC(ch,io)
	INOUT *io;
{	CHAR CH;

	CH.c_bcharset = io->out_whichASCII;
	CH.c_ch = ch;
	DE_FPUTCX(&CH,io);
}


static scan_eword(in,reads,charset,encoding,text)
	FILE *in;
	char *reads,*charset,*encoding,*text;
{	int i,cs;

	for(i = 0; ;i++){
		cs = NLfgetc(in);
		if(cs==NL || cs==EOF) goto error;
		*reads++ = cs;
		if(cs==CHARSET_DONE) break;
		charset[i] = cs;
		charset[i+1] = 0;
	}
	for(i = 0; ;i++){
		cs = NLfgetc(in);
		if(cs==NL || cs==EOF) goto error;
		*reads++ = cs;
		if(cs==ENCODING_DONE) break;
		encoding[i] = cs;
		encoding[i+1] = 0;
	}
	for(i = 0; i < 80; i++ ){
		cs = NLfgetc(in);
		if(cs==NL || cs==EOF) goto error;
		*reads++ = cs;
		if(cs == ENCODE_DONE[0]){
			cs = NLfgetc(in);
			if(cs==NL || cs==EOF) goto error;
			*reads++ = cs;
			if( cs == ENCODE_DONE[1] ){
				text[i] = 0;
				break;
			}
			ungetc(cs,in);
			cs = ENCODE_DONE[0];
		}
		text[i] = cs;
		text[i+1] = 0;
	}
	return 0;
error:
	*reads = 0;
	return cs;
}

static disp_word(Out,dtext,len)
	INOUT *Out;
	char *dtext;
{	FILE *DecodedText;
	INOUT tmpInb,*tmpIn = &tmpInb;
	int dch;
	int sdlen,dlen;
	int eat_space = 0;
	CHAR *CH;

	if( len <= 0 )
		return 0;

	if( Out )
		sdlen = disp_word(0,dtext,len);

	DecodedText = str_fopen(dtext,len);
	INOUT_init(tmpIn,DecodedText,NULL);

	dlen = 0;
	for(;;){
		CH = EN_FGETC(tmpIn);
		if( (dch = CH->c_ch) == EOF )
			break;

		if( Out && dlen == 0 ){
			if( Out->ENCODE_EXT )
				eat_space = tmpIn->EAT_SPACE;
			else	eat_space = 0;

			if( eat_space & DELSP_PRE ){
				DE_FPUTC(XC_DE_DEL_LWSP,Out);
				DEBUG(DE_FPUTC('{',Out));
			}else{
				if( Out->out_column + sdlen < DISPCOLS )
					DE_FPUTC(XC_DE_CATENATE,Out);
				else	DE_FPUTC(XC_DE_OFLUSH,Out);
			}
		}
		if( Out )
			DE_FPUTCX(CH,Out);
		dlen++;
	}
	str_fclose(DecodedText);

	if(Out){
		DE_FPUTC(XC_DE_EWORD_WAS_PUT,Out);
		Out->MIME_ENCODED = 1;
		Out->out_whichASCII = CH->c_bcharset; /* CH == EOF */

		if( eat_space & DELSP_POST ){
			DEBUG(DE_FPUTC(XC_DE_OFLUSH,Out));
			DEBUG(DE_FPUTC('}',Out));
			DE_FPUTC(XC_DE_IGN_LWSP,Out);
		}
	}
	return dlen;
}

/* 2.0 add ret_charset parameter */
static decode_word(io, ret_charset)
	INOUT *io;
	char **ret_charset;
{	MsgLine reads,charset,encoding,itext,dtext;
	int ilen,dsize,len,pad,dlen;
	int eow;

	*charset = *encoding = *itext = 0;
	eow = scan_eword(io->in_file,reads,charset,encoding,itext);

	if(charset[0]){
		if(*ret_charset)
			free(*ret_charset);
		*ret_charset = (char *)malloc(strlen(charset) + 1);
		if(*ret_charset)
			strcpy(*ret_charset, charset);
	}

	if( eow == NL || eow == EOF ){
		DE_FPUTC(XC_DE_OFLUSH,io);
		fprintf(io->out_file,"=?%s",reads);
		if( eow != EOF )
			ungetc(eow,io->in_file);
		return eow;
	}

/*
	if( !MIME_localCharset(charset) ){
		DE_FPUTC(XC_DE_OFLUSH,io);
		fprintf(io->out_file,"=?%s?%s?%s?=",charset,encoding,itext);
		if( eow )
			fprintf(io->out_file,"%c",eow);
		return 0;
	}
*/

	ilen = strlen(itext);
	dsize = sizeof(dtext);
	if( lcl_strcasecmp(encoding,ENCODE_QP) == 0 )
		len = str_fromqp(itext,ilen,dtext,dsize);
	else
	if( lcl_strcasecmp(encoding,ENCODE_BASE64) == 0 )
		len = str_from64(itext,ilen,dtext,dsize);
	else{
		strcpy(dtext,itext);
		len = ilen;
	}
	disp_word(io,dtext,len);
	return 0;
}


static nodecode_word(io,ch)
	INOUT *io;
{
	if( io->MIME_ENCODED ){
		/* if the next noencoded-word ends before DISPCOLS ...*/
		if( io->out_column < MAXCOL )
			DE_FPUTC(XC_DE_UNFOLD,io);
		else{
			/* the following is experimental */
			if( LWSP_CHAR(ch) ){
				DE_FPUTC(XC_DE_OFLUSH,io);
				if( MAXCOL <= io->out_column ){
					DE_FPUTC(NL,io);

/* this ch shuld be put if the next character is not LWSP_CHAR... (?) */
	DE_FPUTC(ch,io);
	return(0);
				}
			}
		}
	}
	DE_FPUTC(ch,io);
}

static DE_FGETC(io)
	INOUT *io;
{	FILE *in;
	int ch;

	in = io->in_file;
	ch = NLfgetc(in);
	if( ch == FORMFEED ){
		ch = NLfgetc(in);
		if( ch == NL )
			ch = XC_DE_FORMFEED;
		else{
			if( ch != EOF )
				ungetc(ch,in);
			ch = FORMFEED;
		}
		io->MIME_ENCODED = 0;
	}else
	if( ch == NL ){
		ch = NLfgetc(in);
		if( !LWSP_CHAR(ch) )/* at the top of a filed */
			io->MIME_ENCODED = 0;

		if( ch == NL )
			ch = NLNL;
		else{
			if( ch != EOF )
				ungetc(ch,in);
			ch = NL;
		}
	}
	return ch;
}
/* 2.0 add charset parameter */
/*     type bodytoo to int   */
MIME_headerDecode(in,out,bodytoo, charset)
	FILE *in,*out;
	int	bodytoo;
	char **charset;
{	int ch,next_ch;
	INOUT iob,*io = &iob;

	INOUT_init(io,in,out);
	io->UNFOLD_LINE = 1;

	*charset = (char *)NULL;

	for(;;){
		ch = DE_FGETC(io);
		if( ch == EOF )
			break;

		if( ch == ENCODE_BEGIN[0] ){
			ch = NLfgetc(in);
			if( ch == EOF )
				break;
			if( ch == ENCODE_BEGIN[1] ){
				if( decode_word(io, charset) == EOF )
					break;
			}else{
				DE_FPUTC(ENCODE_BEGIN[0],io);
				ungetc(ch,in);
			}
		}else{
			if( ch == NLNL ){
				io->UNFOLD_LINE = 0;
				DE_FPUTC(NL,io);
				DE_FPUTC(NL,io);
				break;
			}
			nodecode_word(io,ch);
		}
	}
	io->UNFOLD_LINE = 0;
	if( ch != EOF && bodytoo )
		while( (ch = NLfgetc(in)) != EOF )
			DE_FPUTC(ch,io);
	DE_FPUTC(EOF,io);
}

/* 2.0 add charset parameter */
/*     type osize to int     */
MIME_strHeaderDecode(ins,outs,osize,charset)
	char *ins,*outs;
	int  osize;
	char **charset;
{	FILE *In,*Out;
	int oi;

	In = str_fopen(ins,strlen(ins));
	Out = str_fopen(outs,osize);
	MIME_headerDecode(In,Out,1,charset);
	fflush(Out);
	for(oi = 0; outs[oi]; oi++)
		if((outs[oi] & 0xFF) == 0xFF)
			strcpy(&outs[oi],&outs[oi+1]);
	str_fclose(In);
	str_fclose(Out);
}
MIME_strHeaderEncode(ins,outs,osize)
	char *ins,*outs;
{	FILE *In,*Out;

	In = str_fopen(ins,strlen(ins));
	Out = str_fopen(outs,osize);
	MIME_headerEncode(In,Out);
	fflush(Out);
	str_fclose(In);
	str_fclose(Out);
}

is_MIME_header(fp)
	FILE *fp;
{	MsgLine line;
	int off;

	off = ftell(fp);
	while( fgets(line,sizeof(line),fp) != NULL ){
		if( *line == NL )
			break;
		if( *line == CR && line[1] == NL )
			break;

		if( strstr(line,ENCODE_BEGIN) ){
			fseek(fp,off,0);
			return 1;
		}
	}
	fseek(fp,off,0);
	return 0;
}

/* 2.0 not used function
FILE *
MIME_tmpHeaderDecode(fp,bodytoo)
	FILE *fp;
{	FILE *tfp;

	if( fp == NULL )
		return NULL;

	if( fseek(fp,0,1) == 0 ){
		if( !is_MIME_header(fp) )
			return NULL;
	}

	tfp = tmpfile();
	MIME_headerDecode(fp,tfp,bodytoo);
	fflush(tfp);
	fseek(tfp,0,0);
	return tfp;
}
*/
FILE *
MIME_tmpHeaderEncode(fp,savFILE)
	FILE *fp,savFILE;
{	FILE *tin,*tfp;
	MsgLine line;
	int ch;

	if( fp == NULL )
		return(0);
	tin = tmpfile();
	while( fgets(line,sizeof(line),fp) != NULL ){
		fputs(line,tin);
		if(strcmp(line,".\n")==0 || strcmp(line,".\r\n")==0)
			break;
	}
	fflush(tin);
	fseek(tin,0,0);

	tfp = tmpfile();
	ch = MIME_headerEncode0(tin,tfp);
	if( ch == NLNL ){
		fputs("\r\n",tfp);
		while( fgets(line,sizeof(line),tin) != NULL )
			fputs(line,tfp);
	}
	fputs(".\r\n",tfp);
	fflush(tfp);
	fseek(tfp,0,0);

	fclose(tin);
	return tfp;
}

/*//////////////////////////////////////////////////////////////////////*/
MIME_localStrColumns(str)
	char *str;
{	INOUT iob,*io = &iob;
	FILE *sfp;
	int len;
	CHAR *CH;

	sfp = str_fopen(str,strlen(str));
	INOUT_init(io,sfp,NULL);

	len = 0;

	for(;;){
		CH = EN_FGETC(io);
		if( CH->c_ch == EOF ) 
			break;
		len++;
	}

	str_fclose(sfp);
	return len;
}
