#!/sbin/sh
#	copyright	"%c%"

#ident	"@(#)initpkg:i386/cmd/initpkg/init.d/:mk.init.d.sh	1.2.9.2"
#ident "$Header$"

INSDIR=${ROOT}/${MACH}/etc/init.d
INS=${INS:-install}
if u3b2 || i386
then
	if [ ! -d ${INSDIR} ] 
	then 
		mkdir ${INSDIR} 
		if [ $? != 0 ]
		then
			exit 1
		fi
		eval ${CH}chmod 755 ${INSDIR}
		eval ${CH}chgrp sys ${INSDIR}
		eval ${CH}chown root ${INSDIR}
	fi 
	for f in [a-zA-Z0-9]* 
	do 
		eval ${INS} -f ${INSDIR} -m 0744 -u root -g sys $f
	done
fi
