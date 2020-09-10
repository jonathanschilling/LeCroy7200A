/************************** ris_args.c *************************************

Author: Larry Salant		LeCroy Corporation
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

CHAR *RIS_process_arguments(num_args, arguments)
int num_args;
char *arguments[];

/*--------------------------------------------------------------------------

    Purpose: To process all the arguments on the command line.

    Inputs: Various configuration arguments; see seq_print_usage()

    Outputs:

--------------------------------------------------------------------------*/
{   /* RIS_process_arguments() */

    /* index into seg array */
    WORD index[MAX_PLUGINS][MAX_CHANNELS][MAX_SEG_TYPES]; 
    CHAR *argP;
    BYTE p,c;
    WORD i,j,k,t;
    WORD state;
    BOOL keep_going;
    BOOL all_values;
    WORD max_chan;
    CHAR option[32];

    static CHAR filename[80];

    /* Set default values */
    SEQ_options.packed = TRUE;
    SEQ_options.debug = FALSE;
    SEQ_options.test_mode = FALSE;
    SEQ_options.print_times = FALSE;
    SEQ_options.output.type = SEQ_OUTPUT_SCREEN;
    SEQ_options.output.format = SEQ_SCREEN_OUTPUT_1;
    SEQ_options.format = SEQ_FORMAT_COMPENSATED;
    SEQ_options.process_plugin = RIS_PROC_ALL;
    SEQ_options.process_chan = RIS_PROC_ALL;
    strcpy (SEQ_options.prt_fmt, "%g");
    plugin_field = 0;

    for (p=0; p < MAX_PLUGINS; ++p)
    {
	channel_field[p] = 0;
	for (c=0; c < MAX_CHANNELS; ++c)
	{
	    SEQ_options.print_params[p][c] = FALSE;
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
        else if (!strncmp(arguments[i], "-d", 2)) /* Test mode active */
        {
	    SEQ_options.test_mode = TRUE;
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
				SEQ_options.format = SEQ_FORMAT_COMPENSATED;
			    }
			    else
			    {
				printf("Unknown format type: %s\n", argP);
				printf("Valid options are: RAW, COM\n");
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
	    else if (arguments[i][2] == '2')
		SEQ_options.debug = 3;
	    else if (arguments[i][2] == '3')
		SEQ_options.debug = 4; /* print illegal tdc to stderr */
	    else if (arguments[i][2] == '4')
		SEQ_options.debug = 5; /* tdc histogram */
	    else
		SEQ_options.debug = 1;
	}
        else if (!strncmp(arguments[i], "-p", 2)) /* select plugin to process */
	{
	    if (arguments[i][2] == 'A')
		SEQ_options.process_plugin = RIS_PROC_A;
	    else
		SEQ_options.process_plugin = RIS_PROC_B;

	    if (arguments[i][3] == '1')
		SEQ_options.process_chan = 0;
	    else if (arguments[i][3] == '2')
		SEQ_options.process_chan = 1;
	}
        else if (!strncmp(arguments[i], "-c", 2)) /* data not compressed */
	    SEQ_options.packed = FALSE;
        else if (!strncmp(arguments[i], "-h", 2)) /* user needs help */
        {
	    seq_print_usage();
	    EXIT
        }
    }

    if (SEQ_options.debug == 1)
    {
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

    printf("\nusage: ristran <file> [ options ]\n\n");
    printf("where file = SCSI data file & options are:\n");
    printf("\
-c  = data is not compressed descriptors (default = compressed)\n\
-a  = print acquisition parameters (descriptors) to screen  (default = off)\n\
      ex: -aA1,B2     (Print descriptor for plugin A, channel 1 and \n\
		       plugin B, channel 2)\n\
	  -aB	      (Print descriptor for plugin B all channels)\n\
	  -a	      (Print descriptors for all plugins, all channels)\n");
    printf("\
-p  = A or B plugin [and channel] to process (default = ALL)\n\
          -pA1        only process plugin A channel 1 data\n\
	  -pB         only process plugin B (all channels) data\n");
    printf("\
-f  = format of the data                                    (default = COM)\n\
      -fRAW[,fmt] = raw data, promoted to 16 bits   (fmt=%%d)\n\
      -fCOM[,fmt] = compensated for vertical gain (fmt=%%g)\n\
	     fmt, if specified, overrides the default.\n\
		  ex: -fRAW,%%04x outputs raw uncorrected data in hexadecimal\n");
    printf("\
-o  = output destination for results			    (default = -o1)\n\
      -oF = write complete waveform (descriptor + data) to disk in binary\n\
	    [filename is trace_PC.000 where  P=plugin, C=channel] \n\
      -o1 = print data only in ASCII to the screen in 1 column\n\
      -o2 = print time,data in ASCII to the screen in 2 columns\n");
    printf("\
-v  = Verbose mode: print progress                          (default = off)\n");
}

