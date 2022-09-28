/*
 * Copyright (c) 1996, Sun Microsystems, Inc.
 * All rights reserved.
 */
 
#pragma ident	"@(#)lcl_segment.c 1.2	96/06/27 SMI"

#include "lcl.h"
#include "lcl_internal.h"

LclCharsetSegmentSet *
_lcl_create_one_segment_set(char *charset, char *buf, size_t size)
{
	LclCharsetSegmentSet	*segs;

	LclSegmentList	*list;

	list = _lcl_add_segment_list((LclSegmentList *)NULL, charset, buf, size);
	if(list == (LclSegmentList *)NULL)
		return (LclCharsetSegmentSet *)NULL;

	segs = _lcl_make_charset_segment_set(list);
	_lcl_destroy_segment_list(list);
/*
	segs = (LclCharsetSegmentSet *)malloc(sizeof(LclCharsetSegmentSet));
	if(segs == (LclCharsetSegmentSet *)NULL)
		return (LclCharsetSegmentSet *)NULL;

	segs->seg = (LclCharsetSegment *)malloc(sizeof(LclCharsetSegment) * 1);
	if(segs->seg == (LclCharsetSegment *)NULL){
		free(segs);
		return (LclCharsetSegmentSet *)NULL;
	}
	segs->num = 1;

	segs->seg[0].charset = _LclCreateStr(charset);
	segs->seg[0].segment = buf;
	segs->seg[0].size = size;
*/

	return segs;
}

void
lcl_destroy_segment_set(LclCharsetSegmentSet *segs)
{
	if(segs){
		if(segs->seg){
			int	i;
			for(i = 0; i < segs->num; i++){
				if(segs->seg[i].charset)
					free(segs->seg[i].charset);
				if(segs->seg[i].segment)
					free(segs->seg[i].segment);
			}
			free(segs->seg);
		}
		free(segs);
	}
}

LclSegmentList *
_lcl_add_segment_list(LclSegmentList *list, char *charset, char *buf, size_t size)
{
	LclSegmentList	*new_ptr, *ptr, *prev_ptr;

	new_ptr = (LclSegmentList *)malloc(sizeof(LclSegmentList));
	if(new_ptr == (LclSegmentList *)NULL){
		return (LclSegmentList *)NULL;
	}

	new_ptr->charset = charset;
	new_ptr->segment = buf;
	new_ptr->size = size;
	new_ptr->next = (LclSegmentList *)NULL;

	if(list){
		ptr = list;
		while(ptr){
			prev_ptr = ptr;
			ptr = ptr->next;
		}
		prev_ptr->next = new_ptr;
	}
	return new_ptr;
}

void
_lcl_destroy_segment_list(LclSegmentList *list)
{
	LclSegmentList	*ptr, *old_ptr;

	ptr = list;
	while(ptr){
		old_ptr = ptr;
		ptr = ptr->next;
		free(old_ptr);
	}
}

LclCharsetSegmentSet *
_lcl_make_charset_segment_set(LclSegmentList *list)
{
	LclCharsetSegmentSet	*segs;
	LclSegmentList	*ptr;
	int	num, i;

	segs = (LclCharsetSegmentSet *)malloc(sizeof(LclCharsetSegmentSet));
	if(segs == (LclCharsetSegmentSet *)NULL)
		return (LclCharsetSegmentSet *)NULL;

	num = 0;
	ptr = list;
	while(ptr){
		num++;
		ptr = ptr->next;
	}

	segs->seg = (LclCharsetSegment *)malloc(sizeof(LclCharsetSegment) * num);
	if(segs->seg == (LclCharsetSegment *)NULL){
		free(segs);
		return (LclCharsetSegmentSet *)NULL;
	}
	segs->num = num;

	ptr = list;
	for(i = 0; i < segs->num; i++){
		segs->seg[i].charset = _LclCreateStr(ptr->charset);
		segs->seg[i].segment = ptr->segment;
		segs->seg[i].size = ptr->size;
		ptr = ptr->next;
	}

	return segs;
}
