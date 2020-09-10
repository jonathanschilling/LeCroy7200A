/* -------- Low-level routines for the AHA-1540B SCSI Controller -------

This file contains low-level routines for the AHA-1540B SCSI Controller.

Author:  Donald Reaves

----------------------------------------------------------------------- */

#include "aha1540b.h"
#include <string.h>
#include "conio.h"

/* -------------------------------------------------------------------- */

/* Diagnostic definitions. */

#ifdef DEBUG
extern INT printf();
#define PRINTF(x)	{ (VOID) printf x; }
#else /* DEBUG */
#define PRINTF(x)	{ }
#endif /* DEBUG */

/* -------------------------------------------------------------------- */

/* External definitions. */

extern VOID ATM_set_isp();

/*XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX*/

VOID AHA_put_3_addr(byteP, addr)
    UBYTE *byteP;
    VOID *addr;

/* -----------------------------------------------------------------------

    Purpose:
	Store a memory address in a 3-byte SCSI address field, high byte first.

    Inputs:
	byteP -- the address of the high byte of the 3-byte address field
	addr -- the address to be stored

    Outputs:

    Machine dependencies:

    Notes:

    Procedure:

/CODE
----------------------------------------------------------------------- */

{   /* AHA_put_3_addr */
    ULONG linear_addr;

    /* Compute the linear address from the logical address. */
#ifdef MSDOS
	linear_addr = ((ULONG) addr & 0xFFFFL) +
		      ((((ULONG) addr >> 12) & 0xFFFF0L));
#else /* MSDOS */
	linear_addr = (ULONG) addr;
#endif /* MSDOS */

    /* Store the address as 3 bytes with the high byte first. */
	*byteP++ = (UBYTE) (linear_addr >> 16);
	*byteP++ = (UBYTE) (linear_addr >> 8);
	*byteP = (UBYTE) linear_addr;
}

/*XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX*/

VOID *AHA_get_3_addr(byteP)
    UBYTE *byteP;

/* -----------------------------------------------------------------------

    Purpose:
	Read a memory address from a 3-byte SCSI address field, high byte first.

    Inputs:
	byteP -- the address of the high byte of the 3-byte address field

    Outputs:
	The return value is the address.

    Machine dependencies:

    Notes:

    Procedure:

/CODE
----------------------------------------------------------------------- */

{   /* AHA_get_3_addr */
    ULONG linear_addr;
    VOID *addr;

    /* Read the 3-byte address field into linear_addr. */
	linear_addr = ((ULONG) *byteP++) << 16;
	linear_addr |= ((ULONG) *byteP++) << 8;
	linear_addr |= (ULONG) *byteP;

    /* Compute the logical address from the linear address. */
#ifdef MSDOS
	/*lint -e511*/
	addr = (VOID *) (((linear_addr & 0xFFFF0L) << 12) | (linear_addr & 15));
	/*lint +e511*/
#else /* MSDOS */
	addr = (VOID *) linear_addr;
#endif /* MSDOS */

    /* Return the logical address. */
	return (addr);
}

/*XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX*/

VOID AHA_put_3_data(byteP, value)
    UBYTE *byteP;
    ULONG value;

/* -----------------------------------------------------------------------

    Purpose:
	Store a value in a 3-byte SCSI data field, high byte first.

    Inputs:
	byteP -- the address of the high byte of the 3-byte data field
	value -- the value to be stored

    Outputs:

    Machine dependencies:

    Notes:

    Procedure:

/CODE
----------------------------------------------------------------------- */

{   /* AHA_put_3_data */

    /* Store the value as 3 bytes with the high byte first. */
	*byteP++ = (UBYTE) (value >> 16);
	*byteP++ = (UBYTE) (value >> 8);
	*byteP = (UBYTE) value;
}

/*XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX*/

ULONG AHA_get_3_data(byteP)
    UBYTE *byteP;

/* -----------------------------------------------------------------------

    Purpose:
	Read a value from a 3-byte SCSI data field, high byte first.

    Inputs:
	byteP -- the address of the high byte of the 3-byte data field

    Outputs:
	The return value is the extracted value.

    Machine dependencies:

    Notes:

    Procedure:

/CODE
----------------------------------------------------------------------- */

{   /* AHA_get_3_data */
    ULONG value;

    /* Read the 3-byte address field into value. */
	value = ((ULONG) *byteP++) << 16;
	value |= ((ULONG) *byteP++) << 8;
	value |= (ULONG) *byteP;

    /* Return the value. */
	return (value);
}

/*XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX*/

BOOL AHA_hard_reset()

/* -----------------------------------------------------------------------

    Purpose:
	Perform a hard reset of the AHA-1540B.

    Inputs:

    Outputs:
	The return value is the TRUE on success and FALSE on failure.

    Machine dependencies:

    Notes:

    Procedure:

/CODE
----------------------------------------------------------------------- */

{   /* AHA_hard_reset */
    ULONG timeout = 10000;

    /* Assert the hard reset bit in the control register. */
	(void) outp(AHA_CTRL, AHA_HRST);

    /* Wait for AHA_STAT.AHA_STST to be zero. */
	while ((inp(AHA_STAT) & AHA_STST) != 0)
	{
	    if (--timeout == 0)
	    {
		PRINTF(("AHA_hard_reset timeout\n"))
		return (FALSE);
	    }
	    PAUSE
	}

    /* Check for failure. */
	if ((inp(AHA_STAT) & AHA_DIAGF) != 0)
	{
	    PRINTF(("AHA_hard_reset: diagnostic failure\n"))
	    return (FALSE);
	}

    /* Check for success. */
	if ((inp(AHA_STAT) & AHA_INIT) != 0)
	    return (TRUE);

    /* Something else happened. */
	PRINTF(("AHA_hard_reset: unexpected failure\n"))
	return (FALSE);
}

/*XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX*/

#ifdef NEEDED

static BOOL AHA_soft_reset()

/* -----------------------------------------------------------------------

    Purpose:
	Perform a soft reset of the AHA-1540B.

    Inputs:

    Outputs:
	The return value is the TRUE on success and FALSE on failure.

    Machine dependencies:

    Notes:

    Procedure:

/CODE
----------------------------------------------------------------------- */

{   /* AHA_soft_reset */
    ULONG timeout = 10000;

    /* Assert the soft reset bit in the control register. */
	(void) outp(AHA_CTRL, AHA_SRST);

    /* Wait for AHA_STAT.AHA_IDLE to be one. */
	while ((inp(AHA_STAT) & AHA_IDLE) == 0)
	{
	    if (--timeout == 0)
	    {
		PRINTF(("AHA_soft_reset timeout\n"))
		return (FALSE);
	    }
	    PAUSE
	}

    /* Check for success. */
	if ((inp(AHA_STAT) & AHA_INIT) != 0)
	    return (TRUE);

    /* Something else happened. */
	PRINTF(("AHA_soft_reset: unexpected failure\n"))
	return (FALSE);
}

#endif /* NEEDED */

/*XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX*/

VOID AHA_reset_interrupt()

/* -----------------------------------------------------------------------

    Purpose:
	Reset any interrupt condition in the AHA-1540B.

    Inputs:

    Outputs:

    Machine dependencies:

    Notes:

    Procedure:

/CODE
----------------------------------------------------------------------- */

{   /* AHA_reset_interrupt */

    /* Assert the interrupt reset bit in the control register. */
	(void) outp(AHA_CTRL, AHA_IRST);
}

/*XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX*/

#ifdef NEEDED

static VOID AHA_reset_scsi()

/* -----------------------------------------------------------------------

    Purpose:
	Reset the SCSI Bus in the AHA-1540B.

    Inputs:

    Outputs:

    Machine dependencies:

    Notes:

    Procedure:

/CODE
----------------------------------------------------------------------- */

{   /* AHA_reset_scsi */

    /* Assert the SCSI reset bit in the control register. */
	(void) outp(AHA_CTRL, AHA_SCRST);
}

#endif /* NEEDED */

/*XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX*/

static BOOL AHA_output_byte(value)
    UBYTE value;

/* -----------------------------------------------------------------------

    Purpose:
	Output a single byte of data to the AHA-1540B SCSI controller.

    Inputs:
	value -- the value to be written

    Outputs:
	The return value is TRUE on success, FALSE on failure.

    Machine dependencies:

    Notes:

    Procedure:

/CODE
----------------------------------------------------------------------- */

{   /* AHA_output_byte */
    ULONG timeout = 10000;

    /* Wait for AHA_STAT.AHA_CDF to be zero. */
	while ((inp(AHA_STAT) & AHA_CDF) != 0)
	{
	    if (--timeout == 0)
	    {
		PRINTF(("AHA_output_byte timeout\n"))
		return (FALSE);
	    }
	    PAUSE
	}

    /* Write the value to the data port. */
	(void) outp(AHA_DATA, (INT) value);

    return (TRUE);
}

/*XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX*/

static UBYTE AHA_input_byte()

/* -----------------------------------------------------------------------

    Purpose:
	Input a single byte of data from the AHA-1540B SCSI controller.

    Inputs:

    Outputs:
	The return value is the value read from the data port.

    Machine dependencies:

    Notes:

    Procedure:

/CODE
----------------------------------------------------------------------- */

{   /* AHA_input_byte */
    ULONG timeout = 10000;

    /* Wait for AHA_STAT.AHA_DF to be one. */
	while ((inp(AHA_STAT) & AHA_DF) == 0)
	{
	    if (--timeout == 0)
	    {
		PRINTF(("AHA_input_byte timeout\n"))
		return (0);
	    }
	    PAUSE
	}

    /* Return the value of the data port. */
	return ((UBYTE) inp(AHA_DATA));
}

/*XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX*/

static VOID AHA_wait_for_ready()

/* -----------------------------------------------------------------------

    Purpose:
	Wait until the AHA-1540B is ready to accept a command byte.

    Inputs:

    Outputs:

    Machine dependencies:

    Notes:

    Procedure:

/CODE
----------------------------------------------------------------------- */

{   /* AHA_wait_for_ready */
    ULONG timeout = 10000;

    /* Wait for AHA_STAT.AHA_IDLE to be one. */
	while ((inp(AHA_STAT) & AHA_IDLE) == 0)
	{
	    if (--timeout == 0)
	    {
		PRINTF(("AHA_wait_for_ready timeout\n"))
		return;
	    }
	    PAUSE
	}
}

/*XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX*/

static VOID AHA_check_end_command()

/* -----------------------------------------------------------------------

    Purpose:
	Check for proper end of command.

    Inputs:

    Outputs:

    Machine dependencies:

    Notes:

    Procedure:

/CODE
----------------------------------------------------------------------- */

{   /* AHA_check_end_command */
    ULONG timeout = 10000;
    UBYTE status;

    /* Wait for AHA_STAT.AHA_IDLE to be one. */
	while (((status = inp(AHA_STAT)) & AHA_IDLE) == 0)
	{
	    if (--timeout == 0)
	    {
		PRINTF(("Command did not complete\n"))
		return;
	    }
	    PAUSE
	}

    /* Check for invalid command. */
	if ((status & AHA_INVC) != 0)
	    PRINTF(("Invalid SCSI command\n"))

    /* Clear the interrupt if necessary. */
	if ((inp(AHA_IFLG) & AHA_ANY) != 0)
	    AHA_reset_interrupt();
}

/*XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX*/

static BOOL AHA_init_mailbox(num, addr)
    UBYTE num;
    MAILBOX_ENTRY *addr;

/* -----------------------------------------------------------------------

    Purpose:
	Initialize the mailbox in the AHA-1540B.

    Inputs:
	num -- number of output slots == number of input slots
	addr -- address of the mailbox array

    Outputs:
	The return value is TRUE on success, FALSE on failure.

    Machine dependencies:

    Notes:

    Procedure:

/CODE
----------------------------------------------------------------------- */

{   /* AHA_init_mailbox */
    UBYTE i;
    register MAILBOX_ENTRY *mP = addr;
    UBYTE addr_3[3];

    /* Clear the mailbox outs. */
	for (i = 0; i < num; ++i, ++mP)
	{
	    mP->flag = MBO_FREE;
	    mP->addr_2 = 0;
	    mP->addr_1 = 0;
	    mP->addr_0 = 0;
	}

    /* Clear the mailbox ins. */
	for (i = 0; i < num; ++i, ++mP)
	{
	    mP->flag = MBI_FREE;
	    mP->addr_2 = 0;
	    mP->addr_1 = 0;
	    mP->addr_0 = 0;
	}

    /* Wait for the board to be ready for a command. */
	AHA_wait_for_ready();

    /* Write the opcode. */
	if (!AHA_output_byte(MBOX_INIT))
	    return (FALSE);

    /* Write the number of slots in the mailbox. */
	if (!AHA_output_byte(num))
	    return (FALSE);

    /* Store the address of the mailbox in addr_3. */
	AHA_put_3_addr(addr_3, (VOID *) addr);

    /* Write the address to the mailbox. */
	if (!AHA_output_byte(addr_3[0]))
	    return (FALSE);
	if (!AHA_output_byte(addr_3[1]))
	    return (FALSE);
	if (!AHA_output_byte(addr_3[2]))
	    return (FALSE);

    /* Check for proper end of command. */
	AHA_check_end_command();

    return (TRUE);
}

/*XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX*/

BOOL AHA_start_scsi_command()

/* -----------------------------------------------------------------------

    Purpose:
	Start a SCSI command.

    Inputs:

    Outputs:
	The return value is TRUE on success, FALSE on failure.

    Machine dependencies:

    Notes:

    Procedure:

/CODE
----------------------------------------------------------------------- */

{   /* AHA_start_scsi_command */

    /* Write the opcode. */
	if (!AHA_output_byte(START_SCSI_CMD))
	    return (FALSE);
    
    return (TRUE);
}

/*XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX*/

BOOL AHA_inquire(board_id, options, rev)
    CHAR *board_id;
    INT *options;
    CHAR *rev;

/* -----------------------------------------------------------------------

    Purpose:
	Adapter Inquiry.

    Inputs:

    Outputs:
	board_id -- a string identifying the board is copied into this buffer
	options -- set to the special options byte
	rev -- a string indicating the revision is copied into this buffer
	The return value is TRUE on success, FALSE on failure.

    Machine dependencies:

    Notes:
	The buffers passed to receive the board_id and the revision must be
	large enough to hold the result.  No check is made for buffer overflow.

    Procedure:

/CODE
----------------------------------------------------------------------- */

{   /* AHA_inquire */
    CHAR *p;

    /* Wait for the board to be ready for a command. */
	AHA_wait_for_ready();

    /* Write the opcode. */
	if (!AHA_output_byte(ADPT_INQUIRY))
	    return (FALSE);

    /* Read and decode the board ID byte. */
	switch (AHA_input_byte())
	{
	    case 0:     p = "AHA-1540 (16)"; break;
	    case 0x30:  p = "AHA-1540 (64)"; break;
	    case 0x41:  p = "AHA-154XB";     break;
	    case 0x42:  p = "AHA-1640";      break;
	    default:    p = "unknown";       break;
	}
	(VOID) strcpy(board_id, p);

    /* Read the options byte. */
	*options = (INT) AHA_input_byte();

    /* Read and print the firmware revision. */
	rev[0] = (INT) AHA_input_byte();
	rev[1] = '.';
	rev[2] = (INT) AHA_input_byte();
	rev[3] = '\0';

    /* Check for proper end of command. */
	AHA_check_end_command();

    return (TRUE);
}

/*XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX*/

#ifdef NEEDED

static BOOL AHA_enable_mailbox_interrupt(value)
    UBYTE value;

/* -----------------------------------------------------------------------

    Purpose:
	Enable (or disable) the mailbox out available interrupt.

    Inputs:
	value -- 1 to enable interrupts, 0 to disable interrupts

    Outputs:
	The return value is TRUE on success, FALSE on failure.

    Machine dependencies:

    Notes:

    Procedure:

/CODE
----------------------------------------------------------------------- */

{   /* AHA_enable_mailbox_interrupt */

    /* Wait for the board to be ready for a command. */
	AHA_wait_for_ready();

    /* Write the opcode. */
	if (!AHA_output_byte(EN_MBOX_OUT_INT))
	    return (FALSE);

    /* Write the value. */
	if (!AHA_output_byte(value))
	    return (FALSE);

    return (TRUE);
}

#endif /* NEEDED */

/*XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX*/

#ifdef NEEDED

static BOOL AHA_set_sel_timeout(value)
    LONG value;

/* -----------------------------------------------------------------------

    Purpose:
	Set Selection Time Out.

    Inputs:
	value -- timeout in milliseconds, or -1 for none

    Outputs:
	The return value is TRUE on success, FALSE on failure.

    Machine dependencies:

    Notes:

    Procedure:

/CODE
----------------------------------------------------------------------- */

{   /* AHA_set_sel_timeout */

    /* Wait for the board to be ready for a command. */
	AHA_wait_for_ready();

    /* Write the opcode. */
	if (!AHA_output_byte(SET_SEL_TIMEOUT))
	    return (FALSE);

    /* Write the enable/disable value. */
	if (!AHA_output_byte((UBYTE) ((value < 0) ? 0 : 1)))
	    return (FALSE);

    /* Write the reserved byte. */
	if (!AHA_output_byte(0))
	    return (FALSE);

    /* Write the timeout duration. */
	if (!AHA_output_byte((UBYTE) ((value & 0xFF00) >> 8)))
	    return (FALSE);
	if (!AHA_output_byte((UBYTE) (value & 0xFF)))
	    return (FALSE);

    /* Check for proper end of command. */
	AHA_check_end_command();

    return (TRUE);
}

#endif /* NEEDED */

/*XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX*/

#ifdef NEEDED

static BOOL AHA_set_bus_times(on, off)
    UBYTE on, off;

/* -----------------------------------------------------------------------

    Purpose:
	Set Bus On and Off Times.

    Inputs:
	on -- bus on time, in microseconds
	off -- bus off time, in microseconds

    Outputs:
	The return value is TRUE on success, FALSE on failure.

    Machine dependencies:

    Notes:
	The value of on should be between 2 and 15, inclusive.
	The value of off should be between 1 and 64, inclusive.

    Procedure:

/CODE
----------------------------------------------------------------------- */

{   /* AHA_set_bus_times */

    /* Wait for the board to be ready for a command. */
	AHA_wait_for_ready();

    /* Write the opcode. */
	if (!AHA_output_byte(SET_BUS_ON_TIME))
	    return (FALSE);

    /* Write the on time. */
	if (!AHA_output_byte(on))
	    return (FALSE);

    /* Check for proper end of command. */
	AHA_check_end_command();

    /* Wait for the board to be ready for a command. */
	AHA_wait_for_ready();

    /* Write the opcode. */
	if (!AHA_output_byte(SET_BUS_OFF_TIME))
	    return (FALSE);

    /* Write the off time. */
	if (!AHA_output_byte(off))
	    return (FALSE);

    /* Check for proper end of command. */
	AHA_check_end_command();

    return (TRUE);
}

#endif /* NEEDED */

/*XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX*/

static BOOL AHA_set_tran_speed(value)
    UBYTE value;

/* -----------------------------------------------------------------------

    Purpose:
	Set Transfer Speed.

    Inputs:
	value -- selection value

	    Values 0x00 through 0x04 specify standard times:
		    0x00 -- 5.0 MB/sec
		    0x01 -- 6.7 MB/sec
		    0x02 -- 8.0 MB/sec
		    0x03 -- 10 MB/sec
		    0x04 -- 5.7 MB/sec

	    Values 0x80 through 0xFF specify custom times.  Consider the
	    value to be a binary number in the following format:

		    1 RRR S WWW

		read pulse width  = RRR * 50 + 100  (ns)
		strobe off time   =  S  * 50 + 100  (ns)
		write pulse width = WWW * 50 + 100  (ns)


    Outputs:
	The return value is TRUE on success, FALSE on failure.

    Machine dependencies:

    Notes:

    Procedure:

/CODE
----------------------------------------------------------------------- */

{   /* AHA_set_tran_speed */

    /* Wait for the board to be ready for a command. */
	AHA_wait_for_ready();

    /* Write the opcode. */
	if (!AHA_output_byte(SET_TRAN_SPEED))
	    return (FALSE);

    /* Write the value. */
	if (!AHA_output_byte(value))
	    return (FALSE);

    /* Check for proper end of command. */
	AHA_check_end_command();

    return (TRUE);
}

/*XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX*/

BOOL AHA_ret_conf_data(dma, intr, scsi_id)
    INT *dma, *intr, *scsi_id;

/* -----------------------------------------------------------------------

    Purpose:
	Return Configuration Data.

    Inputs:

    Outputs:
	dma -- set to the dma channel used by the board
	intr -- set to the interrupt channel used by the board
	scsi_id -- set to the SCSI id for the board

	The return value is TRUE on success, FALSE on failure.

    Machine dependencies:

    Notes:

    Procedure:

/CODE
----------------------------------------------------------------------- */

{   /* AHA_ret_conf_data */

    /* Wait for the board to be ready for a command. */
	AHA_wait_for_ready();

    /* Write the opcode. */
	if (!AHA_output_byte(RET_CONF_DATA))
	    return (FALSE);

    /* Read and decode the DMA byte. */
	switch (AHA_input_byte())
	{
	    case 0x80:  *dma = 7;  break;
	    case 0x40:  *dma = 6;  break;
	    case 0x20:  *dma = 5;  break;
	    case 0x01:  *dma = 0;  break;
	    default:    return (FALSE);
	}

    /* Read and decode the interrupt byte. */
	switch (AHA_input_byte())
	{
	    case 0x40:  *intr = 15; break;
	    case 0x20:  *intr = 14; break;
	    case 0x08:  *intr = 12; break;
	    case 0x04:  *intr = 11; break;
	    case 0x02:  *intr = 10; break;
	    case 0x01:  *intr = 9;  break;
	    default:    return (FALSE);
	}

    /* Read the SCSI ID byte. */
	*scsi_id = (INT) (AHA_input_byte() & 7);

    /* Check for proper end of command. */
	AHA_check_end_command();

    return (TRUE);
}

/*XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX*/

static BOOL AHA_en_target_mode(mask)
    UBYTE mask;

/* -----------------------------------------------------------------------

    Purpose:
	Enable Target Mode.

    Inputs:
	mask -- a bit mask indicating which logical units are to be targets
		If bit b is 1, logical unit b will be enabled as a target.
		If all bits are off, no logical units will act at targets.

    Outputs:
	The return value is the TRUE on success and FALSE on failure.

    Machine dependencies:

    Notes:

    Procedure:

/CODE
----------------------------------------------------------------------- */

{   /* AHA_en_target_mode */

    /* Wait for the board to be ready for a command. */
	AHA_wait_for_ready();

    /* Write the opcode. */
	if (!AHA_output_byte(EN_TARGET_MODE))
	    return (FALSE);

    if (mask == 0)
    {
	/* Disable target operation. */
	    if (!AHA_output_byte(0))
		return (FALSE);

	/* Write a non-zero mask byte. */
	    if (!AHA_output_byte(1))
		return (FALSE);
    }
    else
    {
	/* Enable target operation. */
	    if (!AHA_output_byte(1))
		return (FALSE);

	/* Write the mask byte. */
	    if (!AHA_output_byte(mask))
		return (FALSE);
    }

    /* Check for proper end of command. */
	AHA_check_end_command();

    return (TRUE);
}

/*XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX*/

BOOL AHA_ret_setup_data(sdt,par,speed,on,off,n_mb,mb_addr,sper,soff,disc)
    INT *sdt, *par, *speed, *on, *off, *n_mb;
    ULONG *mb_addr;
    INT sper[8], soff[8], disc[8];

/* -----------------------------------------------------------------------

    Purpose:
	Return Setup Data.

    Inputs:

    Outputs:
	sdt -- synchronous data transfer negotiation enabled (1) or disabled (0)
	par -- parith checking enabled (1) or disabled (0)
	speed -- transfer speed byte
	on -- bus on time, in microseconds
	off -- bus off time, in microseconds
	n_mb -- number of mailboxes
	mb_addr -- mailbox base address (if n_mb != 0)
	sper[i] -- synchronous negotiation period (ns) for target i
	soff[i] -- synchronous negotiation offset for target i
	disc[i] -- disconnection is allowed (1) or disallowed (0) for target i
	The return value is TRUE on success, FALSE on failure.

    Machine dependencies:

    Notes:
       If synchronous negotiation for target i has not taken place, the
       corresponding period and offset are set to -1.

       A zero value in soff[i] indicates asynchronous transfer with target i.

    Procedure:

/CODE
----------------------------------------------------------------------- */

{   /* AHA_ret_setup_data */
    UBYTE value;
    INT i, period, offset;

    /* Wait for the board to be ready for a command. */
	AHA_wait_for_ready();

    /* Write the opcode. */
	if (!AHA_output_byte(RET_SETUP_DATA))
	    return (FALSE);

    /* Request 17 bytes of data. */
	if (!AHA_output_byte(17))
	    return (FALSE);

    /* Read and decode the SDT/Parity byte. */
	value = AHA_input_byte();
	*sdt = (value & 1) != 0;
	*par = (value & 2) != 0;

    /* Get the transfer speed. */
	*speed = (INT) AHA_input_byte();

    /* Get the Bus On Time. */
	*on = (INT) AHA_input_byte();

    /* Get the Bus Off Time. */
	*off = (INT) AHA_input_byte();

    /* Get the Mailbox info. */
	*n_mb = (INT) AHA_input_byte();
	*mb_addr = ((ULONG) AHA_input_byte()) << 16;
	*mb_addr |= ((ULONG) AHA_input_byte()) << 8;
	*mb_addr |= (ULONG) AHA_input_byte();

    /* Get synchronous negotiation info for each target. */
	for (i = 0; i < 8; ++i)
	{
	    value = AHA_input_byte();
	    if ((value & 0x80) != 0)
	    {
		offset = (INT) (value & 15);
		period = (INT) ((value >> 4) & 7);
		period = 200 + 50 * period;
	    }
	    else
	    {
		period = -1;
		offset = -1;
	    }
	    sper[i] = period;
	    soff[i] = offset;
	}

    /* Get the disconnect option. */
	value = AHA_input_byte();
	for (i = 0; i < 8; ++i, value >>= 1)
	    disc[i] = (value & 1) == 0;

    /* Check for proper end of command. */
	AHA_check_end_command();

    return (TRUE);
}

/*XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX*/

#ifdef NEEDED

static BOOL AHA_set_adpt_options(mask)
    UBYTE mask;

/* -----------------------------------------------------------------------

    Purpose:
	Set Host Adapter Options.

    Inputs:
	mask -- disconnection option byte mask
		If bit b is 1, disconnection is disallowed for target b.

    Outputs:
	The return value is TRUE on success, FALSE on failure.

    Machine dependencies:

    Notes:
	The default state is to allow disconnection.

    Procedure:

/CODE
----------------------------------------------------------------------- */

{   /* AHA_set_adpt_options */

    /* Wait for the board to be ready for a command. */
	AHA_wait_for_ready();

    /* Write the opcode. */
	if (!AHA_output_byte(SET_ADPT_OPTIONS))
	    return (FALSE);

    /* Write the number of bytes to follow. */
	if (!AHA_output_byte(1))
	    return (FALSE);

    /* Write the value. */
	if (!AHA_output_byte(mask))
	    return (FALSE);

    /* Check for proper end of command. */
	AHA_check_end_command();

    return (TRUE);
}

#endif /* NEEDED */

/*XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX*/

INT AHA_init(num, mailboxP, isrP)
    UBYTE num;
    MAILBOX_ENTRY *mailboxP;
    PFV isrP;

/* -----------------------------------------------------------------------

    Purpose:
	Initialize the SCSI board.

    Inputs:
	num -- number of mailbox out or in entries
	mailboxP -- address of the mailbox
	target -- initialize the board as a target if TRUE
	isrP -- pointer to the interrupt routine for the board

    Outputs:
	The return value is the SCSI id of the board.  A value of -1 indicates
	an error.

    Machine dependencies:

    Notes:

    Procedure:

/CODE
----------------------------------------------------------------------- */

{   /* AHA_init */
    extern int tran_speed;
    INT dma, intr, scsi_id;

    /* Reset the board. */
	if (!AHA_hard_reset())
	    return (-1);

    /* Select a moderate bus speed. */
#ifdef OLD_WAY
	if (!AHA_set_tran_speed(0xFF))
	    return (-1);
#else
	if (!AHA_set_tran_speed((UBYTE)tran_speed))
	    return (-1);
#endif

    /* Initialize the mailbox. */
	if (!AHA_init_mailbox(num, mailboxP))
	    return (-1);

    /* Enable logical unit TARGET_LUN for use as a target. */
	if (!AHA_en_target_mode((UBYTE) (1 << TARGET_LUN)))
	    return (-1);

    /* Get the configuration data. */
	if (!AHA_ret_conf_data(&dma, &intr, &scsi_id))
	    return (-1);

    /* Initialize the DMA channel. */
	switch (dma)
	{
	    case 0:
		(void) outp(0x0B, 0x0C);
		(void) outp(0x0A, 0x00);
		break;

	    case 5:
		(void) outp(0xD6, 0xC1);
		(void) outp(0xD4, 0x01);
		break;

	    case 6:
		(void) outp(0xD6, 0xC2);
		(void) outp(0xD4, 0x02);
		break;

	    case 7:
		(void) outp(0xD6, 0xC3);
		(void) outp(0xD4, 0x03);
		break;
	}

    /* Clear any previous interrupts. */
	AHA_reset_interrupt();

    /* Install the interrupt handler. */
	ATM_set_isp(intr + 0x68, isrP);

    /* Enable the interrupt. */
	(void) outp(0xA1, (INT) (inp(0xA1) & ~((UBYTE) 1 << (intr - 8))));

    /* Return the SCSI id of the device. */
	return (scsi_id);
}

/* ------------------------  end of file  ----------------------------- */

