/*OOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOO

        ------------- aagen.h -------------

 -------------------------------------------------------------------------------

   NOTE: SAME INCLUDE FILE AS AAGEN.H EXCEPT THAT IT DOES NOT INCLUDE AAMSDOS.H
	 AND ALL OF ITS INCLUDE FILES.

   Type definitions, general structures, and some macros of general utility. 
   There should be no declaration specific to a particular instrument here.

   The use of these definitions instead of the explicit ones
   in "C" programs aids in portability to machines with different sizes.
     
 -------------------------------------------------------------------------------

    Written by: Jean-Francois Oldani & Allan Rothenberg, 
                 LeCroy SA, Geneva, Switzerland

    Started:        October 14, 1986

    Last Revised: 88/04/12 18:21:11   (SCCS Version 1.45)


OOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOO
Latest changes:
    Version   1.45   by  allan_r Tue Apr 12 18:19:01 ETE 1988  
add ATOM mods of April 12 shareware
remove erroneous USIMODULO
    Version   1.44   by  allan_r Fri Feb 5 18:10:59 HIV 1988  
add  UCOPYOVL USIMODULO UFEQUAL 
remove USMODULO which was too complicated and never used
    Version   1.42  
modify UCLIP to yield a more uniform and hopefully shorter macro expansion.  WD
    Version   1.41  
add UROUND, modify VOID treatment for MS-DOS, 
modify UNORET to expand for lint only.
    Version   1.40  
add UISBOOL UINRANGE macros and STRARRAY type
    Version   1.39  
add USTRUCTEQ and UFUZZY macros
    Version   1.38  
(JFO) redo some macros (AFR) final edit and install
(PL)  add MSDOS compatibility.        
    Pre Version   1.37  
(JFO) add U_SEXP, U_DEXP, U_SIxxx(), U_DISxxx() macros.
Name change to aagen and separtion of DSO only facilities into aadso.h
Introduction of aatom.h
(JFO) remove ';' at the end of the macro UNORET().
(AFR) make VOID short for APOLLO to trap undeclared functions
(AFR)  make VOID void for DSO
(JFO) Modify UINIT() macro. 
(AFR) implement dso versions of DB macros, add SCID macro , modify UINIT macro
add CONST and VOLATILE qualifiers
add macro UMINABS3()
(JFO) add macro UCLOSER() 
add UFAR()
Define UNORET Macro.
Add USTRINC() macro.   
Add UCOPY() macro.
Change names of apollo debug print macros
Add DEBUG PRINT Macros for APOLLO environment.  
Conditional definition of NULL for use "aagen.h" with <stdio.h>  
Add BUG, WARN (RF).                                                       
Add STATUS (RF).
Minor format corrections
Insert new Macro UHEXBIN()
Insert new Macro : UBINHEX() 
Correction in macro UINIT()
Install macro for memory initialisation.
First version derived from gen9400.h with minor changes.    Version  1.0 
OOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOO*/
#ifndef AAGEN_ID 

#ifdef DSO

#ifdef APOLLO
#define SCID(a,b) static  char a[] = { b } ;
#else
#define SCID(a,b) static const char a[] = { b } ;
#endif

#else

#define SCID(a,b) 

#endif

      SCID( AAGEN_id, "@(#)aagen.h:1.45")


#define AAGEN_ID        



#define ON              1                           
#define OFF             0
#define TRUE            1
#define FALSE           0 
#ifndef NULL   
#define NULL            0 
#endif                  
#define EOS             '\0'
                              


/*XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
primitive LeCroy typedef                
    standard typedef's for LeCroy DSO software
 --------------------------------------------------------------------------
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX*/
 


typedef unsigned char  UBYTE;    /* 8 bits unsigned */
typedef unsigned char  UCHAR;    /* unsigned character unit */
typedef unsigned short UWORD;    /* 16 bits unsigned */
typedef unsigned long  ULONG;    /* 32 bits unsigned */
typedef unsigned long  UINT;     /* 32 bits unsinged */     
                          

typedef char  BYTE ;             /* 8 bits signed */
typedef char  CHAR;              /* signed character unit */
typedef char  FLAG;              /* flag */
typedef short WORD;              /* 16 bits signed */
typedef long  LONG;              /* 32 bits signed */
typedef int   INT;               /* natural size of machine, e.g.
                                      32 in 68K, 16 in PDP-11 */ 

typedef CHAR BOOL ;                    

#ifdef MSDOS
#define VOID void
#else /* MSDOS */
#ifdef VAX
#define VOID void
#else /* VAX */
#ifdef APOLLO
typedef short VOID;            /* type of functions which return nothing 
                                   and used for pointers whose type
                                   is not uniquely defined */ 

#else

#ifdef MAKE_PROTO
#define VOID void
#else
typedef void VOID ;
#endif

#endif
#endif /* VAX */
#endif /* MSDOS */

typedef float  FLOAT;            /* 32-bit floating point number */
typedef double DOUBLE;           /* 64-bit floating point number */ 

typedef INT    (*PFI)();         /* pointer to function returning an integer */
typedef UINT   (*PFUI)();        /* pointer to function ret an unsigned Integer */
typedef FLOAT  (*PFF)();         /* pointer to a function ret. a float     */
typedef VOID   (*PFV)();         /* pointer to function returning nothing  */
typedef BOOL   (*PFB)();         /* pointer to a function ret a BOOL       */
typedef CHAR   (*PFC)();         /* pointer to a funtion ret a CHAR        */
typedef UCHAR  (*PFUC)();        /* pointer to a function ret a UCHAR      */
typedef DOUBLE (*PFD)();         /* pointer to a function ret a DOUBLE     */
typedef WORD   (*PFW)();         /* pointer to a function ret a WORD       */
typedef UWORD  (*PFUW)();        /* pointer to a function ret a UWORD      */

typedef VOID  *((*PFVP)());      /* pointer to a function ret a VOID ptr   */
typedef FLOAT *((*PFFP)());      /* pointer to a function ret a FLOAT ptr  */
typedef INT   *((*PFIP)());      /* pointer to a function ret a INT ptr    */
typedef UINT  *((*PFUIP)());     /* pointer to a function ret a UINT ptr   */
typedef WORD  *((*PFWP)());      /* pointer to a function ret a WORD ptr   */
typedef UWORD *((*PFUWP)());     /* pointer to a function ret a UWORD ptr  */
typedef CHAR  *((*PFCP)());      /* pointer to a function ret a CHAR ptr   */
typedef UCHAR *((*PFUCP)());     /* pointer to a function ret a UCHAR ptr  */
typedef BOOL  *((*PFBP)());      /* pointer to a function ret a BOOL ptr   */
typedef DOUBLE *((*PFDP)());     /* pointer to a function ret a DOUBLE ptr */

/* CONST and VOLATILE are defined here, but will be undefined in aatom.h */
/* If FRAME ID is 7200A                                                  */
#define CONST const
#define VOLATILE volatile

#ifdef ATOM
#ifdef APOLLO
#undef CONST   
#undef VOLATILE
#define CONST   
#define VOLATILE
#endif /* APOLLO */

#endif /* ATOM */


typedef struct                  /* structure to hold floating complex numb */
       {
        FLOAT  real ;
        FLOAT  imag ;
       } FCOMPLEX ;

                   
typedef struct                  /* structure to hold word complex numbers */
       {
         WORD  real ;
         WORD  imag ;
       } WCOMPLEX ;

typedef struct  
       {                        /* double prec. signed integer = 64-bit int */
        LONG  hi;
        ULONG lo;
       } VLONG ;

typedef struct  
       {                        /* double prec. unsigned integer = 64-bit int */
        ULONG  hi;
        ULONG lo;
       } VULONG ;

typedef struct
{
    CHAR *String;
} STRARRAY;                     /* used for getting arrays of pointers to
                                   strings in the region const             */


#define PUTUBYTE  *(UBYTE *)    /* direct memory access by explicit */
#define PUTUWORD  *(UWORD *)    /* length only */
#define PUTULONG  *(ULONG *)
#define PUTLONG  *(LONG *)
#define GETUBYTE  *(UBYTE *)
#define GETUWORD  *(UWORD *)
#define GETULONG  *(ULONG *)
#define GETLONG  *(LONG *)
#define GETWORD   *(WORD  *)
#define GETFLOAT  *(FLOAT *)    /* for example :  
                                
                                    LONG    one = 0x3F800000 ( IEEE_ONE)
                                    FLOAT   f ;

                                    f = (FLOAT)(IEEE_ONE) will perform a
                                        conversion and f will be the floater
                                        equivalent of the integer "one".
                                        But, f = GETFLOAT(&one) has the 
                                        value 1.0 */
 
                                 
/* status variables and functions returning status shall have this type: */

typedef WORD STATUS;            /* possible values:                 */
#define STATUS_OK   0           /* status == 0 : success            */   
                                /* status >  0 : success, specific  */   
#define STATUS_ERR -1           /* status = -1 : error              */
                                /* status < -1 : error, specific    */

                                                    
                                                  
/*XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
general constants definition            
    standard typedef's for LeCroy DSO software
 --------------------------------------------------------------------------
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX*/
                                                                        
#define LONG_ONE        0x00000001L     /* +1 in LONG format */
#define NEG_LONG_ONE    0xFFFFFFFFL     /* -1 in LONG format */ 
#define IEEE_ONE        0x3F800000L     /* IEEE format for floating +1 */
/* single and double precision overflows. In such case, we have :
   max exponent : 0xFF for single,  0x7FF for double.
   mantissa     : 0
   !! max exponent can occur also for NAN cases, but mantissa not 0 !! */
#define IEEE_SOVF       0x7F800000L                       
#define IEEE_DOVF       0x7FF00000L                           
#define IEEE_SUNF       0xFF800000L                       
#define IEEE_DUNF       0xFFF00000L                           




/*XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
    Macro definitions                   
    Basic LeCroy Utility Macros
 --------------------------------------------------------------------------
XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX*/

      
#define   UABS(a)               ((a) < 0 ? -(a) : (a))

#define   UMIN2(a,b)            ((a) < (b) ? (a) : (b))

#define   UMAX2(a,b)            ((a) > (b) ? (a) : (b))

#define   UMIN3(a,b,c)          (UMIN2(a,UMIN2(b,c)))

#define   UMAX3(a,b,c)          (UMAX2(a,UMAX2(b,c)))

#define   UMAXABS(a,b)          (UMAX2(UABS(a), UABS(b)))

#define   UMINABS(a,b)          (UMIN2(UABS(a), UABS(b)))    

#define   UMINABS3(a,b,c)       (UMIN3(UABS(a),UABS(b),UABS(c)))

                                /* clip b to limits a, c */
#define   UCLIP(a, b, c)        ( (b) < (a) ? (a) : ( (b) > (c) ? (c) : (b)))

 
#define   UMODULE(a,b)          ((a)*(a) + (b)*(b))
                                                       
                                 /* perform a left arith shift if by >= 0
                                    and a right arith shift if by < 0
                                 */                                 

#define   USHIFT(a,by)          ( (by) >= 0 ? (a) << (by) : (a) >> -(by) )

                                           /* set the bit "n" of "a" */     
#define   USETBIT(n,a)    { GETULONG(&a) = \
                            ((GETULONG(&a)) | (ULONG)(1 << (INT)UABS(n)) ) ; }
                                 
                                           /* clear the bit "n" of "a" */
#define   UCLRBIT(n,a)    { GETULONG(&a) = \
                           (GETULONG(&a)) &  (ULONG)( ~(1 << (INT)UABS(n)) ) ; }

                                
                                          /* get the bit  "n" of "a" */
#define   UGETBIT(n,a)   (((GETULONG(&a)) & (ULONG)(1 << (INT)UABS(n)) ) != 0 ) 

                                
#define   USTREQ(a,b)           (! strcmp(a, b)) 
 
#define   UFEQUAL(val, ref, eps)  ((val > ref - eps ) && (val < ref + eps))                                                 
      
/* Initialise memory starting at the address "Ustart" end ending at 
    "Ustart + Un " with a value "Uval" casted in the type "Utype".
*/

#define UINIT(Ustart, Un, Uval, Utype) { \
                  register LONG Uinit_cnt = (LONG)(Un) ; \
                  register Utype *Utmp = (Utype *)Ustart ; \
                  do{ *Utmp++ = (Utype)(Uval) ; }while( --Uinit_cnt ) ; }
       
 
/* Copy 'Un' values of type 'Utype', from 'UsP' source pointer 
    to 'UdP' destination pointer.  !!Dont take care of overlaps. 

   ex :   
        typedef struct
            {
                INT a ;
                CHAR b ;
            }  VSTRUC ;
        ......

        VSTRUC v1[3], v[3] = { {1, 'z'},
                       {2,'y'},
                       {3,'x'}} ; 
        ......

        UCOPY(VSTRUC,v,v1,3) ; 

*/

#define     UCOPY(Utype,UsP, UdP, Un) { register ULONG  Ui = (LONG)Un ;\
                                        register Utype *UssP = (Utype *)UsP ;\
                                        register Utype *UddP = (Utype *)UdP;\
                                        do{ *UddP++ = *UssP++; }while(--Ui);}

/* copy possibly overlapping ranges */

#define UCOPYOVL( Utype, UsP, UdP, Un)                  \
        {   register ULONG  Ui = ( LONG)Un ;            \
            register Utype *UssP;                       \
            register Utype *UddP;                       \
            if ( UdP <= UsP)                            \
            {   UssP = ( Utype*)UsP;                    \
                UddP = ( Utype*)UdP;                    \
                do{ *UddP++ = *UssP++; }while( --Ui); } \
            else                                        \
            {   UssP = ( Utype*)UsP + Ui;               \
                UddP = ( Utype*)UdP + Ui;               \
                do { *( --UddP) = *( --UssP); } while( --Ui); }}



/*  Check for the inclusion of the string pointed by 'UsP' in the string 'UtP'.
    Return :
             Un = 0 if no inclusion.
             Un = length of the string 'UsP' if inclusion of 'UsP' in 'UtP'.
*/

#define USTRINC(UsP, UtP, Un) { CHAR *Utmp = UtP ; \
                                Un = 0 ; \
                                while(*Utmp != '\0') \
                                { \
                                    while((*(Utmp++) == *(UsP +Un)) && \
                                                 (*(UsP+ Un) != '\0') ) \
                                    {\
                                        Un++; \
                                    }\
                                    if ( *(UsP+Un) == '\0' ) break ;\
                                    else Un = 0 ; \
                                } \
                              }  

                                                                      
/* UNORET() Macro is usefull for calling function which have a return value,
 when it would to be ignored. */
#ifdef lint
#define     UNORET(Ufunc_call)      if ((Ufunc_call))        
#else
#ifdef MSDOS
#define     UNORET(Ufunc_call)      (VOID)(Ufunc_call)        
#else
#define     UNORET(Ufunc_call)      (Ufunc_call)        
#endif
#endif

/* UROUND for rounding a float or double to nearest integer */
#define UROUND( fl) ( ( LONG)( ( fl) < 0.0 ? ( ( fl) - 0.5) : ( ( fl) + 0.5))) 

/* UFAR to test Return 0 if Uc is far from Ua than Ub and 1 else */
#define   UFAR(Ua, Ub, Uc)      ( ((2*(Ua))-(Ub)-(Uc)) > 0  ? 1 : 0 )
                                                          
                                                        

/* UCLOSER to choose between Ua and Uc the closest point to Ub and return it

   ex :
     UCLOSER(-5,-1,5)    =   -5
     UCLOSER(-5,1,5)     =    5
     UCLOSER(-5,-10,5)   =   -5
     UCLOSER(-5,10,5)    =    5
     UCLOSER(5,1,6)      =    5
     UCLOSER(1,4,10)     =    1
*/

#define UCLOSER(Ua,Ub,Uc) ( ((Ua) < (Uc) && (2*(Ub)-(Ua)-(Uc) < 0)) || \
                    ((Ua) >= (Uc) && (2*(Ub)-(Ua)-(Uc) > 0)) ? (Ua) : (Uc) )

  
/* U_SEXP() and U_DEXP() give respectively the exponent of a single precision 
   floating point number in IEEE format */
            
#define U_SEXP(fltP) (((*(INT *)(fltP)) & IEEE_SOVF) >> 23) 

#define U_DEXP(dblP) (((*(INT *)(dblP)) & IEEE_DOVF) >> 20) 

/* U_SISNAN() for single precision and U_DISNAN() for double precision
   floating point numbers return 1 if 'NAN' exponent detected, 0 else. */

#define U_SISNAN(fltP) (((*(INT *)(fltP)) & IEEE_SOVF) == IEEE_SOVF)

#define U_DISNAN(dblP) (((*(INT *)(dblP)) & IEEE_DOVF) == IEEE_DOVF)

/* Single precision, double precision over and underflow */

#define U_SISOVF(fltP)  ( *(INT *)(fltP) == IEEE_SOVF )
#define U_SISUNF(fltP)  ( *(INT *)(fltP) == IEEE_SUNF )

#define U_DISOVF(dblP)  ( *(INT *)(dblP) == IEEE_DOVF &&\
                          *((INT *)(dblP) + 1) == 0 ) 

#define U_DISUNF(dblP)  ( *(INT *)(dblP) == IEEE_DUNF &&\
                          *((INT *)(dblP) + 1) == 0 ) 



/* UFUZZYCOMPARE(val, ref, eps) 
 returns [UFUZZYGREATER UFUZZYEQUAL UFUZZYSMALLER] as appropriate
Example:   UFUZZYCOMPARE(x, 5.0, 0.001);                           */

#define UFUZZYGREATER  1
#define UFUZZYEQUAL    0
#define UFUZZYSMALLER -1

#define UFUZZYCOMPARE(val, ref, eps)                            \
       ( ((val) > (ref) + (eps)) ? UFUZZYGREATER :              \
         ((val) < (ref) - (eps)) ? UFUZZYSMALLER : UFUZZYEQUAL) 

            

/* USTRUCTEQ(st1, st2, equal)   
 Given MY_STRUCT st1, st2; BOOL equal; implements equal = (st1 == st2).
 ------------------------------------------------------------------------------ 
Example:  USTRUCTEQ(st1, st2, equal); if (equal) doIt();                */

#define USTRUCTEQ(st1, st2, equal)                      \
    {   INT size1 = sizeof(st1);                        \
        INT size2 = sizeof(st2);                        \
        BYTE *st1P = (BYTE*)&st1;                       \
        BYTE *st2P = (BYTE*)&st2;                       \
                                                        \
        if (! (equal = (size1 == size2)))               \
            ;                                           \
        else                                            \
            for (; size1; size1--)                      \
                if (! (equal = (*st1P++ == *st2P++)))   \
                    break;                              \
    }

#define UINRANGE(min, x, max)   ( ((x) >= (min)) && ((x) <= (max)) )

#define UISBOOL(x)              ( (x)==TRUE || (x)== FALSE)


#ifdef DSO
#include "aadso.h"
#endif

#ifdef ATOM
#include "aatom.h"
#endif 

#ifdef VAX
#include "aamsdos.h"
#endif 


#include <stdio.h>


#endif             /* AAGEN_ID */

