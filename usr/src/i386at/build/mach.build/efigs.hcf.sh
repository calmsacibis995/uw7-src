#ident	"@(#)oemcd:mach.build/efigs.hcf.sh	1.1"
set -x
##cp /home3/unix-src/e7/i386at/etc/inst/locale/it/menus/nics/supported_nics/help/*hcf /home3/unix-src/e7/pkg/prep/.extras.d/locale/it/nics.d
cd $ROOT/$MACH/etc/inst/locale/it/menus/nics/supported_nics/help
ls -1 |grep hcf >> /tmp/file.it
mkdir -p $ROOT/$SPOOL/prep/.extras.d/locale/it/nics.d
for i in `cat /tmp/file.it`
do
cp $ROOT/$MACH/etc/inst/locale/it/menus/nics/supported_nics/help/$i $ROOT/$SPOOL/prep/.extras.d/locale/it/nics.d
done
#rm /tmp/file.it
cd $ROOT/$MACH/etc/inst/locale/es/menus/nics/supported_nics/help
ls -1 |grep hcf >> /tmp/file.es
mkdir -p $ROOT/$SPOOL/prep/.extras.d/locale/es/nics.d
for i in `cat /tmp/file.es`
do
cp $ROOT/$MACH/etc/inst/locale/es/menus/nics/supported_nics/help/$i $ROOT/$SPOOL/prep/.extras.d/locale/es/nics.d
done
#rm /tmp/file.es
cd $ROOT/$MACH/etc/inst/locale/fr/menus/nics/supported_nics/help
ls -1 |grep hcf >> /tmp/file.fr
mkdir -p $ROOT/$SPOOL/prep/.extras.d/locale/fr/nics.d
for i in `cat /tmp/file.fr`
do
cp $ROOT/$MACH/etc/inst/locale/fr/menus/nics/supported_nics/help/$i $ROOT/$SPOOL/prep/.extras.d/locale/fr/nics.d
done
#rm /tmp/file.fr
cd $ROOT/$MACH/etc/inst/locale/de/menus/nics/supported_nics/help
ls -1 |grep hcf >> /tmp/file.de
mkdir -p $ROOT/$SPOOL/prep/.extras.d/locale/de/nics.d
for i in `cat /tmp/file.de`
do
cp $ROOT/$MACH/etc/inst/locale/de/menus/nics/supported_nics/help/$i $ROOT/$SPOOL/prep/.extras.d/locale/de/nics.d
done
#rm /tmp/file.de

