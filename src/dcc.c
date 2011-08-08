/******************************************************************************
 * Steve Rider - stever@area23.org
 * Grufti - an interactive, service-oriented bot for IRC 
 *
 * dcc.c - DCC object
 *
 *****************************************************************************/
/* 28 April 97 */

#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <errno.h>
#include <stdarg.h>
#include "grufti.h"
#include "gruftiaux.h"
#include "user.h"
#include "dcc.h"
#include "note.h"
#include "log.h"
#include "command.h"
#include "server.h"
#include "web.h"
#include "misc.h"
#include "location.h"
#include "net.h"

/* external buffers for sending */
char	send_to_user_buf[BUFSIZ];

extern cmdinfo_t cmdinfo;

/***************************************************************************n
 *
 * DCC object definitions
 *
 ****************************************************************************/

void DCC_new()
{
	/* already in use? */
	if(DCC != NULL)
		DCC_freeall();

	/* allocate memory */
	DCC = (DCC_ob *)xmalloc(sizeof(DCC_ob));

	/* initialize */
	DCC->dcclist = NULL;
	DCC->last = NULL;

	DCC->num_dcc = 0;
	DCC->bytes_in = 0;
	DCC->bytes_out = 0;

	DCC->send_buf[0] = 0;
	DCC->send_buf1[0] = 0;

	DCC->s1[0] = 0;
	DCC->s2[0] = 0;
}

u_long DCC_sizeof()
{
	u_long	tot = 0L;
	dccrec	*d;

	tot += sizeof(DCC_ob);
	for(d=DCC->dcclist; d!=NULL; d=d->next)
		tot += dccrec_sizeof(d);

	return tot;
}

void DCC_freeall()
{
	/* Free all dcc records */
	DCC_freealldccs();

	/* Free the object */
	xfree(DCC);
	DCC = NULL;
}

/****************************************************************************/
/* object DCC data-specific function definitions. */

void DCC_display()
{
	dccrec *d;

	say("%d total dcc connections, bytes in/out: %d/%d",DCC->num_dcc,DCC->bytes_in,DCC->bytes_out);

	say("Estab Idle Name      Host                             In  Out");
	say("----- ---- --------- ------------------------------ ---- ----");
	for(d=DCC->dcclist; d!=NULL; d=d->next)
		dccrec_display(d);
	say("----- ---- --------- ------------------------------ ---- ----");
}

/****************************************************************************/
/* low-level DCC structure function definitions. */

void DCC_appenddcc(dccrec *d)
{
	dccrec_append(&DCC->dcclist, &DCC->last, d);
	DCC->num_dcc++;
}

void DCC_deletedcc(dccrec *d)
{
	dccrec_delete(&DCC->dcclist, &DCC->last, d);
	DCC->num_dcc--;
}

void DCC_freealldccs()
{
	dccrec_freeall(&DCC->dcclist, &DCC->last);
	DCC->num_dcc = 0;
}

/****************************************************************************/
/* object DCC high-level implementation-specific function definitions. */

dccrec *DCC_dccbysock(int socket)
{
	return(dccrec_dcc(&DCC->dcclist, &DCC->last, socket));
}

dccrec *DCC_addnewconnection(char *nick, char *uh, u_long ip, int socket, u_short port, u_long flags)
/*
 *  Parameters:
 *   socket = 0     specifies a connect needs to be made
 *          = other specifies connection has been made (we've accepted)
 *   port = 0     tells system to choose any port
 *        = other tells system to try specified port
 */
{
	dccrec *x;
	int orig_port = port, highestport = 0, binary, strong;

	/* check consistency */
	if(nick == NULL || uh == NULL || flags == 0L)
	{
		internal("Oops.  Improper arguments to DCC_addnewconnection()");
		return NULL;
	}

	/* Create new record */
	x = dccrec_new();

	/* Add it to the list */
	DCC_appenddcc(x);

	/* assign the data */
	dccrec_setnick(x,nick);
	dccrec_setuh(x,uh);
	x->ip = ip;
	x->socket = socket;
	x->port = port;
	x->flags = flags;
	x->starttime = time(NULL);
	x->last = time(NULL);

	context;
	/* Client connection.  Connect to host ONLY IF socket is 0. */
	if(flags & DCC_CLIENT)
	{

		/* Setup user and account if type is dcc chat. */
		if((x->flags & CLIENT_IRC_CHAT) || (x->flags & CLIENT_XFER))
		{
			nickrec	*n;

			x->user = User_user(x->nick);
			x->account = User_account(x->user,x->nick,x->uh);

			context;
			/* check for possible signon */
			User_gotpossiblesignon(x->user,x->account);

			/*
			 * It's ok to turn on nick since we check above to
			 * make sure we're not logging in via telnet.  If we
			 * didn't check that, then the user would be pointing
			 * to a 'pending' nick.  Not what we want.
			 */

			/* Turn on nick via DCC */
			User_nickviadccon(x->user,x);

			/* Set correct case of nick */
			if(x->user->registered)
			{
				n = User_nick(x->user,x->nick);
				if(n != NULL)
					dccrec_setnick(x,n->nick);
			}
		}

		context;
		if(x->socket != 0)
		{
			/*
			 * Socket != 0 means a connection has already been
			 * made on the socket. (Happens when we answer a
			 * listening server's new connection).
			 *
			 * Nothing else to do.
			 */
			return x;
		}

		context;
		/* type of connection is going to depend on whether we are in XFER */
		binary = (x->flags & CLIENT_XFER) ? 1 : 0;
		strong = 0;

		/* Make the connection */
		x->socket = net_connect(iptodn(x->ip), x->port, binary, strong);
		if(x->socket < 0)
		{
			char errstr[MAXNETERRLENGTH];
			strcpy(errstr, x->socket == -1 ? net_errstr() : "Host not found");

			Log_write(LOG_CMD,"*","%s %s - DCC client connect on sock %d failed: %s",x->nick,x->uh,x->socket,errstr);
			DCC_disconnect(x);	
			return NULL;
		}

		context;
		/* We don't know filename params yet.  so don't log */
		if(!(x->flags & CLIENT_XFER))
			Log_write(LOG_CMD,"*","I%d %s %s - DCC connect estab'd.",x->socket,x->nick,x->uh);
	}

	/* Server connection.  Open device for listening. */
	else
	{
		if(x->port == 0)
		{
			/* Just grab any port */
			x->socket = net_listen(&x->port);
		}
		else
		{
			highestport = x->port + Grufti->num_ports_to_try;
			x->socket = -1;
	
			/* Open socket for listening.  Keep trying until we get one. */
			while((x->port < highestport) && (x->socket < 0))
			{
				x->socket = net_listen(&x->port);
				if(x->socket < 0)
					x->port++;
			}
		}

		if(x->socket < 0)
		{
			Log_write(LOG_ERROR | LOG_CMD | LOG_MAIN,"*","Oops.  DCC server \"%s\", no ports between %d and %d found on which we could listen.",x->nick,orig_port,highestport);
			DCC_disconnect(x);
			return NULL;
		}

		/* Assign port number */
		if(x->flags & SERVER_PUBLIC)
		{
			Grufti->actual_telnet_port = x->port;
			if(Grufti->quit_port_request_failed && Grufti->actual_telnet_port != Grufti->reqd_telnet_port)
			{
				Log_write(LOG_ERROR | LOG_MAIN,"*","Unable to get requested port %d for telnet server.",Grufti->reqd_telnet_port);
				fatal_error("Unable to get port %d for telnet server.", Grufti->reqd_telnet_port);
			}
		}
		else if(x->flags & SERVER_INTERNAL)
		{
			Grufti->actual_gateway_port = x->port;
			if(Grufti->quit_port_request_failed && Grufti->actual_gateway_port != Grufti->reqd_gateway_port)
			{
				Log_write(LOG_ERROR | LOG_MAIN,"*","Unable to get requested port %d for gateway server.",Grufti->reqd_gateway_port);
				fatal_error("Unable to get requested port %d for gateway server.", Grufti->reqd_gateway_port);
			}
		}

		Log_write(LOG_CMD,"*","DCC server \"%s\" listening on port %d, sock %d.",x->nick,x->port,x->socket);
	}
		
	return x;
}

void DCC_disconnect(dccrec *x)
/*
 * Update last seen.  We have to wait until the dcc chat has fallen
 * into either TELNET or IRC.  If we're still pending login, ident,
 * or anything like that, the account and user pointers will not have
 * been initialized.
 *
 * Also, delete registered accounts which have no password set.
 * The user did not set a password on creation of the account.  Helps
 * to insure the user actually desires a new account.
 *
 * If an ident client is closing, we need to update it's corresponding
 * dcc record and turn id OFF.  The id pointer is used to kill the
 * corresponding ident client if the dcc record disconnects.
 */
{
	context;
	/* Let's set some signoff messages, shall we? */
	if(x->flags & STAT_WEBCLIENT && !(x->flags & STAT_PENDING_LOGIN) && x->user->registered)
		userrec_setsignoff(x->user,"via web page");
	else if(x->flags & STAT_GATEWAYCLIENT && !(x->flags & STAT_PENDING_LOGIN) && x->user->registered)
		userrec_setsignoff(x->user,"via gateway interface");
	else if(x->flags & CLIENT_TELNET_CHAT && !(x->flags & STAT_PENDING_LOGIN) && x->user->registered)
		userrec_setsignoff(x->user,"via telnet to bot");

	if((x->flags & CLIENT_IRC_CHAT) || ((x->flags & CLIENT_TELNET_CHAT) && !(x->flags & STAT_PENDING_LOGIN)))
	{
		User_gotactivity(x->user,x->account);

	context;
		/* Show that this nick is not connected via dcc anymore */
		if(x->flags & CLIENT_TELNET_CHAT)
			User_nickviatelnetoff(x->user,x);
		else
			User_nickviadccoff(x->user,x);

	context;
		if(x->user->registered && x->user->pass == NULL)
		{
			Log_write(LOG_CMD,"*","%s %s - Account has no password.  Deleting...",x->nick,x->uh);
			User_removeuser(x->user);

			User_resetallpointers();
		}

		x->flags &= ~STAT_COMMAND_REGISTER;
		x->flags &= ~STAT_COMMAND_SETUP;

	}

	/* If SMTP and QUIT, then we got EOF before we could send QUIT. */
	if((x->flags & CLIENT_SMTP) && (x->flags & STAT_SMTP_QUIT))
	{
		noterec	*n;

		/* shit.  Well, all we can do is undelete all their Notes
		 * since we're not sure which ones made it rhough and which
		 * ones didn't.
		 */
		for(n=x->user->notes; n!=NULL; n=n->next)
		{
			n->flags &= ~NOTE_DELETEME;
			n->flags &= ~NOTE_FORWARD;
		}

		Note_sendnotice(x->user,"Encountered problems while forwarding your Notes.  All Notes were kept on the bot even though some may have been sent.  Sorry!");

		Log_write(LOG_ERROR,"*","Encountered problems while trying to send Notes to %s. (got EOF from SMTP server on %s before QUIT)",x->user->acctname,Grufti->smtp_gateway);
	}

	context;
	if((x->flags & CLIENT_IDENT) && x->id != NULL)
	{
	context;
		/* We're closing an IDENT.  Turn off dcc record's pointer. */
	context;
		if(x != x->id->id)
		{
	context;
			Log_write(LOG_DEBUG,"*","Ident client's id pointer to dcc record does not point back to ident client. (%s %s)",x->nick,x->uh);
		}
	context;
		x->id->id = NULL;
	}
	
	context;
	if(x->flags & CLIENT_TELNET_CHAT)
	{
	context;
		/* Kill the ident client if one exists. */
		if(x->id)
		{
	context;
			Log_write(LOG_DEBUG,"*","Here's a case where the dcc client closed before ident. %s %s",x->nick,x->uh);
	context;
			DCC_disconnect(x->id);
	context;
		}
	}


	context;

	/* close connection */
	net_disconnect(x->socket);

	context;
	/* find record and kill it */

	DCC_deletedcc(x);
}

void DCC_gotactivity(int sock, char *buf, int len)
{
	dccrec *d;
	char flags[DCCFLAGLEN];

	/* Locate record */
	d = DCC_dccbysock(sock);
	if(d == NULL)
	{
		internal("dcc: in gotactivity(), got activity on socket %d, but no record. (%s)",sock,buf);
		return;
	}

	context;
	/* update record */
	d->bytes_in += len;
	d->last = time(NULL);

	context;
	/* update dcc device */
	DCC->bytes_in += len;

	DCC_flags2str(d->flags,flags);

	context;
	/* Send activity to appropriate type */
	if(d->flags & DCC_CLIENT)
	{
		context;
		if(d->flags & CLIENT_TELNET_CHAT)
		{
			DCC_strip_telnet(d,buf,&len);
			if(len == 0 && !(d->flags & STAT_PENDING_LOGIN))
			{
				/* logged in, user hit a carriage return. */
				DCC_showprompt(d);
				return;
			}
		}

		context;
		/* Check for disconnect flag if we didn't get it before. */
		if(d->flags & STAT_DISCONNECT_ME)
			DCC_disconnect(d);

		context;
#ifdef RAW_LOGGING
		/* Don't log xfers. doh! */
		if(!(d->flags & CLIENT_XFER))
		{
			Log_write(LOG_RAW,"*","DCC client %d: %s",d->socket,flags);
			Log_write(LOG_RAW,"*","           %d: %s %s \"%s\"",d->socket,d->nick,d->uh,buf);
		}
#endif

		context;
		/* Ignore if we don't have a length for client stuff. */
		if(len <= 0)
			return;

		context;
		if(d->flags & CLIENT_IDENT)
			DCC_gotidentactivity(d,buf);
		else if(d->flags & CLIENT_IRC_CHAT)
			DCC_gotircchatactivity(d,buf);
		else if(d->flags & CLIENT_TELNET_CHAT)
			DCC_gottelnetchatactivity(d,buf);
		else if(d->flags & CLIENT_XFER)
			DCC_gotxferactivity(d,buf,len);
		else if(d->flags & CLIENT_SMTP)
			DCC_gotsmtpactivity(d,buf,len);
		else
			Log_write(LOG_DEBUG,"*","client activity on socket %d not recognized: flags %lu \"%s\"",d->socket,d->flags,buf);
	}
	else if(d->flags & DCC_SERVER)
	{
		context;
#ifdef RAW_LOGGING
		Log_write(LOG_RAW,"*","DCC SERVER %d: %s",d->socket,flags);
#endif

		/* Send activity to appropriate type */
		if(d->flags & SERVER_PUBLIC)
			DCC_gottelnetconnection(d);
		else if(d->flags & SERVER_INTERNAL)
			DCC_gotinternalconnection(d);
		else if(d->flags & SERVER_IRC_CHAT)
			DCC_gotircchatconnection(d);
		else if(d->flags & SERVER_XFER)
			DCC_gotxferconnection(d);
	}
	else
		Log_write(LOG_DEBUG,"*","DCC activity on socket %d is neither CLIENT nor SERVER: flags %lu",d->socket,d->flags,d->socket);
}

void DCC_gotidentactivity(dccrec *id, char *buf)
{
	char user[BUFSIZ],uh[UHOSTLEN], *p, *p1, *p2;

	/*
	 * Extract username for authorization response.
	 * looks like:
	 *   <theirport> , <ourport> : USERID : UNIX :<username>
	 */
	context;
	p = strchr(buf,':');
	if(p == NULL)
	{
		/* No ':' */
		Log_write(LOG_CMD|LOG_DEBUG,"*","Bad identd reply on socket %d: %s",id->socket,buf);
		id->id->flags &= ~STAT_PENDING_IDENT;
		DCC_dologin(id->id);
		DCC_disconnect(id);
		return;
	}
	context;
	p1 = strchr(p+1,':');
	if(p1 == NULL)
	{
		/* Only one ':' */
		Log_write(LOG_CMD|LOG_DEBUG,"*","Bad identd reply on socket %d: %s",id->socket,buf);
		id->id->flags &= ~STAT_PENDING_IDENT;
		DCC_dologin(id->id);
		DCC_disconnect(id);
		return;
	}
	context;
	p2 = strchr(p1+1,':');
	if(p2 == NULL || *p2 == 0 || *(p2+1) == 0)
	{
		/* Only two ':'s.  Check for NO-USER */
		p1++;
		rmspace(p1);
		if(isequal(p1,"NO-USER"))
			Log_write(LOG_CMD | LOG_DEBUG,"*","%s - %s Ident got NO-USER on sock %d. oh well.",id->id->nick,id->id->uh,id->socket);
		else
			Log_write(LOG_CMD|LOG_DEBUG,"*","Bad identd reply on socket %d: %s",id->socket,buf);
		id->id->flags &= ~STAT_PENDING_IDENT;
		DCC_dologin(id->id);
		DCC_disconnect(id);
		return;
	}

	context;
	strcpy(user,p2+1);
	rmspace(user);

	context;
	/* Make sure username is valid */
	if(strlen(user) > 10)
	{
		Log_write(LOG_DEBUG,"*","Long identd username on socket %d: %s (%s).  Using first 10 chars.",id->socket,user,buf);
		user[10] = 0;
	}

	context;
	/* Break apart the old "unknown@x.x.x" and insert "user@x.x.x" */
	splitc(NULL,id->id->uh,'@');
	sprintf(uh,"%s@%s",user,id->id->uh);
	dccrec_setuh(id->id,uh);
	id->id->flags &= ~STAT_PENDING_IDENT;

	context;
	/* Stick a "Login:" on the other end */
	DCC_dologin(id->id);

	context;
	/* and kill the old ident client */
	DCC_disconnect(id);
}

void DCC_gotircchatactivity(dccrec *d, char *buf)
{
	context;
	/* update last seen */
	User_gotactivity(d->user,d->account);

	context;

	/* Don't log passwords */
	if((strlen(buf) >= 4) && strncasecmp(buf,"pass",4) == 0)
		Log_write(LOG_CMD,"*","I%d %s %s - <sent password>",d->socket,d->nick,d->uh);
	else
		Log_write(LOG_CMD,"*","I%d %s %s - \"%s\"",d->socket,d->nick,d->uh,buf);
	context;
	DCC_docommand(d,buf);

	context;
	/* Check for DISCONNECT */
	if(d->flags & STAT_DISCONNECT_ME)
	{
		Log_write(LOG_CMD,"*","I%d %s %s - DCC connection closed.",d->socket,d->nick,d->uh);
		DCC_disconnect(d);
	}
	context;
}

void DCC_gottelnetchatactivity(dccrec *d, char *buf)
{
	context;
	if(d->flags & STAT_PENDING_IDENT)
		return;

	context;
	if(d->flags & STAT_PENDING_LOGIN)
	{
		if(d->flags & STAT_WAITINGFORID)
			DCC_gotid(d,buf);
		else if(d->flags & STAT_LOGIN)
			DCC_gotlogin(d,buf);
		else if(d->flags & STAT_PASSWORD)
			DCC_gotpassword(d,buf);

		return;
	}

	context;
	/* update last seen */
	User_gotactivity(d->user,d->account);

	context;
	rmspace(buf);
	if(buf[0])
	{
		/* Normal chat activity via telnet */
		/* Don't log passwords */
		if((strlen(buf) >= 4) && strncasecmp(buf,"pass",4) == 0)
			Log_write(LOG_CMD,"*","T%d %s %s - <sent password>",d->socket,d->nick,d->uh);
		else if(d->flags & STAT_TINYGRUFTI)
			Log_write(LOG_CMD,"*","N%d %s %s - \"%s\"",d->socket,d->nick,d->uh,buf);
		else if(d->flags & STAT_WEBCLIENT)
			Log_write(LOG_CMD,"*","W%d %s %s - \"%s\"",d->socket,d->nick,d->uh,buf);
		else if(d->flags & STAT_GATEWAYCLIENT)
			Log_write(LOG_CMD,"*","G%d %s %s - \"%s\"",d->socket,d->nick,d->uh,buf);
		else
			Log_write(LOG_CMD,"*","T%d %s %s - \"%s\"",d->socket,d->nick,d->uh,buf);

		if(0);
#ifdef TINYGRUFTI_MODULE
		else if(d->flags & STAT_TINYGRUFTI)
			DCC_gottinygruftiactivity(d,buf);
#endif
#ifdef WEB_MODULE
		else if(d->flags & STAT_WEBCLIENT)
			Web_docommand(d,buf);
#endif
		else
			DCC_docommand(d,buf);

		/* Check for DISCONNECT */
		if(d->flags & STAT_DISCONNECT_ME)
		{
			if(d->flags & STAT_TINYGRUFTI)
				Log_write(LOG_CMD,"*","N%d %s %s - DCC connection closed.",d->socket,d->nick,d->uh);
			else if(d->flags & STAT_WEBCLIENT)
				Log_write(LOG_CMD,"*","W%d %s %s - DCC connection closed.",d->socket,d->nick,d->uh);
			else if(d->flags & STAT_GATEWAYCLIENT)
				Log_write(LOG_CMD,"*","G%d %s %s - DCC connection closed.",d->socket,d->nick,d->uh);
			else
				Log_write(LOG_CMD,"*","T%d %s %s - DCC connection closed.",d->socket,d->nick,d->uh);
			DCC_disconnect(d);
			return;
		}
	}

	context;
	DCC_showprompt(d);
}

void DCC_gotxferactivity(dccrec *d, char *buf, int len)
{
	if(d->flags & STAT_XFER_IN)
		DCC_gotxferactivity_in(d,buf,len);
	else if(d->flags & STAT_XFER_OUT)
		DCC_gotxferactivity_out(d,buf,len);
	else
		Log_write(LOG_DEBUG,"*","Unknown XFER activity type %d!",d->flags);
}

void DCC_gottinygruftiactivity(dccrec *d, char *buf)
{
#if 0
	/* We store the header for later */
	split(d->tinygrufti_header,buf);
	TinyGrufti_docommand(d,buf);
#endif
}

void DCC_gotsmtpactivity(dccrec *d, char *buf, int len)
{
	if(len < 3)
		return;

	if(strncmp(buf,"220",3) == 0)
		DCC_gotsmtp220(d,buf,len);
	else if(strncmp(buf,"250",3) == 0)
		DCC_gotsmtp250(d,buf,len);
	else if(strncmp(buf,"354",3) == 0)
		DCC_gotsmtp354(d,buf,len);
}

/* Got Welcome to SMTP server */
void DCC_gotsmtp220(dccrec *d, char *buf, int len)
{
	/* Send HELO */
	DCC_write(d,"HELO %s",Grufti->bothostname);

	/* Set flags specifying we we're waiting to send MAIL FROM */
	d->flags |= STAT_SMTP_MAILFROM;
}
	
/* Get 250 OK */
void DCC_gotsmtp250(dccrec *d, char *buf, int len)
{
	if(d->flags & STAT_SMTP_MAILFROM)
	{
		DCC_write(d,"MAIL FROM: %s",Grufti->botuh);
		d->flags &= ~STAT_SMTP_MAILFROM;
		d->flags |= STAT_SMTP_RCPTTO;

		/* Delete marked Notes from previous send */
		Note_deletemarkednotes(d->user);
	}
	else if(d->flags & STAT_SMTP_RCPTTO)
	{
		DCC_write(d,"RCPT TO: %s",d->uh);
		d->flags &= ~STAT_SMTP_RCPTTO;
		d->flags |= STAT_SMTP_DATA;
	}
	else if(d->flags & STAT_SMTP_DATA)
	{
		DCC_write(d,"DATA");
		d->flags &= ~STAT_SMTP_DATA;
	}
	else if(d->flags & STAT_SMTP_QUIT)
	{
		DCC_write(d,"QUIT");
		d->flags &= ~STAT_SMTP_QUIT;

		/* OK all done.  Delete all marked Notes. */
		Note_deletemarkednotes(d->user);
	}
}

void DCC_gotsmtp354(dccrec *d, char *buf, int len)
{
	noterec *n;
	nbody *body;
	userrec	*u_from;
	char	subject[LINELENGTH], received[TIMELONGLEN], from[256];

	for(n=d->user->notes; n!=NULL; n=n->next)
	{
		if(n->flags & NOTE_FORWARD)
			break;
	}

	if(n != NULL)
	{
		n->flags &= ~NOTE_FORWARD;
		n->flags |= NOTE_DELETEME;

		Grufti->notes_forwarded++;

		u_from = User_user(n->nick);

		/* Create from */
		if(u_from->email && u_from->email[0])
			sprintf(from,"%s <%s>",n->nick,u_from->email);
		else if(u_from->forward && u_from->forward[0])
			sprintf(from,"%s <%s>",n->nick,u_from->forward);
		else
			sprintf(from,"%s <%s>",n->nick,Grufti->botuh);

		/* Create subject */
		if(n->body != NULL && n->body->text != NULL)
			strcpy(subject,n->body->text);
		else
			strcpy(subject,"");
		if(strlen(subject) > 19)
			strcpy(&subject[19-3],"...");

		/* Create formal date */
		timet_to_date_formal(n->sent,received);

		DCC_write(d,"From: %s",from);
		DCC_write(d,"To: %s <%s>",d->user->acctname,d->uh);
		DCC_write(d,"Subject: %s Note: %s",Grufti->botnick,subject);
		DCC_write(d,"Date: %s",received);
		DCC_write(d,"");
		if(n->flags & NOTE_MEMO)
		{
			DCC_write(d,"<memo to myself>");
			DCC_write(d,"");
		}
		for(body=n->body; body!=NULL; body=body->next)
		{
			if(isequal(body->text,"."))
				DCC_write(d,"");
			else
				DCC_write(d,"%s",body->text);
		}

	}
	DCC_write(d,".");

	/* If we have more Notes to send, just signal to start again */
	if(Note_numforwarded(d->user))
		d->flags |= STAT_SMTP_MAILFROM;
	else
		d->flags |= STAT_SMTP_QUIT;
}


void DCC_gotxferactivity_in(dccrec *d, char *buf, int len)
/*
 * Receiving a file
 */
{
	u_long	netint;

	context;
	/* Write to file */
	fwrite(buf,len,1,d->f);

	context;
	/* update how much has been sent */
	d->sent += len;

	netint = htonl(d->sent);
	write(d->socket,&netint,4);

	context;
	/* See if amount sent is greater than filesize (means bogus) */
	if(d->sent > d->length)
	{
		Log_write(LOG_CMD | LOG_DEBUG,"*","DCC send from %s %s is exceeding the filesize! (%s %d)  Closing.",d->nick,d->uh,d->filename,d->length);
		sendnotice(d->nick,"The length of \"%s\" I have received is exceeding its size!",d->filename);

		fclose(d->f);
		DCC_disconnect(d);
	}
	context;
}

void DCC_gotxferactivity_out(dccrec *d, char *buf, int len)
/*
 * Sending a file (here we're getting an ack)
 */
{
	u_char	bbuf[MAXHOSTLENGTH], *bf;
	u_long	cmp, l, li;

	/* prepend any previous partial acks */
	xmemcpy(bbuf,buf,len);
	if(d->sofar)
	{
		xmemcpy(&d->buf[d->sofar],bbuf,len);
		xmemcpy(bbuf,d->buf,d->sofar+len);
		len += d->sofar;
		d->sofar = 0;
	}

	/* toss previously pent-up acks */
	while(len > 4)
	{
		len = 4;
		xmemcpy(bbuf,&bbuf[4],len);
	}
	if(len < 4)
	{
		/* not a full answer.  store and wait for rest later */
		d->sofar = len;
		xmemcpy(d->buf,bbuf,len);
		return;
	}

	/*
	 * this is more compatible than ntohl for machines where an int
	 * is more than 4 bytes.
	 */
	cmp = ((u_int)(bbuf[0]) << 24) + ((u_int)(bbuf[1]) << 16) + ((u_int)(bbuf[2]) << 8) + bbuf[3];
	if((cmp > d->sent) && (cmp <= d->length))
	{
		/* attempt to resume i guess */
		fseek(d->f,cmp,SEEK_SET);
		d->sent = cmp;
		Log_write(LOG_CMD,"*","Resuming xfer at %dk for \"%s\" to %s.",(int)(cmp / 1024),d->filename,d->nick);
	}

	if(cmp != d->sent)
		return;

	/* done! */
	if(d->sent == d->length)
	{
		char	n1[50];

		comma_number(n1,d->length);
		fclose(d->f);
		Log_write(LOG_CMD,"*","X%d %s %s - Sent \"%s\" (%s bytes)",d->socket,d->nick,d->uh,d->filename,n1);
		DCC_disconnect(d);
		return;
	}

	l = Grufti->xfer_blocksize;
	if(d->sent + Grufti->xfer_blocksize > d->length)
		l = d->length - d->sent;

	bf = (u_char *)xmalloc(l+1);
	fread(bf,l,1,d->f);
	li = write(d->socket,bf,l);
	if(li < l)
		Log_write(LOG_DEBUG,"*","All hell breaks loose now: %lu < %lu",li,l);
	xfree(bf);
	d->sent += l;
}

void DCC_gotinternalconnection(dccrec *gw_serv)
{
	char	s[MAXHOSTLENGTH];
	char	uh[HOSTLEN];
	int	sock;
	u_short port;
	IP	ip;
	dccrec	*d;

	/* Got activity on a gateway server.  Answer it. */
	sock = net_accept(gw_serv->socket,s,&ip,&port,0);
	if(sock == -1 && (errno == EAGAIN))
	{
		Log_write(LOG_CMD|LOG_DEBUG,"*","G%d %s %s - Bad connection on gateway server: %s",gw_serv->socket,gw_serv->nick,gw_serv->uh,net_errstr());
		return;
	}

	context;
	if(sock < 0)
	{
		Log_write(LOG_CMD|LOG_DEBUG,"*","Incoming gateway failed connect on socket %d: %s",gw_serv->socket,net_errstr());
		return;
	}
	context;

	/* Connection coming in is on gateway port. */
	/* (We can't tell yet if it's a WEBCLIENT...) */
	Log_write(LOG_CMD,"*","Gateway connection from %s.",s);
	sprintf(uh,"gateway@%s",s);

	/* Make connection */
	d = DCC_addnewconnection("\"pending\"",uh,ip,sock,port,DCC_CLIENT|CLIENT_TELNET_CHAT|STAT_GATEWAYCLIENT|STAT_PENDING_LOGIN);
	context;
	if(d == NULL)
	{
		internal("Unable to find recently added connection on %s %d %d", uh,
			port, sock);
		return;
	}

	/* Don't login until we get their ID and version # */
	d->flags |= STAT_WAITINGFORID;
	context;
}

void DCC_gotircchatconnection(dccrec *ircchat_serv)
/*
 * A connection here is a result of a DCC CHAT to a user from the bot.
 */
{
	dccrec	*d;
	IP	ip;
	int	sock;
	u_short port;
	char	s[MAXHOSTLENGTH], u[USERLEN], h[HOSTLEN];

	context;
	Log_write(LOG_DEBUG,"*","Got connection on irc chat server.  Answering.");
	/* Got activity on a ircchat server awaiting connection.  Answer it. */
	sock = net_accept(ircchat_serv->socket,s,&ip,&port,0);
	Log_write(LOG_DEBUG,"*","Accepted.");
	context;
	if(sock == -1 && (errno == EAGAIN))
	{
	context;
		Log_write(LOG_CMD|LOG_DEBUG,"*","I%d %s %s - Bad connection on ircchat server: %s",ircchat_serv->socket,ircchat_serv->nick,ircchat_serv->uh,net_errstr());
	context;
		return;
	}

	context;
	if(sock < 0)
	{
		Log_write(LOG_CMD|LOG_DEBUG,"*","%s %s - Failed connect over ircchat server on socket %d: %s",ircchat_serv->nick,ircchat_serv->uh,ircchat_serv->socket,net_errstr());
		/*
		 * Don't kill the ircchat server yet.  Give the user another
		 * chance to try connecting.  Timeout will take care of the
		 * ircchat disconnect later.
		 */
		return;
	}

	/*
	 * Create a new record in the dcclist.  We know their nick and uh
	 * from the chat client, that is unless someone bounced in before us.
	 * Verify their hostname maybe?
	 */
	context;
	d = DCC_addnewconnection(ircchat_serv->nick,ircchat_serv->uh,ip,sock,port,DCC_CLIENT|CLIENT_IRC_CHAT);
	if(d == NULL)
		return;

	splituh(u,h,d->uh);
	if(!isequal(h,s))
	{
/*
		Log_write(LOG_CMD,"*","DCC CHAT hostname \"%s\" does not match connecting client \"%s\". (Ignoring)",h,s);
*/
		/* possible disconnect here. */
	}

	Log_write(LOG_CMD,"*","I%d %s %s - DCC connect estab'd.",d->socket,d->nick,d->uh);

	context;
	/* is client pending command? */
	if(ircchat_serv->flags & STAT_COMMAND_PENDING)
	{
		/*
		 * Only honor commands IF
		 *   - the command was issued in the last n seconds
		 *   - the command was issued by the same nick we're dcc connecting with
		 */
		if((time(NULL) - ircchat_serv->commandtime) < Grufti->pending_command_timeout && isequal(ircchat_serv->commandfromnick, d->nick))
		{
			/* Don't log passwords */
			if((strlen(ircchat_serv->command) >= 4) && strncasecmp(ircchat_serv->command,"pass",4) == 0)
				Log_write(LOG_CMD,"*","I%d %s %s - <sent password>",d->socket,d->nick,d->uh);
			else
				Log_write(LOG_CMD,"*","I%d %s %s - \"%s\"",d->socket,d->nick,d->uh,ircchat_serv->command);
	
			DCC_docommand(d,ircchat_serv->command);
		}
	}
	else
		DCC_ondccchat(d);
		
	context;
	/* Disconnect and discard the server we were using to wait with. */
	DCC_disconnect(ircchat_serv);
}

void DCC_gottelnetconnection(dccrec *serv)
{
	int sock;
	u_short port;
	IP ip;
	char s[MAXHOSTLENGTH];
	char	uh[HOSTLEN];
	dccrec	*d, *id;

	sock = net_accept(serv->socket, s, &ip, &port, 0);
	if(sock == -1)
	{
		Log_write(LOG_ERROR,"*","S%d %s %s - Bad connection on server: %s",
			serv->socket, serv->nick, serv->uh, net_errstr());
		return;
	}

	if(sock < 0)
	{
		Log_write(LOG_CMD|LOG_DEBUG,"*","Incoming telnet failed connect on socket %d: %s",serv->socket,net_errstr());
		return;
	}
	context;

	Log_write(LOG_CMD,"*","Telnet connection from %s.",s);

	/*
	 * Create a new record in the dcclist.  We don't know their nick
	 * until they login.  We don't know username yet either.  Try to
	 * determine username through ident.  If we can't, use "unknown".
	 */
	sprintf(uh,"unknown@%s",s);
	d = DCC_addnewconnection("\"pending\"",uh,ip,sock,port,DCC_CLIENT|CLIENT_TELNET_CHAT|STAT_PENDING_IDENT|STAT_PENDING_LOGIN);
	if(d == NULL)
	{
		Log_write(LOG_ERROR,"*","Couldn't add new connection in gottelnetconnection()");
		return;
	}

	context;
	/* open a connection to their identd */
	id = DCC_addnewconnection("IdentClnt",uh,ip,0,113,DCC_CLIENT|CLIENT_IDENT);
	if(id == NULL)
	{
		Log_write(LOG_CMD,"*","%s %s - Couldn't connect to ident server on %s.  Oh well.",d->nick,d->uh,s);
		d->flags &= ~STAT_PENDING_IDENT;
		DCC_dologin(d);
		return;
	}

	context;
	/* point the ident pointer to the user we're looking up. */
	/*
	 * Ok now we need to be careful.  Here we assign the ident client's id
	 * pointer to point to the new dcc record.  When we get results back
	 * from ident, we'll use this pointer to find the dcc record it's
	 * associated with.  BUT.  If the user disconnects the dcc connection
	 * before ident is done, and ident comes back positive, it's going to
	 * try and write to an unknown pointer.  So here we cross-reference
	 * and point the ident client to it's dcc record, and the dcc record
	 * back to the ident client.
	 */
	d->id = id;
	id->id = d;

	context;
	/* Connected to identd.  Query for username.  Wait for results on i/o */
	DCC_write(id,"%d , %d",d->port,serv->port);
}

void DCC_gotxferconnection(dccrec *xfer_serv)
{
	dccrec	*d;
	char	s[MAXHOSTLENGTH], u[USERLEN], h[HOSTLEN], n1[50];
	char	*bf;
	IP	ip;
	int	sock;
	u_short port;

	context;
	/* Got activity on an xfer server awaiting connection.  Answer it. */
	sock = net_accept(xfer_serv->socket,s,&ip,&port,1);
	if(sock == -1 && (errno == EAGAIN))
	{
		Log_write(LOG_ERROR,"*","X%d %s %s - Bad connection on xfer server: %s",xfer_serv->socket,xfer_serv->nick,xfer_serv->uh,net_errstr());
		return;
	}

	context;
	if(sock < 0)
	{
		Log_write(LOG_CMD|LOG_DEBUG,"*","%s %s - Failed connect over xfer server on socket %d: %s",xfer_serv->nick,xfer_serv->uh,xfer_serv->socket,net_errstr());
		/*
		 * Don't kill the xfer server yet.  Give the user another
		 * chance to try connecting.  Timeout will take care of the
		 * xfer disconnect later.
		 */
		return;
	}

	/*
	 * Create a new record in the dcclist.  We know their nick and uh
	 * from the xfer client, that is unless someone bounced in before us.
	 * Verify their hostname maybe?
	 */
	context;
	d = DCC_addnewconnection(xfer_serv->nick,xfer_serv->uh,ip,sock,port,DCC_CLIENT|CLIENT_XFER|STAT_XFER_OUT);
	if(d == NULL)
		return;

	/* Copy the info from the server */
	d->length = xfer_serv->length;
	d->f = xfer_serv->f;
	dccrec_setfilename(d,xfer_serv->filename);

	splituh(u,h,d->uh);
	if(!isequal(h,s))
	{
/*
		Log_write(LOG_CMD,"*","DCC CHAT hostname \"%s\" does not match connecting client \"%s\". (Ignoring)",h,s);
*/
		/* possible disconnect here. */
	}

	comma_number(n1,d->length);
	Log_write(LOG_CMD,"*","X%d %s %s - Sending \"%s\" (%s bytes)",d->socket,d->nick,d->uh,d->filename,n1);

	context;
	/* Disconnect and discard the server we were using to wait with. */
	DCC_disconnect(xfer_serv);

	/* Now.  Send first piece.  (we'll get ack later in gotxferactivity() */
	if(d->length < Grufti->xfer_blocksize)
		d->sent = d->length;
	else
		d->sent = Grufti->xfer_blocksize;

	bf = (char *)xmalloc(d->sent + 1);
	fread(bf,d->sent,1,d->f);
	write(d->socket,bf,d->sent);
	xfree(bf);
}

void DCC_gotEOF(int sock)
{
	dccrec *d;

	context;
	d = DCC_dccbysock(sock);
	if(d == NULL)
	{
		Log_write(LOG_DEBUG,"*","Oops.  EOF on socket %d.  Doesn't match dcc OR server.",sock);
		/*
		 * We need to close it anyway.  The network module has a record of
		 * it somehow and we don't.  If we don't close it, the network module
		 * is going to spew EOF's on this socket until it's blue in the face.
		 */
		net_disconnect(sock);

		return;
	}

	context;
	/* If the ident client got EOF and we were still waiting, do login */
	if(d->flags & CLIENT_IDENT)
	{
		if(d->id->flags & STAT_PENDING_IDENT)
		{
			d->id->flags &= ~STAT_PENDING_IDENT;
			DCC_dologin(d->id);
		}
	}

	/* If the SMTP client got EOF and we were still waiting for QUIT */
	if(d->flags & CLIENT_SMTP)
	{
		if(d->flags & STAT_SMTP_QUIT)
		{
			Log_write(LOG_DEBUG,"*","Oops.  Got EOF on SMTP client conection to %s.",Grufti->smtp_gateway);
		}
	}

	context;
	if(d->flags & CLIENT_IRC_CHAT)
		Log_write(LOG_CMD,"*","I%d %s %s - DCC connection closed.",d->socket,d->nick,d->uh);
	else if(d->flags & STAT_TINYGRUFTI)
		Log_write(LOG_CMD,"*","N%d %s %s - DCC connection closed.",d->socket,d->nick,d->uh);
	else if(d->flags & STAT_WEBCLIENT)
		Log_write(LOG_CMD,"*","W%d %s %s - DCC connection closed.",d->socket,d->nick,d->uh);
	else if(d->flags & STAT_GATEWAYCLIENT)
		Log_write(LOG_CMD,"*","G%d %s %s - DCC connection closed.",d->socket,d->nick,d->uh);
	else if(d->flags & CLIENT_TELNET_CHAT)
		Log_write(LOG_CMD,"*","T%d %s %s - DCC connection closed.",d->socket,d->nick,d->uh);
	else if(d->flags & CLIENT_XFER && d->flags & STAT_XFER_IN)
	{
		char	n1[50], n2[50];

		context;
		/* close file */
		fclose(d->f);

		context;
		/* Did we get it all? */
		if(d->sent < d->length)
		{
			char	filepath[256];

		context;
			comma_number(n1,d->sent);
			comma_number(n2,d->length);

		context;
			Log_write(LOG_CMD,"*","X%d %s %s - \"%s\" interrupted. (%s of %s bytes).",d->socket,d->nick,d->uh,d->filename,n1,n2);

			/* Remove incomplete files */
			sprintf(filepath,"%s/%s",Grufti->xfer_incoming,d->filename);
			if(Grufti->xfer_remove_incomplete)
				unlink(filepath);

		context;
			sendnotice(d->nick,"File \"%s\" was not fully received. (Got %s of %s bytes)",d->filename,n1,n2);
		}
		else
		{
		context;
			comma_number(n1,d->length);
			Log_write(LOG_CMD,"*","X%d %s %s - Got \"%s\". (%s bytes)",d->socket,d->nick,d->uh,d->filename,n1);

		context;
			bytes_to_kb_long(d->length / (time(NULL)-d->starttime)+1,n2);
		context;
			sendnotice(d->nick,"Thanks for the file! (%s bytes in %ds @ %s/s)",n1,time(NULL)-d->starttime,n2);

		context;
			if(Grufti->send_notice_DCC_receive)
				Note_sendnotification("Got \"%s\" (%s bytes in %ds @ %s/s) from %s %s.",d->filename,n1,time(NULL)-d->starttime,n2,d->nick,d->uh);
		context;
		}
	}
	else if(d->flags & CLIENT_XFER && d->flags & STAT_XFER_OUT)
	{
		char	n1[50], n2[50];

		/* close file */
		fclose(d->f);

		/* We were sending it.  oh well. */
		comma_number(n1,d->sent);
		comma_number(n2,d->length);

		Log_write(LOG_CMD,"*","X%d %s %s - File \"%s\" not fully sent. (sent %s of %s bytes)",d->socket,d->nick,d->uh,d->filename,n1,n2);
	}
	else
		Log_write(LOG_CMD,"*","%s %s - DCC connection closed.",d->nick,d->uh);

	DCC_disconnect(d);
}
	
void DCC_dologin(dccrec *d)
{
	char	telnet_banner[256];

	d->flags |= STAT_LOGIN;

	if(Grufti->show_telnet_banner)
	{
		sprintf(telnet_banner,"%s/%s",Grufti->admindir,Grufti->telnet_banner);
		catfile(d,telnet_banner);
	}

	context;
	DCC_write(d,"");
	DCC_write(d,"Welcome to %s",Grufti->copyright);
	DCC_write(d,"You are identified as: %s",d->uh);
	DCC_write_nocr(d,"login: ");
}

void DCC_gotid(dccrec *d, char *buf)
{
	char	id[BUFSIZ];

	/* Check first token */
	split(id,buf);
	if(strcmp(id,"tinygrufti") == 0)
	{
		char	nick[BUFSIZ], pass[BUFSIZ], version[BUFSIZ];
		int	ver;

		/* we got a tinygrufti, so we've got ver, nick, pass */
		d->flags |= STAT_TINYGRUFTI;
		split(version,buf);
		ver = atoi(version);

		/* Check version first */
		if(ver < INTERFACE_REQUIREDVER)
		{
			Log_write(LOG_CMD,"*","Gateway client is version %d (current is %d)",ver,INTERFACE_REQUIREDVER);
			DCC_write(d,"MESSAGE 0: Your TinyGrufti interface is no longer current.  The required version is v%d.  You have v%d.  Please download a newer version as soon as possible!",INTERFACE_REQUIREDVER,ver);
			return;
		}
	
		split(nick,buf);
		split(pass,buf);
		if(!nick[0] || !pass[0])
		{
			DCC_disconnect(d);
			return;
		}

		d->flags &= ~STAT_WAITINGFORID;
		if(DCC_gotlogin(d,nick))
			DCC_gotpassword(d,pass);
	}
	else if(strcmp(id,"grufti-interface") == 0)
	{
		char	nick[BUFSIZ], pass[BUFSIZ], version[BUFSIZ];
		int	ver;

		/* we got a gb-interface, so we've got ver, nick, pass */
		split(version,buf);
		ver = atoi(version);

		/* Check version first */
		if(ver < INTERFACE_REQUIREDVER)
		{
			Log_write(LOG_CMD,"*","Gateway client is version %d (current is %d)",ver,INTERFACE_REQUIREDVER);
			DCC_write(d,"** Your Grufti interface is no longer current.");
			DCC_write(d,"** The required version is V%d.  You have V%d.",INTERFACE_REQUIREDVER,ver);
			DCC_write(d,"** Please download a newer version as soon as possible, thanks!");
			DCC_write(d,"");
			return;
		}
	
		split(nick,buf);
		split(pass,buf);
		if(!nick[0] || !pass[0])
		{
			DCC_disconnect(d);
			return;
		}

		d->flags &= ~STAT_WAITINGFORID;
		if(DCC_gotlogin(d,nick))
			DCC_gotpassword(d,pass);
	}
#ifdef WEB_MODULE
	else if(strcmp(id,"web-interface") == 0)
	{
		char	nick[BUFSIZ], pass[BUFSIZ];
		char	host[UHOSTLEN], uh[UHOSTLEN];

		context;
		/* ok we've got a web client.  We got user, pass, and host */
		split(nick,buf);
		split(pass,buf);
		split(host,buf);
		if(!nick[0] || !pass[0] || !host[0])
		{
			DCC_disconnect(d);
			return;
		}
		context;
		sprintf(uh,"unknown@%s",host);
		d->flags |= STAT_WEBCLIENT;
		d->flags &= ~STAT_WAITINGFORID;

		context;
		/* 1st, reset our hostname */
		dccrec_setuh(d,uh);
		context;
		if(DCC_gotlogin(d,nick))
			DCC_gotpassword(d,pass);
		context;
	}
#endif
	else
	{
		/* nope.  client is no good. */
		Log_write(LOG_CMD | LOG_ERROR | LOG_MAIN,"*","Gateway client is not authorized (got \"%s\").",id);
		DCC_disconnect(d);
		return;
	}

}

int DCC_gotlogin(dccrec *d, char *nick)
{
	nickrec	*n;

	context;
	/* Locate record with 'nick' */
	d->user = User_user(nick);

	context;
	/* Only registered user can login via telnet */
	if(!d->user->registered)
	{
#ifdef WEB_MODULE
		if(d->flags & STAT_WEBCLIENT)
			Web_saynotregistered(d,nick);
		else
#endif
			DCC_write(d,"Sorry, the nick \"%s\" is not registered.",nick);
		Log_write(LOG_CMD,"*","Invalid login nick \"%s\" from %s.",nick,d->uh);
		d->login_attempts++;
		if(d->login_attempts >= Grufti->login_attempts || (d->flags & STAT_GATEWAYCLIENT))
		{
			Log_write(LOG_CMD,"*","Closed telnet connection to %s.",d->uh);
			DCC_disconnect(d);
			return 0;
		}

		DCC_write(d,"");
		DCC_write_nocr(d,"login: ");
		return 1;
	}

	context;
	if(d->user->pass == NULL)
	{
		DCC_write(d,"This account has no password.  Please set it using 'setpass' from IRC.");
		DCC_disconnect(d);
		return 0;
	}

	context;
	/* Get the case of their actual nick :> */
	n = User_nick(d->user,nick);

	/* Ok we've assigned their user record and account to the dcc record. */
	if(n != NULL)
		dccrec_setnick(d,n->nick);
	else
		dccrec_setnick(d,nick);
	d->account = User_account(d->user,d->nick,d->uh);

	context;
	/* Show that this user has logged in via the web */
	d->user->via_web++;

	context;

	/* If we're a gateway client, don't bother with prompt for password */
	if(d->flags & STAT_GATEWAYCLIENT)
		return 1;

	/* now let's check and see if they're really who they say they are... */
	d->flags &= ~STAT_LOGIN;
	DCC_dopassword(d);

	return 1;
}

void DCC_dopassword(dccrec *d)
{
	context;
	d->flags |= STAT_PASSWORD;

	DCC_write_nocr(d,"Password: \377\373\001");
}

void DCC_gotpassword(dccrec *d, char *password)
{
	char cipher[PASSWORD_CIPHER_LEN];

	context;
	/* Turn back on remote echo */
	if(!(d->flags & STAT_GATEWAYCLIENT))
		DCC_write_nocr(d,"\377\374\001");

	/* We don't encrypt if it's a gateway via the web */
#ifdef WEB_MODULE
	if(d->flags & STAT_WEBCLIENT)
		strcpy(cipher,password);
	else
#endif
		encrypt_pass(password,cipher);


	context;
	/* check password */
	if(!isequalcase(cipher,d->user->pass))
	{
		/* Bad password */
#ifdef WEB_MODULE
		if(d->flags & STAT_WEBCLIENT)
			Web_sayinvalidpassword(d,d->user->acctname);
		else
#endif
		{
			DCC_write(d,"");
			DCC_write(d,"Incorrect password.");
			DCC_write(d,"");
		}

		d->login_attempts++;
		if(d->login_attempts >= Grufti->login_attempts || (d->flags & STAT_GATEWAYCLIENT))
		{
			/* Too many attempts. */
			Log_write(LOG_CMD,"*","Closed telnet connection to %s.",d->uh);
			DCC_disconnect(d);
			return;
		}

		/* Try again */
		d->flags &= ~STAT_PASSWORD;
		d->flags |= STAT_LOGIN;
		DCC_write_nocr(d,"login: ");
		return;
	}


	context;
	/* Password good.  Register all accounts with this userhost */
	User_updateregaccounts(d->user,d->account->uh);
	User_checkforsignon(d->nick);

	/* Clear their last signoff message */
	xfree(d->user->signoff);
	d->user->signoff = NULL;
	d->user->last_login = time(NULL);

	context;
	d->flags &= ~STAT_PASSWORD;
	d->flags |= STAT_VERIFIED;

	context;
	DCC_write(d,"");
	DCC_dosuccessfullogin(d);
}
	
void DCC_dosuccessfullogin(dccrec *d)
{
	int	new;
	locarec	*l;
	char	motd[256];

	context;
	d->flags &= ~STAT_PENDING_LOGIN;

	if(d->flags & STAT_WEBCLIENT)
		Log_write(LOG_CMD|LOG_RAW,"*","W%d %s %s - Logged in.",d->socket,d->nick,d->uh);
	else if(d->flags & STAT_GATEWAYCLIENT)
		Log_write(LOG_CMD|LOG_RAW,"*","G%d %s %s - Logged in.",d->socket,d->nick,d->uh);
	else
		Log_write(LOG_CMD|LOG_RAW,"*","T%d %s %s - Logged in.",d->socket,d->nick,d->uh);

	context;
	/* Show that this nick is connected via telnet */
	User_nickviatelneton(d->user,d);

	/* Don't bother with motd and notes if it's via the gateway */
	if(d->flags & STAT_GATEWAYCLIENT)
	{
		DCC_showprompt(d);
		return;
	}

	/* Show motd */
	if(Grufti->show_motd_on_telnet)
	{
		if(!Grufti->show_motd_onlywhenchanged || !d->user->registered)
		{
			sprintf(motd,"%s/%s",Grufti->admindir,Grufti->motd_telnet);
			catfile(d,motd);
		}
		else if(d->user->seen_motd <= Grufti->motd_modify_date)
		{
			d->user->seen_motd = time(NULL);
			sprintf(motd,"%s/%s",Grufti->admindir,Grufti->motd_telnet);
			catfile(d,motd);
		}
		else
		{
			char when1[TIMELONGLEN], when2[TIMELONGLEN];
			timet_to_date_short(Grufti->motd_modify_date,when1);
			timet_to_date_short(d->user->seen_motd,when2);
			DCC_write(d,"");
			DCC_write(d,"MOTD unchanged since %s.  (Last read: %s)",when1,when2);
			DCC_write(d,"");
		}
			
	}

	context;

	/* Display location */
	l = Location_location(d->user->location);
	if(l != NULL)
		DCC_write(d,"Hello %s in %s!",d->user->acctname,l->city);
	else
		DCC_write(d,"Hello %s!",d->user->acctname);

	/* Display number of Notes */
	new = Note_numnew(d->user);
	if(!d->user->num_notes)
		DCC_write(d,"You have no new Notes.");
	else if(new)
		DCC_write(d,"You have %d new Note%s. (%d total)",new,new==1?"":"s",d->user->num_notes);
	else
		DCC_write(d,"You have no new Notes. (%d total)",d->user->num_notes);

	context;
	DCC_write(d,"");
	DCC_showprompt(d);

	/* Done.  wait for async i/o. */
}
	
void DCC_docommand(dccrec *d, char *buf)
{
	char	*p;

	/* fix buf so we don't get emtpy fields */
	rmspace(buf);

	/* Command variable must be preset with user info. */
	strcpy(cmdinfo.from_n,d->nick);
	strcpy(cmdinfo.from_uh,d->uh);
	strcpy(cmdinfo.from_u,d->uh);
	p = strchr(cmdinfo.from_u,'@');
	if(p != NULL)
		*p = 0;
	strcpy(cmdinfo.from_h,d->uh);
	splitc(NULL,cmdinfo.from_h,'@');
	strcpy(cmdinfo.buf,buf);

	/* Set DCC connection record */
	cmdinfo.d = d;

	/* Set user and account pointers */
	cmdinfo.user = d->user;
	cmdinfo.account = d->account;
	
	/* Now we're ready. */
	command_do();
}

void DCC_write(dccrec *d, char *format, ...)
{
	va_list	va;
	int	bytes;

	/* Error checking. */
	if(d == NULL)
		return;

	/* Load buffer */
	DCC->send_buf[0] = 0;
	va_start(va,format);
	vsprintf(DCC->send_buf,format,va);
	va_end(va);

	if(d->flags & (CLIENT_IRC_CHAT|CLIENT_SMTP|STAT_GATEWAYCLIENT))
		strcat(DCC->send_buf,"\n");
	else
		strcat(DCC->send_buf,"\r\n");


	/* Error checking. */
	if(d->socket < 0)
	{
		Log_write(LOG_DEBUG,"*","Oops.  Attempted DCC_write to negative socket. %s (%s) \"%s\"",d->nick,d->uh,DCC->send_buf);
		return;
	}

	/* update record */
	bytes = strlen(DCC->send_buf);
	d->bytes_out += bytes;

	/* update device */
	DCC->bytes_out += bytes;

#ifdef RAW_LOGGING
	Log_write(LOG_RAW,"*","--> DCC %d: %s",d->socket,DCC->send_buf);
#endif
	net_sockputs(d->socket,DCC->send_buf, bytes);
}
	
void DCC_write_nolog(dccrec *d, char *format, ...)
/*
 * this command is called from Log_write(), so DON'T do any logging here
 * at all!
 */
{
	va_list va;
	int bytes;

	/* Error checking. */
	if(d == NULL)
		return;

	/* Load buffer */
	DCC->send_buf1[0] = 0;
	va_start(va,format);
	vsprintf(DCC->send_buf1,format,va);
	va_end(va);

	if(d->flags & (CLIENT_IRC_CHAT|CLIENT_SMTP|STAT_GATEWAYCLIENT))
		strcat(DCC->send_buf1,"\n");
	else
		strcat(DCC->send_buf1,"\r\n");

	/* Error checking. */
	if(d->socket < 0)
		return;

	/* update record */
	bytes = strlen(DCC->send_buf1);
	d->bytes_out += bytes;

	/* update device */
	DCC->bytes_out += bytes;

	net_sockputs(d->socket, DCC->send_buf1, bytes);
}

void DCC_write_nocr(dccrec *d, char *format, ...)
{
	va_list va;
	int bytes;

	DCC->send_buf[0] = 0;
	va_start(va,format);
	vsprintf(DCC->send_buf,format,va);
	va_end(va);

	/* Error checking. */
	if(d->socket < 0)
	{
		Log_write(LOG_DEBUG,"*","Oops.  Attempted DCC_write to negative socket. %s (%s) \"%s\"",d->nick,d->uh,DCC->send_buf);
		return;
	}

	/* update record */
	bytes = strlen(DCC->send_buf);
	d->bytes_out += bytes;

	/* update device */
	DCC->bytes_out += bytes;

#ifdef RAW_LOGGING
	Log_write(LOG_RAW,"*","--> DCC %d: %s (nocr)",d->socket,DCC->send_buf);
#endif
	net_sockputs(d->socket, DCC->send_buf, bytes);
}
	
void DCC_flags2str(u_long flags, char *s)
{
	s[0] = 0;
	if(flags & DCC_SERVER)		strcat(s,"DCC_SERVER,");
	if(flags & DCC_CLIENT)		strcat(s,"DCC_CLIENT,");
	if(flags & SERVER_PUBLIC)	strcat(s,"SERVER_PUBLIC,");
	if(flags & SERVER_INTERNAL)	strcat(s,"SERVER_INTERNAL,");
	if(flags & SERVER_IRC_CHAT)	strcat(s,"SERVER_IRC_CHAT,");
	if(flags & SERVER_XFER)		strcat(s,"SERVER_XFER,");
	if(flags & CLIENT_IDENT)	strcat(s,"CLIENT_IDENT,");
	if(flags & CLIENT_IRC_CHAT)	strcat(s,"CLIENT_IRC_CHAT,");
	if(flags & CLIENT_TELNET_CHAT)	strcat(s,"CLIENT_TELNET_CHAT,");
	if(flags & CLIENT_XFER)		strcat(s,"CLIENT_XFER,");
	if(flags & CLIENT_SMTP)		strcat(s,"CLIENT_SMTP,");
	if(flags & STAT_LOGIN)		strcat(s,"STAT_LOGIN,");
	if(flags & STAT_PASSWORD)	strcat(s,"STAT_PASSWORD,");
	if(flags & STAT_PENDING_IDENT)	strcat(s,"STAT_PENDING_IDENT,");
	if(flags & STAT_PENDING_LOGIN)	strcat(s,"STAT_PENDING_LOGIN,");
	if(flags & STAT_COMMAND_PENDING)	strcat(s,"STAT_COMMAND_PENDING,");
	if(flags & STAT_COMMAND_SETUP)	strcat(s,"STAT_COMMAND_SETUP,");
	if(flags & STAT_COMMAND_REGISTER)	strcat(s,"STAT_COMMAND_REGISTER,");
	if(flags & STAT_DISCONNECT_ME)	strcat(s,"STAT_DISCONNECT_ME,");
	if(flags & STAT_VERIFIED)	strcat(s,"STAT_VERIFIED,");
	if(flags & STAT_GATEWAYCLIENT)	strcat(s,"STAT_GATEWAYCLIENT,");
	if(flags & STAT_TINYGRUFTI)	strcat(s,"STAT_TINYGRUFTI,");
	if(flags & STAT_XFER_IN)	strcat(s,"STAT_XFER_IN,");
	if(flags & STAT_XFER_OUT)	strcat(s,"STAT_XFER_OUT,");
	if(flags & STAT_WAITINGFORID)	strcat(s,"STAT_WAITINGFORID,");
	if(flags & STAT_WEBCLIENT)	strcat(s,"STAT_WEBCLIENT,");
	if(flags & STAT_DONTSHOWPROMPT)	strcat(s,"STAT_DONTSHOWPROMPT,");
	if(flags & STAT_SMTP_MAILFROM)	strcat(s,"STAT_SMTP_MAILFROM,");
	if(flags & STAT_SMTP_RCPTTO)	strcat(s,"STAT_SMTP_RCPTTO,");
	if(flags & STAT_SMTP_DATA)	strcat(s,"STAT_SMTP_DATA,");
	if(flags & STAT_SMTP_QUIT)	strcat(s,"STAT_SMTP_QUIT,");
	if(s[0] == 0)
		strcat(s,"NOFLAGS");
	else
		s[strlen(s) - 1] = 0;
}

void DCC_checkfortimeout()
{
	dccrec *d, *v;
	time_t now = time(NULL);

	d = DCC->dcclist;
	while(d != NULL)
	{
		v = d->next;

		context;
		/* Check for disconnect flag if we didn't get it before. */
		if(d->flags & STAT_DISCONNECT_ME)
		{
			context;
			Log_write(LOG_DEBUG,"*","%s %s - Floating connection on sock %d marked DISCONNECT...",d->nick,d->uh,d->socket);
			DCC_disconnect(d);
		}
		else if(d->flags & CLIENT_SMTP)
		{
			if((now - d->last) > Grufti->smtp_timeout)
			{
				Log_write(LOG_ERROR,"*","%s - SMTP timeout to %s",d->nick,Grufti->smtp_gateway);
				DCC_disconnect(d);
			}
		}
		else if(d->flags & CLIENT_IDENT)
		{
			context;
			if((now - d->last) > Grufti->chat_ident_timeout)
			{
				Log_write(LOG_CMD,"*","%s %s - Ident timeout on sock %d.",d->nick,d->uh,d->socket);

			context;
				/* login normal connection. */
				d->id->flags &= ~STAT_PENDING_IDENT;
				DCC_dologin(d->id);

			context;
				/* and close ident socket */
				DCC_disconnect(d);
			}
		}
		else if(d->flags & STAT_PENDING_LOGIN)
		{
			context;
			if((now - d->last) > Grufti->chat_login_timeout)
			{
			context;
				Log_write(LOG_CMD,"*","%s %s - Timeout waiting for login on sock %d.",d->nick,d->uh,d->socket);
				DCC_write(d,"Login timed out after %d seconds.",Grufti->chat_login_timeout);
			context;
				DCC_disconnect(d);
			}
		}
		else if(d->flags & (CLIENT_IRC_CHAT|CLIENT_TELNET_CHAT))
		{

			context;
			if((now - d->last) > Grufti->chat_timeout)
			{
				char	ago[TIMESHORTLEN];

				context;
				timet_to_ago_short(d->last,ago);
				Log_write(LOG_CMD,"*","%s %s - Timeout. (No activity in %s)",d->nick,d->uh,ago);
				DCC_write(d,"No activity in %s.  Bye!",ago);
				DCC_disconnect(d);
			}
		}
		else if(d->flags & (SERVER_IRC_CHAT|SERVER_XFER))
		{
			context;
			if((now - d->last) > Grufti->establish_dcc_timeout)
			{
				Log_write(LOG_CMD,"*","S%d %s %s - Timeout waiting for DCC connect.",d->socket, d->nick, d->uh);
				DCC_disconnect(d);
			}
		}

		d = v;
	}
}

dccrec *DCC_sendchat(char *nick, char *uh)
{
	dccrec *d;

	context;
	/* Open server connection and wait for chat */
	d = DCC_addnewconnection(nick,uh,0,0,Grufti->chat_port,DCC_SERVER|SERVER_IRC_CHAT);
	if(d == NULL)
		return NULL;

	context;
	/* Send the user a DCC CHAT request. */
	Server_write(PRIORITY_HIGH,"PRIVMSG %s :\001DCC CHAT chat %lu %d\001",d->nick,ntohl(myip()),d->port);

	return d;
}

int DCC_isuhindcc(char *uh)
{
	dccrec *d;
	int found = 0;

	context;
	for(d=DCC->dcclist; d!=NULL; d=d->next)
		if((d->flags & CLIENT_IRC_CHAT) && isequal(d->uh,uh))
			found++;

	return found;
}

void DCC_resetallpointers()
{
	dccrec *d;

	context;
	for(d=DCC->dcclist; d!=NULL; d=d->next)
	{
		if((d->flags & CLIENT_IRC_CHAT) || (d->flags & CLIENT_SMTP) || ((d->flags & CLIENT_TELNET_CHAT) && !(d->flags & STAT_PENDING_LOGIN)))
		{
			d->user = User_user(d->nick);
			d->account = User_account(d->user,d->nick,d->uh);
		}
	}
}

void DCC_showprompt(dccrec *d)
{
	int	new;

	context;
	if(d->flags & STAT_GATEWAYCLIENT)
	{
		if(!(d->flags & STAT_DONTSHOWPROMPT))
			DCC_write(d,"OKOK>");
		return;
	}

	context;
	/* Display number of Notes */
	new = Note_numnew(d->user);
	if(new)
		DCC_write_nocr(d,"%s (%d new Note%s)> ",Grufti->botnick,new,new==1?"":"s");
	else
		DCC_write_nocr(d,"%s> ",Grufti->botnick);
}


void DCC_strip_telnet(dccrec *d, char *buf, int *len)
{
	u_char *p = (u_char *)buf;
	int mark;

	context;
	while(*p != 0)
	{
		while((*p != 255) && (*p != 0))
			p++;   /* search for IAC */
		if(*p == 255)
		{
			p++;
			mark = 2;
			if(!*p)
				mark=1;  /* bogus */
			if((*p >= 251) && (*p <= 254))
			{
				mark = 3;
				if(!*(p+1))
					mark=2;  /* bogus */
			}
			if(*p == 251)
			{
				/* WILL X -> response: DONT X */
				/* except WILL ECHO: we just smile and ignore */
				if(!(*(p+1) == 1))
				{
					write(d->socket,"\377\376",2);
					write(d->socket,p+1,1);
				}
			}
			if(*p == 253)
			{
				/* DO X -> response: WONT X */
				/* except DO ECHO: we just smile and ignore */
				if(!(*(p+1) == 1))
				{
					write(d->socket,"\377\374",2);
					write(d->socket,p+1,1);
				}
			}
			if(*p == 246)
			{
				/* "are you there?" */
				/* response is: "hell yes!" */
				write(d->socket,"\r\nHell, yes!\r\n",14);
			}
			/* anything else can probably be ignored */
			p--;

			/* wipe code from buffer */
			strcpy((char *)p,(char *)(p+mark));
			*len = *len - mark;
		}
	}
	context;
}

void send_to_user(dccrec *d, char *nick, char *format, ...)
{
	va_list va;

	send_to_user_buf[0] = 0;
	va_start(va,format);
	vsprintf(send_to_user_buf,format,va);
	va_end(va);

	/* Recipient does not exist over DCC.  Send to nick. */
	if(d == NULL)
	{
		Server_write(PRIORITY_NORM,"PRIVMSG %s :%s",nick,send_to_user_buf);
		return;
	}
	/* Recipient has a DCC record.  Use it. */
	else
		DCC_write(d, "%s", send_to_user_buf);

}

void DCC_opentelnetservers()
{
	context;
	DCC_addnewconnection("Server001",Grufti->botuh,0,0,Grufti->reqd_telnet_port,DCC_SERVER|SERVER_PUBLIC);
	DCC_addnewconnection("Server002",Grufti->botuh,0,0,Grufti->reqd_gateway_port,DCC_SERVER|SERVER_INTERNAL);
}

dccrec *DCC_dccserverbyname(char *name)
{
	dccrec *d;

	context;
	for(d=DCC->dcclist; d!=NULL; d=d->next)
		if((d->flags & DCC_SERVER) && isequal(d->nick,name))
			return d;

	return NULL;
}

void DCC_ondccchat(dccrec *d)
/*
 * What to do after a DCC chat has been established.
 */
{
	int	new;
	locarec	*l;
	char	motd[256], newuser[256];

	/* Do i know the user? */
	context;
	if(!d->user->registered)
	{
		if(Grufti->welcome_newdcc)
		{
			sprintf(newuser, "%s/%s", Grufti->admindir, Grufti->welcome_dcc);
			catfile(d, newuser);
		}
	}
	else
	{
		if(Grufti->show_motd_on_dcc_chat)
		{
			if(!Grufti->show_motd_onlywhenchanged || !d->user->registered)
			{
				sprintf(motd,"%s/%s",Grufti->admindir,Grufti->motd_dcc);
				catfile(d,motd);
			}
			else if(d->user->seen_motd <= Grufti->motd_modify_date)
			{
				d->user->seen_motd = time(NULL);
				sprintf(motd,"%s/%s",Grufti->admindir,Grufti->motd_dcc);
				catfile(d,motd);
			}
			else
			{
				char when1[TIMELONGLEN], when2[TIMELONGLEN];
				timet_to_date_short(Grufti->motd_modify_date,when1);
				timet_to_date_short(d->user->seen_motd,when2);
				DCC_write(d,"");
				DCC_write(d,"MOTD unchanged since %s.  (Last read: %s)",when1,when2);
				DCC_write(d,"");
			}
		}

		/* Show locations */
		l = Location_location(d->user->location);
		if(l != NULL)
			DCC_write(d,"Hello %s in %s!",d->user->acctname,l->city);
		else
			DCC_write(d,"Hello %s!",d->user->acctname);

		new = Note_numnew(d->user);
		if(!d->user->num_notes)
			DCC_write(d,"You have no new Notes.");
		else if(new)
			DCC_write(d,"You have %d new Note%s. (%d total)",new,new==1?"":"s",d->user->num_notes);
		else
			DCC_write(d,"You have no new Notes. (%d total)",d->user->num_notes);
	}
	context;
}

int DCC_isnickindcc(char *nick)
{
	dccrec *d;

	context;
	for(d=DCC->dcclist; d!=NULL; d=d->next)
		if(isequal(d->nick,nick))
			return 1;

	return 0;
}

void DCC_bootuser(dccrec *d, char *reason)
{
	context;
	if((d->flags & CLIENT_IRC_CHAT) || (d->flags & CLIENT_TELNET_CHAT))
	{
		if(!(d->flags & CLIENT_XFER))
		{
			DCC_write(d,"");
			if(reason != NULL)
				DCC_write(d,"*** %s",reason);
			DCC_write(d,"*** Bye!");
		}
		DCC_disconnect(d);
	}
}

void DCC_bootallusers(char *reason)
{
	dccrec *d;

	for(d=DCC->dcclist; d!=NULL; d=d->next)
		DCC_bootuser(d,reason);
}

int DCC_orderbystarttime()
{
	dccrec	*d, *d_save = NULL;
	int	counter = 0, done = 0;
	time_t	l, lowest = 0L;

	/* We need to clear all ordering before we start */
	DCC_clearorder();

	/* Continue until all entries have an order number */
	while(!done)
	{
		done = 1;
		l = lowest;
		for(d=DCC->dcclist; d!=NULL; d=d->next)
		{
			if(!d->order && d->starttime >= l)
			{
				done = 0;
				l = d->starttime;
				d_save = d;
			}
		}

		/* Not done means we got u_save and now we need to order it */
		if(!done)
		{
			counter++;
			d_save->order = counter;
		}
	}

	return counter;
}

dccrec *DCC_dccbyordernum(int ordernum)
{
	dccrec	*d;

	for(d=DCC->dcclist; d!=NULL; d=d->next)
		if(d->order == ordernum)
			return d;

	return NULL;
}

void DCC_clearorder()
{
	dccrec	*d;

	for(d=DCC->dcclist; d!=NULL; d=d->next)
		d->order = 0;
}

void DCC_sendfile(dccrec *d, char *nick, char *uh, char *file)
{
	FILE	*f;
	char	filepath[256], n1[50];
	dccrec	*d_xfer;
	struct stat ss;

	sprintf(filepath,"%s/%s",Grufti->xfer_files,file);
	f = fopen(filepath,"r");
	if(f == NULL)
		return;

	d_xfer = DCC_addnewconnection(nick,uh,0,0,Grufti->chat_port,DCC_SERVER|SERVER_XFER);

	stat(filepath,&ss);

	/*
	 * Remember, this is the SERVER dcc.  So we'll need to copy the file
	 * info from it when we establish the actual xfer connection.
	 */
	d_xfer->length = ss.st_size;
	dccrec_setfilename(d_xfer,file);
	d_xfer->f = f;

	sendprivmsg(nick,"\001DCC SEND %s %lu %d %lu\001",d_xfer->filename,ntohl(myip()),d_xfer->port,d_xfer->length);

	comma_number(n1,d_xfer->length);
	Log_write(LOG_CMD,"*","Waiting to send \"%s\" to %s (%s bytes)",d_xfer->filename,nick,n1);

	send_to_user(d,nick,"Sending you \"%s\" (%s bytes).",d_xfer->filename,n1);
	send_to_user(d,nick,"Type \"/dcc get %s %s\" to receive.",Server->currentnick,d_xfer->filename);
}

int DCC_forwardnotes(userrec *u, char *forward)
{
	dccrec	*emailclient;

	emailclient = DCC_addnewconnection(u->acctname,forward,hostnametoip(Grufti->smtp_gateway),0,25,DCC_CLIENT|CLIENT_SMTP);
	if(emailclient == NULL)
	{
		Log_write(LOG_ERROR,"*","%s - Couldn't connect to smtp server on %s.  Oh well.",u->acctname,Grufti->smtp_gateway);
		return 0;
	}

	emailclient->user = u;

	return 1;
}


/* ASDF */
/***************************************************************************n
 *
 * dcc record definitions.  No changes should be made to new(), freeall(),
 * append(), delete(), or movetofront().
 *
 ****************************************************************************/

dccrec *dccrec_new()
{
	dccrec *x;

	/* allocate memory */
	x = (dccrec *)xmalloc(sizeof(dccrec));

	/* initialize */
	x->next = NULL;

	dccrec_init(x);

	return x;
}

void dccrec_freeall(dccrec **dcclist, dccrec **last)
{
	dccrec *d = *dcclist, *v;

	while(d != NULL)
	{
		v = d->next;
		dccrec_free(d);
		d = v;
	}

	*dcclist = NULL;
	*last = NULL;
}

void dccrec_append(dccrec **list, dccrec **last, dccrec *entry)
{
	dccrec *lastentry = *last;

	if(*list == NULL)
		*list = entry;
	else
		lastentry->next = entry;

	*last = entry;
	entry->next = NULL;
}

void dccrec_delete(dccrec **list, dccrec **last, dccrec *x)
{
	dccrec *d, *lastchecked = NULL;

	d = *list;
	while(d != NULL)
	{
		if(d == x)
		{
			if(lastchecked == NULL)
			{
				*list = d->next;
				if(d == *last)
					*last = NULL;
			}
			else
			{
				lastchecked->next = d->next;
				if(d == *last)
					*last = lastchecked;
			}

			dccrec_free(d);
			return;
		}

		lastchecked = d;
		d = d->next;
	}
}

void dccrec_movetofront(dccrec **list, dccrec **last, dccrec *lastchecked, dccrec *d)
{
	if(lastchecked != NULL)
	{
		if(*last == d)
			*last = lastchecked;

		lastchecked->next = d->next;
		d->next = *list;
		*list = d;
	}
}

/****************************************************************************/
/* record dccrec low-level data-specific function definitions. */

void dccrec_init(dccrec *d)
{
	/* initialize */
	d->socket = 0;
	d->ip = 0L;
	d->port = 0;
	d->flags = 0L;
	d->nick = NULL;
	d->uh = NULL;
	d->user = NULL;
	d->account = NULL;
	d->starttime = 0L;
	d->last = 0L;
	d->bytes_in = 0;
	d->bytes_out = 0;
	d->login_attempts = 0;
	d->order = 0;
	d->logtype = 0;
	d->logchname = NULL;

	/* chat stuff */
	d->msgs_per_sec = 0;

	/* xfer stuff */
	d->filename = NULL;
	d->length = 0L;
	d->sent = 0L;
	d->buf = NULL;
	d->sofar = 0;
	d->pending = 0L;
	d->from = NULL;
	d->f = NULL;

	/* ident stuff */
	d->id = NULL;

	/* command pending */
	d->command = NULL;
	d->commandfromnick = NULL;
	d->commandtime = 0;

	/* tinygrufti header */
	d->tinygrufti_header[0] = 0;

	/* ipc */
	d->ipc = 0L;
}

u_long dccrec_sizeof(dccrec *d)
{
	u_long	tot = 0L;

	tot += sizeof(dccrec);
	tot += d->nick ? strlen(d->nick)+1 : 0L;
	tot += d->uh ? strlen(d->uh)+1 : 0L;
	tot += d->logchname ? strlen(d->logchname)+1 : 0L;
	tot += d->filename ? strlen(d->filename)+1 : 0L;
	tot += d->buf ? strlen(d->buf)+1 : 0L;
	tot += d->from ? strlen(d->from)+1 : 0L;
	tot += d->command ? strlen(d->command)+1 : 0L;
	tot += d->commandfromnick ? strlen(d->commandfromnick)+1 : 0L;

	return tot;
}

void dccrec_free(dccrec *d)
{
	/* Free the elements. */
	xfree(d->nick);
	xfree(d->uh);
	xfree(d->logchname);
	xfree(d->filename);
	xfree(d->buf);
	xfree(d->from);
	xfree(d->command);
	xfree(d->commandfromnick);

	/* Free this record. */
	xfree(d);
}

void dccrec_setnick(dccrec *d, char *nick)
{
	mstrcpy(&d->nick,nick);
}

void dccrec_setuh(dccrec *d, char *uh)
{
	mstrcpy(&d->uh,uh);
}
			
void dccrec_setfilename(dccrec *d, char *filename)
{
	mstrcpy(&d->filename,filename);
}
			
void dccrec_setfrom(dccrec *d, char *from)
{
	mstrcpy(&d->from,from);
}
			
void dccrec_setcommand(dccrec *d, char *command)
{
	mstrcpy(&d->command,command);
}

void dccrec_setcommandfromnick(dccrec *d, char *commandfromnick)
{
	mstrcpy(&d->commandfromnick,commandfromnick);
}

void dccrec_setlogchname(dccrec *d, char *logchname)
{
	mstrcpy(&d->logchname,logchname);
}

void dccrec_display(dccrec *d)
{
	char flags[DCCFLAGLEN], estab[TIMESHORTLEN], idle[TIMESHORTLEN];
	char uh[UHOSTLEN];

	timet_to_time_short(d->starttime,estab);
	timet_to_ago_short(d->last,idle);
	DCC_flags2str(d->flags,flags);
	strcpy(uh,d->uh);
	if(strlen(uh) > 30)
		uh[30] = 0;

	say("%s %4s %-9s %-30s %3dk %3dk",estab,idle,d->nick,uh,d->bytes_in/1024,d->bytes_out/1024);
	sayf(0,0,"  --> %d, %d, %s",d->socket,d->port,flags);
}

dccrec *dccrec_dcc(dccrec **list, dccrec **last, int socket)
{
	dccrec *d, *lastchecked = NULL;

	for(d=*list; d!=NULL; d=d->next)
	{
		if(d->socket == socket)
		{
			/* found it.  do a movetofront. */
			dccrec_movetofront(list,last,lastchecked,d);

			return d;
		}
		lastchecked = d;
	}

	return NULL;
}
