#!/usr/bin/perl
$TET_ROOT=$ENV{TET_ROOT};


stat("template.pl");
if (! -f _ || ! -r _) {
	die "Can't find ./template.pl.\n";
}

stat("sig");
if (! -f _ || ! -r _ || ! -x _) {
	die "Can't find executable ./sig.\n";
}

#now must create tcm.pl from template.pl
open(TEMPLATE,"<template.pl") || die("Can't open template.pl");
@template=<TEMPLATE>;
close(TEMPLATE);

$SIGS=`./sig`;
chop($SIGS);
if ($SIGS eq "") {
	$SIGS="NONE HUP INT QUIT ILL TRAP IOT EMT FPE KILL BUS SEGV SYS PIPE ALRM TERM";
}

$COMMA_SIGS=$SIGS;
$COMMA_SIGS =~ s/ /,/g;
@sig_list = split(/ /,$SIGS);
$NSIG=$#sig_list+1;
grep(s/__NSIG__/$NSIG/,@template);
grep(s/__SIGNAMES__/$COMMA_SIGS/,@template);
open(TCM,">tcm.pl") || die("Can't open tcm.pl for output");
print TCM @template;
close(TCM);

