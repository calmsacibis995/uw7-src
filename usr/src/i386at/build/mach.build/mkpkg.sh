#ident	"@(#)oemcd:mach.build/mkpkg.sh	1.1"
set -x
cd $ROOT/usr/src/work/nws/pkg/nws
pkgmk -k -o -l512 -d/home4/pkg/cpq -r`pwd` -p"UW2.0 `date '+%m/%d/%y %H:%M:%S'` $$LOGNAME@`uname -n`"
cd $ROOT/usr/src/work
./:mkpkg -k -B 512 -d /home3/unix-src/pkg/cpq user
./:mkpkg -k -B 512 -d /home3/unix-src/pkg/cpq -L efigs UnixWare
./:mkpkg -k -B 512 -d /home3/unix-src/pkg/cpq sdk
