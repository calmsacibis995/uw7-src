#ident	"@(#)kern-i386:util/compat/compat_f.mk	1.2"
#ident	"$Header$"

# Family-specific part of compat.mk

compat_sysHeaders_f = \
	weitek.h

compat_vmHeaders_f =

sysHeaders_f = \
	v86.h

headinstall_f: $(sysHeaders_f)
	@-[ -d $(INC)/sys ] || mkdir -p $(INC)/sys
	@for f in $(sysHeaders_f); \
	 do \
	    $(INS) -f $(INC)/sys -m $(INCMODE) -u $(OWN) -g $(GRP) $$f; \
	 done
