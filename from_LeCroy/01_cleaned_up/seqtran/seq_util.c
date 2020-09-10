/*------------------------- SEQ_UTIL.C --------------------------------------*/
/*
Author: Gabriel Goodman		LeCroy Corporation
				ATS Division
				700 Chestnut Ridge Road
				Chestnut Ridge, NY 10977
*/
/*---------------------------------------------------------------------------*/


#define  PCW_UTIL_C

#include <string.h>
#include <stdlib.h>
#include <malloc.h>
#include <dos.h>
#include "seq_hdr.h"
#include "seq_tran.h"

extern struct PCW_BLOCK *PCW_blockP[MAX_PLUGINS][MAX_CHANNELS];
extern BYTE   	 	*PCW_waveformP[MAX_PLUGINS][MAX_CHANNELS];
extern VOID    		*PCW_Find_Value_From_Name();
extern VOID   		SEQ_update_time();

#define MAX_TIME 3

#ifndef RIS
/*XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX*/

VOID SEQ_Init_Descriptor(acq_dataP, filt_dataP, wave_dataP, fine_count)
    SEQ_ACQ_DATA    *acq_dataP;
    SEQ_FILTER_DATA *filt_dataP;
    WAVE_PARAMS	    *wave_dataP;
    WORD 	    fine_count;

/*--------------------------------------------------------------------------

    Purpose: To initialize the waveform descriptor for this plugin/channel
		based on this segment's TDC fine count. Also, initialize
		the elements of wave_dataP which are needed when the
		waveform is output.

    Inputs: acq_dataP = parameters associated with the acquisition of the data
	    wave_dataP = parameters needed to compensate the time and vertical
			values.
	    fine_count = TDC fine count for this segment (same for both
			  channels)

    Outputs: 

    Machine dependencies: 
			 
    Notes: 

    Procedure:

/CODE
--------------------------------------------------------------------------*/
{   /* SEQ_Init_Descriptor() */

    FLOAT  *fP;
    WORD   *wP;
    LONG   *lP;
    WORD   p;
    WORD   c;
    DOUBLE *pixel_offsetP;
    DOUBLE *horiz_offsetP;
    FLOAT  *horiz_intervalP;
    FLOAT  *vert_gainP;
    FLOAT  trigger_delay;
    LONG   data_count;
    WORD   *record_typeP;

struct PCW_BLOCK *blockP;
    BYTE   	 *waveformP;

    static BOOL translated[MAX_PLUGINS][MAX_CHANNELS] = {
	FALSE,FALSE,FALSE,FALSE,		/* Plugin A */
	FALSE,FALSE,FALSE,FALSE			/* Plugin B */
    };

    p = acq_dataP->plugin;
    c = acq_dataP->channel;
    blockP = PCW_blockP[p][c];
    waveformP = PCW_waveformP[p][c];
    data_count = filt_dataP->array_size;

    /* Update the basic parameters once only */
    if (translated[p][c] == FALSE)
    {
	translated[p][c] = TRUE;

	/* Change the record type to be single sweep for read-back into 7200A */
	    wP = (WORD *)PCW_Find_Value_From_Name(waveformP, (LONG)0, blockP,
							      "RECORD_TYPE");
	    *wP = 0;

	/* Change COMM_TYPE to 16-bit corrected WORDs */
	    wP = (WORD *)PCW_Find_Value_From_Name(waveformP, (LONG)0, blockP,
							      "COMM_TYPE");
	    *wP = 1;

	/* Change the subarray count to 1 for single sweep */
	    lP = (LONG *)PCW_Find_Value_From_Name(waveformP, (LONG)0, blockP,
							  "NOM_SUBARRAY_CNT");
	    *lP = 1L;

	    lP = (LONG *)PCW_Find_Value_From_Name(waveformP, (LONG)0, blockP,
							  "SUBARRAY_CNT");
	    *lP = 1L;

	/* Change MIN and MAX values from 8-bit to 16-bit equivalents */
	    wP = (WORD *)PCW_Find_Value_From_Name(waveformP, (LONG)0, blockP,
							      "MIN_VALUE");
	    *wP *= 256;
	    wP = (WORD *)PCW_Find_Value_From_Name(waveformP, (LONG)0, blockP,
							      "MAX_VALUE");
	    *wP *= 256;

	/* Change Vertical Gain from 8-bit to 16-bit equivalent */
	    fP = (FLOAT *)PCW_Find_Value_From_Name(waveformP, (LONG)0, 
						    blockP, "VERTICAL_GAIN");
	    *fP /= 256;

	/* Change the Trig time array size to 0 for single sweep */
	    lP = (LONG *)PCW_Find_Value_From_Name(waveformP, (LONG)0, blockP,
							  "TRIGTIME_ARRAY");
	    *lP = 0L;

	/* Change the wave array 1 size to (data_count * sizeof(WORD)) */
	    lP = (LONG *)PCW_Find_Value_From_Name(waveformP, (LONG)0, blockP,
							      "WAVE_ARRAY_1");
	    *lP = (LONG)(sizeof(WORD) * data_count);

	/* Change the wave array count to data count */
	    lP = (LONG *)PCW_Find_Value_From_Name(waveformP, (LONG)0, blockP,
							  "WAVE_ARRAY_COUNT");
	    *lP = data_count;

	/* Change the last valid point to be data_count-1 */
	    lP = (LONG *)PCW_Find_Value_From_Name(waveformP, (LONG)0, blockP,
							  "LAST_VALID_PNT");
	    *lP = data_count-1;
    }

    /*************************************************************************
     Update the Horizontal Offset based on the acquired TDC fine count:

      if (2 GS/s)
	  trigger_delay = (2*TDC fine count * horizontal interval) / 32768
      else
	  trigger_delay = (TDC fine count * horizontal interval) / 32768
      Horizontal Offset = Pixel Offset - (Horizontal Interval - trigger_delay)

     *************************************************************************/
    horiz_intervalP = (FLOAT *)PCW_Find_Value_From_Name(waveformP, (LONG)0, 
						    blockP, "HORIZ_INTERVAL");
    pixel_offsetP = (DOUBLE *)PCW_Find_Value_From_Name(waveformP, (LONG)0,
						      blockP, "PIXEL_OFFSET");
    horiz_offsetP = (DOUBLE *)PCW_Find_Value_From_Name(waveformP, (LONG)0,
						      blockP, "HORIZ_OFFSET");

    if (filt_dataP->paramsP->num_filters > 8)
	fine_count *= 2;

    trigger_delay = ((FLOAT)fine_count * *horiz_intervalP) / 32768.0;
    *horiz_offsetP = *pixel_offsetP - (*horiz_intervalP - trigger_delay);

    /***********************************************************
      Update the wave_data with the values from the descriptor
     ***********************************************************/
    fP = (FLOAT *)PCW_Find_Value_From_Name(waveformP, (LONG)0, blockP,
							  "VERTICAL_OFFSET");
    wave_dataP->vertical_offset = *fP;

    fP = (FLOAT *)PCW_Find_Value_From_Name(waveformP, (LONG)0, blockP, 
							  "VERTICAL_GAIN");
    wave_dataP->vertical_gain = *fP;

    wave_dataP->horizontal_offset = *horiz_offsetP;
}

/*XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX*/

BOOL SEQ_Check_Seg(p, c, segno, time, keep_goingP)
  BYTE   p;
  BYTE   c;
  LONG   segno;
  DOUBLE time;
  BOOL   *keep_goingP;

/*--------------------------------------------------------------------------

    Purpose: Search thru the options that the user selected to determine if
		this segment should be processed or just skipped over.

    Inputs: p = 0 for plugin A
	      = 1 for plugin B
	    
	    c = 0,1,2,3 for 7234
	      = 0,1     for 7242B

	    segno = segment number

	    time  = time in seconds relative to first segment

    Outputs: TRUE  if segment should be processed
	     FALSE if segment should not be processed

	     keep_goingP = TRUE if there are still segs to be processed
		         = FALSE if there are no more segs to be processed

    Machine dependencies: 
			 
    Notes: 

    Procedure:

/CODE
--------------------------------------------------------------------------*/
{   /* SEQ_Check_Seg() */

    static WORD index[MAX_PLUGINS][MAX_CHANNELS][MAX_SEG_TYPES] = {
      0,0, 0,0, 0,0, 0,0,   /* Plugin A, Chan 1..4, bot seg types */
      0,0, 0,0, 0,0, 0,0    /* Plugin B, Chan 1..4, bot seg types */
    };

    SEGS *segP,*seg1P,*seg2P;
    BOOL process;
    BOOL single_seg;

    process = FALSE;

    if (SEQ_options.all_segs[p][c] == TRUE)
	process = TRUE;

    /* Check this plugin/chan segment against any the user may have selected */
    if (process == FALSE)
    {
	segP = &SEQ_options.seg[p][c][SEQ_SEGNO][index[p][c][SEQ_SEGNO]];
	if (segP->select.n.start != -1L)
	{
	    if (segno > segP->select.n.end)
		++index[p][c][SEQ_SEGNO];

	    segP = &SEQ_options.seg[p][c][SEQ_SEGNO][index[p][c][SEQ_SEGNO]];
	    if (segP->select.n.start != -1L)
	    {
		if ((segP->select.n.start <= segno) &&
		    (segP->select.n.end >= segno))
		{
		    process = TRUE;
		}
	    }
	}
    }

    if (process == FALSE)
    {
	segP = &SEQ_options.seg[p][c][SEQ_TIME][index[p][c][SEQ_TIME]];
	if (segP->select.t.start != (DOUBLE)-1)
	{
	    if (segP->select.t.start == segP->select.t.end)
		single_seg = TRUE;
	    else
	    {
		single_seg = FALSE;
		if (time > segP->select.t.end)
		    ++index[p][c][SEQ_TIME];
	    }

	    segP = &SEQ_options.seg[p][c][SEQ_TIME][index[p][c][SEQ_TIME]];
	    if (segP->select.t.start != (DOUBLE)-1)
	    {
		if ((segP->select.t.start <= time) &&
		    (segP->select.t.end >= time))
		{
		    process = TRUE;
		}
		else if (single_seg && time >= segP->select.t.start)
		{
		    /* Process the first seg after a single specified time */
		    process = TRUE;
		    ++index[p][c][SEQ_TIME];
		}
	    }
	}
    }

    if (process == FALSE)
    {
	/* Check if all requested segs were processed */
	*keep_goingP = FALSE;
	for (p=0; p < MAX_PLUGINS; ++p)
	{
	    for (c=0; c <= SEQ_params.last_channel[p]; ++c)
	    {
		seg1P=&SEQ_options.seg[p][c][SEQ_TIME][index[p][c][SEQ_TIME]];
		seg2P=&SEQ_options.seg[p][c][SEQ_SEGNO][index[p][c][SEQ_SEGNO]];

		if ((SEQ_options.test_mode == TRUE) ||
		    (SEQ_options.print_times == TRUE) ||
		    (SEQ_options.all_segs[p][c] == TRUE) ||
		    (seg1P->select.t.start != (DOUBLE)-1) ||
		    (seg2P->select.n.start != -1L))
		    *keep_goingP = TRUE;
	    }
	}
    }
    else
	*keep_goingP = TRUE;

    return(process);
}

/*XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX*/

VOID SEQ_diagnostic(seq_fP, acq_dataP, filt_dataP, size)
    FILE  	  *seq_fP;
    SEQ_ACQ_DATA  *acq_dataP;
    SEQ_FILTER_DATA *filt_dataP;
    LONG	  size;

/*--------------------------------------------------------------------------

    Purpose: To compare the corrected diagnostic data with the output of
		SEQ_fir and print any differences. This routine is only
		for non-7291 data.

    Inputs: acq_dataP = acquisition parameters, data size, channel, etc
			acq_dataP->block offset it set to get the first
			sample in the diagnostic block.
	    filt_dataP = parameters associated with filtering the data.
	    size = size of correction data in diagnostic block (WORDs)

    Outputs: Filters the uncorrected data in the previous segment and
	     compares it to the diagnostic block. Advances the seq_fP
	     past the data it checks and leaves it pointing to the
	     next channel's diagnostic block.

    Machine dependencies: 
			 
    Notes: The diagnostic block consists of 16-bit WORDs in Intel format
	   (Low/High).

	   The data in the diagnostic block is reversed so that the last
	   WORD appears first and the first corrected sample appears last.

	   The diagnostic block is preceeded with 0x4444 ("DD"). This extra
	   word must be accounted for when searching through memory.

	   The seq_fP is pointing to the first corrected WORD for channel 1
	   for this plugin; acq_dataP->channel determines which channel
	   should be tested.

    Procedure:
	block_number = 0
	while (size > 0)
	{
	    seek to size-MAX_BUF
	    read MAX_BUF corrected points
	    un-reverse MAX_BUF points
	    seek to -size-((channel-1) * corrected size)
	    seek to -2-((SEQ_param.last_channel-channel)*uncorrected size)
	    seek to block_number*MAX_BUF
	    if (first time)
		read (MAX_BUF+filt_len-1) uncorrected points
	    else
		read MAX_BUF uncorrected points
	    seek to -(block_number*MAX_BUF)
	    seek to 2+((SEQ_param.last_channel-channel)*uncorrected size)
	    filter the uncorrected buffer
	    compare the filtered points to the diagnostic points and
	       report any errors

	    size -= MAX_BUF
	    block_number++
	    copy the last filt_len-1 points from the end to the beginning,
	       adjust the buffer pointers
	}

/CODE
--------------------------------------------------------------------------*/
{   /* SEQ_diagnostic() */

register WORD i;
register SEQ_ACQ_DATA *dataP;
register WORD *corr1P,*corr2P;
    LONG block;
    LONG count;
    BYTE *corrP;
    WORD channel;
    LONG corr_size;
    LONG uncorr_size;
    WORD filt_len;
    WORD chan;
    BYTE last_chan;
    BYTE *buf1P,*buf2P;
    BYTE *old_baseP;
    WORD error_cnt;
    LONG total;
    
    block = 0;
    dataP = acq_dataP;
    chan = acq_dataP->channel;
    corr_size = size;
    uncorr_size = acq_dataP->array_size;
    last_chan = SEQ_params.last_channel[acq_dataP->plugin];
    filt_len = filt_dataP->paramsP->num_coeffs;
    SEQ_options.format = SEQ_FORMAT_CORRECTED;
    old_baseP = acq_dataP->baseP;
    total = size;

    corrP = (BYTE *)(malloc((size_t)(sizeof(WORD) * MAX_BUF_SIZE)));
    if (!corrP)
	error_handler(OUT_OF_MEMORY);

    SEQ_update_time(FALSE, 0L, 0L);

    while (size > 0)
    {
	/* seek to the last MAX_BUF_SIZE samples of corrected data and read */
	/* At this point seq_fP ALWAYS points to ch 1 of corrected data */
	dataP->byte_offset = chan * corr_size * sizeof(WORD);
	if (size > MAX_BUF_SIZE)
	    count = MAX_BUF_SIZE;
	else
	    count = size;

	/* On first block, filtering produces less than MAX_BUF_SIZE pts */
	/* Calculate pts after filtering to further limit count on first blk */
	if (block == 0)
	{
	    if (dataP->array_size > (LONG)MAX_BUF_SIZE)
		count = MAX_BUF_SIZE - (filt_len-1);
	    else if (count > (dataP->array_size-(filt_len-1)))
		count = dataP->array_size - (filt_len-1);
	}
	dataP->byte_offset += (LONG)(sizeof(WORD)*(size - count));
	dataP->size = (LONG)(sizeof(WORD) * count);
	dataP->bufP = corrP;
	dataP->baseP = corrP;

	SEQ_Read_Blocks_Seg(seq_fP, dataP, TRUE);

	/* Seek to the beginning of the uncorrected data, and read */
	dataP->byte_offset = (LONG)(
	    - dataP->byte_offset 		/* amt skipped */
	    - dataP->size			/* start of this chan */
	    - 2					/* diagnostic tag 0x4444 */
	    - (last_chan-chan+1)*uncorr_size);	/* start of uncorr chans */
	dataP->size = 0L;
	SEQ_Read_Blocks_Seg(seq_fP, dataP, FALSE);

	dataP->baseP = old_baseP;
	if (block > 0)
	{
	    dataP->size = (LONG)count;
	    dataP->byte_offset = (LONG)(block * MAX_BUF_SIZE);
	}
	else  /* block == 0 */
	{
	    if (dataP->array_size > (LONG)MAX_BUF_SIZE)
		dataP->size = (LONG)(MAX_BUF_SIZE);
	    else
		dataP->size = dataP->array_size;
	    dataP->bufP = dataP->baseP;
	    dataP->byte_offset = 0L;
	}

	/* Read MAX_BUF_SIZE+filt_len pts, correct to MAX_BUF_SIZE pts */
	SEQ_Process_Seg(seq_fP, block==0, dataP, filt_dataP, TRUE);

	if (block == 0)
	{
	    /* Seek to channel 1 of uncorrected data */
	    dataP->byte_offset = (LONG)(
		- dataP->size); 		 /* start of uncorr chan */
	    dataP->size = 0;
	    SEQ_Read_Blocks_Seg(seq_fP, dataP, FALSE);

	    dataP->byte_offset = (LONG)(
		(last_chan-chan+1)*uncorr_size	  /* start of corr chan tag */
		+ 2);				  /* diagnostic tag */
	}
	else
	{
	    /* Seek to channel 1 of uncorrected data */
	    dataP->byte_offset = (LONG)(
		- MAX_BUF_SIZE	 	 	 /* first block is larger */
		- dataP->size			 /* last block just read */
		- ((block-1) * MAX_BUF_SIZE)); 	 /* whole blocks in-between */
	    dataP->size = 0;
	    SEQ_Read_Blocks_Seg(seq_fP, dataP, FALSE);

	    dataP->byte_offset = (LONG)(
		(last_chan-chan+1)*uncorr_size	  /* start of corr chan tag */
		+ 2);				  /* diagnostic tag */
	}
	/* Seek to channel 1 of corrected data */
	SEQ_Read_Blocks_Seg(seq_fP, dataP, FALSE);

	/* Update the loop variables */
	size -= count;

	/* Compare corrected data block to our corrections */
	corr1P = filt_dataP->corrP;
	corr2P = (WORD *)corrP;
	error_cnt = 0;

	for (i=count-1; i >= 0; --i)
	{
	    /* printf("%04x\n", corr2P[i]); */
	    if (*corr1P++ != corr2P[i])
	    {
		if (error_cnt < 20)
		{
		    if (error_cnt == 0)
			printf("%c%d: Diagnostic test FAILED.\n", 
				acq_dataP->plugin+'A', chan+1);
		    printf("Corrected byte offset: %ld, ", 
			    2*((count-1-i)+(block * MAX_BUF_SIZE)));
		    printf("Expected: 0x%04x, Calculated: 0x%04x\n",
			    corr2P[i], *(corr1P-1));
		    ++error_cnt;
		}
		else
		{
		    printf("Too many differences...terminating\n");
		    EXIT
		}
	    }
	}

	block++;

	SEQ_update_time(TRUE, total, size);

    }  /* WHILE */

    /* If we made it here, all samples compared correctly */
    printf("PASSED.\n");

    /* Free the allocated memory */
    free(corrP);

}

/*XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX*/

VOID SEQ_91_diagnostic(seq_fP, acq_dataP, filt_dataP, size)
    FILE  	  *seq_fP;
    SEQ_ACQ_DATA  *acq_dataP;
    SEQ_FILTER_DATA *filt_dataP;
    LONG	  size;

/*--------------------------------------------------------------------------

    Purpose: To compare the corrected diagnostic data with the output of
		SEQ_fir and print any differences. This routine is only
		for 7291 data.

    Inputs: acq_dataP = acquisition parameters, data size, channel, etc
			acq_dataP->block offset it set to get the first
			sample in the diagnostic block.
	    filt_dataP = parameters associated with filtering the data.
	    size = size of correction data in diagnostic block (WORDs)

    Outputs: Filters the uncorrected data in the previous segment and
	     compares it to the diagnostic block.

    Machine dependencies: 
			 
    Notes: The diagnostic block consists of 16-bit WORDs in Intel format
	   (Low/High).

	   The 7291 data in the diagnostic block is not reversed and appears
	   in correct order.

	   The diagnostic block is preceeded with 0x4444 ("DD"). This extra
	   word must be accounted for when searching through memory.

	   The seq_fP is pointing to the first corrected WORD for channel 1
	   for this plugin; acq_dataP->channel determines which channel
	   should be tested.

    Procedure:

/CODE
--------------------------------------------------------------------------*/
{   /* SEQ_91_diagnostic() */

register WORD i;
register SEQ_ACQ_DATA *dataP;
register WORD *corr1P,*corr2P;
    LONG block;
    LONG count;
    LONG total_read;
    BYTE *corrP;
    LONG corr_size;
    LONG uncorr_size;
    BYTE *buf1P,*buf2P;
    BYTE *old_baseP;
    WORD error_cnt;
    LONG total;
    
    block = 0L;
    dataP = acq_dataP;
    corr_size = size;
    uncorr_size = acq_dataP->array_size;
    SEQ_options.format = SEQ_FORMAT_CORRECTED;
    total_read = 0L;
    old_baseP = acq_dataP->baseP;
    total = size;

    corrP = (BYTE *)(malloc((size_t)(sizeof(WORD) * MAX_BUF_SIZE)));
    if (!corrP)
	error_handler(OUT_OF_MEMORY);

    SEQ_update_time(FALSE, 0L, 0L);

    while (size > 0)
    {
	/* Read the first MAX_BUF_SIZE samples of corrected data. */
	/* At this point seq_fP ALWAYS points to ch 1 of corrected data */
	dataP->byte_offset = (LONG)(sizeof(WORD) * total_read);
	if (size > MAX_BUF_SIZE)
		count = MAX_BUF_SIZE;
	else
	    count = size;

	/* On first block, filtering produces less than MAX_BUF_SIZE pts */
	/* Calculate pts after filtering to further limit count on first blk */
	if (block == 0)
	{
	    if (dataP->array_size > (LONG)MAX_BUF_SIZE)
		count = MAX_BUF_SIZE 
			- (2*(filt_dataP->paramsP->num_coeffs-1))
		   	- (filt_dataP->paramsP->num_91coeffs-1);
	    else
		count = dataP->array_size 
			- (2*(filt_dataP->paramsP->num_coeffs-1))
			- (filt_dataP->paramsP->num_91coeffs-1) - 2;
	}
	dataP->size = (LONG)(sizeof(WORD) * count);
	dataP->bufP = corrP;
	dataP->baseP = corrP;

	SEQ_Read_Blocks_Seg(seq_fP, dataP, TRUE);

	/* Seek to the beginning of the uncorrected data, and read */
	dataP->byte_offset = (LONG)(
	    - dataP->byte_offset 		/* amt skipped */
	    - dataP->size			/* start of this chan */
	    - 2					/* diagnostic tag 0x4444 */
	    - uncorr_size);			/* start of uncorr data */
	dataP->size = 0L;
	SEQ_Read_Blocks_Seg(seq_fP, dataP, FALSE);

	dataP->baseP = old_baseP;
	if (block > 0)
	{
	    dataP->size = (LONG)count;
	    dataP->byte_offset = (LONG)(block * MAX_BUF_SIZE);
	}
	else  /* block == 0 */
	{
	    if (dataP->array_size > (LONG)MAX_BUF_SIZE)
		dataP->size = (LONG)(MAX_BUF_SIZE);
	    else
		dataP->size = dataP->array_size;
	    dataP->bufP = dataP->baseP;
	    dataP->byte_offset = 0L;
	}

	/* Read dataP->size pts, correct to MAX_BUF_SIZE pts */
	SEQ_Process_Seg(seq_fP, block==0, dataP, filt_dataP, TRUE);

	if (block == 0)
	{
	    /* Seek to start of uncorrected data */
	    dataP->byte_offset = (LONG)(
		- dataP->size); 		 /* start of uncorr chan */
	    dataP->size = 0;
	    SEQ_Read_Blocks_Seg(seq_fP, dataP, FALSE);

	    dataP->byte_offset = (LONG)(
		uncorr_size	  		/* start of corr chan tag */
		+ 2);				/* diagnostic tag */
	}
	else
	{
	    /* Seek to channel 1 of uncorrected data */
	    dataP->byte_offset = (LONG)(
		- MAX_BUF_SIZE	 		 /* first block is larger */
		- dataP->size			 /* last block just read */
		- ((block-1) * MAX_BUF_SIZE)); 	 /* whole blocks in-between */
	    dataP->size = 0;
	    SEQ_Read_Blocks_Seg(seq_fP, dataP, FALSE);

	    dataP->byte_offset = (LONG)(
		uncorr_size	  		/* start of corr chan tag */
		+ 2);				/* diagnostic tag */
	}
	/* Seek to start of corrected data */
	SEQ_Read_Blocks_Seg(seq_fP, dataP, FALSE);

	/* Update the loop variables */
	size -= count;

	/* Compare corrected data block to our corrections */
	corr1P = filt_dataP->corrP;
	corr2P = (WORD *)corrP;
	error_cnt = 0;

	for (i=0; i < count; ++i)
	{
	    if (corr1P[i] != corr2P[i])
	    {
		if (error_cnt < 20)
		{
		    if (error_cnt == 0)
			printf("%c1: Diagnostic test FAILED.\n", 
				    acq_dataP->plugin+'A');
		    printf("Corrected byte offset: %ld, ", 
				2*(i+total_read));
		    printf("Expected: 0x%04x, Calculated: 0x%04x\n",
				corr2P[i], corr1P[i]);
		    ++error_cnt;
		}
		else
		{
		    printf("Too many differences...terminating\n");
		    EXIT
		}
	    }
	}

	block++;
	total_read += count;

	SEQ_update_time(TRUE, total, size);

    }  /* WHILE */

    /* If we made it here, all samples compared correctly */
    printf("PASSED.\n");

    /* Free the allocated memory */
    free(corrP);

}

/*XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX*/

VOID SEQ_update_time(update, total, left)
    BOOL update;
    LONG total;
    LONG left;

/*--------------------------------------------------------------------------

    Purpose: To update the time for very long records so the user knows
		that the the program hasn't died but is just taking a
		long time to correct/check the record.

    Inputs: update = FALSE to initialize the starting time.
		   = TRUE to update the screen with the percentage done.
	    total  = total bytes that must be corrected
	    left   = number of bytes left to correct

    Outputs: Prints the percentage of the record done so far.

    Machine dependencies: 
			 
    Notes:

    Procedure:

/CODE
--------------------------------------------------------------------------*/
{   /* SEQ_update_time() */

    FLOAT percentage;
    struct dostime_t time;
    static WORD  old_time;

    if (update == FALSE)
    {
	_dos_gettime(&time);
	old_time = time.second;
    }
    else  /* Update the percentage done so far */
    {
	_dos_gettime(&time);
	if (time.second < old_time)
	{
	    if ((time.second+60-old_time) > MAX_TIME)
	    {
		old_time = time.second;
		percentage = (FLOAT)(
		   ((FLOAT)(total - left) / (FLOAT)total) * 100.0);
		fprintf(stderr, "%d%%\r", (WORD)percentage);
	    }
	}
	else if ((time.second - old_time) > MAX_TIME)
	{
	    old_time = time.second;
	    percentage = (FLOAT)(
	       ((FLOAT)(total - left) / (FLOAT)total) * 100.0);
	    fprintf(stderr, "%d%%\r", (WORD)percentage);
	}
    }
}
#endif /* RIS */

/*XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
        Following are functions which access the components of a waveform 
	    descriptor in accordance with the machine template.

        PCW_Load_Block_Offsets(templateP,waveformP) is passed pointers to
            the machine template and the waveform descriptor.  It
            initializes local data which is used by the remaining functions
            in this file (which are called by the print functions).
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX*/



/*XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX*/

struct PCW_BLOCK *PCW_Find_Block(name,type,templateP)
    CHAR                    *name;
    enum    PCW_BLOCK_TYPES type;
    struct  PCW_TEMPLATE    *templateP;

/*--------------------------------------------------------------------------
    Purpose: Searches the machine template to find a block with the specified
             name and type; returns a pointer to the Block structure found.

    Inputs:  name is the character string name of the block to be found
             type is the type of the block (a Block_Types enumerated value)
             templateP is a pointer to the machine template to be searched

    Outputs: If a block with the specified name and type is found in the
             machine template, the return value is a pointer to the Block
             structure; otherwise the return value is a NULL pointer.

    Machine dependencies:

    Notes:

    Procedure:

/CODE
--------------------------------------------------------------------------*/

{   /* Find_Block */
    struct  PCW_BLOCK   *blockP;

    blockP = templateP->BlockListP;
    while (*blockP->Name)           /* null name marks end of block list */
    {
        if (!strcmp(blockP->Name,name) && blockP->Type == type)
            return blockP;
        else
            blockP++;
    }

    return NULL;
}




/*XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX*/

struct PCW_VARIABLE *PCW_Find_Variable(name,blockP)
    CHAR                *name;
    struct  PCW_BLOCK   *blockP;

/*--------------------------------------------------------------------------
    Purpose: Searches the variable list for a given block in the machine
             template to find a variable with the specified name; returns
             a pointer to the Variable structure found.

    Inputs: name is the character string name of the variable to find
            blockP is a pointer to the Block structure of the block in which
                the variable is to be found

    Outputs: If the specified variable is found in the given block, the return
             value is a pointer to the Variable structure; otherwise the return
             value is a NULL pointer.

    Machine dependencies:

    Notes:

    Procedure:

/CODE
--------------------------------------------------------------------------*/

{   /* Find_Variable */
    struct  PCW_VARIABLE    *varP;

    varP = blockP->VarListP;
    blockP++;                       /*---------------------------------------- 
                                       End of variable array for this block
                                       is indicated by reaching address 
                                       pointed to by VarListP of next block
                                    -----------------------------------------*/
    while (varP != blockP->VarListP && strcmp(varP->Name,name))
        varP++;

    if (varP == blockP->VarListP)
        return NULL;
    else
        return varP;
}




/*XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX*/

LONG    PCW_Find_Block_Offset(blockP)
    struct  PCW_BLOCK   *blockP;

/*--------------------------------------------------------------------------
    Purpose: Finds the offset in the waveform descriptor of a block with 
             a specified name and block type.

    Inputs:  blockP is a pointer to the Block structure (in the machine
                template) for the block to be found; this Block structure
                contains the block name and block type

    Outputs: If the specified block is present in the waveform descriptor,
             the return value is the offset of the start of the block in
             the waveform descriptor; otherwise the return value is
             STATUS_ERR (-1).

    Machine dependencies:

    Notes:   The offset of the block is retrieved from the regular_blocks
             array (if the block type is Regular) or the array_blocks
             array (if the block type is Array).

    Procedure:

/CODE
--------------------------------------------------------------------------*/

{   /* Find_Block_Offset */
    INT index;

    if (blockP->Type == PCW_Regular)
    {
        for (index=0; index < NUM_REG_BLOCKS; index++)
        {
            if (!strcmp(blockP->Name,regular_blocks[index].Name))
                return regular_blocks[index].Offset;
        }
    }
    else
    {
        for (index=0; index < NUM_ARRAYS; index++)
        {
            if (!strcmp(blockP->Name,array_blocks[index].Name))
                    return array_blocks[index].Array_Offset;
        }
    }

    return STATUS_ERR;
}




/*XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX*/

STATUS  PCW_Find_Array_Dims(array_name,length,count)
    CHAR    *array_name;
    LONG    *length;
    LONG    *count;

/*--------------------------------------------------------------------------
    Purpose: Finds the length in bytes and the count (number of elements) for
             an array with a specified name in the waveform descriptor.

    Inputs:  array_name is the character string name of the array
             length is the length to be returned
             count is the count to be returned

    Outputs: length is the length in bytes of the array in the waveform
                descriptor
             count is the count of the number of elements in the array
             If the specified array is present in the waveform, the return 
                value is STATUS_OK; otherwise the return value is STATUS_ERR.

    Machine dependencies:

    Notes:   The length and count are retrieved from the array_blocks array

    Procedure:

/CODE
--------------------------------------------------------------------------*/

{   /* Find_Array_Dims */
    INT index;
    
    for (index = 0; index < NUM_ARRAYS; index++)
    {
        if (!strcmp(array_name,array_blocks[index].Name))
        {
            *length = array_blocks[index].Array_Length;
            *count = array_blocks[index].Array_Count;
            return STATUS_OK;
        }
    }

    return STATUS_ERR;
}

    


/*XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX*/

VOID    *PCW_Find_Value(waveformP,block_offset,varP)
    PCW_WAVEFORM            waveformP;
    LONG                    block_offset;
    struct  PCW_VARIABLE    *varP;

/*--------------------------------------------------------------------------
    Purpose: Returns a pointer to the location in the waveform descriptor
             of a variable given a pointer to the start of the waveform
             descriptor, the offset of the block in which the variable is
             located, and a pointer to the Variable structure (in the
             machine template) for the variable.

    Inputs:  waveformP is a pointer to the start of the waveform descriptor
             block_offset is the offset of the appropriate block from the
                start of the waveform descriptor
             varP is a pointer to the Variable structure in the machine
                template for the variable; this contains the offset of the
                variable from the start of the block

    Outputs: Returns a pointer to the location in the waveform descriptor
             of the specified variable.

    Machine dependencies:

    Notes:   Returns a pointer to VOID because the type of the variable may
             vary.

    Procedure:

/CODE
--------------------------------------------------------------------------*/

{   /* Find_Value */
    return (VOID *)(waveformP + block_offset + varP->Offset);
}

    


/*XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX*/

VOID    *PCW_Find_Value_From_Name(waveformP,block_offset,blockP,var_name)
    PCW_WAVEFORM        waveformP;
    LONG                block_offset;
    struct  PCW_BLOCK   *blockP;
    CHAR                *var_name;

/*--------------------------------------------------------------------------
    Purpose: Returns a pointer to the location in the waveform descriptor
             of a variable given a pointer to the start of the waveform
             descriptor, the offset of the block in which the variable is
             located, a pointer to the Block structure (in the machine
             template), and the name of the variable.

    Inputs:  waveformP is a pointer to the start of the waveform descriptor
             block_offset is the offset from the start of the waveform
                descriptor of the block in which the variable is located
             blockP is a pointer to the Block structure in the machine 
                template for the block
             var_name is the character string name of the variable

    Outputs: If the specified variable exists in the block, returns a pointer 
             to the location in the waveform descriptor of the variable;
             otherwise returns a NULL pointer.

    Machine dependencies:

    Notes:   Returns a pointer to VOID because the type of the variable may
             vary.

    Procedure:

/CODE
--------------------------------------------------------------------------*/

{   /* PCW_Find_Value_From_Name */
    struct  PCW_VARIABLE    *varP;

    varP = PCW_Find_Variable(var_name,blockP);
    if (varP == NULL)
        return NULL;

    return (VOID *)(waveformP + block_offset + varP->Offset);
}




/*XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX*/

CHAR *PCW_Find_Enumeration(enum_value,varP)
    UWORD                   enum_value;
    struct  PCW_VARIABLE    *varP;

/*--------------------------------------------------------------------------
    Purpose: Returns a pointer to the character string enumerated value name
             corresponding to the specified enumerated value for the specified
             variable.

    Inputs:  enum_value is the integral enumerated value
             varP is a pointer to the Variable structure in the machine
                template for the variable

    Outputs: Return value is a pointer to the appropriate character string
                enumerated value name.

    Machine dependencies:

    Notes:  No range checking is done on the enumerated value

    Procedure:

/CODE
--------------------------------------------------------------------------*/

{   /* Find_Enumeration */
    CHAR    *enumP;
    INT     count;

    enumP = varP->EnumListP;
    for (count=0; count < enum_value; count++)
        enumP += NAMELENGTH;

    return enumP;
}




/*XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX*/

VOID PCW_Load_Block_Offsets(templateP,waveformP)
    struct  PCW_TEMPLATE    *templateP;
    PCW_WAVEFORM            waveformP;

/*--------------------------------------------------------------------------
    Purpose: Loads the regular_blocks and array_blocks arrays with
             offsets and array length information for a waveform descriptor.
             The regular_blocks array contains the name and offset within
             the waveform descriptor of each regular block in the descriptor.  
             The array_blocks array contains the name, offset within the
             waveform descriptor, length in bytes, and element count of
             each array in the waveform descriptor.
             This information is obtained from the appropriate variables in
             the WAVE_DESC block of the waveform descriptor.
             If a regular block or array is not present in the waveform 
             descriptor (i.e., if the corresponding length variable in the
             WAVE_DESC block of the descriptor is 0), the name field of
             the corresponding record in the regular_blocks or 
             array_blocks array is set to null.

    Inputs:  templateP is a pointer to the machine template
             waveformP is a pointer to the waveform descriptor

    Outputs: Loads the regular_blocks and array_blocks arrays as
             described above.

    Machine dependencies:

    Notes:   Because the information describing each block and array is
             obtained from variables with certain names in the WAVE_DESC
             block of the waveform descriptor, this function must be changed
             if the names or meanings of these variables change.  The
             variables are:  WAVE_DESC, USER_TEXT, WAVE_ARRAY_1, WAVE_ARRAY_2,
             TRIG_TIME_ARRAY, RIS_TIME_ARRAY, RES_ARRAY1, RES_ARRAY2,
             WAVE_ARRAY_COUNT, TRIG_TIME_COUNT, RIS_TIME_COUNT, 
             RES_ARRAY1_COUNT, RES_ARRAY2_COUNT.

    Procedure:

/CODE
--------------------------------------------------------------------------*/

{   /* Load_Block_Offsets */
    struct  PCW_BLOCK       *blockP;
    LONG                    *lengthP;
    LONG                    *countP;

    blockP = PCW_Find_Block("WAVEDESC",PCW_Regular,templateP);

    strcpy(regular_blocks[0].Name,"WAVEDESC"); 
    regular_blocks[0].Offset = 0;

    lengthP = (LONG *)PCW_Find_Value_From_Name
                                (waveformP,(LONG)0,blockP,"WAVE_DESCRIPTOR");
    regular_blocks[1].Offset = *lengthP;



    lengthP = (LONG *)PCW_Find_Value_From_Name(waveformP,(LONG)0,blockP,
                                               "USER_TEXT");
    if (*lengthP == 0)
        regular_blocks[1].Name[0] = '\0';
    else
        strcpy(regular_blocks[1].Name,"USERTEXT");

    array_blocks[0].Array_Offset = *lengthP +
                                    regular_blocks[1].Offset;
    countP = 
        (LONG *)PCW_Find_Value_From_Name(waveformP,(LONG)0,blockP,
                                         "SUBARRAY_COUNT");
    array_blocks[0].Array_Count = *countP;


    lengthP = (LONG *)PCW_Find_Value_From_Name
                                (waveformP,(LONG)0,blockP,"TRIGTIME_ARRAY");
    array_blocks[0].Array_Length = *lengthP;
    if (*lengthP == 0)
        array_blocks[0].Name[0] = '\0';
    else
        strcpy(array_blocks[0].Name,"TRIGTIME");

    array_blocks[1].Array_Offset = *lengthP +
                                    array_blocks[0].Array_Offset;
    countP = 
        (LONG *)PCW_Find_Value_From_Name(waveformP,(LONG)0,blockP,
                                         "SWEEPS_PER_ACQ");
    array_blocks[1].Array_Count = *countP;


    lengthP = (LONG *)PCW_Find_Value_From_Name
                                (waveformP,(LONG)0,blockP,"RIS_TIME_ARRAY");
    if ((lengthP == NULL) || (*lengthP == 0))
    {
        array_blocks[1].Name[0] = '\0';
        array_blocks[1].Array_Length = 0;
        array_blocks[2].Array_Offset = array_blocks[1].Array_Offset;
    }
    else
    {
        strcpy(array_blocks[1].Name,"RISTIME");
        array_blocks[1].Array_Length = *lengthP;
        array_blocks[2].Array_Offset = *lengthP + array_blocks[1].Array_Offset;
    }

    countP = 
        (LONG *)PCW_Find_Value_From_Name(waveformP,(LONG)0,blockP,
                                         "WAVE_ARRAY_COUNT");
    array_blocks[2].Array_Count = *countP;


    lengthP = (LONG *)PCW_Find_Value_From_Name
                                (waveformP,(LONG)0,blockP,"WAVE_ARRAY_1");
    array_blocks[2].Array_Length = *lengthP;
    if (*lengthP == 0)
        array_blocks[2].Name[0] = '\0';
    else
        strcpy(array_blocks[2].Name,"WAVE_ARRAY_1");

    array_blocks[3].Array_Offset = *lengthP +
                                        array_blocks[2].Array_Offset;
    array_blocks[3].Array_Count = *countP;


    lengthP = (LONG *)PCW_Find_Value_From_Name
                                (waveformP,(LONG)0,blockP,"WAVE_ARRAY_2");
    array_blocks[3].Array_Length = *lengthP;
    if (*lengthP == 0)
        array_blocks[3].Name[0] = '\0';
    else
        strcpy(array_blocks[3].Name,"WAVE_ARRAY_2");
}




/*XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX*/

VOID PCW_Check_Table()

/*--------------------------------------------------------------------------
    Purpose: Used for debugging purposes.
             Prints out the contents of the regular_blocks and 
             array_blocks arrays.

    Inputs:

    Outputs:

    Machine dependencies:

    Notes:

    Procedure:

/CODE
--------------------------------------------------------------------------*/

{   /* Check_Table */
    INT     index;

    printf("\nName  Offset");
    for (index=0; index < NUM_REG_BLOCKS; index++)
    {
        printf("\n%s %ld",regular_blocks[index].Name,
                             regular_blocks[index].Offset);
    }

    printf("\nName  Offset  Length  Count");
    for (index=0; index < NUM_ARRAYS; index++)
    {
        printf("\n%s %ld %ld %ld",array_blocks[index].Name,
                             array_blocks[index].Array_Offset,
                             array_blocks[index].Array_Length,
                             array_blocks[index].Array_Count);
    }
}

/*----------------------------- end of file -------------------------------*/

