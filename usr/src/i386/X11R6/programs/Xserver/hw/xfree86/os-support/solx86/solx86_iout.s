/ $XFree86: $
/
/
/
/
/ $XConsortium: solx86_iout.s /main/3 1995/11/13 06:18:34 kaleb $
/
/ File: solx86_iout.s
/
/ Purpose: Provide inb(), inw(), inl(), outb(), outw(), outl() functions
/          for Solaris x86 using the ProWorks compiler by SunPro
/
/ Author:  Installed into XFree86 SuperProbe by Doug Anson (danson@lgc.com)
/          Portions donated to XFree86 by Steve Dever (Steve.Dever@Eng.Sun.Com)
/
/ Synopsis: (c callable external declarations)
/          extern unsigned char inb(int port);
/          extern unsigned short inw(int port);
/          extern unsigned long inl(int port);
/          extern void outb(int port, unsigned char value);
/          extern void outw(int port, unsigned short value);
/          extern void outl(int port, unsigned long value);
/


.file "solx86_iout.s"
.text

.globl  inb
.globl  inw
.globl  inl
.globl  outb
.globl  outw
.globl  outl

/
/ unsigned char inb(int port);
/
.align	4
inb:
        movl 4(%esp),%edx
        subl %eax,%eax
        inb  (%dx)
		ret
.type	inb,@function
.size	inb,.-inb

/
/ unsigned short inw(int port);
/
.align	4
inw:
        movl 4(%esp),%edx
        subl %eax,%eax
        inw  (%dx)
		ret
.type	inw,@function
.size	inw,.-inw

/
/ unsigned long inl(int port);
/
.align	4
inl:
        movl 4(%esp),%edx
        inl  (%dx)
		ret
.type	inl,@function
.size	inl,.-inl

/
/     void outb(int port, unsigned char value);
/
.align	4
outb:
        movl 4(%esp),%edx
        movl 8(%esp),%eax
        outb (%dx)
		ret
.type	outb,@function
.size	outb,.-outb

/
/     void outw(int port, unsigned short value);
/
.align	4
outw:
        movl 4(%esp),%edx
        movl 8(%esp),%eax
        outw (%dx)
	    ret
.type	outw,@function
.size	outw,.-outw

/
/     void outl(int port, unsigned long value);
/
.align	4
outl:
        movl 4(%esp),%edx
        movl 8(%esp),%eax
        outl (%dx)
	    ret
.type	outl,@function
.size	outl,.-outl
