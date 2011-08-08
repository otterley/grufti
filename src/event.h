/******************************************************************************
 * Steve Rider - stever@area23.org
 * Grufti - an interactive, service-oriented bot for IRC 
 *
 * event.h - Event lists
 *
 *****************************************************************************/
/* 3 January 98 */

#ifndef EVENT_H
#define EVENT_H

#include "typedefs.h"
#include "grufti.h"

/* Let all files who need Event have access to the Event object */
extern struct Event_object *Event;



/*****************************************************************************/
/* object Event declaration */
typedef struct Event_object {
	evntrec	*events;		/* -> the list of events */
	evntrec *last;			/* -> the last event */

	int	num_events;		/* how many we have */

	u_long	flags;			/* flags (ACTIVE | NEEDS_WRITE) */

} Event_ob;


/* object Event low-level function declarations. */
void	Event_new();
u_long	Event_sizeof();
void	Event_freeall();

/* low-level Event structure function declarations. */
void	Event_appendevent(evntrec *event);
void	Event_deleteevent(evntrec *event);
void	Event_freeallevents();
void	Event_appendattendee(evntrec *event, attnrec *a);
void	Event_deleteattendee(evntrec *event, attnrec *a);
void	Event_freeallattendees(evntrec *event);

/* object Channel high-level implementation-specific function declarations. */
evntrec	*Event_event(int id_num);
evntrec	*Event_addnewevent(char *name);
attnrec	*Event_addnewattendee(evntrec *event, char *nick);
void	Event_loadevents();
void	Event_writeevents();
int	Event_newidnumber();
int	Event_orderbynick(evntrec *event);
void	Event_clearorder(evntrec *event);
attnrec	*Event_attendeebyorder(evntrec *event, int ordernum);
attnrec	*Event_attendee(evntrec *event, char *nick);



/* ASDF */
/*****************************************************************************/
/* record evntrec declaration */
struct evntrec_node {
	evntrec	*next;			/* -> next */

	int	id;			/* Id number */
	char	*name;			/* name of event */
	char	*place;			/* place */
	char	*when;			/* when */
	char	*about;			/* what's it about */
	char	*admin;			/* admin */

	attnrec	*attendees;		/* -> list of attendees for event */
	attnrec *attendees_last;	/* -> last attendee in list */
	int	num_attendees;		/* number of attendees for event */

	time_t	time_created;		/* When event was created */

	u_long	flags;			/* flags */
	int	order;			/* Used to order lists */
};


/* record evntrec low-level function declarations. */
evntrec	*evntrec_new();
void	evntrec_freeall(evntrec **list, evntrec **last);
void	evntrec_append(evntrec **list, evntrec **last, evntrec *entry);
void	evntrec_delete(evntrec **list, evntrec **last, evntrec *x);
void	evntrec_movetofront(evntrec **list, evntrec **last, evntrec *lastchecked, evntrec *x);

/* record evntrec low-level data-specific function declarations. */
void	evntrec_init(evntrec *event);
u_long	evntrec_sizeof(evntrec *event);
void	evntrec_free(evntrec *event);
void	evntrec_setname(evntrec *event, char *name);
void	evntrec_setplace(evntrec *event, char *place);
void	evntrec_setwhen(evntrec *event, char *when);
void	evntrec_setabout(evntrec *event, char *about);
void	evntrec_setadmin(evntrec *event, char *admin);
evntrec	*evntrec_event(evntrec **list, evntrec **last, int id_num);



/*****************************************************************************/
/* record attnrec declaration */
struct attnrec_node {
	attnrec *next;			/* -> next */

	char	*nick;			/* nick of attendee */
	char	*when;			/* when attendee will be at event */
	char	*contact;		/* contact while at event */
	int	order;			/* Used to order lists */
};

/* record attnrec low-level function declarations.  Never changes. */
attnrec	*attnrec_new();
void	attnrec_freeall(attnrec **list, attnrec **last);
void	attnrec_append(attnrec **list, attnrec **last, attnrec *entry);
void	attnrec_delete(attnrec **list, attnrec **last, attnrec *x);
void	attnrec_movetofront(attnrec **list, attnrec **last, attnrec *lastchecked, attnrec *x);

/* record attnrec low-level data-specific function declarations. */
void	attnrec_init(attnrec *a);
u_long	attnrec_sizeof(attnrec *a);
void	attnrec_free(attnrec *a);
void	attnrec_setnick(attnrec *a, char *nick);
void	attnrec_setwhen(attnrec *a, char *when);
void	attnrec_setcontact(attnrec *a, char *contact);
attnrec	*attnrec_attendee(attnrec **list, attnrec **last, char *nick);



/*****************************************************************************/
/* event flags */
#define EVENT_ACTIVE		0x0001	/* if the list is active */
#define EVENT_NEEDS_WRITE	0x0002	/* so we don't write when unnecessary */

#endif /* EVENT_H */
