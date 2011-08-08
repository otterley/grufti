/******************************************************************************
 * Steve Rider - stever@area23.org
 * Grufti - an interactive, service-oriented bot for IRC 
 *
 * channel.c - channel storage and operations.
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
#include "channel.h"
#include "misc.h"
#include "log.h"
#include "server.h"
#include "user.h"

/****************************************************************************
 *
 * Channel object definitions
 *
 ****************************************************************************/

void Channel_new()
{
	/* already in use? */
	if(Channel != NULL)
		Channel_freeall();

	/* allocate memory */
	Channel = (Channel_ob *)xmalloc(sizeof(Channel_ob));

	/* initialize */
	Channel->channels = NULL;
	Channel->last = NULL;

	Channel->num_channels = 0;
	Channel->time_tried_join = 0L;
	Channel->stats_active = 0;
}

u_long Channel_sizeof()
{
	u_long	tot = 0L;
	chanrec	*c;

	tot += sizeof(Channel_ob);
	for(c=Channel->channels; c!=NULL; c=c->next)
		tot += chanrec_sizeof(c);

	return tot;
}

void Channel_freeall()
{
	/* Free all channels */
	Channel_freeallchans();

	/* Free the object */
	xfree(Channel);
	Channel = NULL;
}

/****************************************************************************/
/* object channel data-specific function definitions. */

void Channel_display()
{
	chanrec *chan;

	say("%d total channels.",Channel->num_channels);
	for(chan=Channel->channels; chan!=NULL; chan=chan->next)
		chanrec_display(chan);
}

/****************************************************************************/
/* low-level Channel structure function definitions. */

void Channel_appendchan(chanrec *chan)
{
	chanrec_append(&Channel->channels, &Channel->last, chan);
	Channel->num_channels++;
}

void Channel_deletechan(chanrec *chan)
{
	chanrec_delete(&Channel->channels, &Channel->last, chan);
	Channel->num_channels--;
}

void Channel_freeallchans()
{
	chanrec_freeall(&Channel->channels, &Channel->last);
	Channel->num_channels = 0;
}

void Channel_appendmember(chanrec *chan, membrec *m)
{
	membrec_append(&chan->memberlist, &chan->m_last, m);
	chan->num_members++;
}

void Channel_deletemember(chanrec *chan, membrec *m)
{
	membrec_delete(&chan->memberlist, &chan->m_last, m);
	chan->num_members--;
}

void Channel_freeallmembers(chanrec *chan)
{
	membrec_freeall(&chan->memberlist, &chan->m_last);
	chan->num_members = 0;
}

void Channel_appendban(chanrec *chan, banrec *b)
{
	banrec_append(&chan->banlist, &chan->b_last, b);
	chan->num_bans++;
}

void Channel_deleteban(chanrec *chan, banrec *b)
{
	banrec_delete(&chan->banlist, &chan->b_last, b);
	chan->num_bans--;
}

void Channel_freeallbans(chanrec *chan)
{
	banrec_freeall(&chan->banlist, &chan->b_last);
	chan->num_bans = 0;
}

/****************************************************************************/
/* object Channel high-level implementation-specific function definitions. */

chanrec *Channel_addchannel(char *name)
{
	chanrec *x;

	if(Channel_channel(name))
	{
		Log_write(LOG_ERROR,"*","Attempting to add channel \"%s\", which is already in my records.",name);
		return NULL;
	}

	x = chanrec_new();
	x->flags |= CHAN_PERMANENT;
	chanrec_setname(x,name);

	Channel_appendchan(x);

	return x;
}

void Channel_setchannel(char *name, char *param, char *value)
{
	chanrec	*c;

	c = Channel_channel(name);
	if(c == NULL)
	{
		Log_write(LOG_ERROR,"*","Config file error: No such channel \"%s\" (Unable to set %s)",name,param);
		return;
	}

	if(isequal(param,"key"))
	{
		if(!rmquotes(value))
		{
			Log_write(LOG_ERROR,"*","Config file error: value \"%s\" not quoted.",value);
			return;
		}

		/* got channel, it's for key, and got param. */
		chanrec_setkey_keep(c,value);
	}
	else if(isequal(param,"force-key"))
	{
		if(!rmquotes(value))
		{
			Log_write(LOG_ERROR,"*","Config file error: value \"%s\" not quoted.",value);
			return;
		}

		/* got channel, it's for key, and got param. */
		chanrec_setkey_keep(c,value);
		c->flags |= CHAN_FORCEKEY;
	}
	else if(isequal(param,"show-key"))
	{
		/* Just in case it's quoted */
		if(rmquotes(value))
			Log_write(LOG_ERROR,"*","Config file warning: value %s does not need to be quoted.",value);
		if(atoi(value) == 1)
			c->flags |= CHAN_SHOWKEY;
		else
			c->flags &= ~CHAN_SHOWKEY;
	}
	else if(isequal(param,"topic"))
	{
		if(!rmquotes(value))
		{
			Log_write(LOG_ERROR,"*","Config file error: value \"%s\" not quoted.",value);
			return;
		}

		chanrec_settopic_keep(c,value);
	}
	else if(isequal(param,"force-topic"))
	{
		if(!rmquotes(value))
		{
			Log_write(LOG_ERROR,"*","Config file error: value \"%s\" not quoted.",value);
			return;
		}

		chanrec_settopic_keep(c,value);
		c->flags |= CHAN_FORCETOPIC;
	}
	else if(isequal(param,"mode"))
	{
		char	outmode[25], *p;

		if(!rmquotes(value))
		{
			Log_write(LOG_ERROR,"*","Config file error: value \"%s\" not quoted.",value);
			return;
		}

		outmode[0] = 0;
		/* Parse mode and dump bad values */
		for(p=value; *p; p++)
		{
			if(strchr("+-ikmnptsaq",*p))
				sprintf(outmode,"%s%c",outmode,*p);
			else
				Log_write(LOG_ERROR,"*","Config file error: Bad channel mode \"%c\" in \"%s\".",*p,value);
		}

		chanrec_setmode_keep(c,outmode);
	}
	else if(isequal(param,"force-mode"))
	{
		char	outmode[25], *p;

		if(!rmquotes(value))
		{
			Log_write(LOG_ERROR,"*","Config file error: value \"%s\" not quoted.",value);
			return;
		}

		outmode[0] = 0;
		/* Parse mode and dump bad values */
		for(p=value; *p; p++)
		{
			if(strchr("+-imnpstaq",*p))
				sprintf(outmode,"%s%c",outmode,*p);
			else
				Log_write(LOG_ERROR,"*","Config file error: Bad channel mode \"%c\" in \"%s\".",*p,value);
		}

		chanrec_setmode_keep(c,outmode);
		c->flags |= CHAN_FORCEMODE;
	}
	else if(isequal(param,"limit"))
	{
		/* Just in case it's quoted */
		if(rmquotes(value))
			Log_write(LOG_ERROR,"*","Config file warning: value %s does not need to be quoted.",value);
		c->limit_keep = atoi(value);
	}
	else if(isequal(param,"force-limit"))
	{
		/* Just in case it's quoted */
		if(rmquotes(value))
			Log_write(LOG_ERROR,"*","Config file warning: value %s does not need to be quoted.",value);
		c->limit_keep = atoi(value);
		c->flags |= CHAN_FORCELIMIT;
	}
	else
		Log_write(LOG_ERROR,"*","Config file error: Invalid channel setting \"%s\".",param);
}
		
void Channel_join(chanrec *chan)
{
	/* Don't join if we've been asked to leave. */
	if(chan->flags & CHAN_LEAVE)
		return;

	if(chan->key && chan->key[0])
		Server_write(PRIORITY_HIGH,"JOIN %s %s",chan->name,chan->key);
	else
		Server_write(PRIORITY_HIGH,"JOIN %s",chan->name);
}

void Channel_joinunjoinedchannels()
{
	chanrec *c;

	Channel->time_tried_join = time(NULL);

	for(c=Channel->channels; c!=NULL; c=c->next)
		if(!(c->flags & CHAN_ONCHAN))
			Channel_join(c);
}

chanrec *Channel_channel(char *name)
{
	return(chanrec_channel(&Channel->channels, &Channel->last, name));
}
	
membrec *Channel_member(chanrec *chan, char *nick)
{
	return(membrec_member(&chan->memberlist, &chan->m_last, nick));
}

banrec *Channel_ban(chanrec *chan, char *ban)
{
	return(banrec_ban(&chan->banlist, &chan->b_last, ban));
}

void Channel_resetchaninfo(chanrec *chan)
{
	Channel_clearchaninfo(chan);
	chan->flags |= CHAN_PENDING;
	chan->flags &= ~CHAN_ACTIVE;

	if(!(chan->flags & CHAN_ONCHAN))
	{
		/* Doh! */
		Log_write(LOG_DEBUG,"*","Oops.  Attempting to reset channel info for %s and CHAN_ONCHAN is not set!",chan->name);
		return;
	}

	Server_write(PRIORITY_HIGH,"WHO %s",chan->name);
	Server_write(PRIORITY_HIGH,"MODE %s",chan->name);
	Server_write(PRIORITY_HIGH,"MODE %s +b",chan->name);
	Server_write(PRIORITY_HIGH,"TOPIC %s",chan->name);
}

membrec *Channel_addnewmember(chanrec *chan, char *nick, char *uh)
{
	membrec *m;

	/* Is this person already on the channel? */
	m = Channel_member(chan,nick);
	if(m != NULL)
		Channel_removemember(chan,m);

	/* create new member */
	m = membrec_new();

	/*
	 * Don't set when this user was last seen publicly.  If we set it to
	 * now, we don't know when they last were seen before this for use
	 * in gotjoin().  We also don't know when they last spoke if we are
	 * adding this member in another way, like when we get a WHO response.
	 *
	 * (Ok to set 'last_seen' for account though...)
	 */

	/* write some data */
	membrec_setnick(m,nick);
	membrec_setuh(m,uh);
	m->user = User_user(nick);
	m->account = User_account(m->user,nick,uh);

	/* add them to the channel */
	Channel_appendmember(chan,m);

	/* update last seen time in acctrec */
	User_gotactivity(m->user,m->account);

	/* do statistics stuff */
	User_gotpossiblesignon(m->user,m->account);

	return m;
}

banrec *Channel_addban(chanrec *chan, char *ban, char *nick, char *uh, time_t t)
{
	banrec *b;

	/* Create new node. */
	b = banrec_new();

	/* Write some data */
	banrec_setban(b,ban);
	banrec_setwho_n(b,nick);
	banrec_setwho_uh(b,uh);
	b->time_set = t;

	/* append */
	Channel_appendban(chan,b);

	return b;
}

void Channel_removeban(chanrec *chan, char *ban)
{
	banrec	*b;

	/* Look for ban */
	b = Channel_ban(chan,ban);

	if(b != NULL)
		Channel_deleteban(chan,b);
}
	
	
/* TRUE if channel is verified active */
int Channel_verifychanactive(chanrec *chan)
{
	if(!(chan->flags & CHAN_ONCHAN))
	{
		Log_write(LOG_DEBUG,"*","Oops.  Found myself on %s and CHAN_ONCHAN is not set.  Resetting.",chan->name);
		Channel_resetchaninfo(chan);
		return 0;
	}

	if(!(chan->flags & CHAN_ACTIVE))
		return 0;

	return 1;
}

membrec *Channel_memberofanychan(char *nick)
{
	chanrec *chan;
	membrec	*m;

	for(chan=Channel->channels; chan!=NULL; chan=chan->next)
	{
		m = Channel_member(chan,nick);
		if(m != NULL)
			return m;
	}

	return NULL;
}

int Channel_ismemberofanychan(char *nick)
{
	chanrec *chan;

	for(chan=Channel->channels; chan!=NULL; chan=chan->next)
		if(Channel_member(chan,nick)!=NULL)
			return 1;

	return 0;
}

void Channel_clearallchaninfo()
{
	chanrec *chan;

	for(chan=Channel->channels; chan!=NULL; chan=chan->next)
		Channel_clearchaninfo(chan);
}

void Channel_clearchaninfo(chanrec *chan)
{
	/*
	 * Does NOT free up memory for the channel itself.  Just frees
	 * memory for members, bans, and topic.  Occurs on bot PART'ing
	 * a channel, or getting disconnected from a server.  Remember,
	 * DON'T free up the key, or else we might not be able to rejoin
	 * the channel!  It's ok if we hold onto the key for a bit...
	 */
	if(chan->topic != NULL)
		xfree(chan->topic);
	chan->topic = NULL;

	if(chan->nojoin_reason != NULL)
		xfree(chan->nojoin_reason);
	chan->nojoin_reason = NULL;

	chan->modes = 0L;

	Channel_freeallmembers(chan);
	Channel_freeallbans(chan);
}

void Channel_removemember(chanrec *chan, membrec *m)
{
	/* update last seen time in acctrec */
	User_gotactivity(m->user,m->account);

	/* no need to update time for channel member, they're leaving! */

	/* but update userrec time for last_seen_onchan */
	m->user->last_seen_onchan = time(NULL);

	Channel_deletemember(chan,m);
}

void Channel_resetallpointers()
{
	chanrec *chan;
	membrec *m;

	for(chan=Channel->channels; chan!=NULL; chan=chan->next)
	{
		for(m=chan->memberlist; m!=NULL; m=m->next)
		{
			m->user = User_user(m->nick);
			m->account = User_account(m->user,m->nick,m->uh);
		}
	}
}

void Channel_setallchannelsoff()
{
	chanrec *chan;

	for(chan=Channel->channels; chan!=NULL; chan=chan->next)
	{
		chan->flags &= ~CHAN_ONCHAN;
		chan->flags &= ~CHAN_ACTIVE;
		chan->flags &= ~CHAN_PENDING;
		chan->flags &= ~CHAN_PUBLICFLOOD;
	}
}

void Channel_modes2str(chanrec *chan, char *mode)
{
	char tmp[UHOSTLEN];

	strcpy(mode,"+");

	if(chan->modes & CHANMODE_INVITEONLY)
		strcat(mode,"i");
	if(chan->modes & CHANMODE_KEY)
		strcat(mode,"k");
	if(chan->modes & CHANMODE_LIMIT)
		strcat(mode,"l");
	if(chan->modes & CHANMODE_MODERATED)
		strcat(mode,"m");
	if(chan->modes & CHANMODE_NOMSG)
		strcat(mode,"n");
	if(chan->modes & CHANMODE_PRIVATE)
		strcat(mode,"p");
	if(chan->modes & CHANMODE_SECRET)
		strcat(mode,"s");
	if(chan->modes & CHANMODE_TOPIC)
		strcat(mode,"t");
	if(chan->modes & CHANMODE_ANONYMOUS)
		strcat(mode,"a");
	if(chan->modes & CHANMODE_QUIET)
		strcat(mode,"q");

	if(chan->modes & CHANMODE_KEY)
	{
		if(chan->flags & CHAN_SHOWKEY)
			sprintf(tmp," %s",chan->key);
		else
			sprintf(tmp, " ********");
		strcat(mode,tmp);
	}
	if(chan->modes & CHANMODE_LIMIT)
	{
		sprintf(tmp," %d",chan->limit);
		strcat(mode,tmp);
	}

	/* if no modes are set */
	if(isequal(mode,"+"))
		mode[0] = 0;
}

void Channel_chanflags2str(int flags, char *str)
{
	str[0] = 0;

	if(flags & CHAN_ONCHAN)
		strcat(str,"(on channel) ");
	else
		strcat(str,"(not on channel) ");
	if(flags & CHAN_ACTIVE)
		strcat(str,"(active) ");
	else
		strcat(str,"(not active) ");
	if(flags & CHAN_PENDING)
		strcat(str,"(pending) ");
	else
		strcat(str,"(not pending) ");
}

int Channel_ihaveops(chanrec *chan)
{
	membrec *m;

	if(!(chan->flags & CHAN_ACTIVE))
		return 0;

	m = Channel_member(chan,Server->currentnick);
	if(m == NULL)
	{
		Log_write(LOG_DEBUG,"*","Attempting to find out if ihaveops(), but I can't find myself (%s) on the channel! (%s)",Server->currentnick,chan->name);
		return 0;
	}

	if(m->flags & MEMBER_OPER)
		return 1;
	else
		return 0;
}

void Channel_cycle(chanrec *chan)
{
	Server_write(PRIORITY_HIGH,"PART :%s",chan->name);
}

int Channel_num_members_notsplit(chanrec *chan)
{
	membrec	*m;
	int	tot = 0;

	for(m=chan->memberlist; m!=NULL; m=m->next)
		if(m->split == 0L)
			tot++;

	return tot;
}

void Channel_checkonlymeonchannel(chanrec *chan)
{
	/* Check if only I am left. */
	if(Channel_num_members_notsplit(chan) == 1)
	{
		if(!Channel_ihaveops(chan))
		{
			Log_write(LOG_PUBLIC,chan->name,"Attempting to cycle %s to gain ops...",chan->name);
			Channel_cycle(chan);
		}
	}
}

int Channel_onallchannels()
{
	chanrec *c;

	/* If not on channel and we haven't been asked to leave */
	for(c=Channel->channels; c!=NULL; c=c->next)
		if(!(c->flags & CHAN_ONCHAN) && !(c->flags & CHAN_LEAVE))
			return 0;

	return 1;
}
	

void Channel_makechanlist(char *channels)
{
	chanrec	*c;

	channels[0] = 0;
	for(c=Channel->channels; c!=NULL; c=c->next)
		sprintf(channels,"%s%s, ",channels,c->name);

	if(strlen(channels) >= 2)
		channels[strlen(channels) - 2] = 0;
}

void Channel_giveops(chanrec *chan, membrec *m)
{
	if(m->flags & MEMBER_OPER || m->flags & MEMBER_PENDINGOP)
		return;

	Server_write(PRIORITY_HIGH,"MODE %s +o %s",chan->name,m->nick);
	m->flags |= MEMBER_PENDINGOP;
}

void Channel_givevoice(chanrec *chan, membrec *m)
{
	if(m->flags & MEMBER_VOICE || m->flags & MEMBER_PENDINGVOICE)
		return;

	Server_write(PRIORITY_HIGH,"MODE %s +v %s",chan->name,m->nick);
	m->flags |= MEMBER_PENDINGVOICE;
}

void Channel_checkfortimeout()
{
	chanrec	*chan;
	time_t	now = time(NULL);
	membrec	*m, *m_next;

	for(chan=Channel->channels; chan!=NULL; chan=chan->next)
	{
		/* Check for LEAVE timeout */
		if(chan->flags & CHAN_LEAVE)
		{
			if((now - chan->leave_time) > chan->leave_timeout)
			{
				chan->leave_time = 0L;
				chan->flags &= ~CHAN_LEAVE;
			}
		}

		/* Check for HUSH timeout */
		if(chan->flags & CHAN_HUSH)
		{
			if((now - chan->hush_time) > chan->hush_timeout)
			{
				chan->hush_time = 0L;
				chan->flags &= ~CHAN_HUSH;
			}
		}

		/* Check for flood timeout */
		if(chan->flags & CHAN_PUBLICFLOOD)
		{
			if((now - chan->start_public_flood) > Grufti->public_flood_timeout)
			{
				chan->flags &= ~CHAN_PUBLICFLOOD;
				chan->start_public_flood = 0L;
				Log_write(LOG_PUBLIC,chan->name,"Public flood timeout, returning to normal operation.");
			}
		}

		/* check for split timeout */
		m = chan->memberlist;
		while(m != NULL)
		{
			m_next = m->next;
			if(m->split && ((now - m->split) > Grufti->memberissplit_timeout))
			{
				Log_write(LOG_PUBLIC,chan->name,"Lost %s in the split.",m->nick);
				Channel_removemember(chan,m);
			}

			m = m_next;
		}

		/* Check for idlers */
		if(Grufti->kick_channel_idlers && Channel_ihaveops(chan) &&
		(now - chan->last_checked_idlers) > Grufti->idlekick_timeout)
		{
			time_t last_heard;
			chan->last_checked_idlers = time(NULL);

			for(m=chan->memberlist; m!=NULL; m=m->next)
			{
				/* Don't kick me! */
				if(isequal(m->nick,Server->currentnick))
					continue;

				/* Don't idle kick bots or masters! */
				if(m->user->flags & (USER_BOT|USER_MASTER))
					continue;

				/* Don't kick opers if not configured */
				if(!Grufti->kick_idle_opers && (m->user->flags & USER_OP))
					continue;

				/* No last time.  check join */
				if(!m->last)
				{
					/* No join time, use bot's */
					if(!m->joined)
					{
						membrec	*mbot;
						mbot = Channel_member(chan,Server->currentnick);
						if(mbot == NULL)
							last_heard = 0l;
						else
							last_heard = mbot->joined;
					}
					else
						last_heard = m->joined;
				}
				else
					last_heard = m->last;

				if(last_heard && (now - last_heard) > Grufti->idle_time)
				{
					if(!(chan->modes & CHANMODE_INVITEONLY))
					{
						Server_write(PRIORITY_HIGH,"MODE %s +i",chan->name);
						/* Set timer for reversing +i */
						chan->plus_i_idlekick_time = time(NULL);
					}

					/* Got an idler.  Kick it. */
					Server_write(PRIORITY_HIGH,"KICK %s %s :%s",chan->name,m->nick,Grufti->idlekick_message);
					break;	
				}
			}
		}

		/* See if our +i flag has expired */
		if(chan->plus_i_idlekick_time)
		{
			if((now - chan->plus_i_idlekick_time) > Grufti->plus_i_idlekick_timeout)
			{
				/* Timer expired.  Reset and change mode */
				chan->plus_i_idlekick_time = 0L;
				Server_write(PRIORITY_HIGH,"MODE %s -i",chan->name);
			}
		}
	}
}

int Channel_orderbyjoin_ltoh(chanrec *chan)
{
	membrec	*m, *m_save = NULL;
	int	counter = 0, done = 0;
	time_t	h, highest = time(NULL);

	/* We need to clear all ordering before we start */
	Channel_clearorder(chan);

	/* Continue until all entries have an order number */
	while(!done)
	{
		done = 1;
		h = highest;
		for(m=chan->memberlist; m!=NULL; m=m->next)
		{
			if(!m->order && m->joined <= h)
			{
				done = 0;
				h = m->joined;
				m_save = m;
			}
		}

		/* Not done means we got u_save and now we need to order it */
		if(!done)
		{
			counter++;
			m_save->order = counter;
		}
	}

	return counter;
}

int Channel_orderbyjoin_htol(chanrec *chan)
{
	membrec	*m, *m_save = NULL;
	int	counter = 0, done = 0;
	time_t	l, lowest = 0L;

	/* We need to clear all ordering before we start */
	Channel_clearorder(chan);

	/* Continue until all entries have an order number */
	while(!done)
	{
		done = 1;
		l = lowest;
		for(m=chan->memberlist; m!=NULL; m=m->next)
		{
			if(!m->order && m->joined >= l)
			{
				done = 0;
				l = m->joined;
				m_save = m;
			}
		}

		/* Not done means we got u_save and now we need to order it */
		if(!done)
		{
			counter++;
			m_save->order = counter;
		}
	}

	return counter;
}

void Channel_clearorder(chanrec *chan)
{
	membrec	*m;

	for(m=chan->memberlist; m!=NULL; m=m->next)
		m->order = 0;
}

membrec *Channel_memberbyorder(chanrec *chan, int ordernum)
{
	membrec	*m;

	for(m=chan->memberlist; m!=NULL; m=m->next)
		if(m->order == ordernum)
			return m;

	return NULL;
}

void Channel_updatestatistics(int hour)
{
	chanrec	*chan;
	time_t	now = time(NULL);
	int	len, lasthour;

	/* Format lasthour correctly */
	lasthour = hour ? hour - 1 : 23;

	/* Loop through all channels and update each one's population */
	for(chan=Channel->channels; chan!=NULL; chan=chan->next)
	{
		/* Calculate population */
		chan->pop_prod += chan->pop_currentpop * (now - chan->pop_timestarted);
		chan->pop_currentpop = chan->num_members;
		chan->pop_timestarted = now;

		/* Determine how long during this hour we were on the channel */
		if((now - 3600) < chan->joined)
			len = now - chan->joined;
		else
			len = 3600;

		if(len == 0)
			chan->population[lasthour] = 0;
		else
			chan->population[lasthour] = (chan->pop_prod / len) + 1;

		chan->pop_prod = 0;

		/* Clear join and channel boxes for next hour */
		chan->joins_reg[hour] = 0;
		chan->joins_nonreg[hour] = 0;
		chan->chatter[hour] = 0;
	}
}

int Channel_gotallchaninfo(chanrec *chan)
{
	if((chan->modes & CHANMODE_GOTTOPIC) && (chan->modes & CHANMODE_GOTWHO) && (chan->modes & CHANMODE_GOTBANS) && (chan->modes & CHANMODE_GOTMODES))
		return 1;
	else
		return 0;
}

void Channel_trytogiveops(char *nick)
{
	chanrec	*chan;
	membrec	*m;

	if(!Channel_ismemberofanychan(nick))
		return;

	/* They're ona channel somewhere.  Track em down */
	for(chan=Channel->channels; chan!=NULL; chan=chan->next)
	{
		context;
		/* if i don't have ops, don't bother */
		if(!Channel_ihaveops(chan))
			continue;

		context;
		/* i have ops on this channel.  is member on it? */
		m = Channel_member(chan,nick);
		if(m != NULL)
		{
			context;
			if(!(m->flags & MEMBER_OPER) && !(m->flags & MEMBER_PENDINGOP) && !m->split)
				Channel_giveops(chan,m);
		}
	
	}
}

/* ASDF */
/***************************************************************************
 *
 * channel record definitions.  No changes should be made to new(),
 * freeall(), append(), delete(), or movetofront().
 *
 ****************************************************************************/

chanrec *chanrec_new()
{
	chanrec *x;

	/* allocate memory */
	x = (chanrec *)xmalloc(sizeof(chanrec));

	/* initialize */
	x->next = NULL;

	chanrec_init(x);

	return x;
}

void chanrec_freeall(chanrec **chanlist, chanrec **last)
{
	chanrec *c = *chanlist, *v;

	while(c != NULL)
	{
		v = c->next;
		chanrec_free(c);
		c = v;
	}

	*chanlist = NULL;
	*last = NULL;
}

void chanrec_append(chanrec **list, chanrec **last, chanrec *entry)
{
	chanrec *lastentry = *last;

	if(*list == NULL)
		*list = entry;
	else
		lastentry->next = entry;

	*last = entry;
	entry->next = NULL;
}

void chanrec_delete(chanrec **list, chanrec **last, chanrec *x)
{
	chanrec *c, *lastchecked = NULL;

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

			chanrec_free(c);
			return;
		}

		lastchecked = c;
		c = c->next;
	}
}

void chanrec_movetofront(chanrec **list, chanrec **last, chanrec *lastchecked, chanrec *c)
{
	if(lastchecked != NULL)
	{
		if(*last == c)
			*last = lastchecked;

		lastchecked->next = c->next;
		c->next = *list;
		*list = c;
	}
}

/****************************************************************************/
/* record chanrec low-level data-specific function declarations. */


void chanrec_init(chanrec *chan)
{
	int	i;

	/* initialize */
	chan->name = NULL;
	chan->limit_keep = 0;
	chan->key_keep = NULL;
	chan->topic_keep = NULL;
#ifdef MSK
	chan->mode_keep = NULL;
#endif
	chan->flags = 0L;

	chan->modes = 0L;
	chan->limit = 0;
	chan->key = NULL;
	chan->topic = NULL;
	chan->num_members = 0;
	chan->num_bans = 0;
	chan->memberlist = NULL;
	chan->m_last = NULL;
	chan->banlist = NULL;
	chan->b_last = NULL;
	chan->nojoin_reason = NULL;
	for(i=0; i<RESPRATE_NUM; i++)
		chan->resp_boxes[i] = 0L;
	chan->resp_boxes_pos = 0;
	chan->start_public_flood = 0L;
	chan->joined = 0L;
	chan->leave_time = 0L;
	chan->leave_timeout = 0L;
	chan->hush_time = 0L;
	chan->hush_timeout = 0L;
	chan->plus_i_idlekick_time = 0L;
	chan->last_checked_idlers = 0L;

	for(i=0; i<24; i++)
	{
		chan->joins_reg[i] = 0;
		chan->joins_nonreg[i] = 0;
		chan->chatter[i] = 0L;
		chan->population[i] = 0;
	}
	chan->pop_prod = 0;
	chan->pop_currentpop = 0;
	chan->pop_timestarted = 0L;
	chan->min_population = 0;
	chan->max_population = 0;
	chan->min_population_time = 0L;
	chan->max_population_time = 0L;
}

u_long chanrec_sizeof(chanrec *c)
{
	u_long	tot = 0L;
	membrec	*m;
	banrec	*b;

	tot += sizeof(chanrec);
	tot += c->name ? strlen(c->name)+1 : 0L;
	tot += c->key_keep ? strlen(c->key_keep)+1 : 0L;
	tot += c->topic_keep ? strlen(c->topic_keep)+1 : 0L;
	tot += c->key ? strlen(c->key)+1 : 0L;
	tot += c->topic ? strlen(c->topic)+1 : 0L;
	for(m=c->memberlist; m!=NULL; m=m->next)
		tot += membrec_sizeof(m);
	for(b=c->banlist; b!=NULL; b=b->next)
		tot += banrec_sizeof(b);
	tot += c->nojoin_reason ? strlen(c->nojoin_reason)+1 : 0L;

	return tot;
}

void chanrec_free(chanrec *c)
{
	/* Free the elements */
	xfree(c->name);
	xfree(c->key_keep);
	xfree(c->topic_keep);

	xfree(c->key);
	xfree(c->topic);
	xfree(c->nojoin_reason);

	Channel_freeallmembers(c);
	Channel_freeallbans(c);

	/* Free this record */
	xfree(c);
}

void chanrec_setname(chanrec *chan, char *name)
{
	mstrcpy(&chan->name,name);
}

void chanrec_setkey_keep(chanrec *chan, char *key)
{
	mstrcpy(&chan->key_keep,key);
}

void chanrec_settopic_keep(chanrec *chan, char *topic)
{
	mstrcpy(&chan->topic_keep,topic);
}

void chanrec_setmode_keep(chanrec *chan, char *mode)
{
	mstrcpy(&chan->mode_keep,mode);
}

void chanrec_setkey(chanrec *chan, char *key)
{
	mstrcpy(&chan->key,key);
}

void chanrec_settopic(chanrec *chan, char *topic)
{
	mstrcpy(&chan->topic,topic);
}

void chanrec_setnojoin_reason(chanrec *chan, char *reason)
{
	mstrcpy(&chan->nojoin_reason,reason);
}

void chanrec_display(chanrec *chan)
{
	membrec *m;
	banrec *b;
	char flags[FLAGLONGLEN], modes[FLAGLONGLEN];

	if(!Grufti->irc)
	{
		strcpy(flags,"IRC is set to OFF.");
	}
	else if(chan->flags & CHAN_ONCHAN)
	{
		if(!(chan->flags & CHAN_ACTIVE))
			strcpy(flags,"Waiting for channel info...");
		else
			strcpy(flags,"");
	}
	else
	{
		if(chan->nojoin_reason)
			sprintf(flags,"Couldn't join channel (%s)",chan->nojoin_reason);
		else
			strcpy(flags,"Waiting for JOIN to happen.");
	}
	
	Channel_modes2str(chan,modes);
	say("=================================================================");
	if(modes[0])
		say("Channel: %s (%s)",chan->name,modes);
	else
		say("Channel: %s",chan->name);
	if(flags[0])
		say("Status: %s",flags);
	else
	{
		if(chan->topic == NULL)
			say("Topic: <none>");
		else if(chan->topic[0] == 0)
			say("Topic: <none>");
		else
			sayf(0,0,"Topic: \"%s\"",chan->topic);
	}

	if(chan->memberlist != NULL)
	{
		say("");
		say(" Join  Idle  Nick      Userhost                            Levels");
		say("----- ----- ---------- ----------------------------------- ------");
		for(m=chan->memberlist; m!=NULL; m=m->next)
			membrec_display(m);
		say("----- ----- ---------- ----------------------------------- ------");
	}

	if(chan->banlist != NULL)
	{
		say("When Nick     Userhost                       Ban");
		say("---- -------- ------------------------------ --------------------");
		for(b=chan->banlist; b!=NULL; b=b->next)
			banrec_display(b);
		say("---- -------- ------------------------------ --------------------");
	}
	say("=================================================================");
}

chanrec *chanrec_channel(chanrec **list, chanrec **last, char *name)
{
	chanrec *c, *lastchecked = NULL;

	for(c=*list; c!=NULL; c=c->next)
	{
		if(isequal(c->name,name))
		{
			/* found it.  do a movetofront. */
			chanrec_movetofront(list,last,lastchecked,c);

			return c;
		}
		lastchecked = c;
	}

	return NULL;
}



/***************************************************************************n
 *
 * member record definitions.  No changes should be made to new(),
 * freeall(), append(), delete(), or movetofront().
 *
 ****************************************************************************/

membrec *membrec_new()
{
	membrec *x;

	/* allocate memory */
	x = (membrec *)xmalloc(sizeof(membrec));

	/* initialize */
	x->next = NULL;

	membrec_init(x);

	return x;
}

void membrec_freeall(membrec **memberlist, membrec **last)
{
	membrec *m = *memberlist, *v;

	while(m != NULL)
	{
		v = m->next;
		membrec_free(m);
		m = v;
	}

	*memberlist = NULL;
	*last = NULL;
}

void membrec_append(membrec **list, membrec **last, membrec *entry)
{
	membrec *lastentry = *last;

	if(*list == NULL)
		*list = entry;
	else
		lastentry->next = entry;

	*last = entry;
	entry->next = NULL;
}

void membrec_delete(membrec **list, membrec **last, membrec *x)
{
	membrec *m, *lastchecked = NULL;

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

			membrec_free(m);
			return;
		}

		lastchecked = m;
		m = m->next;
	}
}

void membrec_movetofront(membrec **list, membrec **last, membrec *lastchecked, membrec *m)
{
	if(lastchecked != NULL)
	{
		if(*last == m)
			*last = lastchecked;

		lastchecked->next = m->next;
		m->next = *list;
		*list = m;
	}
}

/****************************************************************************/

void membrec_init(membrec *m)
{
	/* initialize */
	m->nick = NULL;
	m->uh = NULL;
	m->user = NULL;
	m->account = NULL;
	m->flags = 0;
	m->joined = 0L;
	m->split = 0L;
	m->last = 0L;
	m->order = 0;
}

u_long membrec_sizeof(membrec *m)
{
	u_long	tot = 0L;

	tot += sizeof(membrec);
	tot += m->nick ? strlen(m->nick)+1 : 0L;
	tot += m->uh ? strlen(m->uh)+1 : 0L;

	return tot;
}

void membrec_free(membrec *m)
{
	/* Free the elements */
	xfree(m->nick);
	xfree(m->uh);

	/* Free this record */
	xfree(m);
}

void membrec_setnick(membrec *m, char *nick)
{
	mstrcpy(&m->nick,nick);
}

void membrec_setuh(membrec *m, char *uh)
{
	mstrcpy(&m->uh,uh);
}

void membrec_display(membrec *m)
{
	char join[TIMESHORTLEN], idle[TIMESHORTLEN], uh[UHOSTLEN];
	char levels[FLAGSHORTLEN], mflags[FLAGSHORTLEN], split[TIMESHORTLEN];

	if(m->joined == 0L)
		strcpy(join,"?????");
	else
		timet_to_time_short(m->joined,join);
	timet_to_ago_short(m->last,idle);
	User_flags2str(m->user->flags,levels);

	if(m->flags & MEMBER_VOICE)
		strcpy(mflags,"v");
	else if(m->flags & MEMBER_OPER)
		strcpy(mflags,"@");
	else
		strcpy(mflags," ");

	if(m->split)
	{
		timet_to_ago_short(m->split,split);
		sprintf(uh,"<-- netsplit for %s",split);
	}
	else
		strcpy(uh,m->uh);

	if(strlen(uh) > 35)
		uh[35] = 0;
	if(strlen(levels) > 6)
		levels[5] = 0;
	say("%5s %5s %1s%-9s %-35s %-5s",join,idle,mflags,m->nick,uh,levels);
}

membrec *membrec_member(membrec **list, membrec **last, char *nick)
{
	membrec *m, *lastchecked = NULL;

	for(m=*list; m!=NULL; m=m->next)
	{
		if(isequal(m->nick,nick))
		{
			/* found it.  do a movetofront. */
			membrec_movetofront(list,last,lastchecked,m);

			return m;
		}
		lastchecked = m;
	}

	return NULL;
}



/***************************************************************************n
 *
 * ban record definitions.  No changes should be made to new(),
 * freeall(), append(), delete(), or movetofront().
 *
 ****************************************************************************/

banrec *banrec_new()
{
	banrec *x;

	/* allocate memory */
	x = (banrec *)xmalloc(sizeof(banrec));

	/* initialize */
	x->next = NULL;

	banrec_init(x);

	return x;
}

void banrec_freeall(banrec **banlist, banrec **last)
{
	banrec *b = *banlist, *v;

	while(b != NULL)
	{
		v = b->next;
		banrec_free(b);
		b = v;
	}

	*banlist = NULL;
	*last = NULL;
}

void banrec_append(banrec **list, banrec **last, banrec *entry)
{
	banrec *lastentry = *last;

	if(*list == NULL)
		*list = entry;
	else
		lastentry->next = entry;

	*last = entry;
	entry->next = NULL;
}

void banrec_delete(banrec **list, banrec **last, banrec *x)
{
	banrec *b, *lastchecked = NULL;

	b = *list;
	while(b != NULL)
	{
		if(b == x)
		{
			if(lastchecked == NULL)
			{
				*list = b->next;
				if(b == *last)
					*last = NULL;
			}
			else
			{
				lastchecked->next = b->next;
				if(b == *last)
					*last = lastchecked;
			}

			banrec_free(b);
			return;
		}

		lastchecked = b;
		b = b->next;
	}
}

void banrec_movetofront(banrec **list, banrec **last, banrec *lastchecked, banrec *b)
{
	if(lastchecked != NULL)
	{
		if(*last == b)
			*last = lastchecked;

		lastchecked->next = b->next;
		b->next = *list;
		*list = b;
	}
}

/****************************************************************************/

void banrec_init(banrec *b)
{
	/* initialize */
	b->ban = NULL;
	b->who_n = NULL;
	b->who_uh = NULL;
	b->time_set = 0L;
}

u_long banrec_sizeof(banrec *b)
{
	u_long	tot = 0L;

	tot += sizeof(banrec);
	tot += b->ban ? strlen(b->ban)+1 : 0L;
	tot += b->who_n ? strlen(b->who_n)+1 : 0L;
	tot += b->who_uh ? strlen(b->who_uh)+1 : 0L;

	return tot;
}

void banrec_free(banrec *b)
{
	/* Free the elements */
	xfree(b->ban);
	xfree(b->who_n);
	xfree(b->who_uh);

	/* Free this record */
	xfree(b);
}

void banrec_setban(banrec *b, char *ban)
{
	mstrcpy(&b->ban,ban);
}

void banrec_setwho_n(banrec *b, char *who_n)
{
	mstrcpy(&b->who_n,who_n);
}

void banrec_setwho_uh(banrec *b, char *who_uh)
{
	mstrcpy(&b->who_uh,who_uh);
}

void banrec_display(banrec *b)
{
	char set[TIMESHORTLEN];

	timet_to_ago_short(b->time_set,set);
	say("%4s %-9s %-30s %s",set,b->who_n,b->who_uh,b->ban);
}

banrec *banrec_ban(banrec **list, banrec **last, char *ban)
{
	banrec *b, *lastchecked = NULL;

	for(b=*list; b!=NULL; b=b->next)
	{
		if(isequal(b->ban,ban))
		{
			/* found it.  do a movetofront. */
			banrec_movetofront(list,last,lastchecked,b);

			return b;
		}
		lastchecked = b;
	}

	return NULL;
}

