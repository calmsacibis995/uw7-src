/^[a-zA-Z_][a-zA-Z0-9_]*:/ {
	name = $1
	next
}
name != "" {
	if ($1 == ".zero") $2 = "0"
	printf "\t.set\t%s,%s\n",substr(name,1,length(name)-1),$2
	name = ""
}
