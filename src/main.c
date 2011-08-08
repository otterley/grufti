/******************************************************************************
 * Steve Rider - stever@area23.org
 * Grufti - an interactive, service-oriented bot for IRC 
 *
 * main.c - the scheduler
 *
 *****************************************************************************/
/* 28 April 97 */

#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include "grufti.h"
#include "log.h"
#include "user.h"
#include "channel.h"
#include "net.h"
#include "server.h"
#include "sigcatch.h"
#include "dcc.h"
#include "response.h"
#include "codes.h"
#include "location.h"
#include "misc.h"
#include "notify.h"
#include "note.h"
#include "url.h"
#include "banlist.h"
#include "event.h"
#if 0
#include "tgrufti.h"
#endif

#ifdef WEB_MODULE
#include "web.h"
#endif
void mem_status();

/* PUBLIC MAIN OBJECTS */
Grufti_ob	*Grufti;
Log_ob		*Log;
Server_ob	*Server;
User_ob		*User;
Channel_ob	*Channel;
DCC_ob		*DCC;
Response_ob	*Response;
Codes_ob	*Codes;
Location_ob	*Location;
Notify_ob	*Notify;
Note_ob		*Note;
URL_ob		*URL;
#ifdef WEB_MODULE
Web_ob		*Web;
#endif
Banlist_ob	*Banlist;
Event_ob	*Event;
#if 0
TinyGrufti_ob	*TinyGrufti;
#endif

#ifdef MSK
char		mergeusers[MAXPATHLEN];
#endif

/*
 * prototypes for init routines (so we don't have to include the whole header)
 */
void trigger_init();
void command_init();

/* context logging */
char cx_file[30];
int cx_line;

/* Global timers */
time_t	now, then, last_ten = 0L, last_sixty = 0L, last_day, timecnt = 0;

/* Prototypes */
void	everysecond();
u_long	main_sizeof();
void	writepid(int pid);
void	do_arg(char *arg);
int	isbotrunning();
void	writestats();
void	readstats();
void	do_test();
int	last_hour = -1;

int main(int argc, char **argv)
{
	char	s[1024], buf[MAXREADBUFLENGTH], started[DATELONGLEN];
	dccrec	*d;
	int	xx, i = 0, errors;

	init_mem();

#ifdef MSK
	mergeusers[0] = '\0';
#endif

	/* Initialize Grufti object (holds all bot settings) */
	Grufti_new();

	/* Parse command-line arguments, if any */
	if(argc > 1)
		for(i=1; i<argc; i++)
			do_arg(argv[i]);

	/* Check for running bot */
	if(isbotrunning())
	{
		printf("Already running...\n");
		exit(1);
	}

context;
	/* Normal startup */
	printf("%s\n",Grufti->copyright);
	printf("v%s",Grufti->build);
#ifdef WEB_MODULE
	printf(" (+web_module)");
#endif
#ifdef RAW_LOGGING
	printf(" (+raw_logging)");
#endif
	printf("\n\n");

	printf("Initializing main objects:\n");
	printf("  Grufti, Log, Server, User, Channel, Command, DCC, Banlist,\n");
#ifdef WEB_MODULE
	printf("  Response, Codes, Location, Event, Notify, Note, URL, Web\n");
#else
	printf("  Response, Codes, Location, Event, Notify, Note, URL\n");
#endif

context;
	/* Setup signal catching and network sockets */
	sigcatch_init();
	net_init(NULL);  /* will be passed Grufti->hostname, if set, when done */

	/* Initialize main objects (order is important) */
	Log_new();
	Server_new();
	User_new();
	Channel_new();
	trigger_init();
	DCC_new();
context;
	Response_new();
	Codes_new();
	Location_new();
	Notify_new();
	Note_new();
	URL_new();
context;
#ifdef WEB_MODULE
	Web_new();
#endif
	Banlist_new();
	Event_new();
#if 0
	TinyGrufti_new();
#endif
	
	printf("\n");
	printf("Loading:\n");

context;
	/* initialize commands (must be loaded before configfile) */
	command_init();
#if 0
	TinyGrufti_loadcommands();
#endif
#ifdef WEB_MODULE
	Web_loadcommands();
#endif

context;
	/* Load configurations */
	printf("  configs:    ");
context;
	xx = Grufti_readconfigfile(&errors);
context;
	if(errors > 0)
		fatal_error("Encountered %d error%s in %s.", errors,
			errors == 1 ? "" : "s", Grufti->configfile);

context;
	Grufti_checkcrucial(&errors);
	if(errors > 0)
		fatal_error("%d crucial config entr%s not set.  Edit %s.", errors,
			errors == 1 ? "y" : "ies", Grufti->configfile);

	
context;
	printf("(%d line%s)\n",xx,xx==1?"":"s");

	/* Here we go! */
	timet_to_date_long(time(NULL),started);
	Log_write(LOG_ALL,"*","");
	Log_write(LOG_ALL,"*","##############################################################");
	Log_write(LOG_ALL,"*","## %s",Grufti->copyright);
	Log_write(LOG_ALL,"*","## Started: %s",started);
	Log_write(LOG_ALL,"*","##############################################################");

context;
	Log_write(LOG_MAIN,"*","Configs loaded.");

	/* Load locations */
	printf("  locations:  ");
	Location_loadlocations();
	printf("(%d location%s)\n",Location->num_locations,Location->num_locations==1?"":"s");
	Log_write(LOG_MAIN,"*","Locations loaded. (%d)",Location->num_locations);

	/* Load events */
	printf("  events:     ");
	Event_loadevents();
	printf("(%d event%s)\n",Event->num_events, Event->num_events==1?"":"s");
	Log_write(LOG_MAIN,"*","Events loaded. (%d)",Event->num_events);

	/* Load userfile */
	printf("  users:      ");
	User_loaduserfile();
	User_checkcrucial();
	printf("(%d user%s)",User->num_users-1,User->num_users-1==1?"":"s");
	Log_write(LOG_MAIN,"*","Users loaded. (%d)",User->num_users);
#ifdef MSK
	if (mergeusers[0] != '\0') {
		int tmp;

		tmp = User->num_users;
		User_mergeuserfile(mergeusers);
		if (User->num_users != tmp)
			printf(" (%d user%s post-merge)", User->num_users-1,
				User->num_users-1 == 1 ? "" : "s");
	}
#endif
	printf("\n");

	/* Get username and password for master if userlist is empty */
	if(User->num_users <= 1)
	{
		char	master_user[1024], master_pass[1024], newpass[25];
		userrec	*newuser;

		printf("\n");
		printf("#################### Execution interrupted ####################\n");
		printf("\n");
		printf("    Your userlist is EMPTY...\n");
		printf("\n");
		printf("      Please enter the username and password for your account.\n");
		printf("      It will be given the level +m so that you can add users\n");
		printf("      to the bot and give them levels.\n");
		printf("\n");
		printf("      It is also marked as the owner account (level +w) which\n");
		
		printf("      is permanent and cannot be deleted.\n");
		printf("\n");
		printf("        Username: ");
		fgets(master_user,1024,stdin);
		rmspace(master_user);
		if(invalidacctname(master_user))
		{
			printf("      Invalid username \"%s\".  Try again...\n\n",master_user);
			exit(0);
		}
		printf("        Password: ");
		fgets(master_pass,1024,stdin);
		rmspace(master_pass);
		if(invalidpassword(master_pass))
		{
			printf("      Invalid password.  Try again...\n\n");
			exit(0);
		}

		newuser = User_addnewuser(master_user);
		User_addhistory(newuser, "system", time(NULL), "Owner account created.");
		encrypt_pass(master_pass,newpass);
		userrec_setpass(newuser,newpass);

		newuser->flags |= USER_MASTER;
		newuser->flags |= USER_OWNER;

		/*
		 * Write new account to disk in case we don't make it to the
		 * main loop.
		 */
		Log_write(LOG_MAIN,"*","Writing userfile...");
		User_writeuserfile();

		printf("\n");
		printf("      The account \"%s\" has been created as the 'owner'\n",master_user);
		printf("      account and given the levels +w and +m.\n");
		printf("\n");
		printf("      Press <return> to resume execution....\n");
		fgets(master_user,1024,stdin);
		printf("\n");
		printf("#################### Resuming execution ####################\n");
		printf("\n\n");
	}

	/* Load stats */
	readstats();
	if(Grufti->motd_modify_date == 0L)
		Grufti->motd_modify_date = time(NULL);

	/* Load web catches */
	printf("  webcatches: ");
	URL_loadurls();
	printf("(%d url%s)\n",URL->num_urls,URL->num_urls==1?"":"s");
	Log_write(LOG_MAIN,"*","Webcatches loaded. (%d)",URL->num_urls);

	/* Load banlist */
	printf("  banlist:    ");
	Banlist_loadbans();
	printf("(%d ban%s)\n",Banlist->num_bans,Banlist->num_bans==1?"":"s");
	Log_write(LOG_MAIN,"*","Banlist loaded. (%d)",Banlist->num_bans);

	/* Load response file */
	printf("  responses:  ");
	Response_loadresponsefile();
	printf("(%d response%s)\n",Response_numresponses(),Response_numresponses()==1?"":"s");
	Log_write(LOG_MAIN,"*","Responses loaded. (%d)",Response_numresponses());

	/* Load combos file */
	printf("  combos:     ");
	Response_readcombo();
	printf("(%d combo%s)\n",Response_numcombos(),Response_numcombos()==1?"":"s");
	Log_write(LOG_MAIN,"*","Combos loaded. (%d)",Response_numcombos());

	/* Load server codes */
	Codes_loadcodes();

	printf("\n");
	/* Initialize some variables */
	Grufti->online_since = time(NULL);
	strcpy(Grufti->bothostname,myhostname());
	if(!Grufti->bothostname[0])
		fatal("Can't determine hostname! Set 'hostname' in config.gb.",0);

	/* Check virtual-hosting */
	if(Grufti->virtual_hostname[0])
		enable_virtual_host(Grufti->virtual_hostname);

	/* If no smtp-gateway is set, use our current host */
	if(!Grufti->smtp_gateway[0])
		strcpy(Grufti->smtp_gateway,Grufti->bothostname);

	sprintf(Grufti->botuh,"%s@%s",Grufti->botusername,Grufti->bothostname);
	now = time(NULL) - 10;	/* so then != now on first run */
	last_day = today();

	/* Mark that all files are synced */
	Grufti->last_write = now;

	/* Establish telnet listening port(s) (after we have bothostname) */
	DCC_opentelnetservers();

	/* display object settings */
	if(Grufti->irc)
	{
		Channel_makechanlist(s);
		printf("On %d channel%s: %s\n",Channel->num_channels,Channel->num_channels==1?"":"s",s);
		if(Server->num_servers > 0)
			sprintf(s,"  First is %s.",Server->serverlist->name);
		else
			strcpy(s,"");
		printf("%d server%s configured.%s\n",Server->num_servers,Server->num_servers==1?" is":"s are",s);
	}
	else
		printf("IRC is OFF\n");
	printf("\n");

	/* Where are we listening */
	xx = 0;
	for(d=DCC->dcclist; d!=NULL; d=d->next)
	{
		if(d->flags & DCC_SERVER)
		{
			printf("DCC server \"%s\" listening on port %d.",d->nick,d->port);
			if(d->flags & SERVER_PUBLIC)
				printf(" (public telnet connections)\n");
			else if(d->flags & SERVER_INTERNAL)
			{
				printf(" (gateway");
#ifdef WEB_MODULE
				printf(" and web");
#endif
				printf(" connections)\n");
			}
			else
				printf("\n");
			xx++;
		}
	}
	if(xx == 0)
		printf("No DCC servers listening.\n");
	printf("\n");

	
	/* TESTING */
	do_test();

	/* fork */
	if(Grufti->background)
	{
		xx = fork();
		if(xx == -1)
			fatal("Cannot fork process.",0);
		if(xx != 0)
		{
			/* write pid to pidfile */
			writepid(xx);

#if HAVE_SETPGID
			setpgid(xx,xx);
#endif
			exit(0);
		}

#if HAVE_SETPGID
		setpgid(0,0);
#endif
		/* close out stdin/out/err */
		freopen("/dev/null","r",stdin);
		freopen("/dev/null","w",stdout);
		freopen("/dev/null","w",stderr);
	}

	context;
	/* Just before we take off, write all files back to disk */
	writeallfiles(1);

	context;
	/* main event loop */
	while(1)
	{
		context;
		then = now;
		now = time(NULL);

		/* timer stuff */
		if(now != then)
			everysecond();

		/* Check network sockets */
		context;
		net_dequeue();
		context;
		xx = net_sockgets(buf,&i);
		context;

		/* EOF from somewhere */
		if(xx == -1)
		{
			if(i == Server->sock)
				Server_gotEOF();
			else
				DCC_gotEOF(i);
		}

		/* no errors, messages waiting */
		else if(xx >= 0)
		{
			if(xx == Server->sock)
				Server_gotactivity(buf);
			else
				DCC_gotactivity(xx,buf,i);

			context;
		}

		/* select() error */
		else if((xx==-2) && (errno != EINTR))
		{
			context;
			Log_write(LOG_ERROR,"*","* [%d] socket error: #%d (%s)", i, errno,
				net_errstr());

			/*
			 * Signal on EOF on this puppy.
			 */
			/* DCC_gotEOF(i); */

			context;
			if(fcntl(Server->sock,F_GETFD,0) == -1)
			{
			context;
				Log_write(LOG_ERROR,"*","Server sock expired.");
			context;
				Server_disconnect();
			}
			context;
		}

		context;
	}
	
}


void everysecond()
{
	/* Write files */
	context;
	if((now - Grufti->last_write) > Grufti->write_freq)
		writeallfiles(0);

	/* Are we connected to a server? */
	if(!Server_isconnected())
	{
	context;
		/* 
		 * We check for DIE flag only if we're not
		 * connected to a server.  If we were, then
		 * we're just waiting for it to close.
		 */
		if(Grufti->die)
			depart();

	context;
		/* If irc is ON and servers exist... */
		if(Grufti->irc && Server->num_servers)
		{
			if(Server_isanyserverpossible())
				Server_connect();
			else
				Server_noserverpossible();
		}
	}

	context;
	/* If we sang some lyrics within 6.9 days */
	if((now - Response->time_sang_1015) > 120)
		Response_checkforcron();

	context;
	/* go offline if IRC is OFF */
	if(!Grufti->irc && Server_isconnected())
		Server_quit("IRC turned OFF");

	context;
	/* Check for server timeout */
	Server_checkfortimeout();

	context;
	/* check for DCC timeout */
	DCC_checkfortimeout();

	context;
	/* check for channel type timeouts */
	Channel_checkfortimeout();

	/* check for banlist timeouts */
	if(Banlist->num_bans)
		Banlist_checkfortimeouts();

	context;
	/* Check if we need to dequeue forwarded Notes */
	if((now - Note->time_dequeued) > Grufti->dequeuenotes_freq)
		Note_dequeueforwardednotes();

	context;
	/* Dequeue server queues */
	Server_dequeue();

	/* Things to do while we're active on a server */
	if(Server_isactiveIRC() && !(Server->flags & SERVER_QUIT_IN_PROGRESS))
	{
	context;
		/* Load radiate queue if necessary */
		if(!Notify_currentlyradiating())
			if((now - Notify->last_radiated) > Grufti->radiate_freq)
				Notify_loadradiatequeue();

	context;
		/* Radiate messages from radiate queue if any exist */
		if(!Notify_waitingfor302())
			if(Notify->num_nicks)
				Notify_sendmsgfromqueue();

	context;
		/* Ping */
		if(!Server_waitingforpong())
			if((now - Server->time_sent_ping) > Grufti->ping_freq)
				Server_sendping();

	context;
		/* Lusers */
		if(Grufti->min_servers && !Server_waitingforluser())
			if((now - Server->time_sent_luser) > Grufti->luser_freq)
				Server_sendluser();

	context;
		/* See if I need to join any channels (banned, full, etc. ) */
		if(!Channel_onallchannels())
			if((now - Channel->time_tried_join) > Grufti->join_freq)
				Channel_joinunjoinedchannels();

	context;
		/* Try to get my nick back */
		if(Server_usingaltnick())
			if((now - Server->time_tried_nick) > Grufti->nick_freq)
				Server_nick(Grufti->botnick);

		
	context;
		/* Time to calculate population statistics? */
		if(thishour() != last_hour)
		{
			last_hour = thishour();
			Channel_updatestatistics(last_hour);
			Grufti->bot_cmds[last_hour] = 0;
		}
	}


	context;
	/* every night at midnight */
	if(today() != last_day)
	{
		int	x;
		char	backup[256], src[256];
		userrec	*u;
		servrec	*serv;

		last_day = today();
		User_carryoverallactivitytimes();
		Log_carryoveralllogs();
		{ // cph - date stamp new logs
			char started[DATELONGLEN];

			timet_to_date_long(time(NULL), started);
			Log_write(LOG_ALL, "*", "## Started: %s", started);
		}

		/* Backup userlist */
		if(Grufti->backup_userlist)
		{
			sprintf(backup,"%s/%s",Grufti->backupdir,Grufti->userfile);
			sprintf(src,"%s/%s",Grufti->botdir,Grufti->userfile);
			x = copyfile(src,backup);
			verify_write(x,src,backup);
			if(x == 0)
				Log_write(LOG_MAIN,"*","Wrote backup userlist file.");
		}

		/* Backup response */
		if(Grufti->backup_response)
		{
			sprintf(backup,"%s/%s",Grufti->backupdir,Grufti->respfile);
			sprintf(src,"%s/%s",Grufti->botdir,Grufti->respfile);
			x = copyfile(src,backup);
			verify_write(x,src,backup);
			if(x == 0)
				Log_write(LOG_MAIN,"*","Wrote backup response file.");
		}

		/* Backup locations */
		if(Grufti->backup_locations)
		{
			sprintf(backup,"%s/%s",Grufti->backupdir,Grufti->locafile);
			sprintf(src,"%s/%s",Grufti->botdir,Grufti->locafile);
			x = copyfile(src,backup);
			verify_write(x,src,backup);
			if(x == 0)
				Log_write(LOG_MAIN,"*","Wrote backup locations file.");
		}
				
		/* Backup banlist */
		if(Grufti->backup_banlist)
		{
			sprintf(backup,"%s/%s",Grufti->backupdir,Grufti->banlist);
			sprintf(src,"%s/%s",Grufti->botdir,Grufti->banlist);
			x = copyfile(src,backup);
			verify_write(x,src,backup);
			if(x == 0)
				Log_write(LOG_MAIN,"*","Wrote backup banlist file.");
		}
				
		/* Backup events */
		if(Grufti->backup_events)
		{
			sprintf(backup,"%s/%s",Grufti->backupdir,Grufti->eventfile);
			sprintf(src,"%s/%s",Grufti->botdir,Grufti->eventfile);
			x = copyfile(src,backup);
			verify_write(x,src,backup);
			if(x == 0)
				Log_write(LOG_MAIN,"*","Wrote backup events file.");
		}
				
		/* Delete inactive users */
		Grufti->users_deleted = User_timeoutusers();
		if(Grufti->users_deleted)
			Log_write(LOG_MAIN,"*","%d user%s deleted. (%d day timeout)",Grufti->users_deleted,Grufti->users_deleted==1?" was":"s were",Grufti->timeout_users);

		/* Delete inactive accounts */
		Grufti->accts_deleted = User_timeoutaccounts();
		if(Grufti->accts_deleted)
			Log_write(LOG_MAIN,"*","%d hostmask%s deleted. (%d day timeout)",Grufti->accts_deleted,Grufti->accts_deleted==1?" was":"s were",Grufti->timeout_accounts);

		/* Delete inactive notes */
		Grufti->notes_deleted = Note_timeoutnotes();
		if(Grufti->notes_deleted)
			Log_write(LOG_MAIN,"*","%d note%s deleted. (%d day timeout)",Grufti->notes_deleted,Grufti->notes_deleted==1?" was":"s were",Grufti->timeout_notes);

		Grufti->cleanup_time = time(NULL);

		Grufti->notes_forwarded = 0;

		/* Send a nightly report via Grufti Note */
		if(Grufti->send_notice_nightly_report)
			User_sendnightlyreport();

		/* Reset daily user statistics */
		for(u=User->userlist; u!=NULL; u=u->next)
		{
			if(u->registered)
			{
				u->via_web = 0;
				u->bot_commands = 0;
			}
		}

		/* Reset daily server statistics */
		for(serv=Server->serverlist; serv!=NULL; serv=serv->next)
		{
			serv->connected = 0L;
			if(serv->first_activity)
				serv->first_activity = now;
			serv->bytes_in = 0L;
			serv->bytes_out = 0L;
		}

		writeallfiles(1);
	}
}

u_long main_sizeof()
{
	u_long	tot = 0L;

	tot += sizeof(cx_file);
	tot += sizeof(cx_line);
	tot += sizeof(now);
	tot += sizeof(then);
	tot += sizeof(last_ten);
	tot += sizeof(last_sixty);
	tot += sizeof(last_day);
	tot += sizeof(timecnt);

	return tot;
}

void kill_running_bot(int hard_kill)
{
	FILE	*f;
	char	s[1000];
	int	pid;

	/* open pidfile for reading */
	f = fopen(Grufti->pidfile,"r");
	if(f != NULL)
	{
		fgets(s,10,f);
		pid = atoi(s);
		kill(pid,SIGCHLD);	/* is pid being used? */
		if(errno != ESRCH)
		{
			/* bot is running */
			if(hard_kill)
			{
				printf("Shutting down. (Performing hard kill)\n");
				kill(pid,SIGUSR2);
			}
			else
			{
				printf("Shutting down. (Performing soft kill)  This may take a moment...\n");
				kill(pid,SIGUSR1);
			}
			return;
		}

		printf("No process running for pid %d! (Removing pid file)\n",pid);
		unlink(Grufti->pidfile);
		return;
	}

	printf("No pidfile found!  Probably not running... (Looking for \"%s\")\n",Grufti->pidfile);
}

int isbotrunning()
{
	FILE	*f;
	char	s[BUFSIZ];
	int	xx;

	f = fopen(Grufti->pidfile,"r");
	if(f != NULL)
	{
		fgets(s,10,f);
		xx = atoi(s);
		kill(xx,SIGCHLD);	/* is pid being used? */
		if(errno != ESRCH)
			return 1;
	}
	return 0;
}

void writepid(int pid)
{
	FILE	*f;

	unlink(Grufti->pidfile);
	f = fopen(Grufti->pidfile,"w");
	if(f != NULL)
	{
		fprintf(f,"%u\n",pid);
		fclose(f);
	}
	else
		printf("Warning!  Could not write pidfile \"%s\".\n",Grufti->pidfile);
}

void do_arg(char *arg)
{
	int	i;

	if(arg[0] == '-')
	{
		for(i=1; i<strlen(arg); i++)
		{
			if(arg[i] == 'h')
			{
				printf("%s\n\n",Grufti->copyright);
				printf("Usage: grufti [switches | file]\n");
				printf("  -h   Display help and exit.\n");
				printf("  -v   Display version and exit.\n");
				printf("  -n   Don't fork into the background.\n");
				printf("  -k   Soft kill.  QUIT is sent to server, and all files are written.\n");
				printf("  -x   Hard kill.  No QUIT is sent to server and no files are written.  The\n");
				printf("       bot is shut down immediately upon catching this signal.  Use caution.\n");
				printf(" file  Optional configfile.  (Default is \"%s\")\n",DEFAULT_CONFIGFILE);
				printf("\n");
				exit(0);
			}
			else if(arg[i] == 'v')
			{
				printf("%s\n",Grufti->copyright);
				printf("v%s",Grufti->build);
#ifdef WEB_MODULE
				printf(" (+web_module)");
#endif
#ifdef RAW_LOGGING
				printf(" (+raw_logging)");
#endif
				printf("\n");
				exit(0);
			}
			else if(arg[i] == 'n')
			{
				Grufti->background = 0;
			}
			else if(arg[i] == 'k')
			{
				kill_running_bot(0);
				exit(0);
			}
			else if(arg[i] == 'x')
			{
				kill_running_bot(1);
				exit(0);
			}
		}
	}
	else
		strcpy(Grufti->configfile,arg);
}

void writestats()
/*
 * Field info:
 *  j <channel> # reg joins for each hour in 24 hrs
 *  J <channel> # nonreg joins for each hour in 24 hrs
 *  c <channel> amout of chatter in 24 hrs
 *  p <channel> avg population for each hr in 24 hrs
 *
 *  S <server> <bytes_in> <bytes_out> <first> <connected>
 *
 *  u  users deleted
 *  a  accounts deleted
 *  n  notes deleted
 *  C  cleanup time
 *  F  notes forwarded
 *  m  date motd last modified
 */
{
	FILE	*f;
	chanrec	*chan;
	servrec	*serv;
	int	i, x;
	char	date_today[DATELONGLEN], tmpfile[256], statsfile[256];

	if(!Grufti->stats_active)
	{
		Log_write(LOG_MAIN,"*","Channel stats are not stable.  Ignoring write request.");
		return;
	}

	/* Open for writing */
	sprintf(statsfile,"%s/%s",Grufti->botdir,Grufti->statsfile);
	sprintf(tmpfile,"%s.tmp",statsfile);
	f = fopen(tmpfile,"w");
	if(f == NULL)
	{
		Log_write(LOG_MAIN | LOG_ERROR,"*","Couldn't write stats.");
		return;
	}

	/* Write header */
	writeheader(f,"Stats - Channel and User status.",statsfile);

	/* Write last day online time */
	timet_to_date_long(time_today_began(time(NULL)),date_today);
	if(fprintf(f,"# Last day online: \"%s\"\n",date_today) == EOF)
		return;
	if(fprintf(f,"D %lu\n\n",time_today_began(time(NULL))) == EOF)
		return;

	/* Write stats */
	for(chan=Channel->channels; chan!=NULL; chan=chan->next)
	{
		/* Write # reg joins */
		if(fprintf(f,"j %s ",chan->name) == EOF)
			return;
		for(i=0; i<23; i++)
			if(fprintf(f,"%d ",chan->joins_reg[i]) == EOF)
				return;
		if(fprintf(f,"%d\n",chan->joins_reg[i]) == EOF)
			return;

		/* Write # non reg joins */
		if(fprintf(f,"J %s ",chan->name) == EOF)
			return;
		for(i=0; i<23; i++)
			if(fprintf(f,"%d ",chan->joins_nonreg[i]) == EOF)
				return;
		if(fprintf(f,"%d\n",chan->joins_nonreg[i]) == EOF)
			return;

		/* Write public chatter */
		if(fprintf(f,"c %s ",chan->name) == EOF)
			return;
		for(i=0; i<23; i++)
			if(fprintf(f,"%lu ",chan->chatter[i]) == EOF)
				return;
		if(fprintf(f,"%lu\n",chan->chatter[i]) == EOF)
			return;

		/* Write avg population */
		if(fprintf(f,"p %s ",chan->name) == EOF)
			return;
		for(i=0; i<23; i++)
			if(fprintf(f,"%d ",chan->population[i]) == EOF)
				return;
		if(fprintf(f,"%d\n",chan->population[i]) == EOF)
			return;
	}

	for(serv=Server->serverlist; serv!=NULL; serv=serv->next)
	{
		/* Write servers */
		if(fprintf(f,"S %s %d %lu %lu %d\n",serv->name,serv->port,serv->bytes_in,serv->bytes_out,Server_timeconnected(serv)) == EOF)
			return;
	}
	

	/* Now write cleanup stats */
	if(fprintf(f,"u %d\n",Grufti->users_deleted) == EOF)
		return;
	if(fprintf(f,"a %d\n",Grufti->accts_deleted) == EOF)
		return;
	if(fprintf(f,"n %d\n",Grufti->notes_deleted) == EOF)
		return;
	if(fprintf(f,"C %lu\n",Grufti->cleanup_time) == EOF)
		return;

	/* Forwarded notes stats */
	if(fprintf(f,"F %d\n",Grufti->notes_forwarded) == EOF)
		return;

	/* MOTD modification date */
	if(fprintf(f,"m %lu\n",Grufti->motd_modify_date) == EOF)
		return;

	fclose(f);

	/* Move tmpfile over to main */
	if(Grufti->keep_tmpfiles)
		x = copyfile(tmpfile,statsfile);
	else
		x = movefile(tmpfile,statsfile);

	verify_write(x,tmpfile,statsfile);
	if(x == 0)
		Log_write(LOG_MAIN,"*","Wrote stats file.");
}

void readstats()
{
	chanrec	*chan;
	servrec	*serv;
	char	s[1024], ident[1024], elem[1024], chan_name[1024];
	char	statsfile[256], serv_name[1024], portstr[1024];
	int	line = 0, i, portnum;
	FILE	*f;


	/* Open file for reading */
	sprintf(statsfile,"%s/%s",Grufti->botdir,Grufti->statsfile);
	f = fopen(statsfile,"r");
	if(f == NULL)
	{
		Grufti->stats_active = 1;
		return;
	}

	/* Read each line */
	while(readline(f,s))
	{
		line++;

		/* Extract identifier code */
		split(ident,s);

		/* Match identifier code */
		switch(ident[0])
		{
		case 'j':	/* reg joins */

			/* Get channel name */
			split(chan_name,s);

			chan = Channel_channel(chan_name);
			if(chan == NULL)
			{
				Log_write(LOG_ERROR,"*","[%d] Bogus channel entry \"%s\" in stats file (ignoring).",line,chan_name);
				break;
			}

			/* Read in all 24 hrs of activity */
			for(i=0; i<24; i++)
			{
				split(elem,s);
				chan->joins_reg[i] = elem[0] ? atoi(elem) : 0;
			}

			break;

		case 'J':	/* Non-reg'd joins */
			/* Get channel name */
			split(chan_name,s);

			chan = Channel_channel(chan_name);
			if(chan == NULL)
			{
				Log_write(LOG_ERROR,"*","[%d] Bogus channel entry \"%s\" in stats file (ignoring).",line,chan_name);
				break;
			}

			/* Read in all 24 hrs of activity */
			for(i=0; i<24; i++)
			{
				split(elem,s);
				chan->joins_nonreg[i] = elem[0] ? atoi(elem) : 0;
			}

			break;

		case 'c':	/* Non-reg'd joins */
			/* Get channel name */
			split(chan_name,s);

			chan = Channel_channel(chan_name);
			if(chan == NULL)
			{
				Log_write(LOG_ERROR,"*","[%d] Bogus channel entry \"%s\" in stats file (ignoring).",line,chan_name);
				break;
			}

			/* Read in all 24 hrs of activity */
			for(i=0; i<24; i++)
			{
				split(elem,s);
				chan->chatter[i] = elem[0] ? atoi(elem) : 0;
			}

			break;

		case 'p':	/* Non-reg'd joins */
			/* Get channel name */
			split(chan_name,s);

			chan = Channel_channel(chan_name);
			if(chan == NULL)
			{
				Log_write(LOG_ERROR,"*","[%d] Bogus channel entry \"%s\" in stats file (ignoring).",line,chan_name);
				break;
			}

			/* Read in all 24 hrs of activity */
			for(i=0; i<24; i++)
			{
				split(elem,s);
				chan->population[i] = elem[0] ? atoi(elem) : 0;
			}

			break;

		case 'S':	/* list of servers */
			/* If the last day online is not today, skip this */
			if((time(NULL) - Grufti->last_day_online) > 86400)
				break;

			/* Get server name */
			split(serv_name,s);

			/* Get port # */
			split(portstr,s);
			portnum = atoi(portstr);
			
			serv = Server_serverandport(serv_name,portnum);
			if(serv == NULL)
			{
				Log_write(LOG_ERROR,"*","[%d] Bogus server entry \"%s:%d\" in stats file (ignoring).",line,serv_name,portnum);
				break;
			}

			/* Get bytes in */
			split(elem,s);
			serv->bytes_in = (time_t)atol(elem);

			/* Get bytes out */
			split(elem,s);
			serv->bytes_out = (time_t)atol(elem);

			/* Get connected */
			split(elem,s);
			serv->connected = (time_t)atol(elem);

			break;

		case 'u':	/* users deleted */
			Grufti->users_deleted = atoi(s);
			break;

		case 'a':	/* accounts deleted */
			Grufti->accts_deleted = atoi(s);
			break;

		case 'n':	/* notes deleted */
			Grufti->notes_deleted = atoi(s);
			break;

		case 'C':	/* cleanup time */
			Grufti->cleanup_time = (time_t)atol(s);
			break;

		case 'F':	/* notes forwarded */
			Grufti->notes_forwarded = atoi(s);
			break;

		case 'm':	/* motd modify date */
			Grufti->motd_modify_date = (time_t)atol(s);
			break;
		}
	}

	fclose(f);

	Grufti->stats_active = 1;
}

void do_test()
{
}
