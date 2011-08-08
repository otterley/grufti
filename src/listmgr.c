/* -------------------------------------------------------------------------
 * Steve Rider - stever@area23.org
 * Grufti - an interactive, service-oriented bot for IRC 
 *
 * listmgr.c - general-purpose list manager (linked-list version)
 *
 * DEBUG_STRICT rigorously verifies list integrity.  Expensive, should only
 * be used during testing.
 *
 * Compile with -DTEST to test this module.
 *
 * v1.1-compliant 26 April 98
 * ------------------------------------------------------------------------- */
/* 26 April 98 */

#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "grufti.h"
#include "listmgr.h"

#ifdef TEST
#define STANDALONE
#define DEBUG_STRICT
#endif

#ifdef STANDALONE

/* Normally defined in grufti.c */
#define fatal_error debug_to_stderr
#define debug debug_to_stderr
#define warning debug_to_stderr
#define internal debug_to_stderr
static void debug_to_stderr(char *fmt, ...)
{
	va_list args;

	va_start(args, fmt);
	vfprintf(stderr, fmt, args);
	fprintf(stderr, "\n");
	va_end(args);
}

/* Normally defined in xmem.c (#defined in misc.h) */
#undef xmalloc
#define xmalloc malloc
#undef xrealloc
#define xrealloc realloc
#undef xfree
#define xfree free

#endif /* STANDALONE */


/* -------------------------------------------------------------------------
 * BASE ROUTINES (INTERNAL)
 *
 * Five basic functions for operating on objects in a list are possible.
 *
 *   New     - creates a new object
 *   Attach  - attaches object to a list
 *   Detach  - detaches an object from the list
 *   Delete  - detaches and frees an object
 *   Destroy - recursively detaches and frees objects.  destroying a list
 *             frees all attached nodes and items. destroying a node detaches
 *             the node, frees the node, and frees the item.
 *   Free    - frees an object.  it is assumed that the object is
 *             not presently attached to any list.
 *
 * Basic Rules
 *   1) No node shall exist in the list that does not contain
 *      an item.  Newly created nodes must have their item attached
 *      before the node is attached to the list.
 *   2) No node shall be freed while it is still attached
 *      to the list.
 *   3) No items shall be freed while still attached to a
 *      node which is still attached to a list.
 *
 * The following table show how the higher-level (public) functions are built
 * upon the lower-level base functions.  Note that the higher-level functions
 * act upon the item, and never the node.
 *
 * Higher-Level Abstraction (public functions)
 *
 *    High-Level       Low-level steps
 *    ----------       ---------------------------
 *    New (list)       create new list
 *
 *    Append (item)    create new node
 *                     attach item to node
 *                     attach node to list(s)
 *
 *    Retrieve (item)  locate item (based on match token)
 *
 *    Remove (item)    locate node (based on item)
 *                     detach node from list
 *                     free node
 *
 *    Delete (item)    locate node (based on item)
 *                     detach node from list
 *                     free item
 *                     free node
 *  
 *    Free (item)      free item
 *
 *    (using the terminology "delete" may seem confusing.  Since we're
 *     freeing the node and the item, it might make more sense to call this
 *     command "destroy".  But remember, high-level functions act on items,
 *     not nodes.  So the terminology here is correct.)
 *
 * Each of the low-level steps are independent functions, summarized below.
 * The functions that are 'based on item' take a pointer to the item as the
 * parameter, as opposed to taking the match token.  Only the routine
 * locate_item() takes a match token as the parameter.  Other functions deal
 * with pointers to the item.  It is assumed that the user will first locate
 * the item by matching it, then using the item to remove or delete it.
 * 
 *     create new list
 *     create new node
 *     attach item to node
 *     attach node to list
 *     detach node from list
 *     locate node (based on item)
 *     locate item (based on match token)
 *     free node
 *     free item
 *     free list
 *
 * Let's get busy.
 * ------------------------------------------------------------------------- */

/* Node structure */
struct listnode {
	struct listnode *next;
	struct listnode *next_sorted;
	
	void *item;  /* -> data */
} ;

/* Prototypes for internal commands */
static listmgr_t *create_new_list();
static struct listnode *create_new_node();
static void attach_item_to_node(listmgr_t *L, struct listnode *node, void *x);
static void attach_node_to_list(listmgr_t *L, struct listnode *node);
static void attach_node_to_slist(listmgr_t *L, struct listnode *node);
static void detach_node_from_list(listmgr_t *L, struct listnode *node);
static void detach_node_from_slist(listmgr_t *L, struct listnode *node);
static struct listnode *locate_node(listmgr_t *L, void *item);
static void *locate_item(listmgr_t *L, void *match_token);
static void *locate_item_peek(listmgr_t *L, void *match_token);
static void free_node(listmgr_t *L, struct listnode *node);
static void free_item(listmgr_t *L, void *item);
static void free_list(listmgr_t *L);
static void move_to_front(listmgr_t *L, struct listnode *node, struct listnode *lastchecked);
static void sort_list(listmgr_t *L, int direction);
static struct listnode *iterator_begin(listmgr_t *L, int id, int sorted);
static struct listnode *iterator_next(listmgr_t *L, int id, int sorted);
#ifdef DEBUG_STRICT
static void dump_list(listmgr_t *L);
#endif

/* private flags */
#define LISTMGR_ALT_MISSING_ITEMS  0x0001 /* alt list has had items removed */
#define LISTMGR_ALT_SORTED         0x0002 /* alt list is already sorted... */
#define LISTMGR_SORTED_ASCENDING   0x0004 /* ...in ascending direction */
#define LISTMGR_SORTED_DESCENDING  0x0008 /* ...in descending direction */


static listmgr_t *create_new_list()
{
	listmgr_t *x;
	int i;

	x = (listmgr_t *)xmalloc(sizeof(listmgr_t));
	if(x == NULL)
		return NULL;
	x->head = NULL;
	x->head_sorted = NULL;
	x->tail = NULL;
	x->tail_sorted = NULL;

	for(i = 0; i < LISTMGR_NUM_CUR_NODES; i++)
		x->cur_node[i] = NULL;

	x->free = NULL;
	x->match = NULL;
	x->compare = NULL;
	x->length = 0;
	x->length_sorted = 0;

	x->flags = 0;
	return x;
}


static struct listnode *create_new_node()
{
	struct listnode *node;

	/* Create and setup node that will hold data */
	node = (struct listnode *)xmalloc(sizeof(struct listnode));
	if(node == NULL)
		return NULL;

	node->next = NULL;
	node->next_sorted = NULL;
	node->item = NULL;

	return node;
}


static void attach_item_to_node(listmgr_t *L, struct listnode *node, void *x)
{
#ifdef DEBUG_STRICT
	if(node->item != NULL)
		internal("listmgr: in priv attach_item_to_node(), node already has item.");
#endif

	node->item = x;
}


static void attach_node_to_list(listmgr_t *L, struct listnode *node)
{
#ifdef DEBUG_STRICT
	struct listnode *n;
	for(n = L->head; n; n = n->next)
	{
		if(n == node)
			internal("listmgr: in priv attach_node_to_list(), node already attached.");
		if(n->item == node->item)
			internal("listmgr: in priv attach_node_to_list(), item already contained in another node.");
	}
#endif

	/* Append node to list (take care on empty lists) */
	if(L->head == NULL)
		L->head = node;
	else
		L->tail->next = node;

	L->tail = node;
	L->length++;
}


static void attach_node_to_slist(listmgr_t *L, struct listnode *node)
{
#ifdef DEBUG_STRICT
	struct listnode *n;
	for(n = L->head_sorted; n; n = n->next_sorted)
	{
		if(n == node)
			internal("listmgr: in priv attach_node_to_slist(), node already attached.");
		/*
		 * Don't do a check to see if item is already attached, since
		 * it better be!  This is the alternate list...  Items are
		 * already attached to nodes.
		 */
	}
#endif

	/* Append node to sorted list */
	if(L->head_sorted == NULL)
		L->head_sorted = node;
	else
		L->tail_sorted->next_sorted = node;

	L->tail_sorted = node;
	L->length_sorted++;
	L->flags &= ~LISTMGR_ALT_SORTED;
}


static void detach_node_from_list(listmgr_t *L, struct listnode *node)
{
	struct listnode *n, *lastchecked = NULL;

	n = L->head;
	while(n != NULL)
	{
		if(n == node)
		{
			/* If node is first in list */
			if(lastchecked == NULL)
			{
				L->head = n->next;
				if(L->tail == n)
					L->tail = NULL;
			}
			else
			{
				lastchecked->next = n->next;
				if(n == L->tail)
					L->tail = lastchecked;
			}

			L->length--;
			return;
		}

		lastchecked = n;
		n = n->next;
	}

	internal("listmgr: in priv detach_node_from_list(), node not found.");
}


static void detach_node_from_slist(listmgr_t *L, struct listnode *node)
{
	struct listnode *n, *lastchecked = NULL;

	L->flags &= ~LISTMGR_ALT_SORTED;

	n = L->head_sorted;
	while(n != NULL)
	{
		if(n == node)
		{
			/* If node is first in list */
			if(lastchecked == NULL)
			{
				L->head_sorted = n->next_sorted;
				if(L->tail_sorted == n)
					L->tail_sorted = NULL;
			}
			else
			{
				lastchecked->next_sorted = n->next_sorted;
				if(n == L->tail_sorted)
					L->tail_sorted = lastchecked;
			}

			L->length_sorted--;
			return;
		}

		lastchecked = n;
		n = n->next_sorted;
	}

	internal("listmgr: in priv detach_node_from_slist(), node not found.");
}


static struct listnode *locate_node(listmgr_t *L, void *item)
{
	struct listnode *node;

	for(node = L->head; node != NULL; node = node->next)
	{
		if(node->item == item)
			return node;
	}

	return NULL;
}


static void *locate_item(listmgr_t *L, void *match_token)
{
	struct listnode *node, *lastchecked = NULL;

	/* No match function defined */
	if(L->match == NULL)
		return NULL;

	for(node = L->head; node != NULL; node = node->next)
	{
		if((L->match)(node->item, match_token))
		{
			move_to_front(L, node, lastchecked);
			return node->item;
		}
		lastchecked = node;
	}

	return NULL;
}


static void *locate_item_peek(listmgr_t *L, void *match_token)
{
	struct listnode *node;

	/* No match function defined */
	if(L->match == NULL)
		return NULL;

	for(node = L->head; node != NULL; node = node->next)
	{
		if((L->match)(node->item, match_token))
			return node->item;
	}

	return NULL;
}


static void free_node(listmgr_t *L, struct listnode *node)
{
#ifdef DEBUG_STRICT
	struct listnode *n;
	for(n = L->head; n; n = n->next)
	{
		if(n == node)
			internal("listmgr: in priv free_node(), node still attached to list.");
	}
#endif

	xfree(node);
}


static void free_item(listmgr_t *L, void *item)
{
#ifdef DEBUG_STRICT
	struct listnode *node;
	for(node = L->head; node; node = node->next)
	{
		if(node->item == item)
			internal("listmgr: in priv free_item(), item still attached to node.");
	}
#endif

	if(item == NULL)
	{
		internal("listmgr: in priv free_item(), item is NULL.");
		return;
	}

	if(L->free)
		(L->free)(item);
}


static void free_list(listmgr_t *L)
{
	xfree(L);
}


static void move_to_front(listmgr_t *L, struct listnode *node, struct listnode *lastchecked)
{
	if(lastchecked != NULL)
	{
		if(L->tail == node)
			L->tail = lastchecked;

		lastchecked->next = node->next;
		node->next = L->head;
		L->head = node;
	}
}


static void sort_list(listmgr_t *L, int direction)
/*
 * direction:  1 = forward
 *            -1 = reverse
 *             0 = don't sort
 */
{
	int sorted = 0, compare;
	struct listnode *node, *node_tmp, *lastchecked = NULL;

	if(L->flags & LISTMGR_ALT_SORTED)
	{
		if(direction > 0 && (L->flags & LISTMGR_SORTED_ASCENDING))
			return;
		else if(direction < 0 && (L->flags & LISTMGR_SORTED_DESCENDING))
			return;
	}

	/*
	 * Quickly re-build the alternate list if items have been removed.
	 */
	if(L->flags & LISTMGR_ALT_MISSING_ITEMS)
	{
		for(node = L->head; node; node = node->next)
			node->next_sorted = node->next;

		L->length_sorted = L->length;
		L->head_sorted = L->head;
		L->tail_sorted = L->tail;

		L->flags &= ~LISTMGR_ALT_MISSING_ITEMS;
	}

	if(L->compare == NULL)
		return;

#ifdef MSK
	/* hand this function an empty list, and it cores */
	if (L->length == 0)
		return;
#endif

	/* bubble sort for now */
	while(!sorted)
	{
		sorted = 1;
		lastchecked = NULL;
		for(node = L->head_sorted; node->next_sorted; node = node->next_sorted)
		{
			compare = (L->compare)(node->item, node->next_sorted->item);
			if((direction > 0 && compare > 0) || (direction < 0 && compare < 0))
			{
				sorted = 0;

				if(lastchecked == NULL)
					L->head_sorted = node->next_sorted;
				else
					lastchecked->next_sorted = node->next_sorted;

				node_tmp = node->next_sorted->next_sorted;
				node->next_sorted->next_sorted = node;
				node->next_sorted = node_tmp;

				/* check this record again */
				if(lastchecked == NULL)
				{
					if(L->head_sorted->next_sorted->next_sorted != NULL)
						node = L->head_sorted->next_sorted;
					else
						node = L->head_sorted;
				}
				else
					node = lastchecked->next_sorted;
			}
			lastchecked = node;
		}
	}

	/* Update tail */
	for(node = L->head_sorted; node->next_sorted; node = node->next_sorted)
		;

	L->tail_sorted = node;

	L->flags |= LISTMGR_ALT_SORTED;
	if(direction > 0)
		L->flags |= LISTMGR_SORTED_ASCENDING;
	else
		L->flags |= LISTMGR_SORTED_DESCENDING;
}


/*
 * Note that these private functions return the node, not the item. It's up
 * to the high-level (listmgr) functions to pass back the node's item.  It's
 * also up to the high-level (listmgr) functions to determine an appropriate
 * id to use for the differnet iterators (nested, sorted).
 */
static struct listnode *iterator_begin(listmgr_t *L, int id, int sorted)
{
	if(id < 0 || id >= LISTMGR_NUM_CUR_NODES)
	{
		internal("listmgr: in priv iterator_begin(), id=%d exceeds NUM_CUR_NODES.",id);
		return NULL;
	}

	if(sorted)
		L->cur_node[id] = L->head_sorted;
	else
		L->cur_node[id] = L->head;

	return L->cur_node[id];
}


static struct listnode *iterator_next(listmgr_t *L, int id, int sorted)
{
	if(id < 0 || id >= LISTMGR_NUM_CUR_NODES)
	{
		internal("listmgr: in priv iterator_next(), id=%d exceeds NUM_CUR_NODES.",id);
		return NULL;
	}

	if(L->cur_node[id] == NULL)
	{
		internal("listmgr: in priv iterator_next(), cur_node[%d] is NULL. (next() before begin()?)",id);
		return NULL;
	}

	if(sorted)
		L->cur_node[id] = L->cur_node[id]->next_sorted;
	else
		L->cur_node[id] = L->cur_node[id]->next;

	return L->cur_node[id];
}


#ifdef DEBUG_STRICT
static void dump_list(listmgr_t *L)
{
	struct listnode *node;

	debug("Dumping list. (%d items)",listmgr_length(L));
	for(node = L->head; node; node = node->next)
		debug("node: %16lu	item: %16lu", node, node->item);
}
#endif




/* -------------------------------------------------------------------------
 * Higher-Level Abstraction (public functions, same table as above)
 *
 *    High-Level       Low-level steps
 *    ----------       ---------------------------
 *    New (list)       create new list
 *
 *    Append (item)    create new node
 *                     attach item to node
 *                     attach node to list(s)
 *
 *    Retrieve (item)  locate item (based on match token)
 *
 *    Remove (item)    locate node (based on item)
 *                     detach node from list
 *                     free node
 *
 *    Delete (item)    locate node (based on item)
 *                     detach node from list
 *                     free item
 *                     free node
 *  
 *    Free (item)      free item
 *
 * ------------------------------------------------------------------------- */


listmgr_t *listmgr_new_list(int flags)
/*
 * Function: Creates a new list
 *  Returns: pointer to a new list
 *           NULL if list could not be created
 *
 *  Upon receiving a new list, users should initialize member
 *  functions such as match() and compare().
 *
 *  Flags are used to specify how the list will operate.  Flags can be
 *
 *    LM_NOMOVETOFRONT   - list is never optimized
 *
 */
{
	listmgr_t *L = create_new_list();

	if(L == NULL)
		fatal_error("listmgr: in listmgr_new_list(), unable to malloc.");

	L->flags = flags;
	return L;
}


void listmgr_destroy_list(listmgr_t *L)
/*
 * Function: Destroy entire list and free all items
 *
 *  Use this function to destroy entire list and all items.  The
 *  previous pointer to the list will become invalid.
 */
{
	struct listnode *node, *next_node;

	if(L == NULL)
	{
		internal("listmgr: in destroy_list(), list is NULL.");
		return;
	}

	node = L->head;
	while(node != NULL)
	{
		next_node = node->next;
		free_item(L, node->item);
		free_node(L, node);

		node = next_node;
	}

	free_list(L);
}


void listmgr_delete_list(listmgr_t *L)
/*
 * Function: Destroy list but preserve items
 *
 *  Use this function when you need to destroy a list, and want
 *  to leave all items intact.
 */
{
	struct listnode *node, *next_node;

	if(L == NULL)
	{
		internal("listmgr: in delete_list(), list is NULL.");
		return;
	}

	node = L->head;
	while(node != NULL)
	{
		next_node = node->next;
		free_node(L, node);

		node = next_node;
	}

	free_list(L);
}


void listmgr_append(listmgr_t *L, void *item)
/*
 * Function: Appends an item of any type to the list.
 *
 *  The item is appended to both the main list (optimal sorting)
 *  and the alternate list (user sorting).  This allows us to continue
 *  using the user-sorted version without worrying that newly appended
 *  items will be missing.  The list will no longer be sorted though.
 */
{
	struct listnode *node = create_new_node();

	if(L == NULL || node == NULL || item == NULL)
	{
		if(L == NULL)
			internal("listmgr: in append(), list is NULL.");
		if(node == NULL)
			fatal_error("listmgr: in append(), node not malloc'd.");
		if(item == NULL)
			internal("listmgr: in append(), item is NULL.");
		return;
	}

	attach_item_to_node(L, node, item);
	attach_node_to_list(L, node);
	attach_node_to_slist(L, node);
}


int listmgr_isitem(listmgr_t *L, void *match_token)
/*
 * Function: check if an item exists, based on 'match_token'
 *  Returns: 1 if item exists
 *           0 if item does not exist
 *
 *  One should check if an item exists in the list before attempting
 *  to remove, delete, or retrieve it.  We are guaranteed by the
 *  self-optimizing property of the list that the item will be
 *  available immediately on the next search.
 */
{
	void *item;

	if(L == NULL || match_token == NULL)
	{
		if(L == NULL)
			internal("listmgr: in isitem(), list is NULL.");
		if(match_token == NULL)
			internal("listmgr: in isitem(), match_token is NULL.");
		return 0;
	}

	if(L->match == NULL)
	{
		internal("listmgr: in isitem(), no match function set.");
		return 0;
	}

	if(L->flags & LM_NOMOVETOFRONT)
		item = locate_item_peek(L, match_token);
	else
		item = locate_item(L, match_token);

	return item == NULL ? 0 : 1;
}


int listmgr_isitem_peek(listmgr_t *L, void *match_token)
/*
 * Function: Peek to see if an item exists, based on 'match_token'
 *  Returns: 1 if item exists
 *           0 if item does not exist
 *
 *  Use this function only if you do not want the list to optimize
 *  itself based on this search.  This means that you do not want this
 *  item moved to the front of the list.
 */
{
	if(L == NULL || match_token == NULL)
	{
		if(L == NULL)
			internal("listmgr: in isitem_peek(), list is NULL.");
		if(match_token == NULL)
			internal("listmgr: in isitem_peek(), match_token is NULL.");
		return 0;
	}

	if(L->match == NULL)
	{
		internal("listmgr: in isitem_peek(), no match function set.");
		return 0;
	}

	if(locate_item_peek(L, match_token) != NULL)
		return 1;
	else
		return 0;
}


void *listmgr_retrieve(listmgr_t *L, void *match_token)
/*
 * Function: Retrieves the item, based on 'match_token'
 *  Returns: pointer to the item
 *           NULL if item was not found, or if no match function is present
 *
 *  Returns the item which matches the match_token paramater.  The match
 *  function must be provided or the routine will generate an error.
 */
{
	if(L == NULL || match_token == NULL)
	{
		if(L == NULL)
			internal("listmgr: in retrieve(), list is NULL.");
		if(match_token == NULL)
			internal("listmgr: in retrieve(), match_token is NULL.");
		return NULL;
	}

	if(L->match == NULL)
	{
		internal("listmgr: in retrieve(), no match function set.");
		return NULL;
	}

	if(L->flags & LM_NOMOVETOFRONT)
		return locate_item_peek(L, match_token);
	else
		return locate_item(L, match_token);
}


void *listmgr_peek(listmgr_t *L, void *match_token)
/*
 * Function: Peeks at the item, based on 'match_token, but doesn't optimize
 *  Returns: pointer to the item
 *           NULL if item was not found, or if no match function is present
 *
 *  Use this function if you want to retrieve the item but do not want
 *  the list to optimize itself based on this search.  This means that
 *  you do not want this item moved to the front of the list.
 */
{
	if(L == NULL || match_token == NULL)
	{
		if(L == NULL)
			internal("listmgr: in peek(), list is NULL.");
		if(match_token == NULL)
			internal("listmgr: in peek(), match_token is NULL.");
		return NULL;
	}

	if(L->match == NULL)
	{
		internal("listmgr: in peek(), no match function set.");
		return NULL;
	}

	return locate_item_peek(L, match_token);
}
 	

void listmgr_remove(listmgr_t *L, void *item)
/*
 * Function: Removes an item from the list
 *
 *  Use this when you want to remove an item from the list but still
 *  preserve the item (ie. you don't want to free the item).  Removes
 *  the item from both the main list and alternate list.
 */
{
	struct listnode *node = locate_node(L, item);

	if(L == NULL || item == NULL || node == NULL)
	{
		if(L == NULL)
			internal("listmgr: in remove(), list is NULL.");
		if(item == NULL)
			internal("listmgr: in remove(), item is NULL.");
		if(node == NULL)
			internal("listmgr: in remove(), node not found.");
		return;
	}

	detach_node_from_list(L, node);
	detach_node_from_slist(L, node);
	free_node(L, node);
}


void listmgr_remove_from_sorted(listmgr_t *L, void *item)
/*
 * Function: Removes an item from the secondary list only
 *
 *  Use this when you want to remove an item from the secondary
 *  list only.  Useful for times when you need a sorted list minus a
 *  a few entries.  Re-sorting the list automatically attaches the
 *  missing entries.
 */
{
	struct listnode *node = locate_node(L, item);

	if(L == NULL || item == NULL || node == NULL)
	{
		if(L == NULL)
			internal("listmgr: in remove_from_sorted(), list is NULL.");
		if(item == NULL)
			internal("listmgr: in remove_from_sorted(), item is NULL.");
		if(node == NULL)
			internal("listmgr: in remove_from_sorted(), node not found.");
		return;
	}

	detach_node_from_slist(L, node);

	/*
	 * Don't be tempted to free the node!  The main list is still using it.
	 * Think of the sorted list as an alternate list, which doesn't free
	 * anything.  We leave that up to the main list.
	 */
	L->flags |= LISTMGR_ALT_MISSING_ITEMS;
}


void listmgr_delete(listmgr_t *L, void *item)
/*
 * Function: Removes and frees an item from the list
 *
 *  Use this when you want to remove an item from the secondary
 *  list only.  Useful for times when you need a sorted list minus a
 *  a few entries.  Re-sorting the list automatically attaches the
 *  missing entries.
 */
{
	struct listnode *node = locate_node(L, item);

	if(L == NULL || item == NULL || node == NULL)
	{
		if(L == NULL)
			internal("listmgr: in delete(), list is NULL.");
		if(item == NULL)
			internal("listmgr: in delete(), item is NULL.");
		if(node == NULL)
			internal("listmgr: in delete(), node not found.");
		return;
	}

	detach_node_from_list(L, node);
	detach_node_from_slist(L, node);
	free_item(L, node->item);
	free_node(L, node);
}


void listmgr_sort_ascending(listmgr_t *L)
/*
 * Function: Sort a list in ascending order
 *
 *  Use this when you want to sort the list.  The list is sorted
 *  according to the provided compare funciton.  This function can
 *  be swapped out to sort by different fields, for example.
 *
 *  Missing items are attached automatically when the sort function
 *  is called.  Missing items occur when the remove_from_sorted()
 *  function is called.
 */
{
	if(L == NULL)
	{
		internal("listmgr: in sort_ascending(), list is NULL.");
		return;
	}

	if(L->compare == NULL)
		internal("listmgr: in sort_ascending(), no compare function set.");

	sort_list(L, 1);
}


void listmgr_sort_descending(listmgr_t *L)
/*
 * Function: Sort a list in descending order
 *
 *  Like above, but sorts in descending order.
 */
{
	if(L == NULL)
	{
		internal("listmgr: in sort_descending(), list is NULL.");
		return;
	}

	if(L->compare == NULL)
		internal("listmgr: in sort_descending(), no compare function set.");

	sort_list(L, -1);
}


void *listmgr_begin(listmgr_t *L)
/*
 * Function: Begin iterating
 *  Returns: Item at the head of the list
 *
 *  To perform nested iterations, use the begin_nested() routine.
 *
 *  begin() uses id 0
 *  begin_nested() uses ids 1 ... NUM_CUR_NODES-2
 *  begin_sorted() uses id NUM_CUR_NODES-1
 */
{
	struct listnode *node;

	if(L == NULL)
	{
		internal("listmgr: in begin(), list is NULL.");
		return NULL;
	}

	/* L, id, sorted */
	node = iterator_begin(L, 0, 0);

	/* it's ok if node is NULL.  Just signifies an empty list. */
	if(node == NULL)
		return NULL;
	else
		return node->item;
}


void *listmgr_begin_nested(listmgr_t *L, int id)
/*
 * Function: Begin a nested iteration
 *  Returns: Item at the head of the list
 *
 *  begin() uses id 0
 *  begin_nested() uses ids 1 ... NUM_CUR_NODES-2
 *  begin_sorted() uses id NUM_CUR_NODES-1
 */
{
	struct listnode *node;

	if(L == NULL)
	{
		internal("listmgr: in begin_nested(), list is NULL.");
		return NULL;
	}

	/* check range */
	if(id < 1 || id > (LISTMGR_NUM_CUR_NODES-2))
	{
		internal("listmgr: in begin_nested(), id=%d outside range.",id);
		return NULL;
	}

	/* L, id, sorted */
	node = iterator_begin(L, id, 0);

	/* it's ok if node is NULL.  Just signifies an empty list. */
	if(node == NULL)
		return NULL;
	else
		return node->item;
}


void *listmgr_begin_sorted(listmgr_t *L)
/*
 * Function: Begin a sorted iteration
 *  Returns: Item at the head of the list
 *
 *  begin() uses id 0
 *  begin_nested() uses id's 1...NUM_CUR_NODES-2
 *  begin_sorted() uses id NUM_CUR_NODES-1
 */
{
	struct listnode *node;

	if(L == NULL)
	{
		internal("listmgr: in begin_sorted(), list is NULL.");
		return NULL;
	}

	/* L, id, sorted */
	node = iterator_begin(L, LISTMGR_NUM_CUR_NODES-1, 1);

	/* it's ok if node is NULL.  Just signifies an empty list. */
	if(node == NULL)
		return NULL;
	else
		return node->item;
}


void *listmgr_next(listmgr_t *L)
/*
 * Function: Continue a sorted iteration
 *  Returns: Next item in list
 *
 *  begin() uses id 0
 *  begin_nested() uses ids 1 ... NUM_CUR_NODES-2
 *  begin_sorted() uses id NUM_CUR_NODES-1
 */
{
	struct listnode *node;

	if(L == NULL)
	{
		internal("listmgr: in next(), list is NULL.");
		return NULL;
	}

	/* L, id, sorted */
	node = iterator_next(L, 0, 0);

	if(node == NULL)
		return NULL;
	else
		return node->item;
}


void *listmgr_next_nested(listmgr_t *L, int id)
/*
 * Function: Continues a nested iteration
 *  Returns: Next item in list
 *
 *  begin() uses id 0
 *  begin_nested() uses ids 1 ... NUM_CUR_NODES-2
 *  begin_sorted() uses id NUM_CUR_NODES-1
 */
{
	struct listnode *node;

	if(L == NULL)
	{
		internal("listmgr: in next_nested(), list is NULL.");
		return NULL;
	}

	/* check range */
	if(id < 1 || id > (LISTMGR_NUM_CUR_NODES-2))
	{
		internal("listmgr: in next_nested(), id=%d outside range.",id);
		return NULL;
	}

	/* L, id, sorted */
	node = iterator_next(L, id, 0);

	if(node == NULL)
		return NULL;
	else
		return node->item;
}


void *listmgr_next_sorted(listmgr_t *L)
/*
 * Function: Continue a sorted iteration
 *  Returns: Next item in list
 *
 *  begin() uses id 0
 *  begin_nested() uses id's 1...NUM_CUR_NODES-2
 *  begin_sorted() uses id NUM_CUR_NODES-1
 */
{
	struct listnode *node;

	if(L == NULL)
	{
		internal("listmgr: in next_sorted(), list is NULL.");
		return NULL;
	}

	/* L, id, sorted */
	node = iterator_next(L, LISTMGR_NUM_CUR_NODES-1, 1);

	if(node == NULL)
		return NULL;
	else
		return node->item;
}


void listmgr_set_match_func(listmgr_t *L, void *func)
/*
 * Function: Set the list's match function
 *
 *  The match function takes a pointer to an item, and a pointer
 *  to a match_token.  It returns 1 if the item matches, and 0
 *  if it doesn't match.
 */
{
	if(L == NULL || func == NULL)
	{
		if(L == NULL)
			internal("listmgr: in set_match_func(), list is NULL.");
		if(func == NULL)
			internal("listmgr: in set_match_func(), func is NULL.");
		return;
	}

	L->match = func;
}


void listmgr_set_compare_func(listmgr_t *L, void *func)
/*
 * Function: Set the list's compare function
 *
 *  The compare function takes a pointer to item1, and a pointer
 *  to item2.  It returns 1 if item1 is greater than item2, 0 if
 *  item1 is equal to item2, and -1 if item1 is less than item2.
 */
{
	if(L == NULL || func == NULL)
	{
		if(L == NULL)
			internal("listmgr: in set_compare_func(), list is NULL.");
		if(func == NULL)
			internal("listmgr: in set_compare_func(), func is NULL.");
		return;
	}

	L->compare = func;
}


void listmgr_set_free_func(listmgr_t *L, void *func)
/*
 * Function: Set the list's free function
 *
 *  The free function takes a pointer to the item to be freed.  It
 *  returns void.
 */
{
	if(L == NULL || func == NULL)
	{
		if(L == NULL)
			internal("listmgr: in set_free_func(), list is NULL.");
		if(func == NULL)
			internal("listmgr: in set_free_func(), func is NULL.");
		return;
	}

	L->free = func;
}


int listmgr_length(listmgr_t *L)
/*
 * Function: Determine size of list
 *  Returns: size (0 if list is NULL)
 *
 */
{
	if(L == NULL)
	{
		internal("listmgr: in length(), list is NULL.");
		return 0;
	}

	return L->length;
}


int listmgr_length_sorted(listmgr_t *L)
/*
 * Function: Determine size of sorted list
 *  Returns: size (0 if sorted list is NULL)
 *
 *  Sorted list may be of different size than main list if the function
 *  remove_from_sorted() is used.
 */
{
	if(L == NULL)
	{
		internal("listmgr: in length_sorted(), list is NULL.");
		return 0;
	}

	return L->length_sorted;
}




#ifdef TEST
/* -------------------------------------------------------------------------
 * TESTING
 *
 * Some tests, mainly to make sure we don't hose anything.  Doesn't do
 * memory leak tests.  Yet...
 *
 * Note that order matters, ie. don't swap around the verify_append() and
 * verify_delete().  The verify routines are coupled fairly closely together.
 *
 * All tests should perform OK, and no warnings should occur.  Compile with
 *    gcc -o listmgr listmgr.c -DTEST
 * ------------------------------------------------------------------------- */

struct dummy_item {
	int numeric_id;
	char string_id[32];
};

int dummy_match_numeric_id(struct dummy_item *item, int *match_token)
{
	return item->numeric_id == *match_token;
}

int dummy_match_string_id(struct dummy_item *item, char *match_token)
{
	return strcmp(item->string_id, match_token) == 0;
}

int dummy_compare_numeric_id(struct dummy_item *item1, struct dummy_item *item2)
{
	if(item1->numeric_id > item2->numeric_id)
		return 1;
	if(item1->numeric_id < item2->numeric_id)
		return -1;
	return 0;
}

int verify_append(listmgr_t *L)
{
	struct dummy_item *d;
	struct listnode *node;
	int i, ok = 0, ok_test = 0;

	/* Initialize dummy objects and attach them to the list */
	for(i = 0; i < 50; i++)
	{
		d = (struct dummy_item *)xmalloc(sizeof(struct dummy_item));
		d->numeric_id = i;
		sprintf(d->string_id,"%d",i);

		listmgr_append(L, d);
	}

	/* Verify that items are in forward order */
	for(node = L->head, i = 0; node; node = node->next, i++)
	{
		d = node->item;
		if(d->numeric_id == i)
			ok_test++;
		ok++;
	}

	/* Verify length of list */
	ok_test += listmgr_length(L);
	ok += 50;

	return ok == ok_test;
}

int verify_match(listmgr_t *L)
{
	struct dummy_item *d;
	int i, ok = 0, ok_test = 0;
	char buf[20];

	/* Check numeric matching function */
	listmgr_set_match_func(L, dummy_match_numeric_id);
	for(i = 0; i < 50; i++)
	{
		d = listmgr_retrieve(L, &i);
		if(d != NULL && d->numeric_id == i)
			ok_test++;
		ok++;
	}

	/* Check string matching function */
	listmgr_set_match_func(L, dummy_match_string_id);
	for(i = 0; i < 50; i++)
	{
		sprintf(buf,"%d",i);

		d = listmgr_retrieve(L, buf);
		if(d != NULL && strcmp(d->string_id,buf) == 0)
			ok_test++;
		ok++;
	}

	return ok == ok_test;
}

int verify_move_to_front(listmgr_t *L)
{
	struct dummy_item *d;
	int i, ok = 0, ok_test = 0;

	/* Retrieve an item, then verify that it has been moved to front */
	listmgr_set_match_func(L, dummy_match_numeric_id);
	for(i = 0; i < 50; i += 5)
	{
		d = listmgr_retrieve(L, &i);
		if(listmgr_length(L) > 0 && L->head->item == d)
			ok_test++;
		ok++;
	}

	return ok == ok_test;
}

int verify_isitem(listmgr_t *L)
{
	int i, ok = 0, ok_test = 0;

	for(i = 0; i < 100; i++)
	{
		if(listmgr_isitem(L, &i))
			ok_test++;
	}
	ok += 50;	/* only 50 of the 100 should match TRUE */

	return ok == ok_test;
}

int verify_remove(listmgr_t *L)
{
	struct dummy_item *d[50];
	int i, ok = 0, ok_test = 0;

	/* Set match function */
	listmgr_set_match_func(L, dummy_match_numeric_id);
	for(i = 0; i < 25; i++)
	{
		d[i] = listmgr_retrieve(L, &i);
		if(d[i] != NULL)
			listmgr_remove(L, d[i]);
	}

	/* Verify length of both lists (will be 25 after removing items) */
	if(listmgr_length(L) == 25 && listmgr_length_sorted(L) == 25)
		ok_test++;
	ok++;

	/* Verify that removed items are still OK */
	for(i = 0; i < 25; i++)
	{
		if(d[i]->numeric_id == i)
			ok_test++;
		ok++;
	}

	/* Verify that the remaining 25 items are still in the list */
	for(i = 25; i < 50; i++)
	{
		d[i] = listmgr_retrieve(L, &i);
		if(d[i] != NULL)
			ok_test++;
		ok++;
	}

	/* Append back the original entries */
	for(i = 0; i < 25; i++)
		listmgr_append(L, d[i]);

	if(listmgr_length(L) == 50)
		ok_test++;
	ok++;

	return ok == ok_test;
}

int verify_remove_from_sorted(listmgr_t *L)
{
	struct dummy_item *d;
	int i, ok = 0, ok_test = 0;

	/* Retrieve and remove the 1st 25 entries from the sorted list only. */
	listmgr_set_match_func(L, dummy_match_numeric_id);
	for(i = 0; i < 25; i++)
	{
		d = listmgr_retrieve(L, &i);
		if(d != NULL)
			listmgr_remove_from_sorted(L, d);
	}

	/* Verify length of both lists */
	if(listmgr_length(L) == 50 && listmgr_length_sorted(L) == 25)
		ok_test++;
	ok++;

	return ok == ok_test;
}

int verify_sorting(listmgr_t *L)
{
	struct dummy_item *d;
	struct listnode *node;
	int i, ok = 0, ok_test = 0;

	/* Prepare to sort */
	listmgr_set_compare_func(L, dummy_compare_numeric_id);
	listmgr_sort_ascending(L);

	/*
	 * Only 25 items in the sort list coming in.  Verify that missing
	 * entries are attached.
	 */
	if(listmgr_length(L) == 50 && listmgr_length_sorted(L) == 50)
		ok_test++;
	ok++;

	for(node = L->head_sorted, i = 0; node; node = node->next_sorted, i++)
	{
		d = node->item;
		if(d->numeric_id == i)
			ok_test++;
		ok++;
	}

	listmgr_sort_descending(L);
	for(node = L->head_sorted, i = 49; node; node = node->next_sorted, i--)
	{
		d = node->item;
		if(d->numeric_id == i)
			ok_test++;
		ok++;
	}

	return ok == ok_test;
}

int verify_iterators(listmgr_t *L)
{
	struct dummy_item *d, *d1;
	struct listnode *node;
	int i, j, ok = 0, ok_test = 0;
	void *array[50];

	/* build the array, then check against it using the iterators */
	for(node = L->head, i = 0; node; node = node->next, i++)
		array[i] = node->item;

	for(d = listmgr_begin(L), i = 0; d; d = listmgr_next(L), i++)
	{
		if(d == array[i])
			ok_test++;
		ok++;

		/* try a nested loop */
		for(d1 = listmgr_begin_nested(L,1), j = 0; d1; d1 = listmgr_next_nested(L,1), j++)
		{
			if(d1 == array[j])
				ok_test++;
			ok++;
		}
	}

	/* check sorted */
	for(node = L->head_sorted, i = 0; node; node = node->next_sorted, i++)
		array[i] = node->item;

	for(d = listmgr_begin_sorted(L), i = 0; d; d = listmgr_next_sorted(L), i++)
	{
		if(d == array[i])
			ok_test++;
		ok++;
	}

	return ok == ok_test;
}

int main()
{
	listmgr_t *L = listmgr_new_list(0);
	int ok1 = 0, ok2 = 0, ok3 = 0, ok4 = 0, ok5 = 0, ok6 = 0, ok7 = 0, ok8 = 0;

	printf("Testing module: listmgr\n");
	printf("  %s\n",
		(ok1 = verify_append(L)) ? "append: ok" : "append: failed");
	printf("  %s\n",
		(ok2 = verify_match(L)) ? "match: ok" : "match: failed");
	printf("  %s\n",
		(ok3 = verify_move_to_front(L)) ? "move_to_front: ok" : "move_to_front: failed");
	printf("  %s\n",
		(ok4 = verify_isitem(L)) ? "isitem: ok" : "isitem: failed");
	printf("  %s\n",
		(ok5 = verify_remove(L)) ? "remove: ok" : "remove: failed");
	printf("  %s\n",
		(ok6 = verify_remove_from_sorted(L)) ? "remove_from_sorted: ok" : "remove_from_sorted: failed");
	printf("  %s\n",
		(ok7 = verify_sorting(L)) ? "sort: ok" : "sort: failed");
	printf("  %s\n",
		(ok8 = verify_iterators(L)) ? "iterators: ok" : "iterators: failed");

	if(ok1 && ok2 && ok3 && ok4 && ok5 && ok6 && ok7 && ok8)
		printf("listmgr: passed\n");
	else
		printf("listmgr: failed\n");

	printf("\n");
	return 0;
}

#endif /* TEST */
