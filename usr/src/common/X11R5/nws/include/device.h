#ident	"@(#)device.h	1.2"

/*----------------------------------------------------------------------------
 *					Device class - Object that has device data 
 *----------------------------------------------------------------------------*/
#ifndef DEVICE_H 
#define DEVICE_H

class ProcList;

enum { DNSIZE  = 14 };

class Device {			/* device list	 */
	
	friend class Process;

public:

private:
	char			dname[DNSIZE];	/* device name	 */
	dev_t			dev;		/* device number */
} ;

#endif	//  DEVICE_H
