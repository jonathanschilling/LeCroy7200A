/* ---------------- High-level SCSI Interface Routines ----------------

This file contains high-level SCSI routines for the 7200A.

Author:  Donald Reaves

----------------------------------------------------------------------- */

#include "aha1540b.h"
#include "conio.h"

extern VOID _SCSI_isr();

/* -------------------------------------------------------------------- */

/* Local data. */

#define MAX_TRIES	1000		 /* max tries to get a mailbox	*/
#define NUM_MBO		4		 /* # of output mailboxes	*/

MAILBOX_ENTRY SCSI_mailbox[NUM_MBO * 2]; /* SCSI board mailboxes	*/
MAILBOX_ENTRY *cur_mboP;		 /* current MBO pointer		*/
static BOOL scsi_present = FALSE;	 /* SCSI board present flag	*/

/* -------------------------------------------------------------------- */

/* Diagnostic definitions. */

#ifdef DEBUG
extern INT printf();
#define PRINTF(x)	{ (VOID) printf x; }
#else /* DEBUG */
#define PRINTF(x)
#endif /* DEBUG */

/*XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX*/

VOID SCSI_init_CCB(cP, id, lun, ccb_op, cdb_op, addr, len, event)
    register CCB *cP;
    INT id, lun;
    UBYTE ccb_op, cdb_op;
    VOID *addr;
    ULONG len;
    WORD event;

/* -----------------------------------------------------------------------

    Purpose:
	Fill in a CCB for a SCSI command.

    Inputs:
	cP -- pointer to the CCB to be used for the command.
	id -- the SCSI id of the initiator
	lun -- the logical unit number for the transfer
	ccb_op -- the opcode for the CCB
	cdb_op -- the opcode for the CDB
	addr -- the address of the buffer into which the data are to be stored
	len -- the length of the buffer
	event -- events to signal to calling process on interrupt (if non-zero)

    Outputs:

    Machine dependencies:

    Notes:

    Procedure:

/CODE
----------------------------------------------------------------------- */

{   /* SCSI_init_CCB */
    register CDB *dP;
    INT obt, ibt;

    /* Determine values for the obt and ibt fields from the opcodes. */
	if (ccb_op == OP_TCCB)
	{   /* Target command. */

	    if (cdb_op == OP_RECV)
	    {
		obt = 1;
		ibt = 0;
	    }
	    else /* cdb_op == OP_SEND */
	    {
		obt = 0;
		ibt = 1;
	    }
	}
	else
	{   /* Initiator command. */

	    switch (cdb_op)
	    {
		case OP_TEST_RDY:
		   obt = 0;
		   ibt = 0;
		   break;

		case OP_SEND:
		   obt = 1;
		   ibt = 0;
		   break;

		case OP_RECV:
		case OP_INQUIRY:
		default:
		   obt = 0;
		   ibt = 1;
		   break;
	    }
	}

    /* Fill in the fields in the CCB. */
	cP->opcode = ccb_op;
	cP->addr_dir = ADDR_DIR((UBYTE)id, (UBYTE)obt, (UBYTE)ibt, (UBYTE)lun);
	cP->cmd_len = CDB_SIZE;
	cP->sense_len = SDB_SIZE;
	AHA_put_3_data(&cP->data_len_2, len);
	AHA_put_3_addr(&cP->data_addr_2, addr);
	cP->link_addr_2 = 0;
	cP->link_addr_1 = 0;
	cP->link_addr_0 = 0;
	cP->link_id = 0;
	cP->hastat = HS_PENDING;
	cP->tarstat = 0;
	cP->res1 = 0;
	cP->res2 = 0;
	cP->int_received = FALSE;
	cP->mbi_flag = MBI_FREE;
	cP->event = event;
	cP->pid = 0;

    /* Fill in the fields of the Command Data Block. */
	dP = (CDB *) cP->cdb;
	dP->opcode = cdb_op;
	dP->lun = 0;
	AHA_put_3_data(&dP->len_2, len);
	dP->flag_link = 0;
}

/*XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX*/

static BOOL SCSI_send_CCB(cP, cmd)
    register CCB *cP;
    UBYTE cmd;

/* -----------------------------------------------------------------------

    Purpose:
	Send a CCB to the SCSI board.

    Inputs:
	cP -- pointer to a CCB
	cmd -- MBO_START to start a command or MBO_ABORT to abort one
	mailboxP -- pointer to the mailbox for the board

    Outputs:
	The return value is TRUE on success, FALSE on failure.

    Machine dependencies:

    Notes:

    Procedure:

/CODE
----------------------------------------------------------------------- */

{   /* SCSI_send_CCB */
    register INT i;
    register MAILBOX_ENTRY *mP = SCSI_mailbox + NUM_MBO;
    INT tries = 0;

    /* Return failure if the board is not present. */
	if (!scsi_present)
	    return (FALSE);

    /* Locate a free mailbox entry. */
	for (;;)
	{
	    /* Try to find an empty entry. */
		for (i = 0; i < NUM_MBO; ++i)
		{
		    if (++cur_mboP == mP)
			cur_mboP = SCSI_mailbox;
		    if (cur_mboP->flag == MBO_FREE)
			break;
		}

	    /* Break out of the loop if we found one. */
		if (i < NUM_MBO)
		{
		    mP = cur_mboP;
		    break;
		}

	    /* Exit with an error message if we have waited too long. */
		if (++tries >= MAX_TRIES)
		{
		    PRINTF(("SCSI_send_CCB: timeout\n"))
		    return (FALSE);
		}

	    /* Wait a while. */
		PAUSE
	}

    /* Store the address of the CCB in the mailbox. */
	AHA_put_3_addr(&mP->addr_2, (VOID *) cP);

    /* Store the command in the mailbox. */
	mP->flag = cmd;

    /* Indicate that the command hasn't been completed. */
	cP->hastat = HS_PENDING;

    /* Clear the interrupt return fields. */
	cP->int_received = FALSE;
	cP->mbi_flag = MBI_FREE;

    /* Signal the SCSI board to look in the mailbox. */
	if (!AHA_start_scsi_command())
	    return (FALSE);

    return (TRUE);
}

/*XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX*/

static BOOL SCSI_wait_command(cP)
    CCB *cP;

/* -----------------------------------------------------------------------

    Purpose:
	Wait for a SCSI command to complete.

    Inputs:
	cP -- pointer to a CCB

    Outputs:
	The return value is TRUE on success, FALSE on failure.

    Machine dependencies:

    Notes:
	The hastat field of a CCB is initialized to HS_PENDING before a
	SCSI command is passed to the host adapter.  When the command
	terminates, either normally or with error, this field is set to
	another value.

    Procedure:

/CODE
----------------------------------------------------------------------- */

{   /* SCSI_wait_command */
    ULONG timeout = 10000;

    /* Return failure if the board is not present. */
	if (!scsi_present)
	    return (FALSE);

    do
    {
	if (--timeout == 0)
	{
	    PRINTF(("SCSI_wait_command timeout\n"))
	    return (FALSE);
	}
	PAUSE

    } while (cP->hastat == HS_PENDING);

    return (TRUE);
}

/*XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX*/

BOOL SCSI_test_ready(id, lun)
    INT id, lun;

/* -----------------------------------------------------------------------

    Purpose:
	Test a device to see if it is ready to be used as a target.

    Inputs:
	id -- the SCSI id of the board to be tested
	lun -- logical unit to be tested

    Outputs:
	The return value is TRUE if the unit is ready, and FALSE if not.

    Machine dependencies:

    Notes:

    Procedure:

/CODE
----------------------------------------------------------------------- */

{   /* SCSI_test_ready */
    static CCB ccb;
    SDB *sP;
    enum { unknown, is_not_target, is_target } state;
    INT tries = 0;

    /* Return failure if the board is not present. */
	if (!scsi_present)
	    return (FALSE);

    /* Get the address of the SDB in the CCB. */
	sP = (SDB *) ccb.sdb;

    /* Set up the CCB for the transfer. */
	SCSI_init_CCB(&ccb,id,lun,OP_ICCB,OP_TEST_RDY,(VOID*)0,(ULONG)0,0);

    do
    {
	/* Send the CCB to the board. */
	    if (!SCSI_send_CCB(&ccb, MBO_START))
		return (FALSE);

	/* Wait for the command to complete. */
	    if (!SCSI_wait_command(&ccb))
		return (FALSE);

	/* Determine the state of the target. */
	    if (ccb.hastat == HS_SEL_TIMEOUT)
		state = is_not_target;
	    else if (ccb.tarstat == TS_OK)
		state = is_target;
	    else if (ccb.tarstat == TS_CHECK)
	    {
		switch (SENSE_CODE(sP))
		{
		    case INV_LUN:
		    case DUMB_INITIATOR:
			state = is_not_target;
			break;

		    default:
			state = unknown;
			break;
		}
	    }
	    else
		state = unknown;
	
	/* Wait a while. */
	    PAUSE

    } while (state == unknown && ++tries <= 4);

    return (state == is_target);
}

/*XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX*/

INT SCSI_setup()

/* -----------------------------------------------------------------------

    Purpose:
	Set up the SCSI board.

    Inputs:

    Outputs:
	The return value is the SCSI id of the board.

    Machine dependencies:

    Notes:

    Procedure:

/CODE
----------------------------------------------------------------------- */

{   /* SCSI_setup */
    INT scsi_id;

    /* Perform the necessary hardware initializations. */
	scsi_id = AHA_init(NUM_MBO, SCSI_mailbox, _SCSI_isr);

    /* Initialize the current mailbox out pointer. */
	cur_mboP = SCSI_mailbox - 1;

    /* Set the board present flag. */
	scsi_present = (scsi_id >= 0);

    /* Return the SCSI id of the device. */
	return (scsi_id);
}

/*XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX*/

BOOL SCSI_start(cP)
    CCB *cP;

/* -----------------------------------------------------------------------

    Purpose:
	Start up a SCSI transfer.

    Inputs:
	cP -- pointer to the CCB for the command

    Outputs:
	The return value is TRUE on success, FALSE on failure.

    Machine dependencies:

    Notes:

    Procedure:

/CODE
----------------------------------------------------------------------- */

{   /* SCSI_start */

    /* Return failure if the board is not present. */
	if (!scsi_present)
	    return (FALSE);

    /* Send the CCB to the board. */
	return (SCSI_send_CCB(cP, MBO_START));
}

/*XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX*/

BOOL SCSI_abort(cP)
    CCB *cP;

/* -----------------------------------------------------------------------

    Purpose:
	Abort a SCSI transfer.

    Inputs:
	cP -- pointer to the CCB for the command

    Outputs:
	The return value is TRUE on success, FALSE on failure.

    Machine dependencies:

    Notes:

    Procedure:

/CODE
----------------------------------------------------------------------- */

{   /* SCSI_abort */

    /* Return failure if the board is not present. */
	if (!scsi_present)
	    return (FALSE);

    /* Send the CCB to the board. */
	return (SCSI_send_CCB(cP, MBO_ABORT));
}

/*XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX*/

VOID SCSI_isr()

/* -----------------------------------------------------------------------

    Purpose:
	Service SCSI target interrupts.

    Inputs:

    Outputs:

    Machine dependencies:

    Notes:

    Procedure:

/CODE
----------------------------------------------------------------------- */

{   /* SCSI_isr */
    MAILBOX_ENTRY *mP = SCSI_mailbox + NUM_MBO;
    UBYTE flags, i;
    CCB *cP;

    /* Get the interrupt flags. */
	flags = inp(AHA_IFLG);

    /* Return if no interupts are asserted. */
	if ((flags & AHA_ANY) == 0)
	    return;

    /* Reset the interrupt. */
	AHA_reset_interrupt();

    if (flags & AHA_MBIF)
    {   /* An incoming mailbox message was received. */

	for (i = 0; i < NUM_MBO; ++i, ++mP)
	{
	    /* Process the mailbox entry. */
		switch (mP->flag)
		{
		    case MBI_COMPLETE: /* CCB completed without error */
		    case MBI_ABORTED:  /* CCB aborted by host         */
		    case MBI_NOTFOUND: /* aborted CCB not found       */
		    case MBI_ERROR:    /* CCB completed with error    */

			/* Get the address of the corresponding CCB. */
			    cP = (CCB *) AHA_get_3_addr(&mP->addr_2);

			/* Store the flag in the CCB. */
			    cP->mbi_flag = mP->flag;
			    cP->int_received = TRUE;

			/* Indicate that the mailbox entry is now free. */
			    mP->flag = MBI_FREE;
			break;

		    case MBI_CCB_REQ:  /* CCB required */
			/* Indicate that the mailbox entry is now free. */
			    mP->flag = MBI_FREE;
			break;

		    default:           /* mailbox entry is free */
			break;
		}
	}
    }
}

/* ------------------------  end of file  ----------------------------- */

