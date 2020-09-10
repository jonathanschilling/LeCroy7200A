cl /Od /Zi -DATM_PROBE -DMSDOS -Ic:\include -I. /c /AL  seq_tran.c
cl /Od /Zi -DATM_PROBE -DMSDOS -Ic:\include -I. /c /AL  seq_args.c
cl /Od /Zi -DATM_PROBE -DMSDOS -Ic:\include -I. /c /AL  seq_filt.c
cl /Od /Zi -DATM_PROBE -DMSDOS -Ic:\include -I. /c /AL  seq_mtg.c
cl /Od /Zi -DATM_PROBE -DMSDOS -Ic:\include -I. /c /AL  seq_prt.c
cl /Od /Zi -DATM_PROBE -DMSDOS -Ic:\include -I. /c /AL  seq_util.c
cl /Od /Zi -DATM_PROBE -DMSDOS -Ic:\include -I. /c /AL  seq_wfd.c
link /CO seq_tran.obj seq_args.obj seq_filt.obj seq_mtg.obj seq_prt.obj seq_util.obj seq_wfd.obj, seq_tran,, /ST:38000
exepack seq_tran.exe seqtran.exe

