#ident	"@(#)keyserv.mk	1.3"
#ident  "$Header$"

include $(CMDRULES)

#+++++++++++++++++++++++++++++++++++++++++++++++++++++++++
#	PROPRIETARY NOTICE (Combined)
#
# This source code is unpublished proprietary information
# constituting, or derived under license from AT&T's UNIX(r) System V.
# In addition, portions of such source code were derived from Berkeley
# 4.3 BSD under license from the Regents of the University of
# California.
#
#
#
#	Copyright Notice 
#
# Notice of copyright on this source code product does not indicate 
#  publication.
#
#       (c) 1986,1987,1988,1989,1990  Sun Microsystems, Inc                     
#       (c) 1983,1984,1985,1986,1987,1988,1989,1990  AT&T.                      
#       (c) 1990,1991,1992  UNIX System Laboratories, Inc.
#          All rights reserved.
#

#
# Sun RPC is a product of Sun Microsystems, Inc. and is provided for
# unrestricted use provided that this legend is included on all tape
# media and as a part of the software program in whole or part.  Users
# may copy or modify Sun RPC without charge, but are not authorized
# to license or distribute it to anyone else except as part of a product or
# program developed by the user.
#
# SUN RPC IS PROVIDED AS IS WITH NO WARRANTIES OF ANY KIND INCLUDING THE
# WARRANTIES OF DESIGN, MERCHANTIBILITY AND FITNESS FOR A PARTICULAR
# PURPOSE, OR ARISING FROM A COURSE OF DEALING, USAGE OR TRADE PRACTICE.
#
# Sun RPC is provided with no support and without any obligation on the
# part of Sun Microsystems, Inc. to assist in its use, correction,
# modification or enhancement.
#
# SUN MICROSYSTEMS, INC. SHALL HAVE NO LIABILITY WITH RESPECT TO THE
# INFRINGEMENT OF COPYRIGHTS, TRADE SECRETS OR ANY PATENTS BY SUN RPC
# OR ANY PART THEREOF.
#
# In no event will Sun Microsystems, Inc. be liable for any lost revenue
# or profits or other special, indirect and consequential damages, even if
# Sun has been advised of the possibility of such damages.
#
# Sun Microsystems, Inc.
# 2550 Garcia Avenue
# Mountain View, California  94043
#
#

#LOCALDEF = -DYP -DDEBUG -DSPINDEBUG
LOCALDEF = -DYP
DESTSBIN= $(USRSBIN)
DESTBIN = $(USRBIN)
LDLIBS	= -lrpcsvc -lnsl

SBINS	= keyserv newkey   
BINS	= keylogout keylogin domainname chkey 
KEYSERV_OBJS = setkey.o detach.o key_generic.o
LIBMPOBJS= pow.o gcd.o msqrt.o mdiv.o mout.o mult.o madd.o util.o
CHANGE_OBJS  = generic.o update.o
OBJS	= $(KEYSERV_OBJS) $(LIBMPOBJS) $(CHANGE_OBJS) $(SBINS:=.o) $(BINS:=.o)
SRCS	= $(OBJS:.o=.c)

all: $(BINS) $(SBINS)

keyserv: $(KEYSERV_OBJS) $(LIBMPOBJS) keyserv.o
	$(CC) $(CFLAGS) -o $@ $(KEYSERV_OBJS) $(LIBMPOBJS) keyserv.o $(LDFLAGS) $(LDLIBS) $(SHLIBS)

keylogout: keylogout.o 
	$(CC) $(CFLAGS) -o $@ $@.o $(LDFLAGS) $(LDLIBS) $(SHLIBS)

keylogin: keylogin.o
	$(CC) $(CFLAGS) -o $@ $@.o $(LDFLAGS) $(LDLIBS) $(SHLIBS)

chkey: $(CHANGE_OBJS) $(LIBMPOBJS) chkey.o
	$(CC) $(CFLAGS) -o $@ $(CHANGE_OBJS) $(LIBMPOBJS) chkey.o $(LDFLAGS) $(LDLIBS) $(SHLIBS)

newkey:$(CHANGE_OBJS) $(LIBMPOBJS) newkey.o
	$(CC) $(CFLAGS) -o $@ $(CHANGE_OBJS) $(LIBMPOBJS) newkey.o $(LDFLAGS) $(LDLIBS) $(SHLIBS)

generic:$(LIBMPOBJS) generic.o
	$(CC) $(CFLAGS) -o $@ $(LIBMPOBJS) generic.o $(LDFLAGS) $(LDLIBS) $(SHLIBS)

update:$(LIBMPOBJS) update.o
	$(CC) $(CFLAGS) -o $@ $(LIBMPOBJS) update.o $(LDFLAGS) $(LDLIBS) $(SHLIBS)

domainname: domainname.o
	$(CC) $(CFLAGS) -o $@ $@.o $(LDFLAGS) $(LDLIBS) $(SHLIBS)

chkey.o: chkey.c \
	$(INC)/stdio.h \
	$(INC)/stdlib.h \
	$(INC)/rpc/rpc.h \
	$(INC)/rpc/key_prot.h \
	$(INC)/rpcsvc/ypclnt.h \
	$(INC)/pwd.h \
	$(INC)/string.h \
	msg.h

detach.o: detach.c \
	$(INC)/sys/termios.h \
	$(INC)/fcntl.h $(INC)/sys/fcntl.h

domainname.o: domainname.c \
	$(INC)/stdio.h \
	$(INC)/errno.h \
	msg.h

gcd.o: gcd.c \
	mp.h

generic.o: generic.c \
	$(INC)/stdio.h \
	$(INC)/rpc/rpc.h \
	$(INC)/sys/file.h \
	mp.h \
	$(INC)/rpc/key_prot.h

init_tr.o: init_tr.c \
	$(INC)/stdio.h \
	$(INC)/string.h \
	$(INC)/sys/types.h \
	$(INC)/unistd.h $(INC)/sys/unistd.h \
	$(INC)/rpc/rpc.h \
	$(INC)/rpc/rpcb_prot.h \
	$(INC)/netconfig.h \
	$(INC)/netdir.h \
	$(INC)/sys/wait.h \
	$(INC)/sys/signal.h \
	$(INC)/sys/termios.h \
	$(INC)/sys/syslog.h \
	msg.h

key_generic.o: key_generic.c \
	$(INC)/stdio.h \
	$(INC)/string.h \
	$(INC)/rpc/rpc.h \
	$(INC)/errno.h $(INC)/sys/errno.h \
	$(INC)/sys/syslog.h \
	$(INC)/rpc/nettype.h \
	$(INC)/netconfig.h \
	$(INC)/netdir.h

keylogin.o: keylogin.c \
	$(INC)/stdio.h \
	$(INC)/rpc/rpc.h \
	$(INC)/rpc/key_prot.h \
	msg.h

keylogout.o: keylogout.c \
	$(INC)/stdio.h \
	$(INC)/rpc/rpc.h \
	$(INC)/rpc/key_prot.h \
	$(INC)/nfs/nfs.h \
	$(INC)/nfs/nfssys.h \
	msg.h

keyserv.o: keyserv.c \
	$(INC)/stdio.h \
	$(INC)/rpc/rpc.h \
	$(INC)/sys/param.h \
	$(INC)/sys/file.h \
	$(INC)/pwd.h \
	$(INC)/rpc/des_crypt.h \
	$(INC)/rpc/key_prot.h \
	msg.h

madd.o: madd.c \
	mp.h

mdiv.o: mdiv.c \
	mp.h \
	$(INC)/stdio.h

mout.o: mout.c \
	$(INC)/stdio.h \
	mp.h

msqrt.o: msqrt.c \
	mp.h

mult.o: mult.c \
	mp.h

newkey.o: newkey.c \
	$(INC)/stdio.h \
	$(INC)/stdlib.h \
	$(INC)/rpc/rpc.h \
	$(INC)/rpc/key_prot.h \
	$(INC)/rpcsvc/ypclnt.h \
	$(INC)/sys/wait.h \
	$(INC)/netdb.h \
	$(INC)/pwd.h \
	$(INC)/string.h \
	$(INC)/sys/resource.h \
	$(INC)/netconfig.h \
	$(INC)/netdir.h \
	msg.h

pow.o: pow.c \
	mp.h

setkey.o: setkey.c \
	$(INC)/stdio.h \
	$(INC)/stdlib.h \
	mp.h \
	$(INC)/rpc/rpc.h \
	$(INC)/rpc/key_prot.h \
	$(INC)/rpc/des_crypt.h \
	$(INC)/sys/errno.h

update.o: update.c \
	$(INC)/stdio.h \
	$(INC)/stdlib.h \
	$(INC)/rpc/rpc.h \
	$(INC)/rpc/key_prot.h \
	$(INC)/rpcsvc/ypclnt.h \
	$(INC)/sys/wait.h \
	$(INC)/netdb.h \
	$(INC)/pwd.h \
	$(INC)/string.h \
	$(INC)/sys/resource.h

util.o: util.c \
	$(INC)/stdio.h \
	$(INC)/stdlib.h \
	mp.h

key_prot.h : $(ROOT)/usr/include/rpcsvc/key_prot.x
	rpcgen -h $(ROOT)/usr/include/rpcsvc/key_prot.x > key_prot.h

lintit:
	$(LINT) $(LINTFLAGS) $(SRCS)

clean:
	$(RM) -f $(OBJS)

clobber: clean
	$(RM) -f $(SBINS) $(BINS)

install: $(BINS) $(SBINS)
	$(INS) -f $(DESTSBIN) -m 0555 -u root -g sys keyserv
	$(INS) -f $(DESTSBIN) -m 0555 -u root -g sys newkey
	$(INS) -f $(DESTBIN) -m 0555 -u bin -g bin chkey
	$(INS) -f $(DESTBIN) -m 0555 -u bin -g bin domainname
	$(INS) -f $(DESTBIN) -m 0555 -u bin -g bin keylogin
	$(INS) -f $(DESTBIN) -m 0555 -u bin -g bin keylogout
