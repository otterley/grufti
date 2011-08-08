/* -------------------------------------------------------------------------
 * Steve Rider - stever@area23.org
 * Grufti - an interactive, service-oriented bot for IRC 
 *
 * state.c - general state of system tracking
 *
 * ------------------------------------------------------------------------- */
/* 9 July 98 */

#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "grufti.h"
#include "listmgr.h"
#include "state.h"

#ifdef TEST
#define STANDALONE
#endif

#ifdef STANDALONE  /* requires listmgr.c, xmem.c */

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

#endif /* STANDALONE */


static listmgr_t *states;

struct statetbl_node {
	char name[20];
	char s_static;
	char params;
	char param_pos;
	char owner;
} ;

/*
 * The state table must exactly match the order of the state definitions
 * or else the states won't be associated correctly.
 *
 * static states are those which always exist.  Non-static refers to those
 * which may have multiple instances of that state, or none at all, as
 * in S_ONCHAN.
 *
 * params are the number of parameters that state has.
 *
 * owner is the module which controls the state.
 */

static struct statetbl_node statetbl[] = {
	/* name          static?  params    parameter type and pos   owner  */
	{ "IRC",           1,       0,      0,                       SO_SUPER    },
	{ "ONCHAN",        0,       3,      (SP_C1 | SP_C2 | SP_I3), SO_CHANNEL  },
	{ "ONIRC",         0,       1,      (SP_C1),                 SO_SERVER   },
	{ "MEONCHAN",      0,       1,      (SP_C1),                 SO_CHANNEL  },
	{ "NUMONCHAN",     0,       1,      (SP_I1),                 SO_CHANNEL  },
	{ 0,               0,       1,      0,                       0           }
} ;


int initialize_states()
/*
 * Function: Initialize states according to state table
 *  Returns:  0 if OK
 *           -1 if error
 *
 */
{
	int i;
	state_t *x;


	for(i = 0; statetbl[i].name[0] != 0; i++)
	{
		if(statetbl[i].s_static)
		{
			x = (state_t *)xmalloc(sizeof(state_t));
			x->id = i;
			x->name = statetbl[i].name;
			x->set = 0;
			x->p1.str = NULL;
			x->p1.val = 0;
			x->p2.str = NULL;
			x->p2.val = 0;
			x->p3.str = NULL;
			x->p3.val = 0;
			listmgr_append(states, x);
		}
	}

	return 0;
}


#ifdef TEST
/* -------------------------------------------------------------------------
 * TESTING
 *
 * Compile with
 *   gcc -c listmgr.c -DSTANDALONE
 *   gcc -c xmem.c -DSTANDALONE
 *   gcc -c state.c -DTEST
 *   gcc -o state state.o listmgr.o xmem.o
 * ------------------------------------------------------------------------- */

int main()
{
	state_t *x;
	states = listmgr_new_list();

	/* initialize states according to state table */
	initialize_states();

	for(x = listmgr_begin(states); x; x = listmgr_next(states))
	{
		printf("id=%d, name=%s, set=%d, static=%d, params=%d, owner=%d\n",
			x->id, x->name, x->set, statetbl[x->id].s_static,
			statetbl[x->id].params, statetbl[x->id].owner);
	}
	return 0;
}

#endif /* TEST */
