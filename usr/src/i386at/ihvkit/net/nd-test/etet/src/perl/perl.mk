#  Makefile for ETET Perl API
#  Copyright 1992 SunSoft
# 
#  Permission to use, copy, modify, and distribute this software and its
#  documentation for any purpose and without fee is hereby granted, provided
#  that the above copyright notice appear in all copies and that both that
#  copyright notice and this permission notice appear in supporting
#  documentation, and that the name of SunSoft not be used in 
#  advertising or publicity pertaining to distribution of the software 
#  without specific, written prior permission.  SunSoft make 
#  no representations about the suitability of this software for any purpose.  
#  It is provided "as is" without express or implied warranty.
# 
#  SunSoft DISCLAIM ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, 
#  INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO 
#  EVENT SHALL OSF, UI or X/Open BE LIABLE FOR ANY SPECIAL, INDIRECT OR 
#  CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF 
#  USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR 
#  OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR 
#  PERFORMANCE OF THIS SOFTWARE.
# 
##########################################################################
#include $(CMDRULES)
TET_ROOT=../..
PERL_DIR=$(TET_ROOT)/lib/perl


all: api.pl tcm.pl README
	if [ ! -d $(PERL_DIR) ] ; then mkdir $(PERL_DIR); chmod 755 $(PERL_DIR); fi
	cp README tcm.pl api.pl $(PERL_DIR)
	chmod a-wx,u+rw,a+r $(PERL_DIR)/*
	


tcm.pl: template.pl sig make_tcm.pl
	chmod +x sig
	perl make_tcm.pl

clean:	CLEAN

CLEAN:
	rm  -f tcm.pl

CLOBBER:	CLEAN
	rm -f $(PERL_DIR)/README
	rm -f $(PERL_DIR)/tcm.pl
	rm -f $(PERL_DIR)/api.pl

