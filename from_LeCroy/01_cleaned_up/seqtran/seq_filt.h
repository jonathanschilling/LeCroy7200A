/*---------------------    seq_filt.h    --------------------------------

This file defines a basic structure that contains all the variables
needed to perform an FIR filter operation.

Author: Christopher Eck		LeCroy Corporation
				700 Chestnut Ridge Road
				Chestnut Ridge, NY  10977

--------------------------------------------------------------------------*/

#ifndef FILTER_H
#define FILTER_H
         
#include "aagen.h"

#define MAX_91_COEFFS  64 /* Although we have a 63-point filter, 64 samples */
			  /* are always acquired, 32 from Ch1, 32 from Ch2 */

typedef struct FILTER
{
    WORD  last_flash;	  /* different filters for different flashes       */
    WORD  num_filters;	  /* the number of ADC flashes used to acquire data*/
    WORD  num_coeffs;	  /* number of coefficients per filter             */
    WORD  coeffP[8][13];  /* The actual coefficients: 16384 = 1            */
    BOOL  p91_mode;	  /* TRUE if 7291 attached; FALSE otherwise        */
    WORD  num_91coeffs;	  /* number of coefficients for 7291 filter        */
    WORD  coeff_7291[64]; /* The coefficients for 7291 filter if needed    */

} FILTER;
#endif
/*------------------------- end of file ----------------------------------*/

