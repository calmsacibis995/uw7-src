#ident	"@(#)oemcd:mach.build/ja.hcf.sh	1.1"
set -x
cd $ROOT/$MACH/etc/inst/locale/ja/menus/nics/supported_nics/help
ls -1 |grep hcf >> /tmp/file.ja
mkdir -p $ROOT/$SPOOL/prep/.extras.d/locale/ja/nics.d
for i in `cat /tmp/file.ja`
do
cp $ROOT/$MACH/etc/inst/locale/ja/menus/nics/supported_nics/help/$i $ROOT/$SPOOL/prep/.extras.d/locale/ja/nics.d
done
#rm /tmp/file.ja

