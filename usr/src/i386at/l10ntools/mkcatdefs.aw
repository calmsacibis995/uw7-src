#	@(#) mkcatdefs.awk 11.10 95/01/27 
#
#	Copyright (C) 1992-1994 The Santa Cruz Operation, Inc.
#		All Rights Reserved.
#	This Module contains Proprietary Information of
#	The Santa Cruz Operation and should be treated as Confidential.
#
# This utility is an implementation of the OSF/1 mkcatdefs command to
# preprocess message source files. Through the use of this utility
# symbolic identifiers can be used to represent message set numbers and
# message ID numbers. An include file is produced to be included in the
# source files making use of symbolic identifiers for messages. Also
# generated is the input required by the gencat utility to produce the
# catalogue file. For more information on the usage of the mkcatdefs
# utility see the OSF/1 documentation and/or man page.
#

BEGIN {
	#Initialise numeric counts
	count=1; setcount=1; numcount=0; numsetcount=0;
	linecont="false"
	number ="^[0-9]+$"

	#Check for correct number of arguments
	if (ARGC < 3)
	{
		print "mkcatdefs: error insufficient arguments" | "cat 1>&2" ;
		print "usage: mkcatdefs [-h\|-s] symbol_name source_file ..." | "cat 1>&2" ;
		exit -1 ;
	}

	#Process the -h option to stop include file generation
	if (h_option == "true")
	{
		f1 = "/dev/null" ;
		name = ARGV[1] ;
		ARGV[1] = "" ;
	}
	#Generate initial information of Shell script file
	else if (shell_option == "true")
	{
		#Generate shell script file name
		f1 = ARGV[1] "_msg.sh" ;
		name = ARGV[1] ;
		ARGV[1] = "" ;

		#Define initial entries
		print "# The following was generated from " > f1 ;
		for (i = 2 ; i < ARGC ; i++)
			print "# " ARGV[i] > f1 ;
		print "#" > f1 ;
		if ( archive_name == "" )
		{
			print "MF_" toupper(name) "=" name ".cat" > f1 ;
		} else {
			print "MF_" toupper(name) "=" name ".cat@" archive_name > f1 ;
		}

	}
	#Generate initial information of C include file
	else
	{
		#Generate include file name
		f1 = ARGV[1] "_msg.h" ;
		name = ARGV[1] ;
		ARGV[1] = "" ;

		#Define initial entries
		print "#ifndef _H_" toupper(name) "_MSG" > f1 ;
		print "#define _H_" toupper(name) "_MSG" > f1 ;
		print "#include <limits.h>" > f1 ;
		print "#include <nl_types.h>" > f1 ;
		if ( archive_name == "" )
		{
			print "#define MF_" toupper(name) " \"" name ".cat\"" > f1 ;
		} else {
			print "#define MF_" toupper(name) " \"" name  ".cat@" archive_name "\"" > f1 ;
		}
		print "#ifndef MC_FLAGS" > f1 ;
		print "#define MC_FLAGS NL_CAT_LOCALE" > f1 ;
		print "#endif" > f1 ;
		print "#ifndef MSGSTR" > f1 ;
		print "#ifdef lint" > f1 ;
		print "#define MSGSTR(num,str) (str)" > f1 ;
		print "#define MSGSTR_SET(set,num,str) (str)" > f1 ;
		print "#else" > f1 ;
		print "#define MSGSTR(num,str) catgets(catd, MS_" toupper(name) ", (num), (str))" > f1 ;
		print "#define MSGSTR_SET(set,num,str) catgets(catd, (set), (num), (str))" > f1 ;
		print "#endif" > f1 ;
		print "#endif" > f1 ;
		print "/* The following was generated from */" > f1
		for (i = 2 ; i < ARGC ; i++)
			print "/* " ARGV[i] " */" > f1	
		print " " > f1
	}
}
{
	#Process the main body of the source file
	#containing the symbolic identifiers

	#Process lines that are line continuations from a previous line
	#just pass these lines through with no processing
	#IMPORTANT: If line continuation is specified before the
	#line has been fully identified and symbolic info turned
	#into numeric info line continuation will fail and a bad
	#gencat file will be produced. Basicly this means don't put line
	#continuation in the first 20 or so characters. Could fix this
	#but effort is not really worthwhile.
	if (linecont != "false")
	{
		#Could be true_but_bad so better check
		if(linecont == "true")
			print $0

		#if this line does not specify line continuation
		#set state so lines are processed again
		if(!(($NF == "\\") || (substr($NF,length($NF),1) == "\\")))
			linecont = "false";
	}
	#Process gencat $delset subcommand
	else if ($1 == "$delset")
	{
		#if delset has a numeric argument pass through to gencat
		if($2 ~ number)
		{
			print $0;
		}
		#if delset has a symbolic argument generate numeric
		#argument for gencat
		else
		{
			#Search list of defined sets
			i=1;
			found="false";
			while (i <=setcount)
			{
				split(setarray[i], delsetarr, " ") ;
				#if set symbolic found substitute numeric
				if(delsetarr[1] == $2)
				{
					print "$delset " delsetarr[2];
					found="true";
					break;
				}
				i++;
			}
			#if set symbolic not found print warning
			if(found == "false")
			{
				print "mkcatdefs warning: " $2 " not previously defined for use with $delset" | "cat 1>&2" ;

			}
		}
		#Check for line continuation and if so set state
		#so no processing is done of the next line
		if(($NF == "\\") || (substr($NF,length($NF),1) == "\\"))
		{
			#if this line was bad don't want
			#next line passed through or processed
			if(found == "false")
				linecont = "true_but_bad";
			else
				linecont = "true";
		}
	}
	#Process gencat comment subcommand
	else if ($1 == "$")
	{
		#Pass on through
		print $0

		#Check for line continuation and if so set state
		#so no processing is done of the next line
		if(($NF == "\\") || (substr($NF,length($NF),1) == "\\"))
			linecont = "true";
	}
	#Process gencat $quote subcommand
	else if ($1 == "$quote")
	{
		#Pass on through
		print $0

		#Check for line continuation and if so set state
		#so no processing is done of the next line
		if(($NF == "\\") || (substr($NF,length($NF),1) == "\\"))
			linecont = "true";
	}
	#Process empty line
	else if (NF == 0)
	{
		#Pass on through
		print $0
	}
	#Process gencat $set subcommand
	else if ($1 == "$set")
	{

		#reset counts
		count = 1;
		numcount = 0;

		#if $set has a numeric argument check it has not been used
		#already, store a copy and pass line through for gencat
		if($2 ~ number)
		{
			#Check number has not been used explicitly
			i=1;
			while (i <= numsetcount)
			{
				if($2 == numsetarray[i])
				{
					print "mkcatdefs error: Literal set number " $2 " has already been used in message source" | "cat 1>&2" ;
					exit -1 ;
				}
				i++;
			}

			#Check mkcatdef has not already used number
			if($2 < setcount)
			{
				print "mkcatdefs error: Literal set number " $2 " has already been used by mkcatdefs" | "cat 1>&2" ;
				exit -1 ;
			}

			#Keep copy for checking purposes
			numsetcount++;
			numsetarray[numsetcount] = $2;

			#Pass line through for gencat to process
			print $0;
		}
		#if $set has a symbolic argument generate numeric
		#argument for gencat
		else
		{
			#Check autogenerated number has not been
			#specified explicitly
			i=1;
			while (i < numsetcount)
			{
				if(setcount == numsetarray[i])
				{
					#if setcount found it needs to be
					#incremented the whole numsetarray
					#needs to be checked again as it is
					#unsorted.
					setcount++;
					i=0;
				}
				i++;
			}

			# Don't $delset before the $set subcommand
			# as OSF/1 manual specifies
			# as warning messages are generated from
			# our gencat command
			# print "$delset " setcount ;

			#Substitute symbolic definition by numeric value
			s = "$set " setcount " " ;
			#Print rest of line
			for (i = 3 ; i <= NF; i++)
			{
				if (i == NF) s = s  $i  ;
				else s = s  $i " " ;
			}
			print s ;
	
			#Update include of shell script file
			if (shell_option == "true")
			#Update shell script file to map symbolic to numeric value
			{
				print "#" > f1
				print $2 "=" setcount > f1 ;
			}
			else
			#Update include file to map symbolic to numeric value
			{
				print " " > f1
				print "#define " $2 " " setcount > f1 ;
			}
	
			#Keep set symbolic and numeric pairs in array
			setarray[setcount] = $2 " " setcount
	
			#increment setcount
			setcount++ ;
		}

		#Check for line continuation and if so set state
		#so no processing is done of the next line
		if(($NF == "\\") || (substr($NF,length($NF),1) == "\\"))
			linecont = "true";

	}
	#Process Symbolic Message
	else
	{
		#if message identifier is a numeric check it has not been used
		#already, store a copy and pass line through for gencat
		if($1 ~ number)
		{
			#Check number has not been used explicitly
			i=1;
			while (i <= numcount)
			{
				if($1 == numarray[i])
				{
					print "mkcatdefs error: Literal number " $1 " has already been used in message source" | "cat 1>&2" ;
					exit -1 ;
				}
				i++;
			}

			#Check mkcatdef has not already used number
			if($1 < count)
			{
				print "mkcatdefs error: Literal number " $1 " has already been used by mkcatdefs" | "cat 1>&2" ;
				exit -1 ;
			}

			#Check number has not been used automatically
			numcount++;
			numarray[numcount] = $1;

			#Pass line through for gencat to process
			print $0;
		}
		else
		#if message identifier is a symbolic argument generate numeric
		#argument for gencat
		{
			#Check autogenerated number has not been
			#specified explicitly
			i=1;
			while (i < numcount)
			{
				if(count == numarray[i])
				{
					#if count found it needs to be
					#incremented the whole numarray
					#needs to be checked again as it is
					#unsorted.
					count++;
					i=0;
				}
				i++;
			}

			#Update include or shell script file
			if (shell_option == "true")
			#Update shell script file to map symbolic to numeric value
			{
				print $1 "=" count > f1 ;
			}
			else
			#Update include file to map symbolic to numeric value
			{
				print "#define " $1 " " count > f1 ;
			}

			#Substitute symbolic definition by numeric value
			sub($1, count);
			print $0;
	
			count ++
	
		}
		#Check for line continuation and if so set state
		#so no processing is done of the next line
		if(($NF == "\\") || (substr($NF,length($NF),1) == "\\"))
			linecont = "true";

	}
}END {
		if (shell_option != "true")
			print "#endif /* _H_" toupper(name) "_MSG */" > f1 ;
}
