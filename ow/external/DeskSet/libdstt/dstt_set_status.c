/*
 *
 * dstt_set_status.c
 *
 * Copyright (c) 1990 by Sun Microsystems, Inc.
 *
 */

#ifndef lint
static char sccsid [] = "@(#)dstt_set_status.c 1.8 93/01/19 Copyr 1990 Sun Micro";
#endif

#include <desktop/tt_c.h>
#include "dstt.h"
#include "dstt_vtype.h"
#include "ds_verbose_malloc.h"

#define	TT_DESKTOP_ENOENT	1538
#define	TT_DESKTOP_EINVAL	1558
#define	TT_DESKTOP_CANCELED	1698
#define	TT_MEDIA_ERR_SIZE	1700
#define	TT_MEDIA_ERR_FORMAT	1701
#define	TT_MEDIA_NO_CONTENTS	1702
#define	TT_DESKTOP_ENOMSG	1571
#define	TT_DESKTOP_UNMODIFIED	1699
#define	TT_DESKTOP_EPROTO	1610


static	struct error_stat
{
	int	num;
	dstt_status_t	status;
	char	*str;
} error_stat[] =
{
{-1,			dstt_status_not_valid,		"Not valid status"},
{-1,			dstt_status_req_rec,		"Request Received"},
{TT_DESKTOP_ENOENT,	dstt_status_file_not_avail,	"File not accessible"},
{TT_DESKTOP_EINVAL,	dstt_status_invalid_message,	"Invalid parameter"},
{TT_DESKTOP_CANCELED,	dstt_status_user_request_cancel,"User Canceled Request"},
{TT_DESKTOP_CANCELED,	dstt_status_quit_rec,		"Quit Request Received"},
{TT_DESKTOP_CANCELED,	dstt_status_discontinue,	"Discontinuing Service"},
{TT_MEDIA_ERR_SIZE,	dstt_status_data_wrong_size,	"Data size incorrect"},
{TT_MEDIA_ERR_FORMAT,	dstt_status_data_wrong_format,	"Data format incorrect"},
{TT_MEDIA_NO_CONTENTS,	dstt_status_data_not_avail,	"Data not accessible"},
{TT_DESKTOP_UNMODIFIED,	dstt_status_unmodified,		"Data reverted"},
{TT_DESKTOP_EPROTO,	dstt_status_protocol_error,	"Protocol error"},
{TT_ERR_NO_MATCH,	dstt_status_service_not_found,	"No service available"},
{TT_ERR_PROCID,		dstt_status_process_died,	"process exited"}
};

#define	SIZE	sizeof(error_stat)/sizeof(struct error_stat)

char *
dstt_set_status(Tt_message m, dstt_status_t dstt_errno)
{
	int	i;

	if(dstt_errno == dstt_status_req_rec)
	{
		return(error_stat[1].str);
	}

	for(i = 0; i < SIZE; i++)
	{
		if(dstt_errno == error_stat[i].status)
		{
			break;
		}
	}
	if(i >= SIZE)
	{
		fprintf(stderr, "error number %d out of range\n", dstt_errno);
		dstt_errno = dstt_status_not_valid;
		return(error_stat[0].str);
	}
	if(m)
	{
		tt_message_status_set(m, error_stat[i].num);
	}
	return(error_stat[i].str);
}

dstt_status_t
dstt_test_status(Tt_message m)
{
	int	i;
	int	status = tt_message_status(m);

	for(i = 0; i < SIZE; i++)
	{
		if(status == error_stat[i].num)
		{
			return(error_stat[i].status);
		}
	}
	return(dstt_status_not_valid);
}
