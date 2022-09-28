/* Copyright (c) 1990 by Sun Microsystems, Inc. */

#ident	"@(#)silence.c	1.27	92/10/20 SMI"

/*
 * silence.c
 *
 * Description:
 *
 */

#include <stdlib.h>
#include <memory.h>

#include "libaudio_impl.h"
#include "silence_detect.h"

/* Convenient defines for scaling values. */
#define	BIGGEST_INT	2147483647

/* Block size in seconds. */
#define	BLOCK_SIZE	.01

#define	NOISE_LEVEL	0.15		/* default noise floor level */
/* Maximum zerocrossing rate threshold. */
#define	MAX_ZEROCROSS_THR 45

/* Parameter used in setting zerocrossing threshold. It indicates
 * the number of standard deviations above the mean zerocrossing
 * rate which defines the zerocrossing threshold.
 */
#define	STD_DEV_ZEROCROSS 2.0

/* Used to set the lower absolute value threshold. */
#define	NO_TIMES_MEAN	3.0

/* Used to set the higher absolute value  threshold. */
#define	NO_TIMES_LOW 5.0

#define	END_POINTS_SIZE 32

#define LOW_LIMIT	0x20
#define HIGH_LIMIT	0x200


/* _silence_thrcalc()
 *
 * Description:
 *
 * This routine calculates the various thresholds, 'abs_val_thr[0]',
 * 'abs_val_thr[1]' and 'zerocross_thr', based on the average zerocrossing
 * rate and average absolute value of 10 blocks (100ms) of data.  The ten
 * data points are collected by 'silence_detect()'. If the peak average
 * absolute value exceeds a predetermined threshold, the 10 blocks of
 * data are considered non-silence and the three thresholds are set to
 * default values.  The predetermined threshold is chosen such that assuming
 * the 100ms is noise floor, the most optimistic SNR (which occurs if
 * the signal uses all of the available dynamic range) is 30db or higher.
 * The 'zerocross_thr' is set to 25 per 10ms, 'abs_val_thr[0]' is set assuming
 * that the signal utilizes all available dynamic range and assuming an SNR
 * of 30db, and 'abs_val_thr[1]' is set to 3 times 'abs_val_thr[0]'.
 * This default threshold setting technique is problematic in the event
 * the signal does not use all available dynamic range and the SNR is
 * high.  Some kind of threshold adaptation is neccessary to take care
 * of this case.
 *
 * If the peak average absolute value falls below the predetermined
 * threshold, the thresholds are set as follows:
 *
 * 'zerocross_thr = minimimum(MAX_ZERCROSS_THR,average(zerocross) +
 * 2.0 * standarddevation(zerocross))'
 *
 */
static void
_silence_thrcalc(SIL_STATE *state_ptr)
	                     	/* Pointer to state structure */
{
	double mean_zer;	/* Mean zerocrossing rate over 10 datapoints */
	double mean_abs_value;	/* Mean average absolute value */
	int *int_ptr1;		/* Generic pointers to ints */
	int *int_ptr2;
	double var_zer;		/* Variance of zerocrossing rate */
	int cnta;		/* Counter */
	double tmp;		/* Intermediate floating point value */
	char	silence_start;	/* flag for silence start signal */

	/*
	 *Compute mean average zerocrossing rate and mean average
	 * absolute value over 10 data points.  Each data point corresponds
	 * to the relavent measure computed over a 10ms block of data.
	 */
	int_ptr1 = state_ptr->zerocross;
	int_ptr2 = state_ptr->abs_val;
	mean_zer = *int_ptr1++;
	mean_abs_value = *int_ptr2++;
	for (cnta = 1; cnta < DATA_POINTS; cnta++) {
		mean_zer += *int_ptr1++;
		mean_abs_value += *int_ptr2++;
	}
	mean_zer /= cnta;
	mean_abs_value /= cnta;

	/* Set the abs_val thresholds */
	if (mean_abs_value < state_ptr->block_limit_low)
		silence_start = TRUE;
	else if (mean_abs_value > state_ptr->block_limit_high)
		silence_start = FALSE;
	else if (state_ptr->max_abs < (state_ptr->min_abs << 2))
		silence_start = TRUE;
	else
		silence_start = FALSE;

	if (silence_start) {
		/* Compute absolute value thresholds */
		state_ptr->abs_val_thr[0] = state_ptr->max_abs >> 8;
		state_ptr->abs_val_thr[1] = state_ptr->max_abs >> 7;

		/* Compute zerocrossing variance */
		var_zer = 0;
		for (cnta = 0; cnta < DATA_POINTS; cnta++) {
			tmp = ((*(--int_ptr1)) - mean_zer);
			var_zer += tmp*tmp;
		}

		/* Compute standard deviation */
		var_zer = sqrt(var_zer);

		/* Set threshold */
		tmp = mean_zer + STD_DEV_ZEROCROSS * var_zer;
		state_ptr->zerocross_thr =
		    (tmp > MAX_ZEROCROSS_THR) ? MAX_ZEROCROSS_THR : tmp;
		state_ptr->sil_state = 0;
	} else {
		/*
		 * We are reasonably sure the initial 100ms is not silence.
		 * so we set thresholds to default values.
		 */
		state_ptr->min_abs = state_ptr->noise * state_ptr->max_abs;
		state_ptr->abs_val_thr[0] = state_ptr->min_abs >> 8;
		state_ptr->abs_val_thr[1] = state_ptr->min_abs >> 7;
		state_ptr->zerocross_thr = MAX_ZEROCROSS_THR;
		/* Start off in STATE 2, indicating non-silence */
		state_ptr->sil_state = 2;
	}
	/*
	 * Increment data point counter to indicate to 'silence_detect()'
	 * that thresholds have been calculated.
	 */
	state_ptr->thr_calc_data_pts++;
}

static void
_short_buffer_insert(
	short		*in_buf,
	unsigned int	in_size,
	short		*buffer,
	unsigned int	buf_size)
{
	int	old;

	if (in_size < buf_size) {
		old = buf_size - in_size;
		memmove(buffer, buffer + in_size, old << 1);
		memcpy((void*)(buffer + old), (void*)in_buf, in_size << 1);
	} else {
		memcpy((void*)buffer, (void*)(in_buf + in_size - buf_size),
		    buf_size << 1);
	}
}

/*
 * silence_detect()
 *
 * Description:
 *
 * This routine returns a pointer to an array of structures which
 * describe the endpoints of regions of silence in 'in_buf'.
 * The algorithm used to determine the endpoints is described at the
 * top of this file.  The implementation uses a state machine
 * architecture.  There are five main states.   Technically the algorithm
 * has many more states, all the possible values for the SIL_STATE
 * structure. The routine loops through the
 * data in 'in_buf' in chunks of 10ms, and during each iteration it can either
 * switch to a new state or remain in the current state.  When we say that
 * the routine or algorithm is in a particular state, we are referring
 * to the value of 'state_ptr->sil_state' at the start of each iteration
 * of the loop. The meanings of the five states and possibilities
 * for the next state for each current state are as follows:
 *
 * STATE 0 - We established the start of a silence portion and
 * the last block's average absolute value falls below 'abs_val_thr[0]'.
 * NEXT STATE:
 * 0 - if current average absolute value falls below 'abs_val_thr[0]'.
 * 1 - if current average absolute value exceeds 'abs_val_thr[0]'.
 *
 * STATE 1 -
 * The last block's average absolute value exceeds
 * 'abs_val_thr[0]', but not 'abs_val_thr[1]'.
 * NEXT STATE:
 * 0 - if current average absolute value falls below 'abs_val_thr[0]'
 * and the tentative duration of silence (measured before entering STATE 1)
 * exceeds the minimum duration of silence and the time since
 * the tentative start of the most recent region of silence is greater
 * than 250ms.
 * 1 - if current average absolute value falls below 'abs_val_thr[1]'
 * but exceeds 'abs_val_thr[0]'.
 * 2 - if current average absolute value exceeds 'abs_val_thr[1]'.
 * 4 - if current average absolute value falls below 'abs_val_thr[0]'
 * and the tentative duration of silence (measured before entering STATE 1)
 * falls below the minimum duration of silence or the time since the
 * tentative start of the most recent region of silence is less
 * than 250ms.
 *
 * STATE 2 - We established the end of a silence portion and the
 * the last block's average absolute value exceeds 'abs_val_thr[1]'.
 * NEXT STATE:
 * 2 - if current average absolute value exceeds 'abs_val_thr[1]'.
 * 3 - if current average absolute value falls below 'abs_val_thr[1]'.
 *
 * STATE 3 - The last block's average absolute value exceeds
 * 'abs_val_thr[0]', but not 'abs_val_thr[1]'.
 * NEXT STATE:
 * 2 - if current average absolute value exceeds 'abs_val_thr[1]'.
 * 3 - if current average absolute value exceeds 'abs_val_thr[0]',
 * but not 'abs_val_thr[1]'.
 * 4 - if current average absolute value falls below 'abs_val_thr[0]'.
 *
 * STATE 4 - The last block's average absolute value falls below
 * 'abs_val_thr[0]' and the tentative duration of the current portion
 * of silence falls below the minimum silence duration or the time
 * since the tentative start of the current portion of silence falls below
 * 250ms.
 * NEXT STATE:
 * 0 - if the tentative duration of the current portion of silence exceeds
 * the minimum duration of silence and the time since the tentative start of
 * the current region of silence is greater than 250ms (ZEROCROSS_RANGE).
 * Also the 'abs_val' of the current block must fall below 'abs_val_thr[0]'.
 * 1 - if the 'abs_val' of the current block exceeds 'abs_val_thr[0]'.
 * 4 - if the tentative duration of the current portion of silence falls
 * below the minimum duration of silence or the time since the tentative
 * start of the current region of silence is less that 250ms.  The
 * 'abs_val' of the current block must fall below 'abs_val_thr[0]'.
 *
 * The determination of the tentative duration of silence is simply
 * the difference between the index of the tentative start of silence and
 * the index of the tentative end of silence. Both of these points are
 * determined by taking both zerocrossing rate and average absolute value
 * into account.
 *
 * Also various ACTIONS are taken by the routine during each state.
 * The particular ACTION taken, like the NEXT STATE, depends on the
 * status of the current block with respect to the thresholds as well
 * as some additional state variables. Actions consist of updating additional
 * STATE variables as well as saveing silence region end points.
 *
 * Note:
 *
 * As a convention , the 'size + 1'th entry of 'end_points' contains
 * the index of the first sample of the indeterminate region for the
 * current buffer, in the 'ep_start' field. This allows for easier
 * processing of the 'end_points' array by numerous calling applications.
 * This index, incidentally, is equivalent to the return value of 'valid'.
 *
 * This function returns 0 on successful completion,
 * SILENCE_ERR_BUFFER_TOO_SMALL if 'in_buf' is too small (this is the
 * case if 'in_buf' is smaller than the greater of the minimum silence
 * duration plus 20ms and 250ms), and SILENCE_ERR_REALLOC_FAILED if the
 * reallocation of array of structures pointed to by 'end_points' failed.
 */
int
silence_detect(
		short int *in_buf,	/* Input buffer of audio data */
		END_POINTS **end_points,/* Pointer to 'end_points' buffer */
		unsigned int *samples,	/* On input, size of 'in_buf' */
					/* On output, size of '*end_points' */
		int *valid,		/* Number of valid samples in in_buf */
		SIL_STATE *state_ptr)	/* Pointer to state structure */
{
	unsigned int	buf_size;	/* size of 'in_buf' */
	int		cnta;		/* Counters */
	int		cntb;
	short		*short_ptr1;	/* Generic pointers to shorts */
	short		*short_ptr2;
	short		*curr_block;	/* Start of current 10ms buffer */
	int		abs_val;	/* Average abs val for current blk */
	unsigned int	zerocross;	/* Zerocrossing rate for current blk */
	END_POINTS	*end_points_ptr; /* Internal pointer to 'end_points' */
	int		tmp;		/* Temporary internal variable */
	int		m;		/* First sample index + 1 */
	unsigned int	out_cnt;	/* Number of endpoints found */
	int		dangle;		/* Number of leftover samples */

	/* Init return values in case of error before they can be set */
	buf_size = *samples;
	*samples = 0;
	*valid = 0;

	m = state_ptr->m + state_ptr->block_size;

	/*
	 * Return error if the size of 'in_buf' is too small. This
	 * is the case if the size is less than the minimum duration
	 * of silence or 250ms, which ever is larger.
	 */
	if ((buf_size - m) <= state_ptr->block_size) {
		_short_buffer_insert(in_buf, buf_size, state_ptr->leftover,
		    state_ptr->block_size);
		state_ptr->m -= buf_size;
		return (SILENCE_ERR_BUFFER_TOO_SMALL);
	}
	/* Initialize internal variables. */
	dangle = -m;
	end_points_ptr = state_ptr->end_points;
	out_cnt = 0;

	/*
	 * If the algorithm finished in either state 0, or state 1 during
	 * the previous call we know that the first block of the indeterminate
	 * portion of the previous 'in_buf' is silence.  Technically, this
	 * means that this first block should not have been classified as
	 * indeterminate.  We only do it this way as a matter of convenience.
	 */
	end_points_ptr->ep_start = state_ptr->next_silence_start;


	/* Check if any part of current block leaks into previous 'in_buf' */
	if (dangle > 0) {
		/* If so unify fragmented current block into one buffer */
		memcpy((void*)state_ptr->sbuf, (void*)
		    (state_ptr->leftover + state_ptr->block_size - dangle),
		    dangle << 1);
		memcpy((void*)(state_ptr->sbuf + dangle), (void*)in_buf,
		   (state_ptr->block_size + 1 - dangle) << 1);
		/* Set 'curr_block; to point to start of current block */
		curr_block = state_ptr->sbuf;
	} else {
		/*
		 * All of the current block is contained in 'in_buf', so
		 * set 'curr_block' accordingly.
		 */
		curr_block = in_buf + m;
	}
	/*
	 * Initial loop used to collect zerocrossing and average absolute
	 * value data for the first 100ms of data.
	 */
	while (((m + state_ptr->block_size + 1) < buf_size) &&
	    (state_ptr->thr_calc_data_pts < DATA_POINTS)) {
		/*
		 * Compute zerocrossing and average absolute value
		 * for the current 10ms frame.
		 */
		zerocross = 0;
		abs_val = 0;
		for (cntb = 0; cntb < state_ptr->block_size; cntb++) {
			zerocross += (((*curr_block) ^ (*(curr_block + 1)))
			    & 0x8000) >> 15;
			tmp = *curr_block++;
			tmp = *curr_block - tmp + (tmp >> 4);
			abs_val += (tmp < 0) ? -tmp : tmp;
		}
		/* Save in array to be looked at by 'silence_thr_calc()' */
		state_ptr->zerocross[state_ptr->thr_calc_data_pts] = zerocross;
		state_ptr->abs_val[state_ptr->thr_calc_data_pts] = abs_val;

		abs_val <<= 8;
		if (abs_val > state_ptr->max_abs)
			state_ptr->max_abs = abs_val;
		else if (abs_val < state_ptr->min_abs)
			state_ptr->min_abs = abs_val;

		/* Update internal variables. */
		m += state_ptr->block_size;
		curr_block = in_buf + m;
		state_ptr->thr_calc_data_pts++;
	}

	/*
	 * We have enough data points, ie the whole 100ms initial portion
	 * of the signal has been analyzed.  Determine thresholds.
	 */
	if (state_ptr->thr_calc_data_pts == DATA_POINTS)
		_silence_thrcalc(state_ptr);

	/* Loop through 'in_buf'. */
	while ((m + state_ptr->block_size + 1) < buf_size) {

	/*
	 * Compute zerocrossing rate and average absolute value for
	 * the current block.
	 */
	abs_val = 0;
	zerocross = 0;
	for (cntb = 0; cntb < state_ptr->block_size; cntb++) {
		zerocross += (((*curr_block) ^ (*(curr_block + 1))) &
		    0x8000) >> 15;
		tmp = *curr_block++;
		tmp = *curr_block - tmp + (tmp >> 4);
		abs_val += (tmp < 0) ? -tmp : tmp;
	}

	/* Take various actions depending on the current state. */
	switch (state_ptr->sil_state) {
	case 0 :
		/*
		 * Update least recent pointer. We
		 * always want it to point to the delay of the least recent
		 * but relavent instance when the zerocrossing threshold
		 * was exceeded. Note that after this code is executed
		 * 'least_recent' can not be pointing to the last
		 * location of 'zerocross_ind[]'.
		 */
		if ((state_ptr->least_recent != state_ptr->zerocross_ind) &&
		    ((m - *(state_ptr->least_recent))
		    >= state_ptr->zerocross_range))
			state_ptr->least_recent--;

		/* Update zerocrossing counters based on current rate */
		if (zerocross >= state_ptr->zerocross_thr) {
			for (cnta = ZEROCROSS_RANGE - 1; cnta > 0; cnta--) {
				state_ptr->zerocross_ind[cnta] =
				    state_ptr->zerocross_ind[cnta - 1];
			}
			state_ptr->zerocross_ind[0] = m - state_ptr->block_size;
			/*
			 * Update the 'least_recent' pointer.   We always want
			 * it to point to the delay of the least recent but
			 * relevant instance when the zerocrossing threshold
			 * was exceeded.  Note that 'least_recent' can never be
			 * pointing at the last location of 'zerocross_ind[]'
			 * before this section of code is executed.
			 */
			if ((m - *(state_ptr->least_recent + 1)) <
			    state_ptr->zerocross_range)
				state_ptr->least_recent++;
		}

		/*
		 * NEXT STATE is 1 if the current 'abs_val'
		 * exceeds 'abs_val_thr[0]'.
		 */
		if (abs_val >= state_ptr->abs_val_thr[0]) {
			state_ptr->sil_state = 1;
			state_ptr->state_exit_ind[0] = m;
			state_ptr->end_of_sil =
			    ((m - *(state_ptr->least_recent)) <
			    state_ptr->zerocross_range) ?
			    *(state_ptr->least_recent) : m;
		}
		break;

	case 1 :
		/*
		 * Update least recent pointer. We
		 * always want it to point to the delay of the least recent
		 * but relavent instance when the zerocrossing threshold
		 * was exceeded. Note that after this code is executed
		 * 'least_recent' can not be pointing to the last
		 * location of 'zerocross_ind[]'.
		 */
		if ((state_ptr->least_recent != state_ptr->zerocross_ind) &&
		    ((m - *(state_ptr->least_recent))
		    >= state_ptr->zerocross_range))
			state_ptr->least_recent--;

		/* Update zerocrossing counters based on current rate */
		if (zerocross >= state_ptr->zerocross_thr) {
			for (cnta = ZEROCROSS_RANGE - 1; cnta > 0; cnta--) {
				state_ptr->zerocross_ind[cnta] =
				    state_ptr->zerocross_ind[cnta - 1];
			}
			state_ptr->zerocross_ind[0] = m;

			/*
			 * Update the 'least_recent' pointer. We always want
			 * it to point to the delay of the least recent but
			 * relevant instance when the zerocrossing threshold
			 * was exceeded.  Note that 'least_recent' can never be
			 * pointing at the last location of 'zerocross_ind[]'
			 * before this section of code is executed.
			 */
			if ((m - *(state_ptr->least_recent + 1)) <
			    state_ptr->zerocross_range)
				state_ptr->least_recent++;
			/*
			 * XXX - Possibly include some code here to update
			 * 'end_of_speech' indicator.
			 * This would imply modifications elsewhere.
			 */
		}

		/* NEXT STATE is 2 if current abs_val exceeds high threshold */
		if (abs_val >= state_ptr->abs_val_thr[1]) {
			state_ptr->sil_state = 2;
			state_ptr->state_exit_ind[1] = m;
			if (state_ptr->tentative_sil_dur >
			    state_ptr->min_sil_dur) {
				/*
				 * First check if starting point had been saved.
				 * If not then save it.
				 */
				if (m - state_ptr->state_exit_ind[3] <
				    state_ptr->zerocross_range) {
					end_points_ptr->ep_start =
					    state_ptr->end_of_speech;
				}

				/*
				 * We know that point A is an end point of
				 * silence.  Check if neccessary to allocate
				 * more space for the array of END_POINTS
				 * structures in the state structure.
				 * We always make sure that we have 1 more
				 * 'end_point' structure left to write to,
				 * after we've written the current one.
				 * This has to do with certain routine
				 * operations involving the 'end_points' array.
				 * See below for more details.
				 */
				if (out_cnt == state_ptr->end_points_size-1) {
					/*
					 * Reallocation neccessary.
					 * Increase size by END_POINTS_SIZE.
					 */
					end_points_ptr =
					    REALLOC(state_ptr->end_points,
					    (state_ptr->end_points_size +
					    END_POINTS_SIZE), END_POINTS);
					if (end_points_ptr == NULL)
					    return (SILENCE_ERR_REALLOC_FAILED);
					/* Reset ptrs and size indicators */
					state_ptr->end_points_size +=
					    END_POINTS_SIZE;
					state_ptr->end_points = end_points_ptr;
					end_points_ptr =
					    state_ptr->end_points + out_cnt;
				}

				state_ptr->end_of_sil =
				    ((m - *(state_ptr->least_recent)) <
				    state_ptr->zerocross_range) ?
				    *(state_ptr->least_recent) : m;
				if (state_ptr->end_of_sil >
				    end_points_ptr->ep_start) {
					/* Save end of silence in end_points */
					(end_points_ptr++)->ep_end =
					    state_ptr->end_of_sil;
					out_cnt++;
				}
			}
		} else {
			if (abs_val < state_ptr->abs_val_thr[0]) {
				/*
				 * Proceed to STATE 4 if either the end of the
				 * previous speech region could still change
				 * or if the minimum silence duration had yet
				 * to be exceeded when we first got to STATE 1.
				 */
				if (((m - state_ptr->state_exit_ind[3]) <
				    state_ptr->zerocross_range) ||
				    (state_ptr->tentative_sil_dur <
				    state_ptr->min_sil_dur)) {
					state_ptr->sil_state = 4;
					state_ptr->state_exit_ind[1] = m;
				} else {
				    /* Go back to STATE 0 and update counters */
				    state_ptr->sil_state = 0;
				    state_ptr->state_exit_ind[1] = m;
				}
			} else {
				/*
				 * Check to see if the entire buffer is about
				 * to be declared indeterminate as far as
				 * silence/non-silence classification goes.
				 * This occurs if the algorithm has been
				 * in STATE 1 for the entire buffer.
				 * If so, go either to states 0 or 4,
				 * depending on the status of
				 * 'tentative_sil_dur' and whether or not
				 * we're still within 250ms of the end
				 * of STATE 3.
				 */
				if ((state_ptr->end_of_sil < 0) &&
				    ((m + 1 + 3 * state_ptr->block_size) >
				    buf_size)) {
					if ((m - state_ptr->state_exit_ind[3] <
					    state_ptr->zerocross_range) ||
					    (state_ptr->tentative_sil_dur <
					    state_ptr->min_sil_dur)) {
						/* Go to STATE 4 */
						state_ptr->sil_state = 4;
						state_ptr->state_exit_ind[1] =
						    m;
					} else {
						/* Go to STATE 0. */
						state_ptr->sil_state = 0;
						state_ptr->state_exit_ind[1] =
						    m;
					}
				}
			}
		}
		break;

	case 2 :
		/*
		 * Move on to STATE 3 if 'abs_val' dips below the higher
		 * threshold. Otherwise remain in STATE 2.
		 */
		if (abs_val < state_ptr->abs_val_thr[1]) {
			state_ptr->sil_state = 3;
			state_ptr->state_exit_ind[2] = m;
		}
		break;

	case 3 :
		/* If 'abs_val' dips below the lower threshold, go to STATE 4 */
		if (abs_val < state_ptr->abs_val_thr[0]) {
			state_ptr->sil_state = 4;
			state_ptr->state_exit_ind[3] = m;
			state_ptr->end_of_speech = m;

			/* Saturate zerocross indicators */
			for (cnta = 0; cnta < ZEROCROSS_RANGE; cnta++)
				state_ptr->zerocross_ind[cnta] =
				    m - state_ptr->zerocross_range;

			/* Reset the 'least_recent' pointer */
			state_ptr->least_recent = state_ptr->zerocross_ind;
		} else {
			/* Return to STATE 2 if 'abs_val' exceeds high thresh */
			if (abs_val >= state_ptr->abs_val_thr[1]) {
				state_ptr->sil_state = 2;
				state_ptr->state_exit_ind[3] = m;
			}
		}
		break;

	case 4 :
		/*
		 * Update least recent pointer. We always want it to
		 * point to the delay of the least recent but relevant
		 * instance when the zerocrossing threshold was exceeded.
		 * Note that after this code is executed 'least_recent'
		 * can not be pointing to the last location of
		 * 'zerocross_ind[]'.
		 */
		if ((state_ptr->least_recent != state_ptr->zerocross_ind) &&
		    ((m - *(state_ptr->least_recent))
		    >= state_ptr->zerocross_range)) {
			state_ptr->least_recent--;
		}

		/*
		 * If 'zerocross' exceeds the threshold, shift zerocrossing
		 * counters and reset most recent counter to 0.
		 */
		if (zerocross >= state_ptr->zerocross_thr) {
			for (cnta = ZEROCROSS_RANGE - 1; cnta > 0; cnta--) {
				state_ptr->zerocross_ind[cnta] =
				    state_ptr->zerocross_ind[cnta - 1];
			}
			state_ptr->zerocross_ind[0] = m;

			/*
			 * Reset the time since end of speech only if
			 * state_exit_ind[4] is below the zerocrossing search
			 * range.  It is only within this range that point B
			 * (the end of speech) can change as a result of
			 * the zerocrossing threshold being exceeded.
			 */
			if ((m - state_ptr->state_exit_ind[3]) <
			    state_ptr->zerocross_range)
				state_ptr->end_of_speech =
				    m + state_ptr->block_size;

			/*
			 * Update the 'least_recent' pointer.   We always
			 * want it to point to the delay of the least recent
			 * but relevant instance when the zerocrossing
			 * threshold was exceeded.  Note that 'least_recent'
			 * can never be pointing at the last location of
			 * 'zerocross_ind[]' before this section of code
			 * is executed.
			 */
			if ((m - state_ptr->least_recent[1]) <
			    state_ptr->zerocross_range)
				state_ptr->least_recent++;
		}

		/*
		 * Determine how to proceed in the event that 'abs_val' exceeds
		 * the lower threshold.
		 */
		if (abs_val > state_ptr->abs_val_thr[0]) {
			/*
			 * Compute the index of the end of a tentative
			 * region of silence.
			 */
			state_ptr->end_of_sil =
			    (m - *(state_ptr->least_recent) <
			    state_ptr->zerocross_range) ?
			    *(state_ptr->least_recent) : m;

			/* Compute tentative silence duration */
			state_ptr->tentative_sil_dur =
			    state_ptr->end_of_sil - state_ptr->end_of_speech;

			/* Threshold exceeded; go to STATE 1. */
			state_ptr->sil_state = 3;
			state_ptr->state_exit_ind[4] = m;

			/*
			 * Save starting point of silence if the minimum
			 * silence duration has been exceeded.
			*/
			if ((state_ptr->tentative_sil_dur >
			    state_ptr->min_sil_dur) &&
			    ((m - state_ptr->state_exit_ind[3]) >=
			    state_ptr->zerocross_range))
				end_points_ptr->ep_start =
				    state_ptr->end_of_speech;
		} else {
			/*
			 * Compute the index of the end of a tentative
			 * region of silence.
			 */
			state_ptr->end_of_sil =
			    (m - *(state_ptr->least_recent) <
			    state_ptr->zerocross_range) ?
			    *(state_ptr->least_recent) :
			    m + state_ptr->block_size;

			/* Compute tentative silence duration. */
			state_ptr->tentative_sil_dur = state_ptr->end_of_sil -
			    state_ptr->end_of_speech;
			/*
			 * If minimum silence has been exceeded, switch
			 * to STATE 0 and save endpoints.  If minimum
			 * silence has not been exceeded, continue in STATE 4.
			 */
			if ((state_ptr->tentative_sil_dur >
			    state_ptr->min_sil_dur) &&
			    (m - state_ptr->state_exit_ind[3] >=
			    state_ptr->zerocross_range)) {
				/* Go to STATE 0. */
				state_ptr->sil_state = 0;
				state_ptr->state_exit_ind[4] = m;
				/* Save starting point of silence */
				end_points_ptr->ep_start =
				    state_ptr->end_of_speech;
			}
		}
	}
	abs_val *= (int)(state_ptr->thr_scale * 256);
	switch (state_ptr->sil_state) {
	case 0:
	case 4:
	if (abs_val < state_ptr->min_abs)
		state_ptr->min_abs = abs_val;
	else
		state_ptr->min_abs += (abs_val - state_ptr->min_abs) >> 4;
		break;
/*
	case 2:
	if (abs_val > state_ptr->max_abs)
		state_ptr->max_abs = abs_val;
	else
		state_ptr->max_abs += (abs_val - state_ptr->max_abs) >> 8;
		break;
*/
	case 1:
	case 3:
	default:
	if (abs_val < state_ptr->min_abs)
		state_ptr->min_abs = abs_val;
	else if (abs_val > state_ptr->max_abs)
		state_ptr->max_abs = abs_val;
	else {
		state_ptr->min_abs += (abs_val - state_ptr->min_abs) >> 10;
		state_ptr->max_abs += (abs_val - state_ptr->max_abs) >> 8;
	}
	}
	if (state_ptr->max_abs < (state_ptr->min_abs << 2)) {
		if (state_ptr->max_abs < state_ptr->block_limit_high) {
			state_ptr->abs_val_thr[0] = state_ptr->max_abs >> 8;
			state_ptr->abs_val_thr[1] = state_ptr->max_abs >> 7;
		}
	} else if (state_ptr->max_abs > (state_ptr->min_abs << 5)) {
		state_ptr->abs_val_thr[0] = state_ptr->min_abs >> 6;
		state_ptr->abs_val_thr[1] = state_ptr->min_abs >> 5;
	}
	/* Update current block index and pointer. */
	m += state_ptr->block_size;
	curr_block = in_buf + m;
	} /* while */

	/* Compute last valid 'm' relative to start of next 'in_buf'. */
	m -= state_ptr->block_size;
	state_ptr->m = m - buf_size;

	/* Copy 'dangle'ing data into 'leftover'. */
	memcpy((void*)state_ptr->leftover,
	    (void*)(in_buf + buf_size - state_ptr->block_size),
	    state_ptr->block_size << 1);

	/* Determine how much of the current buffer is valid. */
	switch (state_ptr->sil_state) {
	case 0:
		/*
		 * Calculate this in such a way that a 10ms block of
		 * definite silence starts off the invalid region.
		 * This makes synchronization of the silence end point
		 * storing process easier on the next call.
		 */
		*valid =
		(m - *(state_ptr->least_recent) < state_ptr->zerocross_range) ?
		*(state_ptr->least_recent) - state_ptr->block_size : m;

		state_ptr->next_silence_start = *valid - buf_size;
		(end_points_ptr++)->ep_end = *valid;
		out_cnt++;
		end_points_ptr->ep_start = *valid;
		break;

	case 1:
		if ((m - state_ptr->state_exit_ind[3] <
		    state_ptr->zerocross_range) ||
		    (state_ptr->tentative_sil_dur < state_ptr->min_sil_dur)) {
			*valid = state_ptr->end_of_speech;
			state_ptr->next_silence_start = 1;
		} else {
			*valid = state_ptr->end_of_sil - state_ptr->block_size;
			state_ptr->next_silence_start = *valid - buf_size;
			(end_points_ptr++)->ep_end = *valid;
			out_cnt++;
		}
		end_points_ptr->ep_start = *valid;
		break;

	case 2:
		state_ptr->next_silence_start = 1;
		*valid = m;
		end_points_ptr->ep_start = *valid;
		break;

	case 3:
		state_ptr->next_silence_start = 1;
		*valid = m;
		end_points_ptr->ep_start = *valid;
		break;

	case 4:
		state_ptr->next_silence_start = 1;
		*valid = state_ptr->end_of_speech;
		end_points_ptr->ep_start = *valid;
		break;
	}

	*samples = out_cnt;
	*end_points = state_ptr->end_points;

	/* Recalculate various indices relative to start of next 'in_buf'. */
	for (cnta = 0; cnta < ZEROCROSS_RANGE; cnta++) {
		if (m - state_ptr->zerocross_ind[cnta] <
		    state_ptr->zerocross_range) {
			state_ptr->zerocross_ind[cnta] -= buf_size;
		} else {
			state_ptr->zerocross_ind[cnta] =
			    m - state_ptr->zerocross_range - buf_size;
		}
	}
	for (cnta = 0; cnta < 5; cnta++) {
		if (m - state_ptr->state_exit_ind[cnta] < BIGGEST_INT/2) {
			state_ptr->state_exit_ind[cnta] -= buf_size;
		} else {
			state_ptr->zerocross_ind[cnta] =
			    m - BIGGEST_INT/2 - buf_size;
		}
	}
	if (m - state_ptr->end_of_sil < BIGGEST_INT/2) {
		state_ptr->end_of_sil -= buf_size;
		state_ptr->end_of_speech -= buf_size;
	} else {
		state_ptr->end_of_sil = m - BIGGEST_INT/2 - buf_size;
		state_ptr->end_of_speech = state_ptr->end_of_sil -
		    state_ptr->tentative_sil_dur;
	}

	return (0);
}

/*
 * silence_destroy_state()
 *
 * Description:
 *
 * This routines frees all pointers internal to the SIL_STATE structure
 * (created by 'silence_create_state()' pointed to by 'state_ptr', and
 * then frees 'state_ptr' itself.
 */
void
silence_destroy_state(SIL_STATE *state_ptr)
{
	FREE(state_ptr->end_points);
	FREE(state_ptr->sbuf);
	FREE(state_ptr->leftover);
	FREE(state_ptr);
}

/*
 * silence_init_state()
 *
 * Description:
 *
 * This routine initializes the SIL_STATE structure pointed to
 * by 'state_ptr'.  Reset the threshold, sample rate, and minimum
 * silence duration *before* calling this routine.
 * An alternative is to destroy the current state structure
 * and create a new one.
 */
void
silence_init_state(SIL_STATE *state_ptr)
{
	int cnta;

	/*
	 * Saturate the most recent high zerocrossing rate
	 * (> threshold) indices.
	 */
	for (cnta = 0; cnta < ZEROCROSS_RANGE; cnta++)
		state_ptr->zerocross_ind[cnta] = -state_ptr->zerocross_range;

	/* Saturate the state exit indices. */
	for (cnta = 0; cnta < 5; cnta++) {
		state_ptr->state_exit_ind[cnta] =
		    4*((state_ptr->min_sil_dur > state_ptr->zerocross_range) ?
		    -state_ptr->min_sil_dur : -state_ptr->zerocross_range);
	}

	/* Saturate the tentative duration of silence. */
	state_ptr->tentative_sil_dur = 4 * (state_ptr->min_sil_dur +
	    state_ptr->block_size);

	/* Initialize m. */
	state_ptr->m = -state_ptr->block_size;

	/* Do threshold calculation (recalculation). */
	state_ptr->thr_calc_data_pts = 0;

	/* Initialize least_recent. */
	state_ptr->least_recent = &(state_ptr->zerocross_ind[0]);

	state_ptr->min_abs = 0x7FFFFFFF;
	state_ptr->max_abs = 0;
	state_ptr->block_limit_low = LOW_LIMIT * state_ptr->block_size;
	state_ptr->block_limit_high = HIGH_LIMIT * state_ptr->block_size;

	state_ptr->end_of_sil = 0;
	state_ptr->end_of_speech = 0;
	state_ptr->next_silence_start = 0;
}

/*
 * silence_create_state()
 *
 * Description:
 *
 * The purpose of this routine is to create a SIL_STATE structure for
 * use with 'silence_detect()'.  It returns a pointer to the structure
 * if everything could be allocated, otherwise it returns NULL.
 * 'silence_init_state()' is called at the end.
 */
SIL_STATE*
silence_create_state(unsigned int sampling_rate, double min_sil_dur)
{
	SIL_STATE *state_ptr;
	int size;

	if ((state_ptr = CALLOC(1, SIL_STATE)) == NULL)
		return (NULL);

	else if ((state_ptr->end_points = CALLOC(END_POINTS_SIZE, END_POINTS))
	    == NULL) {
		FREE(state_ptr);
		return (NULL);
	} else {
		state_ptr->end_points_size = END_POINTS_SIZE;
		/* Compute in samples, the 10ms block_size. */
		state_ptr->block_size = sampling_rate * BLOCK_SIZE + 0.5;
		if ((state_ptr->leftover = CALLOC(state_ptr->block_size, short))
		    == NULL) {
			FREE(state_ptr->end_points);
			FREE(state_ptr);
			return (NULL);
		} else if ((state_ptr->sbuf = CALLOC(state_ptr->block_size + 1,
		    short)) == NULL) {
			FREE(state_ptr->leftover);
			FREE(state_ptr->end_points);
			FREE(state_ptr);
			return (NULL);
		}
	}

	/*
	 * Compute in samples, the equivalent of 250ms which is the
	 * range overwhich zerocrossings are considered.
	 */

	state_ptr->zerocross_range = state_ptr->block_size * ZEROCROSS_RANGE;
	state_ptr->noise = NOISE_LEVEL;
	state_ptr->thr_scale = 1.0;

	/* Initialize sampling rate and silence duration. */
	state_ptr->sampling_rate = sampling_rate;
	state_ptr->min_sil_dur = sampling_rate * min_sil_dur;
	silence_init_state(state_ptr);

	return (state_ptr);
}

/*
 * silence_set_rate()
 *
 * Resets sample rate used by 'silence_detect()'.
 */
/*ARGSUSED*/
void
silence_set_rate(unsigned int new_rate, SIL_STATE *state_ptr)
{
	state_ptr->sampling_rate = new_rate;
}

/*
 * silence_set_min_sil_dur()
 *
 * Description:
 *
 * Resets minimum silence duration constraint used by
 * 'silence_detect()'. Expects 'min_sil_dur' to be in
 * seconds.
 */
void
silence_set_min_sil_dur(double min_sil_dur, SIL_STATE *state_ptr)
{
	state_ptr->min_sil_dur = state_ptr->sampling_rate * min_sil_dur;
}

/*
 * silence_get_min_sil_dur(state_ptr)
 *
 * Description:
 *
 * Returns the current minimum duration of silence, in seconds from
 * the SIL_STATE structure pointed to by 'state_ptr'.
 */
double
silence_get_min_sil_dur(SIL_STATE *state_ptr)
{
	double min_sil_dur;

	min_sil_dur = ((double)(state_ptr->min_sil_dur) /
	    (double)(state_ptr->sampling_rate));
	return (min_sil_dur);
}

/*
 * silence_set_thr_scale()
 *
 * Description:
 *
 * Resets the amplitude threshold scale used by 'silence_detect()' to
 * discriminate between silence and non-silence.  'new_scale' should be >= 0.
 * The higher the value, the greater the tendency of a signal region
 * being classified as silence.
 */
void
silence_set_thr_scale(double new_scale, SIL_STATE *state_ptr)
{
	state_ptr->thr_scale = (new_scale < 0.0) ? 0.0 : new_scale;;
}

/*
 * silence_get_thr_scale()
 *
 * Description:
 *
 * Returns the current threshold scale as a value lying between
 * 0.0 and 4.0.  The higher the scale, the greater the tendency of
 * something being classified as silence.
 *
 */
double
silence_get_thr_scale(SIL_STATE *state_ptr)
{
	return (state_ptr->thr_scale);
}

/*
 * silence_set_noise_ratio()
 */
void
silence_set_noise_ratio(double ratio, SIL_STATE *state_ptr)
{
	if (ratio < 0.0)
		state_ptr->noise = 0.0;
	else if (ratio > 1.0)
		state_ptr->noise = 1.0;
	else
		state_ptr->noise = ratio;
}

/*
 * silence_get_noise_ratio(state_ptr)
 */
double
silence_get_noise_ratio(SIL_STATE *state_ptr)
{
	return (state_ptr->noise);
}
