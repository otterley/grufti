/******************************************************************************
 * Steve Rider - stever@area23.org
 * Grufti - an interactive, service-oriented bot for IRC 
 *
 * event.c - Event lists
 *
 *****************************************************************************/
/* 28 April 97 */

#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include "grufti.h"
#include "gruftiaux.h"
#include "event.h"
#include "misc.h"
#include "log.h"
#include "user.h"

/****************************************************************************
 *
 * Event object definitions
 *
 ****************************************************************************/

void Event_new()
{
	/* already in use? */
	if(Event != NULL)
		Event_freeall();

	/* allocate memory */
	Event = (Event_ob *)xmalloc(sizeof(Event_ob));

	/* initialize */
	Event->events = NULL;
	Event->last = NULL;

	Event->num_events = 0;
	Event->flags = 0L;
}

u_long Event_sizeof()
{
	u_long	tot = 0L;
	evntrec	*e;

	tot += sizeof(Event_ob);
	for(e=Event->events; e!=NULL; e=e->next)
		tot += evntrec_sizeof(e);

	return tot;
}

void Event_freeall()
{
	/* Free all events */
	Event_freeallevents();

	/* Free the object */
	xfree(Event);
	Event = NULL;
}



/****************************************************************************/
/* low-level Event structure function definitions. */

void Event_appendevent(evntrec *event)
{
	evntrec_append(&Event->events, &Event->last, event);
	Event->num_events++;
}

void Event_deleteevent(evntrec *event)
{
	evntrec_delete(&Event->events, &Event->last, event);
	Event->num_events--;
}

void Event_freeallevents()
{
	evntrec_freeall(&Event->events, &Event->last);
	Event->num_events = 0;
}

void Event_appendattendee(evntrec *event, attnrec *a)
{
	attnrec_append(&event->attendees, &event->attendees_last, a);
	event->num_attendees++;
}

void Event_deleteattendee(evntrec *event, attnrec *a)
{
	attnrec_delete(&event->attendees, &event->attendees_last, a);
	event->num_attendees--;
}

void Event_freeallattendees(evntrec *event)
{
	attnrec_freeall(&event->attendees, &event->attendees_last);
	event->num_attendees = 0;
}



/****************************************************************************/
/* object Event high-level implementation-specific function definitions. */

evntrec *Event_event(int id_num)
{
	return evntrec_event(&Event->events, &Event->last, id_num);
}

evntrec *Event_addnewevent(char *name)
{
	evntrec *x;

	/* Allocate memory for new node */
	x = evntrec_new();

	/* Assign it some data */
	evntrec_setname(x,name);
	x->id = Event_newidnumber();
	x->time_created = time(NULL);

	/* Append the record */
	Event_appendevent(x);

	return x;
}

attnrec *Event_addnewattendee(evntrec *event, char *nick)
{
	attnrec	*a;

	/* Allocate memory for new node */
	a = attnrec_new();

	/* Assign it some data */
	attnrec_setnick(a,nick);

	/* Append the record */
	Event_appendattendee(event,a);

	return a;
}

void Event_loadevents()
/*
 * Field info:
 *    E - event name
 *    i - id
 *    p - place
 *    w - when
 *    a - about
 *    A - admin
 *    @ - attendees
 *    W - attendee when
 *    C - attendee contact
 *    t - time created
 */
{
	evntrec	*e_tmp;
	attnrec	*a_tmp;
	char	s[BUFSIZ], ident[BUFSIZ], eventfile[256];
	int	line = 0;
	FILE	*f;

	/* Open file for reading */
	sprintf(eventfile,"%s/%s",Grufti->botdir,Grufti->eventfile);
	f = fopen(eventfile,"r");
	if(f == NULL)
	{
		Event->flags |= EVENT_ACTIVE;
		return;
	}

	/* Allocate memory for new node */
	e_tmp = evntrec_new();
	a_tmp = attnrec_new();

	/* Read each line */
	while(readline(f,s))
	{
		line++;

		/* Extract identifier code */
		split(ident,s);

		/* Match identifier code */
		switch(ident[0])
		{
		    case 'i':	/* ID NUMBER */
			if(e_tmp->id != 0)
			{
				Event_appendevent(e_tmp);
				e_tmp = evntrec_new();
			}
			e_tmp->id = atoi(s);
			break;

		    /* Event name, place, when, about, admin */
		    case 'E': evntrec_setname(e_tmp,s); break;
		    case 'p': evntrec_setplace(e_tmp,s); break;
		    case 'w': evntrec_setwhen(e_tmp,s); break;
		    case 'a': evntrec_setabout(e_tmp,s); break;
		    case 'A': evntrec_setadmin(e_tmp,s); break;
		    case 't': e_tmp->time_created = (time_t)atol(s); break;

		    case '@':	/* Attendee nick */
			attnrec_setnick(a_tmp,s);
			break;

		    case 'W':	/* Attendee when */
			attnrec_setwhen(a_tmp,s);
			break;

		    case 'C':	/* Attendee contact (even if NULL) */
			if(s && s[0])
				attnrec_setcontact(a_tmp,s);
			Event_appendattendee(e_tmp,a_tmp);
			a_tmp = attnrec_new();
			break;
		}
	}
	fclose(f);

	/* read last entry */
	if(e_tmp->id != 0)
		Event_appendevent(e_tmp);

	/* now the list is active, and synced */
	Event->flags |= EVENT_ACTIVE;
	Event->flags &= ~EVENT_NEEDS_WRITE;
}

void Event_writeevents()
/*
 * Field info:
 *    E - event name
 *    i - id
 *    p - place
 *    w - when
 *    a - about
 *    A - admin
 *    @ - attendees
 *    W - attendee when
 *    C - attendee contact
 *    t - time created
 */
{
	FILE	*f;
	evntrec	*event;
	attnrec	*a;
	char	tmpfile[256], eventfile[256];
	int	x;

	/* bail if events file is inactive (pending read...) */
	if(!(Event->flags & EVENT_ACTIVE))
	{
		Log_write(LOG_MAIN,"*","Event list is not active.  Ignoring write request.");
		return;
	}

	/* Open for writing */
	sprintf(eventfile,"%s/%s",Grufti->botdir,Grufti->eventfile);
	sprintf(tmpfile,"%s.tmp",eventfile);
	f = fopen(tmpfile,"w");
	if(f == NULL)
	{
		Log_write(LOG_MAIN|LOG_ERROR,"*","Couldn't write eventfile.");
		return;
	}

	/* Write header */
	writeheader(f,"Event->events - Event lists and attendees.",eventfile);

	/* Write each record */
	for(event=Event->events; event!=NULL; event=event->next)
	{
		/* Write event id */
		if(fprintf(f,"i %d\n",event->id) == EOF)
			return;

		/* Write event name */
		if(event->name != NULL)
		{
			if(fprintf(f,"E %s\n",event->name) == EOF)
				return;
		}

		/* Write event place */
		if(event->place != NULL)
		{
			if(fprintf(f,"p %s\n",event->place) == EOF)
				return;
		}

		/* Write event when */
		if(event->when != NULL)
		{
			if(fprintf(f,"w %s\n",event->when) == EOF)
				return;
		}

		/* Write event about */
		if(event->about != NULL)
		{
			if(fprintf(f,"a %s\n",event->about) == EOF)
				return;
		}

		/* Write event admin */
		if(event->admin != NULL)
		{
			if(fprintf(f,"A %s\n",event->admin) == EOF)
				return;
		}

		/* Write all attendees */
		for(a=event->attendees; a!=NULL; a=a->next)
		{
			if(fprintf(f,"@ %s\n",a->nick) == EOF)
				return;

			if(a->when)
				if(fprintf(f,"W %s\n",a->when) == EOF)
					return;

			/* Write contact even if NULL (marks END of attendee) */
			if(a->contact)
			{
				if(fprintf(f,"C %s\n",a->contact) == EOF)
						return;
			}
			else
			{
				if(fprintf(f,"C\n") == EOF)
					return;
			}
		}
			
		/* Write time created */
		if(event->time_created != 0L)
			if(fprintf(f,"t %lu\n",event->time_created) == EOF)
				return;

		if(fprintf(f,"#####\n") == EOF)
			return;

	}
	fclose(f);

	/* Move tmpfile over to main eventfile */
	if(Grufti->keep_tmpfiles)
		x = copyfile(tmpfile,eventfile);
	else
		x = movefile(tmpfile,eventfile);

	verify_write(x,tmpfile,eventfile);
	if(x == 0)
		Log_write(LOG_MAIN,"*","Wrote eventfile.");
}

int Event_newidnumber()
{
	evntrec	*event;
	int	id_max = 0;

	/* Search through the list for the highest id, then add 1 */
	for(event=Event->events; event!=NULL; event=event->next)
		if(event->id > id_max)
			id_max = event->id;

	return id_max + 1;
}

int Event_orderbynick(evntrec *event)
{
	attnrec	*a, *a_save = NULL;
	int	counter = 0, done = 0;
	char	h[NICKLEN], highest[NICKLEN];

	/* Put the highest ascii character in 'highest'. */
	sprintf(highest,"%c",HIGHESTASCII);

	/* We need to clear all ordering before we start */
	Event_clearorder(event);

	/* Continue until all entries have an order number */
	while(!done)
	{
		done = 1;
		strcpy(h,highest);
		for(a=event->attendees; a!=NULL; a=a->next)
		{
			if(!a->order && strcasecmp(a->nick,h) <= 0)
			{
				done = 0;
				strcpy(h,a->nick);
				a_save = a;
			}
		}

		/* Not done means we got a_save and now we need to order it */
		if(!done)
		{
			counter++;
			a_save->order = counter;
		}
	}

	return counter;
}

void Event_clearorder(evntrec *event)
{
	attnrec *a;

	for(a=event->attendees; a!=NULL; a=a->next)
		a->order = 0;
}

attnrec *Event_attendeebyorder(evntrec *event, int ordernum)
{
	attnrec	*a;

	for(a=event->attendees; a!=NULL; a=a->next)
		if(a->order == ordernum)
			return a;

	return NULL;
}

attnrec *Event_attendee(evntrec *event, char *nick)
{
	attnrec	*a;

	for(a=event->attendees; a!=NULL; a=a->next)
		if(isequal(a->nick,nick))
			return a;

	return NULL;
}


/* ASDF */
/***************************************************************************
 *
 * event record definitions.  No changes should be made to new(),
 * freeall(), append(), delete(), or movetofront().
 *
 ****************************************************************************/

evntrec *evntrec_new()
{
	evntrec *x;

	/* allocate memory */
	x = (evntrec *)xmalloc(sizeof(evntrec));

	/* initialize */
	x->next = NULL;

	evntrec_init(x);

	return x;
}

void evntrec_freeall(evntrec **list, evntrec **last)
{
	evntrec *c = *list, *v;

	while(c != NULL)
	{
		v = c->next;
		evntrec_free(c);
		c = v;
	}

	*list = NULL;
	*last = NULL;
}

void evntrec_append(evntrec **list, evntrec **last, evntrec *entry)
{
	evntrec *lastentry = *last;

	if(*list == NULL)
		*list = entry;
	else
		lastentry->next = entry;

	*last = entry;
	entry->next = NULL;
}

void evntrec_delete(evntrec **list, evntrec **last, evntrec *x)
{
	evntrec *c, *lastchecked = NULL;

	c = *list;
	while(c != NULL)
	{
		if(c == x)
		{
			if(lastchecked == NULL)
			{
				*list = c->next;
				if(c == *last)
					*last = NULL;
			}
			else
			{
				lastchecked->next = c->next;
				if(c == *last)
					*last = lastchecked;
			}

			evntrec_free(c);
			return;
		}

		lastchecked = c;
		c = c->next;
	}
}

void evntrec_movetofront(evntrec **list, evntrec **last, evntrec *lastchecked, evntrec *x)
{
	if(lastchecked != NULL)
	{
		if(*last == x)
			*last = lastchecked;

		lastchecked->next = x->next;
		x->next = *list;
		*list = x;
	}
}

/****************************************************************************/
/* record evntrec low-level data-specific function declarations. */


void evntrec_init(evntrec *event)
{
	/* initialize */
	event->id = 0;
	event->name = NULL;
	event->place = NULL;
	event->when = NULL;
	event->about = NULL;
	event->admin = NULL;

	event->attendees = NULL;
	event->attendees_last = NULL;
	event->num_attendees = 0;

	event->time_created = 0L;

	event->flags = 0L;
	event->order = 0;
}

u_long evntrec_sizeof(evntrec *e)
{
	u_long	tot = 0L;
	attnrec	*a;

	tot += sizeof(evntrec);
	tot += e->name ? strlen(e->name)+1 : 0L;
	tot += e->place ? strlen(e->place)+1 : 0L;
	tot += e->when ? strlen(e->when)+1 : 0L;
	tot += e->about ? strlen(e->about)+1 : 0L;
	tot += e->admin ? strlen(e->admin)+1 : 0L;
	for(a=e->attendees; a!=NULL; a=a->next)
		tot += attnrec_sizeof(a);

	return tot;
}

void evntrec_free(evntrec *e)
{
	/* Free the elements */
	xfree(e->name);
	xfree(e->place);
	xfree(e->when);
	xfree(e->about);
	xfree(e->admin);

	Event_freeallattendees(e);

	/* Free this record */
	xfree(e);
}

void evntrec_setname(evntrec *event, char *name)
{
	mstrcpy(&event->name,name);
}

void evntrec_setplace(evntrec *event, char *place)
{
	mstrcpy(&event->place,place);
}

void evntrec_setwhen(evntrec *event, char *when)
{
	mstrcpy(&event->when,when);
}

void evntrec_setabout(evntrec *event, char *about)
{
	mstrcpy(&event->about,about);
}

void evntrec_setadmin(evntrec *event, char *admin)
{
	mstrcpy(&event->admin,admin);
}


evntrec *evntrec_event(evntrec **list, evntrec **last, int id_num)
{
	evntrec *e, *lastchecked = NULL;

	for(e=*list; e!=NULL; e=e->next)
	{
		if(e->id == id_num)
		{
			/* No movetofront's for this list */
			/* evntrec_movetofront(list,last,lastchecked,e); */

			return e;
		}
		lastchecked = e;
	}

	return NULL;
}



/***************************************************************************n
 *
 * attendee record definitions.  No changes should be made to new(),
 * freeall(), append(), delete(), or movetofront().
 *
 ****************************************************************************/

attnrec *attnrec_new()
{
	attnrec *x;

	/* allocate memory */
	x = (attnrec *)xmalloc(sizeof(attnrec));

	/* initialize */
	x->next = NULL;

	attnrec_init(x);

	return x;
}

void attnrec_freeall(attnrec **list, attnrec **last)
{
	attnrec *m = *list, *v;

	while(m != NULL)
	{
		v = m->next;
		attnrec_free(m);
		m = v;
	}

	*list = NULL;
	*last = NULL;
}

void attnrec_append(attnrec **list, attnrec **last, attnrec *entry)
{
	attnrec *lastentry = *last;

	if(*list == NULL)
		*list = entry;
	else
		lastentry->next = entry;

	*last = entry;
	entry->next = NULL;
}

void attnrec_delete(attnrec **list, attnrec **last, attnrec *x)
{
	attnrec *m, *lastchecked = NULL;

	m = *list;
	while(m != NULL)
	{
		if(m == x)
		{
			if(lastchecked == NULL)
			{
				*list = m->next;
				if(m == *last)
					*last = NULL;
			}
			else
			{
				lastchecked->next = m->next;
				if(m == *last)
					*last = lastchecked;
			}

			attnrec_free(m);
			return;
		}

		lastchecked = m;
		m = m->next;
	}
}

void attnrec_movetofront(attnrec **list, attnrec **last, attnrec *lastchecked, attnrec *x)
{
	if(lastchecked != NULL)
	{
		if(*last == x)
			*last = lastchecked;

		lastchecked->next = x->next;
		x->next = *list;
		*list = x;
	}
}

/****************************************************************************/

void attnrec_init(attnrec *a)
{
	/* initialize */
	a->nick = NULL;
	a->when = NULL;
	a->contact = NULL;
	a->order = 0;
}

u_long attnrec_sizeof(attnrec *a)
{
	u_long	tot = 0L;

	tot += sizeof(attnrec);
	tot += a->nick ? strlen(a->nick)+1 : 0L;
	tot += a->when ? strlen(a->when)+1 : 0L;
	tot += a->contact ? strlen(a->contact)+1 : 0L;

	return tot;
}

void attnrec_free(attnrec *a)
{
	/* Free the elements */
	xfree(a->nick);
	xfree(a->when);
	xfree(a->contact);

	/* Free this record */
	xfree(a);
}

void attnrec_setnick(attnrec *a, char *nick)
{
	mstrcpy(&a->nick,nick);
}

void attnrec_setwhen(attnrec *a, char *when)
{
	mstrcpy(&a->when,when);
}

void attnrec_setcontact(attnrec *a, char *contact)
{
	mstrcpy(&a->contact,contact);
}

attnrec *attnrec_attendee(attnrec **list, attnrec **last, char *nick)
{
	attnrec *a, *lastchecked = NULL;

	for(a=*list; a!=NULL; a=a->next)
	{
		if(isequal(a->nick,nick))
		{
			/* found it.  don't do a movetofront. */
			/* attnrec_movetofront(list,last,lastchecked,a); */

			return a;
		}
		lastchecked = a;
	}

	return NULL;
}
