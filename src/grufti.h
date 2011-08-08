/******************************************************************************
 * Steve Rider - stever@area23.org
 * Grufti - an interactive, service-oriented bot for IRC 
 *
 * grufti.h - Grufti specific defines.  Config file settings.
 *
 *****************************************************************************/
/* 28 April 97 */

#ifndef GRUFTI_H
#define GRUFTI_H

#include "config.h"

#if HAVE_SYS_NDIR_H || HAVE_SYS_DIR_H || HAVE_NDIR_H
# define dirent direct
# define NAMLEN(dirent) (dirent)->d_namlen
# if HAVE_SYS_NDIR_H
#  include <sys/ndir.h>
# endif
# if HAVE_SYS_DIR_H
#  include <sys/dir.h>
# endif
# if HAVE_NDIR_H
#  include <ndir.h>
# endif
#else
# include <dirent.h>
# define NAMLEN(dirent) Extent((dirent)->d_name)
#endif
#include <pwd.h>

#if HAVE_FCNTL_H
#include <fcntl.h>
#endif

#if !defined(S_ISDIR)
#define S_ISDIR(mode) (((mode) & S_IFMT) == S_IFDIR)
#endif

#if HAVE_STDARG_H
#include <stdarg.h>
#else
#include <varargs.h>
#endif

#if HAVE_UNISTD_H
#include <unistd.h>
#endif

#if HAVE_SYS_TIME_H
#if TIME_WITH_SYS_TIME
#include <time.h>
#endif
#include <sys/time.h>
#endif

#if !STDC_HEADERS
#include "This_compiler_doesn't_support_ANSI_standard_headers!"
#endif

/*
 * Unsigned data types (BSD) usually defined in <sys/types.h>.  (which is
 * usually included before we get here, so no need to include again.)
 */
#ifndef u_long
#define u_long unsigned long
#endif
#ifndef u_short
#define u_short unsigned short
#endif
#ifndef u_char
#define u_char unsigned char
#endif
#ifndef u_int
#define u_int unsigned int
#endif

#if SIZEOF_INT == 4
#define IP unsigned int
#else
#if SIZEOF_LONG == 4
#define IP unsigned long
#else
#include "no_32_bit_type_found!"
#endif
#endif

/* Core modules */
#include "misc.h"  /* xmem, strcasecmp, ... */

#include "typedefs.h"

extern char cx_file[];
extern int cx_line;

/*****************************************************************************/
/* Grufti-specific defines */

#define DEFAULT_CONFIGFILE "config.gb"
#define DEFAULT_PORT 6667
#define INTERFACE_REQUIREDVER 1001

#define LINELENGTH 65
#define SCREENWIDTHLEN 80

/* String lengths */
#define HOSTLEN 180
#ifdef MAXHOSTNAMELEN
#define MAXHOSTLENGTH MAXHOSTNAMELEN
#else
#define MAXHOSTLENGTH 256
#endif
#define MAXREADBUFLENGTH 512 /* Max length over network (IRC specific) */
#define STDBUF MAXREADBUFLENGTH
#define UHOSTLEN 180
#define CHANLEN 81
#define NICKLEN 30
#define USERLEN 30
#define KEYLEN 50
#define TOPICLEN 180
#define SERVERLEN 528
#define USERFLAGLEN 40
#define FLAGLONGLEN 250
#define FLAGSHORTLEN 40
#define LOGFLAGLEN 40
#define DCCFLAGLEN 240
#define NICKLISTLEN 180
#define TIMESHORTLEN 40
#define TIMELONGLEN 60
#define DATESHORTLEN 40
#define DATELONGLEN 40
#define USERFIELDLEN 54
#define PASSWORD_CIPHER_LEN 20
#define PASSWORD_TEXT_LEN 9
#define GREETLENGTH 160
#define LENGTH_FINGERINFO 47
#define LENGTH_EMAILLINE 52
#define LENGTH_WEBLINE 52
#define LENGTH_PLANLINE LINELENGTH - 1
#define FLOOD_INTERVAL 4
#define FLOOD_SIZE 460
#define RESPRATE_NUM 50
#define ENTRIES_FOR_ONE_COL 20

#define NUMPLANLINES 6
#define TRUE 1
#define FALSE 0
#define ON 1
#define OFF 0
#define HIGHESTASCII 126
#define null(type) (type) 0L
#define DAYSINWEEK 7 
#define context { strcpy(cx_file,__FILE__); cx_line=__LINE__; }

/* IRCFREQ (time in seconds per week, low end) */
#define IRC_RARELY 1000		/* few-1/2 hr per day per week */
#define IRC_LOW 12600		/* 1/2-2 hrs per day per week */
#define IRC_MEDIUM 50400	/* 2-5 hrs per day per week */
#define IRC_HIGH 126000		/* 5-10 hrs per day per week */
#define IRC_NO_LIFE 252000	/* over 10 hrs per day per week */

/* Let all files have access to the Grufti object */
extern struct Grufti_object *Grufti;

/*****************************************************************************/
/* Main Grufti object */
typedef struct Grufti_object {

	/* Config entries */
	char botnick[NICKLEN];		/* bot nick */
	char botusername[HOSTLEN];	/* bot account username */
	char botrealname[128];		/* bot finger info */
	char maintainer[128];		/* who is the maintainer */
	char bothostname[HOSTLEN];	/* hostname bot is running on */
	char virtual_hostname[HOSTLEN];	/* hostname bot is running on */
	char homepage[256];		/* homepage info */
	char tmpdir[256];		/* tmp directory */
	char userfile[256];		/* userfile */
	char respfile[256];		/* responsefile */
	char helpfile[256];		/* helpfile */
	char locafile[256];		/* location file */
	char webfile[256];		/* webcatch file */
	char statsfile[256];		/* statistics file */
	char banlist[256];		/* banlist file */
	char eventfile[256];		/* eventfile file */
	char xfer_incoming[256];	/* Incoming directory for xfers */
	char xfer_files[256];		/* Where outgoing files are stored */
	char backupdir[256];		/* backup directory */
	char botdir[256];		/* bot directory */
	char admindir[256];		/* admin directory */
	char logdir[256];		/* logs directory */
	char motd_dcc[256];		/* MOTD for dcc chats */
	char motd_telnet[256];		/* MOTD for telnet */
	char welcome_dcc[256];		/* New user info for dcc chat */
	char telnet_banner[256];	/* bannerfile for telnet connections */
	char combofile[256];		/* combo file */
	char pidfile[256];		/* pid file */
	char alt_name1[256];		/* alternate name 1 */
	char alt_name2[256];		/* alternate name 2 */
	char ctcp_version[256];		/* ctcp version */
	char ctcp_clientinfo[256];	/* ctcp clientinfo options */
	char ctcp_finger[256];		/* ctcp finger */
	char shitlist_message[256];	/* when user is banned */
	char mymode_on_IRC_join[25];	/* my mode when i first join IRC */
	char takeover_in_progress_msg[256];
	char smtp_gateway[256];		/* smtp hostname */
	char idlekick_message[256];	/* message when kicking idlers */

	int irc;			/* IRC yes or no */
	int server_timeout;		/* timeout: connect to server */
	int server_ping_timeout;	/* timeout: no pong reply */
	int server_quit_timeout;	/* timeout: no quit reply */
	int chat_login_timeout;		/* timeout: user logging in */
	int chat_ident_timeout;		/* timeout: waiting for ident reply */
	int smtp_timeout;		/* timeout: smtp client */
	int chat_timeout;		/* timeout: IRC_CHAT or TELNET_CHAT */
	int establish_dcc_timeout;	/* timeout: waiting for connection */
	int serverpossible_timeout;	/* timeout: waiting for possible serv */
	int max_dcc_connections;	/* max number of dcc connections */
	int reqd_telnet_port;		/* requested telnet port */
	int reqd_gateway_port;		/* requested gateway port */
	int chat_port;			/* requested chat port to start trying*/
	int num_ports_to_try;		/* how many ports before giving up */
	int max_nicks;			/* how many nicks are they allowed */
	int max_greets;			/* how many greets are they allowed */
	int login_attempts;		/* number of attemps we'll allow */
	int dequeuenotes_freq;		/* how often to dump queue of Notes */
	int radiate_freq;		/* how often to radiate USERHOST */
	int ping_freq;			/* how often to ping myself */
	int join_freq;			/* how often to attempt joins */
	int nick_freq;			/* how often to try to get our nick */
	int luser_freq;			/* how often to send LUSER */
	int netsplit_timeout;		/* how long to stay split before jump */
	int memberissplit_timeout;	/* remove member if split too long */
	int radiate_timeout;		/* reset radiate queue if no 302 */
	int min_servers;		/* minimum servers before jumping */
	int welcome_newdcc;		/* welcome new dcc chats? */
	int show_motd_on_dcc_chat;	/* show motd on dcc chat? */
	int show_motd_on_telnet;	/* show motd on telnet */
	int show_motd_onlywhenchanged;	/* show motd only when changed */
	int show_telnet_banner;		/* show banner on telnet connect */
	int background;			/* fork into the background */
	int console;			/* use console mode */
	int show_internal_ports;	/* show internal ports in dccinfo */
	int max_saved_notes;		/* number of notes that can be saved */
	int location_changetime;	/* length a location can be changed */
	int max_urls;			/* max number of URL's to catch */
	int wait_to_greet;		/* time to wait before greeting again */
	int wait_to_informofnotes;	/* before informing of notes again */
	int flood_interval;		/* server flood time */
	int write_freq;			/* how often to write the datafiles */
	int public_flood_num;		/* num times in n seconds is a flood */
	int public_flood_interval;	/* n times in num seconds is flood */
	int public_flood_timeout;	/* how long before normal operations */
	int xfer_maxfilesize;		/* max filesize we'll accept */
	int xfer_remove_incomplete;	/* remove incomplete dcc xfer files */
	int xfer_blocksize;		/* block size of data to send */
	int timeout_accounts;		/* How long before accounts are del'd */
	int timeout_notes;		/* How long before notes are del'd */
	int timeout_notices;		/* How long before notices are del'd */
	int timeout_users;		/* How long before users are del'd */
	int notify_of_notes_on_signon;	/* Notify of notes on signon */
	int auto_detect_takeover;
	int takeover_in_progress;
	int keep_tmpfiles;		/* Keep .tmp files */
	int backup_userlist;		/* write backup files */
	int backup_response;		/* write backup files */
	int backup_locations;		/* write backup files */
	int backup_banlist;		/* write backup banlist */
	int backup_events;		/* write backup events */
	int unban_protected_users;	/* unban users with +f, +p, +3, +m */
	int kick_those_who_ban_prot;	/* should we kick them */
	int respond_to_haveyouseen;	/* respond to have you seen */
	int respond_to_url;		/* respond to url */
	int respond_to_bday;		/* respond to bday */
	int respond_to_showbdays;	/* respond to showbdays */
	int respond_to_email;		/* respond to email */
	int respond_to_stat;		/* respond to stat */
	int send_notice_nightly_report;	/* note masters nightly report */
	int send_notice_die;		/* send notice on die request */
	int send_notice_level_plus;	/* notice when user gets level */
	int send_notice_level_minus;	/* notice when user has level removed */
	int send_notice_DCC_receive;	/* notice when file is received */
	int send_notice_user_created;	/* notice when user is created */
	int send_notice_user_deleted;	/* notice when user is deleted */
	int send_notice_response_created; /* notice when response is created */
	int send_notice_response_deleted; /* notice when response is deleted */
	int send_notice_location_created; /* notice when location is created */
	int send_notice_location_deleted; /* notice when location is deleted */
	int send_notice_event_created;	/* notice when event is created */
	int send_notice_event_deleted;	/* notice when event is deleted */
	int send_notice_event_signsup;	/* notice */
	int send_notice_event_removes;	/* notice */
	int default_leave_timeout;	/* default leave timeout */
	int max_leave_timeout;		/* max leave timeout */
	int default_hush_timeout;	/* default hush timeout */
	int max_hush_timeout;		/* max hush timeout */
	int sing_1015;			/* sing "10:15" by the Cure */
	int allow_dcc_send;		/* allow dcc send requests */
	int kick_channel_idlers;	/* Kick channel idle members */
	int idlekick_timeout;		/* How often to check for idlers */
	int kick_idle_opers;		/* Kick idlers if they have +o? */
	int plus_i_idlekick_timeout;	/* Timer for reversing +i */
	int idle_time;			/* How long before user is idle */
	int quit_port_request_failed;	/* quit if port request failed */
	int require_dcc_for_pass_cmd;	/* require dcc for pass cmd */
	int pending_command_timeout;	/* Time until pending command is invalid */
	

	/* Internal settings */
	char	configfile[256];	/* configfile */
	char	copyright[256];		/* copyright info */
	time_t	online_since;		/* when bot was started */
	char	botuh[UHOSTLEN];	/* bot user@hostname */
	int	actual_telnet_port;	/* port we got for telnet server */
	int	actual_gateway_port;	/* port we got for gateway server */
	int	die;			/* Signalled when we want to die. */
	char	build[25];		/* which build */
	time_t	last_day_online;	/* Last day online, set at midnight */
	time_t	last_write;		/* last write to disk */
	int	users_deleted;		/* how many users were deleted yest */
	int	accts_deleted;		/* how many accts were deleted yest */
	int	notes_deleted;		/* how many notes were deleted yest */
	int	notes_forwarded;	/* how many notes were forwarded yest */
	int	bot_cmds[24];		/* how many bot cmds in last 24 hrs*/
	time_t	cleanup_time;		/* when the bot cleaned up */
	int	stats_active;		/* stats file */
	time_t	motd_modify_date;	/* When motd was last modified */

} Grufti_ob;

/* object Grufti low-level function declarations. */
void	Grufti_new();
u_long	Grufti_sizeof();
void	Grufti_freeall();

/* object Grufti data-specific function declarations. */
void	Grufti_display();

/* object Grufti high-level implementation-specific function declarations. */
void Grufti_checkcrucial(int *errors);
int	Grufti_readconfigfile(int *errors);
int	Grufti_setvar(char *n, char *v);
int	Grufti_displayvar(char *n, char *ret);
void Grufti_showconfig();

/* debugging */
void warning(char *fmt, ...);
void debug(char *fmt, ...);
void internal(char *fmt, ...);
void fatal_error(char *fmt, ...);

#ifdef MSK
extern char mergeusers[];
#endif
#endif /* GRUFTI_H */
