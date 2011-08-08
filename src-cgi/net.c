/******************************************************************************
 * Steve Rider - stever@area23.org
 * Grufti - an interactive, service-oriented bot for IRC 
 *
 * net.c - network i/o
 *
 *****************************************************************************/
/* 28 April 97 */

#include "../src/config.h"
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <limits.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#if HAVE_SYS_SELECT_H
#include <sys/select.h>
#endif
#include <sys/time.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <errno.h>
#if HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <fcntl.h>
#include "net.h"

char username[32]="";
char hostname[121]="";
u_int myip = 0;
char firewall[121]="";
int firewallport=178;

typedef struct {
	int	sock;
	char	flags;
	char	*inbuf;
	char	*outbuf;
} sock_list;

sock_list socklist[MAXSOCKS];

u_long net_sizeof()
{
	u_long	tot = 0L;
	int	i;

	tot += sizeof(username);
	tot += sizeof(hostname);
	tot += sizeof(myip);
	tot += sizeof(firewall);
	tot += sizeof(firewallport);

	for(i=0; i<MAXSOCKS; i++)
	{
		tot += sizeof(socklist[i]);
		tot += sizeof(socklist[i].sock);
		tot += sizeof(socklist[i].flags);
		tot += sizeof(socklist[i].inbuf);
		tot += socklist[i].inbuf ? strlen(socklist[i].inbuf)+1 : 0L;
		tot += sizeof(socklist[i].outbuf);
		tot += socklist[i].outbuf ? strlen(socklist[i].outbuf)+1 : 0L;
	}

	return tot;
}

u_long my_atoul(char *s)
{
	u_long ret = 0L;
	while((*s >= '0') && (*s <= '9'))
	{
		ret *= 10L;
		ret += *s - '0';
		s++;
	}

	return ret;
}

/*
 * starts a connection attempt to a socket.
 * If connection refused, returns:
 *   -1  neterror() type error,
 *   -2  can't resolve hostname
 */
int open_telnet_raw(int sock, char *server, int sport)
{
	struct sockaddr_in name;
	struct hostent *hp;
	int i, port, proxy = 0;
	char host[121];

	/* firewall?  use socks */
	if(firewall[0])
	{
		if(firewall[0] == '!')
		{
			proxy = PROXY_SUN;
			strcpy(host,&firewall[1]);
		}
		else
		{
			proxy = PROXY_SOCKS;
			strcpy(host,firewall);
		}
		port = firewallport;
	}
	else
	{
		strcpy(host,server);
		port = sport;
	}

	/* patch by tris for multi-hosted machines: */
	my_bzero((char *)&name,sizeof(struct sockaddr_in));
	name.sin_family = AF_INET;
	name.sin_addr.s_addr = (myip ? getmyip() : INADDR_ANY);
	if(bind(sock,(struct sockaddr *)&name,sizeof(name)) < 0)
	{
		killsock(sock);
		return -1;
	}
	my_bzero((char *)&name,sizeof(struct sockaddr_in));
	name.sin_family = AF_INET;
	name.sin_port = htons(port);

	/* numeric IP? */
	if((host[strlen(host)-1] >= '0') && (host[strlen(host)-1] <= '9'))
		name.sin_addr.s_addr = inet_addr(host);
	else
	{
		/* no, must be host.domain */
		alarm(10);
		hp = gethostbyname(host);
		alarm(0);
		if(hp == NULL)
		{
			killsock(sock);
			return -2;
		}
		my_memcpy((char *)&name.sin_addr,hp->h_addr,hp->h_length);
		name.sin_family = hp->h_addrtype;
	}
	for(i=0; i<MAXSOCKS; i++)
	{
		if(!(socklist[i].flags & SOCK_UNUSED) && (socklist[i].sock == sock))
		{
			socklist[i].flags |= SOCK_CONNECT;
		}
	}
	if(connect(sock,(struct sockaddr *)&name,sizeof(struct sockaddr_in)) < 0)
	{
		if(errno == EINPROGRESS)
		{
			/* firewall?  announce connect attempt to proxy */
			if(firewall[0])
				return proxy_connect(sock,server,sport,proxy);
			return sock;   /* async success! */
		}
		else
		{
			killsock(sock);
			return -1;
		}
	}
	/* synchronous? :/ */
	if(firewall[0])
		return proxy_connect(sock,server,sport,proxy);
	return sock;
}

int open_telnet(char *server, int port)
{
	return open_telnet_raw(getsock(0),server,port);
}

int getsock(int options)
{
	int sock = socket(AF_INET,SOCK_STREAM,0);
	if(sock < 0)
		fatal("socket call returned negative socket.",0);
	setsock(sock,options);
	return sock;
}

/* request a normal socket for i/o */
void setsock(int sock, int options)
{
	int i;
	int parm;
	for(i=0; i<MAXSOCKS; i++)
	{
		if(socklist[i].flags & SOCK_UNUSED)
		{
			/* yay!  there is table space */
			socklist[i].inbuf = NULL;
			socklist[i].outbuf = NULL;
			socklist[i].flags = options;
			socklist[i].sock = sock;
			if((sock != STDOUT) && !(socklist[i].flags & SOCK_NONSOCK))
			{
				parm = 1;
				setsockopt(sock,SOL_SOCKET,SO_KEEPALIVE,(void *)&parm,sizeof(int));
				parm = 0;
				setsockopt(sock,SOL_SOCKET,SO_LINGER,(void *)&parm,sizeof(int));
			}
			if(options & SOCK_LISTEN)
			{
				parm=1;
				setsockopt(sock,SOL_SOCKET,SO_REUSEADDR,(void *)&parm,sizeof(int));
			}
			/* yay async i/o ! */
			fcntl(sock,F_SETFL,O_NONBLOCK);
			return;
		}
	}

	/* We didn't get a socket. */
	fatal("No more sockets available.",0);
}

/* done with a socket */
void killsock(int sock)
{
	int i;
	for(i=0; i<MAXSOCKS; i++)
	{
		if(socklist[i].sock == sock)
		{
			close(socklist[i].sock);
			if (socklist[i].inbuf != NULL)
				free(socklist[i].inbuf);
			if (socklist[i].outbuf != NULL)
				free(socklist[i].outbuf);
			socklist[i].flags = SOCK_UNUSED;
			return;
		}
	}
}

/* get my ip number */
u_int getmyip()
{
	struct hostent *hp;
	char s[121];
	u_int ip;
	struct in_addr *in;

	/* could be pre-defined */
	if(myip)
		return myip;

	/* also could be pre-defined */
	if(hostname[0])
		hp = gethostbyname(hostname);
	else
	{
		gethostname(s,120);
		hp = gethostbyname(s);
	}
	if (hp == NULL)
	{
		return 0;
	}
	in = (struct in_addr *)(hp->h_addr_list[0]);
	ip = (u_int)(in->s_addr);
	return ip;
}

void my_memcpy(char *dest, char *src, int len)
{
	while (len--)
		*dest++ = *src++;
}

/* bzero() is bsd-only, so here's one for non-bsd systems */
void my_bzero(char *dest, int len)
{
	while (len--)
		*dest++ = 0;
}

/* send connection request to proxy */
int proxy_connect(int sock, char *host, int port, int proxy)
{
	u_char x[10];
	struct hostent *hp;
	char s[30];

	/* socks proxy */
	if (proxy == PROXY_SOCKS)
	{
		/* numeric IP? */
		if((host[strlen(host)-1] >= '0') && (host[strlen(host)-1] <= '9'))
		{
			u_int ip=(u_int)inet_addr(host);
			x[0] = (ip >> 24);
			x[1] = (ip >> 16) & 0xff;
			x[2] = (ip >> 8) & 0xff;
			x[3] = ip & 0xff;
		}
		else
		{
			/* no, must be host.domain */
			alarm(10);
			hp = gethostbyname(host);
			alarm(0);
			if(hp == NULL)
			{
				killsock(sock);
				return -2;
			}
			my_memcpy(x,(char *)hp->h_addr,hp->h_length);
		}
		sprintf(s,"\004\001%c%c%c%c%c%c%s",(port>>8) % 256,(port % 256),x[0],x[1],x[2],x[3],username);
		write(sock,s,strlen(s)+1);
	}
	else if(proxy == PROXY_SUN)
	{
		sprintf(s,"%s %d\n",host,port);
		write(sock,s,strlen(s));
	}
	return sock;
}

void init_net()
{
	int i;
	for(i=0; i<MAXSOCKS; i++)
		socklist[i].flags = SOCK_UNUSED;

	myip = getmyip();
	if(myip == 0)
		fatal("getmyip returned null hostentry.",0);
	
}

/* puts full hostname in s */
void getmyhostname(char *s)
{
	struct hostent *hp;
	char *p;

	if(hostname[0])
	{
		strcpy(s,hostname);
		return;
	}
	p = getenv("HOSTNAME");
	if(p != NULL)
	{
		strcpy(s,p);
		if(strchr(s,'.') != NULL)
			return;
	}
	gethostname(s,80);
	if(strchr(s,'.') != NULL)
		return;
	hp = gethostbyname(s);
	if(hp == NULL)
		fatal("Hostname self-lookup failed.",0);
	strcpy(s,hp->h_name);
	if(strchr(s,'.') != NULL)
		return;
	if(hp->h_aliases[0] == NULL)
		fatal("Can't determine your hostname!",0);
	strcpy(s,hp->h_aliases[0]);
	if(strchr(s,'.') == NULL)
		fatal("Can't determine your hostname!",0);
}

void neterror(char *s)
{
	switch(errno)
	{
	case EADDRINUSE: strcpy(s,"Address already in use"); break;
	case EADDRNOTAVAIL: strcpy(s,"Address invalid on remote machine"); break;
	case EAFNOSUPPORT: strcpy(s,"Address family not supported"); break;
	case EALREADY: strcpy(s,"Socket already in use"); break;
	case EBADF: strcpy(s,"Socket descriptor is bad"); break;
	case ECONNREFUSED: strcpy(s,"Connection refused"); break;
	case EFAULT: strcpy(s,"Namespace segment violation"); break;
	case EINPROGRESS: strcpy(s,"Operation in progress"); break;
	case EINTR: strcpy(s,"Timeout"); break;
	case EINVAL: strcpy(s,"Invalid namespace"); break;
	case EISCONN: strcpy(s,"Socket already connected"); break;
	case ENETUNREACH: strcpy(s,"Network unreachable"); break;
	case ENOTSOCK: strcpy(s,"File descriptor, not a socket"); break;
	case ETIMEDOUT: strcpy(s,"Connection timed out"); break;
	case ENOTCONN: strcpy(s,"Socket is not connected"); break;
	case EHOSTUNREACH: strcpy(s,"Host is unreachable"); break;
	case EPIPE: strcpy(s,"Broken pipe"); break;
#ifdef ECONNRESET
	case ECONNRESET: strcpy(s,"Connection reset by peer"); break;
#endif
#ifdef EACCES
	case EACCES: strcpy(s,"Permission denied"); break;
#endif
	case 0: strcpy(s,"Error 0"); break;
	default: sprintf(s,"Unforseen error %d",errno); break;
	}
}  

/*
 * given network-style IP address, return hostname.
 * hostname will be "##.##.##.##" format if there was an error.
 */
char *hostnamefromip(u_long ip)
{
	struct hostent *hp;
	u_long addr = ip;
	u_char *p;
	static char s[121];

	alarm(10);
	hp = gethostbyaddr((char *)&addr,sizeof(addr),AF_INET);
	alarm(0);
	if(hp == NULL)
	{
		p = (u_char *)&addr;
		sprintf(s,"%u.%u.%u.%u",p[0],p[1],p[2],p[3]);
		return s;
	}
	strcpy(s,hp->h_name);
	return s;
}

/*
 * returns a socket number for a listening socket that will accept any
 * connection -- port # is returned in port
 */
int open_listen(int *port)
{
	int sock, addrlen;
	struct sockaddr_in name;

	if(firewall[0])
	{
		/* FIXME: can't do listen port thru firewall yet */
		return -1;
	}

	sock = getsock(SOCK_LISTEN);
	my_bzero((char *)&name,sizeof(struct sockaddr_in));
	name.sin_family = AF_INET;
	name.sin_port = htons(*port);   /* 0 = just assign us a port */
	name.sin_addr.s_addr = (myip ? getmyip() : INADDR_ANY);

	if(bind(sock,(struct sockaddr *)&name,sizeof(name)) < 0)
	{
		killsock(sock);
		return -1;
	}

	/* what port are we on? */
	addrlen = sizeof(name);
	if(getsockname(sock,(struct sockaddr *)&name,&addrlen) < 0)
	{
		killsock(sock);
		return -1;
	}

	*port = ntohs(name.sin_port);
	if(listen(sock,1) < 0)
	{
		killsock(sock);
		return -1;
	}
	return sock;
}

/*
 * short routine to answer a connect received on a socket made previously
 * by open_listen ... returns hostname of the caller, new socket, port, and ip.
 * does NOT dispose of old "public" socket!
 */
int answer(int sock, int *port, char *caller, u_long *ip, int bin)
{
	int new_sock, addrlen;
	struct sockaddr_in from;

	addrlen = sizeof(struct sockaddr);
	new_sock = accept(sock,(struct sockaddr *)&from,&addrlen);
	if(new_sock < 0)
		return -1;
	*ip = from.sin_addr.s_addr;
	strcpy(caller,hostnamefromip(*ip));
	if(strcasecmp(caller,"localhost") == 0)
		getmyhostname(caller);
	*ip = ntohl(*ip);
	*port = ntohs(from.sin_port);

	/* set up all the normal socket crap */
	setsock(new_sock,(bin ? SOCK_BINARY : 0));
	return new_sock;
}
	

/* like open_telnet, but uses server & port specifications of dcc */
int open_telnet_dcc(int sock, u_long addr, int p)
{
	char sv[121];
	u_char c[4];

	if(addr < (1 << 24))
		return -3;  /* fake address */

	c[0] = (addr >> 24) & 0xff;
	c[1] = (addr >> 16) & 0xff;
	c[2] = (addr >> 8) & 0xff;
	c[3] = addr & 0xff;
	sprintf(sv,"%u.%u.%u.%u",c[0],c[1],c[2],c[3]);

	/* strcpy(sv,hostnamefromip(addr)); */
	p = open_telnet_raw(sock,sv,p);
	return p;
}

/*
 * attempts to read from all the sockets in socklist.
 *
 * fills 's' with up to 511 bytes if available.
 * returns:
 *   socket if data is available,
 *   -1 on EOF, with socket in len,
 *   -2 on socket error,
 *   -3 if nothing is ready
 */
int sockread(char *s, int *len)
{
	fd_set fd;
	int fds, i, x, grab = 511;
	struct timeval t;

	fds = getdtablesize();
#ifdef FD_SETSIZE
	if (fds > FD_SETSIZE)
		fds = FD_SETSIZE; /* fixes YET ANOTHER freebsd bug!!! */
#endif
	/* timeout: 1 sec */
	t.tv_sec = 1;
	t.tv_usec = 0;
	FD_ZERO(&fd);
	for(i=0; i<MAXSOCKS; i++)
		if(!(socklist[i].flags & SOCK_UNUSED))
		{
			if((socklist[i].sock == STDOUT))
				FD_SET(STDIN,&fd);
			else
				FD_SET(socklist[i].sock,&fd);
		}
#ifdef HPUX
	x = select(fds,(int *)&fd,(int *)NULL,(int *)NULL,&t);
#else
	x = select(fds,&fd,NULL,NULL,&t); 
#endif
	if(x > 0)
	{
		/* something happened */
		for (i=0; i<MAXSOCKS; i++)
		{
			if((!(socklist[i].flags & SOCK_UNUSED)) && ((FD_ISSET(socklist[i].sock,&fd)) || ((socklist[i].sock == STDOUT) && (FD_ISSET(STDIN,&fd)))))
			{
				if(socklist[i].flags & (SOCK_LISTEN|SOCK_CONNECT))
				{
					/*
					 * listening socket -- don't read,
					 * just return activity.  same for
					 * connection attempt.
					 */
					if(!(socklist[i].flags & SOCK_STRONGCONN))
					{
						s[0] = 0;
						*len = 0;
						return i;
					}
					/*
					 * (for strong connections, require
					 * a read to succeed first)
					 */
					if((firewall[0]) && (firewall[0] != '!') && (socklist[i].flags & SOCK_CONNECT))
					{
						/*
						 * hang around to get the
						 * return code from proxy
					 	 */
						grab = 8;
					}
				}
				if((socklist[i].sock == STDOUT))
					x = read(STDIN,s,grab);
				else
					x = read(socklist[i].sock,s,grab);
				if(x <= 0)
				{
					/* eof */
					*len = socklist[i].sock;
					socklist[i].flags &= ~SOCK_CONNECT;
					return -1;
				}
				s[x] = 0;
				*len = x;
				if((firewall[0]) && (socklist[i].flags & SOCK_CONNECT))
				{
					switch(s[1])
					{
					/* success */
					case 90: s[0] = 0; *len = 0; return i;

					/* failed */
					case 91: errno = ECONNREFUSED; break;

					/* no identd */
					case 92:

					/* identd said wrong username */
					case 93: errno = ENETUNREACH; break;
					/*
					 * a better error message would be
					 * "socks misconfigured" or "identd
					 * not working" but this is simplest.
					 */
					}

					*len = socklist[i].sock;
					socklist[i].flags &= ~SOCK_CONNECT;
					return -1;
				}
				return i;
			}
		}
	}
	else if(x == -1)
		return -2;   /* socket error */
	else
	{
		s[0] = 0;
		*len = 0;
	}
	return -3;
}

/*
 * sockgets: buffer and read from sockets
 *
 * - attempts to read from all registered sockets for up to one second.  if
 *   after one second, no complete data has been received from any of the
 *   sockets, 's' will be empty, 'len' will be 0, and sockgets will return
 *   -3.
 *
 * - if there is returnable data received from a socket, the data will be
 *   in 's' (null-terminated if non-binary), the length will be returned
 *   in len, and the socket number will be returned.
 *
 * - normal sockets have their input buffered, and each call to sockgets
 *   will return one line terminated with a '\n'.  binary sockets are not
 *   buffered and return whatever comes in as soon as it arrives.
 *
 * - listening sockets will return an empty string when a connection comes in.
 *   connecting sockets will return an empty string on a successful connect,
 *   or EOF on a failed connect.
 *
 * - if an EOF is detected from any of the sockets, that socket number will be
 *   put in len, and -1 will be returned.
 *
 * - the maximum length of the string returned is 512 (including null).
*/

int sockgets(char *s, int *len)
{
	char xx[514], *p, *px;
	int ret, i;

	/* check for stored-up data waiting to be processed */
	for(i=0; i<MAXSOCKS; i++)
	{
		if(!(socklist[i].flags & SOCK_UNUSED) && (socklist[i].inbuf != NULL))
		{
			/* look for \r too cos windows can't follow RFCs */
			p = strchr(socklist[i].inbuf,'\n');
			if(p == NULL)
				p = strchr(socklist[i].inbuf,'\r');
			if(p != NULL)
			{
				*p = 0;
				if(strlen(socklist[i].inbuf) > 510)
					socklist[i].inbuf[510] = 0;
				strcpy(s,socklist[i].inbuf);
				px = (char *)malloc(strlen(p+1)+1);
				strcpy(px,p+1);
				free(socklist[i].inbuf);
				if(px[0])
					socklist[i].inbuf = px;
				else
				{
					free(px);
					socklist[i].inbuf = NULL;
				}

				/* strip CR if this was CR/LF combo */
				if(s[strlen(s)-1] == '\r')
					s[strlen(s)-1] = 0;
				*len = strlen(s);
				return socklist[i].sock;
			}
		}
	}

	/* no pent-up data of any worth -- down to business */
	*len = 0;
	ret = sockread(xx,len);
	if(ret < 0)
	{
		s[0] = 0;
		return ret;
	}

	/* binary and listening sockets don't get buffered */
	if(socklist[ret].flags & SOCK_CONNECT)
	{
		if(socklist[ret].flags & SOCK_STRONGCONN)
		{
			socklist[ret].flags &= ~SOCK_STRONGCONN;
			/* buffer any data that came in, for future read */
			socklist[ret].inbuf = (char *)malloc(strlen(xx)+1);
			strcpy(socklist[ret].inbuf,xx);
		}
		socklist[ret].flags &= ~SOCK_CONNECT;
		s[0] = 0;
		return socklist[ret].sock;
	}
	if(socklist[ret].flags & SOCK_BINARY)
	{
		my_memcpy(s,xx,*len);
		return socklist[ret].sock;
	}
	if(socklist[ret].flags & SOCK_LISTEN)
		return socklist[ret].sock;

	/* might be necessary to prepend stored-up data! */
	if(socklist[ret].inbuf != NULL)
	{
		p = socklist[ret].inbuf;
		socklist[ret].inbuf = (char *)malloc(strlen(p) + strlen(xx)+1);
		strcpy(socklist[ret].inbuf,p);
		strcat(socklist[ret].inbuf,xx);
		free(p);
		if(strlen(socklist[ret].inbuf) < 512)
		{
			strcpy(xx,socklist[ret].inbuf);
			free(socklist[ret].inbuf);
			socklist[ret].inbuf = NULL;
		}
		else
		{
			p = socklist[ret].inbuf;
			socklist[ret].inbuf = (char *)malloc(strlen(p)-509);
			strcpy(socklist[ret].inbuf,p+510);
			*(p+510) = 0;
			strcpy(xx,p);
			free(p);
			/* (leave the rest to be post-pended later) */
		}
	}

	/* look for EOL marker; if it's there, i have something to show */
	p = strchr(xx,'\n');
	if(p == NULL)
		p = strchr(xx,'\r');
	if(p != NULL)
	{
		*p = 0;
		strcpy(s,xx);
		strcpy(xx,p+1);
		if(s[strlen(s)-1] == '\r')
			s[strlen(s)-1] = 0;
/* No!
		if(!s[0])
			strcpy(s," ");
*/
	}
	else
	{
		s[0] = 0;
		if(strlen(xx) >= 510)
		{
			/* string is too long, so just insert fake \n */
			strcpy(s,xx);
			xx[0] = 0;
		}
	}

	*len = strlen(s);
	/* anything left that needs to be saved? */
	if(!xx[0])
	{
		if(s[0])
			return socklist[ret].sock;
		else
			return -3;
	}

	/* prepend old data back */
	if(socklist[ret].inbuf != NULL)
	{
		p = socklist[ret].inbuf;
		socklist[ret].inbuf = (char *)malloc(strlen(p) + strlen(xx)+1);
		strcpy(socklist[ret].inbuf,xx);
		strcat(socklist[ret].inbuf,p);
		free(p);
	}
	else
	{
		socklist[ret].inbuf = (char *)malloc(strlen(xx)+1);
		strcpy(socklist[ret].inbuf,xx);
	}

	if(s[0])
		return socklist[ret].sock;
	else
		return -3;
}

/* dump something to a socket */
void tputs(int z, char *s)
{
	int i,x;
	char *p;

	if(z < 0)
		return;   /* um... HELLO?!  sanity check please! */
	if(((z == STDOUT) || (z == STDERR)))
	{
		write(z,s,strlen(s));
		return;
	}
	for(i=0; i<MAXSOCKS; i++)
	{
		if(!(socklist[i].flags & SOCK_UNUSED) && (socklist[i].sock==z))
		{
			if(socklist[i].outbuf != NULL)
			{
				/* already queueing: just add it */
				p = (char *)malloc(strlen(socklist[i].outbuf) + strlen(s)+1);
				strcpy(p,socklist[i].outbuf);
				strcat(p,s);
				free(socklist[i].outbuf);
				socklist[i].outbuf = p;
				return;
			}
			x = write(z,s,strlen(s));
			if(x == (-1))
				x = 0;
			if(x < strlen(s))
			{
				/* socket is full, queue it */
				socklist[i].outbuf = (char *)malloc(strlen(s) - x+1);
				strcpy(socklist[i].outbuf,&s[x]);
			}
			return;
		}
	}
	printf("    '%s'\n",s);
}

/* tputs might queue data for sockets, let's dump as much of it as possible */
void dequeue_sockets()
{
	int i;
	char *p;

	for(i=0; i<MAXSOCKS; i++)
	{
		if(!(socklist[i].flags & SOCK_UNUSED) && (socklist[i].outbuf != NULL))
		{
			/* trick tputs into doing the work */
			p = socklist[i].outbuf;
			socklist[i].outbuf = NULL;
			tputs(socklist[i].sock,p);
			free(p);
		}
	}
}

/* like fprintf, but instead of preceding the format string with a FILE
   pointer, precede with a socket number */
void tprintf(int sock, char *format, ...)
{
	va_list va;
	static char SBUF2[768];

	va_start(va,format);
	vsprintf(SBUF2,format,va);
	va_end(va);

	if(strlen(SBUF2) > 510)
		SBUF2[510] = 0;   /* server can only take so much */
	tputs(sock,SBUF2);
}
