#!/sbin/sh



#ident	"@(#)adm:i386/cmd/.adm/deluser.sh	1.1.3.3"

# delete a user from the system
# 
#	Deluser allows you to remove users from the computer.  The
#	deleted user's files are removed from the file systems and their
#	logins are removed from the /etc/passwd file.

rm -f /usr/tmp/addcfm

loginid=$1
home=$3

if [ $# -lt 3 ]
then
	echo "USAGE: $0 <login id> <Yes/No> <home directory>"
	exit 1
fi

if [ "$home" = "" ]
then
	echo "Home directory can't be null"
	exit 1
fi

#	check if login name exists

grep "^$1:" /etc/passwd >/dev/null
rc=$?
if [ $rc -ne 0 ]
then
	echo "That login doesn't exist on the system."
	exit 1
fi

if [ "$2" = "Yes" ]
then

	rm -f /usr/mail/${loginid} 2>/dev/null

#
#	Copy all files to lost+found
#

        if [ -d ${home} ]
        then

        	mkdir /lost+found/${loginid} 2>/dev/null 
        	cd ${home} 2>/dev/null
        	find . -print | cpio -pcvd /lost+found/${loginid}
#
#	Remove home directory
#
		cd /
	 	rm -r ${home}
        else
		echo "The home directory doesn't exist on the system."
		exit 1
        fi

#
#	Search system for all files owned by loginid
#	Not sure if they want this yet.
#

#nohup find / -user $loginid -type f -exec mv {} /lost+found/${loginid} \; > /dev/null 2>&1 &

fi

#
#	Omit /etc/passwd entry
#

/usr/sbin/userdel $loginid

#
#	Omit /etc/.useradm entry (if it exits)
#

if [ -f /etc/.useradm ]
then
	grep "^${loginid}" /etc/.useradm > /dev/null
	if test $? = 0
	then
		if test -r /etc/adm.lock
		then
			exit
		else
			ed - /etc/.useradm <<-!
			H
			/^${loginid}/d
			w
			q
			!
			rm -f /etc/adm.lock
		fi
	fi
fi
