#ident	"@(#)debugger:libsymbol/common/Locdesc.C	1.7"

#include	<memory.h>
#include	"Locdesc.h"
#include	"ProcObj.h"
#include	"Frame.h"
#include	"Interface.h"

enum LocOp	{
			loc_add,
			loc_deerf2,	// unimplemented
			loc_deref4,
			loc_reg,
			loc_reg_pair,
			loc_basereg,
			loc_offset,
			loc_addr,
		};

Locdesc &
Locdesc::clear()
{
	unsigned char	byte = 0;

	vector.clear().add(&byte,sizeof byte);	// will eventually store length
	return *this;
}

Locdesc &
Locdesc::add()
{
	char	byte = loc_add;

	vector.add(&byte,sizeof byte);
	return *this;
}

Locdesc &
Locdesc::deref4()
{
	char	byte = loc_deref4;

	vector.add(&byte,sizeof byte);
	return *this;
}

Locdesc &
Locdesc::reg( int r )
{
	char	byte = loc_reg;
	short	s    = (short)r;

	vector.add(&byte,sizeof byte).add(&s,sizeof s);
	return *this;
}

Locdesc &
Locdesc::reg_pair( int r1, int r2 )
{
	char	byte = loc_reg_pair;
	short	s1    = (short)r1;
	short	s2    = (short)r2;

	vector.add(&byte,sizeof byte).add(&s1,sizeof s1).add(&s2, sizeof s2);
	return *this;
}


Locdesc &
Locdesc::basereg( int r )
{
	char	byte = loc_basereg;
	short	s    = (short) r;

	vector.add(&byte,sizeof byte).add(&s,sizeof s);
	return *this;
}

Locdesc &
Locdesc::offset( long l )
{
	char	byte = loc_offset;

	vector.add(&byte,sizeof byte).add(&l,sizeof l);
	return *this;
}

Locdesc &
Locdesc::addr( Iaddr l )
{
	char	byte = loc_addr;

	vector.add(&byte,sizeof byte).add(&l,sizeof l);
	return *this;
}

Addrexp
Locdesc::addrexp()
{
	if ( vector.size() > 1 )
	{
		if ( vector.size() > 255 ) {
			printe(ERR_loc_too_complex, E_ERROR);
			return 0;
		}
		(*(unsigned char *)vector.ptr()) =
				vector.size();  // set length byte
		return (Addrexp)vector.ptr();
	}
	else
	{
		return 0;
	}
}

Locdesc &
Locdesc::operator=( Locdesc &l )
{
	int	len;

	vector.clear();
	if ( l.vector.size() > 1 )
	{
		len = l.vector.size();
		vector.add(l.vector.ptr(),len);
	}
	return *this;
}

Locdesc &
Locdesc::operator=( Addrexp aexp )
{
	int	len;

	vector.clear();
	if ( aexp != 0 )
	{
		len = *(unsigned char *)aexp;
		vector.add(aexp,len);
	}
	return *this;
}

Place
Locdesc::place( ProcObj * process, Frame * frame )
{
	Place		lvalue;

	stack.clear();
	if ( vector.size() <= 1 )
	{
		lvalue.null();
	}
	else
	{
		(void)calculate_expr( lvalue, process, frame );
	}
	return lvalue;
}

Place
Locdesc::place( ProcObj * process, Frame * frame, Iaddr addr )
{
	Place		lvalue;

	stack.clear();
	if ( vector.size() <= 1 )
	{
		lvalue.null();
	}
	else
	{
		stack.push( addr );
		(void)calculate_expr( lvalue, process, frame );
	}
	return lvalue;
}

int
Locdesc::place(Place &lvalue, ProcObj *proc, Frame *frame, Iaddr base, int deref)
{
	stack.clear();
	if ( vector.size() <= 1 )
	{
		lvalue.null();
		return 0;
	}
	stack.push( base );
	return calculate_expr( lvalue, proc, frame, deref );
}

int
Locdesc::calculate_expr( Place & lvalue, ProcObj * process, Frame * frame, int deref )
{
	int		len;
	char *		p;
	PlaceMark	kind;
	short		regname1;
	short		regname2;
	unsigned long	word;
	short		hwrd;
	int		retval;

	if ( (len = vector.size()) <= 1 )
	{
		lvalue.null();
		return 0;
	}
	--len;
	kind = pAddress;
	p = (char*)vector.ptr() + 1;
	retval = 1;
	while ( len > 0 )
	{
		switch (*p)
		{
			case loc_add:
				// temp not required; + is commutative...
				stack.push( stack.pop() + stack.pop() );
				--len;
				++p;
				break;
			case loc_deref4:
				if (deref)
				{
					if ( process == 0 || 
						process->read(stack.pop(),
								4,(char*)&word) != 4 )
					{
						kind = pUnknown;
						len = 0;	// break while()
					}
					else
					{
						stack.push( word );
						--len;
						++p;
					}
				}
				else
				{
					if (len > 1)
					{
						// deref not last item
						len = 1; //  break out of while
						retval = 0;
					}
					--len;
					++p;
				}
				break;
			case loc_reg:
				kind = pRegister;
				++p;
				memcpy((char *)&regname1, p,
				       sizeof regname1);
				p += sizeof regname1;
				len -= sizeof regname1 + 1;
				break;
			case loc_reg_pair:
				kind = pRegister_pair;
				++p;
				memcpy((char *)&regname1, p,
				       sizeof regname1);
				p += sizeof regname1;
				memcpy((char *)&regname2, p,
				       sizeof regname2);
				p += sizeof regname2;
				len -= sizeof regname1 + 
					sizeof regname2 + 1;
				break;
			case loc_basereg:
				if ( (frame == 0) && (process == 0) )
				{
					kind = pUnknown;
					len = 0;	// break while()
				}
				else if ( frame == 0 )
				{
					++p;
					memcpy((char *)&hwrd, p,
					       sizeof(hwrd));
					stack.push(process->curframe()->
								getreg(hwrd) );
					p += sizeof hwrd;
					len -= sizeof hwrd + 1;
				}
				else
				{
					++p;
					memcpy((char *)&hwrd, p,
					       sizeof(hwrd));
					stack.push(frame->getreg(hwrd));
					p += sizeof hwrd;
					len -= sizeof hwrd + 1;
				}
				break;
			case loc_addr:
			case loc_offset:
				++p;
				memcpy((char *)&word, p, sizeof(word));
				stack.push( word );
				p += 4;
				len -= 5;
				break;
			default:
				kind = pUnknown;
				printe(ERR_internal, E_ERROR, "calculate_expr",
					 __LINE__);
				len = 0;	// break while()
		}
	}	// end while()
	switch ( kind )
	{
		case pUnknown:
			lvalue.null();
			break;
		case pAddress:
			lvalue.kind = pAddress;
			lvalue.addr = stack.pop();
			break;
		case pRegister:
			lvalue.kind = pRegister;
			lvalue.reg[0] = regname1;
			break;
		case pRegister_pair:
			lvalue.kind = pRegister_pair;
			lvalue.reg[0] = regname1;
			lvalue.reg[1] = regname2;
			break;
		case pDebugvar:
		default:
			printe(ERR_internal, E_ERROR, "calculate_expr", __LINE__);
			break;
	}
	return retval;
}

// adjust each loc_addr operand by adding "base" to it
Locdesc &
Locdesc::adjust( Iaddr base )
{
	int		len;
	char *		p;
	unsigned long	word;

	if ( (len = vector.size()) > 4 )  // min size is 5 for loc_addr,addr
	{
		--len;
		p = (char*)vector.ptr() + 1;
		while ( len > 0 )
		{
			switch ( *p )
			{
				case loc_add:
				case loc_deref4:
					--len;
					++p;
					break;
				case loc_reg:
				case loc_basereg:
					p += sizeof(short) + 1;
					len -= sizeof(short) + 1;
					break;
				case loc_reg_pair:
					p += sizeof(short) * 2 + 1;
					len -= sizeof(short) * 2 + 1;
					break;
				case loc_offset:
					p += sizeof(long) + 1;
					len -= sizeof(long) + 1;
					break;
				case loc_addr:
					++p;
					memcpy((char *)&word, p, sizeof(word));
					word += base;
					memcpy( p, (char *)&word, sizeof(word));
					p += sizeof word;
					len -= sizeof word + 1;
					break;
				default:
					len = 0;
					printe(ERR_debug_entry, E_WARNING);
					break;
			}
		}
	}
	return *this;
}
