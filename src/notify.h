/******************************************************************************
 * Steve Rider - stever@area23.org
 * Grufti - an interactive, service-oriented bot for IRC 
 *
 * notify.h - keep track of who's on IRC.  Uses the nickrec field in user.h
 *            to indicate.
 *
 *****************************************************************************/
/* 28 April 97 */

#ifndef NOTIFY_H
#define NOTIFY_H

#include <stdio.h>
#include "typedefs.h"
#include "grufti.h"

/* Let all files who need Notify have access to the Notify object */
extern struct Notify_object *Notify;


/*****************************************************************************/
/* object Notify declaration */
typedef struct Notify_object {

	time_t	last_radiated;		/* time last radiated */
	time_t	last_radiated_msg;	/* time a single message was radiated */
	char	flags;			/* flags */
	int	num_queries;		/* how many queries were sent */
	char	uh_buf[BUFSIZ];		/* fast buffer */

	radirec	*radiate_queue;		/* -> radiate queue */
	radirec	*radiate_last;		/* -> last radiate queue message */
	int	num_nicks;		/* num messages in queue */
	generec	*genericlist;		/* -> genericlist */
	generec	*genericlast;		/* -> genericlast element */
	int	num_generics;		/* num generic lines in list */

} Notify_ob;

/* object Notify low-level function declarations. */
void	Notify_new();
u_long	Notify_sizeof();
void	Notify_freeall();

/* object Notify structure function declarations */
void	Notify_appendnick(radirec *r);
void	Notify_deletenick(radirec *r);
void	Notify_freeallnicks();
void	Notify_appendgeneric(generec *f);
void	Notify_deletegeneric(generec *f);
void	Notify_freeallgenerics();

/* object Notify high-level implementation-specific function declarations. */
void	Notify_loadradiatequeue();
void	Notify_pushqueue(char *nick);
void	Notify_sendmsgfromqueue();
int	Notify_waitingfor302();
int	Notify_currently_radiating();
void	Notify_dumpqueue();
int	Notify_orderbylogin();
void	Notify_clearorder();
generec	*Notify_genericbyorder(int ordernum);
void	Notify_addtogenericlist(char *display, time_t login);



/* ASDF */
/*****************************************************************************/
/* record radirec declaration */
struct radirec_node {
	radirec	*next;			/* -> next */

	char	*nick;			/* each nick to check */
} ;

/* record radirec low-level function declarations. */
radirec	*radirec_new();
void	radirec_freeall(radirec **list, radirec **last);
void	radirec_append(radirec **list, radirec **last, radirec *x);
void	radirec_delete(radirec **list, radirec **last, radirec *x);

/* record radirec low-level data-specific function declarations. */
void	radirec_init(radirec *x);
u_long	radirec_sizeof(radirec *x);
void	radirec_free(radirec *x);
void	radirec_setnick(radirec *x, char *nick);

/*****************************************************************************/
/* record generec declaration */
struct generec_node {
	generec	*next;			/* -> next */

	char	*display;		/* display line */
	time_t	login;			/* login time */
	int	order;			/* used for ordering lists */
} ;

/* record generec low-level function declarations. */
generec	*generec_new();
void	generec_freeall(generec **list, generec **last);
void	generec_append(generec **list, generec **last, generec *x);
void	generec_delete(generec **list, generec **last, generec *x);

/* record generec low-level data-specific function declarations. */
void	generec_init(generec *x);
u_long	generec_sizeof(generec *x);
void	generec_free(generec *x);
void	generec_setdisplay(generec *x, char *display);




/*****************************************************************************/
/* Notify flags */
#define NOTIFY_PENDING		0x01	/* we're waiting on RPL_USERHOST */
#define NOTIFY_ACTIVE		0x02	/* notifylist is active */
#define NOTIFY_WAITING_FOR_302	0x04	/* waiting on 302 */
#define NOTIFY_CURRENTLY_RADIATING 0x08	/* in the middle of a radiate */

#endif /* NOTIFY_H */
