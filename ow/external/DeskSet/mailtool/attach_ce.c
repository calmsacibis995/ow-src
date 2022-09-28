#ifndef lint
static 	char sccsid[] = "@(#)attach_ce.c 3.8 97/02/10 Copyr 1990 Sun Micro";
#endif

/*
 * Copyright (c) 1990 by Sun Microsystems, Inc.
 */

#ifdef SVR4
#include <string.h>
#else
#include <strings.h>
#endif SVR4
#include <sys/stat.h>
#include <sys/param.h>
#include <fcntl.h>

#include <xview/xview.h>
#include <xview/canvas.h>
#include <xview/panel.h>
#include <xview/font.h>
#include <xview/svrimage.h>

#include <desktop/ce.h>

#include "attach.h"
#include "../maillib/obj.h"


#define DEBUG_FLAG mt_debugging
extern int DEBUG_FLAG;
#include "debug.h"

/* TRUE if mode describes a file which is executable by somebody */
#define MT_ISEXEC(mode)	((mode & (S_IRWXU | S_IRWXG | S_IRWXO)) & \
			 (S_IXUSR | S_IXGRP | S_IXOTH))

/* Default icon images */
unsigned short	document_image[] = {
#include "document.icon"
};

unsigned short	application_image[] = {
#include "application.icon"
};

static Server_image	document_icon;
static Server_image	application_icon;

extern	char	*ce_get_attribute();

Server_image get_icon_image_from_file(char *icon_file);


/* 
 * Magic tokens required for calls to the classing engine
 */
static CE_NAMESPACE File_Namespace;
static CE_NAMESPACE Type_Namespace;
static CE_ATTRIBUTE Type_Icon;
static CE_ATTRIBUTE Type_Open;
static CE_ATTRIBUTE Type_BGColor;
static CE_ATTRIBUTE File_Type;
static CE_ATTRIBUTE Type_Open_Tt;
static CE_ATTRIBUTE Type_Media;
static CE_ATTRIBUTE Type_Executable;
static CE_ATTRIBUTE Type_AttachmentName;

/* Attributes from the Compression name space */
static CE_ATTRIBUTE Type_Compression;
static CE_ATTRIBUTE Type_Encode;
static CE_ATTRIBUTE Type_Decode;
static CE_ATTRIBUTE Type_Minsize;


int
mt_init_ce(void)

{
	/*
	 * Initialize stuff we need to use with the Classing Engine
	 */

	if(ce_begin(0)) return(-1);

	/* turn strings into magic attribute tokens */
	File_Namespace = ce_get_namespace_id("Files");
	Type_Namespace = ce_get_namespace_id("Types");

	if (!File_Namespace || !Type_Namespace) {
		return(-1);
	}

	Type_Icon    = ce_get_attribute_id(Type_Namespace, "TYPE_ICON");
	Type_Open    = ce_get_attribute_id(Type_Namespace, "TYPE_OPEN");
	Type_BGColor = ce_get_attribute_id(Type_Namespace, "TYPE_BGCOLOR");
	Type_Open_Tt = ce_get_attribute_id(Type_Namespace, "TYPE_OPEN_TT");
	Type_Media   = ce_get_attribute_id(Type_Namespace, "TYPE_MEDIA");
	Type_Compression =
		ce_get_attribute_id(Type_Namespace, "TYPE_COMPRESSION_METHOD");
	Type_Executable =
		ce_get_attribute_id(Type_Namespace, "TYPE_EXECUTABLE");

	File_Type    = ce_get_attribute_id(File_Namespace, "FNS_TYPE");

	Type_Encode = ce_get_attribute_id(Type_Namespace, "TYPE_ENCODE");
	Type_Decode = ce_get_attribute_id(Type_Namespace, "TYPE_DECODE");
	Type_Minsize = ce_get_attribute_id(Type_Namespace, "TYPE_MINSIZE");
	Type_AttachmentName = ce_get_attribute_id(Type_Namespace,
					"TYPE_FILE_TEMPLATE");

	return(0);
}



/* mt_get_encode() and mt_get_decode() are needed since we don't want libmail
 * to know about the classing engine.  When it needs the encode/decode strings
 * from the classing engine it calls us
 */
char *
mt_get_encode(
	char	*compression_token,
	struct attach *at
)
{
	char	*encode_str, *minsize_str;
	int	n = 0;
	CE_ENTRY	entry;
	
	DP(("mt_get_encode: called for %s\n", compression_token));

	entry = ce_get_entry(Type_Namespace, 1, compression_token);

	if (entry == NULL)
		return(NULL);

	encode_str = ce_get_attribute(Type_Namespace, entry, Type_Encode);
	minsize_str = ce_get_attribute(Type_Namespace, entry, Type_Minsize);

	if (minsize_str != NULL) {
		n = atoi(minsize_str);
	}

	/* If there is no encoding method or the attachment is smaller than the
	 * minimum size to encode then we don't encode
	 */
	if (encode_str == NULL ||
	    (int)attach_methods.at_get(at, ATTACH_CONTENT_LEN) < n)
		encode_str = "";

	DP(("mt_get_encode: returning %s\n", encode_str));
	return(encode_str);

}




char *
mt_get_decode(
	char	*compression_token,
	struct attach *at
)
{
	char	*decode_str;
	int	n = 0;
	CE_ENTRY	entry;
	
	DP(("mt_get_decode: called for %s\n", compression_token));

	entry = ce_get_entry(Type_Namespace, 1, compression_token);

	if (entry == NULL)
		return(NULL);

	decode_str = ce_get_attribute(Type_Namespace, entry,
				  Type_Decode);
	if (decode_str == NULL)
		decode_str = "";

	DP(("mt_get_decode: returning %s\n", decode_str));

	return(decode_str);
}





char *
mt_get_data_type(
	char *label,
	void *buf,
	int bufsize
)
{
	CE_ENTRY	entry;	/* CE entry for file */
	char		*type;

	/* set up the default return value */
	type = MT_CE_DEFAULT_TYPE;

	/* Get the CE entry for the file */
	entry = NULL;
	if (File_Namespace != NULL) {
		entry = ce_get_entry(File_Namespace, 3, label, buf, bufsize);

		if (entry) {

			/* Find out what type of file we have */
			type = ce_get_attribute(File_Namespace, entry,
				File_Type);

			if (type == NULL) {
				type = MT_CE_DEFAULT_TYPE;
			}
		}
	}

	return (type);
}




char *
mt_get_compression_type(
	char *type
)
{
	CE_ENTRY	entry;	/* CE entry for type */
	char		*compression;

	compression = NULL;

	/* Get the CE entry for the file */
	entry = NULL;
	if (Type_Namespace != NULL) {
		entry = ce_get_entry(Type_Namespace, 1, type);

		if (entry) {

			/* Find out the compression method for the type */
			compression = ce_get_attribute(Type_Namespace, entry,
				Type_Compression);
		}
	}

	return (compression);
}




char *
mt_get_attachment_name(
	char *type
)
{
	CE_ENTRY	entry;	/* CE entry for type */
	char		*name;

	name = NULL;

	/* Get the CE entry for the file */
	entry = NULL;
	if (Type_Namespace != NULL) {
		entry = ce_get_entry(Type_Namespace, 1, type);

		if (entry) {

			/* Find out the file template for the type */
			name = ce_get_attribute(Type_Namespace, entry,
				Type_AttachmentName);
		}
	}

	return (name);
}




struct attach *
mt_create_at(
	u_char	*file,
	u_char	*label,
	int	tmpfile		/* TRUE if file is a temporary file */
)
{
	u_char	buf[64];	/* First n bytes of file */
	int	bufsize;	/* how many chars we actually got */
	int	fd;
	struct attach	*at;
	char	*type;
	FILE	*fp;
	int	rcode;

	/* 
	 * Create an attachment object used with back end
	 */

	/*
	 * If no label is specified use last component of file path
	 */
	if (label != NULL) {
		while (isspace (*label))
			label++;
		if (*label == '\0')
			label = NULL;
	}
	if (label == NULL) {
		if ((label = (u_char *) strrchr((char *)file, '/')) == NULL)
			label = file;
		else
			label++;
	}

	/*  Get first part of file.  */
	bufsize = 0;
	if ((fd = open((char *)file, O_RDONLY)) >= 0) {
		bufsize = read(fd, buf, sizeof(buf));
		if (bufsize < -1)
			bufsize = 0;
		close(fd);
	} else {
		return(NULL);
	}

	type = mt_get_data_type((char *)file, buf, bufsize);
	
	/* Create the attachment handle */
	if ((at = attach_methods.at_create()) == NULL)
		return(NULL);

	attach_methods.at_set(at, ATTACH_DATA_TYPE, type);
	attach_methods.at_set(at, ATTACH_DATA_DESC, type);
	attach_methods.at_set(at, ATTACH_DATA_NAME, label);

	attach_methods.at_set(at, ATTACH_ENCODE_INFO,
				mt_get_compression_type(type));

	/*
	 * ATTACH_DATA_TMP_FILE causes the file to be removed
	 * when the at is destroyed.
	 */
	if (tmpfile)
		rcode = attach_methods.at_set(at, ATTACH_DATA_TMP_FILE, file);
	else
		rcode = attach_methods.at_set(at, ATTACH_DATA_FILE, file);

	if (rcode < 0) {
		/* at_set failed */
		attach_methods.at_destroy(at);
		at = NULL;
	}

	return(at);
}

/* defined in attach_canvas.c */
extern int mt_check_for_nulls(char *string, int count);

Attach_node *
mt_init_attach_node(
	Attach_list	*list,
	Attach_node	*node,
	struct attach	*at,
	struct stat	*statb
)
{
	Font_string_dims	font_size;
	CE_ENTRY	entry;	/* CE entry for file */
	char	*type;
	char	*label;
	char	*p;
	Server_image	icon;
	int	executable;
	char *def_type = "default";
	char *dtype = "";

	executable = (int) attach_methods.at_get(at, ATTACH_EXECUTABLE);
	type = (char *) attach_methods.at_get(at, ATTACH_DATA_TYPE);
	label = mt_attach_name(at);

	node->an_at = at;

	if (statb) {
		executable = ((S_ISREG(statb->st_mode) ||
				S_ISLNK(statb->st_mode)) &&
				MT_ISEXEC(statb->st_mode));
	} 

	/* Default icons in case CE calls fail */
	if (executable) {
		node->an_icon = application_icon;

		if (!strcmp(type, MT_CE_DEFAULT_TYPE)) {
			/* change the default name to an "executable" */
			attach_methods.at_set(at, ATTACH_DATA_TYPE,
				MT_CE_APPLICATION_TYPE);
			type = attach_methods.at_get(at, ATTACH_DATA_TYPE);
		}
	} else {
		node->an_icon = document_icon;
	}

	node->an_label_length = (label != NULL) ? strlen(label) : 0;

	if (list) {
		xv_get(list->al_font, FONT_STRING_DIMS, label, &font_size);
		node->an_label_width  = font_size.width;
	}

	/* Get the entry for the type of file we have */
	if (Type_Namespace == NULL)
		return(node);

	if ((entry = ce_get_entry(Type_Namespace, 1, type)) == NULL)
		return(node);

	/* Get the application and icon for the type of file we have */
	if ((p = ce_get_attribute(Type_Namespace, entry, Type_Open)) != NULL)
	{
		node->an_application = (char *)strdup(p);
	}

	/* Get the TT open method */
	if ((p = ce_get_attribute(Type_Namespace, entry, Type_Open_Tt))
		!= NULL)
	{
		node->an_open_tt = (char *)strdup(p);
	}

	/* Get the TT Alliance Media Type */
	
	/*
	**	BIG PROBLEM
	**
	**	If the classing engine doesn't recognise a file
	**	it assumes that it's a text file. But what if it isn't?
	**	What if it contains a NULL? Well, the text gets corrupted
	**	the first time you call strcpy, that's what.
	**
	**	Solution: If we find a NULL before the end of the string,
	**	but the type string says it's 'default' then give it a null type.
	*/
	

	if (at->at_dtype)
		dtype = at->at_dtype;
	else if (at->at_type)
		dtype = at->at_type;

	p = ce_get_attribute(Type_Namespace, entry, Type_Media);
	if ((at->at_buffer != NULL) && (strcmp(dtype, def_type) == 0)) {
 		if (at->at_buffer->bm_buffer == NULL)
			p = NULL;
		else if (mt_check_for_nulls(at->at_buffer->bm_buffer, at->at_buffer->bm_size))
			p = NULL;
	}
	node->an_tt_media = (p) ? (char *)strdup(p) : NULL;

	/* get the executable type */
	if ((p = ce_get_attribute(Type_Namespace, entry, Type_Executable))
		!= NULL)
	{
		if (! statb) {
			executable = 1;
		}
	}

	/* Generate icon image from the icon file */
	if ((p = ce_get_attribute(Type_Namespace, entry, Type_Icon)) != NULL) {
		if ((icon = get_icon_image_from_file(p)) != NULL) {
			node->an_icon = icon;
		}
	}

	attach_methods.at_set(at, ATTACH_EXECUTABLE, executable);

	return(node);
}





Server_image
get_icon_image_from_file(
	char	*icon_file
)
{
	char	fullpath[MAXPATHLEN + 1];
	char	errmsg[80];
	Server_image	image;

	/*
	 * Generate a server image from an iconedit file
	 */

	ds_expand_pathname(icon_file, fullpath);
	image = (Server_image) icon_load_svrim(fullpath, errmsg);

	return(image);
}

mt_init_default_attach_icons()

{
	/*
	 * Get the default document and executable icons
	 */
	document_icon = (Server_image)xv_create(NULL, SERVER_IMAGE,
		SERVER_IMAGE_BITS,      document_image,
		SERVER_IMAGE_DEPTH,     1,
		XV_WIDTH,               MT_ATTACH_ICON_W,
		XV_HEIGHT,              MT_ATTACH_ICON_H,
		0);

	application_icon = (Server_image)xv_create(NULL, SERVER_IMAGE,
		SERVER_IMAGE_BITS,      application_image,
		SERVER_IMAGE_DEPTH,     1,
		XV_WIDTH,               MT_ATTACH_ICON_W,
		XV_HEIGHT,              MT_ATTACH_ICON_H,
		0);
}

