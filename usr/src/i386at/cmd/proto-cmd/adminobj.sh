#ident	"@(#)adminobj.sh	15.1"

ROLE=false
USER=false
while getopts ru c
do
	case $c in
	r)	ROLE=true
		;;
	u)	USER=true
		;;
	\?) exit 1
		;;
	*)	echo Internal error during getopts. >&2
		exit 2
		;;
	esac
done
$ROLE && $USER && {
	echo Bad usage >&2
	exit 1
}
$ROLE || $USER || {
	echo Bad usage >&2
	exit 1
}

# The following while loop reads the commands and the roles or users to which
# these commands are to be assigned.  If privileges, separated by a colon,
# appear next to the role or user in the input it means that those privileges
# are to be shut off for that command when it is assigned to the role or user.

while read cmd objects
do
	echo $cmd | egrep "^#" > /dev/null 2>&1 && continue	# Skip comments
	base=`basename $cmd`
	privs=`
	egrep ":${cmd}$" /etc/security/tcb/privs |	# find command in tcb database
	sed 's/^.*%inher,\(.*\):.*/\1/p' |			# get the set of inher privs
	sed 's/^.*%fixed,\(.*\):.*//p' |			# delete the fixed privs
	sed 's/,/:/gp'								# change ,'s to :'s
	`
	if [ -z "$privs" ]
	then
		continue
	else
		prvd="yes"
	fi
	set $objects
	save="$privs"
	while [ $# -gt 0 ]
	do
		object=$1
		if echo "$1" | grep ":" > /dev/null
		then
			object=`echo "$1" | sed 's/:.*$//p'`
			if [ "$prvd" = "yes" ]
			then
				shutoff=`echo "$1" | sed 's/^[A-Z]*://p'`
				shutoff=`echo "$shutoff"|sed 's/:/ /gp'`
				fullset=`echo "$save"|sed 's/:/ /gp'`
				for i in $shutoff	#check if privileges to be shut off
				do					#are in full set of privilges
					found="false"
					for j in $fullset
					do
						if [ "$i" = "$j" ]
						then
							found="true"
							break
						fi
					done
					privs=""
					if [ "$found" = "false" ]
					then
						echo "Warning: \c"
						echo "$i privilege specified to be shut off for $cmd,"
						echo "\tbut it is NOT in its set of privileges."
						break
					fi
				done
				if [ -z "$shutoff" ]
				then
					privs="$save"
				else
					for i in $fullset
					do
						found="false"
						for j in $shutoff
						do
							if [ "$i" = "$j" ]
							then
								found="true"
								break
							fi
						done
						if [ "$found" = "false" ]
						then
							if [ -z "$privs" ]
							then
								privs=$i
							else
								privs=$privs:$i
							fi
						fi
					done
				fi
			fi
		else
			privs="$save"
		fi
		if $ROLE
		then
			adminrole -a $base:$cmd:$privs $object
		fi
		if $USER
		then
			adminuser -a $base:$cmd:$privs $object
		fi
		shift
	done
done
