#ifdef WEB_MODULE
/******************************************************************************
 * Steve Rider - stever@area23.org
 * Grufti - an interactive, service-oriented bot for IRC 
 *
 * web.c - module-like web interface
 *
 *****************************************************************************/
/* 28 April 97 */

#include "config.h"
#include "../webconfig.h"
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include "grufti.h"
#include "gruftiaux.h"
#include "log.h"
#include "user.h"
#include "channel.h"
#include "command.h"
#include "dcc.h"
#include "misc.h"
#include "webcmd.h" /* temporary until command files get moved around */
#include "web.h"


/* record cmdrec_t low-level data-specific function declarations. */
static void	cmdrec_init(cmdrec_t *c);
static u_long	cmdrec_sizeof(cmdrec_t *c);
static void	cmdrec_free(cmdrec_t *c);
static void	cmdrec_setname(cmdrec_t *c, char *name);
static void	cmdrec_setfunction(cmdrec_t *c, void (*function)());
static void	cmdrec_display(cmdrec_t *c);
static cmdrec_t  *cmdrec_cmd(cmdrec_t **list, cmdrec_t **last, char *name);

/* record cmdrec_t low-level function declarations. */
static cmdrec_t	*cmdrec_new();
static void	cmdrec_freeall(cmdrec_t **cmdlist, cmdrec_t **last);
static void	cmdrec_append(cmdrec_t **list, cmdrec_t **last, cmdrec_t *entry);
static void	cmdrec_delete(cmdrec_t **list, cmdrec_t **last, cmdrec_t *x);
static void	cmdrec_movetofront(cmdrec_t **list, cmdrec_t **last, cmdrec_t *lastchecked, cmdrec_t *c);


cmdrec_fixed_t webcmd_base_tbl[] =
{
/*	Command			function		levels	DCC?	*/
/*	=====================	=====================	======	======	*/
	{ "intro",		NULL, webcmd_intro,		"-",	FALSE	},
	{ "login",		NULL, webcmd_login,		"-",	FALSE	},
	{ "reply",		NULL, webcmd_reply,		"-",	FALSE	},
	{ "note",		NULL, webcmd_note,		"-",	FALSE	},
	{ "putnote",	NULL, 	webcmd_putnote,		"-",	FALSE	},
	{ "putsavdel",	NULL, 	webcmd_putsavdel,	"-",	FALSE	},
	{ "rn",			NULL, webcmd_rn,		"-",	FALSE	},
	{ "dn",			NULL, webcmd_dn,		"-",	FALSE	},
	{ "read",		NULL, webcmd_read,		"-",	FALSE	},
	{ "putsearch",	NULL, 	webcmd_putsearch,	"-",	FALSE	},
	{ "users",		NULL, webcmd_users,		"-",	FALSE	},
	{ "finger",		NULL, webcmd_finger,		"-",	FALSE	},
	{ "channel",	NULL, 	webcmd_channel,		"-",	FALSE	},
	{ "cmdline",	NULL,	webcmd_cmdline,		"-",	FALSE	},
	{ "setup",		NULL, webcmd_setup,		"-",	FALSE	},
	{ "putcmdline",	NULL, 	webcmd_putcmdline,	"-",	FALSE	},
	{ "putsetup",	NULL, 	webcmd_putsetup,	"-",	FALSE	},
	{ "response",	NULL, 	webcmd_response,	"-",	FALSE	},
	{ "putnewresponse",	NULL, webcmd_putnewresponse,	"-",	FALSE	},
	{ "vresponse",		NULL, webcmd_vresponse,	"-",	FALSE	},
	{ "dresponse",		NULL, webcmd_dresponse,	"-",	FALSE	},
	{ "putresponse",	NULL, webcmd_putresponse,	"-",	FALSE	},
	{ "webcatch",		NULL, webcmd_webcatch,	"-",	FALSE	},
	{ "status",		NULL, webcmd_stat,		"-",	FALSE	},
	{ "showevents",		NULL, webcmd_showevents,	"-",	FALSE	},
	{ "showevent",		NULL, webcmd_showevent,	"-",	FALSE	},
	{ "putevent",		NULL, webcmd_putevent,	"-",	FALSE	},
	{ "unattendevent",	NULL, webcmd_unattendevent,	"-",	FALSE	},
	/*
	 :
	 */
	{ NULL,			NULL, null(void(*)()),	NULL,	FALSE	},
};

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
	Web = (Web_ob *)xmalloc(sizeof(Web_ob));

	/* initialize */
	Web->commands = NULL;
	Web->last = NULL;
	Web->num_commands = 0;
	Web->cmdinuse = 0;
	Web->from_n[0] = 0;
	Web->from_uh[0] = 0;
	Web->cmdname[0] = 0;
	Web->cmdparam[0] = 0;
	Web->user = NULL;
	Web->account = NULL;
	Web->d = NULL;
	Web->tok1[0] = 0;
	Web->tok2[0] = 0;
	Web->tok3[0] = 0;
	Web->istok1 = 0;
	Web->istok2 = 0;
	Web->istok3 = 0;
	Web->num_elements = 0;
	Web->paramlen = 0;
	Web->isparam = 0;
	Web->flags = 0L;

	Web->websay_buf[0] = 0;
	Web->display[0] = 0;

	strcpy(Web->title,WEB_TITLE);
	strcpy(Web->main_url,WEB_MAIN_URL);
	strcpy(Web->welcome_image,WEB_WELCOME_IMAGE);
	Web->welcome_image_width = WEB_WELCOME_IMAGE_WIDTH;
	Web->welcome_image_height = WEB_WELCOME_IMAGE_HEIGHT;
	strcpy(Web->cgi_url,WEB_CGI_URL);
	strcpy(Web->bgcolor,WEB_BGCOLOR);
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
	if(Web->bgcolor[0])
		sprintf(Web->html_body,"%s bgcolor=\"%s\"",Web->html_body,Web->bgcolor);
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
}

u_long Web_sizeof()
{
	u_long	tot = 0L;

	return tot;
}

void Web_freeall()
{
	/* Free the object */
	xfree(Web);
	Web = NULL;
}

/****************************************************************************/
/* low-level Web structure function declarations. */

void Web_appendcmd(cmdrec_t *c)
{
	cmdrec_append(&Web->commands, &Web->last, c);
	Web->num_commands++;
}

void Web_deletecmd(cmdrec_t *c)
{
	cmdrec_delete(&Web->commands, &Web->last, c);
	Web->num_commands--;
}

void Web_freeallcmds()
{
	cmdrec_freeall(&Web->commands, &Web->last);
	Web->num_commands = 0;
}


/****************************************************************************/
/* object Web high-level implementation-specific function definitions. */

void Web_loadcommands()
{
	int	i;
	cmdrec_t	*newcmd;

	for(i=0; webcmd_base_tbl[i].name != NULL; i++)
	{
		/* create a new node */
		newcmd = cmdrec_new();

		/* copy data */
		cmdrec_setname(newcmd,webcmd_base_tbl[i].name);
		cmdrec_setfunction(newcmd,webcmd_base_tbl[i].function);

		User_str2flags(webcmd_base_tbl[i].levels,&newcmd->levels);
		if(newcmd->levels & USER_INVALID)
			Log_write(LOG_ERROR,"*","Warning: web command \"%s\" has invalid levels \"%s\".",webcmd_base_tbl[i].name,webcmd_base_tbl[i].levels);

		/* we don't care about dcc requirements here */
		newcmd->needsdcc = FALSE;

		/* Append it to the web commands list */
		Web_appendcmd(newcmd);
	}
}

void Web_docommand(dccrec *d, char *buf)
{
	cmdrec_t	*wc;

	/* fix buf so we don't get empty fields */
	rmspace(buf);

	/* Web command variables must be preset */
	strcpy(Web->from_n,d->nick);
	strcpy(Web->from_uh,d->uh);
	split(Web->cmdname,buf);
	strcpy(Web->cmdparam,buf);

	/* Set DCC connection record */
	Web->d = d;

	/* Set user and account pointers */
	Web->user = d->user;
	Web->account = d->account;

	/* Now we're ready */
	if(Web->cmdname[0] == 0)
	{
		Log_write(LOG_DEBUG,"*","Web_docommand() invoked with empty cmdname.");
		return;
	}

	/* Mark command as active */
	Web->cmdinuse = 1;

	/* Find the command */
	wc = Web_cmd(Web->cmdname);
	if(wc == NULL)
	{
		/* Not found. */
		websay("No such command \"%s\".",Web->cmdname);
		Web->cmdinuse = 0;
		Web_reset();
		return;
	}

	/* Check levels */
	if(wc->levels != 0L)
	{
		if(!(Web->user->flags & wc->levels))
		{
			char	flags[USERFLAGLEN];
	
			User_flags2str(wc->levels,flags);
			websay("That command requires +%s and you don't have it.  Go away.",flags);
			Web->cmdinuse = 0;
			Web_reset();
			return;
		}
	}

	/* Ok here we go */
	str_element(Web->tok1,Web->cmdparam,1);
	str_element(Web->tok2,Web->cmdparam,2);
	str_element(Web->tok3,Web->cmdparam,3);
	Web->istok1 = Web->tok1[0] ? 1 : 0;
	Web->istok2 = Web->tok2[0] ? 1 : 0;
	Web->istok3 = Web->tok3[0] ? 1 : 0;
	Web->num_elements = num_elements(Web->cmdparam);
	Web->paramlen = strlen(Web->cmdparam);
	Web->isparam = Web->cmdparam[0] ? 1 : 0;

	/* Execute command */
	wc->accessed++;
	wc->function();

	/* Reset */
	Web_reset();
	Web->cmdinuse = 0;
}

void Web_reset()
{
	Web->from_n[0] = 0;
	Web->from_uh[0] = 0;
	Web->cmdname[0] = 0;
	Web->cmdparam[0] = 0;
	Web->user = NULL;
	Web->account = NULL;
	Web->d = NULL;
	Web->tok1[0] = 0;
	Web->tok2[0] = 0;
	Web->tok3[0] = 0;
	Web->istok1 = 0;
	Web->istok2 = 0;
	Web->istok3 = 0;
	Web->num_elements = 0;
	Web->paramlen = 0;
	Web->isparam = 0;
}

cmdrec_t *Web_cmd(char *name)
{
	return cmdrec_cmd(&Web->commands, &Web->last, name);
}

void websay(char *format, ...)
{
	va_list	va;

	Web->websay_buf[0] = 0;
	va_start(va,format);
	vsprintf(Web->websay_buf,format,va);
	va_end(va);

	/* Web say is used only from inside command loop */
	if(!Web->cmdinuse)
	{
		Log_write(LOG_DEBUG,"*","Attempted to 'websay' outside of command loop. \"%s\"",Web->websay_buf);
		return;
	}

	if(Web->d == NULL)
	{
		Log_write(LOG_ERROR,"*","Recipient has no DCC record! \"%s\"",Web->websay_buf);
		return;
	}

	DCC_write(Web->d, "%s", Web->websay_buf);
}

void Web_saynotregistered(dccrec *d, char *nick)
{
	DCC_write(d,"<html><head>");
	DCC_write(d,"<title>%s - Invalid Nick</title></head>",Web->title);
	DCC_write(d,"%s",Web->html_body);
	DCC_write(d,"<br><br>");
	DCC_write(d,"<center>");
	DCC_write(d,"Sorry, the nick \"%s\" does not belong to a registered user.<br>",nick);
	DCC_write(d,"Click <a href=\"%s/register.html\">here</a> to find out how you can become a registered user.<p>",Web->main_url);
	DCC_write(d,"<a href=\"%s\">Go back</a>",Web->main_url);
	DCC_write(d,"</center></body></html>");
}

void Web_sayinvalidpassword(dccrec *d, char *nick)
{
	DCC_write(d,"<html><head>");
	DCC_write(d,"<title>%s - Invalid Password</title></head>",Web->title);
	DCC_write(d,"%s",Web->html_body);
	DCC_write(d,"<br><br>");
	DCC_write(d,"<center>");
	DCC_write(d,"Sorry, that is not the correct password for %s.<p>",nick);
	DCC_write(d,"<a href=\"%s\">Go back</a>",Web->main_url);
	DCC_write(d,"</center></body></html>");
}

void Web_fixforhtml(char *s)
{
	char	*p, out[BUFSIZ];
	int	bold_on = 0;

	out[0] = 0;
	if(s == NULL)
		return;

	/* We look for ", <, and >, stripping control codes, and replacing ^b with html tags */
	for(p=s; *p; p++)
	{
		if(*p == '"')
			strcat(out,"&quot;");
		else if(*p == '<')
			strcat(out,"&lt;");
		else if(*p == '>')
			strcat(out,"&gt;");
		else if(*p > 31)
			sprintf(out,"%s%c",out,*p);
		else if(*p == 2)
		{
			if(bold_on)
				strcat(out,"</font>");
			else
				sprintf(out,"%s<font color=\"%s\">",out,Web->color3);
			bold_on = 1 - bold_on;
		}
			 
		else if(*p == 13)
			sprintf(out,"%s\n",out);
	}
	if(bold_on)
		strcat(out,"</font>");

	strcpy(s, out);
}

void Web_fixforinput(char *in, char *out)
{
	char	*p;

	out[0] = 0;
	if(in == NULL)
		return;

	/* We look for ", <, and > */
	for(p=in; *p; p++)
	{
		if(*p == '"')
			strcat(out,"&quot;");
		else if(*p == '<')
			strcat(out,"&lt;");
		else if(*p == '>')
			strcat(out,"&gt;");
		else 
			sprintf(out,"%s%c",out,*p);
	}
}

void Web_catfile(dccrec *d, char *filename)
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
		Web_fixforhtml(s);
		DCC_write(d,"%s",s);
	}

	fclose(f);
}

/* ASDF */
/****************************************************************************
 *
 * cmd record definitions.  No changes should be made to new(), freeall(),
 * append(), delete(), or movetofront().
 *
 ****************************************************************************/

static cmdrec_t *cmdrec_new()
{
	cmdrec_t *x;

	/* allocate memory */
	x = (cmdrec_t *)xmalloc(sizeof(cmdrec_t));

	/* initialize */
	x->next = NULL;

	cmdrec_init(x);

	return x;
}

static void cmdrec_freeall(cmdrec_t **cmdlist, cmdrec_t **last)
{
	cmdrec_t *c = *cmdlist, *v;

	while(c != NULL)
	{
		v = c->next;
		cmdrec_free(c);
		c = v;
	}

	*cmdlist = NULL;
	*last = NULL;
}

static void cmdrec_append(cmdrec_t **list, cmdrec_t **last, cmdrec_t *entry)
{
	cmdrec_t *lastentry = *last;

	if(*list == NULL)
		*list = entry;
	else
		lastentry->next = entry;

	*last = entry;
	entry->next = NULL;
}

static void cmdrec_delete(cmdrec_t **list, cmdrec_t **last, cmdrec_t *x)
{
	cmdrec_t *c, *lastchecked = NULL;

	c = *list;
	while(c != NULL)
	{
		if(c == x)
		{
			if(lastchecked == NULL)
			{
				*list = c->next;
				if(c == *last)
					*last = NULL;
			}
			else
			{
				lastchecked->next = c->next;
				if(c == *last)
					*last = lastchecked;
			}

			cmdrec_free(c);
			return;
		}

		lastchecked = c;
		c = c->next;
	}
}

static void cmdrec_movetofront(cmdrec_t **list, cmdrec_t **last, cmdrec_t *lastchecked, cmdrec_t *c)
{
	if(lastchecked != NULL)
	{
		if(*last == c)
			*last = lastchecked;

		lastchecked->next = c->next;
		c->next = *list;
		*list = c;
	}
}

/****************************************************************************/
/* record cmdrec_t low-level data-specific function definitions. */

static void cmdrec_init(cmdrec_t *c)
{
	/* initialize */
	c->name = NULL;
	c->function = null(void(*)());
	c->levels = 0L;
	c->needsdcc = 0;
	c->accessed = 0;
	c->order = 0;
}

static u_long cmdrec_sizeof(cmdrec_t *c)
{
	u_long	tot = 0L;

	tot += sizeof(cmdrec_t);
	tot += c->name ? strlen(c->name)+1 : 0L;

	return tot;
}

static void cmdrec_free(cmdrec_t *c)
{
	/* Free the elements. */
	xfree(c->name);

	/* Free this record. */
	xfree(c);
}

static void cmdrec_setname(cmdrec_t *c, char *name)
{
	mstrcpy(&c->name,name);
}

static void cmdrec_setfunction(cmdrec_t *c, void (*function)())
{
	c->function = function;
}
			
static void cmdrec_display(cmdrec_t *c)
{
	char flags[USERFLAGLEN];

	User_flags2str(c->levels,flags);
	say("%-20s %-10s %-10s %4d",c->name,flags,c->needsdcc?"TRUE":"FALSE",c->accessed);
}

static cmdrec_t *cmdrec_cmd(cmdrec_t **list, cmdrec_t **last, char *name)
{
	cmdrec_t *c, *lastchecked = NULL;

	for(c=*list; c!=NULL; c=c->next)
	{
		if(isequal(c->name,name))
		{
			/* found it.  do a movetofront. */
			cmdrec_movetofront(list,last,lastchecked,c);

			return c;
		}
		lastchecked = c;
	}

	return NULL;
}

#endif /* WEB_MODULE */
