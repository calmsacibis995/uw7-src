#include $(CMDRULES)
all: e7-par-all t15-p-2ics-sep t22-inc-all-cmp common.pl e8-par-test\
		 t16-s-2ics-sep t3-c-1tp e1-indirect t1-dropdead t17-c-noics \
		 t4-p-1tp e2-ind-ser-cmp t10-1ic-2tp-all t18-p-noics t5-s-1tp \
		 e3-par1 t11-c-2ics-tog t19-s-noics t6-1tp-allcmp e4-c-par \
		 t12-p-2ics-tog t2-build t7-c-1ic-2tp e5-p-par t13-s-2ics-tog \
		 t20-all t8-p-1ic-2tp e6-s-par t14-c-2ics-sep t21-serial t9-s-1ic-2tp
	-for i in $? ; do chmod a+rx $$i; \
	done

CLEAN:
	rm -fr output
