BEGIN{
	p_cnt=0;
	first_time = 1;
}
{
	if ( "$1" == "UID" )
		continue;
	user_name[p_cnt] = "$1";
	proc_id[p_cnt] = $2;
	pproc_id[p_cnt]= $3;
	if ( index($5,":") == 0 )
		s_fld = 9;
	else
		s_fld = 8;
	proc_name="";
	while ( s_fld <= NF ) {
		proc_name = sprintf("%s %s",proc_name,$s_fld);
		s_fld++;
	}
	p_name[p_cnt] = proc_name;
	sub_string = substr(p_name[p_cnt],0,18);
	if ( sub_string != " nawk -f procs.awk") {
		if (index(p_name[p_cnt],PROC_NAME) != 0) {
			if (first_time) {
				st_pid = $2;
				st_ppid = $3;
				st_pname=proc_name;
				first_time = 0;
			}
		}
	}
	p_cnt++;
}
END{
	error_occurred = 0;
	prcs=0;
	if (st_pid != 0) {
		p_id[prcs] = st_pid;
		pp_id[prcs] = st_ppid;
		p_na[prcs] = st_pname;
		prcs++;
		display_pid(st_pid);
		if ( ACT == "SHOW") {
			for(i=0;i<prcs;i++) {
				printf "%d %d %s\n",p_id[i],pp_id[i],p_na[i];
			}
		}
		else {
			if (ACT == "KILL") {
				for(i=0;i<prcs;i++) {
					cmd=sprintf("kill -9 %d >/dev/null 2>&1",p_id[i]);
					ret_val=system(cmd);
					ret_val2=1;
					if (ret_val != 0) {
						cmd=sprintf("kill -0 %d >/dev/null 2>&1",p_id[i]);
						ret_val2=system(cmd);
#						printf "%s\n", cmd;
					}
					if (ret_val2 == 0) {
						printf "ERROR while killing %s.\n", p_na[i];
						error_occurred = 1;
					}
				}
			}
		}
		exit(error_occurred);
	}
	else
		exit(1);
}
function display_pid(start_pid)
{
	for(i=0;i<p_cnt;i++) {
		if (pproc_id[i] == start_pid) {
			p_id[prcs] = proc_id[i];
			pp_id[prcs] = pproc_id[i];
			p_na[prcs] = p_name[i];
			prcs++;
			display_pid(proc_id[i]);
		}
	}
	return(0);
}
