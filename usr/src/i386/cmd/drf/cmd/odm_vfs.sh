#!/sbin/sh
#ident	"@(#)drf:cmd/odm_vfs.sh	1.1"

RTDEV=`devattr disk1 cdevice 2>/dev/null`
RDEV1=`basename $RTDEV | sed "s/..$//"`
RDEV2=""
RTDEV=`devattr disk2 cdevice 2>/dev/null`
[ $? -eq 0 ] && RDEV2=`basename $RTDEV | sed "s/..$//"`

ODM_DIR=/etc/vx/reconfig.d/disk.d
CMDS=/tmp/emer_cmds_file
>$CMDS
for i in `ls $ODM_DIR 2>/dev/null`   #for all encapsulated disks
do
   [ "$i" = "$RDEV1" -o "$i" = "$RDEV2" ] || continue
   grep "^#renam" $ODM_DIR/$i/newpart | {
	while read a nodenum nodename
	do
	    grep "^/dev/vx/dsk/$nodename	/dev/vx/rdsk/$nodename" /etc/vfstab >/dev/null 2>&1
	    [ $? -eq 0 ] &&
		echo "s?/dev/vx/dsk/$nodename	/dev/vx/rdsk/$nodename?/dev/dsk/$nodenum	/dev/rdsk/$nodenum?g" >>$CMDS
        done }
done

sed -f $CMDS /etc/vfstab > $1
rm -f $CMDS 

for i in `ls $ODM_DIR 2>/dev/null`   #for all encapsulated disks
do
   if [ "$i" = "$RDEV1" ] 
   then
	cp $ODM_DIR/$i/vtoc $2
	cp $ODM_DIR/$i/newpart $3
   elif [ "$i" = "$RDEV2" ] 
   then
	for j in /usr /home /home2 /var
	do
	   spl=`grep "[	]$j[	]" $1 | cut -f1`
	   if [ $? -eq 0 ]
	   then
		echo $spl | grep $RDEV2 >/dev/null 2>&1
		[ $? -eq 0 ] && {
			cp $ODM_DIR/$i/vtoc $4
			cp $ODM_DIR/$i/newpart $5
			break
		}
	   fi
	done
   fi
done
