	cl /Gs /c /AH -UDEBUG -Ic:\include acquire.c
	cl /Gs /c /AH -UDEBUG -Ic:\include acq_dos.c
	cl /Gs /c /AH -DMSDOS -Ic:\include scsi.c
	cl /Gs /c /AH -DMSDOS -Ic:\include aha1540b.c
	cl /Gs /c /AH -UDEBUG -DMSDOS -Ic:\include ahaprint.c
	masm /Ml /t intsubs.asm,,nul,nul;
	link /CO /ST:30000 acquire+acq_dos+scsi+aha1540b+ahaprint+intsubs,,nul,;

