#ident	"@(#)sizer.awk	15.1"


BEGIN {var=home=usr=root=0;}

{
num = split(ARGV[1], name, "/")
if ($2 == "f" || $2 == "v" ) {
	if ($4 ~ /^\/var\//)
		var = var + $8/1024;
	else 
	if ($4 ~ /^\/usr\//)
		usr = usr + $8/1024;
	else 
# If any package starts installing significant content in /home then the
# following lines should eb uncommented, at the time this script was
# written there was no need, and eliminating a bunch of array elements
# was a good thing
#
#	if ($4 ~ /^\/home\//)
#		home = home + $8/1024;
#	else 
		root = root + $8/1024;
}

}
END {
printf("%s[1]=%d\t%s[3]=%d\t%s[4]=%d\n",name[num-1], root/1024 + .99,name[num-1], usr/1024 + .99,name[num-1], (var/1024 + .99));
#printf("%s[1]=%d\t%s[3]=%d\t%s[4]=%d\t%s[11]=%d\n",name[num-1], root/1024 + .99,name[num-1], usr/1024 + .99,name[num-1], (var/1024 + .99),name[num-1], home/1024 + .99);
}
