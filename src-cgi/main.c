/******************************************************************************
 * Steve Rider - stever@area23.org
 * Grufti - an interactive, service-oriented bot for IRC 
 *
 * main.c - CGI interface for Grufti
 *
 *****************************************************************************/
/* 28 April 97 */

#include "../src/config.h"
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include "webclient.h"
#include "net.h"
#include "cgic.h"
#include "misc.h"

/* Public main objects */
Web_ob	*Web;

/* context logging */
char	cx_file[30];
int	cx_line;


int cgiMain()
{
	int	xx, i;
	char	oldpass[BUFSIZ];
	char	buf[BUFSIZ];
	time_t	now = 0L, then = 0L;


	/* Initialize objects */
	Web_new();

	/* Keep this first! */
	cgiHeaderContentType("text/html");

	/* Retrieve referer, username & password */
	if(cgiQueryString[0])
	{
		/* Get referer, username, & password from query */
		strcpy(buf,cgiQueryString);
		splitc(Web->username,buf,';');
		splitc(Web->password,buf,';');
		splitc(Web->referer,buf,';');
	}
	else
	{
		/* Get referer, username & password from forms */
		cgiFormStringNoNewlines("referer",Web->referer,81);
		cgiFormStringNoNewlines("username",Web->username,81);
		cgiFormStringNoNewlines("password",Web->password,81);
		if(Web->password[0])
		{
			strcpy(oldpass,Web->password);
			encrypt_pass(oldpass,Web->password);
		}
	}

	/*
	 * We need to investigate the 'blank page' behavior that occurs
	 * when the following code is ifdef'd out
	 */
#if 1

	/* If no referer, user is not using cgi via web page */
	if(!Web->referer[0])
		Client_accesserror();

	if(!Web->username[0] || !Web->password[0])
		Client_blankentry();
#endif

	/* Open socket to grufti */
	init_net();
	Web->sock = open_telnet(Web->grufti_host,Web->grufti_port);
	if(Web->sock < 0)
		Client_gruftidown();


	/* now wait for activity */
	now = time(NULL);
	while(1)
	{
		then = now;
		now = time(NULL);

		if(now != then)
		{
			/* Check for timeouts */
			if(Web->last_gotanything)
				if((now - Web->last_gotanything) > Web->timeout)
					Client_gruftidown();
		}

		/* check network */
		dequeue_sockets();

		xx = sockgets(buf,&i);

		if(xx == -1)
			Client_gotEOF();
		else if(xx >= 0)
		{
			Web->last_gotanything = time(NULL);
			Client_gotactivity(xx,buf,i);
		}
		else if((xx == -2) && errno != EINTR)
			Client_gotsocketerror();
		else if((xx == -3) && !(Web->flags & CONNECTED))
		{
			/* Quick way to find if we're connected */
			Web->flags |= CONNECTED;
			putgrufti("web-interface %s %s %s",Web->username,Web->password,cgiRemoteHost);
		}
	}
}
