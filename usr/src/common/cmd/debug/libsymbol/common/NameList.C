#ident	"@(#)debugger:libsymbol/common/NameList.C	1.9"
#include	"Attribute.h"
#include	"Build.h"
#include	"NameList.h"
#include	<string.h>

NameEntry::NameEntry()
{
	namep = 0;
	form = af_none;
	value.word = 0;
}

NameEntry::NameEntry( const NameEntry & name_entry )
{
	namep = name_entry.namep;
	form = name_entry.form;
	value = name_entry.value;
}

// Make a NameEntry instance
Rbnode *
NameEntry::makenode()
{
	char *	s;

	s = new(char[sizeof(NameEntry)]);
	memcpy(s,(char*)this,sizeof(NameEntry));
	return (Rbnode*)s;
}

//used to do lookup
// s is the name being searched for and may be a partial name;
// namep will include the full function signature for C++ names
int
NameEntry::cmpName(const char* s)
{
	int rslt;
	const char *p;
	if( !(rslt=strcmp(namep, s)))
	{
		return rslt;
	}

	// if s is "X::", looking for any member of class "X"
	if ((p = strrchr(s, ':')) != 0 && *(p+1) == '\0')
		return strncmp(namep, s, p-s+1);

	// demangled function names are prototyped.  If s is not
	// prototyped, compare name up to the first '(' (i.e.,
	// up to the parameter list).

	if (!strchr(namep, '('))
		return rslt;

	char* sParenPosition;
	int sLen = strlen(s);
	int namepLen = strlen(namep);
	if ((p = strstr(s, "operator()")) != 0)
		p += strlen("operator()");
	else
		p = s;
	sParenPosition = strchr(p, '(');

	if (!sParenPosition)
	{
		if (namepLen > sLen)
		{
			if (strncmp(namep,s,sLen) == 0)
				if (namep[sLen] == '(' || namep[sLen] == '<')
					rslt = 0;
		}
	}
	return rslt;
}

// used to do insert
int
NameEntry::cmp( Rbnode & t )
{
	NameEntry *	name_entry = (NameEntry*)(&t);

	return ( strcmp(namep,name_entry->namep));
}

void
NameEntry::setNodeName(char* name)
{
	if ((namep = demangle_name(name)) == 0)
		namep = name;
}

NameEntry *
NameList::add( const char * s, const Attribute * a )
{
	NameEntry	node;

	node.setNodeName((char *)s);
	node.form = af_symbol;
	node.value.symbol = (Attribute *) a;
	return (NameEntry*)tinsert(node);
}

NameEntry *
NameList::add( const char * s, long w, Attr_form form )
{
	NameEntry	node;

	node.setNodeName((char *)s);
	node.form = form;
	node.value.word = w;
	return (NameEntry*)tinsert(node);
}
