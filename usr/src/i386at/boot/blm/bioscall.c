#ident	"@(#)stand:i386at/boot/blm/bioscall.c	1.1"
#ident	"$Header$"

#include <boot.h>
#include <bioscall.h>

extern int _bios(struct biosregs *rp, void (*)(int));

STATIC int _bioscall(struct biosregs *rp);
STATIC int _bioscall32(struct biosregs *rp);
STATIC char *_biosbuffer(void);
STATIC int _rmcall(struct biosregs *rp, void *orig_code, ulong_t code_size);

STATIC void (*_RMbios)(int);
STATIC struct _p1ext _p1ext;
char *_RMcode;

void
bioscall_start(void)
{
	/*
	 * Extract platform-specific information passed by the media driver.
	 */
	_RMbios = RMp1ext.p1_RMbios;

	/*
	 * Fill in new extension structure, and point p1ext to it. This
	 * allows other BLMs to find this structure. We have to do this
	 * since we can't export symbols.
	 */
	_p1ext.p1_bioscall = _bioscall;
	_p1ext.p1_bioscall32 = _bioscall32;
	_p1ext.p1_biosbuffer = _biosbuffer;
	_p1ext.p1_rmcall = _rmcall;
	p1ext = &_p1ext;

	/*
	 * Media driver left us at least 0x100 bytes of extra space
	 * preceding the block buffer, which we'll use later for
	 * real-address mode calls (in rmcall()).
	 */
	_RMcode = b_driver->d_buffer - 0x100;
}

/*
 * Basic BIOS call front-end. Most of the real work happens in either
 * the register loader, p1ext->p1_bios, or the real-address mode stub,
 * p1ext->p1_RMbios.
 */
int
_bioscall32(struct biosregs *rp)
{
	return _bios(rp, _RMbios);
}

/*
 * 16-bit BIOS call first zeros upper bits of all registers, just in case.
 */
int
_bioscall(struct biosregs *rp)
{
	rp->_eax._16[1] = 0;
	rp->_ebx._16[1] = 0;
	rp->_ecx._16[1] = 0;
	rp->_edx._16[1] = 0;
	rp->_esi._16[1] = 0;
	rp->_edi._16[1] = 0;
	rp->_ebp._16[1] = 0;

	return _bioscall32(rp);
}

/*
 * Return a buffer which can be passed to BIOS calls.
 * There is only one such buffer, and its contents are not guaranteed to
 * stay the same across filesystem accesses.
 */
char *
_biosbuffer(void)
{
	/*
	 * Temporarily steal the block driver's buffer, since we know it's
	 * in real-address mode addressable memory. Hope it's big enough!
	 */
	b_driver->d_blkno = NOBLK;
	return b_driver->d_buffer;
}

/*
 * General real-address mode call.
 *
 * This is passed a pointer to a block of code to be executed in real-address
 * mode, along with register values to be passed to that code.
 *
 * The resulting code will be copied to real-mode addressable memory and
 * executed in real-address mode. All registers are caller-saved. If the
 * code wishes to return to the main program it must use 'iret' rather than
 * a procedure return.
 *
 * The real-mode code will persist after rmcall() returns (in case it hooks
 * interrupt vectors), until another call to rmcall().
 *
 * rp->eax is used by rmcall(), so its passed-in value will be ignored.
 */
STATIC int
_rmcall(struct biosregs *rp, void *orig_code, ulong_t code_size)
{
	extern char rmcall_stub[], rmcall_end[];
	const int stub_size = rmcall_end - rmcall_stub;

	/*
	 * Copy the passed-in code to real-address mode addressable memory.
	 * Prepend the front-end stub code.
	 */
	memcpy(_RMcode, rmcall_stub, stub_size);
	memcpy(_RMcode + stub_size, orig_code, code_size);

	/*
	 * Set up a fake BIOS call in order to get to this code in real-mode.
	 */
	rp->intnum = 0;

	/*
	 * "Borrow" the INT 0 vector, to point to the fake BIOS call. Leave
	 * the old value in eax, to be restored by the front-end stub code.
	 */
	rp->eax = *(ulong_t *)0;
	*(ulong_t *)0 = (ulong_t)_RMcode;

	/*
	 * Do the "BIOS call" to finish the rest in real-address mode.
	 */
	return bioscall32(rp);
}
