/******************************************************************************
 * Steve Rider - stever@area23.org
 * Grufti - an interactive, service-oriented bot for IRC 
 *
 * dcc.h - DCC object declarations
 *
 *****************************************************************************/
/* 28 April 97 */

#ifndef DCC_H
#define DCC_H

#include <stdio.h>
#include "typedefs.h"
#include "grufti.h"

/* Let all files who need DCC have access to the DCC object */
extern struct DCC_object *DCC;


/*****************************************************************************/
/* object DCC declaration */
typedef struct DCC_object {
	dccrec	*dcclist;		/* -> list of active DCC connections */
	dccrec	*last;			/* -> the last dcc record */

        int	num_dcc;		/* number of dcc records */
	int	bytes_in;		/* bytes recd from the device */
	int	bytes_out;		/* bytes sent over the device */

	/* send buffer */
	char	send_buf[SERVERLEN];	/* send buffer */
	char	send_buf1[SERVERLEN];	/* send buffer */

	/* misc buffers */
	char	s1[BUFSIZ];		/* misc buffer 1 */
	char	s2[BUFSIZ];		/* misc buffer 2 */

} DCC_ob;

/* object DCC low-level function declarations. */
void	DCC_new();
u_long	DCC_sizeof();
void	DCC_freeall();

/* object DCC data-specific function declarations. */
void	DCC_display();

/* low-level DCC structure function declarations. */
void	DCC_appenddcc(dccrec *d);
void	DCC_deletedcc(dccrec *d);
void	DCC_freealldccs();

/* object DCC high-level implementation-specific function declarations. */
dccrec	*DCC_dccbysock(int socket);
dccrec	*DCC_addnewconnection(char *nick, char *uh, u_long ip, int socket, u_short port, u_long type);
void	DCC_disconnect(dccrec *x);
void	DCC_gotactivity(int sock, char *buf, int len);
void	DCC_gotidentactivity(dccrec *id, char *buf);
void	DCC_gotircchatactivity(dccrec *d, char *buf);
void	DCC_gottelnetchatactivity(dccrec *d, char *buf);
void	DCC_gotxferactivity(dccrec *d, char *buf, int len);
void	DCC_gotxferactivity_in(dccrec *d, char *buf, int len);
void	DCC_gotxferactivity_out(dccrec *d, char *buf, int len);
void	DCC_gottinygruftiactivity(dccrec *d, char *buf);
void	DCC_gotsmtpactivity(dccrec *d, char *buf, int len);
void	DCC_gotsmtp220(dccrec *d, char *buf, int len);
void	DCC_gotsmtp250(dccrec *d, char *buf, int len);
void	DCC_gotsmtp354(dccrec *d, char *buf, int len);
void	DCC_gottelnetconnection(dccrec *telnet_serv);
void	DCC_gotinternalconnection(dccrec *gw_serv);
void	DCC_gotircchatconnection(dccrec *ircchat_serv);
void	DCC_answergenericserver(dccrec *d);
void	DCC_gotxferconnection(dccrec *xfer_serv);
void	DCC_gotEOF(int sock);
void	DCC_dologin(dccrec *d);
int	DCC_gotlogin(dccrec *d, char *nick);
void	DCC_gotid(dccrec *d, char *buf);
void	DCC_dopassword(dccrec *d);
void	DCC_gotpassword(dccrec *d, char *password);
void	DCC_dosuccessfullogin(dccrec *d);
void	DCC_docommand(dccrec *d, char *buf);
void	DCC_write(dccrec *d, char *format, ...);
void	DCC_write_nolog(dccrec *d, char *format, ...);
void	DCC_write_nocr(dccrec *d, char *format, ...);
void	DCC_flags2str(u_long flags, char *s);
void	DCC_checkfortimeout();
dccrec	*DCC_sendchat(char *nick, char *uh);
int	DCC_isuhindcc(char *uh);
void	DCC_resetallpointers();
void	DCC_showprompt(dccrec *d);
void	DCC_strip_telnet(dccrec *d, char *buf, int *len);
void	send_to_user(dccrec *d, char *nick, char *format, ...);
void	DCC_opentelnetservers();
dccrec	*DCC_dccserverbyname(char *name);
void	DCC_ondccchat(dccrec *d);
int	DCC_isnickindcc(char *nick);
void	DCC_bootuser(dccrec *d, char *reason);
void	DCC_bootallusers(char *reason);
int	DCC_orderbystarttime();
dccrec	*DCC_dccbyordernum();
void	DCC_clearorder();
void	DCC_sendfile(dccrec *d, char *nick, char *uh, char *file);
int	DCC_forwardnotes(userrec *u, char *forward);


/* ASDF */
/*****************************************************************************/
/* record dccrec declaration */
struct dccrec_node {
	struct dccrec_node *next;	/* -> next */

	int	socket;			/* socket */
	u_long	ip;			/* ip address */
	u_short	port;			/* connection port */
	u_long	flags;			/* type of connection */
	char	*nick;			/* nick */
	char	*uh;			/* userhost */
	userrec	*user;			/* -> to user */
	acctrec	*account;		/* -> to account */
	time_t	starttime;		/* time connection was made */
	time_t	last;			/* last time seen (via dcc) */
	int	bytes_in;		/* bytes recvd from this socket */
	int	bytes_out;		/* bytes sent to this socket */
	int	login_attempts;		/* number of login attempts */
	int	order;			/* used for ordering lists */
	int	logtype;		/* used for watching logs */
	char	*logchname;		/* channelname for log watching */

	/* chat stuff */
	int	msgs_per_sec;		/* used to check for flooding in chat */

	/* xfer stuff */
	char	*filename;		/* xfer filename */
	u_long	length;			/* length of file */
	u_long	sent;			/* how much has been sent */
	char	*buf;
	u_char	sofar;
	time_t	pending;
	char	*from;			/* who incoming file is from */
	FILE	*f;			/* FILE pointer */

	/* ident client stuff */
	dccrec *id;			/* pointer to user we're checking */

	/* command pending */
	char	*command;		/* When a command is pending */
	char	*commandfromnick;
	time_t	commandtime;

	/* tinygrufti stuff */
	char	tinygrufti_header[40];	/* header that goes back to client */

	/* inter-process commmunication */
	u_long ipc;
} ;

/* record dccrec low-level function declarations. */
dccrec	*dccrec_new();
void	dccrec_freeall(dccrec **dcclist, dccrec **last);
void	dccrec_append(dccrec **list, dccrec **last, dccrec *entry);
void	dccrec_delete(dccrec **list, dccrec **last, dccrec *x);
void	dccrec_movetofront(dccrec **list, dccrec **last, dccrec *lastchecked, dccrec *x);

/* record dccrec low-level data-specific function declarations. */
void	dccrec_init(dccrec *d);
u_long	dccrec_sizeof(dccrec *d);
void	dccrec_free(dccrec *d);
void	dccrec_setnick(dccrec *d, char *nick);
void	dccrec_setuh(dccrec *d, char *uh);
void	dccrec_setfilename(dccrec *d, char *filename);
void	dccrec_setfrom(dccrec *d, char *from);
void	dccrec_setcommand(dccrec *d, char *command);
void	dccrec_setcommandfromnick(dccrec *d, char *commandfromnick);
void	dccrec_setlogchname(dccrec *d, char *logchname);
void	dccrec_display(dccrec *d);
dccrec	*dccrec_dcc(dccrec **list, dccrec **last, int socket);



/*****************************************************************************/
/* DCC flags */
#define DCC_SERVER		0x00000001	/* Type server */
#define DCC_CLIENT		0x00000002	/* Type client */

#define SERVER_PUBLIC		0x00000004	/* Public telnet server */
#define SERVER_INTERNAL		0x00000008	/* Internal server*/
#define SERVER_IRC_CHAT		0x00000010	/* Waiting for DCCCHAT via irc*/
#define SERVER_XFER		0x00000020	/* Server waiting for xfer */

#define CLIENT_IDENT		0x00000040	/* Ident client */
#define CLIENT_IRC_CHAT		0x00000080	/* Chat client via IRC */
#define CLIENT_TELNET_CHAT	0x00000100	/* Chat client via telnet */
#define CLIENT_XFER		0x00000200	/* Xfer client */
#define CLIENT_SMTP		0x00000400

#define STAT_LOGIN		0x00000800	/* Chat client doing login */
#define STAT_PASSWORD		0x00001000	/* Chat client doing pass */
#define STAT_PENDING_IDENT	0x00002000	/* Chat client pending auth */
#define STAT_PENDING_LOGIN	0x00004000	/* Waiting for login */

#define STAT_COMMAND_PENDING	0x00008000	/* A command is pending */
#define STAT_COMMAND_SETUP	0x00010000	/* Involved in setup */
#define STAT_COMMAND_REGISTER	0x00020000	/* Completing registration */
#define STAT_DISCONNECT_ME	0x00040000	/* Signal we should close. */
#define STAT_VERIFIED		0x00080000	/* User is verified */
#define STAT_GATEWAYCLIENT	0x00100000	/* client is from gateway */
#define STAT_TINYGRUFTI		0x00200000
#define STAT_XFER_IN		0x00400000	/* xfer being received */
#define STAT_XFER_OUT		0x00800000	/* xfer going out */
#define STAT_WAITINGFORID	0x01000000	/* waiting for gateway id */
#define STAT_WEBCLIENT		0x02000000	/* client is coming from web */
#define STAT_DONTSHOWPROMPT	0x04000000	/* for web clients */

#define STAT_SMTP_MAILFROM	0x08000000
#define STAT_SMTP_RCPTTO	0x10000000
#define STAT_SMTP_DATA		0x20000000
#define STAT_SMTP_QUIT		0x40000000



/*****************************************************************************/

#endif /* DCC_H */
