$ SCCSID(@(#)pc850.cs	7.1	LCC)	/* Modified: 15:33:54 10/15/90 */
$	Table pc850

$hexadecimal
$default_char 2a

$input
00-7f	direct char_bias 2000 
80-ff	table
	80  20c7
	81  20fc
	82  20e9
	83  20e2
	84  20e4
	85  20e0
	86  20e5
	87  20e7
	88  20ea
	89  20eb
	8a  20e8
	8b  20ef
	8c  20ee
	8d  20ec
	8e  20c4
	8f  20c5
	90  20c9
	91  20e6
	92  20c6
	93  20f4
	94  20f6
	95  20f2
	96  20fb
	97  20f9
	98  20ff
	99  20d6
	9a  20dc
	9b  20f8
	9c  20a3
	9d  20d8
	9e  20d7
	9f  ac70
	a0  20e1
	a1  20ed
	a2  20f3
	a3  20fa
	a4  20f1
	a5  20d1
	a6  20aa
	a7  20ba
	a8  20bf
	a9  20ae
	aa  20ac
	ab  20bd
	ac  20bc
	ad  20a1
	ae  20ab
	af  20bb
	b0  25de
	b1  25df
	b2  25e0
	b3  25a2
	b4  25a9
	b5  20c1
	b6  20c2
	b7  20c0
	b8  20a9
	b9  25c9
	ba  25c2
	bb  25c4
	bc  25c5
	bd  20a2
	be  20a5
	bf  25a4
	c0  25a6
	c1  25aa
	c2  25a8
	c3  25a7
	c4  25a1
	c5  25ab
	c6  20e3
	c7  20c3
	c8  25c6
	c9  25c3
	ca  25ca
	cb  25c8
	cc  25c7
	cd  25c1
	ce  25cb
	cf  20a4
	d0  20f0
	d1  20d0
	d2  20ca
	d3  20cb
	d4  20c8
	d5  21a1
	d6  20cd
	d7  20ce
	d8  20cf
	d9  25a5
	da  25a3
	db  25e1
	dc  25e2
	dd  20a6
	de  20cc
	df  25e3
	e0  20d3
	e1  20df
	e2  20d4
	e3  20d2
	e4  20f5
	e5  20d5
	e6  20b5
	e7  20fe
	e8  20de
	e9  20da
	ea  20db
	eb  20d9
	ec  20fd
	ed  20dd
	ee  20af
	ef  20b4
	f0  20ad
	f1  20b1
	f2  ac2f
	f3  20be
	f4  20b6
	f5  20a7
	f6  20f7
	f7  20b8
	f8  20b0
	f9  20a8
	fa  20b7
	fb  20b9
	fc  20b3
	fd  20b2
	fe  25e6
	ff  20a0


$output
2000-207f	direct_cell 
20a0-20ff	table 
	20a0  ff	
	20a1  ad	
	20a2  bd	
	20a3  9c	
	20a4  cf	
	20a5  be	
	20a6  dd	
	20a7  f5	
	20a8  f9	
	20a9  b8	
	20aa  a6	
	20ab  ae	
	20ac  aa	
	20ad  f0	
	20ae  a9	
	20af  ee	
	20b0  f8	
	20b1  f1	
	20b2  fd	
	20b3  fc	
	20b4  ef	
	20b5  e6	
	20b6  f4	
	20b7  fa	
	20b8  f7	
	20b9  fb	
	20ba  a7	
	20bb  af	
	20bc  ac	
	20bd  ab	
	20be  f3	
	20bf  a8	
	20c0  b7	
	20c1  b5	
	20c2  b6	
	20c3  c7	
	20c4  8e	
	20c5  8f	
	20c6  92	
	20c7  80	
	20c8  d4	
	20c9  90	
	20ca  d2	
	20cb  d3	
	20cc  de	
	20cd  d6	
	20ce  d7	
	20cf  d8	
	20d0  d1	
	20d1  a5	
	20d2  e3	
	20d3  e0	
	20d4  e2	
	20d5  e5	
	20d6  99	
	20d7  9e	
	20d8  9d	
	20d9  eb	
	20da  e9	
	20db  ea	
	20dc  9a	
	20dd  ed	
	20de  e8	
	20df  e1	
	20e0  85	
	20e1  a0	
	20e2  83	
	20e3  c6	
	20e4  84	
	20e5  86	
	20e6  91	
	20e7  87	
	20e8  8a	
	20e9  82	
	20ea  88	
	20eb  89	
	20ec  8d	
	20ed  a1	
	20ee  8c	
	20ef  8b	
	20f0  d0	
	20f1  a4	
	20f2  95	
	20f3  a2	
	20f4  93	
	20f5  e4	
	20f6  94	
	20f7  f6	
	20f8  9b	
	20f9  97	
	20fa  a3	
	20fb  96	
	20fc  81	
	20fd  ec	
	20fe  e7	
	20ff  98	

25a0-25ab	table 
	25a1  c4
	25a2  b3
	25a3  da
	25a4  bf
	25a5  d9
	25a6  c0
	25a7  c3
	25a8  c2
	25a9  b4
	25aa  c1
	25ab  c5

25c0-25e6	table 
	25c1  cd
	25c2  ba
	25c3  c9
	25c4  bb
	25c5  bc
	25c6  c8
	25c7  cc
	25c8  cb
	25c9  b9
	25ca  ca
	25cb  ce
	25cc  c3	not_exact
	25cd  c2	not_exact
	25ce  b4	not_exact
	25cf  c1	not_exact
	25d0  c5	not_exact
	25d1  c5	not_exact
	25d2  b4	not_exact
	25d3  c3	not_exact
	25d4  c2	not_exact
	25d5  c1	not_exact
	25d6  bf	not_exact
	25d7  da	not_exact
	25d8  d9	not_exact
	25d9  c0	not_exact
	25da  bf	not_exact
	25db  da	not_exact
	25dc  d9	not_exact
	25dd  c0	not_exact
	25de  b0
	25df  b1
	25e0  b2
	25e1  db
	25e2  dc
	25e3  df
	25e4  2a	not_exact
	25e5  2a	not_exact
	25e6  fe

21a1-21a1	direct_cell char_bias 34
ac2f-ac2f	direct_cell char_bias c3
ac70-ac70	direct_cell char_bias 2f
