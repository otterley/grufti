/******************************************************************************
 * Steve Rider - stever@area23.org
 * Grufti - an interactive, service-oriented bot for IRC 
 *
 * server.c - definitions for the main Server object.  Provides server setup
 *            and communication
 *
 *****************************************************************************/
/* 28 April 97 */

#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <sys/types.h>
#include <sys/time.h>
#include <time.h>
#include "grufti.h"
#include "gruftiaux.h"
#include "server.h"
#include "got.h"
#include "net.h"
#include "log.h"
#include "channel.h"
#include "codes.h"
#include "notify.h"

/*****************************************************************************
 *
 * Server object definitions.
 *
 ****************************************************************************/

void Server_new()
{
	int	i;

	/* already in use? */
	if(Server != NULL)
		Server_freeall();

	/* allocate memory */
	Server = (Server_ob *)xmalloc(sizeof(Server_ob));

	/* initialize */
	Server->sock = 0;
	Server->serverlist = NULL;
	Server->last = NULL;
	Server->num_servers = 0;
	Server->cur = NULL;
	Server->tojump = NULL;
	Server->trying_server = 0L;
	Server->started_altnick = 0L;
	Server->time_sent_ping = 0L;
	Server->time_sent_quit = 0L;
	Server->time_sent_luser = 0L;
	Server->time_noserverpossible = 0L;
	Server->time_tried_nick = 0L;
	Server->bytes_in = 0L;
	Server->bytes_out = 0L;
	Server->last_write_time = 0L;
	Server->lag = 0L;
	Server->currentnick[0] = 0;
	Server->flags = 0L;
	for(i=0; i<FLOOD_INTERVAL; i++)
		Server->q_sent[i] = 0;
	Server->q_sent_pos = 0;
	Server->queue = NULL;
	Server->lastqueue = NULL;
	Server->num_q_msgs = 0;

	Server->sbuf[0] = 0;
	Server->mode[0] = 0;
	Server->chname[0] = 0;
	Server->to[0] = 0;
	Server->code[0] = 0;
	Server->from[0] = 0;
	Server->from_nuh[0] = 0;
	Server->from_n[0] = 0;
	Server->from_u[0] = 0;
	Server->from_h[0] = 0;
	Server->from_uh[0] = 0;
	Server->msg[0] = 0;

	Server->send_buf[0] = 0;
}

u_long Server_sizeof()
{
	u_long	tot = 0L;
	servrec	*s;
	squeue	*q;

	tot += sizeof(Server_ob);

	for(s=Server->serverlist; s!=NULL; s=s->next)
		tot += servrec_sizeof(s);
	for(q=Server->queue; s!=NULL; s=s->next)
		tot += squeue_sizeof(q);

	return tot;
}

void Server_freeall()
{
	/* Free all server records */
	Server_freeallservers();

	/* Free the object */
	xfree(Server);
	Server = NULL;
}

/****************************************************************************/
/* object Server data-specific function definitions. */

void Server_display()
{
	servrec *serv;
	char lag[TIMESHORTLEN], retry[TIMESHORTLEN], unab[TIMESHORTLEN];
	char in[TIMESHORTLEN], out[TIMESHORTLEN], status[256];
	time_t serv_lag;


	/* Server is connected.  Display name, status, nick, and lag. */
	if(Server->flags & SERVER_CONNECTED)
	{
		say("Server: %s %d",Server_name(),Server_port());
		if(!(Server->flags & SERVER_ACTIVITY))
			sprintf(status,"Connected, waiting for signs of life...");
		else if(!(Server->flags & SERVER_ACTIVEIRC))
			sprintf(status,"Connected, waiting to be welcomed to IRC...");
		else if(Server->flags & SERVER_QUIT_IN_PROGRESS)
			sprintf(status,"Waiting for server to shut down...");
		else
			sprintf(status,"Connected and active on IRC.");

		say("Status: %s",status);

		/* If pong is pending, calculate lag from that */
		if(Server_waitingforpong())
			serv_lag = time(NULL) - Server->time_sent_ping;
		else
			serv_lag = Server->lag;
		time_to_elap_short(serv_lag,lag);
		say("Current nick: %-30s  Current lag: %s",Server->currentnick,lag);
	}

	/*
	 * If no servers could be connected, display when a retry will be
	 * made, or if we're trying a server right now.
	 *
	 * If IRC is NO, display that.
	 */
	else
	{
		say("Server: <not connected>");

		if(Grufti->irc == OFF)
			say("IRC is set to OFF.");
		else if(Server->num_servers)
		{
			if(Server->time_noserverpossible)
			{
				timet_to_ago_short(Server->time_noserverpossible,unab);
				time_to_elap_short(Grufti->serverpossible_timeout - (time(NULL) - Server->time_noserverpossible),retry);
				if(retry[0] == '-')
					strcpy(retry,"0s");
				say("Unable to connect to any servers for %s (retry in %s)",unab,retry);
			}
			else
				say("Trying %s:%d...",Server_name(),Server_port());
		}
	}
	
	bytes_to_kb_short(Server->bytes_in,in);
	bytes_to_kb_short(Server->bytes_out,out);
	say("Total in/out: %s in, %s out",in,out);
	say("");
	if(Server->serverlist)
	{
		if(Server->flags & SERVER_SHOW_REASONS)
		{
			say("  Server                        Reason for disconnect");
			say("------------------------------- ---------------------------------");
			for(serv=Server->serverlist; serv!=NULL; serv=serv->next)
				servrec_display(serv);
			say("------------------------------- ---------------------------------");
		}
		else
		{
			say("  Server                         Port Con'd AvLag    In   Out Net");
			say("------------------------------- ----- ----- ----- ----- ----- ---");
			for(serv=Server->serverlist; serv!=NULL; serv=serv->next)
				servrec_display(serv);
			say("------------------------------- ----- ----- ----- ----- ----- ---");
		}
	}
	else
		say("No servers configured.");
}

/****************************************************************************/
/* low-level Server structure function definitions. */

void Server_appendserver(servrec *serv)
{
	servrec_append(&Server->serverlist, &Server->last, serv);
	Server->num_servers++;
}

void Server_deleteserver(servrec *serv)
{
	servrec_delete(&Server->serverlist, &Server->last, serv);
	Server->num_servers--;
}

void Server_freeallservers(servrec *serv)
{
	servrec_freeall(&Server->serverlist, &Server->last);
	Server->num_servers = 0;
}

void Server_appendq(squeue *q)
{
	squeue_append(&Server->queue, &Server->lastqueue, q);
	Server->num_q_msgs++;
}

void Server_insertqatfront(squeue *q)
{
	squeue_insertatfront(&Server->queue, &Server->lastqueue, q);
	Server->num_q_msgs++;
}

void Server_deleteq(squeue *q)
{
	squeue_delete(&Server->queue, &Server->lastqueue, q);
	Server->num_q_msgs--;
}

void Server_freeallqs()
{
	squeue_freeall(&Server->queue, &Server->lastqueue);
	Server->num_q_msgs = 0;
}

/****************************************************************************/
/* object Server high-level implementation-specific function declarations. */

servrec *Server_addserver(char *servername, int port)
{
	servrec *serv;

	/* Create a new node */
	serv = servrec_new();

	/* Copy the data */
	servrec_setname(serv,servername);
	serv->port = port;
	serv->flags |= SERV_NEVER_TRIED;

	/* Append it to the serverlist */
	Server_appendserver(serv);

	return serv;
}

servrec *Server_nextserver()
{
	servrec *serv;

	if(Server->cur == NULL)
		serv = Server->serverlist;
	else
	{
		serv = Server->cur->next;
		if(serv == NULL)
			serv = Server->serverlist;
	}

	return serv;
}

void Server_disconnect()
{
	time_t dur = 0L;

	if(Server->cur == NULL)
	{
		Log_write(LOG_ERROR,"*","Attempted disconnect on an empty server.");
		return;
	}

	/* Close the connection (sock might still hold error value) */
	if(Server->sock >= 0)
		net_disconnect(Server->sock);

	Log_write(LOG_MAIN,"*","Disconnected IRC server %s:%d.",Server_name(),Server_port());

	Server->cur->disconnect_time = time(NULL);
	if(Server->cur->first_activity)
	{
		Server->cur->connected += (time(NULL) - Server->cur->first_activity);
		Server->cur->first_activity = 0L;
	}

	/* Calculate last ping average */
	if(Server->cur->startlag != Server->cur->lastlag)
	{
		dur = Server->cur->p_lasttime - Server->cur->p_starttime;
		Server->cur->p_dur += dur;
		Server->cur->lag_x_dur += (dur * Server->cur->startlag);
		Server->cur->startlag = Server->cur->lastlag;
		Server->cur->p_starttime = Server->cur->p_lasttime;
	}

	/* Finish up ping time. (now until last starttime) */
	dur = time(NULL) - Server->cur->p_starttime;
	Server->cur->p_dur += dur;
	Server->cur->lag_x_dur += (dur * Server->cur->startlag);
	Server->cur->startlag = 0L;
	Server->cur->lastlag = 0L;
	Server->cur->p_starttime = 0L;
	Server->cur->p_lasttime = 0L;

	/* Is server flagged for delete? */
	if(Server->flags & SERVER_DELETE_ON_DISCON)
	{
		/*
		 * When we set DELETE_ON_DISCON, we also set ->tojump so we
		 * don't have to worry about telling Connect() where it's 
		 * going to connect to next...
		 */
		Server_deleteserver(Server->cur);
	}

	/* Finally, reset other stuff */
	Server->sock = -1;
	Server->trying_server = 0L;
	Server->started_altnick = 0L;
	Server->time_sent_ping = 0L;
	Server->time_sent_quit = 0L;
	Server->last_write_time = 0L;
	Server->lag = 0;
	Server->currentnick[0] = 0;
	Server->flags = 0L;

	Notify->last_radiated = 0L;
	Notify->flags &= ~NOTIFY_PENDING;
}

void Server_connect()
/*
 * This looks hairy up front because a lot of things may happen on a connect.
 * Since server QUITS are all handled by async IO, here is usually the first
 * thing that happens after a disconnect.  So we check if the disconnect was
 * caused by a JUMP, or maybe a delete, etc.  We have to take care not to
 * delete a server while activity is still going on (ie Server->cur is still
 * pointing to the soon-to-be-deleted server record...)
 */
{
	if(Server_isconnected())
	{
		Log_write(LOG_ERROR,"*","Oops.  Attempted connect to IRC server %s:%d while already connected",Server_name(),Server_port());
		return;
	}

	if(Server->flags & SERVER_QUIT_IN_PROGRESS)
	{
		Log_write(LOG_ERROR,"*","Oops.  We're attempting to connect while a QUIT is still in progress...");
		return;
	}

	/* Any servers? */
	if(!Server->num_servers)
		return;

	/* Do we have a jump pending?? */
	if(Server->tojump != NULL)
	{
		/* yes! */
		Server->cur = Server->tojump;
		Server->tojump = NULL;
	}
	else
	{
		/* Point cur to next server */
		Server->cur = Server_nextserver();
	}

	/*
	 * Mark that server has had a connect attempted.  Wait to see if it
	 * can get active IRC.
	 */
	Server->cur->flags |= SERV_CONNECT_ATTEMPTED;
	Server->cur->flags &= ~SERV_CONNECT_FAILED;
	Server->cur->flags &= ~SERV_NEVER_TRIED;
	Server->cur->flags &= ~SERV_WAS_ACTIVEIRC;
	Server->cur->flags &= ~SERV_BANNED;

	/* Mark beginning of trying server */
	Server->trying_server = time(NULL);
	xfree(Server->cur->reason);
	Server->cur->reason = NULL;

	Log_write(LOG_MAIN,"*","Trying IRC server %s:%d...",Server_name(),Server_port());
	Server->sock = net_connect(Server_name(),Server_port(), 0, 0);

	if(Server->sock < 0)
	{
		if(Server->sock == -2)
			strcpy(Server->sbuf,"DNS lookup failed");
		else
			strcpy(Server->sbuf,net_errstr());
		Log_write(LOG_MAIN,"*","Failed connect to %s:%d. (%s)",Server_name(),Server_port(),Server->sbuf);
		Server_setreason(Server->sbuf);
		Server->cur->flags |= SERV_CONNECT_FAILED;
		Server_disconnect();
	}
	else
	{
		/* Connected to a server socket.  Does not mean activity */
		Server->flags |= SERVER_CONNECTED;

		/* Execute IRC login */
		Server_write(PRIORITY_HIGH,"NICK %s",Grufti->botnick);
		Server_setcurrentnick(Grufti->botnick);
		Server_write(PRIORITY_HIGH,"USER %s %s %s :%s",Grufti->botusername,Grufti->bothostname,Server_name(),Grufti->botrealname);
	}

	/* wait for connect results later... */
}

void Server_makeunattempted(servrec *serv)
{
	serv->flags &= ~SERV_CONNECT_ATTEMPTED;
	serv->flags &= ~SERV_WAS_ACTIVEIRC;
}
	
void Server_makeallunattempted()
{
	servrec *serv;

	for(serv=Server->serverlist; serv!=NULL; serv=serv->next)
		Server_makeunattempted(serv);
}

void Server_noserverpossible()
{
	/* we know! */
	if(Server->time_noserverpossible)
		return;

	/* All servers have had attempts, and none were active IRC. */
	Server->time_noserverpossible = time(NULL);
	Log_write(LOG_MAIN,"*","Unable to connect with any servers.  Waiting %ds...",Grufti->serverpossible_timeout);
}

int Server_isanyserverpossible()
{
	servrec *serv;

	/* If a time has been set, we already know no server is possible. */
	if(Server->time_noserverpossible != 0L)
		return 0;

	for(serv=Server->serverlist; serv!=NULL; serv=serv->next)
	{
		/* If no attempt has been made */
		if(!(serv->flags & SERV_CONNECT_ATTEMPTED))
			return 1;

		/* If server was active last time we saw it */
		if(serv->flags & SERV_WAS_ACTIVEIRC)
			return 1;
	}

	return 0;
}
			
void Server_gotactivity(char *buf)
{
	int bytes;
	coderec *code;

	/* Nothing? bail */
	if(!buf[0])
		return;

	if(Server->cur == NULL)
	{
		Log_write(LOG_DEBUG,"*","Shit.  Got activity on a server but cur is NULL!");
		return;
	}

	/* First time we've seen activity from the server? */
	if(Server->trying_server)
	{
		Log_write(LOG_MAIN,"*","Connected to IRC server %s.  Waiting for welcome...",Server_name());
		Server->trying_server = 0L;
		Server->flags |= SERVER_ACTIVITY;
		Server->cur->first_activity = time(NULL);
		xfree(Server->cur->reason);
		Server->cur->reason = NULL;
	}

	/* Update stats */
	bytes = strlen(buf);
	Server->cur->bytes_in += (u_long)bytes;
	Server->bytes_in += (u_long)bytes;

	/* extract elements of server message */
	Server_parsemsg(buf);

#ifdef RAW_LOGGING
	Log_write(LOG_RAW,"*","%s %s %s",Server->code,Server->from,Server->msg);
#endif

	/* Check codes */
	code = Codes_code(Server->code);
	if(code != NULL)
	{
		code->function();
		code->accessed++;
	}
	else
		gotunknown();
}

void Server_parsemsg(char *in)
{
	char *p, *p1, *p2;

	Server_resetbuffers();

	if(in[0]==':')
	{
		strcpy(in,&in[1]);

		/* got code, but nothing else */
		p = strchr(in,' ');
		if (p == NULL)
		{
			strcpy(Server->code,in);
			return;
		}

		strcpy(Server->msg,p+1);

		/* Set 'in' at first space to NULL.  It's left with 'from' */
		*p=0;

		strcpy(Server->from,in);

		/* Create from_nuh */
		strcpy(Server->from_nuh,in);

		/* Create from_n */
		strcpy(Server->from_n,Server->from_nuh);
		p1 = strchr(Server->from_n,'!');
		if(p1 == NULL)
		{
			/* No '!' in from, so no n!u@h.  maybe from is uh? */
			Server->from_n[0] = 0;
			Server->from_nuh[0] = 0;
			strcpy(Server->from_uh,Server->from);
		}
		else
		{
			/* Set 'from_n' at '!' to NULL.  It's left with nick */
			*p1 = 0;
			strcpy(Server->from_uh,p1+1);
		}

		/* Remove leading '~' (or '+'...) from from_uh... */
		fixfrom(Server->from_uh);

		/* Create from_u */
		strcpy(Server->from_u,Server->from_uh);
		p2 = strchr(Server->from_u,'@');
		if(p2 == NULL)
		{
			/* no '@'?  then from_uh is not uh.  u=0,uh must be h */
			Server->from_u[0] = 0;
			strcpy(Server->from_h,Server->from_uh);
			Server->from_uh[0] = 0;
		}
		else
		{
			/* Set 'from_u' at '@' to NULL.  it's left with user */
			*p2 = 0;
			strcpy(Server->from_h,strchr(Server->from_uh,'@')+1);
		}

		*p = ' ';
		p++;
		strcpy(in,p);
	}

	/* Any arguments after the code? */
	p = (char *)strchr(in,' ');
	if(p == NULL)
	{	
		strcpy(Server->code,in);
		Server->msg[0] = 0;
		return;
	}

	/* Split the msg off the code */
	*p = 0;
	strcpy(Server->code,in);
	*p = ' ';
	strcpy(Server->msg,p+1);
}

void Server_resetbuffers()
{
	Server->sbuf[0] = 0;
	Server->chname[0] = 0;
	Server->to[0] = 0;
	Server->mode[0] = 0;
	Server->code[0] = 0;
	Server->from[0] = 0;
	Server->from_nuh[0] = 0;
	Server->from_n[0] = 0;
	Server->from_u[0] = 0;
	Server->from_h[0] = 0;
	Server->from_uh[0] = 0;
	Server->msg[0] = 0;
}

void Server_write(char priority, char *format, ...)
{
	va_list va;
	int bytes;

	Server->send_buf[0] = 0;
	va_start(va,format);
	vsprintf(Server->send_buf,format,va);
	va_end(va);

	strcat(Server->send_buf,"\n");

	if(Server->cur == NULL)
	{
		Log_write(LOG_DEBUG,"*","Oops.  Attempted Server_write(\"%s\") while cur is NULL.",Server->send_buf);
		return;
	}

	if(!Server_isconnected())
	{
		Log_write(LOG_DEBUG,"*","Oops.  Attempted Server_write(\"%s\") to %s while no activity.",Server->send_buf,Server_name());
		return;
	}

	/* Queue has something in it.  Have to push regardless */
	if(!Server_isqueueempty())
	{
		Server_pushqueue(Server->send_buf,priority);
		return;
	}
	
	/* flood protection */
	bytes = strlen(Server->send_buf);
	if((bytes + Server_bytesinfloodinterval()) > FLOOD_SIZE)
	{
		/* sending this risks excess flood.  Queue it. */
		Server_pushqueue(Server->send_buf,priority);
		return;
	}

	/* no problem sending this line.  Update q_sent */
	Server->q_sent[Server->q_sent_pos] += bytes;

	/* Update stats */
	Server->cur->bytes_out += (u_long)bytes;
	Server->bytes_out += (u_long)bytes;

	/* Log last time written. */
	Server->last_write_time = time(NULL);

	/* Write */
	net_sockputs(Server->sock,Server->send_buf, bytes);

#ifdef RAW_LOGGING
	Log_write(LOG_RAW,"*","--> server: %s",Server->send_buf);
#endif
}

char *Server_name()
{
	if(Server->cur != NULL)
		return Server->cur->name;
	else
	{
		strcpy(Server->sbuf,"No server");
		return Server->sbuf;
	}
}

int Server_port()
{
	if(Server->cur != NULL)
		return Server->cur->port;
	else
		return 0;
}

int Server_isfromme()
{
	if(isequal(Server->from_n,Server->currentnick))
		
		return 1;
	else
		return 0;
}

void Server_checkfortimeout()
{
	time_t now = time(NULL);

	/* If no servers are possible, see if we've waited long enough */
	if(Server->time_noserverpossible)
	{
		if((now - Server->time_noserverpossible) > Grufti->serverpossible_timeout)
		{
			Server_makeallunattempted();
			Server->time_noserverpossible = 0L;
		}
	}

	if(!Server_isconnected())
		return;

	/*
	 * Timeouts past this point need to
	 *   a) Set a reason,
	 *   b) Execute Server_leavingIRC() if IRC is active,
	 *   c) Disconnect
	 */

	/* Now check for timeout trying server */
	if(Server->trying_server)
	{
		if((now - Server->trying_server) > Grufti->server_timeout)
		{
			char timedout[TIMESHORTLEN];

			Log_write(LOG_MAIN,"*","Timed out connecting to %s:%d.",Server_name(),Server_port());
			time_to_elap_short(Grufti->server_timeout,timedout);
			sprintf(Server->sbuf,"Timed out connecting (%s)",timedout);
			Server_setreason(Server->sbuf);
			Server->cur->flags |= SERV_CONNECT_FAILED;

			Server_disconnect();
		}
	}

	/* Now check for server lag (haven't got a pong in some time) */
	if(Server_waitingforpong())
	{
		/* still waiting for pong.  how long have we waited? */
		if((now - Server->time_sent_ping) > Grufti->server_ping_timeout)
		{
			char pingedout[TIMESHORTLEN];

			/* We've waited too long.  move to next server. */
			time_to_elap_short(Grufti->server_ping_timeout,pingedout);
			Log_write(LOG_MAIN,"*","Server %s:%d pinged out. (no response in %s)",Server_name(),Server_port(),pingedout);

			/*
			 * Normally we'd QUIT, but that wouldn't do any good
			 * if we're disconnecting because of ping timeout...
			 *
			 * Since we're not QUIT'ting, we're not going to
			 * generate an EOF.  Signal that we're leaving IRC.
			 */

			Server_leavingIRC();

			sprintf(Server->sbuf,"Pinged out (after %s)",pingedout);
			Server_setreason(Server->sbuf);
			Server_disconnect();
		}
	}
	if(Server->flags & SERVER_QUIT_IN_PROGRESS)
	{
		/* We sent a quit and we're still waiting.  how long? */
		if((now - Server->time_sent_quit) > Grufti->server_quit_timeout)
		{
			char timeout[TIMESHORTLEN];

			/* It's been too long.  kill it. */
			time_to_elap_short(Grufti->server_quit_timeout,timeout);

			Log_write(LOG_MAIN,"*","Server %s:%d pinged out (no QUIT in %s)",Server_name(),Server_port(),timeout);

			sprintf(Server->sbuf,"Pinged out (no QUIT in %s)",timeout);
			Server_leavingIRC();

			Server_setreason(Server->sbuf);
			Server_disconnect();
		}
	}

	if(Notify_waitingfor302())
	{
		if((now - Notify->last_radiated_msg) > Grufti->radiate_timeout)
		{
			/* didn't get a 302 in a long time. reset everything */
			Notify_dumpqueue();
		}
	}
}

/* A connected server does not necessarily mean there is activity. */
int Server_isconnected()
{
	if(Server->flags & SERVER_CONNECTED)
		return 1;
	else
		return 0;
}

/* A server marked active may not show us being active on IRC */
int Server_isactive()
{
	if(Server->flags & SERVER_ACTIVITY)
		return 1;
	else
		return 0;
}

/* USER and NICK have been ok'd by the server.  effectively 'logged in'. */
int Server_isactiveIRC()
{
	if(Server->flags & SERVER_ACTIVEIRC)
		return 1;
	else
		return 0;
}

void Server_gotEOF()
/*
 * TODO: If we really got an EOF, then net_errstr() won't be valid...
 * maybe handle the errcode in the argument?
 */
{
	strcpy(Server->sbuf, net_errstr());
	if(Server->cur == NULL)
	{
		Log_write(LOG_DEBUG,"*","Got EOF but Server->cur == NULL");
		return;
	}

	/* connected, but found no activity */
	if(!Server_isactive())
	{
		Log_write(LOG_MAIN,"*","Failed connect to %s:%d. (%s)",Server_name(),Server_port(),Server->sbuf);
		Server->cur->flags |= SERV_CONNECT_FAILED;
	}
	else
	{
		/* If we have a reason (from gotERROR), use that instead. */
		if(Server->cur->reason)
			strcpy(Server->sbuf,Server->cur->reason);

		Log_write(LOG_MAIN,"*","IRC server %s:%d got EOF. (%s)",Server_name(),Server_port(),Server->sbuf);

		Server_leavingIRC();
	}

	Server_disconnect();
}

int Server_usingaltnick()
{
	if(Server->currentnick[0])
		return !isequal(Server->currentnick,Grufti->botnick);
	else
		return 0;
}

time_t Server_avglag(servrec *serv)
{
	/* Don't calculate latest ping times if the server's not connected */
	if(!serv->first_activity)
	{
		if(serv->p_dur)
			return (serv->lag_x_dur / serv->p_dur);
		else
			return 0L;
	}
	else
	{
		time_t startlag = serv->startlag;
		time_t lastlag = serv->lastlag;
		time_t lag_x_dur = serv->lag_x_dur;
		time_t p_dur = serv->p_dur;
		time_t p_starttime = serv->p_starttime;
		time_t p_lasttime = serv->p_lasttime;
		time_t dur;
	
		/* Calculate last ping average */
		if(startlag != lastlag)
		{
			dur = p_lasttime - p_starttime;
			p_dur += dur;
			lag_x_dur += (dur * startlag);
			startlag = lastlag;
			p_starttime = p_lasttime;
		}
	
		/* Finish up ping time. (now until last starttime) */
		dur = time(NULL) - p_starttime;
		p_dur += dur;
		lag_x_dur += (dur * startlag);
	
		/* Finally... */
		if(p_dur)
			return lag_x_dur / p_dur;
		else
			return 0L;
	}
}

int Server_waitingforpong()
{
	if(Server->flags & SERVER_WAITING_4_PONG)
		return 1;
	else
		return 0;
}

int Server_waitingforluser()
{
	if(Server->flags & SERVER_WAITING_4_LUSER)
		return 1;
	else
		return 0;
}

void Server_sendluser()
{
	if(Server->cur == NULL)
	{
		Log_write(LOG_DEBUG,"*","Attempted to sendluser() and Server->cur == NULL.");
		return;
	}

	/* Don't luser again if we haven't received the last luser */
	if(Server_waitingforluser())
		return;

	/* Send LUSERS  */
	Server_write(PRIORITY_HIGH,"LUSERS");
	Server->time_sent_luser = time(NULL);
	Server->flags |= SERVER_WAITING_4_LUSER;
}


void Server_gotpong(time_t lag)
{
	if(Server->cur == NULL)
		return;

	/* Lag times of '0' won't show up in average times */
	lag += 1;

	Server->lag = lag;
	Server->cur->lastlag = lag;
	Server->flags &= ~SERVER_WAITING_4_PONG;
}

void Server_sendping()
/*
 * Instead of just displaying current lag, we keep track of what the lag was,
 * and how long we remained at that lag time.  This way, we can display an
 * average lag time over the connection time of the server.
 */
{
	time_t dur, now = time(NULL);

	if(Server->cur == NULL)
	{
		Log_write(LOG_DEBUG,"*","Attempted to sendping() and Server->cur == NULL.");
		return;
	}

	/* Don't ping again if we haven't received the last ping */
	if(Server_waitingforpong())
		return;

	if(Server->cur->p_starttime == 0L)
		Server->cur->p_starttime = now;

	if(Server->cur->startlag != Server->cur->lastlag)
	{
		dur = Server->cur->p_lasttime - Server->cur->p_starttime;
		Server->cur->p_dur += dur;
		Server->cur->lag_x_dur += (dur * Server->cur->startlag);
		Server->cur->startlag = Server->cur->lastlag;
		Server->cur->p_starttime = Server->cur->p_lasttime;
	}

	/* Send PING (pong to ourselves) */
	Server_write(PRIORITY_HIGH,"NOTICE %s :PONG %lu",Server->currentnick,now);
	Server->time_sent_ping = now;
	Server->flags |= SERVER_WAITING_4_PONG;
	Server->cur->p_lasttime = now;
}

int Server_timeconnected(servrec *serv)
{
	if(serv->first_activity)
		return serv->connected + (time(NULL) - serv->first_activity);
	else
		return serv->connected;
}

void Server_setcurrentnick(char *nick)
{
	strcpy(Server->currentnick,nick);
}

servrec *Server_serverandport(char *name, int port)
{
	servrec *serv;

	for(serv=Server->serverlist; serv!=NULL; serv=serv->next)
		if(isequal(serv->name,name) && (serv->port == port))
			return serv;

	return NULL;
}

servrec *Server_server(char *name)
{
	servrec *serv;

	for(serv=Server->serverlist; serv!=NULL; serv=serv->next)
		if(isequal(serv->name,name))
			return serv;

	return NULL;
}

void Server_quit(char *msg)
{
	if(Server->cur == NULL)
	{
		Log_write(LOG_DEBUG,"*","Server_quit attempted and Server->cur == NULL.");
		return;
	}

	if(!Server_isconnected())
	{
		/* No server is even connected.  Ignore. */
		return;
	}

	if(!Server_isactiveIRC())
	{
		/* if we're not even active, don't bother waiting for QUIT. */
		Server->cur->flags &= ~SERV_CONNECT_ATTEMPTED;
		Server_disconnect();
		return;
	}
		
	if(Server->flags & SERVER_QUIT_IN_PROGRESS)
	{
		Log_write(LOG_ERROR,"*","Server_quit attempted while QUIT IN PROGRESS...%s:%d (%s)",Server_name(),Server_port(),msg);
		return;
	}

	Server->flags |= SERVER_QUIT_IN_PROGRESS;
	Server_write(PRIORITY_HIGH,"QUIT :%s",msg);
	Server->time_sent_quit = time(NULL);

	/* now we just wait for disconnect and ignore everything until then. */
}

void Server_setreason(char *reason)
{
	if(Server->cur == NULL)
	{
		Log_write(LOG_DEBUG,"*","Attempted to setreason() and Server->cur == NULL. (%s)",reason);
		return;
	}

	servrec_setreason(Server->cur,reason);
}


void Server_nick(char *nick)
{
	if(Server->cur == NULL)
	{
		Log_write(LOG_DEBUG,"*","Server_nick attempted and Server->cur == NULL.");
		return;
	}

	Server->time_tried_nick = time(NULL);
	Server_write(PRIORITY_HIGH,"NICK %s",nick);
}
		
void Server_leavingIRC()
/*
 * LeavingIRC() happens when a server catches an EOF, pings out, or is stuck
 * waiting for QUIT to happen.  It's important that we do cleanup and bring
 * necessary records back to the state they were in before joining IRC.  It
 * is also important that we don't call leavingIRC() if we are not yet active
 * on IRC (ie in Server_disconnect()).
 *
 * We update all activity times since all users have effectively signed off,
 * and we clear all channels and mark them as inactive.
 *
 */
{
	/* Update activity times for all nicks and sign them off */
	User_signoffallnicks();

	/* Erase topic, key, members, bans, etc. */
	Channel_clearallchaninfo();

	/* Mark each channel as INACTIVE */
	Channel_setallchannelsoff();

	/* Reset last time_tried_join */
	Channel->time_tried_join = 0L;

	/* Dump server queue */
	Server_dumpqueue();

	/* Dump radiate queue */
	Notify_dumpqueue();
}

void Server_pushqueue(char *msg, char flag)
{
	squeue	*q;

	/* Allocate new node */
	q = squeue_new();

	/* copy the message */
	squeue_setmsg(q,msg);

	if(flag & PRIORITY_HIGH)
		Server_insertqatfront(q);
	else
		Server_appendq(q);
}

void Server_popqueue(char *msg)
{
	if(Server->queue == NULL)
	{
		Log_write(LOG_DEBUG,"*","Attempting to pop empty queue.");
		return;
	}

	strcpy(msg,Server->queue->msg);

	/* only deletes the first element */
	Server_deleteq(Server->queue);
}

int Server_bytesinfloodinterval()
{
	int	i, tot = 0;

	for(i=0; i<FLOOD_INTERVAL; i++)
		tot += Server->q_sent[i];

	return tot;
}

int Server_firstmsglen()
{
	if(Server_isqueueempty())
		return 0;

	return strlen(Server->queue->msg);
}

void Server_dequeue()
{
	int	bytes;

	/* If we're not on irc, forget it (we should clear the queue...) */
	if(!Server_isactiveIRC())
		return;

	/* bump pos pointer up a notch and clear at that pos */
	Server->q_sent_pos = (Server->q_sent_pos + 1) % FLOOD_INTERVAL;
	Server->q_sent[Server->q_sent_pos] = 0;

	/*
	 * if the first message in the queue plus the bytes sent in the last
	 * FLOOD_INTERVAL seconds is less than FLOOD_SIZE, we can send the
	 * message.
	 */
	while(!Server_isqueueempty() && (Server_bytesinfloodinterval() + Server_firstmsglen()) < FLOOD_SIZE)
	{
	
		/* message is OK to send. */
		Server_popqueue(Server->send_buf);
		bytes = strlen(Server->send_buf);

		/* Update q_sent */
		Server->q_sent[Server->q_sent_pos] += bytes;
	
		/* Update stats */
		Server->cur->bytes_out += (u_long)bytes;
		Server->bytes_out += (u_long)bytes;
	
		/* Log last time written. */
		Server->last_write_time = time(NULL);

		/* Write */
		net_sockputs(Server->sock,Server->send_buf,bytes);

#ifdef RAW_LOGGING
		Log_write(LOG_RAW,"*","--> server (queued): %s",Server->send_buf);
#endif
	}
}

void Server_dumpqueue()
{
	int	i;

	for(i=0; i<FLOOD_INTERVAL; i++)
		Server->q_sent[i] = 0;
	Server->q_sent_pos = 0;

	Server_freeallqs();
}

int Server_isqueueempty()
{
	if(Server->num_q_msgs == 0)
		return 1;
	else
		return 0;
}

/* ASDF */
/*****************************************************************************
 *
 * servrec object definitions.  No changes should be made to new(), freeall(),
 * append(), delete(), or movetofront().
 *
 ****************************************************************************/

servrec *servrec_new()
{
	servrec *x;

	/* allocate memory */
	x = (servrec *)xmalloc(sizeof(servrec));

	/* initialize */
	x->next = NULL;

	servrec_init(x);

	return x;
}

void servrec_freeall(servrec **list, servrec **last)
{
	servrec *entry = *list, *v;

	while(entry != NULL)
	{
		v = entry->next;
		servrec_free(entry);
		entry = v;
	}

	*list = NULL;
	*last = NULL;
}

void servrec_append(servrec **list, servrec **last, servrec *x)
{
	servrec *lastentry = *last;

	if(*list == NULL)
		*list = x;
	else
		lastentry->next = x;

	*last = x;
	x->next = NULL;
}

void servrec_delete(servrec **list, servrec **last, servrec *x)
{
	servrec *entry, *lastchecked = NULL;

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

			servrec_free(entry);
			return;
		}

		lastchecked = entry;
		entry = entry->next;
	}
}
			
/****************************************************************************/
/* record servrec low-level data-specific function definitions. */

void servrec_init(servrec *serv)
{
	/* initialize */
	serv->name = NULL;
	serv->port = 0;
	serv->bytes_in = 0L;
	serv->bytes_out = 0L;
	serv->motd = NULL;
	serv->first_activity = 0L;
	serv->connected = 0L;
	serv->lag_x_dur = 0L;
	serv->p_dur = 0L;
	serv->p_starttime = 0L;
	serv->p_lasttime = 0L;
	serv->startlag = 0L;
	serv->lastlag = 0L;
	serv->num_servers = 0;
	serv->reason = NULL;
	serv->disconnect_time = 0L;
	serv->flags = 0L;
}

u_long servrec_sizeof(servrec *s)
{
	u_long	tot = 0L;

	tot += sizeof(servrec);
	tot += s->name ? strlen(s->name)+1 : 0L;
	tot += s->motd ? strlen(s->motd)+1 : 0L;
	tot += s->reason ? strlen(s->reason)+1 : 0L;

	return tot;
}

void servrec_free(servrec *serv)
{
	/* Free the elements. */
	xfree(serv->name);
	xfree(serv->motd);
	xfree(serv->reason);

	/* Free this record. */
	xfree(serv);
}

void servrec_setname(servrec *serv, char *name)
{
	mstrcpy(&serv->name,name);
}

void servrec_setmotd(servrec *serv, char *motd)
{
	mstrcpy(&serv->motd,motd);
}

void servrec_setreason(servrec *serv, char *reason)
{
	mstrcpy(&serv->reason,reason);
}

void servrec_display(servrec *serv)
{
	char avg_lag[TIMESHORTLEN], cur[5], tconn[TIMESHORTLEN];
	char in[TIMESHORTLEN], out[TIMESHORTLEN], name[UHOSTLEN], num_s[5];

	/*
	 * Server codes:
	 *  ->  current server
	 *  !   couldn't connect (DNS failure, timeout, refused)
	 *  ?   connect has never been tried
	 *  *   connect was made, but was disconnected before we got active IRC
	 *  ' ' we have used this server, and everything went fine.
	 */
	if(Server->cur == serv && !Server->time_noserverpossible && Grufti->irc)
		strcpy(cur,"->");
	else if(serv->flags & SERV_CONNECT_FAILED)
		strcpy(cur,"! ");
	else if(serv->flags & SERV_NEVER_TRIED)
		strcpy(cur,"? ");
	else if(serv->flags & SERV_BANNED)
		strcpy(cur,"B ");
	else if((serv->flags & SERV_CONNECT_ATTEMPTED) && !(serv->flags & SERV_WAS_ACTIVEIRC))
		strcpy(cur,"* ");
	else
		strcpy(cur,"  ");

	if(Server->flags & SERVER_SHOW_REASONS)
	{
		char reason[BUFSIZ];

		if(serv->reason)
			strcpy(reason,serv->reason);
		else if(Server->cur == serv && !Server->time_noserverpossible)
			strcpy(reason,"");
		else
			strcpy(reason,"<none>");

		say("%-2s%-29s %s",cur,serv->name,reason);

		return;
	}

	if(Server_timeconnected(serv))
	{
		time_to_elap_short(Server_avglag(serv),avg_lag);
		time_to_elap_short(Server_timeconnected(serv),tconn);
		bytes_to_kb_short(serv->bytes_in,in);
		bytes_to_kb_short(serv->bytes_out,out);
		sprintf(num_s,"%d",serv->num_servers);
	}
	else
	{
		strcpy(avg_lag,"--");
		strcpy(tconn,"--");
		strcpy(in,"--");
		strcpy(out,"--");
		strcpy(num_s,"--");
	}


	strcpy(name,serv->name);
	if(strlen(name) > 29)
		name[29] = 0;
	say("%-2s%-29s %5d %5s %5s %5s %5s %3s",cur,name,serv->port,tconn,avg_lag,in,out,num_s);
}

/*****************************************************************************
 *
 * squeue object definitions.  No changes should be made to new(), freeall(),
 * append(), delete(), or movetofront().
 *
 ****************************************************************************/

squeue *squeue_new()
{
	squeue *x;

	/* allocate memory */
	x = (squeue *)xmalloc(sizeof(squeue));

	/* initialize */
	x->next = NULL;

	squeue_init(x);

	return x;
}

void squeue_freeall(squeue **list, squeue **last)
{
	squeue *entry = *list, *v;

	while(entry != NULL)
	{
		v = entry->next;
		squeue_free(entry);
		entry = v;
	}

	*list = NULL;
	*last = NULL;
}

void squeue_append(squeue **list, squeue **last, squeue *x)
{
	squeue *lastentry = *last;

	if(*list == NULL)
		*list = x;
	else
		lastentry->next = x;

	*last = x;
	x->next = NULL;
}

void squeue_insertatfront(squeue **list, squeue **last, squeue *x)
{
	x->next = *list;
	*list = x;

	if(x->next == NULL)
		*last = x;
}

void squeue_delete(squeue **list, squeue **last, squeue *x)
{
	squeue *entry, *lastchecked = NULL;

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

			squeue_free(entry);
			return;
		}

		lastchecked = entry;
		entry = entry->next;
	}
}
			
/****************************************************************************/
/* record squeue low-level data-specific function definitions. */

void squeue_init(squeue *q)
{
	/* initialize */
	q->msg = NULL;
	q->flags = 0;
}

u_long squeue_sizeof(squeue *q)
{
	u_long	tot = 0L;

	tot += sizeof(squeue);
	tot += q->msg ? strlen(q->msg)+1 : 0L;

	return tot;
}

void squeue_free(squeue *q)
{
	/* Free the elements. */
	xfree(q->msg);

	/* Free this record. */
	xfree(q);
}

void squeue_setmsg(squeue *q, char *msg)
{
	mstrcpy(&q->msg,msg);
}
