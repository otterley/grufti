/******************************************************************************
 * Steve Rider - stever@area23.org
 * Grufti - an interactive, service-oriented bot for IRC 
 *
 * note.c - Grufti Notes
 *
 *****************************************************************************/
/* 28 April 97 */

#include "config.h"
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include "grufti.h"
#include "gruftiaux.h"
#include "note.h"
#include "user.h"
#include "dcc.h"
#include "log.h"
#include "misc.h"

/***************************************************************************n
 *
 * Note object definitions.
 *
 ****************************************************************************/

void Note_new()
{
	/* already in use? */
	if(Note != NULL)
		Note_freeall();

	/* allocate memory */
	Note = (Note_ob *)xmalloc(sizeof(Note_ob));

	/* initialize */
	Note->flags = 0L;
	Note->send_buf[0] = 0;
	Note->send_buf1[0] = 0;
	Note->numNotesQueued = 0;
	Note->time_dequeued = 0L;
}

void Note_freeall()
{
	/* Free the object */
	xfree(Note);
	Note = NULL;
}

/****************************************************************************/
/* low-level Note structure function declarations. */

void Note_appendbody(noterec *n, nbody *body)
{
	nbody_append(&n->body, &n->body_last, body);
	n->num_lines++;
}

void Note_deletebody(noterec *n, nbody *body)
{
	nbody_delete(&n->body, &n->body_last, body);
	n->num_lines--;
}

void Note_freeallbodys(noterec *n)
{
	nbody_freeall(&n->body, &n->body_last);
	n->num_lines = 0;
}

/****************************************************************************/
/* object Note high-level implementation-specific function definitions. */


void Note_addbody(noterec *n, char *text)
{
	char	*p, *q, *r, c;

	p = text;

	while(strlen(p) > LINELENGTH)
	{
		q = p + LINELENGTH;

		/* Search for embedded linefeed */
		r = strchr(p,'\n');
		if((r != NULL) && (r < q))
		{
			/* Great!  dump that line and start over. */
			*r = 0;
			Note_addbodyline(n,p);
			*r = '\n';
			p = r + 1;
		}
		else
		{
			/* Search backwards for last space */
			while((*q != ' ') && (q != p))
				q --;
			if(q == p)
				q = p + LINELENGTH;
			c = *q;
			*q = 0;
			Note_addbodyline(n,p);
			*q = c;
			p = q + 1;
		}
	}

	/* Left with string < LINELENGTH */
	r = strchr(p,'\n');
	while(r != NULL)
	{
		*r = 0;
		Note_addbodyline(n,p);
		*r = '\n';
		p = r + 1;
		r = strchr(p,'\n');
	}
	if(*p)
		Note_addbodyline(n,p);
}

void Note_addbodyline(noterec *n, char *line)
{
	nbody	*body;

	/* Allocate new node */
	body = nbody_new();
	nbody_settext(body,line);

	Note_appendbody(n,body);
}

void Note_str2flags(char *s, char *f)
{
	char *p;

	*f = 0;
	for(p=s; *p; p++)
	{
		if(*p == 'n')		*f |= NOTE_NEW;
		else if(*p == 'r')	*f |= NOTE_READ;
		else if(*p == 'm')	*f |= NOTE_MEMO;
		else if(*p == 'w')	*f |= NOTE_WALL;
		else if(*p == 's')	*f |= NOTE_SAVE;
		else if(*p == '!')	*f |= NOTE_NOTICE;
	}
}

void Note_flags2str(char f, char *s)
{
	s[0] = 0;
	if(f & NOTE_NEW)	strcat(s,"n");
	if(f & NOTE_READ)	strcat(s,"r");
	if(f & NOTE_MEMO)	strcat(s,"m");
	if(f & NOTE_WALL)	strcat(s,"w");
	if(f & NOTE_SAVE)	strcat(s,"s");
	if(f & NOTE_NOTICE)	strcat(s,"!");
	if(s[0] == 0)		strcat(s,"-");
}

noterec	*Note_notebynumber(userrec *u, int num)
{
	int	i = 0;
	noterec	*n;

	for(n=u->notes; n!=NULL; n=n->next)
	{
		i++;
		if(i == num)
			return n;
	}

	return NULL;
}

int Note_numforwarded(userrec *u)
{
	int	tot = 0;
	noterec	*n;

	for(n=u->notes; n!=NULL; n=n->next)
		if(n->flags & NOTE_FORWARD)
			tot++;

	return tot;
}

int Note_numsaved(userrec *u)
{
	int	tot = 0;
	noterec	*n;

	for(n=u->notes; n!=NULL; n=n->next)
		if(n->flags & NOTE_SAVE)
			tot++;

	return tot;
}

int Note_numnew(userrec *u)
{
	int	tot = 0;
	noterec	*n;

	for(n=u->notes; n!=NULL; n=n->next)
		if(n->flags & NOTE_NEW)
			tot++;

	return tot;
}

int Note_numread(userrec *u)
{
	int	tot = 0;
	noterec	*n;

	for(n=u->notes; n!=NULL; n=n->next)
		if(n->flags & NOTE_READ)
			tot++;

	return tot;
}

int Note_numnotice(userrec *u)
{
	int	tot = 0;
	noterec	*n;

	for(n=u->notes; n!=NULL; n=n->next)
		if(n->flags & NOTE_NOTICE)
			tot++;

	return tot;
}

int Note_nummemo(userrec *u)
{
	int	tot = 0;
	noterec	*n;

	for(n=u->notes; n!=NULL; n=n->next)
		if(n->flags & NOTE_MEMO)
			tot++;

	return tot;
}

int Note_deletemarkednotes(userrec *u)
{
	int	tot = 0;
	noterec	*n, *n_next;

	context;
	n = u->notes;
	context;
	while(n != NULL)
	{
		n_next = n->next;
		if(n->flags & NOTE_DELETEME)
		{
			User_deletenote(u,n);
			tot++;
		}
		n = n_next;
	}

	context;
	return tot;
}

void Note_sendnotice(userrec *u, char *format, ...)
{
	va_list	va;
	noterec	*n;
	char	tmp[25];

	Note->send_buf[0] = 0;
	va_start(va,format);
	vsprintf(Note->send_buf,format,va);
	va_end(va);

	n = noterec_new();

	noterec_setnick(n,Grufti->botnick);
	noterec_setuh(n,Grufti->botuh);
	sprintf(tmp,"%s Notice",Grufti->botnick);
	noterec_setfinger(n,tmp);
	n->sent = time(NULL);
	n->flags |= NOTE_NOTICE;
	n->flags |= NOTE_NEW;

	/* Append notice info */
	sprintf(Note->send_buf,"%s\n\nThis Note is a %s Notice, sent to inform you of %s events that happen while you are away.",Note->send_buf,Grufti->botnick,Grufti->botnick);

	Note_addbody(n,Note->send_buf);
	User_appendnote(u,n);
}
	
noterec *Note_lastnotefromnick(userrec *u_recp, userrec *u_from, char *nick)
{
	noterec	*n, *got_n = NULL;
	char	lookup[NICKLEN];

	/* If nick is registered, use acctname */
	if(u_from->registered)
		strcpy(lookup,u_from->acctname);
	else
		strcpy(lookup,nick);

	/* Search all notes */
	for(n=u_recp->notes; n!=NULL; n=n->next)
	{
		if(isequal(n->nick,lookup))
			got_n = n;
	}
	
	if(got_n == NULL)
		return NULL;

	if(!(got_n->flags & NOTE_NEW))
		return NULL;

	return got_n;
}

void Note_informuseroflevelplus(userrec *u, u_long flag)
{
	char	msg[SERVERLEN], level[FLAGSHORTLEN];

	User_flags2str(flag,level);
	sprintf(msg,"You have been given the level +%s!",level);
	if(flag & USER_MASTER)
		sprintf(msg,"%s  You are a master now.  Be careful, +m means you can delete users, responses, locations, etc. even when not intended.  Have fun!",msg);
	else if(flag & USER_GREET)
		sprintf(msg,"%s  The bot will greet you when you join a channel.  You can set your own greetings with 'addgreet' and 'delgreet'.  Don't make the bot say stupid things, ok?  Think of something creative. :>  (See help on 'greets' for more info)",msg);
	else if(flag & USER_OP)
		sprintf(msg,"%s  You can get ops from the bot now with the command 'op', or by just sending your password with 'pass'.",msg);
	else if(flag & USER_VOICE)
		sprintf(msg,"%s  You can get the mode +v from the bot now with the command 'voice'.",msg);
	else if(flag & USER_NOTICES)
		sprintf(msg,"%s  You will now receive notices from the bot regarding bot status and activity.",msg);
	else if(flag & USER_TRUSTED)
		sprintf(msg,"%s  You can now make the bot do certain things now like leaving and joining a channel, kicking and banning, and other commands which require some degree of responsibility.",msg);
	else if(flag & USER_RESPONSE)
		sprintf(msg,"%s  You can add and delete responses from the bot!  Check out help on 'response' to see how one goes about doing this.  It's not as hard as it looks :>",msg);
	else if(flag & USER_31337)
		sprintf(msg,"%s  Welcome '1337 user :>",msg);
	else if(flag & USER_PROTECT)
		sprintf(msg,"%s  You will be protected by the bot from being banned.",msg);
	else if(flag & USER_FRIEND)
		sprintf(msg,"%s  You will be unbanned if ever you are banned on the channel.",msg);
	
	Note_sendnotice(u,msg);
}

void Note_informuseroflevelmin(userrec *u, u_long flag)
{
	char	level[FLAGSHORTLEN];

	User_flags2str(flag,level);

	Note_sendnotice(u,"Your level +%s has been taken away.  Oh well!",level);
}

void Note_sendnotification(char *format, ...)
{
	va_list	va;
	userrec	*u;

	Note->send_buf1[0] = 0;
	va_start(va,format);
	vsprintf(Note->send_buf1,format,va);
	va_end(va);

	for(u=User->userlist; u!=NULL; u=u->next)
		if(u->flags & USER_NOTICES)
			Note_sendnotice(u,Note->send_buf1);
}

int Note_timeoutnotes()
{
	userrec	*u;
	noterec	*n, *n_next;
	time_t	now = time(NULL), timeout;
	time_t	timeout_notes = (Grufti->timeout_notes+1) * 86400;
	time_t	timeout_notices = (Grufti->timeout_notices+1) * 86400;
	int	notes_deleted = 0;

	for(u=User->userlist; u!=NULL; u=u->next)
	{
		n = u->notes;
		while(n != NULL)
		{
			n_next = n->next;

			/* Notices timeout differently than regular Notes */
			if(n->flags & NOTE_NOTICE)
				timeout = timeout_notices;
			else
				timeout = timeout_notes;

			/* If it's timed out, junk it */
			if((now - n->sent) > timeout && !(n->flags & NOTE_SAVE))
			{
				User_deletenote(u,n);
				notes_deleted++;
			}

			n = n_next;
		}
	}

	return notes_deleted;
}

int Note_numnewnote(userrec *u)
{
	noterec	*n;
	int	num = 0, note_num = 0;

	for(n=u->notes; n!=NULL; n=n->next)
	{
		note_num++;
		if(n->flags & NOTE_NEW)
		{
			num = note_num;
			break;
		}
	}

	return num;
}

void Note_dequeueforwardednotes()
{
	userrec	*u;

	context;
	Note->time_dequeued = time(NULL);

	if(!Note->numNotesQueued)
		return;

	for(u=User->userlist; u!=NULL; u=u->next)
	{
		if(u->forward && u->forward[0] && Note_numforwarded(u))
			DCC_forwardnotes(u,u->forward);
	}

	Note->numNotesQueued = 0;
}




/* ASDF */
/***************************************************************************n
 *
 * noterec record definitions.  No changes should be made to new(), freeall(),
 * append(), delete(), or movetofront().
 *
 ****************************************************************************/

noterec *noterec_new()
{
	noterec *x;

	/* allocate memory */
	x = (noterec *)xmalloc(sizeof(noterec));

	/* initialize */
	x->next = NULL;

	noterec_init(x);

	return x;
}

void noterec_freeall(noterec **list, noterec **last)
{
	noterec *entry = *list, *v;

	while(entry != NULL)
	{
		v = entry->next;
		noterec_free(entry);
		entry = v;
	}

	*list = NULL;
	*last = NULL;
}

void noterec_append(noterec **list, noterec **last, noterec *x)
{
	noterec *lastentry = *last;

	if(*list == NULL)
		*list = x;
	else
		lastentry->next = x;

	*last = x;
	x->next = NULL;
}

void noterec_delete(noterec **list, noterec **last, noterec *x)
{
	noterec *entry, *lastchecked = NULL;

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

			noterec_free(entry);
			return;
		}

		lastchecked = entry;
		entry = entry->next;
	}
}

void noterec_movetofront(noterec **list, noterec **last, noterec *lastchecked, noterec *x)
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
/* record noterec low-level data-specific function definitions. */

void noterec_init(noterec *n)
{
	/* initialize */
	n->nick = NULL;
	n->uh = NULL;
	n->finger = NULL;
	n->sent = 0L;
	n->body = NULL;
	n->body_last = NULL;
	n->num_lines = 0;
	n->flags = 0;
}

u_long noterec_sizeof(noterec *n)
{
	u_long	tot = 0L;
	nbody	*b;

	tot += sizeof(n->next);
	tot += sizeof(n->nick);
	tot += n->nick ? strlen(n->nick)+1 : 0L;
	tot += sizeof(n->uh);
	tot += n->uh ? strlen(n->uh)+1 : 0L;
	tot += sizeof(n->finger);
	tot += n->finger ? strlen(n->finger)+1 : 0L;
	tot += sizeof(n->sent);

	/* body records */
	tot += sizeof(n->body);
	for(b=n->body; b!=NULL; b=b->next)
		tot += nbody_sizeof(b);

	tot += sizeof(n->body_last);
	tot += sizeof(n->num_lines);
	tot += sizeof(n->flags);

	return tot;
}

void noterec_free(noterec *n)
{
	/* Free the elements. */
	xfree(n->nick);
	xfree(n->uh);
	xfree(n->finger);
	Note_freeallbodys(n);

	/* Free this record. */
	xfree(n);
}

void noterec_setnick(noterec *n, char *nick)
{
	mstrcpy(&n->nick,nick);
}

void noterec_setuh(noterec *n, char *uh)
{
	mstrcpy(&n->uh,uh);
}

void noterec_setfinger(noterec *n, char *finger)
{
	mstrcpy(&n->finger,finger);
}

void noterec_display(noterec *n)
{
	nbody	*body;
	char	when[TIMESHORTLEN];

	timet_to_date_short(n->sent,when);
	say("%-9 %-30 %14s",n->nick,n->uh,when);
	for(body=n->body; body!=NULL; body=body->next)
		nbody_display(body);
}


/***************************************************************************n
 *
 * nbody record definitions.  No changes should be made to new(), freeall(),
 * append(), delete(), or movetofront().
 *
 ****************************************************************************/

nbody *nbody_new()
{
	nbody *x;

	/* allocate memory */
	x = (nbody *)xmalloc(sizeof(nbody));

	/* initialize */
	x->next = NULL;

	nbody_init(x);

	return x;
}

void nbody_freeall(nbody **list, nbody **last)
{
	nbody *entry = *list, *v;

	while(entry != NULL)
	{
		v = entry->next;
		nbody_free(entry);
		entry = v;
	}

	*list = NULL;
	*last = NULL;
}

void nbody_append(nbody **list, nbody **last, nbody *x)
{
	nbody *lastentry = *last;

	if(*list == NULL)
		*list = x;
	else
		lastentry->next = x;

	*last = x;
	x->next = NULL;
}

void nbody_delete(nbody **list, nbody **last, nbody *x)
{
	nbody *entry, *lastchecked = NULL;

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

			nbody_free(entry);
			return;
		}

		lastchecked = entry;
		entry = entry->next;
	}
}

void nbody_movetofront(nbody **list, nbody **last, nbody *lastchecked, nbody *x)
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
/* record nbody low-level data-specific function definitions. */

void nbody_init(nbody *n)
{
	/* initialize */
	n->text = NULL;
}

u_long nbody_sizeof(nbody *n)
{
	u_long	tot = 0L;

	tot += sizeof(n->next);
	tot += sizeof(n->text);
	tot += n->text ? strlen(n->text)+1 : 0L;

	return tot;
}

void nbody_free(nbody *n)
{
	/* Free the elements. */
	xfree(n->text);

	/* Free this record. */
	xfree(n);
}

void nbody_settext(nbody *n, char *text)
{
	mstrcpy(&n->text,text);
}

void nbody_display(nbody *n)
{
	say("%s",n->text);
}
