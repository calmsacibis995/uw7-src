#ident	"@(#)debugger:libsymbol/common/Protorec.C	1.1"
#include	"Protorec.h"
#include	<string.h>

Protorec::Protorec()
{
	Attribute	attribute;

	attribute.name = an_count;
	attribute.form = af_int;
	attribute.value.word = 0;
	vector.add(&attribute,sizeof(Attribute));
	count = 1;
}

Protorec &
Protorec::add_attr(Attr_name name, Attr_form form, long word )
{
	Attribute	attribute;

	attribute.name = name;
	attribute.form = form;
	attribute.value.word = word;
	vector.add(&attribute,sizeof(Attribute));
	count++ ;
	return * this;
}

Protorec &
Protorec::add_attr(Attr_name name, Attr_form form, void * ptr )
{
	Attribute	attribute;

	attribute.name = name;
	attribute.form = form;
	attribute.value.word = (long)ptr;
	vector.add(&attribute,sizeof(Attribute));
	count++ ;
	return * this;
}

Protorec &
Protorec::add_attr(Attr_name name, Attr_form form, const Attr_value & value )
{
	Attribute	attribute;

	attribute.name = name;
	attribute.form = form;
	attribute.value = value;
	vector.add(&attribute,sizeof(Attribute));
	count++ ;
	return * this;
}

Attribute *
Protorec::put_record()
{
	Attribute *	x;
	Attribute	attribute, *p;

	attribute.name = an_nomore;
	attribute.form = af_none;
	attribute.value.word = 0;
	vector.add(&attribute,sizeof(Attribute));
	count++ ;
	p = (Attribute *) vector.ptr();
	p->value.word = count;
	x = (Attribute *)new(char[vector.size()]);
	memcpy((char*)x,(char*)vector.ptr(),vector.size());
	vector.clear();	
	attribute.name = an_count;
	attribute.form = af_int;
	attribute.value.word = 0;
	vector.add(&attribute,sizeof(Attribute));
	count = 1;
	return x;
}
