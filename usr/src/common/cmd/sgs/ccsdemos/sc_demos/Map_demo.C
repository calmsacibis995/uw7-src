/*ident "@(#)ccsdemos:sc_demos/Map_demo.C	1.2" */

#include <String.h>
#include <Map.h>
#include <iostream.h>

using SCO_SC::String;
using SCO_SC::Map;  using SCO_SC::Mapiter;

int main() {
        Map<String,int> count;
        String word;

        while (cin >> word)
                count[word]++;

	Mapiter<String,int> p(count);
	while (p.next())
		cout << p.curr()->value << '\t' << p.curr()->key << endl;

	return 0;
}
