#!/sbin/sh
#	copyright	"%c%"

#ident	"@(#)dircmp:dircmp.sh	1.13.3.2"
PATH=/usr/bin
USAGE="Usage: dircmp -s -d -wn directory directory\n"
trap "rm -f /var/tmp/dc$$*;exit" 1 2 3 15
Sflag=""
Dflag=""
width=72
if LC_MESSAGES=C type getopts | grep 'not found' > /dev/null
then
	eval set -- "`getopt dsw: "$@"`"
	if [ $? != 0 ]
	then
		pfmt -l UX:dircmp -s action -g uxdfm:78 "$USAGE"
		exit 2
	fi
	for i in $*
	do
		case $i in
		-d)	Dflag="yes"; shift;;
		-s)	Sflag="yes"; shift;;
		-w)	width=`expr $2 + 0 2>/dev/null`
			if [ $? = 2 ]
			then pfmt -l UX:dircmp -s error -g uxdfm:79 "numeric argument required\n"
				exit 2
			fi
			shift 2
			;;
		--)	shift; break;;
		esac
	done
else
	while getopts dsw: i
	do
		case $i in
		d)	Dflag="yes";; 
		s)	Sflag="yes";; 
		w)	width=`expr $OPTARG + 0 2>/dev/null`
			if [ $? = 2 ]
			then pfmt -l UX:dircmp -s error -g  uxdfm:79 "numeric argument required\n"
				exit 2
			fi
			;;
		\?)	pfmt -l UX:dircmp -s action -g uxdfm:78 "$USAGE"
			exit 2;;
		esac
	done
	shift `expr $OPTIND - 1`
fi
D0=`pwd`
D1=$1
D2=$2
if [ $# -lt 2 ]
then pfmt -l UX:dircmp -s action -g uxdfm:78 "$USAGE"
     exit 1
elif [ ! -d "$D1" ]
then pfmt -l UX:dircmp -s error -g uxdfm:80 "%s not a directory !\n" $D1
     exit 2
elif [ ! -d "$D2" ]
then pfmt -l UX:dircmp -s error -g uxdfm:80 "%s not a directory !\n" $D2
     exit 2
fi
cd $D1
find . -print | sort > /var/tmp/dc$$a
cd $D0
cd $D2
find . -print | sort > /var/tmp/dc$$b
comm /var/tmp/dc$$a /var/tmp/dc$$b | sed -n \
	-e "/^		/w /var/tmp/dc$$c" \
	-e "/^	[^	]/w /var/tmp/dc$$d" \
	-e "/^[^	]/w /var/tmp/dc$$e"
rm -f /var/tmp/dc$$a /var/tmp/dc$$b
pr -w${width} -h "$D1 only and $D2 only" -m /var/tmp/dc$$e /var/tmp/dc$$d
rm -f /var/tmp/dc$$e /var/tmp/dc$$d
sed -e s/..// < /var/tmp/dc$$c > /var/tmp/dc$$f
rm -f /var/tmp/dc$$c
cd $D0
> /var/tmp/dc$$g
while read a
do
	if [ -d $D1/"$a" ]
	then if [ "$Sflag" != "yes" ]
	     then gettxt uxdfm:81 "directory"
		  echo "	$a"
	     fi
	elif [ -f $D1/"$a" ]
	then cmp -s $D1/"$a" $D2/"$a"
	     if [ $? = 0 ]
	     then if [ "$Sflag" != "yes" ]
		  then  gettxt uxdfm:82 "same"
			echo "		$a"
		  fi
	     else gettxt uxdfm:83 "different"
		  echo "	$a"
		  if [ "$Dflag" = "yes" ]
		  then
			type=`LC_MESSAGES=C file $D1/"$a"`
			case "$type" in
				*text)	;;
				*Empty*)dname=$D1/`basename "$a"`
					pfmt -s warn -g uxdfm:84 "%s is an empty file\n" $dname 2>&1 |
					 pr -h "diff of $a in $D1 and $D2" >> /var/tmp/dc$$g
					continue
				;;
				*)	dname=$D1/`basename "$a"`
					pfmt -s warn -g uxdfm:85 "%s is an object file\n" $dname 2>&1 |
					 pr -h "diff of $a in $D1 and $D2" >> /var/tmp/dc$$g
					continue
				;;
			esac
			type=`LC_MESSAGES=C file $D2/"$a"`
			case "$type" in
				*text)	;;
				*Empty*)dname=$D2/`basename "$a"`
					pfmt -s warn -g uxdfm:84 "%s is an empty file\n" $dname 2>&1 |
					 pr -h "diff of $a in $D1 and $D2" >> /var/tmp/dc$$g
					continue
				;;
				*)	dname=$D2/`basename "$a"`
					pfmt -s warn -g uxdfm:85 "%s is an object file\n" $dname 2>&1 |
					 pr -h "diff of $a in $D1 and $D2" >> /var/tmp/dc$$g
					continue
				;;
			esac
			diff $D1/"$a" $D2/"$a" | pr -h "diff of $a in $D1 and $D2" >> /var/tmp/dc$$g
		  fi
	     fi
	elif [ "$Sflag" != "yes" ]
	then gettxt uxdfm:86 "special"
	     echo "  	$a"
	fi
done < /var/tmp/dc$$f | pr -r -h "Comparison of $D1 $D2"
if [ "$Dflag" = "yes" ]
then cat /var/tmp/dc$$g
fi
rm -f /var/tmp/dc$$*
