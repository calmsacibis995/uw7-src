#ident	"@(#)debugger:catalog.d/common/GLab.awk	1.4"

# Label support for menu and dialog buttons, toggles, etc.
# This awk script reads the Label table description file GLab.awk.in
# and creates three new files:
#
# gui_label.h
#	enum declaration of the types of labels
#	There is one entry for each non-comment line in GLab.awk.in
#
# GLabel.C
#	The initialization code for the label table,
#	one line per Label type.  Each line looks like:
#		{catalog number, 0, default string}, /* LableId */
#
# GLcatalog
#	Label text, one per line.  This is the input to mkmsgs,
#	which transforms it into the gui-specific label catalog, 
#	debug.label
#
# We implement a simple conditional inclusion mechanism;
# it supports #define foo; #ifdef foo; #ifndef foo; 
# #else; #endif; #undef foo
# The values for foo off by default;  we read a file
# called ../../awk.cond at startup to process any
# outside defines.
#
# The format for entries in GLab.awk.in is:
# LAB_id	label text	menu_name	HAS_MNE|MNE	extra comments&

# !!! NOTE: the field separator is a single tab !!!
# !!! Labels may NOT contain embedded tabs (\t is okay) !!!

# The first two fields are required and specify the
# label id as used in the label enumeration in the gui
# and the label text.  Fields 3 and 4 are used
# in generating comments for the message catalog.  If field
# 3 is non-null, it is used as the menu name for MENU=x comments.
# If field 4 contains HAS_MNE, we generate a MNEM_ID=id comment
# where id is the number of the next message.  If field 4 contains
# MNE, we generate a STR_ID=id comment, where id is the number
# of the previous message.  For this to work, all mnemonic
# labels must directly follow the string label they are
# associated with.  If field 5 is present, it is printed
# out as a separate, extra line of commentary.
# Missing fields should be replaced by a single "-"


BEGIN {
	cur_index = 0
	modes[cur_index] = "on"
	exit_status = 0
	FS="\t"

	f_tab_c = "GLabel.C"
	f_msg_h = "gui_label.h"
	f_cat = "GLcatalog"

	# print the necesary header information to each file
	print "/* file produced by ../../catalog.d/common/GLab.awk */\n" >f_msg_h
	print "#ifndef _GUI_LABEL_H"	>f_msg_h
	print "#define _GUI_LABEL_H\n"	>f_msg_h
	print "enum LabelId\n{"		>f_msg_h
	print "\tLAB_none = 0,"	>f_msg_h

	print "/* file produced by ../../catalog.d/common/GLab.awk\n */"  >f_tab_c
	print "#include \"gui_label.h\"" > f_tab_c
	print "#include \"Label.h\"" > f_tab_c
	print "struct Label labtable[] = {" > f_tab_c
	print "{ 0, 0 },\t/* LAB_none */" >f_tab_c

	# ------ WARNING -------
	# Messages in an existing catalog cannot be modified or removed,
	# because we have no control over the translated catalogs,
	# also, calls to gettxt have hard-coded numbers in them.
	# Messages MUST stay in the same order.
	next_num = 1			# next catalog entry
}

# main loop
# the command line in the makefile is
# 	awk -f GLab.awk GLab.awk.in

{
	# The only lines we are interested in are the ones that
	# start with LAB_
	# or the conditional lines #ifdef, #ifndef, #define, #undef,
	# #else, #endif,

	if ($1 == "#define")
	{
		if (modes[cur_index] == "on")
			defines[$2] = 1
		next
	}
	else if ($1 == "#undef")
	{
		if (modes[cur_index] == "on")
			defines[$2] = 0
		next
	}
	else if ($1 == "#ifdef")
	{
		if (modes[cur_index] == "on")
		{
			cur_index++
			if (defines[$2] == 1)
			{
				modes[cur_index] = "on"
			}
			else
			{
				modes[cur_index] = "off"
			}
		}
		else
		{
			cur_index++
			modes[cur_index] = "skip"
		}
		next
	}
	else if ($1 == "#ifndef")
	{
		if (modes[cur_index] == "on")
		{
			cur_index++
			if (defines[$2] == 0)
			{
				modes[cur_index] = "on"
			}
			else
			{
				modes[cur_index] = "off"
			}
		}
		else
		{
			cur_index++
			modes[cur_index] = "skip"
		}
		next
	}
	else if ($1 == "#else")
	{
		if (cur_index <= 0)
		{
			print("Error: #else with no matching #ifdef")
			exit_status = 1
			exit exit_status
		}
		if (modes[cur_index] == "on")
		{
			# ifdef was true - now skip #else part
			modes[cur_index] = "off"
		}
		else if (modes[cur_index] == "off")
		{
			# ifdef was false - now process #else part
			modes[cur_index] = "on"
		}
		next
	}
	else if ($1 == "#endif")
	{
		if (cur_index <= 0)
		{
			print("Error: #endif with no matching #ifdef")
			exit_status = 1
			exit exit_status
		}
		cur_index--
		next
	}

	else if (modes[cur_index] != "on")
		next

	if (substr($1, 1, 4) != "LAB_")
		next

	msg_num = next_num++
	printf "\t%s,\n", $1 >f_msg_h
	printf "{ %d, 0, \t\"", msg_num >f_tab_c
	mid = $1
	fstring = $2

	printf "%s\"},\t/* %s */\n", fstring, mid > f_tab_c
	printf "%%# %d;", msg_num > f_cat
	if ($3 != "-")
		printf "MENU=%s,", $3 > f_cat
	if ($4 == "MNE")
		printf "STR_ID=%d", msg_num-1 > f_cat
	else if ($4 == "HAS_MNE")
		printf "MNEM_ID=%d", msg_num+1 > f_cat
	printf";\n" > f_cat
	if ($5 != "-")
		printf "%%# %s\n", $5 > f_cat
	print fstring > f_cat
}

END {
	if (exit_status != 0)
		exit exit_status
	if (cur_index > 0)
	{
		print("Error: #ifdef with no matching #endif")
		exit 1
	}
	# finish off the message table files
	print "\tLAB_last\n};\n"	>f_msg_h
	print "#endif // _GUI_LABEL_H"	>f_msg_h

	print "{0}\t/* LAB_last */\n};" >f_tab_c
}
