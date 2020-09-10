#/*------------------------- SEQ_TRAN.MAK -------------------------------------
#
# This is the POLYMAKE makefile for generating the code to transfer uncorrected
# and protected data from the 7200A, filter it in the PC, then send it back
# to the 7200A as a complete waveform file.
#
#---------------------------------------------------------------------------*/
#   MAKEFILE FOR SANDIA NATIONAL LABORATORIES

.SUFFIXES  #turns off ALL polymake defaults!

REV =
GET = get
COPY = copy
STDPATH = c:\include
INCLPATH = .


CVFLAGS=/Od /Zi 
CFLAGS=-DATM_PROBE -DMSDOS -I$(STDPATH) -I$(INCLPATH) /c /AL

#CL=cl $(CFLAGS)
CL=cl $(CVFLAGS) $(CFLAGS)

#LINK=link
LINK=link /CO

#directory declarations
.PATH.h = .
.PATH.c = .


#archive declarations
.LOGFILE .h_v(.h $(REV))
.LOGFILE .c_v(.c $(REV))


#transformation rules

.c.obj:  # make a .obj file from a .c file
		$(CL)  $<

.asm.obj: #make a .obj file from a .asm file
	masm /Ml /t $< ,,nul,nul;

# Specify in one place all source modules
#
CSOURCES =\
		seq_tran.c\
		seq_args.c\
		seq_filt.c\
		seq_mtg.c\
		seq_prt.c\
		seq_util.c\
		seq_wfd.c

SOURCES = $(CSOURCES)

# Specify in one place all object modules
#
OBJS = $[f,,$(SOURCES),obj]

# The rules for linking
#
seq_tran.exe : $(OBJS)
	
		$(LINK) <@<
$[s,"+\n",$(OBJS)],
seq_tran,, /ST:38000
<
    exepack seq_tran.exe seqtran.exe


# The source file dependencies
#
seq_filt.obj  :  seq_filt.h seq_tran.h

seq_wfd.obj   :  seq_hdr.h

seq_mtg.obj   :  seq_hdr.h

seq_prt.obj   :  seq_hdr.h

seq_util.obj  :  seq_tran.h seq_hdr.h

seq_tran.obj  :  seq_filt.h seq_hdr.h seq_tran.h

seq_args.obj  :  seq_tran.h

