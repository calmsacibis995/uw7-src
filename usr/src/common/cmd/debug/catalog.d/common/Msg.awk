#ident	"@(#)debugger:catalog.d/common/Msg.awk	1.10"
# $Copyright: $
#Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990, 1991
#Sequent Computer Systems, Inc.   All rights reserved.
# 
#This software is furnished under a license and may be used
#only in accordance with the terms of that license and with the
#inclusion of the above copyright notice.   This software may not
#be provided or otherwise made available to, or used by, any
#other person.  No title to or ownership of the software is
#hereby transferred.

# This awk script reads the Message table description file Msg.awk.in
# and Signature.h and creates ten new files
# containing declarations or C++ code:
#
# Msgtypes.h
#	enum declaration of the types of messages
#	There is one entry for each non-comment line in Msg.awk.in
# Msgtypes.C
#	message type name table (for internal debugging purposes)
#	There is one string for each message id in Msgtypes.h
#
# Mtable.h
#	The array/structure initialization code for the message table,
#	one line per Message type.  Each line looks like:
#		{msg_class, signature, catalog number, 0, format string}, /* Msg_id */
#	The catalog number will be zero if the format string does not need
#	to be translated (i.e. "%s\n")
#	The format string will be absent if the message does not
#	produce any output
#
# Mcatalog
#	Message text, one per line.  This is the input to mkmsgs,
#	which transforms it into the message catalog, uxdebug
#
# Unbundle.h:
#	function prototypes for the Message::unbundle functions
#	example: void unbundle(char *&, Word &);
#	there is one unbundle function for each signature
#
# Unbundle.C:
#	The Message::unbundle functions.
#	Each function does the following:
#		Checks that the signature for the message matches the
#			function's signature
#		Checks that the message's state is appropriate for unpacking
#		Picks apart the message data, and assigns the values to
#			the pointers passed to the function.
#		Checks that the length of the data unpacked matches the
#			message length
#
# Mformat.h:
#	The code for Message::format that unbundles and formats
#	a message.  This is a big switch statement with one case
#	per signature
#	example:
#		case SIG_str:
#			unbundle(s1);
#			i = sprintf(buf, fmt, s1);
#			break;
#
# Sigtable.h:
#	table entries used to bundle the messages, the first field is the
#	number of fields, the second is a bit map with the bits set only
#	if the field is a string
#	example:  {2, SET(1)},
#
# print.h:
#	function prototypes for the printm, printe, and query routines
#	examples:  void printm(Msg_id, const char *, Word);
#		   void printe(Msg_id, Severity, Word);
#		   int query(Msg_id, int, const char *);
#
# print.C:
#	The printm, printe, and query functions.
#	Each function does the following:
#		Checks that the signature for the given Msg_id matches the
#			function's signature
#		Checks that each string argument is non-null
#		Sends the message
#		Logs the message, if logging is on
#		Exits if the message is a fatal error

# The awk script makes one pass over the input, putting out the Msgtypes.h,
# Mtable.h, Mcatalog, and Sigtable.h files.  In this pass it also lists which
# signatures have been used for informational messages (printm's),
# which for error messages (printe's), and which for queries.
# It then produces the other files, which are dependent on the signatures

# We implement a simple conditional inclusion mechanism;
# it supports #define foo; #ifdef foo; #ifndef foo; 
# #else; #endif; #undef foo
# The values for foo off by default;  we read a file
# called ../../awk.cond at startup to process any
# outside defines.
#
# The format for entries in Msg.awk.in is:
# msg_id	translate?	message text	parameter description	extra comments

# !!! NOTE: the field separator is a single tab !!!
# !!! Messages may NOT contain embedded tabs (\t is okay) !!!

# The first 3 fields are required and specify the
# msg id as used in the Msg_id enumeration, yes or no,
# indicating whether or not the message should be translated,
# and the printf format string for the message.
# The fourth and fifth fields appear as comment lines
# in the mkmsgs message catalog we generate (GMcatalog).
# The fourth field contains a description of each parameter
# in a message.  The fifth field, if present, is printed
# as an extra line of commentary.  Missing fields should
# be replaced by a single "-".

BEGIN {
	FS="\t"
	f_tab_c = "Mtable.c"
	f_msg_h = "Msgtypes.h"
	f_msg_c = "Msgtypes.C"
	f_cat = "Mcatalog"
	f_unb_c = "Unbundle.C"
	f_unb_h = "Unbundle.h"
	f_sigtab_h = "Sigtable.h"
	f_printc = "print.C"
	f_printh = "print.h"
	f_format = "Mformat.h"
	cur_index = 0
	modes[cur_index] = "on"
	exit_status = 0

	# print the necesary header information to each file
	print "/* file produced by ../../catalog.d/common/Msg.awk */\n" >f_msg_h
	print "#ifndef _Msgtypes_h"	>f_msg_h
	print "#define _Msgtypes_h\n"	>f_msg_h
	print "enum Msg_id\n{"		>f_msg_h
	print "\tMSG_invalid = 0,"	>f_msg_h

	print "/* file produced by ../../catalog.d/common/Msg.awk */\n" >f_msg_c
	print "const char *Msg_type_names[] =\n{"		>f_msg_c
	print "\t\"MSG_invalid\","	>f_msg_c

	print "/* file produced by ../../catalog.d/common/Msg.awk\n */"  >f_tab_c
	print "#include \"Msgtable.h\"" > f_tab_c
	print "struct Msgtab mtable[] = {" > f_tab_c
	print "{MSGCL_invalid, SIG_invalid},\t/* Msg_invalid */" >f_tab_c

	print "// file produced by ../../catalog.d/common/Msg.awk\n" >f_printh
	print "#ifndef _Word_"				>f_printh
	print "#define _Word_"				>f_printh
	print "typedef unsigned long Word;"		>f_printh
	print "#endif\n"				>f_printh
	print "#ifndef __cplusplus"			>f_printh
	print "overload printe;"			>f_printh
	print "overload printm;"			>f_printh
	print "overload query;"				>f_printh
	print "#endif"					>f_printh
	print "void printe(Severity, const char * ...);\n" >f_printh

	print "// file produced by ../../catalog.d/common/Msg.awk\n" >f_printc
	print "#include \"Interface.h\""		>f_printc
	print "#include \"Signature.h\""		>f_printc
	print "#include \"global.h\""			>f_printc
	print "#include \"utility.h\""			>f_printc
	print "#include \"Msgtab.h\""			>f_printc
	print "#include \"UIutil.h\""			>f_printc
	print "#include \"libint.h\""			>f_printc
	print "#include \"Manager.h\"\n"		>f_printc
	print "#ifndef NOCHECKS"			>f_printc
	print "static const char *fname = \"print.C\";"	>f_printc
	print "static const char *mismatch_string = \"<<error: mismatch>>\";" >f_printc
	print "static const char *null_string = \"<<error: null>>\";" >f_printc
	print "#endif // NOCHECKS\n"			>f_printc

	print "// file produced by ../../catalog.d/common/Msg.awk\n"	  >f_unb_c
	print "#include \"Signature.h\""			  >f_unb_c
	print "#include \"Message.h\""				  >f_unb_c
	print "#include \"Msgtab.h\""				  >f_unb_c
	print "#include \"UIutil.h\"\n"				  >f_unb_c
	print "#ifndef NOCHECKS"				  >f_unb_c
	print "static const char *fname = \"Message::unbundle\";" >f_unb_c
	print "#endif // NOCHECKS\n"				  >f_unb_c

	print "// file produced by ../../catalog.d/common/Msg.awk\n"	  >f_unb_h
	print "// file produced by ../../catalog.d/common/Msg.awk\n"	  >f_format
	print "// file produced by ../../catalog.d/common/Msg.awk\n"	  >f_sigtab_h

	# The Message catalog includes several strings that are
	# not retrieved through the message table.
	# ------ WARNING -------
	# Messages in an existing catalog cannot be modified or removed,
	# because we have no control over the translated catalogs,
	# also, calls to gettxt have hard-coded numbers in them.
	# Messages MUST stay in the same order.
	print "%# 1;WIDTH=0;\nWarning: "	>f_cat
	print "%# 2;WIDTH=0;\nError: "		>f_cat
	print "%# 3;WIDTH=0;\nFatal error: "	>f_cat
	print "%# 4;WIDTH=0;\nFatal error: Internal error in %s at line %d\\n" >f_cat
	print "%# 5;NO_SWAP,WIDTH=0;%s file, %d line\nError: Internal error in %s at line %d; contents of next message are suspect\\n" >f_cat
	next_num = 6			# next catalog entry
	num_sigs = 0
}

# main loop reading Signature.h and Msg.awk.in
# the command line in the makefile is
# 	cat ../../awk.cond Signature.h Msg.awk.in | awk -f Msg.awk
# the script therefore assumes that all signatures come before any messages

{
	# The only lines we are interested in are the ones that
	# start with MSG_, ERR_, QUERY_, or SIG_
	# or the conditional lines #ifdef, #define, #else, #endif,
	# #undef, #ifndef
	# Signatures will be used as a check on the message table input
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
	else if ($1 == "" && substr($2, 1, 4) == "SIG_")
	{
		num_sigs++

		# remove the trailing comma
		tmp = substr($2, 1, length($2) - 1)
		SIG[num_sigs] = tmp

		# the call to split breaks the signature down into
		# its sub-components
		n = split(tmp, sig, "_")

		if (sig[2] == "last" || sig[2] == "none" || sig[2] == "invalid")
			next

		printf "\t{%d,\t", n-1 >f_sigtab_h
		used_str = 0
		for (j = 1; j < n; j++)
		{
			if (sig[j+1] == "str")
			{
				if (used_str == 1)
					printf "|" >f_sigtab_h
				printf "SET(%d)", j >f_sigtab_h
				used_str = 1
			}
			else if (sig[j+1] != "word")
			{
				print "Error: invalid signature: " SIG[i]
				exit_status = 1
				exit exit_status
			}
		}
		print "},\t// " tmp >f_sigtab_h
		next	# the rest of the main loop applies only to messages
	}

	if (substr($1, 1, 4) == "MSG_")
		class = "MSGCL_info"
	else if (substr($1, 1, 4) == "ERR_")
		class = "MSGCL_error"
	else if (substr($1, 1, 6) == "QUERY_")
		class = "MSGCL_query"
	else
		next

	printf "\t%s,\n", $1 >f_msg_h
	printf "\t\"%s\",\n", $1 >f_msg_c

	mid = $1
	ystring = $2
	if ($3 != "-")
	{
		if (ystring == "yes")
			msg_num = next_num++
		else
			msg_num = 0

		fstring = $3

		# create the signature from the format specifiers (%s, etc.)
		# in the message text
		signature = "SIG"
		fstr2 = fstring
		while ((n = index(fstr2, "%")) != 0)
		{
			fmt_spec = substr(fstr2, n + 1)
			fstr2 = substr(fstr2, n + 2)

			while (fmt_char = substr(fmt_spec, 1, 1))
			{
				fmt_spec = substr(fmt_spec, 2)
				if (fmt_char ~ /s/)
				{
					signature = signature "_str"
					break
				}
				else if (fmt_char ~ /[dox]/)
				{
					signature = signature "_word"
					break
				}
				else if (fmt_char !~ /[-#0-9.]*/)
				{
					printf "Error: invalid format specifier for %s\n", $2
					exit_status = 1
					exit exit_status
				}
			}
		}

		if (signature == "SIG")
			signature = signature "_none"
		else
		{
			for (i = 1; i <= num_sigs; i++)
				if (SIG[i] == signature)
					break

			if (i > num_sigs)
			{
				printf "Error: unrecognized signature %s for %s\n",\
					signature, mid
				exit_status = 1
				exit exit_status
			}
		}

		if (class == "MSGCL_info")
			infom[signature] = 1
		else if (class == "MSGCL_error")
			errorm[signature] = 1
		else if (class == "MSGCL_query")
			querym[signature] = 1
		else
		{
			printf "Error: unrecognized message class %s for %s\n", class, mid
			exit_status = 1
			exit exit_status
		}

		printf "{%s,\t%s,\t%d,\t0,\t\"%s\"},", class, signature, msg_num, fstring >f_tab_c
		if (ystring == "yes")
		{
			if ($4 != "-")
				printf "%%# %d;NO_SWAP,WIDTH=0;%s\n", msg_num, $4 > f_cat
			else
				printf "%%# %d;NO_SWAP,WIDTH=0;\n", msg_num > f_cat

			if ($5 != "-")
				printf "%%# %s\n", $5 > f_cat
			print fstring >f_cat
		}
	}
	else
		printf "{%s,\tSIG_none},", class >f_tab_c
	printf "\t/* %s */\n", mid >f_tab_c
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
	print "\tMSG_last\n};" >f_msg_h
	print "extern const char *Msg_type_names[];\n" >f_msg_h
	print "\n#endif" >f_msg_h
	print "\t\"MSG_last\"\n};" >f_msg_c
	print "{MSGCL_invalid, SIG_invalid}\t/* MSG_last */" >f_tab_c
	print "};" > f_tab_c

	for (lc = 1; lc <= num_sigs; lc++)  # lc for loop counter
	{
		if (SIG[lc] == "SIG_invalid" || SIG[lc] == "SIG_last")
			continue;

		if (infom[SIG[lc]] == 0 && errorm[SIG[lc]] == 0 && querym[SIG[lc]] == 0)
			printf "Warning: unused signature %s\n", SIG[lc]

		udecl = ""
		proto = ""
		params = ""
		checks = ""
		recover = "\n"
		uparams = ""
		qlen = ""

		if (SIG[lc] != "SIG_none")
		{
			# the call to split breaks the signature down into
			# its sub-components
			n = split(SIG[lc], sig, "_")

			# walk through the sig array, collecting the arguments
			# for function prototypes and function definitions
			# proto collects the arguments for printm's and printe's,
			# which take const char *'s and Word's
			# udecl collects them for the unbundle's which take
			# char *'s and Word &'s
			# params is just the list of parameter names without the types
			# checks is used to check that incoming pointer parameters
			# are non-null

			for (j = 1; j < n; j++)
			{
				proto = proto ", "
				params = params ", "

				if (j > 1)
				{
					udecl = udecl ", "
					uparams = uparams ", "
				}

				if (sig[j+1] == "str")
				{
					udecl = udecl "char *&s" j
					proto = proto "const char *s" j
					params = params "s" j
					uparams = uparams "s" j
					recover = recover "\t\t\ts" j " = mismatch_string;\n"
					checks = checks "if (!s" j ") s" j "= null_string;\n"
					qlen = "+strlen(s" j ")"
				}
				else if (sig[j+1] == "word")
				{
					udecl = udecl "Word &i" j
					proto = proto "Word i" j
					params = params "i" j
					uparams = uparams "i" j
				}
			}
		}

		# create the printm functions and prototypes
		if (infom[SIG[lc]] == 1)
		{
			printf "void\nprintm(Msg_id mid%s)\n", proto >f_printc
			printf "void printm(Msg_id%s);\n", proto >f_printh

			printf "{\n#ifndef NOCHECKS\n"		  >f_printc
			printf "\tif ((Mtable.signature(mid) != %s)\n", SIG[lc] >f_printc
			printf "\t\t|| Mtable.msg_class(mid) != MSGCL_info)\n" >f_printc
			printf  "\t\t{\n\t\t\tinterface_error(fname, __LINE__);" >f_printc
			printf "%s", recover > f_printc
			printf "\t\t}\n" >f_printc

			if (checks != "")
				printf "\t%s\n", checks > f_printc
			printf "#endif // NOCHECKS\n"		  >f_printc

			printf "\n\tint len = message_manager->send_msg(mid, E_NONE%s);\n", params >f_printc
			printf "\tif (log_file)\n"		  >f_printc
			printf "\t\tlog_msg(len, mid, E_NONE%s);\n}\n\n", params >f_printc
		}

		# printe functions and prototypes
		if (errorm[SIG[lc]] == 1)
		{
			printf "void printe(Msg_id, Severity%s);\n", proto >f_printh
			printf "void\nprinte(Msg_id mid, Severity etype%s)\n", proto >f_printc

			printf "{\n#ifndef NOCHECKS\n"	  >f_printc
			printf "\tif (Mtable.signature(mid) != %s\n", SIG[lc]  >f_printc
			printf "\t\t|| Mtable.msg_class(mid) != MSGCL_error\n" >f_printc
			printf "\t\t|| etype < E_NONE || etype > E_FATAL)\n"  >f_printc
			printf  "\t\t{\n\t\t\tinterface_error(fname, __LINE__);" >f_printc
			printf "%s", recover > f_printc
			printf "\t\t}\n" >f_printc

			if (checks != "")
				printf "\t%s\n", checks > f_printc
			printf "#endif // NOCHECKS\n"		  >f_printc

			printf "\n\tint len = message_manager->send_msg(mid, etype%s);\n", params  >f_printc
			printf "\tif (log_file)\n"		  >f_printc
			printf "\t\tlog_msg(len, mid, etype%s);\n", params  >f_printc
			printf "\tif (etype == E_FATAL)\n"	  >f_printc
			printf "\t\tdb_exit(1);\n}\n\n"		  >f_printc
		}

		# query functions and prototypes
		if (querym[SIG[lc]] == 1)
		{
			printf "int\nquery(Msg_id mid, int yorn_answer%s)\n", proto >f_printc
			printf "int query(Msg_id, int%s);\n", proto >f_printh

			printf "{\n#ifndef NOCHECKS\n"		  >f_printc
			printf "\tif ((Mtable.signature(mid) != %s)\n", SIG[lc] >f_printc
			printf "\t\t|| (Mtable.msg_class(mid) != MSGCL_query))\n" >f_printc
			printf  "\t\t{\n\t\t\tinterface_error(fname, __LINE__);" >f_printc
			printf "%s", recover > f_printc
			printf "\t\t}\n" >f_printc

			if (checks != "")
				printf "\t%s\n", checks > f_printc
			printf "#endif // NOCHECKS\n"		  >f_printc

			printf "return message_manager->query(mid, yorn_answer%s);\n}\n", params >f_printc
		}

		if (SIG[lc] == "SIG_none")
			continue

		# create the unbundle functions and prototypes
		printf "void\nMessage::unbundle(%s)\n", udecl >f_unb_c
		printf "\tvoid\tunbundle(%s);\n", udecl >f_unb_h

		printf "{\n#ifndef NOCHECKS\n"				>f_unb_c
		printf "\tif (((msg_state != MSTATE_received)\n"	>f_unb_c
		printf "\t\t&& (msg_state != MSTATE_ready_to_send)\n"	>f_unb_c
		printf "\t\t&& (msg_state != MSTATE_sent))\n"		>f_unb_c
		printf "\t\t|| (Mtable.signature(msg_id) != %s))\n", SIG[lc] >f_unb_c
		printf "\t\tinterface_error(fname, __LINE__);\n"	>f_unb_c
		printf "#endif // NOCHECKS\n"				>f_unb_c
		printf "\n\tchar *ptr = (char *)msg_data;\n"		>f_unb_c

		for (i = 1; i < n; i++)
		{
			if (sig[i+1] == "str")
				printf "\tpick_string(ptr, s%d);\n", i >f_unb_c
			else
				printf "\tpick_word(ptr, i%d);\n", i	>f_unb_c
		}

		printf "\n#ifndef NOCHECKS\n"				>f_unb_c
		printf "\tif ((ptr - (char *)msg_data) != msg_length)\n" >f_unb_c
		printf "\t\tinterface_error(fname, __LINE__);\n" 	>f_unb_c
		printf "#endif // NOCHECKS\n"				>f_unb_c
		printf "}\n\n"						>f_unb_c

		printf "\t\tcase %s:\n", SIG[lc]		>f_format
		printf "\t\t\tunbundle(%s);\n", uparams		>f_format
		printf "\t\t\ti = sprintf(buf, fmt, %s);\n", uparams	>f_format
		printf "\t\t\tbreak;\n\n"			>f_format
	}
}
