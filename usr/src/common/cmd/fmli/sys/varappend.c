/*		copyright	"%c%" 	*/

/*
 * Copyright  (c) 1986 AT&T
 *	All Rights Reserved
 */
#ident	"@(#)fmli:sys/varappend.c	1.1.4.3"

#include	<stdio.h>
#include	"wish.h"
#include	"var_arrays.h"

/*
 * add another element onto the end of a v_array
 */
struct v_array *
array_append(array, element)
struct v_array	array[];
char	*element;
{
	register struct v_array	*ptr;

	ptr = v_header(array_grow(array, 1));
	if (element != NULL)
		memcpy(ptr_to_ele(ptr, ptr->tot_used - 1), element, ptr->ele_size);
	return v_body(ptr);
}
