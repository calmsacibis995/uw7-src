/*		copyright	"%c%" 	*/

/*
 * Copyright  (c) 1986 AT&T
 *	All Rights Reserved
 */
#ident	"@(#)fmli:sys/vardelete.c	1.1.4.3"

#include	<stdio.h>
#include	"wish.h"
#include	"var_arrays.h"

/*
 * delete an element from a v_array
 * this does not reduce the size of the v_array in-core
 * array_shrink does that
 */
struct v_array *
array_delete(array, position)
struct v_array	array[];
unsigned	position;
{
	register struct v_array	*ptr;

	ptr = v_header(array);
	if (position >= ptr->tot_used)
		return array;
	ptr->tot_used--;
	ptr->tot_left++;
	if (position < ptr->tot_used)
		memshift(ptr_to_ele(ptr, position), ptr_to_ele(ptr, position + 1), ptr->ele_size * (ptr->tot_used - position));
	return v_body(ptr);
}
