#!/usr/bin/nawk -f
BEGIN { FS="=";
		while (getline < ARGV[1])
			arr1[$1] = $2;
		while (getline < ARGV[2])
		if ($1 in arr1)
			printf "%s=%s\n",$1,arr1[$1];
		else
		   print $0
	  }
				

