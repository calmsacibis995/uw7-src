#ident "@(#)diva.mk	19.1"
#make all components

include $(UTSRULES)

MAKEF = $(MAKE) -f
MAKEFILE = diva.mk
# DEBUG = -DDEBUG
# BUILD_DIR=/usr/src/uts/io/nd/mdi/diva
# BUILD_DIR=/user/cmcgover/work/geminicode/sampletree/usr/src/uts/io/nd/mdi_wan/diva
ETDC = make_EtdC
ETDD = make_EtdD
ETDK = make_EtdK
ETDM = make_EtdM
5ESS = make_5ess
ATEL = make_atel
ETSI = make_etsi
JAPAN = make_japan
NI1 = make_ni1
IDI = make_lib.idi
KLOG = make_klog
PROT = make_drv.diprot
Q931 = make_drv.diq931
DITO = make_ditools

DRV_CAPI = drv.capi20.mk
DRV_DIVA = drv.diva.mk
DRV_KLOG = drv.klog.mk
DRV_DIMAINT = drv.dimaint.mk
DRV_PROT = drv.diprot.mk
DRV_NI1 = drv.dini1.mk
DRV_JAPAN = drv.dijapan.mk
DRV_5ESS = drv.di5ess.mk
DRV_ATEL = drv.diatel.mk
DRV_ETSI = drv.dietsi.mk
KLOG_MK = klog.mk
DITOOLS = ditools.mk
DRV_Q931 = drv.diq931.mk
LIB_IDI = lib.idi.mk


all:  $(KLOG) $(IDI) $(ETDC) $(ETDD) $(ETDK) $(ETDM) $(5ESS) $(ETSI) $(ATEL) $(NI1) $(JAPAN)


$(ETDK):
	(cd drv.klog; $(MAKEF) $(DRV_KLOG) $(MAKEARGS))

$(ETDD):
	(cd drv.diva; $(MAKEF) $(DRV_DIVA) $(MAKEARGS)) 

$(ETDC):
	(cd drv.capi20; $(MAKEF) $(DRV_CAPI) $(MAKEARGS))

$(ETDM):
	(cd drv.dimaint; $(MAKEF) $(DRV_DIMAINT) $(MAKEARGS))

$(ETSI): $(Q931) $(DITO)
	(cd drv.dietsi; $(MAKEF) $(DRV_ETSI) $(MAKEARGS))

$(5ESS): $(Q931) $(DITO)
	(cd drv.di5ess; $(MAKEF) $(DRV_5ESS) $(MAKEARGS))

$(ATEL): $(Q931) $(DITO)
	(cd drv.diatel; $(MAKEF) $(DRV_ATEL) $(MAKEARGS))

$(JAPAN): $(Q931) $(DITO)
	(cd drv.dijapan; $(MAKEF) $(DRV_JAPAN) $(MAKEARGS))

$(NI1): $(Q931) $(DITO)
	(cd drv.dini1; $(MAKEF) $(DRV_NI1) $(MAKEARGS))

$(Q931): $(PROT) 
	(cd drv.diq931; $(MAKEF) $(DRV_Q931) $(MAKEARGS))

$(DITO):   
	(cd ditools; $(MAKEF) $(DITOOLS) $(MAKEARGS))

$(IDI):
	(cd lib.idi; $(MAKEF) $(LIB_IDI) $(MAKEARGS))

$(KLOG):
	(cd klog; $(MAKEF) $(KLOG_MK) $(MAKEARGS))

$(PROT):
	(cd drv.diprot; $(MAKEF) $(DRV_PROT) $(MAKEARGS))

install: all
	(cd drv.klog; $(MAKEF) $(DRV_KLOG) $@ $(MAKEARGS))
	(cd drv.diva; $(MAKEF) $(DRV_DIVA) $@ $(MAKEARGS)) 
	(cd drv.capi20; $(MAKEF) $(DRV_CAPI) $@ $(MAKEARGS))
	(cd drv.dimaint; $(MAKEF) $(DRV_DIMAINT) $@ $(MAKEARGS))
	(cd drv.dietsi; $(MAKEF) $(DRV_ETSI) $@ $(MAKEARGS))
	(cd drv.di5ess; $(MAKEF) $(DRV_5ESS) $@ $(MAKEARGS))
	(cd drv.diatel; $(MAKEF) $(DRV_ATEL) $@ $(MAKEARGS))
	(cd drv.dijapan; $(MAKEF) $(DRV_JAPAN) $@ $(MAKEARGS))
	(cd drv.dini1; $(MAKEF) $(DRV_NI1) $@ $(MAKEARGS))
	(cd drv.diq931; $(MAKEF) $(DRV_Q931) $@ $(MAKEARGS))
	(cd ditools; $(MAKEF) $(DITOOLS) $@ $(MAKEARGS))
	(cd lib.idi; $(MAKEF) $(LIB_IDI) $@ $(MAKEARGS))
	(cd klog; $(MAKEF) $(KLOG_MK) $@ $(MAKEARGS))
	(cd drv.diprot; $(MAKEF) $(DRV_PROT) $@ $(MAKEARGS))

headinstall: 

clean:
	(cd drv.klog; $(MAKEF) $(DRV_KLOG) $@ $(MAKEARGS))
	(cd drv.diva; $(MAKEF) $(DRV_DIVA) $@ $(MAKEARGS)) 
	(cd drv.capi20; $(MAKEF) $(DRV_CAPI) $@ $(MAKEARGS))
	(cd drv.dimaint; $(MAKEF) $(DRV_DIMAINT) $@ $(MAKEARGS))
	(cd drv.dietsi; $(MAKEF) $(DRV_ETSI) $@ $(MAKEARGS))
	(cd drv.di5ess; $(MAKEF) $(DRV_5ESS) $@ $(MAKEARGS))
	(cd drv.diatel; $(MAKEF) $(DRV_ATEL) $@ $(MAKEARGS))
	(cd drv.dijapan; $(MAKEF) $(DRV_JAPAN) $@ $(MAKEARGS))
	(cd drv.dini1; $(MAKEF) $(DRV_NI1) $@ $(MAKEARGS))
	(cd drv.diq931; $(MAKEF) $(DRV_Q931) $@ $(MAKEARGS))
	(cd ditools; $(MAKEF) $(DITOOLS) $@ $(MAKEARGS))
	(cd lib.idi; $(MAKEF) $(LIB_IDI) $@ $(MAKEARGS))
	(cd klog; $(MAKEF) $(KLOG_MK) $@ $(MAKEARGS))
	(cd drv.diprot; $(MAKEF) $(DRV_PROT) $@ $(MAKEARGS))

clobber: clean
	(cd drv.klog; $(MAKEF) $(DRV_KLOG) $@ $(MAKEARGS))
	(cd drv.diva; $(MAKEF) $(DRV_DIVA) $@ $(MAKEARGS)) 
	(cd drv.capi20; $(MAKEF) $(DRV_CAPI) $@ $(MAKEARGS))
	(cd drv.dimaint; $(MAKEF) $(DRV_DIMAINT) $@ $(MAKEARGS))
	(cd drv.dietsi; $(MAKEF) $(DRV_ETSI) $@ $(MAKEARGS))
	(cd drv.di5ess; $(MAKEF) $(DRV_5ESS) $@ $(MAKEARGS))
	(cd drv.diatel; $(MAKEF) $(DRV_ATEL) $@ $(MAKEARGS))
	(cd drv.dijapan; $(MAKEF) $(DRV_JAPAN) $@ $(MAKEARGS))
	(cd drv.dini1; $(MAKEF) $(DRV_NI1) $@ $(MAKEARGS))
	(cd drv.diq931; $(MAKEF) $(DRV_Q931) $@ $(MAKEARGS))
	(cd ditools; $(MAKEF) $(DITOOLS) $@ $(MAKEARGS))
	(cd lib.idi; $(MAKEF) $(LIB_IDI) $@ $(MAKEARGS))
	(cd klog; $(MAKEF) $(KLOG_MK) $@ $(MAKEARGS))
	(cd drv.diprot; $(MAKEF) $(DRV_PROT) $@ $(MAKEARGS))
	
