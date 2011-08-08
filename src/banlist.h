/******************************************************************************
 * Steve Rider - stever@area23.org
 * Grufti - an interactive, service-oriented bot for IRC 
 *
 * banlist.h - ban records
 *
 *****************************************************************************/
/* 28 April 97 */

#ifndef BANLIST_H
#define BANLIST_H

#include <stdio.h>
#include "typedefs.h"
#include "grufti.h"

/* Let all files who need Banlist have access to the Banlist object */
extern struct Banlist_object *Banlist;


/*****************************************************************************/
/* object Banlist declaration */
typedef struct Banlist_object {
	banlrec	*banlist;		/* -> list of bans */
	banlrec	*last;			/* -> last ban record */

        int	num_bans;		/* number of ban records */
	u_long	flags;			/* flags (ACTIVE | NEEDSWRITE) */

} Banlist_ob;

/* object Banlist low-level function declarations. */
void	Banlist_new();
u_long	Banlist_sizeof();
void	Banlist_freeall();

/* low-level Banlist structure function declarations. */
void	Banlist_appendban(banlrec *b);
void	Banlist_deleteban(banlrec *b);
void	Banlist_freeallbans();

/* object Banlist high-level implementation-specific function declarations. */
void	Banlist_clearorder();
int	Banlist_orderbysettime();
banlrec	*Banlist_banbyordernum(int ordernum);
banlrec	*Banlist_addban(char *nick, char *ban, chanrec *chan, time_t expires, time_t t);
banlrec	*Banlist_ban(char *ban);
void	Banlist_writebans();
void	Banlist_loadbans();
void	Banlist_checkfortimeouts();
banlrec	*Banlist_match(char *nuh, chanrec *chan);
int	Banlist_verifyformat(char *ban);


/* ASDF */
/*****************************************************************************/
/* record banlrec declaration */
struct banlrec_node {
	banlrec	*next;			/* -> next */

	char	*nick;			/* who set it */
	char	*ban;			/* ban string */
	time_t	time_set;		/* settime */
	time_t	expires;		/* when ban expires */
	int	used;			/* # times ban was used */
	u_long	flags;			/* banlist flags */
	chanrec	*chan;			/* The channel. if NULL, then global */
	int	order;			/* order */
} ;

/* record banlrec low-level function declarations. */
banlrec	*banlrec_new();
void	banlrec_freeall(banlrec **list, banlrec **last);
void	banlrec_append(banlrec **list, banlrec **last, banlrec *x);
void	banlrec_delete(banlrec **list, banlrec **last, banlrec *x);
void	banlrec_movetofront(banlrec **list, banlrec **last, banlrec *lastchecked, banlrec *x);

/* record banlrec low-level data-specific function declarations. */
void	banlrec_init(banlrec *b);
u_long	banlrec_sizeof(banlrec *b);
void	banlrec_free(banlrec *b);
void	banlrec_setnick(banlrec *b, char *nick);
void	banlrec_setban(banlrec *b, char *ban);



/*****************************************************************************/
/* Banlist flags */
#define BANLIST_ACTIVE		0x0001	/* If the list is active */
#define BANLIST_NEEDS_WRITE	0x0002	/* so we don't write when unnecessary */

#endif /* BANLIST_H */
