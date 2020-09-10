/*------------------------- SEQ_PRT.C ---------------------------------------*/
/*
Author: Gabriel Goodman		LeCroy Corporation
				ATS Division
				700 Chestnut Ridge Road
				Chestnut Ridge, NY 10977

*/
/*---------------------------------------------------------------------------*/


#define  PCW_PRT_C

#include <stdio.h>
#include <string.h>
#include "seq_hdr.h"

/*XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
        This file contains the functions which read a file of
            format commands and print the appropriate parts of the
            waveform descriptor.
        PCW_Interpret_Commands(in_fP,out_fP,templateP,waveformP) is
            passed pointers to the open print format file, the open
            output file, the machine template, and the waveform descriptor.
            It interprets and executes each print command in the format file.
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX*/



/*XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX*/

VOID skip_lines(fP,numlines)
FILE    *fP;
INT     numlines;

/*--------------------------------------------------------------------------
    Purpose: Writes the specified number of blank lines to the output file.

    Inputs:  fP is a pointer to the open ouput text file
             numlines is the number of blank lines to write

    Outputs: Writes numlines blank lines to the file pointed to by fP

    Machine dependencies:

    Notes:

    Procedure:

/CODE
--------------------------------------------------------------------------*/

{   /* skip_lines */
    INT     line_count;

    for (line_count = 0; line_count < numlines; line_count++)
        fprintf(fP,"\n");
}




/*XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX*/

static  CHAR *month_string(month_number)
UBYTE   month_number;

/*--------------------------------------------------------------------------
    Purpose: Converts the number of a month to a 3-letter character string
             month name

    Inputs: month_number is the number of the month (1-12)

    Outputs: Returns a pointer to the 3-letter character string month name

    Machine dependencies:

    Notes:

    Procedure:

/CODE
--------------------------------------------------------------------------*/

{   /* month_string */
    static  CHAR    *months[]={
                            "JAN",
                            "FEB",
                            "MAR",
                            "APR",
                            "MAY",
                            "JUN",
                            "JUL",
                            "AUG",
                            "SEP",
                            "OCT",
                            "NOV",
                            "DEC"
    };

    return months[month_number-1];
}




/*XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX*/

static  VOID print_value(fP,valueP,type,varP)
FILE                    *fP;
VOID                    *valueP;
enum    PCW_TYPE_KINDS  type;
struct  PCW_VARIABLE    *varP;

/*--------------------------------------------------------------------------
    Purpose: Prints to the output file the value of the specified variable
             in the waveform descriptor.  This function is used to print
             both variables in regular blocks and elements of arrays.

    Inputs:  fP is a pointer to the open output text file
             valueP is a pointer to the variable in the waveform descriptor
             type is the type of the variable (a Type_Kinds enumerated value)
             varP is a pointer to the Variable structure for the variable in
                the machine template -- this is needed for an enumerated type

    Outputs: Prints the value of the specified variable to the output file

    Machine dependencies:  This function uses types as defined in aagen.h
                           These types must be defined correctly for the
                           machine to correspond to the variable size types
                           defined for the waveform descriptor.

    Notes:

    Procedure:

/CODE
--------------------------------------------------------------------------*/

{   /* print_value */
    switch (type)
    {
        case PCW_Text:
            numprinted = -1;        /* start new line after this variable */
        case PCW_String_Type:
            fprintf(fP,"%-19s",(CHAR *)valueP);
            break;
        case PCW_Enum_Type:
            fprintf(fP,"%-19s",
                        PCW_Find_Enumeration(GETUWORD valueP,varP));
            break;
        case PCW_Byte_Type:
            fprintf(fP,"%-19hd",GETUBYTE valueP);
            break;
        case PCW_Word_Type:
            fprintf(fP,"%-19hd",GETWORD valueP);
            break;
        case PCW_Long_Type:
            fprintf(fP,"%-19ld",GETLONG valueP);
            break;
        case PCW_Float_Type:
            fprintf(fP,"%-19g",GETFLOAT valueP);
            break;
        case PCW_Double_Type:
            fprintf(fP,"%-19.9lg",*((DOUBLE *)valueP));
            break;
        case PCW_Unit_Definition:
            fprintf(fP,"%s",(CHAR *)valueP);
            numprinted = -1;        /* start new line after this variable */
            break;
        case PCW_Time_Stamp:
            fprintf(fP,"Date = %3s %2hd, %4hd, Time = %2hd:%02hd:%06.4lf",
                    month_string(*((UBYTE *) valueP + 11)),
                    *((UBYTE *) valueP + 10),
                    *((WORD *)valueP + 6),
                    *((UBYTE *)valueP + 9),
                    *((UBYTE *)valueP + 8),
                    *((DOUBLE *)valueP));
            numprinted = -1;        /* start new line after this variable */
            break;
    }

}




/*XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX*/

static  VOID print_variable(fP,valueP,varP,type,labeled,numcols,numlines)
FILE                    *fP;
VOID                    *valueP;
struct  PCW_VARIABLE    *varP;
enum    PCW_TYPE_KINDS  type;
BOOL                    labeled;
INT                     numcols;
INT                     numlines;

/*--------------------------------------------------------------------------
    Purpose: Prints a specified variable to the output file 
             formatted according to the specified labelling, number of columns,
             and number of lines to be skipped between each line of variables.
             This function is used both for printing a variable in a regular
             block and for printing an element of an array.

    Inputs:  fP is a pointer to the open output text file
             valueP is a pointer to the variable in the waveform descriptor
             varP is a pointer to the Variable structure for the variable
                in the machine template
             type is the type of the variable (a Type_Kinds enumerated value)
             labeled=TRUE means print the name of the variable in addition to
                the value; labeled=FALSE means print the value only
             numcols is the number of columns of variables to print per line
             numlines is the number of lines to skip between lines of variables

    Outputs: Prints the specified variable to the output file appropriately 
             formatted 

    Machine dependencies:

    Notes:

    Procedure:

/CODE
--------------------------------------------------------------------------*/

{   /* print_variable */
    if (numprinted == numcols || numprinted == -1 || 
        varP->Type == PCW_Unit_Definition || varP->Type == PCW_Time_Stamp ||
        varP->Type == PCW_Text)
            /*----------------------------------------------------------------
                Start new line after have filled previous line, when new-line
                flag is set, or before printing a unit definition, a time
                stamp, or a text variable
            -----------------------------------------------------------------*/
    { 
        skip_lines(fP,numlines+1);
        numprinted = 1;
    }
    else
        numprinted++;

    if (labeled)
        fprintf(fP,"%-18s: ",varP->Name);

    print_value(fP,valueP,type,varP);
}

    


/*XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX*/

static  STATUS print_variable_from_name(fP,waveformP,block_offset,blockP,
                                 var_name,labeled,numcols,numlines)
FILE                *fP;
PCW_WAVEFORM        waveformP;
LONG                block_offset;
struct  PCW_BLOCK   *blockP;
CHAR                *var_name;
BOOL                labeled;
INT                 numcols;
INT                 numlines;

/*--------------------------------------------------------------------------
    Purpose: Prints a variable specified by its name to the output file 
             formatted according to the specified labelling, number of columns,
             and number of lines to be skipped between each line of variables.

    Inputs:  fP is a pointer to the open output text file
             waveformP is a pointer to the waveform descriptor
             block_offset is the offset in the waveform descriptor of the block
                which contains the variable to be printed
             blockP is a pointer to the Block structure in the machine template
                for the block which contains the variable to be printed
             var_name is the character string name of the variable
             labeled=TRUE means print the name of the variable in addition to
                the value; labeled=FALSE means print the value only
             numcols is the number of columns of variables to print per line
             numlines is the number of lines to skip between lines of variables

    Outputs: If the specified variable exists, prints the variable to the 
             output file appropriately formatted and returns STATUS_OK;
             otherwise returns STATUS_ERR.

    Machine dependencies:

    Notes:

    Procedure:

/CODE
--------------------------------------------------------------------------*/

{   /* print_variable_from_name */
    struct  PCW_VARIABLE    *varP;

    varP = PCW_Find_Variable(var_name,blockP);
    if (varP == NULL)
        return STATUS_ERR;

    print_variable(fP,PCW_Find_Value(waveformP,block_offset,varP),varP,
                   varP->Type,labeled,numcols,numlines);

    return STATUS_OK;
}




/*XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX*/

enum PCW_TYPE_KINDS     PCW_Array_Element_Type(varP,templateP,waveformP)
struct  PCW_VARIABLE    *varP;
struct  PCW_TEMPLATE    *templateP;
PCW_WAVEFORM            waveformP;

/*--------------------------------------------------------------------------
    Purpose: Returns the type of the specified array element.  This is
             needed because of the complex array element type "data" which
             means that the type of the array element is given by the value
             of COMM_TYPE in the WAVEDESC block.

    Inputs:  varP is a pointer to the Variable structure in the machine 
                template for the array element variable (in the template
                block for the array)
             templateP is a pointer to the machine template
             waveformP is a pointer to the waveform descriptor

    Outputs: Returns the type (as an enumerated Type_Kinds value) of the
             array element indicated by varP. 

    Machine dependencies:

    Notes:   

    Procedure:
            If the type of the array element in the machine template is not
            Data, simply return the type of the element.
            Otherwise, find the value of the COMM_TYPE variable in the
            WAVEDESC block in the waveform descriptor, and classify that
            value as one of the Type_Kinds.

/CODE
--------------------------------------------------------------------------*/

{   /* PCW_Array_Element_Type */
    struct  PCW_BLOCK       *blockP;
    struct  PCW_VARIABLE    *comm_type_varP;
    LONG                    block_offset;

    if (varP->Type != PCW_Data)
        return varP->Type;

    blockP = PCW_Find_Block("WAVEDESC",PCW_Regular,templateP);
    block_offset = PCW_Find_Block_Offset(blockP);
    comm_type_varP = PCW_Find_Variable("COMM_TYPE",blockP);
    
    return PCW_Classify_Type(PCW_Find_Enumeration(
            GETUWORD PCW_Find_Value(waveformP,block_offset,comm_type_varP),
            comm_type_varP));
}

    



/*XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX*/

VOID PCW_Print_Wave_Array(fP,templateP,waveformP,array_name,numcols,raw_ADC)
FILE                    *fP;
struct  PCW_TEMPLATE    *templateP;
PCW_WAVEFORM            waveformP;
CHAR                    *array_name;
INT                     numcols;
BOOL                    raw_ADC;

/*--------------------------------------------------------------------------
    Purpose: 

    Inputs:  fP is a pointer to the open output text file
             templateP is a pointer to the machine template
             waveformP is a pointer to the waveform descriptor
             array_name is the character string name of the array to print
             numcols is the number of array points to print per line

    Outputs: The array is printed to the output file appropriately formatted.

    Machine dependencies:

    Notes:

    Procedure:

/CODE
--------------------------------------------------------------------------*/

{   /* PCW_Print_Wave_Array */
    struct  PCW_BLOCK       *blockP;
    LONG                    array_offset;
    LONG                    num_points;
    enum    PCW_TYPE_KINDS  element_type;
    LONG                    index;
    BYTE                    *valueP;
    FLOAT                   vertical_gain;
    FLOAT                   vertical_offset;
    INT                     value;
    DOUBLE		    d_time_value;
    DOUBLE		    d_offset_value;


    blockP = PCW_Find_Block(array_name,PCW_Array,templateP);
    if (blockP == NULL)
        return;

    array_offset = PCW_Find_Block_Offset(blockP);
    if (array_offset == -1)
        return;

    element_type =PCW_Array_Element_Type(blockP->VarListP,templateP,waveformP);

    blockP = PCW_Find_Block("WAVEDESC",PCW_Regular,templateP);
    vertical_gain = *(FLOAT *)PCW_Find_Value_From_Name(waveformP,(LONG)0,
                                                blockP,"VERTICAL_GAIN");
    vertical_offset = *(FLOAT *)PCW_Find_Value_From_Name(waveformP,(LONG)0,
                                                blockP,"VERTICAL_OFFSET");


    (VOID)PCW_Find_Array_Dims(array_name,&index,&num_points);

    numprinted = 0;

    valueP = waveformP + array_offset;
    for (index = 0; index < num_points; index++)
    {
        if (element_type == PCW_Byte_Type)
        {
            value = GETUBYTE valueP;
            valueP++;
        }
        else if (element_type == PCW_Word_Type)
        {
            value = GETWORD valueP;
            valueP += 2;
        }
        else if (element_type == PCW_Double_Type)
	{
            d_time_value = *(DOUBLE *)valueP;
            valueP += 8;
            d_offset_value = *(DOUBLE *)valueP;
            valueP += 8;
	}
        else
        {
            printf("Invalid WAVE ARRAY element type\n");
            return;
        }

	if (element_type == PCW_Double_Type)
	{
	    fprintf(fP,"%-19G%-19G", d_time_value,d_offset_value);
	}
	else
	{
	    if (raw_ADC)
	    {
		fprintf(fP,"%-19d",value);
	    }
	    else
	    {
		fprintf(fP,"%-19G", value * vertical_gain - vertical_offset);
	    }
	}

        numprinted++;
        if (numprinted >= numcols)
        {
            fprintf(fP,"\n");
            numprinted = 0;
        }
    }

}
        



/*XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX*/

static  INT print_array_labels(fP,blockP,numlines,labeled)
FILE                    *fP;
struct  PCW_BLOCK       *blockP;
BOOL                    labeled;

/*--------------------------------------------------------------------------
    Purpose: Prints the column header names for a multidimensional array and
             counts the number of dimensions of each point.

    Inputs:  fP is a pointer to the open output text file
             blockP is a pointer to the Block structure in the machine template
                for the array block
             numlines is the number of lines to skip between each line of
                output
            labeled=TRUE means print the column header names; labeled=FALSE
                means do not print the column header names, but still count
                the number of dimensions

    Outputs: If labeled=TRUE, prints the column header names for the array to
             the output file. 
             Returns the number of dimensions of each point of the array.

    Machine dependencies:

    Notes:

    Procedure:

/CODE
--------------------------------------------------------------------------*/

{   /* print_array_labels */
    struct  PCW_VARIABLE    *varP;
    INT                     num_dimensions;

    if (labeled)
        fprintf(fP,"ARRAY: %s\n",blockP->Name);

    num_dimensions = 0;
    varP = blockP->VarListP;
    blockP++;                       /*---------------------------------------- 
                                       End of variable array for this block
                                       is indicated by reaching address 
                                       pointed to by VarListP of next block
                                    -----------------------------------------*/
    while (varP != blockP->VarListP)
    {
        if (labeled)
            fprintf(fP,"%-19s",varP->Name);
        varP++;
        num_dimensions++;
    }

    skip_lines(fP,numlines+1);
    
    return num_dimensions;
}
            



/*XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX*/

static  VOID print_array(fP,templateP,waveformP,array_name,blockP,block_offset,
                 labeled,numcols,numlines,start_point,num_points)
FILE                    *fP;
struct  PCW_TEMPLATE    *templateP;
PCW_WAVEFORM            waveformP;
CHAR                    *array_name;
struct  PCW_BLOCK       *blockP;
LONG                    block_offset;
BOOL                    labeled;
INT                     numcols;
INT                     numlines;
LONG                    start_point;
LONG                    num_points;

/*--------------------------------------------------------------------------
    Purpose: Prints the specified array to the output file formatted
             appropriately as specified by the labelling mode (on or off),
             the number of columns per line, the number of blank lines to skip 
             between rows of the array, the starting point of the array, and
             the number of points to print.

    Inputs:  fP is a pointer to the open output text file
             templateP is a pointer to the machine template
             waveformP is a pointer to the waveform descriptor
             array_name is the character string name of the array to print
             blockP is a pointer to the Block structure for the array in
                the machine template
             block_offset is the offset within the waveform descriptor of
                the array
             labeled=TRUE means label the array; labeled=FALSE means print
                the array without labels
             numcols is the number of array points to print per line for a
                single dimensional array; if the array is multidimensional,
                one multidimensional point is printed per line
             numlines is the number of blank lines to skip between each line
                of array output
             start_point is the first array point to start printing out
                (the first point is indicated by start_point = 1)
             num_points is the number of array points to print out
                num_points = -1 means print the entire array

    Outputs: The array is printed to the output file appropriately formatted.

    Machine dependencies:

    Notes:   The length in bytes of each (possibly multidimensional) 
             array point is calculated by dividing the total length in bytes
             of the array by the count of the number of points.  This must
             be modified if the array contains a header and/or trailer.

    Procedure:

/CODE
--------------------------------------------------------------------------*/

{   /* print_array */
    LONG                    length;
    LONG                    count;
    LONG                    group_length;
    struct  PCW_VARIABLE    *varP;
    LONG                    group_index;
    BYTE                    *valueP;
    INT                     num_dimensions;


    (VOID)PCW_Find_Array_Dims(array_name,&length,&count);
    group_length = length / count;              /* change for header, trailer*/

    num_dimensions = print_array_labels(fP,blockP,numlines,labeled);

    numcols = (num_dimensions > 1) ? num_dimensions : numcols;

    if (num_points == -1)
        num_points = count;
    else
        num_points = (num_points > count) ? count : num_points;

    valueP = waveformP + block_offset + group_length * (start_point - 1);
    for (group_index = 0; group_index < num_points; group_index++)
    {
        varP = blockP->VarListP;
        while (varP != (blockP+1)->VarListP)
                                    /*---------------------------------------- 
                                       End of variable array for this block
                                       is indicated by reaching address 
                                       pointed to by VarListP of next block
                                    -----------------------------------------*/
        {
            print_variable(fP,valueP+varP->Offset,varP,
                           PCW_Array_Element_Type(varP,templateP,waveformP),
                           FALSE,numcols,numlines);
            varP++;
        }
        valueP += group_length;
    }
}
        



/*XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX*/

STATUS PCW_Print_Block(fP,templateP,waveformP,block_name,block_type,
                       labeled,numcols,numlines,start_point,num_points)
FILE                    *fP;
struct  PCW_TEMPLATE    *templateP;
PCW_WAVEFORM            waveformP;
CHAR                    *block_name;
enum    PCW_BLOCK_TYPES block_type;
BOOL                    labeled;
INT                     numcols;
INT                     numlines;
LONG                    start_point;
LONG                    num_points;

/*--------------------------------------------------------------------------
    Purpose: Prints the block specified by name and block type to the output 
             file formatted appropriately as specified by the labelling mode,
             the number of columns of variables per line, the number of blank 
             lines to skip between lines of output, the starting point and
             number of points to print (if the block is an array).

    Inputs:  fP is a pointer to the open output text file
             templateP is a pointer to the machine template
             waveformP is a pointer to the waveform descriptor
             block_name is the character string name of the block to print
             block_type is the type of the block 
                (a Block_Types enumerated value)
             labeled=TRUE means label the variables in the block; 
                labeled=FALSE means print the variables without labels
             numcols is the number of variables to print per line
             numlines is the number of blank lines to skip between each line
                of output
             start_point is the first array point to start printing out
                (the first point is indicated by start_point = 1)
             num_points is the number of array points to print out
                num_points = -1 means print the entire array

    Outputs: The variables of the block are printed to the output file 
                appropriately formatted.
             If the specified block does not exist in the machine template,
                the return value is STATUS_ERR; otherwise the return value
                is STATUS_OK.  If the specified block exists in the machine
                template but not in the waveform descriptor, nothing is 
                printed (the return value is STATUS_OK).  This allows an
                entire waveform descriptor to be printed out by going through
                the machine template and printing each block, whether or not
                it is actually present in the wavefrom descriptor. 

    Machine dependencies:

    Notes:   

    Procedure:

/CODE
--------------------------------------------------------------------------*/

{   /* PCW_Print_Block */
    struct  PCW_BLOCK       *blockP;
    struct  PCW_VARIABLE    *varP;
    LONG                    block_offset;


    blockP = PCW_Find_Block(block_name,block_type,templateP);
    if (blockP == NULL)
        return STATUS_ERR;

    block_offset = PCW_Find_Block_Offset(blockP);
    if (block_offset == -1)
        return STATUS_OK;

    numprinted = 0;

    if (blockP->Type == PCW_Array)
        print_array(fP,templateP,waveformP,block_name,blockP,block_offset,
                 labeled,numcols,numlines,start_point,num_points);
    else
    {
        varP = blockP->VarListP;
        blockP++;
        while (varP != blockP->VarListP)
        {
            print_variable(fP,PCW_Find_Value(waveformP,block_offset,varP),varP,
                       varP->Type,labeled,numcols,numlines);
            varP++;
        }
    }

    skip_lines(fP,numlines+3);
    return STATUS_OK;
}




/*XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX*/

static  VOID print_waveform(out_fP,templateP,waveformP,labeled,
                            numcols,numlines)
FILE                    *out_fP;
struct  PCW_TEMPLATE    *templateP;
PCW_WAVEFORM            waveformP;
BOOL                    labeled;
INT                     numcols;
INT                     numlines;

/*--------------------------------------------------------------------------
    Purpose: Prints to the output file the contents of an entire waveform
             descriptor (all regular blocks and all arrays, in the order
             given in the machine template).

    Inputs:  out_fP is a pointer to the open output text file
             templateP is a pointer to the machine template
             waveformP is a pointer to the waveform descriptor
             labeled=TRUE means label the variables in the block; 
                labeled=FALSE means print the variables without labels
             numcols is the number of variables to print per line
             numlines is the number of blank lines to skip between each line
                of output

    Outputs: Prints the entire waveform descriptor to the output file formatted
             according to labeled, numcols and numlines

    Machine dependencies:

    Notes:

    Procedure:

/CODE
--------------------------------------------------------------------------*/

{   /* print_waveform */
    struct  PCW_BLOCK   *blockP;
    
    blockP = templateP->BlockListP;
    while (*blockP->Name)           /* null name marks end of block array */
    {
        (VOID)PCW_Print_Block(out_fP,templateP,waveformP,blockP->Name,
                    blockP->Type,labeled,numcols,numlines,(LONG)1,(LONG)-1);
        blockP++;
    }
}




/*XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX*/

static  VOID do_print_command(in_fP,out_fP,waveformP,templateP,blockPP,
                              block_offsetP,labeled,numcols,numlines)
FILE                    *in_fP;
FILE                    *out_fP;
PCW_WAVEFORM            waveformP;
struct  PCW_TEMPLATE    *templateP;
struct  PCW_BLOCK       **blockPP;
LONG                    *block_offsetP;
BOOL                    labeled;
INT                     numcols;
INT                     numlines;

/*--------------------------------------------------------------------------
    Purpose: Interprets and executes a $PRINT command from the print format 
             file.

    Inputs:  in_fP is a pointer to the open print format text file (the file
                containing the print format commands)
             out_fP is a pointer to the open output text file
             waveformP is a pointer to the waveform descriptor
             templateP is a pointer to the machine template
             blockPP is a pointer to the Block structure in the machine
                template for the block whose variables are currently being
                printed as specified in the print format file
             block_offsetP is the offset of the block whose variables are
                currently being printed as specified in the format file
             labeled=TRUE means label the variables in the block; 
                labeled=FALSE means print the variables without labels
             numcols is the number of variables to print per line
             numlines is the number of blank lines to skip between each line
                of output

    Outputs: If the $PRINT command is $PRINT WAVEFORM, the entire waveform
                descriptor is printed to the output file.
             If the $PRINT command is print an array or print a block in
                the default manner, the appropriate block or array is printed.
             If the $PRINT command is print a block as specified by the
                variables listed in the following lines of the format file,
                blockPP is set to point to the Block structure in the machine
                template for the given block, and block_offsetP is set to the
                offset in the waveform descriptor of the given block.  If the
                block is not in the template or is in the template but is not
                in the waveform descriptor, block_offsetP is set to -1.

    Machine dependencies:

    Notes:

    Procedure:

/CODE
--------------------------------------------------------------------------*/

{   /* do_print_command */
    CHAR                    type_string[NAMELENGTH];
    CHAR                    block_name[NAMELENGTH];
    CHAR                    manner[NAMELENGTH];
    enum  PCW_BLOCK_TYPES   block_type;
    LONG                    start_point;
    LONG                    num_points;

    fscanf(in_fP, "%s", type_string);

    if (!strcmp(type_string,"WAVEFORM"))
    {
        print_waveform(out_fP,templateP,waveformP,labeled,numcols,numlines);
        return;
    }

    fscanf(in_fP, "%s", block_name);

    block_type = PCW_Classify_Block(type_string);

    if (block_type != PCW_Array)
        fscanf(in_fP,"%s",manner);
    else
    {
        fscanf(in_fP,"%ld",&start_point);
        if (!fscanf(in_fP,"%ld",&num_points))
            num_points = -1;                        /* unspecified number of
                                                       points means print all*/
    }
        

    if (block_type == PCW_Array || !strcmp(manner,"DEFAULT"))
        (VOID)PCW_Print_Block(out_fP,templateP,waveformP,block_name,block_type,
                        labeled,numcols,numlines,start_point,num_points);
    else
    {
        *blockPP = PCW_Find_Block(block_name,block_type,templateP);
        if (*blockPP != NULL)
            *block_offsetP = PCW_Find_Block_Offset(*blockPP);
        else
            *block_offsetP = -1;        /* flag block not in template */

        numprinted = 0;
    }
}
    



/*XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX*/

VOID    PCW_Interpret_Commands(in_fP,out_fP,templateP,waveformP)
FILE                    *in_fP;
FILE                    *out_fP;
struct  PCW_TEMPLATE    *templateP;
PCW_WAVEFORM            waveformP;

/*--------------------------------------------------------------------------
    Purpose: Interprets and executes each command in the print format file.

    Inputs:  in_fP is a pointer to the open print format text file containing
                a command on each line
             out_fP is a pointer to the open output text file to which the
                waveform is to be printed
             templateP is a pointer to the machine template
             waveformP is a pointer to the waveform descriptor

    Outputs: Executes each command in the print format file and prints the
             appropriate waveform contents to the output file.

    Machine dependencies:

    Notes:

    Procedure:
            For each line of the print format file:
                If the line is a command (starts with '$'):
                    1)$PRINT command: execute print command
                    2)$COLS: set numcols variable
                    3)$LINES: set numlines variable
                    4)$LABELS: set labels variable
                    5)$SKIP: skip the specified number of lines
                    6)$TITLE: print the specified title string
                Otherwise the line is the name of a variable:
                    Print the variable, assuming it is located in the block
                    specified by the last $PRINT BLOCK "name" SPECIFIED
                    command

/CODE
--------------------------------------------------------------------------*/

{   /* Interpret_Commands */
    CHAR                command[NAMELENGTH];
    struct  PCW_BLOCK   *blockP;
    LONG                block_offset;
    INT                 numcols;
    INT                 numlines;
    BOOL                labeled;
    INT                 lines_to_skip;
    CHAR                title[LINELENGTH];

    numcols = 1;
    numlines = 0;
    labeled = TRUE;
    blockP = NULL;
    block_offset = -1;

    while (!feof(in_fP))
    {
        fscanf(in_fP, "%s", command);

        if (*command != '$')
        {
            if (block_offset != -1)     /* -1 means block not in waveform  */
                (VOID)print_variable_from_name(out_fP,waveformP,block_offset,
                    blockP,command,labeled,numcols,numlines);
        }
        else if (!strcmp(command,"$PRINT"))
            do_print_command(in_fP,out_fP,waveformP,templateP,&blockP,
                    &block_offset,labeled,numcols,numlines);
        else if (!strcmp(command,"$COLS"))
        {
            fscanf(in_fP, "%d", &numcols);
            numprinted = -1;
        }
        else if (!strcmp(command,"$LINES"))
            fscanf(in_fP, "%d", &numlines);
        else if (!strcmp(command,"$LABELS"))
        {
            fscanf(in_fP, "%s", command);
            labeled = strcmp(command,"OFF");
        }
        else if (!strcmp(command,"$SKIP"))
        {
            fscanf(in_fP, "%d", &lines_to_skip);
            skip_lines(out_fP,lines_to_skip);
        }
        else if (!strcmp(command,"$TITLE"))
        {
            (VOID)fgets(title,LINELENGTH,in_fP);
            skip_lines(out_fP,numlines+3);
            fprintf(out_fP,"%s",title+1);
            numprinted = -1;
        }
    }
}
            
/*----------------------------- end of file -------------------------------*/

