/*
 * Copyright (c) 1996, Sun Microsystems, Inc.
 * All rights reserved.
 */
 
#pragma ident	"@(#)lcl_db.h 1.2	96/06/27 SMI"

int	_LclCreateDatabase(LCLd lcld, char *pathname);
void	_LclDestroyDatabase(LCLd lcld);
int	_LclParseCharsetInfo(LCLd lcld);
int	_LclParseIconvInfo(LCLd lcld);
int	_LclParseFormInfo(void *db, LclFormInfo **form_info);
