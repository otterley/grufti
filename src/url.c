/******************************************************************************
 * Steve Rider - stever@area23.org
 * Grufti - an interactive, service-oriented bot for IRC 
 *
 * url.c - URL catcher
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
#include "url.h"
#include "log.h"
#include "misc.h"

/***************************************************************************n
 *
 * URL object definitions.
 *
 ****************************************************************************/

void URL_new()
{
	/* already in use? */
	if(URL != NULL)
		URL_freeall();

	/* allocate memory */
	URL = (URL_ob *)xmalloc(sizeof(URL_ob));

	/* initialize */
	URL->urllist = NULL;
	URL->last = NULL;

	URL->num_urls = 0;
	URL->flags = 0L;

	URL->sbuf[0] = 0;
}

u_long URL_sizeof()
{
	u_long	tot = 0L;
	urlrec	*u;

	tot += sizeof(URL_ob);
	for(u=URL->urllist; u!=NULL; u=u->next)
		tot += urlrec_sizeof(u);

	return tot;
}

void URL_freeall()
{
	/* Free all url records */
	URL_freeallurls();

	/* Free the object */
	xfree(URL);
	URL = NULL;
}

/****************************************************************************/
/* object URL data-specific function definitions. */


/****************************************************************************/
/* low-level URL structure function definitions. */

void URL_inserturlatfront(urlrec *u)
{
	urlrec_insertatfront(&URL->urllist, &URL->last, u);
	URL->num_urls++;
}

void URL_appendurl(urlrec *u)
{
	urlrec_append(&URL->urllist, &URL->last, u);
	URL->num_urls++;
}

void URL_deleteurl(urlrec *u)
{
	urlrec_delete(&URL->urllist, &URL->last, u);
	URL->num_urls--;
}

void URL_freeallurls()
{
	urlrec_freeall(&URL->urllist, &URL->last);
	URL->num_urls = 0;
}

/****************************************************************************/
/* object URL high-level implementation-specific function definitions. */

void URL_checkforurl(char *nick, char *uh, char *text)
{
	char	*p, *q;

	/* return if max_urls is off */
	if(Grufti->max_urls == 0)
		return;

	p = strstr(text,"http://");
	if(p != NULL)
	{
		/* got http:// */
		q = strchr(p,' ');
		if(q != NULL)
			*q = 0;
	context;

		/* check for validity (needs something like http://xx.blea */
		q = strchr(p+8,'.');
		if(q != NULL)
		{
			if(*(q+1) != 0)
			{
				stripfromend(p);
				if(URL_url(p) == NULL)
				{
					if(URL->num_urls >= Grufti->max_urls)
						URL_deleteurl(URL->last);
					URL_addnewurl(nick,uh,p);
				}
			}
		}
	context;
		return;
	}

	/* Check for www. */
	p = strstr(text,"www.");
	if(p != NULL)
	{
		/* make sure it's somewhat valid */
		q = strchr(p,' ');
		if(q != NULL)
			*q = 0;
		q = strchr(p+4,'.');
		if(q != NULL)
		{
			if(*(q+1) != 0)
			{
				sprintf(URL->sbuf,"http://%s",p);
				stripfromend(URL->sbuf);
				if(URL_url(URL->sbuf) == NULL)
				{
					if(URL->num_urls >= Grufti->max_urls)
						URL_deleteurl(URL->last);
					URL_addnewurl(nick,uh,URL->sbuf);
				}
			}
		}
	}
}

void URL_addnewurl(char *nick, char *uh, char *url)
{
	urlrec	*x;

	/* Allocate new node */
	x = urlrec_new();

	/* assign it some data */
	urlrec_seturl(x,url);
	urlrec_setnick(x,nick);
	urlrec_setuh(x,uh);
	x->time_caught = time(NULL);
	x->flags |= URL_NOTCHECKED;

	/* insert at front of list */
	URL_inserturlatfront(x);

	URL->flags |= URL_NEEDS_WRITE;
}

urlrec *URL_url(char *url)
{
	urlrec	*u;

	context;
	for(u=URL->urllist; u!=NULL; u=u->next)
		if(isequal(url,u->url))
			return u;

	context;
	return NULL;
}

void URL_writeurls()
/*
 * Prefix codes are:
 *    'u' - url
 *    '@' - who said it: <nick> <uh> <time_caught>
 */
{
	FILE	*f;
	urlrec	*u;
	char	webfile[256], tmpfile[256];
	int	x;

	/* bail if urllist is inactive (pending read) */
	if(!(URL->flags & URL_ACTIVE))
	{
		Log_write(LOG_MAIN,"*","URL list is not active.  Ignoring write request.");
		return;
	}

	/* open for writing */
	sprintf(webfile,"%s/%s",Grufti->botdir,Grufti->webfile);
	sprintf(tmpfile,"%s.tmp",webfile);
	f = fopen(tmpfile,"w");
	if(f == NULL)
	{
		Log_write(LOG_MAIN|LOG_ERROR,"*","Couldn't write webcatchfile.");
		return;
	}

	/* write header */
	writeheader(f,"URL->urllist - Webcatch URL entries.",webfile);

	/* write each record */
	for(u=URL->urllist; u!=NULL; u=u->next)
	{
		/* Write url */
		if(fprintf(f,"u %s\n",u->url) == EOF)
			return;

		/* write nick, uh, and time caught */
		if(fprintf(f,"@ %s %s %lu\n",u->nick,u->uh,u->time_caught) == EOF)
			return;

	}

	fclose(f);

	/* move tmpfile over to main url */
	if(Grufti->keep_tmpfiles)
		x = copyfile(tmpfile,webfile);
	else
		x = movefile(tmpfile,webfile);

	verify_write(x,tmpfile,webfile);
	if(x == 0)
	{
		Log_write(LOG_MAIN,"*","Wrote webcatches.");
		URL->flags &= ~URL_NEEDS_WRITE;
	}
}

void URL_loadurls()
{
	urlrec	*u_tmp;
	char	ident[BUFSIZ], tok1[BUFSIZ], tok2[BUFSIZ], tok3[BUFSIZ];
	char	s[BUFSIZ], webfile[256];
	int	line = 0;
	FILE	*f;

	/* open file for reading */
	sprintf(webfile,"%s/%s",Grufti->botdir,Grufti->webfile);
	f = fopen(webfile,"r");
	if(f == NULL)
	{
		URL->flags |= URL_ACTIVE;
		return;
	}

	/* allocate memory for new node */
	u_tmp = urlrec_new();

	/* read each line */
	while(readline(f,s))
	{
		line++;

		/* extract identifier code */
		split(ident,s);

		/* match identifier code */
		switch(ident[0])
		{
		    case 'u':	/* URL */
			if(u_tmp->url != NULL)
			{
				URL_appendurl(u_tmp);
				u_tmp = urlrec_new();
			}
			urlrec_seturl(u_tmp,s);
			break;

		    case '@':	/* account */
			str_element(tok1,s,1);
			str_element(tok2,s,2);
			str_element(tok3,s,3);
			if(tok1[0] && tok2[0] && tok3[0])
			{
				urlrec_setnick(u_tmp,tok1);
				urlrec_setuh(u_tmp,tok2);
				u_tmp->time_caught = (time_t)atol(tok3);
			}
			else
				Log_write(LOG_ERROR,"*","[%d] Webcatches file is corrupt!",line);
			break;
		}
	}

	fclose(f);

	/* read last entry */
	if(u_tmp->url != NULL)
		URL_appendurl(u_tmp);

	/* now the list is active and syned */
	URL->flags |= URL_ACTIVE;
	URL->flags &= ~URL_NEEDS_WRITE;
}


/* ASDF */
/***************************************************************************n
 *
 * urlrec record definitions.  No changes should be made to new(), freeall(),
 * append(), delete(), or movetofront().
 *
 ****************************************************************************/

urlrec *urlrec_new()
{
	urlrec *x;

	/* allocate memory */
	x = (urlrec *)xmalloc(sizeof(urlrec));

	/* initialize */
	x->next = NULL;

	urlrec_init(x);

	return x;
}

void urlrec_freeall(urlrec **list, urlrec **last)
{
	urlrec *entry = *list, *v;

	while(entry != NULL)
	{
		v = entry->next;
		urlrec_free(entry);
		entry = v;
	}

	*list = NULL;
	*last = NULL;
}

void urlrec_insertatfront(urlrec **list, urlrec **last, urlrec *x)
/*
 * We want to insert rather than append at the beginning.  This list is
 * a queue.  We want the earliest events to be discarded and the most
 * recent events to be displayed first.  Keep the *last pointer however,
 * it speeds up finding the last element for when we want to drop it
 * from the list.
 */
{
	x->next = *list;
	*list = x;

	if(x->next == NULL)
		*last = x;
}

void urlrec_append(urlrec **list, urlrec **last, urlrec *x)
{
	urlrec	*lastentry = *last;

	if(*list == NULL)
		*list = x;
	else
		lastentry->next = x;

	*last = x;
	x->next = NULL;
}

void urlrec_delete(urlrec **list, urlrec **last, urlrec *x)
{
	urlrec *entry, *lastchecked = NULL;

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

			urlrec_free(entry);
			return;
		}

		lastchecked = entry;
		entry = entry->next;
	}
}

/****************************************************************************/
/* record urlrec low-level data-specific function definitions. */

void urlrec_init(urlrec *x)
{
	/* initialize */
	x->url = NULL;
	x->nick = NULL;
	x->uh = NULL;
	x->time_caught = 0L;
	x->title = NULL;
	x->flags = 0L;
}

u_long urlrec_sizeof(urlrec *x)
{
	u_long	tot = 0L;

	tot += sizeof(urlrec);
	tot += x->url ? strlen(x->url)+1 : 0L;
	tot += x->nick ? strlen(x->nick)+1 : 0L;
	tot += x->uh ? strlen(x->uh)+1 : 0L;
	tot += x->title ? strlen(x->title)+1 : 0L;

	return tot;
}

void urlrec_free(urlrec *x)
{
	/* Free the elements. */
	xfree(x->url);
	xfree(x->nick);
	xfree(x->uh);
	xfree(x->title);

	/* Free this record. */
	xfree(x);
}

void urlrec_seturl(urlrec *u, char *url)
{
	mstrcpy(&u->url,url);
}

void urlrec_setnick(urlrec *u, char *nick)
{
	mstrcpy(&u->nick,nick);
}

void urlrec_setuh(urlrec *u, char *uh)
{
	mstrcpy(&u->uh,uh);
}

void urlrec_settitle(urlrec *u, char *title)
{
	mstrcpy(&u->title,title);
}
