/*---------------------    SEQ_TRAN.H    --------------------------------

This file defines all the arrays used to contain the data.

Author: Christopher Eck		LeCroy Corporation
	             		700 Chestnut Ridge Road
				Chestnut Ridge, NY  10977

--------------------------------------------------------------------------*/

#ifndef SEQ_TRAN_H
#define SEQ_TRAN_H

#include "aagen.h"
#include "seq_filt.h"
         
/* System definitions based on existing plugins and available memory */
#define MAX_PLUGINS  	    2
#define MAX_CHANNELS 	    4
#define MAX_SEGS	  100	/* Max uncontinuous segs to translate */
#define MAX_FILTER_SIZE    64	/* Room to copy the extra filter points */

/* Max filter buffer...cannot be less than 63+13=76 */
#define MAX_BUF_SIZE    16384    /* MUST BE DIVISIBLE BY 4 */

/* Definitions of actions to perform from main() */
#define SEQ_INIT_PARAMETERS 	0
#define SEQ_READ_DESCRIPTOR	1
#define SEQ_READ_SEGMENT_NUMBER	2
#define SEQ_READ_SEGMENT_TIME	3

#define SEQ_ALL_SEGS   -1L	/*******  REMOVE THIS IN FINAL VERSION */
#define SEQ_ALL_TIME   -1L

/* Definition of segment identifier types */
#define SEQ_UNUSED		0
#define SEQ_USED		1

#define MAX_SEG_TYPES	        2	/* can specify seg in time or segno */
#define SEQ_SEGNO		0
#define SEQ_TIME		1

/* plugin to process */
#define RIS_PROC_A	0
#define RIS_PROC_B	1
#define RIS_PROC_ALL	2

/* Format of the data to be output */
#define SEQ_FORMAT_RAW		0	/* no corrections, raw data */
#define SEQ_FORMAT_CORRECTED    1	/* correct data with filter coeffs */
#define SEQ_FORMAT_COMPENSATED  2	/* correct and compensate data */

/* Definitions of which block is being processed */
#define SEQ_FIRST_BLOCK		(1 << 0)
#define SEQ_NEXT_BLOCK		(1 << 1)
#define SEQ_LAST_BLOCK		(1 << 2)

#define SEQ_SEGMENT_BLOCK    0xffff	/* Channel tag for normal segment */
#define SEQ_DIAGNOSTIC_BLOCK 0x4444     /* Channel tag for test seg ("DD") */

#define GET_DOUBLE(ts)  					     	\
	(((DOUBLE)(((ts.umw * 65536.0) + ts.ulw) * (65536.0*65536.0)))	\
	 + ((DOUBLE)((ts.lmw * 65536.0) + ts.llw)))		  	\

#define EXIT								\
	exit(1);

#define PAUSE1								\
    if (kbhit())							\
    {									\
	(VOID) printf("exit at line %d of %s\n", __LINE__, __FILE__);	\
	fclose(output1_fP);						\
	fclose(output2_fP);						\
	EXIT								\
    }

typedef union IWORD		/* Intel word format */
{
  struct
  {
      BYTE lsb;
      BYTE msb;
  } byt;
  WORD wrd;

} IWORD;

/* 64-bit timestamp definition */
typedef struct TS
{
    UWORD umw;       /* Upper most sig word of timestamp */
    UWORD ulw;       /* Upper least sig word of timestamp */
    UWORD lmw;       /* Lower most sig word of timestamp */
    UWORD llw;       /* Lower least sig word of timestamp */
} TS;

typedef struct TIME {		/* hours : minutes : seconds */
    LONG  hours;
    WORD  minutes;
    FLOAT seconds;
} TIME;

typedef struct SEQ_ACQ_PARAMS {

    UWORD  last_flash;
    UWORD  fine_count;
    TS     time_stamp;

} SEQ_ACQ_PARAMS;

#define SEQ_OUTPUT_FILE	    0	/* output data to a file */
#define SEQ_OUTPUT_SCREEN   1	/* output data to the screen */

#define SEQ_SCREEN_OUTPUT_1 0	/* output screen data only in 1 column */
#define SEQ_SCREEN_OUTPUT_2 1	/* output screen time,data in 2 columns */

typedef struct DEST {
    BYTE type;			/* identifies FILE or SCREEN output */
    BYTE format;		/* format of screen output: 1 or 2 columns */
} DEST;

typedef struct SEGS {
    union
    {
	struct {
	    LONG start;		/* Starting segment range */
	    LONG end;		/* Ending segment range */
	} n;

	struct {
	    DOUBLE start;	/* starting time in seconds */
	    DOUBLE end;		/* ending time in seconds */
	} t;
    } select;
} SEGS;

typedef struct SEQ_OPTIONS {

    BOOL debug;			/* Print progress through code */
    BOOL print_params[MAX_PLUGINS][MAX_CHANNELS]; /* Print acq parameters */
    BOOL test_mode;		/* Compare last segment */
    BOOL print_times;		/* Print time of each segment */
    BYTE format;		/* format of output data: RAW,COR,COM */
    CHAR prt_fmt[8];		/* %d, %g, or %04x output format */
    DEST output;		/* output format: file, 1 col, 2 cols */
#ifdef RIS
    CHAR process_plugin;	/* plugin to process */
    CHAR process_chan;		/* channel to process */
    BOOL packed;		/* indicates if data is packed */
#else /* RIS */
    BOOL print_coeffs;		/* Print filter coefficients */
    BOOL all_segs[MAX_PLUGINS][MAX_CHANNELS];
    SEGS seg[MAX_PLUGINS][MAX_CHANNELS][MAX_SEG_TYPES][MAX_SEGS];
#endif /* RIS */

} SEQ_OPTIONS;

typedef struct SEQ_PARAMS {

    BYTE first_plugin;			/* First plugin present */
    BYTE last_plugin;			/* Last plugin present */
    BYTE last_channel[MAX_PLUGINS];	/* Last channel present per plugin */
    LONG block_size;			/* Block size of each packet */
    LONG seg_offset;			/* Absolute offset to start of segs */
    BOOL last_packet[MAX_PLUGINS];	/* Flag set if processed last packet */
#ifdef RIS
    UWORD *waveP[MAX_PLUGINS][MAX_CHANNELS];
    UWORD *binP[MAX_PLUGINS];
    INT  bins_filled;
#else /* RIS */
    WORD tmb_block_size[MAX_PLUGINS];   /* TMB block size for seq times */
#endif /* RIS */

} SEQ_PARAMS;

typedef struct SEQ_ACQ_DATA {

    BYTE *bufP;			/* ptr to new samples to work on */
    BYTE *baseP;		/* ptr to base memory allocated */
    LONG size;			/* amt of data to read */
    LONG bytes_read;		/* amt actually read from file */
    LONG block_offset;		/* offset within block: 0..block_size */
    LONG byte_offset;		/* offset to skip to before reading bytes */
    WORD plugin;
    WORD channel;
    LONG difference;		/* NEEDED??? */
    LONG array_size;		/* NEEDED??? */
    WORD packet;		/* NEEDED??? */

} SEQ_ACQ_DATA;

typedef struct SEQ_FILTER_DATA {

    BYTE   *rawP;		/* ptr to raw BYTES to correct */
    WORD   *p91P;		/* ptr to 7291 filter buffer */
    WORD   *p91_baseP;		/* start of memory for 7291 filter buffer */
    WORD   *corrP;		/* ptr to corrected WORDs */
    FILTER *paramsP;
    LONG   num_bytes;
    LONG   size;		/* amt of data in corrP */
    LONG   size_91;		/* amt of data in p91P */
    LONG   array_size;		/* corrected array size */

} SEQ_FILTER_DATA;

typedef struct WAVE_PARAMS {

    DOUBLE seg_start_time;	/* start of this seg relative to first */
    FLOAT  time_per_point;	/* Horizontal interval */
    FLOAT  vertical_gain;	/* Overall Vertical Gain (fixed + variable) */
    FLOAT  vertical_offset;	/* Vertical Offset control setting */
    DOUBLE horizontal_offset;	/* Horizontal Offset field of descriptor */

} WAVE_PARAMS;

extern SEQ_OPTIONS SEQ_options;
extern SEQ_PARAMS  SEQ_params;

#endif /* SEQ_TRAN_H */
/*------------------------- end of file ----------------------------------*/

