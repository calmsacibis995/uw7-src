BEGIN{
	p_cnt=1;
	first_time = 1;
	A=0;
}
{
	if ( "$1" == "UID" )
		continue;
	if ( index($5,":") == 0 )
		s_fld = 9;
	else
		s_fld = 8;
	proc_name="";
	while ( s_fld <= NF ) {
		proc_name = sprintf("%s %s",proc_name,$s_fld);
		s_fld++;
	}
	
	if (index(proc_name,"RUN") !=0 ){
		p_name[p_cnt] = proc_name;
		user_name[p_cnt] = "$1";
		proc_id[p_cnt] = $2;
		pproc_id[p_cnt]= $3;
		parent[$3]=p_cnt
		p_cnt++;
         }
}
END{
	for(i=1;i<p_cnt;i++ ) {
	if (parent[proc_id[i]]!=0)
		proc_id[parent[proc_id[i]]]=0; 
	}
	for(i=1;i<p_cnt;i++ ) {
	if (proc_id[i] !=0 ){
	if ( ACT == "SHOW" )
		printf "{%d %d %s},\n",proc_id[i],pproc_id[i],p_name[i] >PROCFILE;
	else
		printf "%d %d %s\n",proc_id[i],pproc_id[i],p_name[i] >PROCFILE;
	A++;
			}
			}
printf "%d\n",A
}
