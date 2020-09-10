/************************** ris_tran.c *************************************

Author: Larry Salant		LeCroy Corporation
	             		700 Chestnut Ridge Road
				Chestnut Ridge, NY  10977

This program reads a data file that was previously acquired from a 7262
in SCSI sequence mode and unpacks the data and builds a RIS waveform.
The resultant waveform is written to a file or printed to the screen.
The waveform files can be read by the 7200A and examined using the
built-in processing capabilities of the 7200A.

 **********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <dos.h>
#include "seq_tran.h"
#include "seq_hdr.h"

/* this values are hard coded for now. If they become dynamic, they
could become arguments on the command line or deduced from the waveform
descriptor */
#define NUM_BINS	2500	/* # of bins (interleave factor) */
#define NUM_PTS_BIN	12 	/* # of points per bin */
#define NUM_PTS_PACKED	15 	/* # of bytes of packed data per bin */
#define NUM_PTS_WAVE	(NUM_BINS*NUM_PTS_BIN) 	/* # of points per waveform */
#define BIT_WIDTH	10

/* -------------------------------------------------------------------- */

extern CHAR   *PCW_translate_descriptor();
extern STATUS PCW_Print_Block();
extern CHAR   *RIS_process_arguments();
LONG SEQ_Read_Desc(FILE *seq_fP, CHAR plugin);
static INT unpack10_bits(UCHAR *packedP, INT count, UWORD *outP);
BOOL RIS_interleave(BYTE plugin, UWORD *ch1P, UWORD *ch2P, UWORD bin_num);
VOID RIS_build_waveform(FILE *seq_fP, BYTE plugin);
BOOL SEQ_Read_Blocks_Seg(FILE *seq_fP, SEQ_ACQ_DATA *dataP, BOOL read);
BOOL SEQ_Read_Blocks_Desc(FILE *seq_fP, BYTE *bufferP, CHAR plugin,
						LONG size, LONG offset);
VOID RIS_output_wave(BYTE plugin, INT chan);
VOID RIS_interpret(FILE *seq_fP, BYTE action);

/* -------------------------------------------------------------------- */

#define MAX_TIME 3

/* Global Variables. */

struct PCW_BLOCK    *PCW_blockP[MAX_PLUGINS][MAX_CHANNELS];
BYTE   		    *PCW_waveformP[MAX_PLUGINS][MAX_CHANNELS];
struct PCW_TEMPLATE *PCW_templateP = NULL;

struct SEQ_OPTIONS  SEQ_options;
struct SEQ_PARAMS   SEQ_params;

WORD SEQ_desc_size;    /* size in BYTEs of the waveform descriptor */
FILE *out_fP = NULL;

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
  "LeCroy 7200A SCSI Ris Data Translator (Version 1.1, Dec 22, 1993).\n");

    /* Process all the arguments to this routine */
	seq_filenameP = RIS_process_arguments(ac, &av[0]);

    /* Open the specified data file */
	if ((seq_fP = fopen(seq_filenameP,"rb")) == NULL)
	{
	    printf("Could not open file %s\n", seq_filenameP);
	    EXIT
	}

    /* initialize the acquisition parameters in descriptor */
	RIS_interpret(seq_fP, SEQ_INIT_PARAMETERS);

    /* initialize the descriptor for all channels */
	RIS_interpret(seq_fP, SEQ_READ_DESCRIPTOR);

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
	RIS_interpret(seq_fP, SEQ_READ_SEGMENT_NUMBER);

    /* Close the file before ending program */
	fclose(seq_fP);

    exit (0);
}


/*XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX*/

VOID RIS_interpret(FILE *seq_fP, BYTE action)

/*--------------------------------------------------------------------------

    Purpose: To interpret the descriptor or data.

    Inputs: seq_fP = FILE pointer to the opened data file

	    action = SEQ_INIT_PARAMETERS 
		     SEQ_READ_DESCRIPTOR
		     SEQ_READ_SEGMENT_NUMBER

    Notes: It is assumed that the data file has been opened for reading.

--------------------------------------------------------------------------*/
{   /* RIS_interpret() */

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

	/* Read the number of channels and allocate buffers for them */
	num_read = fread((CHAR *)&num_chan, sizeof(UWORD),1,seq_fP);
	SEQ_params.last_channel[plugin] = num_chan - 1;
	SEQ_params.bins_filled = 0;
	SEQ_params.last_packet[plugin] = FALSE;
	SEQ_params.binP[plugin] = (UWORD *)(calloc((size_t)(sizeof(UWORD)
							    * NUM_BINS), 1));
	if (!SEQ_params.binP[plugin])
	    error_handler(OUT_OF_MEMORY);
	SEQ_params.waveP[plugin][0] = (UWORD *)(calloc((size_t)(sizeof(UWORD)
							* NUM_PTS_WAVE), 1));
	if (!SEQ_params.waveP[plugin][0])
	    error_handler(OUT_OF_MEMORY);
	if (num_chan == 2)
	{
	    SEQ_params.waveP[plugin][1] =
		(UWORD *)(calloc((size_t)(sizeof(UWORD) * NUM_PTS_WAVE), 1));
	    if (!SEQ_params.waveP[plugin][1])
		error_handler(OUT_OF_MEMORY);
	}
	else
	    SEQ_params.waveP[plugin][1] = NULL;

	/* Search through the rest of the file looking for the presence of
	 * the other plugin. If found, initialize its parameters. Assuming
	 * a max descriptor size, don't search beyond num_chan for
	 * the other plugin.
	 */
	fseek(seq_fP, (LONG)sizeof(UWORD), SEEK_SET);
	search_size = num_chan * 400;
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
		SEQ_params.last_packet[plugin] = FALSE;

		/* allocate buffers for it */
		SEQ_params.binP[plugin] = (UWORD*)(calloc((size_t)(sizeof(UWORD)
							    * NUM_BINS), 1));
		if (!SEQ_params.binP[plugin])
		    error_handler(OUT_OF_MEMORY);
		SEQ_params.waveP[plugin][0] =
		    (UWORD *)(calloc((size_t)(sizeof(UWORD)*NUM_PTS_WAVE), 1));
		if (!SEQ_params.waveP[plugin][0])
		    error_handler(OUT_OF_MEMORY);
		if (num_chan == 2)
		{
		    SEQ_params.waveP[plugin][1] =
			(UWORD *)(calloc((size_t)(sizeof(UWORD)*NUM_PTS_WAVE),
									1));
		    if (!SEQ_params.waveP[plugin][1])
			error_handler(OUT_OF_MEMORY);
		}
		else
		    SEQ_params.waveP[plugin][1] = NULL;
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
	}
    }
    else if (action == SEQ_READ_DESCRIPTOR)
    {
	/* Read the descriptor for each channel */
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

	/* if wants data from all plugins */
	if (SEQ_options.process_plugin == RIS_PROC_ALL)
	{
	    /* for each plugin present, build waveform and output it */
	    for (p=SEQ_params.first_plugin; p <= SEQ_params.last_plugin; ++p)
	    {
		RIS_build_waveform(seq_fP, p);
		for (c=0; c <= SEQ_params.last_channel[p]; ++c)
		    RIS_output_wave(p, c);
	    }
	}
	else 
	{
	    p = SEQ_options.process_plugin;
	    if (p == SEQ_params.first_plugin || p == SEQ_params.last_plugin)
	    {
		RIS_build_waveform(seq_fP, p);

		if (SEQ_options.process_chan == RIS_PROC_ALL)
		{
		    for (c=0; c <= SEQ_params.last_channel[p]; ++c)
			RIS_output_wave(p, c);
		}
		else
		    RIS_output_wave(p, SEQ_options.process_chan);
	    }
	}
    }
}

/*XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX*/

LONG SEQ_Read_Desc(FILE *seq_fP, CHAR plugin)
/*--------------------------------------------------------------------------

    Purpose: For all plugins present, read the descriptors for all channels
		and initialize all blocks of the descriptor based on the
		7200.tpl template file.

    Inputs: plugin = 0 for plugin A
		   = 1 for plugin B
	    
	    PCW_Waveform[plugin][channel] = buffer to copy data

    Outputs: Returns the number of bytes used to contain the descriptors
		for all the channels of this plugin.

    Notes: It is assumed that the data file has been opened for reading.
	   The first block of each plugin is special because of the pre-amble 
	   information (block size and number of channels). We need to read 
	   enough of first descriptor to call the PCW package. Therefore, we 
	   will read 400 bytes which is more than enough. Then we can determine
	   the real size of the descriptor in order to read the rest of
	   the descriptors for the other channels. The data for the
	   first descriptor begins 6 bytes into the data file
	   (4 for block size, 2 for num channels).
	   
--------------------------------------------------------------------------*/
{   /* SEQ_Read_Desc() */

    BYTE  c;
    LONG  *desc_sizeP;
    LONG  num_packets;
    LONG  blocks;
    LONG  offset;

    /* Starting offset is 6: SCSI block size(4), # chan(2)  */
    offset = 6;

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

	if (SEQ_options.debug == 1)
	{
	    printf("Desc size = %ld\n", *desc_sizeP);
	}

	offset += *desc_sizeP;
    }

    /* Save the descriptor size: same for all plugins */
    SEQ_desc_size = (WORD)*desc_sizeP;

    /* Now determine how many blocks were required to send the descriptor
     * information for this plugin. This will also define the extra padding
     * that was necessary. This calculation must take into account the
     * preamble (6 bytes preceding the descriptor for the first channel)
     */
    num_packets = ((offset-1) / (SEQ_params.block_size-2)) + 1;
    offset += (2*num_packets);
    blocks = offset / SEQ_params.block_size;
    if (offset % SEQ_params.block_size)
	blocks++;

    return (blocks * SEQ_params.block_size);
}

/*XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX*/

static INT unpack10_bits(UCHAR *packedP, INT count, UWORD *outP)
/*--------------------------------------------------------------------------

    Purpose: unpacks data from input buffer into output buffer

    Inputs: packedP - pointer to buffer containing packed data
	    count - number of unpacked points desired
	    outP - pointer to output buffer

    Outputs: number of packed bytes read 

    Notes: this assumes each acquistion segment unpacks from an integer
	   number of bytes
--------------------------------------------------------------------------*/
{   /* unpack10_bits() */
#ifdef LITTLE_ENDIAN
    INT index;
    LONG offset;

    offset = 0;
    for (index = 0; index < count; ++index)
    {
	/* get word from packed data buffer */
	*outP = *(UWORD *)(packedP + (offset >> 3));

	/* extract 10 bit data from word */
	*outP <<= (6 - (offset & 7));
	*outP &= 0xffc0;

	offset += BIT_WIDTH;
	++outP;
    }
    return(offset>>3);
#else /* LITTLE_ENDIAN - BIG */
    /* The packing is done by the 7262's DSP which works in Motorola
      (big endian format).  This could be unpacked using the generic
       method above, if you first transform the data into little edian format.
       For speed, since we have a fixed number of bytes/record, we will
       hard code the transformation. */

        UWORD *wordP;

	wordP = (UWORD *)packedP;		/* for clarity */
	*outP = *wordP & 0xffc0;		/* word 0 in bytes 0 & 1 */
	++outP;
						/* word 1 in bytes 0 & 3 */
	*outP = *wordP << 10;
	*outP |= ((UWORD)*(packedP + 3) << 2);
	*outP &= 0xffc0;
	++outP;
						/* word 2 in bytes 2 & 3 */
	*outP = *(wordP+1) << 4;
	*outP &= 0xffc0;
	++outP;
						/* word 3 in bytes 2 & 5 */
	*outP = ((UWORD)*(packedP + 2) << 14);
	*outP |= (*(wordP + 2) >> 2);
	*outP &= 0xffc0;
	++outP;
						/* word 4 in bytes 4 & 7 */
	*outP = ((UWORD)*(packedP + 4)) << 8;
	*outP |= ((UWORD)*(packedP + 7));
	*outP &= 0xffc0;
	++outP;
						/* word 5 in bytes 6 & 7 */
	*outP = *(wordP+3) << 2;
	*outP &= 0xffc0;
	++outP;
						/* word 6 in bytes 6 & 9 */
	*outP = ((UWORD)*(packedP + 6)) << 12;
	*outP |= (((UWORD)*(packedP + 9)) << 4);
	*outP &= 0xffc0;
	++outP;
						/* word 7 in bytes 8 & 9 */
	*outP = *(wordP+4) << 6;
	++outP;
						/* word 8 in bytes 10 & 11 */
	*outP = *(wordP+5) & 0xffc0;
	++outP;
						/* word 9 in bytes 10 & 13 */
	*outP = *(wordP+5) << 10;
	*outP |= (UWORD)(*(packedP + 13) << 2);
	*outP &= 0xffc0;
	++outP;
						/* word 10 in bytes 12 & 13 */
	*outP = *(wordP+6) << 4;
	*outP &= 0xffc0;
	++outP;
						/* word 11 in bytes 12 & 14 */
	*outP = ((UWORD)*(packedP + 12)) << 14;
	*outP |= (((UWORD)*(packedP + 14)) << 6);
	++outP;

	return(15);
#endif /* LITTLE_ENDIAN */
}

/*XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX*/

BOOL RIS_interleave(BYTE plugin, UWORD *ch1P, UWORD *ch2P, UWORD bin_num)
/*--------------------------------------------------------------------------

    Purpose: Interleave the current record into the RIS Waveform if the
	     bin was not hit yet.	

    Inputs: plugin - number of plugin
	    ch1P   - pointer to current acquisition for channel 1
	    ch2P   - pointer to current acquisition for channel 2
	    bin_num- number of bin for current acquisition

    Outputs: returns TRUE if the RIS waveform is complete.

--------------------------------------------------------------------------*/
{   /* RIS_interleave() */
    BOOL done = FALSE;
    UWORD *wave1P, *wave2P;
    UWORD *binP;
    INT point;
    
    if (bin_num >= NUM_BINS)
    {
	if (SEQ_options.debug == 4)
	    fprintf(stderr, "\nIllegal TDC value = %d.\n", bin_num);
	return(done);	/* ignore bin */
    }

    /* if this bin isn't alreay filled */
    binP = SEQ_params.binP[plugin] + bin_num;
    if (*binP == 0)
    {
	*binP = 1;

	if (ch2P == NULL)
	{
	    /* copy new acqusition into RIS record */
	    wave1P = SEQ_params.waveP[plugin][0] + bin_num;
	    for (point = 0; point < NUM_PTS_BIN; ++point)
	    {
		*wave1P = *ch1P;
		++ch1P;
		wave1P += NUM_BINS;
	    }
	}
	else /* 2 channel mode */
	{
	    /* copy new acqusition into RIS record */
	    wave1P = SEQ_params.waveP[plugin][0] + bin_num;
	    wave2P = SEQ_params.waveP[plugin][1] + bin_num;
	    for (point = 0; point < NUM_PTS_BIN; ++point)
	    {
		*wave1P = *ch1P;
		++ch1P;
		wave1P += NUM_BINS;
		*wave2P = *ch2P;
		++ch2P;
		wave2P += NUM_BINS;
	    }
	}

	/* check if all the bins are filled */
	++SEQ_params.bins_filled;
	if (SEQ_params.bins_filled == NUM_BINS)
	    done = TRUE;
    }
    else if (SEQ_options.debug == 5)
	*binP += 1;
    return(done);
}

/*XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX*/

VOID RIS_build_waveform(FILE *seq_fP, BYTE plugin)
/*--------------------------------------------------------------------------

    Purpose: Read and unpack each record and build RIS waveform of specified
	     plugin

    Inputs: file pointer

    Outputs: 

    Notes: It is assumed that the data file has been opened for reading.

--------------------------------------------------------------------------*/
{   /* RIS_build_waveform() */
    BYTE  *array1P, *packedP;
    UWORD *ch1P, *ch2P;
    SEQ_ACQ_DATA   acq_data;
    UWORD *bin_numP;
    INT block_size, num_chan;
    INT chan;
    BOOL done = FALSE;
    ULONG count;

    if (SEQ_params.waveP[plugin][1] == NULL)
	num_chan = 1;
    else
	num_chan = 2;

    /* determine if data is packed or not ... runtime switch for now ...
       should be in descriptor */
    if (SEQ_options.packed)
    {
	block_size = (NUM_PTS_PACKED * num_chan) + 2;

	ch1P = (UWORD *)(malloc((size_t)(sizeof(WORD) * NUM_PTS_BIN * 2)));
	if (!ch1P)
	    error_handler(OUT_OF_MEMORY);

	if (SEQ_options.debug == 5)
	    SEQ_options.packed = FALSE;
    }
    else
    {
	block_size = (NUM_PTS_BIN * 2 * num_chan) + 2;
    }

    /* allocate temporary buffer for reading in data and one for unpacking. */
    array1P = (BYTE *)(malloc((size_t)(sizeof(BYTE) * block_size)));
    if (!array1P)
	error_handler(OUT_OF_MEMORY);

    if (!SEQ_options.packed)
	ch1P = (UWORD *)array1P;

    if (num_chan == 2)
	ch2P = ch1P + NUM_PTS_BIN;
    else
	ch2P = NULL;

    /* bin number is always the last 2 bytes of the block */
    bin_numP = (UWORD *)(array1P + block_size - 2);

    /* Seek to the start of the segment information before scanning */
    fseek(seq_fP, SEQ_params.seg_offset, SEEK_SET);
    acq_data.block_offset = 0;
    acq_data.byte_offset = 0;
    acq_data.plugin = plugin;
    acq_data.bufP = array1P;
    acq_data.size = block_size;

    /* for each block until the end of the file or until the RIS record
       is complete, read the block in and process it */
    fprintf(stderr, "bin number = ");
    count = 0;
    while (!done)
    {
	++count;
	if ((count & 0x3f) == 0x20)
	    fprintf(stderr, "%6ld\b\b\b\b\b\b", count);

	/* get data for this segment */
	if (SEQ_Read_Blocks_Seg(seq_fP, &acq_data, TRUE) == FALSE)
	    break;	/* assume no more data */

	/* for each channel */
	if (SEQ_options.packed)
	{
	    packedP = array1P;

	    /* unpack data from channel */
	    packedP += unpack10_bits(packedP, NUM_PTS_BIN, ch1P);

	    if (num_chan == 2)
		(VOID) unpack10_bits(packedP, NUM_PTS_BIN, ch2P);

	    /* put data into RIS waveform */
	    done = RIS_interleave(plugin, ch1P, ch2P, *bin_numP);
	}
	else
	{ /* put data into RIS waveform */
	    done = RIS_interleave(plugin, ch1P, ch2P, *bin_numP);
	}

	/* if this is the last block, and there is no more data in it...done */
	if (SEQ_params.last_packet[plugin] && ((UWORD)(SEQ_params.block_size - 2
				- acq_data.block_offset) < block_size))
	    done = TRUE;
    }
    fprintf(stderr, "%6ld\b\b\b\b\b\b", count);
    if (ch1P != (UWORD *)array1P)
	free(ch1P);
    free(array1P);
}

/*XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX*/

BOOL SEQ_Read_Blocks_Seg(FILE *seq_fP, SEQ_ACQ_DATA *dataP, BOOL read)
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
#ifndef RIS
	printf("Could not find requested block in data file.\n");
	fseek(seq_fP, SEQ_params.seg_offset, SEEK_SET);
#endif /* RIS */
	return (FALSE);
    }

    return (TRUE);
}

/*XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX*/

BOOL SEQ_Read_Blocks_Desc(FILE *seq_fP, BYTE *bufferP, CHAR plugin,
						LONG size, LONG offset)
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

VOID RIS_output_wave(BYTE plugin, INT chan)
/*--------------------------------------------------------------------------

    Purpose: To read determine the output device and send the block
		of data to that output device in the correct format.

    Inputs: plugin = plugin to be processed
	    chan = number of channel

    Outputs: 

--------------------------------------------------------------------------*/
{   /* RIS_output_wave() */

    CHAR  format[16];
    CHAR  filename[32];
    FLOAT time_per_pt, time;
    register UWORD j;
    FLOAT vertical_gain, vertical_offset;
    WORD *waveP;

    if (SEQ_options.debug == 5)
    {
	UWORD *binP;

	binP = SEQ_params.binP[plugin];
	for (j=0; j < NUM_BINS; j++)
	{
	    printf("%d\n", *binP);
	    ++binP;
	}
	return;
    }
    if (SEQ_options.debug == 2 || SEQ_options.debug == 3)
    	return;

    if (SEQ_options.output.type == SEQ_OUTPUT_FILE)
    {
	/* write the descriptor to a created file */
	sprintf(filename, "trace_%c%d.000", plugin+'a', chan+1);
	if ((out_fP = fopen(filename,"wb")) == NULL)
	{
	    printf("Could not open file %s for writing.\n", filename);
	    EXIT
	}

	/* modify the waveform size */
	*(LONG *)PCW_Find_Value_From_Name(PCW_waveformP[plugin][0],
					      (LONG)0, PCW_blockP[plugin][0],
					      "WAVE_ARRAY_1") = NUM_PTS_WAVE;

	/* Write the corrected descriptor to a file */
	fwrite(PCW_Waveform[plugin][chan], sizeof(BYTE),
					    (size_t)SEQ_desc_size, out_fP);

	/* write the data*/
	fwrite(SEQ_params.waveP[plugin][chan], sizeof(WORD),
					    (size_t)NUM_PTS_WAVE, out_fP);

	/* close file */
	fclose(out_fP);
    }
    else  /* SEQ_OUTPUT_SCREEN */
    {
	/* get vertical parameters from waveform */
	vertical_gain = *(FLOAT *)PCW_Find_Value_From_Name(
				    PCW_waveformP[plugin][0], (LONG)0,
				    PCW_blockP[plugin][0], "VERTICAL_GAIN");
	vertical_offset = *(FLOAT *)PCW_Find_Value_From_Name(
				    PCW_waveformP[plugin][0], (LONG)0,
				    PCW_blockP[plugin][0], "VERTICAL_OFFSET");
	waveP = (WORD *)SEQ_params.waveP[plugin][chan];

	time = 0.0;
	if (SEQ_options.output.format == SEQ_SCREEN_OUTPUT_1)
	{
	    sprintf(format, "%s\n", SEQ_options.prt_fmt);
	    if (SEQ_options.format == SEQ_FORMAT_RAW)
	    {
		for (j=0; j < NUM_PTS_WAVE; j++)
		{
		    printf(format, *waveP);
		    ++waveP;
		}
	    }
	    else  /* SEQ_FORMAT_COMPENSATED */
	    {
		for (j=0; j < NUM_PTS_WAVE; ++j)
		{
		    printf(format, (vertical_gain * *waveP) - vertical_offset);
		    ++waveP;
		}
	    }
	}
	else  /* SEQ_SCREEN_OUTPUT_2 */
	{
	    sprintf(format, "%.9f %s\n", SEQ_options.prt_fmt);

	    /* get the time_per_point */
	    time_per_pt = *(FLOAT *)PCW_Find_Value_From_Name(PCW_waveformP
		[plugin][0], (LONG)0, PCW_blockP[plugin][0], "HORIZ_INTERVAL");

	    if (SEQ_options.format == SEQ_FORMAT_RAW)
	    {
		for (j=0; j < NUM_PTS_WAVE; j++)
		{
		    printf(format, time, *waveP);
		    time += time_per_pt;
		    ++waveP;
		}
	    }
	    else  /* SEQ_FORMAT_COMPENSATED */
	    {
		for (j=0; j < NUM_PTS_WAVE; ++j)
		{
		    printf(format, time, (vertical_gain * *waveP) -
							    vertical_offset);
		    time += time_per_pt;
		    ++waveP;
		}
	    }
	}
    }
}

/* -------------------------------------------------------------------- */

