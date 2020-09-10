/* --------------- SCSI Sequence Acquisition Program ------------------

This is a program to transfer SCSI Sequence data from a LeCroy 7200A
oscilloscope to a file.

This file contains the parts of the program which ARE NOT dependent on
the machine, operating system, or compiler which is used to execute
this program.

A separate file contains the parts of the program which are dependent on
the machine, operating system, and compiler.  For the MS-DOS version, the
file is named acq_dos.c.  In addition, other files are used to provide the
SCSI interface.

The command required to execute this program is explained in the function
"usage", below.

Author:  Donald Reaves, ATS Division, LeCroy Corp.

-----------------------------------------------------------------------

			Implementation Notes

This program attempts to transfer data from a SCSI device to a file.  Each
block of data read from SCSI has the same size, which is determined when the
first block of data is received.  If it is not possible to determine the size
of a block of SCSI data automatically, the actual size may be set using a
command parameter.

In general, if an unrecoverable error occurs, the program is terminated with
an explanatory message.

The constant "DEBUG" may be specified when compiling this program.  In this
case, the command option "-v" is available to enable printing of diagnostic
and status messages.

The items in this file are ordered to make explicit forward declarations
unnecessary.  Therefor, the main function is last.

----------------------------------------------------------------------- */

#include "stdio.h"
#include "stdlib.h"
#include "string.h"

/* -------------------------------------------------------------------- */

/* Constants. */

#ifndef TRUE
/* Boolean values. */
#define FALSE 0
#define TRUE  1
#endif /* TRUE */

#ifndef NULL
/* Empty pointer value. */
#define NULL  0
#endif /* NULL */

/* END_PACKET checks to see if a packet is the last packet. */

#define PACKET_MASK    ((unsigned short) 0x7FFF)
#define END_PACKET(x)  (((x) & PACKET_MASK) == PACKET_MASK)

/* -------------------------------------------------------------------- */

/* Types. */

typedef char BOOL;              /* TRUE/FALSE data type                 */

/* -------------------------------------------------------------------- */

/* Command parameter variables. */

BOOL quiet;                     /* do not print anything                */
BOOL verbose;                   /* print diagnostic information if TRUE */
BOOL to_mem;                    /* time reading SCSI data into memory   */
BOOL time_it;                   /* time transfers to disk               */
unsigned long block_size;       /* SCSI data block size                 */
unsigned long blocks_to_alloc;  /* number of data buffers allocated     */
int scope_id;                   /* SCSI identifier of scope             */
int time_out;                   /* SCSI timeout, in seconds             */
int tran_speed;			/* Transfer speed (see SCSI document)   */
char file_name[200];            /* name of the output file              */

/* -------------------------------------------------------------------- */

/* Default values for command parameters.  These may need to be adjusted
   when porting this program to a new operating system. */

#define QUIET            FALSE    /* quiet flag value                   */
#define VERBOSE          FALSE    /* verbose flag value                 */
#define TO_MEM           FALSE    /* to_mem flag value                  */
#define TIME_IT          FALSE    /* time_it flag value                 */
#define BLOCK_SIZE       65536L   /* SCSI data block size               */
#define BLOCKS_TO_ALLOC  10L      /* number of data buffers allocated   */
#define SCOPE_ID         7        /* SCSI identifier of scope           */
#define TIME_OUT         500      /* SCSI time out (5 seconds)          */
#define FILE_NAME        "output" /* name of the output file            */
#define TRAN_SPEED	 0	  /* Transfer speed default is 5 MB/sec */

/* -------------------------------------------------------------------- */

/* Other global data. */

unsigned short **block_list;   /* array of pointers to allocated blocks */
unsigned long blocks_allocated;/* number of blocks in block_list        */
unsigned long input_block;     /* number of block being read from SCSI  */
unsigned long output_block;    /* number of block being written to disk */
unsigned long blocks_full;     /* number of blocks containing data      */
unsigned long total_blocks;    /* total # of SCSI blocks transferred    */
unsigned short packet_number;  /* current packet number                 */

/* -------------------------------------------------------------------- */

/* Definitions of external routines.  See acq_dos.c for details. */

extern void open_file();
extern void write_file();
extern void close_file();

extern void initialize_scsi();
extern void start_scsi_input();
extern int check_scsi_input();
extern void turn_off_scsi();

extern void *allocate_memory();
extern void *reallocate_memory();
extern void free_memory();

extern void short_delay();
extern void start_timing();
extern unsigned long elapsed_time();

/*XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX*/

void usage()

/* -----------------------------------------------------------------------

    Purpose:
	Print the command usage message and exit.

    Inputs:

    Outputs:

    Notes:
	This routine does not return.

/CODE
----------------------------------------------------------------------- */

{   /* usage */

    (void) printf("ACQUIRE Version 1.2, June 20, 1994\n\n");
    (void) printf("usage:  acquire [ options ]\n");
    (void) printf("\noptions are:\n");
    (void) printf("  -h\t\tprint this message\n");
    (void) printf("  -q\t\tdo not print anything\n");
    (void) printf("  -v\t\tprint diagnostic and status messages\n");
    (void) printf("  -t\t\tprint timing information about SCSI transfers\n");
    (void) printf("  -m\t\ttransfer SCSI data to memory (no disk output)\n");
    (void) printf("  bs=#\t\tset SCSI block size to # (default is %ld)\n",
			BLOCK_SIZE);
    (void) printf("  nb=#\t\tallocate # SCSI blocks (default is %ld)\n",
			BLOCKS_TO_ALLOC);
    (void) printf("  id=#\t\tSCSI identifier of scope (default is %d)\n",
			SCOPE_ID);
    (void) printf("  timeout=#\tSCSI time out in seconds(0-327) (default is %d)\n",
			TIME_OUT/100);
    (void) printf("  file=name\toutput file name (default is %s)\n",
			FILE_NAME);
    (void) printf("  speed=#\tSCSI transfer speed (see SCSI manual)\n");

    /* Shut down the SCSI interface. */
	turn_off_scsi();

    exit (1);
}

/*XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX*/

void error_exit(format, argument)
    char *format, *argument;

/* -----------------------------------------------------------------------

    Purpose:
	Write an error message and exit.

    Inputs:
	format -- printf-style format string
	argument -- a string argument for printf

    Outputs:

    Machine dependencies:

    Notes:
	This routine does not return.

/CODE
----------------------------------------------------------------------- */

{   /* error_exit */
    static BOOL already_called = FALSE;

    if (!already_called)
    {
        already_called = TRUE;

	(void) fprintf(stderr, format, argument);

	/* Shut down the SCSI interface. */
	    turn_off_scsi();
    }

    exit (1);
}

/*XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX*/

void allocate_blocks(blockP)
    unsigned short *blockP;

/* -----------------------------------------------------------------------

    Purpose:
	Set up the structures to hold SCSI data in memory.

    Inputs:
	blockP -- pointer to the first block for the list or NULL if none
	global block_size -- size of each SCSI block
	global blocks_to_alloc -- number of blocks to allocate

    Outputs:
	global blocks_allocated -- set to actual number of blocks allocated
	global block_list -- allocated
	global input_block -- reset
	global output_block -- reset
	global blocks_full -- reset

    Notes:

/CODE
----------------------------------------------------------------------- */

{   /* allocate_blocks */
    unsigned long i;

    /* Allocate the block list. */
	block_list = (unsigned short **) allocate_memory(
		(unsigned long) (blocks_to_alloc * sizeof (block_list[0])));
	if (block_list == (unsigned short **) NULL)
	    error_exit("Cannot allocate memory for SCSI data\n", (char *) NULL);

    /* Use blockP for the first block in the list if it is not NULL. */
	if (blockP != NULL)
	{
	    block_list[0] = blockP;
	    i = 1;
	}
	else
	    i = 0;

    /* Fill in the remainder of the block list. */
	for (; i < blocks_to_alloc; ++i)
	{
	    block_list[i] = (unsigned short *) allocate_memory(block_size);
	    if (block_list[i] == (unsigned short *) NULL)
		break;
	}

    /* Note the number of blocks allocated. */
	blocks_allocated = i;

#ifdef DEBUG
    if (verbose)
    {
	(void) printf("allocated %ld data blocks\n", blocks_allocated);
	(void) printf("actual block size = %ld\n", block_size);
    }
#endif /* DEBUG */
}

/*XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX*/

void initialize(ac, av)
    int ac;
    char *av[];

/* -----------------------------------------------------------------------

    Purpose:
	Initialize global data and initialize the SCSI interface.

    Inputs:
	ac -- the number of command parameters (including the name of the
	      program)
	av -- a pointer to an array of pointers to the command parameters

    Outputs:
	global command parameter variables:
	    quiet
	    verbose
	    to_mem
	    time_it
	    block_size
	    blocks_to_alloc
	    scope_id
	    file_name
	output file created

    Notes:

/CODE
----------------------------------------------------------------------- */

{   /* initialize */
    int i;

    /* Set the default values for command parameters. */
	quiet = QUIET;
	verbose = VERBOSE;
	to_mem = TO_MEM;
	time_it = TIME_IT;
	block_size = BLOCK_SIZE;
	blocks_to_alloc = BLOCKS_TO_ALLOC;
	scope_id = SCOPE_ID;
	time_out = TIME_OUT;
	tran_speed = TRAN_SPEED;
	(void) strcpy(file_name, FILE_NAME);

    /* Process the command parameters. */
	for (i = 1; i < ac; ++i)
	{
	    if (!strncmp(av[i], "-h", 2))
		usage();
	    else if (!strncmp(av[i], "-q", 2))
	    {
		quiet = TRUE;
		verbose = FALSE;
	    }
	    else if (!strncmp(av[i], "-v", 2))
	    {
		quiet = FALSE;
		verbose = TRUE;
	    }
	    else if (!strncmp(av[i], "-m", 2))
		to_mem = TRUE;
	    else if (!strncmp(av[i], "-t", 2))
		time_it = TRUE;
	    else if (!strncmp(av[i], "bs=", 3))
		block_size = (unsigned long) atol(av[i] + 3);
	    else if (!strncmp(av[i], "nb=", 3))
		blocks_to_alloc = (unsigned long) atol(av[i] + 3);
	    else if (!strncmp(av[i], "id=", 3))
		scope_id = atoi(av[i] + 3);
	    else if (!strncmp(av[i], "timeout=", 8))
	    {
		if (atoi(av[i]+8) > 327  ||  atoi(av[i]+8) < 0)
		{
		    printf("Invalid timeout value: %d, value not changed\n",
				atoi(av[i]+8));
		}
		else
		    time_out = 100 * atoi(av[i] + 8);
	    }
	    else if (!strncmp(av[i], "file=", 5))
		(void) strcpy(file_name, av[i] + 5);
	    else if (!strncmp(av[i], "speed=", 6))
		tran_speed = atoi(av[i] + 6);
	    else
		usage();
	}

#ifdef DEBUG
    if (verbose)
    {
	/* Print the values of command parameters. */
	    (void) printf("block size = %ld\n", block_size);
	    (void) printf("blocks to allocate = %ld\n", blocks_to_alloc);
	    (void) printf("id = %d\n", scope_id);
	    (void) printf("file name = %s\n", file_name);
    }
#endif /* DEBUG */

    /* Initialize the SCSI interface. */
	initialize_scsi(scope_id);

    /* Open the output file. */
	if (!to_mem)
	    open_file(file_name);

    /* Allocate the block list, assuming the block size is correct. */
	allocate_blocks((unsigned short *) NULL);
}

/*XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX*/

void get_first_block()

/* -----------------------------------------------------------------------

    Purpose:
	Read the first block of SCSI data, and set up the structures to
	hold SCSI data in memory.

    Inputs:

    Outputs:

    Notes:
	All SCSI blocks for a particular set of 7200A SCSI Sequence data
	are the same length.  The size of each block is determined by
	measuring the size of the first block of data.  This is done by
	attempting to read a very large SCSI block and having the SCSI
	hardware tell us exactly how large the block was.

/CODE
----------------------------------------------------------------------- */

{   /* get_first_block */
    unsigned short *blockP;
    unsigned long i, size;
    int status;

    /* Get a pointer to the first block. */
	blockP = block_list[0];

    /* Initiate a read from SCSI. */
	start_scsi_input(blockP, block_size);
	++total_blocks;

    /* Wait for the operation to complete. */
	while ((status = check_scsi_input(&size, FALSE)) == 0)
	    short_delay();

    if (status == 3)
    {   /* An error occurred. */

	error_exit("SCSI error occurred while reading first block\n",
			(char *) NULL);
    }

    /* Get the packet number. */
	packet_number = *blockP;

    if (verbose)
	(void) printf("read SCSI packet %04x\n", packet_number);
    else if (!quiet)
	(void) printf("%04x\b\b\b\b", packet_number);

    if (size != block_size)
    {   /* The block size is not what was specified on the command line. */

	/* Free blocks in the block list except the first one. */
	    for (i = 1; i < blocks_allocated; ++i)
		free_memory((void *) block_list[i]);

	/* Free the block list. */
	    free_memory((void *) block_list);

	/* Adjust the SCSI block size if necessary. */
	    if (size < block_size)
	    {
		blockP = reallocate_memory(blockP, block_size, size);
		block_size = size;
	    }

	/* Allocate the block list. */
	    allocate_blocks(blockP);
    }

    /* The first block is now in the block list. */
	if (blocks_allocated == 1)
	    input_block = 0;
	else
	    input_block = 1;
	output_block = 0;
	blocks_full = 1;

    /* Reset statistics. */
	total_blocks = 1;
	start_timing();
}

/*XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX*/

void report_statistics()

/* -----------------------------------------------------------------------

    Purpose:
	Report statistics about amount and rate of data transferred.

    Inputs:

    Outputs:

    Machine dependencies:

    Notes:

/CODE
----------------------------------------------------------------------- */

{   /* report_statistics */
    unsigned long msec = elapsed_time();
    double bytes, rate;

    (void) printf("total of %ld blocks = %.0f bytes transferred\n",
		total_blocks, (double) total_blocks * (double) block_size);
    (void) printf("elapsed time = %ld msec\n", msec);

    if (msec != 0)
    {
	/* Compute the number of bytes transferred after the first block. */
	    bytes = (double) (total_blocks - 1) * (double) block_size;

	/* Compute the transfer rate in bytes/sec. */
	    rate = bytes / (double) msec * 1000.0;

	(void) printf("transfer rate = %.0f bytes/sec\n", rate);
    }
#ifdef DEBUG
    if (verbose)
    {
	/* Print statistics about the SCSI interface */
	AHA_print_setup(0x330);
    }
#endif /* DEBUG */
}

/*XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX*/

void transfer_data_to_memory()

/* -----------------------------------------------------------------------

    Purpose:
	Read SCSI Sequence data and transfer them to memory.  Print the
	time required for the transfer to standard output.

    Inputs:
	global block_size -- size of each SCSI block
	global blocks_allocated -- number of blocks allocated
	global block_list -- pointers to data blocks
	global input_block -- current input block number
	global output_block -- current output block number
	global blocks_full -- number of blocks containing data

    Outputs:

    Notes:

/CODE
----------------------------------------------------------------------- */

{   /* transfer_data_to_memory */
    unsigned long size;
    int status;

    /* Transfer SCSI data to memory. */
	while (!END_PACKET(packet_number))
	{
	    /* Initiate the next transfer. */
		start_scsi_input(block_list[input_block], block_size);
		++total_blocks;

	    /* Wait for the operation to complete. */
		while ((status = check_scsi_input(&size, TRUE)) == 0)
		    short_delay();

	    if (status != 0)
	    {
		/* Check for errors. */
		    if (status == 3)
			error_exit("SCSI error during transfer\n",
					    (char *) NULL);

		/* Get the packet number from the SCSI data. */
		    packet_number = *block_list[input_block];

		if (verbose)
		    (void) printf("read SCSI packet %04x\n", packet_number);
		else if (!quiet)
		    (void) printf("%04x\b\b\b\b", packet_number);
	    }
	}
}

/*XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX*/

void transfer_data_to_disk()

/* -----------------------------------------------------------------------

    Purpose:
	Read SCSI Sequence data and transfer them to the output file.

    Inputs:
	global block_size -- size of each SCSI block
	global blocks_allocated -- number of blocks allocated
	global block_list -- pointers to data blocks
	global input_block -- current input block number
	global output_block -- current output block number
	global blocks_full -- number of blocks containing data

    Outputs:

    Notes:

/CODE
----------------------------------------------------------------------- */

{   /* transfer_data_to_disk */
    unsigned long size;
    int status;
    BOOL input_pending = FALSE;

    /* Read from the SCSI port to fill the allocated buffers. */
	while (blocks_full < blocks_allocated && !END_PACKET(packet_number))
	{
#ifdef DEBUG
	    if (verbose)
		(void) printf("initiate SCSI read of block #%ld\n",
					input_block);
#endif /* DEBUG */

	    /* Initiate the next transfer. */
		start_scsi_input(block_list[input_block], block_size);
		++total_blocks;

	    /* Wait for the operation to complete. */
		while ((status = check_scsi_input(&size, TRUE)) == 0)
		    short_delay();

	    /* Check for errors. */
		if (status == 3)
		    error_exit("SCSI error during transfer\n", (char *) NULL);

	    /* Get the packet number from the SCSI data. */
		packet_number = *block_list[input_block];

#ifdef DEBUG
	    if (verbose)
		(void) printf("read SCSI block #%ld, packet %04x\n",
				input_block, packet_number);
#endif /* DEBUG */

	    /* Update the number of the next block to read. */
		if (++input_block == blocks_allocated)
		    input_block = 0;

	    /* Update the number of blocks containing data. */
		++blocks_full;
	}

    /* Interleave SCSI reads and disk writes to complete the transfer. */
	while (!END_PACKET(packet_number))
	{
	    if (blocks_full != 0)
	    {   /* There is at least one block available to write. */

		/* Write the data to the disk. */
		    write_file(block_list[output_block], block_size);

		if (verbose)
		    (void) printf("write packet %04x\n",
				    *block_list[output_block]);
		else if (!quiet)
		    (void) printf("%04x\b\b\b\b", *block_list[output_block]);

		/* Update the number of the next block to write. */
		    if (++output_block == blocks_allocated)
			output_block = 0;

		/* Update the number of blocks containing data. */
		    --blocks_full;
	    }

	    if (!input_pending)
	    {   /* There is no input operation in progress. */

		if (blocks_full < blocks_allocated)
		{   /* There is at least one empty block. */
#ifdef DEBUG
		    if (verbose)
			(void) printf("initiate SCSI read of block #%ld\n",
					input_block);
#endif /* DEBUG */

		    /* Initiate the next transfer. */
			start_scsi_input(block_list[input_block], block_size);
			++total_blocks;

		    /* There is now an input pending. */
			input_pending = TRUE;
		}
	    }
	    else
	    {   /* There is an input operation in progress. */

		if ((status = check_scsi_input(&size, TRUE)) != 0)
		{
		    /* The operation has completed. */
			input_pending = FALSE;

		    /* Check for errors. */
			if (status == 3)
			    error_exit("SCSI error during transfer\n",
						(char *) NULL);

		    /* Get the packet number from the SCSI data. */
			packet_number = *block_list[input_block];

#ifdef DEBUG
		    if (verbose)
			(void) printf("read SCSI block #%ld, packet %04x\n",
					input_block, packet_number);
#endif /* DEBUG */

		    /* Update the number of the next block to read. */
			if (++input_block == blocks_allocated)
			    input_block = 0;

		    /* Update the number of blocks containing data. */
			++blocks_full;
		}
	    }
	}

    /* Write the remaining data to the disk. */
	while (blocks_full != 0)
	{
	    /* Write the data to the disk. */
		write_file(block_list[output_block], block_size);

	    if (verbose)
		(void) printf("write packet %04x\n",
				    *block_list[output_block]);
	    else if (!quiet)
		(void) printf("%04x\b\b\b\b", *block_list[output_block]);

	    /* Update the number of the next block to write. */
		if (++output_block == blocks_allocated)
		    output_block = 0;

	    /* Update the number of blocks containing data. */
		--blocks_full;
	}
}

/*XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX*/

int main(ac, av)
    int ac;
    char *av[];

/* -----------------------------------------------------------------------

    Purpose:
	This is the main function for the SCSI Sequence acquisition program.

    Inputs:
	ac -- the number of command parameters (including the name of the
	      program)
	av -- a pointer to an array of pointers to the command parameters

    Outputs:
	The return value is 0 on success and 1 on failure.

    Notes:

/CODE
----------------------------------------------------------------------- */

{   /* main */

    /* Initialize everything except the SCSI data buffer structure. */
	initialize(ac, av);

    /* Get the first block of SCSI data. */
	get_first_block();

    if (to_mem)
    {
	/* Transfer data from SCSI to memory and time it. */
	    transfer_data_to_memory();
    }
    else
    {
	/* Transfer data from SCSI to the output file. */
	    transfer_data_to_disk();
    }

    /* Close the output file. */
	close_file();

    if (time_it)
	report_statistics();

    /* Shut down the SCSI interface. */
	turn_off_scsi();

    /* Return success. */
	return (0);
}

/* ------------------------  end of file  ----------------------------- */

