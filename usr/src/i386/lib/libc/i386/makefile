#ident	"@(#)libc-i386:makefile	1.44.1.70"
#
# makefile for libc/i386
#

include $(LIBRULES)
include ../$(CPU)/suffixes

OBJS	= ar_a_objs ar_p_a_objs ar_so_objs ar_p_so_objs \
	rt_O_objs rt_P_objs rt_O2_objs rt_P2_objs rt_O5_objs rt_P5_objs \
	crt_objs

all: $(OBJS)

include incl

install: all
	for x in $(CRT_OBJS); do \
		o=`basename "$$x" .x`.o; \
		$(INSTALL) 644 $(OWN) $(GRP) $(CCSLIB)/$(SGS)$$o $$x; \
	done

clean:
	-rm -f $(AR_A_OBJS)
	-rm -f $(AR_P_A_OBJS)
	-rm -f $(AR_SO_OBJS)
	-rm -f $(AR_P_SO_OBJS)
	-rm -f $(RT_O_OBJS)
	-rm -f $(RT_P_OBJS)
	-rm -f $(RT_O2_OBJS)
	-rm -f $(RT_P2_OBJS)
	-rm -f $(RT_O5_OBJS)
	-rm -f $(RT_P5_OBJS)
	-rm -f $(CRT_OBJS)

clobber: clean
	rm -f ./mk_cvt fmt/_cvt.c

creations: fmt/_cvt.c

fmt/_cvt.c: ./mk_cvt fmt/cvt-config ../port/fmt/cvt-template \
		../port/fmt/cvt-script
	./mk_cvt -o fmt/_cvt.c -c fmt/cvt-config -s ../port/fmt/cvt-script \
		-t ../port/fmt/cvt-template

./mk_cvt: ../port/fmt/mk_cvt.c
	$(HCC) -o ./mk_cvt ../port/fmt/mk_cvt.c
