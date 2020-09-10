/*------------------- General Definitions for ATOM -------------------------

NOTE: SAME AS AATOM.H EXCEPT DOES NOT INCLUDE ERR_EXT.H AND ALL OF ITS INCLUDE
      FILES.

Author: Larry Salant

$Header:   F:/asg/code/seqtran/aatom.h_v   1.0   20 Nov 1993 09:56:26   CHRIS_E  $

Cumulative check-in comments:
$Log:   F:/asg/code/seqtran/aatom.h_v  $
 * 
 *    Rev 1.0   20 Nov 1993 09:56:26   CHRIS_E
 * Initial revision.
 * 
 *    Rev 1.0   12 May 1992 08:10:38   CHRIS_E
 * Initial revision.
   
      Rev 1.23   16 Apr 1991 17:13:40   LARRY_S
   added frame id definitions
   
      Rev 1.22   26 Nov 1990 18:11:24   BILL_M
   added ATM_MAX_NUM_TMS constant
   
      Rev 1.21   16 Aug 1990 17:15:36   LARRY_S
   changed ATM_MAX_PLUGIN to 2 (no more 7201)
   
      Rev 1.20   19 Jan 1990 17:26:08   PETER_S
   corrected FOREVER macro
   
      Rev 1.19   18 Jan 1990 17:29:16   PETER_S
   added FOREVER macro
   
      Rev 1.18   13 Jul 1989 18:26:10   GABE_G
   Added definition of ATM_MAX_MEMORIES
   
      Rev 1.17   15 Jun 1989 17:50:50   DON_R
   Removed the definition of ATM_PROBE.  Added LCAST and TOUCH macros.
   
      Rev 1.16   20 Feb 1989 16:34:22   DON_R
   Move status type definitions to om_ext.h.
   
      Rev 1.15   02 Dec 1988 16:48:06   LARRY
   moved definition of COL_WFREF from coldef.h
   
      Rev 1.14   28 Nov 1988 15:04:42   DBR
   Moved definition of UNIT from untdef.h.
   
      Rev 1.13   31 Oct 1988 13:03:30   CHRIS
   added typedefs for structs ATM_STATUS, ATM_STATUS_MASK and ATM_STATUS_REG
   
      Rev 1.12   14 Oct 1988 14:33:48   LARRY
   added ATM_PROBE debug switch
   
      Rev 1.11   04 May 1988 16:10:36   BILL
   added max number of trigger sources
   
      Rev 1.10   15 Apr 1988 10:38:48   LARRY
   add ATM_MAX_SEGMENTS_SEQ and MAX_POINTS_PAGE
   
      Rev 1.9   12 Apr 1988 15:04:02   GABE
   Added ATM_MAX_CHANELS_PER_PLUGIN

   Rev 1.8   24 Mar 1988 14:48:08   BILL
   Added more device names

   Rev 1.7   24 Mar 1988  8:59:06   BILL
*   added define for all plugins

   Rev 1.6   23 Mar 1988 15:20:28   BILL
*       Added plugin defines
   
      Rev 1.5   15 Mar 1988  9:28:54   LARRY
   change errors.h to err_ext.h
   
      Rev 1.4   18 Feb 1988  9:33:40   LARRY
   add NOT_PRESENT
   
      Rev 1.3   03 Feb 1988 16:41:28   PETER_S
   added dummy stop watch macros for shareware
   
      Rev 1.2   16 Jan 1988 13:50:40   LARRY
   add ATM_MAX definitions
   
      Rev 1.1   31 Dec 1987 14:20:32   PETER_S
   fix include of errors.h
   
      Rev 1.0   30 Oct 1987  7:49:12   LARRY
   Initial revision.

--------------------------------------------------------------------------*/

#ifndef AATOM_ID
         
      SCID( AATOM_id, "@(#)aatom.h:1.1")    /* for shareware compatibility */

/* dummy Geneva stop watch macros for shareware pieces */
#define DB_WATCH_START
#define DB_WATCH_STOP

/* dummy Geneva debug print macros for shareware pieces */
#define DPRD(s, d)     
#define DPRE(s, f)     
#define DPRF(s, f)   
#define DPRS(s)        
#define DPRU(s, u)     
#define DPRX(s, x)
    
/*XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX

Each frame is compiled with a -DFR7XXX switch.  This information is used
to generate a value for FRAME_ID, which can be tested more easily.

The order and values are not important, they must be unique
--------------------------------------------------------------------------*/

#define F7200 	0
#define F7200A	1
#define F7100 	2

#ifdef FR7200
#define FRAME_ID F7200
#define ATM_MAX_PLUGINS  2		/* maximum number of plugins */
#endif

#ifdef FR7200A
#define FRAME_ID F7200A
#define ATM_MAX_PLUGINS  2		/* maximum number of plugins */
#endif

#ifdef FR7100
#define FRAME_ID F7100
#define ATM_MAX_PLUGINS  1		/* maximum number of plugins */
#endif

/*XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX

General System Definitions
--------------------------------------------------------------------------*/
#define ATM_MAX_TRACES   8		/* maximum number of traces */
#define ATM_MAX_MEMORIES 8		/* maximum number of memories */
#define ATM_MAX_DEVICES  (ATM_MAX_PLUGINS + 1) /* maximum number of devices */
                                               /* Num Plugins + Main Frame  */
#define ATM_MAX_CHANNELS_PER_PLUGIN	 4

#define ATM_MAX_TRGSRCS         6      /* Max number of trigger sources */
#define ATM_MAX_SEGMENTS_SEQ	250
#define ATM_MAX_POINTS_PAGE	50000	/* nominal max. number of points/page  */
#define ATM_MAX_NUM_TMS         2   /* maximum number of TMS(s) per plugin */

/* Defines to be used when referencing devices   */

#define ATM_MAIN_FRAME   0      /* Main Frame                   */
#define ATM_PLUGIN_A     1      /* Plugin A                     */
#define ATM_PLUGIN_B     2      /* Plugin B                     */
#define ATM_DISPLAY      7      /* Display - used by test mgr   */
#define ATM_PCON         8      /* Pcon  - used by test mgr     */
#define ATM_PROC         9      /* Processor - used by test mgr */
#define ATM_PLUGIN_ALL   10     /* Specifies all plugins        */

#define NOT_PRESENT -1

/*------------------------------------------------------------------------*/

/* Typedef for a unit, so everyone doesn't have to include untdef.h */

typedef short	UNIT;		/* Handle for a unit		*/

/* CONST and VOLATILE must me undefined for the 7200A and 7100 */
/* They are previously defined in aagen.h                      */
#if (FRAME_ID == F7200A) || (FRAME_ID == F7100)
#undef CONST   
#undef VOLATILE
#define CONST   
#define VOLATILE
#endif /* FRAME_ID */

/*------------------------------------------------------------------------*/
/* Typedef for a reference to a waveform descriptor, so everyone doesn't
   have to include coldef.h */

typedef WORD COL_WFREF;		/* Handle for a wavedesc		*/

/*------------------------------------------------------------------------*/

/* Macros to help keep lint quiet:

    LCAST is used to cast a value to a given type, without "Loss of precision".
    Unfortunately, it can't be used as an argument to a function, because this
    causes another lint error -- "Boolean argument to function".

    TOUCH is used to keep lint from complaining that a variable is not used. */

#ifndef LCAST

#ifdef _lint
#define	LCAST(type, value)	((type) !(value))
#else /* _lint */
#define	LCAST(type, value)	((type) (value))
#endif /* _lint */

#endif /* LCAST */

#ifndef TOUCH

#ifdef _lint
#define	TOUCH(x)	if (x) x = 0;
#else /* _lint */
#define	TOUCH(x)
#endif /* _lint */

#endif /* TOUCH */

/* forever (while(1)) loop that everything (lint 2.14,2.15,3.0 & CCC) likes */
#ifndef FOREVER
#define FOREVER         for (;;)
#endif  /* FOREVER */

/*------------------------------------------------------------------------*/

#endif     /* AATOM_ID */
/*------------------------- end of file ----------------------------------*/

