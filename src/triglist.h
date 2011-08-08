/* -------------------------------------------------------------------------
 * Steve Rider - stever@area23.org
 * Grufti - an interactive, service-oriented bot for IRC 
 *
 * triglist.h - maintains list of triggers, setting fields, deleting, etc.
 *              Used by trigger.c (trigger interface) and trigexec.c
 *              (trigger executor)
 *
 *       trigger_interface
 *          |        |
 *    triglist --- trigexec
 *
 * ------------------------------------------------------------------------- */
/* 9 July 98 */

#ifndef TRIGLIST_H
#define TRIGLIST_H

#include "listmgr.h"  /* for listmgr_t definition */

/*
 * trigger record
 */
typedef struct {
	char *name;            /* name of trigger (no spaces) */
	char *type;            /* pre-built, custom, etc. */

	struct {
		listmgr_t *cron;
		listmgr_t *pub;
		listmgr_t *priv;
		listmgr_t *action;
		listmgr_t *state;
	} method ;             /* trigger methods */


	char *owner;           /* owner of trigger */
	char *lifespan;        /* if set, how long to live (2d, 3w, ...) */
	float prob;            /* trigger probability */
	time_t created;
	time_t changed;
	time_t accessed;
	u_long hits;           /* # hits */

	char action_mode;
	listmgr_t *actions;
	listmgr_t *not_actions;

	/* internal */
	char flags;
	time_t expires;        /* absolute time of lifespan field */

} trigrec_t ;


/* create, destroy main list */
void trigger_init();

/* Add, delete, search */
trigrec_t *trigger_add_trigger(char *name);
void trigger_delete_trigger(char *name);
int trigger_istrigger(char *name);
trigrec_t *trigger_get_trigger(char *name);

/* Set string fields */
void trigrec_set_name(trigrec_t *t, char *name);
void trigrec_set_type(trigrec_t *t, char *type);
void trigrec_set_owner(trigrec_t *t, char *owner);

/* Set method fields */
void trigrec_add_pubmethod(trigrec_t *t, char *s);
void trigrec_del_pubmethod(trigrec_t *t, int id);


/* records for different trigger methods */
struct method_cronrec {
	char *min;
	char *hr;
	char *day;
	char *dow;
	char *month;

} ;

struct method_pubrec {
	int id;
	char *line;
	char flags;
} ;

struct method_privrec {
	int id;
} ;

struct method_staterec {
	int id;
} ;

struct method_actionrec {
	int id;
} ;

struct trigaction {
	int id;
} ;

/* Flags */
#define TRIGGER_ERROR      0x01  /* Indicates trigger has an error */
#define TRIGGER_ENABLED    0x02  /* Trigger is enabled */
#define TRIGGER_SET        0x04  /* Trigger is ready to fire */

/* Action mode */
#define TRIGGER_RANDOM_NR  0     /* random no repeat */
#define TRIGGER_RANDOM     1     /* random */
#define TRIGGER_SEQUENTIAL 2     /* sequential (script-like) */


#endif /* TRIGLIST_H */
