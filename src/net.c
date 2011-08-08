/* -------------------------------------------------------------------------
 * Steve Rider - stever@area23.org
 * Grufti - an interactive, service-oriented bot for IRC 
 *
 * net.c - general-purpose buffered network I/O
 *
 * DEBUG_STRICT rigorously verifies socket table integrity.  Expensive,
 * should only be used during testing.
 *
 * Compile with -DTEST to test this module.
 *
 * v1.1-compliant 15 July 98
 * ------------------------------------------------------------------------- */
/* 22 June 98 */

#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#ifdef WIN32
#include <windows.h>
#else
#include <sys/socket.h>
#include <netinet/in.h>
#ifndef INADDR_NONE
#define INADDR_NONE  0xffffffff  /* should be in <netinet/in.h> */
#endif
#include <arpa/inet.h>
#include <netdb.h>
#endif
#if HAVE_SYS_SELECT_H
#include <sys/select.h>
#endif
#include <fcntl.h>
#include "grufti.h"
#include "net.h"

#ifdef TEST
#define DEBUG_STRICT

/* Normally defined in grufti.c */
#define fatal_error debug_to_stderr
#define debug debug_to_stderr
#define warning debug_to_stderr
#define internal debug_to_stderr
static void debug_to_stderr(char *fmt, ...);

/* Normally defined in xmem.c (#defined in misc.h) */
#undef xmalloc
#define xmalloc malloc
#undef xfree
#define xfree free
#undef xrealloc
#define xrealloc realloc

/* Normally defined in misc.c */
#define xmemcpy memcpy

#endif /* TEST */

/* Windows uses WSA base for error codes */
#ifdef WIN32
#define EADDRINUSE     WSAEADDRINUSE
#define EADDRNOTAVAIL  WSAEADDRNOTAVAIL
#define EAFNOSUPPORT   WSAEAFNOSUPPORT
#define EALREADY       WSAEALREADY
#define ECONNREFUSED   WSAECONNREFUSED
#define EINPROGRESS    WSAEINPROGRESS
#define EISCONN        WSAEISCONN
#define ENETUNREACH    WSAENETUNREACH
#define ENOTSOCK       WSAENOTSOCK
#define ETIMEDOUT      WSAETIMEDOUT
#define ENOTCONN       WSAENOTCONN
#define EHOSTUNREACH   WSAEHOSTUNREACH
#endif


/* Prototypes for internal commands */
static int new_socket(char flags);
static void add_socket(int sock, char flags);
static int find_socket(int sock);
static void destroy_socket(int sock);
static void set_socket(int sock, int opt, int val);
static int sockread(char *buf, int *len);
static int bind_virtual_host(int sock);
static int net_errno();
static int getmyhostname(char *s);
static IP getmyip(int sock);

/*
 * Virtual hosting support - available to the world
 */
struct {
	char vh_name[MAXHOSTLENGTH];
	char vh_official[MAXHOSTLENGTH];
	int vh_enabled;
	struct sockaddr_in vh_sin;
} vh;

/*
 * Local stuff - available via myhostname()
 * When unset, this module will automatically retrieve it on a call
 * to myhostname().  This way, if our network connection has changed,
 * we just clear the name field and let the net module get it again.
 */
static struct {
	char name[MAXHOSTLENGTH];
	char vh_name[MAXHOSTLENGTH];
	int perm;	/* if hostname is explicitly set */
	int use_vh;
} local_host;


/* socket table (a la Eggdrop) */
#define NET_MAXSOCKS 100

#define SOCK_UNUSED        0x01 /* empty socket */
#define SOCK_BINARY        0x02 /* do not buffer input */
#define SOCK_LISTEN        0x04 /* listening port */
#define SOCK_CONNECTING    0x08 /* connection attempt */
#define SOCK_NONSOCK       0x10 /* used for file i/o on debug (not used) */
#define SOCK_STRONG        0x20 /* don't report success until sure */
#define SOCK_EOFD          0x40 /* socket EOF'd recently during a write */

static struct {
	int sock;
	char *inbuf;
	char *outbuf;
	int outbuflen;	/* outbuf could be binary data */
	char flags;
} socktbl[NET_MAXSOCKS];


static int new_socket(char flags)
/*
 * Function: get a socket and add it to the socktbl
 *  Returns: sock if OK
 *           -1   if failed
 *
 *  Socket flags (#defined above) specify how activity on a socket is handled.
 */
{
	int sock;
	
	sock = socket(AF_INET, SOCK_STREAM, 0);
	if(sock < 0)
	{
		internal("net: in priv new_socket(), unable to get a socket.");
		return -1;
	}

	add_socket(sock, flags);

	/* non-blocking I/O used for connect() and write() */
#ifndef WIN32
	fcntl(sock, F_SETFL, O_NONBLOCK);
#endif

	return sock;
}


static void add_socket(int sock, char flags)
/*
 * Function: Add socket to socktbl, with flags
 *  Returns:  0 if OK
 *           -1 if socktbl is full
 */
{
	int i, pos = -1;

#ifdef DEBUG_STRICT
	if(find_socket(sock) != -1)
		internal("net: in priv add_socket(), duplicate sock entry! (sock=%d)",
			sock);
#endif

	for(i = 0; (i < NET_MAXSOCKS) && (pos == -1); i++)
		if(socktbl[i].flags & SOCK_UNUSED)
			pos = i;

	if(pos == -1)
	{
		internal("net: in priv add_socket(), sock table is full.");
		return;
	}

	socktbl[pos].sock = sock;
	socktbl[pos].inbuf = NULL;
	socktbl[pos].outbuf = NULL;
	socktbl[pos].outbuflen = 0;
	socktbl[pos].flags = flags;
}


static int find_socket(int sock)
/*
 * Function: Finds a socket in the socktbl
 *  Returns: pos if found
 *           -1  if not found
 */
{
	int i;
	for(i = 0; i < NET_MAXSOCKS; i++)
		if(!(socktbl[i].flags & SOCK_UNUSED) && socktbl[i].sock == sock)
			return i;

	return -1;
}


static void destroy_socket(int sock)
/*
 * Function: close and delete a socket from the socktbl
 *
 */
{
	int pos;

	pos = find_socket(sock);
	if(pos == -1)
	{
		internal("net: in priv destroy_socket(), socket not found. (sock=%d)",
			sock);
		return;
	}

#ifdef WIN32
	closesocket(socktbl[pos].sock);
#else
	close(socktbl[pos].sock);
#endif

	if(socktbl[pos].inbuf != NULL)
		xfree(socktbl[pos].inbuf);
	if(socktbl[pos].outbuf != NULL)
		xfree(socktbl[pos].outbuf);
	socktbl[pos].flags |= SOCK_UNUSED;
}


static void set_socket(int sock, int opt, int val)
{
	setsockopt(sock, SOL_SOCKET, opt, (void *)&val, sizeof(val));
}


static int sockread(char *buf, int *len)
/*
 * Function: read from all sockets in socktbl
 *  Returns: sock if data ready (data --> buf, len)
 *           -1   on EOF (socket returned in len)
 *           -2   if read() error
 *           -3   if no data ready
 *           -4   if select() failed
 *
 *  Sits on a read request for 1 full second.  If no data is ready, -3
 *  is returned.  Data returned here is not buffered, meaning it's returned
 *  as soon as it is received.  net_sockgets() is responsible for buffering
 *  read data.
 */
{
	fd_set readfds;
	int fdssize, i, x;
	struct timeval tv;

#ifdef FD_SETSIZE
	fdssize = FD_SETSIZE;
#else
#ifdef HAVE_GETDTABLESIZE
	fdssize = getdtablesize();
#else
	fdssize = 200;
#endif
#endif

	/* timeout: 1 second */
	tv.tv_sec = 1;
	tv.tv_usec = 0;

	buf[0] = 0;
	*len = 0;

	FD_ZERO(&readfds);

	/* Add all sockets to the file descriptor set */
	for(i = 0; i < NET_MAXSOCKS; i++)
		if(!(socktbl[i].flags & SOCK_UNUSED))
			FD_SET(socktbl[i].sock, &readfds);

	if((x = select(fdssize, &readfds, NULL, NULL, &tv)) == 0)
		return -3; /* nothing ready */

	if(x == -1)
	{
		/* Caught a signal, bail out */
		if(net_errno() == EINTR)
			return -3;

		debug("net: in priv sockread(), select() failed: %s", net_errstr());
		return -4;
	}

	/*
	 * Something's ready...
	 */
	for(i = 0; i < NET_MAXSOCKS; i++)
	{
		if(socktbl[i].flags & SOCK_UNUSED)
			continue;

		if(FD_ISSET(socktbl[i].sock, &readfds))
		{
			if(socktbl[i].flags & (SOCK_LISTEN|SOCK_CONNECTING))
			{
				/*
				 * Show that there was activity on listening
				 * sockets.  Same goes for connection attempts.
				 *
				 * (STRONG requires a successful read first)
				 */
				if(!(socktbl[i].flags & SOCK_STRONG))
					return i;
			}

#ifdef WIN32
			x = recv(socktbl[i].sock, buf, MAXREADBUFLENGTH - 1, 0);
#else
			x = read(socktbl[i].sock, buf, MAXREADBUFLENGTH - 1);
#endif
			if(x <= 0)
			{
				buf[0] = 0;
				*len = 0;

				/* non-blocking I/O */
				if(x == -1 && net_errno() == EAGAIN)
					return -3;

				/* caught a signal during read, bail out */
				if(x == -1 && net_errno() == EINTR)
					return -3;

				*len = socktbl[i].sock;
				socktbl[i].flags &= ~SOCK_CONNECTING;

				if(x == -1 && net_errno() == ECONNREFUSED)
				{
					internal("EOF was from ECONNREFUSED.");
					return -1;  /* EOF */
				}

#ifdef ECONNRESET
				if(x == -1 && net_errno() == ECONNRESET)
				{
					internal("EOF was from ECONNRESET.");
					return -1;  /* EOF */
				}
#endif

				if(x == 0)
					return -1;  /* EOF */

				debug("net: in priv sockread(), read() failed: %s (sock=%d)",
					net_errstr(), socktbl[i].sock);

				return -2;  /* Socket read error */
			}

			/*
			 * read() was OK.  Return sock, data, and length
			 */
			*len = x;
			buf[*len] = 0;

			return i;
		}
	}

	internal("net: in sockread(), unable to find data indicated by select().");
	return -3;
}


static int bind_virtual_host(int sock)
/*
 * Function: bind virtual host
 *  Returns:  0 if OK
 *           -1 if didn't bind
 */
{
	if(vh.vh_enabled == 0)
		return -1;

	/* Address already setup for us by enable_virtual_host() */
	if(bind(sock, (struct sockaddr *)&vh.vh_sin, sizeof(vh.vh_sin)) < 0)
	{
		debug("net: in priv bind_virtual_host(), bind() failed: %s",
			net_errstr());
		return -1;
	}

	return 0;
}


static int net_errno()
{
#ifdef WIN32
	int x = WSAGetLastError();
#else
	int x = errno;
#endif

	return x;
}


static int getmyhostname(char *s)
/*
 * Function: Determine our hostname
 *  Returns:  0 if OK
 *           -1 if failed
 *
 *  Blindly determines hostname.  We use this when initializing
 *  and also when we think the hostname on the machine has changed.
 */
{
	struct hostent *hp;

	gethostname(s, MAXHOSTLENGTH-1);
	hp = gethostbyname(s);
	if(hp == NULL || (hp->h_length != 4 && hp->h_length != 8))
	{
		/*
		 * If self-lookup fails, the machine isn't configured right.
		 * Even a machine not connected to the net should still be
		 * aliased to the loopback device.
		 */
		strcpy(s, "localhost");
		hp = gethostbyname(s);
		if(hp == NULL || (hp->h_length != 4 && hp->h_length != 8))
			return -1;

		/*
		 * Allow routine below to get offical name of loopback device
		 * if there is one.  This should have been the name we got
		 * in the first place!
		 */
	}

	/* Get official name */
	strcpy(s, hp->h_name);

	return 0;
}


static IP getmyip(int sock)
/*
 * Function: Find our ip.
 *  Returns: ip if OK
 *           -1 if lookup failed
 * 
 *  Returns ip in network byte order.
 *
 *  If a sock number is specified, getsockbyname() is used to get the
 *  IP address associated with that socket.
 */
{
	IP ip;
	char s[MAXHOSTLENGTH];

	/* getsockname() method? */
	if(sock != -1)
	{
		struct sockaddr_in localaddr;
		int address_len;

		address_len = sizeof(struct sockaddr_in);
		if(getsockname(sock, (struct sockaddr *) &localaddr, &address_len) < 0)
			return -1;

		if(localaddr.sin_addr.s_addr == 0)
			return -1;

		return localaddr.sin_addr.s_addr;
	}

	/* old-fashioned way. */
	if(!local_host.name[0])
	{
		warning("net: in getmyip(), our hostname not set yet.");
		gethostname(s, MAXHOSTLENGTH-1);
	}
	else
		strcpy(s, local_host.name);

	ip = hostnametoip(s);
	if(ip == 0)
		return -1;

	return ip;
}

	


/* -------------------------------------------------------------------------
 * PUBLIC
 *
 * net_init() must always be called first.  If our hostname has been
 * explicitly declared, we can pass it in as an argument which will tell
 * net_init() to hardcode it in.  Otherwise, set to NULL.
 *
 * ------------------------------------------------------------------------- */


int net_init(char *our_hostname)
/*
 * Function: Initialize connection stuff
 *  Returns:  0 if OK
 *           -1 if initialization failed.
 *
 *  Setup structs, clear sock table, look up our hostname and copy it
 *  to local_host struct.  If our_hostname is specified, use that.
 *
 *  Must be called before any network activity can begin.
 */
{
	int i;
	struct hostent *hp;
#ifdef WIN32
	WSADATA wsaData;

	WSAStartup(0x101, &wsaData);
#endif

	/* Clear virtual host struct */
	vh.vh_name[0] = 0;
	vh.vh_official[0] = 0;
	vh.vh_enabled = 0;

	/* Clear local_host struct */
	local_host.name[0] = 0;
	local_host.vh_name[0] = 0;
	local_host.perm = 0;
	local_host.use_vh = 0;

	/* Initialize socktbl */
	for(i = 0; i < NET_MAXSOCKS; i++)
	{
		socktbl[i].flags = SOCK_UNUSED;
		socktbl[i].inbuf = NULL;
		socktbl[i].outbuf = NULL;
	}

	/* Hostname explicitly declared.  */
	if(our_hostname != NULL && our_hostname[0])
	{
		/* Let's check it just to be sure. */
		hp = gethostbyname(our_hostname);
		if(hp == NULL || (hp->h_length != 4 && hp->h_length != 8))
		{
			fatal_error("net: in net_init(), specified hostname not found. (host=\"%s\"", our_hostname);
			return -1;
		}

		/*
		 * Normally we'd like to use the official name, but since the
		 * specified hostname resolved, we'll just use that.
		 */
		strncpy(local_host.name, our_hostname, MAXHOSTLENGTH-1);
		local_host.name[MAXHOSTLENGTH-1] = 0;

		local_host.perm = 1;	/* don't try and lookup our hostname again */

		return 0;
	}

	if(getmyhostname(local_host.name) == -1)
	{
		fatal_error("net: in net_init(), hostname self-lookup failed.");
		return -1;
	}

	return 0;
}


int enable_virtual_host(char *virtual_host)
/*
 * Function: Enable virtual hosting
 *  Returns:  0 if OK
 *           -1 if not enabled (couldn't determine host)
 *  
 *  Enable virtual hosting by validating the host, and building the
 *  address struct.  (vh.vh_sin)  The virtual host will be bound to
 *  all sockets from this point on.
 *
 *  Specify virtual_host as regular hostname or IP address.
 *
 *  An OK from this function doesn't necessarily mean that we will
 *  be able to bind it during the connect().  Note that since we create
 *  the address once, if the IP address for the virtual host changes,
 *  it won't bind anymore.
 * 
 *  We'll set our hostname to be the virtual_host, and mark it as
 *  being permanent.
 *
 *  To disable the virtual host, set virtual_host to NULL.
 */
{
	struct hostent *hp;
	u_long inaddr;

	vh.vh_enabled = 0;

	if(virtual_host == NULL)
	{
		vh.vh_enabled = 0;
		local_host.use_vh = 0;
		
		return 0;
	}

	if(!virtual_host[0])
	{
		internal("net: in enable_virtual_host(), host is empty.");
		return -1;
	}

	/* Setup address (and get official hostname) */
	memset((char *)&vh.vh_sin, 0, sizeof(vh.vh_sin));
	vh.vh_sin.sin_family = AF_INET;
	vh.vh_sin.sin_port = 0;

	/* Is host in numeric IP form? */
	inaddr = (IP)inet_addr(virtual_host);
	if(inaddr != INADDR_NONE)
		hp = gethostbyaddr((char *)&inaddr, sizeof(inaddr), AF_INET);
	else
		hp = gethostbyname(virtual_host);

	if(hp == NULL || (hp->h_length != 4 && hp->h_length != 8))
		return -1;

 	xmemcpy((char *)&vh.vh_sin.sin_addr, hp->h_addr, hp->h_length);

	/* Virtual host resolved.  Setup virtual host struct. */
	strncpy(vh.vh_name, virtual_host, MAXHOSTLENGTH-1);
	vh.vh_name[MAXHOSTLENGTH-1] = 0;

	strncpy(vh.vh_official, hp->h_name, MAXHOSTLENGTH-1);
	vh.vh_official[MAXHOSTLENGTH-1] = 0;

	vh.vh_enabled = 1;

	/* Copy it to our local_host information */
	strcpy(local_host.vh_name, vh.vh_name);
	local_host.perm = 1;	/* don't try and lookup our hostname again */
	local_host.use_vh = 1;	/* use virtual host from now on */

	return 0;
}


int net_connect(char *host, u_short port, int binary, int strong)
/*
 * Function: Connect to the given host and port
 *  Returns:  sock if OK
 *            -1   if error
 *            -2   if host not found
 *
 *  Supports virtual hosting.  Use enable_virtual_host() to configure.
 *  Set binary to 1 if a binary (non-buffered) connection is needed.
 *  Set strong to 1 if a connect() is considered successful only if we
 *  detect a read first.
 *
 *  Actually, in most cases we don't care whether we're connected.  If
 *  net_connect() doesn't fail, then we start dumping data to the socket.
 *  We'll get an EOF soon enough if we didn't actually connect.
 */
{
	IP inaddr;
	int sock;
	char flags;
	struct sockaddr_in sin;
	struct hostent *hp;

	if(host == NULL || !host[0])
	{
		internal("net: in net_connect(), host is NULL.");
		return -1;
	}

	/* setup address */
	memset((char *)&sin, 0, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_port = htons(port);

	/* Is host in numeric IP form? */
	inaddr = (IP)inet_addr(host);
	if(inaddr != INADDR_NONE)
	{
		/* yep */
		xmemcpy((char *)&sin.sin_addr, (char *)&inaddr, sizeof(inaddr));
	}
	else
	{
		/* nope */
		hp = gethostbyname(host);
		if(hp == NULL || (hp->h_length != 4 && hp->h_length != 8))
			return -2; /* host not found */

 		xmemcpy((char *)&sin.sin_addr, hp->h_addr, hp->h_length);
	}

	flags = SOCK_CONNECTING;
	if(binary)
		flags |= SOCK_BINARY;
	if(strong)
		flags |= SOCK_STRONG;

	sock = new_socket(flags);
	if(sock == -1)
		return -1; /* fatal */

	set_socket(sock, SO_KEEPALIVE, 1);
	set_socket(sock, SO_LINGER, 0);

	/*
	 * Virtual host?  (enabled using enable_virtual_host())
	 */
	if(vh.vh_enabled)
		bind_virtual_host(sock);

	if(connect(sock, (struct sockaddr *)&sin, sizeof(sin)) < 0)
	{
		/* non-blocking I/O (we'll catch error as EOF during write) */
		if(net_errno() == EINPROGRESS)
			return sock;

		debug("net: in net_connect(), connect() failed: %s (sock=%d)",
			net_errstr(), sock);
		destroy_socket(sock);
		return -1;
	}

	return sock;
}


void net_disconnect(int sock)
/*
 * Function: Disconnect a socket and close
 *
 */
{
	destroy_socket(sock);
}


int net_listen(u_short *port)
/*
 * Function: Listen on a port
 *  Returns: sock if OK
 *           -1   if couldn't listen
 *
 *  Returns actual port that was bound in *port.  Specifying a port of 0
 *  tells the system to choose one itself.
 *
 *  A connection on a listening socket is indicated when sockgets()
 *  returns the socket number of the listening connection.  To complete
 *  the connection, call net_accept().
 *
 *  Note that the port is specified in host byte ordering.
 */
{
	int sock, addrlen;
	struct sockaddr_in sin;

	sock = new_socket(SOCK_LISTEN);
	if(sock == -1)
		return -1; /* fatal, no socktbl space */

	set_socket(sock, SO_REUSEADDR, 1);

	/*
	 * Bind our address
	 */
	memset((char *)&sin, 0, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = htonl(INADDR_ANY); /* Accept from any network */
	sin.sin_port = htons(*port);

	if(bind(sock, (struct sockaddr *)&sin, sizeof(sin)) < 0)
	{
		debug("net: in net_listen(), bind() failed: %s (addr=%s, sock=%d)",
			net_errstr(), iptodn(sin.sin_addr.s_addr), sock);
		destroy_socket(sock);

		return -1;
	}

	/*
	 * Second arg specifies the backlog.  BSD limits to 5 if set higher.
	 */
	if(listen(sock, 5) < 0)
	{
		debug("net: in net_listen(), listen() failed: %s (sock=%d)",
			net_errstr(), sock);
		destroy_socket(sock);

		return -1;
	}

	/*
	 * Return actual port we connected on.
	 */
	addrlen = sizeof(sin);
	if(getsockname(sock, (struct sockaddr *)&sin, &addrlen) < 0)
	{
		debug("net: in listen(), getsockname() failed: %s (sock=%d)",
			net_errstr(), sock);
		destroy_socket(sock);

		return -1;
	}
	*port = ntohs(sin.sin_port);

	return sock;
}


int net_accept(int sock, char *fromhost, IP *fromip, u_short *port, int binary)
/*
 * Function: Accept an incoming connect
 *  Returns: sock if OK
 *           -1   if couldn't accept
 *
 *  A listening socket that has just received activity should call this
 *  to accept the connection.  The new socket is returned, and fromhost,
 *  fromip, and port will contain peer information, if non-NULL.
 *
 *  net_accept() automatically adds the new socket to the socktbl.
 */
{
	int new_sock, addrlen;
	struct sockaddr_in from;
	char flags = 0;

	addrlen = sizeof(struct sockaddr);
	new_sock = accept(sock, (struct sockaddr *)&from, &addrlen);
	if(new_sock < 0)
	{
		debug("net: in net_accept(), accept() failed: %s (sock=%d)",
			net_errstr(), sock);
		return -1;
	}

	if(fromip != NULL)
		*fromip = from.sin_addr.s_addr;

	if(fromhost != NULL)
	{
		struct hostent *hp;
		IP ip = from.sin_addr.s_addr;

		/*
		 * If gethostbyaddr() fails, fromhost will contain the IP address of
		 * the connecting host (in decimal notation) instead of the hostname.
		 */
		hp = gethostbyaddr((char *)&ip, sizeof(ip), AF_INET);
		if(hp == NULL || (hp->h_length != 4 && hp->h_length != 8))
			strncpy(fromhost, iptodn(ip), MAXHOSTLENGTH-1);
		else
			strncpy(fromhost, hp->h_name, MAXHOSTLENGTH-1);

		fromhost[MAXHOSTLENGTH-1] = 0;
	}

	if(port != NULL)
		*port = ntohs(from.sin_port);

	/* Add socket to the socket table */
	if(binary)
		flags |= SOCK_BINARY;
	add_socket(new_sock, flags);

	return new_sock;
}


int net_sockgets(char *buf, int *len)
/*
 * Function: Buffer socket read data
 *  Returns: socket if data ready (return in *buf)
 *           -1 on EOF (socket returned in *len)
 *           -2 on select() or read() error
 *           -3 if no data ready
 *
 *  Reads from all registered sockets for up to one second.  If
 *  after one second no complete data has been received from any of
 *  the sockets, 'buf' will be empty, 'len' will be 0, and -3 will
 *  be returned.
 *
 *  sockgets() buffers incoming data until it finds a newline, which
 *  indicates the data is complete.  If there is returnable data from
 *  a socket, 'buf' will contain the data (null-terminated if non-binary),
 *  'len' will contain the length, and the socket number will be returned.
 *
 *  When a listening socket receives a connection, its socket number is
 *  returned, 'buf' will be empty, and 'len' will be 0.
 *
 *  When a connecting socket is successful, its socket number is returned,
 *  'buf' will be empty, and 'len' will be 0.  Strong connections require
 *  a read to be successful, but an empty string is still returned.  Data
 *  which was read is buffered and can be picked up next time around.
 *
 *  If an EOF is detected from any socket, that socket number will be
 *  put in 'len', and -1 will be returned.
 *
 *  If a read() or socket() error occurs, that socket number will be put
 *  in 'len', and -2 will be returned.
 *
 *  The maximum length of any string returned is MAXREADBUFLENGTH (512),
 *  including NULL.
 */
{
	char xx[MAXREADBUFLENGTH+3], *p, *px;
	int ret, i, data = 0;

	/*
	 * Check for stored-up data ready to go on a socket.  Data which is
	 * ready to go is that which has an EOL marker.  We return it right
	 * away and skip the read().
	 */
	for(i = 0; i < NET_MAXSOCKS; i++)
	{
		if(socktbl[i].flags & SOCK_UNUSED)
			continue;

		/* also check any sockets that might have EOF'd during write */
		if(socktbl[i].flags & SOCK_EOFD)
		{
			buf[0] = 0;
			*len = socktbl[i].sock;
			return -1; /* EOF */
		}

		if(socktbl[i].inbuf == NULL)
			continue;

		p = strchr(socktbl[i].inbuf, '\n');
		if(p == NULL)
			p = strchr(socktbl[i].inbuf, '\r');
		if(p != NULL)
		{
			*p = 0;
			p++;

			/* truncate if block (up to EOL) exceeds buffersize */
			if(strlen(socktbl[i].inbuf) > MAXREADBUFLENGTH-1)
				socktbl[i].inbuf[MAXREADBUFLENGTH-1] = 0;
			strcpy(buf, socktbl[i].inbuf);

			/* If more data past EOL exists, reset inbuf to point to it */
			px = NULL;
			if(p[0])
			{
				px = (char *)xmalloc(strlen(p) + 1);
				strcpy(px, p);
			}

			xfree(socktbl[i].inbuf);
			socktbl[i].inbuf = px;

			/* strip CR if this was CR/LF combo */
			if(buf[strlen(buf) - 1] == '\r')
				buf[strlen(buf) - 1] = 0;

			*len = strlen(buf);

			return socktbl[i].sock;
		}
	}

	/*
	 * No stored-up data, or not complete.  Check for new data.
	 */
	*len = 0;
	ret = sockread(xx, len);
	if(ret < 0)
	{
		buf[0] = 0;
		return ret;	 /* return error code */
	}

	/*
	 * Indicate a socket has connected.
	 */
	if(socktbl[ret].flags & SOCK_CONNECTING)
	{
		if(socktbl[ret].flags & SOCK_STRONG)
		{
			/*
			 * On strong connections, we wait until we read some
			 * data to indicate a successful connect attempt.
			 * We got a read, so the connection was successful.
			 * Buffer the incoming data, and we'll pick it up
			 * the next time around.
			 */
			socktbl[ret].flags &= ~SOCK_STRONG;
			socktbl[ret].inbuf = (char *)xmalloc(strlen(xx) + 1);
			strcpy(socktbl[ret].inbuf, xx);
		}

		socktbl[ret].flags &= ~SOCK_CONNECTING;
		buf[0] = 0;

		return socktbl[ret].sock;
	}

	/* Binary and listening sockets don't get buffered */
	if(socktbl[ret].flags & SOCK_BINARY)
	{
		xmemcpy(buf, xx, *len);
		return socktbl[ret].sock;
	}

	if(socktbl[ret].flags & SOCK_LISTEN)
	{
		buf[0] = 0;
		*len = 0;
		return socktbl[ret].sock;
	}

	/*
	 * If inbuf isn't empty, prepend it to the new data before we
	 * go through the motions of checking EOL.  (new = old+new)
	 */
	if(socktbl[ret].inbuf != NULL)
	{
		/* Append new data to contents of inbuf */
		int new_len = strlen(xx) + strlen(socktbl[ret].inbuf);

		p = socktbl[ret].inbuf;
		socktbl[ret].inbuf = (char *)xmalloc(new_len + 1);
		strcpy(socktbl[ret].inbuf, p);
		strcat(socktbl[ret].inbuf, xx);
		xfree(p);

		if(new_len <= MAXREADBUFLENGTH-1)
		{
			/*
			 * Total data doesn't exceed buffersize.  Move it all back
			 * to new data string ready for EOL checking.
			 */
			strcpy(xx, socktbl[ret].inbuf);
			xfree(socktbl[ret].inbuf);
			socktbl[ret].inbuf = NULL;
		}
		else
		{
			/*
			 * Exceeds buffersize.  Copy everything up to MAX to incoming
			 * data string and process as normal.  Reset inbuf to point to
			 * everything after MAX.
			 */
			p = socktbl[ret].inbuf + MAXREADBUFLENGTH-1;
			px = (char *)xmalloc((new_len - (MAXREADBUFLENGTH-1)) + 1);
			strcpy(px, p);

			*p = 0;
			strcpy(xx, socktbl[ret].inbuf);
			xfree(socktbl[ret].inbuf);
			socktbl[ret].inbuf = px;
		}
	}

	/*
	 * Time to deal with new data.  If EOL marker exists then we have
	 * something to show.  If not, buffer it for later.
	 */
	p = strchr(xx, '\n');
	if(p == NULL)
		p = strchr(xx, '\r');
	if(p != NULL)
	{
		*p = 0;
		p++;

		strcpy(buf, xx);
		strcpy(xx, p);

		/* strip CR if this was CR/LF combo */
		if(buf[strlen(buf) - 1] == '\r')
			buf[strlen(buf) - 1] = 0;

		/*
		 * Make sure we indicate data, even if it's a blank line.
		 */
		data = 1;
	}
	else
	{
		/* Nothing to show. */
		buf[0] = 0;

		/*
		 * If the new data is full up to the buffersize, it has no room
		 * for EOL, so we return it the way it is.  Because we were
		 * careful above, the new data will never be longer than MAX,
		 * so we don't have to worry about truncation.  Any additional
		 * data (most likely) has already been buffered and we'll pick
		 * it up the next time around.
		 *
		 * Basically this just amounts to chopping longs blocks of data
		 * at the MAX point.  No data is ever lost.
		 */
		if(strlen(xx) >= (MAXREADBUFLENGTH-1))
		{
			strcpy(buf, xx);
			xx[0] = 0;
			data = 1;
		}
	}

	/*
	 * Check if there is any unbuffered data left over.  If so, pre-pend it
	 * back to inbuf.
	 */
	if(xx[0])
	{
		if(socktbl[ret].inbuf != NULL)
		{
			p = socktbl[ret].inbuf;
	
			socktbl[ret].inbuf = (char *)xmalloc(strlen(p) + strlen(xx) + 1);
			strcpy(socktbl[ret].inbuf, xx);
			strcat(socktbl[ret].inbuf, p);
			xfree(p);
		}
		else
		{
			socktbl[ret].inbuf = (char *)xmalloc(strlen(xx) + 1);
			strcpy(socktbl[ret].inbuf, xx);
		}
	}

	if(data)
	{
		*len = strlen(buf);
		return socktbl[ret].sock;
	}
	
	*len = 0;
	buf[0] = 0;

	return -3;
}


int net_sockputs(int sock, char *buf, int len)
/*
 * Function: Send something to a socket
 *  Returns:  0 if OK
 *           -1 if error (socket not found)
 *
 *  Send data to a socket.  Data may be queued. 'len' must be explicitly
 *  declared, since we may be dumping binary data.
 */
{
	int pos, x;
	char *p;

	pos = find_socket(sock);
	if(pos == -1)
	{
		internal("net: in net_sockputs(), socket not found. (sock=%d)", sock);
		return -1;
	}

	/*
	 * Check if we're already queuing.  If so, just add new data and
	 * return.
	 */
	if(socktbl[pos].outbuf != NULL)
	{
		p = (char *)xrealloc(socktbl[pos].outbuf, socktbl[pos].outbuflen + len);
		xmemcpy(p + socktbl[pos].outbuflen, buf, len);

		socktbl[pos].outbuf = p;
		socktbl[pos].outbuflen += len;

		return 0;
	}

	/*
	 * Attempt a write.  If it fails we queue it for later.  We'll check
	 * for EOF when we dequeue.
	 */
#ifdef WIN32
	x = send(sock, buf, len, 0);
#else
	x = write(sock, buf, len);
#endif
	if(x == -1)
	{
		if(net_errno() != EAGAIN)
			debug("net: in net_sockputs(), write() failed: %s (sock %d)",
				net_errstr(), sock);
		x = 0;
	}
	if(x < len)
	{
		/* socket full.  queue it. */
		socktbl[pos].outbuf = (char *)xmalloc(len - x);
		xmemcpy(socktbl[pos].outbuf, &buf[x], len - x);
		socktbl[pos].outbuflen = len - x;
	}

	return 0;
}


void net_dequeue()
/*
 * Function: Dequeue any data pending write
 *
 *  Sockets have their outbuf queued by net_sockputs() when the write()
 *  request could not write complete data.  EOF on any of the sockets
 *  may be detected here, which is signaled to the user in net_sockgets().
 */
{
	int i, x;

	for(i = 0; i < NET_MAXSOCKS; i++)
	{
		if((socktbl[i].flags & SOCK_UNUSED) || socktbl[i].outbuf == NULL)
			continue;

#ifdef WIN32
		x = send(socktbl[i].sock, socktbl[i].outbuf, socktbl[i].outbuflen, 0);
#else
		x = write(socktbl[i].sock, socktbl[i].outbuf, socktbl[i].outbuflen);
#endif

		if((x < 0) && (net_errno() != EAGAIN)
#ifdef EBADSLT
			&& (net_errno() != EBADSLT)
#endif
#ifdef ENOTCONN
			&& (net_errno() != ENOTCONN)
#endif
			)
		{
			/* Detected EOF. */
			internal("net: in net_dequeue(), detected EOF: %s", net_errstr());
			socktbl[i].flags |= SOCK_EOFD;
		}
		else if(x == socktbl[i].outbuflen)
		{
			/* write was successful.  Free queued data. */
			xfree(socktbl[i].outbuf);
			socktbl[i].outbuf = NULL;
			socktbl[i].outbuflen = 0;
		}
		else if(x > 0)
		{
			char *p = socktbl[i].outbuf;

			/* Remove sent bytes from beginning of buffer */
			socktbl[i].outbuf = (char *)xmalloc(socktbl[i].outbuflen - x);
			xmemcpy(socktbl[i].outbuf, p + x, socktbl[i].outbuflen - x);
			socktbl[i].outbuflen -= x;
			xfree(p);
		}
	}
}


char *net_errstr()
/*
 * Function: Get name of error
 *  Returns: pointer to string containing name of error
 *
 *  Used to get name of error after an operations fails.
 *  (pulled from <errno.h>)
 */
{
	static char errstr[MAXNETERRLENGTH];
	int x = net_errno();

	errstr[0] = 0;
	switch(x)
	{
	case EAGAIN:
		strcpy(errstr, "Try again");
		break;
	case EADDRINUSE:
		strcpy(errstr, "Address already in use");
		break;
	case EADDRNOTAVAIL:
		strcpy(errstr, "Cannot assign requested address");
		break;
	case EAFNOSUPPORT:
		strcpy(errstr, "Address family not supported by protocol");
		break;
	case EALREADY:
		strcpy(errstr, "Operation already in progress");
		break;
	case EBADF:
		strcpy(errstr, "Socket descriptor is bad");
		break;
	case ECONNREFUSED:
		strcpy(errstr, "Connection refused");
		break;
	case EFAULT:
		strcpy(errstr, "Bad address (namespace segment violation)");
		break;
	case EINPROGRESS:
		strcpy(errstr, "Operation now in progress");
		break;
	case EINTR:
		strcpy(errstr, "Interrupted system call");
		break;
	case EINVAL:
		strcpy(errstr, "Invalid argument");
		break;
	case EISCONN:
		strcpy(errstr, "Socket already connected");
		break;
	case ENETUNREACH:
		strcpy(errstr, "Network unreachable");
		break;
	case ENOTSOCK:
		strcpy(errstr, "Socket operation on non-socket");
		break;
	case ETIMEDOUT:
		strcpy(errstr, "Connection timed out");
		break;
#ifdef ENOTCONN
	case ENOTCONN:
		strcpy(errstr, "Socket is not connected");
		break;
#endif
	case EHOSTUNREACH:
		strcpy(errstr, "Host is unreachable");
		break;
	case EPIPE:
		strcpy(errstr, "Broken pipe");
		break;
#ifdef ECONNRESET
	case ECONNRESET:
		strcpy(errstr, "Connection reset by peer");
		break;
#endif
#ifdef EACCES
	case EACCES:
		strcpy(errstr, "Permission denied");
		break;
#endif
	case 0:
		strcpy(errstr, "Error 0");
		break;
	default:
		sprintf(errstr, "Unforseen error %d", x);
		break;
	}

	return errstr;
}

void net_dump_socktbl()
{
	int i;
	char s[80];

	for(i = 0; i < NET_MAXSOCKS; i++)
	{
		if(socktbl[i].flags & SOCK_UNUSED)
			continue;

		sprintf(s, "SOCKTBL: [%d] ", socktbl[i].sock);
		if(socktbl[i].flags & SOCK_NONSOCK)
			strcat(s, " (file)");
		if(socktbl[i].flags & SOCK_CONNECTING)
			strcat(s, " (connecting)");
		if(socktbl[i].flags & SOCK_LISTEN)
			strcat(s, " (listening)");
		if(socktbl[i].flags & SOCK_STRONG)
			strcat(s, " (strong)");
		if(socktbl[i].flags & SOCK_BINARY)
			strcat(s, " (binary)");

		if(socktbl[i].inbuf != NULL)
			sprintf(&s[strlen(s)], " (inbuf: %u)", strlen(socktbl[i].inbuf));
		if(socktbl[i].outbuf != NULL)
			sprintf(&s[strlen(s)], " (outbuf: %d)", socktbl[i].outbuflen);

		debug("%s", s);
	}
}


/*
 * Miscellanea
 */

u_long net_sizeof()
/*
 * Function: Determine how much memory the network module is using
 *  Returns: amount of memory (in bytes)
 */
{
	u_long tot = 0L;
	int	i;

	tot += sizeof(vh.vh_name);
	tot += sizeof(vh.vh_official);
	tot += sizeof(vh.vh_enabled);
	tot += sizeof(vh.vh_sin);

	tot += sizeof(local_host.name);
	tot += sizeof(local_host.vh_name);
	tot += sizeof(local_host.perm);
	tot += sizeof(local_host.use_vh);

	for(i = 0; i < NET_MAXSOCKS; i++)
	{
		tot += sizeof(socktbl[i]);
		tot += sizeof(socktbl[i].sock);
		tot += sizeof(socktbl[i].flags);
		tot += sizeof(socktbl[i].inbuf);
		tot += socktbl[i].inbuf ? strlen(socktbl[i].inbuf) + 1 : 0L;
		tot += sizeof(socktbl[i].outbuf);
		tot += socktbl[i].outbuflen;
	}

	return tot;
}


char *iptodn(IP ip)
/*
 * Function: Converts network byte order IP into decimal notation
 *  Returns: pointer to buffer containing decimal notation
 *
 *  2130706433 --> "127.0.0.1"
 *
 *  Note that iptodn() works on decimal-notation strings whereas atoip()
 *  works on a simple numeric string.
 *
 */
{
	u_char *p;
	static char dec_addr[16];

	dec_addr[0] = 0;
	p = (u_char *)&ip;
	sprintf(dec_addr, "%u.%u.%u.%u", p[0], p[1], p[2], p[3]);

	return dec_addr;
}


char *iptohostname(IP ip)
/*
 * Function: Looks up the hostname for the specified IP address
 *  Returns: pointer to buffer containing hostname (empty if fail)
 *
 *  2130706433 --> "asylum.rt66.com"
 *
 */
{
	static char hostname[MAXHOSTLENGTH];
	struct hostent *hp;

	hostname[0] = 0;
	hp = gethostbyaddr((char *)&ip, sizeof(ip), AF_INET);

	if(hp == NULL || (hp->h_length != 4 && hp->h_length != 8))
		return hostname;
	
	strncpy(hostname, hp->h_name, MAXHOSTLENGTH-1);
	hostname[MAXHOSTLENGTH-1] = 0;

	return hostname;
}


IP hostnametoip(char *hostname)
/*
 * Function: Looks up the IP address of specified hostname
 *  Returns: IP address in network byte order
 *
 *  "asylum.rt66.com" --> 2130706433
 *
 *  Only the first address is returned if multiple IP addresses are present.
 *
 */
{
	struct hostent *hp;
	struct in_addr *in;

	hp = gethostbyname(hostname);
	if(hp == NULL || (hp->h_length != 4 && hp->h_length != 8))
		return 0;

	in = (struct in_addr *)(hp->h_addr);
	return (IP)(in->s_addr);
}


IP atoip(char *ip)
/*
 * Function: Converts string into IP type (32bit)
 *  Returns: IP long type
 *
 *  "2130706433" --> 2130706433
 *
 *  Note this works a little differently than iptodn().  iptodn() converts
 *  an IP long address into a decimal notation string.  atoip() converts
 *  a numeric string into an IP long number.  Works just like atoul(), except
 *  that IP may be an int or an unsigned long, depending on their length.
 *
 */
{
	IP ret = 0;
	while((*ip >= '0') && (*ip <= '9'))
	{
		ret *= 10;
		ret += ((*ip) - '0');
		ip++;
	}

	return ret;
}


/*
 * Local information
 */

char *myhostname()
/*
 * Function: Return our hostname
 *  Returns: pointer to buffer containing hostname
 *
 *  Must have been previously set using getmyhostname().  Usually handled
 *  in net_init().
 */
{
	if(local_host.use_vh)
		return local_host.vh_name;

	return local_host.name;
}

IP myip()
/*
 * Function: Return our ip address
 *  Returns: our ip address in network byte order
 *
 *  This is provided for backward compatibility so that we can plug it
 *  in to dcc.c.  Eventually, all local IP information will be stored in
 *  the server struct.
 */
{
	return getmyip(-1);
}





#ifdef TEST
/* -------------------------------------------------------------------------
 * TESTING
 *
 * Verify that our hostname is what it should be, and that our IP is also
 * what it should be.
 *
 * All tests should perform OK, and no warnings should occur.  Compile with
 *    gcc -o net net.c -DTEST -DDEBUG
 * ------------------------------------------------------------------------- */

static void debug_to_stderr(char *fmt, ...)
{
	va_list args;

	va_start(args, fmt);
	vfprintf(stderr, fmt, args);
	fprintf(stderr, "\n");
	va_end(args);
}

void try_this()
{
}

int main(int argc, char **argv)
{
	int x, len;
	int wrote_data = 0, connect1 = 0, connect2 = 0, connect3 = 0;
	int line1 = 0;
	int ok1 = 0, ok2 = 0, ok3 = 0;
	int ok1_tmp = 0;
	char buf[BUFSIZ];
	char data1[15][90] = {
		"Data from connection 1 (line 1).  ABCDEFGHIJKLMNOPQRSTUVWXYZ-ABCDEFGHIJKLMNOPQRSTUVWXYZ",
		"Data from connection 1 (line 2).  ABCDEFGHIJKLMNOPQRSTUVWXYZ-ABCDEFGHIJKLMNOPQRSTUVWXYZ",
		"Data from connection 1 (line 3).  ABCDEFGHIJKLMNOPQRSTUVWXYZ-ABCDEFGHIJKLMNOPQRSTUVWXYZ",
		"Data from connection 1 (line 4).  ABCDEFGHIJKLMNOPQRSTUVWXYZ-ABCDEFGHIJKLMNOPQRSTUVWXYZ",
		"Data from connection 1 (line 5).  ABCDEFGHIJKLMNOPQRSTUVWXYZ-ABCDEFGHIJKLMNOPQRSTUVWXYZ",
		"Data from connection 1 (line 6).  ABCDEFGHIJKLMNOPQRSTUVWXYZ-ABCDEFGHIJKLMNOPQRSTUVWXYZ",
		"Data from connection 1 (line 7).  ABCDEFGHIJKLMNOPQRSTUVWXYZ-ABCDEFGHIJKLMNOPQRSTUVWXYZ",
		"Data from connection 1 (line 8).  ABCDEFGHIJKLMNOPQRSTUVWXYZ-ABCDEFGHIJKLMNOPQRSTUVWXYZ",
		"Data from connection 1 (line 9).  ABCDEFGHIJKLMNOPQRSTUVWXYZ-ABCDEFGHIJKLMNOPQRSTUVWXYZ",
		"Data from connection 1 (line 10).  ABCDEFGHIJKLMNOPQRSTUVWXYZ-ABCDEFGHIJKLMNOPQRSTUVWXYZ",
		"Data from connection 1 (line 11).  ABCDEFGHIJKLMNOPQRSTUVWXYZ-ABCDEFGHIJKLMNOPQRSTUVWXYZ",
		"Data from connection 1 (line 12).  ABCDEFGHIJKLMNOPQRSTUVWXYZ-ABCDEFGHIJKLMNOPQRSTUVWXYZ",
		"Data from connection 1 (line 13).  ABCDEFGHIJKLMNOPQRSTUVWXYZ-ABCDEFGHIJKLMNOPQRSTUVWXYZ",
		"Data from connection 1 (line 14).  ABCDEFGHIJKLMNOPQRSTUVWXYZ-ABCDEFGHIJKLMNOPQRSTUVWXYZ"
	};
	char data2[] = "Data from connection 2 (two).  ABCDEFGHIJKLMNOPQRSTUVWXY";
	char data3[] = "Data from connection 3 (three).  ABCDEFGHIJKLMNOPQRSTUVWXY";
	int l1 = -1, l2 = -1, l3 = -1;
	int c1 = -1, c2 = -1, c3 = -1;
	int a1 = -1, a2 = -1, a3 = -1;
	u_short port1 = 0, port2 = 0, port3 = 0;

	printf("Testing module: net\n");
	printf("  %s\n", net_init(NULL) == 0 ? "init: ok" : "init: failed");
	printf("  hostname: %s\n", local_host.name);

	l1 = net_listen(&port1);
	l2 = net_listen(&port2);
	l3 = net_listen(&port3);
	if(l1 > 0 && l2 > 0 && l3 > 0)
		printf("  listen: all sockets listening.\n");
	else
	{
		printf("  listen: failed. (%d %d %d)\n", l1, l2, l3);
		return 1;
	}

	c1 = net_connect(local_host.name, port1, 0, 0);   /* normal */
	c2 = net_connect(local_host.name, port2, 0, 1);   /* strong */
	c3 = net_connect(local_host.name, port3, 1, 0);   /* binary */
	if(c1 > 0 && c2 > 0 && c3 > 0)
		printf("  connect: all sockets connected.\n");
	else
	{
		printf("  connect: failed. (%d %d %d).\n", c1, c2, c3);
		return 1;
	}

	while(1)
	{
		net_dequeue();
		x = net_sockgets(buf, &len);
		if(x == -1 || x == -2)
			destroy_socket(len);
		else if(x >= 0)
		{
			if(x == l1)
			{
				a1 = net_accept(l1, NULL, 0, 0, 0);
				printf("  [%d] Accepted connection on line 1.\n", x);
				connect1 = 1;
			}
			else if(x == l2)
			{
				a2 = net_accept(l2, NULL, 0, 0, 0);
				printf("  [%d] Accepted connection on line 2.\n", x);
				connect2 = 1;
			}
			else if(x == l3)
			{
				a3 = net_accept(l3, NULL, 0, 0, 1); /* binary */
				printf("  [%d] Accepted connection on line 3.\n", x);
				connect3 = 1;
			}
			else if(x == a1)
			{
				if(strcmp(buf, data1[line1]) == 0)
					ok1_tmp++; 
				else
					printf("  [%d] Data on line 1 failed. (%s)\n", x, buf);

				line1++;
				if(line1 == 15)
					destroy_socket(a1);

				if(ok1_tmp == 15)
				{
					printf("  [%d] Data on line 1 was correct.\n", x);
					ok1 = 1;
				}
			}
			else if(x == a2)
			{
				if(strcmp(buf, data2) == 0)
				{
					printf("  [%d] Data on line 2 was correct.\n", x);
					ok2 = 1;
				}
				else
					printf("  [%d] Data on line 2 failed. (%s)\n", x, buf);
				destroy_socket(a2);
			}
			else if(x == a3)
			{
				if(memcmp(buf, data3, len) == 0)  /* binary */
				{
					printf("  [%d] Data on line 3 was correct.\n", x);
					ok3 = 1;
				}
				else
					printf("  [%d] Data on line 3 failed.\n", x);
				destroy_socket(a3);
			}
		}

		if((connect1 && connect2 && connect3) && !wrote_data)
		{
			/*
			 * all sockets connected.  Write data to each one.
			 * The reason for so many writes to the first connection is
			 * the we want to exercise all possible cases in sockgets(),
			 * and to do that we need to exceed the buffersize, yadda yadda.
			 */
			sprintf(buf, "%s\n", data1[0]);
			net_sockputs(c1, buf, strlen(buf));
			sprintf(buf, "%s\n", data1[1]);
			net_sockputs(c1, buf, strlen(buf));
			sprintf(buf, "%s\n", data1[2]);
			net_sockputs(c1, buf, strlen(buf));
			sprintf(buf, "%s\n", data1[3]);
			net_sockputs(c1, buf, strlen(buf));
			sprintf(buf, "%s\n", data1[4]);
			net_sockputs(c1, buf, strlen(buf));
			sprintf(buf, "%s\n", data1[5]);
			net_sockputs(c1, buf, strlen(buf));
			sprintf(buf, "%s\n", data1[6]);
			net_sockputs(c1, buf, strlen(buf));
			sprintf(buf, "%s\n", data1[7]);
			net_sockputs(c1, buf, strlen(buf));
			sprintf(buf, "%s\n", data1[8]);
			net_sockputs(c1, buf, strlen(buf));
			sprintf(buf, "%s\n", data1[9]);
			net_sockputs(c1, buf, strlen(buf));
			sprintf(buf, "%s\n", data1[10]);
			net_sockputs(c1, buf, strlen(buf));
			sprintf(buf, "%s\n", data1[11]);
			net_sockputs(c1, buf, strlen(buf));
			sprintf(buf, "%s\n", data1[12]);
			net_sockputs(c1, buf, strlen(buf));
			sprintf(buf, "%s\n", data1[13]);
			net_sockputs(c1, buf, strlen(buf));
			sprintf(buf, "%s\n", data1[14]);
			net_sockputs(c1, buf, strlen(buf));

			sprintf(buf, "%s\n", data2);
			net_sockputs(c2, buf, strlen(buf));

			sprintf(buf, "%s", data3);  /* binary */
			net_sockputs(c3, buf, strlen(buf));

			printf("  [%d,%d,%d] Wrote data on all lines.\n", c1, c2, c3);

			wrote_data = 1;
		}

		if(ok1 && ok2 && ok3)
			break;
	}

	if(ok1 && ok2 && ok3)
		printf("net: passed\n");
	else
		printf("net: failed\n");

	printf("\n");

	return 0;
}


#endif /* TEST */
