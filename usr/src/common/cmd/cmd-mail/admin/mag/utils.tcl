#ident "@(#)utils.tcl	11.2"
#
# mail administration utilities.
#

# our internal catalog routines.
proc \
mag_setcat { catname } \
{
	global mag_catname

	set mag_catname $catname
}

proc \
mag_msg { msg } \
{
	global mag_catname

	return [IntlLocalizeMsg ${mag_catname}_MSG_$msg]
}

proc \
mag_msg1 { msg arg } \
{
	global mag_catname

	return [IntlLocalizeMsg ${mag_catname}_MSG_$msg [list $arg]]
}

proc \
mag_err { msg } \
{
	global mag_catname

	return [IntlLocalizeMsg ${mag_catname}_ERR_$msg]
}

proc \
mag_err1 { msg arg } \
{
	global mag_catname

	return [IntlLocalizeMsg ${mag_catname}_ERR_$msg [list $arg]]
}

# blocking dialog box routine (eyn ErrorDialog yes no)
proc \
mag_query_eyn { msg arg } \
{
	global app

	set ret [VtErrorDialog $app.err -block \
		-message [mag_err1 $msg $arg] \
		-ok -okLabel [mag_msg STR_YES] \
		-apply -applyLabel [mag_msg STR_NO]]
	switch "$ret" {
	"OK"	 {
		return yes
	}
	"APPLY"	 {
		return no
	}
	"CANCEL" {
		return cancel
	}
	}
	error "mag_query_eyn: Unknown result code: $ret"
}

# blocking dialog box routine (eok ErrorDialog yes)
proc \
mag_query_eok { msg arg } \
{
	global app

	set ret [VtErrorDialog $app.err -block \
		-message [mag_err1 $msg $arg] \
		-ok -okLabel [mag_msg STR_OK]]
	if {"$ret" == "OK"} {
		return yes
	}
	error "mag_query_eok: Unknown result code: $ret"
}

# blocking dialog box routine (qync QuestionDialog yes no cancel)
proc \
mag_query_qync { msg arg } \
{
	global app

	set ret [VtQuestionDialog $app.err -block \
		-message [mag_msg1 $msg $arg] \
		-ok -okLabel [mag_msg STR_YES] \
		-apply -applyLabel [mag_msg STR_NO] \
		-cancel -cancelLabel [mag_msg STR_CANCEL]]
	switch "$ret" {
 	"OK"	 {
	 	return yes
 	}
	"APPLY"	 {
		return no
	}
	"CANCEL" {
		return cancel
	}
	}
	error "mag_query_qync: Unknown result code: $ret"
}

# blocking dialog box routine (qyn QuestionDialog yes no)
proc \
mag_query_qyn { msg arg } \
{
	global app

	set ret [VtQuestionDialog $app.err -block \
		-message [mag_msg1 $msg $arg] \
		-ok -okLabel [mag_msg STR_YES] \
		-apply -applyLabel [mag_msg STR_NO]]
	switch "$ret" {
 	"OK"	 {
	 	return yes
 	}
	"APPLY"	 {
		return no
	}
	}
	error "mag_query_qyn: Unknown result code: $ret"
}

# regular error message, just has an ok button.
proc \
mag_error { msg } \
{
	global app

	VtErrorDialog $app.err -block -ok -message [mag_err $msg]
}

# regular error message, just has an ok button.
proc \
mag_error1 { msg arg } \
{
	global app

	VtErrorDialog $app.err -block -ok -message [mag_err1 $msg $arg]
}

@if notused
# regular warning message, just has an ok button.
proc \
mag_warn { msg } \
{
	global app

	VtWarningDialog $app.err -block -ok -message [mag_msg $msg]
}
@endif

# regular warning message, just has an ok button.
proc \
mag_warn1 { msg arg } \
{
	global app

	VtWarningDialog $app.err -block -ok -message [mag_msg1 $msg $arg]
}

# give up the short version of the default machine name
proc \
mag_short_name_default {} \
{
	global mag_host_name

	set fqdn $mag_host_name
	set list [split $fqdn "."]
	return [lindex $list 0]
}

# convert 1/0 to YES/NO
proc \
mag_num_to_bool num \
{
	if {$num == 0} {
		return [mag_msg STR_NO]
	}
	if {$num == 1} {
		return [mag_msg STR_YES]
	}
	error "Unknown boolean numeric $num"
}

# convert YES/NO to 1/0
proc \
mag_bool_to_num bool \
{
	if {"$bool" == "[mag_msg STR_NO]"} {
		return 0
	}
	if {"$bool" == "[mag_msg STR_YES]"} {
		return 1
	}
	error "Unknown boolean string $bool"
}

# convert TRUE/FALSE to YES/NO
proc \
mag_string_to_bool str \
{
	set str [string toupper $str]
	if {"$str" == "FALSE"} {
		return [mag_msg STR_NO]
	}
	if {"$str" == "TRUE"} {
		return [mag_msg STR_YES]
	}
	error "Unknown boolean string $str"
}

# convert YES/NO to TRUE/FALSE
proc \
mag_bool_to_string bool \
{
	if {"$bool" == "[mag_msg STR_NO]"} {
		return FALSE
	}
	if {"$bool" == "[mag_msg STR_YES]"} {
		return TRUE
	}
	error "Unknown boolean string $bool"
}

#
# routines to assist in message store editting.
#

#
# get current folder location in main screen value format.
#
proc \
mag_ms1_location {} \
{
	global MAILSPOOL

	set inboxdir [ma_ms1_get MS1_INBOX_DIR]
	set inboxname [ma_ms1_get MS1_INBOX_NAME]

	# system spool directory
	if {"$inboxdir" == "$MAILSPOOL" && "$inboxname" == ""} {
		return [mag_msg FOLDER_SYSTEM]
	}
	if {"$inboxdir" == "" && "$inboxname" == ".mailbox"} {
		return [mag_msg FOLDER_HOME]
	}
	return [mag_msg FOLDER_CUSTOM]
}

#
# get current folder format in main screen value format
#
proc \
mag_ms1_format {} \
{
	global MAILSPOOL

	set format [ma_ms1_get MS1_FOLDER_FORMAT]

	# case insensitive compares
	set format [string toupper $format]

	if {"$format" == "SENDMAIL"} {
		return [mag_msg FOLDER_SENDMAIL]
	}
	if {"$format" == "MMDF"} {
		return [mag_msg FOLDER_MMDF]
	}
	error "Unknown folder format: $format"
}

# convert table type from external to internal
proc \
mag_table_to_internal { ttype } \
{
	if {"$ttype" == "[mag_msg TTBADUSER]"} {
		return baduser
	}
	if {"$ttype" == "[mag_msg TTDNS]"} {
		return DNS
	}
	if {"$ttype" == "[mag_msg TTUUCP]"} {
		return UUCP
	}
	if {"$ttype" == "[mag_msg TTREMOTE]"} {
		return remote
	}
	if {"$ttype" == "[mag_msg TTLOCAL]"} {
		return local
	}
	if {"$ttype" == "[mag_msg TTFILE]"} {
		return file
	}
	error "Unknown table type $ttype."
}

# generate our list of valid table values.
proc \
mag_table_enum {} \
{
	set list [list \
		[mag_msg TTBADUSER] \
		[mag_msg TTDNS] \
		[mag_msg TTUUCP] \
		[mag_msg TTREMOTE] \
		[mag_msg TTLOCAL] \
		[mag_msg TTFILE] \
	]
	return $list
}

# convert table type from external to internal
proc \
mag_table_to_external { ttype } \
{
	if {"$ttype" == "baduser"} {
		return [mag_msg TTBADUSER]
	}
	if {"$ttype" == "DNS"} {
		return [mag_msg TTDNS]
	}
	if {"$ttype" == "UUCP"} {
		return [mag_msg TTUUCP]
	}
	if {"$ttype" == "remote"} {
		return [mag_msg TTREMOTE]
	}
	if {"$ttype" == "local"} {
		return [mag_msg TTLOCAL]
	}
	if {"$ttype" == "file"} {
		return [mag_msg TTFILE]
	}
	error "Unknown table type $ttype."
}

proc \
mag_channel_to_alias { chname } \
{
	switch "$chname" {
	"local" {
		return [mag_msg CLOCAL]
	}
	"badhost" {
		return [mag_msg CBADHOST]
	}
	"baduser" {
		return [mag_msg CBADUSER]
	}
	"multihome" {
		return [mag_msg CMHOME]
	}
	}
	# no change
	return $chname
}

proc \
mag_channel_to_internal { chname } \
{
	if {"$chname" == "[mag_msg CLOCAL]"} {
		return local
	}
	if {"$chname" == "[mag_msg CBADHOST]"} {
		return badhost
	}
	if {"$chname" == "[mag_msg CBADUSER]"} {
		return baduser
	}
	if {"$chname" == "[mag_msg CMHOME]"} {
		return multihome
	}
	# no change
	return $chname
}

# rebuild UUCP map file if needed
#
# copy in the uucp systems file, the uucp db file, and rebuild the database
# copy back the database only if it changed.  Put up message if database
# changed so user knows we are rebuilding this file.
# prompt is whether the GUI or cmd line is calling the routine
# returns ok or fail
proc \
mag_rebuild_uucp_map { prompt } \
{
	global PID UUCP UUCPDB MAKEMAP
@if test
	global TEST
@endif

	# check if any UUCP channels
	set channels [mai_ch_names_get]
	set found no
	foreach chname $channels {
		set type [ma_ch_table_type_get $chname]
		if {"$type" == "UUCP"} {
			set found yes
			break
		}
	}
	if {"$found" == "no"} {
		return ok
	}

	# yes, check if UUCP map is out of date.
	set tmp1 "/tmp/systems.$PID"
	set tmp1db "/tmp/systems.$PID.db"
	set tmp2db "/tmp/systems.1.$PID.db"

	system "rm -f $tmp1 $tmp1db $tmp2db"
	set ret [mag_remote_copyin $UUCP $tmp1]
	set ret [mag_remote_copyin $UUCPDB $tmp2db]

	# now make a map
	if {[file exists $tmp1] == 1} {
		set copy no
		catch {system "$MAKEMAP hash $tmp1 < $tmp1 >/dev/null 2>&1"} ret
		if {$ret != 0} {
			if {"$prompt" == "yes"} {
				VtUnLock
				mag_error1 NO_MAP $UUCP
				VtLock
			}
			system "rm -f $tmp1 $tmp1db $tmp2db"
			return fail
		}
		# no previous map, always copy back
		if {[file exists $tmp2db] == 0} {
			set copy yes
		} else {
			catch {system "cmp $tmp1db $tmp2db > /dev/null 2>&1"} tmp
			# files are different
			if {$tmp == 1} {
				set copy yes
			}
		}
		if {"$prompt" == "no"} {
			set copy yes
		}
		if {"$copy" == "no"} {
			system "rm -f $tmp1 $tmp1db $tmp2db"
			return ok
		}
		# prompt if copy is ok.
		set ret yes
		if {"$prompt" == "yes"} {
			VtUnLock
			set ret [mag_query_qyn UUCP_UPDATE $UUCPDB]
			VtLock
		}
		if {"$ret" == "yes"} {
			set ret [mag_remote_copyout $tmp1db $UUCPDB]
@if test
			if {"$TEST" == "utils_test6"} {
				set ret fail
			}
@endif
			if {"$ret" == "fail"} {
				if {"$prompt" == "yes"} {
					VtUnLock
					mag_error1 COPY_BACK $UUCPDB
					VtLock
				}
				system "rm -f $tmp1 $tmp1db $tmp2db"
				return fail
			}
		}
		system "rm -f $tmp1 $tmp1db $tmp2db"
	}
	return ok
}

# given the envelope and header strings build the appropriate cf string.
proc \
mag_build_ruleset { env hdr } \
{
	if {"$hdr" == ""} {
		set value "$env"
	} else {
		if {"$env" != ""} {
			set value "$env/$hdr"
		} else {
			set value "0/$hdr"
		}
	}
	return $value
}

# want to pass this string via system, need to escape any $ chars.
proc \
mag_escape { string } \
{
	set list [split $string "$$"]
	set newlist [join $list "\\$"]
	return $newlist
}

# trim leading zero's from integers to prevent octal comparison
proc \
mag_trim_zero { value } \
{
	set sign ""
	set test $value
	if {"[csubstr $value 0 1]" == "-"} {
		set sign "-"
		set test [csubstr $value 1 end]
	}
	set test [string trimleft $test "0"]
	if {"$test" == ""} {
		set test "0"
	}
	set value "$sign$test"
	return $value
}
