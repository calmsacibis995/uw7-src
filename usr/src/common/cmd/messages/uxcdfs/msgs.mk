#	Copyright (c) 1991, 1992  Intel Corporation
#	All Rights Reserved

#	INTEL CORPORATION CONFIDENTIAL INFORMATION

#	This software is supplied to USL under the terms of a license
#	agreement with Intel Corporation and may not be copied nor
#	disclosed except in accordance with the terms of that agreement.


#ident	"@(#)messages:common/cmd/messages/uxcdfs/msgs.mk	1.4"

include $(CMDRULES)


all	: msgs

install: all 
	cp msgs cdfs.str
	[ -d $(USRLIB)/locale/C/MSGFILES ] || \
		mkdir -p $(USRLIB)/locale/C/MSGFILES
	$(INS) -f $(USRLIB)/locale/C/MSGFILES -m 644 cdfs.str

lintit : 

clean :
	rm -f cdfs.str

clobber : clean

