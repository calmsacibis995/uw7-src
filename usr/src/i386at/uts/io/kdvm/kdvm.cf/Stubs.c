#ident	"@(#)kern-i386at:io/kdvm/kdvm.cf/Stubs.c	1.2"

#include <sys/types.h>
#include <sys/kd.h>

struct kd_range kd_basevmem[64];

kdvm_unmapdisp() { return(0); }
