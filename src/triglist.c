/* -------------------------------------------------------------------------
 * Steve Rider - stever@area23.org
 * Grufti - an interactive, service-oriented bot for IRC 
 *
 * triglist.c - maintains list of triggers, setting fields, deleting, etc.
 *              Used by trigger.c (trigger interface) and trigexec.c
 *              (trigger executor)
 *
 *       trigger_interface
 *          |        |
 *    triglist --- trigexec
 *
 * See below to see how to compile this as a test module.
 * ------------------------------------------------------------------------- */
/* 9 July 98 */

#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include "grufti.h"
#include "listmgr.h"
#include "triglist.h"

#ifdef TEST
#define STANDALONE
#endif

#ifdef STANDALONE  /* requires listmgr,c, xmem.c */

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


/* the trigger list */
listmgr_t *triggers = NULL;


/* prototypes for internal routines */
static trigrec_t *trigrec_new();
static void trigrec_init(trigrec_t *t);
static void trigrec_free(trigrec_t *t);
static int trigrec_match_by_name(trigrec_t *t, char *name);
static struct method_pubrec *pubrec_new();
static void pubrec_init(struct method_pubrec *p);
static void pubrec_free(struct method_pubrec *p);
static int pubrec_match(struct method_pubrec *p, int *id);
static void pubrec_set_line(struct method_pubrec *p, char *line);

static trigrec_t *trigrec_new()
{
	trigrec_t *t;

	t = (trigrec_t *)xmalloc(sizeof(trigrec_t));
	trigrec_init(t);

	return t;
}

static void trigrec_init(trigrec_t *t)
{
	if(t == NULL)
	{
		internal("trigrec: in init(), pointer is NULL.");
		return;
	}

	t->name = NULL;
	t->type = NULL;

	t->method.cron = NULL;
	t->method.pub = NULL;
	t->method.priv = NULL;
	t->method.action = NULL;
	t->method.state = NULL;

	t->owner = NULL;
	t->lifespan = NULL;
	t->prob = 0.0f;
	t->created = 0;
	t->changed = 0;
	t->accessed = 0;
	t->hits = 0;

	t->action_mode = 0;
	t->actions = NULL;
	t->not_actions = NULL;

	t->flags = 0;
	t->expires = 0;
}

/* listmgr free function for trigrec */
static void trigrec_free(trigrec_t *t)
{
	xfree(t->name);
	xfree(t->type);

	/* TODO: free methods */

	xfree(t->owner);
	xfree(t->lifespan);

	/* TODO: free actions */

	xfree(t);
}

/* listmgr match function for trigrec */
static int trigrec_match_by_name(trigrec_t *t, char *name)
{
	return isequal(t->name, name) ? 1 : 0;
}




static struct method_pubrec *pubrec_new()
{
	struct method_pubrec *p;

	p = (struct method_pubrec *)xmalloc(sizeof(struct method_pubrec));
	pubrec_init(p);

	return p;
}

static void pubrec_init(struct method_pubrec *p)
{
	p->id = 0;
	p->line = NULL;
	p->flags = 0;
}

/* listmgr free function for pubrec */
static void pubrec_free(struct method_pubrec *p)
{
	xfree(p->line);
	xfree(p);
}

/* listmgr match function for pubrec */
static int pubrec_match(struct method_pubrec *p, int *id)
{
	return p->id == *id ? 1 : 0;
}

static void pubrec_set_line(struct method_pubrec *p, char *line)
{
	mem_strcpy(&p->line, line);
}


/* -------------------------------------------------------------------------
 * Public interface
 *
 *  add, delete, find, set fields
 *
 * Notation
 *    We use 'trigger' when we're acting on the whole list, and 'trigrec'
 *    when we're acting on a single trigger record.
 *
 *    trigrec routines are typically low-level, and don't do error-checking.
 *
 * Users will be allowed to have their own triggers, but the user routines
 * won't use these public functions.  They'll have their own.  Might be a good
 * idea to drop some of these routines into low-level area so we're not
 * being redundant with code.
 *
 * All functions assume an initialized triggers list.
 *
 * ------------------------------------------------------------------------- */

void trigger_init()
/*
 * Function: Creates new trigger list
 *
 *  (Should only be done once, at startup)
 *
 */
{
	if(triggers != NULL)
		internal("trigrec: in init(), list not NULL.");

	triggers = listmgr_new_list(0);
	listmgr_set_free_func(triggers, trigrec_free);
	listmgr_set_match_func(triggers, trigrec_match_by_name);
}


trigrec_t *trigger_add_trigger(char *name)
/*
 * Function: Create a new trigger and add it to the list
 *  Returns: pointer to trigger, or NULL if error
 *
 *  Initializes trigger and sets name field.
 */
{
	trigrec_t *t;

	if(trigger_istrigger(name))
	{
		internal("triglist: in add_trigger(), trigger \"%s\" already exists.",
			name);

		return NULL;
	}

	/* Create the trigger and initialize it */
	t = trigrec_new();
	listmgr_append(triggers, t);

	trigrec_set_name(t, name);

	/* set defaults */
	t->prob = 1.0f;
	t->flags |= TRIGGER_ENABLED;

	t->created = time(NULL);
	t->changed = time(NULL);
	t->accessed = time(NULL);

	return t;
}


void trigger_delete_trigger(char *name)
/*
 * Function: Delete trigger
 *
 */
{
	trigrec_t *t;

	t = trigger_get_trigger(name);
	if(t == NULL)
	{
		internal("triglist: in delete_trigger(), trigger \"%s\" not found.",
			name);
		return;
	}

	/*
	 * all we do is tell listmgr to drop it from the list and voila.  Our
	 * supplied free() function gets called to do the rest.
	 */
	listmgr_delete(triggers, t);
}


int trigger_istrigger(char *name)
/*
 * Function: Determine whether trigger entry exists
 *  Returns: 1 if trigger exists
 *           0 if not
 */
{
	listmgr_set_match_func(triggers, trigrec_match_by_name);
	return listmgr_isitem(triggers, name);
}


trigrec_t *trigger_get_trigger(char *name)
/*
 * Function: Find trigger entry by name
 *  Returns: pointer to trigger entry, or NULL if not found
 */
{
	listmgr_set_match_func(triggers, trigrec_match_by_name);
	if(listmgr_isitem(triggers, name))
		return listmgr_retrieve(triggers, name);

	internal("trigrec: in get_trigger(), trigger \"%s\" not found.", name);

	return NULL;
}


/*
 * Function: set string fields
 *
 * Public, low-level routines for dealing with trigger entries.
 *
 */
void trigrec_set_name(trigrec_t *t, char *name)
{
	mem_strcpy(&t->name, name);
}

void trigrec_set_type(trigrec_t *t, char *type)
{
	mem_strcpy(&t->type, type);
}

void trigrec_set_owner(trigrec_t *t, char *owner)
{
	mem_strcpy(&t->owner, owner);
}

void trigrec_add_pubmethod(trigrec_t *t, char *s)
/*
 * Function: add entry to pub-trigger list
 *
 */
{
	struct method_pubrec *p;

	/*
	 * Methods (listmgr objects) are by default NULL.  See if pub method
	 * has been initialized yet.
	 */
	if(t->method.pub == NULL)
	{
		t->method.pub = listmgr_new_list(LM_NOMOVETOFRONT);
		listmgr_set_free_func(t->method.pub, pubrec_free);
		listmgr_set_match_func(t->method.pub, pubrec_match);
	}

	p = pubrec_new();
	listmgr_append(t->method.pub, p);

	pubrec_set_line(p, s);
	p->id = listmgr_length(t->method.pub);
}


void trigrec_del_pubmethod(trigrec_t *t, int id)
/*
 * Function: deletes entry from pub-trigger list
 *
 *  Entries are specified by line number (id).  Calling function needs
 *  to verify t is not NULL and that the entry at line number exists.
 */
{
	struct method_pubrec *p;
	int i;

	if(!listmgr_isitem(t->method.pub, &id))
	{
		internal("triglist: in del_pubmethod(), line %d not found in trigger \"%s\".", id, t->name);
		return;
	}

	p = listmgr_retrieve(t->method.pub, &id);
	listmgr_delete(t->method.pub, p);

	/* renumber */
	for(p = listmgr_begin(t->method.pub), i = 1; p != NULL;
		p = listmgr_next(t->method.pub), i++)
		p->id = i;

	/*
	 * We save a little bit of space if we free the listmanager object
	 * when it's empty.
	 */
	if(listmgr_length(t->method.pub) == 0)
	{
		listmgr_destroy_list(t->method.pub);
		t->method.pub = NULL;
	}
}




#ifdef TEST
/* -------------------------------------------------------------------------
 * TESTING
 *
 * All tests should perform OK, and no warnings should occur.  Compile with
 * ------------------------------------------------------------------------- */
/* split first word off of rest and put it in first */
void splitc(char *first, char *rest, char divider)
{
       char *p;

       p = strchr(rest,divider);
       if(p == NULL)
       {
                if((first != rest) && (first != NULL))
		{
			strcpy(first,rest);
                        rest[0] = 0;
		}
                return;
        }
        *p = 0;
        if(first != NULL)
                strcpy(first,rest);
        if(first != rest)
                strcpy(rest,p+1);
}

void split(char *first, char *rest)
{
        splitc(first,rest,' ');
}


void cmd_showall(char *buf)
{
	trigrec_t *t;
	struct method_pubrec *p;

	for(t = listmgr_begin(triggers); t; t = listmgr_next(triggers))
	{
		printf("     name: %s\n", t->name);
		printf("     type: %s\n", t->type);
		if(t->method.pub != NULL)
		{
			for(p = listmgr_begin(t->method.pub); p != NULL;
				p = listmgr_next(t->method.pub))
			{
				printf("pub-trigger %d: %s\n", p->id, p->line);
			}
		}
	}
}

int main()
{
	char buf[256], first[256];
	trigrec_t *t;

	/* setup trigger list */
	trigger_init();

	/* build a generic trigger */
	t = trigger_add_trigger("bauhaus");
	if(t == NULL)
	{
		internal("couldn't create trigger.");
		return;
	}

	trigrec_set_type(t, "lyrics");
	trigrec_set_owner(t, "Arkham");

	trigrec_add_pubmethod(t, "\"bauhaus\"");


	while(1)
	{
		printf("> ");
		fgets(buf, 255, stdin);
		if(buf[strlen(buf) - 1] == '\n')
			buf[strlen(buf) - 1] = 0;

		split(first, buf);

		if(strcmp(first, "show") == 0)
			cmd_showall(buf);

		if(isequal(first, "add"))
			trigrec_add_pubmethod(t, buf);

		if(isequal(first, "del"))
			trigrec_del_pubmethod(t, atoi(buf));

		if(strcmp(first, "quit") == 0)
			exit(0);

	}
	return 0;
}

#endif /* TEST */
