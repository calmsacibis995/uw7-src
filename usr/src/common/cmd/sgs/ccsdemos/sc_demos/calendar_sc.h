/*ident	"@(#)ccsdemos:sc_demos/calendar_sc.h	1.2" */

#include "appoint.h"
#include <Regex.h>
#include <Block.h>
#include <Array_alg.h>
#include <Map.h>
#include <iostream.h>

using SCO_SC::Regex;
using SCO_SC::Block;
using SCO_SC::insert;
using SCO_SC::Map;  using SCO_SC::Mapiter;

// an Appts contains all of the Appointments
// for a given day, in time order
class Appts {
    Block<Appointment> appts;
    int nappts;
public:
    Appts() : nappts(0) {}
    void add(const Appointment& a) {
        appts.reserve(nappts);
        insert(a, &appts[0], &appts[nappts]);
        ++nappts;
    }
    int num() const {
        return nappts;
    }
    const Appointment& operator[](int i) const {
        return appts[i];
    }
};

// represent Dates by the midnight of that date
class Date {
    friend ostream& operator<<(ostream& os, Date d);
    friend int operator<(Date a, Date b);
    Time midnight;
public:
    Date() {}
    Date(Time t);
};

typedef Mapiter<Date,Appts> Calendariter;

// a Calendar is basically a Map from Dates to Appts
class Calendar {
    Map<Date,Appts> m;
public:
    void add(const Appointment& a) {
        Date d(a.time);
        m[d].add(a);
    }
    Calendariter element(Date d) const { 
        return m.element(d);
    }
    Calendariter first() const { 
        return m.first();
    }
    const Appts& operator[](Date t) const {
        return ((Calendar*)this)->m[t];
    }
};
