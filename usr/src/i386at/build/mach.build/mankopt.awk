#ident	"@(#)oemcd:mach.build/mankopt.awk	1.1"
#awkcchint -u MP
#awkcchint -u ISCOMPR
#awkcchint -o mankopt.awkcc

	$2 == "f" || $2 == "v" {
		new_path = getpath("noi",$4)
		$9 = my_sum(new_path)+0
		if ($9 != "")
			print $0
	}
	$2 == "i" {
		new_path = getpath("i",$3)
		$5 = my_sum(new_path)+0
		if ($5 != "")
			print $0
	}
	$2 != "i" && $2 != "f" && $2 != "v" { print $0 }


function getpath(mode,orig,	slash,TYPE) {
	if ( mode == "noi" ) {
		if ( "'" == substr(orig,1,1) ) {
			orig=substr(orig,2,length(orig))
			orig=substr(orig,1,length(orig)-1)
		}
		if ( "/" == substr(orig,1,1) ) {
			slash=""
			TYPE="root"
		} else {
			slash="/"
			TYPE="reloc"
		}
		return sprintf("%s%s%s%s",TYPE, (MP == "yes") ? "."$1 : "", slash , orig)
	} else
		if ( orig == "pkginfo" || orig == "setinfo" )
			return orig
		else
			return "install/"orig
}
	

function my_sum(npath,	res,tmp) {
    if ( (ISCOMPR == "yes") && (substr(npath,length(npath)-1,2) != ".Z") ) {
        "/home/builder/ibin/iscompress " npath | getline res
        close("/home/builder/ibin/iscompress " npath)
        if ( res == npath ) {
            "/usr/bin/uncompress < " npath "| /usr/bin/sum -r" | getline res
            close("/usr/bin/uncompress < " npath "| /usr/bin/sum -r")
            split(res, tmp)
            return(tmp[1])
	}
    }
    "/usr/bin/sum -r " npath | getline res
    close ("/usr/bin/sum -r " npath)
    split(res, tmp)
    return(tmp[1])
} 
