#ident	"@(#)drf:cmd/txtstr.C	1.13.2.1"

FOOTER_HEIGHT=1
Yes=Yes
No=No
MiniWelcome="
Welcome to emergency recovery mode.

There are few utilities available for you to use in this mode.
The utilities available are vi, fsck, cpio, dd, grep, ls etc. 
If your hard disk is sane, you can mount the file systems on
the hard disk under /mnt using mountall command. You can 
unmount them using umountall command.

To exit from this emergency recovery shell, just type 'exit<Enter>'
followed by '<Alt>-<SysReq>, then <F1>.
"
DRF_NEXT_FLP="

	Insert the second emergency recovery

	    floppy and press ENTER.

"
DRF_READ_FLP="

	Reading the second emergency recovery floppy.

"
DRF_HARD_MODS_WAIT="

	Please wait while the system

	hardware drivers are loaded. 

"
DRF_SOFT_MODS_WAIT="

	Please wait while the system

	software drivers are loaded. 

"
DRF_UNMNT_MSG="

	Unmounting all the file systems from /mnt.
	
	Please wait.
"
DRF_MNT_MSG="

	Mounting the file systems on the hard 

	disk under /mnt. Please wait.
"
DRF_DSK_GOOD="
	The hard disk is sane.

	Press ENTER to continue.
"
DRF_DSK_BAD="
	The hard disk is NOT sane.

	Press ENTER to continue.
"
DSK_SETUP="

	Setting up the hard disk(s) to match 

	    the original configuration.

"
DSK_SETUP_ERR="

          Encountered an error while setting

                   up the disk.

              Press ENTER to continue.
"
DSK_RST_ERR="

          Encountered an error while 

             restoring the disk.

          Press ENTER to continue.
"
DSK_RST_MSG="

         Restoring the disk(s). 

   (This will take several minutes.)

"
RESTORE_SUCCESS="

         Disk(s) is(are) successfully restored.

            Press ENTER to continue

"
MAIN_TITLE="Emergency Recovery Menu"
SH_PROMPT="Access the UnixWare shell"
MNT_PROMPT="Mount the hard disk file systems under /mnt"
UNMNT_PROMPT="Unmount the file systems mounted under /mnt"
RESTORE_PROMPT="Restore from an emergency recovery tape"
REBOOT_PROMPT="Reboot the system"
DRF_SHELL="^Access UnixWare Shell"
DRF_MOUNT="^Mount File Systems" 
DRF_UNMOUNT="^Unmount File Systems"
DRF_RESTORE="^Restore Disk(s)"
DRF_REBOOT="Re^boot"
GENERIC_FORM_FOOTER="Press F1 for Help"
GENERIC_MENU_FOOTER="Use up/down arrow keys and ENTER to select; F1 for help"
GENERIC_WAIT_FOOTER="Please wait."
GENERIC_HEADER="UnixWare 7 - Emergency Recovery"

# For use in 'helpwin' script
FIRST_PAGE_FOOTER="F1=More help, Page Down, ESC to exit help"
MIDDLE_PAGE_FOOTER="F1=More help, Page Down, Page Up, ESC to exit help"
LAST_PAGE_FOOTER="F1=More help, Page Up, ESC to exit help"
ONLY_PAGE_FOOTER="F1=More help, ESC to exit help"
MOREHELP_FOOTER="Use up/down arrow keys to select or ESC to exit help."
MOREHELP_TITLE="Related help topics"

TAPE_PROMPT="
  Please insert the tape into the tape drive and press
'ENTER' when ready.  If you have more than one
tape drive, make sure all of them are empty except
the one that contains the emergency recovery tape.

If the tape is already in the drive, just press 'ENTER'.
"
TAPE_OPEN_ERROR="
  No tape has been detected in the tape drive.
Make sure the tape is properly inserted in the drive,
the latch(if any) is engaged, and the power to the
tape drive is on.

             Press ENTER to continue."
TAPE_READ_ERROR="
   The tape in the tape drive is unreadable.
Make sure the tape is properly inserted in the drive,
the latch (if any) is engaged, and the power to the 
tape drive is on.

             Press ENTER to continue."
NEXT_TAPE_PROMPT="
   Remove the current tape from the drive and insert

   the next tape.

            Press ENTER to continue."
#for use in Color_Console
MONITOR_TYPE="
On a color monitor this screen will show white text on a blue
background.  Otherwise this should be considered a non-color
monitor.  What type of monitor is attached to your computer?

"
MONITOR_FOOTER="Use arrow keys to select a monitor type and then press 'ENTER'."
MONITOR_ENTRY="Choose Monitor Type"

REBOOT_MSG="

  ****MAKE SURE THE BOOT DRIVE (DISKETTE OR CD-ROM) IS EMPTY***  

  When you press 'ENTER', the system will be shut down and
  rebooted.
"
SERIAL_FIELD="The serial number : "
SERIAL_PROMPT="Enter the serial number of your UnixWare copy and press F10"
SERIAL_TITLE="Serial Number form" 
INCORRECT_SNUM="The serial number you entered is not correct. Enter it again and press F10"
SNUM_FATAL="

	The serial number did not match in the maximum allowed 
	number of tries. The machine will be rebooted when you 
	press 'ENTER'.

"
COLOR_PROMPT="

                Monitor Type Selection

  UnixWare was not able to determine whether your computer 
  is equipped with a color or non-color (monochrome) monitor.

  To ensure that you can read all the screens during recovery,
  UnixWare must know which type of monitor you have.

  On a color monitor this screen will show white text on a blue
  background.  Otherwise this should be considered a non-color
  monitor.  What type of monitor is attached to your computer?

  Your choices are:

  1. Color monitor.
  2. Non-color monitor.

  Please type '1' or '2' followed by 'ENTER': "

RETENSION_WAIT="

        Retensioning the tape.

   This will take about three minutes.

"
FLPY2_READ_ERROR_1="
   An error occurred while reading from the second
emergency diskette. Please check that you have the
correct diskettes and restart the boot process from
the beginning. Without the correct second diskette,
you may proceed but will have limited capabilities.

             Press ENTER to continue."
FLPY2_READ_ERROR_2="
   An error occurred while reading from the second
emergency diskette. Please check that you have the
correct diskettes and restart the boot process from
the beginning. Without the correct second diskette,
you may proceed but will have limited capabilities.

             Press ENTER to continue."
FS_MOD_LOAD_ERROR="
   An error occurred while loading the file system or
kdb modules. Please  check that you have the correct
diskettes and restart  the boot process from the 
beginning. Without the correct second  diskette,
you may proceed but will have limited capabilities.

             Press ENTER to continue."
DISK_INFO_READ_MSG="

         Reading the original disk(s) configuration

                  information.

"
NWS_INFO="
 You now have to reinsert the Emergency Recovery Tape
 (or the first tape, if there is more than one).

 Please put the tape into the tape drive and press ENTER. 
"
ASKSERVER_CHOOSE="Single server, or no replicas
Multiple servers, replicas exist"
NWS_SERVER_INFO="
 If your system is the only server in the NetWare Directory
 Services (NDS) tree, the recovery can finish automatically.

 If your server is one of several, there may be replicas of
 the NDS information that was on this server.  Use these replicas 
 instead of the information on your disaster recovery tape.
 See \"Restoring NetWare Directory Services in UnixWare X.X\"
 for details.

 Indicate whether your NDS tree has more than one server,
 then press ENTER
"
NWS_CHOOSE="NDS tree type"
