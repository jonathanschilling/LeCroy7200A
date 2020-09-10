/* --------------- SCSI Sequence Acquisition Program ------------------

This is a program to transfer SCSI Sequence data from a LeCroy 7200A
oscilloscope to a file.

This file contains the parts of the program which ARE dependent on
the machine, operating system, or compiler which is used to execute
this program.

Author:  Donald Reaves, ATS Division, LeCroy Corp.

----------------------------------------------------------------------- */

#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include "wat_port.h"
#include "fcntl.h"
#include "io.h"
#include "dos.h"
#include "time.h"
#include "conio.h"
#include "aha1540b.h"

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

#ifndef O_BINARY
#define O_BINARY 0
#endif /* O_BINARY */

#define MAX_WRITE (31 * 1024)

/* -------------------------------------------------------------------- */

/* Data local to this file. */

static int output_file;            /* the output file descriptor        */
static unsigned long current_size; /* size of SCSI block being read     */
static unsigned short *current_data;/* address of block being read      */
static unsigned long start_time;   /* time when start_timing was called */
static unsigned long time_limit;   /* time to abort a SCSI transfer     */
static int retry_count;            /* number of retries for SCSI xfer   */
static int host_id;                /* SCSI identifier of host interface */
static int scope_id;               /* SCSI identifier of scope          */
static CCB input_ccb;              /* SCSI channel input control block  */

#ifdef DEBUG
BOOL print_ccb_flag =  FALSE;      /* Print CCB from short_delay.       */
#endif /* DEBUG */

/* -------------------------------------------------------------------- */

/* Definitions of external routines. */

extern void error_exit();
extern int time_out;               /* time-out value for SCSI transfers */
extern char quiet;                 /* controls error prints             */
extern char verbose;               /* controls diagnostic prints        */

/*XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX*/

void open_file(name)
    char *name;

/* -----------------------------------------------------------------------

    Purpose:
        Open a file for binary output.

    Inputs:
        name -- the full pathname of the file

    Outputs:

    Machine dependencies:

    Notes:
        This routine exits on error.

/CODE
----------------------------------------------------------------------- */

{   /* open_file */

    /* Attempt to open the output file. */
        output_file = open(name, (int) (O_RDWR|O_CREAT|O_TRUNC|O_BINARY), 0666);

    if (output_file == -1)
        error_exit("Cannot open output file '%s'\n", name);
}

/*XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX*/

void write_file(data, size)
    unsigned short *data;
    unsigned long size;

/* -----------------------------------------------------------------------

    Purpose:
        Write a block of data to the output file.

    Inputs:
        data -- pointer to the data
        size -- number of bytes to write

    Outputs:

    Machine dependencies:

    Notes:
        This routine exits on error.

	The Microsoft write routine cannot write more than 32767 bytes at
	a time.  The MAX_WRITE constant (31K) prevents attempting to write
	too many bytes at a time, and keeps each write at a multiple of 1 Kb
	if possible.

/CODE
----------------------------------------------------------------------- */

{   /* write_file */
    int bytes_written;
    unsigned bytes;

    while (size != 0)
    {
	/* Determine the number of bytes to write this time. */
	    if (size > (unsigned long) MAX_WRITE)
		bytes = MAX_WRITE;
	    else
		bytes = (unsigned) size;

	/* Attempt the write. */
	    bytes_written = write(output_file, (void *) data, bytes);

	/* Check for errors. */
	    if (bytes_written != bytes)
		error_exit("Cannot write to output file\n", (char *) NULL);

	/* Update the number of bytes left. */
	    size -= bytes;

	/* Update the data pointer. */
	    data = (unsigned short *) ((char *) data + bytes);
    }
}

/*XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX*/

void close_file()

/* -----------------------------------------------------------------------

    Purpose:
        Close the output file.

    Inputs:

    Outputs:

    Machine dependencies:

    Notes:

/CODE
----------------------------------------------------------------------- */

{   /* close_file */

    (void) close(output_file);
}

/*XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX*/

unsigned long get_current_time()

/* -----------------------------------------------------------------------

    Purpose:
	Get the current system time, in hundredths of a second.

    Inputs:

    Outputs:
	The return value is the number of 1/100th seconds of the current time.

    Machine dependencies:

    Notes:
	This routine is only ever called to get a start and stop time with the
	difference being important. As such, the difference becomes invalid
	when the most significant time unit wraps. In this case, the time unit
	wraps each day. To extend the time accuracy, _dos_getdate should be
	called and the day, month, and possibly year taken into account.

/CODE
----------------------------------------------------------------------- */

{   /* get_current_time */

    struct dostime_t time;
    unsigned long hseconds;

    _dos_gettime (&time);

    hseconds = (unsigned long)(time.hour) * 60 * 60 * 100 +
		(unsigned long)(time.minute) * 60 * 100 +
	        (unsigned long)(time.second) * 100 +
	       (unsigned long)(time.hsecond);

    return ((unsigned long) hseconds);
}

/*XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX*/

void start_timing()

/* -----------------------------------------------------------------------

    Purpose:
        Start timing an interval.

    Inputs:

    Outputs:

    Machine dependencies:

    Notes:

/CODE
----------------------------------------------------------------------- */

{   /* start_timing */

    start_time = get_current_time();
}

/*XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX*/

unsigned long elapsed_time()

/* -----------------------------------------------------------------------

    Purpose:
        Return the elapsed time, in msec, since start_timing was called.

    Inputs:

    Outputs:

    Machine dependencies:

    Notes:

/CODE
----------------------------------------------------------------------- */

{   /* elapsed_time */

    return ((unsigned long) (get_current_time() - start_time) * 10L);
}

/*XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX*/

void turn_off_scsi()

/* -----------------------------------------------------------------------

    Purpose:
	Turn off the SCSI interface.  This prevents interrupts from
	occurring after the interrupt handler is abandoned.

    Inputs:

    Outputs:

    Machine dependencies:

    Notes:

/CODE
----------------------------------------------------------------------- */

{   /* turn_off_scsi */

    /* Turn off SCSI interrupts. */
	(void) outp(0xA1, 0xBD);

    /* Reset the board. */
	(VOID) AHA_hard_reset();
}

/*XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX*/

void short_delay()

/* -----------------------------------------------------------------------

    Purpose:
        Wait for a little while.  In a multitasking system, it may be
        necessary to allow other tasks to execute.  In a single task
        system, such as MS-DOS, it is acceptable to waste some time.

    Inputs:

    Outputs:

    Machine dependencies:

    Notes:
        Most operating systems have to ability to terminate a program
        from the keyboard regardless of its activity.  In MS-DOS, this is
        not always possible, so this routine checks the keyboard to see
        if control-C is pressed.

/CODE
----------------------------------------------------------------------- */

{   /* short_delay */
    extern BOOL AHA_hard_reset();

#ifdef DEBUG
    if (print_ccb_flag)
    {
	print_ccb_flag = FALSE;
	AHA_print_CCB(&input_ccb);
    }
#endif /* DEBUG */

    if (kbhit())
    {
	/* Turn off the SCSI board. */
	    turn_off_scsi();

	/* Exit the program. */
	    exit (2);
    }
}

/*XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX*/

void initialize_scsi(id)
    int id;

/* -----------------------------------------------------------------------

    Purpose:
        Initialize the SCSI interface to accept data from the indicated
        SCSI id to logical unit 7.

    Inputs:
        id -- the SCSI identifier of the scope

    Outputs:

    Machine dependencies:

    Notes:
        This routine exits on error.

/CODE
----------------------------------------------------------------------- */

{   /* initialize_scsi */

    /* Turn off SCSI interrupts. */
	(void) outp(0xA1, 0xBD);

    /* Initialize the SCSI board. */
	host_id = SCSI_setup();

    /* Make sure the board is present. */
	if (host_id < 0)
	    error_exit("No SCSI board is present.\n", (char *) NULL);

    /* Save the identifier of the scope. */
	scope_id = id;

    /* Make sure the scope is ready. */
	if (!SCSI_test_ready(scope_id, TARGET_LUN))
	    error_exit("SCSI ID %d not ready\n", (char *) scope_id);
}

/*XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX*/

void start_scsi_input(data, size)
    unsigned short *data;
    unsigned long size;

/* -----------------------------------------------------------------------

    Purpose:
        Initiate a SCSI read.

    Inputs:
        data -- the location at which to store the data
        size -- the number of bytes to read

    Outputs:

    Machine dependencies:

    Notes:
        This routine exits on error.

/CODE
----------------------------------------------------------------------- */

{   /* start_scsi_input */

    /* Initialize input_ccb for reading data from scope_id. */
	SCSI_init_CCB(&input_ccb, scope_id, TARGET_LUN, OP_TCCB, OP_SEND,
			(VOID *) data, size, 0);

    /* Enable input from the source. */
	if (!SCSI_start(&input_ccb))
	    error_exit("unable to start SCSI input.\n", (char *) NULL);

    /* Save the size and address of the buffer. */
	current_size = size;
	current_data = data;

    /* Note the time at which we will abort the transfer. */
	time_limit = get_current_time() + time_out;
	retry_count = 0;
}

/*XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX*/

int check_scsi_input(sizeP, enable_timeout)
    unsigned long *sizeP;
    BOOL enable_timeout;

/* -----------------------------------------------------------------------

    Purpose:
        Check the progress of a previously initiated SCSI input operation.

    Inputs:
	enable_timeout -- If TRUE, if the transfer doesn't complete within
			  time_out seconds, timeout processing will begin.

    Outputs:
        *sizeP -- set to the number of bytes transferred
        The return value indicates the status of the most recently initiated
        input operation:
            0 -- operation is in progress
            1 -- operation completed normally (*sizeP set)
            2 -- operation completed with short record (*sizeP set)
            3 -- error

    Machine dependencies:

    Notes:

/CODE
----------------------------------------------------------------------- */

{   /* check_scsi_input */
    unsigned long size, current_time;

    if (!input_ccb.int_received)
    {   /* The operation is still in progress. */

	/* Return now if timeout processing is not enabled. */
	    if (!enable_timeout)
		return (0);

	/* Return now if the time limit hasn't expired. */
	    if ((current_time = get_current_time()) < time_limit)
		return (0);

	/* Indicte an error if the retry count has expired. */
	    if (++retry_count >= 5)
		return (3);

	/* Reset the SCSI board. */
	    if (!quiet)
		(VOID) printf("Try again...\n");
	    (VOID) SCSI_setup();

	/* Restart the operation. */
	    SCSI_init_CCB(&input_ccb, scope_id, TARGET_LUN, OP_TCCB, OP_SEND,
			    (VOID *) current_data, current_size, 0);
	    (VOID) SCSI_start(&input_ccb);
	    time_limit = current_time + time_out;

	/* Return, indicating that the operation is not complete. */
	    return (0);
    }

    /* The operation has completed.  Check for errors. */
	switch (input_ccb.mbi_flag)
	{
	    case MBI_COMPLETE:
		/* The command completed normally. */
		    *sizeP = current_size;
		    return (1);

	    case MBI_ERROR:
		if (input_ccb.hastat == HS_OVER_RUN)
		{   /* The command completed with an over or underrun. */

		    /* Get the size of the block. */
			size = AHA_get_3_data(&input_ccb.data_len_2);

		    if (size < current_size)
		    {   /* The command terminated with a short block. */

			*sizeP = size;
			return (2);
		    }
		}
		break;

	    default:
		break;
	}

    /* Since we got here, there was an error. */
	return (3);
}

/*XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX*/

void *allocate_memory(size)
    unsigned long size;

/* -----------------------------------------------------------------------

    Purpose:
        Allocate a block of memory.

    Inputs:
        size -- the number of bytes to allocate

    Outputs:
        The return value is a pointer to the allocated memory, or NULL if
        none could be allocated.

    Machine dependencies:

    Notes:
        This routine is used rather than the standard "malloc" since some
        implementations (e.g. Microsoft C with MS-DOS) have 16-bit integers.
        Because the argument to malloc is limited by the size of an integer,
        it may therefore be unable to allocate more than 32 Kb or 64 Kb,
        which is insufficient for this program.

/CODE
----------------------------------------------------------------------- */

{   /* allocate_memory */

    return (halloc((long) size, (size_t) 1));
}

/*XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX*/

void *reallocate_memory(data, old_size, new_size)
    void *data;
    unsigned long old_size, new_size;

/* -----------------------------------------------------------------------

    Purpose:
        Adjust the size of a previously allocated block of memory.

    Inputs:
        data -- points to the previously allocated memory
        old_size -- the old size of the block
	new_size -- the new required size of the block

    Outputs:
        The return value is a pointer to the new block of memory.  The
        contents of the block is unchanged, except that some data will be
        lost at the end if the block is reduced in size.  If the block is
        expanded, the additional bytes at the end are undefined.

    Machine dependencies:

    Notes:
	If the attempt to allocate the new block fails, the old block is
	returned, as long as the old block was at least as large as the
	new one.

/CODE
----------------------------------------------------------------------- */

{   /* reallocate_memory */
    char *oldP, *newP;
    unsigned long bytes;

    /* Try to allocate the required amount of memory. */
	newP = (char *) halloc((long) new_size, (size_t) 1);

    if (newP != (char *) NULL)
    {    /* The allocation succeeded. */

	/* Get a copy of the data to be copied. */
	    oldP = (char *) data;

	/* Save the pointer to the data to be returned. */
	    data = (void *) newP;

	/* Determine the number of bytes to copy. */
	    bytes = old_size;
	    if (bytes > new_size)
		bytes = new_size;

	/* Copy the data. */
	    while (bytes != 0)
	    {
		*newP = *oldP;
		++newP;
		++oldP;
		--bytes;
	    }
    }
    else
    {   /* The allocate failed. */

	if (new_size > old_size)
	    error_exit("realloc: unable to allocate memory\n", (char *) NULL);
    }

    return (data);
}

/*XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX*/

void free_memory(data)
    void *data;

/* -----------------------------------------------------------------------

    Purpose:
        Return a previously allocated block of memory to the system.

    Inputs:
        data -- pointer to the block to be freed

    Outputs:

    Machine dependencies:

    Notes:

/CODE
----------------------------------------------------------------------- */

{   /* free_memory */

    hfree(data);
}

/* ------------------------  end of file  ----------------------------- */

