/* -------------------------------------------------------------------------
 * Steve Rider - stever@area23.org
 * Grufti - an interactive, service-oriented bot for IRC 
 *
 * net.h - general-purpose buffered network I/O
 *
 * ------------------------------------------------------------------------- */
/* 22 June 98 */

#ifndef NET_H
#define NET_H

#define MAXNETERRLENGTH 80

/* init */
int net_init(char *our_hostname);

/* virtual hosting support */
int enable_virtual_host(char *virtual_host);

/* connect, listen, disconnect, send, read */
int net_connect(char *host, u_short port, int binary, int strong);
void net_disconnect(int sock);
int net_listen(u_short *port);
int net_accept(int sock, char *fromhost, IP *fromip, u_short *port, int binary);
int net_sockgets(char *buf, int *len);
int net_sockputs(int sock, char *buf, int len);
void net_dequeue();

/* error reporting and debugging */
char *net_errstr();
void net_dump_socktbl();

/* miscelleana */
u_long net_sizeof();
char *iptodn(IP ip);
char *iptohostname(IP ip);
IP hostnametoip(char *hostname);
IP atoip(char *ip);

/* local info */
char *myhostname();
IP myip();

#endif /* NET_H */
