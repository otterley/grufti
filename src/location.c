/******************************************************************************
 * Steve Rider - stever@area23.org
 * Grufti - an interactive, service-oriented bot for IRC 
 *
 * location.c - location records
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
#include "location.h"
#include "log.h"
#include "user.h"
#include "misc.h"

/***************************************************************************n
 *
 * Location object definitions.
 *
 ****************************************************************************/

void Location_new()
{
	/* already in use? */
	if(Location != NULL)
		Location_freeall();

	/* allocate memory */
	Location = (Location_ob *)xmalloc(sizeof(Location_ob));

	/* initialize */
	Location->localist = NULL;
	Location->last = NULL;

	Location->num_locations = 0;
	Location->flags = 0L;
}

u_long Location_sizeof()
{
	u_long	tot = 0L;
	locarec	*l;

	tot += sizeof(Location_ob);
	for(l=Location->localist; l!=NULL; l=l->next)
		tot += locarec_sizeof(l);

	return tot;
}

void Location_freeall()
{
	/* Free all location records */
	Location_freealllocations();

	/* Free the object */
	xfree(Location);
	Location = NULL;
}

/****************************************************************************/
/* object Location data-specific function definitions. */

void Location_display()
{
	locarec *loca;

	say("%d total locations.",Location->num_locations);

	for(loca=Location->localist; loca!=NULL; loca=loca->next)
		locarec_display(loca);
}

/****************************************************************************/
/* low-level Location structure function definitions. */

void Location_appendlocation(locarec *loca)
{
	locarec_append(&Location->localist, &Location->last, loca);
	Location->num_locations++;
}

void Location_deletelocation(locarec *loca)
{
	locarec_delete(&Location->localist, &Location->last, loca);
	Location->num_locations--;
}

void Location_freealllocations()
{
	locarec_freeall(&Location->localist, &Location->last);
	Location->num_locations = 0;
}

/****************************************************************************/
/* object Location high-level implementation-specific function definitions. */

locarec *Location_location(int id_num)
{
	return locarec_location(&Location->localist, &Location->last, id_num);
}


void Location_loadlocations()
/*
 * Prefix codes are:
 *   .  ID #
 *   l  "city" "state" "country"
 *   w  nick time
 *   d  description
 */
{
	locarec	*l_tmp;
	char	s[BUFSIZ], ident[BUFSIZ];
	char	tok1[BUFSIZ], tok2[BUFSIZ], tok3[BUFSIZ], locafile[256];
	int	line = 0;
	FILE	*f;

	/* Open file for reading */
	sprintf(locafile,"%s/%s",Grufti->botdir,Grufti->locafile);
	f = fopen(locafile,"r");
	if(f == NULL)
	{
		Location->flags |= LOCATION_ACTIVE;
		return;
	}

	/* Allocate memory for new node */
	l_tmp = locarec_new();

	/* Read each line */
	while(readline(f,s))
	{
		line++;

		/* Extract identifier code */
		split(ident,s);

		/* Match identifier code */
		switch(ident[0])
		{
		    case '.':	/* ID NUMBER */
			if(l_tmp->id != 0)
			{
				Location_appendlocation(l_tmp);
				l_tmp = locarec_new();
			}
			l_tmp->id = atoi(s);
			break;

		    case 'l':	/* location info */
			quoted_str_element(tok1,s,1);
			quoted_str_element(tok2,s,2);
			quoted_str_element(tok3,s,3);
			if(tok1[0] && tok2[0] && tok3[0])
			{
				if(!rmquotes(tok1))
					Log_write(LOG_ERROR,"*","[%d] Location file is corrupt!",line);
				else
					locarec_setcity(l_tmp,tok1);
				if(!rmquotes(tok2))
					Log_write(LOG_ERROR,"*","[%d] Location file is corrupt!",line);
				else
					locarec_setstate(l_tmp,tok2);
				if(!rmquotes(tok3))
					Log_write(LOG_ERROR,"*","[%d] Location file is corrupt!",line);
				else
					locarec_setcountry(l_tmp,tok3);
			}
			else
				Log_write(LOG_ERROR,"*","[%d] Location file is corrupt!",line);
			break;

		    case 'w':	/* added by whom and when */
			str_element(tok1,s,1);
			str_element(tok2,s,2);
			if(tok1[0] && tok2[0])
			{
				locarec_setaddedby(l_tmp,tok1);
				l_tmp->time_added = (time_t)atol(tok2);
			}
			else
				Log_write(LOG_ERROR,"*","[%d] Location file is corrupt!",line);
			break;

		    case 'd':	/* Description */
			locarec_setdescription(l_tmp,s);
			break;
		}
	}
	fclose(f);

	/* read last entry */
	if(l_tmp->id != 0)
		Location_appendlocation(l_tmp);

	/* Sort the list */
	Location_sort();

	/* now the list is active, and synced */
	Location->flags |= LOCATION_ACTIVE;
	Location->flags &= ~LOCATION_NEEDS_WRITE;
}

void Location_writelocations()
/*
 * Prefix codes are:
 *   .  ID #
 *   l  "city" "state" "country"
 *   w  nick time
 *   d  description
 */
{
	FILE	*f;
	locarec	*l;
	char	tmpfile[256], locafile[256];
	int	x;

	/* bail if locationlist is inactive (pending read...) */
	if(!(Location->flags & LOCATION_ACTIVE))
	{
		Log_write(LOG_MAIN,"*","Locationlist is not active.  Ignoring write request.");
		return;
	}

	/* open tmpfile for writing */
	sprintf(locafile,"%s/%s",Grufti->botdir,Grufti->locafile);
	sprintf(tmpfile,"%s.tmp",locafile);
	f = fopen(tmpfile,"w");
	if(f == NULL)
	{
		Log_write(LOG_MAIN|LOG_ERROR,"*","Couldn't write locationfile.");
		return;
	}

	/* Write header */
	writeheader(f,"Location->localist - User location records.",locafile);

	/* Write each record */
	for(l=Location->localist; l!=NULL; l=l->next)
	{
		/* Write id number */
		if(fprintf(f,". %d\n",l->id) == EOF)
			return;

		/* Write location line ("city" "state" "country") */
		if(l->city && l->state && l->country)
			if(fprintf(f,"l \"%s\" \"%s\" \"%s\"\n",l->city,l->state,l->country) == EOF)
				return;

		/* Write whom and when info (nick uh time) */
		if(l->addedby && l->time_added != 0L)
			if(fprintf(f,"w %s %lu\n",l->addedby,l->time_added) == EOF)
				return;

		/* Write description, if any */
		if(l->description)
			if(fprintf(f,"d %s\n",l->description) == EOF)
				return;
	}
	fclose(f);

	/* Move tmpfile over to main location */
	if(Grufti->keep_tmpfiles)
		x = copyfile(tmpfile,locafile);
	else
		x = movefile(tmpfile,locafile);

	verify_write(x,tmpfile,locafile);
	if(x == 0)
	{
                Log_write(LOG_MAIN,"*","Wrote location file.");
		Location->flags &= ~LOCATION_NEEDS_WRITE;
	}
}
		
int Location_newidnumber()
{
	locarec	*l;
	int	id_max = 0;

	/* search through the list for the highest number, then add 1. */
	for(l=Location->localist; l!=NULL; l=l->next)
		if(l->id > id_max)
			id_max = l->id;

	return id_max + 1;
}

void Location_idtostr(int id_num, char *l_info)
{
	locarec *l;

	l = Location_location(id_num);
	if(l == NULL)
	{
		l_info[0] = 0;
		return;
	}

	Location_ltostr(l,l_info);
}

void Location_ltostr(locarec *l, char *l_info)
{
	if(l->state[0])
		sprintf(l_info,"%s, %s  %s",l->city,l->state,l->country);
	else
		sprintf(l_info,"%s, %s",l->city,l->country);
}
	

void Location_sort()
{
	/* sort by city */
	Location_sortby(1);

	/* sort by country */
	Location_sortby(3);
}

void Location_sortby(int which)
{
	int	sorted = 0;
	locarec	*l, *l_tmp, *lastchecked = NULL;
	char	*cmp_a = NULL, *cmp_b = NULL;

	if(Location->localist == NULL)
		return;

	while(!sorted)
	{
		sorted = 1;
		lastchecked = NULL;
		for(l=Location->localist; l->next!=NULL; l=l->next)
		{
			if(which == 1)
			{
				cmp_a = l->city;
				cmp_b = l->next->city;
			}
			else if(which == 2)
			{
				cmp_a = l->state;
				cmp_b = l->next->state;
			}
			else if(which == 3)
			{
				cmp_a = l->country;
				cmp_b = l->next->country;
			}
			else
			{
				Log_write(LOG_ERROR,"*","Invalid argument %d passed to Location_sortby().",which);
				return;
			}
			
			if(strcasecmp(cmp_a,cmp_b) > 0)
			{
				sorted = 0;

				if(lastchecked == NULL)
					Location->localist = l->next;
				else
					lastchecked->next = l->next;

				l_tmp = l->next->next;
				l->next->next = l;
				l->next = l_tmp;

				/* We want to check this record again. */
				if(lastchecked == NULL)
				{
					if(Location->localist->next->next != NULL)
						l = Location->localist->next;
					else
						l = Location->localist;
				}
				else
					l = lastchecked->next;
			}
			lastchecked = l;
		}
	}

	/* find last */
	for(l=Location->localist; l->next!=NULL; l=l->next)
		;

	Location->last = l;

	Location->flags |= LOCATION_NEEDS_WRITE;
}

locarec *Location_addnewlocation(char *city, char *state, char *country, userrec *u, char *nick)
{
	locarec	*l;

	/* Allocate memory for new node */
	l = locarec_new();

	/* Assign it some data */
	l->id = Location_newidnumber();
	locarec_setcity(l,city);
	locarec_setstate(l,state);
	locarec_setcountry(l,country);
	if(u->registered)
		locarec_setaddedby(l,u->acctname);
	else
		locarec_setaddedby(l,nick);
	l->time_added = time(NULL);

	/* append the record */
	Location_appendlocation(l);

	return l;
}

locarec *Location_locationbyarea(char *city, char *state, char *country)
{
	locarec	*l;

	for(l=Location->localist; l!=NULL; l=l->next)
		if(isequal(l->city,city) && isequal(l->state,state) && isequal(l->country,country))
			return l;

	return NULL;
}

void Location_locationidsforarea(char *area, char *str)
{
	locarec	*l;

	strcpy(str," ");
	for(l=Location->localist; l!=NULL; l=l->next)
	{
		if(isequal(l->city,area) || isequal(l->state,area) || isequal(l->country,area))
		{
			sprintf(str,"%s%d ",str,l->id);
		}
	}
}

locarec	*Location_locationbytok(char *tok)
{
	locarec	*l;

	for(l=Location->localist; l!=NULL; l=l->next)
	{
		if(isequal(l->city,tok) || isequal(l->state,tok) || isequal(l->country,tok))
			return l;
	}

	return NULL;
}

locarec *Location_locationbyindex(int item)
{
	locarec	*l;
	int	i = 0;

	if(item > Location->num_locations)
		return NULL;

	for(l=Location->localist; l!=NULL; l=l->next)
	{
		i++;
		if(i == item)
			return l;
	}

	return NULL;
}

void Location_increment(userrec *u)
{
	locarec	*l;

	if(!u->location)
		return;

	l = Location_location(u->location);
	if(l != NULL)
		l->numusers++;
}

void Location_decrement(userrec *u)
{
	locarec	*l;

	if(!u->location)
		return;

	l = Location_location(u->location);
	if(l != NULL)
		l->numusers--;
}


/* ASDF */
/***************************************************************************n
 *
 * locarec record definitions.  No changes should be made to new(), freeall(),
 * append(), delete(), or movetofront().
 *
 ****************************************************************************/

locarec *locarec_new()
{
	locarec *x;

	/* allocate memory */
	x = (locarec *)xmalloc(sizeof(locarec));

	/* initialize */
	x->next = NULL;

	locarec_init(x);

	return x;
}

void locarec_freeall(locarec **list, locarec **last)
{
	locarec *entry = *list, *v;

	while(entry != NULL)
	{
		v = entry->next;
		locarec_free(entry);
		entry = v;
	}

	*list = NULL;
	*last = NULL;
}

void locarec_append(locarec **list, locarec **last, locarec *x)
{
	locarec *lastentry = *last;

	if(*list == NULL)
		*list = x;
	else
		lastentry->next = x;

	*last = x;
	x->next = NULL;
}

void locarec_delete(locarec **list, locarec **last, locarec *x)
{
	locarec *entry, *lastchecked = NULL;

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

			locarec_free(entry);
			return;
		}

		lastchecked = entry;
		entry = entry->next;
	}
}

void locarec_movetofront(locarec **list, locarec **last, locarec *lastchecked, locarec *x)
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
/* record locarec low-level data-specific function definitions. */

void locarec_init(locarec *loca)
{
	/* initialize */
	loca->city = NULL;
	loca->state = NULL;
	loca->country = NULL;
	loca->addedby = NULL;
	loca->time_added = 0L;
	loca->description = NULL;
	loca->id = 0;
	loca->flags = 0L;
	loca->numusers = 0;
}

u_long locarec_sizeof(locarec *l)
{
	u_long	tot = 0L;

	tot += sizeof(locarec);
	tot += l->city ? strlen(l->city)+1 : 0L;
	tot += l->state ? strlen(l->state)+1 : 0L;
	tot += l->country ? strlen(l->country)+1 : 0L;
	tot += l->addedby ? strlen(l->addedby)+1 : 0L;
	tot += l->description ? strlen(l->description)+1 : 0L;

	return tot;
}

void locarec_free(locarec *loca)
{
	/* Free the elements. */
	xfree(loca->city);
	xfree(loca->state);
	xfree(loca->country);
	xfree(loca->addedby);
	xfree(loca->description);

	/* Free this record. */
	xfree(loca);
}

void locarec_setcity(locarec *loca, char *city)
{
	mstrcpy(&loca->city,city);
}

void locarec_setstate(locarec *loca, char *state)
{
	mstrcpy(&loca->state,state);
}

void locarec_setcountry(locarec *loca, char *country)
{
	mstrcpy(&loca->country,country);
}

void locarec_setaddedby(locarec *loca, char *n)
{
	mstrcpy(&loca->addedby,n);
}

void locarec_setdescription(locarec *loca, char *description)
{
	mstrcpy(&loca->description,description);
}

void locarec_display(locarec *loca)
{
	char when[TIMELONGLEN];

	timet_to_date_short(loca->time_added,when);

	say("%-20s %-10s %-10s",loca->city,loca->state,loca->country);
	say("Added by: %s (%s)",loca->addedby,when);
	if(loca->description)
		say("Description: %s",loca->description);
	else
		say("Description: <none>");
}

locarec *locarec_location(locarec **list, locarec **last, int id_num)
{
	locarec *loca;

	for(loca=*list; loca!=NULL; loca=loca->next)
		if(loca->id == id_num)
			return loca;

	return NULL;
}
