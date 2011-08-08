/******************************************************************************
 * Steve Rider - stever@area23.org
 * Grufti - an interactive, service-oriented bot for IRC 
 *
 * user.c - definitions for the main User object and associated objects
 *
 *****************************************************************************/
/* 28 April 97 */

#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#ifdef MSK
# include <sys/param.h>
#endif
#include <fcntl.h>
#include "grufti.h"
#include "gruftiaux.h"
#include "response.h"
#include "user.h"
#include "misc.h"
#include "log.h"
#include "notify.h"
#include "dcc.h"
#include "listmgr.h"
#include "server.h"
#include "channel.h"
#include "note.h"

/***************************************************************************n
 *
 * User object User definitions
 *
 ****************************************************************************/

static struct userrec_history *userrec_history_new()
{
	struct userrec_history *l;

	l = (struct userrec_history *)xmalloc(sizeof(struct userrec_history));

	l->info = NULL;
	l->date = 0L;
	l->who = NULL;

	return l;
}

static void userrec_history_free(struct userrec_history *l)
{
	xfree(l->info);
	xfree(l->who);

	xfree(l);
}

void User_new()
{
	/* already in use? */
	if(User != NULL)
		User_freeall();

	/* Allocate memory */
	User = (User_ob *)xmalloc(sizeof(User_ob));

	/* Initialize */
	User->userlist = NULL;
	User->last = NULL;
	User->luser = NULL;

	User->num_users = 0;

	User->userlist_active = 0;
}

u_long User_sizeof()
{
	u_long	tot = 0L;
	userrec	*u;

	tot += sizeof(User_ob);
	for(u=User->userlist; u!=NULL; u=u->next)
		tot += userrec_sizeof(u);

	return tot;
}

void User_freeall()
{
	/* Free all user records */
	User_freeallusers();

	/* Free the object */
	xfree(User);
}

/****************************************************************************/
/* object User data-specific function definitions. */

void User_display()
{
	userrec *u;

	say("%d users.",User->num_users);
	for(u=User->userlist; u!=NULL; u=u->next)
	{
		say("-----------------------------------------------------------------");
		userrec_display(u);
		say("-----------------------------------------------------------------");
	}
}

/****************************************************************************/
/* low-level User structure function definitions. */

void User_appenduser(userrec *u)
{
	userrec_append(&User->userlist, &User->last, u);
	User->num_users++;
}

void User_deleteuser(userrec *u)
{
	userrec_delete(&User->userlist, &User->last, u);
	User->num_users--;
}

void User_freeallusers()
{
	userrec_freeall(&User->userlist, &User->last);
	User->num_users = 0;
}

void User_appendnick(userrec *u, nickrec *n)
{
	nickrec_append(&u->nicks, &u->n_last, n);
	u->num_nicks++;
}

void User_deletenick(userrec *u, nickrec *n)
{
	nickrec_delete(&u->nicks, &u->n_last, n);
	u->num_nicks--;
}

void User_freeallnicks(userrec *u)
{
	nickrec_freeall(&u->nicks, &u->n_last);
	u->num_nicks = 0;
}

void User_appendaccount(userrec *u, acctrec *a)
{
	acctrec_append(&u->accounts, &u->a_last, a);
	u->num_accounts++;
}

/* Use caution, since this command does not free the account after unlinking. */
void User_unlinkaccount(userrec *u, acctrec *a)
{
	acctrec_unlink(&u->accounts, &u->a_last, a);
	u->num_accounts--;
}

void User_deleteaccount(userrec *u, acctrec *a)
{
	acctrec_delete(&u->accounts, &u->a_last, a);
	u->num_accounts--;
}

void User_freeallaccounts(userrec *u)
{
	acctrec_freeall(&u->accounts, &u->a_last);
	u->num_accounts = 0;
}

void User_appendgreet(userrec *u, gretrec *g)
{
	gretrec_append(&u->greets, &u->lastgreet, g);
	u->num_greets++;
}

void User_deletegreet(userrec *u, gretrec *g)
{
	gretrec_delete(&u->greets, &u->lastgreet, g);
	u->num_greets--;
}

void User_freeallgreets(userrec *u)
{
	gretrec_freeall(&u->greets, &u->lastgreet);
	u->num_greets = 0;
}

void User_appendnote(userrec *u, noterec *n)
{
	noterec_append(&u->notes, &u->lastnote, n);
	u->num_notes++;
}

void User_deletenote(userrec *u, noterec *n)
{
	noterec_delete(&u->notes, &u->lastnote, n);
	u->num_notes--;
}

void User_freeallnotes(userrec *u)
{
	noterec_freeall(&u->notes, &u->lastnote);
	u->num_notes = 0;
}

/****************************************************************************/
/* object User high-level implementation-specific function definitions. */

void User_loaduserfile()
{
	userrec *u;
	acctrec	*a;

	User_readuserfile();

	context;
	/* If list is NULL, then userfile is either empty or not found. */
	if(User->userlist == NULL)
	{
		Log_write(LOG_MAIN,"*","No userfile.  Creating...");

		/* Create instance of 'luser' record */
		User->luser = userrec_new();

		/* Set up data */
		User->luser->registered = 0;	/* just in case */
		userrec_setacctname(User->luser,"luser");

		/* Now add luser to userlist */
		User_appenduser(User->luser);
	}
	else
	{
		context;
		/* Point the Luser pointer at the unregistered record. */
		for(u=User->userlist; u!=NULL; u=u->next)
			if(!u->registered)
				User->luser = u;
	}

	/* Make sure we HAVE a luser record */
	if(User->luser == NULL)
	{
		/* nope.  make one */
		User->luser = userrec_new();

		User->luser->registered = 0;
		userrec_setacctname(User->luser,"luser");

		/* Now add luser to userlist */
		User_appenduser(User->luser);
	}

	context;
	/* See if activity times need to be reset */
	if((time(NULL) - Grufti->last_day_online) > 86400)
	{
		char day_today[10];

		weekdaytoday_to_str(day_today);
		Log_write(LOG_MAIN,"*","Resetting activity times for today (%s).",day_today);
		User_resetallactivitytimes();

		/* Reset user statistics for the day */
		User_resetuserstatistics();
	}

	context;
	/* Verify all luser accounts are unregistered */
	for(a=User->luser->accounts; a!=NULL; a=a->next)
	{
		if(a->registered)
		{
			Log_write(LOG_ERROR,"*","Luser acct \"%s %s\" was registered!",a->nick,a->uh);
			a->registered = FALSE;
		}
	}

	context;
	/* now the userlist is active. */
	User->userlist_active = 1;
		
}

#ifdef MSK
/* sort by date; don't care about the rest */
int msk_compare_history (struct userrec_history *h1, struct userrec_history *h2)
{
	return(h1->date - h2->date);
}

int msk_match_history (struct userrec_history *h1, struct userrec_history *h2)
{
	return(	(h1->date == h2->date) &&
			!strcmp(h1->who, h2->who) &&
			!strcmp(h1->info, h2->info) );
}

void User_mergeuserfile(char *file)
{
	userrec *user_upd = NULL;
	userrec *user_tmp;
	noterec	*note_tmp;
	struct userrec_history *l;
	char	s[BUFSIZ], ident[BUFSIZ], elem[BUFSIZ];
	char	userfile[MAXPATHLEN], newuserfile[MAXPATHLEN];
	int	line = 0, error = 0, i;
	int isnew;
	FILE	*f;

	/* this might be wrecking stuff...
	User_freeallaccounts(User->luser);
	*/

	/* Open file for reading. */
	sprintf(userfile,"%s/%s",Grufti->botdir,file);
	sprintf(newuserfile,"%s.merged",userfile);
	f = fopen(userfile,"r");
	if(f == NULL)
	{
		Log_write(LOG_ERROR,"*","Userfile \"%s\" not found for merge.",
			userfile);
		return;
	}

	user_tmp = userrec_new();
	note_tmp = noterec_new();
	l = userrec_history_new();
	user_tmp->history = listmgr_new_list(LM_NOMOVETOFRONT);
	listmgr_set_free_func(user_tmp->history, userrec_history_free);
	listmgr_set_compare_func(user_tmp->history, msk_compare_history);
	listmgr_set_match_func(user_tmp->history, msk_match_history);

	isnew = FALSE;

	/* Read each line. */
	while(readline(f,s))
	{
		line++;

		/* Extract identifier code */
		split(ident,s);

		/* Match identifier code */
		switch(ident[0])
		{

		   case 'D':	/* Last day online */
			/* Disregard during a merge.  -- MSK
			Grufti->last_day_online = (time_t)atol(s);
			*/
			break;

		   case 'a':	/* account name */
			/* save changes to current record */
			if (user_tmp->acctname != NULL)
			{
				/* flag which nicks have REG'd acct entries */
				User_marknickswithregaccts(user_tmp);
				listmgr_sort_ascending(user_tmp->history);
				if (isnew)
					User_appenduser(user_tmp);
			}

			/* find the record matching this user; returns User->luser on
			   failure (odd) */
			user_upd = User_user(s);

			/* if we didn't find this user, which is a user other than "luser",
			   build a new space in memory for it */
			if (user_upd == User->luser && strcmp(s,"luser"))
			{
				user_tmp = userrec_new();
				user_tmp->history = listmgr_new_list(LM_NOMOVETOFRONT);
				listmgr_set_free_func(user_tmp->history,
					userrec_history_free);
				listmgr_set_compare_func(user_tmp->history,
					msk_compare_history);
				listmgr_set_match_func(user_tmp->history,
					msk_match_history);
				userrec_setacctname(user_tmp,s);
				isnew = TRUE;
			}
			else
			/* otherwise, we did find the user (or it was indeed "luser") */
			{
				user_tmp = user_upd;
				listmgr_set_compare_func(user_tmp->history,
					msk_compare_history);
				listmgr_set_match_func(user_tmp->history,
					msk_match_history);
				isnew = FALSE;
			}
			break;

 		   case '!':	/* nicks */
			split(elem,s);
			while(elem[0])
			{
				User_addnewnick(user_tmp,elem);
				split(elem,s);
			}
			break;

		   case '@':	/* accounts */
			error = 0;
			User_addnewacctline(user_tmp,s,&error);
			if(error)
			{
				Log_write(LOG_ERROR,"*","[%d] Userfile error: Corrupt account line.",line);
				fatal("Corrupt userfile line.",0);
			}
			break;

		   /* password, finger, email, www, other, plan, greets... */
		   case 'P': userrec_setpass(user_tmp,s); break;
		   case 'f': userrec_setfinger(user_tmp,s); break;
		   case 'e': userrec_setemail(user_tmp,s); break;
		   case 'w': userrec_setwww(user_tmp,s); break;
		   case 'O': userrec_setforward(user_tmp,s); break;
		   case 'o': userrec_setother(user_tmp,s); break;
		   case 'p': userrec_setplan(user_tmp,s); break;
		   case 'g': User_addnewgreet(user_tmp,s); break;
		   case 'l': user_tmp->location = atoi(s);
			Location_increment(user_tmp); break;
		   case 'u': user_tmp->registered = atoi(s); break;
		   case 'r': user_tmp->time_registered = (time_t)atol(s); break;
		   case 'b': user_tmp->bot_commands = atoi(s); break;
		   case 'W': user_tmp->via_web = atoi(s); break;
		   case 'L': User_str2flags(s,&user_tmp->flags); break;
		   case 's':
			user_tmp->last_seen = MAX((time_t)atol(s),user_tmp->last_seen);
			break;
		   case 'm': user_tmp->seen_motd = (time_t)atol(s); break;
		   case 'i': userrec_setsignoff(user_tmp,s); break;

		   case 'I':	/* Birthday */
			for(i=0; i<3; i++)
			{
				split(elem,s);
				user_tmp->bday[i] = elem[0] ? atoi(elem) : 0;
			}
			break;

		   case 't':	/* Set time seen */
			for(i=0; i<DAYSINWEEK; i++)
			{
				split(elem,s);
				user_tmp->time_seen[i] = elem[0] ? atol(elem) : 0;
			}
			break;

		   case 'c':	/* Set public chatter */
			for(i=0; i<DAYSINWEEK; i++)
			{
				split(elem,s);
				user_tmp->public_chatter[i] = elem[0] ? atol(elem) : 0;
			}
			break;

		   case 'k':	/* Set kicks */
			for(i=0; i<DAYSINWEEK; i++)
			{
				split(elem,s);
				user_tmp->kicks[i] = elem[0] ? atol(elem) : 0;
			}
			break;

		   case 'n':	/* Set bans */
			for(i=0; i<DAYSINWEEK; i++)
			{
				split(elem,s);
				user_tmp->bans[i] = elem[0] ? atol(elem) : 0;
			}
			break;

		   /* Read notes */
		   case 'N':	/* <nick> <uh> <time_sent> <flags> */
			/* set nick */
			split(elem,s);
			noterec_setnick(note_tmp,elem);

			/* Set uh */
			split(elem,s);
			noterec_setuh(note_tmp,elem);

			/* Set time_sent */
			split(elem,s);
			note_tmp->sent = (time_t)atol(elem);

			/* Set flags */
			Note_str2flags(s,&note_tmp->flags);
			break;

		   /* Note finger */
		   case 'F':
			noterec_setfinger(note_tmp,s);
			break;

		   /* Note body */
		   case 'B':
			Note_addbodyline(note_tmp,s);
			break;

		   /* End of Note (last body line) */
		   case 'E':
			Note_addbodyline(note_tmp,s);
			User_appendnote(user_tmp,note_tmp);
			note_tmp = noterec_new();
			break;

		   /* user log */
		   case 'G':
			split(elem, s);
			mem_strcpy(&l->who, elem);
			split(elem, s);
			l->date = (time_t)atol(elem);
			mem_strcpy(&l->info, s);

			if (!listmgr_isitem_peek(user_tmp->history, l))
			{
				listmgr_append(user_tmp->history, l);
				l = userrec_history_new();
			}
			break;
		}
	}

	fclose(f);

	/* Write last entry */
	if(user_tmp->acctname != NULL)
	{
		/* flag which nicks have REG'd acct entries */
		User_marknickswithregaccts(user_tmp);
		listmgr_sort_ascending(user_tmp->history);
		if (isnew)
			User_appenduser(user_tmp);
	}

	if (rename(userfile,newuserfile))
		Log_write(LOG_ERROR,"*","Can't rename \"%s\" after merge.", userfile);
}

#endif

/* Read the USERLIST and insert each object */
void User_readuserfile()
{
	userrec *user_tmp;
	noterec	*note_tmp;
	struct userrec_history *l;
	char	s[BUFSIZ], ident[BUFSIZ], elem[BUFSIZ], userfile[256];
	int	line = 0, error = 0, i;
	FILE	*f;

	/* Open file for reading. */
	sprintf(userfile,"%s/%s",Grufti->botdir,Grufti->userfile);
	f = fopen(userfile,"r");
	if(f == NULL)
	{
		Log_write(LOG_ERROR,"*","Userfile \"%s\" not found.",userfile);
		return;
	}


	/* Allocate memory for new nodes */
	user_tmp = userrec_new();
	note_tmp = noterec_new();
	l = userrec_history_new();
	user_tmp->history = listmgr_new_list(LM_NOMOVETOFRONT);
	listmgr_set_free_func(user_tmp->history, userrec_history_free);

	/* Read each line. */
	while(readline(f,s))
	{
		line++;

		/* Extract identifier code */
		split(ident,s);

		/* Match identifier code */
		switch(ident[0])
		{

		   case 'D':	/* Last day online */
			Grufti->last_day_online = (time_t)atol(s);
			break;

		   case 'a':	/* account name */
			if(user_tmp->acctname != NULL)
			{
				/* flag which nicks have REG'd acct entries */
				User_marknickswithregaccts(user_tmp);

				/* make sure each user has a nonzero last login */
				if(user_tmp->last_login == 0)
					user_tmp->last_login = user_tmp->time_registered;

				User_appenduser(user_tmp);
				user_tmp = userrec_new();
				user_tmp->history = listmgr_new_list(LM_NOMOVETOFRONT);
				listmgr_set_free_func(user_tmp->history, userrec_history_free);
			}
			userrec_setacctname(user_tmp,s);
			break;

 		   case '!':	/* nicks */
			split(elem,s);
			while(elem[0])
			{
				User_addnewnick(user_tmp,elem);
				split(elem,s);
			}
			break;

		   case '@':	/* accounts */
			error = 0;
			User_addnewacctline(user_tmp,s,&error);
			if(error)
			{
				Log_write(LOG_ERROR,"*","[%d] Userfile error: Corrupt account line.",line);
				fatal("Corrupt userfile line.",0);
			}
			break;

		   /* password, finger, email, www, other, plan, greets... */
		   case 'P': userrec_setpass(user_tmp,s); break;
		   case 'f': userrec_setfinger(user_tmp,s); break;
		   case 'e': userrec_setemail(user_tmp,s); break;
		   case 'w': userrec_setwww(user_tmp,s); break;
		   case 'O': userrec_setforward(user_tmp,s); break;
		   case 'o': userrec_setother(user_tmp,s); break;
		   case 'p': userrec_setplan(user_tmp,s); break;
		   case 'g': User_addnewgreet(user_tmp,s); break;
		   case 'l': user_tmp->location = atoi(s);
			Location_increment(user_tmp); break;
		   case 'u': user_tmp->registered = atoi(s); break;
		   case 'r': user_tmp->time_registered = (time_t)atol(s); break;
		   case 'b': user_tmp->bot_commands = atoi(s); break;
		   case 'W': user_tmp->via_web = atoi(s); break;
		   case 'L': User_str2flags(s,&user_tmp->flags); break;
		   case 's': user_tmp->last_seen = (time_t)atol(s); break;
		   case 'X': user_tmp->last_login = (time_t)atol(s); break;
		   case 'm': user_tmp->seen_motd = (time_t)atol(s); break;
		   case 'i': userrec_setsignoff(user_tmp,s); break;

		   case 'I':	/* Birthday */
			for(i=0; i<3; i++)
			{
				split(elem,s);
				user_tmp->bday[i] = elem[0] ? atoi(elem) : 0;
			}
			break;

		   case 't':	/* Set time seen */
			for(i=0; i<DAYSINWEEK; i++)
			{
				split(elem,s);
				user_tmp->time_seen[i] = elem[0] ? atol(elem) : 0;
			}
			break;

		   case 'c':	/* Set public chatter */
			for(i=0; i<DAYSINWEEK; i++)
			{
				split(elem,s);
				user_tmp->public_chatter[i] = elem[0] ? atol(elem) : 0;
			}
			break;

		   case 'k':	/* Set kicks */
			for(i=0; i<DAYSINWEEK; i++)
			{
				split(elem,s);
				user_tmp->kicks[i] = elem[0] ? atol(elem) : 0;
			}
			break;

		   case 'n':	/* Set bans */
			for(i=0; i<DAYSINWEEK; i++)
			{
				split(elem,s);
				user_tmp->bans[i] = elem[0] ? atol(elem) : 0;
			}
			break;

		   /* Read notes */
		   case 'N':	/* <nick> <uh> <time_sent> <flags> */
			/* set nick */
			split(elem,s);
			noterec_setnick(note_tmp,elem);

			/* Set uh */
			split(elem,s);
			noterec_setuh(note_tmp,elem);

			/* Set time_sent */
			split(elem,s);
			note_tmp->sent = (time_t)atol(elem);

			/* Set flags */
			Note_str2flags(s,&note_tmp->flags);
			break;

		   /* Note finger */
		   case 'F':
			noterec_setfinger(note_tmp,s);
			break;

		   /* Note body */
		   case 'B':
			Note_addbodyline(note_tmp,s);
			break;

		   /* End of Note (last body line) */
		   case 'E':
			Note_addbodyline(note_tmp,s);
			User_appendnote(user_tmp,note_tmp);
			note_tmp = noterec_new();
			break;

		   /* user log */
		   case 'G':
			split(elem, s);
			mem_strcpy(&l->who, elem);
			split(elem, s);
			l->date = (time_t)atol(elem);
			mem_strcpy(&l->info, s);

			listmgr_append(user_tmp->history, l);
			l = userrec_history_new();
			break;
		}
	}
	fclose(f);

	/* Write last entry */
	if(user_tmp->acctname != NULL)
	{
		/* flag which nicks have REG'd acct entries */
		User_marknickswithregaccts(user_tmp);
		User_appenduser(user_tmp);
	}
}

void User_addhistory(userrec *u, char *who, time_t t, char *msg)
{
	struct userrec_history *l;

	l = userrec_history_new();

	mem_strcpy(&l->who, who);
	l->date = t;
	mem_strcpy(&l->info, msg);

	listmgr_append(u->history, l);
}

void User_delhistory(userrec *u, int pos)
{
	struct userrec_history *l;
	int i;

	for(l = listmgr_begin(u->history), i = 0; l; l = listmgr_next(u->history), i++)
	{
		if(pos == i)
		{
			listmgr_delete(u->history, l);
			return;
		}
	}
}


void User_addnewgreet(userrec *u, char *greet)
{
	gretrec	*g;

#ifdef MSK
	for (g = u->greets; g != NULL; g = g->next)
		if (isequal(g->text, greet))
			return;
#endif

	/* allocate new node */
	g = gretrec_new();

	/* copy text */
	gretrec_settext(g,greet);

	/* add it to their greets */
	User_appendgreet(u,g);
}


void User_addnewnick(userrec *user, char *nick)
{
	nickrec *n;

#ifdef MSK
	for (n = user->nicks; n != NULL; n = n->next)
	if (isequal(n->nick, nick))
		return;
#endif

	/* allocate a new node */
	n = nickrec_new();

	/* set nick */
	nickrec_setnick(n,nick);

	/* Add the account to the user record */
	User_appendnick(user,n);
}

void User_removenick(userrec *u, char *nick)
{
	nickrec *n;

	/* find nick */
	for(n=u->nicks; n!=NULL; n=n->next)
	{
		if(isequal(n->nick,nick))
		{
			User_deletenick(u,n);
			return;
		}
	}
}

void User_removeuser(userrec *u)
{
	/*
	 * To remove a user we need to first move ALL the accounts from
	 * the account list into the luser record.  Then we can delete
	 * the account.
	 */

	User_releaseallaccounts(u);
	Location_decrement(u);
	User_deleteuser(u);
}

userrec *User_addnewuser(char *acctname)
{
	userrec *x;

	/* Create new record */
	x = userrec_new();

	/* append it to the list */
	User_appenduser(x);

	/* Now fill in the data */
	userrec_setacctname(x,acctname);
	User_addnewnick(x,acctname);

	x->registered = User_highestregnumber() + 1;
	x->time_registered = time(NULL);
	x->last_seen = time(NULL);
	x->last_login = time(NULL);
	x->flags |= USER_REGD;

	x->history = listmgr_new_list(LM_NOMOVETOFRONT);
	listmgr_set_free_func(x->history, userrec_history_free);

	return x;
}

int User_highestregnumber()
{
	userrec *u;
	int max_reg_num = 0;

	for(u=User->userlist; u!=NULL; u=u->next)
		if(u->registered > max_reg_num)
			max_reg_num = u->registered;
		
	return max_reg_num;
}
	
void User_moveaccount(userrec *ufrom, userrec *uto, acctrec *a)
/*
 * Functions calling this must be careful not to use a for() construct
 * to iterate.  Use a while() construct, remember ->next using a tmp variable 
 * before moveaccount() is called, then reset your iterate pointer to the tmp
 * variable.
 */
{
	/* Unlink first, THEN append! (Appending reassignes a->next) */
	User_unlinkaccount(ufrom,a);
	User_appendaccount(uto,a);
}
		
noterec *User_addnewnote(userrec *u_recp, userrec *u_from, acctrec *a_from, char *text, u_long flags)
{
	noterec	*x;
	char msg[STDBUF];

	msg[0] = 0;

	/* Allocate a new node */
	x = noterec_new();

	noterec_setuh(x,a_from->uh);
	if(u_from->registered)
	{
		noterec_setnick(x,u_from->acctname);
		if(flags & NOTE_MEMO)
			noterec_setfinger(x,"<Memo to myself>");
		else if(flags & NOTE_WALL)
		{
			char levels[80], finger[80];
			strcpy(msg, text);
			split(levels, msg);
			sprintf(finger, "<Wall'd Note to: %s>", levels);
			noterec_setfinger(x, finger);
		}
		else if(u_from->finger)
			noterec_setfinger(x,u_from->finger);
		else
			noterec_setfinger(x,"");
	}
	else
	{
		noterec_setfinger(x,"<not registered>");
		noterec_setnick(x,a_from->nick);
	}
	
	x->sent = time(NULL);
	x->flags = flags;

	/* Add body */
	if(msg[0])
		Note_addbody(x,msg);
	else
		Note_addbody(x,text);

	/* Mark forwarded if necessary */
	if(u_recp->forward && u_recp->forward[0])
	{
		x->flags |= NOTE_FORWARD;
		Note->numNotesQueued++;
	}

	User_appendnote(u_recp,x);

	return x;
}

acctrec *User_addnewaccount(userrec *user, char *nick, char *uh)
{
	acctrec	*x, *a;
	char	*p1, *p2;
	int	reg = 0;

	/* allocate a new node */
	x = acctrec_new();

	/* and assign it some data */
	acctrec_setnick(x,nick);

	/* check if uh is already hostmasked */
	p1 = strchr(uh,'@');
	if(p1 == NULL)
	{
		Log_write(LOG_ERROR,"*","Oops.  In User_addnewaccount, uh is not of type uh. (%s)",uh);
		acctrec_free(x);
		return NULL;
	}

	/* Ok to add to list */
	User_appendaccount(user,x);

	p2 = strchr(p1,'*');
	if(p2 == NULL)
	{
		char m_uh[UHOSTLEN];
		makehostmask(m_uh,uh);
		acctrec_setuh(x,m_uh);
	}
	else
		acctrec_setuh(x,uh);

	/*
	 * If any of the other hostmasks matching this one are registered,
	 * we need to set this one as registered.  If we don't, what happens is
	 * that a hostmask might be listed multiple times for each of the
	 * registered nicks using it.  Some may be marked 'registered' and
	 * some may not.  This creates a lot of confusion and doesn't allow
	 * us to determine quickly if the hostmask is verified or not.
	 * 
	 * We also need to reupdate the nicks in case the account added has
	 * a nick in the nicklist which needs to be marked NICK_HAS_ACCT_ENTRY
 	 */
	for(a=user->accounts; a!=NULL; a=a->next)
		if(isequal(a->uh,x->uh))
			if(a->registered)
				reg++;

	if(reg)
	{
		x->registered = TRUE;

		/* Update the nicks in case one matches */
		User_marknickswithregaccts(user);
	}
	else
		x->registered = FALSE;


	x->first_seen = time(NULL);
	x->last_seen = time(NULL);

	return x;
}

void User_addnewacctline(userrec *user, char *s, int *error)
{
	acctrec *x;
	char elem[BUFSIZ];

	/* error checking */
	if(num_elements(s) < ACCT_NUM_USERFILE_ELEMENTS)
	{
		*error=1;
		return;
	}

	/* allocate a new node */
	x = acctrec_new();

	/* Set nick */
	split(elem,s);
	acctrec_setnick(x,elem);

	/* Set uh */
	split(elem,s);
	acctrec_setuh(x,elem);

	/* Set registered */
	split(elem,s);
	x->registered = atoi(elem) == 1 ? TRUE : FALSE;

	/* Set first seen */
	split(elem,s);
	x->first_seen = (time_t)atol(elem);

	/* Set last seen */
	split(elem,s);
	x->last_seen = (time_t)atol(elem);

	/* Add the account to the user record */
	User_appendaccount(user,x);
}

/*
 * We've got enough options here that we need to eventually set up an options
 * flag.
 */
void User_writeuserfile()
/*
 * Field info:
 *    D - Last day online
 *    a - acctname
 *    ! - nicks: <nick> [<nick> ...]
 *    @ - accounts: <nick> <uh> <registered> <first_seen> <last_seen>
 *    P - password
 *    f - finger info
 *    e - email
 *    w - www
 *    O - foward address
 *    o - other: <#> [field] <info>
 *    p - plan: <#> <plan>
 *    I - birthday: <m> <d> <y>
 *    g - greets: <greettext>
 *    l - location number
 *    u - Register # (0 = unregistered)
 *    r - time registered
 *    b - # bot commands
 *    W - # web/telnet commands
 *    L - levels
 *    s - time last seen
 *    X - time last logged in
 *    i - signoff message
 *    t - time online per day per week (Su Mo Tu We Th Fr Sa)
 *    m - when user last saw motd
 *    c - public chatter per day per week
 *    k - kicks per day per week
 *    n - bans per day per week
 *    N - notes: <nick> <uh> <time_sent> <flags>
 *    F - notes: <finger>
 *    B - notes: <text>
 *    E - notes: <last text line>	 (signifies when to append Note)
 *    G - User log
 */
{
	FILE	*f;
	userrec	*u;
	nickrec	*n;
	acctrec	*a;
	gretrec	*g;
	struct userrec_history *l;
	noterec	*note;
	nbody	*body;
	char	s[BUFSIZ], date_today[DATELONGLEN], tmpfile[256], userfile[256];
	int	day, x;

	/* bail if userlist is inactive (pending read...) */
	if(!User->userlist_active)
	{
		Log_write(LOG_MAIN,"*","Userlist is not active.  Ignoring write request.");
		return;
	}

	/* Open for writing */
	sprintf(userfile,"%s/%s",Grufti->botdir,Grufti->userfile);
	sprintf(tmpfile,"%s.tmp",userfile);
	f = fopen(tmpfile,"w");
	if(f == NULL)
	{
		Log_write(LOG_MAIN|LOG_ERROR,"*","Couldn't write userfile.");
		return;
	}

	/* Write header */
	writeheader(f,"User->userlist - User info and accounts.",userfile);

	/* Write last day online time */
	timet_to_date_long(time_today_began(time(NULL)),date_today);
	if(fprintf(f,"# Last day online: \"%s\"\n",date_today) == EOF)
		return;
	if(fprintf(f,"D %lu\n\n",time_today_began(time(NULL))) == EOF)
		return;

	/* Write each record */
	for(u=User->userlist; u!=NULL; u=u->next)
	{
		/* Write account name */
		if(u->acctname != NULL)
		{
			if(fprintf(f,"a %s\n",u->acctname) == EOF)
				return;
		}
		else
		{
			if(fprintf(f,"a -\n") == EOF)
				return;
		}

		/* Write nicks */
		if(u->nicks != NULL)
		{
			if(fprintf(f,"!") == EOF)
				return;
			for(n=u->nicks; n!=NULL; n=n->next)
				if(fprintf(f," %s",n->nick)==EOF)
					return;
			if(fprintf(f,"\n") == EOF)
				return;
		}

		/* Write acctline */
		for(a=u->accounts; a!=NULL; a=a->next)
			if(fprintf(f,"@ %s %s %d %lu %lu\n",a->nick,a->uh,a->registered,a->first_seen,a->last_seen) == EOF)
				return;

		/* Write password */
		if(u->pass != NULL)
			if(fprintf(f,"P %s\n",u->pass) == EOF)
				return;

		/* Write finger */
		if(u->finger != NULL)
			if(fprintf(f,"f %s\n",u->finger) == EOF)
				return;

		/* Write email */
		if(u->email != NULL)
			if(fprintf(f,"e %s\n",u->email) == EOF)
				return;

		/* Write www */
		if(u->www != NULL)
			if(fprintf(f,"w %s\n",u->www) == EOF)
				return;

		/* Write forward */
		if(u->forward != NULL)
			if(fprintf(f,"O %s\n",u->forward) == EOF)
				return;

		/* Write other */
		if(u->other1 != NULL)
			if(fprintf(f,"o 1 %s\n",u->other1) == EOF)
				return;

		if(u->other2 != NULL)
			if(fprintf(f,"o 2 %s\n",u->other2) == EOF)
				return;

		if(u->other3 != NULL)
			if(fprintf(f,"o 3 %s\n",u->other3) == EOF)
				return;

		if(u->other4 != NULL)
			if(fprintf(f,"o 4 %s\n",u->other4) == EOF)
				return;

		if(u->other5 != NULL)
			if(fprintf(f,"o 5 %s\n",u->other5) == EOF)
				return;

		if(u->other6 != NULL)
			if(fprintf(f,"o 6 %s\n",u->other6) == EOF)
				return;

		if(u->other7 != NULL)
			if(fprintf(f,"o 7 %s\n",u->other7) == EOF)
				return;

		if(u->other8 != NULL)
			if(fprintf(f,"o 8 %s\n",u->other8) == EOF)
				return;

		if(u->other9 != NULL)
			if(fprintf(f,"o 9 %s\n",u->other9) == EOF)
				return;

		if(u->other10 != NULL)
			if(fprintf(f,"o 0 %s\n",u->other10) == EOF)
				return;

		/* Write plan */
		if(u->plan1 != NULL)
			if(fprintf(f,"p 1 %s\n",u->plan1) == EOF)
				return;

		if(u->plan2 != NULL)
			if(fprintf(f,"p 2 %s\n",u->plan2) == EOF)
				return;

		if(u->plan3 != NULL)
			if(fprintf(f,"p 3 %s\n",u->plan3) == EOF)
				return;

		if(u->plan4 != NULL)
			if(fprintf(f,"p 4 %s\n",u->plan4) == EOF)
				return;

		if(u->plan5 != NULL)
			if(fprintf(f,"p 5 %s\n",u->plan5) == EOF)
				return;

		if(u->plan6 != NULL)
			if(fprintf(f,"p 6 %s\n",u->plan6) == EOF)
				return;

		/* Write birthday info */
		if(u->bday[0])
			if(fprintf(f,"I %d %d %d\n",u->bday[0],u->bday[1],u->bday[2]) == EOF)
				return;

		/* Write greets */
		for(g=u->greets; g!=NULL; g=g->next)
			if(fprintf(f,"g %s\n",g->text) == EOF)
				return;

		/* Write location */
		if(u->location != 0)
			if(fprintf(f,"l %d\n",u->location) == EOF)
				return;

		/* Write registered bit */
		if(u->registered != 0)
			if(fprintf(f,"u %d\n",u->registered) == EOF)
				return;

		/* Write time registered */
		if(u->time_registered != 0L)
			if(fprintf(f,"r %lu\n",u->time_registered) == EOF)
				return;

		/* Write num bot commands */
		if(u->bot_commands != 0)
			if(fprintf(f,"b %d\n",u->bot_commands) == EOF)
				return;

		/* Write num web/telnet commands */
		if(u->via_web != 0)
			if(fprintf(f,"W %d\n",u->via_web) == EOF)
				return;

		/* Write user flags */
		User_flags2str(u->flags,s);
		if(fprintf(f,"L %s\n",s) == EOF)
			return;

		/* Write last time seen */
		if(u->last_seen != 0L)
			if(fprintf(f,"s %lu\n",u->last_seen) == EOF)
				return;

		/* Write last time logged in */
		if(u->last_login != 0L)
			if(fprintf(f,"X %lu\n",u->last_login) == EOF)
				return;

		/* Write signoff message */
		if(u->signoff != NULL)
			if(fprintf(f,"i %s\n",u->signoff) == EOF)
				return;

		/* Write seen motd */
		if(u->seen_motd)
			if(fprintf(f,"m %lu\n",u->seen_motd) == EOF)
				return;

		/* Print time seen for each day */
		if(fprintf(f,"t ") == EOF)
			return;
		for(day=0; day<6; day++)
			if(fprintf(f,"%lu ",u->time_seen[day]) == EOF)
				return;
		if(fprintf(f,"%lu\n",u->time_seen[day]) == EOF)
			return;

		/* Print public chatter for each day */
		if(fprintf(f,"c ") == EOF)
			return;
		for(day=0; day<6; day++)
			if(fprintf(f,"%lu ",u->public_chatter[day]) == EOF)
				return;
		if(fprintf(f,"%lu\n",u->public_chatter[day]) == EOF)
			return;

		/* Print kicks for each day */
		if(fprintf(f,"k ") == EOF)
			return;
		for(day=0; day<6; day++)
			if(fprintf(f,"%d ",u->kicks[day]) == EOF)
				return;
		if(fprintf(f,"%d\n",u->kicks[day]) == EOF)
			return;

		/* Print bans for each day */
		if(fprintf(f,"n ") == EOF)
			return;
		for(day=0; day<6; day++)
			if(fprintf(f,"%d ",u->bans[day]) == EOF)
				return;
		if(fprintf(f,"%d\n",u->bans[day]) == EOF)
			return;

		/* Write notes */
		for(note=u->notes; note!=NULL; note=note->next)
		{
			Note_flags2str(note->flags,s);
			if(fprintf(f,"N %s %s %lu %s\n",note->nick,note->uh,note->sent,s) == EOF)
				return;
			if(fprintf(f,"F %s\n",note->finger) == EOF)
				return;
			for(body=note->body; body!=NULL; body=body->next)
			{
				/* Print 'E' for last body line */
				if(body == note->body_last)
				{
					if(fprintf(f,"E %s\n",body->text) == EOF)
						return;
				}
				else
				{
					if(fprintf(f,"B %s\n",body->text) == EOF)
						return;
				}
			}
		}

		/* Write user logs */
		if(u->history != NULL)
		{
#ifdef MSK
			for(l = listmgr_begin_sorted(u->history); l;
				l = listmgr_next_sorted(u->history))
#else
			for(l = listmgr_begin(u->history); l; l = listmgr_next(u->history))
#endif
			{
				if(fprintf(f,"G %s %lu %s\n", l->who, l->date, l->info) == EOF)
					return;
			}
		}

		if(fprintf(f,"#####\n") == EOF)
			return;

	}
	fclose(f);

	/* Move tmpfile over to main userfile */
	if(Grufti->keep_tmpfiles)
		x = copyfile(tmpfile,userfile);
	else
		x = movefile(tmpfile,userfile);

	verify_write(x,tmpfile,userfile);
	if(x == 0)
		Log_write(LOG_MAIN,"*","Wrote userfile.");
}
	

void User_str2flags(char *s, u_long *f)
{
	char *p;

	*f = 0L;
	for(p=s; *p; p++)
	{
		if(*p == 'g') *f |= USER_GREET;
		else if(*p == 'o') *f |= USER_OP;
		else if(*p == 'v') *f |= USER_VOICE;
		else if(*p == 'n') *f |= USER_NOTICES;
		else if(*p == 'k') *f |= USER_OK;
		else if(*p == 'm') *f |= USER_MASTER;
		else if(*p == 'l') *f |= USER_LYRICS;
		else if(*p == 'L') *f |= USER_RESPONSE;
		else if(*p == 'f') *f |= USER_FRIEND;
		else if(*p == 'x') *f |= USER_XFER;
		else if(*p == 'b') *f |= USER_BOT;
		else if(*p == 'p') *f |= USER_PROTECT;
		else if(*p == 'r') *f |= USER_REGD;
		else if(*p == 't') *f |= USER_TRUSTED;
		else if(*p == '3') *f |= USER_31337;
		else if(*p == '0') *f |= USER_NOLEVEL;
		else if(*p == 'w') *f |= USER_OWNER;
		else if(*p == '-') *f |= USER_ANYONE;
		else if(*p != '+') *f |= USER_INVALID;
	}

	/* 'No level' is exclusive. */
	if(*f & USER_NOLEVEL)
		*f = USER_NOLEVEL;

	/* 'Any level' is exclusive. */
	if(*f & USER_ANYONE)
		*f = 0L;

	/* Calling functions should check for USER_INVALID. */
}


void User_flags2str(u_long f, char *s)
{
	s[0] = 0;
	if(f & USER_BOT)	strcat(s,"b");
	if(f & USER_FRIEND)	strcat(s,"f");
	if(f & USER_GREET)	strcat(s,"g");
	if(f & USER_OK)		strcat(s,"k");
	if(f & USER_RESPONSE)	strcat(s,"L");
	if(f & USER_LYRICS)	strcat(s,"l");
	if(f & USER_MASTER)	strcat(s,"m");
	if(f & USER_NOTICES)	strcat(s,"n");
	if(f & USER_OP)		strcat(s,"o");
	if(f & USER_PROTECT)	strcat(s,"p");
	if(f & USER_REGD)	strcat(s,"r");
	if(f & USER_TRUSTED)	strcat(s,"t");
	if(f & USER_VOICE)	strcat(s,"v");
	if(f & USER_XFER)	strcat(s,"x");
	if(f & USER_31337)	strcat(s,"3");
	if(f & USER_OWNER)	strcat(s,"w");
	if(f & USER_NOLEVEL)	strcat(s,"0");
	if(f & USER_INVALID)	strcat(s,"?");
	if(s[0] == 0)		strcat(s,"-");
}

userrec *User_user(char *nick)
/*
 * Like account, always returns a user.  If a user is not found it means
 * they're not registered, and the luser record is returned.
 */
{
	userrec *ret;

	ret = userrec_user(&User->userlist, &User->last, nick);

	/* If nick isn't found in the userlist, then it's a luser. (not regd) */
	if(ret == NULL)
		return User->luser;
	else
		return ret;
}

int User_isknown(char *nick)
{
	userrec	*u;
	acctrec	*a;

	u = User_user(nick);
	if(!u->registered)
	{
		/* Search all accounts in luser */
		for(a=u->accounts; a!=NULL; a=a->next)
			if(isequal(a->nick,nick))
				return 1;

		return 0;
	}
	else
		return 1;
}

int User_isuser(char *nick)
{
	userrec	*u;

	u = User_user(nick);
	if(!u->registered)
		return 0;
	else
		return 1;
}

int User_isnick(userrec *user, char *nick)
{
	return(nickrec_isnick(&user->nicks, &user->n_last, nick));
}

acctrec *User_account_dontmakenew(userrec *user, char *nick, char *uh)
{
	char *p1, *p2;
	acctrec *a = NULL;

	/* check if uh is already hostmasked */
	p1 = strchr(uh,'@');
	if(p1 == NULL)
	{
		Log_write(LOG_DEBUG,"*","Oops.  In User_account, uh is not of type uh. %s - %s (%s)",user->acctname,nick,uh);
		return NULL;
	}
	p2 = strchr(p1,'*');

	/* No hostmask found.  Make a hostmask and search for account with it */
	if(p2 == NULL)
	{
		char m_uh[UHOSTLEN];
		makehostmask(m_uh,uh);
		a = acctrec_account(&user->accounts,&user->a_last,nick,m_uh);
		return a;
	}

	/* uh is hostmasked.  search for it. */
	a = acctrec_account(&user->accounts,&user->a_last,nick,uh);

	return a;
}
	
acctrec *User_account(userrec *user, char *nick, char *uh)
/*
 * Always returns an account except in case of error.  If an account is not
 * found, a new one is created and returned.
 * Since accounts are stored as masks (user@*.host1.dom), the uh is made
 * into a hostmask with which it is searched against.  New accounts are
 * created with a hostmask with no further effort.
 */
{
	char *p1, *p2;
	acctrec *a = NULL;

	/* check if uh is already hostmasked */
	p1 = strchr(uh,'@');
	if(p1 == NULL)
	{
		Log_write(LOG_DEBUG,"*","Oops.  In User_account, uh is not of type uh. %s - %s (%s)",user->acctname,nick,uh);
		return NULL;
	}
	p2 = strchr(p1,'*');

	/* No hostmask found.  Make a hostmask and search for account with it */
	if(p2 == NULL)
	{
		char m_uh[UHOSTLEN];
		makehostmask(m_uh,uh);
		a = acctrec_account(&user->accounts,&user->a_last,nick,m_uh);

		/* Didn't find account.  Make a new one and return it */
		if(a == NULL)
			a = User_addnewaccount(user,nick,m_uh);

		return a;
	}

	/* uh is hostmasked.  search for it. */
	a = acctrec_account(&user->accounts,&user->a_last,nick,uh);

	/* Didn't find account.  Make a new one and return it */
	if(a == NULL)
		a = User_addnewaccount(user,nick,uh);

	return a;
}
	
void User_gotpossiblesignon(userrec *u, acctrec *a)
{
	nickrec *n;

	context;
	/* If we're waiting on notify results, we'll catch it shortly */
	if(Notify->flags & NOTIFY_PENDING)
		return;

	context;
	/* Is user registered? */
	if(!u->registered || !a->registered)
		return;

	context;
	n = User_nick(u,a->nick);
	if(n == NULL)
	{
		Log_write(LOG_DEBUG,"*","Nick %s does not exist in user %s even though acct %s shows it does!",a->nick,u->acctname,a->uh);
		return;
	}

	context;
	/* nick was already on.  ignore */
	if(n->flags & NICK_ONIRC)
		return;

	context;

	/* Point the nick's account pointer to the account that is on. */
	n->account = a;

	/* Nick was not on prev.  Trigger a real gotsignon(). Set account! */
	User_gotsignon(u,n);
}

void User_gotsignon(userrec *u, nickrec *n)
/*
 * gotsignon and gotsignoff only update the seen time when a user
 * joins or leaves irc.  The current day's time_seen for each user is updated
 * each night at midnight, so we don't have to worry about the time_seen
 * overlapping days.  The current time for the next day is zeroed at midnight
 * also, so we don't have to worry about data from last month already existing.
 * 
 * Now we also inform the user of New Notes.
 */
{
	context;
	/* Make sure nick has a reg'd account record */
	if(!(n->flags & NICK_HAS_ACCT_ENTRY))
		return;

	context;
	if(n == NULL)
	{
	context;
		Log_write(LOG_DEBUG,"*","shit, n is NULL for %s",u->acctname);
	context;
		return;
	}

	context;
/*
	Log_write(LOG_MAIN,"*","%s signed on.",n->nick);
*/
	context;
	n->flags |= NICK_ONIRC;
	context;
	if(n->account == NULL)
	{
		context;
		Log_write(LOG_DEBUG,"*","Shit, n->account is null for n=%s (%s)",n->nick,u->acctname);
		context;
		return;
	}

	context;
	n->account->flags |= ACCT_ONIRC;
	context;
	n->joined = time(NULL);

	context;
	/* If first activity is set, the user is on via another nick. ignore */
	if(u->first_activity != 0L)
		return;

	context;
	/* Set first activity time */
	u->first_activity = time(NULL);

	context;
	/* Clear signoff message from last time */
	xfree(u->signoff);
	u->signoff = NULL;

	/* and inform them of possible new notes */

	/* (need to find a better way of doing this) */
	if((time(NULL) - u->last_seen) > Grufti->wait_to_informofnotes)
		;

	if(Grufti->notify_of_notes_on_signon)
		User_notifyofnotesonsignon(u,n);

	if(Grufti->auto_detect_takeover && Grufti->takeover_in_progress)
		sendnotice(n->nick,"%s",Grufti->takeover_in_progress_msg);
}

void User_gotpossiblesignoff(userrec *u, acctrec *a)
{
	nickrec *n;

context
	/* If we're waiting on notify results, we'll catch it shortly */
	if(Notify->flags & NOTIFY_PENDING)
		return;

context
	/* Is user registered? */
	if(u == NULL)
	{
		Log_write(LOG_DEBUG,"*","Shit.  username is NULL in gotpossiblesignoff.");
		return;
	}

context
	if(a == NULL)
	{
		Log_write(LOG_DEBUG,"*","Shit.  a is NULL in gotpossiblesignoff. (u=%s)",u->acctname);
		return;
	}

context
	if(!u->registered || !a->registered)
		return;

context
	n = User_nick(u,a->nick);
	if(n == NULL)
	{
		Log_write(LOG_DEBUG,"*","Nick %s does not exist in user %s even though acct %s shows it does!",a->nick,u->acctname,a->uh);
		return;
	}

context
	/* nick was not on previously.  ignore */
	if(!(n->flags & NICK_ONIRC))
		return;

context
	/* Nick was on previously.  Trigger a real gotsignoff(). */
	User_gotsignoff(u,n);

context
	/* Point the nick's account pointer to NULL */
	n->account = NULL;
}
	
void User_gotsignoff(userrec *u, nickrec *n)
/*
 * Careful.  Consider the scenario where a registered user using two separate
 * nicks exists on IRC.  We need to trigger a signoff only when none
 * of the nicks for that user are on IRC.
 */
{
	nickrec	*n_check;
	int	found = 0;

	context;
	/* Make sure nick has a reg'd account record */
	if(!(n->flags & NICK_HAS_ACCT_ENTRY))
		return;

	context;

	/* someday we'll watch signon/signoffs */
/*
	Log_write(LOG_MAIN,"*","%s signed off.",n->nick);
*/
	n->flags &= ~NICK_ONIRC;
	n->joined = 0L;
	if(n->account)
	{
		n->account->flags &= ~ACCT_ONIRC;
		n->account->last_seen = time(NULL);
	}

	context;
	/* Is this user on more than once? */
	for(n_check=u->nicks; n_check!=NULL; n_check=n_check->next)
		if(n_check->flags & NICK_ONIRC)
			found++;

	context;
	/* User is on IRC using another nick.  Ignore */
	if(found)
		return;

	context;
	/* first activity even?  if not, well hell... */
	if(u->first_activity == 0L)
	{
		Log_write(LOG_DEBUG,"*","No first activity was set for this user and we gotsignoff.  %s",n->nick);
		return;
	}

	context;
/*
	Log_write(LOG_MAIN,"*","User %s (%s) is finally leaving.  time_seen = %lu",u->acctname,n->nick,time(NULL) - u->first_activity);
*/

	context;
	/* Calculate time seen */
	u->time_seen[today()] += (time(NULL) - u->first_activity);
	u->first_activity = 0L;

	/* Mark when we last saw them */
	u->last_seen = time(NULL);
}

/* Certain things must be consistent in the userlist */
void User_checkcrucial()
{
	userrec	*u, *u_next;
	int	found = 0;

	/* check that only one record exists for an entry with registered = 0 */
	for(u=User->userlist; u!=NULL; u=u->next)
		if(u->registered == 0)
			found++;

	if(found != 1)
	{
		Log_write(LOG_ERROR|LOG_MAIN,"*","Corrupt userlist.  Contains %d non-registered records.  (Should be 1).",found);
		fatal("Corrupt userlist.  Contains !=1 non-registered records.",0);
	}

	/* check for duplicate nick entries */

	/* check that all the nicks in the account field match the nicks */

	/* delete any nicks that have pass = NULL */
	u = User->userlist;
	while(u != NULL)
	{
		u_next = u->next;
		if(u->pass == NULL && u->registered)
		{
			Log_write(LOG_MAIN,"*","Deleted account %s. (no password)",u->acctname);
			User_deleteuser(u);
		}
		u = u_next;
	}
}

void User_makenicklist(char *nicks, userrec *u)
{
	nickrec	*n;
	char	flags[10];

	context;
	if(u->nicks == NULL)
		return;

	context;
	nicks[0] = 0;

	/* now sort them */
	context;
	nickrec_sort(&u->nicks,&u->n_last);

	context;
	/* now write the output. There must ALWAYS be at least one nick. */
	for(n=u->nicks; n!=NULL; n=n->next)
	{
	context;
		flags[0] = 0;
	context;
		if(n->flags & NICK_VIADCC)
			strcat(flags,"*");
	context;
		if(n->flags & NICK_VIATELNET)
			strcat(flags,"&");
	context;
		if(n->flags & NICK_ONIRC)
			strcat(flags,"+");

	context;
		sprintf(nicks,"%s%s%s, ",nicks,flags,n->nick);
	context;
	}
	context;
	if(nicks[0])
		nicks[strlen(nicks) - 2] = 0;
}

void User_resetallpointers()
{
	/*
	 * After a delete of an account, user, or nick, or a creation
	 * of a user record or account, Channel and DCC pointers
	 * must be reset to reflect new changes.
	 */

	Channel_resetallpointers();
	DCC_resetallpointers();
	command_reset_all_pointers();
}

/* Moves all accounts in 'u' with the nick 'nick' to the luser record. */
void User_releaseaccounts(userrec *u, char *nick)
{
	acctrec *a, *a_next;

	/* Release User MUST be registered, and not the luser record.  Doh! */
	if(u == User->luser)
	{
		Log_write(LOG_DEBUG,"*","Oops.  Attempting to release accounts from the Luser record. %s %s",u->acctname,nick);
		return;
	}

	a = u->accounts;
	while(a != NULL)
	{
		a_next = a->next;
		if(isequal(a->nick,nick))
		{
			User_moveaccount(u,User->luser,a);
			a->registered = 0;
			a->flags &= ~ACCT_ONIRC;
		}
		a = a_next;
	}
}

/* Moves all accounts in 'u' to the luser record. */
void User_releaseallaccounts(userrec *u)
{
	acctrec *a, *a_next;

	/* Release User MUST be registered, and not the luser record.  Doh! */
	if(u == User->luser)
	{
		Log_write(LOG_DEBUG,"*","Oops.  Attempting to release accounts from the Luser record. %s",u->acctname);
		return;
	}

	a = u->accounts;
	while(a != NULL)
	{
		a_next = a->next;
		User_moveaccount(u,User->luser,a);
		a->registered = 0;
		a = a_next;
	}
}

void User_moveaccounts_into_newhome(userrec *u, char *nick)
/*
 * Update: Move accounts matching the nick 'nick' from the Luser record
 * to 'u'.  Hostmasks which match other registered hostmasks in this acct
 * get their registered bit marked.
 */
{
	acctrec *a, *a_next;

	/* Update User MUST be registered, and not the luser record.  Doh! */
	if(u == User->luser)
	{
		Log_write(LOG_DEBUG,"*","Oops.  Attempting to update accounts to the luser record. %s %s",u->acctname,nick);
		return;
	}

	a = User->luser->accounts;
	while(a != NULL)
	{
		a_next = a->next;
		if(isequal(a->nick,nick))
		{
			/*
			 * If any hostmask in newhome is registered,
			 * register this one.
			 */
			if(User_isuhregistered(u,a->uh))
				a->registered = 1;
			User_moveaccount(User->luser,u,a);
		}
		a = a_next;
	}

	/* We updated the registered status of hostmasks... recheck all nicks */
	User_marknickswithregaccts(u);
}

int User_isuhregistered(userrec *u, char *uh)
{
	acctrec	*a;

	for(a=u->accounts; a!=NULL; a=a->next)
		if(isequal(a->uh,uh) && a->registered)
			return 1;

	return 0;
}

nickrec *User_nick(userrec *u, char *nick)
{
	if(!u->registered)
	{
		Log_write(LOG_DEBUG,"*","Attempting to query nickrec with user luser. (%s)",nick);
		return NULL;
	}

	return nickrec_nick(&u->nicks,&u->n_last,nick);
}

void User_updateregaccounts(userrec *u, char *uh)
/*
 * Here we check each hostmask to see if it matches the uh passed to us.
 * If it does, we mark it as being registered.  The passed uh needs to be
 * in hostmask form.
 *
 * Next we update all the nicks which have registered accts, since we may
 * have just made an account registered.
 */
{
	acctrec	*a;


	for(a=u->accounts; a!=NULL; a=a->next)
		if(isequal(a->uh,uh) && !a->registered)
			a->registered = TRUE;

	User_marknickswithregaccts(u);
}

void User_marknickswithregaccts(userrec *u)
{
	acctrec	*a;
	nickrec	*n;

	if(!u->registered)
		return;

	for(a=u->accounts; a!=NULL; a=a->next)
	{
		if(a->registered)
		{
			n = User_nick(u,a->nick);
			if(n != NULL)
				n->flags |= NICK_HAS_ACCT_ENTRY;

		}
	}
}

void User_gotactivity(userrec *u, acctrec *a)
{
	if(u->registered)
		u->last_seen = time(NULL);
	a->last_seen = time(NULL);
}

void User_nickviadccon(userrec *u, dccrec *d)
{
	nickrec	*n;

	/* Turn on account */
	d->account->flags |= ACCT_VIADCC;

	/* Turn on nicks only if registered */
	if(u->registered)
	{
		n = User_nick(u,d->nick);
		if(n != NULL)
			n->flags |= NICK_VIADCC;
	}
}

void User_nickviatelneton(userrec *u, dccrec *d)
{
	nickrec	*n;

	/* Turn on account */
	d->account->flags |= ACCT_VIATELNET;

	/* Turn on nicks only if registered */
	if(u->registered)
	{
		n = User_nick(u,d->nick);
		if(n != NULL)
			n->flags |= NICK_VIATELNET;
	}
}

void User_nickviatelnetoff(userrec *u, dccrec *d)
{
	dccrec	*dcheck;
	nickrec *n;
	acctrec	*a;
	int	count = 0;

	/* Don't turn off _VIATELNET unless no other dcc's point to account */
	for(dcheck=DCC->dcclist; dcheck!=NULL; dcheck=dcheck->next)
		if(dcheck->account && dcheck != d && dcheck->account == d->account && (dcheck->flags & CLIENT_TELNET_CHAT))
			count++;


	/* Account is turned off only when it's the last one via telnet */
	if(!count)
		d->account->flags &= ~ACCT_VIATELNET;


	/* If we're not registered, get the fuck out of here (we better be) */
	if(!u->registered)
		return;


	/* Check if any other nicks in user are connected via telnet on uh */
	count = 0;
	for(a=u->accounts; a!=NULL; a=a->next)
		if(isequal(a->nick,d->nick) && (a->flags & ACCT_VIATELNET))
			count++;


	/* Nick is turned off only when it's the last one via TELNET */
	if(!count)
	{
		n = User_nick(u,d->nick);
		if(n != NULL)
			n->flags &= ~NICK_VIATELNET;
	}
}

void User_nickviadccoff(userrec *u, dccrec *d)
{
	dccrec	*dcheck;
	nickrec *n;
	acctrec	*a;
	int	count = 0;

	/* Don't turn off ACCT_VIADCC unless no other dcc's point to account */
	for(dcheck=DCC->dcclist; dcheck!=NULL; dcheck=dcheck->next)
		if(dcheck->account && dcheck != d && dcheck->account == d->account && !(dcheck->flags & CLIENT_TELNET_CHAT))
			count++;
				

	/* Account is turned off only when it's the last one via DCC */
	if(!count)
		d->account->flags &= ~ACCT_VIADCC;


	/* If we're not registered, get the fuck out of here */
	if(!u->registered)
		return;


	/* Check if any other nicks in user are connected via dcc on that uh */
	count = 0;
	for(a=u->accounts; a!=NULL; a=a->next)
		if(isequal(a->nick,d->nick) && (a->flags & ACCT_VIADCC))
			count++;


	/* Nick is turned off only when it's the last one via DCC */
	if(!count)
	{
		n = User_nick(u,d->nick);
		if(n != NULL)
			n->flags &= ~NICK_VIADCC;
	}
}

void User_carryoverallactivitytimes()
/*
 * Increment all user's activity times from yesterday, and reset today's
 * to 0.  (This gets called at midnight every night)
 */
{
	userrec	*u;
	time_t	now = time(NULL);
	int	day_today = today();
	int	day_yest = yesterday();

	for(u=User->userlist; u!=NULL; u=u->next)
	{
		if(u->registered)
		{
			if(u->first_activity != 0L)
			{
				u->time_seen[day_yest] += (now - u->first_activity);
				u->first_activity = now;
			}
			u->time_seen[day_today] = 0L;

			u->public_chatter[day_today] = 0L;
			u->kicks[day_today] = 0;
			u->bans[day_today] = 0;
		}
	}
}

void User_resetallactivitytimes()
/*
 * Reset user's activity times for today.  This gets called when the bot
 * is started on a day that differs from the last day it was running.
 */
{
	userrec *u;
	int	day_today = today();

	for(u=User->userlist; u!=NULL; u=u->next)
	{
		if(u->registered)
		{
			u->time_seen[day_today] = 0L;
			u->public_chatter[day_today] = 0L;
			u->kicks[day_today] = 0;
			u->bans[day_today] = 0;
		}
	}
}

void User_signoffallnicks()
/*
 * Update times for all nicks and mark OFF IRC.  This gets called when
 * we leave IRC.
 */
{
	userrec	*u;
	nickrec *n;
	acctrec	*a;
	time_t	now = time(NULL);
	int	day_today = today();

	for(u=User->userlist; u!=NULL; u=u->next)
	{
		if(u->registered)
		{
			if(u->first_activity)
			{
				u->time_seen[day_today] += (now - u->first_activity);
				u->first_activity = 0L;
			}

			for(n=u->nicks; n!=NULL; n=n->next)
			{

				/* someday we'll watch signon/signoffs */
/*
				if(n->flags & NICK_ONIRC)
					Log_write(LOG_MAIN,"*","%s signed off.",n->nick);
*/
				n->flags &= ~NICK_ONIRC;
				n->joined = 0L;
			}

			for(a=u->accounts; a!=NULL; a=a->next)
				a->flags &= ~ACCT_ONIRC;
		}
	}
}

void User_makeweekdates(char *week)
/*
 * 'week' becomes: " Mo Tu We Th(Fr)Sa Su " if today is Friday.
 * Note the spaces.
 */
{
	int	i;
	time_t	now;
	struct tm *btime;
	char	*week_days_short[] = { "Su","Mo","Tu","We","Th","Fr","Sa" };

	now = time(NULL);
	btime = localtime(&now);
	strcpy(week," ");
	for(i=0; i<7; i++)
		sprintf(week,"%s%s ",week,week_days_short[i]);

	if(btime->tm_wday >= 0 && btime->tm_wday <= 6 && strlen(week) >= 22)
	{
		week[btime->tm_wday * 3] = '(';
		week[(btime->tm_wday + 1) * 3] = ')';
	}
}

void User_makeweeksusage(char *week, userrec *u)
/*
 * 'week' becomes: " xx xx xx xx xx xx xx "
 * Again, note the spaces.
 */
{
	int i;
	time_t	t = 0L, now;
	struct tm *btime;

	now = time(NULL);
	btime = localtime(&now);
	strcpy(week," ");
	for(i=0; i<7; i++)
	{
		t = u->time_seen[i];

		/* If the day in question is today, update with today's time */
		if(btime->tm_wday == i)
		{
			if(u->first_activity)
				t += (time(NULL) - u->first_activity);
		}

		if(!t)
			sprintf(week,"%s-- ",week);
		else if(t < 360)
			sprintf(week,"%s.1 ",week);
		else if(t < 3600)
			sprintf(week,"%s.%1lu ",week,t / 360);
		else
			sprintf(week,"%s%2lu ",week,t / 3600);
	}
}

void User_makeircfreq(char *ircfreq, userrec *u)
{
	u_long tot_time = 0;

	/* Get time for the whole week */
	tot_time = User_timeperweek(u);

	ircfreq[0] = 0;
	if(tot_time < IRC_RARELY)
		strcpy(ircfreq,"NEVER");
	else if(tot_time < IRC_LOW)
		strcpy(ircfreq,"RARELY");
	else if(tot_time < IRC_MEDIUM)
		strcpy(ircfreq,"LOW");
	else if(tot_time < IRC_HIGH)
		strcpy(ircfreq,"MEDIUM");
	else if(tot_time < IRC_NO_LIFE)
		strcpy(ircfreq,"HIGH");
	else
		strcpy(ircfreq,"NO LIFE");
}

u_long User_timeperweek(userrec *u)
{
	int	i;
	u_long	tot_time = 0;

	/* Add up all weekly activity times */
	for(i=0; i<7; i++)
		tot_time += u->time_seen[i];

	/* Update activity time if they're on right now */
	if(u->first_activity)
		tot_time += (time(NULL) - u->first_activity);

	return tot_time;
}

int User_informuserofnote(userrec *u, acctrec *a_from)
{
	nickrec	*n;
	int	found = 0, dont_bother = 0;

	/* If the user has their notes forwarded, don't bother them. */
	if(u->forward && u->forward[0])
		dont_bother = 1;

	/* Check all nicks, see if any are on IRC */
	for(n=u->nicks; n!=NULL; n=n->next)
	{
		if((n->flags & NICK_ONIRC) && !isequal(a_from->nick,n->nick))
		{
			if(!dont_bother)
				sendnotice(n->nick,"(!) Note from %s %s.  ('rn n' to read)",a_from->nick,a_from->uh);
			found = 1;
		}
	}

	return found;
}

void User_notifyofnotesonsignon(userrec *u, nickrec *n)
{
	int	new;
	/* check to see if they have any New notes */

	context;
	new = Note_numnew(u);
	context;
	if(new)
		sendnotice(n->nick,"You have %d new Note%s. (%d total) - ('rn n' to display)",new,new==1?"":"s",u->num_notes);
	context;
}


gretrec *User_greetbyindex(userrec *u, int index)
{
	gretrec	*g;
	int	i = 0;

	if(index > u->num_greets)
		return NULL;

	for(g=u->greets; g!=NULL; g=g->next)
	{
		i++;
		if(i == index)
			return g;
	}

	return NULL;
}

int User_deletemarkedgreets(userrec *u)
{
	int	tot = 0;
	gretrec	*g, *g_next;

	g = u->greets;
	while(g != NULL)
	{
		g_next = g->next;
		if(g->flags & GREET_DELETEME)
		{
			User_deletegreet(u,g);
			tot++;
		}
		g = g_next;
	}

	return tot;
}

void User_pickgreet(userrec *u, char *greet)
{
	gretrec	*g;
	int	num_unhit, greetnum, i = 0;

	/* double check */
	if(u->greets == NULL)
		return;

	greet[0] = 0;
	num_unhit = User_num_unhit_greets(u);
	if(num_unhit == 0)
	{
		User_unset_all_greet_hits(u);
		num_unhit = User_num_unhit_greets(u);
	}

	if(num_unhit == 1)
		greetnum = 1;
	else
		greetnum = pdf_uniform(1,num_unhit);

	for(g=u->greets; g!=NULL; g=g->next)
	{
		if(!(g->flags & GREET_HIT))
		{
			i++;
			if(i == greetnum)
			{
				g->flags |= GREET_HIT;
				break;
			}
		}
	}

	if(g == NULL)
		return;

	strcpy(greet,g->text);
}

int User_num_unhit_greets(userrec *u)
{
	gretrec	*g;
	int	tot = 0;

	for(g=u->greets; g!=NULL; g=g->next)
		if(!(g->flags & GREET_HIT))
			tot++;

	return tot;
}

int User_num_hit_greets(userrec *u)
{
	gretrec	*g;
	int	tot = 0;

	for(g=u->greets; g!=NULL; g=g->next)
		if(g->flags & GREET_HIT)
			tot++;

	return tot;
}

void User_unset_all_greet_hits(userrec *u)
{
	gretrec	*g;

	for(g=u->greets; g!=NULL; g=g->next)
		g->flags &= ~GREET_HIT;
}

void User_checkforsignon(char *nick)
/*
 * Check all channels to see if we already know this user so we can trigger
 * a quick signon.
 */
{
	chanrec	*chan;
	membrec	*m = NULL;
	nickrec	*n;
	userrec	*u;
	acctrec	*a;

	context;
	if(Channel_ismemberofanychan(nick))
	{
	context;
		for(chan=Channel->channels; chan!=NULL; chan=chan->next)
		{
			m = Channel_member(chan,nick);
			if(m != NULL)
				break;
		}

	context;
		if(m == NULL)
			return;

	context;
		User_gotpossiblesignon(m->user,m->account);
	}

	u = User_user(nick);
	/*
	 * ok now check accounts in this user for any which are registered
	 * and marked VIADCC
	 */
	for(a=u->accounts; a!=NULL; a=a->next)
	{
		if(isequal(a->nick,nick) && a->registered && (a->flags & ACCT_VIADCC))
		{
			n = User_nick(u,nick);
			n->flags |= NICK_VIADCC;
			break;
		}
	}
}

acctrec	*User_lastseentype(userrec *u, char type)
{
	acctrec	*a, *remember_a = NULL;
	time_t	last = 0L;

	for(a=u->accounts; a!=NULL; a=a->next)
	{
		if(a->registered && (a->flags & type) && a->last_seen > last)
		{
			last = a->last_seen;
			remember_a = a;
		}
	}

	return remember_a;
}

acctrec	*User_lastseen(userrec *u)
{
	acctrec	*a, *remember_a = NULL;
	time_t	last = 0L;

	for(a=u->accounts; a!=NULL; a=a->next)
	{
		if(a->registered && a->last_seen > last)
		{
			last = a->last_seen;
			remember_a = a;
		}
	}

	return remember_a;
}

acctrec *User_lastseenunregisterednick(userrec *u, char *nick)
{
	acctrec	*a, *remember_a = NULL;
	time_t	last = 0L;

	for(a=u->accounts; a!=NULL; a=a->next)
	{
		if(isequal(a->nick,nick) && (a->last_seen > last))
		{
			last = a->last_seen;
			remember_a = a;
		}
	}

	return remember_a;
}

userrec *User_userbyindex(int index)
{
	userrec	*u;

	for(u=User->userlist; u!=NULL; u=u->next)
		if(u->registered == index)
			return u;

	return NULL;
}

userrec	*User_userbyorder(int ordernum)
{
	userrec	*u;

	for(u=User->userlist; u!=NULL; u=u->next)
		if(u->order == ordernum)
			return u;

	return NULL;
}

int User_orderbyreg()
{
	userrec	*u, *u_save = NULL;
	int	h, highest = User_highestregnumber(), counter = 0, done = 0;

	/* We need to clear all ordering before we start */
	User_clearorder();

	/* Continue until all entries have an order number */
	while(!done)
	{
		done = 1;
		h = highest;
		for(u=User->userlist; u!=NULL; u=u->next)
		{
			if(u->registered && !u->order && u->registered <= h)
			{
				done = 0;
				h = u->registered;
				u_save = u;
			}
		}

		/* Not done means we got u_save and now we need to order it */
		if(!done)
		{
			counter++;
			u_save->order = counter;
		}
	}

	return counter;
}

int User_orderbyreg_plusflags(u_long flags)
{
	userrec	*u, *u_save = NULL;
	int	h, highest = User_highestregnumber(), counter = 0, done = 0;

	/* We need to clear all ordering before we start */
	User_clearorder();

	/* Continue until all entries have an order number */
	while(!done)
	{
		done = 1;
		h = highest;
		for(u=User->userlist; u!=NULL; u=u->next)
		{
			if(u->registered && (u->flags & flags) && !u->order && u->registered <= h)
			{
				done = 0;
				h = u->registered;
				u_save = u;
			}
		}

		/* Not done means we got u_save and now we need to order it */
		if(!done)
		{
			counter++;
			u_save->order = counter;
		}
	}

	return counter;
}

int User_orderbyacctname()
{
	userrec	*u, *u_save = NULL;
	int	counter = 0, done = 0;
	char	h[NICKLEN], highest[NICKLEN];

	/* Put the highest ascii character in 'highest'. */
	sprintf(highest,"%c",HIGHESTASCII);

	/* We need to clear all ordering before we start */
	User_clearorder();

	/* Continue until all entries have an order number */
	while(!done)
	{
		done = 1;
		strcpy(h,highest);
		for(u=User->userlist; u!=NULL; u=u->next)
		{
			if(u->registered && !u->order && strcasecmp(u->acctname,h) <= 0)
			{
				done = 0;
				strcpy(h,u->acctname);
				u_save = u;
			}
		}

		/* Not done means we got u_save and now we need to order it */
		if(!done)
		{
			counter++;
			u_save->order = counter;
		}
	}

	return counter;
}

int User_orderbyacctname_plusflags(u_long flags)
{
	userrec	*u, *u_save = NULL;
	int	counter = 0, done = 0;
	char	h[NICKLEN], highest[NICKLEN];

	/* Put the highest ascii character in 'highest'. */
	sprintf(highest,"%c",HIGHESTASCII);

	/* We need to clear all ordering before we start */
	User_clearorder();

	/* Continue until all entries have an order number */
	while(!done)
	{
		done = 1;
		strcpy(h,highest);
		for(u=User->userlist; u!=NULL; u=u->next)
		{
			if(u->registered && (u->flags & flags) && !u->order && strcasecmp(u->acctname,h) <= 0)
			{
				done = 0;
				strcpy(h,u->acctname);
				u_save = u;
			}
		}

		/* Not done means we got u_save and now we need to order it */
		if(!done)
		{
			counter++;
			u_save->order = counter;
		}
	}

	return counter;
}

void User_clearorder()
{
	userrec	*u;

	for(u=User->userlist; u!=NULL; u=u->next)
		u->order = 0;
}

int User_orderaccountsbyfirstseen(userrec *u)
{
	acctrec	*a, *a_save = NULL;
	time_t	l, lowest = 0L, counter = 0, done = 0;

	/* We need to clear all ordering before we start */
	User_clearaccountorder(u);

	/* Continue until all entries have an order number */
	while(!done)
	{
		done = 1;
		l = lowest;
		for(a=u->accounts; a!=NULL; a=a->next)
		{
			if(!a->order && a->first_seen >= l)
			{
				done = 0;
				l = a->first_seen;
				a_save = a;
			}
		}

		/* Not done means we got u_save and now we need to order it */
		if(!done)
		{
			counter++;
			a_save->order = counter;
		}
	}

	return counter;
}

void User_clearaccountorder(userrec *u)
{
	acctrec	*a;

	for(a=u->accounts; a!=NULL; a=a->next)
		a->order = 0;
}

acctrec *User_accountbyorder(userrec *u, int ordernum)
{
	acctrec	*a;

	for(a=u->accounts; a!=NULL; a=a->next)
		if(a->order == ordernum)
			return a;

	return NULL;
}

int User_timeoutaccounts()
{
	userrec	*u;
	acctrec	*a, *a_next;
	time_t	now = time(NULL), timeout = (Grufti->timeout_accounts+1) * 86400;
	int	num_accounts = 0;

	/* Track through every account and discard those which have not
	 * been seen in x days
	 */
	for(u=User->userlist; u!=NULL; u=u->next)
	{
		a = u->accounts;
		while(a != NULL)
		{
			a_next = a->next;
			if((now - a->last_seen) > timeout)
			{
				User_deleteaccount(u,a);
				num_accounts++;
			}
			a = a_next;
		}
	}

	/*
	 * We shouldn't have to do this.  If the account hasn't been seen
	 * in 20 or so days, we probably don't have any active records pointing
	 * to it.  Oh well, be on the safe side. (timeout might be 2 days)
	 */
	User_resetallpointers();

	return num_accounts;
}

int User_timeoutusers()
{
	userrec	*u, *u_next;
	time_t	now = time(NULL), timeout = (Grufti->timeout_users+1) * 86400;
	int	num_users = 0;

	u = User->userlist;
	while(u != NULL)
	{
		u_next = u->next;

		/*
		 * If no accounts exist, and user is not owner or master,
		 * discard the user
		 */
		if(!(u->flags & (USER_OWNER|USER_MASTER)) && u->registered && (now - u->last_seen) > timeout)
		{
			User_deleteuser(u);
			num_users++;
		}

		u = u_next;
	}

	/*
	 * We shouldn't have to do this.  If the account hasn't been seen
	 * in 20 or so days, we probably don't have any active records pointing
	 * to it.  Oh well, be on the safe side. (timeout might be 2 days)
	 */
	User_resetallpointers();

	return num_users;
}

int User_ordernumberforuser(userrec *u)
{
	User_orderbyacctname();
	return u->order;
}
	
void User_resetuserstatistics()
{
	userrec	*u;

	for(u=User->userlist; u!=NULL; u=u->next)
	{
		u->via_web = 0;
		u->bot_commands = 0;
	}
}

void User_sendnightlyreport()
{
	int	seen_people = 0, cmds_people = 0, cmds = 0, via_web = 0;
	int	avg_counter = 0, num_hostmasks = 0, num_uhostmasks = 0;
	int	num_notes = 0, num_note_users = 0, num_notes_sent = 0;
	int	joins = 0, joins_reg = 0, chatter = 0, i;
	int	resp_counter = 0, tot, usage_people = 0;
	char	datestr[TIMELONGLEN];
	char	date[TIMELONGLEN], online_time[TIMELONGLEN], memory_used[25];
	char	when_cleaned[TIMELONGLEN], out[200], report[6000];
	char	b_in[25], b_out[25], avg_lag[TIMESHORTLEN];
	generec	*g;
	userrec	*u;
	noterec	*note;
	servrec	*serv;
	chanrec	*chan;
	resprec	*r;
	rtype	*rt;
	time_t	t, now = time(NULL), top_usage = 0L, top_chatter = 0L;
	int	serv_connected = 0;
	float	avg_note_age = 0, avg_user_age = 0;
	float	avg_last_seen = 0, avg_usage = 0, avg_usage_partial;
	float	avg_last_login = 0;
	int	login_people = 0;

	/* List all the statistics for the day */
	report[0] = 0;
	timet_to_date_short(now-60,datestr);
	split(date,datestr);
	sprintf(report,"%sSummary for %s\n",report,date);
	timet_to_ago_long(Grufti->online_since,online_time);
	comma_number(memory_used,net_sizeof() + misc_sizeof() + gruftiaux_sizeof() + blowfish_sizeof() + main_sizeof() + fhelp_sizeof() + User_sizeof() + URL_sizeof() + Server_sizeof() + Response_sizeof() + Notify_sizeof() + Log_sizeof() + Location_sizeof() + Grufti_sizeof() + DCC_sizeof() + command_sizeof() + Codes_sizeof() + Channel_sizeof());
	sprintf(report,"%sOnline: %s      Memory: %s bytes\n",report,online_time,memory_used);
	sprintf(report,"%s\n",report);
	if(Grufti->cleanup_time == 0L)
		sprintf(report,"%s  No cleanup information is available...\n",report);
	else
	{
		timet_to_date_short(Grufti->cleanup_time,when_cleaned);
		sprintf(report,"%s  Auto Cleanup Summary (%s)\n",report,when_cleaned);
		sprintf(report,"%s    %d user%s deleted (timeout: %d days)\n",report,Grufti->users_deleted,Grufti->users_deleted==1 ? " was":"s were",Grufti->timeout_users);
		sprintf(report,"%s    %d hostmask%s deleted (timeout: %d days)\n",report,Grufti->accts_deleted,Grufti->accts_deleted==1 ? " was":"s were",Grufti->timeout_accounts);
		sprintf(report,"%s    %d note%s deleted (timeout: %d days)\n",report,Grufti->notes_deleted,Grufti->notes_deleted==1 ? " was":"s were",Grufti->timeout_notes);
	}
	sprintf(report,"%s\n",report);

	context;

	/* Make sure we get YESTERDAY's time */
	t = time_today_began(time(NULL)-60);
	for(u=User->userlist; u!=NULL; u=u->next)
	{
		context;
		if(u->registered)
			num_hostmasks += u->num_accounts;
		else
			num_uhostmasks += u->num_accounts;

		context;
		if(!u->registered)
			continue;

		context;
		if(u->bot_commands)
		{
			cmds_people++;
			cmds += u->bot_commands;
		}

		context;
		if(u->via_web)
			via_web++;

		context;
		if(u->last_seen >= t)
			seen_people++;

		if(u->last_login >= t)
			login_people++;

		avg_usage_partial = User_timeperweek(u);

		if(avg_usage_partial > 0)
		{
			avg_usage += ((float)(avg_usage_partial) / (3600.0 * 7.0));
			usage_people++;
		}

		context;
		avg_last_seen += ((float)(now - u->last_seen) / 86400.0);
		avg_last_login += ((float)(now - u->last_login) / 86400.0);
		avg_user_age += ((float)(now - u->time_registered) / 86400.0);

		context;

		context;
		if(u->num_notes)
		{
			for(note=u->notes; note!=NULL; note=note->next)
			{
				avg_note_age += ((float)(now - note->sent) / 86400.0);
				if(note->sent >= t)
					num_notes_sent++;
			}

			num_notes += u->num_notes;
			num_note_users++;
		}

		context;
		avg_counter ++;
	}

	if(num_notes)
		avg_note_age /= num_notes;
	else
		avg_note_age = 0;

	if(avg_counter)
	{
		avg_last_seen /= avg_counter;
		avg_last_login /= avg_counter;
		avg_user_age /= avg_counter;
	}
	else
	{
		avg_last_seen = 0;
		avg_last_login = 0;
		avg_user_age = 0;
	}

	if(usage_people)
		avg_usage /= usage_people;
	else
		avg_usage = 0;

	sprintf(report,"%s  Overall User Statistics\n",report);
	sprintf(report,"%s    %d user%s registered (%d hostmask%s)\n",report,User->num_users - 1,User->num_users-1 == 1?" is":"s are",num_hostmasks,num_hostmasks==1?"":"s");
	sprintf(report,"%s    %d unregistered hostmasks are being tracked\n",report,num_uhostmasks);
	if(num_note_users)
		sprintf(report,"%s    %d Note%s being stored by %d user%s (%2.1f notes/user)\n",report,num_notes,num_notes==1?" is":"s are",num_note_users,num_note_users==1?"":"s",(float)(num_notes)/(float)(num_note_users));
	else
		sprintf(report,"%s    %d Note%s being stored by %d user%s\n",report,num_notes,num_notes==1?" is":"s are",num_note_users,num_note_users==1?"":"s");
	sprintf(report,"%s    %d Note%s been sent today, %d %s forwarded\n",report,num_notes_sent,num_notes_sent==1?" has":"s have",Grufti->notes_forwarded,Grufti->notes_forwarded?"was":"were");
	sprintf(report,"%s    Average %s note is %2.1f days old\n",report,Grufti->botnick,avg_note_age);
	sprintf(report,"%s    Average %s user account is %2.1f days old\n",report,Grufti->botnick,avg_user_age);
	sprintf(report,"%s    Average %s user was seen %2.1f days ago\n",report,Grufti->botnick,avg_last_seen);
	sprintf(report,"%s    Average %s user last logged in %2.1f days ago\n",report,Grufti->botnick,avg_last_login);
	sprintf(report,"%s    Average time a %s user is on IRC is %2.1f hrs/day\n",report,Grufti->botnick,avg_usage);
	sprintf(report,"%s\n",report);

	sprintf(report,"%s  Of the %d registered user%s:\n",report,User->num_users - 1, (User->num_users - 1) == 1? "":"s");
	sprintf(report,"%s    %d %s queried the bot via IRC today (%d command%s)\n",report,cmds_people,cmds_people==1?"has":"have",cmds,cmds==1?"":"s");
	sprintf(report,"%s    %d %s logged in from the web or via telnet\n",report,via_web,via_web==1?"has":"have");
	sprintf(report,"%s    %d %s logged in to their account in general\n",report,login_people,login_people==1?"has":"have");
	sprintf(report,"%s    %d %s been seen online\n",report,seen_people,seen_people==1?"has":"have");
	sprintf(report,"%s\n",report);

	/* Determine top users based on usage */
	for(u=User->userlist; u!=NULL; u=u->next)
	{
		if(!u->registered || (u->flags & USER_BOT))
			continue;

		top_usage = User_timeperweek(u);

		Notify_addtogenericlist(u->acctname,top_usage);
	}

	tot = Notify_orderbylogin();
	if(tot)
	{
		sprintf(report,"%s  Top users based on IRC usage this week (hrs/day):\n",report);
		strcpy(out,"    ");
		for(i=1; i<=4; i++)
		{
			g = Notify_genericbyorder(i);
			if(g == NULL)
				continue;

			sprintf(out,"%s%s %2.1f, ",out,g->display,(float)(g->login) / (3600.0 * 7.0));
		}
		out[strlen(out) - 2] = 0;

		if(strlen(out) > 4)
			sprintf(report,"%s%s\n",report,out);

		strcpy(out,"    ");
		for(i=5; i<=8; i++)
		{
			g = Notify_genericbyorder(i);
			if(g == NULL)
				continue;

			sprintf(out,"%s%s %2.1f, ",out,g->display,(float)(g->login) / (3600.0 * 7.0));
		}
		out[strlen(out) - 2] = 0;

		if(strlen(out) > 4)
			sprintf(report,"%s%s\n",report,out);
	}

	Notify_freeallgenerics();

	sprintf(report,"%s\n",report);
	/* Determine top users based on public chatter */
	for(u=User->userlist; u!=NULL; u=u->next)
	{
		if(!u->registered || (u->flags & USER_BOT))
			continue;

		top_chatter = 0;
		for(i=0; i<7; i++)
			top_chatter += u->public_chatter[i];

		Notify_addtogenericlist(u->acctname,top_chatter);
	}

	tot = Notify_orderbylogin();
	if(tot)
	{
		sprintf(report,"%s  Top users based on public chatter this week:\n",report);
		strcpy(out,"    ");
		for(i=1; i<=4; i++)
		{
			g = Notify_genericbyorder(i);
			if(g == NULL)
				continue;

			sprintf(out,"%s%s %luk, ",out,g->display,g->login/(1024*7));
		}
		out[strlen(out) - 2] = 0;
		if(strlen(out) > 4)
			sprintf(report,"%s%s\n",report,out);

		strcpy(out,"    ");
		for(i=5; i<=8; i++)
		{
			g = Notify_genericbyorder(i);
			if(g == NULL)
				continue;

			sprintf(out,"%s%s %luk, ",out,g->display,g->login/(1024*7));
		}
		out[strlen(out) - 2] = 0;
		if(strlen(out) > 4)
			sprintf(report,"%s%s\n",report,out);
	}

	Notify_freeallgenerics();
	sprintf(report,"%s\n",report);

	sprintf(report,"%s  Response Statistics\n",report);
	for(rt=Response->rtypes; rt!=NULL; rt=rt->next)
	{
		resp_counter = 0;
		for(r=rt->responses; r!=NULL; r=r->next)
		{
			if(r->last >= t)
				resp_counter++;
		}

		sprintf(report,"%s    %d of my %d %s response%s matched\n",report,resp_counter,rt->num_responses,rt->name,rt->num_responses==1?" was":"s were");
	}
	sprintf(report,"%s\n",report);

	sprintf(report,"%s  Channel Statistics\n",report);
	for(chan=Channel->channels; chan!=NULL; chan=chan->next)
	{
		joins = 0;
		joins_reg = 0;
		chatter = 0;
		for(i=0; i<24; i++)
		{
			joins += chan->joins_reg[i] + chan->joins_nonreg[i];
			joins_reg += chan->joins_reg[i];
			chatter += chan->chatter[i];
		}

		sprintf(report,"%s    %s had %d join%s (%d by reg'd user%s), %dk chatter\n",report,chan->name,joins,joins==1?"":"s",joins_reg,joins_reg==1?"":"s",chatter/1024);
	}
	sprintf(report,"%s\n",report);

	/* Display server statistics */
	for(serv=Server->serverlist; serv!=NULL; serv=serv->next)
		serv_connected += Server_timeconnected(serv);

	sprintf(report,"%s  Server Statistics (Online %2.1f hrs today)\n",report,(float)serv_connected / 3600.0);
	for(serv=Server->serverlist; serv!=NULL; serv=serv->next)
	{
		if(Server_timeconnected(serv))
		{
			bytes_to_kb_short(serv->bytes_in,b_in);
			bytes_to_kb_short(serv->bytes_out,b_out);
			time_to_elap_short(Server_avglag(serv),avg_lag);
			sprintf(report,"%s    %s - %2.1f hrs, %s in, %s out, %s lag\n",report,serv->name,(float)Server_timeconnected(serv)/3600.0,b_in,b_out,avg_lag);
		}
	}

	Note_sendnotification(report);
}

/* ASDF */
/*****************************************************************************
 *
 * user record definitions.  No changes should be made to new(), freeall(),
 * append(), delete(), or movetofront().
 *
 ****************************************************************************/

userrec *userrec_new()
{
	userrec *x;

	/* allocate memory */
	x = (userrec *)xmalloc(sizeof(userrec));

	/* initialize */
	x->next = NULL;

	userrec_init(x);

	return x;
}

void userrec_freeall(userrec **list, userrec **last)
{
	userrec *entry = *list, *v;

	while(entry != NULL)
	{
		v = entry->next;
		userrec_free(entry);
		entry = v;
	}

	*list = NULL;
	*last = NULL;
}

void userrec_append(userrec **list, userrec **last, userrec *x)
{
	userrec *lastentry = *last;

	if(*list == NULL)
		*list = x;
	else
		lastentry->next = x;

	*last = x;
	x->next = NULL;
}

void userrec_delete(userrec **list, userrec **last, userrec *x)
{
	userrec *entry, *lastchecked = NULL;

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

			userrec_free(entry);
			return;
		}

		lastchecked = entry;
		entry = entry->next;
	}
}

void userrec_movetofront(userrec **list, userrec **last, userrec *lastchecked, userrec *x)
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
/* record userrec low-level data-specific function definitions. */

void userrec_init(userrec *u)
{
	int i;

	u->acctname = NULL;
	u->nicks = NULL;
	u->n_last = NULL;
	u->num_nicks = 0;
	u->accounts = NULL;
	u->a_last = NULL;
	u->num_accounts = 0;
	u->pass = NULL;
	u->finger = NULL;
	u->email = NULL;
	u->www = NULL;
	u->forward = NULL;
	u->other1 = NULL;
	u->other2 = NULL;
	u->other3 = NULL;
	u->other4 = NULL;
	u->other5 = NULL;
	u->other6 = NULL;
	u->other7 = NULL;
	u->other8 = NULL;
	u->other9 = NULL;
	u->other10 = NULL;
	u->plan1 = NULL;
	u->plan2 = NULL;
	u->plan3 = NULL;
	u->plan4 = NULL;
	u->plan5 = NULL;
	u->plan6 = NULL;
	u->bday[0] = 0;
	u->bday[1] = 0;
	u->bday[2] = -1;
	u->greets = NULL;
	u->lastgreet = NULL;
	u->num_greets = 0;
	u->notes = NULL;
	u->lastnote = NULL;
	u->num_notes = 0;
	u->location = 0;
	u->registered = 0;
	u->time_registered = 0L;
	u->bot_commands = 0;
	u->flags = 0L;
	u->first_activity = 0L;
	u->last_seen = 0L;
	u->last_login = 0L;
	u->last_seen_onchan = 0L;
	u->seen_motd = 0L;
	for(i=0; i<7; i++)
	{
		u->time_seen[i] = 0;
		u->public_chatter[i] = 0;
		u->bans[i] = 0;
		u->kicks[i] = 0;
	}

	u->via_web = 0;
	u->signoff = NULL;
	u->history = NULL;
	u->cmd_pending_pass = NULL;
	u->cmdnick_pending_pass = NULL;
	u->cmdtime_pending_pass = 0;
	u->order = 0;
}

u_long userrec_sizeof(userrec *u)
{
	u_long	tot = 0L;
	nickrec	*n;
	acctrec	*a;
	gretrec	*g;
	noterec	*note;

	tot += sizeof(userrec);
	tot += u->acctname ? strlen(u->acctname)+1 : 0L;
	for(n=u->nicks; n!=NULL; n=n->next)
		tot += nickrec_sizeof(n);
	for(a=u->accounts; a!=NULL; a=a->next)
		tot += acctrec_sizeof(a);
	tot += u->pass ? strlen(u->pass)+1 : 0L;
	tot += u->finger ? strlen(u->finger)+1 : 0L;
	tot += u->email ? strlen(u->email)+1 : 0L;
	tot += u->www ? strlen(u->www)+1 : 0L;
	tot += u->forward ? strlen(u->forward)+1 : 0L;
	tot += u->other1 ? strlen(u->other1)+1 : 0L;
	tot += u->other2 ? strlen(u->other2)+1 : 0L;
	tot += u->other3 ? strlen(u->other3)+1 : 0L;
	tot += u->other4 ? strlen(u->other4)+1 : 0L;
	tot += u->other5 ? strlen(u->other5)+1 : 0L;
	tot += u->other6 ? strlen(u->other6)+1 : 0L;
	tot += u->other7 ? strlen(u->other7)+1 : 0L;
	tot += u->other8 ? strlen(u->other8)+1 : 0L;
	tot += u->other9 ? strlen(u->other9)+1 : 0L;
	tot += u->other10 ? strlen(u->other10)+1 : 0L;
	tot += u->plan1 ? strlen(u->plan1)+1 : 0L;
	tot += u->plan2 ? strlen(u->plan2)+1 : 0L;
	tot += u->plan3 ? strlen(u->plan3)+1 : 0L;
	tot += u->plan4 ? strlen(u->plan4)+1 : 0L;
	tot += u->plan5 ? strlen(u->plan5)+1 : 0L;
	tot += u->plan6 ? strlen(u->plan6)+1 : 0L;
	for(g=u->greets; g!=NULL; g=g->next)
		tot += gretrec_sizeof(g);
	for(note=u->notes; note!=NULL; note=note->next)
		tot += noterec_sizeof(note);
	tot += u->signoff ? strlen(u->signoff)+1 : 0L;
	tot += u->cmd_pending_pass ? strlen(u->cmd_pending_pass)+1 : 0L;
	tot += u->cmdnick_pending_pass ? strlen(u->cmdnick_pending_pass)+1 : 0L;

	return tot;
}

void userrec_free(userrec *u)
{
	/* Free the elements */
	xfree(u->acctname);
	User_freeallnicks(u);
	User_freeallaccounts(u);
	xfree(u->pass);
	xfree(u->finger);
	xfree(u->email);
	xfree(u->www);
	xfree(u->forward);
	xfree(u->other1);
	xfree(u->other2);
	xfree(u->other3);
	xfree(u->other4);
	xfree(u->other5);
	xfree(u->other6);
	xfree(u->other7);
	xfree(u->other8);
	xfree(u->other9);
	xfree(u->other10);
	xfree(u->plan1);
	xfree(u->plan2);
	xfree(u->plan3);
	xfree(u->plan4);
	xfree(u->plan5);
	xfree(u->plan6);
	User_freeallgreets(u);
	User_freeallnotes(u);
	xfree(u->signoff);
	listmgr_destroy_list(u->history);
	xfree(u->cmd_pending_pass);
	xfree(u->cmdnick_pending_pass);


	/* Free this record. */
	xfree(u);
}

void userrec_setacctname(userrec *u, char *acctname)
{
	mstrcpy(&u->acctname,acctname);
}

void userrec_setpass(userrec *u, char *pass)
{
	mstrcpy(&u->pass,pass);
}

void userrec_setfinger(userrec *u, char *finger)
{
	mstrcpy(&u->finger,finger);
}

void userrec_setemail(userrec *u, char *email)
{
	mstrcpy(&u->email,email);
}

void userrec_setwww(userrec *u, char *www)
{
	mstrcpy(&u->www,www);
}

void userrec_setforward(userrec *u, char *forward)
{
	mstrcpy(&u->forward,forward);
}

/* Used by readuserfile.  Format is 'o n <other text>', where n is line #. */
void userrec_setother(userrec *u, char *other)
{
	if(other[0] && strlen(other) > 2)
	{
		switch(other[0])
		{
			case '1' : userrec_setother1(u,other+2); break;
			case '2' : userrec_setother2(u,other+2); break;
			case '3' : userrec_setother3(u,other+2); break;
			case '4' : userrec_setother4(u,other+2); break;
			case '5' : userrec_setother5(u,other+2); break;
			case '6' : userrec_setother6(u,other+2); break;
			case '7' : userrec_setother7(u,other+2); break;
			case '8' : userrec_setother8(u,other+2); break;
			case '9' : userrec_setother9(u,other+2); break;
			case '0' : userrec_setother10(u,other+2); break;
		}
	}
}

void userrec_setother1(userrec *u, char *other1)
{
	mstrcpy(&u->other1,other1);
}

void userrec_setother2(userrec *u, char *other2)
{
	mstrcpy(&u->other2,other2);
}

void userrec_setother3(userrec *u, char *other3)
{
	mstrcpy(&u->other3,other3);
}

void userrec_setother4(userrec *u, char *other4)
{
	mstrcpy(&u->other4,other4);
}

void userrec_setother5(userrec *u, char *other5)
{
	mstrcpy(&u->other5,other5);
}

void userrec_setother6(userrec *u, char *other6)
{
	mstrcpy(&u->other6,other6);
}

void userrec_setother7(userrec *u, char *other7)
{
	mstrcpy(&u->other7,other7);
}

void userrec_setother8(userrec *u, char *other8)
{
	mstrcpy(&u->other8,other8);
}

void userrec_setother9(userrec *u, char *other9)
{
	mstrcpy(&u->other9,other9);
}

void userrec_setother10(userrec *u, char *other10)
{
	mstrcpy(&u->other10,other10);
}

/* Used by readuserfile.  Format is 'p n <plan text>', where n is line #. */
void userrec_setplan(userrec *u, char *plan)
{
	if(plan[0] && strlen(plan) > 2)
	{
		switch(plan[0])
		{
			case '1' : userrec_setplan1(u,plan+2); break;
			case '2' : userrec_setplan2(u,plan+2); break;
			case '3' : userrec_setplan3(u,plan+2); break;
			case '4' : userrec_setplan4(u,plan+2); break;
			case '5' : userrec_setplan5(u,plan+2); break;
			case '6' : userrec_setplan6(u,plan+2); break;
		}
	}
}

void userrec_setplanx(userrec *u, char *plan, int x)
{
	switch(x)
	{
		case 1 : userrec_setplan1(u,plan); break;
		case 2 : userrec_setplan2(u,plan); break;
		case 3 : userrec_setplan3(u,plan); break;
		case 4 : userrec_setplan4(u,plan); break;
		case 5 : userrec_setplan5(u,plan); break;
		case 6 : userrec_setplan6(u,plan); break;
	}
}

void userrec_setplan1(userrec *u, char *plan)
{
	mstrcpy(&u->plan1,plan);
}

void userrec_setplan2(userrec *u, char *plan)
{
	mstrcpy(&u->plan2,plan);
}

void userrec_setplan3(userrec *u, char *plan)
{
	mstrcpy(&u->plan3,plan);
}

void userrec_setplan4(userrec *u, char *plan)
{
	mstrcpy(&u->plan4,plan);
}

void userrec_setplan5(userrec *u, char *plan)
{
	mstrcpy(&u->plan5,plan);
}

void userrec_setplan6(userrec *u, char *plan)
{
	mstrcpy(&u->plan6,plan);
}

void userrec_setsignoff(userrec *u, char *signoff)
{
	mstrcpy(&u->signoff,signoff);
}

void userrec_setcmd_pending_pass(userrec *u, char *cmd)
{
	mstrcpy(&u->cmd_pending_pass,cmd);
}

void userrec_setcmdnick_pending_pass(userrec *u, char *nick)
{
	mstrcpy(&u->cmdnick_pending_pass,nick);
}

void userrec_display(userrec *u)
{
	acctrec	*a;
	nickrec	*n;
	char	flags[USERFLAGLEN], timeregd[TIMESHORTLEN];

	User_flags2str(u->flags,flags);
	timet_to_time_short(u->time_registered,timeregd);

	say("Account: \"%s\" - %d nicks, %d hostmasks",u->acctname,u->num_nicks,u->num_accounts);
	for(n=u->nicks; n!=NULL; n=n->next)
		nickrec_display(n);
	for(a=u->accounts; a!=NULL; a=a->next)
		acctrec_display(a);
	say("reg'd: %s, time_reg'd: %s, #botcmds: %d, levels: %s",u->registered?"R":"-",timeregd,u->bot_commands,flags);
}

userrec *userrec_user(userrec **list, userrec **last, char *nick)
{
	userrec *u, *lastchecked = NULL;

	for(u=*list; u!=NULL; u=u->next)
	{
		if(User_isnick(u,nick))
		{
			/* found it.  do a movetofront. */
			userrec_movetofront(list,last,lastchecked,u);

			return u;
		}
		lastchecked = u;
	}

	return NULL;
}



/*****************************************************************************
 *
 * nick record definitions.  No changes should be made to new(), freeall(),
 * append(), delete(), or movetofront().
 *
 ****************************************************************************/

nickrec *nickrec_new()
{
	nickrec *x;

	/* allocate memory */
	x = (nickrec *)xmalloc(sizeof(nickrec));

	/* initialize */
	x->next = NULL;

	nickrec_init(x);

	return x;
}

void nickrec_freeall(nickrec **list, nickrec **last)
{
	nickrec *entry = *list, *v;

	while(entry != NULL)
	{
		v = entry->next;
		nickrec_free(entry);
		entry = v;
	}

	*list = NULL;
	*last = NULL;
}

void nickrec_append(nickrec **list, nickrec **last, nickrec *x)
{
	nickrec *lastentry = *last;

	if(*list == NULL)
		*list = x;
	else
		lastentry->next = x;

	*last = x;
	x->next = NULL;
}

void nickrec_delete(nickrec **list, nickrec **last, nickrec *x)
{
	nickrec *entry, *lastchecked = NULL;

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

			nickrec_free(entry);
			return;
		}

		lastchecked = entry;
		entry = entry->next;
	}
}

void nickrec_movetofront(nickrec **list, nickrec **last, nickrec *lastchecked, nickrec *x)
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
/* record nickrec low-level data-specific function definitions. */

void nickrec_init(nickrec *n)
{
	/* initialize */
	n->nick = NULL;
	n->account = NULL;
	n->flags = 0;
	n->joined = 0L;
}

u_long nickrec_sizeof(nickrec *n)
{
	u_long	tot = 0L;

	tot += sizeof(nickrec);
	tot += n->nick ? strlen(n->nick)+1 : 0L;

	return tot;
}

void nickrec_free(nickrec *n)
{
	/* Free the element. */
	xfree(n->nick);

	/* Free this record. */
	xfree(n);
}

void nickrec_setnick(nickrec *n, char *nick)
{
	mstrcpy(&n->nick,nick);
}

void nickrec_display(nickrec *n)
{
	char flags[FLAGLONGLEN];

	flags[0] = 0;
	if(n->flags & NICK_ONIRC)
		strcat(flags,"(on irc) ");
	if(n->flags & NICK_HAS_ACCT_ENTRY)
		strcat(flags,"(has acct entry) ");
	say(" Nick: %s %s",n->nick,flags);
}

int nickrec_isnick(nickrec **list, nickrec **last, char *nick)
{
	nickrec *n, *lastchecked = NULL;

	for(n=*list; n!=NULL; n=n->next)
	{
		if(isequal(n->nick,nick))
		{
			/* found it.  do a movetofront. */
			nickrec_movetofront(list,last,lastchecked,n);

			return 1;
		}
		lastchecked = n;
	}

	return 0;
}

nickrec *nickrec_nick(nickrec **list, nickrec **last, char *nick)
{
	nickrec *n, *lastchecked = NULL;

	for(n=*list; n!=NULL; n=n->next)
	{
		if(isequal(n->nick,nick))
		{
			/* found it.  do a movetofront. */
			nickrec_movetofront(list,last,lastchecked,n);

			return n;
		}
		lastchecked = n;
	}

	return NULL;
}

void nickrec_sort(nickrec **list, nickrec **last)
{
	nickrec	*n, *n_tmp, *lastchecked, *nlist;
	int	sorted = 0;

	if(*list == NULL)
		return;

	while(!sorted)
	{
		sorted = 1;
		lastchecked = NULL;
		for(n=*list; n->next!=NULL; n=n->next)
		{
			if(strcasecmp(n->nick,n->next->nick) > 0)
			{
				sorted = 0;

				if(lastchecked == NULL)
					*list = n->next;
				else
					lastchecked->next = n->next;

				n_tmp = n->next->next;
				n->next->next = n;
				n->next = n_tmp;

				/* We want to check this record again */
				if(lastchecked == NULL)
				{
					nlist = *list;
					if(nlist->next->next != NULL)
						n = nlist->next;
					else
						n = nlist;
				}
				else
					n = lastchecked->next;
			}
			lastchecked = n;
		}
	}

	/* find last */
	for(n=*list; n->next!=NULL; n=n->next)
		;

	*last = n;
}



/*****************************************************************************
 *
 * account record definitions.  No changes should be made to new(),
 * freeall(), append(), delete(), or movetofront().
 *
 ****************************************************************************/

acctrec *acctrec_new()
{
	acctrec *x;

	/* allocate memory */
	x = (acctrec *)xmalloc(sizeof(acctrec));

	/* initialize */
	x->next = NULL;

	acctrec_init(x);

	return x;
}

void acctrec_freeall(acctrec **list, acctrec **last)
{
	acctrec *entry = *list, *v;

	while(entry != NULL)
	{
		v = entry->next;
		acctrec_free(entry);
		entry = v;
	}

	*list = NULL;
	*last = NULL;
}

void acctrec_append(acctrec **list, acctrec **last, acctrec *x)
{
	acctrec *lastentry = *last;

	if(*list == NULL)
		*list = x;
	else
		lastentry->next = x;

	*last = x;
	x->next = NULL;
}

void acctrec_delete(acctrec **list, acctrec **last, acctrec *x)
{
	acctrec *entry, *lastchecked = NULL;

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

			acctrec_free(entry);
			return;
		}

		lastchecked = entry;
		entry = entry->next;
	}
}

void acctrec_unlink(acctrec **list, acctrec **last, acctrec *x)
{
	acctrec *entry, *lastchecked = NULL;

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

			/* only difference between 'delete' is we don't free. */
			entry->next = NULL;
			return;
		}

		lastchecked = entry;
		entry = entry->next;
	}
}

void acctrec_movetofront(acctrec **list, acctrec **last, acctrec *lastchecked, acctrec *x)
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
/* record acctrec low-level data-specific function definitions. */

void acctrec_init(acctrec *a)
{
	/* initialize */
	a->nick = NULL;
	a->uh = NULL;
	a->registered = FALSE;
	a->first_seen = 0L;
	a->last_seen = 0L;
	a->flags = 0;
	a->order = 0;
}

u_long acctrec_sizeof(acctrec *a)
{
	u_long	tot = 0L;

	tot += sizeof(acctrec);
	tot += a->nick ? strlen(a->nick)+1 : 0L;
	tot += a->uh ? strlen(a->uh)+1 : 0L;

	return tot;
}

void acctrec_free(acctrec *a)
{
	/* Free the elements. */
	xfree(a->nick);
	xfree(a->uh);

	/* Free this record. */
	xfree(a);
}

void acctrec_setnick(acctrec *a, char *nick)
{
	mstrcpy(&a->nick,nick);
}

void acctrec_setuh(acctrec *a, char *uh)
{
	mstrcpy(&a->uh,uh);
}

void acctrec_display(acctrec *a)
{
	char last_seen[TIMESHORTLEN];
	char first_seen[TIMESHORTLEN];

	timet_to_time_short(a->first_seen,first_seen);
	timet_to_ago_short(a->last_seen,last_seen);
	say("  %-9s %-20s %s f:%s l:%-4s",a->nick,a->uh,a->registered?"R":"-",first_seen,last_seen);
}

acctrec *acctrec_account(acctrec **list, acctrec **last, char *nick, char *uh)
{
	acctrec *a, *lastchecked = NULL;

	for(a=*list; a!=NULL; a=a->next)
	{
		if(isequal(a->nick,nick) && isequal(a->uh,uh))
		{
			/* found it.  do a movetofront. */
			acctrec_movetofront(list,last,lastchecked,a);

			return a;
		}
		lastchecked = a;
	}

	return NULL;
}

/*****************************************************************************
 *
 * greet record definitions.  No changes should be made to new(),
 * freeall(), append(), delete(), or movetofront().
 *
 ****************************************************************************/

gretrec *gretrec_new()
{
	gretrec *x;

	/* allocate memory */
	x = (gretrec *)xmalloc(sizeof(gretrec));

	/* initialize */
	x->next = NULL;

	gretrec_init(x);

	return x;
}

void gretrec_freeall(gretrec **list, gretrec **last)
{
	gretrec *entry = *list, *v;

	while(entry != NULL)
	{
		v = entry->next;
		gretrec_free(entry);
		entry = v;
	}

	*list = NULL;
	*last = NULL;
}

void gretrec_append(gretrec **list, gretrec **last, gretrec *x)
{
	gretrec *lastentry = *last;

	if(*list == NULL)
		*list = x;
	else
		lastentry->next = x;

	*last = x;
	x->next = NULL;
}

void gretrec_delete(gretrec **list, gretrec **last, gretrec *x)
{
	gretrec *entry, *lastchecked = NULL;

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

			gretrec_free(entry);
			return;
		}

		lastchecked = entry;
		entry = entry->next;
	}
}


/****************************************************************************/
/* record gretrec low-level data-specific function definitions. */

void gretrec_init(gretrec *g)
{
	/* initialize */
	g->text = NULL;
	g->flags = 0;
}

u_long gretrec_sizeof(gretrec *g)
{
	u_long	tot = 0L;

	tot += sizeof(gretrec);
	tot += g->text ? strlen(g->text)+1 : 0L;

	return tot;
}

void gretrec_free(gretrec *g)
{
	/* Free the elements. */
	xfree(g->text);

	/* Free this record. */
	xfree(g);
}

void gretrec_settext(gretrec *g, char *text)
{
	mstrcpy(&g->text,text);
}
