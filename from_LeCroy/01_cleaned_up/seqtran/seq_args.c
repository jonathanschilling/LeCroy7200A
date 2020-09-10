/************************** seq_args.c *************************************

Author: Christopher Eck		LeCroy Corporation
	             		700 Chestnut Ridge Road
				Chestnut Ridge, NY  10977

 **********************************************************************/


#include <stdio.h>
#include <stdlib.h>
#include <dos.h>
#include <ctype.h>
#include "seq_tran.h"

#define WAIT_FOR_PLUGIN  	0
#define WAIT_FOR_CHANNEL 	1
#define WAIT_FOR_SEG     	2
#define WAIT_FOR_FIRST_SEG	3
#define CHECK_FOR_UPPER_LIMIT  	4
#define WAIT_FOR_UPPER_LIMIT   	5
#define WAIT_FOR_MINUTES	6
#define WAIT_FOR_SECONDS	7
#define WAIT_FOR_FMT		8
#define WAIT_FOR_PRT_FMT	9

/* -------------------------------------------------------------------- */

extern VOID seq_print_usage();
extern INT  compare_seg();
extern INT  compare_time();

BYTE plugin_field;
BYTE channel_field[MAX_PLUGINS];

/*XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX*/

CHAR *SEQ_process_arguments(num_args, arguments)
int num_args;
char *arguments[];

/*--------------------------------------------------------------------------

    Purpose: To process all the arguments on the command line.

    Inputs: Various configuration arguments; see seq_print_usage()

    Outputs:

    Machine dependencies:

    Notes: To specify the segment to translate, the options are vary flexible.
	   In order to provide this flexibility, a 4 dimensional array was
	   needed. It is organized as follows:

Dim 1            Plugin A                           Plugin B
Dim 2      Ch 1............Ch 4	             Ch 1.............Ch 4
Dim 3   segno   time....segno   time     segno   time.....segno   time
	_____   ____...._____   ____     _____   ____....._____   ____
	_____   ____...._____   ____     _____   ____....._____   ____
Dim 4     :       : ....  :       :        :       : .....  :       :
	  :       : ....  :       :        :       : .....  :       :
	_____   ____...._____   ____     _____   ____....._____   ____


	Each channel's segno and time arrays are then sorted in increasing
	numerical order so that when the data file is translated in
	sequential order, each segment found is checked if the user wants
	it by looking in the sorted array. The end of each array is tagged
	with a -1L or (DOUBLE)-1 so not all elements have to be checked.


    Procedure:

/CODE
--------------------------------------------------------------------------*/
{   /* SEQ_process_arguments() */

    /* index into seg array */
    WORD index[MAX_PLUGINS][MAX_CHANNELS][MAX_SEG_TYPES]; 
    CHAR *argP;
    BYTE p,c;
    WORD i,j,k,t;
    WORD state;
    BOOL keep_going;
    BOOL all_values;
    LONG seg,seg1;
    SEGS *segP;
    TIME start,end,*selP;
    WORD max_chan;
    CHAR option[32];
    BOOL no_segs;

    static CHAR filename[80];

    /* Set default values */
    SEQ_options.debug = FALSE;
    SEQ_options.print_coeffs = FALSE;
    SEQ_options.test_mode = FALSE;
    SEQ_options.print_times = FALSE;
    SEQ_options.output.type = SEQ_OUTPUT_SCREEN;
    SEQ_options.output.format = SEQ_SCREEN_OUTPUT_1;
    SEQ_options.format = SEQ_FORMAT_COMPENSATED;
    strcpy (SEQ_options.prt_fmt, "%g");
    no_segs = TRUE;
    plugin_field = 0;

    for (p=0; p < MAX_PLUGINS; ++p)
    {
	channel_field[p] = 0;
	for (c=0; c < MAX_CHANNELS; ++c)
	{
	    SEQ_options.all_segs[p][c] = FALSE;
	    SEQ_options.print_params[p][c] = FALSE;
	    t = SEQ_SEGNO;
	    index[p][c][t] = 0;
	    segP = SEQ_options.seg[p][c][t];
	    segP->select.n.start = -1L;
	    segP->select.n.end = -1L;
	    t = SEQ_TIME;
	    index[p][c][t] = 0;
	    segP = SEQ_options.seg[p][c][t];
	    segP->select.t.start = (DOUBLE)-1;
	    segP->select.t.end = (DOUBLE)-1;
	}
    }

    if (num_args == 1)
    {
	seq_print_usage();
	EXIT
    }
    else if (arguments[1][0] == '-')
    {
	i = 1;
	strcpy(filename,"UNKNOWN");
    }
    else
    {
	/* Initialize the scsi_filename to the named file */
	i = 2;
	strcpy(filename, arguments[1]);
    }

    for (; i < num_args; ++i)
    {

	/* printf("argument[%d] = %s\n", i, arguments[i]); */

        if (arguments[i][0] != '-')
            break;

        if (!strncmp(arguments[i], "-a", 2))   	/* print acq parameters */
        {
	    keep_going = TRUE;
	    all_values = TRUE;
	    argP = &arguments[i][2];
	    p=c=0;
	    state = WAIT_FOR_PLUGIN;

	    while (keep_going && (i < num_args))
	    {
		if (*argP)
		{
		    if (isalpha(*argP))
		    {
			if (state==WAIT_FOR_PLUGIN)
			{
			    p = toupper(*argP) - 'A';
			    state = WAIT_FOR_CHANNEL;
			    c = 0;
			    max_chan = 1;
			    argP++;
			    if (p < 0)
			    {
				printf("Invalid plugin: %s\n", argP);
				EXIT
			    }
			}
			else if (state == WAIT_FOR_CHANNEL)
			{
			    state = WAIT_FOR_PLUGIN;
			    all_values = FALSE;
			    for (k=c; k < max_chan; ++k)
				SEQ_options.print_params[p][k] = TRUE;
			}
			else
			{
			    printf("Unexpected plugin: %s\n", argP);
			    EXIT
			}
		    }
		    else if (isdigit(*argP))
		    {
			if (state == WAIT_FOR_CHANNEL)
			{
			    c = *argP - '1';
			    if (c < 0)
			    {
				printf("Invalid channel: %s\n", argP);
				EXIT
			    }
			    all_values = FALSE;
			    argP++;
			    max_chan = c+1;
			}
			else
			{
			    printf("Unexpected number: %s\n", argP);
			    EXIT
			}
		    }
		    else if (*argP == ',')
		    {
			argP++;
			if (state == WAIT_FOR_CHANNEL)
			{
			    /* All channels of this plugin */
			    max_chan = MAX_CHANNELS;
			}
		    }
		    else
		    {
			printf("Unexpected value: %s\n", argP);
			EXIT
		    }
		}
		else  /* skip over white space if there */
		{
		    if ((i+1) < num_args)
		    {
			if (arguments[i+1][0] != '-')
			{
			    i++;
			    argP = &arguments[i][0];
			}
			else
			    keep_going = FALSE;
		    }
		    else
			keep_going = FALSE;
		}
	    }

	    if (state == WAIT_FOR_CHANNEL)
	    {
		all_values = FALSE;
		for (k=c; k < max_chan; ++k)
		    SEQ_options.print_params[p][k] = TRUE;
	    }

	    if (all_values == TRUE)
	    {
		for (p=0; p < MAX_PLUGINS; ++p)
		    for (c=0; c < MAX_CHANNELS; ++c)
			SEQ_options.print_params[p][c] = TRUE;
	    }
        }

        else if (!strncmp(arguments[i], "-t", 2)) /* Print times of all segs */
        {
	    SEQ_options.print_times = TRUE;
        }

        else if (!strncmp(arguments[i], "-d", 2)) /* Test mode active */
        {
	    SEQ_options.test_mode = TRUE;
        }

        else if (!strncmp(arguments[i], "-s", 2)) /* select a segment */
	{
	    keep_going = TRUE;
	    all_values = TRUE;
	    argP = &arguments[i][2];
	    p=c=0;
	    t = SEQ_SEGNO;
	    state = WAIT_FOR_PLUGIN;
	    selP = &start;
	    no_segs = FALSE;

	    while (keep_going && (i < num_args))
	    {
		if (*argP)
		{
		    if (isalpha(*argP))
		    {
			all_values = FALSE;
			if (state==WAIT_FOR_PLUGIN || state==WAIT_FOR_SEG)
			{
			    p = toupper(*argP) - 'A';
			    plugin_field |= (1 << p);
			    state = WAIT_FOR_CHANNEL;
			    c = 0;
			    max_chan = 1;
			    argP++;
			    if (p < 0)
			    {
				printf("Invalid plugin: %s\n", argP);
				EXIT
			    }
			}
			else if ((state == CHECK_FOR_UPPER_LIMIT) ||
				 (state == WAIT_FOR_MINUTES) ||
				 (state == WAIT_FOR_SECONDS))
			{
			    state = WAIT_FOR_PLUGIN;
			    for (k=c; k < max_chan; ++k)
			    {
				if (t == SEQ_TIME)
				{
				    segP =
				      &SEQ_options.seg[p][k][t][index[p][k][t]];
				    segP->select.t.start = 
					(DOUBLE)(3600.0 * start.hours) +
					(DOUBLE)(60.0 * start.minutes) + 
					(DOUBLE)(start.seconds);

				    if ((selP == &end) && (end.hours != 0))
					segP->select.t.end = 
					    (DOUBLE)(3600.0 * end.hours) +
					    (DOUBLE)(60.0 * end.minutes) + 
					    (DOUBLE)(end.seconds);
				    else
					segP->select.t.end = 
						segP->select.t.start;
				}
				if (index[p][k][t] < (MAX_SEGS-1))
				    index[p][k][t]++;
			    }
			}
			else if (state == WAIT_FOR_FIRST_SEG)
			{
			    state = WAIT_FOR_PLUGIN;
			    for (k=c; k < max_chan; ++k)
				SEQ_options.all_segs[p][k] = TRUE;
			}
			else
			{
			    printf("Unexpected plugin: %s\n", argP);
			    EXIT
			}
		    }
		    else if ((isdigit(*argP)) || (*argP == '.'))
		    {
			if (state == WAIT_FOR_CHANNEL)
			{
			    c = *argP - '1';
			    channel_field[p] |= (1 << c);
			    if (c < 0)
			    {
				printf("Invalid channel: %s\n", argP);
				EXIT
			    }
			    state = WAIT_FOR_FIRST_SEG;
			    argP++;
			    max_chan = c+1;
			}
			else if (state==WAIT_FOR_SEG || 
				 state==WAIT_FOR_FIRST_SEG)
			{
			    seg = atol(argP);
			    if (seg < 0)
			    {
				printf("Invalid seg: %s\n", argP);
				EXIT
			    }
			    for (k=c; k < max_chan; ++k)
			    {
				t = SEQ_SEGNO;
				segP=&SEQ_options.seg[p][k][t][index[p][k][t]];
				segP->select.n.start = seg;
				segP->select.n.end = seg;
			    }
			    state = CHECK_FOR_UPPER_LIMIT;
			    while (*argP && isdigit(*argP))
				++argP;
			}
			else if (state == CHECK_FOR_UPPER_LIMIT)
			{
			    state = WAIT_FOR_SEG;
			    for (k=c; k < max_chan; ++k)
			    {
				if (t == SEQ_TIME)
				{
				    segP =
				      &SEQ_options.seg[p][k][t][index[p][k][t]];
				    segP->select.t.start = 
					(DOUBLE)(3600.0 * start.hours) +
					(DOUBLE)(60.0 * start.minutes) + 
					(DOUBLE)(start.seconds);

				    segP->select.t.end = segP->select.t.start;
				}
				if (index[p][k][t] < (MAX_SEGS-1))
				    index[p][k][t]++;
			    }
			}
			else if (state == WAIT_FOR_UPPER_LIMIT)
			{
			    seg = atol(argP);
			    if (seg < 0)
			    {
				printf("Invalid upper limit: %s\n", argP);
				EXIT
			    }
			    if (t == SEQ_SEGNO)
			    {
				for (k=c; k < max_chan; ++k)
				{
				    segP =
				      &SEQ_options.seg[p][k][t][index[p][k][t]];
				    seg1 = segP->select.n.start;
				    if (seg > seg1)
					segP->select.n.end = seg;
				    else
				    {
					segP->select.n.start = seg;
					segP->select.n.end = seg1;
				    }
				    if (index[p][k][t] < (MAX_SEGS-1))
					index[p][k][t]++;
				}
				state = WAIT_FOR_SEG;
			    }
			    else
			    {
				selP->hours = seg;
				state = WAIT_FOR_MINUTES;
			    }
			    while (*argP && isdigit(*argP))
				++argP;
			}
			else if (state == WAIT_FOR_MINUTES)
			{
			    selP->minutes = atoi(argP);
			    if (selP->minutes < 0)
			    {
				printf("Invalid minutes: %s\n", argP);
				EXIT
			    }
			    state = WAIT_FOR_SECONDS;
			    while (*argP && isdigit(*argP))
				++argP;
			}
			else if (state == WAIT_FOR_SECONDS)
			{
			    selP->seconds = (FLOAT)(atof(argP));
			    if (selP->seconds < 0)
			    {
				printf("Invalid seconds: %s\n", argP);
				EXIT
			    }
			    if (selP == &start)
				state = CHECK_FOR_UPPER_LIMIT;
			    else
			    {
				for (k=c; k < max_chan; ++k)
				{
				    segP =
				      &SEQ_options.seg[p][k][t][index[p][k][t]];
				    segP->select.t.end = 
					(DOUBLE)(3600.0 * end.hours) +
					(DOUBLE)(60.0 * end.minutes) + 
					(DOUBLE)(end.seconds);

				    if (index[p][k][t] < (MAX_SEGS-1))
					index[p][k][t]++;
				}
				state = WAIT_FOR_SEG;
			    }
			    while (*argP && (isdigit(*argP) || *argP=='.'))
				++argP;
			}
			else
			{
			    printf("Unexpected number: %s\n", argP);
			    EXIT
			}
		    }
		    else if (*argP == '-')
		    {
			state = WAIT_FOR_UPPER_LIMIT;
			argP++;

			if (t == SEQ_TIME)
			{
			    for (k=c; k < max_chan; ++k)
			    {
				segP=&SEQ_options.seg[p][k][t][index[p][k][t]];
				segP->select.t.start = 
				    (DOUBLE)(3600.0 * start.hours) +
				    (DOUBLE)(60.0 * start.minutes) + 
				    (DOUBLE)(start.seconds);
			    }
			    selP = &end;
			}
		    }
		    else if (*argP == ':')
		    {
			if (t == SEQ_SEGNO)
			{
			    /* First time here so start = hours */
			    selP = &start;
			    segP = &SEQ_options.seg[p][c][t][index[p][c][t]];
			    start.hours = segP->select.n.start;
			    start.minutes = 0;
			    start.seconds = 0;
			    end.hours = 0;
			    end.minutes = 0;
			    end.seconds = 0;
			    t = SEQ_TIME;
			}

			if (t == SEQ_TIME)
			{
			    if (state == CHECK_FOR_UPPER_LIMIT)
				state = WAIT_FOR_MINUTES;
			}
			argP++;
		    }
		    else if (*argP == ',')
		    {
			argP++;
			if (state == WAIT_FOR_CHANNEL)
			{
			    /* All channels of this plugin */
			    channel_field[p] = 0;
			    max_chan = MAX_CHANNELS;
			    state = WAIT_FOR_FIRST_SEG;
			}
		    }
		    else
		    {
			printf("Unexpected value: %s\n", argP);
			EXIT
		    }
		}
		else  /* skip over white space if present */
		{
		    if ((i+1) < num_args)
		    {
			if (arguments[i+1][0] != '-')
			{
			    i++;
			    argP = &arguments[i][0];
			}
			else if (isdigit(arguments[i+1][1]))
			{
			    i++;
			    argP = &arguments[i][1];
			}
			else
			    keep_going = FALSE;
		    }
		    else
			keep_going = FALSE;
		}
	    }

	    if ((state == CHECK_FOR_UPPER_LIMIT) ||
		(state == WAIT_FOR_MINUTES) ||
		(state == WAIT_FOR_SECONDS))
	    {
		for (k=c; k < max_chan; ++k)
		{
		    if (t == SEQ_TIME)
		    {
			segP = &SEQ_options.seg[p][k][t][index[p][k][t]];
			segP->select.t.start = 
			    (DOUBLE)(3600.0 * start.hours) +
			    (DOUBLE)(60.0 * start.minutes) + 
			    (DOUBLE)(start.seconds);

			if ((selP == &end) && (end.hours != 0))
			    segP->select.t.end = 
				(DOUBLE)(3600.0 * end.hours) +
				(DOUBLE)(60.0 * end.minutes) + 
				(DOUBLE)(end.seconds);
			else
			    segP->select.t.end = segP->select.t.start;
		    }
		    if (index[p][k][t] < (MAX_SEGS-1))
			index[p][k][t]++;
		}
	    }
	    else if (state == WAIT_FOR_FIRST_SEG)
	    {
		for (k=c; k < max_chan; ++k)
		    SEQ_options.all_segs[p][k] = TRUE;
	    }
	    else if (state == WAIT_FOR_CHANNEL)
	    {
		channel_field[p] = 0;
		for (k=0; k < MAX_CHANNELS; ++k)
		    SEQ_options.all_segs[p][k] = TRUE;
	    }

	    if (all_values == TRUE)
	    {
		for (p=0; p < MAX_PLUGINS; ++p)
		    for (c=0; c < MAX_CHANNELS; ++c)
			SEQ_options.all_segs[p][c] = TRUE;
	    }

	    /* Now put a marker at the last location for each channel */
	    for (p=0; p < MAX_PLUGINS; ++p)
	    {
		for (c=0; c < MAX_CHANNELS; ++c)
		{
		    t = SEQ_SEGNO;
		    segP = &SEQ_options.seg[p][c][t][index[p][c][t]];
		    segP->select.n.start = -1L;
		    segP->select.n.end = -1L;
		    t = SEQ_TIME;
		    segP = &SEQ_options.seg[p][c][t][index[p][c][t]];
		    segP->select.t.start = (DOUBLE)-1;
		    segP->select.t.end = (DOUBLE)-1;
		}
	    }

	    /* Now sort the arrays for all plugins/channels */
	    for (p=0; p < MAX_PLUGINS; ++p)
	    {
		for (c=0; c < MAX_CHANNELS; ++c)
		{
		    t = SEQ_SEGNO;
		    segP = &SEQ_options.seg[p][c][t][0];
		    if (segP->select.n.start != -1L)
			qsort((VOID *)segP, index[p][c][t],
				sizeof(SEGS), compare_seg);

		    t = SEQ_TIME;
		    segP = &SEQ_options.seg[p][c][t][0];
		    if (segP->select.t.start != (DOUBLE)-1)
			qsort((VOID *)segP, index[p][c][t],
				sizeof(SEGS), compare_time);
		}
	    }

	}

        else if (!strncmp(arguments[i], "-f", 2)) /* select a format */
	{
	    keep_going = TRUE;
	    argP = &arguments[i][2];
	    state = WAIT_FOR_FMT;

	    while (keep_going && (i < num_args))
	    {
		if (*argP)
		{
		    if (state == WAIT_FOR_FMT)
		    {
			if (isalpha(*argP))
			{
			    if (toupper(*argP) == 'R')
				SEQ_options.format = SEQ_FORMAT_RAW;
			    else if (toupper(*argP) == 'C')
			    {
				if (toupper(argP[2]) == 'M')
				    SEQ_options.format = SEQ_FORMAT_COMPENSATED;
				else
				    SEQ_options.format = SEQ_FORMAT_CORRECTED;
			    }
			    else
			    {
				printf("Unknown format type: %s\n", argP);
				printf("Valid options are: RAW, COR, COM\n");
				EXIT
			    }

			    /* Write the default prt_fmt */
			    if (SEQ_options.format == SEQ_FORMAT_COMPENSATED)
				strcpy(SEQ_options.prt_fmt, "%g");

			    /* Check for hexadecimal output format */
			    state = WAIT_FOR_PRT_FMT;
			    while (*argP)
			    {
				if (*argP == ',')
				    break;
				else
				    argP++;
			    }

			    if (*argP != '\0')
			    {
				if (*argP == ',' && *(argP+1))
				{
				    strcpy(SEQ_options.prt_fmt, argP+1);
				    keep_going = FALSE;
				}
			    }
			}
			else
			{
			    if (*argP != ',')
			    {
				printf("Unexpected value: %s\n", argP);
				EXIT
			    }
			    else
			    {
				state = WAIT_FOR_PRT_FMT;
				argP++;
			    }
			}
		    }
		    else /* else WAIT_FOR_PRT_FMT */
		    {
			if (*argP != ',')
			{
			    strcpy(SEQ_options.prt_fmt, argP);
			    keep_going = FALSE;
			}
			else
			{
			    state = WAIT_FOR_PRT_FMT;
			    argP++;
			}
		    }
		}
		else  /* skip over white space if present */
		{
		    if ((i+1) < num_args)
		    {
			if (arguments[i+1][0] != '-')
			{
			    i++;
			    argP = &arguments[i][0];
			}
			else
			    keep_going = FALSE;
		    }
		    else
			keep_going = FALSE;
		}
	    }
	}

        else if (!strncmp(arguments[i], "-o", 2)) /* select output device */
	{
	    keep_going = TRUE;
	    argP = &arguments[i][2];

	    while (keep_going && (i < num_args))
	    {
		if (*argP)
		{
		    if (isalpha(*argP))
		    {
			SEQ_options.output.type = SEQ_OUTPUT_FILE;
			keep_going = FALSE;
		    }
		    else if (isdigit(*argP))
		    {
			k = atoi(argP);
			SEQ_options.output.type = SEQ_OUTPUT_SCREEN;

			if (k == 1)
			    SEQ_options.output.format=SEQ_SCREEN_OUTPUT_1;
			else if (k == 2)
			    SEQ_options.output.format=SEQ_SCREEN_OUTPUT_2;
			else
			{
			    printf("Invalid screen output option: %d\n", k);
			    printf("Valid options are: 1 or 2\n");
			    EXIT
			}
			keep_going = FALSE;
		    }
		    else
		    {
			printf("Unexpected value: %s\n", argP);
			EXIT
		    }
		}
		else  /* skip over white space if present */
		{
		    if ((i+1) < num_args)
		    {
			if (arguments[i+1][0] != '-')
			{
			    i++;
			    argP = &arguments[i][0];
			}
			else
			    keep_going = FALSE;
		    }
		    else
			keep_going = FALSE;
		}
	    }
	}

        else if (!strncmp(arguments[i], "-v", 2)) /* select verbose mode */
	{
	    if (arguments[i][2] == '1')
		SEQ_options.debug = 2;
	    else
		SEQ_options.debug = 1;
	}

        else if (!strncmp(arguments[i], "-p", 2)) /* print filter coeffs */
	{
	    SEQ_options.print_coeffs = 1;
	}

        else if (!strncmp(arguments[i], "-h", 2)) /* user needs help */
        {
	    seq_print_usage();
	    EXIT
        }
    }

    if (SEQ_options.output.type == SEQ_OUTPUT_FILE)
	SEQ_options.format = SEQ_FORMAT_CORRECTED;

    if ((no_segs == TRUE) && (SEQ_options.test_mode == FALSE))
    {
	fprintf(stderr, "\nNO SEGMENTS TO TRANSLATE.\n\n");
    }

    if (SEQ_options.debug & 3)
    {
	if (SEQ_options.format == SEQ_FORMAT_COMPENSATED)
	    strcpy(option, "COMPENSATED");
	else if (SEQ_options.format == SEQ_FORMAT_CORRECTED)
	    strcpy(option, "CORRECTED");
	else
	    strcpy(option, "RAW");
	printf("Data format: %s\n", option);
	printf("Data destination: %s\n", 
	    SEQ_options.output.type == SEQ_OUTPUT_FILE ? "FILE" : "SCREEN");
	if (SEQ_options.output.type != SEQ_OUTPUT_FILE)
	{
	    if (SEQ_options.output.format == SEQ_SCREEN_OUTPUT_1)
		printf("    Data only in 1 column.\n");
	    else
		printf("    Time,Data in 2 columns.\n");
	}
	if (SEQ_options.test_mode == TRUE)
	    printf("Test diagnostic mode active.\n");

	for (p=0; p < MAX_PLUGINS; ++p)
	{
	    for (c=0; c < MAX_CHANNELS; ++c)
	    {
		if (SEQ_options.all_segs[p][c] == TRUE)
		    printf("%c%d: ALL SEGS\n", p+'A', c+1);
		else
		{
		    t = SEQ_SEGNO;
		    for (j=0; j < MAX_SEGS; ++j)
		    {
			segP = &SEQ_options.seg[p][c][t][j];
			if (segP->select.n.start != -1L)
			    printf("%c%d: %ld, %ld\n", p+'A', c+1,
			    segP->select.n.start, segP->select.n.end);
			else
			    break;
		    }
		    t = SEQ_TIME;
		    for (j=0; j < MAX_SEGS; ++j)
		    {
			segP = &SEQ_options.seg[p][c][t][j];
			if (segP->select.t.start != (DOUBLE)-1)
			    printf("%c%d: %.9g sec, %.9g sec\n", p+'A', c+1,
			    segP->select.t.start, segP->select.t.end);
			else
			    break;
		    }
		}
	    }
	}
    }

    return(&filename[0]);
}

/*XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX*/

static void seq_print_usage()

/*--------------------------------------------------------------------------

    Purpose: To display a usage message for the user.

    Inputs:

    Outputs:

    Machine dependencies:

    Notes:

    Procedure:

/CODE
--------------------------------------------------------------------------*/
{   /* seq_print_usage() */

    printf("\nusage: seqtran <file> [ options ]\n\n");
    printf("options are:\n");
    printf("\
file= SCSI data file\n\
-t  = Print the times of all segments relative to segment 1 (default = off)\n\
      to the screen as hours:minutes:seconds\n\
-a  = print acquisition parameters (descriptors) to screen  (default = off)\n\
      ex: -aA1,B2     (Print descriptor for plugin A, channel 1 and \n\
		       plugin B, channel 2)\n\
	  -aB	      (Print descriptor for plugin B all channels)\n\
	  -a	      (Print descriptors for all plugins, all channels)\n");
    printf("\
-s  = segment number(s) to correct                   (default = no segments)\n\
      ex: -sA2,25,63,B1,100-150  (Plugin A, chan 2, segs 25 and 63,\n\
				  Plugin B, chan 1, segs 100 thru 150)\n\
	  -sA3,1:20:15-25:0:0    (Plugin A, chan 3, segs acquired 1 hour,\n\
				  20 minutes and 15 seconds after seg 1 and\n\
				  before 25 hours after seg 1)\n\
	  -sB,2:20,A2,99	 (The first segment of all channels of \n\
				  plugin B acquired 2 hours, 20 minutes and\n\
				  0 seconds after the first segment,\n");
    printf("\
				  plugin A channel 2 segment 99)\n\
	  -sA,13,52		 (Segs 13 & 52 of all channels of plugin A)\n\
	  -sA2,B1		 (All segs of plugin A, channel 2 and \n\
				  plugin B channel 1)\n\
	  -sA		         (All segs of all channels of plugin)\n\
	  -s		         (All segs of all channels of all plugins)\n");
    printf("\
-f  = format of the data                                    (default = COM)\n\
      -fRAW[,fmt] = uncorrected raw data, promoted to 16 bits   (fmt=%%d)\n\
      -fCOR[,fmt] = corrected with filter coefficients 	        (fmt=%%d)\n\
      -fCOM[,fmt] = corrected and compensated for vertical gain (fmt=%%g)\n\
	     fmt, if specified, overrides the default.\n\
		  ex: -fRAW,%%04x outputs raw uncorrected data in hexadecimal\n");
    printf("\
-o  = output destination for results			    (default = -o1)\n\
      -oF = write complete corrected waveform (descriptor + data) to disk\n\
	    file in binary starting with trace_PC.000 to trace_PC.999 where \n\
	    P=plugin, C=channel (max 1000 segs for each plugin/chan). Note \n\
	    that -f does not apply with this option and is fixed as COR.\n\
      -o1 = print data only in ASCII to the screen in 1 column\n\
      -o2 = print time,data in ASCII to the screen in 2 columns\n");
    printf("\
-d  = Diagnostic test mode: compare last segment to corrected segment\n\
       (all channels) and print any differences as a byte offset.\n\
-p  = Print filter coefficients to the screen	            (default = off)\n\
-v  = Verbose mode: print progress                          (default = off)\n");
}

/*XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX*/

INT compare_seg(seg1P, seg2P)
    SEGS *seg1P;
    SEGS *seg2P;

/*--------------------------------------------------------------------------

    Purpose: To compare 2 array elements and return a value specifying their
	     relationship. The qsort() function calls this routine one or
	     more times during the sort, passing pointers to two array
	     elements on each call. This routine then compares the elements,
	     and returns one of three values.

    Inputs: seg1P = pointer to one SEGS array element
	    seg2P = pointer to another SEGS array element

    Outputs:  Less than 0 = seg1P < seg2P
			0 = seg1P == seg2P
	   Greater than 0 = seg1P > seg2P

    Machine dependencies:

    Notes: 

    Procedure:

/CODE
--------------------------------------------------------------------------*/
{   /* compare_seg() */

LONG diff; 

    diff = seg1P->select.n.start - seg2P->select.n.start;

    if (diff < 0L)
	return -1;
    else if (diff > 0L)
	return 1;
    else
	return 0;
}

/*XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX*/

INT compare_time(seg1P, seg2P)
    SEGS *seg1P;
    SEGS *seg2P;

/*--------------------------------------------------------------------------

    Purpose: To compare 2 array elements and return a value specifying their
	     relationship. The qsort() function calls this routine one or
	     more times during the sort, passing pointers to two array
	     elements on each call. This routine then compares the elements,
	     and returns one of three values.

    Inputs: seg1P = pointer to one SEGS array element
	    seg2P = pointer to another SEGS array element

    Outputs:  Less than 0 = seg1P < seg2P
			0 = seg1P == seg2P
	   Greater than 0 = seg1P > seg2P

    Machine dependencies:

    Notes: 

    Procedure:

/CODE
--------------------------------------------------------------------------*/
{   /* compare_time() */

DOUBLE diff; 

    diff = seg1P->select.t.start - seg2P->select.t.start;

    if (diff < (DOUBLE)0.0)
	return -1;
    else if (diff > (DOUBLE)0.0)
	return 1;
    else
	return 0;
}

