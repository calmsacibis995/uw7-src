#ident	"@(#)kern-i386at:io/hba/adsa/him_code/him_code.mk	1.2"

LD=ld -r
CC=cc
CFLAGS=-O -DUSL -D_LTYPES -DSYSV -DSVR40 -D_SYSTEMENV=4 -D_KERNEL -Di386 -Di386a -DAT386 -DVPIX  

all:		him_drv.o copy

him.o:		him.c him_equ.h him_rel.h him_scb.h seq_off.h sequence.h seqmac.h seq_00.h seq_01.h seq_02.h vulture.h 
		$(CC) $(CFLAGS) him.c -c him.o

him_init.o:	him_init.c him_equ.h him_rel.h him_scb.h seq_off.h sequence.h seqmac.h seq_00.h seq_01.h seq_02.h vulture.h 
		$(CC) $(CFLAGS) him_init.c -c him_init.o

him_drv.o:	him.o him_init.o
		ld -r him.o him_init.o -o him_drv.o

copy:		him_drv.o
		cp him_drv.o ../him_drv.o
