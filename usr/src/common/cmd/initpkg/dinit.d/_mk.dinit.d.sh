#!/sbin/sh
#	copyright	"%c%"

#ident	"@(#)initpkg:common/cmd/initpkg/dinit.d/:mk.dinit.d.sh	1.9"
#ident  "$Header$"

STARTLST="23ttymap 70uucp 75cron 80cs"

INSDIR=${ROOT}/${MACH}/etc/dinit.d
if [ ! -d ${INSDIR} ] 
then 
	mkdir ${INSDIR} 
	eval ${CH}chmod 755 ${INSDIR}
	eval ${CH}chgrp sys ${INSDIR}
	eval ${CH}chown root ${INSDIR}
fi 
for f in ${STARTLST}
do 
	name=`echo $f | sed -e 's/^..//'`
	rm -f ${INSDIR}/S$f
	ln ${ROOT}/${MACH}/etc/init.d/${name} ${INSDIR}/S$f
done
