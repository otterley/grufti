/******************************************************************************
 * Steve Rider - stever@area23.org
 * Grufti - an interactive, service-oriented bot for IRC 
 *
 * channel.h - channel storage and operations
 *
 *****************************************************************************/
/* 28 April 97 */

#ifndef CHANNEL_H
#define CHANNEL_H

#include <sys/time.h>
#include <time.h>
#include "typedefs.h"
#include "grufti.h"

/* Let all files who need Channel have access to the Channel object */
extern struct Channel_object *Channel;



/*****************************************************************************/
/* object Channel declaration */
typedef struct Channel_object {
	chanrec	*channels;		/* -> the list of channels */
	chanrec *last;			/* -> the last channel */

	int	num_channels;		/* how many we're on */

	time_t	time_tried_join;	/* last time we joined unjoined chans */

	int	stats_active;

} Channel_ob;


/* object Channel low-level function declarations. */
void	Channel_new();
u_long	Channel_sizeof();
void	Channel_freeall();

/* object Channel data-specific function declarations. */
void	Channel_display();

/* low-level Channel structure function declarations. */
void	Channel_appendchan(chanrec *chan);
void	Channel_deletechan(chanrec *chan);
void	Channel_freeallchans();
void	Channel_appendmember(chanrec *chan, membrec *m);
void	Channel_deletemember(chanrec *chan, membrec *m);
void	Channel_freeallmembers(chanrec *chan);
void	Channel_appendban(chanrec *chan, banrec *ban);
void	Channel_deleteban(chanrec *chan, banrec *ban);
void	Channel_freeallbans(chanrec *chan);

/* object Channel high-level implementation-specific function declarations. */
chanrec	*Channel_addchannel(char *name);
void	Channel_setchannel(char *name, char *param, char *value);
void	Channel_join(chanrec *c);
void	Channel_joinunjoinedchannels();
chanrec	*Channel_channel(char *name);
membrec	*Channel_member(chanrec *chan, char *nick);
banrec	*Channel_ban(chanrec *chan, char *ban);
void	Channel_resetchaninfo(chanrec *chan);
banrec	*Channel_addban(chanrec *chan, char *ban, char *nick, char *uh, time_t t);
void	Channel_removeban(chanrec *chan, char *ban);
membrec	*Channel_addnewmember(chanrec *c, char *nick, char *uh);
int	Channel_verifychanactive(chanrec *chan);
membrec	*Channel_memberofanychan(char *nick);
int	Channel_ismemberofanychan(char *nick);
void	Channel_clearallchaninfo();
void	Channel_clearchaninfo(chanrec *chan);
void	Channel_removemember(chanrec *chan, membrec *m);
void	Channel_resetallpointers();
void	Channel_setallchannelsoff();
void	Channel_modes2str(chanrec *chan, char *mode);
void	Channel_chanflags2str(int flags, char *str);
int	Channel_ihaveops(chanrec *chan);
void	Channel_cycle(chanrec *chan);
int	Channel_num_members_notsplit(chanrec *chan);
void	Channel_checkonlymeonchannel(chanrec *chan);
void	Channel_makechanlist(char *channels);
void	Channel_giveops(chanrec *chan, membrec *m);
void	Channel_givevoice(chanrec *chan, membrec *m);
void	Channel_checkfortimeout();
int	Channel_orderbyjoin_htol(chanrec *chan);
int	Channel_orderbyjoin_ltoh(chanrec *chan);
void	Channel_clearorder(chanrec *chan);
membrec	*Channel_memberbyorder(chanrec *chan, int ordernum);
void	Channel_updatestatistics();
int	Channel_gotallchaninfo(chanrec *chan);
void	Channel_trytogiveops(char *nick);


/* ASDF */
/*****************************************************************************/
/* record chanrec declaration */
struct chanrec_node {
	chanrec	*next;			/* -> next */

	/* fixed */
	char	*name;			/* name of channel */
	int	limit_keep;		/* keep limit */
	char	*key_keep;		/* keep key */
	char	*topic_keep;		/* keep topic */
	char	*mode_keep;		/* keep mode */
	u_long	flags;			/* channel flags */

	/* reflects latest changes */
	u_long	modes;			/* channel modes */
	int	limit;			/* channel limit */
	char	*key;			/* current channel key */
	char	*topic;			/* current channel topic */
	membrec	*memberlist;		/* -> list of members on channel */
	membrec *m_last;		/* -> last member in list */
	int	num_members;		/* number of members on channel */
	banrec	*banlist;		/* -> list of bans on channel */
	banrec	*b_last;		/* -> last ban in list */
	int	num_bans;		/* number of bans on channel */
	char	*nojoin_reason;		/* Can't join reason */
	time_t	resp_boxes[RESPRATE_NUM];	/* how much is said on chan */
	int	resp_boxes_pos;		/* current rotating box position */
	time_t	start_public_flood;	/* when public flood started */
	time_t	joined;			/* when we joined this channel */
	time_t	leave_time;		/* When we were asked to leave */
	time_t	leave_timeout;		/* How long to stay off channel */
	time_t	hush_time;		/* When we were asked to hush */
	time_t	hush_timeout;		/* How long to stay hushed */
	time_t	plus_i_idlekick_time;	/* When we set +i to kick idlers */
	time_t	last_checked_idlers;	/* Check for idlers every so often */

	/* stats */
	int	joins_reg[24];		/* # joins in the last 24 hours */
	int	joins_nonreg[24];	/* # joins in the last 24 hours */
	u_long	chatter[24];		/* how much chatter in last 24 hrs */
	int	population[24];		/* average population in last 24 hrs */
	int	pop_prod;		/* population product */
	int	pop_currentpop;		/* population we're currently at */
	time_t	pop_timestarted;	/* time we first saw this population */
	int	min_population;		/* min population of channel */
	int	max_population;		/* max population of channel */
	time_t	min_population_time;	/* time of max population */
	time_t	max_population_time;	/* time of min population */
};


/* record chanrec low-level function declarations. */
chanrec	*chanrec_new();
void	chanrec_freeall(chanrec **chanlist, chanrec **last);
void	chanrec_append(chanrec **list, chanrec **last, chanrec *entry);
void	chanrec_delete(chanrec **list, chanrec **last, chanrec *x);
void	chanrec_movetofront(chanrec **list, chanrec **last, chanrec *lastchecked, chanrec *c);

/* record chanrec low-level data-specific function declarations. */
void	chanrec_init(chanrec *chan);
u_long	chanrec_sizeof(chanrec *c);
void	chanrec_free(chanrec *c);
void	chanrec_setname(chanrec *chan, char *name);
void	chanrec_setkey_keep(chanrec *chan, char *key);
void	chanrec_settopic_keep(chanrec *chan, char *key);
void	chanrec_setmode_keep(chanrec *chan, char *key);
void	chanrec_setflags(chanrec *chan, int flags);
void	chanrec_setkey(chanrec *chan, char *key);
void	chanrec_settopic(chanrec *chan, char *key);
void	chanrec_setnojoin_reason(chanrec *chan, char *reason);
void	chanrec_display(chanrec *chanlist);
chanrec	*chanrec_channel(chanrec **list, chanrec **last, char *name);



/*****************************************************************************/
/* record membrec declaration */
struct membrec_node {
	membrec *next;			/* -> next */

	char	*nick;			/* nick of member */
	char	*uh;			/* userhost of member */
	userrec	*user;			/* -> user record */
	acctrec	*account;		/* -> account record */
	char	flags;			/* flags on channel */
	time_t	joined;			/* time joined */
	time_t	split;			/* to determine if split, how long */
	time_t	last;			/* last seen (channel activity only ) */
	int	order;			/* Used to order lists */
};

/* record membrec low-level function declarations.  Never changes. */
membrec	*membrec_new();
void	membrec_freeall(membrec **memberlist, membrec **last);
void	membrec_append(membrec **list, membrec **last, membrec *entry);
void	membrec_delete(membrec **list, membrec **last, membrec *x);
void	membrec_movetofront(membrec **list, membrec **last, membrec *lastchecked, membrec *c);

/* record membrec low-level data-specific function declarations. */
void	membrec_init(membrec *m);
u_long	membrec_sizeof(membrec *m);
void	membrec_free(membrec *m);
void	membrec_setnick(membrec *m, char *nick);
void	membrec_setuh(membrec *m, char *uh);
void	membrec_display(membrec *memberlist);
membrec	*membrec_member(membrec **list, membrec **last, char *nick);



/*****************************************************************************/
/* banlist structure */
struct banrec_node {
	struct banrec_node *next;		/* -> next */

	char	*ban;
	char	*who_n;
	char	*who_uh;
	time_t	time_set;
	
};

/* record banrec low-level function declarations.  Never changes. */
banrec	*banrec_new();
void	banrec_freeall(banrec **banlist, banrec **last);
void	banrec_append(banrec **list, banrec **last, banrec *entry);
void	banrec_delete(banrec **list, banrec **last, banrec *x);
void	banrec_movetofront(banrec **list, banrec **last, banrec *lastchecked, banrec *b);

/* record banrec low-level data-specific function declarations. */
void	banrec_init(banrec *b);
u_long	banrec_sizeof(banrec *b);
void	banrec_free(banrec *b);
void	banrec_setban(banrec *b, char *ban);
void	banrec_setwho_n(banrec *b, char *who_n);
void	banrec_setwho_uh(banrec *b, char *who_uh);
void	banrec_display(banrec *banlist);
banrec	*banrec_ban(banrec **list, banrec **last, char *ban);



/*****************************************************************************/
/* channel flags */
#define CHAN_ACTIVE		0x0001	/* active on channel */
#define CHAN_PENDING		0x0002	/* waiting for end of WHO */
#define CHAN_ONCHAN		0x0004	/* simply on the channel */

#define CHAN_PERMANENT		0x0008	/* This channel is hardwired */
#define CHAN_FORCEKEY		0x0010	/* We're enforcing the key */
#define CHAN_FORCETOPIC		0x0020	/* We're enforcing the topic */
#define CHAN_FORCEMODE		0x0040	/* We're enforcing the mode */
#define CHAN_FORCELIMIT		0x0080	/* We're enforcing the limit */

#define CHAN_PUBLICFLOOD	0x0100	/* we're in a public flood */
#define CHAN_HUSH		0x0200	/* we've been asked to HUSH */
#define CHAN_LEAVE		0x0400	/* we've been asked to LEAVE */
#define CHAN_SHOWKEY    0x0800  /* show the channel key in public output */

/* Channel modes */
#define CHANMODE_PRIVATE	0x00001	/* +p	Channel is private */
#define CHANMODE_SECRET		0x00002	/* +s	Channel is secret */
#define CHANMODE_INVITEONLY	0x00004	/* +i	Channel is invite-only */
#define CHANMODE_TOPIC		0x00008	/* +t	Topic is set only by oper */
#define CHANMODE_NOMSG		0x00010	/* +n	No messages from outside # */
#define CHANMODE_MODERATED	0x00020	/* +m	Only +v and +o can talk on # */
#define CHANMODE_LIMIT		0x00040	/* +l	Channel has limit ( ->limit) */
#define CHANMODE_KEY		0x00080	/* +k	Channel has a key ( ->key) */
#define CHANMODE_ANONYMOUS	0x00100	/* +a	Channel is anonymous */
#define CHANMODE_QUIET		0x00200	/* +q	Channel is quiet */
#define CHANMODE_GOTTOPIC	0x00400 /* Got end of TOPIC */
#define CHANMODE_GOTWHO		0x00800 /* Got end of WHO */
#define CHANMODE_GOTBANS	0x01000 /* Got end of BANS */
#define CHANMODE_GOTMODES	0x02000 /* Got end of MODES */

/* Member flags */
#define MEMBER_OPER		0x0001	/* +o	Member has +o */
#define MEMBER_VOICE		0x0002	/* +v	Member has +v */
#define MEMBER_AWAY		0x0004	/* Member is away */
#define MEMBER_SERVEROP		0x0008	/* Ops are server ops */
#define MEMBER_PENDINGOP	0x0010	/* Waiting for mode to happen */
#define MEMBER_PENDINGDEOP	0x0020	/* Waiting for mode to happen */
#define MEMBER_PENDINGBAN	0x0040	/* Waiting for mode to happen */
#define MEMBER_PENDINGVOICE	0x0080	/* Waiting for mode to happen */


#endif /* CHANNEL_H */
