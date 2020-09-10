/*------------------------- SEQ_WFD.C --------------------------------------*/
/*
Author: Gabriel Goodman		LeCroy Corporation
				ATS Division
				700 Chestnut Ridge Road
				Chestnut Ridge, NY 10977
*/
/*---------------------------------------------------------------------------
    This file was derived from the pcw files for wavetran.
  ---------------------------------------------------------------------------*/
#define  PCW_MAIN_C

#include <stdio.h>
#include <malloc.h>
#include <string.h>
#include "seq_hdr.h"

extern struct PCW_TEMPLATE *PCW_templateP;

/*XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX*/

CHAR *PCW_translate_descriptor(waveformP, blkP)
PCW_WAVEFORM	    waveformP;
struct  PCW_BLOCK   **blkP;

/*--------------------------------------------------------------------------
    Purpose: To translate the waveform descriptor based on the template file.

    Inputs: waveformP = the descriptor information
	    blkP = uninitialized pointer to waveform descriptor block

    Outputs: returns the pointer to the waveform descriptor data
	     sets *blkP to the pointer to the descriptor block

    Machine dependencies:

    Notes:

    Procedure:

/CODE
--------------------------------------------------------------------------*/
{   /* PCW_translate_descriptor() */

    CHAR                *template_file;
    FILE                *fP,*out_fP;
    WORD		*comm_orderP;
    WORD		*last_flashP;
    struct PCW_BLOCK    *blockP;

    static BOOL		translated = FALSE;


    if (!translated)
    {
	template_file = "7200.TPL";

	if ((fP = fopen(template_file,"r")) == NULL)
	{
	    printf("\nCannot open template file: 7200.tpl\n");
	    printf("Must have 7200.tpl in current directory\n");
	    exit(1);
	}

	PCW_templateP = PCW_Get_Template(fP);   /* Generate machine template */
	fclose(fP);

	translated = TRUE;
    }

    if (*waveformP != 'W')
    {
        printf("\nInvalid Waveform Header\n");
        exit(1);
    }


    blockP = PCW_Find_Block("WAVEDESC",PCW_Regular,PCW_templateP);
    PCW_Load_Block_Offsets(PCW_templateP,waveformP);

    /* Return the block pointer and waveform descriptor pointer */
	*blkP = blockP;
	return(waveformP);
}

/*----------------------------- end of file -------------------------------*/

