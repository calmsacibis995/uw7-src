#ident	"@(#)Space.c	1.1"

#include <sys/param.h>
#include <sys/mfpd.h>
#include <config.h>


/* 
 * If there are more than 4 boards/controllers configured in the system, then
 * 1) Increase the size of the mfpdcfg array below accordingly.
 * 2) Duplicate the structure as many times as is necessary correctly specifying
 *    i)  the port type.
 *    ii) the number of devices connected to the port.
 *    iii)information about these devices.
 *   
 *    Set the other fields to the default values shown. They will be corrected 
 *    in mfpdstart, if incorrect.
 */

/*
 * num_mfpd will be initialized in mfpdstart.
 */
int num_mfpd = 0;


struct mfpd_cfg mfpdcfg[]={

	{
		MFPD_SIMPLE_PP,       /* PPT1:Postinstall script modifies this*/
				      /* port type */
		0,                    /* base address */
		0,                    /* last register */
		-1,                   /* dma channel */
		0,                    /* ip level */
		0,                    /* interrupt vector */
		0,                    /* Capability */
		MFPD_CENTRONICS,      /* Current mode */
		NULL,                 /* Pointer to mhr entry */
		2,		      /* Number of devices connected to port */
		{ 		      /* Info. about devices connected to port*/
		 {0,MFPD_PRINTER},{0,MFPD_TRAKKER}
		},	
 		PORT_FREE, 	      /* Exclusive flag */
		0,		      /* Access delay flag */
		NULL,		      /* Cookie for interrupt */
		NULL,		      /* q_head */
		NULL 		      /* q_tail */
	},
	{
		MFPD_SIMPLE_PP,       /* PPT2:Postinstall script modifies this*/
		0, 
		0,  
		-1, 
		0, 
		0, 
		0,          
		MFPD_CENTRONICS,
		NULL,
		2,
		{ 
		 {0,MFPD_PRINTER},{0,MFPD_TRAKKER}
		},	
 		PORT_FREE,
		0,
		NULL,
		NULL,
		NULL 
	},
	{
		MFPD_SIMPLE_PP,       /* PPT3:Postinstall script modifies this*/
		0,     
		0,      
		-1,     
		0,     
		0,   
		0,             
		MFPD_CENTRONICS,
		NULL,
		2,
		{ 
		 {0,MFPD_PRINTER},{0,MFPD_TRAKKER}
		},	
 		PORT_FREE,
		0,
		NULL,
		NULL,
		NULL 
	},
	{
		MFPD_SIMPLE_PP,       /* PPT4:Postinstall script modifies this*/
		0,    
		0,     
		-1,    
		0,    
		0,  
		0,              
		MFPD_CENTRONICS,
		NULL,
		2,
		{ 
		 {0,MFPD_PRINTER},{0,MFPD_TRAKKER}
		},	
 		PORT_FREE,
		0,
		NULL,
		NULL,
		NULL 
	}
};


