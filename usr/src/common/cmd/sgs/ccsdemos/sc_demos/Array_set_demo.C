/*ident	"@(#)ccsdemos:sc_demos/Array_set_demo.C	1.1" */

#include "Array_set.h"
#include <iostream.h>

int main()
{
	Blockset<int> s;
	int i = 0;
	
	for (i = 0; i < 10; i++) {
		s.insert(2*i+1);
	}
	
	cout << s << "\n";
	return 0;
}
