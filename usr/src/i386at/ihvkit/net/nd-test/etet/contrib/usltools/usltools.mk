#ident "@(#)usltools/Makefile	1.3"
#
# Copyright 1993 UNIX System Laboratories (USL)
#
# Permission to use, copy, modify, and distribute this software and its
# documentation for any purpose and without fee is hereby granted, provided
# that the above copyright notice appear in all copies and that both that
# copyright notice and this permission notice appear in supporting
# documentation, and that the name of USL not be used in 
# advertising or publicity pertaining to distribution of the software 
# without specific, written prior permission.  USL make 
# no representations about the suitability of this software for any purpose.  
# It is provided "as is" without express or implied warranty.
#
# USL DISCLAIM ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, 
# INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO 
# EVENT SHALL USL BE LIABLE FOR ANY SPECIAL, INDIRECT OR 
# CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF 
# USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR 
# OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR 
# PERFORMANCE OF THIS SOFTWARE.
#
#include $(CMDRULES)

LIBDIR	= $(TET_ROOT)/lib/posix_c
INCDIR	= $(TET_ROOT)/inc/posix_c
CC	= cc
CFLAGS	= -I$(INCDIR)
# Install the tools in $TET_ROOT/bin
BIN=../../bin

all:	buildtool

buildtool:	buildtool.c $(INCDIR)/tet_api.h
	$(CC) $(CFLAGS) -o buildtool buildtool.c $(LIBDIR)/tcm.o $(LIBDIR)/libapi.a

$(BIN)/buildtool: buildtool
	cp buildtool $@

$(BIN)/cleantool: $(BIN)/buildtool
	ln $(BIN)/buildtool $@

$(BIN)/smake: smake
	cp smake $@
	chmod +x $@

$(BIN)/sclean: $(BIN)/smake
	ln $(BIN)/smake $@

$(BIN)/vres: vres
	cp vres $@
	chmod +x $@

install:  $(BIN)/buildtool $(BIN)/smake $(BIN)/cleantool $(BIN)/sclean $(BIN)/vres

clean:	
	rm -f *.o buildtool

clobber:	
	rm -f buildtool buildtool.o $(BIN)/buildtool $(BIN)/cleantool $(BIN)/smake $(BIN)/sclean $(BIN)/vres

CLEAN:	clean

CLOBBER:	clobber
