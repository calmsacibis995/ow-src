/* Copyright (c) 1992 by Sun Microsystems, Inc. */
#ident	"@(#)tsm.c	1.5	96/02/20 SMI"

#include "tsm.h"
#include "libaudio_impl.h"

/* tsm_init_state()
 *
 * Description:
 * 
 * This routine is called by the application to initialize/reset an existing
 * structure pointed to by 'state_prt' used by 'tsm()' to preserve its
 * internal state between calls.
 */
void
tsm_init_state(
	TSM_STATE	*state_ptr)
{
	/* Clear certain buffers. */
	memset(state_ptr->leftover, '\0', state_ptr->block_size << 1);
	memset(state_ptr->olap, '\0', state_ptr->block_size << 1);

	state_ptr->leftover_size = 0;
	state_ptr->olap_size = 0;
}	

/* tsm_create_state()
 *
 * Description:
 *
 * Creates a TSM_STATE structure used by 'tsm()' to preserve its
 * state between calls. Returns a pointer to the structure if succesful
 * otherwise it returns NULL.
 */

TSM_STATE*
tsm_create_state(
	unsigned int sampling_rate)
{
	TSM_STATE *state_ptr;	/* Temporary state pointer. */

	if (sampling_rate == 0)
		return (NULL);

	else if ((state_ptr = CALLOC(1, TSM_STATE)) == NULL)
		return (NULL);

	else {
		/* Block size is 32ms. */
		state_ptr->block_size = (BLOCK_SIZE * sampling_rate);
		if ((state_ptr->leftover = CALLOC(state_ptr->block_size << 1,
		    short)) == NULL) {
			FREE(state_ptr);
			return (NULL);
		} else if ((state_ptr->olap = CALLOC(state_ptr->block_size,
		    short)) == NULL) {
			FREE(state_ptr->leftover);
			FREE(state_ptr);
			return (NULL);
		} else {
			/* all goes well, initialize and return pointer. */

			/* Overlap size is 8ms. */
			state_ptr->overlap_size = OVERLAP_SIZE * sampling_rate;
			tsm_init_state(state_ptr);
			return (state_ptr);
		}
	}
}

/* 
 * tsm_destroy_state()
 *
 * Description:
 *
 * Destroys a TSM_STATE structure pointed to by 'state_ptr'.
 */
void
tsm_destroy_state(
	TSM_STATE *state_ptr)
{
	FREE(state_ptr->olap);
	FREE(state_ptr->leftover);
	FREE(state_ptr);
}

unsigned int
signxcor(				/* sign bit cross corelation */
	short		*buf_a,
	short		*buf_b,
	unsigned	size)
{
	unsigned int	accum;

	accum = 0;
	while (size--) {
		if (((*buf_a++) ^ (*buf_b++)) >= 0)
			accum++;
	}
	return (accum);
}

/*
 * tsm()
 * 
 * Description:
 *
 * This routine performs time scale modification of speech
 * or music data. The algorithm used is known as the Synchronized Overlap
 * Add (SOLA). It increases or decreases the length of
 * 'in_buf' (which is initially '*samples' samples) by approximately a
 * factor of 'ratio' and stores the result in 'out_buf'. The exact
 * length of 'out_buf' is '*samples' samples.
 * 
 * The implementation below uses some modifications to the
 * basic SOLA algorithm.
 * Refer to "High Quality Time-Scale Modification for Speech" by
 * S. Roucos and A. Wilgus, of IEEE ICASSP 1985 Preceddings.
 *
 * Notes:
 * 
 * The calling application must allocate sufficient space for
 * 'out_buf' to allow for storage of the lengthened or shortened
 * input data. tsm_outbuf_leng() returns the required length of the
 * output buffer.
 */
unsigned int
tsm(
	short	*in_buf,	/* Input buffer */
	unsigned int samples,	/* number of samples in in_buf */
	double	ratio,		/* ratio of out_buf samples to in_buf samples */
	TSM_STATE *state_ptr,	/* Pointer to algorithm state structure. */
	short	*out_buf)	/* Output buffer */
{
	short	*dest_ptr;
	short	*sour_ptr;
	short	*out_ptr;		/* Pointer to 'out_buf'. */
	short	*search_buf; 
	long	best_score; 
	int	score;	/* Matching score for current offset. */
	int	best_cnt;	/* Keeps track of best offset. */
	int	cnta;	/* Generic indexing/counter variables. */
	unsigned	size_anal, size_synth, size_match, match_start;
	int		block_start, block_end;
	int		offset;
	unsigned	size_out;

	if (samples == 0) {
		size_out = state_ptr->leftover_size * ratio + 0.5;
		memcpy(out_buf, state_ptr->leftover, size_out << 1);
		return (size_out);
	} else if ((samples + state_ptr->leftover_size) < state_ptr->block_size)
	    {
		memcpy(state_ptr->leftover + state_ptr->leftover_size,
		    in_buf, samples << 1);
		state_ptr->leftover_size += samples;
		return (0);
	}

	/* guarantee the minimal overlap size between input or output blocks */
	if (ratio < 1) {				/* compression */
		size_anal = state_ptr->block_size - state_ptr->overlap_size;
		size_synth = size_anal * ratio + 0.5;
		size_match = state_ptr->block_size - size_synth;
	} else {					/* expansion */
		size_match = state_ptr->overlap_size;
		size_synth = state_ptr->block_size - size_match;
		size_anal = size_synth / ratio + 0.5;
	}

	/* Initialize some internal variables. */
	block_start = -state_ptr->leftover_size;
	block_end = block_start + state_ptr->block_size;

	memcpy(out_buf, state_ptr->olap, state_ptr->olap_size << 1);
	out_ptr = out_buf;
	size_out = state_ptr->olap_size;

	match_start = size_synth - (state_ptr->overlap_size >> 1);
	if ((int)match_start < 0)   /* MCA cast to int */
		match_start = 0;
	if (block_start < 0) {
		memcpy(state_ptr->leftover + state_ptr->leftover_size,
		    in_buf, state_ptr->block_size << 1);
		search_buf = state_ptr->leftover;
	}
	while (block_end <= samples) {
		if (block_start >= 0)
			search_buf = in_buf + block_start;

		/* Initialize variables for best offset.  */
		dest_ptr = out_ptr + match_start;
		best_score = 0;
		best_cnt = 0;
		for (cnta = 0; cnta < state_ptr->overlap_size; cnta++) {
			/* Calculate the matching score for current offset. */
			score = signxcor(dest_ptr++, search_buf,
			    state_ptr->overlap_size >> 1);
			if (score > best_score) {
				best_score = score;
				best_cnt = cnta;
			}
			if (score == state_ptr->overlap_size >> 1)
				break;
		}
		best_cnt += match_start;

		/* Merge overlap regions of block and the output block. */
		dest_ptr = out_ptr + best_cnt;
		sour_ptr = search_buf;
		offset = best_cnt + state_ptr->block_size;
		if (offset <= size_out) {
			for (cnta = 0; cnta < state_ptr->block_size; cnta++)
				*dest_ptr++ = (*dest_ptr + *sour_ptr++) >> 1;
		} else {
			for (cnta = best_cnt; cnta < size_out; cnta++)
				*dest_ptr++ = (*dest_ptr + *sour_ptr++) >> 1;

			/* Copy remainder of best offset block into output. */
			memcpy(dest_ptr, sour_ptr, (offset - size_out) << 1);
			size_out = offset;
		}
		out_ptr += size_synth;
		size_out -= size_synth;

		/* Increment and update counters and internal variables. */
		block_end += size_anal;
		block_start += size_anal;
	}
	memcpy(state_ptr->olap, out_ptr, size_out << 1);
	state_ptr->olap_size = size_out;

	state_ptr->leftover_size = samples - block_start;
	memcpy(state_ptr->leftover, in_buf + block_start,
	    state_ptr->leftover_size << 1);

	return (out_ptr - out_buf);
}
/*
 * unsigned int tsm_outbuf_leng(samples, ratio, state_ptr)
 *
 * Based on the number of samples in the input buffer, the time
 * scale modification ratio and the algorithm state, the maximum
 * output buffer length in samples is computed and returned.
 *
 */
unsigned int
tsm_outbuf_leng(samples, ratio, state_ptr)
	unsigned	samples;
	double		ratio;
	TSM_STATE	*state_ptr;
{
	unsigned	size_anal, size_synth, size_match;
	int		block_end;
	unsigned	size_out;

	/* make sure there is no gap between input or output blocks */
	if (ratio < 1) {				/* compression */
		size_anal = state_ptr->block_size - state_ptr->overlap_size;
		size_synth = size_anal * ratio + 0.5;
		size_match = state_ptr->block_size - size_synth;
	} else {					/* expansion */
		size_match = state_ptr->overlap_size;
		size_synth = state_ptr->block_size - size_match;
		size_anal = size_synth / ratio + 0.5;
	}

	block_end = 0;					/* worst case */
	size_out = state_ptr->block_size;

	/*
	 * Loop until end of 'search_buf' extends beyond the end of
	 * 'inbuf'.
	 */
	while (block_end <= samples) {
		size_out += size_synth;
		block_end += size_anal;
	}
	size_out += state_ptr->overlap_size >> 1;
	return (size_out);
}
