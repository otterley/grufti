/******************************************************************************
 * Steve Rider - stever@area23.org
 * Grufti - an interactive, service-oriented bot for IRC
 * 
 * user.h - declarations for objects user, list, and account
 * 
 *****************************************************************************/
/* 28 April 97 */

#ifndef USER_H
#define USER_H

#include "typedefs.h"
#include "grufti.h"

/* Let all files who need User have access to the User object. */
extern struct User_object *User;


/*****************************************************************************/
/* Main User object */
typedef struct User_object {
	userrec	*userlist;		/* -> the list of users */
	userrec	*last;			/* -> the last entry */
	userrec	*luser;			/* -> the non-registered user entry */

	int	num_users;		/* number of user records */

	int	userlist_active;	/* so we don't write an inactive list */
} User_ob;

/* object User low-level function declarations. */
void	User_new();
u_long	User_sizeof();
void	User_freeall();

/* object User data-specific function declarations. */
void	User_display();

/* low-level User structure function declarations. */
void	User_appenduser(userrec *u);
void	User_deleteuser(userrec *u);
void	User_freeallusers();
void	User_appendnick(userrec *u, nickrec *n);
void	User_deletenick(userrec *u, nickrec *n);
void	User_freeallnicks(userrec *u);
void	User_appendaccount(userrec *u, acctrec *a);
void	User_deleteaccount(userrec *u, acctrec *a);
void	User_unlinkaccount(userrec *u, acctrec *a);
void	User_freeallaccounts(userrec *u);
void	User_appendgreet(userrec *u, gretrec *g);
void	User_deletegreet(userrec *u, gretrec *g);
void	User_freeallgreets(userrec *u);
void	User_appendnote(userrec *u, noterec *n);
void	User_deletenote(userrec *u, noterec *n);
void	User_freeallnotes(userrec *u);

/* object User high-level implementation-specific function declarations. */
void	User_loaduserfile();
void	User_readuserfile();
void	User_addnewgreet(userrec *u, char *greet);
void	User_addhistory(userrec *u, char *who, time_t t, char *msg);
void	User_delhistory(userrec *u, int pos);
void	User_addnewnick(userrec *user, char *nick);
void	User_removenick(userrec *u, char *nick);
acctrec	*User_addnewaccount(userrec *u, char *n, char *uh);
void	User_addnewacctline(userrec *user, char *s, int *error);
void	User_writeuserfile();
void	User_str2flags(char *s, u_long *f);
void	User_flags2str(u_long f, char *s);
userrec	*User_user(char *nick);
int	User_isuser(char *nick);
int	User_isknown(char *nick);
int	User_isnick(userrec *u, char *nick);
acctrec	*User_account(userrec *user, char *nick, char *uh);
acctrec	*User_account_dontmakenew(userrec *user, char *nick, char *uh);
void	User_gotpossiblesignon(userrec *u, acctrec *a);
void	User_gotsignon(userrec *u, nickrec *n);
void	User_gotpossiblesignoff(userrec *u, acctrec *a);
void	User_gotsignoff(userrec *u, nickrec *n);
void	User_displayaccount();
void	User_displaystats();
void	User_checkcrucial();
userrec	*User_addnewuser(char *acctname);
void	User_moveaccount(userrec *ufrom, userrec *uto, acctrec *a);
noterec	*User_addnewnote(userrec *u_recp, userrec *u_from, acctrec *a_from, char *text, u_long flags);
void	User_makenicklist(char *nicks, userrec *u);
void	User_resetallpointers();
void	User_releaseaccounts(userrec *u, char *nick);
void	User_releaseallaccounts(userrec *u);
void	User_moveaccounts_into_newhome(userrec *u, char *nick);
int	User_isuhregistered(userrec *u, char *uh);
int	User_highestregnumber();
nickrec	*User_nick(userrec *u, char *nick);
void	User_updateregaccounts(userrec *u, char *uh);
void	User_marknickswithregaccts(userrec *u);
void	User_gotactivity(userrec *u, acctrec *a);
void	User_nickviadccon(userrec *u, dccrec *d);
void	User_nickviatelneton(userrec *u, dccrec *d);
void	User_nickviatelnetoff(userrec *u, dccrec *d);
void	User_nickviadccoff(userrec *u, dccrec *d);
void	User_carryoverallactivitytimes();
void	User_resetallactivitytimes();
void	User_signoffallnicks();
void	User_makeweekdates(char *week);
void	User_makeweeksusage(char *week, userrec *u);
void	User_makeircfreq(char *ircfreq, userrec *u);
u_long	User_timeperweek(userrec *u);
int	User_informuserofnote(userrec *u, acctrec *a_from);
void	User_notifyofnotesonsignon(userrec *u, nickrec *n);
gretrec	*User_greetbyindex(userrec *u, int num);
int	User_deletemarkedgreets(userrec *u);
void	User_pickgreet(userrec *u, char *greet);
int	User_num_hit_greets(userrec *u);
int	User_num_unhit_greets(userrec *u);
void	User_unset_all_greet_hits(userrec *u);
void	User_checkforsignon(char *nick);
acctrec	*User_lastseentype(userrec *u, char type);
acctrec	*User_lastseen(userrec *u);
acctrec	*User_lastseenunregisterednick(userrec *u, char *nick);
userrec	*User_userbyindex(int index);
userrec	*User_userbyorder(int ordernum);
int	User_orderbyreg();
int	User_orderbyreg_plusflags(u_long flags);
int	User_orderbyacctname();
int	User_orderbyacctname_plusflags(u_long flags);
void	User_clearorder();
int	User_orderaccountbyreg(userrec *u);
void	User_clearaccountorder(userrec *u);
acctrec	*User_accountbyorder(userrec *u, int ordernum);
u_long	User_sizeof();
int	User_timeoutaccounts();
int	User_timeoutusers();
int	User_ordernumberforuser(userrec *u);
void	User_resetuserstatistics();
void	User_sendnightlyreport();


/* ASDF */
/*****************************************************************************/
/* record userrec declaration */
struct userrec_node {
	userrec	*next;			/* -> next */

	char	*acctname;		/* account name */
	nickrec	*nicks;			/* -> list of verified nicks */
	nickrec *n_last;		/* -> last nick record */
	int	num_nicks;		/* number of nicks */
	acctrec	*accounts;		/* -> list of accounts used */
	acctrec	*a_last;		/* -> last account record */
	int	num_accounts;		/* number of accounts */
	char	*pass;			/* password (encrypted) */
	char	*finger;		/* finger info */
	char	*email;			/* email info */
	char	*www;			/* www info */
	char	*forward;		/* forward address for Notes */

/*
 * This is kind of goofy with all these other lines and plan lines.  But
 * I originally started out with just a few of each and it was easier to
 * hardwire more lines in than making an array of pointers like I *should*
 * have done, and will do eventually.
 */
	char	*other1;		/* other1 */
	char	*other2;		/* other2 */
	char	*other3;		/* other3 */
	char	*other4;		/* other4 */
	char	*other5;		/* other5 */
	char	*other6;		/* other6 */
	char	*other7;		/* other7 */
	char	*other8;		/* other8 */
	char	*other9;		/* other9 */
	char	*other10;		/* other10 */
	char	*plan1;			/* plan */
	char	*plan2;			/* plan */
	char	*plan3;			/* plan */
	char	*plan4;			/* plan */
	char	*plan5;			/* plan */
	char	*plan6;			/* plan */
	int	bday[3];		/* birthday stuff */
	gretrec	*greets;		/* greet lines and accessed bits */
	gretrec	*lastgreet;		/* last greet record */
	int	num_greets;		/* num greet lines */
	noterec	*notes;			/* -> list of notes */
	noterec	*lastnote;		/* -> last note */
	int	num_notes;		/* num notes */
	int	location;		/* location id # */
	int	registered;		/* # of user (0 if not registered )*/
	time_t	time_registered;	/* time when user registered */
	int	bot_commands;		/* # of bot commands executed */
	u_long	flags;			/* levels */
	time_t	first_activity;		/* time we first saw the user */
	time_t	last_seen;		/* last seen for the USER */
	time_t	last_seen_onchan;	/* time last seen on a channel */
	time_t	last_login;	/* time user last logged in */
	time_t	seen_motd;		/* when user last saw motd */
	u_long	time_seen[7];		/* time seen per day of week */
	u_long	public_chatter[7];	/* public chatter for this user */
	int	bans[7];		/* bans per week */
	int	kicks[7];		/* kicks per week */
	int	via_web;		/* has this user logged in via web */
	char	*signoff;		/* Last signoff message */

	/* account history */
	struct listmgr_t *history;

	/* Not stored in database */
	char	*cmd_pending_pass;	/* command pending password */
	char	*cmdnick_pending_pass;	/* nick of user who sent cmd */
	time_t	cmdtime_pending_pass;	/* time command was sent */
	int	order;			/* Used for ordering lists */
};
	
struct userrec_history {
	char *info;
	char *who;
	time_t date;
} ;

/* record userrec low-level function declarations. */
userrec	*userrec_new();
void	userrec_freeall(userrec **list, userrec **last);
void	userrec_append(userrec **list, userrec **last, userrec *x);
void	userrec_delete(userrec **list, userrec **last, userrec *x);
void	userrec_movetofront(userrec **list, userrec **last, userrec *lastchecked, userrec *x);

/* record userrec low-level data-specific function declarations. */
void	userrec_init(userrec *u);
u_long	userrec_sizeof(userrec *u);
void	userrec_free(userrec *u);
void	userrec_setacctname(userrec *u, char *acctname);
void	userrec_setpass(userrec *u, char *pass);
void	userrec_setfinger(userrec *u, char *finger);
void	userrec_setemail(userrec *u, char *email);
void	userrec_setwww(userrec *u, char *www);
void	userrec_setforward(userrec *u, char *forward);
void	userrec_setother(userrec *u, char *other);
void	userrec_setother1(userrec *u, char *other1);
void	userrec_setother2(userrec *u, char *other2);
void	userrec_setother3(userrec *u, char *other3);
void	userrec_setother4(userrec *u, char *other4);
void	userrec_setother5(userrec *u, char *other5);
void	userrec_setother6(userrec *u, char *other6);
void	userrec_setother7(userrec *u, char *other7);
void	userrec_setother8(userrec *u, char *other8);
void	userrec_setother9(userrec *u, char *other9);
void	userrec_setother10(userrec *u, char *other10);
void	userrec_setplan(userrec *u, char *plan);
void	userrec_setplanx(userrec *u, char *plan, int x);
void	userrec_setplan1(userrec *u, char *plan);
void	userrec_setplan2(userrec *u, char *plan);
void	userrec_setplan3(userrec *u, char *plan);
void	userrec_setplan4(userrec *u, char *plan);
void	userrec_setplan5(userrec *u, char *plan);
void	userrec_setplan6(userrec *u, char *plan);
void	userrec_setsignoff(userrec *u, char *signoff);
void	userrec_setcmd_pending_pass(userrec *u, char *cmd);
void	userrec_setcmdnick_pending_pass(userrec *u, char *nick);
void	userrec_display(userrec *userlist);
userrec *userrec_user(userrec **list, userrec **last, char *nick);



/*****************************************************************************/
/* record nickrec declaration */
struct nickrec_node {
	nickrec	*next;			/* -> next */
	char	*nick;			/* nickname */
	acctrec	*account;		/* account on irc with */
	char	flags;			/* on irc? */
	time_t	joined;			/* if on irc, when joined */
};

/* record nickrec low-level function declarations. */
nickrec	*nickrec_new();
void	nickrec_freeall(nickrec **list, nickrec **last);
void	nickrec_append(nickrec **list, nickrec **last, nickrec *x);
void	nickrec_delete(nickrec **list, nickrec **last, nickrec *x);
void	nickrec_movetofront(nickrec **list, nickrec **last, nickrec *lastchecked, nickrec *x);

/* record nickrec low-level data-specific function declarations. */
void	nickrec_init(nickrec *n);
u_long	nickrec_sizeof(nickrec *n);
void	nickrec_free(nickrec *n);
void	nickrec_setnick(nickrec *n, char *nick);
void	nickrec_display(nickrec *n);
int	nickrec_isnick(nickrec **list, nickrec **last, char *nick);
void	nickrec_sort(nickrec **list, nickrec **last);
nickrec	*nickrec_nick(nickrec **list, nickrec **last, char *nick);


/*****************************************************************************/
/* record acctrec declaration */
struct acctrec_node {
	acctrec	*next;			/* -> next */

	/* stored in userfile */
	char	*nick;			/* nick */
	char	*uh;			/* userhost */
	int	registered;		/* is host registered? */
	time_t	first_seen;		/* when host was first seen EVER */
	time_t	last_seen;		/* when host was last seen */

	/* not stored on disk */
	char	flags;			/* flags */
	int	order;			/* used for ordering lists */
};

#define ACCT_NUM_USERFILE_ELEMENTS 5

/* record acctrec low-level function declarations. */
acctrec	*acctrec_new();
void	acctrec_freeall(acctrec **list, acctrec **last);
void	acctrec_append(acctrec **list, acctrec **last, acctrec *x);
void	acctrec_delete(acctrec **list, acctrec **last, acctrec *x);
void	acctrec_unlink(acctrec **list, acctrec **last, acctrec *x);
void	acctrec_movetofront(acctrec **list, acctrec **last, acctrec *lastchecked, acctrec *x);

/* record acctrec low-level data-specific function declarations. */
void	acctrec_init(acctrec *a);
u_long	acctrec_sizeof(acctrec *a);
void	acctrec_free(acctrec *a);
void	acctrec_setnick(acctrec *a, char *nick);
void	acctrec_setuh(acctrec *a, char *uh);
void	acctrec_display(acctrec *a);
acctrec	*acctrec_account(acctrec **list, acctrec **last, char *nick, char *uh);



/*****************************************************************************/
/* record greetrec declaration */
struct gretrec_node {
	gretrec	*next;			/* -> next */

	char	*text;			/* greet text */
	char	flags;			/* flags */
};

/* record gretrec low-level function declarations. */
gretrec	*gretrec_new();
void	gretrec_freeall(gretrec **list, gretrec **last);
void	gretrec_append(gretrec **list, gretrec **last, gretrec *x);
void	gretrec_delete(gretrec **list, gretrec **last, gretrec *x);

/* record gretrec low-level data-specific function declarations. */
void	gretrec_init(gretrec *g);
u_long	gretrec_sizeof(gretrec *g);
void	gretrec_free(gretrec *g);
void	gretrec_settext(gretrec *g, char *text);



/*****************************************************************************/
/* User flags */
#define USER_OP		0x00000001  /* o   bot will op the user */
#define USER_GREET	0x00000002  /* g   user is greeted */
#define USER_REGD	0x00000004  /* r   user is registered */
#define USER_OK		0x00000008  /* k   user has > default privelege */
#define USER_MASTER	0x00000010  /* m   user has full bot access */
#define USER_FRIEND	0x00000020  /* f   user is unbanned if banned */
#define USER_XFER	0x00000040  /* x   user has file area access */
#define USER_PROTECT	0x00000080  /* p   user is prot'd from deop,kick,ban */
#define USER_31337	0x00000100  /* 3   user is 31337 */
#define USER_TRUSTED	0x00000200  /* t   user has special priveleges */
#define USER_BOT	0x00000400  /* b   user is a bot. */
#define USER_RESPONSE	0x00000800  /* L   user can add and delete responses */
#define USER_LYRICS	0x00001000  /* l   user can add and delete lyrics */
#define USER_VOICE	0x00002000  /* v   user can talk on moderated channel */
#define USER_NOTICES	0x00004000  /* n   user is sent notices */
#define USER_OWNER	0x00008000  /* w   user is owner (cannot be deleted) */
#define USER_NOLEVEL	0x00010000  /* 0   no levels will allow this command */
#define USER_ANYONE	0x00020000  /* a   anyone can access this command */
#define USER_INVALID	0x00040000  /* ?   level not listed */

/*****************************************************************************/
/* Nick flags */
#define NICK_ONIRC		0x01	/* on IRC and matches a verified HM */
#define NICK_PENDING		0x02	/* waiting for USERHOST results */
#define NICK_HAS_ACCT_ENTRY	0x04	/* Has an account entry that's REG'd */
#define NICK_VIADCC		0x08	/* Nick is connected via DCC */
#define NICK_VIATELNET		0x10	/* Nick is connected via telnet */

/*****************************************************************************/
/* Account flags */
#define ACCT_ONIRC		0x01	/* on IRC.  may or may not match */
#define ACCT_VIADCC		0x02	/* via DCC */
#define ACCT_VIATELNET		0x04	/* via telnet */

/*****************************************************************************/
/* Greet flags */
#define GREET_DELETEME		0x01	/* marked for delete */
#define GREET_HIT		0x02	/* marked as hit */

#endif /* USER_H */
