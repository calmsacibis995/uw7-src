$
$       @(#) lli.msg 13.1 96/03/28 SCOINC
$
$	Copyright (C) The Santa Cruz Operation, 1993-1996
$	This Module contains Proprietary Information of
$	The Santa Cruz Operation, and should be treated as Confidential.
$set 1 'dlpid' messages
1 dlpid: ServiceFds: Poll returned -1\n
2 dlpid: ServiceFds: Poll returned an error (fd=%d, revent=0x%x)\n
3 dlpid: Unable to fork()\n
4 dlpid: pipeHandler: Read on pipe failed\n
5 dlpid: Unknown command '%s'\n
6 dlpid: openPipes: Unable to open pipe(%s)\n
7 dlpid: openPipes: fstat failed on open pipe(%s)\n
8 dlpid: openPipes: Unable to mknod pipe(%s)\n
9 dlpid: openPipes: Unable to open pipe#2(%s)\n
10 dlpid: openPipes: fstat failed on open pipe#2(%s)\n
11 dlpid: openPipes: ioctl(I_FDINSERT) failed on open pipe(%s)\n
12 dlpid: MAC_HWFAIL_IND is corrupt\n
13 dlpid: ReadMCTable: putmsg(DL_MCTABLE_REQ) failed on DLPI interface (%s)\n
14 dlpid: ReadMCTable: getmsg(DL_MCTABLE_ACK) failed on DLPI interface (%s)\n
15 dlpid: ReadMCTable: getmsg(DL_MCTABLE_ACK) flags incorrect (%x)\n
16 dlpid: ReadMCTable: Got DL_ERROR_ACK\n
17 dlpid: ReadMCTable: Got unknown primitive (0x%x)\n
18 dlpid: MAC_SETMCA ioctl failed on (%s)\n
19 dlpid: dlpiHandler: Unable to find fd=%d in DLPI List\n
20 dlpid: dlpiHandler: Unable to Read MAC_HW_FAIL_IND from DLPI interface (%s)\n
21 dlpid: DLPI Interface (%s) already started\n
22 dlpid: Unable to open DLPI module (%s)\n
23 dlpid: Unable to open network adapter driver (%s)\n
24 dlpid: WriteMCTable failed\n
25 dlpid: Unable to link DLPI module (%s) onto network adapter driver (%s)\n
26 dlpid: Unable to register with DLPI module\n
27 dlpid: DLPI module busy, possible already in use\n
28 dlpid: RemoveIf: Removing active MDI driver (%s) from DLPI module (%s)\n
29 dlpid: DLPI Interface (%s) not started\n
30 dlpid: StopIf: ReadMCTable failed for DLPI Interface (%s)\n
31 dlpid: Error De-registering dlpid from DLPI Interface (%s)\n
32 dlpid: Error unlinking DLPI module (%s) from network adapter driver (%s)\n
33 dlpid: AddInterface cannot allocate memory\n
34 dlpid: MDI driver (%s) already exists under DLPI (%s)\n
35 dlpid: AddInterface cannot allocate memory\n
36 dlpid: RemoveInterface: Unable to find DLPI Interface (%s) in internal table\n
37 dlpid: RemoveInterface: Unable to find MDI Driver (%s) under DLPI Interface (%s)\n
38 dlpid: RemoveInterface: MDI Driver (%s) under DLPI Interface (%s) currently active\n
39 dlpid: RemoveInterface: Stop the interface and then remove the MDI driver (%s)\n
40 dlpid: StartInterface: Unable to find DLPI Interface (%s) in internal table\n
41 dlpid: DLPI Interface (%s) is already started\n
42 dlpid: StopInterface: Unable to find DLPI Interface (%s) in internal table\n
43 dlpid: DLPI Interface (%s) has not been started\n
44 dlpid: RestartInterface: Unable to find DLPI Interface (%s) in internal table\n
45 dlpid: AddBackupInterface: Unable to find DLPI Interface (%s) from MDI(%s) ininternal table\n
46 dlpid: FailoverInterface: Unable to find DLPI Interface (%s) in internal table\n
47 dlpid: AddInterface: cannot obtain mutex\n
48 dlpid: RemoveIf: cannot obtain mutex\n
49 dlpid: OpenPipes: null pipe file name\n
$set 2 Utility 'ndstat' messages
1 Usage: %s [-sSRlcC] [dev...]\n
2 \ts - short\n
3 \tl - long\n
4 \tS - SAP statistics\n
5 \tR - Source Routing statistics\n
6 \tc - clear statistics\n
7 Device       MAC address in use    Factory MAC Address\n
8 %-12s
9 \n\t                     FRAMES\n
10       Unicast  Multicast Broadcast  Error    Octets   Queue Length\n
11 In:\ 
12 %10u\ 
13 %9u\ 
14 %9u\ 
15 %6u\ 
16 %11u\ 
17 %12u\n
18 Out:
19 DLPI Module Info: %d SAPs open, %d SAPs maximum
20 %d Source routes in use, %d Source routes maximum
21                  %d frames received destined for an unbound SAP\n
22 MAC Driver Info: Media_type: 
23 Ethernet
24 Token Ring
25 FDDI
26 Unknown_type(0x%x)
27                  Min_SDU: %d, Max_SDU: %d, Address length: %d\n
28                  Interface speed: 
29 %d Mbits/sec\n
30 %d bits/sec\n
31 Interface Version: MDI %x\n
32 ndstat: Unable to open %s\n
34 ndstat: putmsg(%s) failed\n
35 ndstat: getmsg(%s) failed\n
36 ndstat: getmsg(%s) OK, but flags is wrong (=0x%x)\n
37 ndstat: Incorrect stats_len returned, was %d should be %d\n
38 ndstat: getstats(%s) - Incorrect primitive returned (0x%x)\n
39 ndstat: %s ioctl failed\n
40 SAP Statistics Unavailable\n
41 No SAPs open\n
42              FRAMES FOR EACH SERVICE ACCESS POINT (SAP)\n
43         Unicast XID TEST Multicast Broadcast Error   Octets   Queue Length\n
44 LLC (below DLPI)
45 Unknown(0x%04x)
46    In:\ 
47 %8u\ 
48 %3u\ 
49 %4u\ 
50 %9u\ 
51 %9u\ 
52 %5u\ 
53 %10u\ 
54 %12u\n
55    Out:
56 Source routing table information unavailable\n
57 \n\t\tSOURCE ROUTING TABLE\n
58 \t====================\n
59 %d of %d Source Routes in use\n
60 \tShort timer = %d ticks\n
61 \tMedium timer = %d ticks\n
62 \tLong timer = %d ticks\n
63 \tGarbage collector runs every %d ticks\n\n
64 No source routing table entries present\n
65 state=\"%s\"
66 %15d\ 
67 %12d\n
68 \tRing(s):   %03d
69 ->%03d
70 \n\tvia Bridge(s): %-2d
71    %-2d
72 \t[LOCAL RING]
73 \n\t\tETHERNET SPECIFIC STATISTICS\n
74 \t\t============================\n
75 \n Collision Table - The number of frames successfully transmitted,\n
76 \t           but involved in at least one collision:\n
77 \n\t               Frames                   Frames\n
78 \t%2d collision  %7u    %2d collisions %7u\n
79 \t%2d collisions %7u    %2d collisions %7u\n
80  Bad Alignment          %7u      Number of frames received that were\n
81                                      not an integral number of octets\n
82  FCS Errors             %7u      Number of frames received that did\n
83                                      not pass the Frame Check Sequence\n
84  SQE Test Errors        %7u      Number of Signal Quality Error Test\n
85                                      signals that were detected by the adapter\n
86  Deferred Transmissions %7u      Number of frames delayed on the\n
87                                      first transmission attempt because\n
88  Late Collisions        %7u      Number of times a collision was\n
89                                      detected later than 512 bits into\n
90  Excessive Collisions   %7u      Number of frames dropped on transmission\n
91                                      because of excessive collisions\n
92  Internal MAC Transmit  %7u      Number of frames dropped on transmission\n
93  Errors                              because of errors not covered above\n
94  Carrier Sense Errors   %7u      Number of times that the carrier sense\n
95                                      condition was lost when attempting to\n
96  Excessive Deferrals    %7u      #Frames dropped on transmission because\n
97                                      send a frame that was deferred for an\n
98                                      excessive amount of time\n
99  Frame Too Long         %7u      Number of frames dropped on reception\n
100                                      because they were larger than the\n
101                                      maximum Ethernet frame size\n
102  Internal MAC Receive   %7u      Number of frames dropped on reception\n
103  Errors                              because of errors not covered above\n
104  Spurious Interrupts    %7u      Number of times the adapter interrupted\n
105                                      the system for an unknown reason\n
106  No STREAMS Buffers     %7u      Number of frames dropped on reception\n
107                                      because no STREAMS buffers were available\n
108  Underruns/Overruns     %7u      Number of times the transfer of\n
109                                      data to or from the frame buffer\n
110                                      did not complete successfully\n
111  Device Timeouts        %7u      Number of times the adapter failed to\n
112                                      respond to a request from the driver\n
113 \n\t\tTOKEN RING SPECIFIC STATISTICS\n
114 \t\t==============================\n
115  Ring Status            %7x     
116 Ring is functioning\n
117 \t\t\t\t    Ring Recovery in progress\n
118 \t\t\t\t    Adapter is the only station on the ring\n
119 \t\t\t\t    Remove received\n
120 \t\t\t\t    Auto-Removal error\n
121 \t\t\t\t    Lobe wire fault (cable possibly loose)\n
122 \t\t\t\t    Transmitting beacon frames\n
123 \t\t\t\t    Soft (MAC corrected) error occurred\n
124 \t\t\t\t    Hard (unrecoverable) error occurred\n
125 \t\t\t\t    Signal loss occurred\n
126  Upstream MAC         %02x:%02x:%02x:%02x:%02x:%02x    MAC address of nearest\n
127                                            upstream neighbor\n
128  Functional Address   %02x:%02x:%02x:%02x:%02x:%02x    Functional address set\n
129                                            on the adapter\n
130  Active Monitor         %7u     If non-zero, indicates that\n
131                                     the interface will participate\n
132  Line Errors            %7u     Number of times the adapter detected an\n
133                                     FCS error in the frame with E-bit zero\n
134  Burst Errors           %7u     Number of times the adapter\n
135                                     detected the absence of transitions\n
136  AC Errors              %7u     Number of times the adapter detected\n
137                                     a station unable to set the A & C\n
138  Abort Transmission     %7u     Number of times the adapter aborted a\n
139  Errors                             transmission by sending an abort delimited\n
140  Lost Frame Errors      %7u     Number of times the adapter aborted a\n
141                                     transmission because its TRR timer expired\n
142  Receive Congestions    %7u     Number of times the adapter failed to\n
143                                     receive a frame because of no free\n
144  Frame Copied Errors    %7u     Number of times the adapter recognized\n
145                                     a frame's destination address, but the\n
146                                     by another station on the ring\n
147  Token Errors           %7u     Number of times the adapter recognized\n
148                                     an error which needed a token to be\n
149  MAC Soft Errors        %7u     Number of times the adapter recognized an\n
150                                     error which was corrected by the MAC layer\n
151  MAC Hard Errors        %7u     Number of times the adapter recognized\n
152                                     an error which could not be corrected\n
153  Signal Loss            %7u     Number of times the adapter detected\n
154                                     the absence of a signal on the ring\n
155  Transmit Beacons       %7u     Number of times the adapter has\n
156                                     sent a beacon frame\n
157  Ring Recoverys         %7u     Number of times Claim Token MAC frames\n
158                                     were received or transmitted after\n
159  Lobe Wire Errors       %7u     Number ofimes the adapter detected\n
160                                     an open or short circuit on the cable\n
161  Removes                %7u     Number of times the adapter received\n
162                                     a Remove Station MAC frame\n
163  Single Station         %7u     Number of times the adapter detected that\n
164                                     it was the only station on the ring\n
165  Frequency Errors       %7u     Number of times the adapter detected\n
166                                     the frequency of the incoming signal\n
167                                     differed from the IEEE tolerances\n
168  Frame Too Long         %7u     Number of frames dropped on reception\n
169                                     because they were larger than the\n
170                                     maximum Token Ring frame size\n
171  Spurious Interrupts    %7u     Number of times the adapter interrupted\n
172                                     the system for an unknown reason\n
173  No STREAMS Buffers     %7u     Number of frames dropped on reception\n
174                                     because no STREAMS buffers were available\n
175  Underruns/Overruns     %7u     Number of times the transfer of data\n
176                                     to or from the adapter did not\n
177  Device Timeouts        %7u     Number of times the adapter failed to\n
178                                     respond to a request from the driver\n
179 Cannot get media dependent statistics - unknown media type\n
180 \t\tMulticast address table
181 \n\t\tFDDI Specific Statistics\n
182 \t\t========================\n
183  SMT Station ID           %02x:%02x:%02x:%02x:%02x:%02x    Uniquely identifies station\n
184  SMT Op Version ID      %7u     Version this station is using for operation\n
185  SMT Hi Version ID      %7u     Highest SMT version this station supports\n
186  SMT Lo Version ID      %7u     Lowest version of SMT this station supports\n
187  SMT User Data                  User defined information:\n
188                         %02x %02x %02x %02x\n
189  SMT MIB Version ID     %7u     Version of FDDI MIB of this station\n
190  SMT MAC Count          %7u     Number of MACs in this station\n
191  SMT Non-Master Count   %7u     Number of A, B, and S ports in this station\n
192  SMT Master Count       %7u     Number of M ports in this node\n
193  SMT Available Paths    %7u     Path types available to this station\n
194  SMT Conf Capabilities  %7u     Configuration capabilities of this node\n
195  SMT Conf Policy        %7u     Configuration policy desired in this node\n
196  SMT Connection Policy  %7u     Connection policy in effect on this node\n
197  SMT Notification Timer %7u     #Seconds in neighbor notification timer\n
198  SMT Status Rpt Policy  %7u     Node generates status reporting frames\n
199  SMT Trace Max Expire   %7u     Trace maximum expiration\n
200  SMT Bypass Present     %7u     Station has a bypass on its AB port pair\n
201  SMT ECM State          %7u     Entity Coordination Management state\n
202  SMT CF State           %7u     Attachment configuration for this station\n
203  SMT Remote Disconnect  %7u     Station disconnected, rcvd SMTAction discon\n
204  SMT Station Status     %7u     Stations primary and secondary paths status\n
205  SMT Peer Wrap Flag     %7u     Value of PeerWrapFlag in CFM\n
206  SMT Time Stamp         %7u     Value of TimeStamp\n
207  SMT Trans Time Stamp   %7u     Value of TransitionTimeStamp\n
208  MAC Frame Status Func  %7u     MAC's frame status processing functions\n
209  MAC Maximum Time       %7u     Maximum MAC Time Maximum this MAC supports\n
210  MAC Maximum Tvx Time   %7u     Maximum MAC Tvx Value supported by this MAC\n
211  MAC Available Paths    %7u     Paths available to this MAC\n
212  MAC Current Path       %7u     Path this MAC is currently inserted into\n
213  MAC Upstream Nbr       %02x:%02x:%02x:%02x:%02x:%02x    Upstream neighbor MAC Addr\n
214  MAC Downstream Nbr     %02x:%02x:%02x:%02x:%02x:%02x    Downstream neighbor MAC Addr\n
215  MAC Old Upstream Nbr   %02x:%02x:%02x:%02x:%02x:%02x    Old upstream neighbor MAC Addr\n
216  MAC Old Downstream Nbr %02x:%02x:%02x:%02x:%02x:%02x    Old downstream neighbor MAC Addr\n
217  MAC Dup Address Test   %7u     Duplicate address test flag\n
218  MAC Requested Paths    %7u     Paths the MAC may be inserted into\n
219  MAC Dwnstrm Port Type  %7u     PC-Type of first downstream port\n
220  MAC SMT Address        %02x:%02x:%02x:%02x:%02x:%02x    MAC address used for SMT frames\n
221  MAC Time Request       %7u     Time request value passed to the MAC\n
222  MAC Time Negative      %7u     Time request negative value\n
223  MAC Time Maximum       %7u     Maximum time request value passed to MAC\n
224  MAC Tvx Value          %7u     Transmission Timer Value passed to the MAC\n
225  MAC Frame Count        %7u     #Frames received by this MAC\n
226  MAC Copied Count       %7u     #Frames addressed to and copied by this MAC\n
227  MAC Transmit Count     %7u     #Frames transmitted by this MAC\n
228  MAC Error Count        %7u     #Frames detected in error by this MAC\n
229  MAC Lost Count         %7u     #Frames with format error stripped on recv\n
230  MAC Frame Error Thresh %7u     #Errors before generating MAC Condition rpt\n
231  MAC Frame Error Ratio  %7u     LostCount+ErrorCount / FrameCount+LostCount\n
232  MAC RMT State          %7u     State of Ring Management State Machine\n
233  MAC Dup Address Flag   %7u     Ring Management Duplicate Address Flag\n
234  MAC Upstream Dup Addr  %7u     Upstream neighbor Duplicate Address Flag\n
235  MAC Frame Error Flag   %7u     MAC Frame Error Condition present\n
236  MAC MA Unitdata Avail  %7u     Value of MAC_Avail flag defined in RMT\n
237  MAC Hardware Present   %7u     Underlying hardware support present for MAC\n
238  MAC MA Unitdata Enable %7u     Determines RMT MA_UNITDATA_Enable flag value\n
239  PATH Tvx Lower Bound   %7u     Minimum MAC Tvx Value for MACs in this path\n
240  PATH TMax Lower Bound  %7u     Minimum MAC Time Maximum for MACs in path\n
241  PATH Max Time Request  %7u     Maximum MAC Time Request for MAXs in path\n
242  PATH Configuration:\n
243                         %7u %7u %7u %7u\n
244  PORT My Type           %7u %7u  Value of the PORTs PC_Type\n
245  PORT Neighbor Type     %7u %7u  Type of remote PORTs\n
246  PORTConnectionPolicies %7u %7u  Desired PORT connection policy\n
247  PORT MAC Indicated     %7u %7u  Signalling\n
248  PORT Current Path      %7u %7u  Path(s) PORTs inserted into\n
249  PORT Requested Paths           List of permitted Paths:\n
250                         %7u %7u %7u\n
251  PORT MAC Placement     %7u %7u  MACs that transmit through PORT\n
252  PORT Available Paths   %7u %7u  Paths available to this PORT\n
253  PORT PMD Class         %7u %7u  Physical Layer Medium Dependent\n
254  PORT Connect Capable   %7u %7u  Capable of setting PC_MAC_LCT\n
255  PORT BS Flag           %7u %7u  Value of BS_Flag\n
256  PORT LCT Fail CTS      %7u %7u  Link Confidence Test failed\n
257  PORT Ler Estimate      %7u %7u  Average Link error rate\n
258  PORT Lem Reject Count  %7u %7u  Link error monitoring rejects\n
259  PORT Lem Count         %7u %7u  Link error monitoring errors\n
260  PORT Ler Cutoff        %7u %7u  Ler to break Link connection\n
261  PORT Ler Alarm         %7u %7u  Link error rate generates alarm\n
262  PORT Connect State     %7u %7u  Connection state of this PORT\n
263  PORT PCM State         %7u %7u  PORTs Physical Connection State\n
264  PORT PC Withhold       %7u %7u  Value of PC_Withhold\n
265  PORT Ler Flag          %7u %7u  Ler Estimate <= Ler Alarm\n
266  PORT Hardware Present  %7u %7u  PORT has hardware support\n
300  \tC - clear route table\n
308 Sending ARE frames is enabled (route optimization is on).\n
309 Sending ARE frames is disabled (route optimization is off).\n
310 %11d\ 
311 Request to clear route table failed, errno %d\n
312 ------       ------------------    -------------------\n
313     ---------- --------- --------- ------ ----------- ------------\n
314 \n\t\t-----------------------
315 \t              -------                  -------\n
316                                      the media was busy\n
317                                      the transmitted frame\n
318 Unknown ring status\n
319 Ring status explanation:\n
320                                     in active monitor selection\n
321                                     for 5 half-bit times\n
322                                     bits correctly\n
323                                     receiver buffers\n
324                                     frame had already been recognized\n
325                                     transmitted\n
326                                     the ring was purged\n
327 MAC Address       Maximum PDU Timeout (ticks) Route Length\n
328 ----------------- ----------- --------------- ------------\n
329        -------- --- ---- --------- --------- ----- ---------- ------------\n
330                                     complete successfully\n
331 Must have driver privilege for -c option\n
332 Must have driver privilege for -C option\n
333  %-17s     %-17s\n
334 DLPI Restarts Info: Last queue size: %d\n
335                     Last send time: %d\n
336                     Restart in progress: %d\n
337                     Number of restarts: %d\n
338 Cannot get statistics for device %s\n
339 Cannot get SAP statistics from device %s\n
340 Cannot get source routing table statistics from device %s\n
341 Cannot access device %s - statistics not cleared\n
342 Request to clear route table failed for device %s\n
344 Device name does not begin with \"/dev/net\"\n
345 Open of /dev/kmem failed\n
346 nlist of /unix failed\n
347 n_value = 0\n 
348 Cannot get DLPI Restarts statistics for device %s\n
$ Multiple line message 151, 152 and 343
343                                     by the MAC\n
349 \t\tReceive all multicast addresses: Enabled\n
350 \tM - only show MDI drivers\n
351 \tD - only show DLPI or ODI drivers\n
352 \tL - only show LLI drivers\n
353 only one of -M, -D, and -L allowed
354 ndstat: unknown driver type\n
355 this operation isn't applicable to DLPI or ODI drivers\n
356 ISDN Device\n
357 -----------\n
358 %-17s\n
359 ISDN BRI
360 ISDN PRI
361 Application Identifier=%d
362 \n\t\tISDN SPECIFIC STATISTICS\n
363 \n\t\t       D Channel\n
364 \n\t            Speed: %d bits/sec\n
365 \t            MTU: %d bytes\n
366 \n\t            D Channel FRAMES\n
367        Unicast Broadcast  Error  Discard     Octets   Unknown Protocol\n
368     ---------- --------- ------ --------- ----------- ----------------\n
369 In: %10u %9u %6u %9u %11u %16u\n
370 Out:%10u %9u %6u %9u %11u                0\n
371 \n                Signaling Statistics\n
372 \n     Signaling Protocol: 
373 Protocol Information Not Available\n
374 ITU DSS1 Q.931\n
375 Europe / ETSI\n
376 U.K. / DASS2 (PRI)\n
377 U.S.A. / AT&T 4ESS\n
378 U.S.A. / AT&T 5ESS\n
379 U.S.A / Northern Telecom DMS100\n
380 U.S.A / Northern Telecom DMS250\n
381 U.S.A National ISDN 1 (BRI)\n
382 U.S.A National ISDN 2 (BRI, PRI)\n
383 France / VN2\n
384 France / VN3\n
385 France / VN4 (ETSI with changes)\n
386 France / VN6 (ETSI with changes)\n
387 Japan / KDD\n
388 Japan / NTT INS64\n
389 Japan / NTT INS1500\n
390 Germany / 1TR6 (BRI, PRI)\n
391 Germany / Siemens HiCom CORNET\n
392 Australia / TS013\n
393 Australia / TS014\n
394 Q.SIG\n
395 SwissNet-2\n
396 SwissNet-3\n
397 Unknown Protocol\n
398      Calling Number %d: %s\n
399      Calling Subaddress %d: %s\n
400      Total Call Charge Units: %d\n
401      Number of B Channels: %d\n
402 \n                Calls    Calls Connected\n
403                -------   ---------------\n
404      Incoming: %7u   %15u\n
405      Outgoing: %7u   %15u\n
406 \n                B Channel %d\n
407 \n     Call Control State: 
408 Device not reporting B channel statistics\n
409 Idle\n
410 Connecting\n
411 Connected\n
412 Active\n
413 Invalid response from device\n
414      Application Identifier: %d\n
415      Called Number: %s\n
416      Called Subaddress: %s\n
417      Call Direction: 
418 Outgoing\n
419 Incoming\n
420 Callback\n
421 Unknown\n
422      Call Type: 
423 Speech\n
424 Unrestricted Digital Information\n
425 Unrestricted Digital Information 56Kbit/s Rate Adoption\n
426 Restricted Digital Information\n
427 3.1 kHz Audio\n
428 7 kHz Audio\n
429 Video\n
430 Packet Mode\n
431 Unknown\n
432      Call Initiate Time: %s
433      Call Connect Time: %s
434      Call Charge Units: %d\n
435 \t            LAPD Status: 
436 Inactive\n
437 Layer 1 Active\n
438 Layer 2 Active\n
439 Device Not Reporting LAPD Status\n
440 driver is currently suspended(total of %u data frames dropped on output)\n
441 DLPI Restarts Info: Last mblk_t: 0x%x\n
442                     Largest qsize found: %d\n
$set 3 Utility 'stzget' messages
1 usage: %s stanza-file section-name attribute-name\n
$set 4 Utility 'pop' messages
1 pop: waitpid(%d) failed\n
2 pop: Unable to find any free pseudo-ttys (/dev/ptyp??)\n
3 pop: Unable to open slave ptty (%s)\n
4 pop: Unable to get tty settings\n
5 pop: Unable to set tty settings\n
6 Usage:\n\tpop (-charm|-X) -T title -L logfile file args...\n
7 pop: Select failed\n
8 read(cm) %d\n
9 read(rs) %d\n
10 pop: write(cm) failed\n
11 pop: fork() failed\n
12 pop: execl failed\n
13 pop: ScoTerm failed to start\n
14 pop: Unable to open /dev/tty\n
15 pop: Open Log file (%s) failed\n
16 pop: execv(%s) failed\n
17 pop: Select failed on %s\n
$set 5 Utility 'pcislot' messages
1 Usage: %s [-h] [-d]\n
2 Bus Device Function ID: vend_id(4) dev_id(4) rev(2)\n
3 pcislot: Unable to open /dev/pci\n
4 pcislot: Ioctl PCI_BUS_PRESENT failed\n
5 pcislot: PCI bus not present\n
6 PCI access mechanism %d\n
7 PCI buses: 0x%x\n
8 PCI maxindex: %d\n
9 pcislot: Ioctl PCI_SEARCH_BUS failed\n
10 pcislot: Ioctl PCI_READ_CONFIG (rev) failed\n
11 pcislot: Ioctl PCI_READ_CONFIG (I/O) failed\n
12 pcislot: Ioctl PCI_READ_CONFIG (vendor/device) failed\n
$ Messages modified in AHS 5.2
13 Bus Device Function ID(v-4,d-4,r-2) BaseIO\n
$ Here are the ndcfg messages.  They represent the command set API
$ between netcfg and ndcfg.  can be more descriptive if desired
$ This is only displayed for tcl mode for netcfg (since we prime
$ the error buffer with the text message below)
$ a newline should not be put at the end of each message
$ This is a whole lot faster than doing i18n on each of the 675+ calls 
$ to error() directly.  
$set 6 Utility 'ndcfg' messages
1 Initialization of ndcfg failed
2 Unknown command received
3 the debug command failed
4 the loadfile command which loads a bcfg file has failed
5 the loaddir cmd which loads many bcfg files has failed
6 the tcl command has failed
7 the driver command has failed
8 the count command has failed
9 the showindex command has failed
10 the showbus command has failed
11 the showdriver command has failed
12 the showfailover command has failed
13 the showrejects command has failed
14 the showname command has failed
15 the resdump command has failed
16 the quiet command has failed
17 the verbose command has failed
18 the more command has failed
19 the showvariable command has failed
20 the showalltopologies command has failed
21 the showtopo command has failed
22 the resmgr command has failed
23 the resshowunclaimed command has failed
24 the isaautodetect command has failed
25 the getisaparams command has failed
26 the showcustomnum command has failed
27 the showcustom command has failed
28 the idinstall command has failed
29 the ! command has failed
30 the resget command has failed
31 the resput command has failed
32 the showserialttys command has failed
33 the bcfgpathtoindex command has failed
34 the resshowkey command has failed
35 the showisacurrent command has failed
36 the showcustomcurrent command has failed
37 the idremove command has failed
38 the idmodify command has failed
39 the showhelpfile command has failed
40 the pcishort command has failed
41 the pcilong command has failed
42 the eisashort command has failed
43 the eisalong command has failed
44 the mcashort command has failed
45 the mcalong command has failed
46 the getstamp command has failed
47 the stamp command has failed
48 the version command has failed
49 the auths command has failed
50 the location command has failed
51 the elementtoindex command has failed
52 the network adapter(s) named below were physically removed from\n\rthe system.  Re-insert the adapter, run /usr/sbin/netcfg to remove\n\rits information, then remove the adapter.\n\r
53 the unloadall command has failed
54 the bcfghasverify command has failed
55 the clear command has failed
56 the showlines command has failed
57 the sysdat command has failed
58 the nlist command has failed
59 the xid command has failed
60 the test command has failed
61 the getallisaparams command has failed
62 the iicard command has failed
63 the orphans command has failed
64 the pcivendor command has failed
65 the promiscuous command has failed
66 the determineprom command has failed
67 the numwords command has failed
68 the numlines command has failed
69 the hpsldump command has failed
70 the hpslsuspend command has failed
71 the hpslresume command has failed
72 the hpslgetstate command has failed
73 the hpslcanhotplug command has failed
74 the gethwkey command has failed
76 the i2oshort command has failed
77 the i2olong command has failed
78 the dlpimdi command has failed
79 the writestatus command has failed
80 Starting installation of driver
81 Preparing driver for installation
82 Adding driver '%s' to link kit
83 Executing PRE_SCRIPT commands
84 Installing dependent drivers
85 Installing ODI subsystem
86 Installing MDI subsystem
87 Adding parameters to resource manager
88 Creating netcfg files
89 Adding MODNAME
90 Building drivers
91 Loading drivers
92 Patching memory 
93 Executing CONFIG_CMDS commands
94 Testing device
95 Programming NVRAM/EEPROM settings on card
96 Device successfully added to system
97 An error occurred while installing the device
