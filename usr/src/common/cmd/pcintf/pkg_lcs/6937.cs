$ SCCSID(@(#)6937.cs	7.1	LCC)	/* Modified: 15:30:58 10/15/90 */
$	Table 6937

$hexadecimal
$default_char 2a

$input
24-24	table
	24  20a4
00-9f	direct char_bias 2000 
c1-c1	dead_char
	41  20c0
	61  20e0
	45  20c8
	65  20e8
	49  20cc
	69  20ec
	4f  20d2
	6f  20f2
	55  20d9
	75  20f9

c2-c2	dead_char
	41  20c1
	61  20e1
	43  0000
	63  0000
	45  20c9
	65  20e9
	67  0000
	49  20cd
	69  20ed
	4c  0000
	6c  0000
	4e  0000
	6e  0000
	4f  20d3
	6f  20f3
	52  0000
	72  0000
	53  0000
	73  0000
	55  20da
	75  20fa
	59  20dd
	79  20fd
	5a  0000
	7a  0000
	20  20b4

c3-c3	dead_char
	41  20c2
	61  20e2
	43  0000
	63  0000
	45  20ca
	65  20ea
	47  0000
	67  0000
	48  0000
	68  0000
	49  20ce
	69  20ee
	4a  0000
	6a  0000
	4f  20d4
	6f  20f4
	53  0000
	73  0000
	55  20db
	75  20fb
	57  0000
	77  0000
	59  0000
	79  0000

c4-c4	dead_char
	41  20c3
	61  20e3
	49  0000
	69  0000
	4e  20d1
	6e  20f1
	4f  20d5
	6f  20f5
	55  0000
	75  0000

c5-c5	dead_char
	41  0000
	61  0000
	45  0000
	65  0000
	49  0000
	69  0000
	4f  0000
	6f  0000
	55  0000
	75  0000
	20  20af

c6-c6	dead_char
	41  0000
	61  0000
	47  0000
	67  0000
	55  0000
	75  0000
	20  0000

c7-c7	dead_char
	43  0000
	63  0000
	45  0000
	65  0000
	47  0000
	67  0000
	49  0000
	5a  0000
	7a  0000
	20  0000

c8-c8	dead_char
	41  20c4
	61  20e4
	45  20cb
	65  20eb
	49  20cf
	69  20ef
	4f  20d6
	6f  20f6
	55  20dc
	75  20fc
	59  0000
	79  20ff
	20  20a8

ca-ca	dead_char
	41  20c5
	61  20e5
	55  0000
	75  0000
	20  20b0

cb-cb	dead_char
	43  20c7
	63  20e7
	47  0000
	4b  0000
	6b  0000
	4c  0000
	6c  0000
	4e  0000
	6e  0000
	52  0000
	72  0000
	53  0000
	73  0000
	54  0000
	74  0000
	20  20b8

cd-cd	dead_char
	4f  0000
	6f  0000
	55  0000
	75  0000
	20  0000

ce-ce	dead_char
	41  0000
	61  0000
	45  0000
	65  0000
	49  0000
	69  0000
	55  0000
	75  0000
	20  0000

cf-cf	dead_char
	43  0000
	63  0000
	44  0000
	64  0000
	45  0000
	65  0000
	4c  0000
	6c  0000
	52  0000
	72  0000
	53  0000
	73  0000
	54  0000
	74  0000
	5a  0000
	7a  0000
	20  0000

a0-ff	table
	a0  0000
	a1  20a1
	a2  20a2
	a3  20a3
	a4  2024
	a5  20a5
	a6  2023
	a7  20a7
	a8  20a4
	a9  0000
	aa  0000
	ab  20ab
	ac  0000
	ad  0000
	ae  0000
	af  0000
	b0  20b0
	b1  20b1
	b2  20b2
	b3  20b3
	b4  20d7
	b5  20b5
	b6  20b6
	b7  20b7
	b8  20f7
	b9  0000
	ba  0000
	bb  20bb
	bc  20bc
	bd  20bd
	be  20be
	bf  20bf
	c0  0000
	c1  2060
	c2  20b4
	c3  205e
	c4  207e
	c5  20af
	c6  0000
	c7  0000
	c8  20a8
	c9  0000
	ca  20b0
	cb  20b8
	cc  0000
	cd  0000
	ce  0000
	cf  0000
	d0  0000
	d1  20b9
	d2  20ae
	d3  20a9
	d4  0000
	d5  0000
	d6  0000
	d7  0000
	d8  0000
	d9  0000
	da  0000
	db  0000
	dc  0000
	dd  0000
	de  0000
	df  0000
	e0  0000
	e1  20c6
	e2  20c0
	e3  20aa
	e4  0000
	e5  0000
	e6  0000
	e7  0000
	e8  0000
	e9  20d8
	ea  0000
	eb  20ba
	ec  20de
	ed  0000
	ee  0000
	ef  0000
	f0  0000
	f1  20e6
	f2  0000
	f3  20f0
	f4  0000
	f5  0000
	f6  0000
	f7  0000
	f8  0000
	f9  20f8
	fa  0000
	fb  20df
	fc  20fe
	fd  0000
	fe  0000
	ff  0000


$output
2024-2024	table
	2024  a4	
2000-207f	direct_cell 
20a0-20ff	table_4b
	20a0  00	
	20a1  a1	
	20a2  a2	
	20a3  a3	
	20a4  24	
	20a5  a5	
	20a6  00	
	20a7  a7	
	20a8  c8	has_2b c8 20
	20a9  d3	
	20aa  e3	
	20ab  ab	
	20ac  00	
	20ad  00	
	20ae  d2	
	20af  c5	has_2b c5 20
	20b0  b0	
	20b1  b1	
	20b2  b2	
	20b3  b3	
	20b4  c2	has_2b c2 20
	20b5  b5	
	20b6  b6	
	20b7  b7	
	20b8  cb	has_2b cb 20
	20b9  d1	
	20ba  eb	
	20bb  bb	
	20bc  bc	
	20bd  bd	
	20be  be	
	20bf  bf	
	20c0  41	has_2b c1 41
	20c1  41	has_2b c2 41
	20c2  41	has_2b c3 41
	20c3  41	has_2b c4 41
	20c4  41	has_2b c8 41
	20c5  41	has_2b ca 41
	20c6  e1	
	20c7  43	has_2b cb 43
	20c8  45	has_2b c1 45
	20c9  45	has_2b c2 45
	20ca  45	has_2b c3 45
	20cb  45	has_2b c8 45
	20cc  49	has_2b c1 49
	20cd  49	has_2b c2 49
	20ce  49	has_2b c3 49
	20cf  49	has_2b c8 49
	20d0  e2	
	20d1  4e	has_2b c4 4e
	20d2  4f	has_2b c1 4f
	20d3  4f	has_2b c2 4f
	20d4  4f	has_2b c3 4f
	20d5  4f	has_2b c4 4f
	20d6  4f	has_2b c8 4f
	20d7  b4	
	20d8  e9	
	20d9  55	has_2b c1 55
	20da  55	has_2b c2 55
	20db  55	has_2b c3 55
	20dc  55	has_2b c8 55
	20dd  59	has_2b c2 59
	20de  ec	
	20df  fb	
	20e0  61	has_2b c1 61
	20e1  61	has_2b c2 61
	20e2  61	has_2b c3 61
	20e3  61	has_2b c4 61
	20e4  61	has_2b c8 61
	20e5  61	has_2b ca 61
	20e6  f1	
	20e7  63	has_2b cb 63
	20e8  65	has_2b c1 65
	20e9  65	has_2b c2 65
	20ea  65	has_2b c3 65
	20eb  65	has_2b c8 65
	20ec  69	has_2b c1 69
	20ed  69	has_2b c2 69
	20ee  69	has_2b c3 69
	20ef  69	has_2b c8 69
	20f0  f3	
	20f1  6e	has_2b c4 6e
	20f2  6f	has_2b c1 6f
	20f3  6f	has_2b c2 6f
	20f4  6f	has_2b c3 6f
	20f5  6f	has_2b c4 6f
	20f6  6f	has_2b c8 6f
	20f7  b8	
	20f8  f9	
	20f9  75	has_2b c1 75
	20fa  75	has_2b c2 75
	20fb  75	has_2b c3 75
	20fc  75	has_2b c8 75
	20fd  79	has_2b c2 79
	20fe  fc	
	20ff  79	has_2b c8 79
