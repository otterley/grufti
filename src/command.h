/******************************************************************************
 * Steve Rider - stever@area23.org
 * Grufti - an interactive, service-oriented bot for IRC 
 *
 * command.h - the command object declarations
 *
 *****************************************************************************/
/* 28 April 97 */

#ifndef COMMAND_H
#define COMMAND_H

#include "user.h"     /* for userrec and acctrec definitions */


/*
 * command record
 *
 * Used to store all commands
 */
struct cmdrec_t {
	char *name;                 /* Name of command */
	struct listmgr_t *subcommands;     /* list of subcommands (if NULL, then func) */

	/*
	 * The following are only set
	 * when this node is a leaf.
	 */
	void (*function)();         /* pointer to function */
	u_long levels;              /* levels required */
	int needsdcc;               /* Needs dcc? */
	int accessed;               /* how often accessed */

	/* provided for backwards compatiblity with web.c */
	struct cmdrec_t *next;
	int order;

} ;


/*
 * fixed command record
 *
 * Used to assign functions to commands in each trigger module.
 */
typedef struct cmdrec_fixed_t {
	char *name;                 /* Name of command */
	struct cmdrec_fixed_t *subcmd_tbl;
	void (*function)();         /* pointer to function */
	char *levels;               /* levels required */
	int needsdcc;               /* needs dcc? */

} cmdrec_fixed_t ;


/*
 * Command info structure
 *
 * Used to provide relevant information to each command
 * routine.
 */
typedef struct cmdinfo_t {

	int	active;                 /* set when currently in use */

	/*
	 * The following 8 fields must be preset by the calling
	 * function before invoking do_command().
	 */
	char buf[STDBUF];           /* command string */
	char from_n[UHOSTLEN];      /* nick of person */
	char from_u[UHOSTLEN];      /* username of person */
	char from_uh[UHOSTLEN];     /* userhost of person */
	char from_h[HOSTLEN];       /* hostname of person */
	userrec *user;              /* -> user record */
	acctrec *account;           /* -> account record */
	dccrec *d;                  /* NULL if command not via DCC */
	

	/*
	 * The following 11 fields are set by command_do() when
	 * the command is executed, providing each command function
	 * with detailed information about the command parameters.
	 */
	char cmdname[STDBUF];       /* name of command or alias */
	char param[STDBUF];         /* all the parameters */
	char tok1[STDBUF];          /* parameter 1 */
	char tok2[STDBUF];          /*     "     2 */
	char tok3[STDBUF];          /*     "     3 */
	int istok1;                 /* set if parameter 1 exists */
	int istok2;                 /*  "  "     "      2   "    */
	int istok3;                 /*  "  "     "      3   "    */
	int num_elements;           /* number of parameters */
	int paramlen;               /* length (in bytes) of parameter string */
	int isparam;                /* set if any parameters exist */

	/* Spare storage */
	char s1[STDBUF];
	char s2[STDBUF];

	/* flags */
	u_long flags;

} cmdinfo_t ;

/*
 * Public routines for working with command lists and records
 */
#if 0
listmgr_t *command_new_list();
cmdrec_t *cmdrec_new();
#endif
int command_isentry(struct listmgr_t *list, char *name);
cmdrec_t *command_get_entry(struct listmgr_t *list, char *name);
cmdrec_t *Command_cmdbyorder(int pos);

/* Specific to the command list */
void command_init();
void command_do();
int command_reset_levels(char *cmdname, char *flags);
void command_reset_all_pointers();
u_long command_sizeof();


/* flags */
#define COMMAND_SESSIONVERIFIED   0x01

#endif /* COMMAND_H */
