/************************** seq_tran.c *************************************

Author: Christopher Eck		LeCroy Corporation
	             		700 Chestnut Ridge Road
				Chestnut Ridge, NY  10977

This program reads a data file that was previously acquired from a 7234
or 7242B in SCSI sequence mode and corrects the raw data as the 7200A
would have done under normal conditions. The corrected waveforms are
written to a file or printed to the screen. The waveform files can be
read by the 7200A and examined using the built-in processing capabilities
of the 7200A.

 **********************************************************************/


#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <dos.h>
#include <math.h>
#include "seq_tran.h"
#include "seq_filt.h"
#include "seq_hdr.h"

/* -------------------------------------------------------------------- */

extern CHAR   *PCW_translate_descriptor();
extern STATUS PCW_Print_Block();
extern CHAR   *SEQ_process_arguments();
extern VOID   SEQ_read_filter_coefficients();
extern VOID   SEQ_fir();
extern VOID   SEQ_fir_7291();
extern VOID   SEQ_correct_data();
extern VOID   SEQ_interpret();
extern BOOL   SEQ_Read_Blocks_Desc();
extern BOOL   SEQ_Read_Blocks_Seg();
extern VOID   seq_print_time();
extern LONG   SEQ_Read_Desc();
extern VOID   SEQ_Read_Segment_Number();
extern BOOL   SEQ_Process_Seg();
extern VOID   SEQ_Output_Seg();
extern VOID   SEQ_diagnostic();
extern VOID   SEQ_91_diagnostic();
extern BOOL   SEQ_Check_Seg();

/* -------------------------------------------------------------------- */

#define MAX_TIME 3

/* Global Variables. */

struct FILTER 	    SEQ_filter[MAX_PLUGINS][MAX_CHANNELS]; 
struct PCW_BLOCK    *PCW_blockP[MAX_PLUGINS][MAX_CHANNELS];
BYTE   		    *PCW_waveformP[MAX_PLUGINS][MAX_CHANNELS];
struct PCW_TEMPLATE *PCW_templateP = NULL;

struct SEQ_OPTIONS  SEQ_options;
struct SEQ_PARAMS   SEQ_params;

WORD SEQ_desc_size;    /* size in BYTEs of the waveform descriptor */
FILE *out_fP = NULL;

#undef RESOLUTION_1_PSEC
#undef TESTING_HARK_PROBLEM

#ifdef TESTING_HARK_PROBLEM
#define MAX_TIMES 128         /* must be a multiple of 2 */
DOUBLE rel_time[2][MAX_TIMES];
DOUBLE trig_time[2][MAX_TIMES];
#endif /* TESTING_HARK_PROBLEM */

/* -------------------------------------------------------------------- */

/* Memory allocation for waveform descriptors */
BYTE PCW_Waveform[MAX_PLUGINS][MAX_CHANNELS][800];

/*XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX*/

VOID main(ac, av)
    int ac;
    char *av[];

/*--------------------------------------------------------------------------

    Purpose: Displays sign-on message, processes any switch arguments and 
		calls the driver routine to transfer and process the SCSI
		data.

    Inputs: ac = number of arguments (switches)
	    av = the switches as ASCII strings

    Outputs: Calls sink(), the driver that does all the work.
	     Turns off the SCSI interrupts on exit to restore proper PC
	       operation.

    Machine dependencies: IBM PC or compatible

    Notes: Can define notdef to display the number of SCSI interrupts
	     that were processed; there should be an interrupt for every
	     block transfer of data.

    Procedure:

/CODE
--------------------------------------------------------------------------*/
{   /* main() */

    FILE *seq_fP;
    CHAR *seq_filenameP;
    BYTE p,c;

    /* Print program description */
	fprintf(stderr, 
  "LeCroy 7200A SCSI Sequence Data Translator (Version 1.1, Feb 19, 1993).\n");

    /* Process all the arguments to this routine */
	seq_filenameP = SEQ_process_arguments(ac, &av[0]);

    /* Open the specified data file */
	if ((seq_fP = fopen(seq_filenameP,"rb")) == NULL)
	{
	    printf("Could not open file %s\n", seq_filenameP);
	    EXIT
	}

    /* initialize the acquisition parameters in descriptor */
	SEQ_interpret(seq_fP, SEQ_INIT_PARAMETERS);

    /* initialize the descriptor for all channels */
	SEQ_interpret(seq_fP, SEQ_READ_DESCRIPTOR);

    /* If the acquisition parameters should be displayed, print them out */
	for (p=SEQ_params.first_plugin; p <= SEQ_params.last_plugin; ++p)
	{
	    for (c=0; c <= SEQ_params.last_channel[p]; ++c)
	    {
		if (SEQ_options.print_params[p][c] == TRUE)
		{
		    printf("\nDESCRIPTOR FOR %c%d:\n",p+'A',c+1);
		    (VOID)PCW_Print_Block(stdout,PCW_templateP,PCW_waveformP
		       [p][c],"WAVEDESC",PCW_Regular,TRUE,2,0,(LONG)0,(LONG)0);
		}
	    }
	}

    /* Translate the data requested (# segments, segs after a time, ...) */
	SEQ_interpret(seq_fP, SEQ_READ_SEGMENT_NUMBER);

    /* Close the file before ending program */
	fclose(seq_fP);

    exit (0);
}


/*XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX*/

VOID SEQ_interpret(seq_fP, action)
  FILE  *seq_fP;
  BYTE  action;

/*--------------------------------------------------------------------------

    Purpose: To interpret the descriptor or data.

    Inputs: seq_fP = FILE pointer to the opened data file

	    action = SEQ_INIT_PARAMETERS 
		     SEQ_READ_DESCRIPTOR
		     SEQ_READ_SEGMENT_NUMBER
		     SEQ_READ_SEGMENT_TIME

    Outputs: 

    Machine dependencies: 
			 
    Notes: It is assumed that the data file has been opened for reading.

    Procedure:

/CODE
--------------------------------------------------------------------------*/
{   /* SEQ_interpret() */

extern BYTE plugin_field;
extern BYTE channel_field[MAX_PLUGINS];

    INT   num_read;
    UWORD packet;
    UWORD num_chan;
    BOOL  plugin_a = FALSE;
    BOOL  plugin_b = FALSE;
    UWORD other_plugin;
    BYTE  p,c;
    UWORD plugin;
    WORD  search_size;
    SEGS  *segP;

    if (action == SEQ_INIT_PARAMETERS)
    {

	/******************************** NOTE  ************************

	   If 2 plugins present, find a way to determine the end of the
	   desc data without scanning through the entire document since
	   the descriptors will ALWAYS precede the data so if the other
	   plugin's descriptor is not found (packet = 0), before one
	   plugins's segment is found, then no need to search further.

	 ****************************************************************/

	/* Initialize block size, plugins present, chan/plugin, descriptors */
	fseek(seq_fP,0L,SEEK_SET);

	num_read = fread((CHAR *)&packet,sizeof(UWORD),1,seq_fP);

	if ((packet & 0x7fff) != 0)
	{
	    /* If the first block is not 0, can't interpret anything */
	    printf("Invalid data file.\n");
	    EXIT
	}

	/* MSB of packet number describes which plugin was present */
	plugin = ((packet & 0x8000) >> 15);
	if (plugin)
	{
	    plugin_b = TRUE;
	    other_plugin = 0x0000;
	}
	else
	{
	    plugin_a = TRUE;
	    other_plugin = 0x8000;
	}

	/* Read the block size */
	num_read = fread((CHAR *)&SEQ_params.block_size,sizeof(ULONG),1,seq_fP);

	/* Read the number of channels */
	num_read = fread((CHAR *)&num_chan, sizeof(UWORD),1,seq_fP);
	SEQ_params.last_channel[plugin] = num_chan - 1;

	/* Read the TMB block size */
	num_read = fread((CHAR *)&SEQ_params.tmb_block_size[plugin], 
					sizeof(UWORD),1,seq_fP);

	/* Search through the rest of the file looking for the presence of
	 * the other plugin. If found, initialize its parameters. Assuming
	 * a max descriptor size plus filter coeff size of 600, don't search 
	 * beyond num_chan*600 bytes for the other plugin.
	 */
	fseek(seq_fP, (LONG)sizeof(UWORD), SEEK_SET);
	search_size = num_chan * 600;
	while(search_size > 0)
	{
	    fseek(seq_fP, (LONG)(SEQ_params.block_size-2), SEEK_CUR);
	    search_size -= (SEQ_params.block_size-2);
	    num_read = fread((CHAR *)&packet, sizeof(UWORD), 1, seq_fP);
	    if (num_read == 0)
		break;
	    else if ((packet & 0x8000) == other_plugin)
	    {
		/* Found the other plugin, mainframe has both plugins present */
		SEQ_params.first_plugin = 0;
		SEQ_params.last_plugin = 1;
		plugin = (other_plugin >> 15);

		/* Skip over the block size (the same for both plugins) */
		fseek(seq_fP, (LONG)sizeof(ULONG), SEEK_CUR);

		/* Read the number of channels for this plugin */
		num_read = fread((CHAR *)&num_chan, sizeof(UWORD),1,seq_fP);
		SEQ_params.last_channel[plugin] = num_chan - 1;
		/* Read the TMB block size */
		num_read = fread((CHAR *)&SEQ_params.tmb_block_size[plugin], 
						sizeof(UWORD),1,seq_fP);
		break;
	    }
	    if (SEQ_options.debug == 1)
		printf("Checking packet 0x%04x\n", packet);
	}

	if ((num_read == 0) || (search_size < 0))
	{
	    /* Only 1 plugin present, setup variables accordingly */
	    SEQ_params.first_plugin = plugin;
	    SEQ_params.last_plugin = plugin;
	}

	/* Print statistics */
	if (SEQ_options.debug == 1)
	{
	    printf("First plugin = %c\n", SEQ_params.first_plugin+'A');
	    printf("Last plugin = %c\n", SEQ_params.last_plugin+'A');
	    printf("Block size = %ld\n", SEQ_params.block_size);
	    printf("Channels present:\n");
	    for (p=SEQ_params.first_plugin; p <= SEQ_params.last_plugin; ++p)
		printf("%c:%d  TMB Block Size: %d\n",p+'A', 
		    SEQ_params.last_channel[p]+1, SEQ_params.tmb_block_size[p]);
	}
    }
    else if (action == SEQ_READ_DESCRIPTOR)
    {
	/* Read the descriptor plus filter coefficients for all channels */
	SEQ_params.seg_offset = 0;
	for (p=SEQ_params.first_plugin; p <= SEQ_params.last_plugin; ++p)
	{
	    SEQ_params.seg_offset += SEQ_Read_Desc(seq_fP, p);
	}

    }
    else if (action == SEQ_READ_SEGMENT_NUMBER)
    {
	/* Determine if the user's segs have been acquired */
	for (p=0; p < MAX_PLUGINS; ++p)
	{
	    if (plugin_field & (1 << p))
	    {
		if ((p < SEQ_params.first_plugin) ||
		    (p > SEQ_params.last_plugin))
		{
		    fprintf(stderr, "Plugin %c never acquired.\n", p+'A');
		    EXIT
		}
	    }
	    for (c=0; c < MAX_CHANNELS; ++c)
	    {
		if (channel_field[p] & (1 << c))
		{
		    if (c > SEQ_params.last_channel[p])
		    {
			fprintf(stderr, "Channel %d never acquired.\n", c+1);
			EXIT
		    }
		}
	    }
	}

	/* Determine which segment to translate */
	for (p=SEQ_params.first_plugin; p <= SEQ_params.last_plugin; ++p)
	{
	    SEQ_Read_Segment_Number(seq_fP, p, SEQ_ALL_SEGS);
	}
    }
}

/*XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX*/

LONG SEQ_Read_Desc(seq_fP, plugin)
  FILE  *seq_fP;
  CHAR  plugin;

/*--------------------------------------------------------------------------

    Purpose: For all plugins present, read the descriptors for all channels
		and initialize all blocks of the descriptor based on the
		7200.tpl template file.

    Inputs: plugin = 0 for plugin A
		   = 1 for plugin B
	    
	    PCW_Waveform[plugin][channel] = buffer to copy data

    Outputs: Returns the number of bytes used to contain the descriptors
		for all the channels of this plugin.

    Machine dependencies: 
			 
    Notes: It is assumed that the data file has been opened for reading.
	   The first block of each plugin is special because of the pre-amble 
	   information (block size and number of channels). We need to read 
	   enough of first descriptor to call the PCW package. Therefore, we 
	   will read 400 bytes which is more than enough. Then we can determine
	   the real size of the descriptor in order to read the filter
	   coefficients and the rest of the descriptors for the other
	   channels. The data for the first descriptor begins 6 bytes into
	   the data file (4 for block size, 2 for num channels).
	   

    Procedure:

/CODE
--------------------------------------------------------------------------*/
{   /* SEQ_Read_Desc() */

    BYTE  c;
    LONG  *desc_sizeP;
    LONG  *time_sizeP;
    BYTE  *filter_bufferP;
    LONG  num_packets;
    LONG  blocks;
    LONG  offset;

    /* Starting offset is 8: SCSI block size(4), # chan(2), TMB blk size(2) */
    offset = 8;

    for (c=0; c <= SEQ_params.last_channel[plugin]; ++c)
    {
	if (SEQ_Read_Blocks_Desc(seq_fP, PCW_Waveform[plugin][c],
		    plugin, 400L, (LONG)(offset)) == FALSE)
	{
	    printf("Trouble reading %c%d waveform descriptor.\n",
				plugin+'A', c+1);
	    EXIT
	}

	PCW_waveformP[plugin][c]= PCW_translate_descriptor 
			(PCW_Waveform[plugin][c], &PCW_blockP[plugin][c]);

	desc_sizeP = (LONG *)PCW_Find_Value_From_Name(PCW_waveformP[plugin][c],
		      (LONG)0, PCW_blockP[plugin][c], "WAVE_DESCRIPTOR");
	time_sizeP = (LONG *)PCW_Find_Value_From_Name(PCW_waveformP[plugin][c],
		      (LONG)0, PCW_blockP[plugin][c], "TRIGTIME_ARRAY");

	filter_bufferP = (BYTE *)(malloc((size_t)(*time_sizeP)));
	if (!filter_bufferP)
	    error_handler(OUT_OF_MEMORY);

	offset += *desc_sizeP;
	if (SEQ_Read_Blocks_Desc(seq_fP, filter_bufferP, plugin,
			*time_sizeP, (LONG)(offset)) == FALSE)
	{
	    printf("Trouble reading %c%d filter coefficients.\n",
				plugin+'A', c+1);
	    EXIT
	}

	if (SEQ_options.debug == 1)
	{
	    printf("Desc size = %ld, Time Array Size = %ld\n",
			*desc_sizeP, *time_sizeP);
	}

	/* Initialize the filter coefficient array */
	SEQ_read_filter_coefficients(filter_bufferP, &SEQ_filter[plugin][c]);

	offset += *time_sizeP;

	free(filter_bufferP);

    }

    /* Save the descriptor size: same for all plugins */
    SEQ_desc_size = (WORD)*desc_sizeP;

    /* Now determine how many blocks were required to send the descriptor
     * information for this plugin. This will also define the extra padding
     * that was necessary. This calculation must take into account the
     * preamble (8 bytes preceding the descriptor for the first channel)
     */
    num_packets = ((offset-1) / (SEQ_params.block_size-2)) + 1;
    offset += (2*num_packets);
    blocks = offset / SEQ_params.block_size;
    if (offset % SEQ_params.block_size)
	blocks++;

    return (blocks * SEQ_params.block_size);
}

/*XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX*/

VOID SEQ_Read_Segment_Number(seq_fP, plugin, segno)
  FILE  *seq_fP;
  BYTE  plugin;
  LONG  segno;

/*--------------------------------------------------------------------------

    Purpose: Search for segno for plugin and channel and read the acquisition
		parameters and channel data for it. Then format the
		data (raw, correct, and/or compensate) and output the new
		data (file or screen).

    Inputs: plugin = 0 for plugin A
		   = 1 for plugin B
	    
	    channel= 0,1,2,3 for 7234
		   = 0,1     for 7242B

	    segno   = segment number

    Outputs: 

    Machine dependencies: 
			 
    Notes: It is assumed that the data file has been opened for reading.

	   In order to handle 1M segments, the entire 1M of memory cannot
	   be allocated in one block; therefore, a small block is
	   allocated and portions of the waveform are copied into this
	   block, filtered, and written multiple times. This complicates
	   matters since all routines must remember where they are in
	   the file and in the filter routine.

    Procedure:

/CODE
--------------------------------------------------------------------------*/
{   /* SEQ_Read_Segment_Number() */

    LONG  blocks;
    LONG  num_packets;
    LONG  *array_sizeP;
    LONG  *corr_sizeP;
    LONG  data_count;
    LONG  data_size;
    LONG  total;
    BYTE  *array1P;
    WORD  *array2P;
    WORD  *array3P;
    BYTE  *dataP;
    BOOL  keep_going;
    BOOL  first_seg;
    FLOAT *time_per_ptP;
    FLOAT time_per_pt;
    FLOAT  *horiz_intervalP;
    FLOAT  trigger_delay;
    UWORD channel_tag;
    LONG  i;
    BYTE  c;
    DOUBLE diff_time;
    DOUBLE next_seg_time;
    BOOL  translate;
    BOOL  test_mode;
    BOOL  diagnostic_data;
    BYTE  status;
    WAVE_PARAMS  wave_param;

    SEQ_ACQ_PARAMS acq_params;
    SEQ_ACQ_DATA   data,acq_data;
    SEQ_FILTER_DATA  filt_data;
    static DOUBLE first_seg_time[MAX_PLUGINS] = {0,0};


    array_sizeP = (LONG *)PCW_Find_Value_From_Name(PCW_waveformP[plugin][0],
			  (LONG)0, PCW_blockP[plugin][0], "WAVE_ARRAY_1");

    /* Allocate WORDS so filtered points can be put in place */
    array1P = (BYTE *)(malloc((size_t)(sizeof(BYTE) *
			(MAX_BUF_SIZE+MAX_FILTER_SIZE))));
    if (!array1P)
	error_handler(OUT_OF_MEMORY);

    /* This array is where the 7291 flash-corrected points will go */
    array2P = (WORD *)(malloc((size_t)(sizeof(WORD) *
			(MAX_BUF_SIZE+MAX_FILTER_SIZE))));
    if (!array2P)
	error_handler(OUT_OF_MEMORY);

    /* This array is where the corrected points will go */
    array3P = (WORD *)(malloc((size_t)(sizeof(WORD) * MAX_BUF_SIZE)));
    if (!array3P)
	error_handler(OUT_OF_MEMORY);

    corr_sizeP = (LONG *)PCW_Find_Value_From_Name(PCW_waveformP[plugin][0],
			  (LONG)0, PCW_blockP[plugin][0], "PNTS_PER_SCREEN");
    data_count = *corr_sizeP + 2;
    horiz_intervalP =(FLOAT *)PCW_Find_Value_From_Name(PCW_waveformP[plugin][0],
			(LONG)0, PCW_blockP[plugin][0], "HORIZ_INTERVAL");

    /* Seek to the start of the segment information before scanning */
    fseek(seq_fP, SEQ_params.seg_offset, SEEK_SET);

    /* get the time_per_point for time conversion later */
    time_per_ptP = (FLOAT *)PCW_Find_Value_From_Name(PCW_waveformP
		[plugin][0], (LONG)0, PCW_blockP[plugin][0], "HORIZ_INTERVAL");
    time_per_pt = *time_per_ptP;

    acq_data.baseP = array1P;
    acq_data.array_size = *array_sizeP;
    acq_data.block_offset = 0;
    acq_data.plugin = plugin;
    filt_data.rawP = array1P;
    filt_data.p91_baseP = array2P;
    filt_data.corrP = array3P;
    filt_data.array_size = data_count;

    diagnostic_data = FALSE;
    test_mode = FALSE;
    keep_going = TRUE;
    for (i=1; keep_going == TRUE; ++i)
    {
	for (c=0; c <= SEQ_params.last_channel[plugin]; ++c)
	{
	    if (c == 0)
	    {
		/* Read the channel tag */
		data.bufP = (BYTE *)&channel_tag;
		data.size = 2L;
		data.block_offset = acq_data.block_offset;
		data.byte_offset = 0L;
		data.plugin = plugin;
		if (SEQ_Read_Blocks_Seg(seq_fP, &data, TRUE) == FALSE)
		    EXIT

		if (channel_tag == SEQ_DIAGNOSTIC_BLOCK)
		{
		    if (SEQ_options.debug == 1)
			printf("Found Diagnostic packet for Plugin %c\n",
					plugin+'A');
		    if (SEQ_options.test_mode == TRUE)
		    {
			acq_data.block_offset = data.block_offset;
			test_mode = TRUE;
		    }
		    else
			return;


		}
		else if (channel_tag == SEQ_SEGMENT_BLOCK)
		{
		    /* Read the Last Flash, TDC, timestamp */
		    data.bufP = (BYTE *)&acq_params;
		    data.size = 12L;
		    if (SEQ_Read_Blocks_Seg(seq_fP, &data, TRUE) == FALSE)
			EXIT

		    acq_data.block_offset = data.block_offset;
		    if (i == 1)
		    {
			/* Save the timestamp of first segment of each plugin */
			first_seg_time[plugin] = 
				GET_DOUBLE(acq_params.time_stamp) * time_per_pt;
#ifdef RESOLUTION_1_PSEC
			trigger_delay = ((FLOAT)acq_params.fine_count * 
					    *horiz_intervalP) / 32768.0;
			first_seg_time[plugin] += trigger_delay;
#endif  /* RESOLUTION_1_PSEC */

			SEQ_params.last_packet[plugin] = FALSE;
		    }

		    /* Determine relative trig time, current - first */
			next_seg_time = 
				GET_DOUBLE(acq_params.time_stamp) * time_per_pt;
#ifdef RESOLUTION_1_PSEC
			trigger_delay = ((FLOAT)acq_params.fine_count * 
					    *horiz_intervalP) / 32768.0;
			next_seg_time += trigger_delay;
#endif  /* RESOLUTION_1_PSEC */
			diff_time = next_seg_time - first_seg_time[plugin];

		    if (SEQ_options.debug == 1)
		    {
			printf("Seg %ld, LF=0x%04x, TDC=0x%04x\n", i,
				    acq_params.last_flash, 
				    acq_params.fine_count);
		    }

		    if (SEQ_options.print_times)
			seq_print_time(plugin, i, diff_time, &filt_data,
				trigger_delay);

		}
		else
		{
		    if (SEQ_params.last_packet[plugin] == FALSE)
			printf("Invalid channel tag: 0x%04x\n", channel_tag);

		    goto leave;
		}

	    }

	    SEQ_filter[plugin][c].last_flash = acq_params.last_flash;
	    filt_data.paramsP = &SEQ_filter[plugin][c];
	    acq_data.channel = c;
	    first_seg = TRUE;
	    data_size = total = acq_data.array_size;

	    /* Call a routine to determine if this seg is requested
	       If not, then skip over it else process it unless
	       there are no more segs to process in which case
	       break out of the for() loop */
	    if (test_mode == TRUE)
	    {
		translate = FALSE;

		printf("%c%d: Testing....\n", plugin+'A', c+1);

		if (acq_data.array_size > (LONG)MAX_BUF_SIZE)
		    acq_data.size = MAX_BUF_SIZE;
		else
		    acq_data.size = acq_data.array_size;
		acq_data.bufP = array1P;
		acq_data.byte_offset = 0;
		if (filt_data.paramsP->p91_mode == TRUE)
		{
		    filt_data.p91P = array2P;
		    filt_data.size_91 = acq_data.size;
		    filt_data.size = acq_data.size -
				(2*(filt_data.paramsP->num_coeffs-1));
		    SEQ_91_diagnostic(seq_fP,&acq_data,&filt_data,data_count);
		}
		else  /* non-7291 diagnostics */
		{
		    filt_data.size = acq_data.size;
		    SEQ_diagnostic(seq_fP, &acq_data, &filt_data, data_count);
		}

		/* If last diagnostic channel, nothing left to do so exit */
		if (c == SEQ_params.last_channel[plugin])
		    keep_going = FALSE;

		/* Skip to the top of the loop */
		data_size = 0;
	    }
	    else if (SEQ_Check_Seg(plugin, c, i, diff_time, &keep_going))
	    {
		/* Translate this segment */
		translate = TRUE;

		acq_data.byte_offset = 0L;
		if (acq_data.array_size > (LONG)MAX_BUF_SIZE)
		    acq_data.size = MAX_BUF_SIZE;
		else
		    acq_data.size = acq_data.array_size;
		acq_data.bufP = array1P;
		if (filt_data.paramsP->p91_mode == TRUE)
		{
		    filt_data.p91P = array2P;
		    filt_data.size_91 = acq_data.size;
		    filt_data.size = acq_data.size -
				(2*(filt_data.paramsP->num_coeffs-1));
		}
		else
		    filt_data.size = acq_data.size;

		/* Correct the waveform descriptor for this segment's TDC */
		wave_param.time_per_point = time_per_pt;
		wave_param.seg_start_time = diff_time;
		SEQ_Init_Descriptor(&acq_data, &filt_data, &wave_param, 
					acq_params.fine_count);

		/* Print status information */
		fprintf(stderr, "\n%c%d, Segment %ld:\n", plugin+'A', c+1, i);
	    }
	    else
	    {
		/* Skip over this segment */
		translate = FALSE;

		acq_data.byte_offset = data_size;
		acq_data.size = 0;

	    }

	    SEQ_update_time(FALSE, 0, 0);

	    while ((data_size > 0) && (keep_going == TRUE))
	    {
		if (SEQ_Process_Seg(seq_fP, first_seg, &acq_data, 
					&filt_data, translate) == FALSE)
		{
		    keep_going = FALSE;
		    break; 
		}

		/* Adjust data_size for amount just read */
		data_size -= acq_data.size;

		if (translate)
		{
		    if (first_seg == FALSE)
		    {
			if (data_size == 0)
			    status = SEQ_LAST_BLOCK;
			else
			    status = SEQ_NEXT_BLOCK;
		    }
		    else
		    {
			status = SEQ_FIRST_BLOCK;
			if (data_size == 0)
			    status |= SEQ_LAST_BLOCK;
		    }

		    /* Write the filtered block to display or a disk file */
		    SEQ_Output_Seg(i, status, &acq_data, &filt_data,
					&wave_param);
		}
		else
		    break;

		first_seg = FALSE;

		if (data_size < MAX_BUF_SIZE)
		    acq_data.size = data_size;

		SEQ_update_time(TRUE, total, data_size);
	    }
	}
    }

    leave:;
    free(array1P);
    free(array2P);
    free(array3P);
}


/*XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX*/

BOOL SEQ_Process_Seg(seq_fP, first_seg, acq_dataP, filt_dataP, process)
    FILE  	  *seq_fP;
    BOOL          first_seg;
    SEQ_ACQ_DATA  *acq_dataP;
    SEQ_FILTER_DATA *filt_dataP;
    BOOL	  process;

/*--------------------------------------------------------------------------

    Purpose: To read a block of data for a plugin/channel and filter it.

    Inputs: acq_dataP = parameters associated with the acquisition of the data
	    filt_dataP = parameters associated with filtering the data.
	    first_seg = TRUE if processing first block portion of this record
	    process = TRUE if data should be read into supplied buffer
		    = FALSE if data should be skipped

    Outputs: Filters the data in-place in the buffer.
	     returns TRUE if no errors and caller can continue
	     returns FALSE if an error occurred and we should terminate

    Machine dependencies: 
			 
    Notes: Must be able to remember where I am in the buffer at all times.

	   For the first 7291 block (first_seg==TRUE) we have:
	     -------------------------------------------  filt_dataP->rawP
   array1P   | acq_dataP->size (MAX_BUF_SIZE)          |  acq_dataP->bufP
	     -------------------------------------------  acq_dataP->size
			     |
		      FLASH FIR FILTER
			     |
			     V
	     -------------------------------------------  filt_dataP->p91P
   array2P   | acq_dataP->size - (flash_filt_len-1)    |  filt_dataP->size_91
	     -------------------------------------------
			     |
			7291 FIR FILTER
			     |
			     V
	     -------------------------------------------  filt_dataP->corrP
   array3P   | acq_dataP->size - (2*(13-1)) - (63-1)   |  filt_dataP->size
	     -------------------------------------------  


	   Subsequent blocks (first_seg==FALSE) are organized as:
--------------------------------------------------------  filt_dataP->rawP
|   24       | acq_dataP->size (MAX_BUF_SIZE)          |  acq_dataP->size+24
--------------------------------------------------------  
 past samples                |
		      FLASH FIR FILTER
			     |
			     V
--------------------------------------------------------  filt_dataP->p91P
|   62       | acq_dataP->size (MAX_BUF_SIZE)          |  filt_dataP->size_91+62
--------------------------------------------------------  
 past samples                |
			7291 FIR FILTER
			     |
			     V
	     -------------------------------------------  filt_dataP->corrP
             | acq_dataP->size                         |  filt_dataP->size
	     -------------------------------------------  

			acq_dataP Definitions:
			----------------------
	baseP = buffer containing all the raw data points for
			   this block.
	bufP = ptr to write the newly-read raw data points.
		bufP = baseP for the first block. After the first block, 
		the 24 raw data points (12 from each chan) that are still 
		needed in future flash corrections are copied to baseP and 
		bufP is advanced by 24.
	size = block size; the number of new points that should be processed 
		on each call to this routine.

			filt_dataP Definitions
			----------------------
	p91_baseP = buffer containing all the flash-corrected data points
	p91P = ptr to write the flash-corrected data points in 7291 mode
		  On the first block, p91_baseP=p91P; after the
		  first block, the 62 flash-corrected points that are still
		  needed in future 7291 filters are copied to p91_baseP and
		  p91P is advanced by 62.
	size_91 = number of raw data points in acq_dataP->baseP that can be used
		  to pass across the flash filter, the corrected points of 
		  which are put into p91P. size_91 equals block size for first 
		  block and is block_size+24 on subsequent blocks to account 
		  for the 24 extra raw data points in acq_dataP->baseP.

	corrP = buffer to put "corrected" results. For non-7291 operation,
		these "corrected" points are the result of the flash
		filters. For 7291 operations, these "corrected" points
		are the result of the flash and 7291 filters.
	size  = For non-7291 operation, equals number of raw data points in
		acq_dataP->baseP that can be used to pass across the flash
		filters. For 7291 operation, equals number of flash-corrected 
		WORDs in p91_baseP that can be used to pass across the 7291 
		filter the corrected points of which are put into corrP. 
		For non-7291 data, size equals block_size on the first block 
		and block_size+(flash_filter_len-1) on subsequent blocks. 
		For 7291 data, size equals block_size on the first block 
		and block_size+62 on subsequent blocks.
		  

	For non-7291 sequence records, the data flows from array1P, then
	thru the FLASH FIR FILTER, then to array3P.

    Procedure:

/CODE
--------------------------------------------------------------------------*/
{   /* SEQ_Process_Seg() */

    register IWORD data;
    register WORD  i;
    BYTE  *buf1P,*buf2P;
    WORD  *buf1_wP,*buf2_wP;
    WORD  num_coeffs, num_91_coeffs;

    static UWORD count=0;
    UWORD  *buf_wP;

    if (filt_dataP->paramsP->p91_mode == TRUE)
    {
	num_coeffs = 2 * (filt_dataP->paramsP->num_coeffs-1);
	num_91_coeffs = filt_dataP->paramsP->num_91coeffs-1;
	if (first_seg == FALSE)
	{
	    filt_dataP->p91P = filt_dataP->p91_baseP + num_91_coeffs;
	    filt_dataP->size_91 = acq_dataP->size + num_coeffs;
	}
    }
    else
	num_coeffs = filt_dataP->paramsP->num_coeffs-1;

    /* If not first seg, adjust pointer to include past filt_len-1 pts */
    if (first_seg == FALSE)
    {
	acq_dataP->bufP = acq_dataP->baseP + num_coeffs;
	if (filt_dataP->paramsP->p91_mode == TRUE)
	    filt_dataP->size = acq_dataP->size + num_91_coeffs;
	else
	    filt_dataP->size = acq_dataP->size + num_coeffs;
    }

    /* Copy the uncorrected raw data into the buffer */
    if (SEQ_Read_Blocks_Seg(seq_fP, acq_dataP, process) == FALSE)
	return(FALSE);

    /* Translate the data from Intel Lo/Hi to Motorola Hi/Lo format */
    if (process)
    {
	if (SEQ_options.debug == 2)
	{
	    if ((acq_dataP->channel == 0) && (first_seg == TRUE))
		count += 7;

	    buf_wP = (UWORD *)acq_dataP->bufP;
	    for (i=0; i < (acq_dataP->size/2); ++i)
	    {
		if (count != buf_wP[i])
		{
		    printf("Expected 0x%04x, read 0x%04x\n", count,
				buf_wP[i]);
		    EXIT;
		}
		else
		    ++count;
	    }
	}
	else if (SEQ_options.format != SEQ_FORMAT_RAW)
	{
	    /* Call a routine to filter the data if requested */
	    SEQ_fir(filt_dataP, first_seg);

	    /* Now copy the last filt_len-1 points to the start of the buffer */
	    if (acq_dataP->size == MAX_BUF_SIZE)
	    {
		buf1P = filt_dataP->rawP;
		buf2P = acq_dataP->bufP + acq_dataP->size - num_coeffs;

		for (i=num_coeffs; i > 0; --i)
		    *buf1P++ = *buf2P++;

		/* In 7291 mode, must also copy 7291 filter points to start */
		if (filt_dataP->paramsP->p91_mode == TRUE)
		{
		    buf1_wP = filt_dataP->p91_baseP;
		    buf2_wP = filt_dataP->p91P + 
			    (filt_dataP->size_91 - num_coeffs) - num_91_coeffs; 

		    for (i=num_91_coeffs; i > 0; --i)
			*buf1_wP++ = *buf2_wP++;
		}
	    }
	}
    }

    return(TRUE);
}

/*XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX*/

BOOL SEQ_Read_Blocks_Seg(seq_fP, dataP, read)
  FILE  *seq_fP;
  SEQ_ACQ_DATA  *dataP;
  BOOL read;

/*--------------------------------------------------------------------------

    Purpose: Read data from scsi_fP into the passed buffer checking for
		the correct sequence of packet numbers and stripping off
		the packet numbers when writing into bufferP.

    Inputs: dataP = point to structure containing all pertinent information
			concerning how much to read and where to put it.
	    dataP->block_offset = offset within the current block to seek to.
			This routine ALWAYS leaves seq_fP pointing to the
			beginning of a block so that the first 2 bytes are
			the packet number. block_offset, then, is the offset,
			in bytes, into this block to where we last left off.
			It is initialized to 0 and then maintained by this
			function...it is always positive.
	    dataP->byte_offset = +/- offset from where we last left off to
			where we should be reading the next byte. Therefore,
			the first byte is read at block_offset+byte_offset.
			This value can be used to skip over a segment by
			setting this to a segment's size; or it can be used
			to go back to a previous segment by setting it to
			the negative of a segment's record size.
	    read  = TRUE if should read data into a buffer
		  = FALSE if should skip over data. In this case, don't
			read size BYTEs, but instead read the entire array
			size.

    Outputs: TRUE if no problems reading data
	    FALSE if could not read data

    Machine dependencies: 
			 
    Notes: It is assumed that the data file has been opened for reading.

	   If absolute is FALSE (relative), then offset is ignored and is
	   replaced with the last known position of seq_fP in the file.

    	   The file pointer is set before entering this routine or 
	   remembered from the last call to this routine.

    Procedure:

/CODE
--------------------------------------------------------------------------*/
{   /* SEQ_Read_Blocks_Seg() */

register UWORD i;
register LONG num_read;
register LONG size;
register LONG num_packets;
    UWORD plugin_mask;
    UWORD bytes_remaining;
    UWORD count;
    
    WORD packet;
    LONG offset;
    LONG byte_skip;
    BYTE *bufferP;
    BYTE direction;

    /* Setup the mask to determine which blocks are significant */
    plugin_mask = (dataP->plugin << 15);

    /* Initialize the packet */
    offset = dataP->block_offset + dataP->byte_offset;
    bufferP = dataP->bufP;
    size = dataP->size;
    dataP->bytes_read = 0L;
    num_packets = 0;
    num_read = fread((CHAR *)&packet, sizeof(UWORD), 1, seq_fP);

    if (offset >= 0)
	direction = 1;
    else
	direction = -1;

    if (SEQ_options.debug == 1)
	printf("Found packet 0x%04x...mask = 0x%04x\n", packet, plugin_mask);

    i = packet & 0x7fff;

    while (((labs(offset) >= (SEQ_params.block_size-2)) || 
	   ((packet & 0x8000) != plugin_mask) ||
	   ((packet & 0x7fff) == 0)	      ||           /* Skip Descriptor */
	   (offset < 0)) && (num_read != 0))
    {
	if ((packet & 0x8000) == plugin_mask)
	{
	    if (direction > 0)
	    {
		if (i == (packet & 0x7fff))
		{
		    if (i == 0x7ffe)
			i = 1;	/* wrap */
		    else if (i != 0x7fff)
			++i;
		    if ((packet & 0x7fff) != 0)
			offset -= (SEQ_params.block_size-2);
		    fseek(seq_fP, (LONG)(SEQ_params.block_size-2), SEEK_CUR);
		}
		else if ((packet & 0x7fff) != 0x7fff)  /* 7fff = last packet */
		{
		    printf("Invalid packet: expected %d, read %d\n",
				    i | plugin_mask, (packet & 0x7fff));
		    EXIT
		}
	    }
	    else  /* negative direction */
	    {
		if (i == (packet & 0x7fff))
		{
		    if (i == 1)
			i = 0x7ffe;	/* wrap */
		    else if (i != 0x7fff)
			--i;
		    if ((packet & 0x7fff) != 0)
			offset += (SEQ_params.block_size-2);
		    fseek(seq_fP, (LONG)(-SEQ_params.block_size-2), SEEK_CUR);
		}
		else if ((packet & 0x7fff) != 0x7fff)  /* 7fff = last packet */
		{
		    printf("Invalid packet: expected %d, read %d\n",
				    i | plugin_mask, (packet & 0x7fff));
		    EXIT
		}
	    }
	}
	else
	{
	    /* skip over other plugin data to the next/previous block */
	    if (direction > 0)
		fseek(seq_fP, (LONG)(SEQ_params.block_size-2), SEEK_CUR);
	    else
		fseek(seq_fP, (LONG)(-SEQ_params.block_size-2), SEEK_CUR);
	}

	num_read = fread((CHAR *)&packet, sizeof(UWORD), 1, seq_fP);
	if ((i == 0x7fff) && ((packet & 0x8000) == plugin_mask))
	    i = packet & 0x7fff;
	num_packets++;

	if (SEQ_options.debug == 1)
	    printf("Found packet 0x%04x...offset = %ld\n", packet,offset);

    }

    if (num_read != 0)
    {
      if (size > 0)
      {
	/* Found block, now seek to offset within block */
	fseek(seq_fP, (LONG)offset, SEEK_CUR);
	bytes_remaining = (UWORD)(SEQ_params.block_size - 2 - offset);

	while (size > 0)
	{
	    if (size < (LONG)bytes_remaining)
		count = (UWORD)size;
	    else
		count = bytes_remaining;

	    /* Read the bytes into the supplied buffer */
	    if (read == TRUE)
		num_read = fread((CHAR *)bufferP, sizeof(BYTE), 
				    (size_t)count, seq_fP);
	    else
	    {
		num_read = (LONG)count;
		fseek(seq_fP, (LONG)count, SEEK_CUR);
	    }

	    if (num_read != (UWORD)count)
	    {
		printf("Only read %d bytes out of %ld from input file.\n",
				num_read, count);
		EXIT
	    }

	    size -= count;
	    offset += count;
	    dataP->bytes_read += count;

	    if (size > 0)
	    {
		bufferP += count;

		if (i == 0x7ffe)
		    i = 1;	/* wrap */
		else if (i != 0x7fff)
		    ++i;

		/* Read the beginning of the next block */
		num_read = fread((CHAR *)&packet, sizeof(UWORD), 1, seq_fP);
		if (i == 0x7fff)
		    i = packet & 0x7fff;

		while ( ((packet & 0x8000) != plugin_mask) && (num_read != 0))
		{
		    /* skip to the next block */
		    fseek(seq_fP, (LONG)(SEQ_params.block_size-2), SEEK_CUR);
		    num_read = fread((CHAR *)&packet, sizeof(UWORD), 1, seq_fP);
		    num_packets++;
		}

		if (num_read == 0)
		{
		    printf("Could not find packet %d...ran out of data\n", 
						i | plugin_mask);
		    EXIT
		}
		else  /* found next packet for this plugin */
		{
		    if ( (i != (packet & 0x7fff)) && 
			 ((packet & 0x7fff) != 0x7fff))
		    {
			printf("Invalid packet: expected %d, read %d\n",
					i | plugin_mask, (packet & 0x7fff));
			printf("Still expecting to read %ld bytes\n", size);
			EXIT
		    }
		}
		bytes_remaining = SEQ_params.block_size - 2;
		offset = 0;

	    }

	} /* WHILE */

	/* Seek back to the beginning of this block: offset+2 */
	fseek(seq_fP, (LONG)(-offset-2), SEEK_CUR);
      }
      else
	fseek(seq_fP, (LONG)(-2), SEEK_CUR);

      /* Update the params so we know where we left off */
	dataP->block_offset = offset;
	dataP->packet = num_packets;

	if ((packet & 0x7fff) == 0x7fff)  /* 7fff = last packet */
	    if ((packet & 0x8000) == plugin_mask)
		SEQ_params.last_packet[dataP->plugin] = TRUE;
    }
    else
    {
	printf("Could not find requested block in data file.\n");
	fseek(seq_fP, SEQ_params.seg_offset, SEEK_SET);
	return (FALSE);
    }

    return (TRUE);
}

/*XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX*/

static VOID seq_print_time(plugin, segno, diff_time, filt_dataP, trig_dly)
    BYTE   plugin;
    LONG   segno;
    DOUBLE diff_time;
    SEQ_FILTER_DATA *filt_dataP;
    FLOAT trig_dly;

/*--------------------------------------------------------------------------

    Purpose: Given the time difference expressed in seconds as a DOUBLE,
		convert the seconds to hours, minutes, seconds and 
		print the time.

    Inputs: segno = segment number (needed for print out)
	    
	    diff_time = time in seconds from first segment to segno
	    filt_dataP = ptr to filter information

    Outputs: Prints the time difference from the first segment to segno
		in the familiar hours:minutes:seconds format

    Machine dependencies: 
			 
    Notes:

    Procedure:

/CODE
--------------------------------------------------------------------------*/
{   /* seq_print_time() */

    register LONG	hours;
    register WORD	minutes;

    /* Convert the difference to hours:minutes:seconds */
    hours = (LONG)(diff_time / 3600.0);

    if (hours > 0)
	diff_time -= (DOUBLE)(hours * 3600);

    minutes = (WORD)(diff_time / 60.0);

    if (minutes > 0)
	diff_time -= (DOUBLE)(minutes * 60);

#ifdef TESTING_HARK_PROBLEM
    rel_time[plugin][segno % MAX_TIMES] = diff_time;
    trig_time[plugin][segno % MAX_TIMES] = trig_dly;
#endif /* TESTING_HARK_PROBLEM */

    if (filt_dataP->paramsP->p91_mode == TRUE)
	printf("Plugin %c, Seg %ld, %ld:%d:%.12f\n",
	    plugin+'A', segno, hours, minutes, diff_time);
    else
    {
#ifdef TESTING_HARK_PROBLEM
	if (plugin == 1)
	    printf("Plugin %c, Seg %ld, %ld:%d:%.12f ..... delta=%.12f,%.12f\n",
		plugin+'A', segno, hours, minutes, diff_time,
		diff_time - rel_time[0][segno % MAX_TIMES],
	      trig_time[1][segno % MAX_TIMES]-trig_time[0][segno % MAX_TIMES]);
	else
#endif /* TESTING_HARK_PROBLEM */

#ifdef RESOLUTION_1_PSEC
	    printf("Plugin %c, Seg %ld, %ld:%d:%.12f\n",
		plugin+'A', segno, hours, minutes, diff_time);
#else
	    printf("Plugin %c, Seg %ld, %ld:%d:%.9f\n",
		plugin+'A', segno, hours, minutes, diff_time);
#endif /* RESOLUTION_1_PSEC */
    }

}


/*XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX*/

BOOL SEQ_Read_Blocks_Desc(seq_fP, bufferP, plugin, size, offset)
  FILE  *seq_fP;
  BYTE  *bufferP;
  CHAR  plugin;
  LONG  size;
  LONG  offset;

/*--------------------------------------------------------------------------

    Purpose: Read data from scsi_fP into the passed buffer checking for
		the correct sequence of packet numbers and stripping off
		the packet numbers when writing into bufferP.

    Inputs: plugin = 0 for plugin A
		   = 1 for plugin B
	    
	    bufferP = pointer to buffer to copy data
	    size    = amount of data to copy into bufferP
	    offset  = offset from beginning of buffer to start reading data

    Outputs: TRUE if no problems reading data
	    FALSE if could not read data

    Machine dependencies: 
			 
    Notes: It is assumed that the data file has been opened for reading.

    Procedure:

/CODE
--------------------------------------------------------------------------*/
{   /* SEQ_Read_Blocks_Desc() */

    UWORD plugin_mask;
    UWORD num_read;
    UWORD packet;
    UWORD i;
    UWORD bytes_remaining;
    UWORD count;

    /* Setup the mask to determine which blocks are significant */
    plugin_mask = (plugin << 15);

    /* Advance to the nearest whole block before the required data */
    fseek(seq_fP, 0L, SEEK_SET);
    num_read = fread((CHAR *)&packet, sizeof(UWORD), 1, seq_fP);

    if (SEQ_options.debug == 1)
	printf("Found packet 0x%04x...mask = 0x%04x\n", packet, plugin_mask);

    i = packet & 0x7fff;
    while (((offset >= (SEQ_params.block_size-2)) || 
	   ((packet & 0x8000) != plugin_mask)) && (num_read != 0))
    {
	if ((packet & 0x8000) == plugin_mask)
	{
	    if (i == (packet & 0x7fff))
	    {
		if ((i & 0x7fff) == 0x7ffe)
		    i = 1;	/* wrap */
		else
		    ++i;
		offset -= (SEQ_params.block_size-2);
	    }
	    else if ((packet & 0x7fff) != 0x7fff)  /* 7fff = last packet */
	    {
		printf("Invalid packet: expected %d, read %d\n",
				    i | plugin_mask, (packet & 0x7fff));
		EXIT
	    }
	}

	/* skip to the next block */
	fseek(seq_fP, (LONG)(SEQ_params.block_size-2), SEEK_CUR);
	num_read = fread((CHAR *)&packet, sizeof(UWORD), 1, seq_fP);

	if (SEQ_options.debug == 1)
	    printf("Found packet 0x%04x...offset = %ld\n", packet,offset);

    }

    if (num_read != 0)
    {
	/* Found block, now seek to offset within block */
	fseek(seq_fP, (LONG)offset, SEEK_CUR);
	bytes_remaining = (UWORD)(SEQ_params.block_size - 2 - offset);

	while (size > 0)
	{
	    if (size < (LONG)bytes_remaining)
		count = (UWORD)size;
	    else
		count = bytes_remaining;

	    num_read = fread((CHAR *)bufferP, sizeof(BYTE), 
				    (size_t)count, seq_fP);

	    if (num_read != (UWORD)count)
	    {
		printf("Only read %d bytes out of %ld from input file.\n",
				num_read, count);
		EXIT
	    }

	    bufferP += count;
	    size -= count;

	    if ((i & 0x7fff) == 0x7ffe)
		i = 1;	/* wrap */
	    else
		++i;

	    if (size > 0)
	    {
		/* Read the beginning of the next block */
		num_read = fread((CHAR *)&packet, sizeof(UWORD), 1, seq_fP);

		while ( ((packet & 0x8000) != plugin_mask) && (num_read != 0))
		{
		    /* skip to the next block */
		    fseek(seq_fP, (LONG)(SEQ_params.block_size-2), SEEK_CUR);
		    num_read = fread((CHAR *)&packet, sizeof(UWORD), 1, seq_fP);
		}

		if (num_read == 0)
		{
		    printf("Could not find packet %d...ran out of data\n", 
						i | plugin_mask);
		    EXIT
		}
		else  /* found next packet for this plugin */
		{
		    if ( (i != (packet & 0x7fff)) && 
			 ((packet & 0x7fff) != 0x7fff))
		    {
			printf("Invalid packet: expected %d, read %d\n",
					i | plugin_mask, (packet & 0x7fff));
			EXIT
		    }
		}
		bytes_remaining = SEQ_params.block_size - 2;

	    }

	} /* WHILE */
    }
    else
    {
	printf("Could not find requested block in data file.\n");
	return (FALSE);
    }

    return (TRUE);
}


/*XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX*/

VOID SEQ_Output_Seg(segno, status, acq_dataP, filt_dataP, paramsP)
    LONG   	    segno;
    BOOL	    status;
    SEQ_ACQ_DATA    *acq_dataP;
    SEQ_FILTER_DATA *filt_dataP;
    WAVE_PARAMS     *paramsP;

/*--------------------------------------------------------------------------

    Purpose: To read determine the output device and send the block
		of data to that output device in the correct format.

    Inputs: segno = segment number (needed for print out)
	    acq_dataP = parameters associated with the acquisition of the data
	    filt_dataP = parameters associated with filtering the data.
	    status = indicates first block, last block or in-between block
	    paramsP = pointer to essential parameters derived from the
			waveform descriptor and necessary for printout.

	    seg_start_time = starting time of this segment in seconds relative
			     to the first segment.
	    time_per_point = Horizontal interval between each sample point.

    Outputs: 

    Machine dependencies: 
			 
    Notes: 

    Procedure:

/CODE
--------------------------------------------------------------------------*/
{   /* SEQ_Output_Seg() */

    LONG i;
    UWORD raw_limit;
    UWORD corr_limit;
    CHAR  format[16];
    CHAR  time_fmt[16];
    CHAR  filename[32];
    WORD  p;
    WORD  c;
    register BYTE *buf_bP;
    register WORD *buf_wP;
    register UWORD j;

    static DOUBLE time;
    static WORD file_ext[MAX_PLUGINS][MAX_CHANNELS] = {
	-1,-1,-1,-1,			/* Plugin A */
	-1,-1,-1,-1			/* Plugin B */
    };

    static LONG old_segno[MAX_PLUGINS][MAX_CHANNELS] = {
	-1,-1,-1,-1,			/* Plugin A */
	-1,-1,-1,-1			/* Plugin B */
    };

    if (SEQ_options.debug == 2)
    	return;

    p = acq_dataP->plugin;
    c = acq_dataP->channel;
    raw_limit = (UWORD)(acq_dataP->size);
    if (filt_dataP->paramsP->p91_mode == TRUE)
    {
	strcpy(time_fmt, "%.10f");
	corr_limit = (UWORD)(filt_dataP->size - 
			    (filt_dataP->paramsP->num_91coeffs-1));
	/* For last 7291 block, sub 2 for 2 extra samples collected */
	if (status & SEQ_LAST_BLOCK)
	    corr_limit -= 2;
    }
    else
    {
	strcpy(time_fmt, "%.9f");
	corr_limit = (UWORD)(filt_dataP->size - 
			    (filt_dataP->paramsP->num_coeffs-1));
    }

    if (SEQ_options.output.type == SEQ_OUTPUT_FILE)
    {
	/* If first segment, write the descriptor to a created file */
	/* else append this block of data to the end of the opened file */
	if (status & SEQ_FIRST_BLOCK)
	{
	    if (segno != old_segno[p][c])
	    {
		old_segno[p][c] = segno;
		++file_ext[p][c];
	    }
	}

	if (status & SEQ_FIRST_BLOCK)
	{
	    sprintf(filename, "trace_%c%d.%03d", p+'a', c+1, file_ext[p][c]);
	    if ((out_fP = fopen(filename,"wb")) == NULL)
	    {
		printf("Could not open file %s for writing.\n", filename);
		EXIT
	    }

	    /* Write the corrected descriptor to a file */
	    fwrite(PCW_Waveform[p][c], sizeof(BYTE), (size_t)SEQ_desc_size, 
					out_fP);
	}
	fwrite(filt_dataP->corrP, sizeof(WORD), (size_t)corr_limit, out_fP);

	if (status & SEQ_LAST_BLOCK)
	{
	    fclose(out_fP);
	    out_fP = NULL;
	}
    }
    else  /* SEQ_OUTPUT_SCREEN */
    {
	if (status & SEQ_FIRST_BLOCK)
	{
	    time = paramsP->seg_start_time + paramsP->horizontal_offset;
	}

	if (SEQ_options.output.format == SEQ_SCREEN_OUTPUT_1)
	{
	    sprintf(format, "%s\n", SEQ_options.prt_fmt);
	    if (SEQ_options.format == SEQ_FORMAT_RAW)
	    {
		buf_bP = acq_dataP->bufP;
		for (j=0; j < raw_limit; j++)
		{
		    printf(format, buf_bP[j] << 8);
		}
	    }
	    else if (SEQ_options.format == SEQ_FORMAT_CORRECTED)
	    {
		buf_wP = filt_dataP->corrP;
		for (j=0; j < corr_limit; ++j)
		{
		    printf(format, buf_wP[j]);
		}
	    }
	    else  /* SEQ_FORMAT_COMPENSATED */
	    {
		buf_wP = filt_dataP->corrP;
		for (j=0; j < corr_limit; ++j)
		{
		    printf(format, (paramsP->vertical_gain * buf_wP[j]) -
				    paramsP->vertical_offset);
		}
	    }
	}
	else  /* SEQ_SCREEN_OUTPUT_2 */
	{
	    sprintf(format, "%s %s\n", time_fmt, SEQ_options.prt_fmt);
	    if (SEQ_options.format == SEQ_FORMAT_RAW)
	    {
		buf_bP = acq_dataP->bufP;
		for (j=0; j < raw_limit; j++)
		{
		    printf(format, time, buf_bP[j] << 8);
		    time += paramsP->time_per_point;
		}
	    }
	    else if (SEQ_options.format == SEQ_FORMAT_CORRECTED)
	    {
		buf_wP = filt_dataP->corrP;
		for (j=0; j < corr_limit; ++j)
		{
		    printf(format, time, buf_wP[j]);
		    time += paramsP->time_per_point;
		}
	    }
	    else  /* SEQ_FORMAT_COMPENSATED */
	    {
		buf_wP = filt_dataP->corrP;
		for (j=0; j < corr_limit; ++j)
		{
		    printf(format, time, (paramsP->vertical_gain * buf_wP[j]) -
				    paramsP->vertical_offset);
		    time += paramsP->time_per_point;
		}
	    }
	}
    }
}

/* -------------------------------------------------------------------- */

