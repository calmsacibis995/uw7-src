#!/sbin/sh
#	copyright	"%c%"

#ident	"@(#)initpkg:i386/cmd/initpkg/rc1.d/:mk.rc1.d.sh	1.1.10.5"
#ident "$Header$"

STARTLST="01MOUNTFSYS"

STOPLST="00ANNOUNCE 50fumounts 60rumounts 70cron"

INSDIR=${ROOT}/${MACH}/etc/rc1.d
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
	for f in ${STARTLST}
	do 
		name=`echo $f | sed -e 's/^..//'`
		rm -f ${INSDIR}/S$f
		ln ${ROOT}/${MACH}/etc/init.d/${name} ${INSDIR}/S$f
	done
fi
