#ident	"@(#)fur:i386/cmd/fur/cte.s	1.1"
jo_test:
movl 8(%esp),%eax
testl 4(%esp),%eax
jo .jo_test.lab
movl $0,%eax
ret
.jo_test.lab:
movl $1,%eax
ret
.type jo_test,@function
.size jo_test,.-jo_test
.globl jo_test
jo_cmp:
movl 8(%esp),%eax
cmpl 4(%esp),%eax
jo .jo_cmp.lab
movl $0,%eax
ret
.jo_cmp.lab:
movl $1,%eax
ret
.type jo_cmp,@function
.size jo_cmp,.-jo_cmp
.globl jo_cmp
jno_test:
movl 8(%esp),%eax
testl 4(%esp),%eax
jno .jno_test.lab
movl $0,%eax
ret
.jno_test.lab:
movl $1,%eax
ret
.type jno_test,@function
.size jno_test,.-jno_test
.globl jno_test
jno_cmp:
movl 8(%esp),%eax
cmpl 4(%esp),%eax
jno .jno_cmp.lab
movl $0,%eax
ret
.jno_cmp.lab:
movl $1,%eax
ret
.type jno_cmp,@function
.size jno_cmp,.-jno_cmp
.globl jno_cmp
jb_test:
movl 8(%esp),%eax
testl 4(%esp),%eax
jb .jb_test.lab
movl $0,%eax
ret
.jb_test.lab:
movl $1,%eax
ret
.type jb_test,@function
.size jb_test,.-jb_test
.globl jb_test
jb_cmp:
movl 8(%esp),%eax
cmpl 4(%esp),%eax
jb .jb_cmp.lab
movl $0,%eax
ret
.jb_cmp.lab:
movl $1,%eax
ret
.type jb_cmp,@function
.size jb_cmp,.-jb_cmp
.globl jb_cmp
jae_test:
movl 8(%esp),%eax
testl 4(%esp),%eax
jae .jae_test.lab
movl $0,%eax
ret
.jae_test.lab:
movl $1,%eax
ret
.type jae_test,@function
.size jae_test,.-jae_test
.globl jae_test
jae_cmp:
movl 8(%esp),%eax
cmpl 4(%esp),%eax
jae .jae_cmp.lab
movl $0,%eax
ret
.jae_cmp.lab:
movl $1,%eax
ret
.type jae_cmp,@function
.size jae_cmp,.-jae_cmp
.globl jae_cmp
je_test:
movl 8(%esp),%eax
testl 4(%esp),%eax
je .je_test.lab
movl $0,%eax
ret
.je_test.lab:
movl $1,%eax
ret
.type je_test,@function
.size je_test,.-je_test
.globl je_test
je_cmp:
movl 8(%esp),%eax
cmpl 4(%esp),%eax
je .je_cmp.lab
movl $0,%eax
ret
.je_cmp.lab:
movl $1,%eax
ret
.type je_cmp,@function
.size je_cmp,.-je_cmp
.globl je_cmp
jne_test:
movl 8(%esp),%eax
testl 4(%esp),%eax
jne .jne_test.lab
movl $0,%eax
ret
.jne_test.lab:
movl $1,%eax
ret
.type jne_test,@function
.size jne_test,.-jne_test
.globl jne_test
jne_cmp:
movl 8(%esp),%eax
cmpl 4(%esp),%eax
jne .jne_cmp.lab
movl $0,%eax
ret
.jne_cmp.lab:
movl $1,%eax
ret
.type jne_cmp,@function
.size jne_cmp,.-jne_cmp
.globl jne_cmp
jbe_test:
movl 8(%esp),%eax
testl 4(%esp),%eax
jbe .jbe_test.lab
movl $0,%eax
ret
.jbe_test.lab:
movl $1,%eax
ret
.type jbe_test,@function
.size jbe_test,.-jbe_test
.globl jbe_test
jbe_cmp:
movl 8(%esp),%eax
cmpl 4(%esp),%eax
jbe .jbe_cmp.lab
movl $0,%eax
ret
.jbe_cmp.lab:
movl $1,%eax
ret
.type jbe_cmp,@function
.size jbe_cmp,.-jbe_cmp
.globl jbe_cmp
ja_test:
movl 8(%esp),%eax
testl 4(%esp),%eax
ja .ja_test.lab
movl $0,%eax
ret
.ja_test.lab:
movl $1,%eax
ret
.type ja_test,@function
.size ja_test,.-ja_test
.globl ja_test
ja_cmp:
movl 8(%esp),%eax
cmpl 4(%esp),%eax
ja .ja_cmp.lab
movl $0,%eax
ret
.ja_cmp.lab:
movl $1,%eax
ret
.type ja_cmp,@function
.size ja_cmp,.-ja_cmp
.globl ja_cmp
js_test:
movl 8(%esp),%eax
testl 4(%esp),%eax
js .js_test.lab
movl $0,%eax
ret
.js_test.lab:
movl $1,%eax
ret
.type js_test,@function
.size js_test,.-js_test
.globl js_test
js_cmp:
movl 8(%esp),%eax
cmpl 4(%esp),%eax
js .js_cmp.lab
movl $0,%eax
ret
.js_cmp.lab:
movl $1,%eax
ret
.type js_cmp,@function
.size js_cmp,.-js_cmp
.globl js_cmp
jns_test:
movl 8(%esp),%eax
testl 4(%esp),%eax
jns .jns_test.lab
movl $0,%eax
ret
.jns_test.lab:
movl $1,%eax
ret
.type jns_test,@function
.size jns_test,.-jns_test
.globl jns_test
jns_cmp:
movl 8(%esp),%eax
cmpl 4(%esp),%eax
jns .jns_cmp.lab
movl $0,%eax
ret
.jns_cmp.lab:
movl $1,%eax
ret
.type jns_cmp,@function
.size jns_cmp,.-jns_cmp
.globl jns_cmp
jp_test:
movl 8(%esp),%eax
testl 4(%esp),%eax
jp .jp_test.lab
movl $0,%eax
ret
.jp_test.lab:
movl $1,%eax
ret
.type jp_test,@function
.size jp_test,.-jp_test
.globl jp_test
jp_cmp:
movl 8(%esp),%eax
cmpl 4(%esp),%eax
jp .jp_cmp.lab
movl $0,%eax
ret
.jp_cmp.lab:
movl $1,%eax
ret
.type jp_cmp,@function
.size jp_cmp,.-jp_cmp
.globl jp_cmp
jnp_test:
movl 8(%esp),%eax
testl 4(%esp),%eax
jnp .jnp_test.lab
movl $0,%eax
ret
.jnp_test.lab:
movl $1,%eax
ret
.type jnp_test,@function
.size jnp_test,.-jnp_test
.globl jnp_test
jnp_cmp:
movl 8(%esp),%eax
cmpl 4(%esp),%eax
jnp .jnp_cmp.lab
movl $0,%eax
ret
.jnp_cmp.lab:
movl $1,%eax
ret
.type jnp_cmp,@function
.size jnp_cmp,.-jnp_cmp
.globl jnp_cmp
jl_test:
movl 8(%esp),%eax
testl 4(%esp),%eax
jl .jl_test.lab
movl $0,%eax
ret
.jl_test.lab:
movl $1,%eax
ret
.type jl_test,@function
.size jl_test,.-jl_test
.globl jl_test
jl_cmp:
movl 8(%esp),%eax
cmpl 4(%esp),%eax
jl .jl_cmp.lab
movl $0,%eax
ret
.jl_cmp.lab:
movl $1,%eax
ret
.type jl_cmp,@function
.size jl_cmp,.-jl_cmp
.globl jl_cmp
jge_test:
movl 8(%esp),%eax
testl 4(%esp),%eax
jge .jge_test.lab
movl $0,%eax
ret
.jge_test.lab:
movl $1,%eax
ret
.type jge_test,@function
.size jge_test,.-jge_test
.globl jge_test
jge_cmp:
movl 8(%esp),%eax
cmpl 4(%esp),%eax
jge .jge_cmp.lab
movl $0,%eax
ret
.jge_cmp.lab:
movl $1,%eax
ret
.type jge_cmp,@function
.size jge_cmp,.-jge_cmp
.globl jge_cmp
jle_test:
movl 8(%esp),%eax
testl 4(%esp),%eax
jle .jle_test.lab
movl $0,%eax
ret
.jle_test.lab:
movl $1,%eax
ret
.type jle_test,@function
.size jle_test,.-jle_test
.globl jle_test
jle_cmp:
movl 8(%esp),%eax
cmpl 4(%esp),%eax
jle .jle_cmp.lab
movl $0,%eax
ret
.jle_cmp.lab:
movl $1,%eax
ret
.type jle_cmp,@function
.size jle_cmp,.-jle_cmp
.globl jle_cmp
jg_test:
movl 8(%esp),%eax
testl 4(%esp),%eax
jg .jg_test.lab
movl $0,%eax
ret
.jg_test.lab:
movl $1,%eax
ret
.type jg_test,@function
.size jg_test,.-jg_test
.globl jg_test
jg_cmp:
movl 8(%esp),%eax
cmpl 4(%esp),%eax
jg .jg_cmp.lab
movl $0,%eax
ret
.jg_cmp.lab:
movl $1,%eax
ret
.type jg_cmp,@function
.size jg_cmp,.-jg_cmp
.globl jg_cmp
