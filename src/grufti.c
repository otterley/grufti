/******************************************************************************
 * Steve Rider - stever@area23.org
 * Grufti - an interactive, service-oriented bot for IRC 
 *
 * grufti.c - grufti-specific variables and main Grufti object
 *
 *****************************************************************************/
/* 28 April 97 */

#include "config.h"
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/param.h>
#include <fcntl.h>
#include "grufti.h"
#include "gruftiaux.h"
#include "misc.h"
#include "response.h"
#include "server.h"
#include "channel.h"
#include "log.h"
#include "fhelp.h"

/***************************************************************************n
 *
 * Grufti object definitions
 *
 ****************************************************************************/

void Grufti_new()
{
	int	i;

	/* already in use? */
	if(Grufti != NULL)
		Grufti_freeall();

	/* allocate memory */
	Grufti = (Grufti_ob *)xmalloc(sizeof(Grufti_ob));

	/* initialize char */
	strcpy(Grufti->botnick,"");		/* bot nick (mandatory) */
	strcpy(Grufti->botusername,"");		/* bot username (mandatory) */
	strcpy(Grufti->botrealname,"A Grufti");	/* bot finger info */
	strcpy(Grufti->maintainer,"");		/* optional maintainer info */
	strcpy(Grufti->bothostname,"");		/* bot hostname (set by gb) */
	strcpy(Grufti->virtual_hostname,"");	/* virtual hostname */
	strcpy(Grufti->homepage,"");		/* optional homepage */
	strcpy(Grufti->userfile,"userlist");	/* userlist file */
	strcpy(Grufti->tmpdir,"/tmp");		/* tmp directory */
	strcpy(Grufti->respfile,"response");	/* Response file */
	strcpy(Grufti->helpfile,"help");	/* help file */
	strcpy(Grufti->locafile,"locations");	/* locations file */
	strcpy(Grufti->webfile,"webcatches");	/* web catches */
	strcpy(Grufti->statsfile,"stats");	/* statistics */
	strcpy(Grufti->banlist,"banlist");	/* banlist */
	strcpy(Grufti->eventfile,"events");	/* Event list */
	strcpy(Grufti->xfer_incoming,"incoming");	/* incoming files */
	strcpy(Grufti->xfer_files,"files");	/* outgoing files */
	strcpy(Grufti->backupdir,"backup");	/* backup */
	strcpy(Grufti->botdir,"bot");		/* bot */
	strcpy(Grufti->admindir,"etc");		/* etc */
	strcpy(Grufti->logdir,"logs");		/* logs */
	strcpy(Grufti->motd_dcc,"motd-dcc");	/* dcc motd file */
	strcpy(Grufti->motd_telnet,"motd-telnet");	/* telnet motd file */
	strcpy(Grufti->welcome_dcc,"welcome-dcc");	/* dcc welcome file */
	strcpy(Grufti->telnet_banner,"banner");	/* telnet banner */
	strcpy(Grufti->combofile,"combos");		/* combo file */
	strcpy(Grufti->pidfile,"grufti.pid");	/* pidfile */
	strcpy(Grufti->alt_name1,"");		/* alternate name 1 */
	strcpy(Grufti->alt_name2,"");		/* alternate name 2 */
	strcpy(Grufti->ctcp_version,"mIRC32 v5.02 K.Mardam-Bey");
	strcpy(Grufti->ctcp_clientinfo,"ACCEPT ACTION CLIENTINFO DCC ECHO FINGER PING RESUME SOUND TIME USERINFO VERSION");
	strcpy(Grufti->ctcp_finger,"You're gonna lose that finger!");
	strcpy(Grufti->shitlist_message,"You're not wanted here!");
	strcpy(Grufti->mymode_on_IRC_join,"+i-ws");
	strcpy(Grufti->takeover_in_progress_msg,"*** Takeover on #gothic in progress, join #grufti ***");
	strcpy(Grufti->smtp_gateway,"");
	strcpy(Grufti->idlekick_message,"** No idling **");


	/* initialize int */
	Grufti->irc = 1;			/* Do we connect to irc or no */
	Grufti->server_timeout = 15;		/* how long before next server*/
	Grufti->server_ping_timeout = 180;	/* jump if no pong in 3 mins */
	Grufti->server_quit_timeout = 30;	/* close manually if no QUIT */
	Grufti->chat_login_timeout = 180;	/* user logging in */
	Grufti->chat_ident_timeout = 10;	/* waiting for ident reply */
	Grufti->smtp_timeout = 180;		/* smtp timeout */
	Grufti->chat_timeout = 43200;		/* normal chat. (12 hrs) */
	Grufti->establish_dcc_timeout = 180;	/* waiting for DCC connect */
	Grufti->serverpossible_timeout = 120;	/* server possible timeout */
	Grufti->max_dcc_connections = 20;	/* max dcc connections */
	Grufti->reqd_telnet_port = 9000;	/* port we want for telnet */
	Grufti->reqd_gateway_port = 8888;	/* port we want for gateway */
	Grufti->chat_port = 7900;		/* chat port low end */
	Grufti->num_ports_to_try = 1000;	/* chat port high end. */
	Grufti->max_nicks = 5;			/* num nicks allowed */
	Grufti->max_greets = 5;			/* num greets allowed */
	Grufti->login_attempts = 3;		/* num attempts we'll allow */
	Grufti->dequeuenotes_freq = 120;	/* how often to dequeue notes */
	Grufti->radiate_freq = 180;		/* how often to userhost */
	Grufti->ping_freq = 60;			/* how often to ping */
	Grufti->join_freq = 10;			/* how often to try joining */
	Grufti->nick_freq = 10;			/* how often to try our nick */
	Grufti->luser_freq = 300;		/* how often to LUSERS */
	Grufti->netsplit_timeout = 180;		/* hot long before jumping */
	Grufti->memberissplit_timeout = 1200;	/* remove member if split */
	Grufti->radiate_timeout = 600;		/* no 302 has been received */
	Grufti->min_servers = 5;		/* min servers before jumping */
	Grufti->welcome_newdcc = 1;		/* welcome new dcc's? */
	Grufti->show_motd_on_dcc_chat = 1;	/* show motd on dcc chat? */
	Grufti->show_motd_on_telnet = 1;	/* show motd on telnet */
	Grufti->show_motd_onlywhenchanged = 1;	/* show motd only when changed*/
	Grufti->show_telnet_banner = 1;		/* show telnet banner */
	Grufti->background = 1;			/* fork into background */
	Grufti->console = 0;			/* do console mode */
	Grufti->show_internal_ports = 1;	/* show intr ports in dccinfo */
	Grufti->max_saved_notes = 10;		/* max number of saved notes */
	Grufti->location_changetime = 3600;	/* location change time */
	Grufti->max_urls = 50;			/* max or URL's to catch */
	Grufti->wait_to_greet = 600;		/* wait before greeting again */
	Grufti->wait_to_informofnotes = 600;	/* wait before saying notes */
	Grufti->write_freq = 3600;		/* write every hour */
	Grufti->public_flood_num = 10;		/* num msgs is a flood */
	Grufti->public_flood_interval = 120;	/* in number of seconds */
	Grufti->public_flood_timeout = 120;	/* wait 2 mins */
	Grufti->xfer_maxfilesize = 2048;	/* maxfilesize (in kb) */
	Grufti->xfer_remove_incomplete = 1;	/* remove incomplete files */
	Grufti->xfer_blocksize = 512;		/* blocksize to send */
	Grufti->timeout_accounts = 30;		/* when accounts are deleted */
	Grufti->timeout_notes = 30;		/* when notes are deleted */
	Grufti->timeout_notices = 7;		/* when notices are deleted */
	Grufti->timeout_users = 90;		/* when users are deleted */
	Grufti->notify_of_notes_on_signon = 1;	/* Notify of notes on signon */
	Grufti->auto_detect_takeover = 1;
	Grufti->takeover_in_progress = 0;
	Grufti->keep_tmpfiles = 1;		/* Keep tmp files? */
	Grufti->backup_userlist = 1;		/* write backup userlist */
	Grufti->backup_response = 1;		/* write backup response */
	Grufti->backup_locations = 1;		/* write backup locations */
	Grufti->backup_banlist = 1;		/* write backup banlist */
	Grufti->backup_events = 1;		/* write backup events */
	Grufti->unban_protected_users = 1;	/* unban those with +p, +f */
	Grufti->kick_those_who_ban_prot = 1;	/* kick the banner */
	Grufti->respond_to_haveyouseen = 1;	/* respond to have you seen */
	Grufti->respond_to_url = 1;		/* respond to url */
	Grufti->respond_to_bday = 1;		/* respond to bday */
	Grufti->respond_to_showbdays = 1;	/* respond to bday */
	Grufti->respond_to_email = 1;		/* respond to email */
	Grufti->respond_to_stat = 1;		/* respond to stat */
	Grufti->send_notice_nightly_report = 1;	/* send notice */
	Grufti->send_notice_die = 1;		/* send notice */
	Grufti->send_notice_level_plus = 1;	/* send notice */
	Grufti->send_notice_level_minus = 1;	/* send notice */
	Grufti->send_notice_DCC_receive = 1;	/* send notice */
	Grufti->send_notice_user_created = 1;	/* send notice */
	Grufti->send_notice_user_deleted = 1;	/* send notice */
	Grufti->send_notice_response_created = 1; /* send notice */
	Grufti->send_notice_response_deleted = 1; /* send notice */
	Grufti->send_notice_location_created = 1; /* send notice */
	Grufti->send_notice_location_deleted = 1; /* send notice */
	Grufti->send_notice_event_created = 1;	/* send notice */
	Grufti->send_notice_event_deleted = 1;	/* send notice */
	Grufti->send_notice_event_signsup = 1;	/* send notice */
	Grufti->send_notice_event_removes = 1;	/* send notice */
	Grufti->default_leave_timeout = 300;	/* default leave timeout */
	Grufti->max_leave_timeout = 3600;	/* max leave timeout */
	Grufti->default_hush_timeout = 300;	/* default hush timeout */
	Grufti->max_hush_timeout = 3600;	/* max hush timeout */
	Grufti->sing_1015 = 0;			/* sing "10:15" by the Cure */
	Grufti->allow_dcc_send = 1;		/* allow dcc send requests */
	Grufti->kick_channel_idlers = 0;	/* Kick channel idlers */
	Grufti->idlekick_timeout = 60;		/* How often to kick idlers */
	Grufti->kick_idle_opers = 1;		/* Kick idle opers? */
	Grufti->plus_i_idlekick_timeout = 4;	/* Timer for +i reversal */
	Grufti->idle_time = 3600;		/* When user considered idle */
	Grufti->quit_port_request_failed = 1;	/* If port request failed */
	Grufti->require_dcc_for_pass_cmd = 1;	/* require dcc for pass cmd */
	Grufti->pending_command_timeout = 30;	/* Time until command is invalid */


	/* initialize u_int */


	/* Initialize misc  (No config file entry) */
	strcpy(Grufti->configfile,DEFAULT_CONFIGFILE);
	strcpy(Grufti->build,"1.0.17");		/* build # */
	sprintf(Grufti->copyright, "Grufti %s (c) 2000 Steve Rider (stever@area23.org)", Grufti->build);
	Grufti->online_since = 0L;		/* Time when GB was started */
	strcpy(Grufti->botuh,"");
	Grufti->actual_telnet_port = 0;		/* the port we got for telnet */
	Grufti->actual_gateway_port = 0;	/* the port we got for gw */
	Grufti->die = 0;			/* signal when we want to die */
	Grufti->last_day_online = 0L;		/* set at midnight */
	Grufti->last_write = 0L;		/* last write to disk */
	Grufti->users_deleted = 0;		/* cleanup */
	Grufti->accts_deleted = 0;		/* cleanup */
	Grufti->notes_deleted = 0;		/* cleanup */
	Grufti->notes_forwarded = 0;
	for(i=0; i<24; i++)
		Grufti->bot_cmds[i] = 0;
	Grufti->cleanup_time = 0L;		/* cleanup */
	Grufti->stats_active = 0;
	Grufti->motd_modify_date = 0L;
	strcpy(Grufti->virtual_hostname,"");
}

u_long Grufti_sizeof()
{
	return (u_long)sizeof(Grufti_ob);
}

void Grufti_freeall()
{
	/* Free the elements */

	/* Free this object */
	xfree(Grufti);
	Grufti = NULL;
}

/****************************************************************************/
/* object Grufti data-specific function definitions. */

void Grufti_display()
{
	/* display all internal settings */
}

/****************************************************************************/
/* object Grufti high-level implementation-specific function definitions. */

int Grufti_setvar(char *n, char *v)
/*
 * Function: Sets the value 'n' to parameter 'v'
 *  Returns: 0 if OK
 *           -1 if incorrect # of parameters
 *           -2 if argument not quoted correctly
 *           -3 if variable not found
 */
{
	if(!n[0] || !v[0])
		return -1;

	if(!isanumber(v) && !rmquotes(v))
		return -2;

	/* Initialize char */
	if(isequal(n,"nick"))
		strcpy(Grufti->botnick,v);

	else if(isequal(n,"username"))
		strcpy(Grufti->botusername,v);

	else if(isequal(n,"realname"))
		strcpy(Grufti->botrealname,v);

	else if(isequal(n,"maintainer"))
		strcpy(Grufti->maintainer,v);

	else if(isequal(n,"hostname"))
		strcpy(Grufti->bothostname,v);

	else if(isequal(n,"virtual-hostname"))
		strcpy(Grufti->virtual_hostname,v);

	else if(isequal(n,"homepage"))
		strcpy(Grufti->homepage,v);

	else if(isequal(n,"tmpdir"))
	{
		if(v[0])
			strcpy(Grufti->tmpdir,v);
		else
			strcpy(Grufti->tmpdir,".");
	}

	else if(isequal(n,"userfile"))
		strcpy(Grufti->userfile,v);

	else if(isequal(n,"responsefile"))
		strcpy(Grufti->respfile,v);

	else if(isequal(n,"helpfile"))
		strcpy(Grufti->helpfile,v);

	else if(isequal(n,"locationfile"))
		strcpy(Grufti->locafile,v);

	else if(isequal(n,"webcatchfile"))
		strcpy(Grufti->webfile,v);

	else if(isequal(n,"statsfile"))
		strcpy(Grufti->statsfile,v);

	else if(isequal(n,"banlist"))
		strcpy(Grufti->banlist,v);

	else if(isequal(n,"eventfile"))
		strcpy(Grufti->eventfile,v);

	else if(isequal(n,"xfer-incoming"))
		strcpy(Grufti->xfer_incoming,v);

	else if(isequal(n,"xfer-files"))
		strcpy(Grufti->xfer_files,v);

	else if(isequal(n,"backupdir"))
	{
		if(v[0])
			strcpy(Grufti->backupdir,v);
		else
			strcpy(Grufti->backupdir,".");
	}

	else if(isequal(n,"botdir"))
	{
		if(v[0])
			strcpy(Grufti->botdir,v);
		else
			strcpy(Grufti->botdir,".");
	}

	else if(isequal(n,"admindir"))
	{
		if(v[0])
			strcpy(Grufti->admindir,v);
		else
			strcpy(Grufti->admindir,".");
	}

	else if(isequal(n,"logdir"))
	{
		if(v[0])
			strcpy(Grufti->logdir,v);
		else
			strcpy(Grufti->logdir,".");
	}

	else if(isequal(n,"motd-dcc"))
		strcpy(Grufti->motd_dcc,v);

	else if(isequal(n,"motd-telnet"))
		strcpy(Grufti->motd_telnet,v);

	else if(isequal(n,"welcome-dcc"))
		strcpy(Grufti->welcome_dcc,v);

	else if(isequal(n,"telnet-banner"))
		strcpy(Grufti->telnet_banner,v);

	else if(isequal(n,"combofile"))
		strcpy(Grufti->combofile,v);

	else if(isequal(n,"pidfile"))
		strcpy(Grufti->pidfile,v);

	else if(isequal(n,"alt-name1"))
		strcpy(Grufti->alt_name1,v);

	else if(isequal(n,"alt-name2"))
		strcpy(Grufti->alt_name2,v);

	else if(isequal(n,"ctcp-version"))
		strcpy(Grufti->ctcp_version,v);

	else if(isequal(n,"ctcp-clientinfo"))
		strcpy(Grufti->ctcp_clientinfo,v);

	else if(isequal(n,"ctcp-finger"))
		strcpy(Grufti->ctcp_finger,v);

	else if(isequal(n,"shitlist-message"))
		strcpy(Grufti->shitlist_message,v);

	else if(isequal(n,"mymode-on-IRC-join"))
		strcpy(Grufti->mymode_on_IRC_join,v);

	else if(isequal(n,"takeover-in-progress-msg"))
		strcpy(Grufti->takeover_in_progress_msg,v);

	else if(isequal(n,"smtp-gateway"))
		strcpy(Grufti->smtp_gateway,v);

	else if(isequal(n,"idlekick-message"))
		strcpy(Grufti->idlekick_message,v);



	/* Initialize int */
	else if(isequal(n,"irc"))
		Grufti->irc = atoi(v);

	else if(isequal(n,"server-timeout"))
		Grufti->server_timeout = atoi(v);

	else if(isequal(n,"server-ping-timeout"))
		Grufti->server_ping_timeout = atoi(v);

	else if(isequal(n,"server-quit-timeout"))
		Grufti->server_quit_timeout = atoi(v);

	else if(isequal(n,"chat-login-timeout"))
		Grufti->chat_login_timeout = atoi(v);

	else if(isequal(n,"chat-ident-timeout"))
		Grufti->chat_ident_timeout = atoi(v);

	else if(isequal(n,"smtp-timeout"))
		Grufti->smtp_timeout = atoi(v);

	else if(isequal(n,"chat-timeout"))
		Grufti->chat_timeout = atoi(v);

	else if(isequal(n,"establish-dcc-timeout"))
		Grufti->establish_dcc_timeout = atoi(v);

	else if(isequal(n,"serverpossible-timeout"))
		Grufti->serverpossible_timeout = atoi(v);

	else if(isequal(n,"max-dcc-connections"))
		Grufti->max_dcc_connections = atoi(v);

	else if(isequal(n,"telnet-port"))
		Grufti->reqd_telnet_port = atoi(v);

	else if(isequal(n,"gateway-port"))
		Grufti->reqd_gateway_port = atoi(v);

	else if(isequal(n,"chat-port"))
		Grufti->chat_port = atoi(v);

	else if(isequal(n,"num-ports-to-try"))
		Grufti->num_ports_to_try = atoi(v);

	else if(isequal(n,"max-nicks"))
		Grufti->max_nicks = atoi(v);

	else if(isequal(n,"max-greets"))
		Grufti->max_greets = atoi(v);

	else if(isequal(n,"dequeuenotes-freq"))
		Grufti->dequeuenotes_freq = atoi(v);

	else if(isequal(n,"login-attempts"))
		Grufti->login_attempts = atoi(v);

	else if(isequal(n,"radiate-freq"))
		Grufti->radiate_freq = atoi(v);

	else if(isequal(n,"ping-freq"))
		Grufti->ping_freq = atoi(v);

	else if(isequal(n,"join-freq"))
		Grufti->join_freq = atoi(v);

	else if(isequal(n,"nick-freq"))
		Grufti->nick_freq = atoi(v);

	else if(isequal(n,"luser-freq"))
		Grufti->luser_freq = atoi(v);

	else if(isequal(n,"netsplit-timeout"))
		Grufti->netsplit_timeout = atoi(v);

	else if(isequal(n,"memberissplit-timeout"))
		Grufti->memberissplit_timeout = atoi(v);

	else if(isequal(n,"radiate-timeout"))
		Grufti->radiate_timeout = atoi(v);

	else if(isequal(n,"min-servers"))
		Grufti->min_servers = atoi(v);

	else if(isequal(n,"welcome-newdcc"))
		Grufti->welcome_newdcc = atoi(v);

	else if(isequal(n,"show-motd-on-dcc-chat"))
		Grufti->show_motd_on_dcc_chat = atoi(v);

	else if(isequal(n,"show-motd-on-telnet"))
		Grufti->show_motd_on_telnet = atoi(v);

	else if(isequal(n,"show-motd-onlywhenchanged"))
		Grufti->show_motd_onlywhenchanged = atoi(v);

	else if(isequal(n,"show-telnet-banner"))
		Grufti->show_telnet_banner = atoi(v);

	else if(isequal(n,"background"))
		Grufti->background = atoi(v);

	else if(isequal(n,"console"))
		Grufti->console = atoi(v);

	else if(isequal(n,"show-internal-ports"))
		Grufti->show_internal_ports = atoi(v);

	else if(isequal(n,"max-saved-notes"))
		Grufti->max_saved_notes = atoi(v);

	else if(isequal(n,"location-changetime"))
		Grufti->location_changetime = atoi(v);

	else if(isequal(n,"max-urls"))
		Grufti->max_urls = atoi(v);

	else if(isequal(n,"wait-to-greet"))
		Grufti->wait_to_greet = atoi(v);

	else if(isequal(n,"wait-to-informofnotes"))
		Grufti->wait_to_informofnotes = atoi(v);

	else if(isequal(n,"write-freq"))
		Grufti->write_freq = atoi(v);

	else if(isequal(n,"public-flood-num"))
		Grufti->public_flood_num = atoi(v);

	else if(isequal(n,"public-flood-interval"))
		Grufti->public_flood_interval = atoi(v);

	else if(isequal(n,"public-flood-timeout"))
		Grufti->public_flood_timeout = atoi(v);

	else if(isequal(n,"xfer-maxfilesize"))
		Grufti->xfer_maxfilesize = atoi(v);

	else if(isequal(n,"xfer-remove-incomplete"))
		Grufti->xfer_remove_incomplete = atoi(v);

	else if(isequal(n,"xfer-blocksize"))
		Grufti->xfer_blocksize = atoi(v);

	else if(isequal(n,"timeout-accounts"))
		Grufti->timeout_accounts = atoi(v);

	else if(isequal(n,"timeout-notes"))
		Grufti->timeout_notes = atoi(v);

	else if(isequal(n,"timeout-notices"))
		Grufti->timeout_notices = atoi(v);

	else if(isequal(n,"timeout-users"))
		Grufti->timeout_users = atoi(v);

	else if(isequal(n,"notify-of-notes-on-signon"))
		Grufti->notify_of_notes_on_signon = atoi(v);

	else if(isequal(n,"auto-detect-takeover"))
		Grufti->auto_detect_takeover = atoi(v);

	else if(isequal(n,"takeover-in-progress"))
		Grufti->takeover_in_progress = atoi(v);

	else if(isequal(n,"keep-tmpfiles"))
		Grufti->keep_tmpfiles = atoi(v);

	else if(isequal(n,"backup-userlist"))
		Grufti->backup_userlist = atoi(v);

	else if(isequal(n,"backup-response"))
		Grufti->backup_response = atoi(v);

	else if(isequal(n,"backup-locations"))
		Grufti->backup_locations = atoi(v);

	else if(isequal(n,"backup-banlist"))
		Grufti->backup_banlist = atoi(v);

	else if(isequal(n,"backup-events"))
		Grufti->backup_events = atoi(v);

	else if(isequal(n,"unban-protected-users"))
		Grufti->unban_protected_users = atoi(v);

	else if(isequal(n,"kick-those-who-ban-prot"))
		Grufti->kick_those_who_ban_prot = atoi(v);

	else if(isequal(n,"respond-to-haveyouseen"))
		Grufti->respond_to_haveyouseen = atoi(v);

	else if(isequal(n,"respond-to-url"))
		Grufti->respond_to_url = atoi(v);

	else if(isequal(n,"respond-to-bday"))
		Grufti->respond_to_bday = atoi(v);

	else if(isequal(n,"respond-to-showbdays"))
		Grufti->respond_to_showbdays = atoi(v);

	else if(isequal(n,"respond-to-email"))
		Grufti->respond_to_email = atoi(v);

	else if(isequal(n,"respond-to-stat"))
		Grufti->respond_to_stat = atoi(v);

	else if(isequal(n,"send-notice-nightly-report"))
		Grufti->send_notice_nightly_report = atoi(v);

	else if(isequal(n,"send-notice-die"))
		Grufti->send_notice_die = atoi(v);

	else if(isequal(n,"send-notice-level-plus"))
		Grufti->send_notice_level_plus = atoi(v);

	else if(isequal(n,"send-notice-level-minus"))
		Grufti->send_notice_level_minus = atoi(v);

	else if(isequal(n,"send-notice-DCC-receive"))
		Grufti->send_notice_DCC_receive = atoi(v);

	else if(isequal(n,"send-notice-user-created"))
		Grufti->send_notice_user_created = atoi(v);

	else if(isequal(n,"send-notice-user-deleted"))
		Grufti->send_notice_user_deleted = atoi(v);

	else if(isequal(n,"send-notice-response-created"))
		Grufti->send_notice_response_created = atoi(v);

	else if(isequal(n,"send-notice-response-deleted"))
		Grufti->send_notice_response_deleted = atoi(v);

	else if(isequal(n,"send-notice-location-created"))
		Grufti->send_notice_location_created = atoi(v);

	else if(isequal(n,"send-notice-location-deleted"))
		Grufti->send_notice_location_deleted = atoi(v);

	else if(isequal(n,"send-notice-event-created"))
		Grufti->send_notice_event_created = atoi(v);

	else if(isequal(n,"send-notice-event-deleted"))
		Grufti->send_notice_event_deleted = atoi(v);

	else if(isequal(n,"send-notice-event-signsup"))
		Grufti->send_notice_event_signsup = atoi(v);

	else if(isequal(n,"send-notice-event-removes"))
		Grufti->send_notice_event_removes = atoi(v);

	else if(isequal(n,"default-leave-timeout"))
		Grufti->default_leave_timeout = atoi(v);

	else if(isequal(n,"max-leave-timeout"))
		Grufti->max_leave_timeout = atoi(v);

	else if(isequal(n,"default-hush-timeout"))
		Grufti->default_hush_timeout = atoi(v);

	else if(isequal(n,"max-hush-timeout"))
		Grufti->max_hush_timeout = atoi(v);

	else if(isequal(n,"sing-1015"))
		Grufti->sing_1015 = atoi(v);

	else if(isequal(n,"allow-dcc-send"))
		Grufti->allow_dcc_send = atoi(v);

	else if(isequal(n,"kick-channel-idlers"))
		Grufti->kick_channel_idlers = atoi(v);

	else if(isequal(n,"idlekick-timeout"))
		Grufti->idlekick_timeout = atoi(v);

	else if(isequal(n,"kick-idle-opers"))
		Grufti->kick_idle_opers = atoi(v);

	else if(isequal(n,"plus-i-idlekick-timeout"))
		Grufti->plus_i_idlekick_timeout = atoi(v);

	else if(isequal(n,"idle-time"))
		Grufti->idle_time = atoi(v);

	else if(isequal(n,"quit-port-request-failed"))
		Grufti->quit_port_request_failed = atoi(v);

	else if(isequal(n,"require-dcc-for-pass-command"))
		Grufti->require_dcc_for_pass_cmd = atoi(v);

	else if(isequal(n,"pending-command-timeout"))
		Grufti->pending_command_timeout = atoi(v);

	else
		return -3;

	return 0;
}

int Grufti_displayvar(char *n, char *ret)
{
	/* Display char */
	if(isequal(n,"nick"))
		sprintf(ret,"\"%s\"",Grufti->botnick);

	else if(isequal(n,"username"))
		sprintf(ret,"\"%s\"",Grufti->botusername);

	else if(isequal(n,"realname"))
		sprintf(ret,"\"%s\"",Grufti->botrealname);

	else if(isequal(n,"maintainer"))
		sprintf(ret,"\"%s\"",Grufti->maintainer);

	else if(isequal(n,"hostname"))
		sprintf(ret,"\"%s\"",Grufti->bothostname);

	else if(isequal(n,"virtual-hostname"))
		sprintf(ret,"\"%s\"",Grufti->virtual_hostname);

	else if(isequal(n,"homepage"))
		sprintf(ret,"\"%s\"",Grufti->homepage);

	else if(isequal(n,"tmpdir"))
		sprintf(ret,"\"%s\"",Grufti->tmpdir);

	else if(isequal(n,"userfile"))
		sprintf(ret,"\"%s\"",Grufti->userfile);

	else if(isequal(n,"responsefile"))
		sprintf(ret,"\"%s\"",Grufti->respfile);

	else if(isequal(n,"helpfile"))
		sprintf(ret,"\"%s\"",Grufti->helpfile);

	else if(isequal(n,"locationfile"))
		sprintf(ret,"\"%s\"",Grufti->locafile);

	else if(isequal(n,"webcatchfile"))
		sprintf(ret,"\"%s\"",Grufti->webfile);

	else if(isequal(n,"statsfile"))
		sprintf(ret,"\"%s\"",Grufti->statsfile);

	else if(isequal(n,"banlist"))
		sprintf(ret,"\"%s\"",Grufti->banlist);

	else if(isequal(n,"eventfile"))
		sprintf(ret,"\"%s\"",Grufti->eventfile);

	else if(isequal(n,"xfer-incoming"))
		sprintf(ret,"\"%s\"",Grufti->xfer_incoming);

	else if(isequal(n,"xfer-files"))
		sprintf(ret,"\"%s\"",Grufti->xfer_files);

	else if(isequal(n,"backupdir"))
		sprintf(ret,"\"%s\"",Grufti->backupdir);

	else if(isequal(n,"botdir"))
		sprintf(ret,"\"%s\"",Grufti->botdir);

	else if(isequal(n,"admindir"))
		sprintf(ret,"\"%s\"",Grufti->admindir);

	else if(isequal(n,"logdir"))
		sprintf(ret,"\"%s\"",Grufti->logdir);

	else if(isequal(n,"motd-dcc"))
		sprintf(ret,"\"%s\"",Grufti->motd_dcc);

	else if(isequal(n,"welcome-dcc"))
		sprintf(ret,"\"%s\"",Grufti->welcome_dcc);

	else if(isequal(n,"motd-telnet"))
		sprintf(ret,"\"%s\"",Grufti->motd_telnet);

	else if(isequal(n,"telnet-banner"))
		sprintf(ret,"\"%s\"",Grufti->telnet_banner);

	else if(isequal(n,"combofile"))
		sprintf(ret,"\"%s\"",Grufti->combofile);

	else if(isequal(n,"pidfile"))
		sprintf(ret,"\"%s\"",Grufti->pidfile);

	else if(isequal(n,"alt-name1"))
		sprintf(ret,"\"%s\"",Grufti->alt_name1);

	else if(isequal(n,"alt-name2"))
		sprintf(ret,"\"%s\"",Grufti->alt_name2);

	else if(isequal(n,"ctcp-version"))
		sprintf(ret,"\"%s\"",Grufti->ctcp_version);

	else if(isequal(n,"ctcp-clientinfo"))
		sprintf(ret,"\"%s\"",Grufti->ctcp_clientinfo);

	else if(isequal(n,"ctcp-finger"))
		sprintf(ret,"\"%s\"",Grufti->ctcp_finger);

	else if(isequal(n,"shitlist-message"))
		sprintf(ret,"\"%s\"",Grufti->shitlist_message);

	else if(isequal(n,"mymode-on-IRC-join"))
		sprintf(ret,"\"%s\"",Grufti->mymode_on_IRC_join);

	else if(isequal(n,"takeover-in-progress-msg"))
		sprintf(ret,"\"%s\"",Grufti->takeover_in_progress_msg);

	else if(isequal(n,"smtp-gateway"))
		sprintf(ret,"\"%s\"",Grufti->smtp_gateway);

	else if(isequal(n,"idlekick-message"))
		sprintf(ret,"\"%s\"",Grufti->idlekick_message);


	/* Display int */
	else if(isequal(n,"irc"))
		sprintf(ret,"%d",Grufti->irc);

	else if(isequal(n,"server-timeout"))
		sprintf(ret,"%d",Grufti->server_timeout);

	else if(isequal(n,"server-ping-timeout"))
		sprintf(ret,"%d",Grufti->server_ping_timeout);

	else if(isequal(n,"server-quit-timeout"))
		sprintf(ret,"%d",Grufti->server_quit_timeout);

	else if(isequal(n,"chat-login-timeout"))
		sprintf(ret,"%d",Grufti->chat_login_timeout);

	else if(isequal(n,"chat-ident-timeout"))
		sprintf(ret,"%d",Grufti->chat_ident_timeout);

	else if(isequal(n,"smtp-timeout"))
		sprintf(ret,"%d",Grufti->smtp_timeout);

	else if(isequal(n,"chat-timeout"))
		sprintf(ret,"%d",Grufti->chat_timeout);

	else if(isequal(n,"establish-dcc-timeout"))
		sprintf(ret,"%d",Grufti->establish_dcc_timeout);

	else if(isequal(n,"serverpossible-timeout"))
		sprintf(ret,"%d",Grufti->serverpossible_timeout);

	else if(isequal(n,"max-dcc-connections"))
		sprintf(ret,"%d",Grufti->max_dcc_connections);

	else if(isequal(n,"telnet-port"))
		sprintf(ret,"%d",Grufti->reqd_telnet_port);

	else if(isequal(n,"gateway-port"))
		sprintf(ret,"%d",Grufti->reqd_gateway_port);

	else if(isequal(n,"chat-port"))
		sprintf(ret,"%d",Grufti->chat_port);

	else if(isequal(n,"num-ports-to-try"))
		sprintf(ret,"%d",Grufti->num_ports_to_try);

	else if(isequal(n,"max-nicks"))
		sprintf(ret,"%d",Grufti->max_nicks);

	else if(isequal(n,"max-greets"))
		sprintf(ret,"%d",Grufti->max_greets);

	else if(isequal(n,"dequeuenotes-freq"))
		sprintf(ret,"%d",Grufti->dequeuenotes_freq);

	else if(isequal(n,"login-attempts"))
		sprintf(ret,"%d",Grufti->login_attempts);

	else if(isequal(n,"radiate-freq"))
		sprintf(ret,"%d",Grufti->radiate_freq);

	else if(isequal(n,"ping-freq"))
		sprintf(ret,"%d",Grufti->ping_freq);

	else if(isequal(n,"join-freq"))
		sprintf(ret,"%d",Grufti->join_freq);

	else if(isequal(n,"nick-freq"))
		sprintf(ret,"%d",Grufti->nick_freq);

	else if(isequal(n,"luser-freq"))
		sprintf(ret,"%d",Grufti->luser_freq);

	else if(isequal(n,"netsplit-timeout"))
		sprintf(ret,"%d",Grufti->netsplit_timeout);

	else if(isequal(n,"memberissplit-timeout"))
		sprintf(ret,"%d",Grufti->memberissplit_timeout);

	else if(isequal(n,"radiate-timeout"))
		sprintf(ret,"%d",Grufti->radiate_timeout);

	else if(isequal(n,"min-servers"))
		sprintf(ret,"%d",Grufti->min_servers);

	else if(isequal(n,"welcome-newdcc"))
		sprintf(ret,"%d",Grufti->welcome_newdcc);

	else if(isequal(n,"show-motd-on-dcc-chat"))
		sprintf(ret,"%d",Grufti->show_motd_on_dcc_chat);

	else if(isequal(n,"show-motd-on-telnet"))
		sprintf(ret,"%d",Grufti->show_motd_on_telnet);

	else if(isequal(n,"show-motd-onlywhenchanged"))
		sprintf(ret,"%d",Grufti->show_motd_onlywhenchanged);

	else if(isequal(n,"show-telnet-banner"))
		sprintf(ret,"%d",Grufti->show_telnet_banner);

	else if(isequal(n,"background"))
		sprintf(ret,"%d",Grufti->background);

	else if(isequal(n,"console"))
		sprintf(ret,"%d",Grufti->console);

	else if(isequal(n,"show-internal-ports"))
		sprintf(ret,"%d",Grufti->show_internal_ports);

	else if(isequal(n,"max-saved-notes"))
		sprintf(ret,"%d",Grufti->max_saved_notes);

	else if(isequal(n,"location-changetime"))
		sprintf(ret,"%d",Grufti->location_changetime);

	else if(isequal(n,"max-urls"))
		sprintf(ret,"%d",Grufti->max_urls);

	else if(isequal(n,"wait-to-greet"))
		sprintf(ret,"%d",Grufti->wait_to_greet);

	else if(isequal(n,"wait-to-informofnotes"))
		sprintf(ret,"%d",Grufti->wait_to_informofnotes);

	else if(isequal(n,"write-freq"))
		sprintf(ret,"%d",Grufti->write_freq);

	else if(isequal(n,"public-flood-num"))
		sprintf(ret,"%d",Grufti->public_flood_num);

	else if(isequal(n,"public-flood-interval"))
		sprintf(ret,"%d",Grufti->public_flood_interval);

	else if(isequal(n,"public-flood-timeout"))
		sprintf(ret,"%d",Grufti->public_flood_timeout);

	else if(isequal(n,"xfer-maxfilesize"))
		sprintf(ret,"%d",Grufti->xfer_maxfilesize);

	else if(isequal(n,"xfer-remove-incomplete"))
		sprintf(ret,"%d",Grufti->xfer_remove_incomplete);

	else if(isequal(n,"xfer-blocksize"))
		sprintf(ret,"%d",Grufti->xfer_blocksize);

	else if(isequal(n,"timeout-accounts"))
		sprintf(ret,"%d",Grufti->timeout_accounts);

	else if(isequal(n,"timeout-notes"))
		sprintf(ret,"%d",Grufti->timeout_notes);

	else if(isequal(n,"timeout-notices"))
		sprintf(ret,"%d",Grufti->timeout_notices);

	else if(isequal(n,"timeout-users"))
		sprintf(ret,"%d",Grufti->timeout_users);

	else if(isequal(n,"notify-of-notes-on-signon"))
		sprintf(ret,"%d",Grufti->notify_of_notes_on_signon);

	else if(isequal(n,"auto-detect-takeover"))
		sprintf(ret,"%d",Grufti->auto_detect_takeover);

	else if(isequal(n,"takeover-in-progress"))
		sprintf(ret,"%d",Grufti->takeover_in_progress);

	else if(isequal(n,"keep-tmpfiles"))
		sprintf(ret,"%d",Grufti->keep_tmpfiles);

	else if(isequal(n,"backup-userlist"))
		sprintf(ret,"%d",Grufti->backup_userlist);

	else if(isequal(n,"backup-response"))
		sprintf(ret,"%d",Grufti->backup_response);

	else if(isequal(n,"backup-locations"))
		sprintf(ret,"%d",Grufti->backup_locations);

	else if(isequal(n,"backup-banlist"))
		sprintf(ret,"%d",Grufti->backup_banlist);

	else if(isequal(n,"backup-events"))
		sprintf(ret,"%d",Grufti->backup_events);

	else if(isequal(n,"unban-protected-users"))
		sprintf(ret,"%d",Grufti->unban_protected_users);

	else if(isequal(n,"kick-those-who-ban-prot"))
		sprintf(ret,"%d",Grufti->kick_those_who_ban_prot);

	else if(isequal(n,"respond-to-haveyouseen"))
		sprintf(ret,"%d",Grufti->respond_to_haveyouseen);

	else if(isequal(n,"respond-to-url"))
		sprintf(ret,"%d",Grufti->respond_to_url);

	else if(isequal(n,"respond-to-bday"))
		sprintf(ret,"%d",Grufti->respond_to_bday);

	else if(isequal(n,"respond-to-showbdays"))
		sprintf(ret,"%d",Grufti->respond_to_showbdays);

	else if(isequal(n,"respond-to-email"))
		sprintf(ret,"%d",Grufti->respond_to_email);

	else if(isequal(n,"respond-to-stat"))
		sprintf(ret,"%d",Grufti->respond_to_stat);

	else if(isequal(n,"send-notice-nightly-report"))
		sprintf(ret,"%d",Grufti->send_notice_nightly_report);

	else if(isequal(n,"send-notice-die"))
		sprintf(ret,"%d",Grufti->send_notice_die);

	else if(isequal(n,"send-notice-level-plus"))
		sprintf(ret,"%d",Grufti->send_notice_level_plus);

	else if(isequal(n,"send-notice-level-minus"))
		sprintf(ret,"%d",Grufti->send_notice_level_minus);

	else if(isequal(n,"send-notice-DCC-receive"))
		sprintf(ret,"%d",Grufti->send_notice_DCC_receive);

	else if(isequal(n,"send-notice-user-created"))
		sprintf(ret,"%d",Grufti->send_notice_user_created);

	else if(isequal(n,"send-notice-user-deleted"))
		sprintf(ret,"%d",Grufti->send_notice_user_deleted);

	else if(isequal(n,"send-notice-response-created"))
		sprintf(ret,"%d",Grufti->send_notice_response_created);

	else if(isequal(n,"send-notice-response-deleted"))
		sprintf(ret,"%d",Grufti->send_notice_response_deleted);

	else if(isequal(n,"send-notice-location-created"))
		sprintf(ret,"%d",Grufti->send_notice_location_created);

	else if(isequal(n,"send-notice-location-deleted"))
		sprintf(ret,"%d",Grufti->send_notice_location_deleted);

	else if(isequal(n,"send-notice-event-created"))
		sprintf(ret,"%d",Grufti->send_notice_event_created);

	else if(isequal(n,"send-notice-event-deleted"))
		sprintf(ret,"%d",Grufti->send_notice_event_deleted);

	else if(isequal(n,"send-notice-event-signsup"))
		sprintf(ret,"%d",Grufti->send_notice_event_signsup);

	else if(isequal(n,"send-notice-event-removes"))
		sprintf(ret,"%d",Grufti->send_notice_event_removes);

	else if(isequal(n,"default-leave-timeout"))
		sprintf(ret,"%d",Grufti->default_leave_timeout);

	else if(isequal(n,"max-leave-timeout"))
		sprintf(ret,"%d",Grufti->max_leave_timeout);

	else if(isequal(n,"default-hush-timeout"))
		sprintf(ret,"%d",Grufti->default_hush_timeout);

	else if(isequal(n,"max-hush-timeout"))
		sprintf(ret,"%d",Grufti->max_hush_timeout);

	else if(isequal(n,"sing-1015"))
		sprintf(ret,"%d",Grufti->sing_1015);

	else if(isequal(n,"allow-dcc-send"))
		sprintf(ret,"%d",Grufti->allow_dcc_send);

	else if(isequal(n,"kick-channel-idlers"))
		sprintf(ret,"%d",Grufti->kick_channel_idlers);

	else if(isequal(n,"idlekick-timeout"))
		sprintf(ret,"%d",Grufti->idlekick_timeout);

	else if(isequal(n,"kick-idle-opers"))
		sprintf(ret,"%d",Grufti->kick_idle_opers);

	else if(isequal(n,"idle-time"))
		sprintf(ret,"%d",Grufti->idle_time);

	else if(isequal(n,"quit-port-request-failed"))
		sprintf(ret,"%d",Grufti->quit_port_request_failed);

	else if(isequal(n,"require-dcc-for-pass-command"))
		sprintf(ret,"%d",Grufti->require_dcc_for_pass_cmd);

	else if(isequal(n,"pending-command-timeout"))
		sprintf(ret,"%d",Grufti->pending_command_timeout);

	else
		return 0;

	return 1;
}

void Grufti_showconfig()
{
	/* Display all variables */
	/* Display char */

	say("Config variable                  Value");
	say("-------------------------------- --------------------------------");
	sayf(0,0,"%-32s \"%s\"","nick",Grufti->botnick);
	sayf(0,0,"%-32s \"%s\"","username",Grufti->botusername);
	sayf(0,0,"%-32s \"%s\"","realname",Grufti->botrealname);
	sayf(0,0,"%-32s \"%s\"","maintainer",Grufti->maintainer);
	sayf(0,0,"%-32s \"%s\"","hostname",Grufti->bothostname);
	sayf(0,0,"%-32s \"%s\"","virtual-hostname",Grufti->virtual_hostname);
	sayf(0,0,"%-32s \"%s\"","homepage",Grufti->homepage);
	sayf(0,0,"%-32s \"%s\"","tmpdir",Grufti->tmpdir);
	sayf(0,0,"%-32s \"%s\"","userfile",Grufti->userfile);
	sayf(0,0,"%-32s \"%s\"","responsefile",Grufti->respfile);
	sayf(0,0,"%-32s \"%s\"","helpfile",Grufti->helpfile);
	sayf(0,0,"%-32s \"%s\"","locationfile",Grufti->locafile);
	sayf(0,0,"%-32s \"%s\"","webcatchfile",Grufti->webfile);
	sayf(0,0,"%-32s \"%s\"","statsfile",Grufti->statsfile);
	sayf(0,0,"%-32s \"%s\"","banlist",Grufti->banlist);
	sayf(0,0,"%-32s \"%s\"","eventfile",Grufti->eventfile);
	sayf(0,0,"%-32s \"%s\"","xfer-incoming",Grufti->xfer_incoming);
	sayf(0,0,"%-32s \"%s\"","xfer-files",Grufti->xfer_files);
	sayf(0,0,"%-32s \"%s\"","backupdir",Grufti->backupdir);
	sayf(0,0,"%-32s \"%s\"","botdir",Grufti->botdir);
	sayf(0,0,"%-32s \"%s\"","admindir",Grufti->admindir);
	sayf(0,0,"%-32s \"%s\"","logdir",Grufti->logdir);
	sayf(0,0,"%-32s \"%s\"","motd-dcc",Grufti->motd_dcc);
	sayf(0,0,"%-32s \"%s\"","motd-telnet",Grufti->motd_telnet);
	sayf(0,0,"%-32s \"%s\"","welcome-dcc",Grufti->welcome_dcc);
	sayf(0,0,"%-32s \"%s\"","telnet-banner",Grufti->telnet_banner);
	sayf(0,0,"%-32s \"%s\"","combofile",Grufti->combofile);
	sayf(0,0,"%-32s \"%s\"","pidfile",Grufti->pidfile);
	sayf(0,0,"%-32s \"%s\"","alt-name1",Grufti->alt_name1);
	sayf(0,0,"%-32s \"%s\"","alt-name2",Grufti->alt_name2);
	sayf(0,0,"%-32s \"%s\"","ctcp-version",Grufti->ctcp_version);
	sayf(0,0,"%-32s \"%s\"","ctcp-clientinfo",Grufti->ctcp_clientinfo);
	sayf(0,0,"%-32s \"%s\"","ctcp-finger",Grufti->ctcp_finger);
	sayf(0,0,"%-32s \"%s\"","shitlist-message",Grufti->shitlist_message);
	sayf(0,0,"%-32s \"%s\"","mymode-on-IRC-join",Grufti->mymode_on_IRC_join);
	sayf(0,0,"%-32s \"%s\"","takeover-in-progress-msg",Grufti->takeover_in_progress_msg);
	sayf(0,0,"%-32s \"%s\"","smtp-gateway",Grufti->smtp_gateway);
	sayf(0,0,"%-32s \"%s\"","idlekick-message",Grufti->idlekick_message);


	/* Display int */
	sayf(0,0,"%-32s %d","irc",Grufti->irc);
	sayf(0,0,"%-32s %d","server-timeout",Grufti->server_timeout);
	sayf(0,0,"%-32s %d","server-ping-timeout",Grufti->server_ping_timeout);
	sayf(0,0,"%-32s %d","server-quit-timeout",Grufti->server_quit_timeout);
	sayf(0,0,"%-32s %d","chat-login-timeout",Grufti->chat_login_timeout);
	sayf(0,0,"%-32s %d","chat-ident-timeout",Grufti->chat_ident_timeout);
	sayf(0,0,"%-32s %d","smtp-timeout",Grufti->smtp_timeout);
	sayf(0,0,"%-32s %d","chat-timeout",Grufti->chat_timeout);
	sayf(0,0,"%-32s %d","establish-dcc-timeout",Grufti->establish_dcc_timeout);
	sayf(0,0,"%-32s %d","serverpossible-timeout",Grufti->serverpossible_timeout);
	sayf(0,0,"%-32s %d","max-dcc-connections",Grufti->max_dcc_connections);
	sayf(0,0,"%-32s %d","telnet-port",Grufti->reqd_telnet_port);
	sayf(0,0,"%-32s %d","gateway-port",Grufti->reqd_gateway_port);
	sayf(0,0,"%-32s %d","chat-port",Grufti->chat_port);
	sayf(0,0,"%-32s %d","num-ports-to-try",Grufti->num_ports_to_try);
	sayf(0,0,"%-32s %d","max-nicks",Grufti->max_nicks);
	sayf(0,0,"%-32s %d","max-greets",Grufti->max_greets);
	sayf(0,0,"%-32s %d","login-attempts",Grufti->login_attempts);
	sayf(0,0,"%-32s %d","dequeuenotes-freq",Grufti->dequeuenotes_freq);
	sayf(0,0,"%-32s %d","radiate-freq",Grufti->radiate_freq);
	sayf(0,0,"%-32s %d","ping-freq",Grufti->ping_freq);
	sayf(0,0,"%-32s %d","join-freq",Grufti->join_freq);
	sayf(0,0,"%-32s %d","nick-freq",Grufti->nick_freq);
	sayf(0,0,"%-32s %d","luser-freq",Grufti->luser_freq);
	sayf(0,0,"%-32s %d","netsplit-timeout",Grufti->netsplit_timeout);
	sayf(0,0,"%-32s %d","memberissplit-timeout",Grufti->memberissplit_timeout);
	sayf(0,0,"%-32s %d","radiate-timeout",Grufti->radiate_timeout);
	sayf(0,0,"%-32s %d","min-servers",Grufti->min_servers);
	sayf(0,0,"%-32s %d","welcome-newdcc",Grufti->welcome_newdcc);
	sayf(0,0,"%-32s %d","show-motd-on-dcc-chat",Grufti->show_motd_on_dcc_chat);
	sayf(0,0,"%-32s %d","show-motd-on-telnet",Grufti->show_motd_on_telnet);
	sayf(0,0,"%-32s %d","show-motd-onlywhenchanged",Grufti->show_motd_onlywhenchanged);
	sayf(0,0,"%-32s %d","show-telnet-banner",Grufti->show_telnet_banner);
	sayf(0,0,"%-32s %d","background",Grufti->background);
	sayf(0,0,"%-32s %d","console",Grufti->console);
	sayf(0,0,"%-32s %d","show-internal-ports",Grufti->show_internal_ports);
	sayf(0,0,"%-32s %d","max-saved-notes",Grufti->max_saved_notes);
	sayf(0,0,"%-32s %d","location-changetime",Grufti->location_changetime);
	sayf(0,0,"%-32s %d","max-urls",Grufti->max_urls);
	sayf(0,0,"%-32s %d","wait-to-greet",Grufti->wait_to_greet);
	sayf(0,0,"%-32s %d","wait-to-informofnotes",Grufti->wait_to_informofnotes);
	sayf(0,0,"%-32s %d","write-freq",Grufti->write_freq);
	sayf(0,0,"%-32s %d","public-flood-num",Grufti->public_flood_num);
	sayf(0,0,"%-32s %d","public-flood-interval",Grufti->public_flood_interval);
	sayf(0,0,"%-32s %d","public-flood-timeout",Grufti->public_flood_timeout);
	sayf(0,0,"%-32s %d","xfer-maxfilesize",Grufti->xfer_maxfilesize);
	sayf(0,0,"%-32s %d","xfer-remove-incomplete",Grufti->xfer_remove_incomplete);
	sayf(0,0,"%-32s %d","xfer-blocksize",Grufti->xfer_blocksize);
	sayf(0,0,"%-32s %d","timeout-accounts",Grufti->timeout_accounts);
	sayf(0,0,"%-32s %d","timeout-notes",Grufti->timeout_notes);
	sayf(0,0,"%-32s %d","timeout-notices",Grufti->timeout_notices);
	sayf(0,0,"%-32s %d","timeout-users",Grufti->timeout_users);
	sayf(0,0,"%-32s %d","notify-of-notes-on-signon",Grufti->notify_of_notes_on_signon);
	sayf(0,0,"%-32s %d","auto-detect-takeover",Grufti->auto_detect_takeover);
	sayf(0,0,"%-32s %d","takeover-in-progress",Grufti->takeover_in_progress);
	sayf(0,0,"%-32s %d","keep-tmpfiles",Grufti->keep_tmpfiles);
	sayf(0,0,"%-32s %d","backup-userlist",Grufti->backup_userlist);
	sayf(0,0,"%-32s %d","backup-response",Grufti->backup_response);
	sayf(0,0,"%-32s %d","backup-locations",Grufti->backup_locations);
	sayf(0,0,"%-32s %d","backup-banlist",Grufti->backup_banlist);
	sayf(0,0,"%-32s %d","backup-events",Grufti->backup_events);
	sayf(0,0,"%-32s %d","unban-protected-users",Grufti->unban_protected_users);
	sayf(0,0,"%-32s %d","kick-those-who-ban-prot",Grufti->kick_those_who_ban_prot);
	sayf(0,0,"%-32s %d","respond-to-haveyouseen",Grufti->respond_to_haveyouseen);
	sayf(0,0,"%-32s %d","respond-to-url",Grufti->respond_to_url);
	sayf(0,0,"%-32s %d","respond-to-bday",Grufti->respond_to_bday);
	sayf(0,0,"%-32s %d","respond-to-showbdays",Grufti->respond_to_showbdays);
	sayf(0,0,"%-32s %d","respond-to-email",Grufti->respond_to_email);
	sayf(0,0,"%-32s %d","respond-to-stat",Grufti->respond_to_stat);
	sayf(0,0,"%-32s %d","send-notice-nightly-report",Grufti->send_notice_nightly_report);
	sayf(0,0,"%-32s %d","send-notice-die",Grufti->send_notice_die);
	sayf(0,0,"%-32s %d","send-notice-level-plus",Grufti->send_notice_level_plus);
	sayf(0,0,"%-32s %d","send-notice-level-minus",Grufti->send_notice_level_minus);
	sayf(0,0,"%-32s %d","send-notice-DCC-receive",Grufti->send_notice_DCC_receive);
	sayf(0,0,"%-32s %d","send-notice-user-created",Grufti->send_notice_user_created);
	sayf(0,0,"%-32s %d","send-notice-user-deleted",Grufti->send_notice_user_deleted);
	sayf(0,0,"%-32s %d","send-notice-response-created",Grufti->send_notice_response_created);
	sayf(0,0,"%-32s %d","send-notice-response-deleted",Grufti->send_notice_response_deleted);
	sayf(0,0,"%-32s %d","send-notice-location-created",Grufti->send_notice_location_created);
	sayf(0,0,"%-32s %d","send-notice-location-deleted",Grufti->send_notice_location_deleted);
	sayf(0,0,"%-32s %d","send-notice-event-created",Grufti->send_notice_event_created);
	sayf(0,0,"%-32s %d","send-notice-event-deleted",Grufti->send_notice_event_deleted);
	sayf(0,0,"%-32s %d","send-notice-event-signsup",Grufti->send_notice_event_signsup);
	sayf(0,0,"%-32s %d","send-notice-event-removes",Grufti->send_notice_event_removes);
	sayf(0,0,"%-32s %d","default-leave-timeout",Grufti->default_leave_timeout);
	sayf(0,0,"%-32s %d","max-leave-timeout",Grufti->max_leave_timeout);
	sayf(0,0,"%-32s %d","default-hush-timeout",Grufti->default_hush_timeout);
	sayf(0,0,"%-32s %d","max-hush-timeout",Grufti->max_hush_timeout);
	sayf(0,0,"%-32s %d","sing-1015",Grufti->sing_1015);
	sayf(0,0,"%-32s %d","allow-dcc-send",Grufti->allow_dcc_send);
	sayf(0,0,"%-32s %d","kick-channel-idlers",Grufti->kick_channel_idlers);
	sayf(0,0,"%-32s %d","idlekick-timeout",Grufti->idlekick_timeout);
	sayf(0,0,"%-32s %d","kick-idle-opers",Grufti->kick_idle_opers);
	sayf(0,0,"%-32s %d","plus-i-idlekick-timeout",Grufti->plus_i_idlekick_timeout);
	sayf(0,0,"%-32s %d","idle-time",Grufti->idle_time);
	sayf(0,0,"%-32s %d","quit-port-request-failed",Grufti->quit_port_request_failed);
	sayf(0,0,"%-32s %d","require-dcc-for-pass-cmd",Grufti->require_dcc_for_pass_cmd);
	sayf(0,0,"%-32s %d","pending-command-timeout",Grufti->pending_command_timeout);
	say("-------------------------------- --------------------------------");
}


int Grufti_readconfigfile(int *errors)
{
	int	line = 0, x;
	char s[BUFSIZ], ident[BUFSIZ];
	char token1[BUFSIZ], token2[BUFSIZ], token3[BUFSIZ], token4[BUFSIZ];
	FILE *f;

	*errors = 0;
	f = fopen(Grufti->configfile,"r");
	if(f == NULL)
		fatal_error("Config file \"%s\" not found.", Grufti->configfile);

	/* Read each line */
	while(readline(f,s))
	{
		line++;

		/* Remove spaces from beginning and end */
		rmspace(s);

		/* ignore comments */
		if(s[0] && s[0] != '#')
		{
			/* extract identifier code */
			str_element(ident,s,1);
			quoted_str_element(token1,s,2);
			quoted_str_element(token2,s,3);
			quoted_str_element(token3,s,4);
			quoted_str_element(token4,s,5);

			if(!token1[0])
			{
				warning("%s:%d - Missing argument.", Grufti->configfile, line);
				(*errors)++;
			}
#ifdef MSK
			else if (isequal(ident, "mergeusers"))
			{
				if (!token1[0])
				{
					warning("%s:%d - Incomplete 'mergeusers' expression.",
						Grufti->configfile, line);
					(*errors)++;
				}
				else
				{
					strncpy(mergeusers,token1,MAXPATHLEN);
				}
			}
#endif
			else if(isequal(ident,"set"))
			{
				if(!token1[0] || !token2[0])
				{
					warning("%s:%d - Incomplete 'set' expression.",
						Grufti->configfile, line);
					(*errors)++;
				}
				else
				{
					x = Grufti_setvar(token1, token2);
					if(x < 0)
						(*errors)++;

					if(x == -1)
						warning("%s:%d - Incomplete 'set' expression.",
							Grufti->configfile, line);
					else if(x == -2)
						warning("%s:%d - Argument not quoted.",
							Grufti->configfile, line);
					else if(x == -3)
						warning("%s:%d - Unknown variable: %s",
							Grufti->configfile, line, token1);
				}
			}
			else if(isequal(ident,"server"))
			{
				if(!token2[0])
					Server_addserver(token1,DEFAULT_PORT);
				else if(!isanumber(token2))
				{
					warning("%s:%d - Invalid server port: %s",
						Grufti->configfile, line, token2);
					(*errors)++;
				}
				else
					Server_addserver(token1,atoi(token2));
			}
			else if(isequal(ident,"command"))
			{
				if(!token1[0] || !token2[0])
				{
					warning("%s:%d - Incomplete 'command' expression.",
						Grufti->configfile, line);
					(*errors)++;
				}
				else
				{
					x = command_reset_levels(token1, token2);
					if(x < 0)
						(*errors)++;

					if(x == -1)
						warning("%s:%d - Command name not quoted.",
							Grufti->configfile, line);
					else if(x == -2)
						warning("%s:%d - Command name not found.",
							Grufti->configfile, line);
					else if(x == -3)
						warning("%s:%d - Invalid command levels.",
							Grufti->configfile, line);
				}
			}
			else if(isequal(ident,"channel"))
			{
				if(!token1[0] || !token2[0])
				{
					warning("%s:%d - Incomplete 'channel' expression.",
						Grufti->configfile, line);
					(*errors)++;
				}
				else if(isequal(token1,"add"))
					Channel_addchannel(token2);
				else if(isequal(token1,"set"))
				{
					if(!token3[0] || !token4[0])
					{
						warning("%s:%d - Incomplete 'channel set' expression.",
							Grufti->configfile, line);
						(*errors)++;
					}
					else
						Channel_setchannel(token2,token3,token4);
				}
				else
				{
					warning("%s:%d - Unknown channel command: %s",
						Grufti->configfile, line, token1);
					(*errors)++;
				}
			}
			else if(isequal(ident,"logfile"))
			{
				if(!token1[0] || !token2[0] || !token3[0])
				{
					warning("%s:%d - Incomplete 'logfile' expression.",
						Grufti->configfile, line);
					(*errors)++;
				}
				else
				{
					x = Log_addnewlog(token1, token2, token3, token4);
					if(x < 0)
						(*errors)++;

					if(x == -1)
						warning("%s:%d - Log type RAW not defined.",
							Grufti->configfile, line);
					else if(x == -2)
						warning("%s:%d - Log filename not quoted.",
							Grufti->configfile, line);
				}
			}
			else if(isequal(ident,"response-public"))
			{
				if(!token1[0] || !token2[0] || !token3[0])
				{
					warning("%s:%d - Incomplete 'response-public' expression.",
						Grufti->configfile, line);
					(*errors)++;
				}
				else
				{
					x = Response_addnewtype(token1, token2, token3,
						RTYPE_PUBLIC);
					if(x < 0)
						(*errors)++;

					if(x == -1)
						warning("%s:%d - Response name not quoted.",
							Grufti->configfile, line);
					if(x == -2)
						warning("%s:%d - Response matches not quoted.",
							Grufti->configfile, line);
				}
			}
			else if(isequal(ident,"response-private"))
			{
				if(!token1[0] || !token2[0] || !token3[0])
				{
					warning("%s:%d - Incomplete 'response-private' expression.",
						Grufti->configfile, line);
					(*errors)++;
				}
				else
				{
					x = Response_addnewtype(token1, token2, token3,
						RTYPE_PRIVATE);
					if(x < 0)
						(*errors)++;

					if(x == -1)
						warning("%s:%d - Response name not quoted.",
							Grufti->configfile, line);
					if(x == -2)
						warning("%s:%d - Response matches not quoted.",
							Grufti->configfile, line);
				}
			}
			else
			{
				warning("%s:%d - Invalid identifier: %s",
					Grufti->configfile, line, ident);
				(*errors)++;
			}
		}
	}
	fclose(f);

	return line;
}


void Grufti_checkcrucial(int *errors)
{
	*errors = 0;

	if(!Grufti->botnick[0])
	{
		warning("Crucial config entry not set: nick");
		(*errors)++;
	}

	/* make sure botnick is a valid nick */
	if(Grufti->botnick[0] && invalidnick(Grufti->botnick))
	{
		warning("Invalid bot nick: %s", Grufti->botnick);
		(*errors)++;
	}

	if(!Grufti->botusername[0])
	{
		warning("Crucial config entry not set: username");
		(*errors)++;
	}

	if(!Grufti->botrealname[0])
	{
		warning("Crucial config entry not set: realname");
		(*errors)++;
	}

	if(!Grufti->userfile[0])
	{
		warning("Crucial config entry not set: userfile");
		(*errors)++;
	}
}


/* NEW as of July 5, 98 */
void warning(char *format, ...)
{
	va_list args;
	char buf[BUFSIZ];

	va_start(args, format);
	vsprintf(buf, format, args);
	va_end(args);

	Log_write(LOG_DEBUG,"*","[WARNING] %s", buf);
	printf("WARNING: %s\n", buf);
}

void debug(char *format, ...)
{
	va_list args;
	char buf[BUFSIZ];

	va_start(args, format);
	vsprintf(buf, format, args);
	va_end(args);

	Log_write(LOG_DEBUG,"*","[DEBUG] %s", buf);
#if 0
	if(strncmp(buf, "net:", 4) == 0)
	{
		/* dump the socktbl */
		net_dump_socktbl();
	}
#endif
}

void internal(char *format, ...)
{
	va_list args;
	char buf[BUFSIZ];

	va_start(args, format);
	vsprintf(buf, format, args);
	va_end(args);

	Log_write(LOG_DEBUG,"*","[INTERNAL] %s", buf);
	printf("INTERNAL: %s\n", buf);
}

void fatal_error(char *format, ...)
{
	va_list args;
	char buf[BUFSIZ];

	va_start(args, format);
	vsprintf(buf, format, args);
	va_end(args);

	Log_write(LOG_ALL,"*","[FATAL] %s", buf);
	printf("FATAL: %s\n\n", buf);

	unlink(Grufti->pidfile);
	exit(0);
}

