#pragma comment(exestr, "@(#) data.c 12.1 95/05/09 ")

/************************************************************************* 
************************************************************************** 
**	Copyright (C) The Santa Cruz Operation, 1989-1992
**	This Module contains Proprietary Information of
**	The Santa Cruz Operation and should be treated as Confidential.
**
**  File Name:  	data.c        
**  ---------
**
**  Author:		Kyle Clark
**  ------
**
**  Creation Date:	21 November 1989
**  -------------
**
**  Overview:	
**  --------
**  Implements data structure for maintaining and writing data parsed
**  from .xgi files.
**
**  External Functions:
**  ------------------
** 
**  Data Structures:
**  ---------------
**
**  Bugs:
**  ----
**  Lots of performance improvements can be made.
**
 * Modification History:
 *     10 June 1990  kylec   M000 
 *     Changed memory addresses from low and high addresses to
 *     base plus length.  Function AddMemory replaced with
 *     SetBaseAddress and SetMemoryLength.
**
*************************************************************************** 
***************************************************************************/


/*
 *	S001	Sept 3 91	pavelr
 *	- check if a port is within an OK address range
 *	S002	Sept 24 91	pavelr
 *	- fixes for no memory and no ports
 *	S003	Oct 02 92	buckm
 *	- Eliminate stdout mucking.
 *	- Add some register decl's.
 *	S004	Nov 24 92	buckm
 *	- Suppress the class comments in PrintClassData().
 */

#include "vidparse.h"
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <fcntl.h>
#include <sys/errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <errno.h>
#include <malloc.h>




/************************************************************************ 
*
*                         External functions
*
*************************************************************************/ 

extern int errexit(char *);
extern int warning(char *);



/************************************************************************ 
*
*                                Types
*
*************************************************************************/ 

typedef struct port_element {

    unsigned long address;
    unsigned long count;
    struct port_element *next;

} PortElement;

    
typedef struct class_element {
    
    char *class;
    char *comment;
    unsigned long length;		/* M000 */ 
    unsigned long base;			/* M000 */
    PortElement *port_list;
    struct class_element *next;

} ClassElement;




/************************************************************************ 
*
*                            Private data
*
*************************************************************************/

/*
 * Structure for maintaining info parsed
 * from .xgi files.
 */
static struct {

    int size;
    ClassElement *head;

} classData;




/************************************************************************ 
*
*                            Macros 
*
*************************************************************************/ 

#define M_GreaterThan(x,y)	(strcmp((x), (y)) > 0)
#define M_LessThan(x,y)		(strcmp((x), (y)) < 0)

#define M_ToLower(x)		{\
				char *i = (x);\
				while (*i != NULL_C)\
				    {*i = tolower(*i); i++;}\
				}

#define M_ToUpper(x)		{\
				char *i = (x);\
				while (*i != NULL_C)\
				    {*i = toupper(*i); i++;}\
				}

#define M_InsertClass(x)	(x)->next = classData.head;\
				classData.head = (x);\
				classData.size++

#define M_FindClass(x,y)	(y) = classData.head;\
				while ((y) != NULL)\
				{\
				    if (strcmp((y)->class, x)==0)\
					break;\
				    (y) = (y)->next;\
				}


/*************************************************************************
 *
 *  InitClassData()
 *
 *  Description:
 *  -----------
 *
 *  Arguments:
 *  ---------
 *
 *  Output:
 *  ------
 *
 *************************************************************************/

int
InitClassData()

{
    
    dbg("InitClassData\n");
    classData.head = (ClassElement *)NULL;
    classData.size = 0;

    return(GOOD);
}






/*************************************************************************
 *
 *  AddClass()
 *
 *  Description:
 *  -----------
 *  Adds a new class to classData if it doesn't already exist.
 *
 *  Arguments:
 *  ---------
 *  class - name of class to add.
 *
 *  Output:
 *  ------
 *
 *************************************************************************/

int
AddClass(class)

char *class;

{
    register ClassElement *class_elp = (ClassElement *)NULL;

    dbg("*\nAddClass*\n");
    M_ToUpper(class);
    M_FindClass(class, class_elp);
    if (class_elp == (ClassElement *)NULL)
    {
	class_elp = (ClassElement *)malloc(sizeof(ClassElement));
	if (class_elp == (ClassElement *)NULL)
	    errexit("system: out of memory");
	if ((class_elp->class = strdup(class)) == (char *)NULL)
	    errexit("system: out of memory");
	class_elp->comment = (char *)NULL;
        class_elp->length = EMPTY;		/* M000 */
        class_elp->base = EMPTY;		/* M000 */
        class_elp->port_list = (PortElement *)NULL;
        class_elp->next = (ClassElement *)NULL;
	M_InsertClass(class_elp);
    }
    return(GOOD);
}





/*************************************************************************
 *
 *  AddComment()
 *
 *  Description:
 *  -----------
 *  Adds a comment field to a class.
 *
 *  Arguments:
 *  ---------
 *  comment
 *  class - class identifier
 *
 *  Output:
 *  ------
 *
 *************************************************************************/

int
AddComment(comment, class)

char *comment;
char *class;

{
    register ClassElement *class_elp = (ClassElement *)NULL;

    dbg("AddComment\n");
    M_FindClass(class, class_elp);
    if (class_elp == (ClassElement *)NULL)
    {
	AddClass(class);
        M_FindClass(class, class_elp);
    }

    if (class_elp->comment == (char *)NULL)
    {
        class_elp->comment = strdup(comment);
        if (class_elp->comment == (char *)NULL)
	    errexit("system: out of memory");
    }
    /* else save the most detailed comment */
    else if (strlen((char *)class_elp->comment) < strlen((char *)comment))
    {
        free((char *)class_elp->comment);
        class_elp->comment = strdup(comment);
        if (class_elp->comment == (char *)NULL)
	    errexit("system: out of memory");
    }

    return(GOOD);
}




/*************************************************************************
 *
 *  SetBaseAddress()
 *
 *  Description:
 *  -----------
 *  Set the base memory address for the class range specified.
 *  If the base is already set, and the new address is nto equal
 *  to the existing address, generates an error.
 *
 *  Arguments:
 *  ---------
 *  address - new memory address
 *  class - class identifier
 *
 *  Output:
 *  ------
 *
 *************************************************************************/

int
SetBaseAddress(address, class)	/* M000 , new function */

unsigned long address;
char *class;

{
    register ClassElement *class_elp = (ClassElement *)NULL;
    char msg[BUF_LENGTH];

    dbg("AddMemory\n");
    M_FindClass(class, class_elp);
    if (class_elp == (ClassElement *)NULL)
    {
	AddClass(class);
        M_FindClass(class, class_elp);
    }

    if (class_elp->base == EMPTY)
    {
	class_elp->base = address;
    }
    else if (class_elp->base != address)
    {
	sprintf(msg, "%s %s", class, "base memory address redefinition"); 
	warning(msg);
    }

    return(GOOD);
}

/*************************************************************************
 *
 *  SetMemoryLength()
 *
 *  Description:
 *  -----------
 *  Sets the amount of memory used by the class.
 *  If the length is already specified, uses the larger of the two.
 *
 *  Arguments:
 *  ---------
 *  mem_length - new memory address
 *  class - class identifier
 *
 *  Output:
 *  ------
 *
 *************************************************************************/

int
SetMemoryLength(mem_length, class)	/* M000 , new function */

unsigned long mem_length;
char *class;

{
    register ClassElement *class_elp = (ClassElement *)NULL;

    dbg("AddMemory\n");
    M_FindClass(class, class_elp);
    if (class_elp == (ClassElement *)NULL)
    {
	AddClass(class);
        M_FindClass(class, class_elp);
    }

    if ((class_elp->length == EMPTY) || (class_elp->length < mem_length))
    {
	class_elp->length = mem_length;
    }

    return(GOOD);
}





/*************************************************************************
 *
 *  AddPort()
 *
 *  Description:
 *  -----------
 *  Add a new port address to the specified class.
 *
 *  Arguments:
 *  ---------
 *  address
 *  class - class identifier.
 *
 *  Output:
 *  ------
 *
 *************************************************************************/

int
AddPort(address, class)

unsigned long address;
char *class;

{
    ClassElement *class_elp = (ClassElement *)NULL;
    register PortElement *port_elp = (PortElement *)NULL;
    register PortElement *tmp_elp = (PortElement *)NULL;
    PortElement *previous_elp = (PortElement *)NULL;
    PortElement *deleteMe_elp = (PortElement *)NULL;

    dbg("AddPort\n");

    /* S001 */
    if (address > MAXPORT)
    	return (GOOD);

    M_FindClass(class, class_elp);
    if (class_elp == (ClassElement *)NULL)
    {
	AddClass(class);
        M_FindClass(class, class_elp);
    }

    tmp_elp = class_elp->port_list;
    while(tmp_elp != (PortElement *)NULL)
    {
	if ((address >= tmp_elp->address) &&
	    (address < (tmp_elp->address + tmp_elp->count)))
	{
	    return(GOOD);
	}
	else if (address == (tmp_elp->address + tmp_elp->count))
	{
	    tmp_elp->count += (unsigned long)1;

	   /*
	    * Check that you are not overlapping the next element.
	    */
	    if ((tmp_elp->next != (PortElement *)NULL) &&
	        ((address+1) == tmp_elp->next->address))
	    {
		/* Merge elements */
		tmp_elp->count += (tmp_elp->next->count-(unsigned long)1);
		deleteMe_elp = tmp_elp->next;
		tmp_elp->next = deleteMe_elp->next;
		free((char *)deleteMe_elp);
	    }
	    return(GOOD);
	}
	else if (address == (tmp_elp->address-(unsigned long)1))
	{
	    tmp_elp->count += (unsigned long) 1;
	    tmp_elp->address = address;
	    return(GOOD);
	}
	else if (address < tmp_elp->address)
	{
	    break;
	}
	else
	{
	    previous_elp = tmp_elp;
	    tmp_elp = tmp_elp->next;
	}
    }


   /*
    * If we reach this stage then we need to add another 
    * port address element to the list.
    */
    port_elp = (PortElement *)malloc(sizeof(PortElement));
    if (port_elp == (PortElement *)NULL)
        errexit("system: out of memory");
    port_elp->count = (unsigned long) 1;
    port_elp->address = address;

    /* Insert element */
    port_elp->next = tmp_elp;
    if (previous_elp != (PortElement *)NULL)
        previous_elp->next = port_elp;
    else
        class_elp->port_list = port_elp;


    return(GOOD);

}





/*************************************************************************
 *
 *  WriteClassData()
 *
 *  Description:
 *  -----------
 *  Writes data in class data structure to CLASS_FILE.
 *  Format is that required by in file /etc/conf/pack.d/cn/class.h
 *
 *  Arguments:
 *  ---------
 *  classFile - file to write output
 *
 *  Output:
 *  ------
 *
 *************************************************************************/

int
WriteClassData(classFile)

char *classFile;

{
    int si;
    char msg[BUF_LENGTH];
    struct stat stat_buf;
    FILE *fp;

    SetCurrentFile(classFile);
    if (Warnings() == YES)		/* S002 */
    {
	sprintf(msg, "%s", "bad data in grafinfo files"); 
	errexit(msg);
    }

    if (stat(classFile, &stat_buf) == ERROR) 
        if (errno != ENOENT)
        {
	    warning ("cannot stat");
	    return(BAD);
        }

    if ((fp = fopen(classFile, "w+")) == NULL)
    {
	warning("cannot open");
	return(BAD);
    }


    fprintf(fp, "%s", HEADER);		/* S002 */
    fprintf(fp, "%s", COPYRIGHT);
    PrintClassData(fp);

    fclose(fp);

    return(GOOD);
}






/*************************************************************************
 *
 *  PrintClassData()
 *
 *  Description:
 *  -----------
 *  Prints all class data to stdout.
 *
 *  Arguments:
 *  ---------
 *  fp - file to write output
 *
 *  Output:
 *  ------
 *
 *************************************************************************/

int
PrintClassData(fp)

FILE *fp;

{
    register ClassElement *class_elp = (ClassElement *)NULL;
    register PortElement *port_elp = (PortElement *)NULL;


   /*
    * First generate all PORT_STRUCT's. S002
    */
    class_elp = classData.head;
    while (class_elp != (ClassElement *)NULL)
    {
      if (class_elp->port_list) {
	fprintf(
	    fp, 
	    "struct portrange vidc_%sports[] = {\n", 
	    class_elp->class);

	port_elp = class_elp->port_list;
	while (port_elp != (PortElement *)NULL)
	{
	    fprintf(
		fp, 
		"\t{ 0x%x, %d },\n", 
		port_elp->address, 
		port_elp->count);

	    port_elp = port_elp->next;
	}
	fprintf(fp, "\t{ 0, 0 },\n};\n\n");

      }
      class_elp = class_elp->next;
    }


   /*
    * Now generate VID_STRUCT. S002
    */
    fprintf(fp, "\n\nstruct vidclass vidclasslist[] = {\n");
    class_elp = classData.head;
    while (class_elp != (ClassElement *)NULL)
    {
	if ((class_elp->length != 0) &&
	    (class_elp->port_list != (PortElement *)NULL))
	  fprintf(
	    fp, 
	    "\t{\t\"%s\", \"%s\",\n\t\t0x%x, 0x%x,\n\t\tvidc_%sports,\n\t},\n", 
	    class_elp->class,
	    "",				/* S004 */
	    class_elp->base,		/* M000 */
	    class_elp->length, 		/* M000 */
	    class_elp->class);
	else if ((class_elp->port_list == (PortElement *) NULL) &&
		 (class_elp->length != 0))
	  fprintf(
	    fp, 
	    "\t{\t\"%s\", \"%s\",\n\t\t0x%x, 0x%x,\n\t\tvidcNULL,\n\t},\n", 
	    class_elp->class,
	    "",				/* S004 */
	    class_elp->base,		/* M000 */
	    class_elp->length);
	else if (class_elp->port_list != (PortElement *) NULL)

	/* hack alert hack alert hack alert
	 * - 0xFF000 should be ROM memory. MAP_CLASS currently dies
	 *   when passed a range of 0. This is here for cards which are not
	 *   memory mapped, for example, the 8514.
	 */

	  fprintf(
	    fp, 
	    "\t{\t\"%s\", \"%s\",\n\t\t0xFF000, 0x1000,\n\t\tvidc_%sports,\n\t},\n", 
	    class_elp->class,
	    "",				/* S004 */
	    class_elp->class);

	class_elp = class_elp->next;
    }
    fprintf(fp, "\t{ 0 }\n");
    fprintf(fp, "};\n\n");

    return(GOOD);
}
