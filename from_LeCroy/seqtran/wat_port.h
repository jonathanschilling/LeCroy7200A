/*--------------------------- wat_port.h ---------------------------*/
/*
Author:  Anthony Cake, subset of svs_port.h written by Joe Schachner

	Watcom C compiler is Microsoft C6 compatible.  
	This declares items used by Scout from Borland C/C++ 3.1 
	that are not supplied with Watcom, which we emulate in 
	wat_port.c.
$Header:   F:/asg/code/seqtran/wat_port.h_v   1.1   17 Dec 1993 12:47:56   LARRY_S  $

------------------------------------------------------------------------*/

#ifdef __WATCOMC__

#define disable _disable
#define enable  _enable

/* I/O Port aliases */

#define inportb(port)             (int) inp((unsigned) port)
#define outportb(port,value)      (void) outp((unsigned) port, (int) value)
#define inport(port)              (int) inpw((unsigned) port)
#define outport(port,value)       (void) outpw((unsigned) port, (int) value)

/* Memory allocation aliases */

#define halloc(nunits, unitsz)   calloc((size_t) nunits, (size_t) unitsz)
#define hfree(pointer)            free(pointer)
#define farcalloc(nunits, unitsz) calloc((size_t) nunits, (size_t) unitsz)
#define farfree(pointer)          free(pointer)
#define farmalloc(size)           malloc((size_t) size)

#endif /* __WATCOMC__ */

/* --------------------------- end of file ---------------------------- */

