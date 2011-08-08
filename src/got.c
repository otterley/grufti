/******************************************************************************
 * Steve Rider - stever@area23.org
 * Grufti - an interactive, service-oriented bot for IRC 
 *
 * got.c - Process server messages.
 *
 *****************************************************************************/
/* 28 April 97 */

#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <netinet/in.h>
#include "grufti.h"
#include "gruftiaux.h"
#include "got.h"
#include "server.h"
#include "misc.h"
#include "user.h"
#include "channel.h"
#include "dcc.h"
#include "log.h"
#include "net.h"
#include "response.h"
#include "command.h"
#include "notify.h"
#include "url.h"
#include "codes.h"
#include "banlist.h"
#include "mode.h"

extern cmdinfo_t cmdinfo;

/*
 * Prefixes, codes, and parameters are loaded by Server_parse() in server.c.
 * This eliminates the need for passing multiple arguments around, and
 * provides static storage space for storing the server messages to help
 * keep memory allocation and deallocation to a minimum.
 *
 * Server buffers are: from[], from_nuh[], from_n[], from_u[], from_h[], msg[], 
 *                     mode[], chname[], to[], code[], and sbuf[].
 * 
 * Not all may be loaded by Server_parse().  The from_x fields guarantee their
 * type, meaning that if <from> is not in the form of n!u@h, then from_nuh[]
 * will be an empty string.
 */

coderec_fixed code_tbl[] =
{
/*	name		function	*/
/*	============	=============	*/
        { "PRIVMSG",	gotmsg		},
        { "NOTICE",	gotnotice	},
        { "MODE",	gotmode		},
        { "JOIN",	gotjoin		},
        { "PART",	gotpart		},
        { "ERROR",	goterror	},
        { "PONG",	gotpong		},
        { "QUIT",	gotquit		},
        { "NICK",	gotnick		},
        { "KICK",	gotkick		},
        { "INVITE",	gotinvite	},
        { "TOPIC",	gottopic	},
        { "PING",	gotping		},
        { "001",	got001		},
        { "002",	got002		},
        { "003",	got003		},
        { "004",	got004		},
        { "212",	got212		},
        { "219",	got219		},
        { "250",	got250		},
        { "251",	got251		},
        { "252",	got252		},
        { "253",	got253		},
        { "254",	got254		},
        { "255",	got255		},
        { "265",	got265		},
        { "266",	got266		},
	{ "301",	got301		},
	{ "302",	got302		},
        { "315",	got315		},
        { "324",	got324		},
        { "329",	got329		},
        { "331",	got331		},
        { "332",	got332		},
        { "333",	got333		},
        { "352",	got352		},
        { "353",	got353		},
        { "366",	got366		},
        { "367",	got367		},
        { "368",	got368		},
        { "372",	got372		},
        { "375",	got375		},
        { "376",	got376		},
        { "377",	got377		},
	{ "401",	got401		},
        { "403",	got403		},
        { "405",	got405		},
        { "421",	got421		},
        { "432",	got432		},
        { "433",	got433		},
	{ "441",	got441		},
	{ "451",	got451		},
        { "465",	got465		},
        { "471",	got471		},
        { "473",	got473		},
        { "474",	got474		},
        { "475",	got475		},
        { "482",	got482		},
        /*
         :
         */
	{ NULL,		null(void(*)())	}
};

void gotmsg()
/*
 * PRIVMSG
 *
 * from: <from_nuh>
 *  msg: <chname> :<text>			(public chatter)
 *  msg: <chname> :\001ACTION <text>\001	(public CTCP action)
 *  msg: <to>     :<text>			(private message)
 *  msg: <to>     :\001ACTION <text>\001	(private CTCP action)
 *
 * 1) Check if we got a ctcp
 * 2) Pass the message to gotpublic(), gotctcp(), gotoper(), or gotprivate()
 */
{
	char *p, *p1;
	int ctcp = 0;

	context;
	/* Extract elements */
	str_element(Server->to,Server->msg,1);
	strcpy(Server->sbuf,Server->msg);
	split(NULL,Server->sbuf);
	fixcolon(Server->sbuf);

	context;
	/* CTCP? */
	p = strchr(Server->sbuf,1);
	while(p != NULL)
	{
		ctcp = 1;
		p++;
		p1 = p;
		while((*p != 1) && (*p != 0))
			p++;
		if(*p == 1)
		{
			*p = 0;
			strcpy(Server->msg,p1);
			strcpy(p1-1,p+1);
		}
		else
		{
			strcpy(Server->msg,p1);
			strcpy(p1-1,p);
		}
		gotctcp();
	
		p = strchr(Server->sbuf,1);
	}
	if(ctcp)
		return;

	context;
	strcpy(Server->msg,Server->sbuf);

	/* public message */
	if((Server->to[0] == '#') || (Server->to[0] == '&') || (Server->to[0] == '+'))
		gotpublic();

	/* msg from oper */
	else if((Server->to[0] == '$') || (strchr(Server->to,'.')!=NULL))
		gotoper();

	/* private message to the bot */
	else
		gotprivate();
}


void gotctcp()
/*
 * PRIVMSG (got ctcp)
 *
 * from: <from_nuh>
 *   to: <to/chname>
 *  msg: <code> <ctcp message>
 *
 * 1) Pass the ctcp message to gotdcc(), or gotunknown()
 */
{
	context;
	split(Server->code,Server->msg);

	context;
	/* Check codes */
	if(isequal(Server->code,"DCC"))
		gotdcc();
	else if(isequal(Server->code,"ACTION"))
		gotaction();
	else if(isequal(Server->code,"VERSION"))
		sendnotice(Server->from_n,"\001VERSION %s\001",Grufti->ctcp_version);
	else if(isequal(Server->code,"PING"))
	{
		if(strlen(Server->msg) <= 70)
			sendnotice(Server->from_n,"\001PING %s\001",Server->msg);
	}
	else if(isequal(Server->code,"USERINFO"))
	{
		sendnotice(Server->from_n,"\001USERINFO %s (%s) Idle %d second%s\001",Grufti->ctcp_finger,Grufti->botusername,time(NULL)-Server->last_write_time,(time(NULL)-Server->last_write_time)==1?"":"s");
	}
	else if(isequal(Server->code,"FINGER"))
	{
		sendnotice(Server->from_n,"\001FINGER %s (%s) Idle %d second%s\001",Grufti->ctcp_finger,Grufti->botusername,time(NULL)-Server->last_write_time,(time(NULL)-Server->last_write_time)==1?"":"s");
	}
	else if(isequal(Server->code,"CLIENTINFO"))
	{
		/*
		 * We only send CLIENTINFO info if no arguments follow
		 * (mirc4.0 and up don't respond to 'clientinfo finger')
		 */
		if(!Server->msg[0])
			sendnotice(Server->from_n,"\001CLIENTINFO %s\001",Grufti->ctcp_clientinfo);
	}
	else if(isequal(Server->code,"ERRMSG"))
	{
		if(strlen(Server->msg) <= 70)
			sendnotice(Server->from_n,"\001ERRMSG %s\001",Server->msg);
	}
	else if(isequal(Server->code,"TIME"))
	{
		char	t[TIMESHORTLEN];
		timet_to_date_long(time(NULL),t);
		sendnotice(Server->from_n,"\001TIME %s\001",t);
	}
	else if(isequal(Server->code,"ECHO"))
	{
		if(strlen(Server->msg) <= 70)
			sendnotice(Server->from_n,"\001ECHO %s\001",Server->msg);
	}
	else
		gotunknownctcp();
}


void gotunknownctcp()
/*
 * PRIVMSG (got ctcp)
 *
 * from: <from_nuh>
 *   to: <to/chname>
 *  msg: <code> <ctcp message>
 *
 */
{
	context;
	Log_write(LOG_DEBUG,"*","Unknown CTCP from %s, to %s, code %s, ctcp %s",Server->from,Server->to,Server->code,Server->msg);
}


void gotdcc()
/*
 * PRIVMSG (got ctcp "DCC")
 *
 * from: <from_nuh>
 *   to: <to>
 *  msg: <code> <param> <theirip> <theirport> [filesize]
 *
 * 1) Open a DCC connection to <theirip>:<theirport>
 */
{
	dccrec	*d;
	char	ip[SERVERLEN], param[SERVERLEN], theirport[SERVERLEN];
	char	filename[256], filepath[256], n1[50];
	FILE	*f;
	u_long	flags = 0L, length;

	context;
	/* Extract the parameters */
	split(Server->code,Server->msg);
	split(param,Server->msg);
	split(ip,Server->msg);
	split(theirport,Server->msg);

	context;
	/* We only accept dcc chat or send */
	if(!isequal(Server->code,"CHAT") && !isequal(Server->code,"SEND"))
		return;

	Log_write(LOG_CMD, "*", "%s %s - Got DCC %s request.", Server->from_n, Server->from_uh, Server->code);

	/* check if DCC SEND is allowed */
	if(isequal(Server->code, "SEND") && !Grufti->allow_dcc_send)
	{
		sendprivmsg(Server->from_n,"Not accepting DCC SEND requests.  Bye!");
		return;
	}

	context;
	if(DCC->num_dcc >= Grufti->max_dcc_connections)
	{
		/* send to user "too full" */
		Log_write(LOG_ERROR|LOG_MAIN,"*","DCC connection limit reached. (%d)",DCC->num_dcc);
		return;
	}

	context;
	/* Set connection type */
	flags |= DCC_CLIENT;
	if(isequal(Server->code,"CHAT"))
		flags |= CLIENT_IRC_CHAT;
	else
		flags |= CLIENT_XFER|STAT_XFER_IN;

	context;
	/* Do motd stuff on chat */
	if(flags & CLIENT_IRC_CHAT)
	{
		/* Add record and connect. */
		d = DCC_addnewconnection(Server->from_n,Server->from_uh,htonl(atoip(ip)),0,atoi(theirport),flags);
		if(d == NULL)
		{
			Log_write(LOG_MAIN,"*","%s %s - DCC connect failed. (couldn't connect to %s)",Server->from_n,Server->from_uh,ip);
			sendprivmsg(Server->from_n,"DCC connect failed.");
			return;
		}

		DCC_ondccchat(d);
		return;
	}

	/*
	 * From this point on we're dealing with a file xfer
	 */

	/* Set xfer parameters */
	sprintf(filepath,"%s/%s",Grufti->xfer_incoming,param);
	strcpy(filename,param);
	length = (u_long)atol(Server->msg);

	/* need a length (do clients do this?) */
	if(length == 0)
	{
		sendnotice(Server->from_n,"Your DCC send request does not include the filesize.  Sorry.");
		Log_write(LOG_CMD | LOG_DEBUG,"*","DCC send request from %s %s did not include filesize.",Server->from_n,Server->from_uh);
		return;
	}

	/* Too big? */
	if(length > (Grufti->xfer_maxfilesize * 1024))
	{
		sendnotice(Server->from_n,"Your file exceeds the maximum filesize I allow (%dk).  Sorry.",Grufti->xfer_maxfilesize);
		Log_write(LOG_CMD | LOG_DEBUG,"*","DCC send request exceeds maxfilesize. (Tried to send %dk).",length / 1024);
		return;
	}

	/* Make sure file doesn't already exist. */
	f = fopen(filepath,"r");
	if(f != NULL)
	{
		fclose(f);
		sendnotice(Server->from_n,"A file by that name already exists.  Sorry.");
		Log_write(LOG_CMD,"*","DCC send request matches an existing file \"%s\".",filename);
		return;
	}

	/* Make sure we can WRITE to that location */
	f = fopen(filepath,"w");
	if(f == NULL)
	{
		Log_write(LOG_ERROR,"*","%s %s - Unable to write file \"%s\"",Server->from_n,Server->from_n,filepath);
		sendprivmsg(Server->from_n,"Unable to write file \"%s\".  My problem, sorry.",filepath);
		return;
	}

	/* Everything's ok.  Add record and connect, and open file. */
	d = DCC_addnewconnection(Server->from_n,Server->from_uh,htonl(atoip(ip)),0,atoi(theirport),flags);
	if(d == NULL)
	{
		Log_write(LOG_CMD | LOG_DEBUG,"*","%s %s - DCC send failed.",Server->from_n,Server->from_uh);
		sendprivmsg(Server->from_n,"DCC send failed.");
		return;
	}

	dccrec_setfilename(d,filename);
	d->length = length;
	d->f = f;

	comma_number(n1,d->length);
	Log_write(LOG_CMD,"*","X%d %s %s - Receiving \"%s\" (%s bytes).",d->socket,d->nick,d->uh,d->filename,n1);
}


void gotaction()
/*
 * PRIVMSG (got ctcp "ACTION")
 *
 * from: <from_nuh>
 *   to: <to/chname>
 *  msg: <action text>
 *
 */
{
	chanrec *chan;
	membrec *m;

	/* private action to the bot.  ignore it. */
	if(!(Server->to[0] == '#') && !(Server->to[0] == '&') && !(Server->to[0] == '+'))
		return;

	chan = Channel_channel(Server->to);
	if(chan == NULL)
	{
		Log_write(LOG_DEBUG,"*","Oops.  What am i doing on %s?",Server->to);
		Server_write(PRIORITY_HIGH,"PART :%s",Server->to);
		return;
	}

	if(!Channel_verifychanactive(chan))
		return;

	m = Channel_member(chan,Server->from_n);
	if(m == NULL)
		m = Channel_addnewmember(chan,Server->from_n,Server->from_uh);

	/* log last time seen */
	User_gotactivity(m->user,m->account);
	m->last = time(NULL);

	Log_write(LOG_PUBLIC,chan->name,"* %s %s",Server->from_n,Server->msg);
	if(m->user->registered)
		m->user->public_chatter[today()] += strlen(Server->msg);

	/* Record any embedded URL's in the action */
	URL_checkforurl(Server->from_n,Server->from_uh,Server->msg);

	killpercentchars(Server->msg);

	/* Check for responses of type URL, stat, email, haveyouseen... */
	Response_checkresponses(Server->from_n,Server->to,Server->msg);

	/* Check for response list responses */
	Response_checkformatch(Server->from_n,Server->from_uh,Server->to,Server->msg);

	/* Update channel statistics */
	chan->chatter[thishour()] += strlen(Server->msg);
}


void gotpublic()
/*
 * PRIVMSG (got public chatter)
 *
 * from: <from_nuh>
 *   to: <chname>
 *  msg: <text>
 *
 * 1) Log it
 * 2) Update last seen
 */
{
	chanrec *chan;
	membrec *m;

	chan = Channel_channel(Server->to);
	if(chan == NULL)
	{
		Log_write(LOG_DEBUG,"*","Oops.  What am i doing on %s?",Server->to);
		Server_write(PRIORITY_HIGH,"PART :%s",Server->to);
		return;
	}

/*
	if(!Channel_verifychanactive(chan))
		return;
*/

	m = Channel_member(chan,Server->from_n);
	if(m == NULL)
		m = Channel_addnewmember(chan,Server->from_n,Server->from_uh);

	/* log time last seen */
	User_gotactivity(m->user,m->account);
	m->last = time(NULL);

	Log_write(LOG_PUBLIC,chan->name,"<%s> %s",Server->from_n,Server->msg);
	if(m->user->registered)
		m->user->public_chatter[today()] += strlen(Server->msg);

	/* Record any URL's embedded in public chatter */
	URL_checkforurl(Server->from_n,Server->from_uh,Server->msg);

	killpercentchars(Server->msg);

	/* Check for responses of type URL, stat, email, haveyouseen... */
	Response_checkresponses(Server->from_n,Server->to,Server->msg);

	/* Check if public message matches a response */
	Response_checkformatch(Server->from_n,Server->from_uh,Server->to,Server->msg);

	/* Update channel statistics */
	chan->chatter[thishour()] += strlen(Server->msg);
	context;
}


void gotprivate()
/*
 * PRIVMSG (got private message)
 *
 * from: <from_nuh>
 *  msg: <text>
 *
 * 1) Get user and account records.
 * 2) Update last seen
 * 3) Preset Command variables
 * 4) Invoke docommand()
 */
{
	chanrec *chan;
	userrec *u = NULL;
	acctrec *a = NULL;
	membrec *m;

	context;
	/* Fix it so we don't get empty fields */
	rmspace(Server->msg);

	/* Make a copy of the message */
	strcpy(Server->sbuf,Server->msg);

	context;
	/* We need user and account record.  First check channels... */
	for(chan=Channel->channels; chan!=NULL; chan=chan->next)
	{
		m = Channel_member(chan,Server->from_n);
		if(m != NULL)
		{
			/* Cool. found it. */
			u = m->user;
			a = m->account;
		}
	}

	context;
	if(u == NULL)
	{
		/* Damn. they're not on any channel. */
		u = User_user(Server->from_n);
		a = User_account(u,Server->from_n,Server->from_uh);
	}

	context;
	/* update last seen */
	User_gotactivity(u,a);

	context;
	/* Might be the first time we saw them */
	User_gotpossiblesignon(u,a);

	context;
	/* Command variables must be preset with user info. */
	strcpy(cmdinfo.from_n,Server->from_n);
	strcpy(cmdinfo.from_u,Server->from_u);
	strcpy(cmdinfo.from_uh,Server->from_uh);
	strcpy(cmdinfo.from_h,Server->from_h);
	strcpy(cmdinfo.buf,Server->msg);

	context;
	/* We're coming from a private message.  DCC is NULL. */
	cmdinfo.d = NULL;

	/* Set user and account pointers */
	cmdinfo.user = u;
	cmdinfo.account = a;

	if((strlen(Server->sbuf) >= 4) && strncasecmp(Server->sbuf,"pass",4) == 0)
		Log_write(LOG_CMD,"*","%s %s - <sent password>",Server->from_n,Server->from_uh);
	else
		Log_write(LOG_CMD,"*","%s %s - \"%s\"",Server->from_n,Server->from_uh,Server->sbuf);

	context;
	/* Now we're ready. */
	command_do();
}


void gotoper()
/*
 * PRIVMSG (got oper message)
 *
 * from: <from_nuh>
 *   to: <to>
 *  msg: <text>
 *
 */
{
	context;
	Log_write(LOG_PUBLIC,"*","[%s (%s) to %s] %s",Server->from_n,Server->from_uh,Server->to,Server->msg);
}


void gotopernotice()
/*
 * NOTICE (got oper notice)
 *
 * from: <from_nuh>
 *   to: <to>
 *  msg: <text>
 *
 */
{
	context;
	Log_write(LOG_PUBLIC,"*","-%s (%s) to %s- %s",Server->from_n,Server->from_uh,Server->to,Server->msg);

}


void gotnotice()
/*
 * NOTICE
 *
 * from: <from_nuh/Server>
 *  msg: <chname> :<text>			(public chatter)
 *  msg: <chname> :\001ACTION <text>\001	(public CTCP action)
 *  msg: <to>     :<text>			(private message)
 *  msg: <to>     :\001ACTION <text>\001	(private CTCP action)
 *  msg: <to>     :Highest connection count	(this sucks: from is server)
 *
 * 1) Check if we got a server notice (from_nuh won't be nuh)
 * 2) Check if we got a ctcp
 * 3) Pass to gotpublicnotice(), gotctcp(), gotopernotice(), or gotprivate()
 */
{
	char *p, *p1;
	int ctcp = 0;

	context;
	/* Extract elements */
	str_element(Server->to,Server->msg,1);
	strcpy(Server->sbuf,Server->msg);
	split(NULL,Server->sbuf);
	fixcolon(Server->sbuf);

	context;
	/* Check for 250 in hiding.  We don't need to log it. */
	if(!Server->from_nuh[0])
		return;

	context;
	/* CTCP? */
	p = strchr(Server->sbuf,1);
	while(p != NULL)
	{
		ctcp = 1;
		context;
		p++;
		p1 = p;
		while((*p != 1) && (*p != 0))
			p++;
		if(*p == 1)
		{
			*p = 0;
			strcpy(Server->msg,p1);
			strcpy(p1-1,p+1);
		}
		else
		{
			strcpy(Server->msg,p1);
			strcpy(p1-1,p);
		}
		gotctcp();
	
		p = strchr(Server->sbuf,1);
	}
	if(ctcp)
		return;

	context;
	strcpy(Server->msg,Server->sbuf);

	context;
	/* public notice */
	if((Server->to[0] == '#') || (Server->to[0] == '&') || (Server->to[0] == '+'))
	{
		gotpublicnotice();
		return;
	}

	context;
	/* msg from oper */
	if((Server->to[0] == '$') || (strchr(Server->to,'.')!=NULL))
	{
		gotopernotice();
		return;
	}

	context;
	/* Check for PONG from me. */
	if(Server_isfromme())
	{
		split(Server->sbuf,Server->msg);
		if(!isequal(Server->sbuf,"PONG"))
			return;

		/* PONG */
		gotpongfromme();
		return;
	}

	context;
	/* notice message to the bot.  (Don't parse as command) */
	Log_write(LOG_CMD,"*","NOTICE: %s %s - \"%s\"",Server->from_n,Server->from_uh,Server->msg);
}


void gotpongfromme()
/* 
 * my PONG
 *
 *  from: <from_me>
 *   msg: <time sent ping>
 */
{
	/* Ignore if we got a bogus pong */
	if(!Server_waitingforpong())
		return;

	context;
	/* Make sure pong time agrees with our sent time */
	if(Server->time_sent_ping != (time_t)atol(Server->msg))
	{
		Server->flags &= ~SERVER_WAITING_4_PONG;
		Log_write(LOG_DEBUG,"*","Got a pong, but times don't agree. %lu vs. %s",Server->time_sent_ping,Server->msg);
		return;
	}

	context;
	/* pass lagtime off to Server object. */
	Server_gotpong(time(NULL) - Server->time_sent_ping);
	return;
}


void gotpublicnotice()
/*
 * NOTICE (got public notice)
 *
 * from: <from_nuh>
 *   to: <chname>
 *  msg: <text>
 *
 * 1) update last seen
 * 2) log it
 */
{
	chanrec *chan;
	membrec *m;
	int	onchannel = 1;

	context;
	chan = Channel_channel(Server->to);
	if(chan == NULL)
	{
		Log_write(LOG_DEBUG,"*","Oops.  What am i doing on %s?",Server->to);
		Server_write(PRIORITY_HIGH,"PART :%s",Server->to);
		return;
	}

	context;
	if(!Channel_verifychanactive(chan))
		return;

	context;
	m = Channel_member(chan,Server->from_n);
	if(m == NULL)
	{
		m = Channel_addnewmember(chan,Server->from_n,Server->from_uh);
		onchannel = 0;
	}

	context;
	/* log time last seen */
	User_gotactivity(m->user,m->account);
	m->last = time(NULL);

	Log_write(LOG_PUBLIC,chan->name,"-%s:%s- %s",Server->from_n,Server->to,Server->msg);
	if(m->user->registered)
		m->user->public_chatter[today()] += strlen(Server->msg);

	/* We just pop them on the channel for a second to update stats... */
	if(!onchannel)
		Channel_removemember(chan,m);
}
	

void gotmode()
/* MODE
 * from: <from_nuh>
 *  msg: <to/chname> <mode>
 */
{
	context;
	mode_gotmode();
}


void gotjoin()
/*
 * from: <from_nuh>
 *  msg: :<chname>[^Gmodes]
 *
 */
{
	chanrec	*chan;
	membrec	*m;
	banlrec	*bl;
	char	*newmode = NULL, *p, greet[SERVERLEN];
	time_t	now = time(NULL);

	context;
	/* Extract the elements */
	fixcolon(Server->msg);
	str_element(Server->chname,Server->msg,1);

	/* ircd 2.9 sometimes does '#chname^Gmodes' when joining from split */
	p = strchr(Server->chname,7);
	if(p != NULL)
	{
		*p = 0;
		newmode = (++p);
	}

	context;
	/* Get channel information */
	chan = Channel_channel(Server->chname);
	if(chan == NULL)
	{
		Log_write(LOG_DEBUG,"*","Oops.  What am i doing on %s?",Server->chname);
		Server_write(PRIORITY_HIGH,"PART :%s",Server->chname);
		return;
	}

	context;
	/* I just joined.  Mark channel as inactive and query for info */
	if(Server_isfromme())
	{
		if(chan->flags & CHAN_ONCHAN)
		{
			Log_write(LOG_DEBUG,"*","Oops.  I just joined %s and CHAN_ONCHAN is already set.  Ignoring.",Server->chname);
		}
		chan->flags |= CHAN_ONCHAN;
		Channel_resetchaninfo(chan);

		/* Add myself to the channel */
		m = Channel_addnewmember(chan,Server->from_n,Server->from_uh);
		m->joined = now;
		m->last = now;

		/* Turn off takeover-in-progress var if auto-detect */
		if(Grufti->auto_detect_takeover)
			Grufti->takeover_in_progress = 0;
		
		Log_write(LOG_PUBLIC,Server->chname,"%s (%s) joined %s. (that's me)",Server->from_n,Server->from_uh,Server->chname);
		return;
	}

	context;
	/* Ignore JOIN if we're still waiting for channel info */
	if(!Channel_verifychanactive(chan))
		return;

	context;
	/* Check if newly joined member is already listed in our chan records */
	/* (if so, then they must have been split...) */
	m = Channel_member(chan,Server->from_n);
	if(m != NULL)
	{
		/* user is returning from a split */
		if(m->split)
		{
			char	time_split[TIMESHORTLEN];

			timet_to_ago_short(m->split,time_split);
			m->split = 0L;
			m->last = now;
			m->flags = 0;	/* server will declare flags shortly */

			if(newmode)
			{
				Log_write(LOG_PUBLIC,chan->name,"%s (%s) returned from split (with +%s, gone %s).",Server->from_n,Server->from_uh,newmode,time_split);
				/* do_embedded_mode(chan,nick,m,newmode); */
			}
			else
				Log_write(LOG_PUBLIC,chan->name,"%s (%s) returned from split (gone %s).",Server->from_n,Server->from_uh,time_split);
			return;
		}

		/* newly joined member wasn't split, and we got a join? */
		Log_write(LOG_DEBUG,"*","Oops.  Member just joined and we already show them as being on the channel.  (%s on %s)",Server->from_n,chan->name);
		Channel_removemember(chan,m);
	}
	

	context;
	/*
	 * New person on channel.  Add member to channel records.  User
	 * record is attached by addnewmember.
	 */
	m = Channel_addnewmember(chan,Server->from_n,Server->from_uh);
	m->joined = now;

	context;
	if(newmode)
		Log_write(LOG_PUBLIC,Server->chname,"%s (%s) joined %s (with +%s).",Server->from_n,Server->from_uh,Server->chname,newmode);
	else
		Log_write(LOG_PUBLIC,Server->chname,"%s (%s) joined %s.",Server->from_n,Server->from_uh,Server->chname);

	/* Kick/ban user if they're shitlisted */
	bl = Banlist_match(Server->from,chan);
	if(bl != NULL && Channel_ihaveops(chan))
	{
		Log_write(LOG_PUBLIC,Server->chname,"Kickbanned %s %s from %s (on banlist)",Server->from_n,Server->from_uh,Server->chname);
		Server_write(PRIORITY_HIGH,"MODE %s +b %s",Server->chname,bl->ban);
		Server_write(PRIORITY_HIGH,"KICK %s %s :%s",Server->chname,Server->from_n,Grufti->shitlist_message);
		bl->used++;
	}

	/* Check for signon */
	User_gotpossiblesignon(m->user,m->account);

	/* is account registered (are they the actual user) */
	if(m->user->registered && m->account->registered)
	{
		int	mnow, dnow, ynow;
		mdy_today(&mnow, &dnow, &ynow);

		if(m->user->bday[0] && (m->user->bday[0] == mnow && m->user->bday[1] == dnow))
		{
			/* Happy birthday */
			if((now - m->user->last_seen_onchan) > Grufti->wait_to_greet)
				sendaction(Server->chname,"waves to %s! *wave*  \"Happy Birthday %s!\"",m->nick,m->nick);
		}
		else if((m->user->flags & USER_GREET) && m->user->greets)
		{
			/* Have they been seen publicly lately? */
			if((now - m->user->last_seen_onchan) > Grufti->wait_to_greet)
			{
				char	action = 0;

				User_pickgreet(m->user,Server->sbuf);
				Response_parse_reply(greet,Server->sbuf,Server->from_n,Server->from_uh,Server->chname,&action);
				rmspace(greet);
				if(action)
					sendaction(Server->chname,greet);
				else
					sendprivmsg(Server->chname,greet);
			}
		}
	}

	/* Update channel statistics */
	if(m->user->registered)
		chan->joins_reg[thishour()]++;
	else
		chan->joins_nonreg[thishour()]++;

	chan->pop_prod += chan->pop_currentpop * (now - chan->pop_timestarted);
	chan->pop_currentpop = chan->num_members;
	chan->pop_timestarted = now;

	if(chan->num_members >= chan->max_population)
	{
		chan->max_population = chan->num_members;
		chan->max_population_time = now;
	}

	context;
	m->last = now;
}


void gotpart()
/*
 * PART
 *
 * from: <from_nuh>
 *  msg: <chname>
 *
 * 1) Kill member from channel
 * 2) Rejoin the channel if it was me that left
 * 3) Attempt nick change if my nick just left
 *
 */
{
	chanrec *chan;
	membrec *m;
	time_t	now = time(NULL);

	/* extract channel name */
	str_element(Server->chname,Server->msg,1);

	context;
	chan = Channel_channel(Server->chname);
	if(chan == NULL)
	{
		Log_write(LOG_DEBUG,"*","Oops.  What am i doing on %s?",Server->chname);
		Server_write(PRIORITY_HIGH,"PART :%s",Server->chname);
		return;
	}

	context;
	/* Doublecheck I'm actually active on this channel */
	if(!Channel_verifychanactive(chan))
		return;
		
	Log_write(LOG_PUBLIC,Server->chname,"%s (%s) left %s.",Server->from_n,Server->from_uh,Server->chname);

	/* I left the channel? */
	context;
	if(Server_isfromme())
	{
		chan->flags &= ~CHAN_ONCHAN;
		chan->flags &= ~CHAN_ACTIVE;
		chan->flags &= ~CHAN_PENDING;
		Channel_clearchaninfo(chan);
		Channel_join(chan);
		return;
	}

	context;
	/* bye bye */
	m = Channel_member(chan,Server->from_n);
	if(m == NULL)
	{
		Log_write(LOG_DEBUG,"*","Oops.  Attempted to delete %s on %s and they're not listed in my records.",Server->from_n,chan->name);
		return;
	}

	context;
	/* Check if the nick that left is mine */
	if(Server_usingaltnick())
		if(isequal(m->nick,Grufti->botnick))
			Server_nick(Grufti->botnick);

	context;
	Channel_removemember(chan,m);
	Channel_checkonlymeonchannel(chan);

	/* Update channel statistics */
	chan->pop_prod += chan->pop_currentpop * (now - chan->pop_timestarted);
	chan->pop_currentpop = chan->num_members;
	chan->pop_timestarted = now;

	if(chan->num_members <= chan->min_population)
	{
		chan->min_population = chan->num_members;
		chan->min_population_time = now;
	}
}


void goterror()
/*
 * from: NULL
 *  msg: :<error message>
 *
 */
{
	fixcolon(Server->msg);

	context;
	if(Server->cur == NULL)
		return;

	context;
	if(Server->cur->reason == NULL)
	{
		split(NULL,Server->msg);
		split(NULL,Server->msg);
		split(NULL,Server->msg);

		if(!Server->msg[0])
			return;

		/* Remove the parens... */
		if(Server->msg[0] == '(' && Server->msg[strlen(Server->msg) - 1] == ')')
		{
			Server->msg[strlen(Server->msg) - 1] = 0;
			Server_setreason(Server->msg+1);
		}
		else
			Server_setreason(Server->msg);
	}
}

void gotpong()
{ }

void got001()
/*
 * Welcome to IRC
 *
 * from: <server>
 *  msg: <to_n> :Welcome to the Internet Relay Chat Network...
 *
 * 1) Set current nick we finally logged in with
 * 2) Don't fix server name.  If we connected with it the first time, don't
 *    make it so we can't connect again.
 * 3) Mark that we registered successfully
 *
 */
{
	context;
	/* Set current nick */
	str_element(Server->sbuf,Server->msg,1);
	Server_setcurrentnick(Server->sbuf);

	context;
	/* Successful registration. */
	Server->flags |= SERVER_ACTIVEIRC;
	Server->cur->flags |= SERV_WAS_ACTIVEIRC;

	/* Set my mode */
	if(Grufti->mymode_on_IRC_join[0])
		Server_write(PRIORITY_HIGH,"MODE %s %s",Server->currentnick,Grufti->mymode_on_IRC_join);

	context;
	Log_write(LOG_MAIN,"*","Welcomed to IRC by %s.  Nick is \"%s\".",Server_name(),Server->currentnick);
}


void got002()
/* 
 * Your host is...
 *
 * from: <server>
 *  msg: <to> :Your host is <server>, running version x.x.x
 */
{ }


void got003()
/* 
 * Server created...
 * 
 * from: <server>
 *  msg: <to> :This server was created...
 */
{ }


void got004()
/*
 * Server version and supported modes
 *
 * from: <server>
 *  msg: <to> <servername> <version> <usermodes> <chanmodes>
 */
{ }


void got212()
/*
 * STATS
 *
 * from: <server>
 *  msg: <to> <commandname> <#> <#>
 */
{ }


void got219()
/*
 * End of /STATS report
 *
 * from: <server>
 *  msg: <to> * :End of /STATS report
 */
{ }


void got250()
/*
 * Highest connection count...
 *
 * from: <server>
 *  msg: <to> :Highest connection count x (x)
 */
{ }


void got251()
/*
 * There are x users and x invisible on x servers
 *
 * from: <server>
 *  msg: <to> :There are x users and x invisible on x servers
 */
{
	int found = 0;
	char num[SERVERLEN];

	context;
	split(NULL,Server->msg);
	fixcolon(Server->msg);

	context;
	/* Display if we're not waiting (we just connected) */
	if(Server_waitingforluser())
		Server->flags &= ~SERVER_WAITING_4_LUSER;
	else
		Log_write(LOG_MAIN,"*","%s",Server->msg);

	context;
	/* Remember # servers */
	while(!found)
	{
		split(num,Server->msg);
		if(strncasecmp("server",Server->msg,6) == 0)
			found = 1;
	}

	context;
	if(Server->cur == NULL)
		return;

	context;
	if(found)
		Server->cur->num_servers = atoi(num);

	if(Grufti->min_servers && (Server->cur->num_servers < Grufti->min_servers))
	{
		Log_write(LOG_MAIN,"*","Only %d server%s visible from %s.  Need %d.",Server->cur->num_servers,Server->cur->num_servers==1?"":"s",Server_name(),Grufti->min_servers);

		Server_leavingIRC();

		sprintf(Server->sbuf,"Need %d servers. (Had %d)",Grufti->min_servers,Server->cur->num_servers);
		Server_setreason(Server->sbuf);
		Server_disconnect();
	}

}


void got252()
/*
 * Operators online
 *
 * from: <server>
 *  msg: <to> <#opers> :IRC Operators online
 */
{ }


void got253()
{ }


void got254()
/*
 * Channels formed
 *
 * from: <server>
 *  msg: <to> <#chans> :channels formed
 */
{ }


void got255()
/*
 * local clients and servers
 *
 * from: <server>
 *  msg: <to> :I have x clients and x servers
 */
{ }


void got265()
/*
 * Current/max local users
 *
 * from: <server>
 *  msg: <to> :Current local users: x  Max: x
 */
{ }


void got266()
/*
 * Current/max global users
 *
 * from: <server>
 *  msg: <to> :Current global users: x  Max: x
 */
{ }


void got301()
/*
 * AWAY message
 *
 * from: <server>
 *  msg: <to> <nick> :<away message>
 */
{}


void got302()
/*
 * USERHOST reply
 * 
 * from: <server>
 *  msg: <to> :<nick>[*]=<+|-><userhost> [<nick>[*]=<+|-><userhost> ... ]
 *
 * When we get a 302, we look at the notify queue to see which nicks were
 * radiated.  We can iterate through both lists in order, looking for the
 * nicks that are or aren't there.
 */
{
	char	*p;
	userrec	*u;
	acctrec	*a;
	nickrec	*n;
	radirec	*r, *r_next;
	int	num_entries = 0;

	context;
	/* If we're not even waiting (internal error would cause this) */
	if(!Notify_waitingfor302())
	{
		Log_write(LOG_DEBUG,"*","Got 302 and we're not even waiting for one!");
		return;
	}

	split(Server->to,Server->msg);
	fixcolon(Server->msg);

	/* Get first element from 302 */
	split(Server->sbuf,Server->msg);
	if(Server->sbuf[0])
	{
#if 0
		p = strchr(Server->sbuf,'=');
#endif
		if ((p = strchr(Server->sbuf,'='))) { /* cph - problems on Xnet */

			/* '*' = oper */
			if(*(p-1) == '*')
				*(p-1) = 0;
			else
				*p = 0;

			/* We can use the server from buffers (it holds servername) */
			strcpy(Server->from_n,Server->sbuf);
			strcpy(Server->from_uh,p+2);
	
			/* Fix from */
			fixfrom(Server->from_uh);
		}
	}

	/* Iterate through the radiate queue */
	r = Notify->radiate_queue;
	while(r != NULL && num_entries < 5)
	{
		r_next = r->next;
		num_entries++;

		u = User_user(r->nick);
		n = User_nick(u,r->nick);

		/* might happen if user deletes account right before this */
		if(!u->registered)
		{
			Notify_deletenick(r);
			r = r_next;
			continue;
		}

		/* If next entry is blank, this nick is not online */
		if(!Server->sbuf[0])
		{
			if(n->flags & NICK_ONIRC)
				User_gotsignoff(u,n);
			Notify_deletenick(r);
			r = r_next;
			continue;
		}

		/* Check nick in queue against 302 nick */
		if(isequal(r->nick,Server->from_n))
		{
			/* ok this nick matches.  is the account registered? */
			a = User_account_dontmakenew(u,Server->from_n,Server->from_uh);

			/* we only check online status if registered */
			if(a != NULL && a->registered)
			{
				/* this nick is online and registered. */
				if(!(n->flags & NICK_ONIRC))
				{
					n->account = a;
					User_gotactivity(u,a);
					User_gotsignon(u,n);
				}
			}
			else
			{
				/* account is not registered anymore */
				if(n->flags & NICK_ONIRC)
					User_gotsignoff(u,n);
			}

			/* Move to next element (only when the nick matches) */
			split(Server->sbuf,Server->msg);
			if(Server->sbuf[0])
			{
				p = strchr(Server->sbuf,'=');
		
				/* '*' = oper */
				if(*(p-1) == '*')
					*(p-1) = 0;
				else
					*p = 0;
		
				/* We can use the server from buffers */
				strcpy(Server->from_n,Server->sbuf);
				strcpy(Server->from_uh,p+2);
		
				/* Fix from */
				fixfrom(Server->from_uh);
			}
		}
		else
		{
			/* This nick is not online. */
			if(n->flags & NICK_ONIRC)
				User_gotsignoff(u,n);
		}

		Notify_deletenick(r);
		r = r_next;
	}

	/* Mark that we're no longer waiting for 302 */
	Notify->flags &= ~NOTIFY_WAITING_FOR_302;

	/* if no more messages are in the queue */
	if(!Notify->num_nicks)
	{
		Notify->flags &= ~NOTIFY_CURRENTLY_RADIATING;
	}
}


void got315()
/*
 * End of /WHO
 *
 * from: <server>
 *  msg: <to> <chname> :End of /WHO list.
 */
{
	chanrec *chan;
	time_t	now = time(NULL);

	context;
	/* extract channel name */
	str_element(Server->chname,Server->msg,2);

	context;
	/* Might be end of WHO listing for user.  (we don't care about END) */
	if(Server->chname[0] != '#' && Server->chname[0] != '&')
		return;

	context;
	chan = Channel_channel(Server->chname);
	if(chan == NULL)
	{
		Log_write(LOG_DEBUG,"*","Oops.  What am i doing on %s?",Server->chname);
		Server_write(PRIORITY_HIGH,"PART :%s",Server->chname);
		return;
	}
	
	context;
	/* weird.  meant i wasn't waiting for any WHO info... */
	if(!(chan->flags & CHAN_PENDING))
	{
		Log_write(LOG_DEBUG,"*","Oops.  Got end of WHO info on %s but CHAN_PENDING is not set.",chan->name);
		return;
	}

	context;
	/* end of WHO, can consider myself officially on channel. */
	chan->flags |= CHAN_ACTIVE;
	chan->flags &= ~CHAN_PENDING;

	context;
	/* doublecheck that i actually _am_ on the channel */
	if(Channel_member(chan,Server->currentnick) == NULL)
	{
		Log_write(LOG_DEBUG,chan->name,"Oops.  Got end of WHO listing, but am not on %s.",Server->chname);

		chan->flags &= ~CHAN_ONCHAN;
		chan->flags &= ~CHAN_ACTIVE;
		chan->flags &= ~CHAN_PENDING;
		Channel_join(chan);
	}

	/* Reset channel population counters */
	chan->pop_currentpop = chan->num_members;
	chan->pop_timestarted = now;
	if(chan->num_members <= chan->min_population || chan->min_population_time == 0L)
	{
		chan->min_population = chan->num_members;
		chan->min_population_time = now;
	}
	if(chan->num_members >= chan->max_population || chan->max_population_time == 0L)
	{
		chan->max_population = chan->num_members;
		chan->max_population_time = now;
	}
	chan->joined = now;
	chan->pop_prod = 0;

	/* Mark that we've got all the WHO info */
	if(!(chan->modes & CHANMODE_GOTWHO))
	{
		chan->modes |= CHANMODE_GOTWHO;
		if(Channel_gotallchaninfo(chan))
			mode_pushchannelmodes(chan);
	}
}


void got324()
/*
 * Channel modes 
 *
 * from: <server>
 *  msg: <to> <chname> <mode> <params>
 *
 */
{
	chanrec *chan;

	split(NULL,Server->msg);
	split(Server->chname,Server->msg);
	split(Server->mode,Server->msg);
	strcpy(Server->sbuf,Server->msg);

	context;
	chan = Channel_channel(Server->chname);
	
	context;
	if(chan == NULL)
	{
		Log_write(LOG_DEBUG,"*","Oops.  What am i doing on %s?",Server->to);
		Server_write(PRIORITY_HIGH,"PART :%s",Server->to);
		return;
	}

	context;
	/* bans doesn't show up in 324... */
	mode_parsemode(chan,"","",Server->mode,Server->sbuf,1);

	/* Mark that we've got all modes */
	if(!(chan->modes & CHANMODE_GOTMODES))
	{
		chan->modes |= CHANMODE_GOTMODES;
		if(Channel_gotallchaninfo(chan))
			mode_pushchannelmodes(chan);
	}
}


void got329()
/*
 * When mode was set i think
 */
{}


void got331()
/*
 * No topic set
 *
 * from: <server>
 *  msg: <to> <chname> :No topic is set.
 */
{
	chanrec *chan;

	context;
	/* Extract elements from msg */
	str_element(Server->chname,Server->msg,2);

	context;
	/* Store topic in chan info */
	chan = Channel_channel(Server->chname);
	if(chan == NULL)
	{
		Log_write(LOG_DEBUG,"*","Oops.  What am i doing on %s?",Server->chname);
		Server_write(PRIORITY_HIGH,"PART :%s",Server->chname);
		return;
	}
	else
		chanrec_settopic(chan,"");

	context;
	/* Mark that we got topic info */
	if(!(chan->modes & CHANMODE_GOTTOPIC))
	{
		chan->modes |= CHANMODE_GOTTOPIC;
		if(Channel_gotallchaninfo(chan))
			mode_pushchannelmodes(chan);
	}
}


void got332()
/*
 * Topic on channel
 *
 * from: <server>
 *  msg: <to> <chname> :<topic>
 */
{
	chanrec *chan;

	context;
	/* Extract elements from msg */
	strcpy(Server->sbuf,Server->msg);
	str_element(Server->chname,Server->msg,2);
	split(NULL,Server->sbuf);
	split(NULL,Server->sbuf);
	fixcolon(Server->sbuf);

	context;
	/* Store topic in chan info */
	chan = Channel_channel(Server->chname);
	if(chan == NULL)
	{
		Log_write(LOG_DEBUG,"*","Oops.  What am i doing on %s?",Server->chname);
		Server_write(PRIORITY_HIGH,"PART :%s",Server->chname);
		return;
	}
	else
		chanrec_settopic(chan,Server->sbuf);

	/* Mark that we got topic info */
	if(!(chan->modes & CHANMODE_GOTTOPIC))
	{
		chan->modes |= CHANMODE_GOTTOPIC;
		if(Channel_gotallchaninfo(chan))
			mode_pushchannelmodes(chan);
	}
}


void got333()
/*
 * Who set topic and when
 *
 * from: <server>
 *  msg: <to> <chname> <nick> <time>
 */
{ }


void got352()
/*
 * WHO info
 *
 * from: <server>
 *  msg: <to> <chname> <user> <host> <server> <nick> <H@> <finger>
 *
 */
{
	chanrec *chan;
	membrec *m;

	context;
	/* store channel name and mode */
	str_element(Server->chname,Server->msg,2);
	str_element(Server->mode,Server->msg,7);
		
	context;
	/* Find channel for which the WHO info is describing */
	chan = Channel_channel(Server->chname);
	if(chan == NULL)
	{
		Log_write(LOG_DEBUG,"*","Oops.  What am i doing on %s?",Server->chname);
		Server_write(PRIORITY_HIGH,"PART :%s",Server->chname);
		return;
	}

	context;
	/* 'from' contains server info.  build 'from' with user's info. */
	str_element(Server->from_u, Server->msg,3);
	fixfrom(Server->from_u);
	str_element(Server->from_h, Server->msg,4);
	str_element(Server->from_n, Server->msg,6);
	sprintf(Server->from_uh,"%s@%s",Server->from_u,Server->from_h);
	sprintf(Server->from_nuh,"%s!%s",Server->from_n,Server->from_uh);

	context;
	/* Add user to the channel */
	m = Channel_member(chan,Server->from_n);
	context;
	if(m == NULL)
	{
		m = Channel_addnewmember(chan,Server->from_n,Server->from_uh);

		/* We don't know when they joined. */
		m->joined = 0L;

		/* We don't know when they last said something either */
		m->last = 0L;
	}

	context;
	/* check for ops */
	if(strchr(Server->mode,'@') != NULL)
		m->flags |= MEMBER_OPER;
	else
		m->flags &= ~MEMBER_OPER;
		
	context;
	/* check for voice */
	if(strchr(Server->mode,'+') != NULL)
		m->flags |= MEMBER_VOICE;
	else
		m->flags &= ~MEMBER_VOICE;

	context;
	/* check for away */
	if(strchr(Server->mode,'G') != NULL)
		m->flags |= MEMBER_AWAY;
	else
		m->flags &= ~MEMBER_AWAY;
}


void got353()
/*
 * NAMES list
 *
 * from: <server>
 *  msg: <to> <"="?> <chname> :[mode]<nick> [...]
 *
 */
{}


void got366()
/*
 * End of /NAMES list.
 *
 * from: <server>
 *  msg: <to> <chname> :End of /NAMES list
 */
{}


void got367()
/*
 * BAN list
 *
 * from: <server>
 *  msg: <to> <chname> <ban> <nuh> <time>
 *
 */
{
	chanrec	*chan;
	char	ban[UHOSTLEN], uh[UHOSTLEN], nick[NICKLEN], t[25];

	/* Extract elements */
	str_element(Server->to,Server->msg,1);
	str_element(Server->chname,Server->msg,2);
	str_element(ban,Server->msg,3);
	str_element(uh,Server->msg,4);
	str_element(t,Server->msg,5);

	splitc(nick,uh,'!');

	chan = Channel_channel(Server->chname);
	if(chan)
		Channel_addban(chan,ban,nick,uh,atol(t));
}


void got368()
/*
 * End of BAN list
 *
 * from: <server>
 *  msg: <to> <chname> :End of Channel Ban list
 *
 */
{
	chanrec	*chan;

	str_element(Server->chname,Server->msg,2);
	chan = Channel_channel(Server->chname);
	if(chan == NULL)
		return;

	if(!(chan->modes & CHANMODE_GOTBANS))
	{
		chan->modes |= CHANMODE_GOTBANS;
		if(Channel_gotallchaninfo(chan))
			mode_pushchannelmodes(chan);
	}
}


void got372()
/*
 * MOTD body
 *
 * from: <server>
 *  msg: <to_n> :<motd>
 *
 * 1) Append this line to motd
 */
{
	char *tmp, *p;

	context;
	if(Server->cur == NULL)
		return;

	context;
	/* Ignore everything before the : */
	p = strchr(Server->msg,':');
	if(p == NULL)
		p = Server->msg;
	else
		p++;

	context;
	/* Allocate memory for new line */
	if(Server->cur->motd != NULL)
	{
		tmp = (char *)xmalloc(strlen(p)+strlen(Server->cur->motd)+2);
		sprintf(tmp,"%s\n%s",Server->cur->motd,p);
	}
	else
	{
		tmp = (char *)xmalloc(strlen(p)+1);
		sprintf(tmp,"%s",p);
	}

	context;
	/* Free old memory, and reassign */
	xfree(Server->cur->motd);
	Server->cur->motd = tmp;
}


void got375()
/*
 * START OF MOTD
 *
 * from: <server>
 *  msg: <to_n> :<motd first line>
 *
 * 1) Free motd memory, and set as empty line
 */
{
	context;
	if(Server->cur == NULL)
		return;

	context;
	/* clear out the old motd for this server. */
	xfree(Server->cur->motd);

	context;
	Server->cur->motd = (char *)xmalloc(1);
	Server->cur->motd[0] = 0;
}


void got376()
{}


void got377()
/*
 * MOTD body
 *
 * from: <server>
 *  msg: <to_n> :<motd>
 *
 * (An ircd > 2.8.21 numeric)
 */
{
	got372();
}


void got401()
/*
 * ERROR: No such nick/channel
 *
 * from: <server>
 *  msg: <nickto> <nick_in_question> :No such nick/channel
 *
 * Happens occasionally when a nick has signed off and we haven't caught
 * it yet in our notify.  But we'll catch it now!
 *
 */
{
	userrec	*u;
	nickrec	*n;

	context;
	str_element(Server->from_n,Server->msg,2);

	context;
	u = User_user(Server->from_n);
	context;
	if(!u->registered)
		return;

	context;
	n = User_nick(u,Server->from_n);
	context;
	if(n == NULL)
		return;

	context;
	/* nick was not on previously.  ignore */
	if(!(n->flags & NICK_ONIRC))
		return;

	context;
	User_gotsignoff(u,n);

	context;
	n->account = NULL;
}


void got403()
/*
 * ERROR: NO SUCH CHANNEL
 * from: <server>
 *  msg: <to> <chname> :No such channel
 *
 * Treat it as an error. (it is an error.  an internal one.)
 */
{
	context;
	str_element(Server->chname,Server->msg,2);
	Log_write(LOG_DEBUG,"*","Oops.  Attempting operations on a nonexistant channel: %s",Server->chname);
}


void got405()
{}


void got421()
/*
 * ERROR: Unknown command
 *
 * from: <server>
 *  msg: <to> <commandname> :Unknown command
 */
{
	context;
	split(Server->sbuf,Server->msg);
	Log_write(LOG_DEBUG,"*","Command \"%s\" is not recognized by server.",Server->sbuf);
}


void got432()
/*
 * ERROR: Invalid nick
 *
 * There's no reason to have an invalid nick unless the server is just being
 * weird.  Probably an internal error.
 */
{
	context;
	Log_write(LOG_ERROR|LOG_MAIN,"*","Server says my nickname \"%s\" is inavlid.",Server->currentnick);
	fatal("Invalid nick.",0);
}


void got433()
/*
 * ERROR: SORRY THAT NICK IS IN USE
 *
 * from: <server>
 *  msg: <to> <attempted_nick> :Sorry, that Nickname is already in use
 *
 * Method of creating new nick:
 * - If it's possible, add a '_' to the nick.
 * - else replace the last non-'_' with a '_'  i.e. GruftiBot -> GruftiBo_
 * - if the nick is all '_''s, try the original nick with a random character.
 */
{
	int i;
	char newnick[NICKLEN];

	context;
	/* Extract attempted nick */
	str_element(Server->sbuf,Server->msg,2);

	context;
	/* trying to regain original nick?  resume current nick */
	if(isequal(Server->sbuf,Grufti->botnick) && !isequal(Server->currentnick,Grufti->botnick))
	{
		Server_nick(Server->currentnick);
		return;
	}
		
	context;
	/* nick is full 9 characters. */
	if(strlen(Server->sbuf) == 9)
	{
		strcpy(newnick,Server->sbuf);
		for(i=8; i>=0 && newnick[i] == '_'; i--)
			;

		if(i >= 0)
		{
			/* Replace last non '_' with '_' */
			newnick[i] = '_';
		}
		else
		{
			/* Nick is "_________".  Use random char in nick. */
			strcpy(newnick,Grufti->botnick);
			newnick[strlen(newnick)-1] = (char)(rand() % 10000);
		}

		Server_nick(newnick);
		return;
	}

	context;
	/* nick is shorter than 9 chars.  just append with '_' */
	sprintf(newnick,"%s_",Server->sbuf);

	context;
	Server_nick(newnick);
	return;
}


void got441()
/*
 * ERROR: User is not on that channel
 *
 * from: <server>
 *  msg: <to> <nick> <chname> :They aren't on that channel
 *
 * Probably results when we do an action, and the user PART's before the
 * action gets processed.  Check if they're on that channel.  If so, remove
 * them.
 */
{
	chanrec	*chan;
	membrec	*m;
	char	nick[NICKLEN];

	str_element(Server->chname,Server->msg,3);
	str_element(nick,Server->msg,2);

	/* Retrieve channel info */
	chan = Channel_channel(Server->chname);
	if(chan == NULL)
		return;

	/* Retrieve member info */
	m = Channel_member(chan,nick);
	if(m == NULL)
		return;

	/* uh oh.  We still show this member as being on the channel */
	Channel_removemember(chan,m);
}


void got451()
/*
 * ERROR: NOT REGISTERED
 *
 * from: <server>
 *  msg: <attempted_cmd> :Register first
 *
 *  We shouldn't get this, but if so, disconnect the server and try again.
 */
{
	context;
	Log_write(LOG_DEBUG,"*","Got 451 (not registered) on command: %s",cmdinfo.param);
	Server_setreason("Got 451");
	Server_disconnect();
}


void got465()
/*
 * ERROR: BANNED
 *
 * from: <server>
 *  msg: <to> :You are banned from this server: ^O<Reason>
 *
 */
{
	if(Server->cur != NULL)
		Server->cur->flags |= SERV_BANNED;

	context;
	split(NULL,Server->msg);
	fixcolon(Server->msg);
	if(strchr(Server->msg,':') != NULL)
		strcpy(Server->sbuf,strchr(Server->msg,':')+1);
	else
		strcpy(Server->sbuf,"");

	context;
	Log_write(LOG_MAIN,"*","Banned from server. (%s)",Server->sbuf);
}


void got471()
/* 
 * ERROR: Channel is FULL
 *
 * from: <server>
 *  msg: <to> <chname> :Sorry, cannot join channel.
 *
 */
{
	chanrec *chan;

	context;
	str_element(Server->chname,Server->msg,2);

	context;
	chan = Channel_channel(Server->chname);
	if(chan == NULL)
	{
		Log_write(LOG_DEBUG,"*","Got 471 for a channel i'm not supposed to be on (%s)",Server->chname);
		return;
	}

	context;
	/* Set reason */
	chanrec_setnojoin_reason(chan,"full");
}


void got473()
/*
 * ERROR: Channel is INVITE_ONLY
 *
 * from: <server>
 *  msg: <to> <chname> :Sorry, cannot join channel.
 *
 */
{
	chanrec *chan;

	context;
	str_element(Server->chname,Server->msg,2);

	context;
	chan = Channel_channel(Server->chname);
	if(chan == NULL)
	{
		Log_write(LOG_DEBUG,"*","Got 473 for a channel i'm not supposed to be on (%s)",Server->chname);
		return;
	}

	context;
	/* Set reason */
	chanrec_setnojoin_reason(chan,"invite-only");

	/* Set takeover-in-progress flag if auto-detect */
	if(Grufti->auto_detect_takeover)
		Grufti->takeover_in_progress = 1;
}


void got474()
/*
 * ERROR: BANNED from channel
 *
 * from: <server>
 *  msg: <to> <chname> :Sorry, cannot join channel.
 *
 */
{
	chanrec *chan;

	context;
	str_element(Server->chname,Server->msg,2);

	context;
	chan = Channel_channel(Server->chname);
	if(chan == NULL)
	{
		Log_write(LOG_DEBUG,"*","Got 474 for a channel i'm not supposed to be on (%s)",Server->chname);
		return;
	}

	context;
	/* Set reason */
	chanrec_setnojoin_reason(chan,"banned");

	/* Set takeover-in-progress flag if auto-detect */
	if(Grufti->auto_detect_takeover)
		Grufti->takeover_in_progress = 1;
}


void got475()
/*
 * ERROR: Channel has key (and you're not using it)
 *
 * from: <server>
 *  msg: <to> <chname> :Sorry, cannot join channel.
 *
 */
{
	chanrec *chan;

	context;
	str_element(Server->chname,Server->msg,2);

	context;
	chan = Channel_channel(Server->chname);
	if(chan == NULL)
	{
		Log_write(LOG_DEBUG,"*","Got 475 for a channel i'm not supposed to be on (%s)",Server->chname);
		return;
	}

	context;
	if(chan->key)
		sprintf(Server->sbuf,"bad key \"%s\"",chan->key);
	else
	{
		strcpy(Server->sbuf,"bad key");
		chanrec_setkey(chan,"");
	}

	/* Set reason */
	chanrec_setnojoin_reason(chan,Server->sbuf);

	context;
	/* Try the original key, if we have one, and if it's different. */
	if(chan->key_keep && chan->key_keep[0])
	{
		if(!isequal(chan->key,chan->key_keep))
		{
			chanrec_setkey(chan,chan->key_keep);
			Channel_join(chan);
		}
	}
}

void got482()
/*
 * ERROR: YOU'RE NOT CHANNEL OPERATOR
 * from: <server>
 *  msg: <to> <chname> :You're not channel operator
 *
 * Check and see if I have ops on that channel.  If so, remove them
 */
{
	chanrec	*chan;
	membrec	*m;
	char	nick[NICKLEN];

	str_element(nick,Server->msg,1);
	str_element(Server->chname,Server->msg,2);

	/* Make sure it's MY nick we're talking about */
	if(!isequal(nick,Server->currentnick))
		return;

	/* Retrieve channel info */
	chan = Channel_channel(Server->chname);
	if(chan == NULL)
		return;

	/* Retrieve member info */
	m = Channel_member(chan,nick);
	if(m == NULL)
		return;

	/* If i have ops, unflag it *shrug* */
	if(m->flags & MEMBER_OPER)
		m->flags &= ~MEMBER_OPER;

	return;
}


void gotquit()
/*
 * QUIT
 * from: <from_nuh>
 *  msg: :<leave message>
 *
 * Similar to gotpart, except it's global.
 */
{
	chanrec	*chan;
	membrec	*m;
	userrec	*u = NULL;
	acctrec	*a = NULL;
	int	split = 0;
	char	*p;
	time_t	now = time(NULL);

	context;
	/* store channel name and mode */
	str_element(Server->chname,Server->msg,1);
	fixcolon(Server->msg);

	context;
	/* check if quit message matches server split message */
	p = strchr(Server->msg,' ');
	if ((p != NULL) && (p == strrchr(Server->msg,' ')))
        {
		char *z1, *z2;
		*p = 0;
		z1 = strchr(p+1,'.');
		z2 = strchr(Server->msg,'.');
		if((z1 != NULL) && (z2 != NULL) && (*(z1+1) != 0) && (z1-1 != p) && (z2+1 != p) && (z2 != Server->msg))
                {
			/* server split, or else it looked like it anyway */
			/* (no harm in assuming) */
			split = 1;
		}
		else
			*p = ' ';
	}

	context;
	/* Update info for each channel */
	for(chan=Channel->channels; chan!=NULL; chan=chan->next)
	{
		m = Channel_member(chan,Server->from_n);
		if(m != NULL)
		{
			/* 'u' is the same regardless of channel */
			u = m->user;
			a = m->account;
			if(split)
			{
				m->split = time(NULL);
				Log_write(LOG_PUBLIC,chan->name,"%s (%s) got netsplit.",Server->from_n,Server->from_uh);
				Channel_checkonlymeonchannel(chan);
			}
			else
			{
				Log_write(LOG_PUBLIC,chan->name,"%s (%s) left irc: %s",Server->from_n,Server->from_uh,Server->msg);
				Channel_removemember(chan,m);
				Channel_checkonlymeonchannel(chan);
				/* Update channel statistics */
				chan->pop_prod += chan->pop_currentpop * (now - chan->pop_timestarted);
				chan->pop_currentpop = chan->num_members;
				chan->pop_timestarted = now;

	context;
				if(chan->num_members <= chan->min_population)
				{
					chan->min_population = chan->num_members;
					chan->min_population_time = now;
				}
			}
		}
	}

	context;
	/* Set signoff reason for user */
	if(u != NULL && a != NULL)	/* Better not be */
	{
		if(u->registered)
		{
	context;
			if(split)
				userrec_setsignoff(u,"Got netsplit");
			else
				userrec_setsignoff(u,Server->msg);
		}

	context;
		if(!split)
			User_gotpossiblesignoff(u,a);
	}

	/* I don't get a QUIT message if it was me that QUIT.  *sniff* */

	context;
	/* Check if the nick that left is mine */
	if(Server_usingaltnick())
		if(isequal(Server->from_n,Grufti->botnick))
			Server_nick(Grufti->botnick);

	context;
}


void gotnick()
/*
 * NICK change
 *
 * from: <from_nuh>
 *  msg: :<newnick>
 *
 * Nick change.  Is global for all channels...
 * 1) Update on each channel we're on
 * 2) If it was the bot, update our current nick
 */
{
	chanrec *chan;
	membrec *m = NULL;
	userrec	*u = NULL;
	banlrec	*bl;
	acctrec	*a = NULL;
	time_t	now = time(NULL), joined;
	char	howlong[TIMESHORTLEN], flags;

	fixcolon(Server->msg);

	context;
	/* Nickname change is global for all channels */
	for(chan=Channel->channels; chan!=NULL; chan=chan->next)
	{
		m = Channel_member(chan,Server->from_n);
		if(m != NULL)
		{
			Log_write(LOG_PUBLIC,chan->name,"Nick change on %s: %s --> %s",chan->name,Server->from_n,Server->msg);

			/* Remember flags, user, host, and time joined */
			flags = m->flags;
			joined = m->joined;
			u = m->user;
			a = m->account;

			/* Swap */
			Channel_removemember(chan,m);
			m = Channel_addnewmember(chan,Server->msg,Server->from_uh);

			/* Reset new entry to old settings */
			m->last = now;
			m->joined = joined;
			m->flags = flags;

			/* Kick/ban user if they're shitlisted */
			bl = Banlist_match(Server->from,chan);
			if(bl != NULL && Channel_ihaveops(chan))
			{
				Log_write(LOG_PUBLIC,Server->chname,"Kickbanned %s %s from %s (on banlist)",Server->from_n,Server->from_uh,Server->chname);
				Server_write(PRIORITY_HIGH,"MODE %s +b %s",Server->chname,bl->ban);
				Server_write(PRIORITY_HIGH,"KICK %s %s :%s",Server->chname,Server->from_n,Grufti->shitlist_message);
				bl->used++;
			}
		}
	}

	context;
	/* We know the old nick signed off, and the new nick signed on */
	if(a != NULL && u != NULL)
		User_gotpossiblesignoff(u,a);
	if(m != NULL)
		User_gotpossiblesignon(m->user,m->account);

	context;
	/* it was the bot who changed nicks */
	if(Server_isfromme())
	{
		Server_setcurrentnick(Server->msg);
		if(isequal(Server->currentnick,Grufti->botnick))
		{
			if(Server->started_altnick != 0L)
			{
				timet_to_elap_short(Server->started_altnick,howlong);
				Log_write(LOG_MAIN,"*","Regained nick.  (Took %s)",howlong);
				Server->started_altnick = 0L;
			}
		}
		else
		{
			if(Server->started_altnick == 0L)
				Server->started_altnick = time(NULL);
		}
	}
}


void gotkick()
/*
 * KICK
 *
 * from: <kicker_uh>
 *  msg: <chname> <who> :<message>
 *
 * 1) Kill member from channel
 * 2) Rejoin the channel if it was me that got kicked
 * 3) Attempt nick change if my nick just left
 *
 */
{
	chanrec *chan;
	membrec *m;
	userrec	*u, *u_check;
	time_t	now = time(NULL);

	context;
	/* extract channel name */
	split(Server->chname,Server->msg);
	split(Server->to,Server->msg);
	fixcolon(Server->msg);

	context;
	chan = Channel_channel(Server->chname);
	if(chan == NULL)
	{
		Log_write(LOG_DEBUG,"*","Oops.  What am i doing on %s?",Server->chname);
		Server_write(PRIORITY_HIGH,"PART :%s",Server->chname);
		return;
	}

	context;
	/* Doublecheck I'm actually active on this channel */
	if(!Channel_verifychanactive(chan))
		return;
		
	/* Increment kicks */
	u = User_user(Server->from_n);
	if(u->registered)
	{
		u_check = User_user(Server->to);
		if(u != u_check)
			u->kicks[today()]++;
	}

	context;
	if(Server->msg[0])
		Log_write(LOG_PUBLIC,Server->chname,"%s (%s) kicked %s off %s. (%s)",Server->from_n,Server->from_uh,Server->to,Server->chname,Server->msg);
	else
		Log_write(LOG_PUBLIC,Server->chname,"%s (%s) kicked %s off %s.",Server->from_n,Server->from_uh,Server->to,Server->chname);

	/* I left the channel? */
	if(isequal(Server->to,Server->currentnick))
	{
		chan->flags &= ~CHAN_ONCHAN;
		chan->flags &= ~CHAN_ACTIVE;
		chan->flags &= ~CHAN_PENDING;
		Channel_clearchaninfo(chan);
		Channel_join(chan);
		return;
	}

	context;
	/* bye bye */
	m = Channel_member(chan,Server->to);
	if(m == NULL)
	{
		Log_write(LOG_DEBUG,"*","Oops.  Attempted to delete %s on %s and they're not listed in my records.",Server->from_n,chan->name);
		return;
	}

	context;
	/* Check if the nick that left is mine */
	if(Server_usingaltnick())
		if(isequal(m->nick,Grufti->botnick))
			Server_nick(Grufti->botnick);

	context;
	Channel_removemember(chan,m);
	Channel_checkonlymeonchannel(chan);

	/* Update channel statistics */
	chan->pop_prod += chan->pop_currentpop * (now - chan->pop_timestarted);
	chan->pop_currentpop = chan->num_members;
	chan->pop_timestarted = now;

	if(chan->num_members <= chan->min_population)
	{
		chan->min_population = chan->num_members;
		chan->min_population_time = now;
	}
}


void gotinvite()
/*
 * INVITE
 *
 * from: <from_nuh>
 *  msg: <to> :<chname>
 *
 */
{
	chanrec	*chan;

	str_element(Server->chname,Server->msg,2);
	fixcolon(Server->chname);

	chan = Channel_channel(Server->chname);
	if(chan == NULL)
		return;

	Channel_join(chan);
}


void gottopic()
/*
 * TOPIC
 *
 * from: <from_nuh>
 *  msg: <chname> :<topic>
 *
 */
{
	chanrec *chan;

	context;
	/* Extract elements from msg */
	str_element(Server->chname,Server->msg,1);
	split(NULL,Server->msg);
	fixcolon(Server->msg);

	context;
	/* Store topic in chan info */
	chan = Channel_channel(Server->chname);
	if(chan == NULL)
	{
		Log_write(LOG_DEBUG,"*","Oops.  What am i doing on %s?",Server->chname);
		Server_write(PRIORITY_HIGH,"PART :%s",Server->chname);
		return;
	}
	else
		chanrec_settopic(chan,Server->msg);

	Log_write(LOG_PUBLIC,chan->name,"%s has changed the topic to %s",Server->from_n,Server->msg);

	context;
	/* If we did the topic, don't do ANY enforcing */
	if(!Channel_gotallchaninfo(chan) || Server_isfromme() || !(chan->flags & CHAN_FORCETOPIC))
		return;

	context;
	/* If we don't have ops, and CHANMODE_TOPIC is set, nothing we can do */
	if(!Channel_ihaveops(chan) && (chan->modes & CHANMODE_TOPIC))
		return;

	context;
	/* Reverse topic if it's not what we want */
	if(!isequal(chan->topic,chan->topic_keep))
	{
		Server_write(PRIORITY_HIGH,"TOPIC %s :%s",chan->name,chan->topic_keep);
		sendnotice(Server->from_n,"I'm enforcing the topic on %s, sorry!",chan->name);
	}
}

void gotping()
{
	context;

	/*
	 * Some servers send a PING before we've regiestered, and if we respond
	 * with PONG, the server yells at us that we're not connected.
	 */
	if(!Server_isactive())
		return;

	fixcolon(Server->msg);
	Server_write(PRIORITY_HIGH,"PONG :%s",Server->msg);
}

void gotunknown()
{
	context;
	Log_write(LOG_DEBUG,"*","Unknown server numeric: %s %s %s",Server->code,Server->from,Server->msg);
}
