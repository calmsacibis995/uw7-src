#ident	"@(#)cplusinc:makefile	1.7"

# This is an install-only makefile.  Its lack of shortcuts ensures
# that only the files which have changed get installed.

include $(CMDRULES)

CCINC =$(USRINC)/CC
CCINCRPC =$(USRINC)/CC/rpc
CCINCSVC = $(USRINC)/CC/rpcsvc

TARGETS = $(CCINC)/common.h $(CCINC)/complex.h $(CCINC)/fstream.h \
	$(CCINC)/generic.h $(CCINC)/iomanip.h $(CCINC)/iostream.h \
	$(CCINC)/stdiostream.h $(CCINC)/stream.h \
	$(CCINC)/strstream.h $(CCINC)/vector.h \
	$(CCINCRPC)/auth.h $(CCINCRPC)/auth_sys.h $(CCINCRPC)/clnt.h \
	$(CCINCRPC)/clnt_soc.h $(CCINCRPC)/key_prot.h \
	$(CCINCRPC)/pmap_prot.h $(CCINCRPC)/rpc_msg.h \
	$(CCINCRPC)/rpcb_clnt.h $(CCINCRPC)/rpcb_prot.h \
	$(CCINCRPC)/svc.h $(CCINCRPC)/xdr.h \
	$(CCINCSVC)/dbm.h $(CCINCSVC)/spray.h $(CCINCSVC)/ypclnt.h

all:

.MUTEX: $(CCINC) $(CCINCRPC) $(CCINCSVC) headers 

install: $(CCINC) $(CCINCRPC) $(CCINCSVC) headers

headers:	$(TARGETS)

clean:

clobber:
	$(RM) -f $(TARGETS)

$(CCINC):
	mkdir $(@)

$(CCINCRPC):
	mkdir $(@)

$(CCINCSVC):
	mkdir $(@)

$(CCINC)/common.h : common.h
	$(CP) $(?) $(@)

$(CCINC)/complex.h : complex.h
	$(CP) $(?) $(@)

$(CCINC)/fstream.h : fstream.h
	$(CP) $(?) $(@)

$(CCINC)/generic.h : generic.h
	$(CP) $(?) $(@)

$(CCINC)/iomanip.h : iomanip.h
	$(CP) $(?) $(@)

$(CCINC)/iostream.h : iostream.h
	$(CP) $(?) $(@)

$(CCINC)/stdiostream.h : stdiostream.h
	$(CP) $(?) $(@)

$(CCINC)/stream.h : stream.h
	$(CP) $(?) $(@)

$(CCINC)/strstream.h : strstream.h
	$(CP) $(?) $(@)

$(CCINC)/vector.h : vector.h
	$(CP) $(?) $(@)


$(CCINCRPC)/auth.h : rpc/auth.h
	$(CP) $(?) $(@)

$(CCINCRPC)/auth_sys.h : rpc/auth_sys.h
	$(CP) $(?) $(@)

$(CCINCRPC)/clnt.h : rpc/clnt.h
	$(CP) $(?) $(@)

$(CCINCRPC)/clnt_soc.h : rpc/clnt_soc.h
	$(CP) $(?) $(@)

$(CCINCRPC)/key_prot.h : rpc/key_prot.h
	$(CP) $(?) $(@)

$(CCINCRPC)/pmap_prot.h : rpc/pmap_prot.h
	$(CP) $(?) $(@)

$(CCINCRPC)/rpc_msg.h : rpc/rpc_msg.h
	$(CP) $(?) $(@)

$(CCINCRPC)/rpcb_clnt.h : rpc/rpcb_clnt.h
	$(CP) $(?) $(@)

$(CCINCRPC)/rpcb_prot.h : rpc/rpcb_prot.h
	$(CP) $(?) $(@)

$(CCINCRPC)/svc.h : rpc/svc.h
	$(CP) $(?) $(@)

$(CCINCRPC)/xdr.h : rpc/xdr.h
	$(CP) $(?) $(@)

$(CCINCSVC)/dbm.h : rpcsvc/dbm.h
	$(CP) $(?) $(@)

$(CCINCSVC)/spray.h : rpcsvc/spray.h
	$(CP) $(?) $(@)

$(CCINCSVC)/ypclnt.h : rpcsvc/ypclnt.h
	$(CP) $(?) $(@)
