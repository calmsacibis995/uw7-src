#ident	"@(#)stdcmds:suscfg/suscfg.sh	1.3"

####################################################################
# 
# suscfg - script to configure/deconfigure UNIX95 on running system. 
#
# To configure UNIX95, changes are made to /etc/profile to set 
# environment variables for UNIX95 users. RSTCHOWN tuneable is 
# turned on and the default login shell in /etc/default/useradd is set 
# to /u95/bin/sh. A kernel build and reboot is required to configure 
# RSTCHOWN. 
# 
####################################################################

# Print usage message and exit 
usage()
{
	pfmt -l UX:suscfg -s error -g uxsuscfg:1 "usage: suscfg [-e] [-d] [-n]\n"
	exit 1
}

# Setup Single UNIX Specification if not already done. 
# Otherwise, just return. 
onSUSenv()
{
	if [ -r /u95/bin/rstchown.orig ]
	then
		pfmt -l UX:suscfg -s error -g uxsuscfg:3 "Single UNIX Specification already configured.\n"
		return 1
	fi

        yes="`gettxt Xopen_info:4 'yes'`"; no="`gettxt Xopen_info:5 'no'`"
	msg="echo `gettxt uxsuscfg:10 \"Do you want to continue? [$yes/$no] \"`"
        pfmt  -s nostd -g uxsuscfg:9 "Configuring the Single UNIX Specification will rebuild the system.\n"
        while
		eval "$msg"
                read reply
        do
                case $reply in
                ${yes})
                                break
                                ;;
                ${no})
                                return 1
                                ;;
                *)
                                ;;
                esac
        done

	#
	# Update the /etc/profile.  Place the additional code before 
	# the "trap 1 2 3", otherwise at EOF.  
	#

	INSERT="# ADDED BY SINGLE UNIX SPECIFICATION SETUP\n\
# If the default SHELL environment variable is /u95/bin/sh,\n\
# then a UNIX 95 conformant user is logging in. Set environment\n\
# variables appropriately. \n\n\
if [ \"\$SHELL\" = \"/u95/bin/sh\" ]\n\
then\n\
        PATH=/u95/bin:\$PATH\n\
        POSIX2=on\n\
        export PATH POSIX2\n\
fi\n\
# ADDED BY SINGLE UNIX SPECIFICATION SETUP"

        if [ "`/usr/bin/grep 'trap 1 2 3' /etc/profile`" ]
        then
                /usr/bin/cat /etc/profile | /usr/bin/awk '
                        {if (index($0, "trap 1 2 3") != 0) {print INSERT }; print $0}
                ' INSERT="$INSERT" > /tmp/profile
        else
                /usr/bin/cp /etc/profile /tmp/profile
                /usr/bin/echo "${INSERT}" >> /tmp/profile
        fi
        /usr/bin/mv /tmp/profile /etc/profile >/dev/null 2>&1
        /usr/bin/chmod 0644 /etc/profile
        /usr/bin/chown sys:root /etc/profile

	#
	# Turn on RSTCHOWN tuneable after saving current state.
	# idtune returns tab separated list of values, first one is 
	# current value
	#
	/etc/conf/bin/idtune -g RSTCHOWN 2>/dev/null | /usr/bin/cut -f1 > /u95/bin/rstchown.orig
        # Now, retune it
        /etc/conf/bin/idtune -f RSTCHOWN 1 > /dev/null 2>&1


	#
	# Change the default shell to the POSIX shell, /u95/bin/sh.
	#
	/usr/bin/defadm useradd SHELL | /usr/bin/cut -d'=' -f2 > /u95/bin/shell.orig
        # Now, set it
        /usr/bin/defadm useradd SHELL=/u95/bin/sh > /dev/null 2>&1

	return 0
}

# Deconfigure Single UNIX Specification if not already configured.
# Otherwise, just return.
offSUSenv()
{
if [ ! -r /u95/bin/rstchown.orig ]
then
	pfmt -l UX:suscfg -s error -g uxsuscfg:4 "Single UNIX Specification not configured.\n"
	return 1
fi

yes="`gettxt Xopen_info:4 'yes'`"; no="`gettxt Xopen_info:5 'no'`"
msg="echo `gettxt uxsuscfg:10 \"Do you want to continue? [$yes/$no] \"`"
pfmt  -s nostd -g uxsuscfg:11 "Deconfiguring the Single UNIX Specification will rebuild the system.\n"
while
	eval "$msg"
        read reply
do
        case $reply in
        ${yes})
                        break
                        ;;
        ${no})
                        return 1
                        ;;
        *)
                        ;;
        esac
done

	#
	# Update /etc/profile by removing the UNIX95 related code
	#
	/usr/bin/cat /etc/profile | /usr/bin/awk '{if (ST==2) print $0; \
	        if (ST==1) {if ($0 == "# ADDED BY SINGLE UNIX SPECIFICATION SETUP") ST=2}; \
	        if (ST==0) {if ($0 != "# ADDED BY SINGLE UNIX SPECIFICATION SETUP") {print $0} else {ST=1}}}' \
	        > /tmp/profile
	/usr/bin/mv /tmp/profile /etc/profile
	/usr/bin/chmod 644 /etc/profile
	/usr/bin/chown root:sys /etc/profile
	
	#
	# Reset the RSTCHOWN parameter
	#
	val="`/usr/bin/cat /u95/bin/rstchown.orig`"
	/etc/conf/bin/idtune -f RSTCHOWN "$val" 1>/dev/null 2>&1
	/usr/bin/rm /u95/bin/rstchown.orig

	#
	# Reset the SHELL variable in /etc/default/useradd
	#
	curval=`/usr/bin/defadm useradd SHELL | /usr/bin/cut -d"=" -f2`
	if [ "${curval}" = "/u95/bin/sh" ]
	then
	        if [ -r /u95/bin/shell.orig ]
	        then
	                newval=`/usr/bin/cat /u95/bin/shell.orig`
	                /usr/bin/defadm useradd SHELL=${newval} 1>/dev/null 2>&1
			/usr/bin/rm /u95/bin/shell.orig
	        else
	                /usr/bin/defadm useradd SHELL=/usr/bin/sh 1>/dev/null 2>&1
	        fi
	else
		rm -f /u95/bin/shell.orig
	fi

	return 0
}

# main routine - for turning on/off Single UNIX Specification.

TMP=/tmp/suscfg.err
rm -f ${TMP}

while getopts edn c; do
	case $c in
	e)	onSUSenv
		rc=${?}
		;;
	d)	offSUSenv
		rc=${?}
		;;
	n)	if [ -r /u95/bin/rstchown.orig ]; then
			pfmt -s nostd -g uxsuscfg:8 "Single UNIX Specification is configured.\n"
		else
			pfmt -s nostd -g uxsuscfg:4 "Single UNIX Specification not configured.\n"
		fi
		exit 0	
		;;
	*)	usage
		;;
	esac
done
#shift `expr $OPTIND - 1`

if [ $# -eq 0 ]; then 
	usage
fi

if [ ${rc} = 0 ]
then
	# Reconfigure system
	/etc/conf/bin/idbuild -B 2>${TMP}
	if [ "$?" != 0 ] 
	then
		pfmt -l UX:suscfg -s error -g uxsuscfg:5 "Error during idbuild.\n"
		exit 1
	fi
	pfmt -s nostd -g uxsuscfg:6 "A system reboot is required to complete setup or removal\nof the Single UNIX Specification configuration.\n"
	exit 0
else
	exit 1
fi
