#ident	"@(#)nas:i386/chkgen.awk	1.3"
# chkgen.awk
# Generate check and generate tables for i386 assembler from input

BEGIN {
    # operand modes
    om["$"] = "OC_LIT";
    om["m"] = "OC_MEM";
    om["m8"] = "OC_MEM";
    om["m16"] = "OC_MEM";
    om["m32"] = "OC_MEM";
    om["r8"] = "OC_R8";
    om["r16"] = "OC_R16";
    om["r32"] = "OC_R32";
    om["sreg"] = "OC_SEG";
    om["spreg"] = "OC_SPEC";
    om["%st"] = "OC_ST";
    om["%stn"] = "OC_STn";
    om["%al"] = "OC_R8";
    om["%ax"] = "OC_R16";
    om["%eax"] = "OC_R32";
    om["%cl"] = "OC_R8";

    # These qualify as memory operands.
    mem["m"] = 1;
    mem["m8"] = 1;
    mem["m16"] = 1;
    mem["m32"] = 1;

    # register numbers
    rn["%al"] = "Reg_al";
    rn["%ax"] = "Reg_ax";
    rn["%eax"] = "Reg_eax";
    rn["%cl"] = "Reg_cl";
    # These are really operand sizes
    rn["m8"] = 1;
    rn["m16"] = 2;
    rn["m32"] = 4;

    # generate flags
    gf["/r"] = "GL_SLASH_R";
    gf["/0"] = "GL_SLASH_N";
    gf["/1"] = "GL_SLASH_N";
    gf["/2"] = "GL_SLASH_N";
    gf["/3"] = "GL_SLASH_N";
    gf["/4"] = "GL_SLASH_N";
    gf["/5"] = "GL_SLASH_N";
    gf["/6"] = "GL_SLASH_N";
    gf["/7"] = "GL_SLASH_N";
    gf["+r"] = "GL_PLUS_R";
    gf["ib"] = "GL_IMM8";
    gf["iw"] = "GL_IMM";
    gf["id"] = "GL_IMM";
    gf["R"] = "GL_MEMRIGHT";
    gf["L"] = "GL_MEMLEFT";
    gf["W"] = "GL_OVERRIDE";
    gf["<"] = "GL_PREFIX";
    gf["~"] = "GL_FWAIT";
    gf["moff"] = "GL_MOFFSET";
    gf["@1"] = "GL_PCREL";
    gf["@4"] = "GL_PCREL";

    # /n flags
    sn["/0"] = "0";
    sn["/1"] = "1";
    sn["/2"] = "2";
    sn["/3"] = "3";
    sn["/4"] = "4";
    sn["/5"] = "5";
    sn["/6"] = "6";
    sn["/7"] = "7";
    sn["@1"] = "1";
    sn["@4"] = "4";

    if (ARGC != 4) {
	printf("Usage:  awk -f chkgen.awk <input> <.h output> <.c output>\n");
	exit(1);
    }
    dot_h = ARGV[2];
    dot_c = ARGV[3];
    ARGC = 2;			# no further input after first

    system("(echo '#ifndef CHKGEN_H\n#define CHKGEN_H\n'; cat ./chkgen.0.h) >" dot_h);
    printf("") >>dot_h;		# so further stuff is appended
}

# comments
$1 ~ /^[#\/*]/	{ print >dot_c; next }
# start table
($0 ~ /^&?[a-z]/) && (intable == 0)	{
		intable = 1;
		ntablines = 0;
		noutlines = 0;
		chklab = $1;
		ngentabs = NF - 1;
		for (i = 0; i < ngentabs; ++i)
		    genlab[i] = $(i+2);
		next;
		}
# label
(NF == 1) && (intable != 0) && ($1 ~ /^[a-zA-Z_0-9]*:$/) {
			split($1, t, ":");
			print "#define", t[1], noutlines >dot_h;
			next;
		}
# end of lookup part of table
(NF == 1) && (intable != 0) && ($1 ~ /^--*$/) {
			chktab[noutlines] = "CLEND,";
			for (i = 0; i < ngentabs; ++i)
			    gentab[noutlines, i] = "GL(0,0,0),";
			++noutlines;
			++ntablines;
			next;
		}
# end table
NF == 0		{ if (intable) {
		    # output the chk table and gen tables
		    printf "%s const chklist_t %s[] = {\n",
			stat_flag(chklab), label(chklab) >dot_c;
		    for (i = 0; i < noutlines; ++i)
			print chktab[i] >dot_c;
		    printf("};\n\n") >dot_c;
		    if (stat_flag(chklab) == "")
			printf "extern const chklist_t %s[];\n", label(chklab) >dot_h;
		    for (i = 0; i < ngentabs; ++i) {
			printf "%s const genlist_t %s[] = {\n",
			    stat_flag(genlab[i]), label(genlab[i]) >dot_c;
			for (j = 0; j < noutlines; ++j)
			    print gentab[j, i] >dot_c;
			printf "};\n" >dot_c;
			if (stat_flag(genlab[i]) == "")
			    printf "extern const genlist_t %s[];\n", label(genlab[i]) >dot_h;
		    }
		    print "" >dot_c;
		  }
		  intable = 0; next;
		  }
(NF == ngentabs+1) && (intable != 0) {
			do_line();
			++ntablines;
			next;
		  }
		{ err("bad input: " $0); next; }
END 		{ if (intable)
			err("need blank line at end");
		  printf("#endif /* ndef CHKGEN_H */\n") >dot_h;
		  exit(nerrors);
		}
function err(s) {
    printf(">> line %d: %s\n", NR, s);
    ++nerrors;
}
function do_gentabs(impflgs) {
    for (i = 0; i < ngentabs; ++i) {
	flags = $(i+2);
	if (impflgs != "")
	    flags = flags","impflgs
	do_gen(flags, i);
    }
    ++noutlines;
}
function do_line(n,opns,t,rn,implicit,nl,lsplit,nr,rsplit) {
    n = split($1, opns, ",");
    # Single operand sets right-side auxno.
    if (n == 1) {
	nl = split(opns[1], lsplit, "/");
	for (il = 1; il <= nl; ++il) {
	    chktab[noutlines] = sprintf("{ %s, 0, %s },",
					get_om(lsplit[il]), get_rn(lsplit[il]));
	    do_gentabs("");
	}
    }
    else if (n == 2) {
	nl = split(opns[1], lsplit, "/");
	nr = split(opns[2], rsplit, "/");
	# Collect implicit generate flags, which apply to all combinations
	# on this line
	implicit = "";
	for (il = 1; il <= nl; ++il) {
	    for (ir = 1; ir <= nr; ++ir) {
		# check for memory operands
		if (mem[lsplit[il]] != 0)
		    implicit = "L";
		if (mem[rsplit[ir]] != 0)
		    implicit = "R";
		else if (rsplit[ir] == "$")	# literal behaves like reg. op.
		    implicit = "L,id";
	    }
	}
	for (il = 1; il <= nl; ++il) {
	    for (ir = 1; ir <= nr; ++ir) {
		chktab[noutlines] = sprintf("{ CASEVAL(%s,%s), %s, %s },",
						get_om(lsplit[il]),
						get_om(rsplit[ir]),
						get_rn(lsplit[il]),
						get_rn(rsplit[ir]));
		do_gentabs(implicit);
	    }
	}
    }
    else
	err("bad number of operands: " n);
}
function do_gen(s, nn, n, gens, t, i, op, op2, snt) {
    n = split(s, gens, ",");
    op = gens[1];		# opcode 1
    if (op !~ /^0/)
	err("bad op code: " op);
    i = 2;
    if ((n >= 2) && (gens[2] ~ /^0/)) {
	op2 = gens[2];
	gens[n+1] = "op2";
	++n;
	++i;
    }
    op2 = "0";
    if (n < 2)
	t = "0";
    else
	t = "";
    for (; i <= n; ++i) {
	t = sprintf("%s%s%s", t, t != "" ? "|" : "", get_gf(gens[i]));
	snt = get_sn(gens[i]);
	# pick up /n, if present
	if (snt != "0")
	    op2 = snt;
    }
    # Check for both MEMLEFT and MEMRIGHT
    if ((t ~ /GL_MEMLEFT/) && (t ~ /GL_MEMRIGHT/))
	err("both L and R specified");
    gentab[noutlines, nn] = sprintf("GL(%s, %s, %s),", op, t, op2);
}
function get_om(s,t) {
    t = om[s];
    if (t == "")
	err("bad operand mode: " s);
    return t;
}
function get_rn(s, t) {
    t = rn[s];
    if (t == "")
	return "0";
    if (s ~ "%")		# add 1 to register numbers
	t = t"+1";
    return t;
}
function get_gf(s, t) {
    t = gf[s];
    if (t == "")
	err("bad generate flag: " s);
    return t;
}
function get_sn(s, t) {
    t = sn[s];
    if (t == "")
	t = "0";
    return t;
}
# Return "static" or null string, depending on whether
# label is not or is prefixed by "&".
function stat_flag(s) {
    if (s ~ /^&/) return "";
    return "static";
}

# Return label, unprefixed by "&", if any.
function label(s) {
    if (s ~ /^&/) return substr(s,2);
    return s;
}
