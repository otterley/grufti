#ifndef NET_H
#define NET_H

/*
    INITIALIZATION & CLEANUP
	init_net()
		Use this to initialize the socklist.  Returns void.

	dequeue_sockets()
		Use this to clear all socket buffers.  Normally used before
		a read.

    OPEN & CLOSE
	open_telnet_raw(sock,server,sport)
		Connects to socket on server at sport.  returns <0
		if connection refused:
			-1  neterror() type error
			-2  can't resolve hostname

	open_telnet(server,port)
		Finds first available socket.  Same returns as above.

	kill_sock(sock)
		Done with this socket.  Returns void.

    I/O
	sockgets(s,len)
		Buffered gets.  attempts to read from all registered sockets
		for up to one second.  if after one second, no complete data
		has been received from any of the sockets, 's' will be empty,
		'len' will be 0, and sockgets will return -3.

		if there is returnable data received from a socket, the data
		will be in 's' (null-terminated if non-binary), the length will
		be returned in len, and the socket number will be returned.

		normal sockets have their input buffered, and each call to
		sockgets will return one line terminated with a '\n'.  binary
		sockets are not buffered and return whatever comes in as soon
		as it arrives.

		listening sockets will return an empty string when a
		connection comes in.  connecting sockets will return an empty
		string on a successful connect, or EOF on a failed connect.

		if an EOF is detected from any of the sockets, that socket
		number will be put in len, and -1 will be returned.

		* the maximum length of the string returned is 512
		(including null)

	tprintf(fmt,...)
		
    MISC
	getmyip()
		Returns our IP in IP form.  (long int usually)

	getmyhostname(s)
		Copies our hostname into s.  Returns void.

	hostnamefromip(ip)
		Returns pointer to hostname taking ip address in IP format.

	neterror(s)
		Copies error message into s.  Returns void.
*/

#define PROXY_SUN 2
#define PROXY_SOCKS 1
#define MAXSOCKS 60
#define SOCK_UNUSED     0x01    /* empty socket */
#define SOCK_BINARY	0x02	/* do not buffer input */
#define SOCK_LISTEN	0x04	/* listening port */
#define SOCK_CONNECT    0x08    /* connection attempt */
#define SOCK_NONSOCK	0x10	/* used for file i/o on debug */
#define SOCK_STRONGCONN	0x20	/* don't report success until sure */
#define STDIN 0
#define STDOUT 1
#define STDERR 2

u_long	net_sizeof();
int	open_telnet_raw(int sock, char *server, int sport);
int	open_telnet(char *server, int port);
int	getsock(int options);
void	setsock(int sock, int options);
void	killsock(int sock);
u_int	getmyip();
void	my_memcpy(char *dest, char *src, int len);
void	my_bzero(char *dest, int len);
int	proxy_connect(int sock, char *host, int port, int proxy);
void	init_net();
int	answer(int sock, int *port, char *caller, u_long *ip, int bin);
void	getmyhostname(char *s);
void	neterror(char *s);
char	*hostnamefromip(u_long ip);
int	sockread(char *s, int *len);
int	sockgets(char *s, int *len);
void	tputs(int z, char *s);
void	dequeue_sockets();
void	tprintf(int sock, char *format, ...);
int	open_telnet_dcc(int sock, u_long addr, int p);
u_long	my_atoul(char *s);
int	open_listen(int *port);

#endif /* NET_H */
