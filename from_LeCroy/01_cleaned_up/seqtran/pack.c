#include "aagen.h"
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define BIT_WIDTH	10
#define RAW_BLK_SIZE	24
#define PACK_BLK_SIZE	15
static FILE *infile, *outfile;  /* input & output files */

/*XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX*/

ULONG pack10_bits(UCHAR *outP, ULONG offset, UWORD value)
/*--------------------------------------------------------------------------

    Purpose: pack 10 bit data into output buffer

    Inputs: outP   - pointer to output buffer
	    offset - current bit offset into buffer
	    value  - 16 bit word containing 10bits of data in msb's

    Outputs: offset - new  bit offset into buffer

--------------------------------------------------------------------------*/
{   /* pack10_bits() */
    UINT shift;
    UWORD *dataP;

    dataP = (UWORD *)(outP + (offset >> 3));

    /* mask out old data. if array is zero'ed between records, this is
       not necessary */
    shift = offset & 7;
    *dataP &= ~(0x3ff << shift);

    /* put new data into array. The plugin guarentees the lsb's will be 0 ...
       no need to mask off the bits.  Note that the following works since
       'shift' is ALWAYS even.  Therefore at most it can be is 6 */
    *dataP |= (value >> (6 - shift));

    return(offset+BIT_WIDTH);
}

/*XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX*/

ULONG unpack10_bits(UCHAR *inP, ULONG offset, UWORD *valueP)
/*--------------------------------------------------------------------------

    Purpose: unpacks 10 bit data from input buffer

    Inputs: inP    - pointer to output buffer
	    offset - current bit offset into buffer
	    valueP - pointer to 16 bit word containing 10bits of data in msb's

    Outputs: offset - new bit offset into buffer

--------------------------------------------------------------------------*/
{   /* unpack10_bits() */
    UINT shift;

    *valueP = *(UWORD *)(inP + (offset >> 3));

    /* extract 10 bit data from word */
    *valueP <<= (6 - (offset & 7));
    *valueP &= 0xffc0;

    return(offset+BIT_WIDTH);
}

/*XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX*/

VOID pack(VOID)
/*--------------------------------------------------------------------------

    Purpose: packs data from input file into output file

    Inputs: 

    Outputs:

--------------------------------------------------------------------------*/
{   /* pack() */
    INT count, index;
    LONG offset;
    UWORD inbuf[RAW_BLK_SIZE];
    UWORD outbuf[PACK_BLK_SIZE];

    /* read in a block of data and pack it */
    while ((count = fread(&inbuf, sizeof(UWORD), RAW_BLK_SIZE, infile)) != 0)
    {
	offset = 0;
	for (index = 0; index < count; ++index)
	    offset = pack10_bits((UCHAR*)&outbuf, offset, inbuf[index]);

	count = (offset+7) >> 3;  /* rounded # of packed bytes */
	fwrite(&outbuf, 1, count, outfile);
    }
}

/*XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX*/

VOID unpack(VOID)
/*--------------------------------------------------------------------------

    Purpose: unpacks data from input file into output file

    Inputs: 

    Outputs:

--------------------------------------------------------------------------*/
{   /* unpack() */
    INT count, index;
    LONG offset;
    UWORD inbuf[PACK_BLK_SIZE];
    UWORD outbuf[RAW_BLK_SIZE];

    /* read in a block of data and pack it */
    while ((count = fread(&inbuf, sizeof(UWORD), PACK_BLK_SIZE, infile)) != 0)
    {
	offset = 0;
	count = ((count * 16) + 9)/10;
	for (index = 0; index < count; ++index)
	    offset = unpack10_bits((UCHAR*)&inbuf, offset, &outbuf[index]);

	fwrite(&outbuf, sizeof(UWORD), count, outfile);
    }
}

/*XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX*/

VOID main(INT argc, CHAR *argv[])
/*--------------------------------------------------------------------------

    Purpose: Displays sign-on message, processes any switch arguments and 
		calls the pack/unpack routine

    Inputs: argc = number of arguments (switches)
	    argv = the switches as ASCII strings

    Outputs:

--------------------------------------------------------------------------*/
{   /* main() */
    CHAR  *s;
    BOOL error = FALSE;

    if (argc != 4)
    {
	error = TRUE;
    }
    else
    {
	if ((s = argv[2], (infile = fopen(s, "rb")) == NULL))
	{
	    fprintf(stderr, "%s: can't open for reading %s\n", argv[0], s);
	    error = TRUE;
	}
	if ((s = argv[3], (outfile = fopen(s, "wb")) == NULL))
	{
	    fprintf(stderr, "%s: can't open for writing %s\n", argv[0], s);
	    error = TRUE;
	}
    }

    if (error)
    {
	fprintf(stderr,
	    "[p|u] file1 file2' packs or unpacks file1 into file2.\n");
	exit(1);
    }
    else
    {
	if (toupper(*argv[1]) == 'P')
	    pack();
	else if (toupper(*argv[1]) == 'U')
	    unpack();

	fclose(infile);
	fclose(outfile);
    }
    exit(0);
}

