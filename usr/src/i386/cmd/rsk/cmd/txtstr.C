#ident	"@(#)txtstr.C	15.1"

FOOTER_HEIGHT=1
Yes=Yes
No=No
DRF_HARD_MODS_WAIT="

 Please wait while the system 

 hardware drivers are loaded. 

"
DRF_SOFT_MODS_WAIT="

 Please wait while the system 

 software drivers are loaded. 

"
DSK_SETUP="

 Setting up the hard disk(s) 

"
DSK_SETUP_ERR="

 Encountered an error while setting 

          up the disk.

     Press ENTER to continue.
"
DSK_RST_ERR="

 Encountered an error while 

    installing the disk.

  Press ENTER to continue.
"
DSK_RST_MSG="

       Loading the system. 

 (This will take several minutes.) 

"
RESTORE_SUCCESS="

 System successfully installed. 

    Press ENTER to continue

"
GENERIC_FORM_FOOTER="Press F1 for Help"
GENERIC_MENU_FOOTER="Use up/down arrow keys and ENTER to select; F1 for help"
GENERIC_WAIT_FOOTER="Please wait."
GENERIC_HEADER="UnixWare 2 - Replicated Installation Utility"

# For use in 'helpwin' script
FIRST_PAGE_FOOTER="F1=More help, Page Down, ESC to exit help"
MIDDLE_PAGE_FOOTER="F1=More help, Page Down, Page Up, ESC to exit help"
LAST_PAGE_FOOTER="F1=More help, Page Up, ESC to exit help"
ONLY_PAGE_FOOTER="F1=More help, ESC to exit help"
MOREHELP_FOOTER="Use up/down arrow keys to select or ESC to exit help."
MOREHELP_TITLE="Related help topics"

TAPE_PROMPT="
 Please insert the tape into the tape drive and press 
 'ENTER' when ready.  If you have more than one tape
 drive, make sure all of them are empty except the
 one that contains the installation tape.

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
TAPE_ERROR="
  The tape in the drive is not the RSK  
  installation tape.  Please put the
  RSK installation tape (the first one
  if there are several) in the drive.

     Press ENTER to continue."

NEXT_TAPE_PROMPT="
 Remove the current tape from the 
 drive and insert the next tape.

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
DISK_INFO_READ_MSG="

    Reading the disk(s) 

 configuration information. 

"
##
## For 'asknodename' script
##
System_Name="System Node Name:"
SHORTNAME="The System Node Name must be at least 3 characters long."
BADCHARNAME="The System Node Name must consist of ASCII letters and numerals."
NODEPROMPT="Type the node name for your system and press ENTER."
INITIAL_NODE_TITLE="Type System Node Name"
TITLE_NAME="System Node Name"
#Don't translate DSK_NOT,  it is an integer variable representing disk #
DSK_NOT_MSG1="Disk \#\$DSK_NOT does not exist."
RSK_DSK_ERR="

 Replication can not be continued. 

     Press ENTER to continue.
 
"
#Don't translate DSK_NOT - Integer variable representing disk#
#Don't translate DSK_REQ_SIZE and DKS_CUR_SIZE; 
DSK_SIZE_MSG1="Size of the \#\$DSK_NOT disk is not sufficient. "
DSK_SIZE_MSG2="The required disk size is \$DSK_REQ_SIZE MB and "
DSK_SIZE_MSG3="the size of the disk present is \$DSK_CUR_SIZE MB.  "

NWS_INFO="
 You now have to reinsert the Replicated System Kit installation 
 tape (or the first tape, if there is more than one).

 Please put the tape into the tape drive and press ENTER. 
"
ASKSERVER_CHOOSE="Single server, or no replicas
Multiple servers, replicas exist"
NWS_SERVER_INFO="
 If your system is the only server in the NetWare Directory
 Services (NDS) tree, the installation can finish automatically.

 If your server is one of several, there may be replicas of
 the NDS information that was on this server.  Use these replicas 
 instead of the information on your installation tape.
 See \"Restoring NetWare Directory Services in UnixWare 2.1\"
 for details.

 Indicate whether your NDS tree has more than one server,
 then press ENTER
"
NWS_CHOOSE="NDS tree type"
