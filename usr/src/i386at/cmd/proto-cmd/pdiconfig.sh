#ident	"@(#)proto-cmd:i386at/cmd/proto-cmd/pdiconfig.sh	1.1.1.13"

# This is a quick fix to overcome trouble installing dual hba's.
# The in-core resmgr is sync'ed with /stand/resmgr and the sdevice
# files BEFORE the hba sdevice files are installed, so they never
# get the correct params from NVRAM.
/etc/conf/bin/idconfupdate -f -o /stand/resmgr

# run pdiconfig to turn off all unused HBA drivers.
/etc/scsi/pdiconfig -I /etc/scsi/.pdicfglog || {
	faultvt "$PDI_FAILED"
	halt
}
/etc/scsi/diskcfg      /etc/scsi/.pdicfglog || {
	faultvt "$PDI_FAILED"
	halt
}

# Create reverse dependency of the base package on the IHV HBA package
# which is associated with the boot device.  This is done to prevent
# removal of the HBA associated with the boot device.

OIFS="$IFS"
IFS='	'
while read f1 f2 f3 f4 f5 pdi_line
do
	if [ "$f4" = Y ] && (( $f5 == 0 ))
	then
		pdi_boot_name=$f1
	fi
done < /etc/scsi/.pdicfglog
IFS="$OIFS"
# If can't find directory, just skip
PKGPATH=/var/sadm/pkg/${pdi_boot_name}/install
if [ -d ${PKGPATH} ]
then
	# Create reverse dependency on the base package
	echo "R\tbase\tBase System" > ${PKGPATH}/depend
fi

call unlink /etc/scsi/.pdicfglog
call unlink /etc/scsi/pdi_edt
