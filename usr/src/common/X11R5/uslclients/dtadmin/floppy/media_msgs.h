#ifndef NOIDENT
#pragma	ident	"%W%"
#endif

/*
 * This file contains all the message strings for the Desktop UNIX
 * dtadmin client MediaMgr.
 * 
 */

#define	string_hasData		"dtmedia:1" FS \
			"Floppy in drive has data; formatting will erase this"
#define	string_unreadDisk	"dtmedia:4" FS \
			"%s in drive %s is unreadable; try another."
#define	string_isFormatted	"dtmedia:5" FS \
			"Floppy in drive %s is already formatted"

#define	string_fmtDOStoUNIX 	"dtmedia:7" FS \
				"Floppy has DOS format; reformat it?"
#define	string_notOwner 	"dtmedia:8" FS \
	"You do not have permission to open media that contain File Systems"

#define	string_doingFmt  	"dtmedia:9" FS "formatting please wait ..."
#define	string_doingMkfs 	"dtmedia:10" FS \
			"Creating file system please wait ..."
#define	string_waitIndex	"dtmedia:15" FS "Creating index; please wait ..."
#define	string_overwrite	"dtmedia:16" FS "Overwrite files if they exist"
#define	string_bdocTitle	"dtmedia:17" FS "Enter a file to backup to"
#define	string_rstTitle		"dtmedia:18" FS "Enter a file to restore from"
#define	string_noneset		"dtmedia:19" FS "No files currently selected"
#define	string_notaBkup		"dtmedia:21" FS "%s is not a backup archive"
#define	string_unreadFile	"dtmedia:22" FS "<%s> is unreadable"
#define	string_newFile		"dtmedia:23" FS \
		"<%s> already exists. Choose a different name for this backup"
#define	string_etc		"dtmedia:25" FS "selected files"
#define	string_badMalloc	"dtmedia:26" FS "Memory allocation error"

#define	string_fmtOp		"dtmedia:27" FS "Formatting"
#define	string_mkfsOp		"dtmedia:28" FS "File system creation"

#define	label_open		"dtmedia:30" FS "Open ..."
#define	label_cancel		"dtmedia:31" FS "Cancel"
#define	label_sched		"dtmedia:32" FS "Schedule ..."
#define	label_backup		"dtmedia:33" FS "Backup"
#define	label_restore		"dtmedia:34" FS "Restore"
#define	label_format		"dtmedia:35" FS "Format"
#define	label_rewind		"dtmedia:36" FS "Rewind"
#define	label_reset		"dtmedia:37" FS "Reset"
#define	label_file		"dtmedia:38" FS "File"
#define	label_help		"dtmedia:39" FS "Help"
#define	label_type		"dtmedia:40" FS "Type"
#define	label_insert		"dtmedia:41" FS "Insert"
/* label_log defined later, for cataloging, changed strings should
   have a new tag: label_log "dtmedia:504" FS "Create Backup Log" */

#define	label_doc		"dtmedia:44" FS "File"
#define	label_action		"dtmedia:45" FS "Actions"
#define	label_exit		"dtmedia:46" FS "Exit"
#define	label_setdoc		"dtmedia:47" FS "Set File"
#define	label_immed		"dtmedia:48" FS "Immediate"
#define	label_save		"dtmedia:49" FS "Save"
#define	label_saveas		"dtmedia:50" FS "Save As ..."
#define	label_copy		"dtmedia:51" FS "Copy ..."
#define	label_delete		"dtmedia:52" FS "Erase"
#define	label_convert		"dtmedia:53" FS "Convert on Copy"
#define	label_noconvert		"dtmedia:54" FS "Don't Convert"
#define	label_selectF		"dtmedia:55" FS "Copy"
#define	label_continue		"dtmedia:56" FS "Continue"
#define	label_overwrite		"dtmedia:57" FS "Overwrite"

#define	mnemonic_selectF	"dtmedia:59" FS "C"
#define	mnemonic_open		"dtmedia:60" FS "O"
#define	mnemonic_cancel		"dtmedia:61" FS "C"
#define	mnemonic_sched		"dtmedia:62" FS "S"
#define	mnemonic_backup		"dtmedia:63" FS "B"
#define	mnemonic_restore	"dtmedia:64" FS "R"
#define	mnemonic_format		"dtmedia:65" FS "F"
#define	mnemonic_reset		"dtmedia:66" FS "R"
#define	mnemonic_file		"dtmedia:67" FS "F"
#define	mnemonic_help		"dtmedia:68" FS "H"
#define	mnemonic_type		"dtmedia:69" FS "T"
#define	mnemonic_insert		"dtmedia:70" FS "I"
#define	mnemonic_log		"dtmedia:71" FS "A"
#define	mnemonic_action		"dtmedia:72" FS "A"
#define	mnemonic_exit		"dtmedia:73" FS "X"
#define	mnemonic_immed		"dtmedia:74" FS "I"
#define	mnemonic_save		"dtmedia:75" FS "S"
#define	mnemonic_saveas		"dtmedia:76" FS "V"
#define	mnemonic_copy		"dtmedia:77" FS "P"
#define	mnemonic_delete		"dtmedia:78" FS "R"
#define	mnemonic_continue	"dtmedia:79" FS "O"

#define	label_highDns		"dtmedia:80" FS "HIGH"
#define	label_lowDns		"dtmedia:81" FS "LOW"

#define	label_bkupFmt		"dtmedia:82" FS "Backup Use"
#define	label_s5Fmt		"dtmedia:83" FS "Desktop Folder"
#define	label_dosFmt		"dtmedia:84" FS "DOS Format"

#define	mnemonic_highDns	"dtmedia:85" FS "G"
#define	mnemonic_lowDns		"dtmedia:86" FS "W"

#define	mnemonic_bkupFmt	"dtmedia:87" FS "B"
#define	mnemonic_s5Fmt		"dtmedia:88" FS "D"
#define	mnemonic_dosFmt		"dtmedia:89" FS "M"

#define	label_devCaption	"dtmedia:90" FS "Device: "
#define	label_dnsCaption	"dtmedia:91" FS "Density: "
#define	label_fmtCaption	"dtmedia:92" FS "Type: "

#define	label_targetCaption	"dtmedia:99" FS "Target File: "
#define	label_filesCaption	"dtmedia:100" FS "Files: "

#define	label_bkupToCaption	"dtmedia:101" FS "Backup To: "
#define	label_bkupTypeCaption	"dtmedia:102" FS "Backup Type: "
#define	label_bkupClassCaption	"dtmedia:604" FS "Backup Data: "
#define	label_rstFromCaption	"dtmedia:104" FS "Restore From: "

#define	label_systemClass	"dtmedia:105" FS "Full System"
#define	label_selfClass		"dtmedia:106" FS "Personal        "
#define	label_userClass		"dtmedia:107" FS "Other Users"
#define	label_ndsData		"dtmedia:608" FS "Directory Services"
#define	label_complType		"dtmedia:108" FS "Complete       "
#define	label_incrType		"dtmedia:109" FS "Incremental"
#define	label_selectFiles	"dtmedia:110" FS "Selected Files"
#define	label_select		"dtmedia:111" FS "Select All"
#define	label_unselect		"dtmedia:112" FS "Unselect All"
#define	label_show		"dtmedia:113" FS "Show Files"
#define	label_edit		"dtmedia:114" FS "Edit"
#define	label_exclude		"dtmedia:115" FS "Exclude"
#define	label_groupCaption	"dtmedia:116" FS "Files & Folders:"
#define	label_prev_group	"dtmedia:117" FS "Prev Group"
#define	label_next_group	"dtmedia:118" FS "Next Group"

#define	mnemonic_systemClass	"dtmedia:120" FS "Y"
#define	mnemonic_selfClass	"dtmedia:121" FS "P"
#define	mnemonic_userClass	"dtmedia:122" FS "T"
#define	mnemonic_ndsData	"dtmedia:605" FS "D"
/* defined later:  mnemonic_complType	"dtmedia:515" FS "C" */
#define	mnemonic_incrType	"dtmedia:124" FS "N"
#define	mnemonic_selectFiles	"dtmedia:125" FS "S"
#define	mnemonic_select		"dtmedia:126" FS "S"
#define	mnemonic_unselect	"dtmedia:127" FS "U"
#define	mnemonic_show		"dtmedia:128" FS "H"
#define	mnemonic_edit		"dtmedia:129" FS "E"
#define	mnemonic_exclude	"dtmedia:130" FS "E"
#define	mnemonic_convert	"dtmedia:131" FS "V"
#define	mnemonic_noconvert	"dtmedia:132" FS "N"

#define	label_convertCaption	"dtmedia:140" FS "UNIX <-> DOS:"
#define	label_parentdir		"dtmedia:141" FS "previous directory"

#define	string_UtoMcopy		"dtmedia:150" FS \
			"UNIX file(s) copied to DOS floppy"
#define	string_MtoUcopy		"dtmedia:151" FS \
			"file(s) copied from DOS floppy to UNIX System."
#define	string_UtoMerror	"dtmedia:152" FS \
			"error copying UNIX file(s) to DOS floppy"
#define	string_cantCopy		"dtmedia:153" FS "Unable to copy DOS file(s)"
#define	string_cantDelete	"dtmedia:154" FS "Unable to delete DOS file(s)"

#define	string_cpyTitle		"dtmedia:156" FS "Copy DOS Files"
#define	string_cpyPrompt	"dtmedia:157" FS "Choose the destination folder"
#define	string_cantList		"dtmedia:158" FS \
			"Unable to read DOS directory"
#define	string_DOStitle		"dtmedia:159" FS "DOS Floppy - %s"

#define	string_fmtVolume	"dtmedia:160" FS "%s number %d"
#define	string_callSched	"dtmedia:161" FS \
			"Invoking Task Scheduler utility"
#define	string_cantIndex	"dtmedia:162" FS \
			"Error indexing files for backup - cancelling"
#define	string_doingBkup	"dtmedia:163" FS \
			"Creating backup archive; please wait ..."
#define	string_doingRst		"dtmedia:164" FS \
			"Restore in progress; please wait ..."
#define	string_savedAs		"dtmedia:165" FS "Backup script saved as %s"
#define	string_notScript	"dtmedia:166" FS "Not a backup script file"
#define	string_readIndex	"dtmedia:167" FS \
			"Reading contents; please wait ..."
#define	string_startBkup	"dtmedia:168" FS "Starting backup at %s"
#define	string_skip		"dtmedia:169" FS "Skipping older file:\n\n%s"

#define	label_toc		"dtmedia:170" FS "Table of Contents..."
#define	label_hlpdsk		"dtmedia:171" FS "Help Desk..."
#define	label_bkrst		"dtmedia:172" FS "Backup-Restore..."
#define	label_fmtHlp		"dtmedia:173" FS "Format..."
#define	label_dosHlp		"dtmedia:174" FS "DOS Floppy..."
#define	mnemonic_toc		"dtmedia:175" FS "T"
#define	mnemonic_hlpdsk		"dtmedia:176" FS "K"
#define	mnemonic_fmtHlp		"dtmedia:177" FS "F"
#define	mnemonic_bkrst		"dtmedia:178" FS "B"
#define	mnemonic_dos		"dtmedia:179" FS "D"

#define	help_intro		"dtmedia:180" FS "10"

#define	string_mountErr		"dtmedia:190" FS "Unable to mount %s"
#define	string_umountErr	"dtmedia:191" FS "Unable to unmount %s"
#define	string_noRoom		"dtmedia:192" FS \
			"Insufficient space on %s for %s"
#define	string_cantOpen		"dtmedia:193" FS "Unable to open folder %s"
#define	string_skipFile		"dtmedia:194" FS "Skipping older file:\n\n%s"
#define	string_dosDirCpy	"dtmedia:195" FS \
	"Cannot copy DOS directory %s; double click to see files."
#define	string_genMedia		"dtmedia:196" FS "media volume"

#define	title_doingBkup		"dtmedia:200" FS "Backup in Progress"
#define	title_doingRst		"dtmedia:201" FS "Restore in Progress"
#define	title_doingFmt		"dtmedia:202" FS "Format in Progress"
#define	title_confirmBkup	"dtmedia:203" FS "Backup:  Confirmation Notice"
#define	title_confirmRst	"dtmedia:204" FS "Restore:  Confirmation Notice"
#define	title_confirmFmt	"dtmedia:205" FS "Format:  Confirmation Notice"
#define	title_bkupUsers		"dtmedia:206" FS "Backup:  User List"
#define	title_bkupOpen		"dtmedia:207" FS "Backup:  Open Script"
#define	title_bkupSave		"dtmedia:208" FS "Backup:  Save Script"
#define	title_confirmUmt	"dtmedia:209" FS "Exit:  Confirmation"
#define	title_confirmIns	"dtmedia:210" FS "Exit:  Insert Media"

#define	string_reInsMsg		"dtmedia:211" FS \
				"Re-insert media in drive %s"
#define	string_busyMsg		"dtmedia:212" FS \
    "%s is still in use.  Click Exit to close the %s folder window or click " \
    "Cancel to leave it open."
#define	string_selected		"dtmedia:213" FS \
	"Selected item(s): %d                       Total item(s): %d"


#define	label_selectAll		"dtmedia:220" FS "Select All Files"
#define	label_unselectAll	"dtmedia:221" FS "Unselect All"
#define label_createDir		"dtmedia:222" FS "Create Directory..."
#define label_createOpen	"dtmedia:223" FS "Create & Open"
#define label_create		"dtmedia:224" FS "Create"
#define label_name		"dtmedia:225" FS "Name:"
#define label_help3dot		"dtmedia:226" FS "Help..."

#define mnemonic_selectAll	"dtmedia:230" FS "S"
#define mnemonic_unselectAll	"dtmedia:231" FS "U"
#define mnemonic_createDir	"dtmedia:232" FS "C"
#define mnemonic_createOpen	"dtmedia:233" FS "O"
#define mnemonic_create		"dtmedia:234" FS "R"
#define mnemonic_name		"dtmedia:235" FS "N"

#define title_createDir		"dtmedia:240" FS "File: Create DOS Directory"
#define string_cantCopyDirs	"dtmedia:242" FS "Cannot copy directories.  You must open a directory to copy its contents."
#define string_copyFilesOnly	"dtmedia:243" FS "Cannot copy special files or directories.  You must open a directory to copy its contents."
#define string_cantCopySpecial	"dtmedia:244" FS "Cannot copy special files."
#define string_fillInName	"dtmedia:245" FS "Must provide a Directory Name."
#define	string_DOSdelete	"dtmedia:246" FS "Selected item(s) erased"
#define title_eraseConfirm	"dtmedia:247" FS "File: Erase"
#define string_eraseConfirm	"dtmedia:248" FS "Erase selected item(s).  Are you sure?"
#define string_nothingErased	"dtmedia:249" FS "Erase canceled."
#define string_rmdirFailed	"dtmedia:251" FS "Could not erase selected directory(ies).  Directory(ies) might not be empty or floppy might be write-protected."
#define string_mixedEraseFailed	"dtmedia:252" FS "Could not erase selected items.  Directory(ies) might not be empty or floppy might be write-protected."
#define title_errorNotice	"dtmedia:253" FS "DOS Error"
#define	string_filesRenamed	"dtmedia:254" FS "Warning: file name(s) truncated when copied to DOS floppy"
#define string_UtoMcopyFailed	"dtmedia:255" FS "Could not copy selected file(s) to DOS floppy"
#define string_MtoUcopyFailed	"dtmedia:256" FS "Could not copy selected file(s) to UNIX System"
#define string_fileEraseFailed	"dtmedia:257" FS "Could not erase selected file(s)"
#define string_dirCreateFailed	"dtmedia:258" FS "Could not create directory on DOS floppy"
#define string_findErrors	"dtmedia:259" FS "%s finished but some item(s) could not be backed-up"

#define title_mounted		"dtmedia:270" FS "Backup: Media Unavailable"
#define	string_mounted		"dtmedia:271" FS \
    "Error: The floppy is currently in use.  " 
#define title_in_use		"dtmedia:272" FS "Backup: Overwrite Data?"
#define	string_in_use		"dtmedia:273" FS \
    "Media in drive has data. Overwrite?"
#define	string_unknownErr	"dtmedia:274" FS \
    "Transient error in writing media. Please try again."

#define	mnemonic_overwrite	"dtmedia:280" FS "O"

#define string_wrongDensity	"dtmedia:281" FS "Formatting wrong density; please check"
#define	string_scheduling	"dtmedia:283" FS "Schedule"
#define	string_eraseFailed	"dtmedia:284" FS "Erase failed"
#define	string_bkupResError	"dtmedia:285" FS "Backup-Restore: Error"

/*  Note:  To support the new feature for the 2.88 floppy drive, we
 *	   need to reserve the indexes range from 290 to 300 in this header
 *         file.  The actual messages for the 2.88 floppy support are
 *	   defined in mkdtab.h.  
 *        
 *         <<<<   SO STARTING THE NEXT MESSAGE FROM INDEX   301  >>>>>
 */

#define	help_bkup_win 		"dtmedia:301" FS "20"
#define label_prop		"dtmedia:302" FS "Properties..."
#define mnemonic_prop		"dtmedia:303" FS "P"
#define label_apply		"dtmedia:304" FS "Apply"
#define mnemonic_apply		"dtmedia:305" FS "A"
#define string_propLine		"dtmedia:306" FS "Format: Properties"
#define label_fs		"dtmedia:307" FS "File System Type:"
#define string_propfile		"dtmedia:308" FS "Unable to read the file system table"
#define label_ok		"dtmedia:309" FS "OK"
#define mnemonic_ok		"dtmedia:310" FS "O"

/*  Note:  To support the new feature on vxfs, we need to reserve the index
 *	   range from 400 to 499 in this header file.  The actual messages
 *	   for the vxfs support are defined in /usr/X/desktop/MediaMgr/FsTable.
 *
 *	   <<<  SO STARTING THE NEXT MESSAGES FROM INDEX   500  >>>>
 *
*/ 

#define string_dosExecFailed	"dtmedia:500" FS "Could not execute DOS command"
#define	string_DOSexec		"dtmedia:501" FS "DOS command is executed"
#define string_cantExecDOS	"dtmedia:502" FS "Cannot execute DOS command"
#define string_dosNotFound	"dtmedia:503" FS "DOS not found"

#define	label_log		"dtmedia:504" FS "Create Backup Log"
#define	label_local		"dtmedia:505" FS "Backup Local Files Only         "
#define	mnemonic_local		"dtmedia:506" FS "L"
#define	label_now		"dtmedia:507" FS "Backup Now"
#define	mnemonic_now		"dtmedia:508" FS "W"
#define	label_later		"dtmedia:509" FS "Backup Later ..."
#define	mnemonic_later		"dtmedia:510" FS "R"
#define	label_gotoRes		"dtmedia:511" FS "Go to Restore"
#define	mnemonic_gotoRes	"dtmedia:512" FS "G"
#define	label_gotoBack		"dtmedia:513" FS "Go to Backup"
#define	mnemonic_gotoBack	"dtmedia:514" FS "G"
#define	mnemonic_complType	"dtmedia:515" FS "C"

#define	help_bkup_open		"dtmedia:516" FS "170"	
#define	help_bkup_save		"dtmedia:517" FS "150"	
#define	help_bkup_confirm	"dtmedia:518" FS "70"	
#define	help_rst_intro		"dtmedia:519" FS "120"	
#define	help_rst_doing		"dtmedia:520" FS "130"
#define	help_format		"dtmedia:521" FS "20"
#define	help_dos_intro		"dtmedia:522" FS "80"
#define	help_dos_copy		"dtmedia:523" FS "80"
#define	help_user_list		"dtmedia:524" FS "30"
#define	help_bkup_doing		"dtmedia:525" FS "90"
#define	help_overwrite		"dtmedia:526" FS "80"
#define	install			"dtmedia:527" FS "Install"
#define	diskette1		"dtmedia:528" FS "diskette1"
#define	diskette2		"dtmedia:529" FS "diskette2"
#define	ctape1			"dtmedia:530" FS "ctape1"
#define	string_readTitle	"dtmedia:531" FS "Showing Files"
#define	ctape2			"dtmedia:532" FS "ctape2"
#define	string_privRstButton	"dtmedia:535" FS "Restore privileges on files"
#define	string_restoringPrivs	"dtmedia:536" FS "Restoring privileges..."
#define	string_complBkupSumm 	"dtmedia:537" FS "Complete backup to %s: %s"
#define	string_incrBkupSumm 	"dtmedia:538" FS "Incremental backup to %s: %s"
#define	string_selBkupSumm 	"dtmedia:539" FS "Selected Files backup to %s"
#define	string_ndsBkupSumm 	"dtmedia:606" FS "Local Directory Services partitions backup to %s"
#define	string_bkupKilled	"dtmedia:540" FS "Backup cancelled"
#define	string_restKilled	"dtmedia:541" FS "Restore cancelled"
#define	string_fmtKilled	"dtmedia:542" FS "Formatting cancelled"
#define	saveCantWriteScript	"dtmedia:543" FS "Save failed: cannot write to file \"%s\""
#define	schedCantWriteScript	"dtmedia:544" FS "Scheduling failed: cannot write to file \"%s\""
#define	string_fmtOK		"dtmedia:545" FS "Formatting complete"
#define	string_mkfsOK		"dtmedia:546" FS "File system creation complete"
#define	string_bkupOK		"dtmedia:547" FS "Backup complete"
#define	string_restOK		"dtmedia:548" FS "Restore complete"
#define	string_fmtFailed	"dtmedia:549" FS "Formatting failed"
#define	string_mkfsFailed	"dtmedia:550" FS "File system creation failed"
#define	string_openFailed	"dtmedia:551" FS "Open... failed"
#define	string_bkupFailed	"dtmedia:552" FS "Backup failed"
#define	string_saveFailed	"dtmedia:553" FS "Save Script failed"
#define	string_fmtStderr	"dtmedia:554" FS "MediaMgr: format: %s"
#define	string_mkfsStderr	"dtmedia:555" FS "MediaMgr: mkfs: %s"
#define string_ins2Msg		"dtmedia:556" FS "Insert %s in drive and click the %s button"
#define	string_rstSummary	"dtmedia:557" FS "Restore files from %s"
#define	string_cantWrite	"dtmedia:558" FS \
			"Unable to write to %s; check write-protection"
#define	help_fproperties	"dtmedia:559" FS "45"
#define	string_formatFailed	"dtmedia:560" FS "Formatting failed.\nThe possible causes may include a bad diskette, a diskette that is incompatible with density selected or a diskette that is write protected."
#define	string_errTitle		"dtmedia:561" FS "MediaMgr: Error"
#define string_noSelectedFiles	"dtmedia:562" FS "Select the files (and folders) to back up.\n\nTo select a file, open the folder containing the file then\ndrag-and-drop the file onto the box in the backup window.\n\nTo remove a file from the backup window, select the file and\nclick on Edit=>Exclude."
#define string_noUpdate		"dtmedia:563" FS "Backup not performed: None of the files that were to be backed\nup were modified since your last backup.\n\nTo force a backup, set the Backup Type to Complete."
#define string_infoTitle	"dtmedia:564" FS "Backup: Information"
#define string_restFail		"dtmedia:565" FS "Restore not completed.\n\nThe possible causes may include no space left on the file system or could not write to the file system."
#define label_continueOp	"dtmedia:566" FS "Continue"
#define mnemonic_continueOp	"dtmedia:567" FS "C"
#define label_dontContinue	"dtmedia:568" FS "Discontinue"
#define mnemonic_dontContinue	"dtmedia:569" FS "D"
#define title_namePrompt 	"dtmedia:570" FS "File name(s) may be truncated or changed. Also, '.' files will not be copied."
#define label_dontOverwrite	"dtmedia:571" FS "Don't Overwrite"
#define mnemonic_dontOverwrite	"dtmedia:572" FS "D"
#define label_overwriteNotice 	"dtmedia:573" FS "The file `%s' already exists.\n\nDo you want to overwrite it?"
#define title_fileop		"dtmedia:574" FS "Folder"
#define msg_failedFileOp	"dtmedia:575" FS "Error: Cannot copy %s to %s"
#define string_formatError	"dtmedia:576" FS "Format: Error"
#define string_restoreError	"dtmedia:577" FS "Restore: Error"
#define string_backupError	"dtmedia:578" FS "Backup: Error"
#define	string_noRoom2		"dtmedia:579" FS "Insufficient space on device:  %s"
#define string_showFail		"dtmedia:580" FS "Cannot Show Files.\n\nAn error occurred reading your backup.\nPossible cause: defective or damaged tape or diskette."
#define string_bkupError	"dtmedia:581" FS "Backup failed.\nThe possible causes may include a bad media (e.g. diskette, cartridge tape, etc.) or media that is write protected."
#define	busyMntDisk		"dtmedia:582" FS \
    "%s is still in use.\n\nIMPORTANT:  Do not remove the diskette while in " \
    "use.\n\nIf you click Close to close this window, you must do the " \
    "following after the diskette is no longer in use:\n\n" \
    "Open the %s icon in the Disks-etc folder and then close the " \
    "resulting window, otherwise the files on the next diskette you insert " \
    "may be lost.\n\nClick Cancel in this window to leave the %s folder " \
    "open (recommended)."
#define	busyMntCD		"dtmedia:583" FS \
    "%s is still in use.\n\nIMPORTANT:  Do not remove the CD while in " \
    "use.\n\nIf you click Close to close this window, you must do the " \
    "following after the CD is no longer in use:\n\n" \
    "Open the %s icon in the Disks-etc folder and then close the " \
    "resulting window.  Otherwise, the information in the " \
    "CD folder will be incorrect for the next CD you insert.\n\n" \
    "Click Cancel in this window to leave the %s folder open (recommended)."
#define label_close		"dtmedia:584" FS "Close"
#define mnemonic_close		"dtmedia:585" FS "L"
#define	ctape3			"dtmedia:586" FS "ctape3"
#define	ctape4			"dtmedia:587" FS "ctape4"
#define	ctape5			"dtmedia:588" FS "ctape5"
#define please_wait		"dtmedia:589" FS "Checking device (%s)\nPlease wait ..."
#define string_fmountWarn	"dtmedia:601" FS "IMPORTANT: Do not remove the floppy disk until you close the Folder: %s.\n\nIf you switch diskettes without closing the folder, UnixWare will not recognize the change and will use and display out-of-date information.  This could destroy the files on your diskette."
#define string_mountTitle	"dtmedia:591" FS "%s: Warning"
#define string_cmountWarn	"dtmedia:602" FS "IMPORTANT: Do not remove the CD until you close the Folder: %s.\n\nIf you switch CD's without closing the folder, UnixWare will not recognize the change and will display and use out-of-date information."
#define label_delWarn		"dtmedia:593" FS "Delete Warning"
#define mnemonic_delWarn	"dtmedia:594" FS "D"
#define string_fullWarn			"dtmedia:603" FS "IMPORTANT: A Full System backup cannot be used to restore your entire system if it becomes unusable.  Create an  emergency recovery tape and diskette to guard against such failures.  See \"Recovering Your System\" in the System Owner's Handbook for details.\n\nA Full System backup includes NetWare Volumes but not NetWare Directory Services (NDS) partitions.  To back up NDS select  \"Directory Services\".\n\nFiles and directories listed in /etc/Ignore will not be backed up."
#define	string_rstNDSSummary	"dtmedia:607" FS "Restore Local Directory Services partitions from %s"
#define	string_NDSshowfile		"dtmedia:610" FS "You are attempting to restore a NetWare Directory Service archive.  The Show Files option is not available when restoring a NetWare Directory Services archive."
#define	string_NDSnwsDown		"dtmedia:611" FS "NetWare Services must be running in order to backup Directory Services.  See \"Supervising the Network\" for information on starting NetWare Services"
#define	string_NDSnwsFail		"dtmedia:612" FS "NetWare Services error: See See your NetWare documentation to ensure that your NetWare server is properly configured."
#define	string_NDSmedFail		"dtmedia:613" FS "Backup Local partitions of Directory Services failed: media failed"
#define	string_WarnTitle		"dtmedia:614" FS "Warning"
#define	string_NDSInstall		"dtmedia:615" FS "The archive you are attempting to restore is an NetWare Directory Services Archive.  Your system is not configured as a NetWare Server.  This restore will not be performed."
#define	string_cpioWarnings		"dtmedia:616" FS "Backup Complete: warnings written to %s"
