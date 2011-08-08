/******************************************************************************
 * Steve Rider - stever@area23.org
 * Grufti - an interactive, service-oriented bot for IRC 
 *
 * webcmd.c - module-like web interface - the commands
 *
 *****************************************************************************/
/* 28 April 97 */

#ifdef WEB_MODULE
#include "config.h"
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
#include "dcc.h"
#include "misc.h"
#include "command.h"
#include "web.h"
#include "location.h"
#include "note.h"
#include "response.h"
#include "url.h"
#include "server.h"
#include "webcmd.h"
#include "event.h"

/* damn kludge to get the flags for the 'addrespline' command. */
extern struct listmgr_t *commands;

void openhtml(char *title)
{
	websay("<html><head>");
	websay("<title>%s</title></head>",title);
	websay(Web->html_body);
}

void closehtml()
{
	websay("</body></html>");
}

void webcmd_intro()
{
	openhtml("intro");
	websay("hi");
	closehtml();
}

void websay_statusline()
{
	char	today[TIMELONGLEN];
	char	notes[512];
	int	new, new_notenum;

	/* Open table */
	websay("");
	websay("<table width=570 border=0 cellspacing=0 cellpadding=1><tr>");
	websay("<td bgcolor=\"%s\" width=188>",Web->color1);
	websay("<font face=\"arial\" size=\"2\"><b>%s</b></font></td>",Web->user->acctname);

	
	/* Display number of Notes */
	websay("<td bgcolor=\"%s\" width=188 align=center>",Web->color1);
	new = Note_numnew(Web->user);
	if(new)
	{
		new_notenum = Note_numnewnote(Web->user);
		sprintf(notes,"%d <a href=\"%s?%s;%s;RN:%d\">new</a> Note%s (%d total)",new,Web->cgi_url,Web->user->acctname,Web->user->pass,new_notenum,new==1?"":"s",Web->user->num_notes);
	}
	else
		sprintf(notes,"%d Note%s",Web->user->num_notes,Web->user->num_notes==1?"":"s");
	websay("<font face=\"arial\" size=\"2\"><b>%s</b></font></td>",notes);
	websay("<td bgcolor=\"%s\" width=188 align=right>",Web->color1);

	/* Display date */
	timet_to_date_long(time(NULL),today);
	websay("<font face=\"arial\" size=\"2\"><b>%s</b></font></td>",today);

	/* Close table */
	websay("</tr></table>");
	websay("");
}

void websay_menuline(char *refresh)
{
	chanrec	*chan;

	websay("");
	websay("<table width=570 border=0 cellspacing=0 cellpadding=1>");
	websay("<tr><td width=518>");
	websay("<font face=\"arial\" size=\"1\">");
	websay("<a href=\"%s?%s;%s;Login\">Main</a> |",Web->cgi_url,Web->user->acctname,Web->user->pass);
	websay("<a href=\"%s?%s;%s;Read\">Notes</a> |",Web->cgi_url,Web->user->acctname,Web->user->pass);
	for(chan=Channel->channels; chan!=NULL; chan=chan->next)
		websay("<a href=\"%s?%s;%s;Channel:%s\">%s</a> |",Web->cgi_url,Web->user->acctname,Web->user->pass,chan->name+1,chan->name);
	websay("<a href=\"%s?%s;%s;Setup\">Setup</a> |",Web->cgi_url,Web->user->acctname,Web->user->pass);
	websay("<a href=\"%s?%s;%s;Users\">Users</a> |",Web->cgi_url,Web->user->acctname,Web->user->pass);
	websay("<a href=\"%s?%s;%s;Response\">Responses</a> |",Web->cgi_url,Web->user->acctname,Web->user->pass);
	websay("<a href=\"%s?%s;%s;Showevents\">Events</a> |",Web->cgi_url,Web->user->acctname,Web->user->pass);
	websay("<a href=\"%s?%s;%s;Status\">Status</a> |",Web->cgi_url,Web->user->acctname,Web->user->pass);
	websay("<a href=\"%s?%s;%s;Webcatch\">Webcatch</a> |",Web->cgi_url,Web->user->acctname,Web->user->pass);
	websay("<a href=\"%s?%s;%s;Cmdline\">Cmdline</a>",Web->cgi_url,Web->user->acctname,Web->user->pass);
	websay("</font>");
	websay("</td><td width=48 align=right>");
	websay("<font face=\"arial\" size=\"1\">");
	websay("<a href=\"%s?%s;%s;%s\">Refresh</a>",Web->cgi_url,Web->user->acctname,Web->user->pass,refresh);
	websay("</font>");
	websay("</td></tr></table>");
	websay("");
}

void websay_bar(char *str)
{
	websay("<table width=570 border=0 cellspacing=0 cellpadding=2><tr>");
	if(str[0])
	{
		websay("<td bgcolor=\"%s\"><font face=\"arial\" size=\"2\">",Web->color1);
		websay("<b>%s</b>",str);
	}
	else
	{
		websay("<td bgcolor=\"%s\"><font face=\"arial\" size=\"2\" color=\"%s\">",Web->color1,Web->color1);
		websay(".");
	}
	websay("</font></td></tr></table>");
}

void websay_nicksearchinput()
{
	websay("<form method=\"post\" action=\"%s?%s;%s;!finger_lookup\">",Web->cgi_url,Web->user->acctname,Web->user->pass);
	websay("<input type=\"text\" name=\"nick\" value=\"\" size=9 maxlength=9>");
	websay("<input type=\"submit\" value=\"Find nick\">");
}

void webcmd_login()
{
	locarec	*l;
	chanrec	*chan;
	int	new;
	char	motd[256], mod[256];

	/* Display headers, statusline, and menu */
	sprintf(Web->display,"%s - Welcome",Web->title);
	openhtml(Web->display);
	if(Web->welcome_image[0])
		websay("<img src=\"%s\" width=%d height=%d alt=\"Welcome to %s\"><br>",Web->welcome_image,Web->welcome_image_width,Web->welcome_image_height,Web->title);
	websay_statusline();
	websay_menuline("Login");
	websay("<br>");


	/* Display greeting */
	websay("<table width=570 border=0 cellspacing=0 cellpadding=1>");
	websay("<tr><td width=48><br></td><td width=520>");
	l = Location_location(Web->user->location);
	if(l != NULL)
		websay("Hello %s in %s! ",Web->user->acctname,l->city);
	else
		websay("Hello %s! ",Web->user->acctname);

	/* Display number of Notes */
	new = Note_numnew(Web->user);
	if(new)
		websay("You have %d new Note%s. (%d total)<br>",new,new==1?"":"s",Web->user->num_notes);
	else
		websay("You have %d Note%s.<br>",Web->user->num_notes,Web->user->num_notes==1?"":"s");

	if(Web->user->email == NULL)
		websay("You have no email address set!  Click <a href=\"%s?%s;%s;Setup\">Setup</a> to setup your account!<br>",Web->cgi_url,Web->user->acctname,Web->user->pass);

	websay("");
	websay("<table width=520 border=0 cellspacing=10 cellpadding=0><tr>");
	websay("<td valign=top>");
	websay("<font size=\"4\" color=\"%s\">Events</font><br>",Web->color2);
	websay("<font size=\"2\">");
	websay("<font color=\"%s\">....</font>",Web->bgcolor);
	websay("%s's <a href=\"%s?%s;%s;ShowEvents\">Events</a> Calendar<br>",Grufti->botnick,Web->cgi_url,Web->user->acctname,Web->user->pass);
	websay("</td>");
	websay("<td  width=160 valign=top>");
	websay("<font size=\"4\" color=\"%s\">Channel</font><br>",Web->color2);
	websay("<font size=\"2\">");
	for(chan=Channel->channels; chan!=NULL; chan=chan->next)
	{
		websay("<font color=\"%s\">....</font>",Web->bgcolor);
		websay("Who's on <a href=\"%s?%s;%s;Channel:%s\">%s</a><br>",Web->cgi_url,Web->user->acctname,Web->user->pass,chan->name+1,chan->name);
	}
	websay("</font>");

	websay("</td><td width=160 valign=top>");
	websay("<font size=\"4\" color=\"%s\">Command-Line</font><br>", Web->color2);
	websay("<font size=\"2\">");
	websay("<font color=\"%s\">....</font>", Web->bgcolor);
	websay("<a href=\"%s?%s;%s;Cmdline\">Enter</a> commands directly<br>", Web->cgi_url, Web->user->acctname, Web->user->pass);
	websay("</font>");
	websay("</td></tr>");

	websay("<tr><td width=160 valign=top>");

	websay("<font size=\"4\" color=\"%s\">Setup</font><br>",Web->color2);
	websay("<font size=\"2\">");
	websay("<font color=\"%s\">....</font>",Web->bgcolor);
	websay("<a href=\"%s?%s;%s;Setup\">Setup</a> your account<br>",Web->cgi_url,Web->user->acctname,Web->user->pass);
	websay("</font>");

	websay("</td><td width=160 valign=top>");

	websay("<font size=\"4\" color=\"%s\">Notes</font><br>",Web->color2);
	websay("<font size=\"2\">");
	websay("<font color=\"%s\">....</font>",Web->bgcolor);
	websay("<a href=\"%s?%s;%s;Read\">Read</a> your Notes<br>",Web->cgi_url,Web->user->acctname,Web->user->pass);
	websay("<font color=\"%s\">....</font>",Web->bgcolor);
	websay("<a href=\"%s?%s;%s;Note\">Send</a> a Note<br>",Web->cgi_url,Web->user->acctname,Web->user->pass);
	websay("</font>");

	websay("</td><td width=160 valign=top>");

	websay("<font size=\"4\" color=\"%s\">Finger</font><br>",Web->color2);
	websay("<font size=\"2\">");
	websay("<font color=\"%s\">....</font>",Web->bgcolor);
	websay("<a href=\"%s?%s;%s;Users\">Finger</a> a user<br>",Web->cgi_url,Web->user->acctname,Web->user->pass);
	websay("</font>");

	websay("</td></tr><tr><td width=160 valign=top>");

	websay("<font size=\"4\" color=\"%s\">Bot Status</font><br>",Web->color2);
	websay("<font size=\"2\">");
	websay("<font color=\"%s\">....</font>",Web->bgcolor);
	websay("View %s's <a href=\"%s?%s;%s;Status\">Status</a><br>",Grufti->botnick,Web->cgi_url,Web->user->acctname,Web->user->pass);
	websay("</font>");

	websay("</td><td width=160 valign=top>");

	websay("<font size=\"4\" color=\"%s\">Response</font><br>",Web->color2);
	websay("<font size=\"2\">");
	websay("<font color=\"%s\">....</font>",Web->bgcolor);
	websay("View a %s <a href=\"%s?%s;%s;Response\">Response</a><br>",Grufti->botnick,Web->cgi_url,Web->user->acctname,Web->user->pass);
	websay("</font>");

	websay("</td><td width=160 valign=top>");

	websay("<font size=\"4\" color=\"%s\">URL Catcher</font><br>",Web->color2);
	websay("<font size=\"2\">");
	websay("<font color=\"%s\">....</font>",Web->bgcolor);
	websay("View the list of <a href=\"%s?%s;%s;Webcatch\">URL's</a><br>",Web->cgi_url,Web->user->acctname,Web->user->pass);
	websay("</font>");

	websay("</td></tr></table>");
	websay("</td></tr></table>");

	websay("");
	websay("<br>");
	timet_to_date_short(Grufti->motd_modify_date,mod);
	sprintf(Web->display,"MOTD (Last modified: %s)",mod);
	websay_bar(Web->display);
	websay("");
	websay("<table width=570 border=0 cellspacing=0 cellpadding=2>");
	websay("<tr><td width=48><br></td><td width=518>");

	/* Show motd */
	websay("<pre>");
	sprintf(motd,"%s/%s",Grufti->admindir,Grufti->motd_telnet);
	Web_catfile(Web->d,motd);
	websay("</pre>");

	websay("</td></tr></table>");
	websay("</td></tr></table>");
	closehtml();
}

void webcmd_reply()
{
	char	tmp[BUFSIZ];
	int	num;
	noterec	*n;
	nbody	*body;

	/* Display headers, statusline, and menu */
	sprintf(tmp,"%s - Note %s",Web->title,Web->tok1);
	openhtml(tmp);
	websay_statusline();
	sprintf(tmp,"Reply:%s",Web->tok1);
	websay_menuline(tmp);
	websay("<br><br>");
	websay("");

	websay("<form method=\"post\" action=\"%s?%s;%s;!note_post\">",Web->cgi_url,Web->user->acctname,Web->user->pass);

	/* We do this just like Note below, but we show original. */
	num = atoi(Web->tok1);
	n = Note_notebynumber(Web->user,num);
	if(n == NULL)
	{
		websay("Note %d doesn't exist anymore!",num);
		closehtml();
		return;
	}

	/* Draw Note */
	websay("<font face=\"arial\" size=\"2\"><b><a href=\"%s?%s;%s;Finger:%s\">%s</a> said:</b></font><br>",Web->cgi_url,Web->user->acctname,Web->user->pass,n->nick,n->nick);
	websay("<table width=570 border=0><tr><td>");
	for(body=n->body; body!=NULL; body=body->next)
	{
		if(body->text[0])
		{
			strcpy(tmp, body->text);
			Web_fixforhtml(tmp);
			websay("%s",tmp);
		}
		else
			websay("<p>");
	}
	websay("</td></tr></table>");
	websay("<br>");
	websay("");

	websay("<input type=\"hidden\" name=\"nick\" value=\"%s\">",n->nick);
	websay("<font face=\"arial\" size=\"2\"><b>Your reply:</b></font>");
	websay("<br>");
	websay("<textarea wrap=\"virtual\" name=\"body\" rows=7 cols=67></textarea><p>");
	websay("<input type=\"submit\" value=\"Send Note\"> <input type=\"reset\" value=\"Clear\"><br><br");

	closehtml();
}
	
void webcmd_note()
{
	char	tmp[BUFSIZ];

	/* Display headers, statusline, and menu */
	sprintf(tmp,"%s - Note %s",Web->title,Web->tok1);
	openhtml(tmp);
	websay_statusline();
	if(Web->istok1)
		sprintf(tmp,"Note:%s",Web->tok1);
	else
		strcpy(tmp,"Note");
	websay_menuline(tmp);
	websay("<form method=\"post\" action=\"%s?%s;%s;!note_post\">",Web->cgi_url,Web->user->acctname,Web->user->pass);
	websay("<br>");

	if(!Web->istok1)
	{
		websay("<table width=570 border=0><tr><td>");
		websay("<font size=\"2\">");
		websay("Enter the nick for whom you want to send the Note.  The nick must belong to a registered user.  See the <a href=\"%s?%s;%s;Users\">Users</a> page for a listing of all registered users.",Web->cgi_url,Web->user->acctname,Web->user->pass);
		websay("</font>");
		websay("</td></tr></table>");

		websay("<br>");
		websay("");


		websay("<font face=\"arial\" size=\"2\"><b>Nick: </b></font><input type=\"text\" name=\"nick\" value=\"%s\" size=20 maxlength=20><p>",Web->tok1);
	}
	else
	{
		websay("Enter your Note to %s here:<br>",Web->tok1);
		websay("<input type=\"hidden\" name=\"nick\" value=\"%s\">",Web->tok1);
		websay("<br>");
		websay("");
	}

	websay("<textarea wrap=\"virtual\" name=\"body\" rows=7 cols=67></textarea><p>");
	websay("<input type=\"submit\" value=\"Send Note\"> <input type=\"reset\" value=\"Clear\"><br><br");

	closehtml();
}

void webcmd_putnote()
{
	userrec	*u;
	noterec	*n;
	char	flags[BUFSIZ];

	if(!Web->istok1)
		return;

	DCC_flags2str(Web->d->flags,flags);
	Log_write(LOG_RAW,"*","Checking for DONTSHOWPROMPT");
	Log_write(LOG_RAW,"*","flags: %s",flags);
	if(!(Web->d->flags & STAT_DONTSHOWPROMPT))
	{
		Log_write(LOG_RAW,"*","DONTSHOWPROMPT is OFF, setting...");

		/* Turn off prompt until we get entire body. */
		Web->d->flags |= STAT_DONTSHOWPROMPT;
		DCC_flags2str(Web->d->flags,flags);
		Log_write(LOG_RAW,"*","flags: %s",flags);
	}

	/* Check for EOF */
	if(isequal(Web->tok1,"<EOF>"))
	{
		/* ok we're done. */
		Web->d->flags &= ~STAT_DONTSHOWPROMPT;

		/* Display headers, statusline, and menu */
		sprintf(Web->display,"%s - Post Note",Web->title);
		openhtml(Web->display);
		websay_statusline();
		websay_menuline("Login");
		websay("<br><br>");
		websay("");
		websay("Note sent.  ");

		u = User_user(Web->tok2);
		if(u->registered)
		{
			/* inform user of note */
			if(User_informuserofnote(u,Web->account))
				websay("(%s is online...)  ",u->acctname);
		}

		websay("Thanks!<p>");

		closehtml();

		return;
	}

	/* Ok what we're getting is 'nick' and each line of a note */
	u = User_user(Web->tok1);
	if(!u->registered)
	{
		/* Display headers, statusline, and menu */
		sprintf(Web->display,"%s - Post Note",Web->title);
		openhtml(Web->display);
		websay_statusline();
		websay_menuline("Login");
		websay("<br><br>");
		websay("");

		websay("The nick \"%s\" does not belong to a registered user.",Web->tok1);
		Web->d->flags &= ~STAT_DONTSHOWPROMPT;
		closehtml();
		return;
	}

	if(!u->registered)
	{
		/* Display headers, statusline, and menu */
		sprintf(Web->display,"%s - Post Note",Web->title);
		openhtml(Web->display);
		websay_statusline();
		websay_menuline("Login");
		websay("<br><br>");
		websay("");

		websay("The nick \"%s\" does not belong to a registered user.",Web->tok1);
		Web->d->flags &= ~STAT_DONTSHOWPROMPT;
		closehtml();
		return;
	}

	if(Web->istok2)
		split(NULL,Web->cmdparam);
	else
		strcpy(Web->cmdparam,"");

	n = Note_lastnotefromnick(u,Web->user,Web->user->acctname);
	if(n == NULL)
	{
		/* fuck */
		User_addnewnote(u,Web->user,Web->account,Web->cmdparam,NOTE_NEW);
		return;
	}

	if(u->forward && u->forward[0])
	{
		if(!(n->flags & NOTE_FORWARD))
		{
			n->flags |= NOTE_FORWARD;
			Note->numNotesQueued++;
		}
	}
	Note_addbodyline(n,Web->cmdparam);
}

void webcmd_putsavdel()
{
	char	eachnum[BUFSIZ];
	int	i, counter;
	noterec	*n;

	if(!(Web->d->flags & STAT_DONTSHOWPROMPT))
	{
		/* Turn off prompt until we get entire body. */
		Web->d->flags |= STAT_DONTSHOWPROMPT;
	}

	/* Check for EOF */
	if(isequal(Web->tok1,"<EOF>"))
	{
		/* ok we're done. */
		Web->d->flags &= ~STAT_DONTSHOWPROMPT;

		/* Pass control to 'read' */
		strcpy(Web->cmdparam,"");
		strcpy(Web->tok1,"");

		webcmd_read();
		return;
	}

	if(isequal(Web->tok1,"sav"))
	{
		strcpy(Web->cmdparam,&Web->cmdparam[3]);
		rmspace(Web->cmdparam);

		i = 0;
		counter = 0;
		split(eachnum,Web->cmdparam);
		for(n=Web->user->notes; n!=NULL; n=n->next)
		{
			i++;

			if(eachnum[0] && atoi(eachnum) == i)
			{
				split(eachnum,Web->cmdparam);
				counter++;
				if(counter <= Grufti->max_saved_notes)
					n->flags |= NOTE_SAVE;
			}
			else
				n->flags &= ~NOTE_SAVE;
		}
	}
	else if(isequal(Web->tok1,"del"))
	{
		strcpy(Web->cmdparam,&Web->cmdparam[3]);
		rmspace(Web->cmdparam);

		i = 0;
		split(eachnum,Web->cmdparam);
		for(n=Web->user->notes; n!=NULL; n=n->next)
		{
			i++;

			if(eachnum[0] && atoi(eachnum) == i)
			{
				split(eachnum,Web->cmdparam);
				n->flags |= NOTE_DELETEME;
			}
		}
		Note_deletemarkednotes(Web->user);
	}
}

void webcmd_rn()
{
	noterec	*n, *nprev, *nnext;
	nbody	*body;
	int	num = 0;
	char	sent[TIMESHORTLEN], tmp[BUFSIZ];

	if(!Web->istok1)
		return;

	num = atoi(Web->tok1);

	/* Display headers, statusline, and menu */
	sprintf(tmp,"%s - Read Note %d",Web->title,num);
	openhtml(tmp);

	/* Mark Note as 'Read' before we get to statusline */
	n = Note_notebynumber(Web->user,num);
	if(n == NULL)
	{
		websay("That Note doesn't exist anymore!");
		closehtml();
		return;
	}
	n->flags |= NOTE_READ;
	n->flags &= ~NOTE_NEW;

	websay_statusline();
	sprintf(tmp,"RN:%s",Web->tok1);
	websay_menuline(tmp);
	websay("<br><br>");
	websay("");

	/* Display 'prev' and 'next' */
	nprev = Note_notebynumber(Web->user,num-1);
	websay("<table width=570 border=0><tr><td width=190 align=left>");
	websay("<font face=\"arial\" size=\"2\">");
	if(nprev)
		websay("<a href=\"%s?%s;%s;RN:%d\">Previous</a> Note is from %s",Web->cgi_url,Web->user->acctname,Web->user->pass,num-1,nprev->nick);
	else
		websay("At first Note");

	websay("</font>");
	websay("</td><td width=190 align=center>");
	websay("<font face=\"arial\" size=\"2\">");
	websay("Note %d of %d",num,Web->user->num_notes);
	websay("</font>");
	websay("</td><td width=190 align=right>");
	websay("<font face=\"arial\" size=\"2\">");

	nnext = Note_notebynumber(Web->user,num+1);

	if(nnext)
		websay("<a href=\"%s?%s;%s;RN:%d\">Next</a> Note is from %s",Web->cgi_url,Web->user->acctname,Web->user->pass,num+1,nnext->nick);
	else
		websay("At last Note");
	websay("</font>");
	websay("</td></tr></table>");
	websay("");

	/* Draw bar */
	websay("<table width=570 border=0 cellspacing=2 cellpadding=0><tr>");
	websay("<td bgcolor=\"%s\"><font size=\"1\" color=\"%s\">.</font></td></tr></table>",Web->color1,Web->color1);
	websay("");

	/* Draw 'From' */
	websay("<table width=570 border=0 cellpadding=1 cellspacing=2><tr>");
	websay("<td bgcolor=\"%s\" align=center width=50>",Web->color1);
	websay("<font face=\"arial\" size=\"2\"><b>From</b></font></td>");
	websay("<td width=520><font size=\"4\"><b><a href=\"%s?%s;%s;Finger:%s\">%s </a><font color=\"%s\">.</font> <i>\"%s\"</i></b></font></td>",Web->cgi_url,Web->user->acctname,Web->user->pass,n->nick,n->nick,Web->bgcolor,n->finger);
	websay("</tr>");
	websay("");

	/* Draw 'Host' */
	websay("<tr>");
	websay("<td bgcolor=\"%s\" align=center width=50>",Web->color1);
	websay("<font face=\"arial\" size=\"2\"><b>Host</b></font></td>");
	websay("<td width=520>%s</td>",n->uh);
	websay("</tr>");
	websay("");

	/* Draw 'Sent' */
	timet_to_date_long(n->sent,sent);
	websay("<tr>");
	websay("<td bgcolor=\"%s\" align=center width=50>",Web->color1);
	websay("<font face=\"arial\" size=\"2\"><b>Sent</b></font></td>");
	websay("<td width=520>%s</td>",sent);
	websay("</tr>");
	websay("</table>");
	websay("");

	/* Draw Note */
	websay("<br>");
	websay("<table width=570 border=0><tr><td>");
	for(body=n->body; body!=NULL; body=body->next)
	{
		if(body->text[0])
		{
			strcpy(tmp, body->text);
			Web_fixforhtml(tmp);
			websay("%s",body->text);
		}
		else
			websay("<p>");
	}
	websay("</td></tr></table>");
	websay("");

	/* Draw bar */
	websay("<table width=570 border=0 cellspacing=2 cellpadding=0><tr>");
	websay("<td bgcolor=\"%s\"><font size=\"1\" color=\"%s\">.</font></td></tr></table>",Web->color1,Web->color1);
	websay("");

	/* Draw footer */
	websay("<font face=\"arial\" size=\"2\">");
	websay("<a href=\"%s?%s;%s;DN:%d\">Delete</a> this Note",Web->cgi_url,Web->user->acctname,Web->user->pass,num);
	websay(" <font color=\"%s\">...</font> ",Web->bgcolor);
	websay("<a href=\"%s?%s;%s;Reply:%d\">Reply</a> to %s",Web->cgi_url,Web->user->acctname,Web->user->pass,num,n->nick);

	websay("<br>");
	closehtml();
}

void webcmd_dn()
{
	noterec	*n;
	int	num = 0;

	if(!Web->istok1)
		return;

	num = atoi(Web->tok1);

	if(num <= 0 || num > Web->user->num_notes)
		return;

	n = Note_notebynumber(Web->user,num);
	if(n == NULL)
		return;

	/* Mark for delete */
	n->flags |= NOTE_DELETEME;

	Note_deletemarkednotes(Web->user);

	n = Note_notebynumber(Web->user,num);
	if(n == NULL)
	{
		n = Note_notebynumber(Web->user,Web->user->num_notes);
		if(n == NULL)
		{
			/* oops!  no more Notes */
			webcmd_login();
			return;
		}
		sprintf(Web->tok1,"%d",Web->user->num_notes);
	}
	else
		sprintf(Web->tok1,"%d",num);

	/* Pass control to webcmd_rn() */
	webcmd_rn();
}

void webcmd_read()
{
	noterec	*n;
	int	num;
	char	sent[TIMESHORTLEN], subject[BUFSIZ], finger[BUFSIZ];
	char	checked[25];

	/* Display headers, statusline, and menu */
	sprintf(Web->display,"%s - Read Notes",Web->title);
	openhtml(Web->display);
	websay("<form method=\"post\" action=\"%s?%s;%s;!update_notes\">",Web->cgi_url,Web->user->acctname,Web->user->pass);
	websay_statusline();
	websay_menuline("Read");
	websay("<br>");

	websay("<table width=570 border=0 cellspacing=0 cellpadding=0><tr><td>");
	websay("<font size=\"6\" color=\"%s\"><i>%s Notes</i></font><br>",Web->color4,Grufti->botnick);
	websay("Click on the nick to display a Note.  By marking a Note as 'Saved', you keep it from being automatically deleted after %d days.  A maximum of %d Notes can be saved.",Grufti->timeout_notes,Grufti->max_saved_notes);
	websay("</td></tr></table>");
	websay("<br>");
	websay("");

	websay("Click <a href=\"%s?%s;%s;Note\">here</a> to compose a new Note.<br>",Web->cgi_url,Web->user->acctname,Web->user->pass);
	websay("<br>");
	websay("");

	/* Display title */
	websay("<table width=570 border=0 cellspacing=1 cellpadding=0>");
	websay("<tr>");
	websay("<td bgcolor=\"%s\" width=30 align=center><font face=\"arial\" size=\"2\">#</font></td>",Web->color1);
	websay("<td width=5><font face=\"arial\" size=\"2\" color=\"%s\">.</font></td>",Web->bgcolor);
	websay("<td bgcolor=\"%s\" width=50 align=center><font face=\"arial\" size=\"1\">Sav Del</font></td>",Web->color1);
	websay("<td width=5><font face=\"arial\" size=\"2\" color=\"%s\">.</font></td>",Web->bgcolor);
	websay("<td bgcolor=\"%s\" width=35 align=center><font face=\"arial\" size=\"2\">Sent</font></td>",Web->color1);
	websay("<td width=5><font face=\"arial\" size=\"2\" color=\"%s\">.</font></td>",Web->bgcolor);
	websay("<td bgcolor=\"%s\" width=88><font face=\"arial\" size=\"2\">From</font></td>",Web->color1);
	websay("<td width=5><font face=\"arial\" size=\"2\" color=\"%s\">.</font></td>",Web->bgcolor);
	websay("<td bgcolor=\"%s\" width=180><font face=\"arial\" size=\"2\">Real Name</font></td>",Web->color1);
	websay("<td width=5><font face=\"arial\" size=\"2\" color=\"%s\">.</font></td>",Web->bgcolor);
	websay("<td bgcolor=\"%s\" width=150><font face=\"arial\" size=\"2\">Subject</font></td>",Web->color1);
	websay("</tr></table>");
	websay("");

	/* Now, we display all Notes with hyperlinks */
	num = 0;
	for(n=Web->user->notes; n!=NULL; n=n->next)
	{
		num++;

		websay("<table width=570 border=0 cellspacing=1 cellpadding=0>");
		websay("<tr>");

		/* Display num */
		if(n->flags & NOTE_NEW)
			websay("<td width=30 align=center><font size=\"4\" color=\"%s\"><b>%d</b></font></td>",Web->color2,num);
		else
			websay("<td width=30 align=center><font size=\"4\"><b>%d</b></font></td>",num);

		/* Display Save / Delete*/
		if(n->flags & NOTE_SAVE)
			strcpy(checked," CHECKED");
		else
			strcpy(checked,"");
		websay("<td width=62 colspan=2 align=center><input type=\"checkbox\" name=\"save\" value=\"%d\"%s><input type=\"checkbox\" name=\"del\" value=\"%d\"></td>",num,checked,num);

		/* Display Sent */
		timet_to_dateonly_short(n->sent,sent);
		sent[5] = 0;
		websay("<td width=35 align=center><font size=\"2\">%s</font></td>",sent);
		websay("<td width=5><font size=\"2\" color=\"%s\">.</font></td>",Web->bgcolor);

		/* Display from */
		websay("<td width=94 colspan=2><a href=\"%s?%s;%s;RN:%d\"><font size=\"2\">%s</font></a></td>",Web->cgi_url,Web->user->acctname,Web->user->pass,num,n->nick);

		/* Display real name */
		strcpy(finger,n->finger);
		if(strlen(n->finger) > 25)
			strcpy(&finger[25-3],"...");

		websay("<td width=186 colspan=2><font size=\"2\"><i>%s</i></font></td>",finger);


		/* Display subject */
		if(n->body != NULL && n->body->text != NULL)
			strcpy(subject,n->body->text);
		else
			strcpy(subject,"");
		if(strlen(subject) > 30)
			strcpy(&subject[30-3],"...");
		websay("<td width=150><font size=\"1\" face=\"arial\">%s</font></td>",subject);

		websay("</tr></table>");
		websay("");
	}

	/* Display title */
	websay("<table width=570 border=0 cellspacing=1 cellpadding=0>");
	websay("<tr>");
	websay("<td bgcolor=\"%s\" width=30 align=center><font face=\"arial\" size=\"2\">#</font></td>",Web->color1);
	websay("<td width=5><font face=\"arial\" size=\"2\" color=\"%s\">.</font></td>",Web->bgcolor);
	websay("<td bgcolor=\"%s\" width=50 align=center><font face=\"arial\" size=\"1\">Sav Del</font></td>",Web->color1);
	websay("<td width=5><font face=\"arial\" size=\"2\" color=\"%s\">.</font></td>",Web->bgcolor);
	websay("<td bgcolor=\"%s\" width=35 align=center><font face=\"arial\" size=\"2\">Sent</font></td>",Web->color1);
	websay("<td width=5><font face=\"arial\" size=\"2\" color=\"%s\">.</font></td>",Web->bgcolor);
	websay("<td bgcolor=\"%s\" width=88><font face=\"arial\" size=\"2\">From</font></td>",Web->color1);
	websay("<td width=5><font face=\"arial\" size=\"2\" color=\"%s\">.</font></td>",Web->bgcolor);
	websay("<td bgcolor=\"%s\" width=180><font face=\"arial\" size=\"2\">Real Name</font></td>",Web->color1);
	websay("<td width=5><font face=\"arial\" size=\"2\" color=\"%s\">.</font></td>",Web->bgcolor);
	websay("<td bgcolor=\"%s\" width=150><font face=\"arial\" size=\"2\">Subject</font></td>",Web->color1);
	websay("</tr></table>");
	websay("");

	websay("<br>");
	websay("<table width=570 border=0><tr><td>");
	websay("<font size=\"2\">If you checked or unchecked any boxes in the 'Sav' or 'Del' columns, click the [Save/Delete] button to complete the request.</font>");
	websay("</td></tr></table>");
	websay("<br>");
	websay("<input type=submit name=submit value=\"Save/Delete\"><br>");
	closehtml();
}

void webcmd_putsearch()
{
	/* From here we get: searchword */
	userrec	*u;

	if(!Web->istok1)
	{
		sprintf(Web->display,"%s - Search",Web->title);
		openhtml(Web->display);
		websay_statusline();
		websay_menuline("Users");
		websay("<br>");
		websay("No text specified in search box!");
		websay("<br>");
		closehtml();
		return;
	}

	u = User_user(Web->tok1);
	if(u && u->registered)
	{
		/* Pass control to 'finger' */
		strcpy(Web->cmdparam,u->acctname);
		strcpy(Web->tok1,u->acctname);

		webcmd_finger();
		return;
	}

	sprintf(Web->display,"%s - Search",Web->title);
	openhtml(Web->display);
	websay_statusline();
	websay_menuline("Users");
	websay("<br>");
	websay("Unable to find the nick: %s<br>",Web->tok1);
	websay("Try again<br>");
	websay("<br>");
	websay_nicksearchinput();
	websay("<br>");

	closehtml();
}

	
void webcmd_users()
{
	userrec	*u;
	int	tot, i, in_table = 0, col = 0;
	char	c, cur_letter = 0;

	/* Display headers, statusline, and menu */
	sprintf(Web->display,"%s - Users",Web->title);
	openhtml(Web->display);
	websay_statusline();
	websay_menuline("Users");

	websay("<form method=\"post\" action=\"%s?%s;%s;!finger_lookup\">",Web->cgi_url,Web->user->acctname,Web->user->pass);
	websay("<font size=\"6\" color=\"%s\"><i>%s's Users</i></font><br>",Web->color4,Grufti->botnick);
	tot = User_orderbyacctname();
	websay("The following is an alphabetized list of all %d users registered with the bot.<br>",tot);
	websay("Click on a user to display account information, or use the 'quick search' tool.<br>");
	websay("<br>");
	websay_nicksearchinput();
	websay("<br><br>");
	/* Now, we display all users in each letter of the alphabet */
	for(i=0; i<=tot; i++)
	{
		u = User_userbyorder(i);
		if(!u->registered)
			continue;

		c = toupper(u->acctname[0]);
		if(c < 65 || c > 90)
			c = '#';
		if(cur_letter != c)
		{
			cur_letter = c;

			if(in_table)
			{
				/* finish remaining cells */
				while(col < 6)
				{
					col++;
					websay("<td width=90><font size=\"2\"><br></font></td>");
				}

				/* close table */
				websay("</tr></table>");
				websay("<br><br>");
				websay("");
				in_table = 0;
			}
			/* Display title */
			websay("<font size=\"5\" color=\"%s\"><b>- %c -</b></font><br>",Web->color2,c);

			/* Begin table */
			websay("<table width=570 border=0 cellspacing=5 cellpadding=0><tr>");
			col = 0;
			in_table = 1;
		}

		/* Write cell */
		col++;
		if(col > 6)
		{
			/* Start a new row */
			col = 1;
			websay("</tr><tr>");
		}
			
		websay("<td width=90><a href=\"%s?%s;%s;Finger:%s:%d\"><font size=\"2\">%s</font></a></td>",Web->cgi_url,Web->user->acctname,Web->user->pass,u->acctname,i,u->acctname);

	}
	/* finish remaining cells */
	while(col < 6)
	{
		col++;
		websay("<td width=90><font size=\"2\"><br></font></td>");
	}

	/* close table */
	websay("</tr></table>");
	websay("<br><br>");

	closehtml();
}

void webcmd_finger()
{
	userrec	*u, *uprev, *unext;
	char	display[BUFSIZ], pre[BUFSIZ], tmp[BUFSIZ], signoff[SERVERLEN];
	struct	tm *btime;
	time_t	now, t = 0L;
	int	i, highest_plan, gotplan = 0, x, new, curorder = 0, tot;
	u_long	tot_time;
	float	hrs;

	if(!Web->istok1)
		return;

	u = User_user(Web->tok1);
	if(!u->registered)
		return;

	/* If no 2nd argument, we'll have to figure out usernumber ourselves */
	if(!Web->istok2)
		curorder = User_ordernumberforuser(u);
	else
		curorder = atoi(Web->tok2);

	/* Display headers, statusline, and menu */
	sprintf(display,"%s - Finger %s",Web->title,u->acctname);
	openhtml(display);
	websay_statusline();
	sprintf(display,"Finger:%s:%s",Web->tok1,Web->tok2);
	websay_menuline(display);
	websay("<br><br>");

	tot = User_orderbyacctname();
	uprev = User_userbyorder(curorder - 1);

	websay_nicksearchinput();
	websay("<br><br>");

	websay("<table width=570 border=0><tr><td width=190 align=left>");
	websay("<font face=\"arial\" size=\"2\">");
	if(uprev && uprev->registered)
		websay("Previous is <a href=\"%s?%s;%s;Finger:%s:%d\">%s</a>",Web->cgi_url,Web->user->acctname,Web->user->pass,uprev->acctname,curorder-1,uprev->acctname);
	else
		websay("At first record");

	websay("</font>");
	websay("</td><td width=190 align=center>");
	websay("<font face=\"arial\" size=\"2\">");
	websay("%s is %d of %d",u->acctname,curorder,tot);
	websay("</font>");
	websay("</td><td width=190 align=right>");
	websay("<font face=\"arial\" size=\"2\">");

	unext = User_userbyorder(curorder + 1);
	if(unext)
		websay("Next is <a href=\"%s?%s;%s;Finger:%s:%d\">%s</a>",Web->cgi_url,Web->user->acctname,Web->user->pass,unext->acctname,curorder+1,unext->acctname);
	else
		websay("At last record");
	websay("</font>");
	websay("</td></tr></table>");

	/* Display bar */
	websay("<table width=570 border=0 cellspacing=0 cellpadding=0><tr>");
	websay("<td bgcolor=\"%s\"><font size=\"1\" color=\"%s\">-</font></td></tr></table>",Web->color1,Web->color1);
	websay("");

	websay("<font size=\"5\"><b>%s</b>",u->acctname);
	websay("<font color=\"%s\"> .. </font>",Web->bgcolor);
	if(u->finger)
		websay("<i>\"%s\"</i>",u->finger);
	else
		websay("<i>\"\"</i>");
	websay("</font><br>");

	websay("<br>");

	/* Display nicks */
	User_makenicklist(display,u);
	websay("<table width=570 border=0 cellpadding=0 cellspacing=2><tr>");
	websay("<td width=20><br></td>");
	websay("<td bgcolor=\"%s\" align=center width=80>",Web->color1);
	websay("<font face=\"arial\" size=\"2\"><b>nicks</b></font>");
	websay("</td>");
	websay("<td width=20><br></td>");
	websay("<td width=450>%s</td>",display);
	websay("</tr><tr>");
	websay("");

	/* Display email */
	websay("<td width=20><br></td>");
	websay("<td bgcolor=\"%s\" align=center width=80>",Web->color1);
	websay("<font face=\"arial\" size=\"2\"><b>email</b></font>");
	websay("</td>");
	websay("<td width=20><br></td>");
	if(u->email)
		websay("<td width=450><a href=\"mailto:%s\">%s</a></td>",u->email,u->email);
	else
		websay("<td width=450>(No email address)</td>");
	websay("</tr><tr>");
	websay("");

	/* Display www */
	if(u->www)
	{
		websay("<td width=20><br></td>");
		websay("<td bgcolor=\"%s\" align=center width=80>",Web->color1);
		websay("<font face=\"arial\" size=\"2\"><b>www</b></font>");
		websay("</td>");
		websay("<td width=20><br></td>");
		websay("<td width=450><a href=\"%s\" target=\"_top\">%s</a></td>",u->www,u->www);
		websay("</tr><tr>");
		websay("");
	}

	/* Display birthday */
	if(u->bday[0])
	{
		char hor[25];

		websay("<td width=20><br></td>");
		websay("<td bgcolor=\"%s\" align=center width=80>",Web->color1);
		websay("<font face=\"arial\" size=\"2\"><b>bday</b></font>");
		websay("</td>");
		websay("<td width=20><br></td>");
		bday_plus_hor(display,hor,u->bday[0],u->bday[1],u->bday[2]);
		websay("<td width=450>%s (%s)</td>",display,hor);
		websay("</tr><tr>");
		websay("");
	}

	/* Display other1 */
	if(u->other1)
	{
		websay("<td width=20><br></td>");
		websay("<td bgcolor=\"%s\" align=center width=80>",Web->color1);
		if(strchr(u->other1,' '))
		{
			strcpy(tmp,u->other1);
			split(pre,tmp);
			if(strlen(pre) > 7)
				pre[7] = 0;
			websay("<font face=\"arial\" size=\"2\"><b>%s</b></font>",pre);
		}
		else
			websay("<font face=\"arial\" size=\"2\"><b>other</b></font>");
		websay("</td>");
		websay("<td width=20><br></td>");
		websay("<td width=450>%s</td>",tmp);
		websay("</tr><tr>");
		websay("");
	}

	/* Display other2 */
	if(u->other2)
	{
		websay("<td width=20><br></td>");
		websay("<td bgcolor=\"%s\" align=center width=80>",Web->color1);
		if(strchr(u->other2,' '))
		{
			strcpy(tmp,u->other2);
			split(pre,tmp);
			if(strlen(pre) > 7)
				pre[7] = 0;
			websay("<font face=\"arial\" size=\"2\"><b>%s</b></font>",pre);
		}
		else
			websay("<font face=\"arial\" size=\"2\"><b>other</b></font>");
		websay("</td>");
		websay("<td width=20><br></td>");
		websay("<td width=450>%s</td>",tmp);
		websay("</tr><tr>");
		websay("");
	}

	/* Display other3 */
	if(u->other3)
	{
		websay("<td width=20><br></td>");
		websay("<td bgcolor=\"%s\" align=center width=80>",Web->color1);
		if(strchr(u->other3,' '))
		{
			strcpy(tmp,u->other3);
			split(pre,tmp);
			if(strlen(pre) > 7)
				pre[7] = 0;
			websay("<font face=\"arial\" size=\"2\"><b>%s</b></font>",pre);
		}
		else
			websay("<font face=\"arial\" size=\"2\"><b>other</b></font>");
		websay("</td>");
		websay("<td width=20><br></td>");
		websay("<td width=450>%s</td>",tmp);
		websay("</tr><tr>");
		websay("");
	}

	/* Display other4 */
	if(u->other4)
	{
		websay("<td width=20><br></td>");
		websay("<td bgcolor=\"%s\" align=center width=80>",Web->color1);
		if(strchr(u->other4,' '))
		{
			strcpy(tmp,u->other4);
			split(pre,tmp);
			if(strlen(pre) > 7)
				pre[7] = 0;
			websay("<font face=\"arial\" size=\"2\"><b>%s</b></font>",pre);
		}
		else
			websay("<font face=\"arial\" size=\"2\"><b>other</b></font>");
		websay("</td>");
		websay("<td width=20><br></td>");
		websay("<td width=450>%s</td>",tmp);
		websay("</tr><tr>");
		websay("");
	}

	/* Display other5 */
	if(u->other5)
	{
		websay("<td width=20><br></td>");
		websay("<td bgcolor=\"%s\" align=center width=80>",Web->color1);
		if(strchr(u->other5,' '))
		{
			strcpy(tmp,u->other5);
			split(pre,tmp);
			if(strlen(pre) > 7)
				pre[7] = 0;
			websay("<font face=\"arial\" size=\"2\"><b>%s</b></font>",pre);
		}
		else
			websay("<font face=\"arial\" size=\"2\"><b>other</b></font>");
		websay("</td>");
		websay("<td width=20><br></td>");
		websay("<td width=450>%s</td>",tmp);
		websay("</tr><tr>");
		websay("");
	}

	/* Display other6 */
	if(u->other6)
	{
		websay("<td width=20><br></td>");
		websay("<td bgcolor=\"%s\" align=center width=80>",Web->color1);
		if(strchr(u->other6,' '))
		{
			strcpy(tmp,u->other6);
			split(pre,tmp);
			if(strlen(pre) > 7)
				pre[7] = 0;
			websay("<font face=\"arial\" size=\"2\"><b>%s</b></font>",pre);
		}
		else
			websay("<font face=\"arial\" size=\"2\"><b>other</b></font>");
		websay("</td>");
		websay("<td width=20><br></td>");
		websay("<td width=450>%s</td>",tmp);
		websay("</tr><tr>");
		websay("");
	}

	/* Display other7 */
	if(u->other7)
	{
		websay("<td width=20><br></td>");
		websay("<td bgcolor=\"%s\" align=center width=80>",Web->color1);
		if(strchr(u->other7,' '))
		{
			strcpy(tmp,u->other7);
			split(pre,tmp);
			if(strlen(pre) > 7)
				pre[7] = 0;
			websay("<font face=\"arial\" size=\"2\"><b>%s</b></font>",pre);
		}
		else
			websay("<font face=\"arial\" size=\"2\"><b>other</b></font>");
		websay("</td>");
		websay("<td width=20><br></td>");
		websay("<td width=450>%s</td>",tmp);
		websay("</tr><tr>");
		websay("");
	}

	/* Display other8 */
	if(u->other8)
	{
		websay("<td width=20><br></td>");
		websay("<td bgcolor=\"%s\" align=center width=80>",Web->color1);
		if(strchr(u->other8,' '))
		{
			strcpy(tmp,u->other8);
			split(pre,tmp);
			if(strlen(pre) > 7)
				pre[7] = 0;
			websay("<font face=\"arial\" size=\"2\"><b>%s</b></font>",pre);
		}
		else
			websay("<font face=\"arial\" size=\"2\"><b>other</b></font>");
		websay("</td>");
		websay("<td width=20><br></td>");
		websay("<td width=450>%s</td>",tmp);
		websay("</tr><tr>");
		websay("");
	}

	/* Display other9 */
	if(u->other9)
	{
		websay("<td width=20><br></td>");
		websay("<td bgcolor=\"%s\" align=center width=80>",Web->color1);
		if(strchr(u->other9,' '))
		{
			strcpy(tmp,u->other9);
			split(pre,tmp);
			if(strlen(pre) > 7)
				pre[7] = 0;
			websay("<font face=\"arial\" size=\"2\"><b>%s</b></font>",pre);
		}
		else
			websay("<font face=\"arial\" size=\"2\"><b>other</b></font>");
		websay("</td>");
		websay("<td width=20><br></td>");
		websay("<td width=450>%s</td>",tmp);
		websay("</tr><tr>");
		websay("");
	}

	/* Display other10 */
	if(u->other10)
	{
		websay("<td width=20><br></td>");
		websay("<td bgcolor=\"%s\" align=center width=80>",Web->color1);
		if(strchr(u->other10,' '))
		{
			strcpy(tmp,u->other10);
			split(pre,tmp);
			if(strlen(pre) > 7)
				pre[7] = 0;
			websay("<font face=\"arial\" size=\"2\"><b>%s</b></font>",pre);
		}
		else
			websay("<font face=\"arial\" size=\"2\"><b>other</b></font>");
		websay("</td>");
		websay("<td width=20><br></td>");
		websay("<td width=450>%s</td>",tmp);
		websay("</tr><tr>");
		websay("");
	}

	/* Display location */
	Location_idtostr(u->location,display);
	if(display[0])
	{
		websay("<td width=20><br></td>");
		websay("<td bgcolor=\"%s\" align=center width=80>",Web->color1);
		websay("<font face=\"arial\" size=\"2\"><b>location</b></font>");
		websay("</td>");
		websay("<td width=20><br></td>");
		websay("<td width=450>%s</td>",display);
		websay("</tr><tr>");
		websay("");
	}

	/* Display last login */
	websay("<td width=20><br></td>");
	websay("<td bgcolor=\"%s\" align=center width=80>",Web->color1);
	websay("<font face=\"arial\" size=\"2\"><b>last login</b></font>");
	websay("</td>");
	websay("<td width=20><br></td>");
	timet_to_date_long(u->last_login, display);
	websay("<td width=450>%s</td>",display);
	websay("</tr><tr>");
	websay("");

	/* Display last seen */
	x = cmd_finger_helper_makelast(u,display);
	websay("<td width=20><br></td>");
	websay("<td bgcolor=\"%s\" align=center width=80>",Web->color1);
	strcpy(signoff,"");
	if(x == 9 || x == 10 || x == 11)
	{
		if(u->signoff)
			sprintf(signoff," (%s)",u->signoff);
		websay("<font face=\"arial\" size=\"2\"><b>last seen</b></font>");
	}
	else
		websay("<font face=\"arial\" size=\"2\"><b>online</b></font>");

	if(x == 11)
		sprintf(display,"Over %d days ago",Grufti->timeout_accounts);
	websay("</td>");
	websay("<td width=20><br></td>");
	websay("<td width=450>%s%s</td>",display,signoff);
	websay("</tr></table>");
	websay("");

	/* Display plan */
	websay("<br>");
	websay("<pre>");
        highest_plan = 0;
        if(u->plan1)
                highest_plan = 1;
        if(u->plan2)
                highest_plan = 2;
        if(u->plan3)
                highest_plan = 3;
        if(u->plan4)
                highest_plan = 4;
        if(u->plan5)
                highest_plan = 5;
        if(u->plan6)
                highest_plan = 6;

        if(u->plan1 || highest_plan >= 1)
        {
                gotplan = 1;
                if(u->plan1)
                        websay("%s",u->plan1);
                else
                        websay("");
        }

	if(u->plan2 || (gotplan && highest_plan >= 2))
	{
                gotplan = 2;
                if(u->plan2)
                        websay("%s",u->plan2);
                else
                        websay("");
        }

        if(u->plan3 || (gotplan && highest_plan >= 3))
        {
                gotplan = 3;
                if(u->plan3)
                        websay("%s",u->plan3);
                else
                        websay("");
        }

        if(u->plan4 || (gotplan && highest_plan >= 4))
        {
                gotplan = 4;
                if(u->plan4)
                        websay("%s",u->plan4);
                else
                        websay(""); 
        }
                
        if(u->plan5 || (gotplan && highest_plan >= 5))
        {       
                gotplan = 5;
                if(u->plan5) 
                        websay("%s",u->plan5);
                else
                        websay("");
        }

        if(u->plan6 || (gotplan && highest_plan >= 6))
        {
                gotplan = 6;
                if(u->plan6)
                        websay("%s",u->plan6);
                else
                        websay("");
        }

	if(!highest_plan)
	{
		websay("");
		websay("                     -= No plan info set =-");
		websay("");
	}
	else if(highest_plan < 6)
		websay("");

	websay("</pre>");

	/* Figure out days of week */
	now = time(NULL);
	btime = localtime(&now);
	sprintf(display," color=\"%s\"",Web->color2);
	websay("");
	websay("<table width=570 border=0>");
	websay("<tr>");
	websay("<td bgcolor=\"%s\" width=22 align=center><font face=\"arial\" size=\"2\"%s><b>Su</b></font></td>",Web->color1,btime->tm_wday == 0 ? display : "");
	websay("<td bgcolor=\"%s\" width=22 align=center><font face=\"arial\" size=\"2\"%s><b>Mo</b></font></td>",Web->color1,btime->tm_wday == 1 ? display : "");
	websay("<td bgcolor=\"%s\" width=22 align=center><font face=\"arial\" size=\"2\"%s><b>Tu</b></font></td>",Web->color1,btime->tm_wday == 2 ? display : "");
	websay("<td bgcolor=\"%s\" width=22 align=center><font face=\"arial\" size=\"2\"%s><b>We</b></font></td>",Web->color1,btime->tm_wday == 3 ? display : "");
	websay("<td bgcolor=\"%s\" width=22 align=center><font face=\"arial\" size=\"2\"%s><b>Th</b></font></td>",Web->color1,btime->tm_wday == 4 ? display : "");
	websay("<td bgcolor=\"%s\" width=22 align=center><font face=\"arial\" size=\"2\"%s><b>Fr</b></font></td>",Web->color1,btime->tm_wday == 5 ? display : "");
	websay("<td bgcolor=\"%s\" width=22 align=center><font face=\"arial\" size=\"2\"%s><b>Sa</b></font></td>",Web->color1,btime->tm_wday == 6 ? display : "");

	websay("<td width=10><font face=\"arial\" size=\"2\"><br></td>");

	websay("<td bgcolor=\"%s\" width=70 align=center><font face=\"arial\" size=\"2\"><b>Hrs/Day</b></font></td>",Web->color1);
	websay("<td width=10><font face=\"arial\" size=\"2\"><br></td>");
	websay("<td bgcolor=\"%s\" width=70 align=center><font face=\"arial\" size=\"2\"><b>Usage</b></font></td>",Web->color1);
	websay("<td width=10><font face=\"arial\" size=\"2\"><br></td>");
	websay("<td bgcolor=\"%s\" width=70 align=center><font face=\"arial\" size=\"2\"><b>Created</b></font></td>",Web->color1);
	websay("<td width=10><font face=\"arial\" size=\"2\"><br></td>");
	websay("<td bgcolor=\"%s\" width=70 align=center><font face=\"arial\" size=\"2\"><b>Notes</b></font></td>",Web->color1);
	websay("<td width=10><font face=\"arial\" size=\"2\"><br></td>");
	websay("<td bgcolor=\"%s\" width=70 align=center><font face=\"arial\" size=\"2\"><b>Levels</b></font></td>",Web->color1);
	websay("</tr>");

	websay("");
		
	for(i=0; i<7; i++)
	{
		t = u->time_seen[i];

		if(btime->tm_wday == i)
		{
			if(u->first_activity)
				t += (time(NULL) - u->first_activity);
		}

		if(!t)
			websay("<td align=center>--</td>");
		else if(t < 360)
			websay("<td align=center>.1</td>");
		else if(t < 3600)
			websay("<td align=center>.%1lu</td>",t / 360);
		else
			websay("<td align=center>%2lu</td>",t / 3600);
	}
	
	websay("<td><br></td>");

	/* Show hrs/day */
	tot_time = User_timeperweek(u);
	if(tot_time < 2562)
		tot_time = 2562;
	hrs = (float)tot_time / 25620.0;
	websay("<td align=center>%3.1f</td>",hrs);
	websay("<td><br></td>");

	/* Show freq */
	User_makeircfreq(display,u);
	websay("<td align=center>%s</td>",display);
	websay("<td><br></td>");

	/* Show created */
	timet_to_date_short(u->time_registered,display);
	display[8] = 0;
	websay("<td align=center>%s</td>",display);
	websay("<td><br></td>");

	/* Show Notes */
	new = Note_numnew(u);
	if(new)
		websay("<td align=center>%d/%d</td>",new,u->num_notes);
	else
		websay("<td align=center>%d</td>",u->num_notes);
	websay("<td><br></td>");

	/* Show levels */
	User_flags2str(u->flags,display);
	websay("<td align=center>+%s</td>",display);
	
	
	websay("</tr></table>");

	/* Display bar */
	websay("<table width=570 border=0 cellspacing=0 cellpadding=0><tr>");
	websay("<td bgcolor=\"%s\"><font size=\"1\" color=\"%s\">-</font></td></tr></table>",Web->color1,Web->color1);
	websay("</tr></table>");
	websay("");
	websay("<font size=\"2\">Click <a href=\"%s?%s;%s;Note:%s\">here</a> to send a Note to %s</font>",Web->cgi_url,Web->user->acctname,Web->user->pass,u->acctname,u->acctname);

	closehtml();
}

void webcmd_channel()
{
	chanrec	*chan;
	membrec	*m;
	char	display[BUFSIZ], idle[TIMESHORTLEN], join[TIMESHORTLEN];
	char	uh[UHOSTLEN], levels[FLAGLONGLEN], chname[CHANLEN];
	char	spl[TIMESHORTLEN];
	int	i, tot;

	/* Build channel name (add leading '#') */
	sprintf(chname,"#%s",Web->tok1);

	/* Display headers, statusline, and menu */
	sprintf(display,"%s - Channel %s",Web->title,chname);
	openhtml(display);
	websay_statusline();
	sprintf(display,"Channel:%s",Web->tok1);
	websay_menuline(display);

	websay("<br>");
	websay("<table width=570 border=0 cellspacing=0 cellpadding=0><tr><td>");
	websay("<font size=\"6\" color=\"%s\"><i>Who %s</i></font><br>",Web->color4,chname);
	websay("The following list reflects who is on the channel at this given moment.  Use the 'refresh' menu item on the top right to display a more recent listing.  Hyperlinked nicks denote users who are registered with the bot.  Click on the nick to view their finger information.<br>");
	websay("</td></tr></table>");
	websay("<br>");
	websay("");
	
	/* Retrieve channel info */
	chan = Channel_channel(chname);
	if(chan == NULL)
	{
		websay("No such channel \"%s\".",chname);
		closehtml();
		return;
	}

	/* Display channel name and modes */
	Channel_modes2str(chan,display);
	if(display[0])
		websay("<font size=\"4\"><b>Channel: %s (%s)<br>",chan->name,display);
	else
		websay("<font size=\"4\"><b>Channel: %s<br>",chan->name);

	/* Display topic */
	if(chan->topic == NULL)
		websay("Topic:<br>");
	else if(chan->topic[0] == 0)
		websay("Topic:<br>");
	else
	{
		strcpy(display, chan->topic);
		Web_fixforhtml(display);
		websay("Topic: <i>\"%s\"</i><br>",display);
	}

	websay("</b></font>");
	websay("<br>");
	websay("");

	/* Display header */
	websay("<table width=570 border=0 cellspacing=1 cellpadding=0>");
	websay("<tr>");
	websay("<td bgcolor=\"%s\" width=40><font face=\"arial\" size=\"2\">Join</font></td>",Web->color1);
	websay("<td width=5><font face=\"arial\" size=\"2\" color=\"%s\">.</font></td>",Web->bgcolor);
	websay("<td bgcolor=\"%s\" width=40 align=center><font face=\"arial\" size=\"2\">Idle</font></td>",Web->color1);
	websay("<td width=5><font face=\"arial\" size=\"2\" color=\"%s\">.</font></td>",Web->bgcolor);
	websay("<td bgcolor=\"%s\" align=center width=15><font size=\"1\">@</font></td>",Web->color1);
	websay("<td bgcolor=\"%s\" width=88><font face=\"arial\" size=\"2\">Nick</font></td>",Web->color1);
	websay("<td width=5><font face=\"arial\" size=\"2\" color=\"%s\">.</font></td>",Web->bgcolor);
	websay("<td bgcolor=\"%s\" width=288><font face=\"arial\" size=\"2\">Userhost</font></td>",Web->color1);
	websay("<td width=5><font face=\"arial\" size=\"2\" color=\"%s\">.</font></td>",Web->bgcolor);
	websay("<td bgcolor=\"%s\" width=70><font face=\"arial\" size=\"2\">Levels</font></td>",Web->color1);
	websay("</tr></table>");

	/* Ok here we go */
	tot = Channel_orderbyjoin_htol(chan);

	for(i=1; i<=tot; i++)
	{
		websay("<table width=570 border=0 cellspacing=1 cellpadding=0>");
		websay("<tr>");

		m = Channel_memberbyorder(chan,i);
		if(m == NULL)
			continue;

		/* Display Join */
		if(m->joined == 0L)
			strcpy(join,"????");
		else
			timet_to_time_short(m->joined,join);
		websay("<td width=40 align=center><font size=\"2\">%s</font></td>",join);
		websay("<td width=5><font size=\"2\" color=\"%s\">.</font></td>",Web->bgcolor);

		/* Display idle */
		if(m->last == 0L)
			strcpy(idle,"????");
		else
			timet_to_ago_short(m->last,idle);
		websay("<td width=40 align=right><font size=\"2\">%s</font></td>",idle);
		websay("<td width=5><font size=\"2\" color=\"%s\">.</font></td>",Web->bgcolor);

		/* Display channel flags */
		if(m->flags & MEMBER_VOICE)
			websay("<td width=15 align=center><font size=\"2\">+</font></td>");
		if(m->flags & MEMBER_OPER)
			websay("<td width=15 align=center><font size=\"1\">@</font></td>");
		else
			websay("<td width=15 align=center><font size=\"2\" color=\"%s\">.</font></td>",Web->bgcolor);

		/* Display nick */
		if(m->user->registered)
			websay("<td width=88><a href=\"%s?%s;%s;Finger:%s\"><font size=\"2\">%s</font></a></td>",Web->cgi_url,Web->user->acctname,Web->user->pass,m->nick,m->nick);
		else
			websay("<td width=88><font size=\"2\">%s</font></td>",m->nick);
		websay("<td width=5><font size=\"2\" color=\"%s\">.</font></td>",Web->bgcolor);

		/* Display uh */
		if(m->split)
		{
			timet_to_ago_short(m->split,spl);
			sprintf(uh,"(Netsplit for %s)",spl);
		}
		else
			strcpy(uh,m->uh);

		if(strlen(uh) > 55)
			uh[55] = 0;
		websay("<td width=288><font size=\"2\">%s</font></td>",uh);
		websay("<td width=5><font size=\"2\" color=\"%s\">.</font></td>",Web->bgcolor);


		/* Display levels */
		User_flags2str(m->user->flags,levels);
		websay("<td width=70><font size=\"2\">%s</font></td>",levels);
		websay("</tr></table>");
		websay("");
	}

	websay("<table width=570 border=0 cellspacing=1 cellpadding=0>");
	websay("<tr>");
	websay("<td bgcolor=\"%s\" width=40><font size=\"2\" color=\"%s\">.</font></td>",Web->color1,Web->color1);
	websay("<td width=5><font face=\"arial\" size=\"2\" color=\"%s\">.</font></td>",Web->color1);
	websay("<td bgcolor=\"%s\" width=40><font size=\"2\" color=\"%s\">.</font></td>",Web->color1,Web->color1);
	websay("<td width=5><font face=\"arial\" size=\"2\" color=\"%s\">.</font></td>",Web->color1);
	websay("<td bgcolor=\"%s\" width=15><font size=\"2\" color=\"%s\">.</font></td>",Web->color1,Web->color1);
	websay("<td bgcolor=\"%s\" width=88><font size=\"2\" color=\"%s\">.</font></td>",Web->color1,Web->color1);
	websay("<td width=5><font face=\"arial\" size=\"2\" color=\"%s\">.</font></td>",Web->color1);
	websay("<td bgcolor=\"%s\" width=288><font size=\"2\" color=\"%s\">.</font></td>",Web->color1,Web->color1);
	websay("<td width=5><font face=\"arial\" size=\"2\" color=\"%s\">.</font></td>",Web->color1);
	websay("<td bgcolor=\"%s\" width=70><font size=\"2\" color=\"%s\">.</font></td>",Web->color1,Web->color1);
	websay("</tr>");
	websay("</table>");

	websay("<br>");
	
	closehtml();
}

void webcmd_cmdline()
{
	char display[BUFSIZ];

	/* Display headers, statusline, and menu */
	sprintf(display,"%s - Command-Line Interface",Web->title);
	openhtml(display);
	websay_statusline();
	websay_menuline("Cmdline");
	websay("<form method=\"post\" action=\"%s?%s;%s;!cmdline_post\">",Web->cgi_url,Web->user->acctname,Web->user->pass);
	websay("");

	websay("<table width=570 border=0 cellspacing=0 cellpadding=0><tr><td>");
	websay("<font size=\"6\" color=\"%s\"><i>Command-Line Interface</i></font><br>", Web->color4);
	websay("Enter bot commands here just as if you were in a DCC or telnet session to the bot.<br>");
	websay("</td></tr></table>");
	websay("<br>");
	websay("");

	websay("<form method=\"post\" action=\"%s?%s;%s;!webcmd_lookup\">",Web->cgi_url,Web->user->acctname,Web->user->pass);
	websay("<input type=\"text\" name=\"cmdline\" value=\"\" size=60 maxlength=4095>");
	websay("<input type=\"submit\" value=\"Enter\">");

	closehtml();
}

void webcmd_setup()
{
	gretrec	*g;
	locarec	*l;
	char	pre[25], display[BUFSIZ]; 
	int	i;


	/* Display headers, statusline, and menu */
	sprintf(display,"%s - Setup Profile",Web->title);
	openhtml(display);
	websay_statusline();
	websay_menuline("Setup");
	websay("<form method=\"post\" action=\"%s?%s;%s;!setup_post\">",Web->cgi_url,Web->user->acctname,Web->user->pass);
	websay("");

	websay("<table width=570 border=0 cellspacing=0 cellpadding=0><tr><td>");
	websay("<font size=\"6\" color=\"%s\"><i>Your Profile</i></font><br>",Web->color4);
	websay("This section allows you to change the information in your profile as seen when users use the 'finger' command.  The information is updated immediately and is made readily available as soon as you click the [Submit Changes] button.<br>");
	websay("</td></tr></table>");
	websay("<br>");
	websay("");

	/* Draw Real Name input */
	websay("<table width=570 border=0 cellspacing=0 cellpadding=0><tr>");
	websay("<font size=\"4\" color=\"%s\">Real Name (or finger)</font><br>",Web->color2);
	websay("<td width=30><br></td><td width=540>");
	websay("Your 'real name' shows up in finger listings, notify listings, and in Notes sent to other users.<br>");
	websay("<pre>");
	Web_fixforinput(Web->user->finger,display);
	websay("real name: <input type=\"text\" name=\"finger\" value=\"%s\" size=50 maxlength=%d>",display,LENGTH_FINGERINFO);
	websay("</pre>");
	websay("</td></tr></table>");
	websay("<br><br>");
	websay("");

	/* Draw Contact input */
	websay("<font size=\"4\" color=\"%s\">Contact (email & web)</font><br>",Web->color2);
	websay("<table width=570 border=0><tr>");
	websay("<td width=30><br></td><td width=540>");
	websay("Your email and web address can be set so that other users can find a way to contact you or so that they can check out your web page.  Don't forget to include the leading 'http://' in front of your web page.<br>");
	websay("<pre>");
	Web_fixforinput(Web->user->email,display);
	websay("email: <input type=\"text\" name=\"email\" value=\"%s\" size=50 maxlength=%d>",display,USERFIELDLEN);
	Web_fixforinput(Web->user->www,display);
	if(display[0])
		websay("  www: <input type=\"text\" name=\"www\" value=\"%s\" size=50 maxlength=%d>",display,USERFIELDLEN);
	else
		websay("  www: <input type=\"text\" name=\"www\" value=\"http://\" size=50 maxlength=%d>",USERFIELDLEN);
	websay("</pre>");
	websay("</td></tr></table>");
	websay("<br><br>");
	websay("");

	/* Draw Contact input */
	websay("<font size=\"4\" color=\"%s\">Birthday</font><br>",Web->color2);
	websay("<table width=570 border=0><tr>");
	websay("<td width=30><br></td><td width=540>");
	websay("Set your birthday and the %s will wish you a Happy Birthday on that day!  It also shows up under the command 'showbdays' and in your finger info, and can be used by others to find out when your birthday will be.<br>",Grufti->botnick);
	websay("(Specify birthdays using the format MM/DD/YY or MM/DD)");
	websay("<pre>");
	if(Web->user->bday[0])
	{
		if(Web->user->bday[2] == -1)
			sprintf(display,"%-2.2d/%-2.2d",Web->user->bday[0], Web->user->bday[1]);
		else
			sprintf(display,"%-2.2d/%-2.2d/%-4.2d",Web->user->bday[0], Web->user->bday[1], Web->user->bday[2]);
	}
	else
		strcpy(display,"");
	websay(" birthday: <input type=\"text\" name=\"bday\" value=\"%s\" size=50 maxlength=10>",display);
	websay("</pre>");
	websay("</td></tr></table>");
	websay("<br><br>");
	websay("");

	/* Draw Other input */
	websay("<font size=\"4\" color=\"%s\">User-Defined (other fields)</font><br>",Web->color2);
	websay("<table width=570 border=0><tr>");
	websay("<td width=30><br></td><td width=540>");
	websay("You can set up to 8 'other' slots, which can display any kind of information you choose.<br>");
	websay("Possible examples are: phone, work, ICQ #, music, job, school, hobbies, etc.");
	websay("<pre>");

	display[0] = 0;
	pre[0] = 0;
	if(Web->user->other1)
	{
		if(strchr(Web->user->other1,' '))
		{
			Web_fixforinput(Web->user->other1,display);
			split(pre,display);
			if(strlen(pre) > 7)
				pre[7] = 0;
		}
	}
	websay("other1: <input type=\"text\" name=\"other1_descript\" value=\"%s\" size=7 maxlength=7> <input type=\"text\" name=\"other1_value\" value=\"%s\" size=50 maxlength=%d>",pre,display,USERFIELDLEN);

	display[0] = 0;
	pre[0] = 0;
	if(Web->user->other2)
	{
		if(strchr(Web->user->other2,' '))
		{
			Web_fixforinput(Web->user->other2,display);
			split(pre,display);
			if(strlen(pre) > 7)
				pre[7] = 0;
		}
	}
	websay("other2: <input type=\"text\" name=\"other2_descript\" value=\"%s\" size=7 maxlength=7> <input type=\"text\" name=\"other2_value\" value=\"%s\" size=50 maxlength=%d>",pre,display,USERFIELDLEN);

	display[0] = 0;
	pre[0] = 0;
	if(Web->user->other3)
	{
		if(strchr(Web->user->other3,' '))
		{
			Web_fixforinput(Web->user->other3,display);
			split(pre,display);
			if(strlen(pre) > 7)
				pre[7] = 0;
		}
	}
	websay("other3: <input type=\"text\" name=\"other3_descript\" value=\"%s\" size=7 maxlength=7> <input type=\"text\" name=\"other3_value\" value=\"%s\" size=50 maxlength=%d>",pre,display,USERFIELDLEN);

	display[0] = 0;
	pre[0] = 0;
	if(Web->user->other4)
	{
		if(strchr(Web->user->other4,' '))
		{
			Web_fixforinput(Web->user->other4,display);
			split(pre,display);
			if(strlen(pre) > 7)
				pre[7] = 0;
		}
	}
	websay("other4: <input type=\"text\" name=\"other4_descript\" value=\"%s\" size=7 maxlength=7> <input type=\"text\" name=\"other4_value\" value=\"%s\" size=50 maxlength=%d>",pre,display,USERFIELDLEN);

	display[0] = 0;
	pre[0] = 0;
	if(Web->user->other5)
	{
		if(strchr(Web->user->other5,' '))
		{
			Web_fixforinput(Web->user->other5,display);
			split(pre,display);
			if(strlen(pre) > 7)
				pre[7] = 0;
		}
	}
	websay("other5: <input type=\"text\" name=\"other5_descript\" value=\"%s\" size=7 maxlength=7> <input type=\"text\" name=\"other5_value\" value=\"%s\" size=50 maxlength=%d>",pre,display,USERFIELDLEN);

	display[0] = 0;
	pre[0] = 0;
	if(Web->user->other6)
	{
		if(strchr(Web->user->other6,' '))
		{
			Web_fixforinput(Web->user->other6,display);
			split(pre,display);
			if(strlen(pre) > 7)
				pre[7] = 0;
		}
	}
	websay("other6: <input type=\"text\" name=\"other6_descript\" value=\"%s\" size=7 maxlength=7> <input type=\"text\" name=\"other6_value\" value=\"%s\" size=50 maxlength=%d>",pre,display,USERFIELDLEN);

	display[0] = 0;
	pre[0] = 0;
	if(Web->user->other7)
	{
		if(strchr(Web->user->other7,' '))
		{
			Web_fixforinput(Web->user->other7,display);
			split(pre,display);
			if(strlen(pre) > 7)
				pre[7] = 0;
		}
	}
	websay("other7: <input type=\"text\" name=\"other7_descript\" value=\"%s\" size=7 maxlength=7> <input type=\"text\" name=\"other7_value\" value=\"%s\" size=50 maxlength=%d>",pre,display,USERFIELDLEN);

	display[0] = 0;
	pre[0] = 0;
	if(Web->user->other8)
	{
		if(strchr(Web->user->other8,' '))
		{
			Web_fixforinput(Web->user->other8,display);
			split(pre,display);
			if(strlen(pre) > 7)
				pre[7] = 0;
		}
	}
	websay("other8: <input type=\"text\" name=\"other8_descript\" value=\"%s\" size=7 maxlength=7> <input type=\"text\" name=\"other8_value\" value=\"%s\" size=50 maxlength=%d>",pre,display,USERFIELDLEN);
	websay("</pre>");
	websay("</td></tr></table>");
	websay("<br><br>");
	websay("");

	/* Draw Location input */
	websay("<font size=\"4\" color=\"%s\">Location</font><br>",Web->color2);
	websay("<table width=570 border=0><tr>");
	websay("<td width=30><br></td><td width=540>");
	websay("You can specify your location by choosing a location from the following drop-down list, or entering a new one if yours is not listed.  Invalid locations will be deleted.<br>");
	websay("<select name=\"location\">");

	/* Loop through all locations */
	for(i=1; i<= Location->num_locations; i++)
	{
		l = Location_locationbyindex(i);
		if(l)
		{
			Location_ltostr(l,display);
			if(Web->user->location == l->id)
				websay("<option SELECTED value=\"%d\">%s</option>",l->id,display);
			else
				websay("<option value=\"%d\">%s</option>",l->id,display);
		}
	}

	if(Web->user->location == 0)
		websay("<option SELECTED value=\"0\">My location isn't listed! (Enter below)</option>");
	else
		websay("<option value=\"0\">My location isn't listed! (Enter below)</option>");

	websay("<option value=\"-1\">I don't want to enter a location.  Hrmph.</option>");

	websay("</select>");
	websay("<br><br>");
	websay("Enter your location below if it could not be found in the list.<br>");
	websay("City: <input type=\"text\" name=\"city\" size=18 maxlength=18> ");
	websay("State or Region: <input type=\"text\" name=\"state\" size=10 maxlength=10>" );
	websay("Country: <input type=\"text\" name=\"country\" size=10 maxlength=10>");
	websay("<br>");
	websay("Please make sure your State and Country are abbreviated and in the same form as the locations in the list.  If State or Region does not apply, leave blank.<br>");
	websay("</td></tr></table>");
	websay("<br><br>");
	websay("");

	/* Draw Plan input */
	websay("<font size=\"4\" color=\"%s\">Misc Info (ie. Plan)</font><br>",Web->color2);
	websay("<table width=570 border=0><tr>");
	websay("<td width=30><br></td><td width=540>");
	websay("Here you can enter something about yourself, like a favorite lyric or quote, rants on life, things you hate, your outlook on life, or just what you're all about.<br>");
	websay("<br>");
	websay("This information shows up on finger listings as one block of text.  Use each line separately to control how the text is broken up between lines.<br>");
	websay("<pre>");
	Web_fixforinput(Web->user->plan1,display);
	websay(" 1: <input type=\"text\" name=\"plan1\" value=\"%s\" size=64 maxlength=%d>",display,LENGTH_PLANLINE);
	Web_fixforinput(Web->user->plan2,display);
	websay(" 2: <input type=\"text\" name=\"plan2\" value=\"%s\" size=64 maxlength=%d>",display,LENGTH_PLANLINE);
	Web_fixforinput(Web->user->plan3,display);
	websay(" 3: <input type=\"text\" name=\"plan3\" value=\"%s\" size=64 maxlength=%d>",display,LENGTH_PLANLINE);
	Web_fixforinput(Web->user->plan4,display);
	websay(" 4: <input type=\"text\" name=\"plan4\" value=\"%s\" size=64 maxlength=%d>",display,LENGTH_PLANLINE);
	Web_fixforinput(Web->user->plan5,display);
	websay(" 5: <input type=\"text\" name=\"plan5\" value=\"%s\" size=64 maxlength=%d>",display,LENGTH_PLANLINE);
	Web_fixforinput(Web->user->plan6,display);
	websay(" 6: <input type=\"text\" name=\"plan6\" value=\"%s\" size=64 maxlength=%d>",display,LENGTH_PLANLINE);
	websay("</pre>");
	websay("</td></tr></table>");
	websay("<br><br>");
	websay("");

	/* Draw Greets input */
	if(Web->user->flags & USER_GREET)
	{
		websay("<font size=\"4\" color=\"%s\">%s Greetings</font><br>",Web->color2,Grufti->botnick);
		websay("<table width=570 border=0><tr>");
		websay("<td width=30><br></td><td width=540>");
		websay("Your greetings.  If this area is visible, congratulations!  You have the level +g.<br>");
		websay("<br>");
		websay("All rules still apply, meaning you need to include a '\\n' modifier somewhere in your greeting.  If you don't, the greeting will be discarded. Use '/m' or '/me' at the beginning of the greet to specify an ACTION.<br>");
		websay("<br>");
		websay("Example: <tt>Hi \\n!  It's nice to see you again.</tt><br>");
		websay("<pre>");
		g = Web->user->greets;
		for(i=1; i<=Grufti->max_greets; i++)
		{
			display[0] = 0;
			if(g)
			{
				Web_fixforinput(g->text,display);
				g = g->next;
			}
			websay("%2d: <input type=\"text\" name=\"greet%d\" value=\"%s\" size=64 maxlength=%d>",i,i,display,GREETLENGTH);
		}
		websay("</pre>");
		websay("</td></tr></table>");
		websay("<br><br>");
		websay("");
	}
	
	websay("<input type=\"submit\" value=\"Submit Changes\">");
	websay("<br><br><br>");

	closehtml();
}

void webcmd_putcmdline()
{
	char display[BUFSIZ];

	/* Display headers, statusline, and menu */
	sprintf(display,"%s - Command-Line Interface",Web->title);
	openhtml(display);
	websay_statusline();
	websay_menuline("Cmdline");
	websay("<form method=\"post\" action=\"%s?%s;%s;!cmdline_post\">",Web->cgi_url,Web->user->acctname,Web->user->pass);
	websay("");

	websay("<table width=570 border=0 cellspacing=0 cellpadding=0><tr><td>");
	websay("<font size=\"6\" color=\"%s\"><i>Command-Line Interface</i></font><br>", Web->color4);
	websay("Enter bot commands here just as if you were in a DCC or telnet session to the bot.<br>");
	websay("</td></tr></table>");
	websay("<br>");
	websay("");

	websay("<form method=\"post\" action=\"%s?%s;%s;!webcmd_lookup\">",Web->cgi_url,Web->user->acctname,Web->user->pass);
	websay("<input type=\"text\" name=\"cmdline\" value=\"\" size=60 maxlength=4095>");
	websay("<input type=\"submit\" value=\"Enter\">");

	websay("<br><br>");
	websay("<font size=\"6\" color=\"%s\"><i>Output:</i></font><br>", Web->color4);

	websay("<pre>");
	DCC_docommand(Web->d, Web->cmdparam);
	websay("</pre>");

	closehtml();
}
	
void webcmd_putsetup()
/*
 * Takes form input from page and updates user's information
 */
{
	char	*p, num[50], city[50], state[50], country[50];
	int	n;
	locarec	*l;

	if(!(Web->d->flags & STAT_DONTSHOWPROMPT))
	{
		/* Turn off prompt until we get entire form. */
		Web->d->flags |= STAT_DONTSHOWPROMPT;
	}

	/* Check for EOF */
	if(isequal(Web->tok1,"<EOF>"))
	{
		/* ok we're done. */
		Web->d->flags &= ~STAT_DONTSHOWPROMPT;

		/* Pass control to 'finger' */
		strcpy(Web->cmdparam,Web->user->acctname);
		strcpy(Web->tok1,Web->user->acctname);

		webcmd_finger();
		return;
	}

	/* Got finger info */
	if(isequal(Web->tok1,"finger"))
	{
		p = Web->cmdparam + 6;
		rmspace(p);
		if(*p)
		{
			if(strlen(p) > LENGTH_FINGERINFO)
				p[LENGTH_FINGERINFO] = 0;
			userrec_setfinger(Web->user,p);
		}
		else
		{
			xfree(Web->user->finger);
			Web->user->finger = NULL;
		}
	}
	else if(isequal(Web->tok1,"email"))
	{
		p = Web->cmdparam + 5;
		rmspace(p);
		if(*p)
		{
			if(strlen(p) > USERFIELDLEN)
				p[USERFIELDLEN] = 0;
			userrec_setemail(Web->user,p);
		}
		else
		{
			xfree(Web->user->email);
			Web->user->email = NULL;
		}
	}
	else if(isequal(Web->tok1,"www"))
	{
		p = Web->cmdparam + 3;
		rmspace(p);
		if(*p && !isequal(p,"http://"))
		{
			if(strlen(p) > USERFIELDLEN)
				p[USERFIELDLEN] = 0;
			userrec_setwww(Web->user,p);
		}
		else
		{
			xfree(Web->user->www);
			Web->user->www = NULL;
		}
	}
	else if(isequal(Web->tok1,"bday"))
	{
		char *q, date[80], elem[80];
		int	m, d, y, count = 0;

		p = Web->cmdparam + 4;
		rmspace(p);
		if(*p)
		{
			strcpy(date,p);
			for(q=date; *q; q++)
			{
				if(*q == '/')
				count++;
			}

			if(count < 1 || count > 2)
				return;

			splitc(elem,date,'/');
			m = elem[0] ? atoi(elem) : 0;

			if(count == 1)
			{
				d = date[0] ? atoi(date) : 0;
				y = -1;
			}
			else
			{
				splitc(elem,date,'/');
				d = elem[0] ? atoi(elem) : 0;
				y = date[0] ? atoi(date) : -1;
			}

			if(!check_date(m,d,y))
				return;

			Web->user->bday[0] = m;
			Web->user->bday[1] = d;

			if(y == 0)
				y = 2000;
			else if(y < 100 && y > 0)
				y += 1900;

			if(y < 0 || y > 3000)
				y = -1;

			Web->user->bday[2] = y;
		}
		else
			Web->user->bday[0] = 0;
	}

	else if(isequal(Web->tok1,"other1"))
	{
		p = Web->cmdparam + 6;
		rmspace(p);
		if(*p)
		{
			if(strlen(p) > (USERFIELDLEN + 8))
				p[USERFIELDLEN + 8] = 0;
			userrec_setother1(Web->user,p);
		}
		else
		{
			xfree(Web->user->other1);
			Web->user->other1 = NULL;
		}
	}
	else if(isequal(Web->tok1,"other2"))
	{
		p = Web->cmdparam + 6;
		rmspace(p);
		if(*p)
		{
			if(strlen(p) > (USERFIELDLEN + 8))
				p[USERFIELDLEN + 8] = 0;
			userrec_setother2(Web->user,p);
		}
		else
		{
			xfree(Web->user->other2);
			Web->user->other2 = NULL;
		}
	}
	else if(isequal(Web->tok1,"other3"))
	{
		p = Web->cmdparam + 6;
		rmspace(p);
		if(*p)
		{
			if(strlen(p) > (USERFIELDLEN + 8))
				p[USERFIELDLEN + 8] = 0;
			userrec_setother3(Web->user,p);
		}
		else
		{
			xfree(Web->user->other3);
			Web->user->other3 = NULL;
		}
	}
	else if(isequal(Web->tok1,"other4"))
	{
		p = Web->cmdparam + 6;
		rmspace(p);
		if(*p)
		{
			if(strlen(p) > (USERFIELDLEN + 8))
				p[USERFIELDLEN + 8] = 0;
			userrec_setother4(Web->user,p);
		}
		else
		{
			xfree(Web->user->other4);
			Web->user->other4 = NULL;
		}
	}
	else if(isequal(Web->tok1,"other5"))
	{
		p = Web->cmdparam + 6;
		rmspace(p);
		if(*p)
		{
			if(strlen(p) > (USERFIELDLEN + 8))
				p[USERFIELDLEN + 8] = 0;
			userrec_setother5(Web->user,p);
		}
		else
		{
			xfree(Web->user->other5);
			Web->user->other5 = NULL;
		}
	}
	else if(isequal(Web->tok1,"other6"))
	{
		p = Web->cmdparam + 6;
		rmspace(p);
		if(*p)
		{
			if(strlen(p) > (USERFIELDLEN + 8))
				p[USERFIELDLEN + 8] = 0;
			userrec_setother6(Web->user,p);
		}
		else
		{
			xfree(Web->user->other6);
			Web->user->other6 = NULL;
		}
	}
	else if(isequal(Web->tok1,"other7"))
	{
		p = Web->cmdparam + 6;
		rmspace(p);
		if(*p)
		{
			if(strlen(p) > (USERFIELDLEN + 8))
				p[USERFIELDLEN + 8] = 0;
			userrec_setother7(Web->user,p);
		}
		else
		{
			xfree(Web->user->other7);
			Web->user->other7 = NULL;
		}
	}
	else if(isequal(Web->tok1,"other8"))
	{
		p = Web->cmdparam + 6;
		rmspace(p);
		if(*p)
		{
			if(strlen(p) > (USERFIELDLEN + 8))
				p[USERFIELDLEN + 8] = 0;
			userrec_setother8(Web->user,p);
		}
		else
		{
			xfree(Web->user->other8);
			Web->user->other8 = NULL;
		}
	}
	else if(isequal(Web->tok1,"location"))
	{
		p = Web->cmdparam + 8;
		rmspace(p);
		split(num,p);
		n = atoi(num);
		if(n == -1)
		{
			Web->user->location = 0;
		}
		else if(n == 0)
		{
			splitc(city,p,'$');
			rmspace(city);
			splitc(state,p,'$');
			rmspace(state);
			strcpy(country,p);
			rmspace(country);
			if(!city[0] || !country[0])
				return;
			l = Location_locationbyarea(city,state,country);
			if(l == NULL)
				l = Location_addnewlocation(city,state,country,Web->user,Web->user->acctname);
			Web->user->location = l->id;
			Location_sort();
			Location->flags |= LOCATION_NEEDS_WRITE;
		}
		else
		{
			l = Location_location(n);
			if(l == NULL)
				Web->user->location = 0;
			else
				Web->user->location = n;
		}
	}
	else if(isequal(Web->tok1,"plan1"))
	{
		p = Web->cmdparam + 5;
		if(*p)
		{
			p++;
			if(strlen(p) > LENGTH_PLANLINE)
				p[LENGTH_PLANLINE] = 0;
			userrec_setplanx(Web->user,p,1);
		}
		else
		{
			xfree(Web->user->plan1);
			Web->user->plan1 = NULL;
		}
	}
	else if(isequal(Web->tok1,"plan2"))
	{
		p = Web->cmdparam + 5;
		if(*p)
		{
			p++;
			if(strlen(p) > LENGTH_PLANLINE)
				p[LENGTH_PLANLINE] = 0;
			userrec_setplanx(Web->user,p,2);
		}
		else
		{
			xfree(Web->user->plan2);
			Web->user->plan2 = NULL;
		}
	}
	else if(isequal(Web->tok1,"plan3"))
	{
		p = Web->cmdparam + 5;
		if(*p)
		{
			p++;
			if(strlen(p) > LENGTH_PLANLINE)
				p[LENGTH_PLANLINE] = 0;
			userrec_setplanx(Web->user,p,3);
		}
		else
		{
			xfree(Web->user->plan3);
			Web->user->plan3 = NULL;
		}
	}
	else if(isequal(Web->tok1,"plan4"))
	{
		p = Web->cmdparam + 5;
		if(*p)
		{
			p++;
			if(strlen(p) > LENGTH_PLANLINE)
				p[LENGTH_PLANLINE] = 0;
			userrec_setplanx(Web->user,p,4);
		}
		else
		{
			xfree(Web->user->plan4);
			Web->user->plan4 = NULL;
		}
	}
	else if(isequal(Web->tok1,"plan5"))
	{
		p = Web->cmdparam + 5;
		if(*p)
		{
			p++;
			if(strlen(p) > LENGTH_PLANLINE)
				p[LENGTH_PLANLINE] = 0;
			userrec_setplanx(Web->user,p,5);
		}
		else
		{
			xfree(Web->user->plan5);
			Web->user->plan5 = NULL;
		}
	}
	else if(isequal(Web->tok1,"plan6"))
	{
		p = Web->cmdparam + 5;
		if(*p)
		{
			p++;
			if(strlen(p) > LENGTH_PLANLINE)
				p[LENGTH_PLANLINE] = 0;
			userrec_setplanx(Web->user,p,6);
		}
		else
		{
			xfree(Web->user->plan6);
			Web->user->plan6 = NULL;
		}
	}
	else if(isequal(Web->tok1,"greet1"))
	{
		/* Delete all greets.  We'll be putting them back in shortly */
		User_freeallgreets(Web->user);

		p = Web->cmdparam + 6;
		if(*p)
		{
			p++;
			if(!strstr(p,"\\n"))
				return;
			if(strchr(p,'%'))
				return;
			if(strlen(p) > GREETLENGTH)
				p[GREETLENGTH] = 0;
			User_addnewgreet(Web->user,p);
		}
	}
	else if(isequal(Web->tok1,"greet"))
	{
		p = Web->cmdparam + 5;
		if(*p)
		{
			p++;
			if(!strstr(p,"\\n"))
				return;
			if(strchr(p,'%'))
				return;
			if(strlen(p) > GREETLENGTH)
				p[GREETLENGTH] = 0;
			User_addnewgreet(Web->user,p);
		}
	}
}

void webcmd_response()
{
	rtype	*rt;
	cmdrec_t	*c;
	resprec	*resp;
	int	col1 = 0, col2 = 0, i, tot;

	/* Display headers, statusline, and menu */
	sprintf(Web->display,"%s - Responses",Web->title);
	openhtml(Web->display);
	websay_statusline();
	websay_menuline("Response");
	websay("<br>");

	websay("<table width=570 border=0 cellspacing=0 cellpadding=0><tr><td>");
	websay("<font size=\"6\" color=\"%s\"><i>%s's Responses</i></font><br>",Web->color4,Grufti->botnick);
	websay("Here you can see all of %s's responses, everything the bot can say, do, and sing.  Click on a response name to see the actual responses.",Grufti->botnick);
	
	/*
	 * get the flags for the addrespline command, and we'll see if the
	 * user has em.
	 */
	c = command_get_entry(commands, "addrespline");
	if(c == NULL)
	{
		Log_write(LOG_ERROR,"*","Hey, the adddrespline command is missing from my list!!");
		return;
	}

	if(c->levels & Web->user->flags)
		websay("You posess the ability to create new responses and edit existing ones.<br>");

	websay("</td></tr></table>");
	websay("<br><br>");
	websay("");

	if(c->levels & Web->user->flags)
	{
		websay("<font size=\"4\" color=\"%s\">Create a New Response</font><br>",Web->color2);
		websay("<table width=570 border=0 cellpadding=0 cellspacing=1><tr>");
		websay("<td width=30><br></td><td width=538>");
		websay("Create a new response here.  Response names must be ONE WORD ONLY! If spaces are inserted, they will be changed to the underscore character.<br>");
		websay("<br>");
		websay("A public response matches anything said on the channel.  A private response matches anything said which contains the bot's nick.  Other response types have additional matching characteristics; please consult your botmaster for further information.");
		websay("<table width=538 border=0 cellspacing=0 cellpadding=1>");
		for(rt=Response->rtypes; rt!=NULL; rt=rt->next)
		{
			/* display valid response types for user's level */
			if(rt->level & Web->user->flags)
			{
				websay("<form method=\"post\" action=\"%s?%s;%s;!new_response\">",Web->cgi_url,Web->user->acctname,Web->user->pass);
				websay("<tr><td>New '%s' response:  </td>",rt->name);
				websay("<td><input type=\"hidden\" name=\"type\" value=\"%s\"><input type=\"text\" name=\"respname\" size=25 maxlength=25>",rt->name);
				websay("<input type=\"submit\" value=\"Create Response\"></td></tr>",rt->name);
				websay("</form>");
			}
		}
		websay("</table>");
		websay("</td></tr></table>");
		websay("<br><br>");
		websay("");
#if 0
		websay("<div align=center>( | ",Web->color1);
		for(rt=Response->rtypes; rt!=NULL; rt=rt->next)
			websay("<a href=\"#%s\">%s</a> |",rt->name,rt->name);
                websay(")</font></div><br><br>");
#endif

	}

	/* We'll let everyone look at the responses */
	for(rt=Response->rtypes; rt!=NULL; rt=rt->next)
	{	
		/* Skip internal responses for now */
		if(isequal(rt->name,"internal"))
			continue;

		websay("<table width=570 border=0 bgcolor=\"%s\"><tr><td>",Web->color1);
		websay("<font size=\"4\" color=\"%s\">Response type: <font face=\"arial\" size=\"3\"><a name=\"%s\"><b>%s</b></a></font></font><br>",Web->color2,rt->name,rt->name);
		websay("<font size=\"3\">%d records</font>",rt->num_responses);
		websay("</td></tr></table>");
		
		websay("<table width=570 border=0><tr>");
		websay("<td width=30><br></td><td width=540>");
	
		websay("<table width=540 border=0><tr>");
		websay("<td valign=top>");
		col1 = 1;
		i = 0;
		tot = Response_orderbyresponse(rt);
		for(i=1; i<=tot; i++)
		{
			resp = Response_responsebyorder(rt,i);
			if(col1 && i > (rt->num_responses / 3))
			{
				col1 = 0;
				col2 = 1;
				websay("</td><td valign=top>");
			}
			if(col2 && i > 2*(rt->num_responses / 3))
			{
				col2 = 0;
				websay("</td><td valign=top>");
			}
			websay("<a href=\"%s?%s;%s;VResponse:%s:%s\">%s</a><br>",Web->cgi_url,Web->user->acctname,Web->user->pass,rt->name,resp->name,resp->name);
		}
		websay("</td></tr></table>");

		websay("</td></tr></table>");
		websay("<br><br>");
	}

	websay("<br>");
	websay("<br>");
	websay("");

	closehtml();
}

void webcmd_putnewresponse()
{
	char	tmp[100], save[256];
	rtype	*rt;
	resprec	*resp;

	/* First thing.  Get response and type */
	strcpy(save,Web->cmdparam);
	split(tmp,save);
	rt = Response_type(tmp);
	if(rt == NULL)
		return;

	if(!(Web->user->flags & USER_MASTER) && !(Web->user->flags & rt->level))
		return;

	split(tmp,save);
	strcpy(Web->cmdparam,save);
	resp = Response_response(rt,tmp);
	if(resp == NULL)
	{
		Response_addnewresponse(rt,tmp);
		if(Grufti->send_notice_response_created)
			Note_sendnotification("%s has created the %s response \"%s\".",Web->user->acctname,rt->name,tmp);
		Response->flags |= RESPONSE_NEEDS_WRITE;
	}
	webcmd_vresponse();
}

void webcmd_vresponse()
{
	rtype	*rt;
	resprec	*resp;
	rline	*rl;
	machrec	*mr;
	char	display[BUFSIZ];
	int	num_matches = 0, num_excepts = 0, num_lines = 0, i;

	/* Display headers, statusline, and menu */
	sprintf(display,"%s - View Response %s:%s",Web->title,Web->tok1,Web->tok2);
	openhtml(display);
	websay_statusline();
	sprintf(display,"VResponse:%s:%s",Web->tok1,Web->tok2);
	websay_menuline(display);
	websay("<br>");

	rt = Response_type(Web->tok1);
	if(rt == NULL)
		return;

	if(rt->level & Web->user->flags)
		websay("<form method=\"post\" action=\"%s?%s;%s;!response_post\">",Web->cgi_url,Web->user->acctname,Web->user->pass);
		

	resp = Response_response(rt,Web->tok2);
	if(resp == NULL)
		return;

	if((Web->user->flags & USER_MASTER) || (Web->user->flags & rt->level))
	{
		websay("<table width=570 border=0><tr><td>");
		websay("You possess the ability to make changes to this response.  Ten extra lines are provided for adding new lines.  If you need to add more responses, just submit these, and you will return to this screen with 10 more blank lines.<br>");
		websay("<br>");
		websay("Remember that multiple match lines evaluate on and AND basis so if you are using the 2nd match line, one element from both lines must exist in the string to match or the evaluation will be false.");
		websay("If an except line is used, evaluation will be false when any element in the except line exists in the string to match.<br>");
		websay("<br>");
		websay("Click <a href=\"%s?%s;%s;DResponse:%s:%s\">here</a> to delete this response.",Web->cgi_url,Web->user->acctname,Web->user->pass,rt->name,resp->name);
		websay("</td></tr></table>");
	}
	websay("<br>");

	websay("<font size=\"5\" color=\"%s\">Response: %s (%s)</font><br>",Web->color2,resp->name,rt->name);

	if(!isequal(rt->name,"internal"))
	{
		if(Web->user->flags & rt->level)
			websay("<pre>");
	
		if(resp->matches == NULL)
		{
			if(Web->user->flags & rt->level)
			{
				websay("Matches: <input type=\"text\" name=\"matches0\" size=40 maxlength=300>");
				websay("Matches: <input type=\"text\" name=\"matches1\" size=40 maxlength=300>");
				num_matches = 2;
			}
			else
			{
				websay("<font size=\"4\">Matches: (no match lines set!)</font><br>");
			}
		}
		else
		{
			i = 0;
			for(mr=resp->matches; mr!=NULL; mr=mr->next)
			{
				if(Web->user->flags & rt->level)
				{
					Web_fixforinput(mr->match,display);
					websay("Matches: <input type=\"text\" name=\"matches%d\" value=\"%s\" size=40 maxlength=300>",i,display);
					i++;
				}
				else
					websay("<font size=\"4\">Matches: %s</font><br>",mr->match);
			}
			if(Web->user->flags & rt->level && resp->num_matches == 1)
			{
				websay("Matches: <input type=\"text\" name=\"matches1\" size=40 maxlength=300>");
				num_matches = 2;
			}
			else
				num_matches = resp->num_matches;
		}
	
		if(resp->except != NULL)
		{
			i = 0;
			for(mr=resp->except; mr!=NULL; mr=mr->next)
			{
				if(Web->user->flags & rt->level)
				{
					Web_fixforinput(mr->match,display);
					websay(" Except: <input type=\"text\" name=\"excepts%d\" value=\"%s\" size=40 maxlength=300>",i,display);
					i++;
					num_excepts = resp->num_excepts;
				}
				else
					websay("<font size=\"4\"> Except: %s</font><br>",mr->match);
			}
		}
		else
		{
			if(Web->user->flags & rt->level)
			{
				websay(" Except: <input type=\"text\" name=\"excepts0\" size=40 maxlength=300>");
				num_excepts = 1;
			}
		}
	
		if(Web->user->flags & rt->level)
			websay("</pre>");
	}

	websay("<br>");
	websay("<font size=\"4\">%d response line%s</font><br>",resp->num_lines,resp->num_lines==1?"":"s");
	if(!(Web->user->flags & rt->level))
	{
		websay("<table width=570 border=0><tr>");
		websay("<td width=30><br></td><td width=540>");
		websay("<table width=540 border=0 cellspacing=0 cellpadding=4>");
	}

	i = 0;
	for(rl=resp->lines; rl!=NULL; rl=rl->next)
	{
		if(Web->user->flags & rt->level)
		{
			Web_fixforinput(rl->text,display);
			websay("<input type=\"text\" name=\"line%d\" value=\"%s\" size=78 maxlength=300><br>",i,display);
			i++;
		}
		else
			websay("<tr><td>%s</td></tr>",rl->text);
	}

	/* Draw 10 more lines for RESPONSE people */
	if(Web->user->flags & rt->level)
	{
		for(i=resp->num_lines; i<10+resp->num_lines; i++)
			websay("<input type=\"text\" name=\"line%d\" size=78 maxlength=300><br>",i);
		num_lines = resp->num_lines + 10;
	}

	if(!(Web->user->flags & rt->level))
	{
		websay("</table>");
		websay("</td></tr></table>");
	}
	websay("<br>");


	if(Web->user->flags & rt->level)
	{
		/* Put hidden inputs saying how many of each type we have */
		websay("<input type=\"hidden\" name=\"num_matches\" value=\"%d\">",num_matches);
		websay("<input type=\"hidden\" name=\"num_excepts\" value=\"%d\">",num_excepts);
		websay("<input type=\"hidden\" name=\"num_lines\" value=\"%d\">",num_lines);

		/* And what type and response it is */
		websay("<input type=\"hidden\" name=\"type\" value=\"%s\">",rt->name);
		websay("<input type=\"hidden\" name=\"response\" value=\"%s\">",resp->name);

		websay("<input type=\"submit\" value=\"Submit Changes\">");
	}

	closehtml();
}

void webcmd_dresponse()
{
	rtype	*rt;
	resprec	*resp;
	char	tmp[BUFSIZ];

	/* First thing.  Get response and type */
	split(tmp,Web->cmdparam);
	rt = Response_type(tmp);
	if(rt == NULL)
		return;

	if(!(Web->user->flags & rt->level))
		return;

	split(tmp,Web->cmdparam);
	resp = Response_response(rt,tmp);
	if(resp == NULL)
		return;

	if(Grufti->send_notice_response_deleted)
		Note_sendnotification("%s has deleted the %s response \"%s\".",Web->user->acctname,rt->name,resp->name);

	Response_deleteresponse(rt,resp);
	Response->flags |= RESPONSE_NEEDS_WRITE;

	/* Return to Response list */
	strcpy(Web->cmdparam,"Response");
	strcpy(Web->tok1,"");
	strcpy(Web->tok2,"");
	webcmd_response();
}

void webcmd_putresponse()
{
	char	tmp[100];
	rtype	*rt;
	resprec	*resp;

	if(!(Web->d->flags & STAT_DONTSHOWPROMPT))
	{
		/* Turn off prompt until we get entire form. */
		Web->d->flags |= STAT_DONTSHOWPROMPT;
	}

	/* First thing.  Get response and type */
	split(tmp,Web->cmdparam);
	rt = Response_type(tmp);
	if(rt == NULL)
		return;

	/* If user isn't verified, no go. */
	if(!(Web->user->flags & rt->level))
		return;

	split(tmp,Web->cmdparam);
	resp = Response_response(rt,tmp);
	if(resp == NULL)
		return;

	split(Web->tok1,Web->cmdparam);
	rmspace(Web->cmdparam);

	/* Check for EOF */
	if(isequal(Web->tok1,"<EOF>"))
	{
		/* ok we're done. */
		Web->d->flags &= ~STAT_DONTSHOWPROMPT;
		Response->flags |= RESPONSE_NEEDS_WRITE;

		/* Pass control to 'vresponse' */
		sprintf(Web->cmdparam,"%s %s",rt->name,resp->name);
		strcpy(Web->tok1,rt->name);
		strcpy(Web->tok2,resp->name);

		webcmd_vresponse();
		return;
	}

	if(isequal(Web->tok1,"init"))
	{
		/* This means clear out existing response, ready for input */
		Response_freeallmatches(resp);
		Response_freeallexcepts(resp);
		Response_freealllines(resp);
	}
	else if(isequal(Web->tok1,"matches"))
	{
		if(Web->cmdparam[0])
			Response_addmatches(resp,Web->cmdparam);
	}
	else if(isequal(Web->tok1,"excepts"))
	{
		if(Web->cmdparam[0])
			Response_addexcept(resp,Web->cmdparam);
	}
	else if(isequal(Web->tok1,"line"))
	{
		if(Web->cmdparam[0])
			Response_addline(resp,Web->cmdparam);
	}
}
	
void webcmd_webcatch()
{
	urlrec  *url;
	userrec	*u;
	int     count = 0, num = URL->num_urls;
	char    when[TIMESHORTLEN];

	/* Display headers, statusline, and menu */
	sprintf(Web->display,"%s - URL Catcher",Web->title);
	openhtml(Web->display);
	websay_statusline();
	websay_menuline("Webcatch");
	websay("<br>");

	websay("<table width=570 border=0 cellspacing=0 cellpadding=0><tr><td>");
	websay("<font size=\"6\" color=\"%s\"><i>%s's URL Catcher</i></font><br>",Web->color4,Grufti->botnick);
	websay("The following is an unvalidated list of the last %d URL%s caught by the bot from any of its channels.  Click on the URL to visit it. Hyperlinked nicks denote users who are registered with the bot.  Click on the nick to view their finger information.<br>",num,num==1?"":"'s");
	websay("<br>");
	websay("<b>Note:</b> The maintainer of this site does not accept any responsibility for the content of the following link%s.<br><br>",num==1?"":"s");
	websay("</td></tr></table>");
	websay("<br>");
	websay("");

        if(!URL->num_urls)
        {
                websay("No URL's caught yet.  Try again later!<br>");
                return;
        }
	

	/* Display header */
	websay("<table width=570 border=0 cellspacing=1 cellpadding=0>");
	websay("<tr>");
	websay("<td bgcolor=\"%s\" width=30><font face=\"arial\" size=\"2\">No.</font></td>",Web->color1);
	websay("<td width=5><font face=\"arial\" size=\"2\" color=\"%s\">.</font></td>",Web->bgcolor);
	websay("<td bgcolor=\"%s\" width=40 align=center><font face=\"arial\" size=\"2\">When</font></td>",Web->color1);
	websay("<td width=5><font face=\"arial\" size=\"2\" color=\"%s\">.</font></td>",Web->bgcolor);
	websay("<td bgcolor=\"%s\" width=88><font face=\"arial\" size=\"2\">From</font></td>",Web->color1);
	websay("<td width=5><font face=\"arial\" size=\"2\" color=\"%s\">.</font></td>",Web->bgcolor);
	websay("<td bgcolor=\"%s\" width=389><font face=\"arial\" size=\"2\">URL</font></td>",Web->color1);

	/* List URL's */
	for(url=URL->urllist; url!=NULL; url=url->next)
	{
		count++;
		timet_to_time_short(url->time_caught,when);

		websay("<tr>");
		websay("<td width=30><font size=\"2\"><div align=right><b>%2d</b></div></font></td>",count);
		websay("<td width=5><font size=\"2\" color=\"%s\">.</font></td>",Web->bgcolor);
		websay("<td width=40 align=center><font size=\"2\">%s</font></td>",when);
		websay("<td width=5><font size=\"2\" color=\"%s\">.</font></td>",Web->bgcolor);

		/* Display nick */
		u = User_user(url->nick);
		if(u->registered)
			websay("<td width=88><a href=\"%s?%s;%s;Finger:%s\"><font size=\"2\">%s</font></a></td>",Web->cgi_url,Web->user->acctname,Web->user->pass,u->acctname,u->acctname);
		else
			websay("<td width=88><font size=\"2\">%s</font></td>",url->nick);

		websay("<td width=5><font size=\"2\" color=\"%s\">.</font></td>",Web->bgcolor);
		websay("<td width=389><font size=\"2\"><a href=\"%s\" target=\"_top\">%s</a></font></td>",url->url,url->url);
	}

	/* Footer */	
	websay("<tr>");
	websay("<td bgcolor=\"%s\" width=30><font size=\"2\" color=\"%s\">.</font></td>",Web->color1,Web->color1);
	websay("<td width=5><font face=\"arial\" size=\"2\" color=\"%s\">.</font></td>",Web->color1);
	websay("<td bgcolor=\"%s\" width=40><font size=\"2\" color=\"%s\">.</font></td>",Web->color1,Web->color1);
	websay("<td width=5><font face=\"arial\" size=\"2\" color=\"%s\">.</font></td>",Web->color1);
	websay("<td bgcolor=\"%s\" width=88><font size=\"2\" color=\"%s\">.</font></td>",Web->color1,Web->color1);
	websay("<td width=5><font face=\"arial\" size=\"2\" color=\"%s\">.</font></td>",Web->color1);
	websay("<td bgcolor=\"%s\" width=389><font size=\"2\" color=\"%s\">.</font></td>",Web->color1,Web->color1);
	websay("</tr>");
	websay("</table>");
	closehtml();

}

void webcmd_stat()
{
	int	seen_people = 0, cmds_people = 0, cmds = 0, via_web = 0;
	int	avg_counter = 0, num_hostmasks = 0, num_uhostmasks = 0;
	int	num_notes = 0, num_note_users = 0, num_notes_sent = 0;
	int	joins = 0, joins_reg = 0, chatter = 0, i;
	int	resp_counter = 0, usage_people = 0;
	char	datestr[TIMELONGLEN];
	char	date[TIMELONGLEN], online_time[TIMELONGLEN], memory_used[25];
	char	when_cleaned[TIMELONGLEN];
	char	b_in[25], b_out[25], avg_lag[TIMESHORTLEN];
	userrec	*u;
	servrec	*serv;
	noterec	*note;
	chanrec	*chan;
	resprec	*r;
	rtype	*rt;
	time_t	t, now = time(NULL);	
	int	serv_connected = 0;
	float	avg_note_age = 0, avg_user_age = 0;
	float	avg_last_seen = 0, avg_usage = 0, avg_usage_partial;
	float	avg_last_login = 0;
	int	login_people = 0;

	/* Display headers, statusline, and menu */
	sprintf(Web->display,"%s - Status",Web->title);
	openhtml(Web->display);
	websay_statusline();
	websay_menuline("Status");
	websay("<br>");

	websay("<table width=570 border=0 cellspacing=0 cellpadding=0><tr><td>");
	timet_to_date_short(now,datestr);
	split(date,datestr);
	websay("<font size=\"6\" color=\"%s\"><i>Summary for %s</i></font><br>",Web->color4,date);
	websay("This page displays %s's daily statistics regarding usage, storage, channel, server, and user activity.  The statistics reflect one day's worth of activity, so some results may not be particularly interesting unless they are viewed near the end of the day.<br>",Grufti->botnick);
	websay("</td></tr></table>");
	websay("<br><br>");
	websay("");
	

	/* List all the statistics for the day */
	websay("<table width=570 border=0 cellspacing=0 cellpadding=0>");
	timet_to_ago_long(Grufti->online_since,online_time);
	comma_number(memory_used,net_sizeof() + misc_sizeof() + gruftiaux_sizeof() + blowfish_sizeof() + main_sizeof() + fhelp_sizeof() + User_sizeof() + URL_sizeof() + Server_sizeof() + Response_sizeof() + Notify_sizeof() + Log_sizeof() + Location_sizeof() + Grufti_sizeof() + DCC_sizeof() + command_sizeof() + Codes_sizeof() + Channel_sizeof());
	websay("<tr>");
	websay("<td bgcolor=\"%s\"><font face=\"arial\" size=2><b>Online:</b></font> %s</td>",Web->color1,online_time);
	websay("<td bgcolor=\"%s\"><font face=\"arial\" size=2><b>Memory:</b></font> %s bytes</td>",Web->color1,memory_used);
	websay("</tr><tr><td colspan=2><br></td></tr>");
	if(Grufti->cleanup_time == 0L)
		websay("<tr><td colspan=2><div align=center><font face=\"arial\">No cleanup information is available</font></div></td></tr>");
	else
	{
		timet_to_date_short(Grufti->cleanup_time,when_cleaned);
		websay("<tr><td bgcolor=\"%s\" colspan=2><font face=\"arial\" size=2><b>Auto Cleanup Summary </b></font>(%s)</td></tr>",Web->color1,when_cleaned);
		websay("<tr><td colspan=2><ul>%d user%s deleted (timeout: %d days)</ul></td></tr>",Grufti->users_deleted,Grufti->users_deleted==1 ? " was":"s were",Grufti->timeout_users);
		websay("<tr><td colspan=2><ul>%d hostmask%s deleted (timeout: %d days)</ul></td></tr>",Grufti->accts_deleted,Grufti->accts_deleted==1 ? " was":"s were",Grufti->timeout_accounts);
		websay("<tr><td colspan=2><ul>%d note%s deleted (timeout: %d days)</ul></td></tr>",Grufti->notes_deleted,Grufti->notes_deleted==1 ? " was":"s were",Grufti->timeout_notes);
	}
	websay("<tr><td colspan=2><br></td></tr>");

	t = time_today_began(time(NULL));
	for(u=User->userlist; u!=NULL; u=u->next)
	{
		if(u->registered)
			num_hostmasks += u->num_accounts;
		else
			num_uhostmasks += u->num_accounts;

		if(!u->registered)
			continue;

		if(u->bot_commands)
		{
			cmds_people++;
			cmds += u->bot_commands;
		}

		if(u->via_web)
			via_web++;

		if(u->last_seen >= t)
			seen_people++;

		if(u->last_login >= t)
			login_people++;

		avg_usage_partial = User_timeperweek(u);

		if(avg_usage_partial > 0)
		{
			avg_usage += ((float)(avg_usage_partial) / (3600.0 * 7.0));
			usage_people++;
		}

		avg_last_seen += ((float)(now - u->last_seen) / 86400.0);
		avg_last_login += ((float)(now - u->last_login) / 86400.0);
		avg_user_age += ((float)(now - u->time_registered) / 86400.0);

		if(u->num_notes)
		{
			for(note=u->notes; note!=NULL; note=note->next)
			{
				avg_note_age += ((float)(now - note->sent) / 86400.0);
				if(note->sent >= t)
					num_notes_sent++;
			}

			num_notes += u->num_notes;
			num_note_users++;
		}

		avg_counter ++;
	}

	if(num_notes)
		avg_note_age /= num_notes;
	else
		avg_note_age = 0;

	if(avg_counter)
	{
		avg_last_seen /= avg_counter;
		avg_last_login /= avg_counter;
		avg_user_age /= avg_counter;
	}
	else
	{
		avg_last_seen = 0;
		avg_last_login = 0;
		avg_user_age = 0;
	}

	if(usage_people)
		avg_usage /= usage_people;
	else
		avg_usage = 0;

	websay("<tr><td bgcolor=\"%s\" colspan=2><font face=\"arial\" size=2><b>Overall User Statistics</b></font></td></tr>",Web->color1);
	websay("<tr><td colspan=2><ul>%d user%s registered (%d hostmask%s)</ul></td></tr>",User->num_users - 1,User->num_users-1 == 1?" is":"s are",num_hostmasks,num_hostmasks==1?"":"s");
	websay("<tr><td colspan=2><ul>%d unregistered hostmasks are being tracked</ul></td></tr>",num_uhostmasks);
	if(num_note_users)
		websay("<tr><td colspan=2><ul>%d Note%s being stored by %d user%s (%2.1f notes/user)</ul></td></tr>",num_notes,num_notes==1?" is":"s are",num_note_users,num_note_users==1?"":"s",(float)(num_notes)/(float)(num_note_users));
	else
		websay("<tr><td colspan=2><ul>%d Note%s being stored by %d user%s",num_notes,num_notes==1?" is":"s are",num_note_users,num_note_users==1?"</ul></td></tr>":"s</ul></td></tr>");
	websay("<tr><td colspan=2><ul>%d Note%s been sent today, %d %s forwarded</ul></td></tr>",num_notes_sent,num_notes_sent==1?" has":"s have", Grufti->notes_forwarded,Grufti->notes_forwarded==1?"was":"were");
	websay("<tr><td colspan=2><ul>Average %s note is %2.1f days old</ul></td></tr>",Grufti->botnick,avg_note_age);
	websay("<tr><td colspan=2><ul>Average %s user account is %2.1f days old</ul></td></tr>",Grufti->botnick,avg_user_age);
	websay("<tr><td colspan=2><ul>Average %s user was seen %2.1f days ago</ul></td></tr>",Grufti->botnick,avg_last_seen);
	websay("<tr><td colspan=2><ul>Average %s user last logged in %2.1f days ago</ul></td></tr>",Grufti->botnick,avg_last_login);
	websay("<tr><td colspan=2><ul>Average time a %s user is on IRC is %2.1f hrs/day</ul></td></tr>",Grufti->botnick,avg_usage);
	websay("<tr><td colspan=2><br></td></tr>");

	websay("<tr><td bgcolor=\"%s\" colspan=2><font face=\"arial\" size=2><b>Of the %d registered user%s:</b></font></td></tr>",Web->color1,User->num_users - 1, (User->num_users - 1) == 1? "":"s");
	websay("<tr><td colspan=2><ul>%d %s queried the bot via IRC today (%d command%s)</ul></td></tr>",cmds_people,cmds_people==1?"has":"have",cmds,cmds==1?"":"s");
	websay("<tr><td colspan=2><ul>%d %s logged in from the web or via telnet</ul></td></tr>",via_web,via_web==1?"has":"have");
	websay("<tr><td colspan=2><ul>%d %s logged in to their account in general</ul></td></tr>",login_people,login_people==1?"has":"have");
	websay("<tr><td colspan=2><ul>%d %s been seen online</ul></td></tr>",seen_people,seen_people==1?"has":"have");
	websay("<tr><td colspan=2><br></td></tr>");

	websay("<tr><td bgcolor=\"%s\" colspan=2><font face=\"arial\" size=2><b>Response Statistics</b></font></td></tr>",Web->color1);
	for(rt=Response->rtypes; rt!=NULL; rt=rt->next)
	{
		resp_counter = 0;
		for(r=rt->responses; r!=NULL; r=r->next)
		{
			if(r->last >= t)
				resp_counter++;
		}

		websay("<tr><td colspan=2><ul>%d of my %d <a href=\"%s?%s;%s;Response#%s\">%s</a> response%s matched</ul></td></tr>",resp_counter,rt->num_responses,Web->cgi_url,Web->user->acctname,Web->user->pass,rt->name,rt->name,resp_counter==1?" was":"s were");
	}
	websay("<tr><td colspan=2><br></td></tr>");

	websay("<tr><td bgcolor=\"%s\" colspan=2><font face=\"arial\" size=2><b>Channel Statistics</b></font></td></tr>",Web->color1);
	for(chan=Channel->channels; chan!=NULL; chan=chan->next)
	{
		joins = 0;
		joins_reg = 0;
		chatter = 0;
		for(i=0; i<=thishour(); i++)
		{
			joins += chan->joins_reg[i] + chan->joins_nonreg[i];
			joins_reg += chan->joins_reg[i];
			chatter += chan->chatter[i];
		}
		websay("<tr><td colspan=2><ul><a href=\"%s?%s;%s;Channel:%s\">%s</a> had %d join%s (%d by reg'd user%s), %dk chatter</ul></td></tr>",Web->cgi_url,Web->user->acctname,Web->user->pass,chan->name + 1,chan->name,joins,joins==1?"":"s",joins_reg,joins_reg==1?"":"s",chatter/1024);
	}
	websay("<tr><td colspan=2><br></td></tr>");

	/* Display server statistics */
	for(serv=Server->serverlist; serv!=NULL; serv=serv->next)
		serv_connected += Server_timeconnected(serv);

	websay("<tr><td bgcolor=\"%s\" colspan=2><font face=\"arial\" size=2><b>Server Statistics </b></font>(Online %2.1f hrs today)</td></tr>",Web->color1,(float)serv_connected / 3600.0);
	for(serv=Server->serverlist; serv!=NULL; serv=serv->next)
	{
		if(Server_timeconnected(serv))
		{
			bytes_to_kb_short(serv->bytes_in,b_in);
			bytes_to_kb_short(serv->bytes_out,b_out);
			time_to_elap_short(Server_avglag(serv),avg_lag);
			websay("<tr><td colspan=2><ul>%s - %2.1f hrs, %s in, %s out, %s lag</ul></td></tr>",serv->name,(float)Server_timeconnected(serv)/3600.0,b_in,b_out,avg_lag);
		}
	}
	websay("</table>");
	websay("<br><br>");
}

void webcmd_showevents()
{
	evntrec	*event;
	char	display[BUFSIZ], name[80], place[80], when[80];

	/* Display headers, statusline, and menu */
	sprintf(display,"%s - Event Calendar",Web->title);
	openhtml(display);
	websay_statusline();
	sprintf(display,"showevents");

	sprintf(display,"showevents");
	websay_menuline(display);
	websay("<br>");
	websay("<table width=570 border=0 cellspacing=0 cellpadding=0><tr><td>");
	websay("<font size=\"6\" color=\"%s\"><i>%s's Event Calendar</i></font><br>",Web->color4,Grufti->botnick);
	websay("<font size=\"3\">");
	websay("The following is a listing of channel-oriented events that have been submitted by channel members.  Use this page to see what's happening and who's going to be there.<br>");
	websay("<br>");
	websay("Click on any event to see its info and a listing of attendees.");
	websay("</font></td></tr></table>");
	websay("<br>");
	websay("");

	if(Event->num_events == 0)
	{
		websay("There are no Events in the Event list.");
		websay("<br>");
	}

	/* Display header */
	websay("<table width=570 border=0 cellspacing=1 cellpadding=0>");
	websay("<tr>");
	websay("<td bgcolor=\"%s\" width=18><font face=\"arial\" size=\"2\">#</font></td>",Web->color1);
	websay("<td width=5><font face=\"arial\" size=\"2\" color=\"%s\">.</font></td>",Web->bgcolor);
	websay("<td bgcolor=\"%s\" width=220><font face=\"arial\" size=\"2\">Event</font></td>",Web->color1);
	websay("<td width=5><font face=\"arial\" size=\"2\" color=\"%s\">.</font></td>",Web->bgcolor);
	websay("<td bgcolor=\"%s\" width=159><font face=\"arial\" size=\"2\">Place</font></td>",Web->color1);
	websay("<td width=5><font face=\"arial\" size=\"2\" color=\"%s\">.</font></td>",Web->bgcolor);
	websay("<td bgcolor=\"%s\" width=98><font face=\"arial\" size=\"2\">When</font></td>",Web->color1);
	websay("<td width=5><font face=\"arial\" size=\"2\" color=\"%s\">.</font></td>",Web->bgcolor);
	websay("<td bgcolor=\"%s\" width=45><font face=\"arial\" size=\"2\"># Att</font></td>",Web->color1);
	websay("</tr></table>");
	websay("");

	/* Ok here we go */
	for(event=Event->events; event!=NULL; event=event->next)
	{
		websay("<table width=570 border=0 cellspacing=1 cellpadding=0><tr>");

		/* Display event ID */
		websay("<td width=18 align=center><font size=\"2\" face=\"arial\">%d</font></td>",event->id);
		websay("<td width=5><font size=\"2\" color=\"%s\">.</font></td>",Web->bgcolor);

		/* Display event name */
		if(event->name)
		{
			strcpy(name,event->name);
			if(strlen(name) > 25)
				name[25] = 0;
		}
		else
			strcpy(name,"");
		websay("<td width=220><a href=\"%s?%s;%s;ShowEvent:%d\"><font size=\"2\" face=\"arial\">%s</font></a></td>",Web->cgi_url,Web->user->acctname,Web->user->pass,event->id,name);
		websay("<td width=5><font size=\"2\" color=\"%s\">.</font></td>",Web->bgcolor);

		/* Display event place */
		if(event->place)
		{
			strcpy(place,event->place);
			if(strlen(place) > 18)
				place[18] = 0;
		}
		else
			strcpy(place,"");
		websay("<td width=159><font size=\"2\" face=\"arial\">%s</font></td>",place);
		websay("<td width=5><font size=\"2\" color=\"%s\">.</font></td>",Web->bgcolor);

		/* Display event when */
		if(event->when)
		{
			strcpy(when,event->when);
			if(strlen(when) > 11)
				when[11] = 0;
		}
		else
			strcpy(when,"");
		websay("<td width=98><font size=\"2\" face=\"arial\">%s</font></td>",when);
		websay("<td width=5><font size=\"2\" color=\"%s\">.</font></td>",Web->bgcolor);

		/* Display event # attendees */
		websay("<td width=45 align=right><font size=\"2\" face=\"arial\">%d</font></td>",event->num_attendees);
		websay("</tr></table>");
		websay("");
	}
		

	websay("<table width=570 border=0 cellspacing=1 cellpadding=0>");
	websay("<tr>");
	websay("<td bgcolor=\"%s\" width=18><font size=\"2\" color=\"%s\">.</font></td>",Web->color1,Web->color1);
	websay("<td width=5><font face=\"arial\" size=\"2\" color=\"%s\">.</font></td>",Web->color1);
	websay("<td bgcolor=\"%s\" width=220><font size=\"2\" color=\"%s\">.</font></td>",Web->color1,Web->color1);
	websay("<td width=5><font face=\"arial\" size=\"2\" color=\"%s\">.</font></td>",Web->color1);
	websay("<td bgcolor=\"%s\" width=159><font size=\"2\" color=\"%s\">.</font></td>",Web->color1,Web->color1);
	websay("<td width=5><font face=\"arial\" size=\"2\" color=\"%s\">.</font></td>",Web->color1);
	websay("<td bgcolor=\"%s\" width=98><font size=\"2\" color=\"%s\">.</font></td>",Web->color1,Web->color1);
	websay("<td width=5><font face=\"arial\" size=\"2\" color=\"%s\">.</font></td>",Web->color1);
	websay("<td bgcolor=\"%s\" width=45><font size=\"2\" color=\"%s\">.</font></td>",Web->color1,Web->color1);
	websay("</tr>");
	websay("</table>");

	websay("<br>");
	websay("<br>");
	websay("Contact a bot master or maintainer to have your event listed here.");
	closehtml();
}

void webcmd_showevent()
{
	evntrec	*event;
	attnrec	*a;
	int	num, tot, i, user_on_list = 0, this_user_on_list = 0;
	char	nick[80], when[80], contact[80], display[BUFSIZ];
	
	if(!Web->istok1)
	{
		Log_write(LOG_MAIN,"*","DEBUG: istok=0");
		return;
	}

	num = atoi(Web->tok1);
	event = Event_event(num);
	if(event == NULL)
	{
		Log_write(LOG_MAIN,"*","No such event %d in Web module",num);
		return;
	}

	/* Display headers, statusline, and menu */
	sprintf(display,"%s - Event %s",Web->title,event->name);
	openhtml(display);
	websay_statusline();
	sprintf(display,"ShowEvent:%s",event->name);
	websay_menuline(display);
	websay("<br>");
	websay("<form method=\"post\" action=\"%s?%s;%s;!event_post\">",Web->cgi_url,Web->user->acctname,Web->user->pass);
	websay("");

	websay("<table width=570 border=0 cellspacing=0 cellpadding=0>");
	websay("<tr><td>");
	websay("<font size=\"6\" color=\"%s\"><i>Event: %s</i></font><br>",Web->color4,event->name);

	/* Display event about */
	if(event->about)
		websay("%s<br>",event->about);
	else
		websay("No ABOUT information<br>");

	/* Display place, when, admin */
	websay("<table width=570 border=0 cellspacing=1 cellpadding=0>");
	websay("<tr><td width=50><font color=\"%s\">Place: </font></td>",Web->color2);
	if(event->place)
		websay("<td width=515>%s</td></tr>",event->place);
	else
		websay("<td width=515><br></td></tr>");

	websay("<tr><td width=50><font color=\"%s\">When: </font></td>",Web->color2);
	if(event->when)
		websay("<td width=515>%s</td></tr>",event->when);
	else
		websay("<td width=515><br></td></tr>");

	websay("<tr><td width=50><font color=\"%s\">Admin: </font></td>",Web->color2);
	if(event->admin)
		websay("<td width=515>%s</td></tr>",event->admin);
	else
		websay("<td width=515><br></td></tr>");

	websay("</table>");
	websay("</font></td></tr></table>");
	websay("<br>");
	websay("");

	websay("<table width=570 border=0 cellspacing=1 cellpadding=0>");
	websay("<tr>");
	websay("<td bgcolor=\"%s\" width=78><font face=\"arial\" size=\"2\">Attendee</font></td>",Web->color1);
	websay("<td width=5><font face=\"arial\" size=\"2\" color=\"%s\">.</font></td>",Web->bgcolor);
	websay("<td bgcolor=\"%s\" width=123><font face=\"arial\" size=\"2\">When</font></td>",Web->color1);
	websay("<td width=5><font face=\"arial\" size=\"2\" color=\"%s\">.</font></td>",Web->bgcolor);
	websay("<td bgcolor=\"%s\" width=353><font face=\"arial\" size=\"2\">Contact While at Event -or- Misc Info</font></td>",Web->color1);
	websay("</tr></table>");
	websay("");

	tot = Event_orderbynick(event);
	if(tot == 0)
	{
		say("No attendees have signed up for this Event.");
		websay("<br>");
	}

	for(i=1; i<=tot; i++)
	{
		this_user_on_list = 0;
		a = Event_attendeebyorder(event,i);
		if(a == NULL)
			continue;

		websay("<table width=570 border=0 cellspacing=1 cellpadding=0><tr>");

		/* Display nick */
		if(a->nick)
			strcpy(nick,a->nick);
		else
			strcpy(nick,"");
		
		websay("<td width=78><a href=\"%s?%s;%s;Finger:%s\"><tt>%s</tt></a></td>",Web->cgi_url,Web->user->acctname,Web->user->pass,nick,nick);
		websay("<td width=5><font size=\"2\" color=\"%s\">.</font></td>",Web->bgcolor);

		/* See if nick matches who the web user is */
		if(isequal(nick,Web->user->acctname))
		{
			user_on_list = 1;
			this_user_on_list = 1;
		}
		

		/* Display when */
		if(a->when)
			strcpy(when,a->when);
		else
			strcpy(when,"");

		if(this_user_on_list)
			websay("<td width=123><font face=\"arial\" size=\"2\"><input type=\"text\" name=\"when\" value=\"%s\" size=14 maxlength=14></font></td>",when);
		else
			websay("<td width=123><tt>%s</tt></td>",when);
		websay("<td width=5><font size=\"2\" color=\"%s\">.</font></td>",Web->bgcolor);

		/* Display contact */
		if(a->contact)
			strcpy(contact,a->contact);
		else
			strcpy(contact,"");

		if(this_user_on_list)
			websay("<td width=353><font face=\"arial\" size=\"2\"><input type=\"text\" name=\"contact\" value=\"%s\" size=43 maxlength=40></font></td>",contact);
		else
			websay("<td width=353><tt>%s</tt></td>",contact);

		websay("</tr></table>");
		websay("");
	}

	websay("<table width=570 border=0 cellspacing=1 cellpadding=0>");
	websay("<tr>");
	websay("<td bgcolor=\"%s\" width=78><font size=\"2\" color=\"%s\">.</font></td>",Web->color1,Web->color1);
	websay("<td width=5><font face=\"arial\" size=\"2\" color=\"%s\">.</font></td>",Web->color1);
	websay("<td bgcolor=\"%s\" width=123><font size=\"2\" color=\"%s\">.</font></td>",Web->color1,Web->color1);
	websay("<td width=5><font face=\"arial\" size=\"2\" color=\"%s\">.</font></td>",Web->color1);
	websay("<td bgcolor=\"%s\" width=353><font size=\"2\" color=\"%s\">.</font></td>",Web->color1,Web->color1);

	websay("</tr></table>");
	websay("");
	websay("<br><br>");
	if(user_on_list)
	{
		websay("To change your event information, make the changes above and click [Submit Changes].<br>");
		websay("Click <a href=\"%s?%s;%s;UnattendEvent:%d\">here</a> to remove yourself from this event.<br>",Web->cgi_url,Web->user->acctname,Web->user->pass,event->id);
		websay("<br>");
		websay("<input type=\"hidden\" name=\"event\" value=\"%d\">",event->id);
		websay("<input type=\"submit\" value=\"Submit Changes\">");
	}
	else
	{
		websay("To attend this event, fill in the following information and click [Attend Event].<br>");
		websay("<br>");
		websay("");
		websay("<table width=570 border=0 cellspacing=1 cellpadding=0>");
		websay("<tr>");
		websay("<td width=78><font face=\"arial\" size=\"2\">Attendee</font></td>");
		websay("<td width=5><font face=\"arial\" size=\"2\" color=\"%s\">.</font></td>",Web->bgcolor);
		websay("<td width=123><font face=\"arial\" size=\"2\">When</font></td>");
		websay("<td width=5><font face=\"arial\" size=\"2\" color=\"%s\">.</font></td>",Web->bgcolor);
		websay("<td width=353><font face=\"arial\" size=\"2\">Contact While at Event -or- Misc Info</font></td>");
		websay("</tr></table>");
		websay("");
		websay("<table width=570 border=0 cellspacing=1 cellpadding=0>");
		websay("<tr>");
		websay("<td width=78><font size=\"2\" face=\"arial\">%s</font></td>",Web->user->acctname);
		websay("<td width=5><font size=\"2\" color=\"%s\">.</font></td>",Web->bgcolor);
		websay("<td width=123><font size=\"2\" face=\"arial\"><input type=\"text\" name=\"when\" value=\"\" size=14 maxlength=14></font></td>");
		websay("<td width=5><font size=\"2\" color=\"%s\">.</font></td>",Web->bgcolor);
		websay("<td width=353><font size=\"2\" face=\"arial\"><input type=\"text\" name=\"when\" value=\"\" size=43 maxlength=40></font></td>");
		websay("</tr></table>");
		websay("");
		websay("<br>");
	
		websay("<input type=\"hidden\" name=\"event\" value=\"%d\">",event->id);
		websay("<input type=\"submit\" value=\"Attend Event\">");
	}

	websay("");
	websay("<br>");
	closehtml();
}

/* Looking at event#, when, contact */
void webcmd_putevent()
{
	evntrec	*event;
	attnrec	*a;
	int	num;
	char	tmp[BUFSIZ], ident[BUFSIZ];

	context;
	if(!Web->istok1)
		return;

	if(!(Web->d->flags & STAT_DONTSHOWPROMPT))
	{
		/* Turn off prompt until we get entire form. */
		Web->d->flags |= STAT_DONTSHOWPROMPT;
	}

	context;
	/* Get ident of command */
	split(ident,Web->cmdparam);

	context;
	/* Get event number */
	split(tmp,Web->cmdparam);
	num = atoi(tmp);
	event = Event_event(num);
	if(event == NULL)
		return;

	context;
	/* Now we can start processing */
	if(isequal(ident,"<EOF>"))
	{
		/* ok we're done. */
		Web->d->flags &= ~STAT_DONTSHOWPROMPT;
		Event->flags |= EVENT_NEEDS_WRITE;

		/* Pass control to 'showevent' */
		sprintf(Web->cmdparam,"%d",event->id);
		sprintf(Web->tok1,"%d",event->id);

		webcmd_showevent();
		return;
	}

	context;
	/* If they're not an attendee, add them */
	a = Event_attendee(event,Web->user->acctname);
	if(a == NULL)
		a = Event_addnewattendee(event,Web->user->acctname);

	context;
	/* Now we can update their when and contact info */
	if(isequal(ident,"when"))
	{
	context;
		strcpy(tmp,Web->cmdparam);
		if(isequal(tmp,"none"))
		{
	context;
			xfree(a->when);
			a->when = NULL;
		}
		else
		{
	context;
			if(strlen(tmp) > 14)
				tmp[14] = 0;
			attnrec_setwhen(a,tmp);
		}
	context;
	}

	/* Set Contact info */
	else if(isequal(ident,"contact"))
	{
		strcpy(tmp,Web->cmdparam);
		if(isequal(tmp,"none"))
		{
			xfree(a->contact);
			a->contact = NULL;
		}
		else
		{
			if(strlen(tmp) > 40)
				tmp[40] = 0;
				attnrec_setcontact(a,tmp);
		}
	}
}

void webcmd_unattendevent()
{
	evntrec	*event;
	attnrec	*a;
	int	num;

	num = atoi(Web->tok1);
	event = Event_event(num);
	if(event == NULL)
		return;

	a = Event_attendee(event,Web->user->acctname);
	if(a == NULL)
		return;

	Event_deleteattendee(event,a);

	Event->flags |= EVENT_NEEDS_WRITE;

	/* Pass control to 'showevent' */
	sprintf(Web->cmdparam,"%d",event->id);
	sprintf(Web->tok1,"%d",event->id);
	Web->num_elements = 1;
	Web->istok1 = 1;
	Web->istok2 = 0;
	Web->istok3 = 0;

	webcmd_showevent();
}

	
/* ASDF */

#endif /* WEB_MODULE */
