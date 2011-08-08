/* -------------------------------------------------------------------------
 * Steve Rider - stever@area23.org
 * Grufti - an interactive, service-oriented bot for IRC 
 *
 * trigcmd.c - command module to manipulate triggers (setting, viewing)
 *
 * Rules:
 *  - Code here is only that which is required to handle user-interface.
 *    Anything else not ui oriented should go in trigops module.  Very basic
 *    trigger list operations should go in triglist module.
 *  - This module is allowed to access triglist public-interface routines
 *    and trigger record entries directly.  (no need to provide functions
 *    just to get name field!)
 *
 * ------------------------------------------------------------------------- */
/* 12 July 98 */

#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "grufti.h"
#include "command.h"
#include "triglist.h"
#include "trigops.h"
#include "trigcmd.h"

#ifdef STANDALONE  /* requires listmgr.c, triglist.c, trigcmd.c, xmem.c */

/* Normally defined in grufti.c */
#define say debug_to_stderr
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


extern cmdinfo_t cmdinfo;


/* prototypes for base command */
static void create();
static void show();
static void showall();

/* prototypes for add subcommand */
static void add_pubmethod();
static void add_privmethod();
static void add_cronmethod();
static void add_actionmethod();
static void add_statemethod();

/* prototypes for del subcommand */
static void del_pubmethod();
static void del_privmethod();
static void del_cronmethod();
static void del_actionmethod();
static void del_statemethod();


/* subcommand tables must be listed first */
static cmdrec_fixed_t add_tbl[] = 
{
	{ "pubmethod",		NULL,		add_pubmethod,		"lL",	TRUE	},
	{ "privmethod",		NULL,		add_privmethod,		"lL",	TRUE	},
	{ "cronmethod",		NULL,		add_cronmethod,		"lL",	TRUE	},
	{ "actionmethod",	NULL,		add_actionmethod,	"lL",	TRUE	},
	{ "statemethod",	NULL,		add_statemethod,	"lL",	TRUE	},
	{ NULL,				NULL,		null(void(*)()),	NULL,	0		}
} ;

static cmdrec_fixed_t del_tbl[] = 
{
	{ "pubmethod",		NULL,		del_pubmethod,		"lL",	TRUE	},
	{ "privmethod",		NULL,		del_privmethod,		"lL",	TRUE	},
	{ "cronmethod",		NULL,		del_cronmethod,		"lL",	TRUE	},
	{ "actionmethod",	NULL,		del_actionmethod,	"lL",	TRUE	},
	{ "statemethod",	NULL,		del_statemethod,	"lL",	TRUE	},
	{ NULL,				NULL,		null(void(*)()),	NULL,	0		}
} ;

/* entry point table for trigger commands (used by command module) */
cmdrec_fixed_t trigcmd_base_tbl[] = 
{
	{ "create",			NULL,		create,				"lL",	TRUE	},
	{ "add",			add_tbl,	NULL,				NULL,	0		},
	{ "del",			del_tbl,	NULL,				NULL,	0		},
	{ "show",			NULL,		show,				"lL",	TRUE	},
	{ "showall",		NULL,		showall,			"lL",	TRUE	},
	{ NULL,				NULL,		null(void(*)()),	NULL,	0		}
} ;


/*
 * Fields found in the cmdinfo struct.
 *
 *  param  - entire parameter line
 *  isparam  - set if parameter line is not empty
 *  paramlen  - length in bytes of parameter line
 *  num_elements  - number of space-delimited elements in parameter line
 *  tok1  - token1 in parameter list
 *  tok2  - token2 in parameter list
 *  tok3  - token3 in parameter list
 *  istok1  - set if token1 is not empty
 *  istok2  - set if token2 is not empty
 *  istok3  - set if token3 is not empty

 *  from_n  - nick
 *  from_u  - user
 *  from_uh  - user@hostname
 *  from_h  - hostname
 *
 *  user  - pointer to user record
 *  account  - pointer to account record
 *  d  - pointer to dcc record (NULL if not via dcc)
 *
 *  Respond using say() routine.
 */

static void create()
{
	trigrec_t *t;

	if(!cmdinfo.istok2)
	{
		say("Usage: trigger-create <type> <name>");
		say("Use this command to create a new trigger.  The <name> must be unique, and not contain spaces.  The <type> must be from the following:");
		say("TODO.");

		return;
	}

	if(trigger_istrigger(cmdinfo.tok2))
	{
		say("A trigger under the name \"%s\" already exists.", cmdinfo.tok2);
		return;
	}

	t = trigger_add_trigger(cmdinfo.tok2);
	if(t == NULL)
	{
		say("Unable to create trigger. (internal error)");
		internal("trigcmd: in create(), unable to create trigger: \"%s\"", 
			cmdinfo.tok2);
		return;
	}

	say("Created trigger \"%s\".", cmdinfo.tok2);
}


static void show()
{
	trigrec_t *t;

	if(!cmdinfo.istok1)
	{
		say("Usage: trigger-show <name>");
		say("Use this command to display a trigger entry.");
		return;
	}

	if(!trigger_istrigger(cmdinfo.tok1))
	{
		say("The trigger \"%s\" does not exist.", cmdinfo.tok1);
		return;
	}

	t = trigger_get_trigger(cmdinfo.tok1);
	say("    name: %s", t->name);
	if(t->method.pub != NULL)
	{
		struct method_pubrec *p;
		for(p = listmgr_begin(t->method.pub); p != NULL;
			p = listmgr_next(t->method.pub))
			{
				say("pub-trigger %d: %s", p->id, p->line);
			}
	}
}

extern listmgr_t *triggers;

static void showall()
{
	trigrec_t *t;
	struct method_pubrec *p;

	say("displaying triggers");
	for(t = listmgr_begin(triggers); t; t = listmgr_next(triggers))
	{
		say("    name: %s", t->name);
		if(t->method.pub != NULL)
		{
			for(p = listmgr_begin(t->method.pub); p != NULL;
				p = listmgr_next(t->method.pub))
			{
				say("pub-trigger %d: %s", p->id, p->line);
			}
		}
	}
}


void add_pubmethod()
{
	trigrec_t *t;

	if(!cmdinfo.tok2)
	{
		say("Usage: trigger-add-pubmethod <trigger> <pubmethod...>");
		return;
	}

	if(!trigger_istrigger(cmdinfo.tok1))
	{
		say("No such trigger \"%s\".", cmdinfo.tok1);
		return;
	}

	t = trigger_get_trigger(cmdinfo.tok1);

	trigrec_add_pubmethod(t, cmdinfo.tok2);

	say("Added pubmethod to trigger: %s", t->name); 
}

void add_privmethod()
{}

void add_actionmethod()
{}

void add_cronmethod()
{}

void add_statemethod()
{}

void del_pubmethod()
{}

void del_privmethod()
{}

void del_actionmethod()
{}

void del_cronmethod()
{}

void del_statemethod()
{}

