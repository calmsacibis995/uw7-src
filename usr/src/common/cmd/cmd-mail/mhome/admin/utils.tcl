#ident "@(#)utils.tcl	11.2"
#
# user mhome administration utilities.
#

# our internal catalog routines.
proc \
mhm_setcat { catname } \
{
	global mhm_catname

	set mhm_catname $catname
}

proc \
mhm_msg { msg } \
{
	global mhm_catname

	return [IntlLocalizeMsg ${mhm_catname}_MSG_$msg]
}

proc \
mhm_msg1 { msg arg } \
{
	global mhm_catname

	return [IntlLocalizeMsg ${mhm_catname}_MSG_$msg $arg]
}

proc \
mhm_err { msg } \
{
	global mhm_catname

	return [IntlLocalizeMsg ${mhm_catname}_ERR_$msg]
}

proc \
mhm_err1 { msg arg } \
{
	global mhm_catname

	return [IntlLocalizeMsg ${mhm_catname}_ERR_$msg $arg]
}

# blocking dialog box routine (eyn ErrorDialog yes no)
proc \
mhm_query_eyn { msg arg } \
{
	global app

	set ret [VtErrorDialog $app.err -block \
		-message [mhm_err1 $msg $arg] \
		-ok -okLabel [mhm_msg STR_YES] \
		-apply -applyLabel [mhm_msg STR_NO]]
	switch "$ret" {
	"OK"	 {
		return yes
	}
	"APPLY"	 {
		return no
	}
	}
	error "mhm_query_eyn: Unknown result code"
}

# blocking dialog box routine (eok ErrorDialog yes)
proc \
mhm_query_eok { msg arg } \
{
	global app

	set ret [VtErrorDialog $app.err -block \
		-message [mhm_err1 $msg $arg] \
		-ok -okLabel [mhm_msg STR_OK]]
	if {"$ret" == "OK"} {
		return yes
	}
	error "mhm_query_eok: Unknown result code"
}

# blocking dialog box routine (qync QuestionDialog yes no cancel)
proc \
mhm_query_qync { msg arg } \
{
	global app

	set ret [VtQuestionDialog $app.err -block \
		-message [mhm_msg1 $msg $arg] \
		-ok -okLabel [mhm_msg STR_YES] \
		-apply -applyLabel [mhm_msg STR_NO] \
		-cancel -cancelLabel [mhm_msg STR_CANCEL]]
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
	error "mhm_query_qync: Unknown result code"
}

# blocking dialog box routine (qyn QuestionDialog yes no)
proc \
mhm_query_qyn { msg arg } \
{
	global app

	set ret [VtQuestionDialog $app.err -block \
		-message [mhm_msg1 $msg $arg] \
		-ok -okLabel [mhm_msg STR_YES] \
		-apply -applyLabel [mhm_msg STR_NO]]
	switch "$ret" {
 	"OK"	 {
	 	return yes
 	}
	"APPLY"	 {
		return no
	}
	}
	error "mhm_query_qyn: Unknown result code"
}

# regular error message, just has an ok button.
proc \
mhm_error { msg } \
{
	global app

	VtErrorDialog $app.err -block -ok -message [mhm_err $msg]
}

# regular error message, just has an ok button.
proc \
mhm_error1 { msg arg } \
{
	global app

	VtErrorDialog $app.err -block -ok -message [mhm_err1 $msg $arg]
}

# return union of domains found in virtusers file and those registered
# in the osa.  They are in case-insensitive sorted order.
proc \
mhm_union_domains {} \
{
	set list1 [mhm_remote_aliases]
	set list2 [mh_vd_names_get]

	foreach i $list2 {
		set found 0
		foreach j $list1 {
			if {$i == $j} {
				set found 1
				break
			}
		}
		if {$found == 0} {
			lappend list1 $i
		}
	}
	set list1 [lsort $list1]
	return $list1
}

# remove a system user from all domains
# returns ok if the user was found in at least one domain
# returns fail if the user did not exists in any domains.
proc \
mhm_retire { user } \
{
	set useri [string tolower $user]
	set domains [mhm_union_domains]
	set found fail
	foreach domain $domains {
		set users [mh_vd_users_get $domain]
		set newusers ""
		foreach item $users {
			set puser [lindex $item 1]
			set puser [string tolower $puser]
			if {"$puser" != "$useri"} {
				lappend newusers $item
			}
		}
		if {"$users" != "$newusers"} {
			mh_vd_users_set $domain $newusers
			set found ok
		}
	}
	return $found
}

# rebuild domain map file if needed
#
# copy in the old db file if there, build a new one, compare,
# copy back new one if needed, prompt if in GUI mode.
# put the text file there too for reference only.
# returns ok or fail
proc \
mhm_rebuild_domain_map { prompt } \
{
	global PID MAKEMAP MDOMAIN MDOMAINDB MDOMAINPATH

	# yes, check if domain map is out of date.
	set tmp1 "/tmp/domain.$PID"
	set tmp1db "/tmp/domain.$PID.db"
	set tmp2db "/tmp/domain.1.$PID.db"

	system "rm -f $tmp1 $tmp1db $tmp2db"
	set ret [mhm_remote_copyin $MDOMAINDB $tmp2db]

	# must write out our domains to tmp1
	if {[catch {open $tmp1 w} fd] != 0} {
		return fail
	}
	# only use the ones that really exist
	set domains [mhm_remote_aliases]
	foreach domain $domains {
		set ip [mhm_remote_alias_to_ip $domain]
		if {"$ip" == "fail"} {
			set ip "N/A"
		}
		puts $fd "$domain $ip"
	}
	close $fd
	# now make a map
	if {[file exists $tmp1] == 1} {
		set copy no
		catch {system "$MAKEMAP hash $tmp1 < $tmp1 >/dev/null 2>&1"} ret
		if {$ret != 0} {
			if {"$prompt" == "yes"} {
				VtUnLock
				mhm_error1 NO_MAP $MDOMAINDB
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
			set ret [mhm_query_qyn DOMAIN_UPDATE $MDOMAINDB]
			VtLock
		}
		if {"$ret" == "yes"} {
			# first mkdir
			mhm_remote_mkdir $MDOMAINPATH
			set ret [mhm_remote_copyout $tmp1 $MDOMAIN]
			if {"$ret" == "fail"} {
				if {"$prompt" == "yes"} {
					VtUnLock
					mhm_error1 COPY_BACK $MDOMAIN
					VtLock
				}
				system "rm -f $tmp1 $tmp1db $tmp2db"
				return fail
			}
			set ret [mhm_remote_copyout $tmp1db $MDOMAINDB]
			if {"$ret" == "fail"} {
				if {"$prompt" == "yes"} {
					VtUnLock
					mhm_error1 COPY_BACK $MDOMAINDB
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
