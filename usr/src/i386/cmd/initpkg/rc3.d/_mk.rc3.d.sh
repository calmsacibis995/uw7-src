#!/sbin/sh
#	copyright	"%c%"

#ident	"@(#)initpkg:i386/cmd/initpkg/rc3.d/:mk.rc3.d.sh	1.7.8.5"
#ident "$Header$"

STARTLST=
STOPLST= 

INSDIR=${ROOT}/${MACH}/etc/rc3.d
if u3b2 || i386
then
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
fi
