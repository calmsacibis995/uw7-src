#ident	"@(#)kern-i386:util/symbols.awk	1.2.2.1"

/^__SYMBOL__[a-zA-Z0-9_]*:/ {
	name = substr($1, 11, length($1)-11)
	next
}
name != "" {
	if ($1 ~ /\.zero/) $2 = "0"
	if (CH != "") {
		printf "#define\t%s\t[%s]\n", name, $2 >CH
	}
	if (AH != "") {
		printf "\tdefine(`%s',`ifelse($#,0,`%s',`%s'($@))')\n", name, $2, $2 >AH
	}
	name = ""
}
