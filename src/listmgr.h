/* -------------------------------------------------------------------------
 * Steve Rider - stever@area23.org
 * Grufti - an interactive, service-oriented bot for IRC 
 *
 * listmgr.h - general-purpose list manager (linked-list version)
 *
 * ------------------------------------------------------------------------- */
/* 21 April 98 */

#ifndef LISTMGR_H
#define LISTMGR_H

/*
 * ListManager declaration
 *
 * When we desire a linked-list structure, we use the listmgr structure to
 * manage the list.  This keeps details such as efficiency routines and
 * storage structure abstracted from the programmer and allows us to add
 * more sophisticated structures quickly and in one place. 
 *
 * The structure is as follows:
 *
 *    |-----------|
 *    | ListMgr   |
 *    |  info     |
 *    |  head --------> listnode --> listnode --> ... --> listnode --> NULL
 *    |-----------|         |            |                    |
 *                          |-> object   |-> object           |-> object
 *
 * List objects can be of any type, and do not need 'next' pointers and
 * other list maintenance variables.
 *
 * Each list node, which contains a single object, looks like this:
 *
 *    |-----------|
 *    | ListNode  |
 *    |           |
 *    |  item --------> object
 *    |-----------|
 *
 * Efficiency considerations:
 *   When the distribution of requested items is non-uniform, the list can
 *   self-optimize by allowing an item to move to the front of the list each
 *   time it is accessed.
 *
 * Iterating lists
 *   The functions begin() and next() are used to iterate through a list.
 *   begin() returns a pointer to the item at the head of the list.
 *   next() returns a pointer to the item next in the list.  A for loop
 *   could be constructed as follows
 *
 *       for(x = listmgr_begin(List); x; x = listmgr_next(List))
 *
 *   Nesting loops presents a problem, since the cur_node pointer in the
 *   listmanager structure can not identify which loop it belongs to.  For
 *   this reason, the user must explicitly show which iterator is being used.
 *   The following shows an example of three nested for-loops operating
 *   on the same listmanager object.
 *
 *       for(i = listmgr_begin(L); i; i = listmgr_next(L))
 *         for(j = listmgr_begin_nested(L,1); j; j = listmgr_next_nested(L,1))
 *           for(k = listmgr_begin_nested(L,2); k; k = listmgr_next_nested(L,2))
 *
 * Sorting lists
 *   A 'next_sorted' pointer is used to provide a low-level mechanism for
 *   iterating through a sorted list.  We refer to the main list (optimally
 *   sorted) and the alternate list (user sorted) as two separate lists, but
 *   they both refer to the same instances of data.
 *
 *   The compare() function, provided by the user, is used to sort the lists.
 *
 *   The list is sorted by calling sort(), and iterated by calling
 *   begin_sorted() and next_sorted().  No nested iterators are provided.
 *   The list can be sorted by different fields by swapping out the compare
 *   function prior to sorting.
 *
 *   It is important to note that the original sorting of the list is
 *   maintained during any call to sort().  This original sorting is
 *   called the optimal sorting, since it is an ordering based on access
 *   frequency, which is why we would like to preserve it.  This sorting
 *   is done automatically, and does not need to be explicitly called by
 *   the user.
 *
 * Preserving sorted lists
 *   If a list has not been sorted and begin_sorted() and begin_next() are
 *   called, the ordering will be the same as the optimal ordering.  When an
 *   item is deleted from the list, the sorted list pointers are maintained,
 *   which means the user can still iterate through a sorted list without
 *   having to re-sort.  Items which are appended to the list will appear
 *   at the end of the sorted list.
 *
 *   It is a good idea to re-sort the list before each use.  List manager
 *   keeps track of the state of the list, and will not re-sort if the list
 *   is already sorted.
 *
 *   List manager provides routines to selectively remove items from the
 *   sorted representation of the list.  When the list is re-sorted, these
 *   items are re-attached automatically.
 *
 * Matching
 *   The match() function is used to locate items in the list.  Like the
 *   compare() function, the match() function can be swapped out too,
 *   allowing the user to perform a variety of searches based on different
 *   fields by swapping out the match() function before searching.
 */

/* must be at least 3 */
#define LISTMGR_NUM_CUR_NODES 5
#define LISTMGR_MAX_NESTED_ITER LISTMGR_NUM_CUR_NODES - 2

/* List manager object structure */
typedef struct listmgr_t {
	struct listnode *head;
	struct listnode *head_sorted;
	struct listnode *tail;
	struct listnode *tail_sorted;
 	struct listnode *cur_node[LISTMGR_NUM_CUR_NODES];

	void (*free)(void *item);
	int (*match)(const void *item, const void *match_token);
	int (*compare)(const void *item1, const void *item2);
	int length;
	int length_sorted;

	int flags;
} listmgr_t ;

/* public flags (private flags start with 0x0) */
#define LM_NOMOVETOFRONT 0x1000

/* Create and delete lists */
listmgr_t *listmgr_new_list(int flags);
void listmgr_destroy_list(listmgr_t *L);
void listmgr_delete_list(listmgr_t *L);

/* Append, retrieve, remove, delete */
void listmgr_append(listmgr_t *L, void *item);
void *listmgr_retrieve(listmgr_t *L, void *match_token);
void *listmgr_peek(listmgr_t *L, void *match_token);
void listmgr_remove(listmgr_t *L, void *item);
void listmgr_remove_from_sorted(listmgr_t *L, void *item);
void listmgr_delete(listmgr_t *L, void *item);

/*
 * Match function:
 *    takes pointer to item and pointer to match_token
 *    returns 1 if match, 0 if not
 *
 * Compare function:
 *    takes pointer to 2 items
 *    returns 1 if item1 > item2
 *            0 if item1 = item2
 *           -1 if item1 < item2
 *
 * Free function:
 *    takes a pointer to the item
 *    responsible for freeing the item itself
 */

/* List properties */
void listmgr_set_match_func(listmgr_t *L, void *func);
void listmgr_set_compare_func(listmgr_t *L, void *func);
void listmgr_set_free_func(listmgr_t *L, void *func);
int listmgr_length(listmgr_t *L);
int listmgr_length_sorted(listmgr_t *L);
int listmgr_isitem(listmgr_t *L, void *match_token);
int listmgr_isitem_peek(listmgr_t *L, void *match_token);

/* List ordering */
void listmgr_sort_ascending(listmgr_t *L);
void listmgr_sort_descending(listmgr_t *L);

/* Iterators */
void *listmgr_begin(listmgr_t *L);
void *listmgr_begin_nested(listmgr_t *L, int id);
void *listmgr_begin_sorted(listmgr_t *L);
void *listmgr_next(listmgr_t *L);
void *listmgr_next_nested(listmgr_t *L, int id);
void *listmgr_next_sorted(listmgr_t *L);

#endif /* LISTMGR_H */
