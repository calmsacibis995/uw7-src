#ident "@(#)vdomains.tcl	11.1"
#
# private little utility routine to parse output of ifconfig -a
# and produce list of virtual domains and their ip addresses.
#
# this program is rexec'd via the rcmd OSA from the multihome admin GUI.
#

set RESOLVER /usr/sbin/host
set IFCONFIG /usr/sbin/ifconfig

proc \
do_ifconfig {} \
{
	global IFCONFIG

	set ret [catch {exec $IFCONFIG -a} output]
	if {$ret != 0} {
		return ""
	}
	set lines [split $output "\n"]

	# looking for two kinds of lines only, inet and (alias)
	set inet ""
	set alias ""
	foreach line $lines {
		if {"[csubstr $line 0 6]" == "\tinet "} {
			set list [split $line " "]
			set item [lindex $list 1]
			lappend inet $item
		}
		if {"[csubstr $line 0 9]" == "\t(alias) "} {
			set list [split $line " "]
			set item [lindex $list 2]
			lappend alias $item
		}
	}
	# now remove from alias list everything that was found in inet
	set newlist [lindex [intersect3 $inet $alias] 2]
	return $newlist
}

# take a list of ip addresses and convert them into "name ip" pairs.
# drop any that cannot be resolved.
proc \
do_resolve { list } \
{
	global RESOLVER

	set newlist ""
	foreach item $list {
		set ret [catch {exec $RESOLVER $item} output]
		if {$ret != 0} {
			continue
		}
		set lines [split $output "\n"]
		# looking for Name field
		foreach line $lines {
			if {"[csubstr $line 0 6]" == "Name: "} {
				set value [csubstr $line 6 end]
				set value [string tolower $value]
				set newitem [list $value $item]
				lappend newlist $newitem
				break
			}
		}
	}
	return $newlist
}

proc \
main {} \
{
	set list [do_ifconfig]
	set list [do_resolve $list]
	foreach item $list {
		echo $item
	}
}

main
