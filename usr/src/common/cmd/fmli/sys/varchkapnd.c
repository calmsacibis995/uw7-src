/*		copyright	"%c%" 	*/

/*
 * Copyright  (c) 1986 AT&T
 *	All Rights Reserved
 */
#ident	"@(#)fmli:sys/varchkapnd.c	1.1.4.3"

#include	<stdio.h>
#include	"wish.h"
#include	"var_arrays.h"

/*
 * like array_append, but creates array if NULL
 */
struct v_array *
array_check_append(size, array, element)
unsigned size;
struct v_array	array[];
char	*element;
{
	if (array == NULL)
		array = array_create(size, 8);
	return array_append(array, element);
}
