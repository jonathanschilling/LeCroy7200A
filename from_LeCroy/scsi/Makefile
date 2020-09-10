all!	acquire.exe lint.out tags
	-

acquire.exe! acquire.obj acq_dos.obj scsi.obj aha1540b.obj ahaprint.obj \
	intsubs.obj
	link /CO /ST:30000 acquire+acq_dos+scsi+aha1540b+ahaprint+intsubs,,nul,;

acquire.obj! acquire.c
	cl /Gs /c /AH -UDEBUG -Ic:\include acquire.c

acq_dos.obj! acq_dos.c aha1540b.h
	cl /Gs /c /AH -UDEBUG -Ic:\include acq_dos.c

scsi.obj! scsi.c
	cl /Gs /c /AH -DMSDOS -Ic:\include scsi.c

aha1540b.obj! aha1540b.c
	cl /Gs /c /AH -DMSDOS -Ic:\include aha1540b.c

ahaprint.obj! ahaprint.c
	cl /Gs /c /AH -UDEBUG -DMSDOS -Ic:\include ahaprint.c

intsubs.obj!	intsubs.asm
	masm /Ml /t intsubs.asm,,nul,nul;

lint.out!	acquire.c acq_dos.c scsi.c aha1540b.c ahaprint.c aha1540b.h
	lint -u acquire.c acq_dos.c scsi.c aha1540b.c ahaprint.c >lint.out

tags!	acquire.c acq_dos.c scsi.c aha1540b.c ahaprint.c
	ctags acquire.c acq_dos.c scsi.c aha1540b.c ahaprint.c

acquire.lzh!	acq_dos.c acquire.c aha1540b.c ahaprint.c aha1540b.h scsi.c \
	makefile intsubs.asm
	lharc a acquire.lzh acq_dos.c acquire.c aha1540b.c ahaprint.c \
	aha1540b.h scsi.c makefile intsubs.asm

clean!	tags
	-rm -f *.obj *.exe tags lint.out

