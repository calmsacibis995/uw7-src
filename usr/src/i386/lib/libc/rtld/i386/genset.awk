#ident	"@(#)rtld:i386/genset.awk	1.5.1.3"

# Create rtsetaddr.s
# _rt_setaddr() looks up the values of several special symbols 
# defined in rtld that may also be
# found in the user's a.out or other shared libraries.
# If the special symbol is found,
# set the address in the GOT, else do nothing.  
# This is in assembler
# because we have to access the GOT directly.
# The list of symbols is in genset.in. 

BEGIN	{
	count = 1

	printf "\t.file\t\"rtsetaddr.s\"\n"
	printf "/ Generated by genset.awk\n"
	printf "\n\t.globl\t_rt_setaddr\n"
	printf "\t.text\n\t.type\t_rt_setaddr,@function\n\t.align\t16\n"
	printf "_rt_setaddr:\n"
	printf "\tpushl\t%%edx\n"
	printf "\tpushl\t%%edx\n"
	printf "\tpushl\t%%edi\n"
	printf "\tpushl\t%%ebx\n"
	printf "\tcall\t.L1\n"
	printf ".L1:\n"
	printf "\tpopl\t%%ebx\n"
	printf "\taddl\t$_GLOBAL_OFFSET_TABLE_+[.-.L1],%%ebx\n"
	printf "\n"
}

{
	#skip newlines and comments
	if ((NF == 0) || (substr($1,1,1) == "#"))
		next
	# put out a .globl and a .string for the symbol
	printf "/ %s\n", $1
	printf "\t.globl\t%s\n", $1
	printf "\t.section\t.rodata\n\t.align\t4\n"
	printf ".X%.3d:\n\t.string\t\"%s\"\n", count, $1, $1

	# put out the code to look up the symbol and fix up the GOT.
	printf "\t.text\n"
	printf ".SYM%.3d:\n", count
	printf "/ sym = _rt_lookup(\"%s\", 0, _rt_map_head, _rt_map_head, &lm, LOOKUP_NORM);\n", $1
	printf "\tpushl\t$0\n"
	printf "\tmovl\t$16,%%eax\n"
	printf "\taddl\t%%esp,%%eax\n"
	printf "\tpushl\t%%eax\n"
	printf "\tmovl\t_rt_map_head@GOT(%%ebx),%%eax\n"
	printf "\tmovl\t(%%eax),%%edx\n"
	printf "\tleal\t.X%.3d@GOTOFF(%%ebx),%%eax\n", count
	printf "\tpushl\t%%edx\n"
	printf "\tpushl\t%%edx\n"
	printf "\tpushl\t$0\n"
	printf "\tpushl\t%%eax\n"
	printf "\tcall\t_rt_lookup@PLT\n"
	printf "\taddl\t$24,%%esp\n"
	printf "/ if (sym) {\n"
	printf "/    if (!NAME(lm)) {\n"
	printf "/       %s@GOT = sym->st_value;\n", $1
	printf "/    else\n"
	printf "/        %s@GOT = sym->st_value + ADDR(lm);\n", $1
	printf "/ }\n"
	printf "\tmovl\t%%eax,%%edi\n"
	printf "\ttestl\t%%edi,%%edi\n"
	printf "\tje\t.SYM%.3d\n", count + 1
	printf "\tmovl\t12(%%esp),%%eax\n"
	printf "\tcmpl\t$0,4(%%eax)\n"
	printf "\tjne\t.ADR%.3d\n", count
	printf "\tmovl\t4(%%edi),%%edx\n"
	printf "\tmovl\t%%edx,%s@GOT(%%ebx)\n", $1
	printf "\tjmp\t.SYM%.3d\n", count + 1
	printf ".ADR%.3d:\n", count
	printf "\tmovl\t12(%%esp),%%edx\n"
	printf "\tmovl\t4(%%edi),%%ecx\n"
	printf "\tmovl\t(%%edx),%%edx\n"
	printf "\taddl\t%%ecx,%%edx\n"
	printf "\tmovl\t%%edx,%s@GOT(%%ebx)\n", $1
	count++
}

END	{
	printf ".SYM%.3d:\n", count
	printf "\tpopl\t%%ebx\n"
	printf "\tpopl\t%%edi\n"
	printf "\tpopl\t%%edx\n"
	printf "\tpopl\t%%ecx\n"
	printf "\tret\n"
	printf "\t.align\t4\n"
	printf "\t.size\t_rt_setaddr,.-_rt_setaddr\n"
	printf "\t.text\n"
}