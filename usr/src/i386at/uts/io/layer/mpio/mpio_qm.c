#ident	"@(#)kern-pdi:io/layer/mpio/mpio_qm.c	1.1"

#if defined(__cplusplus)
extern "C" {
#endif

/*
 *
 *	This module contains queue functions. The queue abstraction is
 *	managed by the operations defined below. These operations use
 *	the following structure:
 *
 * anchor ::= { head, tail, rotor, link_offset, size }
 *
 *                          +----anchor---+
 * .------------------------|-head | tail-|------------------------.
 * |                        +------+------+                        |
 * |                         +-- rotor                             | 
 * |                         |                                     |
 * |                         v                                     |
 * `-->+entry-+<--\   /-->+entry-+\                     +entry-+<--'
 *     |      |    \ /    |      | \                    |      |
 *     :      :     X     :      :  > link_offset       :      :
 *     |      |    / \    |      | /                    |      |
 *     +------+   /   \   +------+/  ...         ...    +------+
 *     | next |--/     \  | next |--/               \   | NULL |
 *     | NULL |         \-| prev |                   \--| prev |
 *     +------+           +------+                      +------+
 *     :      :           :      :                      :      :
 *     +------+           +------+                      +------+
 *
 *
 *
 * The rotor design is truely a hack. A rotor is added to create an
 * artificial circular indicator. The rotor keeps track the current
 * position of each access so that next accesses via mpio_qm_next_and_rotate()
 * will point to the next items in the queue. This rotor adds a small
 * overhead to other queue primitives.
 */

#include "mpio_qm.h"

/*
 * void
 * mpio_qm_initialize  ( mpio_qm_anchor_t *anchor_ptr, ulong_t link_offset )
 *
 * Calling/Exit State
 *	None.
 */
void           
mpio_qm_initialize  ( mpio_qm_anchor_t *anchor_ptr, ulong_t link_offset )
{
	anchor_ptr->links.next  = NULL;
	anchor_ptr->links.prev  = NULL;
	anchor_ptr->rotor  		= NULL;
	anchor_ptr->link_offset = link_offset / sizeof(void *);
	anchor_ptr->size	    = 0;
}

/*
 * void
 * mpio_qm_move  ( mpio_qm_anchor_t *from_anchor_ptr, mpio_qm_anchor_t *to_anchor_ptr )
 *      The queue is moved by simply copying the anchor information.
 *      The from_anchor is then "initialized" to the empty state.
 *      The caller must regulate access to the queue.
 *
 * Calling/Exit State
 *	None.
 */
void
mpio_qm_move  ( mpio_qm_anchor_t *from_anchor_ptr, mpio_qm_anchor_t *to_anchor_ptr )
{
	to_anchor_ptr->links.next = from_anchor_ptr->links.next;
	to_anchor_ptr->links.prev = from_anchor_ptr->links.prev;
	to_anchor_ptr->link_offset = from_anchor_ptr->link_offset;
	to_anchor_ptr->size = from_anchor_ptr->size;
	to_anchor_ptr->rotor = to_anchor_ptr->links.next;

	/*
	 *	Re-initialize the from_anchor_ptr.  The link_offset field
	 *	does not change.
	 */
	from_anchor_ptr->links.next = NULL;
	from_anchor_ptr->links.prev = NULL;
	from_anchor_ptr->rotor      = NULL;
	from_anchor_ptr->size = 0;
return;
}

/*
 * boolean_t
 * mpio_qm_is_member( mpio_qm_anchor_t *anchor_ptr, void *entry_ptr )
 *      Returns TRUE if the given entry is on the queue.
 *
 * Calling/Exit State
 *      The caller must control access to the queue.
 */
boolean_t
mpio_qm_is_member( mpio_qm_anchor_t *anchor_ptr, void *entry_ptr )
{
	void	*q_entry_ptr;

	/*
	 *	Walk all entries on the queue until we encounter the entry
	 *	in question or hit the end of the queue.
	 */

	for ( q_entry_ptr = MPIO_QM_HEAD( anchor_ptr );
		  q_entry_ptr != NULL;
		  q_entry_ptr = MPIO_QM_NEXT( anchor_ptr, q_entry_ptr ) ) {
		if ( q_entry_ptr == entry_ptr ){
			return B_TRUE;
		}
	}
	return B_FALSE;
}

/*
 * void
 * mpio_qm_add_head ( mpio_qm_anchor_t *anchor_ptr, void *entry_ptr )
 *      The given entry is placed at the head of the queue (it must not
 *      currently be on the queue).
 *
 * Calling/Exit State
 *      The caller must control access to the queue.
 */

void
mpio_qm_add_head ( mpio_qm_anchor_t *anchor_ptr, void * entry_ptr )
{

	mpio_qm_link_t * next_links_ptr;
	mpio_qm_link_t * new_links_ptr;


	if( anchor_ptr->links.next == NULL ) {
		next_links_ptr = &anchor_ptr->links;
    }
	else {
		next_links_ptr =
			MPIO_QM_ENTRY_PTR_TO_LINK_PTR( anchor_ptr, anchor_ptr->links.next );
    }
	new_links_ptr = MPIO_QM_ENTRY_PTR_TO_LINK_PTR( anchor_ptr, entry_ptr );
	new_links_ptr->next = anchor_ptr->links.next;
	new_links_ptr->prev = next_links_ptr->prev;
	anchor_ptr->links.next = entry_ptr;
	next_links_ptr->prev = entry_ptr;
	anchor_ptr->size += 1;
	if (anchor_ptr->rotor == NULL){
		anchor_ptr->rotor = entry_ptr;
	}
	return;
}


/*
 * void
 * mpio_qm_add_tail ( mpio_qm_anchor_t *anchor_ptr, void *entry_ptr )
 *      The given entry is placed at the tail of the queue (it must not
 *      currently be on the queue).
 *
 * Calling/Exit State
 *      The caller must control access to the queue.
 */

void
mpio_qm_add_tail ( mpio_qm_anchor_t *anchor_ptr, void * entry_ptr )
{
	mpio_qm_link_t * prev_links_ptr;
	mpio_qm_link_t * new_links_ptr;

	if( anchor_ptr->links.next == NULL ) {
		prev_links_ptr = &anchor_ptr->links;
	}
	else {
		prev_links_ptr =
        MPIO_QM_ENTRY_PTR_TO_LINK_PTR( anchor_ptr, anchor_ptr->links.prev );
	}

	new_links_ptr = MPIO_QM_ENTRY_PTR_TO_LINK_PTR( anchor_ptr, entry_ptr );
	new_links_ptr->prev = anchor_ptr->links.prev;
	new_links_ptr->next = prev_links_ptr->next;
	anchor_ptr->links.prev = entry_ptr;
	prev_links_ptr->next = entry_ptr;
	anchor_ptr->size += 1;
	if (anchor_ptr->rotor == NULL){
		anchor_ptr->rotor = entry_ptr;
	}
}

/*
 * void
 * mpio_qm_next_and_rotate ( mpio_qm_anchor_t *anchor_ptr)
 *	Rotate the queue head and return the head before rotation.
 *
 * Calling/Exit State
 *      The caller must control access to the queue.
 */
mpio_qm_link_t *
mpio_qm_next_and_rotate( mpio_qm_anchor_t *anchor_ptr)
{
	mpio_qm_link_t * return_value;
	mpio_qm_link_t * next_links_ptr;

	return_value = anchor_ptr->rotor;
    if (return_value != NULL){
    	next_links_ptr = MPIO_QM_ENTRY_PTR_TO_LINK_PTR( anchor_ptr, return_value );
		if (next_links_ptr->next != NULL){
			 anchor_ptr->rotor = next_links_ptr->next;
		}
		else{
			 anchor_ptr->rotor = anchor_ptr->links.next;
		}
	}
	return return_value;	
}

/*
 * void *
 * mpio_qm_remove_head ( mpio_qm_anchor_t *anchor_ptr )
 * 	The first entry in the queue is removed.  If the queue is empty,
 *	the value NULL is returned. 
 *
 * Calling/Exit State
 *      The caller must control access to the queue.
 */
void *
mpio_qm_remove_head ( mpio_qm_anchor_t *anchor_ptr )
{
	mpio_qm_link_t * removed_links_ptr;
	mpio_qm_link_t * next_links_ptr;
	mpio_qm_link_t * return_value;

	return_value = anchor_ptr->links.next;

	if( return_value != NULL ) {
		anchor_ptr->size -= 1;
		removed_links_ptr = MPIO_QM_ENTRY_PTR_TO_LINK_PTR( 
								anchor_ptr, return_value );
		if( removed_links_ptr->next == NULL ) {
			next_links_ptr = &anchor_ptr->links;
		}
		else {
			next_links_ptr = MPIO_QM_ENTRY_PTR_TO_LINK_PTR(
                    anchor_ptr, removed_links_ptr->next );
		}
		next_links_ptr->prev = NULL;
		anchor_ptr->links.next = removed_links_ptr->next;
	}
	else {
		return return_value;
	}
	/*
 	 *  hack the rotor in
	*/
	if ( anchor_ptr->rotor == return_value){
		anchor_ptr->rotor = anchor_ptr->links.next;
	}

	return return_value;

}


/*
 * void *
 * mpio_qm_remove_tail ( mpio_qm_anchor_t *anchor_ptr )
 *  The last entry in the queue is removed.  If the queue is empty,
 *	the value NULL is returned. 
 *
 * Calling/Exit State
 *      The caller must control access to the queue.
 */

void *
mpio_qm_remove_tail ( mpio_qm_anchor_t *anchor_ptr )
{
	mpio_qm_link_t * removed_links_ptr;
	mpio_qm_link_t * prev_links_ptr;
	mpio_qm_link_t * return_value;

	return_value = anchor_ptr->links.prev;

	if( return_value != NULL ) {
		anchor_ptr->size -= 1;
		removed_links_ptr = MPIO_QM_ENTRY_PTR_TO_LINK_PTR( 
								anchor_ptr, return_value );
		if( removed_links_ptr->prev == NULL ) {
			prev_links_ptr = &anchor_ptr->links;
		}
		else {
			prev_links_ptr = MPIO_QM_ENTRY_PTR_TO_LINK_PTR(
                    anchor_ptr, removed_links_ptr->prev );
		}
		prev_links_ptr->next = NULL;
		anchor_ptr->links.prev = removed_links_ptr->prev;
	}
	else {
		return return_value;
	}
	/*
 	 *  hack the rotor in
	*/
	if ( anchor_ptr->rotor == return_value){
		anchor_ptr->rotor = anchor_ptr->links.next;
	}
	return return_value;
}


/*
 * void
 * mpio_qm_remove ( mpio_qm_anchor_t *anchor_ptr, void *entry_ptr )
 *	The entry is removed from the queue (it must currently be on the
 *	queue).
 *
 * Calling/Exit State
 *      The caller must control access to the queue.
 */
void
mpio_qm_remove ( mpio_qm_anchor_t *anchor_ptr, void * entry_ptr )
{
	mpio_qm_link_t * next_links_ptr;
	mpio_qm_link_t * prev_links_ptr;
	mpio_qm_link_t * removed_links_ptr;

	anchor_ptr->size -= 1;
	removed_links_ptr = MPIO_QM_ENTRY_PTR_TO_LINK_PTR( anchor_ptr, entry_ptr );
	if( removed_links_ptr->prev == NULL ) {
		prev_links_ptr = &anchor_ptr->links;
	}
	else {
		prev_links_ptr = MPIO_QM_ENTRY_PTR_TO_LINK_PTR(
						anchor_ptr, removed_links_ptr->prev );
	}
	if( removed_links_ptr->next == NULL ) {
		next_links_ptr = &anchor_ptr->links;
	}
	else {
		next_links_ptr = MPIO_QM_ENTRY_PTR_TO_LINK_PTR(
						anchor_ptr, removed_links_ptr->next );
	}
	prev_links_ptr->next = removed_links_ptr->next;
	next_links_ptr->prev = removed_links_ptr->prev;

	if ( anchor_ptr->rotor == entry_ptr){
		anchor_ptr->rotor = anchor_ptr->links.next;
	}

	return;
}
