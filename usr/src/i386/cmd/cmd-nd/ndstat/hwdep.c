#ident "@(#)hwdep.c	25.1"
#ident "$Header$"

/*
 *      Copyright (C) The Santa Cruz Operation, 1993-1996.
 *      This Module contains Proprietary Information of
 *      The Santa Cruz Operation and should be treated
 *      as Confidential.
 */

#include <sys/types.h>
#include <sys/stream.h>
#include <stdio.h>
#include <sys/mdi.h>
#include <sys/lli31.h>
#include "ndstat.h"

extern int	hwdep_type;
extern char	hwdep_buf[4096];
		
void
display_hw_dep_stats(void)
{
	int i, j;

	/* see long comment elsewhere about why we don't have any stats
	 * for FDDI or token ring.  rather than report all zeros for these,
	 * skip this routine if ODIDLPI and not ethernet.
	 */
	if ((type == ODIDLPI) && (hwdep_type != MAC_CSMACD)) {
		return;
	}

	switch (hwdep_type) {
	case MAC_CSMACD: {
		mac_stats_eth_t *mp = (mac_stats_eth_t *)hwdep_buf;
catfprintf(stdout, 73, "\n\t\tETHERNET SPECIFIC STATISTICS\n");
catfprintf(stdout, 75, "\n Collision Table - The number of frames successfully transmitted,\n");
catfprintf(stdout, 76, "	           but involved in at least one collision:\n");
catfprintf(stdout, 77, "\n\t               Frames                   Frames\n");
catfprintf(stdout, 315,  "\t              -------                  -------\n");
for (i=0; i<8; i++) {
	if (i == 0)
		catfprintf(stdout, 78, "\t%2d collision  %7u    %2d collisions %7u\n", i+1, mp->mac_colltable[i], i+9, mp->mac_colltable[i+8]);
	else
		catfprintf(stdout, 79, "\t%2d collisions %7u    %2d collisions %7u\n", i+1, mp->mac_colltable[i], i+9, mp->mac_colltable[i+8]);
}
printf("\n\n");
catfprintf(stdout, 80, " Bad Alignment          %7u      Number of frames received that were\n", mp->mac_align);
catfprintf(stdout, 81, "                                     not an integral number of octets\n");
printf("\n");
catfprintf(stdout, 82, " FCS Errors             %7u      Number of frames received that did\n", mp->mac_badsum);
catfprintf(stdout, 83, "                                     not pass the Frame Check Sequence\n");
printf("\n");
catfprintf(stdout, 84, " SQE Test Errors        %7u      Number of Signal Quality Error Test\n", mp->mac_sqetesterrors);
catfprintf(stdout, 85, "                                     signals that were detected by the adapter\n");
printf("\n");
catfprintf(stdout, 86, " Deferred Transmissions %7u      Number of frames delayed on the\n", mp->mac_frame_def);
catfprintf(stdout, 87, "                                     first transmission attempt because\n");
catfprintf(stdout, 316,"                                     the media was busy\n");
printf("\n");
catfprintf(stdout, 88, " Late Collisions        %7u      Number of times a collision was\n", mp->mac_oframe_coll);
catfprintf(stdout, 89, "                                     detected later than 512 bits into\n");
catfprintf(stdout, 317,"                                     the transmitted frame\n");
printf("\n");
catfprintf(stdout, 90, " Excessive Collisions   %7u      Number of frames dropped on transmission\n", mp->mac_xs_coll);
catfprintf(stdout, 91, "                                     because of excessive collisions\n");
printf("\n");
catfprintf(stdout, 92, " Internal MAC Transmit  %7u      Number of frames dropped on transmission\n", mp->mac_tx_errors);
catfprintf(stdout, 93, " Errors                              because of errors not covered above\n");
printf("\n");
catfprintf(stdout, 94, " Carrier Sense Errors   %7u      Number of times that the carrier sense\n", mp->mac_carrier);
catfprintf(stdout, 95, "                                     condition was lost when attempting to\n");
catfprintf(stdout, 97, "                                     send a frame that was deferred for an\n");
catfprintf(stdout, 98, "                                     excessive amount of time\n");
printf("\n");
catfprintf(stdout, 99, " Frame Too Long         %7u      Number of frames dropped on reception\n", mp->mac_badlen);
catfprintf(stdout,100, "                                     because they were larger than the\n");
catfprintf(stdout,101, "                                     maximum Ethernet frame size\n");
printf("\n");
catfprintf(stdout,102, " Internal MAC Receive   %7u      Number of frames dropped on reception\n", mp->mac_no_resource);
catfprintf(stdout,103, " Errors                              because of errors not covered above\n");
printf("\n");
catfprintf(stdout,104, " Spurious Interrupts    %7u      Number of times the adapter interrupted\n", mp->mac_spur_intr);
catfprintf(stdout,105, "                                     the system for an unknown reason\n");
printf("\n");
catfprintf(stdout,106, " No STREAMS Buffers     %7u      Number of frames dropped on reception\n", mp->mac_frame_nosr);
catfprintf(stdout,107, "                                     because no STREAMS buffers were available\n");
printf("\n");
catfprintf(stdout,108, " Underruns/Overruns     %7u      Number of times the transfer of\n", mp->mac_baddma);
catfprintf(stdout,109, "                                     data to or from the frame buffer\n");
catfprintf(stdout,110,"                                     did not complete successfully\n");
printf("\n");
catfprintf(stdout,111, " Device Timeouts        %7u      Number of times the adapter failed to\n", mp->mac_timeouts);
catfprintf(stdout,112,"                                     respond to a request from the driver\n");
		break;
		}
	case MAC_TPR: {
		mac_stats_tr_t *mp = (mac_stats_tr_t *)hwdep_buf;

catfprintf(stdout,113,"\n\t\tTOKEN RING SPECIFIC STATISTICS\n");
printf("\n");
catfprintf(stdout,115," Ring Status            %7x     ", mp->mac_ringstatus);
if (!mp->mac_ringstatus)
	catfprintf(stdout, 116, "Ring is functioning\n");
else
	if (mp->mac_ringstatus > 32768)
		catfprintf(stdout, 318, "Unknown ring status\n");
	else
		catfprintf(stdout, 319, "Ring status explanation:\n");
if (mp->mac_ringstatus & 32)
	catfprintf(stdout, 117, "\t\t\t\t    Ring Recovery in progress\n");
if (mp->mac_ringstatus & 64)
	catfprintf(stdout, 118, "\t\t\t\t    Adapter is the only station on the ring\n");
if (mp->mac_ringstatus & 256)
	catfprintf(stdout, 119, "\t\t\t\t    Remove received\n");
if (mp->mac_ringstatus & 1024)
	catfprintf(stdout, 120, "\t\t\t\t    Auto-Removal error\n");
if (mp->mac_ringstatus & 2048)
	catfprintf(stdout, 121, "\t\t\t\t    Lobe wire fault (cable possibly loose)\n");
if (mp->mac_ringstatus & 4096)
	catfprintf(stdout, 122, "\t\t\t\t    Transmitting beacon frames\n");
if (mp->mac_ringstatus & 8192)
	catfprintf(stdout, 123, "\t\t\t\t    Soft (MAC corrected) error occurred\n");
if (mp->mac_ringstatus & 16384)
	catfprintf(stdout, 124, "\t\t\t\t    Hard (unrecoverable) error occurred\n");
if (mp->mac_ringstatus & 32768)
	catfprintf(stdout, 125, "\t\t\t\t    Signal loss occurred\n");
printf("\n");
catfprintf(stdout, 126, " Upstream MAC         %02x:%02x:%02x:%02x:%02x:%02x    MAC address of nearest\n", mp->mac_upstream[0],mp->mac_upstream[1],mp->mac_upstream[2], mp->mac_upstream[3],mp->mac_upstream[4],mp->mac_upstream[5]);
catfprintf(stdout, 127, "                                           upstream neighbor\n"); 
printf("\n");
catfprintf(stdout, 128, " Functional Address   %02x:%02x:%02x:%02x:%02x:%02x    Functional address set\n", mp->mac_funcaddr[0],mp->mac_funcaddr[1],mp->mac_funcaddr[2], mp->mac_funcaddr[3],mp->mac_funcaddr[4],mp->mac_funcaddr[5]);
catfprintf(stdout, 129, "                                           on the adapter\n"); 

printf("\n");
catfprintf(stdout, 130, " Active Monitor         %7u     If non-zero, indicates that\n", mp->mac_actmonparticipate);
catfprintf(stdout, 131, "                                    the interface will participate\n");
catfprintf(stdout, 320, "                                    in active monitor selection\n");
printf("\n");
catfprintf(stdout, 132, " Line Errors            %7u     Number of times the adapter detected an\n", mp->mac_lineerrors);
catfprintf(stdout, 133, "                                    FCS error in the frame with E-bit zero\n");
printf("\n");
catfprintf(stdout, 134, " Burst Errors           %7u     Number of times the adapter\n", mp->mac_bursterrors);
catfprintf(stdout, 135, "                                    detected the absence of transitions\n");
catfprintf(stdout, 321, "                                    for 5 half-bit times\n");
printf("\n");
catfprintf(stdout, 136, " AC Errors              %7u     Number of times the adapter detected\n", mp->mac_acerrors);
catfprintf(stdout, 137, "                                    a station unable to set the A & C\n");
catfprintf(stdout, 322, "                                    bits correctly\n");
printf("\n");

catfprintf(stdout, 138, " Abort Transmission     %7u     Number of times the adapter aborted a\n", mp->mac_aborttranserrors);
catfprintf(stdout, 139, " Errors                             transmission by sending an abort delimited\n");
printf("\n");
catfprintf(stdout, 140, " Lost Frame Errors      %7u     Number of times the adapter aborted a\n", mp->mac_lostframeerrors);
catfprintf(stdout, 141, "                                    transmission because its TRR timer expired\n");
printf("\n");
catfprintf(stdout, 142, " Receive Congestions    %7u     Number of times the adapter failed to\n", mp->mac_receivecongestions);
catfprintf(stdout, 143, "                                    receive a frame because of no free\n");
catfprintf(stdout, 323, "                                    receive buffers\n");
printf("\n");
catfprintf(stdout, 144, " Frame Copied Errors    %7u     Number of times the adapter recognized\n", mp->mac_framecopiederrors);
catfprintf(stdout, 145, "                                    a frame's destination address, but the\n");
catfprintf(stdout, 324, "                                    frame had already been recognized\n");
catfprintf(stdout, 146, "                                    by another station on the ring\n");
printf("\n");
catfprintf(stdout, 147, " Token Errors           %7u     Number of times the adapter recognized\n", mp->mac_tokenerrors);
catfprintf(stdout, 148, "                                    an error which needed a token to be\n");
catfprintf(stdout, 325, "                                    transmitted\n");
printf("\n");
catfprintf(stdout, 149, " MAC Soft Errors        %7u     Number of times the adapter recognized an\n", mp->mac_softerrors);
catfprintf(stdout, 150, "                                    error which was corrected by the MAC layer\n");
printf("\n");
catfprintf(stdout, 151, " MAC Hard Errors        %7u     Number of times the adapter recognized\n", mp->mac_harderrors);
catfprintf(stdout, 152, "                                    an error which could not be corrected\n");
catfprintf(stdout, 343, "                                    by the MAC\n");
printf("\n");
catfprintf(stdout, 153, " Signal Loss            %7u     Number of times the adapter detected\n", mp->mac_signalloss);
catfprintf(stdout, 154, "                                    the absence of a signal on the ring\n");
printf("\n");
catfprintf(stdout, 155, " Transmit Beacons       %7u     Number of times the adapter has\n", mp->mac_transmitbeacons);
catfprintf(stdout, 156, "                                    sent a beacon frame\n");
printf("\n");
catfprintf(stdout, 157, " Ring Recoveries        %7u     Number of times Claim Token MAC frames\n", mp->mac_recoverys);
catfprintf(stdout, 158, "                                    were received or transmitted after\n");
catfprintf(stdout, 326, "                                    the ring was purged\n");
printf("\n");
catfprintf(stdout, 159, " Lobe Wire Errors       %7u     Number of times the adapter detected\n", mp->mac_lobewires);
catfprintf(stdout, 160, "                                    an open or short circuit on the cable\n");
printf("\n");
catfprintf(stdout, 161, " Removes                %7u     Number of times the adapter received\n", mp->mac_removes);
catfprintf(stdout, 162, "                                    a Remove Station MAC frame\n");
printf("\n");
catfprintf(stdout, 163, " Single Station         %7u     Number of times the adapter detected that\n", mp->mac_statssingles);
catfprintf(stdout, 164, "                                    it was the only station on the ring\n");
printf("\n");
catfprintf(stdout, 165, " Frequency Errors       %7u     Number of times the adapter detected\n", mp->mac_frequencyerrors);
catfprintf(stdout, 166, "                                    the frequency of the incoming signal\n");
catfprintf(stdout, 167, "                                    differed from the IEEE tolerances\n");
printf("\n");
catfprintf(stdout, 168, " Frame Too Long         %7u     Number of frames dropped on reception\n", mp->mac_badlen);
catfprintf(stdout, 169, "                                    because they were larger than the\n");
catfprintf(stdout, 170, "                                    maximum Token Ring frame size\n");
printf("\n");
catfprintf(stdout, 171, " Spurious Interrupts    %7u     Number of times the adapter interrupted\n", mp->mac_spur_intr);
catfprintf(stdout, 172, "                                    the system for an unknown reason\n");
printf("\n");
catfprintf(stdout, 173, " No STREAMS Buffers     %7u     Number of frames dropped on reception\n", mp->mac_frame_nosr);
catfprintf(stdout, 174, "                                    because no STREAMS buffers were available\n");
printf("\n");
catfprintf(stdout, 175, " Underruns/Overruns     %7u     Number of times the transfer of data\n", mp->mac_baddma);
catfprintf(stdout, 176, "                                    to or from the adapter did not\n");
catfprintf(stdout, 330, "                                    complete successfully\n");
printf("\n");
catfprintf(stdout, 177, " Device Timeouts        %7u     Number of times the adapter failed to\n", mp->mac_timeouts);
catfprintf(stdout, 178, "                                    respond to a request from the driver\n");
		break;
		}
	case MAC_FDDI: {
		mac_stats_fddi_t *mp = (mac_stats_fddi_t *)hwdep_buf;

catfprintf(stdout, 181,"\n\t\tFDDI Specific Statistics\n");
catfprintf(stdout, 182,"\t\t========================\n");

catfprintf(stdout, 183," SMT Station ID           %02x:%02x:%02x:%02x:%02x:%02x    Uniquely identifies station\n", mp->smt_station_id[0],mp->smt_station_id[1],mp->smt_station_id[2], mp->smt_station_id[3],mp->smt_station_id[4],mp->smt_station_id[5]);

catfprintf(stdout, 184," SMT Op Version ID      %7u     Version this station is using for operation\n", mp->smt_op_version_id);

catfprintf(stdout, 185," SMT Hi Version ID      %7u     Highest SMT version this station supports\n", mp->smt_hi_version_id);

catfprintf(stdout, 186," SMT Lo Version ID      %7u     Lowest version of SMT this station supports\n", mp->smt_lo_version_id);

catfprintf(stdout, 187," SMT User Data                  User defined information:\n");

catfprintf(stdout, 188,"                        %02x %02x %02x %02x\n", mp->smt_user_data[0], mp->smt_user_data[1], mp->smt_user_data[2], mp->smt_user_data[3]);
catfprintf(stdout, 188,"                        %02x %02x %02x %02x\n", mp->smt_user_data[4], mp->smt_user_data[5], mp->smt_user_data[6], mp->smt_user_data[7]);
catfprintf(stdout, 188,"                        %02x %02x %02x %02x\n", mp->smt_user_data[8], mp->smt_user_data[9], mp->smt_user_data[10], mp->smt_user_data[11]);
catfprintf(stdout, 188,"                        %02x %02x %02x %02x\n", mp->smt_user_data[12], mp->smt_user_data[13], mp->smt_user_data[14], mp->smt_user_data[15]);
catfprintf(stdout, 188,"                        %02x %02x %02x %02x\n", mp->smt_user_data[16], mp->smt_user_data[17], mp->smt_user_data[18], mp->smt_user_data[19]);
catfprintf(stdout, 188,"                        %02x %02x %02x %02x\n", mp->smt_user_data[20], mp->smt_user_data[21], mp->smt_user_data[22], mp->smt_user_data[23]);
catfprintf(stdout, 188,"                        %02x %02x %02x %02x\n", mp->smt_user_data[24], mp->smt_user_data[25], mp->smt_user_data[26], mp->smt_user_data[27]);
catfprintf(stdout, 188,"                        %02x %02x %02x %02x\n", mp->smt_user_data[28], mp->smt_user_data[29], mp->smt_user_data[30], mp->smt_user_data[31]);

catfprintf(stdout, 189," SMT MIB Version ID     %7u     Version of FDDI MIB of this station\n", mp->smt_mib_version_id);

catfprintf(stdout, 190," SMT MAC Count          %7u     Number of MACs in this station\n", mp->smt_mac_cts);

catfprintf(stdout, 191," SMT Non-Master Count   %7u     Number of A, B, and S ports in this station\n", mp->smt_non_master_cts);

catfprintf(stdout, 192," SMT Master Count       %7u     Number of M ports in this node\n", mp->smt_master_cts);

catfprintf(stdout, 193," SMT Available Paths    %7u     Path types available to this station\n", mp->smt_available_paths);

catfprintf(stdout, 194," SMT Conf Capabilities  %7u     Configuration capabilities of this node\n", mp->smt_config_capabilities);

catfprintf(stdout, 195," SMT Conf Policy        %7u     Configuration policy desired in this node\n", mp->smt_config_policy);

catfprintf(stdout, 196," SMT Connection Policy  %7u     Connection policy in effect on this node\n", mp->smt_connection_policy);

catfprintf(stdout, 197," SMT Notification Timer %7u     #Seconds in neighbor notification timer\n", mp->smt_t_notify);

catfprintf(stdout, 198," SMT Status Rpt Policy  %7u     Node generates status reporting frames\n", mp->smt_stat_rpt_policy);

catfprintf(stdout, 199," SMT Trace Max Expire   %7u     Trace maximum expiration\n", mp->smt_trace_max_expiration);

catfprintf(stdout, 200," SMT Bypass Present     %7u     Station has a bypass on its AB port pair\n", mp->smt_bypass_present);

catfprintf(stdout, 201," SMT ECM State          %7u     Entity Coordination Management state\n", mp->smt_ecm_state);

catfprintf(stdout, 202," SMT CF State           %7u     Attachment configuration for this station\n", mp->smt_cf_state);

catfprintf(stdout, 203," SMT Remote Disconnect  %7u     Station disconnected, rcvd SMTAction discon\n", mp->smt_remote_disconnect_flag);

catfprintf(stdout, 204," SMT Station Status     %7u     Stations primary and secondary paths status\n", mp->smt_station_status);

catfprintf(stdout, 205," SMT Peer Wrap Flag     %7u     Value of PeerWrapFlag in CFM\n", mp->smt_peer_wrap_flag);

catfprintf(stdout, 206," SMT Time Stamp         %7u     Value of TimeStamp\n", mp->smt_time_stamp);

catfprintf(stdout, 207," SMT Trans Time Stamp   %7u     Value of TransitionTimeStamp\n", mp->smt_transition_time_stamp);

catfprintf(stdout, 208," MAC Frame Status Func  %7u     MAC's frame status processing functions\n", mp->mac_frame_status_functions);

catfprintf(stdout, 209," MAC Maximum Time       %7u     Maximum MAC Time Maximum this MAC supports\n", mp->mac_t_max_capability);

catfprintf(stdout, 210," MAC Maximum Tvx Time   %7u     Maximum MAC Tvx Value supported by this MAC\n", mp->mac_tvx_capability);

catfprintf(stdout, 211," MAC Available Paths    %7u     Paths available to this MAC\n", mp->mac_available_paths);

catfprintf(stdout, 212," MAC Current Path       %7u     Path this MAC is currently inserted into\n", mp->mac_current_path);

catfprintf(stdout, 213," MAC Upstream Nbr       %02x:%02x:%02x:%02x:%02x:%02x    Upstream neighbor MAC Addr\n", mp->mac_upstream_nbr[0], mp->mac_upstream_nbr[1], mp->mac_upstream_nbr[2], mp->mac_upstream_nbr[3], mp->mac_upstream_nbr[4], mp->mac_upstream_nbr[5]);

catfprintf(stdout, 214," MAC Downstream Nbr     %02x:%02x:%02x:%02x:%02x:%02x    Downstream neighbor MAC Addr\n", mp->mac_downstream_nbr[0], mp->mac_downstream_nbr[1], mp->mac_downstream_nbr[2], mp->mac_downstream_nbr[3], mp->mac_downstream_nbr[4], mp->mac_downstream_nbr[5]);

catfprintf(stdout, 215," MAC Old Upstream Nbr   %02x:%02x:%02x:%02x:%02x:%02x    Old upstream neighbor MAC Addr\n", mp->mac_old_upstream_nbr[0], mp->mac_old_upstream_nbr[1], mp->mac_old_upstream_nbr[2], mp->mac_old_upstream_nbr[3], mp->mac_old_upstream_nbr[4], mp->mac_old_upstream_nbr[5]);

catfprintf(stdout, 216," MAC Old Downstream Nbr %02x:%02x:%02x:%02x:%02x:%02x    Old downstream neighbor MAC Addr\n", mp->mac_old_downstream_nbr[0], mp->mac_old_downstream_nbr[1], mp->mac_old_downstream_nbr[2], mp->mac_old_downstream_nbr[3], mp->mac_old_downstream_nbr[4], mp->mac_old_downstream_nbr[5]);

catfprintf(stdout, 217," MAC Dup Address Test   %7u     Duplicate address test flag\n", mp->mac_dup_address_test);

catfprintf(stdout, 218," MAC Requested Paths    %7u     Paths the MAC may be inserted into\n", mp->mac_requested_paths);

catfprintf(stdout, 219," MAC Dwnstrm Port Type  %7u     PC-Type of first downstream port\n", mp->mac_downstream_port_type);

catfprintf(stdout, 220," MAC SMT Address        %02x:%02x:%02x:%02x:%02x:%02x    MAC address used for SMT frames\n", mp->mac_smt_address[0], mp->mac_smt_address[1], mp->mac_smt_address[2], mp->mac_smt_address[3], mp->mac_smt_address[4], mp->mac_smt_address[5]);

catfprintf(stdout, 221," MAC Time Request       %7u     Time request value passed to the MAC\n", mp->mac_t_req);

catfprintf(stdout, 222," MAC Time Negative      %7u     Time request negative value\n", mp->mac_t_neg);

catfprintf(stdout, 223," MAC Time Maximum       %7u     Maximum time request value passed to MAC\n", mp->mac_t_max);

catfprintf(stdout, 224," MAC Tvx Value          %7u     Transmission Timer Value passed to the MAC\n", mp->mac_tvx_value);

catfprintf(stdout, 225," MAC Frame Count        %7u     #Frames received by this MAC\n", mp->mac_frame_cts);

catfprintf(stdout, 226," MAC Copied Count       %7u     #Frames addressed to and copied by this MAC\n", mp->mac_copied_cts);

catfprintf(stdout, 227," MAC Transmit Count     %7u     #Frames transmitted by this MAC\n", mp->mac_transmit_cts);

catfprintf(stdout, 228," MAC Error Count        %7u     #Frames detected in error by this MAC\n", mp->mac_error_cts);

catfprintf(stdout, 229," MAC Lost Count         %7u     #Frames with format error stripped on recv\n", mp->mac_lost_cts);

catfprintf(stdout, 230," MAC Frame Error Thresh %7u     #Errors before generating MAC Condition rpt\n", mp->mac_frame_error_threshold);

catfprintf(stdout, 231," MAC Frame Error Ratio  %7u     LostCount+ErrorCount / FrameCount+LostCount\n", mp->mac_frame_error_ratio);

catfprintf(stdout, 232," MAC RMT State          %7u     State of Ring Management State Machine\n", mp->mac_rmt_state);

catfprintf(stdout, 233," MAC Dup Address Flag   %7u     Ring Management Duplicate Address Flag\n", mp->mac_da_flag);

catfprintf(stdout, 234," MAC Upstream Dup Addr  %7u     Upstream neighbor Duplicate Address Flag\n", mp->mac_una_da_flag);

catfprintf(stdout, 235," MAC Frame Error Flag   %7u     MAC Frame Error Condition present\n", mp->mac_frame_error_flag);

catfprintf(stdout, 236," MAC MA Unitdata Avail  %7u     Value of MAC_Avail flag defined in RMT\n", mp->mac_ma_unitdata_available);

catfprintf(stdout, 237," MAC Hardware Present   %7u     Underlying hardware support present for MAC\n", mp->mac_hardware_present);

catfprintf(stdout, 238," MAC MA Unitdata Enable %7u     Determines RMT MA_UNITDATA_Enable flag value\n", mp->mac_ma_unitdata_enable);

catfprintf(stdout, 239," PATH Tvx Lower Bound   %7u     Minimum MAC Tvx Value for MACs in this path\n", mp->path_tvx_lower_bound);

catfprintf(stdout, 240," PATH TMax Lower Bound  %7u     Minimum MAC Time Maximum for MACs in path\n", mp->path_t_max_lower_bound);

catfprintf(stdout, 241," PATH Max Time Request  %7u     Maximum MAC Time Request for MAXs in path\n", mp->path_max_t_req);

catfprintf(stdout, 242," PATH Configuration:\n");
catfprintf(stdout, 243,"                        %7u %7u %7u %7u\n", mp->path_configuration[0], mp->path_configuration[1], mp->path_configuration[2], mp->path_configuration[3]);
catfprintf(stdout, 243,"                        %7u %7u %7u %7u\n", mp->path_configuration[4], mp->path_configuration[5], mp->path_configuration[6], mp->path_configuration[7]);

catfprintf(stdout, 244," PORT My Type           %7u %7u  Value of the PORTs PC_Type\n", mp->port_my_type[0], mp->port_my_type[1]);

catfprintf(stdout, 245," PORT Neighbor Type     %7u %7u  Type of remote PORTs\n", mp->port_neighbor_type[0], mp->port_neighbor_type[1]);

catfprintf(stdout, 246," PORTConnectionPolicies %7u %7u  Desired PORT connection policy\n", mp->port_connection_policies[0], mp->port_connection_policies[1]);

catfprintf(stdout, 247," PORT MAC Indicated     %7u %7u  Signalling\n", mp->port_mac_indicated[0], mp->port_mac_indicated[1]);

catfprintf(stdout, 248," PORT Current Path      %7u %7u  Path(s) PORTs inserted into\n", mp->port_current_path[0], mp->port_current_path[1]);

catfprintf(stdout, 249," PORT Requested Paths           List of permitted Paths:\n");
catfprintf(stdout, 250,"                        %7u %7u %7u\n", mp->port_requested_paths[0], mp->port_requested_paths[1], mp->port_requested_paths[2]);
catfprintf(stdout, 250,"                        %7u %7u %7u\n", mp->port_requested_paths[3], mp->port_requested_paths[4], mp->port_requested_paths[5]);

catfprintf(stdout, 251," PORT MAC Placement     %7u %7u  MACs that transmit through PORT\n", mp->port_mac_placement[0], mp->port_mac_placement[1]);

catfprintf(stdout, 252," PORT Available Paths   %7u %7u  Paths available to this PORT\n", mp->port_available_paths[0], mp->port_available_paths[1]);

catfprintf(stdout, 253," PORT PMD Class         %7u %7u  Physical Layer Medium Dependent\n", mp->port_pmd_class[0], mp->port_pmd_class[1]);

catfprintf(stdout, 254," PORT Connect Capable   %7u %7u  Capable of setting PC_MAC_LCT\n", mp->port_connection_capabilities[0], mp->port_connection_capabilities[1]);

catfprintf(stdout, 255," PORT BS Flag           %7u %7u  Value of BS_Flag\n", mp->port_bs_flag[0], mp->port_bs_flag[1]);

catfprintf(stdout, 256," PORT LCT Fail CTS      %7u %7u  Link Confidence Test failed\n", mp->port_lct_fail_cts[0], mp->port_lct_fail_cts[1]);

catfprintf(stdout, 257," PORT Ler Estimate      %7u %7u  Average Link error rate\n", mp->port_ler_estimate[0], mp->port_ler_estimate[1]);

catfprintf(stdout, 258," PORT Lem Reject Count  %7u %7u  Link error monitoring rejects\n", mp->port_lem_reject_cts[0], mp->port_lem_reject_cts[1]);

catfprintf(stdout, 259," PORT Lem Count         %7u %7u  Link error monitoring errors\n", mp->port_lem_cts[0], mp->port_lem_cts[1]);

catfprintf(stdout, 260," PORT Ler Cutoff        %7u %7u  Ler to break Link connection\n", mp->port_ler_cutoff[0], mp->port_ler_cutoff[1]);

catfprintf(stdout, 261," PORT Ler Alarm         %7u %7u  Link error rate generates alarm\n", mp->port_ler_alarm[0], mp->port_ler_alarm[1]);

catfprintf(stdout, 262," PORT Connect State     %7u %7u  Connection state of this PORT\n", mp->port_connect_state[0], mp->port_connect_state[1]);

catfprintf(stdout, 263," PORT PCM State         %7u %7u  PORTs Physical Connection State\n", mp->port_pcm_state[0], mp->port_pcm_state[1]);

catfprintf(stdout, 264," PORT PC Withhold       %7u %7u  Value of PC_Withhold\n", mp->port_pc_withhold[0], mp->port_pc_withhold[1]);

catfprintf(stdout, 265," PORT Ler Flag          %7u %7u  Ler Estimate <= Ler Alarm\n", mp->port_ler_flag[0], mp->port_ler_flag[1]);

catfprintf(stdout, 266," PORT Hardware Present  %7u %7u  PORT has hardware support\n", mp->port_hardware_present[0], mp->port_hardware_present[1]);

		break;
	}
	case MAC_ISDN_BRI:
	case MAC_ISDN_PRI: {
		mac_stats_isdn_t *mp = (mac_stats_isdn_t *)hwdep_buf;
catfprintf(stdout, 362, "\n\t\tISDN SPECIFIC STATISTICS\n");
catfprintf(stdout, 363, "\n\t\t       D Channel\n");
catfprintf(stdout, 364, "\n\t            Speed: %d bits/sec\n", mp->dchannel.speed);
catfprintf(stdout, 365, "\t            MTU: %d bytes\n", mp->dchannel.mtu);
catfprintf(stdout, 435, "\t            LAPD Status: ");
switch (mp->dchannel.LapdOperStatus) {
	case 1 : catfprintf(stdout, 436, "Inactive\n"); break;
	case 2 : catfprintf(stdout, 437, "Layer 1 Active\n"); break;
	case 3 : catfprintf(stdout, 438, "Layer 2 Active\n"); break;
	default: catfprintf(stdout, 439, "Device Not Reporting LAPD Status\n"); break;
}
catfprintf(stdout, 366, "\n\t            D Channel FRAMES\n");
catfprintf(stdout, 367, "       Unicast Broadcast  Error  Discard     Octets   Unknown Protocol\n");
catfprintf(stdout, 368, "    ---------- --------- ------ --------- ----------- ----------------\n");
catfprintf(stdout, 369, "In: %10u %9u %6u %9u %11u %16u\n",
				mp->dchannel.InUcastPkts,
				mp->dchannel.InBroadcastPkts,
				mp->dchannel.InErrors,
				mp->dchannel.InDiscards,
				mp->dchannel.InOctets,
				mp->dchannel.InUnkownProtos);
catfprintf(stdout, 370, "Out:%10u %9u %6u %9u %11u                0\n",
				mp->dchannel.OutUcastPkts,
				mp->dchannel.OutBroadcastPkts,
				mp->dchannel.OutErrors,
				mp->dchannel.OutDiscards,
				mp->dchannel.OutOctets);
catfprintf(stdout, 371, "\n                Signaling Statistics\n");
catfprintf(stdout, 372, "\n     Signaling Protocol: ");
switch (mp->signaling.SignalingProtocol){
	case 0 : catfprintf(stdout, 373, "Protocol Information Not Available\n"); break;
	case 2 : catfprintf(stdout, 374, "ITU DSS1 Q.931\n"); break;
	case 3 : catfprintf(stdout, 375, "Europe / ETSI\n"); break;
	case 4 : catfprintf(stdout, 376, "U.K. / DASS2 (PRI)\n"); break;
	case 5 : catfprintf(stdout, 377, "U.S.A. / AT&T 4ESS\n"); break;
	case 6 : catfprintf(stdout, 378, "U.S.A. / AT&T 5ESS\n"); break;
	case 7 : catfprintf(stdout, 379, "U.S.A / Northern Telecom DMS100\n"); break;
	case 8 : catfprintf(stdout, 380, "U.S.A / Northern Telecom DMS250\n"); break;
	case 9 : catfprintf(stdout, 381, "U.S.A National ISDN 1 (BRI)\n"); break;
	case 10 : catfprintf(stdout, 382, "U.S.A National ISDN 2 (BRI, PRI)\n"); break;
	case 12 : catfprintf(stdout, 383, "France / VN2\n"); break;
	case 13 : catfprintf(stdout, 384, "France / VN3\n"); break;
	case 14 : catfprintf(stdout, 385, "France / VN4 (ETSI with changes)\n"); break;
	case 15 : catfprintf(stdout, 386, "France / VN6 (ETSI with changes)\n"); break;
	case 16 : catfprintf(stdout, 387, "Japan / KDD\n"); break;
	case 17 : catfprintf(stdout, 388, "Japan / NTT INS64\n"); break;
	case 18 : catfprintf(stdout, 389, "Japan / NTT INS1500\n"); break;
	case 19 : catfprintf(stdout, 390, "Germany / 1TR6 (BRI, PRI)\n"); break;
	case 20 : catfprintf(stdout, 391, "Germany / Siemens HiCom CORNET\n"); break;
	case 21 : catfprintf(stdout, 392, "Australia / TS013\n"); break;
	case 22 : catfprintf(stdout, 393, "Australia / TS014\n"); break;
	case 23 : catfprintf(stdout, 394, "Q.SIG\n"); break;
	case 24 : catfprintf(stdout, 395, "SwissNet-2\n"); break;
	case 25 : catfprintf(stdout, 396, "SwissNet-3\n"); break;
	default: catfprintf(stdout, 397, "Unknown Protocol\n");
}
for (i = 0; i < 2; i++) {
	if (mp->signaling.CallingAddress[i][0] != 0)
		catfprintf(stdout, 398, "     Calling Number %d: %s\n", i + 1, &mp->signaling.CallingAddress[i][0]);
	if (mp->signaling.CallingSubAddress[i][0] != 0)
		catfprintf(stdout, 399, "     Calling Subaddress %d: %s\n", i + 1, &mp->signaling.CallingSubAddress[i][0]);
}
catfprintf(stdout, 400, "     Total Call Charge Units: %d\n", mp->signaling.ChargedUnits);
catfprintf(stdout, 401, "     Number of B Channels: %d\n", mp->signaling.BchannelCount);
catfprintf(stdout, 402, "\n                Calls    Calls Connected\n");
catfprintf(stdout, 403,   "               -------   ---------------\n");
catfprintf(stdout, 404,   "     Incoming: %7u   %15u\n",  mp->signaling.InCalls, mp->signaling.InConnected);
catfprintf(stdout, 405,   "     Outgoing: %7u   %15u\n", mp->signaling.OutCalls, mp->signaling.OutConnected);
for (i = 0; i < mp->signaling.BchannelCount; i++) {
	catfprintf(stdout, 406, "\n                B Channel %d\n", mp->bchannel[i].ChannelNumber);
	catfprintf(stdout, 407, "\n     Call Control State: ");
	switch (mp->bchannel[i].OperStatus) {
		case 0 : catfprintf(stdout, 408, "Device not reporting B channel statistics\n"); break;
		case 1 : catfprintf(stdout, 409, "Idle\n"); break;
		case 2 : catfprintf(stdout, 410, "Connecting\n"); break;
		case 3 : catfprintf(stdout, 411, "Connected\n"); break;
		case 4 : catfprintf(stdout, 412, "Active\n"); break;
		default: catfprintf(stdout, 413, "Invalid response from device\n"); break;
	}
	catfprintf(stdout, 414, "     Application Identifier: %d\n", mp->bchannel[i].AppID);
	catfprintf(stdout, 415, "     Called Number: %s\n", mp->bchannel[i].PeerAddress);
	catfprintf(stdout, 416, "     Called Subaddress: %s\n", mp->bchannel[i].PeerSubAddress);
	catfprintf(stdout, 417, "     Call Direction: ");
	switch (mp->bchannel[i].CallOrigin) {
		case 2 : catfprintf(stdout, 418, "Outgoing\n"); break;
		case 3 : catfprintf(stdout, 419, "Incoming\n"); break;
		case 4 : catfprintf(stdout, 420, "Callback\n"); break;
		default: catfprintf(stdout, 421, "Unknown\n"); break;
	}
	catfprintf(stdout, 422, "     Call Type: ");
	switch (mp->bchannel[i].InfoType) {
		case 2 : catfprintf(stdout, 423, "Speech\n"); break;
		case 3 : catfprintf(stdout, 424, "Unrestricted Digital Information\n"); break;
		case 4 : catfprintf(stdout, 425, "Unrestricted Digital Information 56Kbit/s Rate Adoption\n"); break;
		case 5 : catfprintf(stdout, 426, "Restricted Digital Information\n"); break;
		case 6 : catfprintf(stdout, 427, "3.1 kHz Audio\n"); break;
		case 7 : catfprintf(stdout, 428, "7 kHz Audio\n"); break;
		case 8 : catfprintf(stdout, 429, "Video\n"); break;
		case 9 : catfprintf(stdout, 430, "Packet Mode\n"); break;
		default: catfprintf(stdout, 431, "Unknown\n"); break;
	}
	catfprintf(stdout, 432, "     Call Initiate Time: %s", ctime(&mp->bchannel[i].CallSetupTime));
	catfprintf(stdout, 433, "     Call Connect Time: %s", ctime(&mp->bchannel[i].CallConnectTime));
	catfprintf(stdout, 434, "     Call Charge Units: %d\n", mp->bchannel[i].ChargedUnits);	 
} /* end for B channel */
		break; /* ISDN */
	}
	default:
		catfprintf(stderr, 179, "Cannot get media dependent statistics - unknown media type\n");
	}
}

void
convert_to_hwdep(lli31_mac_stats_t *mp)
{
	mac_stats_eth_t *ep = (mac_stats_eth_t *)hwdep_buf;

	hwdep_type = MAC_CSMACD;
	ep=(mac_stats_eth_t *)hwdep_buf;
	memset(ep, 0, sizeof(mac_stats_eth_t));
	ep->mac_align = mp->mac_align;
	ep->mac_badsum = mp->mac_badsum;
	ep->mac_colltable[0] = mp->mac_frame_coll;
	ep->mac_sqetesterrors = 0;
	ep->mac_frame_def = mp->mac_frame_def;
	ep->mac_oframe_coll = mp->mac_oframe_coll;
	ep->mac_xs_coll = mp->mac_xs_coll;
	ep->mac_tx_errors = mp->mac_no_resource;
	ep->mac_carrier = mp->mac_carrier;
	ep->mac_badlen = mp->mac_badlen;
	ep->mac_no_resource = mp->mac_no_resource;
	ep->mac_spur_intr = mp->mac_spur_intr;
	ep->mac_frame_nosr = mp->mac_frame_nosr;
	ep->mac_baddma = mp->mac_baddma;
	ep->mac_timeouts = mp->mac_timeouts;
}
