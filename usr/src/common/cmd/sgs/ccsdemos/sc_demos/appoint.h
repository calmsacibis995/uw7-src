/*ident	"@(#)ccsdemos:sc_demos/appoint.h	1.2" */

#include <Time.h>
#include <String.h>

using SCO_SC::Time;
using SCO_SC::String;

// an Appointment consists of a time and a description
struct Appointment {
    Time time;
    String desc;
};

inline int operator<(const Appointment& a, const Appointment& b) {
    return a.time < b.time;
}
