#ident	"@(#)debugger:inc/common/Protorec.h	1.1"
#ifndef Protorec_h
#define Protorec_h

#include	"Attribute.h"
#include	"Vector.h"

class Protorec {
	Vector		vector;
	int		count;
public:
			Protorec();
			~Protorec()	{}
	Protorec &	add_attr(Attr_name, Attr_form, long );
	Protorec &	add_attr(Attr_name, Attr_form, void * );
	Protorec &	add_attr(Attr_name, Attr_form, const Attr_value & );
	Attribute *	put_record();
};

#endif /* Protorec_h */
