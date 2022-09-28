/* Copyright (c) 1993 by Sun Microsystems, Inc. */
#ident	"@(#)format.c	1.18	93/01/15 SMI"

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/param.h>

#include "atool_i18n.h"
#include "undolist_c.h"
#include "format_panel_impl.h"

caddr_t format_decfcn(List *list, char *buf);
caddr_t format_getlabel(List *list, caddr_t dp);		      
caddr_t format_encfcn(List *list, caddr_t dp);
caddr_t format_cpyfcn(List *list, caddr_t dp);
caddr_t format_newfcn(List *list);
int 	format_delfcn(List *list, caddr_t dp);

ptr_t
FormatPanel_Init(ptr_t owner)
{
	struct format_panel_data	*dp;

	/* allocate storage for file panel data */
	if ((dp = (struct format_panel_data *)calloc(1, sizeof (*dp))) == NULL)
		return (NULL);
	dp->owner = owner;

	/* create linked list for format objects */
	dp->fmtlist = NewList(NULL, format_decfcn, format_getlabel, 
			      format_encfcn, NULL, format_cpyfcn,
			      format_delfcn, format_newfcn);
	if (dp->fmtlist == NULL) {
		(void) free((ptr_t)dp);
		return (NULL);
	}

	/* Init the format panel window */
	dp->panel = FormatPanel_INIT(owner, (ptr_t)dp);

	if (dp->panel == NULL) {
		(void) free((ptr_t)dp);
		return (NULL);
	}


	return ((ptr_t) dp);
}

FormatPanel_Readdefaults(ptr_t dp)
{
	Audio_hdr			hdr;
	char				*cp;
	char				*resval;
	struct format_list_entry	*fle;
	struct format_panel_data	*fdp = (struct format_panel_data*)dp;

	/* read format list from resource database and add 'em to list */
	if (cp = ((char*)AudPanel_Getdefault(fdp->owner, "formatList"))) {
		resval = strdup(cp);
		for (cp = strtok(resval, ";"); cp && *cp; 
		     cp = strtok(NULL, ";")) {
			if (fle = (struct format_list_entry*)
			    format_decfcn(fdp->fmtlist, cp)) {
				append_format(fdp, fle);
			}
		}
		free(resval);
	}

	if (fdp->fmtlist->num <= 0) {
		/* if nothing in the list, add "hardwired" entries to list */
		hdr.samples_per_unit = 1;
		hdr.encoding = AUDIO_ENCODING_ULAW;
		hdr.bytes_per_unit = 1;
		hdr.channels = 1;
		hdr.sample_rate = 8000;
		add_format_entry(fdp, MGET("Voice Format"), &hdr, FALSE);

		hdr.encoding = AUDIO_ENCODING_LINEAR;
		hdr.bytes_per_unit = 2;
		hdr.channels = 2;
		hdr.sample_rate = 44100;
		add_format_entry(fdp, MGET("CD Format"), &hdr, FALSE);

		hdr.sample_rate = 48000;
		add_format_entry(fdp, MGET("DAT Format"), &hdr, FALSE);
	}

}

/* Pop up the Format Data Panel */
void
FormatPanel_Show(ptr_t dp, int (*apply_proc)(), ptr_t cdata, char *banner)
{
	struct format_panel_data	*fdp;
	char				buf[BUFSIZ];
	char				*str;

	fdp = (struct format_panel_data *)dp;

	fdp->apply_proc = apply_proc;
	fdp->cdata = cdata;

	if (banner && *banner) {
		sprintf(buf, MGET("Audio Tool: %s"), banner);
		str = banner;
	} else {
		strncpy(buf, MGET("Audio Tool: Format"), BUFSIZ);
		str = MGET("Apply");
	}
	FormatPanel_SETLABEL(fdp->panel, buf);
	FormatPanel_SETAPPLY(fdp->panel, str);

	/* make sure the panel info is current */
	FormatPanel_SHOW(fdp->panel);
}

/* Pop up the Format Data Panel */
void
FormatPanel_Unshow(ptr_t dp)
{
	struct format_panel_data	*fdp;

	fdp = (struct format_panel_data *)dp;

	FormatPanel_UNSHOW(fdp->panel);
}

/* Return the Format Data Panel owner handle */
ptr_t
FormatPanel_Getowner(ptr_t dp)
{
	struct format_panel_data	*fdp;

	fdp = (struct format_panel_data *)dp;
	return ((ptr_t)fdp->owner);
}


int
FormatPanel_Setformat(ptr_t dp, Audio_hdr *hdrp)
{
	struct format_panel_data *fdp;

	fdp = (struct format_panel_data *)dp;
	load_format_from_header(fdp->panel, hdrp);

	/* store a copy - so we can do a reset */
	memcpy((char*)&(fdp->curhdr), (char*)hdrp, sizeof (Audio_hdr));
}

int
change_format_entry(
	struct format_list_entry	*fle,
	char				*label,
	Audio_hdr			*hdrp)
{
	if (fle->label) {
		if (strcmp(fle->label, label) != 0) {
			free(fle->label);
			fle->label = strdup(label);
		}
	} else {
		fle->label = strdup(label);
	}
	/* copy hdr */
	memcpy((void*)&(fle->hdr), (void*)hdrp, sizeof (Audio_hdr));
	return (TRUE);
}

/* mainly for adding "hardwired" values */
int
add_format_entry(
	struct format_panel_data	*dp,
	char				*label,
	Audio_hdr			*hdrp,
	int				readonly)
{
	struct format_list_entry	*fle;

	if ((fle = (struct format_list_entry *)calloc(1, 
			     sizeof (struct format_list_entry))) == NULL) {
		/* XXX - bail out? */
		return (FALSE);
	}
	change_format_entry(fle, label, hdrp);
	fle->readonly = readonly;
	append_format(dp, fle);
	return (TRUE);
}

caddr_t
format_decfcn(List *list, char *s)
{
	struct format_list_entry *fle;
	char buf[BUFSIZ];
	char *fmtstr;
	char *label;

	if (!(fle = (struct format_list_entry *)calloc(1,
				sizeof (struct format_list_entry)))) {
		/* XXX - should bomb out */
		return (NULL);
	}

	fle->readonly = FALSE;

	GETSFIELD(s, fle->label, buf, BUFSIZ);
	GETSFIELD(s, fmtstr, buf, BUFSIZ);

	if (fmtstr == NULL) {
		free(fle);
		return (NULL);
	}
			
	if (audio_parsehdr(fmtstr, &(fle->hdr)) != AUDIO_SUCCESS) {
		/* bad format string, this is a bogus entry */
		free(fle);
		fle = NULL;
	}
	free(fmtstr);
	return ((caddr_t)fle);
}

/* 
 * given a format list entry, encode as a string, and return the 
 * string (in a static area).
 */
caddr_t
format_encfcn(List *list, caddr_t dp)
{
	return (NULL);
}

caddr_t
format_cpyfcn(List *list, caddr_t dp)
{
	return (NULL);
}

int
format_delfcn(List *list, caddr_t dp)
{
	struct format_list_entry *fle = (struct format_list_entry*)dp;

	/* nuke this puppy */
	if (!dp) {
		return (NULL);
	}

	if (fle->label) {
		free(fle->label);
	}
	free(fle);

	return (NULL);
}

caddr_t
format_getlabel(List *list, caddr_t dp)
{
	struct format_list_entry *fle = (struct format_list_entry *)dp;

	return (fle->label);
}

caddr_t
format_newfcn(List *list)
{
	struct format_list_entry *fle;

	if ((fle = (struct format_list_entry*)calloc(1, 
			      sizeof (struct format_list_entry))) == NULL) {
		/* XXX - should bomb out ... */
		return (NULL);
	}
	/* 
	 * init the header to 8-k ulaw. insert_proc from UI code will 
	 * set it to current values
	 */
	fle->hdr.sample_rate = 8000;
	fle->hdr.encoding = AUDIO_ENCODING_ULAW;
	fle->hdr.bytes_per_unit = 1;
	fle->hdr.samples_per_unit = 1;
	fle->hdr.channels = 1;
	fle->label = strdup(MGET("NewFormat"));
	fle->readonly = FALSE;
	return ((caddr_t)fle);
}

struct format_list_entry *
find_format_hdr_entry(struct format_panel_data *fdp, Audio_hdr *hdrp)
{
	Link *lp;
	struct format_list_entry *fle;
	int i;

	for (i = 0, lp = fdp->fmtlist->head;
	     lp && (i < fdp->fmtlist->num); lp=lp->next, i++) {
		if (fle = (struct format_list_entry*)lp->dp) {
			if (audio_cmp_hdr(hdrp, &(fle->hdr)) == 0) {
				return (fle);
			}
		}
	}
	return (NULL);
}

char *
find_format_hdr(struct format_panel_data *fdp, Audio_hdr *hdrp)
{
	struct format_list_entry *fle;
	
	if (fle = find_format_hdr_entry(fdp, hdrp)) {
		return (fle->label);
	}
	return (NULL);
}

/* 
 * return type name that hdr describes (look it up in list, or
 * make up a name based on encoding info). returns string in a
 * static area. caller must copy it....
 */
char *
FormatPanel_Getformatname(ptr_t dp, Audio_hdr *hdrp)
{
	struct format_panel_data	*fdp;
	static char buf[BUFSIZ];
	char *cp;
	char ratebuf[100];	/* XXX */

	fdp = (struct format_panel_data *)dp;

	if (cp = find_format_hdr(fdp, hdrp)) {
		strncpy(buf, cp, BUFSIZ);
	} else {
		if (cp = audio_printhdr(hdrp)) {
			strncpy(buf, cp, BUFSIZ);
			free(cp);
		} else {
			return (NULL);
		}
	}
	return (buf);
}
