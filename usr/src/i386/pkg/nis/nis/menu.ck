#ident	"@(#)menu.ck	1.3"
#ident  "$Header$"

.ul
Network Information Service (NIS) Installation and Configuration
.lr
F1=Help
.top
`echo  "\n \nYou have chosen the following NIS configuration:"`
`echo  "\n \n  $host is a NIS $host_type"` 
`echo  "\n \n  NIS domain: $NIS_DOMAIN"`
`[ "$slavep" = "F" ] && echo  "\n \n  NIS servers for domain $NIS_DOMAIN: $SERV1"`
`[ "$slavep" = "F" ] && echo  "                                       $SERV2"`
`[ "$slavep" = "F" ] && echo  "                                       $SERV3"`
`[ "$slavep" = "T" ] && echo  "\n \n  NIS master server for domain $NIS_DOMAIN: $master"`
`echo	"\n \nTo configure again or to cancel NIS configuration enter No."`
.button
Apply
Reset
.bottom
Press 'TAB' to move the cursor between fields.  When finished, move the
cursor to "Apply" and then press 'ENTER' to continue.
.form
2 2//No::No//Yes::Yes//Accept NIS configuration?://ACCEPT//
//Use Left/Right arrow keys to choose Yes to accept configuration//
.help
If this NIS configuration is not acceptable, enter "No" and you will
be given the opportunity to configure NIS again or cancel the configuration
on the subsequent menu. Regardless of whether or not NIS configuration is
cancelled, NIS will still be installed.

Note: This menu only displays the first three NIS servers configured 
      for this domain.
.helpinst
Del=Cancel F1=Help ESC=Exit help 1=Forward 2=Back
