# Copyright 1992 SunSoft, Inc.

# Permission to use, copy, modify, and distribute this software and its
# documentation for any purpose and without fee is hereby granted, provided
# that the above copyright notice appear in all copies and that both that
# copyright notice and this permission notice appear in supporting
# documentation, and that the name of SunSoft, Inc. not be used in 
# advertising or publicity pertaining to distribution of the software 
# without specific, written prior permission.  SunSoft, Inc. makes
# no representations about the suitability of this software for any purpose.  
# It is provided "as is" without express or implied warranty.
#
# SunSoft, Inc. DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, 
# INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO 
# EVENT SHALL SunSoft Inc. BE LIABLE FOR ANY SPECIAL, INDIRECT OR 
# CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF 
# USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR 
# OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR 
# PERFORMANCE OF THIS SOFTWARE.
#
# MODIFICATIONS:
#
#	 Ranjan Das-Gupta, UNIX System Labs Inc, 1 Jul 1993
#		Code review cleanup
#


package tet;

# set current context to process ID and reset block and sequence
# usage: &setcontext()

sub setcontext 
{
	if ($>!=$context) 
	{
		$context=$$;
		$block=1;
		$sequence=1;
	}
}

# increment the current block ID, reset the sequence number to 1
# usage: &setblock()

sub setblock {
	++$block;
	$sequence=1;
}

# print an information line to the execution results file
# and increment the sequence number
# usage: &infoline("<info>");

sub infoline
{
	&output(520,"$tpnumber $context $block $sequence","$_[0]");
	++$sequence;
}


# print an error message & die
# passed the routine name in question
sub wrong_params
{
	die("wrong number of parameters passed to tet\'$_[0]");
}
	

# record a test result for later emmision to the execution results file
# by tet_tpend
# usage: &result(result_name)
# (note that a result name is expected, not a result code number)

sub result 
{
	($#_!=0) &&	&wrong_params("result");
	if (&getcode($_[0])!=0) 
      {
		&error("invalid result name $_[0] passed to tet'result");
		$_[0]="NORESULT";
      }

	print TMPRES "$_[0]\n";
#	system("echo $_[0] >> $tmpres\n");
	
}

# mark a test purpose as deleted
# usage: delete(test_name,[reason]);
# keep delete 'file' in perl var 'DELETES_FILE'
sub delete 
{
	($#_<0 || $#_>1) && &wrong_params("delete");
	if ($#_==1) 
	{
		&reason($_[1]);
	}

	&undelete($_[0]);
	@DELETES_FILE=(@DELETES_FILE,"$_[0] $_[1]");
}



sub get_reason 
{
	local(*DELETES_HANDLE,@del_file,@matches);
	($#_!=0) && &wrong_params("get_reason");
	@matches=grep(/^$_[0]/,@DELETES_FILE);
	if ($#matches==-1) {return(1);};
	$_=$matches[0];
	split;
	$_=$_[1];
	return 0;
}

# print the reason why a test purpose has been deleted
# return 0 if the test purpose has been deleted, 1 otherwise
# usage: reason(test_name)
sub reason 
{
	local($_);
	
	($#_!=0) && &wrong_params("get_reason");
	if (&get_reason($_[0]) == 0) {
		print "$_\n";
		0;
	} 
	else 
	{
		1;
	}
}

	

# ******************************************************************

#
# "private" functions for internal use by the shell API
# these are not published interfaces and may go away one day
#


# getcode
# look up a result code name in the result code definition file
# return 0 if successful with the result number in TET_RESNUM and TET_ABORT
# set to YES or NO
# otherwise return 1 if the code could not be found

sub getcode {
	local($_);
	
	($#_!=0) && &wrong_params("getcode");
	$abort="NO";
	$resnum=-1;

	local($tet_a)=$_[0];

	foreach(@TET_CODE_FILE) {
		s/^#.*//;
		if ( ! /^[	 ]*$/) {
			split;
			$_[1] =~ s/\"//g;
			if ("$_[1]" eq "$tet_a") {
				$resnum=$_[0];
				$ABACTION=$_[2];
			}
		}
	}
	if ($resnum==-1) {return(1);};

	$_=$ABACTION;
	G_SWITCH: {

		/""|Continue/ && do { $abort="NO"; last G_SWITCH;};
		/Abort/ && do { $abort="YES"; last G_SWITCH;};

		&error("invalid action field $ABACTION in file $code\n");
		$abort="NO";
	}

	0;
}


# tet_undelete - undelete a test purpose
sub undelete {
	($#_!=0) && &wrong_params("undelete");
	local($F);
	local(@a)=@DELETES_FILE;
	@DELETES_FILE=grep(!"/^$[0]/", @a);
}


# tet_error - print an error message to stderr and on TCM Message line
sub error
{
	print STDERR "$PNAME: $_[0]\n";
	if ("$activity"=="") { $activity=0;}
	printf($JOURNAL_HANDLE "510|$activity|$_[0]\n");
}

# tet_output - print a line to the execution results file
sub output {
	local($_);
	
	#ensure no newline chars in data & line<512
	local($arg1,$arg2,$arg3)=@_;
	local($sp);
	if (length($arg2)>0) {
		$sp=" "; } else { $sp=""; }
	if ("$activity"=="") { $activity=0;}
	$_=sprintf("%d|%s%s%s|%s",$arg1,$activity,$sp,$arg2,$arg3);
	s/\n//;
	local($l)=0;
	local($mess);
	if (length()>511) {
		$mess=
			sprintf("warning: results file line truncated: prefix: %d|%s%s%s|\n",
			$arg1,$activity,$sp,$arg2,$arg3);
		$l=1;			
	}
	printf($JOURNAL_HANDLE "%.511s\n",$_);

	if ($l) { &error($mess);}
	
}

1;

package main;
