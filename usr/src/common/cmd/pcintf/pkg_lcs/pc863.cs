$ SCCSID(@(#)pc863.cs	7.1	LCC)	/* Modified: 15:34:12 10/15/90 */
$	Table pc863

$hexadecimal
$default_char 2a

$input
00-7f	direct char_bias 2000
80-ff	table
	80  20c7
	81  20fc
	82  20e9
	83  20e2
	84  20c2
	85  20e0
	86  20b6
	87  20e7
	88  20ea
	89  20eb
	8a  20e8
	8b  20ef
	8c  20ee
	8d  ac2f
	8e  20c0
	8f  20a7
	90  20c9
	91  20c8
	92  20ca
	93  20f4
	94  20cb
	95  20cf
	96  20fb
	97  20f9
	98  20a4
	99  20d4
	9a  20dc
	9b  20a2
	9c  20a3
	9d  20d9
	9e  20db
	9f  ac70
	a0  20a6
	a1  20b4
	a2  20f3
	a3  20fa
	a4  20a8
	a5  20b8
	a6  20b3
	a7  20af
	a8  20ce
	a9  ac6f
	aa  20ac
	ab  20bd
	ac  20bc
	ad  20be
	ae  20ab
	af  20bb
	b0  25de
	b1  25df
	b2  25e0
	b3  25a2
	b4  25a9
	b5  25d2
	b6  25ce
	b7  25da
	b8  25d6
	b9  25c9
	ba  25c2
	bb  25c4
	bc  25c5
	bd  25dc
	be  25d8
	bf  25a4
	c0  25a6
	c1  25aa
	c2  25a8
	c3  25a7
	c4  25a1
	c5  25ab
	c6  25d3
	c7  25cc
	c8  25c6
	c9  25c3
	ca  25ca
	cb  25c8
	cc  25c7
	cd  25c1
	ce  25cb
	cf  25cf
	d0  25d5
	d1  25cd
	d2  25d4
	d3  25dd
	d4  25d9
	d5  25d7
	d6  25db
	d7  25d1
	d8  25d0
	d9  25a5
	da  25a3
	db  25e1
	dc  25e2
	dd  25e4
	de  25e5
	df  25e3
	e0  2841
	e1  20df
	e2  2823
	e3  2850
	e4  2832
	e5  2852
	e6  20b5
	e7  2853
	e8  2835
	e9  2828
	ea  2838
	eb  2844
	ec  2567
	ed  2855
	ee  2845
	ef  ac41
	f0  ac61
	f1  20b1
	f2  2566
	f3  2565
	f4  ac6b
	f5  ac6c
	f6  20f7
	f7  ac6d
	f8  20b0
	f9  2526
	fa  20b7
	fb  ac65
	fc  ac6e
	fd  20b2
	fe  25e6
	ff  20a0


$output
2000-207f	direct_cell
20a0-20ff	table
	20a0  ff
	20a1  2a	not_exact
	20a2  9b
	20a3  9c
	20a4  98
	20a5  59	not_exact
	20a6  a0
	20a7  8f
	20a8  a4
	20a9  2a	not_exact has_multi 28 43 29
	20aa  2a	not_exact
	20ab  ae
	20ac  aa
	20ad  2d	not_exact
	20ae  2a	not_exact has_multi 28 52 29
	20af  a7
	20b0  f8
	20b1  f1
	20b2  fd
	20b3  a6
	20b4  a1
	20b5  e6
	20b6  86
	20b7  fa
	20b8  a5
	20b9  2a	not_exact
	20ba  2a	not_exact
	20bb  af
	20bc  ac
	20bd  ab
	20be  ad
	20bf  2a	not_exact
	20c0  8e
	20c1  41	not_exact
	20c2  84
	20c3  41	not_exact
	20c4  41	not_exact
	20c5  41	not_exact
	20c6  2a	not_exact has_multi 41 45
	20c7  80
	20c8  91
	20c9  90
	20ca  92
	20cb  94
	20cc  49	not_exact
	20cd  49	not_exact
	20ce  a8
	20cf  95
	20d0  44	not_exact
	20d1  4e	not_exact
	20d2  4f	not_exact
	20d3  4f	not_exact
	20d4  99
	20d5  4f	not_exact
	20d6  4f	not_exact
	20d7  78	not_exact
	20d8  ed	not_exact
	20d9  9d
	20da  55	not_exact
	20db  9e
	20dc  9a
	20dd  59	not_exact
	20de  2a	not_exact
	20df  e1
	20e0  85
	20e1  61	not_exact
	20e2  83
	20e3  61	not_exact
	20e4  61	not_exact
	20e5  61	not_exact
	20e6  2a	not_exact has_multi 61 65
	20e7  87
	20e8  8a
	20e9  82
	20ea  88
	20eb  89
	20ec  69	not_exact
	20ed  69	not_exact
	20ee  8c
	20ef  8b
	20f0  eb	not_exact
	20f1  6e	not_exact
	20f2  6f	not_exact
	20f3  a2
	20f4  93
	20f5  6f	not_exact
	20f6  6f	not_exact
	20f7  f6
	20f8  ed	not_exact
	20f9  97
	20fa  a3
	20fb  96
	20fc  81
	20fd  79	not_exact
	20fe  2a	not_exact
	20ff  79	not_exact

2526-2526	direct_cell char_bias d3

2565-2567	table
	2565  f3
	2566  f2
	2567  ec

25a1-25ab	table
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

25c1-25e6	table
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
	25cc  c7
	25cd  d1
	25ce  b6
	25cf  cf
	25d0  d8
	25d1  d7
	25d2  b5
	25d3  c6
	25d4  d2
	25d5  d0
	25d6  b8
	25d7  d5
	25d8  be
	25d9  d4
	25da  b7
	25db  d6
	25dc  bd
	25dd  d3
	25de  b0
	25df  b1
	25e0  b2
	25e1  db
	25e2  dc
	25e3  df
	25e4  dd
	25e5  de
	25e6  fe

2823-2823	direct_cell char_bias bf
2828-2828	direct_cell char_bias c1
2832-2832	direct_cell char_bias b2
2835-2835	direct_cell char_bias b3
2838-2838	direct_cell char_bias b2

2841-2845	table
	2841  e0
	2842  2a	not_exact
	2843  2a	not_exact
	2844  eb
	2845  ee

2850-2855	table
	2850  e3
	2851  2a	not_exact
	2852  e5
	2853  e7
	2854  2a	not_exact
	2855  ed

ac2f-ac2f	direct_cell char_bias 5e
ac41-ac41	direct_cell char_bias ae
ac61-ac61	direct_cell char_bias 8f
ac65-ac65	direct_cell char_bias 96

ac6b-ac71	table
	ac6b  f4
	ac6c  f5
	ac6d  f7
	ac6e  fc
	ac6f  a9
	ac70  9f
	ac71  9e
