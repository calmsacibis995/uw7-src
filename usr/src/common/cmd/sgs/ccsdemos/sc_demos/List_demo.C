/*ident	"@(#)ccsdemos:sc_demos/List_demo.C	1.2" */

#include "List_demo.h"
#include <List.h>
#include <iostream.h>
 
int main()
{
    Student s1("George","A");
    Student s2("Arthur","D");
    Student s3("Chester","C");
    Student s4("Willy","B");
 
    SCO_SC::List<Student> Class(s1,s2,s3,s4);
    cout << "No order    " << Class << "\n";

    Class.sort(name_compare);
    cout << "Name order  " << Class << "\n";

    Class.sort(grade_compare);
    cout << "Grade order " << Class << "\n";

    return 0;
}
