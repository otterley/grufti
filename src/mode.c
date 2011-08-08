/******************************************************************************
 * Steve Rider - stever@area23.org
 * Grufti - an interactive, service-oriented bot for IRC 
 *
 * mode.c - Perform mode operations
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
#include "server.h"
#include "misc.h"
#include "user.h"
#include "channel.h"
#include "dcc.h"
#include "log.h"
#include "banlist.h"
#include "mode.h"


/*
 * Like got.c, we use the server buffers.
 */

void mode_gotmode()
/*
 * MODE
 * from: <from_nuh | from_server>
 *  msg: <to/chame> <mode>
 */
{
	/* Either a user (+-iwso) or channel (+-opsitnmlbvk) mode. */
	if(Server->msg[0] != '#' && Server->msg[0] != '&')
		mode_gotusermode();
	else
		mode_gotchanmode();
}

void mode_gotusermode()
/*
 * MODE (user)
 * from: <from_nuh | from_server>
 *  msg: <to> <mode>
 */
{
	split(Server->to,Server->msg);
	strcpy(Server->mode,Server->msg);

	/* It should be our nick */
	if(!isequal(Server->to,Server->currentnick))
		return;

	/* +r is i-lined */
	if(Server->mode[0] == '+' && (strchr(Server->mode,'r')) != NULL)
	{
		Log_write(LOG_MAIN,"*","%s has me i-lined.  (jumping)",Server_name());
		sprintf(Server->sbuf,"%s has me i-lined.",Server_name());
		Server_setreason(Server->sbuf);
		Server_quit("i-line this.");
	}
}

void mode_gotchanmode()
/*
 * MODE (chan)
 * from: <from_nuh | from_server>
 *  msg: <chname> <mode> [<limit> | <user> | <key>]
 */
{
	chanrec *chan;
	membrec *m = NULL;

	split(Server->chname,Server->msg);
	split(Server->mode,Server->msg);

	chan = Channel_channel(Server->chname);
	if(chan == NULL)
	{
		Log_write(LOG_ERROR,"*","Oops.  What am I doing on %s?",Server->chname);
		Server_write(PRIORITY_HIGH,"PART :%s",Server->to);
		return;
	}

	if(!Channel_verifychanactive(chan))
		return;

	/* Mode change by server? */
	if(!strchr(Server->from,'@'))
	{
		/* Server made mode change. */
		Log_write(LOG_PUBLIC,chan->name,"%s: mode change '%s %s' by %s",Server->chname,Server->mode,Server->msg,Server->from);

		mode_parsemode(chan,"server",Server->from,Server->mode,Server->msg,1);
	}
	else
	{
		m = Channel_member(chan,Server->from_n);
		if(m == NULL)
			m = Channel_addnewmember(chan,Server->from_n,Server->from_uh);
	
		/* log last time seen */
		m->last = time(NULL);
		m->account->last_seen = time(NULL);

		/* Parse the mode line */
		Log_write(LOG_PUBLIC,chan->name,"%s: mode change '%s %s' by %s",Server->chname,Server->mode,Server->msg,Server->from_n);

		mode_parsemode(chan,m->nick,m->uh,Server->mode,Server->msg,0);
	}

}

void mode_parsemode(chanrec *chan, char *nick, char *uh, char *mode, char *args, int server)
{
	char *p, arg[UHOSTLEN];
	int addmode = 0;

	for(p=mode; *p; p++)
	{
		switch(*p)
		{
		    case '+' :	/* mode + */
			addmode = 1;
			break;

		    case '-' :	/* mode - */
			addmode = 0;
			break;

		    case 'p' :	/* private channel */
			mode_changemode(chan,addmode,CHANMODE_PRIVATE);
			if(chan->mode_keep && strchr(chan->mode_keep,'p'))
				mode_reversemode(addmode,chan,nick,'p',server);
			break;

		    case 's' :	/* secret channel */
			mode_changemode(chan,addmode,CHANMODE_SECRET);
			if(chan->mode_keep && strchr(chan->mode_keep,'s'))
				mode_reversemode(addmode,chan,nick,'s',server);
			break;

		    case 'i' :	/* invite-only channel */
			mode_changemode(chan,addmode,CHANMODE_INVITEONLY);
			if(chan->mode_keep && strchr(chan->mode_keep,'i'))
				mode_reversemode(addmode,chan,nick,'i',server);
			break;

		    case 't' :	/* topic settable by chanops only */
			mode_changemode(chan,addmode,CHANMODE_TOPIC);
			if(chan->mode_keep && strchr(chan->mode_keep,'t'))
				mode_reversemode(addmode,chan,nick,'t',server);
			break;

		    case 'n' :	/* no messages to channel from outside */
			mode_changemode(chan,addmode,CHANMODE_NOMSG);
			if(chan->mode_keep && strchr(chan->mode_keep,'n'))
				mode_reversemode(addmode,chan,nick,'n',server);
			break;

		    case 'm' :	/* moderated channel */
			mode_changemode(chan,addmode,CHANMODE_MODERATED);
			if(chan->mode_keep && strchr(chan->mode_keep,'m'))
				mode_reversemode(addmode,chan,nick,'m',server);
			break;

		    case 'a' :	/* anonymous channel */
			mode_changemode(chan,addmode,CHANMODE_ANONYMOUS);
			if(chan->mode_keep && strchr(chan->mode_keep,'a'))
				mode_reversemode(addmode,chan,nick,'a',server);
			break;

		    case 'q' :	/* quiet channel */
			mode_changemode(chan,addmode,CHANMODE_QUIET);
			if(chan->mode_keep && strchr(chan->mode_keep,'q'))
				mode_reversemode(addmode,chan,nick,'q',server);
			break;

		    case 'l' :	/* channel has limit */
			mode_changemode(chan,addmode,CHANMODE_LIMIT);
			if(addmode)
				split(arg,args);
			else
				strcpy(arg,"");
			mode_setlimit(addmode,chan,nick,server,atoi(arg));
			break;

		    case 'k' :	/* channel key */
			mode_changemode(chan,addmode,CHANMODE_KEY);
			split(arg,args);
			mode_setkey(addmode,chan,nick,server,arg);
			break;

		    case 'o' :	/* oper */
			/*
			 * If 'm' is NULL, it means the mode change was by
			 * a server.  Flag 'serverops' since this mode was
			 * a +o.
			 */
			split(arg,args);
			if(!server)
				mode_changeumode(chan,addmode,MEMBER_OPER|MEMBER_SERVEROP,arg);
			else
				mode_changeumode(chan,addmode,MEMBER_OPER,arg);

			/* if it was me, we have some work to do */
			if(addmode && isequal(arg,Server->currentnick) && Channel_gotallchaninfo(chan))
				mode_pushchannelmodes(chan);
			break;

		    case 'v' :	/* voice */
			split(arg,args);
			mode_changeumode(chan,addmode,MEMBER_VOICE,arg);
			break;

		    case 'b' :	/* ban */
			split(arg,args);
			if(addmode)
			{
				Channel_addban(chan,arg,nick,uh,time(NULL));
				mode_onban(chan,arg,nick,uh);
			}
			else
				Channel_removeban(chan,arg);
			break;
		}
	}
}

void mode_changemode(chanrec *chan, int addmode, u_long type)
{
	if(addmode)
		chan->modes |= type;
	else
		chan->modes &= ~type;

	/* Check if mode needs to be reversed (if enforcing) */
	
}

void mode_changeumode(chanrec *chan, int addmode, u_long type, char *who)
{
	membrec *m;

	m = Channel_member(chan,who);
	if(m == NULL)
	{
		Log_write(LOG_ERROR,"*","Mode changes for nonexistant user %s.",who);
		return;
	}

	if(addmode)
	{
		m->flags |= type;
		if(type & MEMBER_OPER)
			m->flags &= ~MEMBER_PENDINGOP;
	}
	else
	{
		m->flags &= ~type;
		if(type & MEMBER_OPER)
			m->flags &= ~MEMBER_PENDINGDEOP;
	}
}

void mode_onban(chanrec *chan, char *ban, char *nick, char *uh)
{
	userrec	*u;
	acctrec	*a;
	char	tmp_uh[UHOSTLEN];
	int	reverse_ban = 0, kick_user = 0;

	/* check if ban matches any user */
	/* if user is protected, remove ban */
	for(u=User->userlist; u!=NULL; u=u->next)
	{
		if(!(u->flags & (USER_BOT | USER_FRIEND | USER_MASTER | USER_PROTECT)))
			continue;

		/* Check if this user matches the ban */
		for(a=u->accounts; a!=NULL; a=a->next)
		{
			if(!a->registered)
				continue;

			sprintf(tmp_uh,"%s!%s",a->nick,a->uh);
			if(wild_match(ban,tmp_uh) || wild_match(tmp_uh,ban))
			{
				if(u->flags & (USER_PROTECT | USER_MASTER))
					kick_user = 1;

				reverse_ban = 1;
			}
		}
	}

	u = User_user(nick);
	if(u->registered)
	{
		/* Increment ban counter */
		u->bans[today()]++;
	}

	/* Remove ban if user was of type +f, +m, +p */
	if(reverse_ban && Channel_ihaveops(chan))
	{
		if(!(u->flags & USER_MASTER))
		{
			Log_write(LOG_PUBLIC,chan->name,"Reversing ban for %s",ban);
			Server_write(PRIORITY_HIGH,"MODE %s -b %s",chan->name,ban);
		}
	}

	/* If user was +p, +m  kick user unless they're +m, +p, or +3 */
	if(kick_user && Channel_ihaveops(chan))
	{
		if(!(u->flags & (USER_PROTECT | USER_MASTER | USER_31337 | USER_TRUSTED | USER_OP | USER_BOT)))
		{
			Log_write(LOG_PUBLIC,chan->name,"Kicking %s (tried to ban protected user)",nick);
			Server_write(PRIORITY_HIGH,"KICK %s %s :That user is being protected.",chan->name,nick);
		}
	}
}

void mode_pushchannelmodes(chanrec *chan)
{
	banrec	*b;
	banlrec	*bl;
	userrec	*u;
	acctrec	*a;
	membrec	*m;
	char	tmp_uh[UHOSTLEN], bbb[10], q_mode[UHOSTLEN * 3], q_nick[512];
	char	nick[NICKLEN], mode1[25], mode2[256], ooo[10];
	int	counter = 0, i;


	context;
	/* Check if we need to set topic */
	if((!(chan->modes & CHANMODE_TOPIC) || Channel_ihaveops(chan)) && chan->topic_keep && chan->topic_keep[0] && !isequal(chan->topic,chan->topic_keep))
		Server_write(PRIORITY_HIGH,"TOPIC %s :%s",chan->name,chan->topic_keep);

	context;
	/* If we have don't have ops, we can quit now. */
	if(!Channel_ihaveops(chan))
		return;

	context;
	/* Push channel modes, key, and limit */
	mode1[0] = 0;
	mode2[0] = 0;
	context;
	if(chan->mode_keep && chan->mode_keep[0])
		sprintf(mode1,"%s",chan->mode_keep);
	context;
	if(chan->limit_keep && (chan->limit != chan->limit_keep))
	{
		sprintf(mode1,"%s+l",mode1);
		sprintf(mode2,"%s %d",mode2,chan->limit_keep);
	}
	context;
	if(chan->key_keep && chan->key_keep[0])
	{
		if(!chan->key)
			chanrec_setkey(chan,"");

		if(chan->key[0])
		{
			if(!isequal(chan->key,chan->key_keep))
			{
				sprintf(mode1,"%s-k",mode1);
				sprintf(mode2,"%s %s",mode2,chan->key);
			}
		}
		else
		{
			sprintf(mode1,"%s+k",mode1);
			sprintf(mode2,"%s %s",mode2,chan->key_keep);
		}
	}

	context;
	if(mode1[0])
		Server_write(PRIORITY_HIGH,"MODE %s %s %s",chan->name,mode1,mode2);

	/* Check the channel banlist, and undo appropriate bans */
	q_mode[0] = 0;
	q_nick[0] = 0;
	counter = 0;

	context;
	for(b=chan->banlist; b!=NULL; b=b->next)
	{
		for(u=User->userlist; u!=NULL; u=u->next)
		{
			if(!(u->flags & (USER_FRIEND | USER_MASTER | USER_PROTECT | USER_BOT)))
				continue;

			/* Check if this user matches the ban */
			for(a=u->accounts; a!=NULL; a=a->next)
			{
				if(!a->registered)
					continue;

				sprintf(tmp_uh,"%s!%s",a->nick,a->uh);
				if(wild_match(b->ban,tmp_uh) || wild_match(tmp_uh,b->ban))
				{
					Log_write(LOG_PUBLIC,chan->name,"Reversing ban previously set for %s",b->ban);
					counter++;
					sprintf(q_mode,"%s%s ",q_mode,b->ban);

					if(counter >= 3)
					{
						Server_write(PRIORITY_HIGH,"MODE %s -bbb %s",chan->name,q_mode);
						q_mode[0] = 0;
						counter = 0;
					}
				}
			}
		}
	}
	if(counter)
	{
		bbb[0] = 0;
		for(i=1; i<=counter; i++)
			sprintf(bbb,"%sb",bbb);

		Server_write(PRIORITY_HIGH,"MODE %s -%s %s",chan->name,bbb,q_mode);
	}

	/* Now check the shitlist, and ban those that need to be banned */
	q_mode[0] = 0;
	q_nick[0] = 0;
	counter = 0;

	context;
	for(m=chan->memberlist; m!=NULL; m=m->next)
	{
		/* Chist, make sure we don't ban ourselves */
		if(isequal(m->nick,Server->currentnick))
			continue;

		sprintf(tmp_uh,"%s!%s",m->nick,m->uh);
		bl = Banlist_match(tmp_uh,chan);
		if(bl != NULL)
		{
			Log_write(LOG_PUBLIC,chan->name,"Kickbanned %s %s from %s (on banlist)",m->nick,m->uh,chan->name);

			counter++;
			bl->used++;
			sprintf(q_mode,"%s%s ",q_mode,bl->ban);
			sprintf(q_nick,"%s%s ",q_nick,m->nick);

			if(counter >= 3)
			{
				Server_write(PRIORITY_HIGH,"MODE %s -ooo+bbb %s %s",chan->name,q_nick,q_mode);
				q_mode[0] = 0;
				counter = 0;

				for(i=0; i<3; i++)
				{
					split(nick,q_nick);
					if(nick[0])
						Server_write(PRIORITY_HIGH,"KICK %s %s :%s",chan->name,nick,Grufti->shitlist_message);
				}
			}
		}
	}
	if(counter)
	{
		bbb[0] = 0;
		ooo[0] = 0;
		for(i=1; i<=counter; i++)
		{
			sprintf(bbb,"%sb",bbb);
			sprintf(ooo,"%so",ooo);
		}

		Server_write(PRIORITY_HIGH,"MODE %s -%s+%s %s %s",chan->name,ooo,bbb,q_nick,q_mode);
		for(i=1; i<=counter; i++)
		{
			split(nick,q_nick);
			if(nick[0])
				Server_write(PRIORITY_HIGH,"KICK %s %s :%s",chan->name,nick,Grufti->shitlist_message);
		}
	}
}



void mode_setlimit(int addmode, chanrec *chan, char *nick, int server, int limit)
{
	context;
	/* Set limit to passed value */
	if(addmode)
		chan->limit = limit;
	else
		chan->limit = 0;

	if(!Channel_gotallchaninfo(chan) || isequal(Server->currentnick,nick) || !(chan->flags & CHAN_FORCELIMIT))
		return;

	/* If we don't have ops, nothing we can do. */
	if(!Channel_ihaveops(chan))
		return;

	context;
	/* We're enforcing a limit.  Reverse if necessary. */
	if(chan->limit != chan->limit_keep)
	{
		if(chan->limit_keep == 0)
		{
			Server_write(PRIORITY_HIGH,"MODE %s -l",chan->name);
			if(!server)
				sendnotice(nick,"I'm enforcing no limits on %s.",chan->name);
		}
		else
		{
			Server_write(PRIORITY_HIGH,"MODE %s +l %d",chan->name,chan->limit_keep);
			if(!server)
				sendnotice(nick,"I'm enforcing a limit of %d on %s.",chan->limit_keep,chan->name);
		}
	}
}

void mode_setkey(int addmode, chanrec *chan, char *nick, int server, char *key)
{
	/* Set limit to passed value */
	if(addmode)
		chanrec_setkey(chan,key);
	else
		chanrec_setkey(chan,"");

	if(!Channel_gotallchaninfo(chan) || !(chan->flags & CHAN_FORCEKEY))
		return;

	if(addmode && isequal(Server->currentnick,nick))
		return;

	/* If we don't have ops, nothing we can do. */
	if(!Channel_ihaveops(chan))
		return;

	/* We're enforcing a key.  Reverse if necessary. */
	if(addmode && !isequal(chan->key_keep,chan->key))
	{
		if(!chan->key_keep[0])
		{
			Server_write(PRIORITY_HIGH,"MODE %s -k %s",chan->name,key);
			if(!server)
				sendnotice(nick,"I'm enforcing no key on %s.",chan->name);
		}
	}

	if(!addmode && !isequal(chan->key_keep,chan->key))
	{
		if(chan->key_keep[0])
		{
			Server_write(PRIORITY_HIGH,"MODE %s +k %s",chan->name,chan->key_keep);
			if(!server && !isequal(Server->currentnick,nick))
				sendnotice(nick,"I'm enforcing the key \"%s\" on %s.",chan->key_keep,chan->name);
		}
	}
}

void mode_reversemode(int addmode, chanrec *chan, char *nick, char mode, int server)
{
	int	pos = 0;
	char	polarity = '+', *p;

	if(!Channel_gotallchaninfo(chan) || !(chan->flags & CHAN_FORCEMODE))
		return;

	if(isequal(Server->currentnick,nick))
		return;

	if(!Channel_ihaveops(chan))
		return;

	/* Search through mode to check polarity */
	for(p=chan->mode_keep; *p; p++)
	{
		if(*p == '+')
			pos = 1;
		else if(*p == '-')
			pos = 0;
		else if(*p == mode)
		{
			polarity = pos ? '+' : '-';
			break;
		}
	}

	/* Reverse if necessary */
	if(addmode && polarity == '-')
	{
		Server_write(PRIORITY_HIGH,"MODE %s -%c",chan->name,mode);
		if(!server)
			sendnotice(nick,"I'm enforcing the mode -%c on %s.",mode,chan->name);
	}
	if(!addmode && polarity == '+')
	{
		Server_write(PRIORITY_HIGH,"MODE %s +%c",chan->name,mode);
		if(!server)	
			sendnotice(nick,"I'm enforcing the mode +%c on %s.",mode,chan->name);
	}
}
