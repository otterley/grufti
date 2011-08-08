/******************************************************************************
 * Steve Rider - stever@area23.org
 * Grufti - an interactive, service-oriented bot for IRC 
 *
 * banlist.c - ban records
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
#include "banlist.h"
#include "log.h"
#include "match.h"
#include "user.h"
#include "channel.h"
#include "misc.h"

/***************************************************************************n
 *
 * Banlist object definitions.
 *
 ****************************************************************************/

void Banlist_new()
{
	/* already in use? */
	if(Banlist != NULL)
		Banlist_freeall();

	/* allocate memory */
	Banlist = (Banlist_ob *)xmalloc(sizeof(Banlist_ob));

	/* initialize */
	Banlist->banlist = NULL;
	Banlist->last = NULL;

	Banlist->num_bans = 0;
	Banlist->flags = 0L;
}

u_long Banlist_sizeof()
{
	u_long	tot = 0L;
	banlrec	*b;

	tot += sizeof(Banlist_ob);
	for(b=Banlist->banlist; b!=NULL; b=b->next)
		tot += banlrec_sizeof(b);

	return tot;
}

void Banlist_freeall()
{
	/* Free all ban records */
	Banlist_freeallbans();

	/* Free the object */
	xfree(Banlist);
	Banlist = NULL;
}

/****************************************************************************/
/* low-level Banlist structure function definitions. */

void Banlist_appendban(banlrec *b)
{
	banlrec_append(&Banlist->banlist, &Banlist->last, b);
	Banlist->num_bans++;
}

void Banlist_deleteban(banlrec *b)
{
	banlrec_delete(&Banlist->banlist, &Banlist->last, b);
	Banlist->num_bans--;
}

void Banlist_freeallbans()
{
	banlrec_freeall(&Banlist->banlist, &Banlist->last);
	Banlist->num_bans = 0;
}

/****************************************************************************/
/* object Banlist high-level implementation-specific function definitions. */

void Banlist_clearorder()
{
	banlrec	*b;

	for(b=Banlist->banlist; b!=NULL; b=b->next)
		b->order = 0;
}

int Banlist_orderbysettime()
{
	banlrec	*b, *b_save = NULL;
	int	counter = 0, done = 0;
	time_t	h, highest = time(NULL);

	/* We need to clear all ordering before we start */
	Banlist_clearorder();

	/* Continue until all entries have an order number */
	while(!done)
	{
		done = 1;
		h = highest;
		for(b=Banlist->banlist; b!=NULL; b=b->next)
		{
			if(!b->order && b->time_set <= h)
			{
				done = 0;
				h = b->time_set;
				b_save = b;
			}
		}

		/* Not done means we got u_save and now we need to order it */
		if(!done)
		{
			counter++;
			b_save->order = counter;
		}
	}

	return counter;
}

banlrec *Banlist_banbyordernum(int ordernum)
{
	banlrec	*b;

	for(b=Banlist->banlist; b!=NULL; b=b->next)
		if(b->order == ordernum)
			return b;

	return NULL;
}

banlrec *Banlist_addban(char *nick, char *ban, chanrec *chan, time_t expires, time_t t)
{
	banlrec	*x;

	/* Allocate new node */
	x = banlrec_new();

	/* Assign it some data */
	banlrec_setnick(x,nick);
	banlrec_setban(x,ban);
	x->time_set = t;
	x->expires = expires;
	x->chan = chan;

	/* Append to list */
	Banlist_appendban(x);

	return x;
}

banlrec *Banlist_ban(char *ban)
{
	banlrec	*b;

	for(b=Banlist->banlist; b!=NULL; b=b->next)
		if(isequal(b->ban,ban))
			return b;

	return NULL;
}
	
void Banlist_writebans()
/*
 * Prefix codes are:
 *    'n' - <nick>
 *    'b' - <ban>
 *    't' - <time_set>
 *    'e' - <expires>
 *    'u' - <used>
 */
{
	FILE	*f;
	banlrec	*b;
	int	x;
	char	banlist[256], tmpfile[256];

	/* bail if banlist is inactive (pending read) */
	if(!(Banlist->flags & BANLIST_ACTIVE))
	{
		Log_write(LOG_MAIN,"*","Banlist list is not active.  Ignoring write request.");
		return;
	}

	/* Open for writing */
	sprintf(banlist,"%s/%s",Grufti->botdir,Grufti->banlist);
	sprintf(tmpfile,"%s.tmp",banlist);
	f = fopen(tmpfile,"w");
	if(f == NULL)
	{
		Log_write(LOG_MAIN|LOG_ERROR,"*","Couldn't write banlist.");
		return;
	}

	/* write header */
	writeheader(f,"Banlist->banlist - The shitlist",banlist);

	/* write each record */
	for(b=Banlist->banlist; b!=NULL; b=b->next)
	{
		/* Write nick */
		if(fprintf(f,"n %s\n",b->nick) == EOF)
			return;

		/* Write ban */
		if(fprintf(f,"b %s\n",b->ban) == EOF)
			return;

		/* Write time */
		if(fprintf(f,"t %lu\n",(u_long)b->time_set) == EOF)
			return;

		/* Write expires */
		if(fprintf(f,"e %lu\n",(u_long)b->expires) == EOF)
			return;

		/* Write used */
		if(fprintf(f,"u %d\n",b->used) == EOF)
			return;
	}

	fclose(f);

	/* Move tmpfile over to main response */
	if(Grufti->keep_tmpfiles)
		x = copyfile(tmpfile,banlist);
	else
		x = movefile(tmpfile,banlist);

	verify_write(x,tmpfile,banlist);
	if(x == 0)
	{
		Log_write(LOG_MAIN,"*","Wrote banlist.");
		Banlist->flags &= ~BANLIST_NEEDS_WRITE;
	}
}

void Banlist_loadbans()
{
	banlrec	*b, *b_next, *b_tmp;
	char	s[BUFSIZ], ident[BUFSIZ], banlist[256];
	int	line = 0;
	time_t	now = time(NULL);
	FILE	*f;

	/* open file for reading */
	sprintf(banlist,"%s/%s",Grufti->botdir,Grufti->banlist);
	f = fopen(banlist,"r");
	if(f == NULL)
	{
		Banlist->flags |= BANLIST_ACTIVE;
		return;
	}

	/* allocate memory for new node */
	b_tmp = banlrec_new();

	/* read each line */
	while(readline(f,s))
	{
		line++;

		/* extract identifier code */
		split(ident,s);

		/* match identifier code */
		switch(ident[0])
		{
		    case 'n':	/* nick */
			if(b_tmp->nick != NULL)
			{
				Banlist_appendban(b_tmp);
				b_tmp = banlrec_new();
			}
			banlrec_setnick(b_tmp,s);
			break;

		    case 'b':	/* ban */
			banlrec_setban(b_tmp,s);
			break;

		    case 't':	/* time set */
			b_tmp->time_set = (time_t)atol(s);
			break;

		    case 'e':	/* expires */
			b_tmp->expires = (time_t)atol(s);
			break;

		    case 'u':	/* used */
			b_tmp->used = atoi(s);
			break;
		}
	}

	fclose(f);

	/* read last entry */
	if(b_tmp->nick != NULL)
		Banlist_appendban(b_tmp);

	/* now the list is active and syned */
	Banlist->flags |= BANLIST_ACTIVE;
	Banlist->flags &= ~BANLIST_NEEDS_WRITE;

	/* Now run through the list real quick and dump old bans */
	b = Banlist->banlist;
	while(b != NULL)
	{
		b_next = b->next;
		if(b->expires != 0)
		{
			if((b->time_set + b->expires) < now)
			{
				Banlist_deleteban(b);
				Banlist->flags |= BANLIST_NEEDS_WRITE;
			}
		}

		b = b_next;
	}
}

void Banlist_checkfortimeouts()
{
	banlrec	*b, *b_next;
	time_t	now = time(NULL);

	b = Banlist->banlist;
	while(b != NULL)
	{
		b_next = b->next;
		if(b->expires != 0)
		{
			if((b->time_set + b->expires < now))
			{
				Banlist_deleteban(b);
				Banlist->flags |= BANLIST_NEEDS_WRITE;
			}
		}

		b = b_next;
	}
}

banlrec *Banlist_match(char *ban, chanrec *chan)
{
	banlrec	*bl;

	for(bl=Banlist->banlist; bl!=NULL; bl=bl->next)
	{
		if(bl->chan == NULL)
		{
			if(wild_match(bl->ban,ban) || wild_match(ban,bl->ban))
				return bl;
		}
		else if(isequal(bl->chan->name,chan->name))
		{
			if(wild_match(bl->ban,ban) || wild_match(ban,bl->ban))
				return bl;
		}
	}
		

	return NULL;
}

int Banlist_verifyformat(char *ban)
{
	char	*p;
	int	gotbang = 0, gotat = 0, first = 0;

	for(p=ban; *p; p++)
	{
		if(*p == '!')
		{
			if(!first)
				first = 1;
			gotbang++;
		}
		else if(*p == '@')
		{
			if(!first)
				first = 2;
			gotat++;
		}
	}

	if(!gotat || gotat > 1 || !gotbang || gotbang > 1)
		return 0;

	if(first == 2)
		return 0;

	return 1;
}


/* ASDF */
/***************************************************************************n
 *
 * banlrec record definitions.  No changes should be made to new(), freeall(),
 * append(), delete(), or movetofront().
 *
 ****************************************************************************/

banlrec *banlrec_new()
{
	banlrec *x;

	/* allocate memory */
	x = (banlrec *)xmalloc(sizeof(banlrec));

	/* initialize */
	x->next = NULL;

	banlrec_init(x);

	return x;
}

void banlrec_freeall(banlrec **list, banlrec **last)
{
	banlrec *entry = *list, *v;

	while(entry != NULL)
	{
		v = entry->next;
		banlrec_free(entry);
		entry = v;
	}

	*list = NULL;
	*last = NULL;
}

void banlrec_append(banlrec **list, banlrec **last, banlrec *x)
{
	banlrec *lastentry = *last;

	if(*list == NULL)
		*list = x;
	else
		lastentry->next = x;

	*last = x;
	x->next = NULL;
}

void banlrec_delete(banlrec **list, banlrec **last, banlrec *x)
{
	banlrec *entry, *lastchecked = NULL;

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

			banlrec_free(entry);
			return;
		}

		lastchecked = entry;
		entry = entry->next;
	}
}

void banlrec_movetofront(banlrec **list, banlrec **last, banlrec *lastchecked, banlrec *x)
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
/* record banlrec low-level data-specific function definitions. */

void banlrec_init(banlrec *b)
{
	/* initialize */
	b->nick = NULL;
	b->ban = NULL;
	b->time_set = 0L;
	b->expires = 0L;
	b->used = 0;
	b->flags = 0L;
	b->chan = NULL;
	b->order = 0;
}

u_long banlrec_sizeof(banlrec *b)
{
	u_long	tot = 0L;

	tot += sizeof(banlrec);
	tot += b->ban ? strlen(b->ban)+1 : 0L;

	return tot;
}

void banlrec_free(banlrec *b)
{
	/* Free the elements. */
	xfree(b->ban);
	xfree(b->nick);

	/* Free this record. */
	xfree(b);
}

void banlrec_setnick(banlrec *b, char *nick)
{
	mstrcpy(&b->nick,nick);
}

void banlrec_setban(banlrec *b, char *ban)
{
	mstrcpy(&b->ban,ban);
}
