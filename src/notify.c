/******************************************************************************
 * Steve Rider - stever@area23.org
 * Grufti - an interactive, service-oriented bot for IRC 
 *
 * notify.c - keep track of who's on IRC
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
#include "notify.h"
#include "dcc.h"
#include "user.h"
#include "server.h"
#include "log.h"
#include "misc.h"

/***************************************************************************n
 *
 * Notify object definitions.
 *
 ****************************************************************************/

void Notify_new()
{
	/* already in use? */
	if(Notify != NULL)
		Notify_freeall();

	/* allocate memory */
	Notify = (Notify_ob *)xmalloc(sizeof(Notify_ob));

	/* initialize */
	Notify->last_radiated = 0L;
	Notify->last_radiated_msg = 0L;
	Notify->flags = 0;
	Notify->num_queries = 0;
	Notify->uh_buf[0] = 0;

	Notify->radiate_queue = NULL;
	Notify->radiate_last = NULL;
	Notify->num_nicks = 0;
	Notify->genericlist = NULL;
	Notify->genericlast = NULL;
	Notify->num_generics = 0;
}

u_long Notify_sizeof()
{
	u_long	tot = 0L;
	radirec	*r;
	generec	*f;

	tot += sizeof(Notify_ob);
	for(r=Notify->radiate_queue; r!=NULL; r=r->next)
		tot += radirec_sizeof(r);
	for(f=Notify->genericlist; f!=NULL; f=f->next)
		tot += generec_sizeof(f);

	return tot;
}

void Notify_freeall()
{
	/* Free the object */
	xfree(Notify);
	Notify = NULL;
}

/****************************************************************************/
/* object Notify structure function definitions. */

void Notify_appendnick(radirec *r)
{
	radirec_append(&Notify->radiate_queue, &Notify->radiate_last, r);
	Notify->num_nicks++;
}

void Notify_deletenick(radirec *r)
{
	radirec_delete(&Notify->radiate_queue, &Notify->radiate_last, r);
	Notify->num_nicks--;
}

void Notify_freeallnicks()
{
	radirec_freeall(&Notify->radiate_queue, &Notify->radiate_last);
	Notify->num_nicks = 0;
}

void Notify_appendgeneric(generec *f)
{
	generec_append(&Notify->genericlist, &Notify->genericlast, f);
	Notify->num_generics++;
}

void Notify_deletegeneric(generec *f)
{
	generec_delete(&Notify->genericlist, &Notify->genericlast, f);
	Notify->num_generics--;
}

void Notify_freeallgenerics()
{
	generec_freeall(&Notify->genericlist, &Notify->genericlast);
	Notify->num_generics = 0;
}

/****************************************************************************/
/* object Notify high-level implementation-specific function definitions. */

/****************************************************************************/
/* object Notify high-level implementation-specific function definitions. */

void Notify_loadradiatequeue()
/*
 * This routine loads up the radiate queue with every registered nick in the
 * database.  Each line is sent to the server, and a 302 is generated.  Once
 * we get the 302, we send out another line until the queue is empty.
 *
 * On each 302, we update the nicks for that 302 line.
 */
{
	userrec	*u;
	nickrec	*n;

	Notify->uh_buf[0] = 0;
	Notify->num_queries = 0;
	for(u=User->userlist; u!=NULL; u=u->next)
	{
		if(u->registered)
		{
			for(n=u->nicks; n!=NULL; n=n->next)
			{
				if(n->flags & NICK_HAS_ACCT_ENTRY)
				{
					Notify_pushqueue(n->nick);
				}
			}
		}
	}
}

void Notify_pushqueue(char *nick)
{
	radirec	*x;

	/* make a new node and assign data */
	x = radirec_new();

	/* append node to radiate queue */
	Notify_appendnick(x);

	/* copy nick */
	radirec_setnick(x,nick);
}
	
void Notify_sendmsgfromqueue()
{
	int	num_entries = 0;
	radirec	*r;

	if(Notify->flags & NOTIFY_WAITING_FOR_302)
	{
		Log_write(LOG_DEBUG,"*","Attempting to send msg from radiate queue before we've received last 302!");
		return;
	}

	/* Build a message */
	Notify->uh_buf[0] = 0;
	for(r=Notify->radiate_queue; r!=NULL && num_entries < 5; r=r->next)
	{
		sprintf(Notify->uh_buf,"%s %s",Notify->uh_buf,r->nick);
		num_entries++;
	}

	if(num_entries == 0)
	{
		Log_write(LOG_DEBUG,"*","Tried to send something from queue and it was empty.");
		return;
	}

	Server_write(PRIORITY_NORM,"USERHOST %s",Notify->uh_buf+1);
	Notify->flags |= NOTIFY_WAITING_FOR_302;
	Notify->last_radiated_msg = time(NULL);

	/* Set last_radiated at the beginning of a radiate session */
	if(!(Notify->flags & NOTIFY_CURRENTLY_RADIATING))
	{
		Notify->last_radiated = time(NULL);
		Notify->flags |= NOTIFY_CURRENTLY_RADIATING;
	}

	/* Now we wait for 302.  We don't delete this entry until we get it. */
}
	
int Notify_waitingfor302()
{
	if(Notify->flags & NOTIFY_WAITING_FOR_302)
		return 1;
	else
		return 0;
}

int Notify_currentlyradiating()
{
	if(Notify->flags & NOTIFY_CURRENTLY_RADIATING)
		return 1;
	else
		return 0;
}

void Notify_dumpqueue()
{
	/* Free radiate queue */
	Notify_freeallnicks();

	/* Mark that we're no longer waiting. */
	Notify->flags &= ~NOTIFY_WAITING_FOR_302;
	Notify->flags &= ~NOTIFY_CURRENTLY_RADIATING;
	Notify->last_radiated = 0L;
	Notify->last_radiated_msg = 0L;
}
	
int Notify_orderbylogin()
{
	generec	*f, *f_save = NULL;
	int	counter = 0, done = 0;
	time_t	h, highest = 0L;

	/* We need to clear all ordering before we start */
	Notify_clearorder();

	/* Continue until all entries have an order number */
	while(!done)
	{
		done = 1;
		h = highest;
		for(f=Notify->genericlist; f!=NULL; f=f->next)
		{
			if(!f->order && f->login >= h)
			{
				done = 0;
				h = f->login;
				f_save = f;
			}
		}

		/* Not done means we got u_save and now we need to order it */
		if(!done)
		{
			counter++;
			f_save->order = counter;
		}
	}

	return counter;
}

void Notify_clearorder()
{
	generec	*f;

	for(f=Notify->genericlist; f!=NULL; f=f->next)
		f->order = 0;
}

generec *Notify_genericbyorder(int ordernum)
{
	generec	*f;

	for(f=Notify->genericlist; f!=NULL; f=f->next)
		if(f->order == ordernum)
			return f;

	return NULL;
}

void Notify_addtogenericlist(char *display, time_t login)
{
	generec	*f;

	/* Allocate memory for new node */
	f = generec_new();

	/* Append to list */
	Notify_appendgeneric(f);

	/* Copy the data */
	generec_setdisplay(f,display);
	f->login = login;
}

/* ASDF */
/***************************************************************************n
 *
 * radirec record definitions.  No changes should be made to new(), freeall(),
 * append(), delete(), or movetofront().
 *
 ****************************************************************************/

radirec *radirec_new()
{
	radirec *x;

	/* allocate memory */
	x = (radirec *)xmalloc(sizeof(radirec));

	/* initialize */
	x->next = NULL;

	radirec_init(x);

	return x;
}

void radirec_freeall(radirec **list, radirec **last)
{
	radirec *entry = *list, *v;

	while(entry != NULL)
	{
		v = entry->next;
		radirec_free(entry);
		entry = v;
	}

	*list = NULL;
	*last = NULL;
}

void radirec_append(radirec **list, radirec **last, radirec *x)
{
	radirec	*lastentry = *last;

	if(*list == NULL)
		*list = x;
	else
		lastentry->next = x;

	*last = x;
	x->next = NULL;
}

void radirec_delete(radirec **list, radirec **last, radirec *x)
{
	radirec *entry, *lastchecked = NULL;

	entry = *list;
	while(entry != NULL)
	{
		if(entry == x)
		{
			if(lastchecked == NULL)
			{
				*list = entry->next;
				if(entry == *last)
					*last = NULL;
			}
			else
			{
				lastchecked->next = entry->next;
				if(entry == *last)
					*last = lastchecked;
			}

			radirec_free(entry);
			return;
		}

		lastchecked = entry;
		entry = entry->next;
	}
}

/****************************************************************************/
/* record radirec low-level data-specific function definitions. */

void radirec_init(radirec *x)
{
	/* initialize */
	x->nick = NULL;
}

u_long radirec_sizeof(radirec *x)
{
	u_long	tot = 0L;

	tot += sizeof(radirec);
	tot += x->nick ? strlen(x->nick)+1 : 0L;

	return tot;
}
	
void radirec_free(radirec *x)
{
	/* Free the elements. */
	xfree(x->nick);

	/* Free this record. */
	xfree(x);
}

void radirec_setnick(radirec *x, char *nick)
{
	mstrcpy(&x->nick,nick);
}

/***************************************************************************n
 *
 * generec record definitions.  No changes should be made to new(), freeall(),
 * append(), delete(), or movetofront().
 *
 ****************************************************************************/

generec *generec_new()
{
	generec *x;

	/* allocate memory */
	x = (generec *)xmalloc(sizeof(generec));

	/* initialize */
	x->next = NULL;

	generec_init(x);

	return x;
}

void generec_freeall(generec **list, generec **last)
{
	generec *entry = *list, *v;

	while(entry != NULL)
	{
		v = entry->next;
		generec_free(entry);
		entry = v;
	}

	*list = NULL;
	*last = NULL;
}

void generec_append(generec **list, generec **last, generec *x)
{
	generec	*lastentry = *last;

	if(*list == NULL)
		*list = x;
	else
		lastentry->next = x;

	*last = x;
	x->next = NULL;
}

void generec_delete(generec **list, generec **last, generec *x)
{
	generec *entry, *lastchecked = NULL;

	entry = *list;
	while(entry != NULL)
	{
		if(entry == x)
		{
			if(lastchecked == NULL)
			{
				*list = entry->next;
				if(entry == *last)
					*last = NULL;
			}
			else
			{
				lastchecked->next = entry->next;
				if(entry == *last)
					*last = lastchecked;
			}

			generec_free(entry);
			return;
		}

		lastchecked = entry;
		entry = entry->next;
	}
}

/****************************************************************************/
/* record generec low-level data-specific function definitions. */

void generec_init(generec *x)
{
	/* initialize */
	x->display = NULL;
	x->login = 0L;
	x->order = 0;
}

u_long generec_sizeof(generec *f)
{
	u_long	tot = 0L;

	tot += sizeof(generec);
	tot += f->display ? strlen(f->display)+1 : 0L;

	return tot;
}

void generec_free(generec *x)
{
	/* Free the elements. */
	xfree(x->display);

	/* Free this record. */
	xfree(x);
}

void generec_setdisplay(generec *x, char *display)
{
	mstrcpy(&x->display,display);
}
