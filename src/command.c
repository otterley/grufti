/* -------------------------------------------------------------------------
 * Steve Rider - stever@area23.org
 * Grufti - an interactive, service-oriented bot for IRC 
 *
 * command.c - the command module
 *
 * Manages command list (tree).  Parses user input and executes as command.
 *
 * ------------------------------------------------------------------------- */
/* 28 April 97 (re-written 13 July 98) */

#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include "grufti.h"
#include "user.h"
#include "listmgr.h"
#include "log.h"
#include "cmd.h" /* temporary until we get all commands moved into modules */
#include "dcc.h" /* checking dcc requirements in command_do() */
#include "command.h"


/* The command list */
listmgr_t *commands = NULL;

/* The command info record (used to shuttle command information around) */
cmdinfo_t cmdinfo;

/* subcommand tables (found in various modules) */
extern cmdrec_fixed_t trigcmd_base_tbl[];


/*
 * Base command table.
 */
cmdrec_fixed_t cmd_base_tbl[] =
{
/*  Command			subcmd_tbl			function		levels	DCC?	*/
/*	===============	===================	===============	=======	======	*/
	{ "die",				NULL, cmd_die,		"m",	FALSE	},
	{ "boot",				NULL, cmd_boot,		"m",	FALSE	},
	{ "raw",				NULL, cmd_raw,		"m",	FALSE	},
	{ "chat",				NULL, cmd_chat,		"-",	FALSE	},
	{ "register",			NULL, cmd_register,		"-",	TRUE 	},
	{ "setpass",			NULL, cmd_setpass,		"r",	FALSE	},
	{ "chpass",				NULL, cmd_chpass,		"m",	FALSE	},
	{ "chusername",			NULL, cmd_chusername,		"r",	FALSE	},
	{ "addnick",			NULL, cmd_addnick,		"r",	FALSE	},
	{ "delnick",			NULL, cmd_delnick,		"r",	FALSE	},
	{ "setfinger",			NULL, cmd_setfinger,		"r",	FALSE	},
	{ "setname",			NULL, cmd_setfinger,		"r",	FALSE	},
	{ "setrealname",		NULL, cmd_setfinger,		"r",	FALSE	},
	{ "setemail",			NULL, cmd_setemail,		"r",	FALSE	},
	{ "setwww",				NULL, cmd_setwww,		"r",	FALSE	},
	{ "setforward",			NULL, cmd_setforward,		"r",	FALSE	},
	{ "forwardnotes",		NULL, cmd_forwardnotes,	"r",	FALSE	},
	{ "setother",			NULL, cmd_setother1,		"r",	FALSE	},
	{ "setother1",			NULL, cmd_setother1,		"r",	FALSE	},
	{ "setother2",			NULL, cmd_setother2,		"r",	FALSE	},
	{ "setother3",			NULL, cmd_setother3,		"r",	FALSE	},
	{ "setother4",			NULL, cmd_setother4,		"r",	FALSE	},
	{ "setother5",			NULL, cmd_setother5,		"r",	FALSE	},
	{ "setother6",			NULL, cmd_setother6,		"r",	FALSE	},
	{ "setother7",			NULL, cmd_setother7,		"r",	FALSE	},
	{ "setother8",			NULL, cmd_setother8,		"r",	FALSE	},
	{ "setother9",			NULL, cmd_setother9,		"r",	FALSE	},
	{ "setother10",			NULL, cmd_setother10,		"r",	FALSE	},
	{ "setplan",			NULL, cmd_setplan1,		"r",	FALSE	},
	{ "setplan1",			NULL, cmd_setplan1,		"r",	FALSE	},
	{ "setplan2",			NULL, cmd_setplan2,		"r",	FALSE	},
	{ "setplan3",			NULL, cmd_setplan3,		"r",	FALSE	},
	{ "setplan4",			NULL, cmd_setplan4,		"r",	FALSE	},
	{ "setplan5",			NULL,	cmd_setplan5,		"r",	FALSE	},
	{ "setplan6",			NULL,	cmd_setplan6,		"r",	FALSE	},
	{ "setlocation",		NULL,	cmd_setlocation,	"r",	FALSE	},
	{ "setlocat",			NULL,	cmd_setlocation,	"r",	FALSE	},
	{ "showlocations",		NULL,	cmd_showlocations,	"-",	TRUE	},
	{ "chlocation",			NULL,	cmd_chlocation,		"r",	FALSE	},
	{ "addlocation",		NULL,	cmd_addlocation,	"r",	FALSE	},
	{ "dellocation",		NULL,	cmd_dellocation,	"r",	FALSE	},
	{ "unregister",			NULL,	cmd_unregister,		"r",	FALSE	},
	{ "motd",				NULL,	cmd_motd,		"-",	TRUE 	},
	{ "resetmotd",			NULL,	cmd_resetmotd,		"m",	FALSE 	},
	{ "pass",				NULL,	cmd_pass,		"-",	FALSE	},
	{ "exit",				NULL,	cmd_exit,		"-",	FALSE	},
	{ "bye",				NULL,	cmd_exit,		"-",	FALSE	},
	{ "quit",				NULL,	cmd_exit,		"-",	FALSE	},
	{ "addserver",			NULL,	cmd_addserver,		"t",	FALSE	},
	{ "jump",				NULL,	cmd_jump,		"t",	FALSE	},
	{ "delserver",			NULL,	cmd_delserver,		"t",	FALSE	},
	{ "setvar",				NULL,	cmd_setvar,		"m",	FALSE	},
	{ "showservers",		NULL,	cmd_servinfo,		"-",	TRUE	},
	{ "servers",			NULL,	cmd_servinfo,		"-",	TRUE	},
	{ "servinfo",			NULL,	cmd_servinfo,		"-",	TRUE	},
	{ "time",				NULL,	cmd_time,		"-",	FALSE	},
	{ "date",				NULL,	cmd_time,		"-",	FALSE	},
	{ "who",				NULL,	cmd_chaninfo,		"-",	TRUE	},
	{ "help",				NULL,	cmd_help,		"-",	TRUE	},
	{ "finger",				NULL,	cmd_finger,		"-",	TRUE	},
	{ "whois",				NULL,	cmd_whois,		"-",	TRUE	},
	{ "adduser",			NULL,	cmd_adduser,		"m",	FALSE	},
	{ "deluser",			NULL,	cmd_deluser,		"m",	FALSE	},
	{ "notify",				NULL,	cmd_notify,		"-",	TRUE	},
	{ "seen",				NULL,	cmd_seen,		"-",	TRUE	},
	{ "write",				NULL,	cmd_write,		"m",	FALSE	},
	{ "addthisnick",		NULL,	cmd_addthisnick,	"-",	FALSE	},
	{ "wallnote",			NULL,	cmd_wallnote,	"m",	TRUE	},
	{ "note",				NULL,	cmd_note,		"r",	FALSE	},
	{ "addnote",			NULL,	cmd_addnote,		"r",	FALSE	},
	{ "showmynote",			NULL,	cmd_showmynote,		"r",	TRUE	},
	{ "memo",				NULL,	cmd_memo,		"r",	FALSE	},
	{ "nn",					NULL,	cmd_nn,			"-",	FALSE	},
	{ "rn",					NULL,	cmd_rn,			"r",	TRUE	},
	{ "dn",					NULL,	cmd_dn,			"r",	FALSE	},
	{ "delnote",			NULL,	cmd_dn,			"r",	FALSE	},
	{ "savenote",			NULL,	cmd_savenote,		"r",	FALSE	},
	{ "unsavenote",			NULL,	cmd_unsavenote,		"r",	FALSE	},
	{ "showdcc",			NULL,	cmd_dccinfo,		"-",	TRUE	},
	{ "dccinfo",			NULL,	cmd_dccinfo,		"-",	TRUE	},
	{ "showcommands",		NULL,	cmd_cmdinfo,		"-",	TRUE	},
	{ "cmdinfo",			NULL,	cmd_cmdinfo,		"-",	TRUE	},
	{ "showcodes",			NULL,	cmd_codesinfo,		"-",	TRUE	},
	{ "codesinfo",			NULL,	cmd_codesinfo,		"-",	TRUE	},
	{ "whoisin",			NULL,	cmd_whoisin,		"r",	TRUE	},
	{ "webcatch",			NULL,	cmd_webcatch,		"-",	TRUE	},
	{ "clearwebcatch",		NULL,	cmd_clearwebcatch,	"m",	FALSE	},
	{ "addgreet",			NULL,	cmd_addgreet,		"r",	FALSE	},
	{ "showgreets",			NULL,	cmd_showgreets,		"r",	TRUE	},
	{ "delgreet",			NULL,	cmd_delgreet,		"r",	FALSE	},
	{ "mode",				NULL,	cmd_mode,		"m",	FALSE	},
	{ "telnet",				NULL,	cmd_telnet,		"-",	FALSE	},
	{ "maintainer",			NULL,	cmd_maint,		"-",	FALSE	},
	{ "version",			NULL,	cmd_version,		"-",	FALSE	},
	{ "ver",				NULL,	cmd_version,		"-",	FALSE	},
	{ "homepage",			NULL,	cmd_homepage,		"-",	FALSE	},
	{ "uptime",				NULL,	cmd_uptime,		"-",	FALSE	},
	{ "showresponse",		NULL,	cmd_showresponse,	"-",	TRUE	},
	{ "showresponses",		NULL,	cmd_showresponse,	"-",	TRUE	},
	{ "addrespline",		NULL,	cmd_addrespline,	"lL",	TRUE	},
	{ "newresponse",		NULL,	cmd_newresponse,	"lL",	TRUE	},
	{ "delresponse",		NULL,	cmd_delresponse,	"lL",	TRUE	},
	{ "delrespline",		NULL,	cmd_delrespline,	"lL",	TRUE	},
	{ "addrespmatch",		NULL,	cmd_addrespmatch,	"lL",	TRUE	},
	{ "newrespmatch",		NULL,	cmd_newrespmatch,	"lL",	TRUE	},
	{ "delrespmatch",		NULL,	cmd_delrespmatch,	"lL",	TRUE	},
	{ "addrespexcept",		NULL,	cmd_addrespexcept,	"lL",	TRUE	},
	{ "delrespexcept",		NULL,	cmd_delrespexcept,	"lL",	TRUE	},
	{ "delhostmask",		NULL,	cmd_delhostmask,	"r",	TRUE	},
	{ "op",					NULL,	cmd_op,			"o",	FALSE	},
	{ "whoami",				NULL,	cmd_whoami,		"-",	TRUE	},
	{ "users",				NULL,	cmd_users,		"-",	TRUE	},
	{ "say",				NULL,	cmd_say,		"m",	FALSE	},
	{ "act",				NULL,	cmd_act,		"m",	FALSE	},
	{ "showconfigs",		NULL,	cmd_configinfo,		"r",	TRUE	},
	{ "showconfig",			NULL,	cmd_configinfo,		"r",	TRUE	},
	{ "configinfo",			NULL,	cmd_configinfo,		"r",	TRUE	},
	{ "watchlog",			NULL,	cmd_watchlog,		"m",	TRUE	},
	{ "files",				NULL,	cmd_files,		"-",	TRUE	},
	{ "send",				NULL,	cmd_send,		"-",	FALSE	},
	{ "showmem",			NULL,	cmd_meminfo,		"-",	TRUE	},
	{ "meminfo",			NULL,	cmd_meminfo,		"-",	TRUE	},
	{ "topic",				NULL,	cmd_topic,		"t3",	FALSE	},
	{ "kick",				NULL,	cmd_kick,		"t",	FALSE	},
	{ "invite",				NULL,	cmd_invite,		"o",	FALSE	},
	{ "ban",				NULL,	cmd_ban,		"t",	FALSE	},
	{ "kb",					NULL,	cmd_kickban,		"t",	FALSE	},
	{ "kickban",			NULL,	cmd_kickban,		"t",	FALSE	},
	{ "stat",				NULL,	cmd_stat,		"-",	TRUE	},
	{ "stats",				NULL,	cmd_stat,		"-",	TRUE	},
	{ "status",				NULL,	cmd_stat,		"-",	TRUE	},
	{ "banlist",			NULL,	cmd_banlist,		"r",	TRUE	},
	{ "addban",				NULL,	cmd_addban,		"t",	FALSE	},
	{ "delban",				NULL,	cmd_delban,		"t",	FALSE	},
	{ "voice",				NULL,	cmd_voice,		"v",	FALSE	},
	{ "leave",				NULL,	cmd_leave,		"t",	FALSE	},
	{ "join",				NULL,	cmd_join,		"t",	FALSE	},
	{ "hush",				NULL,	cmd_hush,		"t",	FALSE	},
	{ "unhush",				NULL,	cmd_unhush,		"t",	FALSE	},
	{ "backup",				NULL,	cmd_backup,		"m",	FALSE	},
	{ "showbans",			NULL,	cmd_showbans,		"o",	TRUE	},
	{ "email",				NULL,	cmd_email,		"-",	FALSE	},
	{ "mail",				NULL,	cmd_email,		"-",	FALSE	},
	{ "url",				NULL,	cmd_url,		"-",	FALSE	},
	{ "bday",				NULL,	cmd_bday,		"-",	FALSE	},
	{ "addevent",			NULL,	cmd_addevent,		"m",	TRUE	},
	{ "delevent",			NULL,	cmd_delevent,		"m",	TRUE	},
	{ "showevents",			NULL,	cmd_showevents,		"-",	TRUE	},
	{ "showevent",			NULL,	cmd_showevent,		"-",	TRUE	},
	{ "seteventname",		NULL,	cmd_seteventname,	"m",	TRUE	},
	{ "seteventplace",		NULL,	cmd_seteventplace,	"m",	TRUE	},
	{ "seteventdates",		NULL,	cmd_seteventdates,	"m",	TRUE	},
	{ "seteventabout",		NULL,	cmd_seteventabout,	"m",	TRUE	},
	{ "seteventadmin",		NULL,	cmd_seteventadmin,	"m",	TRUE	},
	{ "attendevent",		NULL,	cmd_attendevent,	"r",	TRUE	},
	{ "unattendevent",		NULL,	cmd_unattendevent,	"r",	TRUE	},
	{ "seteventwhen",		NULL,	cmd_seteventwhen,	"r",	TRUE	},
	{ "seteventcontact",	NULL,	cmd_seteventcontact,	"r",	TRUE	},
	{ "eventnote",			NULL,	cmd_eventnote,		"r",	TRUE	},
	{ "setbday",			NULL,	cmd_setbday,		"r",	FALSE	},
	{ "showbday",			NULL,	cmd_showbdays,		"-",	TRUE	},
	{ "showbdays",			NULL,	cmd_showbdays,		"-",	TRUE	},
	{ "showhistory",		NULL,	cmd_showhistory,	"r",	TRUE	},
	{ "addhistory",			NULL,	cmd_addhistory,		"m",	FALSE	},
	{ "setusernumber",		NULL,	cmd_setusernumber,	"m",	FALSE	},
	{ "setusercreation",	NULL,	cmd_setusercreation,	"m",	FALSE	},
	{ "showother",			NULL,	cmd_showother,	"r",	TRUE	},
	{ "showothers",			NULL,	cmd_showother,	"r",	TRUE	},
	{ "searchresponse",		NULL,	cmd_searchresponse,	"-",	TRUE	},
		/*
		 :
		 */
	{ NULL,			NULL,	null(void(*)()),	NULL,	FALSE	}
} ;



/* prototypes for internal commands */
static void cmdrec_init(cmdrec_t *c);
static void cmdrec_free(cmdrec_t *c);
static int cmdrec_match(cmdrec_t *c, char *name);
static int cmdrec_compare(cmdrec_t *c1, cmdrec_t *c2);
static void rm_dashes(char *s);
static void command_reset_cmdinfo();

static void append_command(listmgr_t *list, cmdrec_fixed_t *entry);
static cmdrec_t *parse_command(char *buf);
static int verify_levels(cmdrec_t *c);


static listmgr_t *command_new_list()
/*
 * Function: Create and initialize a list
 *  Returns: list object
 *
 *  The list will hold cmdrec objects.
 */
{
	listmgr_t *list;

	list = listmgr_new_list(0);
	listmgr_set_free_func(list, cmdrec_free);
	listmgr_set_match_func(list, cmdrec_match);
	listmgr_set_compare_func(list, cmdrec_compare);

	return list;
}


static cmdrec_t *cmdrec_new()
/*
 * Function: Create and initialize a cmdrec object
 *  Returns: cmdrec object
 *
 */
{
	cmdrec_t *c;

	c = (cmdrec_t *)xmalloc(sizeof(cmdrec_t));
	cmdrec_init(c);

	return c;
}


static void cmdrec_init(cmdrec_t *c)
{
	c->name = NULL;
	c->subcommands = NULL;
	c->function = null(void(*)());
	c->levels = 0;
	c->needsdcc = 0;
	c->accessed = 0;
}


/* listmgr free function for cmdrec */
static void cmdrec_free(cmdrec_t *c)
{
	xfree(c->name);

	/* recursively delete subcommands */
	if(c->subcommands != NULL)
		listmgr_destroy_list(c->subcommands);

	xfree(c);
}


/* listmgr match function for cmdrec */
static int cmdrec_match(cmdrec_t *c, char *name)
{
	return isequal(c->name, name) ? 1 : 0;
}

/* listmgr compare function for cmdrec */
static int cmdrec_compare(cmdrec_t *c1, cmdrec_t *c2)
{
	return strcmp(c1->name, c2->name);
}

static void rm_dashes(char *s)
/*
 * Function: remove extraneous dashes in command
 *
 *  Wipe beginning and end of string, then iterate through string
 *  and remove multiple consecutive dashes.
 *
 *    "--this-is---my--command--"  -->  "this-is-my-command"
 */
{
	char *p;
	int pos = 0, lastwasdash = 0;

	/* wipe end of string */
	for(p = s + strlen(s) - 1; ((*p == '-') && (p >= s)); p--);

	if(p != s + strlen(s) - 1)
		*(p + 1) = 0;
	
	/* wipe beginning */
	for(p = s; ((*p == '-') && (*p)); p++);

	/* remove multiple consecutive dashes between words */
	for( ; *p; p++)
	{
		if(*p == '-')
		{
			if(lastwasdash)
				continue;

			lastwasdash = 1;
		}
		else
			lastwasdash = 0;

		s[pos] = *p;
		pos++;
	}

	s[pos] = 0;
}


static void command_reset_cmdinfo()
/*
 * Function: Reset cmdinfo record
 *
 */
{
	cmdinfo.active = 0;

	cmdinfo.buf[0] = 0;
	cmdinfo.from_n[0] = 0;
	cmdinfo.from_u[0] = 0;
	cmdinfo.from_uh[0] = 0;
	cmdinfo.from_h[0] = 0;
	cmdinfo.user = NULL;
	cmdinfo.account = NULL;
	cmdinfo.d = NULL;

	cmdinfo.cmdname[0] = 0;
	cmdinfo.param[0] = 0;
	cmdinfo.tok1[0] = 0;
	cmdinfo.tok2[0] = 0;
	cmdinfo.tok3[0] = 0;
	cmdinfo.istok1 = 0;
	cmdinfo.istok2 = 0;
	cmdinfo.istok3 = 0;
	cmdinfo.num_elements = 0;
	cmdinfo.paramlen = 0;
	cmdinfo.isparam = 0;

	cmdinfo.s1[0] = 0;
	cmdinfo.s2[0] = 0;

	cmdinfo.flags = 0;
}






/* -------------------------------------------------------------------------
 * Public interface
 *
 *
 *
 * ------------------------------------------------------------------------- */

void command_init()
/*
 * Function: Initialize and load command list
 *
 *  Creates the command list and loads it with the fixed command tables.
 *
 *  The command list is actually a tree.  Leaves are commands, and branches
 *  are command categories.  For example, the command 'trigger-add-pubmethod'
 *  is a leaf on the 'trigger-add' branch, which is a branch on the 'trigger'
 *  branch.
 *
 *  To create the tree, we do some recursive calls to follow all the branches
 *  in the various subcommand tables to their leaves.
 *
 *  A leaf is a node which has a NULL subcommands entry.  A node with a valid
 *  subcommands entry is a branch.  Branches do not have functions, levels,
 *  or needsdcc fields.
 */
{
	int i;

	/* create list object */
	commands = command_new_list();

	/* Load commands from table into list */
	for(i = 0; cmd_base_tbl[i].name != NULL; i++)
		append_command(commands, &cmd_base_tbl[i]);
}

static void append_command(listmgr_t *list, cmdrec_fixed_t *entry)
{
	cmdrec_t *newcmd;
	int i;

	newcmd = cmdrec_new();
	listmgr_append(list, newcmd);
	
	newcmd->name = entry->name;  /* no need to alloc */

	/* If NULL, we've reached a leaf (complete command) */
	if(entry->subcmd_tbl == NULL)
	{
		newcmd->function = entry->function;
		newcmd->needsdcc = entry->needsdcc;
		User_str2flags(entry->levels, &newcmd->levels);
		if(newcmd->levels & USER_INVALID)
			internal("command: in init(), invalid level %s for %s.",
				entry->levels, entry->name);
		return;
	}

	/* At a branch.  Create a new list for it... */
	newcmd->subcommands = command_new_list();

	/* and go recursive to append commands on this branch. */
	for(i = 0; entry->subcmd_tbl[i].name != NULL; i++)
		append_command(newcmd->subcommands, &entry->subcmd_tbl[i]);
}


void command_do()
/*
 * Function: parses and executes command
 *
 *  The calling function must load the command info fields in the
 *  cmdinfo record before calling this function.
 *
 *  Parses the string, and executes the command if a complete command
 *  is found, or generates an error and available subcommands if an
 *  incomplete command is found.
 *
 *  If a complete command is found, this function sets additional fields
 *  in the cmdinfo record, and executes the command.
 */
{
	static char cmd[MAXREADBUFLENGTH];
	dccrec *d;
	cmdrec_t *c;

	if(cmdinfo.buf[0] == 0)
	{
		internal("command: in do(), cmdinfo record not set. (emtpy buf)");
		return;
	}

	/* (old) Update command statistics */
	Grufti->bot_cmds[thishour()]++;

	cmdinfo.active = 1;

	/*
	 * First, break apart command and arguments.  Then see if it matches
	 * a command in the tree.
	 */
	strcpy(cmdinfo.param, cmdinfo.buf);
	split(cmdinfo.cmdname, cmdinfo.param);
	strcpy(cmd, cmdinfo.cmdname);

	c = parse_command(cmd);
	if(c == NULL)
	{
		/* command not found.  (parse_command() displayed reason) */
		cmdinfo.active = 0;
		command_reset_cmdinfo();
		return;
	}

	/*
	 * Check DCC before checking levels.  This way, if the command requires
	 * DCC and the user doesn't have it, they won't have to send their
	 * password, establish the dcc connection, then send their password
	 * again via dcc.  They'll establish a dcc connection, then send their
	 * password to create a secure connection.
	 */
	if((c->needsdcc || isequal(cmdinfo.param, "-h")) && cmdinfo.d == NULL)
	{
		/*
		 * Command needs DCC CHAT and none exists.  Send a chat
		 * request, mark the record as "command pending", and save
		 * the command.
		 */
		if(DCC->num_dcc >= Grufti->max_dcc_connections)
			return;

		d = DCC_sendchat(cmdinfo.from_n, cmdinfo.from_uh);
		if(d == NULL)
			return;
		d->flags |= STAT_COMMAND_PENDING;

		/*
		 * To keep users from sending commands on behalf of other users,
		 * to be executed when that user DCC chats the bot, we timestamp
		 * the command, and only allow execution if the nick is identical.
		 */
		dccrec_setcommand(d, cmdinfo.buf);
		dccrec_setcommandfromnick(d, cmdinfo.from_n);
		d->commandtime = time(NULL);

		cmdinfo.active = 0;
		command_reset_cmdinfo();
		return;
	}

	/*
	 * If '-h' switch, do 'help command'
	 */
	if(isequal(cmdinfo.param,"-h"))
	{
		sprintf(cmdinfo.buf, "help %s", cmdinfo.cmdname);

		cmdinfo.active = 0;
		command_do();

		return;
	}
		
	/* Check levels */
	if(c->levels != 0L)
	{
		if(verify_levels(c) == 0)
		{
			cmdinfo.active = 0;
			command_reset_cmdinfo();
			return;
		}
	}

	/* Set command parameter info */
	str_element(cmdinfo.tok1,cmdinfo.param,1);
	str_element(cmdinfo.tok2,cmdinfo.param,2);
	str_element(cmdinfo.tok3,cmdinfo.param,3);
	cmdinfo.istok1 = cmdinfo.tok1[0] ? 1 : 0;
	cmdinfo.istok2 = cmdinfo.tok2[0] ? 1 : 0;
	cmdinfo.istok3 = cmdinfo.tok3[0] ? 1 : 0;
	cmdinfo.num_elements = num_elements(cmdinfo.param);
	cmdinfo.paramlen = strlen(cmdinfo.param);
	cmdinfo.isparam = cmdinfo.param[0] ? 1 : 0;

	/* update statistics on user */
	cmdinfo.user->bot_commands++;
	c->accessed++;
	(c->function)();

	cmdinfo.active = 0;
}


static cmdrec_t *parse_command(char *buf)
/*
 * Function: tries to find command that matches buf
 *  Returns: pointer to cmdrec structure, NULL if command not found
 *
 *  Takes a string in the form of "basecmd-sub-sub-sub" and tries to
 *  find the corresponding command.  If command is partial, displays
 *  available subcommands for outermost branch.
 *
 *  If command matches but contains additional text, displays an error
 *  to that effect.
 */
{
	listmgr_t *list;
	cmdrec_t *c;
	char tok[MAXREADBUFLENGTH], available[256];
	char whole[MAXREADBUFLENGTH], lastwhole[MAXREADBUFLENGTH];
	int unmatched = 0;
	
	if(buf == NULL || buf[0] == 0)
		return NULL;

	whole[0] = 0;

	/* base case */
	list = commands;

	while(buf[0] != 0)
	{
		splitc(tok, buf, '-');

		strcpy(lastwhole, whole);
		if(whole[0] == 0)
			strcpy(whole, tok);
		else
			sprintf(whole, "%s-%s", whole, tok);

		/*
		 * Search for entry in current branch.  If found, then it's either
		 * a leaf or another branch.  If it's a leaf, and the rest of the
		 * command is blank, we've got our command.  If it's not a leaf,
		 * then it's a branch, and we'll try again, matching the next
		 * token for the new branch.
		 */
		if(command_isentry(list, tok))
		{
			c = command_get_entry(list, tok);

			if(c->subcommands == NULL)
			{
				/*
				 * Got a leaf.  Check if rest of command isn't emtpy.
				 */
				if(buf[0] != 0)
				{
					/* only respond if via DCC */
					if(cmdinfo.d != NULL)
					{
						say("Bogus subcommand: \"%s\"", buf);
						say("\"%s\" is a complete command.", whole);
					}
					return NULL;
				}

				return c;
			}

			/*
			 * Not a leaf, but a branch.  Update 'list' variable to
			 * point to it, and check the next token.
			 */
			list = c->subcommands;
		}
		else
		{
			unmatched = 1;
			break;
		}
	}

	/*
	 * If here, then a branch or leaf was not found, or we hit the end
	 * of the command string without finding a leaf.
	 *
	 * If we hit the end of the command string without finding a leaf,
	 * tell user the available subcommands for this branch unless we're
	 * at the base case.
	 *
	 * If we didn't hit the end of the command string, then tell user
	 * which token didn't match, and list all available subcommands.
	 */
	if(list == commands)
	{
		if(cmdinfo.d != NULL)
			say("Unknown command: %s", whole);
		return NULL;
	}

	available[0] = 0;
	for(c = listmgr_begin(list); c != NULL; c = listmgr_next(list))
	{
		if(available[0] == 0)
			strcpy(available, c->name);
		else
			sprintf(available, "%s, %s", available, c->name);
	}

	if(unmatched)
	{
		if(cmdinfo.d != NULL)
			say("Bogus subcommand: \"%s\"", tok);
		strcpy(whole, lastwhole);
	}

	if(cmdinfo.d != NULL)
		say("Subcommands for %s: %s", whole, available);
	return NULL;
}


static int verify_levels(cmdrec_t *c)
/*
 * Function: Verify that user has appropriate levels
 *  Returns: 1 if levels were OK, proceed with command
 *           0 if not OK, return now
 *
 */
{
	/* Command requires +r only and user is not registered */
	if(c->levels == USER_REGD && !cmdinfo.user->registered)
	{
		if(cmdinfo.d)
		{
			say("This command requires that you be registered.");	
			say("");
			sayf(0,0,"The nick \"%s\" is not a registered nick.  To register this nick with the bot type 'register'.  By registering with the bot you will secure this nick as yours, have the ability to set personal info like your email and www address, and you will be able to receive Notes and acquire levels.",cmdinfo.from_n);
			say("");
			sayf(0,0,"If you are already registered and are using a nick which is not listed for your account, type 'addthisnick <acctname> <password>' to add it.");
			say("");
			sayf(0,0,"Thanks, have a nice day!");
		}
		else
			sayf(0,0,"This command requires that you be registered.  Type 'register' to create an account.  (Type 'addthisnick' if you are already registered...)");

		return 0;
	}
		
	/* User is not master and doesn't match levels */
	if(!(cmdinfo.user->flags & c->levels) && !(cmdinfo.user->flags & USER_MASTER))
	{
		char flags[USERFLAGLEN], _flags[USERFLAGLEN], *p;

		User_flags2str(c->levels,_flags);
		flags[0] = 0;
		for(p=_flags; *p; p++)
			sprintf(flags,"%s+%c or ",flags,*p);

		if(flags[0])
			flags[strlen(flags) - 4] = 0;

		say("That command requires %s, and you don't have it.  Go away.",flags);

		return 0;
	}

	/* User has required levels.  Check verification */
	if(cmdinfo.d)
	{
		/* DCC.  Check for verified */
		if(!(cmdinfo.d->flags & STAT_VERIFIED))
		{
			/*
			 * Not verified, but they are active via DCC.
			 * At this point the user is registered since
			 * the command required some levels and they
			 * had it.  Only registered users will have
			 * levels...
			 *
			 * To keep malicious users from sending commands on behalf
			 * of other users, we timestamp the command, and record the
			 * nick.  Commands will only be executed if they haven't
			 * expired, and if they match the nick of the original user.
			 */
			
			say("Please identify yourself.  'pass <password>'");
			sprintf(cmdinfo.s1,"%s %s",cmdinfo.cmdname,cmdinfo.param);
			userrec_setcmd_pending_pass(cmdinfo.user,cmdinfo.s1);
			userrec_setcmdnick_pending_pass(cmdinfo.user,cmdinfo.from_n);
			cmdinfo.user->cmdtime_pending_pass = time(NULL);

			return 0;
		}

		/* Verified via DCC.  */
		return 1;
	}

	/* NO DCC */
	/* They're ok if SESSIONVERIFIED is flagged */
	if(!(cmdinfo.flags & COMMAND_SESSIONVERIFIED))
	{
		/*
		 * It's not.  We always require a password.
		 *
		 * Like above, to keep malicious users from sending commands on behalf
		 * of other users, we timestamp the command, and record the
		 * nick.  Commands will only be executed if they haven't
		 * expired, and if they match the nick of the original user.
		 */
		say("Please identify yourself.  'pass <password>'");
		sprintf(cmdinfo.s1,"%s %s",cmdinfo.cmdname,cmdinfo.param);
		userrec_setcmd_pending_pass(cmdinfo.user,cmdinfo.s1);
		userrec_setcmdnick_pending_pass(cmdinfo.user,cmdinfo.from_n);
		cmdinfo.user->cmdtime_pending_pass = time(NULL);

		return 0;
	}

	return 1;
}



int command_reset_levels(char *cmdname, char *flags)
/*
 * Function: Change levels for a command
 *  Returns:  0 if OK
 *           -1 if command name not quoted
 *           -2 if command not found
 *           -3 if command levels are invalid
 *
 *  Used to reset levels when level override is encountered in
 *  config file.
 */
{
	cmdrec_t *c;

	/* Command name must have quotes. */
	if(!rmquotes(cmdname))
		return -1;

	if(!command_isentry(commands, cmdname))
		return -2;

	c = command_get_entry(commands, cmdname);
	User_str2flags(flags,&c->levels);
	if(c->levels & USER_INVALID)
		return -3;

	return 0;
}

cmdrec_t *Command_cmdbyorder(int pos)
{
	int i;
	cmdrec_t *c;

	for(c = listmgr_begin_sorted(commands), i = 1; c;
		c = listmgr_next_sorted(commands), i++)
	{
		if(i == pos)
			return c;
	}

	return NULL;
}

u_long command_sizeof()
{
	/* TODO */
	return 0;
}


void command_reset_all_pointers()
/*
 * Function: reset all user and account pointers in cmdinfo record.
 *
 *  Should be more generic: i.e. reset_user_pointers, reset_dcc_pointers
 */
{
	if(cmdinfo.active)
	{
		cmdinfo.user = User_user(cmdinfo.from_n);
		cmdinfo.account = User_account(cmdinfo.user, cmdinfo.from_n,
			cmdinfo.from_uh);
	}
}


int command_isentry(listmgr_t *list, char *name)
/*
 * Function: Determine whether command entry exists in branch
 *  Returns: 1 if command exists
 *           0 if not
 */
{
	listmgr_set_match_func(list, cmdrec_match);
	return listmgr_isitem(list, name);
}


cmdrec_t *command_get_entry(listmgr_t *list, char *name)
/*
 * Function: Retrieve command entry in branch
 *  Returns: pointer to command entry, or NULL if not found
 *
 */
{
	listmgr_set_match_func(list, cmdrec_match);
	if(listmgr_isitem(list, name))
		return listmgr_retrieve(list, name);

	internal("command: in get_entry(), command \"%s\" not found.", name);

	return NULL;
}
