/*********************** seq_filt.c *************************************

  This program accepts a set of filter coefficients, the data points
  acquired with that set of coefficients, and the last flash and then
  applies the FIR filter to the data and outputs the filtered results.

Author: Christopher Eck		LeCroy Corporation
				700 Chestnut Ridge Road
				Chestnut Ridge, NY  10977

 **********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include "seq_filt.h"
#include "seq_tran.h"
#include "aagen.h"

#define CA_OVERFLOW  ((long) 2080768)        /*  0x7f00 <<  6 */
#define CA_UNDERFLOW ((long)-2097152)        /* -0x8000 <<  6 */

#define POS_SATURATION ((long) 2097088)      /*  0x7fff <<  6 */

#define CA1_OVERFLOW  ((long) 532676608)     /*  0x7f00 << 14 */
#define CA1_UNDERFLOW ((long)-536870912)     /* -0x8000 << 14 */

#define OVERFLOW    0x7f00
#define UNDERFLOW   0x8000
#define SATURATION  0x7fff

#define DECIMAL "%d"
#define HEX     "%x"

/***************************************************************
 The relationship between last_flash # and flash/channel is:
	
	 last flash  flash/channel
	 ----------  -------------
	     0 .......... A1
	     1 .......... B1
	     2 .......... C1
	     3 .......... D1
	     4 .......... A2
	     5 .......... B2
	     6 .......... C2
	     7 .......... D2

  The order that the data is acquired in is as follows:
    .... D2 D1 C2 C1 B2 B1 A2 A1 D2 D1 C2 C1 B2 B1 A2 A1 D2 ....

 ***************************************************************/

static BYTE index_8flash[8][8] =   /* 2GSa/sec */
{
    4,0,7,3,6,2,5,1,	/* last_flash = 0   A2 A1 D2 D1....B2 B1 A2 A1 */
    5,1,4,0,7,3,6,2,	/* last_flash = 1   B2 B1 A2 A1....C2 C1 B2 B1 */ 
    6,2,5,1,4,0,7,3,	/* last_flash = 2   C2 C1 B2 B1....D2 D1 C2 C1 */
    7,3,6,2,5,1,4,0,	/* last_flash = 3   D2 D1 C2 C1....A2 A1 D2 D1 */

    1,4,0,7,3,6,2,5,	/* last_flash = 4   B1 A2 A1 D2....C1 B2 B1 A2 */
    2,5,1,4,0,7,3,6,	/* last_flash = 5   C1 B2 B1 A2....D1 C2 C1 B2 */
    3,6,2,5,1,4,0,7,	/* last_flash = 6   D1 C2 C1 B2....A1 D2 D1 C2 */
    0,7,3,6,2,5,1,4	/* last_flash = 7   A1 D2 D1 C2....B1 A2 A1 D2 */
};

static BYTE index_4flash[4][4] =   /* 1GSa/sec */
{
    0,3,2,1,		/* last_flash = 0   A D C B .... D C B A */
    1,0,3,2,		/* last_flash = 1   B A D C .... A D C B */
    2,1,0,3,		/* last_flash = 2   C B A D .... B A D C */
    3,2,1,0		/* last_flash = 3   D C B A .... C B A D */
};

static BYTE index_2flash[3][4] =   /* 400 MSa/sec */
{
    0,1,0,1,		/* last_flash = 0   A C A C .... C A C A */
    0,0,0,0,		/* last_flash = 1   B  ==> IMPOSSIBLE */
    1,0,1,0		/* last_flash = 2   C A C A .... A C A C */
};

static BYTE index_1flash[1][4] =   /* <= 200 MSa/sec */
{
    0,0,0,0		/* last_flash = 0   A A A A .... A A A A */
};

extern CHAR *read_data_points();
extern VOID SEQ_fir();
extern VOID SEQ_fir_7291();

/*XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX*/

VOID SEQ_fir (filt_dataP, new_block)
SEQ_FILTER_DATA *filt_dataP;
BOOL new_block;

/*--------------------------------------------------------------------------

    Purpose: To filter the raw data contained in input_fP with the
		coefficients in coeff based on the last_flash and
		the number of filters and coefficients per filter.

    Inputs: new_block = TRUE if called to work on a new segment
		      = FALSE if called to work on a different part of a
				previous segment.

	    filt_dataP = pointer to structure of all pertinent filter info

	    The filter coefficients(h[n]) are arranged in a 2 dimensional 
		array in the following order based on sampling rate:

	    2 GSa/sec:
		coeff[0][0..12] = h[0],h[1],...,h[12] for flash A Channel 1
		coeff[1][0..12] = h[0],h[1],...,h[12] for flash B Channel 1
		coeff[2][0..12] = h[0],h[1],...,h[12] for flash C Channel 1
		coeff[3][0..12] = h[0],h[1],...,h[12] for flash D Channel 1
		coeff[4][0..12] = h[0],h[1],...,h[12] for flash A Channel 2
		coeff[5][0..12] = h[0],h[1],...,h[12] for flash B Channel 2
		coeff[6][0..12] = h[0],h[1],...,h[12] for flash C Channel 2
		coeff[7][0..12] = h[0],h[1],...,h[12] for flash D Channel 2

	    1 GSa/sec:
		coeff[0][0..12] = h[0],h[1],...,h[12] for flash A
		coeff[1][0..12] = h[0],h[1],...,h[12] for flash B
		coeff[2][0..12] = h[0],h[1],...,h[12] for flash C
		coeff[3][0..12] = h[0],h[1],...,h[12] for flash D

	    400 MSa/sec:
		coeff[0][0..12] = h[0],h[1],...,h[12] for flash A
		coeff[1][0..12] = h[0],h[1],...,h[12] for flash C

	    <= 200 MSa/sec:
		coeff[0][0..12] = h[0],h[1],...,h[12] for flash A


	    The data is arranged in memory in the following order based on
	    sampling rate (ie., A2(4)=flash A, Channel 1 (flash code = 4)  
				C(2) = flash C (flash code = 2)):

	    2 GSa/sec:
		... D2(7) D1(3) C2(6) C1(2) B2(5) B1(1) A2(4) A1(0) D2(7) ...

	    1 GSa/sec:
		... D(3)  C(2)  B(1)  A(0)  D(3)  C(2)  B(1)  A(0)  D(3)  ...

	    400 MSa/sec:
		... C(2)  A(0)  C(2)  A(0)  C(2)  A(0)  C(2)  A(0)  C(2)  ...

	    <= 200 MSa/sec:
		... A(0)  A(0)  A(0)  A(0)  A(0)  A(0)  A(0)  A(0)  A(0)  ...

    Outputs:

    Machine dependencies:

    Notes:
	The filters were derived based on the flash ADC used to acquire
	the last sample of filter_length samples and must be applied to
	the raw data in the proper order based on last flash. The order
	of the flashes remains constant (see above sequences). Therefore,
	we must know which flash is the last flash in the filter_length
	samples in order to properly apply the filters. This information
	is provided in filterP->last_flash. For example, if the raw data 
	buffer were in the following flash order (1GSa/sec):
	  x[0]=C, x[1]=B, x[2]=A, x[3]=D, ... ,x[12]=C, x[13]=B, x[14]=A

	Then, coeff[2][0..12] must be applied first to the first 13
	samples ending in flash C (x[0]..x[12]), then coeff[1][0..12] 
	must be applied next to the second 13 samples ending in flash B
	(x[1]..x[13]), then coeff[0][0..12] must be applied next to the
	third 13 samples ending in flash A (x[2]..x[14]), and so on.

	This situation is futher complicated for 2GSa/s for two reasons:
	  1. Channel 1 data is interleaved with Channel 2 data so the
	     filters must be applied separately to each channel's data,
	     skipping the other channel's data while filtering.
	  2. After both channels have been filtered separately and
	     interleaved together, a final filter is applied to the
	     entire interleaved record (filterP->coeff_7291[63]).
	     This last filter is not flash dependent, however.


    Procedure:
	Since different rates require indexing into the coeff[8][13]
	array differently, a second array will be used that is unique
	to each sampling rate and its contents will determine the
	index into the coeff[8][13] array. For example, the 1GSa/sec
	index array would be arranged as follows:
	    index[last_flash][flash] = {
		0, 3, 2, 1   // last_flash = 0, A
		1, 0, 3, 2   // last_flash = 1, B
		2, 1, 0, 3   // last_flash = 2, C
		     :               :          :

	  so that if the last_flash = 1, we would apply coeff[1][0..12]
	  first, then coeff[0][0..12], coeff[3][0..12], coeff[2][0..12], etc
	  since the flash order is B(1), A(0), D(3), C(2), B(1), A(0), ...  
	  
/CODE
--------------------------------------------------------------------------*/
{   /* SEQ_fir() */

    register BYTE  *raw_data;
    register WORD  *coeffP;
    register LONG  temp;
    register WORD  k;

	     LONG  n;
	     WORD  *corr_dataP;
    	     WORD  i;

	     LONG  count; 
    struct FILTER  *filterP;
	     BYTE  *indexP;
    	     INT   max_coeffs;

      static BYTE  flash;

      static BOOL  direction = 0;

    /* initialize the local copies of important variables */
	raw_data = filt_dataP->rawP;
	filterP = filt_dataP->paramsP;

    /* Initialize the indexP to the correct array based on sampling rate */
	switch (filterP->num_filters)
	{
	    case 1:	/* <= 200 MSa/sec */
		indexP = &index_1flash[filterP->last_flash][0];
		break;

	    case 2:	/* 400 MSa/sec */
		indexP = &index_2flash[filterP->last_flash][0]; 
		break; 
		
	    case 4:	/* 1 GSa/sec */ 
		indexP = &index_4flash[filterP->last_flash][0]; 
		break; 
		
	    default:	/* 2 GSa/sec */ 
		indexP = &index_8flash[filterP->last_flash][0]; 
		break; 
	}


    if (filterP->num_filters < 8)   /* <= 1GSa/sec */
    {
	if (new_block == TRUE)
	    flash = 0;
	count = filt_dataP->size;
	corr_dataP = filt_dataP->corrP;
	for (n=filterP->num_coeffs-1; n < count; ++n,++flash)
	{
	    temp=0;
	    coeffP = &filterP->coeffP[indexP[flash & 3]][0];

	    for (k=0; k < filterP->num_coeffs; ++k)
	    {
		temp += (coeffP[k] * (long)raw_data[n-k]);
	    }

	    /* limit the output based on overflow and underflow */
		if (temp > CA_OVERFLOW)
		    *corr_dataP++ = OVERFLOW;
		else if (temp < CA_UNDERFLOW)
		    *corr_dataP++ = UNDERFLOW;
		else
		    /* round the result like the DSP does it. The DSP does it
		       this way to take advantage of saturation mode arithemetic
		       Note: This is different than just (temp >> 6) by 1 LSB */
		    *corr_dataP++ = (short)((temp >> 7) << 1);

	/* DEBUG STUFF */
#ifdef DEBUG_STUFF
		if (direction == 0)
		{
		    if (*(corr_dataP-1) > 0)
		    {
			direction = 1;
			printf("changed direction to +\n");
		    }
		}
		else
		{
		    if (*(corr_dataP-1) < 0)
		    {
			direction = 0;
			printf("changed direction to -\n");
		    }
		}
#endif /* DEBUG_STUFF */

	}
    }
    else  /* 2GSa/sec, must deal with 2 channels interleaved */
    {
	/* Adjust the new limits for all the loops */
	    max_coeffs = 2*filterP->num_coeffs;
	    count = filt_dataP->size_91;

	/* Each channel's data located at every other point */
	    if (new_block == TRUE)
		flash = 0;
	    corr_dataP = filt_dataP->p91P;
	    for (n=max_coeffs-2; n < count; ++n,++flash)
	    {
		temp=0;
		coeffP = &filterP->coeffP[indexP[flash & 7]][0];

		for (k=0; k < max_coeffs; k += 2)
		{
		    temp += (*coeffP++ * (long)raw_data[n-k]);
		}

		/* Rail the output using DSP saturation mode arithmetic.     */
		/* These limitations are due to the DSP's fixed word length. */
		    if (temp > POS_SATURATION)
			*corr_dataP++ = SATURATION;
		    else if (temp < CA_UNDERFLOW)
			*corr_dataP++ = UNDERFLOW;
		    else
		      /* round the result like the DSP does it. The DSP does it
		         this way to use saturation mode arithemetic. Note that
		         this is different than just (temp >> 6) by 1 LSB */
			*corr_dataP++ = (short)((temp >> 7) << 1);

		/* Print completion status  */
	}

	SEQ_fir_7291(filterP, filt_dataP->size, filt_dataP->p91_baseP,
			filt_dataP->corrP);
    }
}

/*XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX*/

VOID SEQ_fir_7291(filterP, size, data, corr_dataP)
struct FILTER *filterP;
LONG   size;
WORD   *data;
register WORD *corr_dataP;

/*--------------------------------------------------------------------------

    Purpose: To apply either a 500MHz filter or a wide-band filter to the
		interleaved data.

    Inputs: filterP->coeff_7291[63] = coefficients for 63-point FIR filter
	      This array will contain the proper coefficients for either
	      a 500MHz filter or a wide-band filter. This is controlled
	      via the menu selection field "Optimize Pulse Response" or
	      the remote command "P91_FILTER". The options are:
		 YES = use a wideband filter on the interleaved data
		  NO = use a 500MHz filter on the interleaved data

	    filterP = structure containing the filter information
	    size = number of 16-bit words to filter
	    data = array containing flash filtered 16-bit results
	    corr_dataP = ptr to array where to put 7291-corrected results

    Outputs:

    Machine dependencies:

    Notes:

    Procedure:
	apply a standard FIR filter over the data in place.
	  
/CODE
--------------------------------------------------------------------------*/
{   /* SEQ_fir_7291() */

register WORD   *coeffP;
register LONG   temp;
register LONG   n;
register WORD   i,k;

    for (n=filterP->num_91coeffs-1; n < size; ++n)
    {
	temp=0;
	coeffP = &filterP->coeff_7291[0];

	for (k=0; k < filterP->num_91coeffs; ++k)
	{
	    temp += (coeffP[k] * (long)data[n-k]);
	}

	/* limit the output based on overflow and underflow limits */
	    if (temp > CA1_OVERFLOW)
		*corr_dataP++ = OVERFLOW;
	    else if (temp < CA1_UNDERFLOW)
		*corr_dataP++ = UNDERFLOW;
	    else
		/* round the result like the DSP does it. The DSP does it
		   this way to take advantage of saturation mode arithemetic
		   Note: This is different than just (temp >> 14) by 1 LSB */
		*corr_dataP++ = (short)((temp >> 15) << 1);
    }

}

/*XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX*/

VOID SEQ_read_filter_coefficients(bufP, filterP)
WORD *bufP;
struct FILTER *filterP;

/*--------------------------------------------------------------------------

    Purpose: To read the coefficients in bufP into the 2 dimensional array 
		pointed to by filterP->coeffP. The coefficients in bufP
		are in binary, Intel 16-bit WORD format.

    Inputs: bufP = pointer to buffer with filter coefficients
	    filterP->coeffP = 2-dim array to stuff filter coefficients
	    filterP->num_filters = ptr to the variable to put number of ADCs
	    filterP->num_coeffs = number of coefficients for each set of filters

    Outputs: sets filterP->num_filters to # of flashes used to acquire the data
	     sets filterP->num_coeffP to the # of correction coefficients per
		flash.

    Machine dependencies:

    Notes: Assumes Flash A coefficients followed by Flash B, C, D
	    with the coefficients in the following order:
	    h[0] h[1] h[2] h[3] h[4] h[5] h[6] h[7] h[8] h[9] h[10] h[11] h[12]

	   num_filters really determines the sampling rate which defines 
	   num_coefficients as follows:

	       num_filters  sampling rate  num_coefficients/filter
	       -----------  -------------  -----------------------
		   1	     <= 200 MSa/s	 2
		   2		400 MSa/s	 7
		   4		  1 GSa/s	13
		*  9		  2 GSa/s	26 + 63

	    * In 2GSa/s sampling rate, 2 channels are interleaved at 1 GSa/s
	      each, which requires 2 chan * 4 filters/chan * 13 coeff/filter.
	      In addition, a final 63 point filter is applied to the final
	      interleaved record. 

    Format:  num filters, num coefficients, coefficients,...,coefficients 
                          num coefficients, coefficients,...,coefficients 
                          num coefficients, coefficients,...,coefficients 
                          num coefficients, coefficients,...,coefficients 

    Procedure:

/CODE
--------------------------------------------------------------------------*/
{   /* SEQ_read_filter_coefficients() */

    WORD num_filters, num_coefficients;
    INT  filter, coeff;
    WORD n,i;
    WORD channel=0;
    WORD *coeffP;

    num_filters = *bufP++;
    filterP->num_filters = num_filters;

    if (num_filters == 9)
	num_filters = 8;

    for (filter = 0; filter < num_filters; ++filter)
    {
	num_coefficients = *bufP++;
	for (coeff=0; coeff < num_coefficients; ++coeff)
	{
	    filterP->coeffP[filter][coeff] = *bufP++;
	}
    }

    filterP->num_coeffs = num_coefficients;

    if (num_filters == 8)
    {
	/* Read the 7291 interleaved filter */
	num_coefficients = *bufP++;
	filterP->num_91coeffs = num_coefficients;
	for (coeff=0; coeff < num_coefficients; ++coeff)
	{
	    filterP->coeff_7291[coeff] = *bufP++;
	}
	filterP->p91_mode = TRUE;
    }
    else
    {
	/* This is set to 1 because the main program subtracts 1, result=0 */
	    filterP->num_91coeffs = 1;
	    filterP->p91_mode = FALSE;
    }


    if (SEQ_options.print_coeffs)
    {
	for (n=0; n < filterP->num_filters; ++n)
	{
	    if (n == 8)
	    {
		/* 7291 filter: 63 taps */
		printf("\n7291 Filter Coefficients = ");
		coeffP = &filterP->coeff_7291[0];
		for (i=0; i < filterP->num_91coeffs; ++i)
		{
		    if (i % 8 == 0)
			printf("\n");
		    printf("%04x ", coeffP[i]);
		}
	    }
	    else
	    {
		coeffP = &filterP->coeffP[n][0];
		for (i=0; i < filterP->num_coeffs; ++i)
		{
		    printf("%04x ", coeffP[i]);
		}
		printf("<%c%d>", (n & 3)+'A', channel+1+((n&4)>>2));
	    }
	    printf("\n");
	}
    }
}

