#!/sbin/sh
#	copyright	"%c%"

#ident	"@(#)initpkg:i386/cmd/initpkg/rc0.d/:mk.rc0.d.sh	1.7.10.6"
#ident "$Header$"

STARTLST= 
STOPLST="00ANNOUNCE 02mse 50fumounts 60rumounts 70cron"

INSDIR=${ROOT}/${MACH}/etc/rc0.d
if u3b2 || i386
then
	if [ ! -d ${INSDIR} ] 
	then 
		mkdir ${INSDIR} 
		eval ${CH}chmod 755 ${INSDIR}
		eval ${CH}chgrp sys ${INSDIR}
		eval ${CH}chown root ${INSDIR}
	fi 
	for f in ${STOPLST}
	do 
		name=`echo $f | sed -e 's/^..//'`
		rm -f ${INSDIR}/K$f
		ln ${ROOT}/${MACH}/etc/init.d/${name} ${INSDIR}/K$f
	done
fi
