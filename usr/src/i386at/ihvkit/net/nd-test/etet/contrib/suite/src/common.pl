

sub pass {
	&tet'result("PASS");
	print "$testname $tp PASSED\n";
}

sub fail {
	&tet'result("FAIL");
	print "$testname $tp ***FAILED***\n";
}


sub verify {
	if ($_[0]) {&pass;}
	else {&fail;}
}



sub outerror {
	&tet'infoline($_[0]);
	print STDERR "$_[0]\n";
}

sub outmessage {
	&tet'infoline($_[0]);
	print "$_[0]\n";
}


sub compare_arrays{
	local(*a,*b)=@_;

#	first check lengths
	if (@a!=@b) {
#		print "a!=b\n";
		return 0;
	}
	if ($DEBUG) {
		$a=join('',@a);
		$b=join('',@b);

#		print "a=$a\n";
#		print "b=$b\n";
	
		$la=length($a);
		$lb=length($b);
	
#		print "la=$la, lb=$lb\n";
	
		if ($a eq $b) {
#			print "\$a=\$b\n";
		}
	
		return($a eq $b);
	} else {
		return (join('',@a) eq join('',@b));
	}
}
	


chop($OUTPUT=`/bin/pwd`);
$OUTPUT=$OUTPUT."/output";

$ENV{OUTPUT}=$OUTPUT;

$testname=$0;

$testname =~ s!.+/([^/]+)$!$1!;


# constants

$TEST_CASE_START=10;
$TEST_CASE_END=80;
$TEST_PURPOSE_RESULT=220;
$INVOCABLE_COMPONENT_START=400;
$INVOCABLE_COMPONENT_END=410;
$TEST_CASE_INFORMATION=520;



sub breakup_infoline {
#	break out components of an infoline from the journal

	$_=$_[0];
	($activity,$tpnumber,$context,$block,$sequence,$message) =
		/^$TEST_CASE_INFORMATION\|(\d+) (\d+) (\S+) (\d+) (\d+)\|(.*)$/;
}


sub breakup_result {
#	break out components of a result line from the journal

	$_=$_[0];
	($activity,$tpnumber,$result,$time,$result_text)=
		/^$TEST_PURPOSE_RESULT\|(\d+) (\d+) (\d+) (\S+)\|(.*)$/;
}



sub breakup_tcstart {
#	break out components of a tcstart line from the journal

	$_=$_[0];
	($activity,$testcase,$time,$text)=
		/^$TEST_CASE_START\|(\d+) (\S+) (\S+)\|(.*)$/;
}



sub run {
#	run tcc with the specified arguments & trap output.
#	get the output and read it into @output
#	get the journal file name and read into @journal.

	#make sure old output is gone
	unlink($OUTPUT);

	$getjournal=1;
	if ($#_!=-1) {
		system ("tcc @_ >>$OUTPUT 2>&1");
	} else {
		system ("tcc >>$OUTPUT 2>&1");
		$getjournal=0;
	}
		
	open(F,"<$OUTPUT") || &outerror("Failure to read output $OUTPUT");
	@output=<F>;
	close(F);

	if ($getjournal) {
		@line=grep(/journal file name is/,@output);

#		print "output=@output,\nline=@line\n";
	
		$_=$line[0];
#		print "_=$_\n";
	
		s/journal file name is: //;
	
		$journal_pathname=$_;
	
		open(F,"<$journal_pathname") || 
			&outerror("Failure to read journal $journal_pathname");
		@journal=<F>;
		close(F);
	}

}
