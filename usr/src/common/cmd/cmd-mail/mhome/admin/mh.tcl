#ident "@(#)mh.tcl	11.1"

#
# the mh API, files in the virtsers format are editted through this API.
# mhi prefix is for internal routines. mh prefix comprises the API.
#

# our incore data is structured as follows:
# mhi_domain is an array indexed by the virtual domain (in lowercase).
#	each entry in mhi_domain is a list of "virtuser physuser" pairs.
# mhi_file is the name of the file we have copied into memory.

# returns ok, fail, and parserr.
# parserr means that invalid lines were detected and stripped.
#	valid lines are preserved.
proc \
mh_open { file } \
{
	global mhi_file mhi_domain

	if {[info exists mhi_domain]} {
		unset mhi_domain
	}	
	set mhi_file $file

	if {[catch {open $mhi_file r} fd] != 0} {
		return fail
	}

	set errs 0
	while {[gets $fd line] != -1} {
		set line [string tolower $line]
		set list ""
		while {"$line" != ""} {
			lappend list [ctoken line " \t"]
		}
		if {[llength $list] != 2} {
			set errs 1
			continue
		}
		set tmp [lindex $list 0]
		set vuser [ctoken tmp "@"]
		set domain [ctoken tmp "@"]
		if {"$domain" == ""} {
			set errs 1
			continue
		}
		if {"$tmp" != ""} {
			set errs 1
			continue
		}
		set puser [lindex $list 1]
		set list [list $vuser $puser]
		lappend mhi_domain($domain) $list
	}
	close $fd
	# now sort all user lists
	foreach i [array names mhi_domain] {
		set list $mhi_domain($i)
		set list [lsort $list]
		set mhi_domain($i) $list
	}

	if {$errs == 1} {
		return parserr
	}
	return ok
}

# write internal copy of file out to disk.
# returns ok or fail
proc \
mh_write {} \
{
	global mhi_domain mhi_file

	set names [array names mhi_domain]
	set names [lsort $names]

	if {[catch {open $mhi_file w} fd] != 0} {
		return fail
	}

	foreach i $names {
		set list $mhi_domain($i)
		foreach j $list {
			set vuser [lindex $j 0]
			set puser [lindex $j 1]
			puts $fd "$vuser@$i $puser"
		}
	}
	close $fd

	return ok
}

# get a list of the virtual domains found in the virtusers file.
proc \
mh_vd_names_get {} \
{
	global mhi_domain

	set list [array names mhi_domain]
	set list [lsort $list]
	return $list
}

# get the list of users for a given virtual domain.
# returns "" if none for that domain.
# each user is returned as list with two items the first is the alias
# and the second is the system user name, just like the virtusers file.
proc \
mh_vd_users_get { vd } \
{
	global mhi_domain

	if {[info exists mhi_domain($vd)]} {
		set list $mhi_domain($vd)
	} else {
		set list ""
	}
	return $list
}

# Just a set all routine,
# the list is sorted on set.
# only lower case names are expected.
proc \
mh_vd_users_set { vd list } \
{
	global mhi_domain

	set list [lsort $list]
	set mhi_domain($vd) $list
}
