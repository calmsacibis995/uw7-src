#ident  "$Header$"
#ident  "@(#)msg.h	1.5"

/*	This file contains the messages for all the files under cmd/saf. As new
	messages are needed they are added to this file. If a new message is 
	required for a specific file, it is added below.

	These messages are used by pfmt() and gettxt() where appropriate.
*/


static char 

	/* New messages for admutil.c */

	*MSG1 = ":1:Could not stat <%s>.\n",
	*MSG2 = ":2:%s not a regular file -ignored.\n",
	*MSG3 = ":3:Invalid request, cannot access %s.\n",
	*MSG4 = ":4:Invalid request, can not open %s.\n",
	*MSG5 = ":5:Invalid request, script does not exist.\n",
	*MSG6 = ":6:Invalid request, can not open script.\n",
	*MSG7 = ":7:Tempfile busy; try again later.\n",
	*MSG8 = ":8:Cannot create tempfile.\n",
	*MSG9 = ":9:Could not set level on %s.\n",
	*MSG10 = ":10:Could not set attributes on %s.\n",
	*MSG11 = ":11:Unable to rename temp file %s.\n",
	*MSG12 = ":12:Error reading _sactab.\n",
	*MSG13 = ":13:Error in writing tempfile.\n",
	*MSG14 = ":14:Error closing tempfile.\n",
	*MSG15 = ":15:Error setting attributes on %s.\n",
	*MSG16 = ":16:Error setting level on %s.\n",
	*MSG17 = ":17:Unable to find lid of SYS_PRIVATE.\n",
	*MSG18 = ":18:Could not determine if MAC is installed.\n",

	/* New messages for log.c */
	
	*MSG19 = ":19:Could not open logfile.\n",
	*MSG20 = ":20:Could not lock logfile.\n",
	*MSG21 = ":21:Could not open debugfile.\n",
	
	/*
	 * New messages for misc.c 
	 *
	 * Have to use gettxt() here unfortunately.
	 */

	*MSGID22 = ":22",	*MSG22 = "Got AC_START for <%s>",
	*MSGID23 = ":23",	*MSG23 = "Got AC_KILL for <%s>",
	*MSGID24 = ":24",	*MSG24 = "Got AC_ENABLE for <%s>",
	*MSGID25 = ":25",	*MSG25 = "Terminating <%s>",
	*MSGID26 = ":26",	*MSG26 = "Got AC_ENABLE for <%s>",
	*MSGID27 = ":27",  *MSG27 = "Got AC_DISABLE for <%s>",
	*MSGID28 = ":28",  *MSG28 = "Got AC_STATUS",
	*MSGID29 = ":29",  *MSG29 = "ak_size is %d",
	*MSGID30 = ":30",  *MSG30 = "Could not send info",
	*MSGID31 = ":31",  *MSG31 = "Got AC_SACREAD",
	*MSGID32 = ":32",  *MSG32 = "Got AC_PMREAD for <%s>",
	*MSGID33 = ":33",  *MSG33 = "Got unknown message for <%s>",
	*MSGID34 = ":34",  *MSG34 = "Could not send ack",
	*MSGID35 = ":35",  *MSG35 = "Message to <%s> failed",
	*MSGID36 = ":36",  *MSG36 = "Can not open _pid file for <%s>",

	/*
	 * New Messages for global.c 
	 *
	 * Have to use gettxt() here unfortunately.
	 */

	*MSGID37 = ":37",  *MSG37 = "Could not open _sactab",
	*MSGID38 = ":38",  *MSG38 = "Malloc failed",
	*MSGID39 = ":39",  *MSG39 = "_sactab file is corrupt",
	*MSGID40 = ":40",  *MSG40 = "_sactab version # is incorrect",
	*MSGID41 = ":41",  *MSG41 = "Can not chdir to home directory",
	*MSGID42 = ":42",  *MSG42 = "Could not open _sacpipe",
	*MSGID43 = ":43",  *MSG43 = "Internal error - bad state",
	*MSGID44 = ":44",  *MSG44 = "Read of _sacpipe failed",
	*MSGID45 = ":45",  *MSG45 = "Fattach failed",
	*MSGID46 = ":46",  *MSG46 = "I_SETSIG failed",
	*MSGID47 = ":47",  *MSG47 = "Read failed",
	*MSGID48 = ":48",  *MSG48 = "Poll failed",
	*MSGID49 = ":49",  *MSG49 = "System error in _sysconfig",
	*MSGID50 = ":50",  *MSG50 = "Error interpreting _sysconfig",
	*MSGID51 = ":51",  *MSG51 = "Pipe failed",
	*MSGID52 = ":52:",  *MSG52 = "Could not create _cmdpipe",
	*MSGID53 = ":53:",  *MSG53 = "Could not set ownership",
	*MSGID54 = ":54:",  *MSG54 = "Could not set level of process",
	*MSGID55 = ":55:",  *MSG55 = "Could not get level of process",
	*MSGID56 = ":56:",  *MSG56 = "Could not get level identifiers",
	
	/* New messages for pmadm.c */

	*MSG57 = ":57:Tag too long, truncated to <%s>.\n",
	*MSG58 = ":58:Svctag too long, truncated to <%s>.\n",
	*MSG59 = ":59:Type too long, truncated to <%s>.\n",
	*MSG60 = ":60:%s already exists under %s - ignoring.\n",
	*MSG61 = ":61:%s does not exist under %s - ignoring.\n",
	*MSG62 = ":62:Fork failed - could not notify <%s> about modified table.\n",
	*MSG63 = ":63:Try executing the command \"sacadm -x -p %s\"\n",
	*MSG64 = ":64:Port monitor, %s is not running.\n",
	*MSG65 = ":65:Could not notify <%s> about modified table\n",
	*MSG66 = ":66:Try executing the command \"sacadm -x -p%s\"\n",
	*MSG67 = ":67:Service <%s> does not exist.\n",
	*MSG68 = ":68:No services defined.\n",
	*MSG69 = ":69:Usage:%s -a [ -p pmtag | -t type ] -s svctag [ -i id ] -m \"pmspecific\"\n",
	*MSG70 = ":70:-v version [ -f xu ] [ -S scheme ] [ -y comment] [ -z script ]\n",
	*MSG71 = ":71:%s -r -p pmtag -s svctag\n",
	*MSG72 = ":72:%s -e -p pmtag -s svctag\n",
	*MSG73 = ":73:%s -d -p pmtag -s svctag\n",
	*MSG74 = ":74:%s -l [ -p pmtag | -t type ] [ -s svctag ]\n",
	*MSG75 = ":75:%s -L [ -p pmtag | -t type ] [ -s svctag ]\n",
	*MSG76 = ":76:%s -g -p pmtag -s svctag [ -z script ]\n",
	*MSG77 = ":77:%s -g -s svctag -t type -z script\n",
	*MSG78 = ":78:%s -c [ -i id ] [ -S scheme ] -p pmtag -s svctag\n",
	*MSG79 = ":79:Invalid user identity.\n",
	*MSG80 = ":80:Port monitor tag must be alphanumeric.\n",
	*MSG81 = ":81:Service tag must not begin with '_'\n",
	*MSG82 = ":82:Service tag must be alphanumeric.\n",
	*MSG83 = ":83:Port monitor type must be alphanumeric.\n",
	*MSG84 = ":84:Version must be numeric.\n",
	*MSG85 = ":85:Version number can not be negative.\n",	
	*MSG86 = ":86:Invalid request, %s are not valid arguments for \"-f\"",
	*MSG87 = ":87:_sactab version number is incorrect.\n",
	*MSG88 = ":88:Could not open %s.\n",
	*MSG89 = ":89:%s file is corrupt.\n",
	*MSG90 = ":90:Could not open _sactab.\n",
	*MSG91 = ":91:Invalid request, %s does not exist.\n",
	*MSG92 = ":92:%s version number is incorrect.\n",
	*MSG93 = ":93:Could not open %s.\n",
	*MSG94 = ":94:Invalid request, %s already exists under %s.\n",
	*MSG95 = ":95:Could not get level identifier.\n",
	*MSG96 = ":96:Could not set level of %s/%s.\n",
	*MSG97 = ":97:Could not change ownership of %s/%s.\n",
	*MSG98 = ":98:Invalid request, %s does not exist under %s.\n",
	*MSG99 = ":99:Error accessing temp file.\n",
	*MSG100 = ":100:Error closing temp file.\n",
	*MSG101 = ":101:No configuration scripts installed.\n",
	*MSG102 = ":102:Error reading %s.\n",
	*MSG103 = ":103:Error reading %s/%s/_pmtab.\n",
	*MSG104 = ":104:%s/%s/_pmtab is corrupt.\n",
	*MSG105 = ":105:Unrecognized flag <%c>.\n",
	*MSG106 = ":106:Internal error in pflags.\n",
	*MSG107 = ":107:Malloc failed.\n",
	*MSG108 = ":108:Error reading _sactab.\n",
	
	/*
	 * New messages for readtab.c 
	 *
	 * Have to use gettxt() here unfortunately.
	 */

	*MSGID109 = ":109",  *MSG109 = "Ignoring duplicate entry for <%s>",
	*MSGID110 = ":110",  *MSG110 = "Could not send SIGTERM to <%s>",
	*MSGID111 = ":111",  *MSG111 = "Terminating <%s>",

	/* Messages for sacadm.c */

	*MSG112 = ":112:Command must be a full pathname.\n",
	*MSG113 = ":113:Restart count can not be negative.\n",
	*MSG114 = ":114:Usage:%s -a -p pmtag -t type -c cmd -v ver [ -f dx]\n",
	*MSG115 = ":115:[ -n count ] [ -y comment ] [ -z script]\n",
	*MSG116 = ":116:%s -r -p pmtag\n",
	*MSG117 = ":117:%s -s -p pmtag\n",
	*MSG118 = ":118:%s -k -p pmtag\n",
	*MSG119 = ":119:%s -e -p pmtag\n",
	*MSG120 = ":120:%s -d -p pmtag\n",
	*MSG121 = ":121:%s -l [ -p pmtag | -t type ]\n",
	*MSG122 = ":122:%s -L [ -p pmtag | -t type ]\n",
	*MSG123 = ":123:%s -g -p pmtag [ -z script ]\n",
	*MSG124 = ":124:%s -G [ -z script ]\n",
	*MSG125 = ":125:\t%s -x [ -p pmtag ]\n",
	*MSG126 = ":126:Invalid request, %s already exists.\n",
	*MSG127 = ":127:<%s> exists and is not a directory.\n",
	*MSG128 = ":128:Could not fork.\n",
	*MSG129 = ":129:Could not remove files under <%s>.\n",
	*MSG130 = ":130:Could not create directory <%s>.\n",
	*MSG131 = ":131:Could not create communications pipe.\n",
	*MSG132 = ":132:Could not create _pid file",
	*MSG133 = ":133:Could not create _pmtab file",
	*MSG134 = ":134:Error initializing _pmtab.\n",
	*MSG135 = ":135:%s not executable.\n",
	*MSG136 = ":136:%s not a regular file\n",
	*MSG137 = ":137: %s does not exist\n",
	*MSG138 = ":138:Could not find level identifiers.\n",
	*MSG139 = ":139:Not able to set levels for %s files & directories.\n",
	*MSG140 = ":140:Not able to chown() %s files & directories.\n",
	*MSG141 = ":141:No port monitors defined.\n",
	*MSG142 = ":142:Improper message from SAC\n",
	*MSG143 = ":143:Could not allocate storage.\n",
	*MSG144 = ":144:Unable to lock command pipe.\n",
	*MSG145 = ":145:Port monitor, %s, is already running.\n",
	*MSG146 = ":146:Internal error - sent invalid command.\n",
	*MSG147 = ":147:Could not contact %s.\n",
	*MSG148 = ":148:Could not start %s - _pid file locked.\n",
	*MSG149 = ":149:Port monitor, %s, is in recovery.\n",
	*MSG150 = ":150:This request could not be completed - see sac log file for details.\n",
	*MSG151 = ":151:Unknown response.\n",
	*MSG152 = ":152:Bad information from SAC.\n",
	*MSG153 = ":153:Could not ascertain sac status.\n",

	/*
	 * New messages for util.c 
	 *
	 * Have to use gettxt() here unfortunately.
	 */

	*MSGID154 = ":154", *MSG154 = "Tag too long, truncated to <%s>",
	*MSGID155 = ":155", *MSG155 = "Type too long, truncated to <%s>",
	*MSGID156 = ":156", *MSG156 = "Unrecognized flag <%c>",

	/* New messages for sac.c */

	*MSG157 = ":157:Usage: sac -t sanity_interval\n",

	*MSGID158 = ":158", *MSG158 = "*** SAC starting ***",
	*MSGID159 = ":159", *MSG159 = "Error in _sysconfig: line %d",
	*MSGID160 = ":160", *MSG160 = "Could not open _pmpipe for port monitor <%s>",
	*MSGID161 = ":161", *MSG161 = "SAC in recovery",
	*MSGID162 = ":162", *MSG162 = "Ambiguous utmp entry <%.8s>",
	*MSGID163 = ":163", *MSG163 = "Could not start <%s> - _pid file locked",
	*MSGID164 = ":164", *MSG164 = "Could not set attributes for port monitor <%s>, errno is %d",
	*MSGID165 = ":165", *MSG165 = "Could not create _pmpipe for port monitor <%s>, errno is %d",
	*MSGID166 = ":166", *MSG166 = "Could not open _pmpipe for port monitor <%s>, errno is %d",
	*MSGID167 = ":167", *MSG167 = "Could not fork port monitor <%s>",
	*MSGID168 = ":168", *MSG168 = "Missing utmp entry for <%s>",
	*MSGID169 = ":169", *MSG169 = "Could not remove utmp entry for <%s>",
	*MSGID170 = ":170", *MSG170 = "Cannot chdir to <%s/%s>, port monitor not started",
	*MSGID171 = ":171", *MSG171 = "System error in _config script for <%s>",
	*MSGID172 = ":172", *MSG172 = "Error in _config script for <%s>: line %d",
	*MSGID173 = ":173", *MSG173 = "Can't expand port monitor <%s> environment",
	*MSGID174 = ":174", *MSG174 = "Starting port monitor <%s>",
	*MSGID175 = ":175", *MSG175 = "Exec of port monitor <%s> failed",
	*MSGID176 = ":176", *MSG176 = "Invalid command line, non-terminated string for port monitor %s",
	*MSGID177 = ":177", *MSG177 = "Internal error in parse routine",
	*MSGID178 = ":178", *MSG178 = "<%s> has stopped",
	*MSGID179 = ":179", *MSG179 = "<%s> stopped responding to sanity polls - trying to restart",
	*MSGID180 = ":180", *MSG180 = "<%s> has died - trying to restart",
	*MSGID181 = ":181", *MSG181 = "<%s> has FAILED",
	*MSGID182 = ":182", *MSG182 = "Port monitor <%s> didn't recognize message",
	*MSGID183 = ":183", *MSG183 = "Port monitor <%s> reporting invalid state",
	*MSGID184 = ":184", *MSG184 = "Port monitor <%s> sent an invalid message -ignoring it",

	/* More sacadm messages */

	*MSGID185 = ":185", *MSG185 = "Can not contact SAC\n",
	*MSGID186 = ":186", *MSG186 = "Embedded newlines not allowed\n",
	*MSGID187 = ":187", *MSG187 = "*** SAC exiting ***\n";
