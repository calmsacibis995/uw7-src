$ SCCSID(@(#)sjis.cs	7.1	LCC)	/* Modified: 15:35:11 10/15/90 */
$	Table sjis

$hexadecimal
$default_char 2a

$input
00-7f	direct char_bias 2000
a0-df	direct char_bias 2e80

$  Shift-JIS input tables
88-88	direct double_byte 9f-fc char_bias a782
89-89	direct double_byte 40-7e char_bias a7e1
89-89	direct double_byte 80-9e char_bias a7e0
89-89	direct double_byte 9f-fc char_bias a882
8a-8a	direct double_byte 40-7e char_bias a8e1
8a-8a	direct double_byte 80-9e char_bias a8e0
8a-8a	direct double_byte 9f-fc char_bias a982
8b-8b	direct double_byte 40-7e char_bias a9e1
8b-8b	direct double_byte 80-9e char_bias a9e0
8b-8b	direct double_byte 9f-fc char_bias aa82
8c-8c	direct double_byte 40-7e char_bias aae1
8c-8c	direct double_byte 80-9e char_bias aae0
8c-8c	direct double_byte 9f-fc char_bias ab82
8d-8d	direct double_byte 40-7e char_bias abe1
8d-8d	direct double_byte 80-9e char_bias abe0
8d-8d	direct double_byte 9f-fc char_bias ac82
8e-8e	direct double_byte 40-7e char_bias ace1
8e-8e	direct double_byte 80-9e char_bias ace0
8e-8e	direct double_byte 9f-fc char_bias ad82
8f-8f	direct double_byte 40-7e char_bias ade1
8f-8f	direct double_byte 80-9e char_bias ade0
8f-8f	direct double_byte 9f-fc char_bias ae82
90-90	direct double_byte 40-7e char_bias aee1
90-90	direct double_byte 80-9e char_bias aee0
90-90	direct double_byte 9f-fc char_bias af82
91-91	direct double_byte 40-7e char_bias afe1
91-91	direct double_byte 80-9e char_bias afe0
91-91	direct double_byte 9f-fc char_bias b082
92-92	direct double_byte 40-7e char_bias b0e1
92-92	direct double_byte 80-9e char_bias b0e0
92-92	direct double_byte 9f-fc char_bias b182
93-93	direct double_byte 40-7e char_bias b1e1
93-93	direct double_byte 80-9e char_bias b1e0
93-93	direct double_byte 9f-fc char_bias b282
94-94	direct double_byte 40-7e char_bias b2e1
94-94	direct double_byte 80-9e char_bias b2e0
94-94	direct double_byte 9f-fc char_bias b382
95-95	direct double_byte 40-7e char_bias b3e1
95-95	direct double_byte 80-9e char_bias b3e0
95-95	direct double_byte 9f-fc char_bias b482
96-96	direct double_byte 40-7e char_bias b4e1
96-96	direct double_byte 80-9e char_bias b4e0
96-96	direct double_byte 9f-fc char_bias b582
97-97	direct double_byte 40-7e char_bias b5e1
97-97	direct double_byte 80-9e char_bias b5e0
97-97	direct double_byte 9f-fc char_bias b682
98-98	direct double_byte 40-7e char_bias b6e1
98-98	direct double_byte 80-9e char_bias b6e0
98-98	direct double_byte 9f-fc char_bias b782
99-99	direct double_byte 40-7e char_bias b7e1
99-99	direct double_byte 80-9e char_bias b7e0
99-99	direct double_byte 9f-fc char_bias b882
9a-9a	direct double_byte 40-7e char_bias b8e1
9a-9a	direct double_byte 80-9e char_bias b8e0
9a-9a	direct double_byte 9f-fc char_bias b982
9b-9b	direct double_byte 40-7e char_bias b9e1
9b-9b	direct double_byte 80-9e char_bias b9e0
9b-9b	direct double_byte 9f-fc char_bias ba82
9c-9c	direct double_byte 40-7e char_bias bae1
9c-9c	direct double_byte 80-9e char_bias bae0
9c-9c	direct double_byte 9f-fc char_bias bb82
9d-9d	direct double_byte 40-7e char_bias bbe1
9d-9d	direct double_byte 80-9e char_bias bbe0
9d-9d	direct double_byte 9f-fc char_bias bc82
9e-9e	direct double_byte 40-7e char_bias bce1
9e-9e	direct double_byte 80-9e char_bias bce0
9e-9e	direct double_byte 9f-fc char_bias bd82
9f-9f	direct double_byte 40-7e char_bias bde1
9f-9f	direct double_byte 80-9e char_bias bde0
9f-9f	direct double_byte 9f-fc char_bias be82
e0-e0	direct double_byte 40-7e char_bias 7ee1
e0-e0	direct double_byte 80-9e char_bias 7ee0
e0-e0	direct double_byte 9f-fc char_bias 7f82
e1-e1	direct double_byte 40-7e char_bias 8fe1
e1-e1	direct double_byte 80-9e char_bias 7fe0
e1-e1	direct double_byte 9f-fc char_bias 8082
e2-e2	direct double_byte 40-7e char_bias 80e1
e2-e2	direct double_byte 80-9e char_bias 80e0
e2-e2	direct double_byte 9f-fc char_bias 8182
e3-e3	direct double_byte 40-7e char_bias 81e1
e3-e3	direct double_byte 80-9e char_bias 81e0
e3-e3	direct double_byte 9f-fc char_bias 8282
e4-e4	direct double_byte 40-7e char_bias 82e1
e4-e4	direct double_byte 80-9e char_bias 82e0
e4-e4	direct double_byte 9f-fc char_bias 8382
e5-e5	direct double_byte 40-7e char_bias 83e1
e5-e5	direct double_byte 80-9e char_bias 83e0
e5-e5	direct double_byte 9f-fc char_bias 8482
e6-e6	direct double_byte 40-7e char_bias 84e1
e6-e6	direct double_byte 80-9e char_bias 84e0
e6-e6	direct double_byte 9f-fc char_bias 8582
e7-e7	direct double_byte 40-7e char_bias 85e1
e7-e7	direct double_byte 80-9e char_bias 85e0
e7-e7	direct double_byte 9f-fc char_bias 8682
e8-e8	direct double_byte 40-7e char_bias 86e1
e8-e8	direct double_byte 80-9e char_bias 86e0
e8-e8	direct double_byte 9f-fc char_bias 8782
e9-e9	direct double_byte 40-7e char_bias 87e1
e9-e9	direct double_byte 80-9e char_bias 87e0
e9-e9	direct double_byte 9f-fc char_bias 8882
ea-ea	direct double_byte 40-7e char_bias 88e1
ea-ea	direct double_byte 80-9e char_bias 88e0
ea-ea	direct double_byte 9f-fc char_bias 8982
eb-eb	direct double_byte 40-7e char_bias 89e1
eb-eb	direct double_byte 80-9e char_bias 89e0
eb-eb	direct double_byte 9f-fc char_bias 8a82
ec-ec	direct double_byte 40-7e char_bias 8ae1
ec-ec	direct double_byte 80-9e char_bias 8ae0
ec-ec	direct double_byte 9f-fc char_bias 8b82
ed-ed	direct double_byte 40-7e char_bias 8be1
ed-ed	direct double_byte 80-9e char_bias 8be0
ed-ed	direct double_byte 9f-fc char_bias 8c82
ee-ee	direct double_byte 40-7e char_bias 8ce1
ee-ee	direct double_byte 80-9e char_bias 8ce0
ee-ee	direct double_byte 9f-fc char_bias 8d82
ef-ef	direct double_byte 40-7e char_bias 8de1
ef-ef	direct double_byte 80-9e char_bias 8de0
ef-ef	direct double_byte 9f-fc char_bias 8e82

80-ff	direct char_bias 2000


$output
2000-207f	direct_cell 
2f21-2f5f	direct_cell char_bias 80

$  Shift-JIS output tables
3021-307e	direct_cell direct_row char_bias 587e
3121-315f	direct_cell direct_row char_bias 581f
3160-317e	direct_cell direct_row char_bias 5820
3221-327e	direct_cell direct_row char_bias 577e
3321-335f	direct_cell direct_row char_bias 571f
3360-337e	direct_cell direct_row char_bias 5720
3421-347e	direct_cell direct_row char_bias 567e
3521-355f	direct_cell direct_row char_bias 561f
3560-357e	direct_cell direct_row char_bias 5620
3621-367e	direct_cell direct_row char_bias 557e
3721-375f	direct_cell direct_row char_bias 551f
3760-377e	direct_cell direct_row char_bias 5520
3821-387e	direct_cell direct_row char_bias 547e
3921-395f	direct_cell direct_row char_bias 541f
3960-397e	direct_cell direct_row char_bias 5420
3a21-3a7e	direct_cell direct_row char_bias 537e
3b21-3b5f	direct_cell direct_row char_bias 531f
3b60-3b7e	direct_cell direct_row char_bias 5320
3c21-3c7e	direct_cell direct_row char_bias 527e
3d21-3d5f	direct_cell direct_row char_bias 521f
3d60-3d7e	direct_cell direct_row char_bias 5220
3e21-3e7e	direct_cell direct_row char_bias 517e
3f21-3f5f	direct_cell direct_row char_bias 511f
3f60-3f7e	direct_cell direct_row char_bias 5120
4021-407e	direct_cell direct_row char_bias 507e
4121-415f	direct_cell direct_row char_bias 501f
4160-417e	direct_cell direct_row char_bias 5020
4221-427e	direct_cell direct_row char_bias 4f7e
4321-435f	direct_cell direct_row char_bias 4f1f
4360-437e	direct_cell direct_row char_bias 4f20
4421-447e	direct_cell direct_row char_bias 4e7e
4521-455f	direct_cell direct_row char_bias 4e1f
4560-457e	direct_cell direct_row char_bias 4e20
4621-467e	direct_cell direct_row char_bias 4d7e
4721-475f	direct_cell direct_row char_bias 4d1f
4760-477e	direct_cell direct_row char_bias 4d20
4821-487e	direct_cell direct_row char_bias 4c7e
4921-495f	direct_cell direct_row char_bias 4c1f
4960-497e	direct_cell direct_row char_bias 4c20
4a21-4a7e	direct_cell direct_row char_bias 4b7e
4b21-4b5f	direct_cell direct_row char_bias 4b1f
4b60-4b7e	direct_cell direct_row char_bias 4b20
4c21-4c7e	direct_cell direct_row char_bias 4a7e
4d21-4d5f	direct_cell direct_row char_bias 4a1f
4d60-4d7e	direct_cell direct_row char_bias 4a20
4e21-4e7e	direct_cell direct_row char_bias 497e
4f21-4f5f	direct_cell direct_row char_bias 491f
4f60-4f7e	direct_cell direct_row char_bias 4920
5021-507e	direct_cell direct_row char_bias 487e
5121-515f	direct_cell direct_row char_bias 481f
5160-517e	direct_cell direct_row char_bias 4820
5221-527e	direct_cell direct_row char_bias 477e
5321-535f	direct_cell direct_row char_bias 471f
5360-537e	direct_cell direct_row char_bias 4720
5421-547e	direct_cell direct_row char_bias 467e
5521-555f	direct_cell direct_row char_bias 461f
5560-557e	direct_cell direct_row char_bias 4620
5621-567e	direct_cell direct_row char_bias 457e
5721-575f	direct_cell direct_row char_bias 451f
5760-577e	direct_cell direct_row char_bias 4520
5821-587e	direct_cell direct_row char_bias 447e
5921-595f	direct_cell direct_row char_bias 441f
5960-597e	direct_cell direct_row char_bias 4420
5a21-5a7e	direct_cell direct_row char_bias 437e
5b21-5b5f	direct_cell direct_row char_bias 431f
5b60-5b7e	direct_cell direct_row char_bias 4320
5c21-5c7e	direct_cell direct_row char_bias 427e
5d21-5d5f	direct_cell direct_row char_bias 421f
5d60-5d7e	direct_cell direct_row char_bias 4220
5e21-5e7e	direct_cell direct_row char_bias 417e
5f21-5f5f	direct_cell direct_row char_bias 811f
5f60-5f7e	direct_cell direct_row char_bias 8120
6021-607e	direct_cell direct_row char_bias 807e
6121-615f	direct_cell direct_row char_bias 801f
6160-617e	direct_cell direct_row char_bias 8020
6221-627e	direct_cell direct_row char_bias 7f7e
6321-635f	direct_cell direct_row char_bias 7f1f
6360-637e	direct_cell direct_row char_bias 7f20
6421-647e	direct_cell direct_row char_bias 7e7e
6521-655f	direct_cell direct_row char_bias 7e1f
6560-657e	direct_cell direct_row char_bias 7e20
6621-667e	direct_cell direct_row char_bias 7d7e
6721-675f	direct_cell direct_row char_bias 7d1f
6760-677e	direct_cell direct_row char_bias 7d20
6821-687e	direct_cell direct_row char_bias 7c7e
6921-695f	direct_cell direct_row char_bias 7c1f
6960-697e	direct_cell direct_row char_bias 7c20
6a21-6a7e	direct_cell direct_row char_bias 7b7e
6b21-6b5f	direct_cell direct_row char_bias 7b1f
6b60-6b7e	direct_cell direct_row char_bias 7b20
6c21-6c7e	direct_cell direct_row char_bias 7a7e
6d21-6d5f	direct_cell direct_row char_bias 7a1f
6d60-6d7e	direct_cell direct_row char_bias 7a20
6e21-6e7e	direct_cell direct_row char_bias 797e
6f21-6f5f	direct_cell direct_row char_bias 791f
6f60-6f7e	direct_cell direct_row char_bias 7920
7021-707e	direct_cell direct_row char_bias 787e
7121-715f	direct_cell direct_row char_bias 781f
7160-717e	direct_cell direct_row char_bias 7820
7221-727e	direct_cell direct_row char_bias 777e
7321-735f	direct_cell direct_row char_bias 771f
7360-737e	direct_cell direct_row char_bias 7720
7421-747e	direct_cell direct_row char_bias 767e
7521-755f	direct_cell direct_row char_bias 761f
7560-757e	direct_cell direct_row char_bias 7620
7621-767e	direct_cell direct_row char_bias 757e
7721-775f	direct_cell direct_row char_bias 751f
7760-777e	direct_cell direct_row char_bias 7520
7821-787e	direct_cell direct_row char_bias 747e
7921-795f	direct_cell direct_row char_bias 741f
7960-797e	direct_cell direct_row char_bias 7420
7a21-7a7e	direct_cell direct_row char_bias 737e
7b21-7b5f	direct_cell direct_row char_bias 731f
7b60-7b7e	direct_cell direct_row char_bias 7320
7c21-7c7e	direct_cell direct_row char_bias 727e
7d21-7d5f	direct_cell direct_row char_bias 721f
7d60-7d7e	direct_cell direct_row char_bias 7220
7e21-7e7e	direct_cell direct_row char_bias 717e

