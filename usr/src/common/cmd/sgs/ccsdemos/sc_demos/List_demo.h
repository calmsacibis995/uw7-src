/*ident	"@(#)ccsdemos:sc_demos/List_demo.h	1.2" */

#include <String.h>
#include <iostream.h>

class Student {
    SCO_SC::String name;
    SCO_SC::String grade;
public:
    Student(const SCO_SC::String& n,const SCO_SC::String& g) : name(n), grade(g) {}
    Student(const Student& s) : name(s.name), grade(s.grade) {}
    int operator==(const Student& s) { return name == s.name && grade == s.grade; }
    friend inline ostream& operator<<(ostream&,const Student&);
    friend inline int name_compare(const Student&,const Student&);
    friend inline int grade_compare(const Student&,const Student&);
}; 

inline ostream&
operator<<(ostream& os,const Student& s)
{
    os << s.name << ": " << s.grade;
    return os;
}

inline int
name_compare(const Student& s1,const Student& s2)
{
    if(s1.name + s1.grade < s2.name + s2.grade)
        return 1;
    else return 0;
}

inline int
grade_compare(const Student& s1,const Student& s2)
{
    if(s1.grade < s2.grade)
        return 1;
    else return 0;
}
