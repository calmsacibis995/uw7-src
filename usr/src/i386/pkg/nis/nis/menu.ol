#ident	"@(#)menu.ol	1.3"
#ident  "$Header$"

.ul
Network Information Service (NIS) Installation and Configuration
.lr
F1=Help
.top
`echo  "The following NIS configuration already exists on this machine."`
`echo "\n \n \n"`
`[ "$TYPE" = "1" ] && echo "    This machine is a NIS master server.\n \n"`
`[ "$TYPE" = "2" ] && echo "    This machine is a NIS slave server.\n \n"`
`[ "$TYPE" = "3" ] && echo "    This machine is a NIS client.\n \n"`
`echo "    NIS domain: $NIS_DOMAIN"` 
`echo "\n \n    NIS servers for domain $NIS_DOMAIN: \c" && cat /tmp/nis.overlay | xargs echo`
.button
Apply
Reset
.bottom
Press 'TAB' to move the cursor between fields.  When finished, move the
cursor to "Apply" and then press 'ENTER' to continue.
.form
2 2//Yes::Yes//No::No//Use current NIS configuration?://USE_CURRENT//
//Use Left/Right arrow keys to choose Yes to use current NIS configuration//
.help
This machine has already been configured with NIS. This installation
can proceed using the current NIS configuration (Enter Yes), or
a new NIS configuration can be created by the NIS installation 
script (Enter No).

Note: This menu only displays the first three NIS servers configured 
      for this domain.
.helpinst
Del=Cancel F1=Help ESC=Exit help 1=Forward 2=Back
