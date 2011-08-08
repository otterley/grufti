/******************************************************************************
 * Steve Rider - stever@area23.org
 * Grufti - an interactive, service-oriented bot for IRC 
 *
 * server.h - definitions for setting up servers and server communication
 *
 *****************************************************************************/
/* 28 April 97 */

#ifndef SERVER_H
#define SERVER_H

#include <stdio.h>
#include "typedefs.h"
#include "grufti.h"

/* Let all files who need Server have access to the Server object */
extern struct Server_object *Server;


/*****************************************************************************/
/* object Server declaration */
typedef struct Server_object {
	int	sock;			/* server socket */
	servrec	*serverlist;		/* -> list of servers */
	servrec	*last;			/* -> last server record */
	int	num_servers;		/* number of servers config'd */
	servrec	*cur;			/* -> current server */
	servrec	*tojump;		/* -> server to jump to */
	time_t	trying_server;		/* trying a server right now? */
	time_t	started_altnick;	/* When alt nick was used */
	time_t	time_sent_ping;		/* Used to gauge server lag */
	time_t	time_sent_quit;		/* Time when QUIT was sent */
	time_t	time_sent_luser;	/* time when LUSER was sent */
	time_t	time_noserverpossible;	/* Time when no server was possible */
	time_t	time_tried_nick;	/* When we last tried our nick */
	u_long	bytes_in;		/* bytes received from server */
	u_long	bytes_out;		/* bytes written to server */
	time_t	last_write_time;	/* last write time */
	time_t	lag;			/* lag time from last ping. */
	char	currentnick[NICKLEN];	/* current nick on server */
	u_long	flags;			/* server flags */
	int	q_sent[FLOOD_INTERVAL];	/* bytes sent in last n seconds / sec */
	int	q_sent_pos;		/* current q_sent pos */
	squeue	*queue;			/* -> server queue */
	squeue	*lastqueue;		/* -> last queue */
	int	num_q_msgs;		/* number of msgs in queue */

	/* modifiable fields, pre-loaded by Server_parse() */
	char	 sbuf[BUFSIZ];		/* misc storage */
	char	 mode[25];		/* mode storage */
	char	 chname[CHANLEN];	/* channel name storage */
	char	 to[HOSTLEN];		/* to storage */
	char	 code[121];		/* server codes ("001"...) */
	char	 from[SERVERLEN];	/* from field */
	char	 from_nuh[SERVERLEN];	/* guarantees in format n!u@h */
	char	 from_n[UHOSTLEN];	/* nick of who it was from */
	char	 from_u[UHOSTLEN];	/* uh of who it was from */
	char	 from_uh[UHOSTLEN];	/* host of who it was from */
	char	 from_h[HOSTLEN];	/* host of who it was from */
	char	 msg[SERVERLEN];	/* the parameters */
	
	/* send buffer */
	char	send_buf[SERVERLEN];	/* send buffer */

} Server_ob;

/* object Server low-level function declarations. */
void	Server_new();
u_long	Server_sizeof();
void	Server_freeall();

/* object Server data-specific function declarations. */
void	Server_display();

/* low-level Server structure function declarations. */
void	Server_appendserver(servrec *s);
void	Server_deleteserver(servrec *s);
void	Server_freeallservers();
void	Server_appendq(squeue *q);
void	Server_insertqatfront(squeue *q);
void	Server_deleteq(squeue *q);
void	Server_freeallqs();

/* object Server high-level implementation-specific function declarations. */
servrec	*Server_addserver(char *servername, int port);
servrec	*Server_nextserver();
void	Server_disconnect();
void	Server_connect();
void	Server_makeuntried(servrec *serv);
void	Server_makealluntried();
void	Server_noserverpossible();
int	Server_isanyserverpossible();
void	Server_gotactivity(char *buf);
void	Server_parsemsg(char *in);
void	Server_resetbuffers();
void	Server_write(char priority, char *format, ...);
char	*Server_name();
int	Server_port();
int	Server_isfromme();
void	Server_checkfortimeout();
int	Server_isconnected();
int	Server_isactive();
int	Server_isregistered();
void	Server_gotEOF();
int	Server_usingaltnick();
time_t	Server_avglag(servrec *serv);
int	Server_waitingforpong();
int	Server_waitingforluser();
void	Server_sendluser();
void	Server_gotpong(time_t lag);
void	Server_sendping();
int	Server_timeconnected(servrec *serv);
void	Server_setcurrentnick(char *nick);
servrec	*Server_serverandport(char *name, int port);
servrec	*Server_server(char *name);
void	Server_quit(char *msg);
void	Server_setreason(char *reason);
void	Server_nick(char *nick);
void	Server_leavingIRC();
void	Server_pushqueue(char *msg, char flag);
void	Server_popqueue(char *msg);
int	Server_bytesinfloodinterval();
void	Server_dequeue();
void	Server_dumpqueue();
int	Server_isqueueempty();



/* ASDF */
/*****************************************************************************/
/* record servrec declaration */
struct servrec_node {
	servrec	*next;			/* -> next */

	char	*name;			/* server name */
	int	port;			/* port */
	u_long	bytes_in;		/* bytes received */
	u_long	bytes_out;		/* bytes written */
	char	*motd;			/* the server's motd */
	time_t	first_activity;		/* time the server was started */
	time_t	connected;		/* how long it's been connected */
	time_t	lag_x_dur;		/* used to determine avg lag */
	time_t	p_dur;			/* total ping duration */
	time_t	p_starttime;		/* time started for that ping time */
	time_t	p_lasttime;		/* time started for last ping time */
	time_t	startlag;		/* start lag */
	time_t	lastlag;		/* last lag */
	int	num_servers;		/* how many servers we can see */
	char	*reason;		/* disconnect or 'unable' reason */
	time_t	disconnect_time;	/* disconnect time */
	u_long	flags;			/* server record flags */
} ;

/* record servrec low-level function declarations. */
servrec	*servrec_new();
void	servrec_freeall(servrec **list, servrec **last);
void	servrec_append(servrec **list, servrec **last, servrec *x);
void	servrec_delete(servrec **list, servrec **last, servrec *x);

/* record servrec low-level data-specific function declarations */
void	servrec_init(servrec *serv);
u_long	servrec_sizeof(servrec *s);
void	servrec_free(servrec *serv);
void	servrec_setname(servrec *serv, char *nick);
void	servrec_setmotd(servrec *serv, char *motd);
void	servrec_setreason(servrec *serv, char *reason);
void	servrec_display(servrec *serv);



/*****************************************************************************/
/* record squeue declaration */
struct squeue_node {
	squeue	*next;			/* -> next */

	char	*msg;			/* server name */
	char	flags;			/* server record flags */
} ;

/* record squeue low-level function declarations. */
squeue	*squeue_new();
void	squeue_freeall(squeue **list, squeue **last);
void	squeue_append(squeue **list, squeue **last, squeue *x);
void	squeue_insertatfront(squeue **list, squeue **last, squeue *x);
void	squeue_delete(squeue **list, squeue **last, squeue *x);

/* record squeue low-level data-specific function declarations */
void	squeue_init(squeue *q);
u_long	squeue_sizeof(squeue *q);
void	squeue_free(squeue *q);
void	squeue_setmsg(squeue *q, char *msg);



/*****************************************************************************/
/* server object flags */

#define SERVER_CONNECTED	0x0001	/* Server is connected. */
#define	SERVER_ACTIVITY		0x0002	/* Server has shown activity */
#define SERVER_ACTIVEIRC	0x0004	/* USER and NICK ok'd.  Active on IRC */
#define SERVER_DELETE_ON_DISCON	0x0008	/* server marked for delete */
#define SERVER_QUIT_IN_PROGRESS	0x0010	/* Server waiting for its QUIT */
#define SERVER_SHOW_REASONS	0x0020	/* flag to show reasons in output */
#define SERVER_WAITING_4_PONG	0x0040	/* waiting for pong */
#define SERVER_WAITING_4_LUSER	0x0080	/* waiting for pong */


/*****************************************************************************/
/* individual server flags */

#define SERV_CONNECT_ATTEMPTED	0x0001	/* Couldn't get active IRC on last try*/
#define SERV_CONNECT_FAILED	0x0002	/* Connect attempt failed. */
#define SERV_WAS_ACTIVEIRC	0x0004	/* Was active on irc on last try */
#define SERV_NEVER_TRIED	0x0008	/* Has never been tried */
#define SERV_BANNED		0x0010	/* Banned from this server */


/*****************************************************************************/
/* squeue flags */

#define PRIORITY_HIGH		0x0001	/* inserted at front of queue */
#define PRIORITY_NORM		0x0002	/* appended in queue format */

#endif /* SERVER_H */
