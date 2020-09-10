/* ----------- Definitions for the AHA-1540B SCSI Controller ----------

This file contains definitions for the AHA-1540B SCSI Controller.

Author:  Donald Reaves

----------------------------------------------------------------------- */

#ifndef AHA1540B_H
#define	AHA1540B_H

/* -------------------------------------------------------------------- */

/* Basic type definitions. */


typedef unsigned char  UBYTE;    /* 8 bits unsigned                     */
typedef unsigned long  ULONG;    /* 32 bits unsigned                    */
typedef char  BYTE;              /* 8 bits signed                       */
typedef char  CHAR;              /* signed character unit               */
typedef short WORD;              /* 16 bits signed                      */
typedef long  LONG;              /* 32 bits signed                      */
typedef int   INT;               /* natural size of machine             */
typedef CHAR  BOOL;              /* TRUE/FALSE type                     */
#define VOID  void               /* no type                             */
typedef VOID   (*PFV)();         /* pointer to function returning void  */

#ifndef TRUE
#define TRUE            1
#define FALSE           0 
#endif /* TRUE */

#ifndef NULL
#define NULL            0 
#endif /* NULL */

extern int kbhit();
extern void error_exit();

#define PAUSE						\
    if (kbhit())					\
	error_exit("Exiting...\n", (char *) NULL);

/* -------------------------------------------------------------------- */

/* Constants defining the interface to the AHA-1540B. */

#define	IOBASE	0x330		/* I/O base of board			*/

/* I/O locations on the board: */

#define AHA_CTRL (IOBASE+0)	/* Control port (output)		*/
#define AHA_STAT (IOBASE+0)	/* Status port (input)			*/
#define AHA_DATA (IOBASE+1)	/* Data port (input/output)		*/
#define AHA_IFLG (IOBASE+2)	/* Interrupt flags port (input)		*/

/* Bits in the control register.  Writing a one to any of the bits defined
   below activates the corresponding function.  There is no need to reset
   the bit; this is done automatically by the hardware. */

#define AHA_HRST  0x80		/* Hard Reset				*/
#define AHA_SRST  0x40		/* Soft Reset				*/
#define AHA_IRST  0x20		/* Interrupt Reset			*/
#define AHA_SCRST 0x10		/* SCSI Bus Reset			*/

/* Bits in the status register. */

#define AHA_STST  0x80		/* Self Test in Progress		*/
#define AHA_DIAGF 0x40		/* Internal Diagnostic Failure		*/
#define AHA_INIT  0x20		/* Mailbox Initialization Required	*/
#define AHA_IDLE  0x10		/* SCSI Host Adapter Idle		*/
#define AHA_CDF   0x08		/* Command/Data Out Port Full		*/
#define AHA_DF    0x04		/* Data In Port Full			*/
#define AHA_INVC  0x01		/* Invalid Command			*/

/* Bits in the interrupt register. */

#define AHA_ANY	  0x80		/* Any Interrupt			*/
#define AHA_SCRD  0x08		/* SCSI Reset Detected			*/
#define AHA_HACC  0x04		/* Host Adapter Command Complete	*/
#define AHA_MBOA  0x02		/* Mailbox Out Empty			*/
#define AHA_MBIF  0x01		/* Mailbox In Full			*/

/* The data output port should only be written to if AHA_STAT.AHA_CDF
   is zero.  The data input port should only be read when AHA_STAT.AHA_DF
   is one.  New commands should not be started (except for START_SCSI_CMD
   and EN_MBOX_OUT_INT) unless AHA_STAT.AHA_IDLE is one. */

/* -------------------------------------------------------------------- */

/* Host Adapter Command definitions. */

#define NO_OP		  0x00	/* No Operation				*/
#define MBOX_INIT	  0x01	/* Mailbox Initialization		*/
#define START_SCSI_CMD	  0x02	/* Start SCSI Command			*/
#define START_BIOS_CMD	  0x03	/* Start PC/AT BIOS Command		*/
#define ADPT_INQUIRY	  0x04	/* Adapter Inquiry			*/
#define EN_MBOX_OUT_INT	  0x05	/* Enable Mailbox Out Available Int.	*/
#define SET_SEL_TIMEOUT	  0x06	/* Set Selection Time Out		*/
#define SET_BUS_ON_TIME	  0x07	/* Set Bus-On Time			*/
#define SET_BUS_OFF_TIME  0x08	/* Set Bus-Off Time			*/
#define SET_TRAN_SPEED	  0x09	/* Set Transfer Speed			*/
#define RET_CONF_DATA	  0x0B	/* Return Configuration Data		*/
#define EN_TARGET_MODE	  0x0C	/* Enable Target Mode			*/
#define RET_SETUP_DATA	  0x0D	/* Return Setup Data			*/
#define ECHO		  0x1F	/* Echo Command Data (Diagnostic)	*/
#define ADPT_DIAG	  0x20	/* Perform Adapter Diagnostics		*/
#define SET_ADPT_OPTIONS  0x21	/* Set Host Adapter Options		*/

/* -------------------------------------------------------------------- */

/* Mailbox definitions. */

typedef struct MAILBOX_ENTRY	/* an entry in a mailbox		*/
{
    UBYTE flag;			/* MBO command or MBI status value	*/
    UBYTE addr_2;		/* high order address of CCB		*/
    UBYTE addr_1;		/* middle order address of CCB		*/
    UBYTE addr_0;		/* low order address of CCB		*/
} MAILBOX_ENTRY;

/* Definitions of the flag field in a mailbox out entry. */

#define MBO_FREE	0x00	/* mailbox entry is free		*/
#define MBO_START	0x01	/* SCSI command is to be started	*/
#define MBO_ABORT	0x02	/* SCSI command is to be aborted	*/

/* Definitions of the flag field in a mailbox in entry. */

#define MBI_FREE	0x00	/* mailbox entry is free		*/
#define MBI_COMPLETE	0x01	/* CCB completed without error		*/
#define MBI_ABORTED	0x02	/* CCB aborted by host			*/
#define MBI_NOTFOUND	0x03	/* aborted CCB not found		*/
#define MBI_ERROR	0x04	/* CCB completed with error		*/
#define MBI_CCB_REQ	0x10	/* CCB required				*/

/* -------------------------------------------------------------------- */

/* SCSI Command and Sense Data Block definitions.

   Note that several fields use multiple bytes to store addresses and lengths.
   However, these are stored high byte first, and have only 3 bytes.  In order
   to ensure proper alignment, all of these fields are declared as individual
   bytes.  In all cases, the low order byte is named XX_0, the middle byte is
   named XX_1, and the high byte is named XX_2. */

typedef struct CDB		/* SCSI Command Data Block		*/
{
    UBYTE opcode;		/* command data block operation code	*/
    UBYTE lun;			/* logical unit number			*/
    UBYTE len_2;		/* length (high)			*/
    UBYTE len_1;		/* length (middle)			*/
    UBYTE len_0;		/* length (low)				*/
    UBYTE flag_link;		/* flag and link			*/
} CDB;

#define	CDB_SIZE	6	/* size of CDB, excluding any padding	*/

/* Definitions for the opcode field. */

#define OP_TEST_RDY	0x00	/* Test unit ready			*/
#define OP_RECV		0x08	/* Transfer data: target to initiator	*/
#define OP_SEND		0x0A	/* Transfer data: initiator to target	*/
#define OP_INQUIRY	0x12	/* Inquire				*/

typedef struct SDB		/* SCSI Sense Data Block		*/
{
    UBYTE error;		/* Error code				*/
    UBYTE res1;			/* Reserved				*/
    UBYTE ili_key;		/* Incorrect len indicator + sense key	*/
    UBYTE info1;		/* Information bytes (residue)		*/
    UBYTE info2;		/* Information bytes (residue)		*/
    UBYTE info3;		/* Information bytes (residue)		*/
    UBYTE info4;		/* Information bytes (residue)		*/
    UBYTE sense_len;		/* Additional sense length		*/
    UBYTE res2;			/* Reserved				*/
    UBYTE res3;			/* Reserved				*/
    UBYTE res4;			/* Reserved				*/
    UBYTE res5;			/* Reserved				*/
    UBYTE sense_code;		/* Additional sense code		*/
    UBYTE sense_qual;		/* Additional sense qualifier		*/
} SDB;

#define	SDB_SIZE	14	/* size of SDB, excluding any padding	*/

/* Definitions for the error field. */

#define HAS_RESIDUE	0xF0	/* Residue field is valid		*/
#define NO_RESIDUE	0x70	/* Residue field is to be ignored	*/

/* Definitions for the ili_key field. */

#define ILI(x)	(((x) & 0x20) != 0)	/* Non-zero if ILI is true	*/
#define KEY(x)	((x) & 15)		/* Value of key field		*/

/* Given a pointer to an SDB, the following macro returns an error code. */

#define	SENSE_CODE(sdbP) ((KEY((sdbP)->ili_key) << 8) | (sdbP)->sense_code)

/* Values of SENSE_CODE. */

#define NO_SENSE_DATA	0x000	/* No sense data are available		*/
#define INV_CMD_OPCODE	0x020	/* Invalid command operation code	*/
#define INV_LUN		0x525	/* Invalid logical unit number		*/
#define INV_CMD_PARM	0x526	/* Invalid command parameter		*/
#define RESET_ATTN	0x629	/* Reset (or power up) attention	*/
#define PARITY_ERROR	0xB47	/* Interface parity error		*/
#define INITIATOR_ERROR	0xB48	/* Initiator detected error		*/
#define DUMB_INITIATOR	0x52B	/* Dumb initiator			*/

typedef struct IDB		/* SCSI Inquiry Data Block		*/
{
    UBYTE qual_type;		/* Peripheral Qualifier/Processor Type	*/
    UBYTE res1;			/* Reserved				*/
    UBYTE ansi_ver;		/* ANSI version				*/
    UBYTE rdf;			/* Response Data Format			*/
    UBYTE inquiry_len;		/* Additional inquiry length		*/
    UBYTE res2;			/* Reserved				*/
    UBYTE res3;			/* Reserved				*/
    UBYTE sync_link;		/* SYNC and LINK fields			*/
    CHAR vendor_id[8];		/* Vendor Identification (ASCII)	*/
    CHAR product_id[16];	/* Product Identification (ASCII)	*/
    CHAR product_rev[4];	/* Product Revision Level (ASCII)	*/
} IDB;

#define	IDB_SIZE	36	/* size of IDB, excluding any padding	*/

/* -------------------------------------------------------------------- */

/* Command Control Block Definitions.

   Note that several fields use multiple bytes to store addresses and lengths.
   However, these are stored high byte first, and have only 3 bytes.  In order
   to ensure proper alignment, all of these fields are declared as individual
   bytes.  In all cases, the low order byte is named XX_0, the middle byte is
   named XX_1, and the high byte is named XX_2. */

typedef struct CCB		/* Host Adapter Command Control Block	*/
{
    UBYTE opcode;		/* command control block operation code	*/
    UBYTE addr_dir;		/* address and direction control	*/
    UBYTE cmd_len;		/* SCSI command length			*/
    UBYTE sense_len;		/* Request Sense Length / Disable Sense	*/
    UBYTE data_len_2;		/* Data length (high)			*/
    UBYTE data_len_1;		/* Data length (middle)			*/
    UBYTE data_len_0;		/* Data length (low)			*/
    UBYTE data_addr_2;		/* Data Pointer (high)			*/
    UBYTE data_addr_1;		/* Data Pointer (middle)		*/
    UBYTE data_addr_0;		/* Data Pointer (low)			*/
    UBYTE link_addr_2;		/* Link Pointer (high)			*/
    UBYTE link_addr_1;		/* Link Pointer (middle)		*/
    UBYTE link_addr_0;		/* Link Pointer (low)			*/
    UBYTE link_id;		/* Command linking identifier		*/
    UBYTE hastat;		/* Host adapter status			*/
    UBYTE tarstat;		/* Target device status			*/
    UBYTE res1;			/* reserved (should be zero)		*/
    UBYTE res2;			/* reserved (should be zero)		*/
    UBYTE cdb[CDB_SIZE];	/* SCSI command data block		*/
    UBYTE sdb[SDB_SIZE];	/* SCSI sense data block		*/

    /* The remaining entries are used to communicate between the
       interrupt handler and the application program.  They are not
       used by the SCSI controller. */

    BOOL int_received;		/* interrupt has been received		*/
    UBYTE mbi_flag;		/* mailbox in flags			*/
    WORD event;			/* event to signal (if not zero)	*/
    LONG pid;			/* process to signal			*/
} CCB;

/* Definitions for the opcode field. */

#define OP_ICCB		0x00	/* Initiator CCB			*/
#define OP_TCCB		0x01	/* Target CCB				*/
#define OP_ICCB_SG	0x02	/* Initiator CCB with scatter/gather	*/
#define OP_ICCB_LEN	0x03	/* Initiator CCB with returned length	*/
#define OP_ICCB_SG_LEN	0x04	/* Initiator CCB with s/g & len		*/
#define OP_RESET	0x81	/* SCSI Bus Device Reset		*/

/* Definitions for the addr_dir field.

   id  -- the SCSI bus identifier of the device (0..7)
   odt -- outbound data transfer, length is checked (0..1)
   idt -- inbound data transfer, length is checked (0..1)
   lun -- logical unit number of the device (0..7) */

#define ADDR_DIR(id,odt,idt,lun)	\
    ((UBYTE) (((id) << 5) | ((odt) << 4) | ((idt) << 3) | (lun)))

/* Definitions for the hastat field. */

#define HS_NO_ERROR	  0x00	/* No errors detected			*/
#define HS_SEL_TIMEOUT	  0x11	/* Selection timeout			*/
#define HS_OVER_RUN	  0x12	/* Data over run or under run		*/
#define HS_UNEXP_FREE	  0x13	/* Unexpected bus free			*/
#define HS_PHASE_ERROR	  0x14	/* Target bus phase sequence failure	*/
#define HS_INV_OPCODE	  0x16	/* Invalid CCB operation code		*/
#define HS_LINK_LUN_ERROR 0x17	/* Linked CCB does not have the same LUN*/
#define HS_INV_TARG_DIR	  0x18	/* Invalid target dir rec'd from host	*/
#define HS_DUP_CCB	  0x19	/* Duplicate CCB received in target mode*/
#define HS_INV_CCB_PARM	  0x1A	/* Invalid CCB or segment list parameter*/
#define HS_PENDING	  0xFF	/* Completion status hasn't been written*/

/* Definitions for the tarstat field. */

#define TS_OK		  0x00	/* Good status				*/
#define TS_CHECK	  0x02	/* Check status (consult sense data)	*/
#define TS_LUN_BUSY	  0x08	/* logical unit is busy			*/

/* -------------------------------------------------------------------- */

/* Definition of the logical unit number used by the target. */

#define TARGET_LUN  7

/* -------------------------------------------------------------------- */

/* Definitions of the externally accessible functions in aha1540b.c. */

extern BOOL AHA_hard_reset();
extern BOOL AHA_inquire();
extern BOOL AHA_ret_conf_data();
extern BOOL AHA_ret_setup_data();
extern BOOL AHA_start_scsi_command();
extern BOOL SCSI_abort();
extern BOOL SCSI_inquire();
extern BOOL SCSI_start();
extern BOOL SCSI_test_ready();
extern INT AHA_init();
extern INT SCSI_setup();
extern ULONG AHA_get_3_data();
extern VOID *AHA_get_3_addr();
extern VOID AHA_put_3_addr();
extern VOID AHA_put_3_data();
extern VOID AHA_reset_interrupt();
extern VOID SCSI_init_CCB();
extern VOID SCSI_isr();
extern VOID SCSI_reset();

/* -------------------------------------------------------------------- */

#endif /* AHA1540B_H */

/* ------------------------  end of file  ----------------------------- */

