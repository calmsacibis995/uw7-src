/* $Copyright: $
 * Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990, 1991
 * Sequent Computer Systems, Inc.   All rights reserved.
 *  
 * This software is furnished under a license and may be used
 * only in accordance with the terms of that license and with the
 * inclusion of the above copyright notice.   This software may not
 * be provided or otherwise made available to, or used by, any
 * other person.  No title to or ownership of the software is
 * hereby transferred.
 */
#ident	"@(#)debugger:libsymbol/common/TYPE.C	1.26"

#include "Fund_type.h"
#include "TYPE.h"
#include "Tag.h"
#include "Machine.h"
#include "Interface.h"
#include "ProcObj.h"
#include "Place.h"
#include "Attribute.h"

// virtual function
TYPE::~TYPE()
{
}

// virtual function
TYPE *
TYPE::clone()
{
	if (!this)
		return 0;
	return new TYPE(*this);
}

TYPE *
TYPE::clone_type()
{
	return new TYPE();
}

TYPE&
TYPE::operator=(Fund_type ftype)
{
	_form = TF_fund;
	ft = ftype;
	return *this;
}

TYPE&
TYPE::operator=(Symbol &symb)
{
	_form = TF_user;
	symbol = symb;
	return *this;
}

int
TYPE::operator==(TYPE& t)
{
	if (_form != t._form)
		return 0;

	if (_form == TF_fund)
	{
		int ret = (this->ft == t.ft);
		if (!ret && (ft == ft_char ||  t.ft == ft_char))
		{
			Fund_type other_ft = (ft == ft_char) ? t.ft : ft;
#if GENERIC_CHAR_SIGNED
			return other_ft == ft_schar;
#else
			return other_ft == ft_schar;
#endif
		}
		return ret;
	}

	if( this->symbol == t.symbol )
		return 1;

	TYPE thisDrefType;
	TYPE tDrefType;
	if( this->get_base_type(&thisDrefType) && t.get_base_type(&tDrefType) )
		return thisDrefType==tDrefType;

	return 0;
}

// overridden by language-specific routines
int
TYPE::is_assignable(TYPE *t)
{
	return *this == *t;
}

int
TYPE::fund_type(Fund_type& ftype) const // 'read' fundamental type.
{
	if (_form == TF_fund) 
	{
		ftype = ft;
		return 1;
	}
	ftype = ft_none;
	return 0;
}

int
TYPE::user_type(Symbol& symb) const // 'read' user defined type.
{
	if (_form == TF_user && !symbol.isnull()) 
	{
		symb = symbol;
		return 1;
	}
	symb.null();
	return 0;
}

int
TYPE::is_C_type(a_type *&ptr) const // 'read' user defined type.
{
	if (_form == TF_C_lang && c_type) 
	{
		ptr = c_type;
		return 1;
	}
	ptr = 0;
	return 0;
}

int
TYPE::size()
{
	if (isnull() || form() == TF_user) 
		return 0; // don't know - language-specific derived class should
				// handle form == TF_user

	switch (ft) 
	{ 
	case ft_char:
	case ft_schar:
	case ft_uchar:
	case ft_boolean:
		return CHAR_SIZE;
	case ft_short:
	case ft_sshort:
	case ft_ushort:
		return SHORT_SIZE;
	case ft_int:
	case ft_sint:
	case ft_uint:
		return INT_SIZE;
	case ft_long:
	case ft_slong:
	case ft_ulong:
		return LONG_SIZE;
#if LONG_LONG
	case ft_long_long:
	case ft_slong_long:
	case ft_ulong_long:
		return LONG_LONG_SIZE;
#endif
	case ft_pointer:
	case ft_string:
		return PTR_SIZE;
	case ft_sfloat:
		return SFLOAT_SIZE;
	case ft_lfloat:
		return LFLOAT_SIZE;
	case ft_xfloat:
		return XFLOAT_SIZE;
	case ft_none:
	default:
		return 0;
	}
}

// pure virtual
int
TYPE::print(Buffer *, ProcObj *)
{
	return 0;
}

int
TYPE::get_base_type(TYPE *dtype)	// array or modified type
{
	if (form() != TF_user) 
		return 0;

	Attr_name	attr;

	switch (symbol.tag()) 
	{
	case t_pointertype:
	case t_consttype:
	case t_volatiletype:
	case t_reftype:
		attr = an_basetype;
		break;
	case t_arraytype:
		attr = an_elemtype;
		break;
	default:
		return 0;
	}
	return symbol.type(dtype, attr, 0);
}

#if DEBUG
#include "Language.h"
#include <stdio.h>
static char *ftStr[] =
{
	"ft_none",
	"ft_char",
	"ft_schar",
	"ft_uchar",
	"ft_short",
	"ft_sshort",
	"ft_ushort",
	"ft_int",
	"ft_sint",
	"ft_uint",
	"ft_long",
	"ft_slong",
	"ft_ulong",
	"ft_pointer",
	"ft_sfloat",
	"ft_lfloat",
	"ft_xfloat",
	"ft_scomplex",
	"ft_lcomplex",
	"ft_unused",
	"ft_void",
	"ft_boolean",
	"ft_xcomplex",
	"ft_label",
#if LONG_LONG
	"ft_long_long",
	"ft_slong_long",
	"ft_ulong_long",
#endif
	"ft_string"
};

#undef DEFTAG
#define DEFTAG(x,y)	y,
static char *tagstr[] =
{
#include "Tag1.h"
};

static char *Attrform[] =
{
	"af_none",
	"af_tag",				 // value.tag
	"af_int",				 // value.word
	"af_locdesc",			 // value.loc
	"af_stringndx",		   // value.name
	"af_coffrecord",		  // value.word
	"af_coffline",			// value.word
	"af_coffpc",			  // unused
	"af_fundamental_type",	// value.fund_type
	"af_symndx",			  // unused
	"af_reg",				 // unused
	"af_addr",				// value.addr
	"af_local",			   // unused
	"af_visibility",		  // unused
	"af_lineinfo",			// value.lineinfo
	"af_attrlist",			// unused
	"af_cofffile",			// value.word
	"af_symbol",			  // value.symbol
	"af_dwarfoffs",		   // value.word
	"af_dwarfline",		   // value.word
	"af_dwarf2offs",		   // value.word
	"af_dwarf2file",		   // value.word
	"af_dwarf2line",		   // value.word
	"af_elfoffs",			 // value.word
	"af_language",			// value.language
	"af_file_table",		// value.file_table
};

static char *Attrname[] =
{
	"an_nomore",	
	"an_tag",		
	"an_name",
	"an_mangledname",	
	"an_child",	
	"an_sibling",	
	"an_parent",	
	"an_count",	
	"an_type",	
	"an_elemtype",	
	"an_elemspan",	
	"an_subscrtype",	
	"an_lobound",	
	"an_hibound",	
	"an_basetype",	
	"an_resulttype",	
	"an_argtype",	
	"an_bytesize",	
	"an_bitsize",	
	"an_bitoffs",	
	"an_litvalue",	
	"an_stringlen",	
	"an_lineinfo",	
	"an_location",	
	"an_lopc",	
	"an_hipc",	
	"an_visibility",	
	"an_scansize",
	"an_language",
	"an_assumed_type",
	"an_subrange_count",
	"an_prototyped",
	"an_comp_dir",
	"an_file_table",
	"an_decl_file",
	"an_decl_line",
	"an_specification",
	"an_vtbl_slot",
	"an_virtuality",
	"an_artificial",
	"an_containing_type",
	"an_const_value",
	"an_definition",
	"an_external",
	"an_abstract",
};

static void
dumpSymType(Attribute *attrPtr, int indent)
{
	int	i,
		j,
	   	attrCnt;

	if(attrPtr == 0)
	{
		for(j=0; j<indent; j++)
			fprintf(stderr,"  ");

		fprintf(stderr,"Attribute list empty\n");
		return;
	}
	
	for(j=0; j<indent; j++)
		fprintf(stderr,"  ");
	fprintf(stderr, "Attrlist @ %x\n", attrPtr);

	for(attrCnt=attrPtr->value.word, i=0; i < attrCnt; i++, attrPtr++)
	{
		for(j=0; j<indent; j++)
			fprintf(stderr,"  ");

		fprintf(stderr, "attr.name=%s, attr.form=%s, ", 
			Attrname[attrPtr->name], Attrform[attrPtr->form] );

		switch(attrPtr->form)
		{
		case af_tag:				 // value.tag
			fprintf(stderr,"attr.value.tag = %s", 
					tagstr[attrPtr->value.tag]);
			break;
		case af_locdesc:			 // value.loc
			fprintf(stderr,"attr.value.loc= <dump not implemented>");
			break;
		case af_stringndx:		   // value.name
			fprintf(stderr,"attr.value.name = %s",
							 attrPtr->value.name);
			break;
		case af_fundamental_type:	// value.fund_type
			fprintf(stderr,"attr.value.fund_type = %s", 
					ftStr[attrPtr->value.fund_type]);
			break;
		case af_addr:				// value.addr
			fprintf(stderr,"attr.value.addr = %#x", 
						attrPtr->value.addr);
			break;
		case af_lineinfo:			// value.lineinfo
			fprintf(stderr,
				"attr.value.lineinfo = <dump not implemented>");
			break;
		case af_symbol:			  // value.symbol
			if( attrPtr->name==an_type ||
				attrPtr->name==an_elemtype ||
				attrPtr->name==an_basetype ||
				attrPtr->name==an_resulttype )
			{
				fprintf(stderr,"attr.value.symbol=\n");
				dumpSymType(attrPtr->value.symbol, indent+1);
			}
			else
			{
				fprintf(stderr,"attr.value.symbol = %#x",
							 attrPtr->value.symbol);
			}
			break;
		case af_coffrecord:		  // value.word
		case af_coffline:			// value.word
		case af_cofffile:			// value.word
		case af_dwarfoffs:		   // value.word
		case af_dwarfline:		   // value.word
		case af_dwarf2offs:		   // value.word
		case af_dwarf2file:		   // value.word
		case af_dwarf2line:		   // value.word
		case af_elfoffs:			 // value.word
			fprintf(stderr,"attr.value.word = %#x",
							attrPtr->value.word);
			break;
		case af_int:				 // value.word
			fprintf(stderr,"attr.value.word = %#x (%d)",
					attrPtr->value.word, attrPtr->value.word);
			break;
		case af_none:
		case af_coffpc:			  // unused
		case af_symndx:			  // unused
		case af_reg:				 // unused
		case af_local:			   // unused
		case af_visibility:		  // unused
		case af_attrlist:			// unused
			fprintf(stderr,"attr.value = unused");
			break;
		case af_language:
			fprintf(stderr,"attr.value.language = %s",
				language_name(attrPtr->value.language));
			break;
		case af_file_table:
			fprintf(stderr,"attr.value.file_table = %#x",
							attrPtr->value.file_table);
		}
		fprintf(stderr,"\n");
	}
}

void
TYPE::dumpTYPE()
{
	if(_form == TF_fund)
		fprintf(stderr,"fund_type=%s\n", ftStr[ft]);
	else
	{
		fprintf(stderr,"User_type=\n");
		dumpSymType(symbol.attribute(an_count), 1);
	}
}

// dump all siblings and children of root
void
dumpAttr( Attribute *root, int indent)
{
	int	i,
		j,
	   	attrCnt;
	Attribute *attrPtr = root;
	Attribute *siblingAttr = 0;
	Attribute *childAttr = 0;

	if(attrPtr == 0)
	{
		for(j=0; j<indent; j++)
			fprintf(stderr,"  ");

		fprintf(stderr,"Attribute list empty\n");
		return;
	}
	
	for(j=0; j<indent; j++)
		fprintf(stderr,"  ");
	fprintf(stderr, "Attrlist @ %x\n", attrPtr);

	for(attrCnt=attrPtr->value.word, i=0; i < attrCnt; i++, attrPtr++)
	{
		for(j=0; j<indent; j++)
			fprintf(stderr,"  ");

		fprintf(stderr, "attr.name=%s, attr.form=%s, ", 
			Attrname[attrPtr->name], Attrform[attrPtr->form] );

		switch(attrPtr->form)
		{
		case af_tag:				 // value.tag
			fprintf(stderr,"attr.value.tag = %s", 
					tagstr[attrPtr->value.tag]);
			break;
		case af_locdesc:			 // value.loc
			fprintf(stderr,"attr.value.loc = <dump not implemented>");
			break;
		case af_stringndx:		   // value.name
			fprintf(stderr,"attr.value.name = %s",
							 attrPtr->value.name);
			break;
		case af_fundamental_type:	// value.fund_type
			fprintf(stderr,"attr.value.fund_type = %s", 
					ftStr[attrPtr->value.fund_type]);
			break;
		case af_addr:				// value.addr
			fprintf(stderr,"attr.value.addr = %#x", 
					attrPtr->value.addr);
			break;
		case af_lineinfo:			// value.lineinfo
			fprintf(stderr,
				"attr.value.lineinfo = <dump not implemented>");
			break;
		case af_symbol:			  // value.symbol
			if( attrPtr->name==an_parent )
			{
				fprintf(stderr,"attr.value.symbol = %#x", 
							 attrPtr->value.symbol);
			}
			else if( attrPtr->name==an_child )
			{
				fprintf(stderr,"attr.value.symbol = %#x (below)", 
							 attrPtr->value.symbol);
				childAttr = attrPtr->value.symbol;
			}
			else if( attrPtr->name==an_sibling )
			{
				fprintf(stderr,"attr.value.symbol = %#x (below)",
							 attrPtr->value.symbol);
				siblingAttr = attrPtr->value.symbol;
			}
			else
			{
				fprintf(stderr,"attr.value.symbol = \n");
				dumpAttr(attrPtr->value.symbol, indent+1);
			}
			break;
		case af_coffrecord:		  // value.word
		case af_coffline:			// value.word
		case af_cofffile:			// value.word
		case af_dwarfoffs:		   // value.word
		case af_dwarfline:		   // value.word
		case af_dwarf2offs:		   // value.word
		case af_dwarf2line:		   // value.word
		case af_elfoffs:			 // value.word
		// unused (so they say) dump as long word, value may be meaningless
		case af_none:
		case af_coffpc:			  // unused
		case af_symndx:			  // unused
		case af_reg:				 // unused
		case af_local:			   // unused
		case af_visibility:		  // unused
		case af_attrlist:			// unused
			fprintf(stderr,"attr.value.word = %#x",
							attrPtr->value.word);
			break;
		case af_int:				 // value.word
			fprintf(stderr,"attr.value.word = %#x (%d)",
					attrPtr->value.word, attrPtr->value.word);
			break;
		case af_language:
			fprintf(stderr,"attr.value.language = %s",
				language_name(attrPtr->value.language));
		case af_file_table:
			fprintf(stderr,"attr.value.file_table = %#x",
							attrPtr->value.file_table);
		}
		fprintf(stderr,"\n");
	}
	if( childAttr )
	{
		fprintf(stderr,"\n");
		if( childAttr == root )
		{
			fprintf(stderr,"LOOP: child has same address as root\n");
			return;
		}
		dumpAttr(childAttr, indent+1);
	}
	if( siblingAttr )
	{
		fprintf(stderr,"\n");
		if( siblingAttr == root )
		{
			fprintf(stderr,"LOOP: sibling has same address as root\n");
			return;
		}
		dumpAttr(childAttr, indent+1);
	}
}

#endif
