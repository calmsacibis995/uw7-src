#ident	"@(#)kern-i386:util/bitmasks.c	1.6"
#ident	"$Header$"

/*
 * Operations on arbitrary size bit arrays.
 * A bit array is a vector of 1 or more uint_t elements.
 * The user of the package is responsible for range checks and keeping
 * track of sizes.
 *
 * See util/bitmasks.h for details on the operations, many of which are
 * implemented directly there.
 */

#include <util/bitmasks.h>
#include <util/debug.h>
#include <util/types.h>

/*
 * int
 * BITMASK1_FFS(const uint_t bits[])
 *	"Find First Set" for one-word bit array.
 *
 * Calling/Exit State:
 *	Returns the bit number of the lowest set bit; if none, returns -1.
 *
 * Description:
 *	For i386, we implement this with a binary search.
 *	On the 80386, it's a toss-up whether this is better than using the
 *	special bit-scan instructions; each is faster under some circumstances,
 *	and they average out about the same.  On the 80486, this method is
 *	a definite win.
 */
int
BITMASK1_FFS(const uint_t bits[])
{
	register uint_t word;

	if ((word = bits[0]) == 0)
		return -1;

	if (word & 0x0000ffff) {
		if (word & 0x000000ff) {
			if (word & 0x0000000f) {
				if (word & 0x00000003) {
					if (word & 0x00000001) {
						return 0;
					} else {
						return 1;
					}
				} else if (word & 0x00000004) {
					return 2;
				} else {
					return 3;
				}
			} else if (word & 0x00000030) {
				if (word & 0x00000010) {
					return 4;
				} else {
					return 5;
				}
			} else if (word & 0x00000040) {
				return 6;
			} else {
				return 7;
			}
		} else if (word & 0x00000f00) {
			if (word & 0x00000300) {
				if (word & 0x00000100) {
					return 8;
				} else {
					return 9;
				}
			} else if (word & 0x00000400) {
				return 10;
			} else {
				return 11;
			}
		} else if (word & 0x00003000) {
			if (word & 0x00001000) {
				return 12;
			} else {
				return 13;
			}
		} else if (word & 0x00004000) {
			return 14;
		} else {
			return 15;
		}
	} else if (word & 0x00ff0000) {
		if (word & 0x000f0000) {
			if (word & 0x00030000) {
				if (word & 0x00010000) {
					return 16;
				} else {
					return 17;
				}
			} else if (word & 0x00040000) {
				return 18;
			} else {
				return 19;
			}
		} else if (word & 0x00300000) {
			if (word & 0x00100000) {
				return 20;
			} else {
				return 21;
			}
		} else if (word & 0x00400000) {
			return 22;
		} else {
			return 23;
		}
	} else if (word & 0x0f000000) {
		if (word & 0x03000000) {
			if (word & 0x01000000) {
				return 24;
			} else {
				return 25;
			}
		} else if (word & 0x04000000) {
			return 26;
		} else {
			return 27;
		}
	} else if (word & 0x30000000) {
		if (word & 0x10000000) {
			return 28;
		} else {
			return 29;
		}
	} else if (word & 0x40000000) {
		return 30;
	} else {
		return 31;
	}
}

/*
 * int
 * BITMASKN_FFS(const uint_t bits[], uint_t n)
 *	"Find First Set" for arbitrary-length bit array.
 *
 * Calling/Exit State:
 *	Returns the bit number of the lowest set bit; if none, returns -1.
 */
int
BITMASKN_FFS(const uint_t bits[], uint_t n)
{
	register uint_t i;

	for (i = 0; bits[i] == 0;) {
		if (++i == n)
			return -1;
	}

	if ((n = BITMASK1_FFS(&bits[i])) == -1)
		return -1;
	return i * NBITPW + n;
}

/*
 * int
 * BITMASK1_FLS(const uint_t bits[])
 *	"Find Last Set" for one-word bit array.
 *
 * Calling/Exit State:
 *	Returns the bit number of the highest set bit; if none, returns -1.
 *
 * Description:
 *	For i386, we implement this with a binary search.
 *	On the 80386, it's a toss-up whether this is better than using the
 *	special bit-scan instructions; each is faster under some circumstances,
 *	and they average out about the same.  On the 80486, this method is
 *	a definite win.
 */
int
BITMASK1_FLS(const uint_t bits[])
{
	register uint_t word;

	if ((word = bits[0]) == 0)
		return -1;

	if (word & 0xffff0000) {
		if (word & 0xff000000) {
			if (word & 0xf0000000) {
				if (word & 0xc0000000) {
					if (word & 0x80000000) {
						return 31;
					} else {
						return 30;
					}
				} else if (word & 0x20000000) {
					return 29;
				} else {
					return 28;
				}
			} else if (word & 0x0c000000) {
				if (word & 0x08000000) {
					return 27;
				} else {
					return 26;
				}
			} else if (word & 0x02000000) {
				return 25;
			} else {
				return 24;
			}
		} else if (word & 0x00f00000) {
			if (word & 0x00c00000) {
				if (word & 0x00800000) {
					return 23;
				} else {
					return 22;
				}
			} else if (word & 0x00200000) {
				return 21;
			} else {
				return 20;
			}
		} else if (word & 0x000c0000) {
			if (word & 0x00080000) {
				return 19;
			} else {
				return 18;
			}
		} else if (word & 0x00020000) {
			return 17;
		} else {
			return 16;
		}
	} else if (word & 0x0000ff00) {
		if (word & 0x0000f000) {
			if (word & 0x0000c000) {
				if (word & 0x00008000) {
					return 15;
				} else {
					return 14;
				}
			} else if (word & 0x00002000) {
				return 13;
			} else {
				return 12;
			}
		} else if (word & 0x00000c00) {
			if (word & 0x00000800) {
				return 11;
			} else {
				return 10;
			}
		} else if (word & 0x00000200) {
			return 9;
		} else {
			return 8;
		}
	} else if (word & 0x000000f0) {
		if (word & 0x000000c0) {
			if (word & 0x00000080) {
				return 7;
			} else {
				return 6;
			}
		} else if (word & 0x00000020) {
			return 5;
		} else {
			return 4;
		}
	} else if (word & 0x0000000c) {
		if (word & 0x00000008) {
			return 3;
		} else {
			return 2;
		}
	} else if (word & 0x00000002) {
		return 1;
	} else {
		return 0;
	}
}

/*
 * int
 * BITMASKN_FLS(const uint_t bits[], uint_t n)
 *	"Find Last Set" for arbitrary-length bit array.
 *
 * Calling/Exit State:
 *	Returns the bit number of the highest set bit; if none, returns -1.
 */
int
BITMASKN_FLS(const uint_t bits[], uint_t n)
{
	register uint_t i = n;

	do {
		if (i == 0)
			return -1;
	} while (bits[--i] == 0);

	if ((n = BITMASK1_FLS(&bits[i])) == -1)
		return -1;
	return i * NBITPW + n;
}

/*
 * int
 * BITMASK1_FFSCLR(uint_t bits[])
 *	"Find First Set and Clear" for one-word bit array.
 *
 * Calling/Exit State:
 *	Returns the bit number of the lowest set bit, after clearing that bit;
 *	if none were set, returns -1.
 *
 * Description:
 *	For i386, we implement this with a binary search.
 *	On the 80386, it's a toss-up whether this is better than using the
 *	special bit-scan instructions; each is faster under some circumstances,
 *	and they average out about the same.  On the 80486, this method is
 *	a definite win.
 */
int
BITMASK1_FFSCLR(uint_t bits[])
{
	register uint_t word;

	if ((word = bits[0]) == 0)
		return -1;

	if (word & 0x0000ffff) {
		if (word & 0x000000ff) {
			if (word & 0x0000000f) {
				if (word & 0x00000003) {
					if (word & 0x00000001) {
						bits[0] &= ~0x00000001;
						return 0;
					} else {
						bits[0] &= ~0x00000002;
						return 1;
					}
				} else if (word & 0x00000004) {
					bits[0] &= ~0x00000004;
					return 2;
				} else {
					bits[0] &= ~0x00000008;
					return 3;
				}
			} else if (word & 0x00000030) {
				if (word & 0x00000010) {
					bits[0] &= ~0x00000010;
					return 4;
				} else {
					bits[0] &= ~0x00000020;
					return 5;
				}
			} else if (word & 0x00000040) {
				bits[0] &= ~0x00000040;
				return 6;
			} else {
				bits[0] &= ~0x00000080;
				return 7;
			}
		} else if (word & 0x00000f00) {
			if (word & 0x00000300) {
				if (word & 0x00000100) {
					bits[0] &= ~0x00000100;
					return 8;
				} else {
					bits[0] &= ~0x00000200;
					return 9;
				}
			} else if (word & 0x00000400) {
				bits[0] &= ~0x00000400;
				return 10;
			} else {
				bits[0] &= ~0x00000800;
				return 11;
			}
		} else if (word & 0x00003000) {
			if (word & 0x00001000) {
				bits[0] &= ~0x00001000;
				return 12;
			} else {
				bits[0] &= ~0x00002000;
				return 13;
			}
		} else if (word & 0x00004000) {
			bits[0] &= ~0x00004000;
			return 14;
		} else {
			bits[0] &= ~0x00008000;
			return 15;
		}
	} else if (word & 0x00ff0000) {
		if (word & 0x000f0000) {
			if (word & 0x00030000) {
				if (word & 0x00010000) {
					bits[0] &= ~0x00010000;
					return 16;
				} else {
					bits[0] &= ~0x00020000;
					return 17;
				}
			} else if (word & 0x00040000) {
				bits[0] &= ~0x00040000;
				return 18;
			} else {
				bits[0] &= ~0x00080000;
				return 19;
			}
		} else if (word & 0x00300000) {
			if (word & 0x00100000) {
				bits[0] &= ~0x00100000;
				return 20;
			} else {
				bits[0] &= ~0x00200000;
				return 21;
			}
		} else if (word & 0x00400000) {
			bits[0] &= ~0x00400000;
			return 22;
		} else {
			bits[0] &= ~0x00800000;
			return 23;
		}
	} else if (word & 0x0f000000) {
		if (word & 0x03000000) {
			if (word & 0x01000000) {
				bits[0] &= ~0x01000000;
				return 24;
			} else {
				bits[0] &= ~0x02000000;
				return 25;
			}
		} else if (word & 0x04000000) {
			bits[0] &= ~0x04000000;
			return 26;
		} else {
			bits[0] &= ~0x08000000;
			return 27;
		}
	} else if (word & 0x30000000) {
		if (word & 0x10000000) {
			bits[0] &= ~0x10000000;
			return 28;
		} else {
			bits[0] &= ~0x20000000;
			return 29;
		}
	} else if (word & 0x40000000) {
		bits[0] &= ~0x40000000;
		return 30;
	} else {
		bits[0] &= ~0x80000000;
		return 31;
	}
}

/*
 * int
 * BITMASKN_FFSCLR(uint_t bits[], uint_t n)
 *	"Find First Set and Clear" for arbitrary-length bit array.
 *
 * Calling/Exit State:
 *	Returns the bit number of the lowest set bit, after clearing that bit;
 *	if none were set, returns -1.
 */
int
BITMASKN_FFSCLR(uint_t bits[], uint_t n)
{
	register uint_t i;

	for (i = 0; bits[i] == 0;) {
		if (++i == n)
			return -1;
	}

	if ((n = BITMASK1_FFSCLR(&bits[i])) == -1)
		return -1;
	return i * NBITPW + n;
}

/*
 * int
 * BITMASK1_FLSCLR(uint_t bits[])
 *	"Find Last Set and Clear" for one-word bit array.
 *
 * Calling/Exit State:
 *	Returns the bit number of the highest set bit after clearing that bit;
 *	if none were set, returns -1.
 *
 * Description:
 *	For i386, we implement this with a binary search.
 *	On the 80386, it's a toss-up whether this is better than using the
 *	special bit-scan instructions; each is faster under some circumstances,
 *	and they average out about the same.  On the 80486, this method is
 *	a definite win.
 */
int
BITMASK1_FLSCLR(uint_t bits[])
{
	register uint_t word;

	if ((word = bits[0]) == 0)
		return -1;

	if (word & 0xffff0000) {
		if (word & 0xff000000) {
			if (word & 0xf0000000) {
				if (word & 0xc0000000) {
					if (word & 0x80000000) {
						bits[0] &= ~0x80000000;
						return 31;
					} else {
						bits[0] &= ~0x40000000;
						return 30;
					}
				} else if (word & 0x20000000) {
					bits[0] &= ~0x20000000;
					return 29;
				} else {
					bits[0] &= ~0x10000000;
					return 28;
				}
			} else if (word & 0x0c000000) {
				if (word & 0x08000000) {
					bits[0] &= ~0x08000000;
					return 27;
				} else {
					bits[0] &= ~0x04000000;
					return 26;
				}
			} else if (word & 0x02000000) {
				bits[0] &= ~0x02000000;
				return 25;
			} else {
				bits[0] &= ~0x01000000;
				return 24;
			}
		} else if (word & 0x00f00000) {
			if (word & 0x00c00000) {
				if (word & 0x00800000) {
					bits[0] &= ~0x00800000;
					return 23;
				} else {
					bits[0] &= ~0x00400000;
					return 22;
				}
			} else if (word & 0x00200000) {
				bits[0] &= ~0x00200000;
				return 21;
			} else {
				bits[0] &= ~0x00100000;
				return 20;
			}
		} else if (word & 0x000c0000) {
			if (word & 0x00080000) {
				bits[0] &= ~0x00080000;
				return 19;
			} else {
				bits[0] &= ~0x00040000;
				return 18;
			}
		} else if (word & 0x00020000) {
			bits[0] &= ~0x00020000;
			return 17;
		} else {
			bits[0] &= ~0x00010000;
			return 16;
		}
	} else if (word & 0x0000ff00) {
		if (word & 0x0000f000) {
			if (word & 0x0000c000) {
				if (word & 0x00008000) {
					bits[0] &= ~0x00008000;
					return 15;
				} else {
					bits[0] &= ~0x00004000;
					return 14;
				}
			} else if (word & 0x00002000) {
				bits[0] &= ~0x00002000;
				return 13;
			} else {
				bits[0] &= ~0x00001000;
				return 12;
			}
		} else if (word & 0x00000c00) {
			if (word & 0x00000800) {
				bits[0] &= ~0x00000800;
				return 11;
			} else {
				bits[0] &= ~0x00000400;
				return 10;
			}
		} else if (word & 0x00000200) {
			bits[0] &= ~0x00000200;
			return 9;
		} else {
			bits[0] &= ~0x00000100;
			return 8;
		}
	} else if (word & 0x000000f0) {
		if (word & 0x000000c0) {
			if (word & 0x00000080) {
				bits[0] &= ~0x00000080;
				return 7;
			} else {
				bits[0] &= ~0x00000040;
				return 6;
			}
		} else if (word & 0x00000020) {
			bits[0] &= ~0x00000020;
			return 5;
		} else {
			bits[0] &= ~0x00000010;
			return 4;
		}
	} else if (word & 0x0000000c) {
		if (word & 0x00000008) {
			bits[0] &= ~0x00000008;
			return 3;
		} else {
			bits[0] &= ~0x00000004;
			return 2;
		}
	} else if (word & 0x00000002) {
		bits[0] &= ~0x00000002;
		return 1;
	} else {
		bits[0] &= ~0x00000001;
		return 0;
	}
}

/*
 * int
 * BITMASKN_FLSCLR(uint_t bits[], uint_t n)
 *	"Find Last Set and Clear" for arbitrary-length bit array.
 *
 * Calling/Exit State:
 *	Returns the bit number of the highest set bit after clearing that bit;
 *	if none were set, returns -1.
 */
int
BITMASKN_FLSCLR(uint_t bits[], uint_t n)
{
	register uint_t i = n;

	while (bits[--i] == 0) {
		if (i == 0)
			return -1;
	}

	if ((n = BITMASK1_FLSCLR(&bits[i])) == -1)
		return -1;
	return i * NBITPW + n;
}

/*
 * int
 * BITMASK1_FFC(const uint_t bits[])
 *	"Find First Clear" for one-word bit array.
 *
 * Calling/Exit State:
 *	Returns the bit number of the lowest clear bit; if none, returns -1.
 */
int
BITMASK1_FFC(const uint_t bits[])
{
	uint_t word = ~bits[0];
	return BITMASK1_FFS(&word);
}

/*
 * int
 * BITMASKN_FFC(const uint_t bits[], uint_t n)
 *	"Find First Clear" for arbitrary-length bit array.
 *
 * Calling/Exit State:
 *	Returns the bit number of the lowest clear bit; if none, returns -1.
 */
int
BITMASKN_FFC(const uint_t bits[], uint_t n)
{
	register uint_t i;

	for (i = 0; bits[i] == ~0U;) {
		if (++i == n)
			return -1;
	}

	if ((n = BITMASK1_FFC(&bits[i])) == -1)
		return -1;
	return i * NBITPW + n;
}

/*
 * int
 * BITMASK1_FLC(const uint_t bits[])
 *	"Find Last Clear" for one-word bit array.
 *
 * Calling/Exit State:
 *	Returns the bit number of the highest clear bit; if none, returns -1.
 */
int
BITMASK1_FLC(const uint_t bits[])
{
	uint_t word = ~bits[0];
	return BITMASK1_FLS(&word);
}

/*
 * int
 * BITMASKN_FLC(const uint_t bits[], uint_t n)
 *	"Find Last Clear" for arbitrary-length bit array.
 *
 * Calling/Exit State:
 *	Returns the bit number of the highest clear bit; if none, returns -1.
 */
int
BITMASKN_FLC(const uint_t bits[], uint_t n)
{
	register uint_t i = n;

	while (bits[--i] == ~0U) {
		if (i == 0)
			return -1;
	}

	if ((n = BITMASK1_FLC(&bits[i])) == -1)
		return -1;
	return i * NBITPW + n;
}

/*
 * int
 * BITMASK1_FFCSET(uint_t bits[])
 *	"Find First Clear and Set" for one-word bit array.
 *
 * Calling/Exit State:
 *	Returns the bit number of the lowest clear bit, after setting that bit;
 *	if none were clear, returns -1.
 */
int
BITMASK1_FFCSET(uint_t bits[])
{
	uint_t word = ~bits[0];
	register int bitno;

	bitno = BITMASK1_FFSCLR(&word);
	bits[0] = ~word;
	return bitno;
}

/*
 * int
 * BITMASKN_FFCSET(uint_t bits[], uint_t n)
 *	"Find First Clear and Set" for arbitrary-length bit array.
 *
 * Calling/Exit State:
 *	Returns the bit number of the lowest clear bit, after setting that bit;
 *	if none were clear, returns -1.
 */
int
BITMASKN_FFCSET(uint_t bits[], uint_t n)
{
	register uint_t i;

	for (i = 0; bits[i] == ~0U;) {
		if (++i == n)
			return -1;
	}

	if ((n = BITMASK1_FFCSET(&bits[i])) == -1)
		return -1;
	return i * NBITPW + n;
}

/*
 * int
 * BITMASK1_FLCSET(uint_t bits[])
 *	"Find Last Clear and Set" for one-word bit array.
 *
 * Calling/Exit State:
 *	Returns the bit number of the highest clear bit after setting that bit;
 *	if none were clear, returns -1.
 */
int
BITMASK1_FLCSET(uint_t bits[])
{
	uint_t word = ~bits[0];
	register int bitno;

	bitno = BITMASK1_FLSCLR(&word);
	bits[0] = ~word;
	return bitno;
}

/*
 * int
 * BITMASKN_FLCSET(uint_t bits[], uint_t n)
 *	"Find Last Clear and Set" for arbitrary-length bit array.
 *
 * Calling/Exit State:
 *	Returns the bit number of the highest clear bit after setting that bit;
 *	if none were clear, returns -1.
 */
int
BITMASKN_FLCSET(uint_t bits[], uint_t n)
{
	register uint_t i = n;

	while (bits[--i] == ~0U) {
		if (i == 0)
			return -1;
	}

	if ((n = BITMASK1_FLCSET(&bits[i])) == -1)
		return -1;
	return i * NBITPW + n;
}

/*
 * int
 * BITMASKN_ALLOCRANGE(uint_t bits[], uint_t totalbits, uint_t nbits)
 *	Allocate a range from a bitmap.
 *
 * Calling/Exit State:
 *	Finds the first run of "nbits" consecutive clear bits, sets them all,
 *	and returns the bit number of the first one.  If no such run is found,
 *	returns -1.  Totalbits is the total number of bits in the map.
 */
int
BITMASKN_ALLOCRANGE(uint_t bits[], uint_t totalbits, uint_t nbits)
{
	int first_bit, bitno, end_bit;

	ASSERT(totalbits != 0);
	ASSERT(nbits != 0);
	ASSERT(nbits <= totalbits);

	if ((first_bit = BITMASKN_FFC(bits, BITMASK_NWORDS(totalbits))) == -1 ||
	    first_bit > totalbits - nbits)
		return -1;

	for (;;) {
		end_bit = (bitno = first_bit) + nbits;
		do {
			if (++bitno == end_bit) {
				do {
					--bitno;
					BITMASKN_SET1(bits, bitno);
				} while (bitno != first_bit);
				return first_bit;
			}
		} while (!BITMASKN_TEST1(bits, bitno));
		do {
			if (++bitno > totalbits - nbits)
				return -1;
		} while (BITMASKN_TEST1(bits, bitno));
		first_bit = bitno;
	}
}

/*
 * void
 * BITMASKN_FREERANGE(uint_t bits[], int bitno, uint_t nbits)
 *	"Free" a range of bits.
 *
 * Calling/Exit State:
 *	Clears nbits bits starting with bit number bitno.
 */
void
BITMASKN_FREERANGE(uint_t bits[], int bitno, uint_t nbits)
{
	ASSERT(nbits != 0);
	do {
		ASSERT(BITMASKN_TEST1(bits, bitno));
		BITMASKN_CLR1(bits, bitno);
		++bitno;
	} while (--nbits != 0);
}
