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
# Modifications:
#
# June 3rd 1993, Update version number to 1.10.1
#
# July 1st 1993, Code review cleanup
#
# November 1st 1993, Update version number to 1.10.2
#
# March 29th 1994, Update version number to 1.10.3
#

package tet;

# DESCRIPTION:
#	This file contains the support routines for the sequencing and control
#	of invocable components and test purposes.
#	It should be required (by means of the perl 'require' command) into a perl
#	script containing definitions of the invocable components and test
#	purposes that may be executed, after those definitions have been made.
#	Test purposes may be written as perl functions.
#
#	This file 'requires' api.pl which contains the perl API functions.
#
#	The user-supplied shell variable iclist should contain a list of all
#	the invocable components in the testset;
#	these are named ic1, ic2 ... etc.
#	For each invocable component thus specified, the user should define
#	a variable whose name is the same as that of the component.
#	Each such variable should contain the names of the test purposes
#	associated with each invocable component; for example:
#       @iclist=(ic1,ic2,ic3);
#       @ic1=(test1-1,test1-2, test1-3);
#       @ic2=(test2-1, test2-2);
#
#	The NUMBERS of the invocable components to be executed are specified
#	on the command line.
#	In addition, the user may define the variables $tet'startup and
#	$tet'cleanup; if defined, the related functions 
#	are executed at the start and end of processing, respectively.
#
#	The TCM makes the NAME of the currently executing test purpose
#	available in the variable $tet'thistest.
#
#	The TCM reads configuration variables from the file specified by the
#	TET_CONFIG environment variable; these are placed in the environment
#	in the 'tet' package's namespace.
#	This file (or the environment) should contain an assignment for
#	TET_NSIG which should be set to one greater than the highest signal
#	number supported by the implementation.
#

# standard signals - may not be specified in TET_SIG_IGN and TET_SIG_LEAVE     
# SIGHUP, SIGINT, SIGQUIT, SIGILL, SIGABRT, SIGFPE, SIGPIPE, SIGALRM,
# SIGUSR1, SIGUSR2, SIGTSTP, SIGCONT, SIGTTIN, SIGTTOU
#@STD_SIGNAL_LIST=(1,2,3,4,6,8,13,14,15,16,17,25,26,27,28);

@STD_SIGNAL_NAMES=(HUP,INT,QUIT,ILL,ABRT,FPE,PIPE,ALRM,USR1,USR2,TSTP,
	CONT,TTIN,TTOU);

# signals that are always unhandled
# SIGSEGV is here as well because the shell can't trap it
# SIGKILL, SIGSEGV, SIGCHLD, SIGSTOP
#@SPEC_SIGNAL_LIST=(9,11,18,24);

@SPEC_SIGNAL_NAMES=(KILL,SEGV,CHLD,STOP);

$OS_VERSION=`uname -r`;

#print "OS_VERSION=$OS_VERSION\n";

$OS=`uname`;
chop($OS_VERSION);
chop($OS);

$_=$OS;
if (/SunOS/) {
	$_=$OS_VERSION;
	if (/5.*/) 
	{
		@signames=(NONE,HUP,INT,QUIT,ILL,TRAP,IOT,EMT,FPE,KILL,BUS,SEGV,
			SYS,PIPE,ALRM,TERM,USR1,USR2,CHLD,PWR,WINCH,URG,POLL,STOP,
			TSTP,CONT,TTIN,TTOU,VTALRM,PROF,XCPU,XFSZ,WAITING,LWP);
		$NSIG=34;		#number of signals = NSIG-1, (different in Svr4, vs. 4.1)
	}

	else 

	{
		@signames=(NONE,HUP,INT,QUIT,ILL,TRAP,IOT,EMT,FPE,KILL,BUS,SEGV,SYS,
			PIPE,ALRM,TERM,URG,STOP,TSTP,CONT,CHLD,TTIN,TTOU,IO,XCPU,XFSZ,
			VTALRM,PROF,WINCH,LOST,USR1,USR2);
		$NSIG=32;		#number of signals = NSIG-1, (different in Svr4, vs. 4.1)
	}
} 
else 
{
# the install.pl utility will set these params for other OS's

	@signames=(__SIGNAMES__);
	$NSIG=__NSIG__;
}


$#STD_SIGNAL_LIST=$#STD_SIGNAL_NAMES;
for ($i=0; $i<=$#STD_SIGNAL_NAMES; ++$i) 
{
	$STD_SIGNAL_LIST[$i]=&signum($STD_SIGNAL_NAMES[$i]);
}
		

$#SPEC_SIGNAL_LIST=$#SPEC_SIGNAL_NAMES;
for ($i=0; $i<=$#SPEC_SIGNAL_NAMES; ++$i) 
{
	$SPEC_SIGNAL_LIST[$i]=&signum($SPEC_SIGNAL_NAMES[$i]);
}
		


@std_signals=@STD_SIGNAL_LIST;
@spec_signals=@SPEC_SIGNAL_LIST;

@sig_leave=@sig_leave_list;
@sig_ign=@sig_ignore_list;


# TCM global variables
 

$thistest="";

# 
# "private" TCM variables
#
$tet'cwd=`pwd`;
chop($cwd);
$tet_tmp_dir=$ENV{"TET_TMP_DIR"};
if ($tet_tmp_dir eq "")
{
	$tet_tmp_dir=$cwd;
}
@tmpfiles=();
$tmpres="$tet_tmp_dir/tet_tmpres";
$tet_lock_dir="$tet_tmp_dir/.tmpres";

$context=0;
$block=0;
$sequence=0;
$tpcount=0;
$exitval=0;
$version=1.1;
$activity=$ENV{"TET_ACTIVITY"};
$tpnumber=0;

# ***********************************************************************
#	compute tpnumbers for all test cases.
#	use a associative array (easiest way in perl)
#
local($tpcounter)=1;
local ($ic);
foreach $ic (@main'iclist) {
	local(@a)=eval("@main'"."$ic");
	local ($tp);
	foreach $tp (@a) {
		if (!defined($tp_ids{"$tp"})) {
			$tp_ids{"$tp"}=$tpcounter++;
		}
	}
}
#@k=keys %tp_ids;
#@v=values %tp_ids;

#print "k=@k\n";
#print "v=@v\n";

# ***********************************************************************
  
# 
# "private" TCM function definitions
# these interfaces may go away one day
#

# tet_ismember - return 0 if $1 is in the set $2 ... 
# otherwise return 1

sub ismember 
{
	local ($mem,*array) = @_;
	local(@t)=grep("\b$mem\b", @array);
	if ($#t>-1) { return 1; }
	return 0;
}




# tet_setsigs - install traps for signals mentioned in TET_SIGNALS
# if the action is ignore, the signal is ignored
# if the action is default, the signal trap is $1
# signal traps are passed the invoking signal number as argument
sub setsigs {
	local($_);
	
	($#_!=0) && &wrong_params("setsigs");

#	local($SIGFILE);
	local($signum);

#	while (<SIGFILE>) {
	for ($signum=1;$signum<$NSIG;++$signum) {
		# pattern match $_ to match a number, and a string
#		($signum,$_)=/(\d+)\s+(\S+)\s/;
		$_=$signal_actions[$signum];
		SETSIGS: {
			/leave/ && (last SETSIGS);

			/ignore/ && ($SIG{$signame[$signum]}='IGNORE', last SETSIGS);

			$SIG{$signame[$signum]}=$_[0];
		}
			
	}
#	close(SIGFILE);
	
}



# tet_defaultsigs - restore default action for signals mentioned in TET_SIGNALS
# if the action is ignore, the signal is ignored
# if the action is default, any existing signal trap is removed
sub defaultsigs{
	local($_);
	

#	local($SIGFILE);
	local($signum);

#	while (<SIGFILE>) {
	for ($signum=1;$signum<$NSIG;++$signum) {
		# pattern match $_ to match a number, and a string
#		($signum,$_)=/(\d+)\s+(\S+)\s/;
		$_=$signal_actions[$signum];
		SETSIGS: {
			/leave/ && (last SETSIGS);

			/ignore/ && ($SIG{$signame[$_]}='IGNORE', last SETSIGS);

			$SIG{$signame[$_]}='DEFAULT';
		}
			
	}
#	close(SIGFILE);
	
}

sub signum 
{
	($#_!=0) && &wrong_params("signum");
	local($i)=0;
	for($i=0;$i<=$#signames;++$i) 
	{
		if ($signame[$i] eq $_[0])
		{
			return $i;
		}
	}
	return -1;
}



# tet_abandon - signal handler used during startup and cleanup
sub abandon 
{
	local($sig)=@_;
	if ($sig=="TERM") 
	{
		&sigterm($sig);
	} 
	else 
	{
		&error("Abandoning testset: caught unexpected signal $sig");
	}
	&cleanup;
	exit(&signum($sig));
}



# tet_sigterm - signal handler for SIGTERM

sub sigterm 
{
	local($sig)=@_;
	&error("Abandoning test case: received signal $sig");
	&docleanup;
	exit(&signum($sig));
}



# tet_sigskip - signal handler used during test execution
sub sigskip 
{
	local($sig)=@_;
	&infoline("unexpected signal $sig received");
	&result("UNRESOLVED");
	if ($sig=="TERM") {
		&sigterm($sig);
	}
}


sub time {
	($sec,$min,$hour)=localtime;
	$r=sprintf("%02d:%02d:%02d",$hour,$min,$sec);
}



# tet_tpend - report on a test purpose
sub tpend 
{
	local($_);
	
	($#_!=0) && &wrong_params("tpend");
	local($arg)=$_[0];
#	local($TMPRES);
	$result="";
	seek(TMPRES,0,0);
	READLOOP: while (<TMPRES>) {
		chop;
		if ("$result" eq "") {
			$result="$_";
			next READLOOP;
		}
		PAT: {
			/PASS/ && (last PAT);

			/FAIL/ && ($result = $_, last PAT);

			/UNRESOLVED|UNINITIATED/ && do
				{if ("$result" ne FAIL) {
					$result=$_;
				} last PAT;};

			/NORESULT/ && do
				{if ( $result eq FAIL || $result eq UNRESOLVED 
					|| $result eq UNINITIATED) {
						$result=$_;
				}  last PAT;};

			/UNSUPPORTED|NOTINUSE|UNTESTED/ && do
				{if ($result eq PASS) {
					$result=$_;
				} last PAT;};

			if (($result eq PASS) || ($result eq UNSUPPORTED) ||
				($result eq NOTINUSE) || ($result eq UNTESTED) ) {
				$result=$_;
			}
		}
	}

	close(<TMPRES>);	# TMPRES deleted automagically

	$abort="NO";
	if ("$result" eq "") {
		$result=NORESULT;
		$resnum=7;
	} elsif (&getcode($result)!=0) {     # sets $resnum & $abort
		$result="NO RESULT NAME";
		$resnum=-1;
	}

	$time=&time;
	&output(220, "$arg $resnum $time", "$result");

	if ($abort eq YES) {
		&setsigs("tet'abandon");
		&output(510,"","ABORT on result code $resnum \"$result\"");
		if ($cleanup ne "") {
			&docleanup;
		}
		$exitval=1;
		&cleanup;
		exit($exitval);
	}
}

sub docleanup{
	$thistest="";
	$tpcount=0;
	$block=0;
	&setblock;
	if ("$cleanup" ne "") {
		eval("&main'"."$cleanup");
		$@ && ($@ =~ s/\(eval\) line (\d+)/$0 . 
			" line " . ($1+$start)/e, die $@);
	}
}



sub cleanup{
	unlink(@tmpfiles);
}



require "$ENV{\"TET_ROOT\"}/lib/perl/api.pl" ;

#eval <<'End_of_Program';

#args already in $0 and @ARGV

#arrange to clean up on exit

#init this here for lack of a better place
@DELETES_FILE=();



# check for journal file descriptor
# note that JOURNAL_HANDLE is an indirect reference to the actual file handle
# and is used that way in the API


$journal_path=$ENV{"TET_JOURNAL_PATH"};
if (!defined($journal_path)) 
{
	$journal_fd="/dev/tty";
	$JOURNAL_HANDLE=STDOUT;
}
else 
	{
	if (open(JOURNAL_HANDLE_REAL,">>$journal_path")) {
		$JOURNAL_HANDLE=JOURNAL_HANDLE_REAL;
	} 
else 
	{
		$JOURNAL_HANDLE=STDOUT;
	}
}

#no matter what, make sure output is unbuffered.
select((select($JOURNAL_HANDLE), $|=1)[0]);
	



# read in configuration variables and make them readonly
# strip comments and other non-variable assignments
# protect embedded spaces and single quotes in the value part
#
#

$tet_config=$ENV{"TET_CONFIG"};


if ($tet_config ne "" )
{
	if (-r $tet_config) {
		local($FILE);
		open(FILE,"<$tet_config");
		while (<FILE>) {
			/^#/ && next;
			/^[\b]*$/ && next;
			!/^[^\b]+=/ && next;
			s/^/\$/;
			s/=(.*$)/=\"\1\";/;
#			print;
			eval;
		}
		close(FILE);
	} else {
		&error("can't read config file $tet_config");
	}
}


	



&setcontext;

$code=$ENV{"TET_CODE"};

if ("$code" eq "") {$code=tet_code;}


local($TET_CODE_HANDLE);
local($fail)=0;


if (open(TET_CODE_HANDLE,"<$code")) {
	@TET_CODE_FILE=<TET_CODE_HANDLE>;
	close(TET_CODE_HANDLE);
} else {
 
	if (tet_code ne "$code") {
		&error("could not open results code file $code");
	}
	@TET_CODE_FILE=("0   PASS        Continue\n",
		"1   FAIL        Continue\n",
		"2   UNRESOLVED  Continue\n",
		"3   NOTINUSE    Continue\n",
		"4   UNSUPPORTED Continue\n",
		"5   UNTESTED    Continue\n",
		"6   UNINITIATED Continue\n",
		"7   NORESULT    Continue\n");

} 

#process command-line args
$pname=$0;

if ($#ARGV<0) {$ARGV[0]="all";}

$iclast = -1;
#($iclist = $main'iclist)  =~ tr/" 0123456780"#/cd;
@iclist=@main'iclist;

if ($#iclist<0) {
	&error("IClist is not defined");
	die;
}

foreach(@iclist) {
	tr/" 0123456789"//cd;
}

#if("$iclist" eq " ") {$iclist=0;}

$icfirst_def=@iclist[0];
#$icfirst_def =~ s/ .*//;

$iccount=0;

#split comma separate list into separate items
foreach(@ARGV) {
	local(@B)=split(/,/);
	@A=(@A,@B);
};

@ARGV=@A;
foreach(@ARGV) {
	CASE_PAT: {
		/all.*/ && do
			{
				if ($iclast<0) {
					$icfirst=$icfirst_def;
					foreach (@iclist) {
						if ($_<$icfirst) { $icfirst=$_;}
					}
				} else {
					$icfirst=$iclast+1;
				}
				$iclast=$icfirst;
				$_=0;
				foreach(@iclist) {
					if ($_>$iclast) {$iclast=$_;}
				}
				#if ($iclast>$_) {$iclast=$_;}
				last CASE_PAT;
			};
		/.*/ && do
			{
				local($save)=$_;
				s/^([0-9]*).*/\$icfirst=\1;/;
				eval;
				$_=$save;
				s/^[^\-]*-*//;
				s/^([0-9]*).*/\$iclast=\1;/;
				s/=;/="";/;
				eval;
			};
	}
	
	$icno=("$icfirst" eq "") ? $icfirst_def : $icfirst;


	$iclast = ($iclast eq "") ? $icno : $iclast;

	while ($icno <= $iclast) {
		if (grep(/\b$icno\b/,@iclist)) {
			$a="\$#main'ic"."$icno";
			if (eval("\$#main'ic"."$icno") > -1) {
				$tests[$iccount++]="ic$icno";
			} else {
				&error("IC $icno is not defined for this test case\n");
			}
		}
		++$icno;
	}
}


# print startup message to execution results file
&output(15,"1.10.3 $iccount","TCM Start");

# do initial signal list processing
$#sig_leave2=-1;
foreach (@sig_leave)
{
	print "Process signal $_\n";
	if (&ismember($_,$std_signal) || &ismember($_,$spec_signals)) {
		&error("warning: illegal entry $_ in tet'sig_leave ignored");
	} else {
		$sig_leave2[$#sig_leave2+1]=$_;
	}
}

$#sig_ign2=-1;
foreach (@sig_ign)
{
	print "Process signal $_\n";
	if (&ismember($_,$std_signal) || &ismember($_,$spec_signals)) {
		&error("warning: illegal entry $_ in tet'sig_ign ignored");
	} else {
		$sig_ign2[$#sig_ign2+1]=$_;
	}
}

@sig_leave2=(@sig_leave2,@spec_signals);

$signal_actions[$NSIG-1]="";


for (local($S)=1;$S<$NSIG;++$S){
	if (&ismember($S,$sig_leave2)) {
		$signal_actions[$S]=leave;
	} elsif (&ismember($S,$sig_ign2)) {
		$signal_actions[$S]=ignore;
	} else {
		$signal_actions[$S]=default;
	}
}


#do startup processing
&setsigs("tet'abandon");

if ("$startup" ne "") 
{
	eval ("&main'"."$startup");
	$@ && ($@ =~ s/\(eval\) line (\d+)/$0 . 
		" line " . ($1+$start)/e, die $@);
}

for $icname (@tests) {
	$icnumber=$icname;
	$icnumber =~ s/[^0-9]*//;
	$tpmax = $tpcount = eval("\$#main'"."$icname");
	$@ && ($@ =~ s/\(eval\) line (\d+)/$0 . " line " . ($1+$start)/e, die $@);

	++$tpmax;
	
	$time=&time;
	&output(400, "$icnumber $tpmax $time", "IC Start");
	for ($tpcount=1; $tpcount<=$tpmax; ++$tpcount) {
		$thistest=eval("\$main'"."$icname"."[$tpcount-1]");
		$@ && ($@ =~ s/\(eval\) line (\d+)/$0 . 
			" line " . ($1+$start)/e, die $@);
		local($tpnumber)=$tp_ids{$thistest};
		$time=&time;
		&output(200,"$tpnumber $time","TP Start");
		&setcontext;

#		using '$$' would allow for paralle processes to not lock from
#		each other!

		local($timeout_count)=17;

		while (!mkdir("$tet_lock_dir",0700)) {
			sleep(1);
			if (--$timeout_count==0) {
				&error("can't obtain lock dir $tet_lock_dir");
				die;
			}
		}
		open(TMPRES,"+>$tmpres");
		unlink("$tmpres");
		rmdir("$tet_lock_dir");

		if (&get_reason($thistest) == 0 ) {
			&infoline($_);
			&result("UNINITIATED");
		} else {
			&setsigs("tet'sigskip");
			{
				&defaultsigs;
				eval("\&main'"."$thistest");
				$@ && ($@ =~ s/\(eval\) line (\d+)/$0 . 
					" line " . ($1+$start)/e, die $@);
			}
		}
		&tpend($tpnumber);
	}
	$time=&time;
	--$tpcount;
	&output(410,"$icnumber $tpcount $time","IC End");
}

&setsigs("tet'abandon");

if ($cleanup ne "") {
	&docleanup;
}

$TET_EXITVAL=0;
&cleanup;
exit(0);

#End_of_Program

&cleanup;

$@ && ($@ =~ s/\(eval\) line (\d+)/$0 . " line " . ($1+$start)/e, die $@);

exit($exitval);
