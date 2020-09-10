/* ---------- Print routines for the AHA-1540B SCSI Controller ---------

This file contains diagnostic print routines for the AHA-1540B SCSI Controller.

Author:  Donald Reaves

----------------------------------------------------------------------- */

#ifdef DEBUG

#include "aha1540b.h"
#include <string.h>

extern INT printf();

/*XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX*/

VOID AHA_print_inquire(iobase)
    INT iobase;

/* -----------------------------------------------------------------------

    Purpose:
	Print information from the Adapter Inquiry command.

    Inputs:
	iobase -- the base I/O address of the board

    Outputs:

    Machine dependencies:

    Notes:

    Procedure:

/CODE
----------------------------------------------------------------------- */

{   /* AHA_print_inquire */
    static CHAR board_id[40], rev[40];
    INT options;

    /* Inquire the board options. */
	if (!AHA_inquire(iobase, board_id, &options, rev))
	    return;

    /* Print the board ID. */
	(VOID) printf("Board ID %s, ", board_id);

    /* Decode and print the options byte. */
	switch (options)
	{
	    case 0x30:  (VOID) printf("standard model, ");  break;
	    default:    (VOID) printf("unknown options, "); break;
	}

    /* Print the firmware revision. */
	(VOID) printf("Rev %s\r\n", rev);
}

/*XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX*/

VOID AHA_print_config(iobase)
    INT iobase;

/* -----------------------------------------------------------------------

    Purpose:
	Print information from the Return Configuration Data command.

    Inputs:
	iobase -- the base I/O address of the board

    Outputs:

    Machine dependencies:

    Notes:

    Procedure:

/CODE
----------------------------------------------------------------------- */

{   /* AHA_print_config */
    INT dma, intr, scsi_id;

    /* Get the configuration data. */
	if (!AHA_ret_conf_data(iobase, &dma, &intr, &scsi_id))
	    return;

    /* Print the data. */
	(VOID) printf("DMA %d, Interrupt %d, SCSI id %d\r\n",dma,intr,scsi_id);
}

/*XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX*/

VOID AHA_print_setup(iobase)
    INT iobase;

/* -----------------------------------------------------------------------

    Purpose:
	Print information from the Return Setup Data command.

    Inputs:
	iobase -- the base I/O address of the board

    Outputs:

    Machine dependencies:

    Notes:

    Procedure:

/CODE
----------------------------------------------------------------------- */

{   /* AHA_print_setup */
    INT i, offset, period;
    INT sdt, par, speed, on, off, n_mb;
    ULONG mb_addr;
    static INT sper[8], soff[8], disc[8];

    /* Get the setup data. */
	if (!AHA_ret_setup_data(&sdt, &par, &speed, &on, &off, &n_mb,
			   &mb_addr, sper, soff, disc))
	    return;

    /* Print the information. */
	(VOID) printf("Sync Data Transfer %sabled, ", sdt ? "En" : "Dis");
	(VOID) printf("Parity Checking %sabled\r\n", par ? "En" : "Dis");
	(VOID) printf("Transfer speed = 0x%02x, ", speed);
	(VOID) printf("Bus On/Off Time = %d/%d microseconds\r\n", on, off);
	(VOID) printf("Mailbox:  %d slots at 0x%08lx\r\n", n_mb, mb_addr);
	(VOID) printf("Sync Negotiation:");
	for (i = 0; i < 8; ++i)
	{
	    if ((i & 3) == 0)
		(VOID) printf("\r\n");
	    period = sper[i];
	    offset = soff[i];
	    if (offset == 0)
		(VOID) printf(" %d:asynch", i);
	    else if (offset != -1)
		(VOID) printf(" %d:per=%d off=%d", i, period, offset);
	    else
		(VOID) printf(" %d:none", i);
	}
	(VOID) printf("\r\nDisconnect: ");
	for (i = 0; i < 8; ++i)
	    (VOID) printf(" %d:%c", i, disc[i] ? 'Y' : 'N');
	(VOID) printf("\r\n");
}

/*XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX*/

VOID AHA_print_board_info(iobase)
    INT iobase;

/* -----------------------------------------------------------------------

    Purpose:
	Call the above print routines for each board.

    Inputs:
	iobase -- the base I/O address of the board

    Outputs:

    Machine dependencies:

    Notes:

    Procedure:

/CODE
----------------------------------------------------------------------- */

{   /* AHA_print_board_info */

    (VOID) printf("Board I/O base address 0x%x\r\n", iobase);
    AHA_print_inquire(iobase);
    AHA_print_config(iobase);
    AHA_print_setup(iobase);
}

/*XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX*/

VOID AHA_print_mailbox(mP, out)
    MAILBOX_ENTRY *mP;
    BOOL out;

/* -----------------------------------------------------------------------

    Purpose:
	Print the information contained in a mailbox.

    Inputs:
	mP -- pointer to the mailbox
	out -- TRUE for MBO, FALSE for MBI

    Outputs:

    Machine dependencies:
	Both SCSI target and SCSI initiator

    Notes:

    Procedure:

/CODE
----------------------------------------------------------------------- */

{   /* AHA_print_mailbox */

    (VOID) printf("flag: ");
    if (out)
    {
	switch(mP->flag)
	{
	    case MBO_FREE:     (VOID) printf("MBO_FREE,     "); break;
	    case MBO_START:    (VOID) printf("MBO_START,    "); break;
	    case MBO_ABORT:    (VOID) printf("MBO_ABORT,    "); break;
	    default:           (VOID) printf("%d, ", mP->flag); break;
	}
    }
    else
    {
	switch(mP->flag)
	{
	    case MBI_FREE:     (VOID) printf("MBI_FREE,     "); break;
	    case MBI_COMPLETE: (VOID) printf("MBI_COMPLETE, "); break;
	    case MBI_ABORTED:  (VOID) printf("MBI_ABORTED,  "); break;
	    case MBI_NOTFOUND: (VOID) printf("MBI_NOTFOUND, "); break;
	    case MBI_ERROR:    (VOID) printf("MBI_ERROR,    "); break;
	    case MBI_CCB_REQ:  (VOID) printf("MBI_CCB_REQ,  "); break;
	    default:           (VOID) printf("%d, ", mP->flag); break;
	}
    }
    if (mP->flag == MBI_CCB_REQ)
    {
	(VOID) printf("id=%d, ", (mP->addr_2 >> 5) & 7);
	(VOID) printf("RECV=%d, ", (mP->addr_2 >> 4) & 1);
	(VOID) printf("SEND=%d, ", (mP->addr_2 >> 3) & 1);
	(VOID) printf("lun=%d, ", mP->addr_2 & 7);
	(VOID) printf("len: 0x%02x%02xXX\r\n", mP->addr_1, mP->addr_0);
    }
    else
	(VOID) printf("addr: 0x%06lx\r\n", AHA_get_3_data(&mP->addr_2));
}

/*XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX*/

VOID AHA_print_CDB(dP)
    CDB *dP;

/* -----------------------------------------------------------------------

    Purpose:
	Print the information contained in a CDB.

    Inputs:
	dP -- pointer to the CDB

    Outputs:

    Machine dependencies:
	Both SCSI target and SCSI initiator

    Notes:

    Procedure:

/CODE
----------------------------------------------------------------------- */

{   /* AHA_print_CDB */

    (VOID) printf("opcode=");
    switch (dP->opcode)
    {
	case OP_TEST_RDY:    (VOID) printf("TEST_RDY");       break;
	case OP_RECV:        (VOID) printf("RECV");           break;
	case OP_SEND:        (VOID) printf("SEND");           break;
	case OP_INQUIRY:     (VOID) printf("INQUIRY");        break;
	default:             (VOID) printf("%x", dP->opcode); break;
    }
    (VOID) printf("  lun=%d", dP->lun);
    (VOID) printf("  len=%ld", AHA_get_3_data(&dP->len_2));
    (VOID) printf("  flag=%d", (dP->flag_link >> 1) & 1);
    (VOID) printf("  link=%d\r\n", dP->flag_link & 1);
}

/*XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX*/

VOID AHA_print_SDB(sP)
    SDB *sP;

/* -----------------------------------------------------------------------

    Purpose:
	Print the information contained in a SDB.

    Inputs:
	sP -- pointer to the SDB

    Outputs:

    Machine dependencies:
	Both SCSI target and SCSI initiator

    Notes:

    Procedure:

/CODE
----------------------------------------------------------------------- */

{   /* AHA_print_SDB */

    (VOID) printf("  error:     ");
    switch (sP->error)
    {
	case HAS_RESIDUE: (VOID) printf("HAS_RESIDUE         "); break;
	case NO_RESIDUE:  (VOID) printf("NO_RESIDUE          "); break;
	default:          (VOID) printf("%-20x", sP->error);     break;
    }
    (VOID) printf("res1:       %d\r\n", sP->res1);

    (VOID) printf("  ili:       %-20d", (sP->ili_key >> 4) & 1);
    (VOID) printf("info1-4:    %d %d %d %d\r\n",
		sP->info1, sP->info2, sP->info3, sP->info4);

    (VOID) printf("  sense_len: %-20d", sP->sense_len);
    (VOID) printf("res2-5:     %d %d %d %d\r\n",
		sP->res2, sP->res3, sP->res4, sP->res5);

    (VOID) printf("  key+code:  ");
    switch (SENSE_CODE(sP))
    {
	case NO_SENSE_DATA:   (VOID) printf("NO_SENSE_DATA       ");  break;
	case INV_CMD_OPCODE:  (VOID) printf("INV_CMD_OPCODE      ");  break;
	case INV_LUN:         (VOID) printf("INV_LUN             ");  break;
	case INV_CMD_PARM:    (VOID) printf("INV_CMD_PARM        ");  break;
	case RESET_ATTN:      (VOID) printf("RESET_ATTN          ");  break;
	case PARITY_ERROR:    (VOID) printf("PARITY_ERROR        ");  break;
	case INITIATOR_ERROR: (VOID) printf("INITIATOR_ERROR     ");  break;
	case DUMB_INITIATOR:  (VOID) printf("DUMB_INITIATOR      ");  break;
	default:              (VOID) printf("%-20x", SENSE_CODE(sP)); break;
    }
    (VOID) printf("sense_qual: %d\r\n", sP->sense_qual);
}

/*XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX*/

VOID AHA_print_IDB(iP)
    IDB *iP;

/* -----------------------------------------------------------------------

    Purpose:
	Print the information contained in an IDB.

    Inputs:
	iP -- pointer to the IDB

    Outputs:

    Machine dependencies:
	Both SCSI target and SCSI initiator

    Notes:

    Procedure:

/CODE
----------------------------------------------------------------------- */

{   /* AHA_print_IDB */
    static CHAR buf[32];
 
    (VOID) printf("  qualifier:   %-20d", (iP->qual_type >> 5) & 7);
    (VOID) printf("device type:  %d\r\n", iP->qual_type & 15);

    (VOID) printf("  res1:        %-20d", iP->res1);
    (VOID) printf("ANSI version: %d\r\n", iP->ansi_ver & 7);

    (VOID) printf("  data format: %-20d", iP->rdf);
    (VOID) printf("inquiry len:  %d\r\n", iP->inquiry_len);

    (VOID) printf("  res2-3:      %d %-18d", iP->res2, iP->res3);
    (VOID) printf("sync:         %d\r\n", (iP->sync_link >> 4) & 1);

    (VOID) printf("  link:        %-20d", (iP->sync_link >> 3) & 1);
    (VOID) strncpy(buf, iP->vendor_id, sizeof (iP->vendor_id));
    buf[sizeof (iP->vendor_id)] = '\0';
    (VOID) printf("Vendor ID:    %s\r\n", buf);

    (VOID) strncpy(buf, iP->product_id, sizeof (iP->product_id));
    buf[sizeof (iP->product_id)] = '\0';
    (VOID) printf("  Product ID:  %-20s", buf);
    (VOID) strncpy(buf, iP->product_rev, sizeof (iP->product_rev));
    buf[sizeof (iP->product_rev)] = '\0';
    (VOID) printf("Product Rev:  %s\r\n", buf);
}

/*XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX*/

VOID AHA_print_CCB(cP)
    CCB *cP;

/* -----------------------------------------------------------------------

    Purpose:
	Print the information contained in a CCB.

    Inputs:
	cP -- pointer to the CCB

    Outputs:

    Machine dependencies:
	Both SCSI target and SCSI initiator

    Notes:

    Procedure:

/CODE
----------------------------------------------------------------------- */

{   /* AHA_print_CCB */

    (VOID) printf("opcode:    ");
    switch (cP->opcode)
    {
	case OP_ICCB:        (VOID) printf("ICCB                "); break;
	case OP_TCCB:        (VOID) printf("TCCB                "); break;
	case OP_ICCB_SG:     (VOID) printf("ICCB_SG             "); break;
	case OP_ICCB_LEN:    (VOID) printf("ICCB_LEN            "); break;
	case OP_ICCB_SG_LEN: (VOID) printf("ICCB_SG_LEN         "); break;
	case OP_RESET:       (VOID) printf("RESET               "); break;
	default:             (VOID) printf("%-20x", cP->opcode);    break;
    }

    (VOID) printf("id/odt/idt/lun: ");
    (VOID) printf("%d", (cP->addr_dir >> 5) & 7);
    (VOID) printf("/%d", (cP->addr_dir >> 4) & 1);
    (VOID) printf("/%d", (cP->addr_dir >> 3) & 1);
    (VOID) printf("/%d\r\n", cP->addr_dir & 7);

    (VOID) printf("cmd_len:   %-20d", cP->cmd_len);
    (VOID) printf("sense_len:      %d\r\n", cP->sense_len);

    (VOID) printf("data_len:  %-20ld", AHA_get_3_data(&cP->data_len_2));
    (VOID) printf("data_addr:      0x%06lx\r\n",
		   AHA_get_3_data(&cP->data_addr_2));

    (VOID) printf("link_addr: %-20ld", AHA_get_3_data(&cP->link_addr_2));
    (VOID) printf("link_id:        %d\r\n", cP->link_id);

    (VOID) printf("hastat:    ");
    switch (cP->hastat)
    {
	case HS_NO_ERROR:       (VOID) printf("NO_ERROR            "); break;
	case HS_SEL_TIMEOUT:    (VOID) printf("SEL_TIMEOUT         "); break;
	case HS_OVER_RUN:       (VOID) printf("OVER_RUN            "); break;
	case HS_UNEXP_FREE:     (VOID) printf("UNEXP_FREE          "); break;
	case HS_PHASE_ERROR:    (VOID) printf("PHASE_ERROR         "); break;
	case HS_INV_OPCODE:     (VOID) printf("INV_OPCODE          "); break;
	case HS_LINK_LUN_ERROR: (VOID) printf("LINK_LUN_ERROR      "); break;
	case HS_INV_TARG_DIR:   (VOID) printf("INV_TARG_DIR        "); break;
	case HS_DUP_CCB:        (VOID) printf("DUP_CCB             "); break;
	case HS_INV_CCB_PARM:   (VOID) printf("INV_CCB_PARM        "); break;
	case HS_PENDING:        (VOID) printf("PENDING             "); break;
	default:                (VOID) printf("%-20x", cP->hastat);    break;
    }

    (VOID) printf("tarstat:        ");
    switch (cP->tarstat)
    {
	case TS_OK:       (VOID) printf("OK\r\n");              break;
	case TS_CHECK:    (VOID) printf("CHECK\r\n");           break;
	case TS_LUN_BUSY: (VOID) printf("LUN_BUSY\r\n");        break;
	default:          (VOID) printf("%x\r\n", cP->tarstat); break;
    }

    (VOID) printf("res1-2:    %d %d\r\n", cP->res1, cP->res2);
    (VOID) printf("cdb:       ");
    AHA_print_CDB((CDB *) cP->cdb);
    if (cP->tarstat == TS_CHECK)
    {
	(VOID) printf("sdb:\r\n");
	AHA_print_SDB((SDB *) cP->sdb);
    }
}

#endif /* DEBUG */

/* ------------------------  end of file  ----------------------------- */

