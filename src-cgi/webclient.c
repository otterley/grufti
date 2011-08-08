/******************************************************************************
 * Steve Rider - stever@area23.org
 * Grufti - an interactive, service-oriented bot for IRC 
 *
 * webclient.c - CGI interface for Grufti
 *
 *****************************************************************************/
/* 28 April 97 */

#include "../src/config.h"
#include "../webconfig.h"
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include "webclient.h"
#include "net.h"
#include "cgic.h"
#include "misc.h"

/***************************************************************************n
 *
 * Web object definitions.
 *
 ****************************************************************************/

void Web_new()
{
	/* already in use? */
	if(Web != NULL)
		Web_freeall();

	/* allocate memory */
	Web = (Web_ob *)nmalloc(sizeof(Web_ob));

	/* initialize */
	Web->websay_buf[0] = 0;
	strcpy(Web->logfile,"set.if.you.use.putlog");

	strcpy(Web->main_url,WEB_MAIN_URL);
	strcpy(Web->cgi_url,WEB_CGI_URL);
	strcpy(Web->grufti_host,WEB_GRUFTI_HOST);
	Web->grufti_port = WEB_GRUFTI_PORT;
	strcpy(Web->bg_color,WEB_BGCOLOR);
	strcpy(Web->text_color,WEB_TEXT_COLOR);
	strcpy(Web->link_color,WEB_LINK_COLOR);
	strcpy(Web->vlink_color,WEB_VLINK_COLOR);
	strcpy(Web->alink_color,WEB_ALINK_COLOR);
	strcpy(Web->bg_file,WEB_BACKGROUND_IMAGE);
	strcpy(Web->color1,WEB_COLOR1);
	strcpy(Web->color2,WEB_COLOR2);
	strcpy(Web->color3,WEB_COLOR3);
	strcpy(Web->color4,WEB_COLOR4);

	/* Create body line from above information */
	strcpy(Web->html_body,"<body");
	if(Web->bg_color[0])
		sprintf(Web->html_body,"%s bgcolor=\"%s\"",Web->html_body,Web->bg_color);
	if(Web->text_color[0])
		sprintf(Web->html_body,"%s text=\"%s\"",Web->html_body,Web->text_color);
	if(Web->link_color[0])
		sprintf(Web->html_body,"%s link=\"%s\"",Web->html_body,Web->link_color);
	if(Web->vlink_color[0])
		sprintf(Web->html_body,"%s vlink=\"%s\"",Web->html_body,Web->vlink_color);
	if(Web->alink_color[0])
		sprintf(Web->html_body,"%s alink=\"%s\"",Web->html_body,Web->alink_color);
	if(Web->bg_file[0])
		sprintf(Web->html_body,"%s background=\"%s\"",Web->html_body,Web->bg_file);
	sprintf(Web->html_body,"%s>",Web->html_body);

	Web->sock = 0;
	Web->referer[0] = 0;
	Web->username[0] = 0;
	Web->password[0] = 0;
	Web->last_gotanything = 0L;
	Web->timeout = 30;
	Web->flags = 0;
}

void Web_freeall()
{
	/* Free the object */
	mfree(Web);
	Web = NULL;
}

/****************************************************************************/
/* object Web high-level implementation-specific function definitions. */
	
void Client_gotEOF()
{
	if(!(Web->flags & CONNECTED))
		Client_gruftidown();
	else
		Client_quit();
}

void Client_quit()
{
	Client_disconnect();
	exit(0);
}

void Client_disconnect()
{
	if(Web->sock)
		killsock(Web->sock);
}

void Client_gotsocketerror()
{
	/* Socket error */
	Client_gruftidown();
}

void Client_gotactivity(int sock, char *buf, int i)
{
	char	*p;

	if(!(Web->flags & CONNECTED) && buf[0] == 0)
		return;

	if(strncmp(buf,"OKOK>",5) == 0)
	{
		if(Web->flags & SENTCMD)
		{
			/* Cool we're done. */
			Client_quit();
		}

		/* Cool, we logged in.  Split up arguments */
		for(p=Web->referer; *p; p++)
		{
			if(*p == ':')
				*p = ' ';
		}

		/* If '!', then we need to process a form before responding */
		if(Web->referer[0] == '!')
			Client_processform();
		else
			putgrufti(Web->referer);
		Web->flags |= SENTCMD;
	}
	else
		putpage(buf);

}

void Client_processform()
{
	/* Pass form to appropriate type */

	if(isequal(Web->referer+1,"note_post"))
		Client_NotePost();
	else if(isequal(Web->referer+1,"update_notes"))
		Client_UpdateNotes();
	else if(isequal(Web->referer+1,"cmdline_post"))
		Client_CmdlinePost();
	else if(isequal(Web->referer+1,"setup_post"))
		Client_SetupPost();
	else if(isequal(Web->referer+1,"response_post"))
		Client_ResponsePost();
	else if(isequal(Web->referer+1,"new_response"))
		Client_NewResponsePost();
	else if(isequal(Web->referer+1,"event_post"))
		Client_EventPost();
	else if(isequal(Web->referer+1,"finger_lookup"))
		Client_FingerLookup();
}

void Client_NotePost()
{
	char	nick[25];
	char	body[8001];
	char	*p, *q, *n, c;

	cgiFormStringNoNewlines("nick",nick,21);
	cgiFormString("body",body,8000);
	if(!nick[0] || !body[0])
		Client_blanknote();


	/* Split Note up into LINELENGTH chunks */
	p = body;

	while(strlen(p) > LINELENGTH)
	{
		q = p + LINELENGTH;

		/* Search for embedded linefeed. */
		n = strchr(p,10);
		if((n != NULL) && (n < q))
		{
			/* Great!  dump that first line then start over. */
			*n = 0;
			putgrufti("putnote %s %s",nick,p);
			*n = 10;
			p = n + 1;
		}
		else
		{
			/* Search backwards for the last space */
			while((*q != ' ') && (q != p))
				q--;
			if(q == p)
				q = p + LINELENGTH;
			c = *q;
			*q = 0;
			putgrufti("putnote %s %s",nick,p);
			*q = c;
			p = q + 1;
		}
	}

	/* Left with string < LINELENGTH */
	n = strchr(p,10);
	while(n != NULL)
	{
		*n = 0;
		putgrufti("putnote %s %s",nick,p);
		*n = 10;
		p = n + 1;
		n = strchr(p,10);
	}
	if(*p)
		putgrufti("putnote %s %s",nick,p);

	/* all done.  signify EOF */
	putgrufti("putnote <EOF> %s",nick);
}

void Client_UpdateNotes()
{
	char	**_save, **_del;
	char	saved[256], deleted[256];
	int	result;
	int	i;

	saved[0] = 0;
	deleted[0] = 0;

	/* Retrieve saved Notes */
	result = cgiFormStringMultiple("save",&_save);
	if(result != cgiFormNotFound)
	{	
		i = 0;
		while(_save[i])
		{
			sprintf(saved,"%s%s ",saved,_save[i]);
			i++;
		}
	}
	cgiStringArrayFree(_save);

	/* Retrieve deleted Notes */
	result = cgiFormStringMultiple("del",&_del);
	if(result != cgiFormNotFound)
	{	
		i = 0;
		while(_del[i])
		{
			sprintf(deleted,"%s%s ",deleted,_del[i]);
			i++;
		}
	}
	cgiStringArrayFree(_del);

	/* put each string */
	putgrufti("putsavdel sav %s",saved);
	putgrufti("putsavdel del %s",deleted);
	putgrufti("putsavdel <EOF>");
}

void Client_CmdlinePost()
{
	char buf[5096];

	cgiFormStringNoNewlines("cmdline", buf, 5095);
	putgrufti("putcmdline %s", buf);
}

void Client_SetupPost()
{
	char	buf[BUFSIZ], pre[100], retrieve[20];
	char	city[50], state[50], country[50];
	int	i, id;

	/* Feed form information to grufti */
	cgiFormStringNoNewlines("finger",buf,65);
	putgrufti("putsetup finger %s",buf);

	cgiFormStringNoNewlines("email",buf,65);
	putgrufti("putsetup email %s",buf);

	cgiFormStringNoNewlines("www",buf,65);
	putgrufti("putsetup www %s",buf);

	cgiFormStringNoNewlines("bday",buf,65);
	putgrufti("putsetup bday %s",buf);

	cgiFormStringNoNewlines("other1_descript",pre,50);
	cgiFormStringNoNewlines("other1_value",buf,65);
	if(buf[0] && !pre[0])
		strcpy(pre,"other1");
	putgrufti("putsetup other1 %s %s",pre,buf);

	cgiFormStringNoNewlines("other2_descript",pre,50);
	cgiFormStringNoNewlines("other2_value",buf,65);
	if(buf[0] && !pre[0])
		strcpy(pre,"other2");
	putgrufti("putsetup other2 %s %s",pre,buf);

	cgiFormStringNoNewlines("other3_descript",pre,50);
	cgiFormStringNoNewlines("other3_value",buf,65);
	if(buf[0] && !pre[0])
		strcpy(pre,"other3");
	putgrufti("putsetup other3 %s %s",pre,buf);

	cgiFormStringNoNewlines("other4_descript",pre,50);
	cgiFormStringNoNewlines("other4_value",buf,65);
	if(buf[0] && !pre[0])
		strcpy(pre,"other4");
	putgrufti("putsetup other4 %s %s",pre,buf);

	cgiFormStringNoNewlines("other5_descript",pre,50);
	cgiFormStringNoNewlines("other5_value",buf,65);
	if(buf[0] && !pre[0])
		strcpy(pre,"other5");
	putgrufti("putsetup other5 %s %s",pre,buf);

	cgiFormStringNoNewlines("other6_descript",pre,50);
	cgiFormStringNoNewlines("other6_value",buf,65);
	if(buf[0] && !pre[0])
		strcpy(pre,"other6");
	putgrufti("putsetup other6 %s %s",pre,buf);

	cgiFormStringNoNewlines("other7_descript",pre,50);
	cgiFormStringNoNewlines("other7_value",buf,65);
	if(buf[0] && !pre[0])
		strcpy(pre,"other7");
	putgrufti("putsetup other7 %s %s",pre,buf);

	cgiFormStringNoNewlines("other8_descript",pre,50);
	cgiFormStringNoNewlines("other8_value",buf,65);
	if(buf[0] && !pre[0])
		strcpy(pre,"other8");
	putgrufti("putsetup other8 %s %s",pre,buf);

	cgiFormStringNoNewlines("other9_descript",pre,50);
	cgiFormStringNoNewlines("other9_value",buf,65);
	if(buf[0] && !pre[0])
		strcpy(pre,"other9");
	putgrufti("putsetup other9 %s %s",pre,buf);

	cgiFormInteger("location",&id,-1);
	cgiFormStringNoNewlines("city",city,50);
	cgiFormStringNoNewlines("state",state,50);
	cgiFormStringNoNewlines("country",country,50);
	putgrufti("putsetup location %d %s$%s$%s",id,city,state,country);

	for(i=0; i<6; i++)
	{
		sprintf(retrieve,"plan%d",i+1);
		cgiFormStringNoNewlines(retrieve,buf,65);
		putgrufti("putsetup %s %s",retrieve,buf);
	}

	for(i=0; i<10; i++)
	{
		sprintf(retrieve,"greet%d",i+1);
		cgiFormStringNoNewlines(retrieve,buf,160);
		if(i == 0)
			putgrufti("putsetup greet1 %s",buf);
		else
			putgrufti("putsetup greet %s",buf);
	}

	/* All done */
	putgrufti("putsetup <EOF>");
}

void Client_ResponsePost()
{
	char	buf[BUFSIZ], retrieve[20], type[65], response[65];
	int	i, matches, excepts, lines;

	/* Find out what type and response it is */
	cgiFormStringNoNewlines("type",type,65);
	cgiFormStringNoNewlines("response",response,65);

	/* Determine how many of each set we're dealing with */
	cgiFormInteger("num_matches",&matches,0);
	cgiFormInteger("num_excepts",&excepts,0);
	cgiFormInteger("num_lines",&lines,0);

	/* Tell grufti we're ready to initialize this response */
	putgrufti("putresponse %s %s init",type,response);

	/* Now get that number for each set */
	for(i=0; i<matches; i++)
	{
		sprintf(retrieve,"matches%d",i);
		cgiFormStringNoNewlines(retrieve,buf,300);
		putgrufti("putresponse %s %s matches %s",type,response,buf);
	}

	for(i=0; i<excepts; i++)
	{
		sprintf(retrieve,"excepts%d",i);
		cgiFormStringNoNewlines(retrieve,buf,300);
		putgrufti("putresponse %s %s excepts %s",type,response,buf);
	}

	for(i=0; i<lines; i++)
	{
		sprintf(retrieve,"line%d",i);
		cgiFormStringNoNewlines(retrieve,buf,300);
		putgrufti("putresponse %s %s line %s",type,response,buf);
	}

	/* All done */
	putgrufti("putresponse %s %s <EOF>",type,response);
}

void Client_NewResponsePost()
{
	char	type[65], respname[100], *p;

	/* Find out what type and response it is */
	cgiFormStringNoNewlines("type",type,65);
	cgiFormStringNoNewlines("respname",respname,65);

	/* Change response names with spaces to underscore chars */
	for(p=respname; *p; p++)
		if(*p == ' ')
			*p = '_';

	if(respname[0])
	{
		putgrufti("putnewresponse %s %s ",type,respname);
		return;
	}

	putpage("<html><head>");
	putpage("<title>Blank entry</title></head>");
	putpage(Web->html_body);
	putpage("<br><br>");
	putpage("<center>");
	putpage("You need to enter a response NAME if you're going to hit that button!");
	putpage("</center></body></html>");
	exit(0);
}

void Client_EventPost()
{
	char	event[80], when[80], contact[80];

	/* Figure out which event it is */
	cgiFormStringNoNewlines("event",event,65);
	cgiFormStringNoNewlines("when",when,65);
	cgiFormStringNoNewlines("contact",contact,65);

	if(!event[0])
		return;

	putgrufti("putevent when %s %s",event,when[0] ? when : "none");
	putgrufti("putevent contact %s %s",event,contact[0] ? contact : "none");
	putgrufti("putevent <EOF> %s",event);
}

void Client_FingerLookup()
{
	char	buf[20];

	/* Feed form information to grufti */
	cgiFormStringNoNewlines("nick",buf,20);
	putgrufti("putsearch %s",buf);
}

void Client_accesserror()
{
	putpage("<html><head>");
	putpage("<title>Grufti Access Error</title></head>");
	putpage(Web->html_body);
	putpage("<br><br>");
	putpage("<center>");
	putpage("Sorry, you are attempting to access the bot in a way that is not supported.<p>");
	putpage("Please use <a href=\"%s\" target=\"_top\">%s</a> to continue...",Web->main_url,Web->main_url);
	putpage("</center></body></html>");
	exit(0);
}

void Client_error()
{
	putpage("<html><head>");
	putpage("<title>Grufti Error</title></head>");
	putpage(Web->html_body);
	putpage("<br><br>");
	putpage("<center>");
	putpage("<table width=90%% border=0><tr><td>");
	putpage("Got an unexplained error, I can't process your request!");
	putpage("</td></tr></table>");
	putpage("</center></body></html>");
	exit(0);
}

void Client_gruftidown()
{
	putpage("<html><head>");
	putpage("<title>Grufti Down</title></head>");
	putpage(Web->html_body);
	putpage("<br><br>");
	putpage("<center>");
	putpage("<table width=90%% border=0><tr><td>");
	putpage("Sorry, unable to login at this time.<p>");
	putpage("This could mean Grufti is not running, his host machine is down, or he may simply be too busy to process requests.  Try again later.");
	putpage("</td></tr></table>");
	putpage("</center></body></html>");
	exit(0);
}

void Client_blankentry()
{
	putpage("<html><head>");
	putpage("<title>Incorrect entry</title></head>");
	putpage(Web->html_body);
	putpage("<br><br>");
	putpage("<center>");
	putpage("<table width=90%% border=0><tr><td>");
	putpage("You must enter a username and a password to login...<p>");
	putpage("<a href=\"%s\" target=\"_top\">Go back</a>",Web->main_url);
	putpage("</td></tr></table>");
	putpage("</center></body></html>");
	exit(0);
}

void Client_blanknote()
{
	putpage("<html><head>");
	putpage("<title>Incorrect entry</title></head>");
	putpage(Web->html_body);
	putpage("<br><br>");
	putpage("<center>");
	putpage("<table width=90%% border=0><tr><td>");
	putpage("You must specify a Nick and text for the Note.<p>");
	putpage("</td></tr></table>");
	putpage("</center></body></html>");
	exit(0);
}

void fatal(char *s, int x)
{
	exit(0);
}

void putgrufti(char *format, ...)
{
	va_list	va;

	Web->websay_buf[0] = 0;
	va_start(va,format);
	vsprintf(Web->websay_buf,format,va);
	va_end(va);

	tprintf(Web->sock,"%s\n",Web->websay_buf);
}

void putpage(char *format, ...)
{
	va_list va;

	Web->websay_buf[0] = 0;
	va_start(va,format);
	vsprintf(Web->websay_buf,format,va);
	va_end(va);

	fprintf(cgiOut,"%s\n",Web->websay_buf);
}

void putlog(char *format, ...)
{
	va_list va;
	FILE	*f;

	Web->websay_buf[0] = 0;
	va_start(va,format);
	vsprintf(Web->websay_buf,format,va);
	va_end(va);

	/* Open logfile */
	f = fopen(Web->logfile,"a");
	if(f == NULL)
		return;

	fprintf(f,"%s\n",Web->websay_buf);
	fclose(f);
}
