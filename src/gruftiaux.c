/******************************************************************************
 * Steve Rider - stever@area23.org
 * Grufti - an interactive, service-oriented bot for IRC 
 *
 * gruftiaux.c - generic, miscellanous routines specific to Grufti and IRC
 *
 *****************************************************************************/
/* 28 April 97 */

#include "config.h"
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <ctype.h>
#include "misc.h"
#include "grufti.h"
#include "gruftiaux.h"
#include "log.h"
#include "server.h"
#include "user.h"
#include "dcc.h"
#include "command.h"

extern cmdinfo_t cmdinfo;

/* external buffers for sending */
char	say_buf[BUFSIZ];
char	sendmsg_buf[BUFSIZ];
char	sayf_buf[8096];	/* 100 lines of text should be enough */

u_long gruftiaux_sizeof()
{
	u_long	tot = 0L;

	tot += sizeof(say_buf);
	tot += sizeof(sendmsg_buf);
	tot += sizeof(sayf_buf);

	return tot;
}

void mstrcpy(char **dest, char *src)
{
	if(*dest != NULL)
		xfree(*dest);

	if(src == NULL)
	{
		Log_write(LOG_DEBUG,"*","*** Attempting to mstrcpy with NULL src.");
		return;
	}
	*dest = (char *)xmalloc(strlen(src)+1);
	strcpy(*dest,src);
}

void say(char *format, ...)
{
	va_list va;

	say_buf[0] = 0;
	va_start(va,format);
	vsprintf(say_buf,format,va);
	va_end(va);

	/* Say is only used from inside a command loop. */
	if(!cmdinfo.active)
	{
		internal("Oops.  Attempted to 'say' outside of command loop. \"%s\"",say_buf);
		return;
	}

	if(strlen(say_buf) > LINELENGTH)
	{
		Log_write(LOG_DEBUG,"*","Say string longer than LINELENGTH (by %d): \"%s\"",strlen(say_buf) - LINELENGTH,say_buf);
		sayf(0, 0, "%s", say_buf);
		return;
	}

	/* Recipient does not exist over DCC.  Send to nick. */
	if(cmdinfo.d == NULL)
	{
		Server_write(PRIORITY_NORM,"NOTICE %s :%s",cmdinfo.from_n,say_buf);
		return;
	}
	/* Recipient has a DCC record.  Use it. */
	else
		DCC_write(cmdinfo.d, "%s", say_buf);
}

void sayf(int lm, int rm, char *format, ...)
{
	va_list va;
	char *p, *q, *n, c, indent[SCREENWIDTHLEN];
	int i;
	
	sayf_buf[0] = 0;
	va_start(va,format);
	vsprintf(sayf_buf,format,va);
	va_end(va);

	if(cmdinfo.from_n[0] == 0)
	{
		Log_write(LOG_DEBUG,"*","Oops.  Attempted to 'sayf' outside of command loop. \"%s\"",sayf_buf);
		return;
	}

	/* lm = rm = -1 means CENTER it */
	if(lm == -1 && rm == -1)
	{
		lm = (int)((LINELENGTH - strlen(sayf_buf)) / 2);
		if(lm < 0)
			lm = 0;

		rm = 0;
	}

	indent[0] = 0;
	for(i=0; i<lm; i++)
		strcat(indent," ");
		
	p = sayf_buf;

	while(strlen(p) > (LINELENGTH - (lm + rm)))
	{
		q = p + (LINELENGTH - (lm + rm));

		/* Search for embedded linefeed. */
		n = strchr(p,'\n');
		if((n != NULL) && (n < q))
		{
			/* Great!  dump that first line then start over. */
			*n = 0;
			say("%s%s",indent,p);
			*n = '\n';
			p = n + 1;
		}
		else
		{
			/* Search backwards for the last space */
			while((*q != ' ') && (q != p))
				q --;
			if(q == p)
				q = p + (LINELENGTH - (lm + rm));
			c = *q;
			*q = 0;
			say("%s%s",indent,p);
			*q = c;

			p = (c == ' ') ? q + 1 : q;
		}
	}

	/* Left with string < (LINELENGTH - (lm + rm)) */
	n = strchr(p,'\n');
	while(n != NULL)
	{
		*n = 0;
		say("%s%s",indent,p);
		*n = '\n';
		p = n + 1;
		n = strchr(p,'\n');
	}
	if(*p)
		say("%s%s",indent,p);
}

void sayi(char *indent, char *format, ...)
/*
 * Format looks like
 * indent text...
 *        text...
 *        text...
 */
{
	va_list va;
	char *p, *q, *r, *n, c;
	int lm,rm;

	sayf_buf[0] = 0;
	va_start(va,format);
	vsprintf(sayf_buf,format,va);
	va_end(va);

	if(cmdinfo.from_n[0] == 0)
	{
		Log_write(LOG_DEBUG,"*","Oops.  Attempted to 'sayf' outside of command loop. \"%s\"",sayf_buf);
		return;
	}

	lm = strlen(indent);
	rm = 0;
	p = sayf_buf;

	while(strlen(p) > (LINELENGTH - (lm + rm)))
	{
		q = p + (LINELENGTH - (lm + rm));

		/* Search for embedded linefeed. */
		n = strchr(p,'\n');
		if((n != NULL) && (n < q))
		{
			/* Great!  dump that first line then start over. */
			*n = 0;
			say("%s%s",indent,p);
			for(r=indent; *r; r++)
				*r = 32;
			*n = '\n';
			p = n + 1;
		}
		else
		{
			/* Search backwards for the last space */
			while((*q != ' ') && (q != p))
				q --;
			if(q == p)
				q = p + (LINELENGTH - (lm + rm));
			c = *q;
			*q = 0;
			say("%s%s",indent,p);
			for(r=indent; *r; r++)
				*r = 32;
			*q = c;

			p = (c == ' ') ? q + 1 : q;
		}
	}

	/* Left with string < (LINELENGTH - (lm + rm)) */
	n = strchr(p,'\n');
	while(n != NULL)
	{
		*n = 0;
		say("%s%s",indent,p);
		for(r=indent; *r; r++)
			*r = 32;
		*n = '\n';
		p = n + 1;
		n = strchr(p,'\n');
	}
	if(*p)
	{
		say("%s%s",indent,p);
		for(r=indent; *r; r++)
			*r = 32;
	}
}

void sayframe(char *frame, char *format, ...)
{
	va_list va;
	char *p, *q, *n, c;
	char l_spaces[LINELENGTH], r_spaces[LINELENGTH];
	int framelen;

	sayf_buf[0] = 0;
	va_start(va,format);
	vsprintf(sayf_buf,format,va);
	va_end(va);

	if(cmdinfo.from_n[0] == 0)
	{
		Log_write(LOG_DEBUG,"*","Oops.  Attempted to 'sayframe' outside of command loop. \"%s\"",sayf_buf);
		return;
	}

	framelen = 2 * strlen(frame);
	p = sayf_buf;

	while(strlen(p) > (LINELENGTH - (framelen)))
	{
		q = p + (LINELENGTH - (framelen));

		/* Search for embedded linefeed. */
		n = strchr(p,'\n');
		if((n != NULL) && (n < q))
		{
			/* Great!  dump that first line then start over. */
			*n = 0;
			sayframe_helper(frame,framelen,l_spaces,r_spaces,p);
			say("%s%s%s%s%s",frame,l_spaces,p,r_spaces,frame);
			*n = '\n';
			p = n + 1;
		}
		else
		{
			/* Search backwards for the last space */
			while((*q != ' ') && (q != p))
				q --;
			if(q == p)
				q = p + (LINELENGTH - (framelen));
			c = *q;
			*q = 0;
			sayframe_helper(frame,framelen,l_spaces,r_spaces,p);
			say("%s%s%s%s%s",frame,l_spaces,p,r_spaces,frame);
			*q = c;

			p = (c == ' ') ? q + 1 : q;
		}
	}

	/* Left with string < (LINELENGTH - (framelen)) */
	n = strchr(p,'\n');
	while(n != NULL)
	{
		*n = 0;
		sayframe_helper(frame,framelen,l_spaces,r_spaces,p);
		say("%s%s%s%s%s",frame,l_spaces,p,r_spaces,frame);
		*n = '\n';
		p = n + 1;
		n = strchr(p,'\n');
	}
	if(*p)
	{
		sayframe_helper(frame,framelen,l_spaces,r_spaces,p);
		say("%s%s%s%s%s",frame,l_spaces,p,r_spaces,frame);
	}
}

void sayframe_helper(char *frame, int lenf, char *ls, char *rs, char *p)
{
	int lm, rm, lenp, i;

	lenp = strlen(p);
	lm = (int)((LINELENGTH - lenp) / 2) - (int)(lenf / 2);
	rm = LINELENGTH - (lm + lenf + lenp);
	if(lm < 0)
		lm = 0;
	if(rm < 0)
		rm = 0;

	ls[0] = 0;
	for(i=0; i<lm; i++)
		sprintf(ls,"%s ",ls);
	rs[0] = 0;
	for(i=0; i<rm; i++)
		sprintf(rs,"%s ",rs);
}

int invalidpassword(char *password)
{
	int i;

	if(strlen(password) > 9 || strlen(password) < 3)
		return 1;

	for(i=0; password[i]; i++)
		if(!isalnum((int)password[i]))
			if(!strrchr("`~=?/.,><!@#$%&*()|_-[]\\{}^", password[i]))
				return 1;
	return 0;
}

int invalidnick(char *nick)
{
	return invalidacctname(nick);
}

int invalidacctname(char *acctname)
{
	int i;

	if(strlen(acctname) > 9 || strlen(acctname) < 2)
		return 1;

	for(i=0; acctname[i]; i++)
		if((i==0 && !isalpha((int)acctname[i])) || !isalnum((int)acctname[i]))
			if(!strrchr("`|_-[]\\{}^", acctname[i]))
				return 1;
	return 0;
}

void writeheader(FILE *f, char *fileinfo, char *filename)
{
	char created[DATELONGLEN];

	timet_to_date_long(time(NULL),created);

	if(fprintf(f,"###############################################################\n") == EOF)
		return;
	if(fprintf(f,"## %s\n",fileinfo) == EOF)
		return;
	if(fprintf(f,"## File: %s\n",filename) == EOF)
		return;
	if(fprintf(f,"## Created: %s\n",created) == EOF)
		return;
	if(fprintf(f,"## %s\n",Grufti->copyright) == EOF)
		return;
	if(fprintf(f,"###############################################################\n") == EOF)
		return;
}

void fixfrom(char *from)
{
	if(from == NULL)
		return;

	if(strchr("~+-^=",from[0]) != NULL)
		strcpy(from,&from[1]);
}

void catfile(dccrec *d, char *filename)
{
	FILE	*f;
	char	s[BUFSIZ];

	f = fopen(filename,"r");
	if(f == NULL)
	{
		Log_write(LOG_MAIN|LOG_ERROR,"*","No such file \"%s\".",filename);
		DCC_write(d,"Sorry, the owner has not created \"%s\".",filename);
		return;
	}

	if(!isfile(filename))
	{
		fclose(f);
		Log_write(LOG_ERROR,"*","\"%s\" is not a normal file.",filename);
		DCC_write(d,"\"%s\" is not a normal file.",filename);
		return;
	}

	while(readline(f,s))
	{
		DCC_write(d,"%s",s);
	}

	fclose(f);
}

void sendnotice(char *to, char *format, ...)
{
	va_list	va;

	sendmsg_buf[0] = 0;
	va_start(va,format);
	vsprintf(sendmsg_buf,format,va);
	va_end(va);

	Server_write(PRIORITY_NORM,"NOTICE %s :%s",to,sendmsg_buf);
}

void sendprivmsg(char *to, char *format, ...)
{
	va_list	va;

	sendmsg_buf[0] = 0;
	va_start(va,format);
	vsprintf(sendmsg_buf,format,va);
	va_end(va);

	Server_write(PRIORITY_NORM,"PRIVMSG %s :%s",to,sendmsg_buf);
	if(to[0] == '#' || to[0] == '&' || to[0] == '+')
	{
		userrec	*u;

		/* Don't forget to log our public chatter! */
		u = User_user(Server->currentnick);
		if(u->registered)
			u->public_chatter[today()] += strlen(sendmsg_buf);

		Log_write(LOG_PUBLIC,to,"<%s> %s",Server->currentnick,sendmsg_buf);
	}

}

void sendaction(char *to, char *format, ...)
{
	va_list	va;

	sendmsg_buf[0] = 0;
	va_start(va,format);
	vsprintf(sendmsg_buf,format,va);
	va_end(va);

	Server_write(PRIORITY_NORM,"PRIVMSG %s :\001ACTION %s\001",to,sendmsg_buf);
	if(to[0] == '#' || to[0] == '&' || to[0] == '+')
	{
		userrec	*u;

		/* Don't forget to log our public chatter! */
		u = User_user(Server->currentnick);
		if(u->registered)
			u->public_chatter[today()] += strlen(sendmsg_buf);

		Log_write(LOG_PUBLIC,to,"* %s %s",Server->currentnick,sendmsg_buf);
	}
}

char *my_strcasestr(char *haystack, char *needle)
{
	char	*p_haystack, *p_needle, *q, in_needle = 0, a, b;

	p_needle = needle;
	q = haystack;
	for(p_haystack=haystack; *p_haystack; p_haystack++)
	{
		/* We don't want significant case */
		a = tolower(*p_needle);
		b = tolower(*p_haystack);
		if(a == b)
		{
			if(!in_needle)
			{
				in_needle = 1;

				/* Remember start of needle in haystack */
				q = p_haystack;
			}

			/* push needle to next character */
			p_needle++;

			/* If needle is \n, then we got a whole needle! */
			if(*p_needle == 0)
			{
				/* we got it! */
				return q;
			}
		}
		else
		{
			/* They're not equal.  Rewind needle, and reset q */
			if(in_needle)
			{
				p_needle = needle;
				in_needle = 0;
			}
		}
	}

	return NULL;
}

void verify_write(int x, char *srcfile, char *destfile)
{
	if(x == 1)
		Log_write(LOG_ERROR,"*","Can't read \"%s\".",srcfile);
	else if(x == 2)
		Log_write(LOG_ERROR,"*","Can't open \"%s\" for writing, trying to read from \"%s\".",destfile,srcfile);
	else if(x == 3)
		Log_write(LOG_ERROR,"*","Can't read \"%s\".  Not a normal file.",srcfile);
	else if(x == 4)
		Log_write(LOG_ERROR,"*","Ran out of room trying to write \"%s\"! (still holding \"%s\"...)",destfile,srcfile);
}

time_t timestr_to_secs(char *str)
{
	char	expires[256], type;
	time_t	expire_time;

	strcpy(expires,str);
	type = expires[strlen(expires) - 1];

	if(!strchr("dmsh",type))
		return 0L;

	expires[strlen(expires) - 1] = 0;
	expire_time = (time_t)atol(expires);
	if(expire_time == 0L)
		return 0L;

	if(type == 'd')
		expire_time *= 86400;
	else if(type == 'h')
		expire_time *= 3600;
	else if(type == 'm')
		expire_time *= 60;

	return expire_time;
}

void killpercentchars(char *s)
{
	char	*p;

	for(p=s; *p; p++)
		if(*p == '%')
			*p = ' ';
}
