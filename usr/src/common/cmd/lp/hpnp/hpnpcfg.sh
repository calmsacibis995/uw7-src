#!/bin/ksh
#ident	"@(#)hpnpcfg.sh	1.2"
#		copyright	"%c%"
#
# (c)Copyright Hewlett-Packard Company 1991.  All Rights Reserved.
# (c)Copyright 1983 Regents of the University of California
# (c)Copyright 1988, 1989 by Carnegie Mellon University
# 
#                          RESTRICTED RIGHTS LEGEND
# Use, duplication, or disclosure by the U.S. Government is subject to
# restrictions as set forth in sub-paragraph (c)(1)(ii) of the Rights in
# Technical Data and Computer Software clause in DFARS 252.227-7013.
#
#                          Hewlett-Packard Company
#                          3000 Hanover Street
#                          Palo Alto, CA 94304 U.S.A.
#
#
set -a

ACTIVEHWADDR=""
BOOTPTABLE="/etc/bootptab"
DEBUG="OFF"
HOSTNAME=`uname -n`
HOSTTABLE="/etc/hosts"
HPNP="/usr/lib/hpnp"
PRINTCAP="/etc/printcap"
QUITHELP='echo Enter "q" to return to the main menu'
SAVENAME=""
SAVEIPADDR=""
OS="UnixWare"

typeset -r BOOTPTABLE HOSTNAME HOSTTABLE HPNP PRINTCAP QUITHELP OS

PATH="$HPNP/cfg:$PATH"

clear_scr()
{
	# check if the terminal environment is set up
	[ -n "$TERM" ] && clear 2> /dev/null
}

#
#   Make sure /tmp or /usr/tmp directories exist on host 
#
TMP="/tmp"
if [ ! -d "$TMP" ]
then
	TMP="/usr/tmp" 
	if [ ! -d "$TMP" ]
	then
		echo "No /tmp or /usr/tmp working directories - cannot continue."
		exit 1
	fi
fi
typeset -r TMP


LOG="$TMP/hpnpcfg.log"
typeset -r LOG


#
# Log the start time of hpnpcfg
#
echo "" >> "$LOG"
echo "======== `date` BEGIN HPNPCFG ON  $HOSTNAME" >> "$LOG"


NL="\c"
TFTPDIR="/tftpboot"
TFTP="/usr/bin/tftp"
typeset -r TFTPDIR TFTP NL
if [ ! -d "$TFTPDIR" ]
then
	mkdir -- "$TFTPDIR"
	chmod u=rwx,go=rx "$TFTPDIR"
fi

################################################
#
#  user must be root and have superuser capability
#
################################################

typeset -i UID
UID=`id -u`
[ $UID -ne 0 ] && {
	echo "You must be super-user (root) to execute this script."
	exit 1
}

#
# This may end up being /hpnp if
# tftp is not supported on this system.
#
CONFIGDIR="$TFTPDIR/hpnp"

if [ "$DEBUG" = "ON" ]
then
  echo    "Val of TMP is $TMP"
  echo    "Val of TFTPDIR is $TFTPDIR"
  echo    "Val of CONFIGDIR is $CONFIGDIR"
fi

typeset -i CHANGES=0
while [ 0 ]
do
  clear_scr
  echo ""
  echo "        HP NETWORK PRINTER CONFIGURATION TASKS"
  echo "                       MAIN MENU"
  echo ""
  echo "        1) Verify installation of software"
  echo "        2) Configure a printer with BOOTP/TFTP"
  echo "        3) Verify BOOTP/TFTP configuration"
  echo "        4) Verify network printer connectivity"
  echo "        5) Verify network printer operation"
  echo "        6) Add printer to spooler"
  echo "        7) Remove printer BOOTP/TFTP configuration"
  echo "        8) Remove printer from spooler"
  echo ""
  echo "             ?) Help            q) Quit"
  echo ""
  echo "Please enter selection: $NL "
  read -r SEL
  case $SEL in
	1) echo "======== MENU CHOICE 1 - no system changes made" >> $LOG
	   option1
           ;;
	2) echo "======== BEGIN MENU CHOICE 2" >> $LOG
	   option2
           if [ $? -eq 0 ]
           then
             ACTIVEHWADDR="`cat $TMP/ACTIVEHWADDR`"
             SAVENAME="`cat $TMP/SAVENAME`"
	     if [ -f "$TMP/SAVEIPADDR" ]
	     then
	       SAVEIPADDR="`cat $TMP/SAVEIPADDR`"
	     fi
	     if [ -f "$TMP/GETCOMNAM" ]
	     then
	       SAVEGETCOM="`cat $TMP/GETCOMNAM`"
	     fi
	     CHANGES=1
           fi
	   rm -f -- "$TMP/ACTIVEHWADDR"
           rm -f -- "$TMP/SAVENAME"
	   rm -f -- "$TMP/SAVEIPADDR"
	   rm -f -- "$TMP/GETCOMNAM"
	   echo "======== END MENU CHOICE 2" >> $LOG
	   ;;
	3) echo "======== BEGIN MENU CHOICE 3" >> $LOG
	   option3 "$ACTIVEHWADDR"
	   CHANGES=1
	   echo "======== END MENU CHOICE 3" >> $LOG
	   ;;
	4) echo "======== MENU CHOICE 4 - no system changes made" >> $LOG
	   #
           # Ignore interrupt signals since it is common
           # to interrupt ping.
	   #
	   trap "" 1 2 3 15
	   if [ -z "$SAVEIPADDR" ]
	   then
		option4 "$SAVENAME"
	   else
		option4 "$SAVEIPADDR"
	   fi
	   #
	   # Reset signals handlers.
	   #
	   trap 1 2 3 15
	   ;;
	5) echo "======== MENU CHOICE 5 - no system changes made" >> "$LOG"
	   #
           # Ignore interrupt signals since user might interrupt
           # the file send if peripheral is not responding.
	   #
	   trap "" 1 2 3 15
	   if [ -z "$SAVEIPADDR" ]
	   then
	     option5 "$SAVENAME" "$SAVEGETCOM"
	   else
	     option5 "$SAVEIPADDR" "$SAVEGETCOM"
	   fi
	   trap 1 2 3 15
	   ;;
        6) echo "======== BEGIN MENU CHOICE 6" >> "$LOG"
	   option6 "$SAVENAME"
	   CHANGES=1
	   echo "======== END MENU CHOICE 6" >> "$LOG"
           ;;
	7) echo "======== BEGIN MENU CHOICE 7" >> "$LOG"
	   option7 "$SAVENAME"
           if [ $? -eq 0 ]
           then
             ACTIVEHWADDR=""
           fi
	   CHANGES=1
	   echo "======== END MENU CHOICE 7" >> "$LOG"
	   ;;
        8) echo "======== BEGIN MENU CHOICE 8" >> "$LOG"
	   option8 "$SAVENAME"
	   CHANGES=1
	   echo "======== END MENU CHOICE 8" >> "$LOG"
           ;;
        q|Q) 
	   if [ $CHANGES -eq 1 ]
	   then
	     echo ""
	     echo "A log of the system changes made during this configuration"
	     echo "session was appended to $LOG."
	     echo ""
	   fi
	   break;;
	\?) echo ""
  echo "To configure a new network peripheral, use options 2 through 6 in"
  echo "sequence.  Option 2 creates a BOOTP entry and an optional file"
  echo "of configuration parameters to be retrieved by the network peripheral"
  echo "with TFTP.  Options 3, 4, and 5 verify the local BOOTP/TFTP operation"
  echo "and verify that the network peripheral is operating correctly as"
  echo "a network node.  Option 5 sends a file to the network peripheral"
  echo "without using the spooling system.  Option 6 configures the local"
  echo "spooler to use the network peripheral.  After completing option 6,"
  echo "you are finished with peripheral and spooler configuration and can"
  echo "send printjobs through the spooling system."
  echo ""
  echo "Option 7 removes the BOOTP entry and configuration file created"
  echo "by option 2."
  echo ""
  echo "Option 8 removes the spooler configuration created in option 6."
  echo ""
  echo "Press the return key to continue ... $NL"
  read -r RESP
	   ;;
        *) echo "Invalid selection";;
     esac
	
done
