#ident  "@(#)msg.h	1.2"
#ident  "$Header$"

static char

				/* Messages for chkey.c */

	*MSG1 = ":1:Can't find master of publickey database.\n",
	*MSG2 = ":2:Cannot convert hostname to netname.\n",
	*MSG3 = ":3:Cannot convert username to netname.\n",
	*MSG4 = ":4:Generating new key for %s.\n",
	*MSG5 = ":5:No yp password entry found: can't change key.\n",
	*MSG6 = ":6:No password entry found: can't change key.\n",
	*MSG7 = ":7:Invalid password.\n",
	*MSG8 = ":8:Password incorrect.\n",
	*MSG9 = ":9:Sending key change request to %s...\n",
	*MSG10 = ":10:%s: unable to update yp database (%u): %s.\n",
	*MSG11 = ":11:%s: unable to update publickey database.\n",
	*MSG12 = ":12:Unable to login with new secret key.\n",
	*MSG13 = ":13:Done.\n",
	*MSG14 = ":14:usage: %s [-f].\n",

				/* Messages for domainname.c */

	*MSG15 = ":15:%s: USAGE: %s [ domain ].\n",
	
				/* Messages for init_tr.c */

	*MSG16 = ":16:%s: %d lookup routines :\n",
	*MSG17 = ":17:%s: Cannot open connection.\n",
	*MSG18 = ":18:%s: Cannot allocate netbuf.\n",
	*MSG19 = ":19:My address is %s.\n",
	*MSG20 = ":20:%s: cannot bind.\n",
	*MSG21 = ":21:%s: address in use.\n",
	*MSG22 = ":22:%s: could not create service.\n",
	*MSG23 = ":23:Could not register %s version %d.\n",
	
				/* Messages for keylogin.c */

	*MSG24 = ":24:Cannot get network name.\n",
	*MSG25 = ":25:Cannot find %s's secret key.\n",
	*MSG26 = ":26:Password incorrect for %s.\n",
	*MSG27 = ":27:Could not set %s's secret key.\n",
	*MSG28 = ":28:Maybe the keyserver is down?\n",
	
				/* Messages for keylogout.c */

	*MSG29 = ":29:Keylogout by root would break all servers that use secure rpc!\n",
	*MSG30 = ":30:Root may use keylogout -f to do this (at your own risk)!\n",
	*MSG31 = ":31:Could not unset your secret key.\n",
		
				/* Messages for keyserv.c */

	*MSG32 = ":32:%s must be run as root.\n",
	*MSG33 = ":33:%s: unable to create service.\n",
	*MSG34 = ":34:%s: unable to create service.\n",
	*MSG35 = ":35:Invalid %s.\n",
	*MSG36 = ":36:Password does not decrypt secret key for %s.\n",
	*MSG37 = ":37:Not local privileged process.\n",
	*MSG38 = ":38:Not unix authentication.\n",
	*MSG39 = ":39:Unable to reply.\n",
	*MSG40 = ":40:Unable to free arguments.\n",
	*MSG41 = ":41:_rpc_get_local_uid failed %s %s.\n",
	*MSG42 = ":42:local_uid  %d mismatches auth %d.\n",
	*MSG43 = ":43:Not auth sys.\n",
	*MSG44 = ":44:Usage: keyserv [-n] [-D] [-d]\n",
	*MSG45 = ":45:-d disables the use of default keys.\n",

				/* Messages for newkey.c */

	*MSG46 = ":46:Usage: %s [-u username].\n",
	*MSG47 = ":47:Usage: %s [-h hostname].\n",
	*MSG48 = ":48:Must be superuser to run %s.\n",
	*MSG49 = ":49:Cannot chdir to ",
	*MSG50 = ":50:Unknown user: %s.\n",
	*MSG51 = ":51:Error: failed in routine setnetconfig(0).\n",
	*MSG52 = ":52:Unknown host: %s.\n",
	*MSG53 = ":53:Adding new key for %s.\n",
	*MSG54 = ":54:Password verification is disabled.\n",
	*MSG55 = ":55:Possible Cause: The \"crypt\" package is not installed.\n",
	*MSG56 = ":56:Password incorrect.\n",
	*MSG57 = ":57:Please wait for the database to get updated...\n",
	*MSG58 = ":58:%s: unable to update yp database (%u): %s.\n",
	*MSG59 = ":59:%s: unable to update publickey database (%u): %s.\n",
	*MSG60 = ":60:Your new key has been successfully stored away.\n";
	
