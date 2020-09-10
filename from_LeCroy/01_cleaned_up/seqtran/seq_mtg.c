/*------------------------- SEQ_MTG.C ---------------------------------------*/
/*
Author: Gabriel Goodman		LeCroy Corporation
				ATS Division
				700 Chestnut Ridge Road
				Chestnut Ridge, NY 10977
*/
/*---------------------------------------------------------------------------*/


#define  PCW_MTG_C

#include <malloc.h>
#include <string.h>
#include "seq_hdr.h"



/*XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
        This file contains the functions which read a template assembly
            listing file and build a machine template.
        PCW_Get_Template(fP) is passed a pointer to the open assembly
            listing file and returns a pointer to the machine template.
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX*/



/*XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX*/

VOID error_handler(error_num)
INT	error_num;

/*--------------------------------------------------------------------------
    Purpose: Handles error conditions.

    Inputs:  error_num specifies the type of error that occurred

    Outputs: Prints appropriate message on the screen

    Machine dependencies: This is written for MSDOS

    Notes:

    Procedure:

/CODE
--------------------------------------------------------------------------*/

{   /* error_handler */
	switch (error_num)
	{
		case OUT_OF_MEMORY:
			printf("\nOut of memory\n");
			exit(1);
			break;
		default:
			printf("\nUnknown error\n");
			exit(1);
	}
}
    



/*XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX*/

static  BOOL ignore_line(buff)
CHAR    *buff;

/*--------------------------------------------------------------------------
    Purpose: Determines whether to ignore a line of input because it is 
             blank or contains only a comment

    Inputs:  buff is a pointer to the character string input line

    Outputs: Return value is TRUE if input line is blank or a comment

    Machine dependencies:

    Notes:

    Procedure:
            Find first nonblank character in line; if it is CR or ';',
            return TRUE

/CODE
--------------------------------------------------------------------------*/

{   /* ignore_line */
    while (*buff && (*buff==' ' || *buff=='\t'))
        buff++;

    return (*buff=='\n' || *buff==';');
}
    



/*XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX*/

static  VOID get_next_line(fP,buff)
FILE    *fP;
CHAR    *buff;

/*--------------------------------------------------------------------------
    Purpose: Obtains the next line from the input file which contains valid
             information (not blank and not a comment)

    Inputs:  fP is a pointer to the open input Text file
             buff points to a character string long enough to hold a line

    Outputs: Copies next line of valid input from text file to buff
             character string

    Machine dependencies:

    Notes:

    Procedure:
            Copy each line from the input file to buff character string
            until get a line which should not be ignored

/CODE
--------------------------------------------------------------------------*/

{   /* get_next_line */
    do
    {
        (VOID)fgets(buff,LINELENGTH,fP);
    } while (ignore_line(buff));
}
    



/*XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX*/

static  VOID get_name_and_type(buff,name_string,type_string,maxlength)
CHAR    *buff;
CHAR    *name_string;
CHAR    *type_string;
INT     maxlength;

/*--------------------------------------------------------------------------
    Purpose: Gets the name and type of a block or variable from a line of the
             template assembly listing.  Obtains a maximum of maxlength 
             characters of the name, skipping over the rest.

    Inputs:  buff is a character string containing the assembly listing line
             name_string is a character string long enough to hold the name
             type_string is a character string long enough to hold the 
                character string name of the type
             maxlength is the maximum length of block or variable name to be
                obtained from the line
    
    Outputs: Copies the block or variable name into name_string (maximum
                of maxlength characters)
             Copies the character string name of the type into type_string

    Machine dependencies:

    Notes:

    Procedure:
            Advance to first nonblank character of line
            Copy block or variable name into name_string one character at a
                time until reach terminating ':' or maxlength characters
            Skip over any remaining characters of name until reach ':'
            Copy character string name of type into type_string

/CODE
--------------------------------------------------------------------------*/

{   /* get_name_and_type */
    INT     index;

    while (*buff == ' '|| *buff=='\t') 
        buff++;

    for (index = 0; *buff != ':' && index < maxlength; buff++, index++)
        name_string[index] = *buff;

    name_string[index] = '\0';

    while (*buff != ':')
        buff++;

    sscanf(++buff, "%s", type_string);
}




/*XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX*/

enum PCW_TYPE_KINDS   PCW_Classify_Type(type_string)
CHAR    *type_string;

/*--------------------------------------------------------------------------
    Purpose: Classifies the character string name of a variable type as one
             of the enumerated PCW_TYPE_KINDS

    Inputs:  type_string is a character string containing the name of a 
             variable type

    Outputs: If type_string contains a valid variable type, the return value
             is the corresponding PCW_TYPE_KINDS enumerated value; otherwise 
             the return value is STATUS_ERR 

    Machine dependencies:

    Notes:

    Procedure:

/CODE
--------------------------------------------------------------------------*/

{   /* Classify_Type */
    static  CHAR    *types[]={
                "string",
                "text",
                "enum",
                "byte",
                "word",
                "long",
                "float",
                "double",
                "unit_definition",
                "time_stamp",
                "data",
                NULL
    };
    INT index;

    for (index=0; types[index] && strcmp(types[index],type_string); index++)
        ;

    if (types[index])
        return (enum PCW_TYPE_KINDS)index;
    else
        return (enum PCW_TYPE_KINDS) STATUS_ERR;
}




/*XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX*/

enum PCW_BLOCK_TYPES   PCW_Classify_Block(type_string)
CHAR    *type_string;

/*--------------------------------------------------------------------------
    Purpose: Classifies the character string name of a block type as one
             of the enumerated PCW_BLOCK_TYPES

    Inputs:  type_string is a character string containing the name of a
             block type

    Outputs: If type_string contains a valid block type, the return value
             is the corresponding PCW_BLOCK_TYPES enumerated value; otherwise 
             the return value is STATUS_ERR

    Machine dependencies:

    Notes:

    Procedure:

/CODE
--------------------------------------------------------------------------*/

{   /* Classify_Block */
    switch (*type_string)
    {
        case 'B':
            return PCW_Regular;
        case 'A':
            return PCW_Array;
        default:
            return (enum PCW_BLOCK_TYPES) STATUS_ERR;
    }
}




/*XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX*/

static  VOID get_enum_type(fP,next_enum_entryP)
FILE    *fP;
CHAR    **next_enum_entryP;

/*--------------------------------------------------------------------------
    Purpose: Reads in the enumerated values of an enumerated type from the
             template assembly listing file and stores
             them sequentially in the array of character string enumerated
             value names starting at the next available location (pointed 
             to by next_enum_entryP)

    Inputs:  fP points to the open template assembly listing text file
             next_enum_entryP points to the next available location in the
                array of character strings in which the enumerated type value
                names are stored

    Outputs: Stores the enumerated value names in character strings of
                length NAMELENGTH sequentially starting at address
                next_enum_entryP
             Updates value of next_enum_entryP to point to next available
                location

    Machine dependencies:

    Notes:

    Procedure:
            Get next line from template assembly listing file
            Skip over enumeration number and copy enumeration value name
                to location pointed to by next_enum_entryP
            Increment next_enum_entryP by NAMELENGTH
            Repeat until get line with flag "endenum"

/CODE
--------------------------------------------------------------------------*/

{   /* get_enum_type */
    CHAR    buff[LINELENGTH];

    get_next_line(fP,buff);

    sscanf(buff, "%s", *next_enum_entryP);
    while (strcmp(*next_enum_entryP,"endenum"))
    {
        sscanf(buff, " _%*d %19s", *next_enum_entryP);
        *next_enum_entryP += NAMELENGTH;
        get_next_line(fP,buff);
        sscanf(buff, "%s", *next_enum_entryP);
    }
}
        



/*XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX*/

static  BOOL get_variable(fP,next_varP,next_enum_entryP)
FILE    *fP;
struct  PCW_VARIABLE    *next_varP;
CHAR    **next_enum_entryP;

/*--------------------------------------------------------------------------
    Purpose: Reads the next variable from the template assembly listing file
                and creates an entry for it in the machine template.
                The entry is a Variable structure which contains the variable
                name, offset, type, and a pointer to the list of enumerated
                value names (if the type is enumerated).

    Inputs:  fP points to the open template assembly listing file
             next_varP points to the next available location in the array
                of Variable structures in which the information for each
                variable is stored
             next_enum_entryP points to the next available location in the
                array of character strings in which the enumerated type value
                names are stored

    Outputs: Return value is TRUE if a variable was read from the template
                assembly listing file; FALSE if the end of block was reached
             Stores the variable name, offset, type, and pointer to list of
                enumerated value names (if the type is enumerated) in the
                Variable structure pointed to by next_varP
             Updates value of next_enum_entryP to point to next available
                location

    Machine dependencies:

    Notes:

    Procedure:
            Get next line from template assembly listing file
            If line starts with '/', we have reached end of block -
                return FALSE
            Read variable offset, name, and character string type name
            Convert character string type name to PCW_TYPE_KINDS enumerated
                value
            If variable is an enumerated type, read in enumerated type
                values and store in array starting at next_enum_entryP

/CODE
--------------------------------------------------------------------------*/

{   /* get_variable */
    CHAR    buff[LINELENGTH];
    CHAR    type_string[NAMELENGTH];

    get_next_line(fP,buff);
    if (*buff == '/') 
        return FALSE;

    sscanf(buff, "< %d>", &(next_varP->Offset));

    get_name_and_type(buff+10,next_varP->Name,type_string,NAMELENGTH-1);

    if ((next_varP->Type = PCW_Classify_Type(type_string)) == PCW_Enum_Type)
    {
        next_varP->EnumListP = *next_enum_entryP;
        get_enum_type(fP,next_enum_entryP);
    }
    else    
        next_varP->EnumListP = NULL;

    return TRUE;
}




/*XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX*/

static  VOID get_block(fP,blockP,next_enum_entryP)
FILE    *fP;
struct PCW_BLOCK    *blockP;
CHAR    **next_enum_entryP;

/*--------------------------------------------------------------------------
    Purpose: Reads the next block from the template assembly listing file
                and creates an entry for it in the machine template.
                The entry is a Block structure which contains the block name,
                offset, block type, and a pointer to the array of Variable
                structures for the variables in the block.

    Inputs:  fP points to the open template assembly listing file
             blockP points to the next available location in the array of
                Block structures in which the information for each block
                is stored
             next_enum_entryP points to the next available location in the
                array of character strings in which the enumerated type value
                names are stored

    Outputs: Fills in a Block structure, an array of Variable structures, and
                and array of character string enumerated type values for the
                next block in the template assembly listing file.
                Fills in the Block structure pointed to by blockP, starts
                filling in the array of Variable structures at the location
                pointed to by blockP->VarListP, and starts writing enumerated
                type character string value names at the location pointed
                to by next_enum_entryP.
            Updates VarListP of the next Block structure to point to the next
                available Variable structure.
            Updates value of next_enum_entryP to point to next available 
                location for enumerated type value names.

    Machine dependencies:

    Notes:

    Procedure:
            Get block name and type, and classify block type
            While there is a variable
                Get the variable

/CODE
--------------------------------------------------------------------------*/

{   /* get_block */
    CHAR    buff[LINELENGTH];
    CHAR    type_string[NAMELENGTH];
    struct  PCW_VARIABLE    *next_varP;

    get_next_line(fP,buff);
    get_name_and_type(buff,blockP->Name,type_string,NAMELENGTH-1);
    blockP->Type = PCW_Classify_Block(type_string);

    next_varP = blockP->VarListP;
    while (get_variable(fP,next_varP,next_enum_entryP))
        next_varP++;

    (++blockP)->VarListP = next_varP;
}




/*XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX*/

static  VOID get_template_name(fP,templateP)
FILE    *fP;
struct PCW_TEMPLATE *templateP;

/*--------------------------------------------------------------------------
    Purpose: Skip over any header in the template assembly listing file
                (whether or not the header lines are legitimately labeled
                as comments by starting with ';') and read the template name

    Inputs:  fP points to the open template assembly listing file
             templateP points to the Template structure

    Outputs: stores the name of the template in the Template structure pointed
                to by templateP

    Machine dependencies:

    Notes:

    Procedure:
            Skip lines until read a line which starts with '/'
            Read the template name from the next line

/CODE
--------------------------------------------------------------------------*/

{   /* get_template_name */
    CHAR    buff[LINELENGTH];

    do
    {
        get_next_line(fP,buff);
    } while (buff[0] != '/');
    get_next_line(fP,buff);
    sscanf(buff+20, "%s", templateP->Name);
    templateP->Name[strlen(templateP->Name)-1] = '\0';  /* remove final ':' */
}




/*XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX*/

static  VOID get_sizes(fP,num_blocks,num_vars,num_enums)
FILE    *fP;
INT     *num_blocks,
        *num_vars,
        *num_enums;

/*--------------------------------------------------------------------------
    Purpose: Read the number of blocks, number of variables, and number of
                enumerations from the template assembly listing file

    Inputs:  fP points to the open template assembly listing file
             num_blocks is the number of blocks in the template
             num_vars is the number of variables in the template
             num_enums is the number of enumerations in the template

    Outputs: reads the number of blocks into num_blocks
             reads the number of variables into num_vars
             reads the number of enumerations into num_enums

    Machine dependencies:

    Notes:

    Procedure:

/CODE
--------------------------------------------------------------------------*/

{   /* get_sizes */
    CHAR    buff[LINELENGTH];

    get_next_line(fP,buff);
    sscanf(buff,"%d %d %d", num_blocks, num_vars, num_enums);
}




/*XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX*/

struct PCW_TEMPLATE *PCW_Get_Template(fP)
FILE    *fP;

/*--------------------------------------------------------------------------
    Purpose: Reads in a template from the template assembly listing file
             and creates a machine template for it.  The machine template
             consists of the following:
                Template structure containing:
                    template name
                    pointer to array of Block structures for the template
                Array of Block structures, each containing:
                    block name
                    block type
                    pointer to array of Variable structures for that block
                Array of Variable structures, each containing:
                    variable name
                    offset of variable within block
                    type of variable (as a PCW_TYPE_KINDS enumerated value)
                    pointer to array of character string enumerated value
                        names for the variable (pointer = NULL if the
                        variable type is not enumerated)    

             Because each Block points to the
             beginning of the array of Variables for that Block, the end of
             the Variable array for each Block is indicated by reaching the
             address pointed to by the next Block (the arrays of Variables
             for each Block are allocated sequentially as one large array).
             The array of Block structures is terminated by a dummy Block
             structure which contains a null name and a pointer to the
             next location past the end of the array of Variable structures.
             This gives a termination indication for both the array of Blocks
             and the array of Variables.  There is no termination indication
             for the array of enumerated value names; therefore no range
             checking is done on enumerated values as they are used to index
             into the array of enumrated value names.  If desired, the 
             termination indication can be added in the form of a dummy 
             Variable structure at the end of the array of Variables.

    Inputs:  fP points to the open template assembly listing file

    Outputs: Return value is a pointer to the machine template (specifically,
                a pointer to the Template structure of the machine template).

    Machine dependencies:

    Notes:   All space for the machine template is allocated in this function
             (using malloc) based on the number of blocks, number of variables,
             and number of enumerations given in the template assembly listing
             file.

    Procedure:
            Allocate space for Template structure
            Read in template name
            Read in number of blocks, number of variables, and number of enums
            Allocate space for the array of Block structures (including dummy)
            Allocate space for the array of Variable structures
            Allocate space for the array of character string enumerated value
                names
            For each block
                Get the block
            Set up dummy Block structure

/CODE
--------------------------------------------------------------------------*/

{   /* Get_Template */
    struct PCW_TEMPLATE *templateP;
    INT     num_blocks,
            num_vars,
            num_enums;
    CHAR    *enum_startP;
    INT     block_index;

    templateP = (struct PCW_TEMPLATE *)malloc(2 * sizeof(struct PCW_TEMPLATE));

    get_template_name(fP,templateP);
    get_sizes(fP,&num_blocks,&num_vars,&num_enums);

    templateP->BlockListP = (struct PCW_BLOCK *)malloc((num_blocks+1) * 2 *
                                                    sizeof(struct PCW_BLOCK));
    if (!templateP->BlockListP)
		error_handler(OUT_OF_MEMORY);


    templateP->BlockListP->VarListP = (struct PCW_VARIABLE *)malloc(num_vars *
                                            2 * sizeof(struct PCW_VARIABLE));
    if (!templateP->BlockListP->VarListP)
		error_handler(OUT_OF_MEMORY);


    enum_startP = (char *)malloc(NAMELENGTH * 2 * num_enums * sizeof(char));
    if (!enum_startP)
		error_handler(OUT_OF_MEMORY);

    for (block_index = 0; block_index < num_blocks; block_index++)
        get_block(fP, templateP->BlockListP + block_index, &enum_startP);

    *((templateP->BlockListP + num_blocks) -> Name) = '\0'; /* dummy Block */

    return templateP;
}

/*----------------------------- end of file -------------------------------*/

