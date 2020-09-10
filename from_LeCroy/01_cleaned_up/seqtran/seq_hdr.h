/*------------------------- PCW_HDR.H ---------------------------------------*/
/*
Author: Gabriel Goodman

$Header:   F:/asg/code/seqtran/seq_hdr.h_v   1.0   20 Nov 1993 09:57:12   CHRIS_E  $

Cumulative check-in comments:
$Log:   F:/asg/code/seqtran/seq_hdr.h_v  $
 * 
 *    Rev 1.0   20 Nov 1993 09:57:12   CHRIS_E
 * Initial revision.
 * 
 *    Rev 1.0   18 Sep 1989 15:42:58   GABE_G
 * Initial revision.
*/
/*---------------------------------------------------------------------------*/


#ifndef PCW_HDR_ID
#define PCW_HDR_ID

#include "aagen.h"


#define NAMELENGTH  22
#define LINELENGTH  120

typedef BYTE    *PCW_WAVEFORM;
                                /* --------------------------------------
                                    A waveform actually consists of a 
                                    sequence of many different types.
                                    PCW_WAVEFORM is defined as a pointer
                                    to a BYTE so that all address
                                    arithmetic will be done using bytes.
                                ----------------------------------------*/


enum    PCW_TYPE_KINDS {PCW_String_Type,
                        PCW_Text,
                        PCW_Enum_Type,
                        PCW_Byte_Type,
                        PCW_Word_Type,
                        PCW_Long_Type,
                        PCW_Float_Type,
                        PCW_Double_Type,
                        PCW_Unit_Definition,
                        PCW_Time_Stamp,
                        PCW_Data};


enum    PCW_BLOCK_TYPES {PCW_Regular, PCW_Array};
                    
struct PCW_VARIABLE {
    CHAR    Name[NAMELENGTH];
    INT     Offset;
    enum    PCW_TYPE_KINDS  Type;
    CHAR    *EnumListP;
};

struct PCW_BLOCK{
    enum    PCW_BLOCK_TYPES     Type;
    CHAR                        Name[NAMELENGTH];
    struct  PCW_VARIABLE        *VarListP;
};

struct PCW_TEMPLATE{
    CHAR                Name[NAMELENGTH];
    struct  PCW_BLOCK   *BlockListP;
};
            /* The machine template -- see PCW_Get_Template function
               header for description                               */


/*-------------------------------------------------------------------------*/

#define  OUT_OF_MEMORY 1        /* error code */



#ifdef PCW_UTIL_C

#define  NUM_REG_BLOCKS 2       /* max number of regular blocks in template */
#define  NUM_ARRAYS     6       /* max number of arrays in template */

static struct {
    CHAR    Name[NAMELENGTH];
    LONG    Offset;
} regular_blocks[NUM_REG_BLOCKS];

static struct {
    CHAR    Name[NAMELENGTH];
    LONG    Array_Offset;
    LONG    Array_Length;
    LONG    Array_Count;
} array_blocks[NUM_ARRAYS];

#endif




#ifdef PCW_PRT_C

static  INT numprinted;     /* The number of variables printed so far on
                               the current line of output                */

#endif






/*--------------------------------------------------------------------------
        EXTERNAL FUNCTION DEFINITIONS
--------------------------------------------------------------------------*/

#ifndef PCW_PRT_C
extern VOID PCW_Print_Wave_Array();
extern STATUS PCW_Print_Block();
extern VOID PCW_Interpret_Commands();
extern	enum PCW_TYPE_KINDS PCW_Array_Element_Type();
#endif


#ifndef PCW_UTIL_C
extern VOID PCW_Check_Table();
extern struct PCW_BLOCK *PCW_Find_Block();
extern struct PCW_VARIABLE *PCW_Find_Variable();
extern LONG PCW_Find_Block_Offset();
extern VOID *PCW_Find_Value();
extern VOID *PCW_Find_Value_From_Name();
extern CHAR *PCW_Find_Enumeration();
extern VOID PCW_Load_Block_Offsets();
extern STATUS PCW_Find_Array_Dims();
#endif


#ifndef PCW_MTG_C
extern struct PCW_TEMPLATE *PCW_Get_Template();
extern enum PCW_TYPE_KINDS PCW_Classify_Type();
extern enum PCW_BLOCK_TYPES PCW_Classify_Block();
#endif



#endif  /* PCW_HDR_ID */

/*----------------------------- end of file -------------------------------*/

