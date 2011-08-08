/******************************************************************************
 * Steve Rider - stever@area23.org
 * Grufti - an interactive, service-oriented bot for IRC 
 *
 * cmd1.c - the command declarations
 *
 *****************************************************************************/
/* 28 April 97 (re-written 14 July 98) */

#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <netinet/in.h>
#if HAVE_GETRUSAGE
#include <sys/resource.h>
#if HAVE_SYS_RUSAGE_H
#include <sys/rusage.h>
#endif
#endif
#if HAVE_DIRENT_H
#include <dirent.h>
#else
#define dirent direct
#if HAVE_SYS_NDIR_H
#include <sys/ndir.h>
#endif
#if HAVE_SYS_DIR_H
#include <sys/dir.h>
#endif
#if HAVE_NDIR_H
#include <ndir.h>
#endif
#endif
#include "grufti.h"
#include "gruftiaux.h"
#include "server.h"
#include "misc.h"
#include "user.h"
#include "dcc.h"
#include "cmd.h"
#include "codes.h"
#include "channel.h"
#include "log.h"
#include "blowfish.h"
#include "net.h"
#include "command.h"
#include "listmgr.h"
#include "fhelp.h"
#include "response.h"
#include "location.h"
#include "notify.h"
#include "note.h"
#include "url.h"
#include "banlist.h"
#include "event.h"
#include "trigcmd.h"

extern cmdinfo_t cmdinfo;

/*
 * Commands which need DCC are commands which display output of more than
 * 3 lines.  Don't change the levels here, change them in the configfile.
 * If the DCC? and Reg? settings are changed, the bot may perform erratically.
 *
 * The 'Reg?' field should be set to TRUE only when no levels are required
 * for the command.  For commands which require levels, obviously the user
 * will need to be registered...  But we'd rather say "you don't have enough
 * levels" than "you need to register first"...
 */


/*
 * This table is now in command.c, and will soon only hold base commands.
 */


#define MAX_SEARCHRESPONSE_HITS 50
void cmd_searchresponse()
{
	rtype *rt;
	resprec *r;
	int matched = 0;

	if(!cmdinfo.isparam)
	{
		say("Usage: searchresponse <search string>");
		sayf(2,0,"Search responses by match string.  Use this command to find a response based on the string it matches, rather than response name.  Use 'showresponse' to display a response by name.  Keep in mind that more than one response matching a single string is not usually wanted!");
		return;
	}

	for(rt = Response->rtypes; rt != NULL; rt = rt->next)
	{
		for(r = rt->responses; r != NULL; r = r->next)
		{
			if(Response_matchresponse(r, cmdinfo.param))
				matched++;
		}
	}

	if(!matched)
	{
		say("No responses match the string: %s", cmdinfo.param);
		return;
	}

	if(matched > 1)
	{
		sayf(0,0,"%d responses matched the string: %s", matched, cmdinfo.param);
		sayf(0,0,"Multiple responses matching one match string is bad behavior.  Use except lines to filter out the invalid response.");
		say("");
	}

	/* Check types, we show all that match */
	for(rt = Response->rtypes; rt != NULL; rt = rt->next)
	{
		for(r = rt->responses; r != NULL; r = r->next)
		{
			if(Response_matchresponse(r, cmdinfo.param))
				cmd_showresponses_byname(rt, r);
		}
	}
}

void cmd_setbday()
{
	char	date[SERVERLEN], elem[SERVERLEN], display[150];
	char	*p;
	int	m, d, y, count = 0;

	if(!cmdinfo.isparam)
	{
		say("Usage: setbday < mm/dd/yy | mm/dd >");
		sayf(2,0,"Use this command to set your birthdate on the bot.  The bot will wish you a Happy Birthday on that day every so often if you are online and will also display it from a 'showbdays' command.  Specify 'none' to clear it.");
		return;
	}

	context;
	strcpy(date,cmdinfo.tok1);

	context;
	/* Check for 'none' */
	if(isequal(date,"none"))
	{
		cmdinfo.user->bday[0] = 0;
		say("Your birthday has been erased.");
		return;
	}


	context;
	/* Count number of '/' chars */
	for(p=date; *p; p++)
	{
		if(*p == '/')
			count++;
	}

	if(count < 1 || count > 2)
	{
		sayf(0,0,"Sorry, that date is not in the correct format.  Use mm/dd/yy or mm/dd please.");
		return;
	}

	context;
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
	context;

	if(!check_date(m,d,y))
	{
		sayf(0,0,"Sorry, that date is not in the correct format.  Use mm/dd/yy or mm/dd please.");
		return;
	}
		

	context;
	cmdinfo.user->bday[0] = m;
	cmdinfo.user->bday[1] = d;

	if(y == 0)
		y = 2000;
	else if(y < 100 && y > 0)
		y += 1900;

	/* fix so that users can't set their dates crazy */
	if(y < 0 || y > 3000)
		y = -1;

	context;
	cmdinfo.user->bday[2] = y;

	bday(display,m,d,y);
	say("Thanks!  Your birthday has been set to: %s",display);
}

void cmd_showbdays()
{
	if(!cmdinfo.isparam)
		cmd_helper_showallbdays();
	else
		cmd_helper_showamonthsbdays();
}

void cmd_helper_showallbdays()
{
	int	i, j, m[12], tot = 0;
	int	mnow, dnow, ynow, mnew, dnew, ynew, aM, aD, aY;
	u_long	now, then, check;
	char	display[BUFSIZ], mon[80];
	userrec	*u;
	generec	*g;

	/* Clear the month counter */
	for(i=0; i<12; i++)
		m[i] = 0;

	/* Ok crank through the entire list and collect # bdays */
	for(u=User->userlist; u!=NULL; u=u->next)
	{
		if(u->bday[0])
		{
			m[u->bday[0]-1]++;
			tot++;
		}
	}

	say("Number of birthdays per month: (%d total)",tot);
	for(j=0; j<3; j++)
	{
		display[0] = 0;
		for(i=0; i<4; i++)
		{
			month_str_short(mon,(4*j) + i);
			sprintf(display,"%s    %s: %3d",display,mon,m[(4*j)+i]);
		}

		say("%s", display);
	}
	say("");

	/* OK, now list all birthdays within the next 2 weeks */
	mdy_today(&mnow, &dnow, &ynow);

	/* Determine day 14 days from now */
	mnew = mnow;
	dnew = dnow;
	ynew = ynow;
	calc_new_date(&mnew, &dnew, &ynew, 14);

	/* convert to # */
	now = calc_days(mnow,dnow,ynow);
	then = calc_days(mnew,dnew,ynew);

	/* Ok run through the userlist and get all users within limits */
	for(u=User->userlist; u!=NULL; u=u->next)
	{
		if(u->bday[0])
		{
			if(mnow == 12 && u->bday[0] == 1)
				check = calc_days(u->bday[0],u->bday[1],ynow+1);
			else
				check = calc_days(u->bday[0],u->bday[1],ynow);

			if(now <= check && check <= then)
				Notify_addtogenericlist(u->acctname,check);
		}
	}

	/* Ok now display */
	tot = Notify_orderbylogin();

	strcpy(display,"Birthdays in the next 2 weeks: ");
	if(tot == 0)
		sprintf(display,"%snone",display);
	else
	{
		for(i=tot; i>=1; i--)
		{
			g = Notify_genericbyorder(i);
			if(g != NULL)
			{
				uncalc_days(&aM, &aD, &aY, g->login);
				if(i == 1)
					sprintf(display,"%s%s %d/%d",display,g->display,aM,aD);
				else
					sprintf(display,"%s%s %d/%d, ",display,g->display,aM,aD);
			}
		}
	}

	say("%s", display);
	Notify_freeallgenerics();
	say("");
	say("Type 'showbdays [month | all]' to see each month's birthday ppl.");
}

void cmd_helper_showamonthsbdays()
{
	userrec	*u;
	generec	*g;
	int	m = 0, num_cols = 3, col, row, num_rows, tot;
	int	all = 0, aM, aD, aY;
	u_long	days;
	char	line[160], ds[5], mon[25], token[25];

	/* Get month in question */
	if(isanumber(cmdinfo.tok1))
	{
		m = atoi(cmdinfo.tok1);
		if(m < 1 || m > 12)
			m = 0;
	}
	else
	{
		if(isequal(cmdinfo.tok1,"all") || isequal(cmdinfo.tok1,"*"))
			all = 1;
		else
			m = which_month(cmdinfo.tok1);
	}

	if(m == 0 && !all)
	{
		sayf(0,0,"Please specify the name of a month in abbreviated or normal format, or specify the month's number.  You can also specify '*' or 'all' to see all birthdays.");
		return;
	}

	/* Run through userlist and extract all users with that bday */
	for(u=User->userlist; u!=NULL; u=u->next)
	{
		if(u->registered && u->bday[0] && (u->bday[0] == m || all))
		{
			days = calc_days(u->bday[0], u->bday[1], 2000);
			Notify_addtogenericlist(u->acctname,days);
		}
	}

	/* Ok now display */
	line[0] = 0;

	tot = Notify_orderbylogin();

	if(all)
		strcpy(mon,"All");
	else
		month_str_long(mon,m-1);

	say("%s birthdays: (%d total)",mon,tot);
	say("");

	if(tot == 0)
	{
		if(all)
			say("There are no birthdays set.");
		else
			say("There are no %s birthdays set.",mon);
		return;
	}

	/* Display 1 column if less than 20 entries */
	if(tot <= ENTRIES_FOR_ONE_COL)
	{
		say("Nick      Birthday");
		say("--------- --------");
		num_cols = 1;
	}
	else
	{
		say("Nick      Birthday     Nick      Birthday     Nick      Birthday");
		say("--------- --------     --------- --------     --------- --------");
	}

	num_rows = tot / num_cols;
	if(tot % num_cols)
		num_rows++;

	for(row=1; row<=num_rows; row++)
	{
		for(col=1; col<=num_cols; col++)
		{
			g = Notify_genericbyorder((tot+1) - ((num_rows * (col-1)) + row));
			if(g)
			{
				uncalc_days(&aM, &aD, &aY, g->login);
				month_str_short(mon,aM-1);
				daysuffix(ds,aD);
				sprintf(token,"%-9s %3s %2d%s",g->display,mon,aD,ds);
				sprintf(line,"%s%s     ",line,token);
			}
		}
		line[strlen(line) - 5] = 0;
		say("%s",line);
		line[0] = 0;
	}

	if(tot <= ENTRIES_FOR_ONE_COL)
		say("--------- --------");
	else
		say("--------- --------     --------- --------     --------- --------");

	Notify_freeallgenerics();
}

void cmd_addevent()
{
	evntrec	*event;
	char	name[80];

	if(!cmdinfo.isparam)
	{
		say("Usage: addevent <name>");
		sayf(2,0,"Use this command to setup a new Event in the Event list.  The name of the Event can be any number of words which describe it.");
		return;
	}

	/* Check length */
	if(cmdinfo.paramlen > 25)
	{
		say("That Event name is too long by %d character%s.  Try again.",cmdinfo.paramlen - 25,(cmdinfo.paramlen - 25)==1?"":"s");
		return;
	}

	strcpy(name,cmdinfo.param);
	event = Event_addnewevent(name);

	sayf(0,0,"The Event \"%s\" has been added as Event %d.",name,event->id);
	if(Grufti->send_notice_event_created)
		Note_sendnotification("The Event \"%s\" has been added by %s as Event %d.",name,cmdinfo.user->acctname,event->id);

	say("");
	sayf(0,0,"From now on, this event will be refered to as Event %d when setting place, time, and other Event info.  Attendees will also refer to this event as Event %d when signing up for it.  See help on 'addevent' for Event setup and maintenance commands.",event->id,event->id);

	Event->flags |= EVENT_NEEDS_WRITE;
}

void cmd_delevent()
{
	evntrec	*event;
	int	num;
	char	name[80];

	if(!cmdinfo.isparam)
	{
		say("Usage: delevent <#>");
		sayf(2,0,"Use this command to delete an entire Event.  The list of attendees who have signed up will be lost.");
		return;
	}

	num = atoi(cmdinfo.tok1);
	event = Event_event(num);
	if(event == NULL)
	{
		say("There is no such Event number \"%d\".",num);
		return;
	}

	if(event->name)
		strcpy(name,event->name);
	else
		sprintf(name,"known as Event %d",event->id);

	say("The Event \"%s\" has been deleted.",name);
	if(Grufti->send_notice_event_deleted)
		Note_sendnotification("The Event \"%s\" has been deleted by %s.",name,cmdinfo.user->acctname);

	/* Delete event */
	Event_deleteevent(event);

	Event->flags |= EVENT_NEEDS_WRITE;
}

void cmd_showevents()
{
	evntrec	*event;
	char	name[80], place[80], when[80];

	if(Event->num_events == 0)
	{
		say("There are no Events in the Event list.  Use 'addevent'.");
		return;
	}

	say("#  Event                     Place              When        # Att");
	say("-- ------------------------- ------------------ ----------- -----");
	for(event=Event->events; event!=NULL; event=event->next)
	{
		if(event->name)
		{
			strcpy(name,event->name);
			if(strlen(name) > 25)
				name[25] = 0;
		}
		else
			strcpy(name,"");

		if(event->place)
		{
			strcpy(place,event->place);
			if(strlen(place) > 18)
				place[18] = 0;
		}
		else
			strcpy(place,"");

		if(event->when)
		{
			strcpy(when,event->when);
			if(strlen(when) > 11)
				when[11] = 0;
		}
		else
			strcpy(when,"");
			
		say("%2d %-25s %-18s %-11s %5d",event->id,name,place,when,event->num_attendees);
	}
	say("-- ------------------------- ------------------ ----------- -----");
	say("Contact a bot master or maintainer to have an event listed here.");
	say("Use 'showevent <#>' to see Event info and a listing of attendees.");

}

void cmd_showevent()
{
	evntrec	*event;
	attnrec	*a;
	int	num, tot, i;
	char	nick[80], when[80], contact[80];

	if(!cmdinfo.isparam)
	{
		say("Usage: showevent <#>");
		sayf(2,0,"Use this command to display details on the specified Event.  Use 'showevents' to display a list of all Events and Event numbers.");
		return;
	}

	if(!isanumber(cmdinfo.tok1))
	{
		say("Usage: showevent <#>");
		sayf(2,0,"Use this command to display details on the specified Event.  Use 'showevents' to display a list of all Events and Event numbers.");
		return;
	}

	num = atoi(cmdinfo.tok1);
	event = Event_event(num);
	if(event == NULL)
	{
		say("There is no such Event number \"%d\".",num);
		return;
	}

	/* Display event name */
	if(event->name)
		say("Event: [%d] %s",event->id,event->name);
	else
		say("Event: [%d] ",event->id);

	/* Display event about */
	if(event->about)
		sayf(0,0,"About: %s",event->about);
	else
		say("About: ");

	/* Display event place */
	if(event->place)
		say("Place: %s",event->place);
	else
		say("Place: ");

	/* Display event dates */
	if(event->when)
		say(" When: %s",event->when);
	else
		say(" When: ");

	/* Display event admin */
	if(event->admin)
		sayf(0,0,"Admin: %s",event->admin);
	else
		say("Admin: ");

	say("");

	/* Here we go, put in alphabetical order */
	tot = Event_orderbynick(event);

	if(tot == 0)
	{
		say("No attendees have signed up for this Event.");
		say("Type 'attendevent %d' to show that you are going.",event->id);
		return;
	}

	say("Attendee  When           Contact While at Event -or- Misc Info");
	say("--------- -------------- ----------------------------------------");
	for(i=1; i<=tot; i++)
	{
		a = Event_attendeebyorder(event,i);
		if(a == NULL)
			continue;

		if(a->nick)
			strcpy(nick,a->nick);
		else
			strcpy(nick,"");

		if(a->when)
			strcpy(when,a->when);
		else
			strcpy(when,"");

		if(a->contact)
			strcpy(contact,a->contact);
		else
			strcpy(contact,"");

		say("%-9s %-14s %-40s",nick,when,contact);
	}
	say("--------- -------------- ----------------------------------------");
	if(Event_attendee(event,cmdinfo.user->acctname) == NULL)
		say("Type 'attendevent %d' to sign up for this Event.",event->id);
}

void cmd_seteventname()
{
	evntrec	*event;
	int	num;
	char	name[SERVERLEN];

	if(!cmdinfo.isparam)
	{
		say("Usage: seteventname <#> <name>");
		sayf(2,0,"Use this command to set the name of an Event.  Use 'showevents' to see Event numbers.  Specify 'none' to clear it.");
		return;
	}

	if(!isanumber(cmdinfo.tok1))
	{
		say("Usage: seteventname <#> <name>");
		sayf(2,0,"Use this command to set the name of an Event.  Type 'showevents' to see Event numbers.  Specify 'none' to clear it.");
		return;
	}

	num = atoi(cmdinfo.tok1);
	event = Event_event(num);
	if(event == NULL)
	{
		say("There is no such Event number \"%d\".",num);
		return;
	}

	if(!cmdinfo.istok2)
	{
		say("Specify a name for this Event, or specify 'none' to clear it.");
		if(event->name)
			sayf(0,0,"Current name: %s",event->name);
		else
			sayf(0,0,"Current name: none");
		return;
	}

	/* Extract name */
	strcpy(name,cmdinfo.param);
	split(NULL,name);

	/* Check for 'none' */
	if(isequal(name,"none"))
	{
		xfree(event->name);
		event->name = NULL;
		say("Event %d name erased.  (An Event *should* have a name!)",event->id);
		return;
	}

	/* Check for length */
	if(strlen(name) > 25)
	{
		say("That Event name is too long by %d character%s.  Try again.",strlen(name) - 25,(strlen(name) - 25)==1?"":"s");
		return;
	}

	evntrec_setname(event,name);

	sayf(0,0,"Event %d name changed to \"%s\".",event->id,event->name);
	Event->flags |= EVENT_NEEDS_WRITE;
}

void cmd_seteventplace()
{
	evntrec	*event;
	int	num;
	char	place[SERVERLEN];

	if(!cmdinfo.isparam)
	{
		say("Usage: seteventplace <#> <place>");
		sayf(2,0,"Use this command to set the place of an Event.  Type 'showevents' to see Event numbers.  Specify 'none' to clear it.");
		return;
	}

	if(!isanumber(cmdinfo.tok1))
	{
		say("Usage: seteventplace <#> <place>");
		sayf(2,0,"Use this command to set the place of an Event.  Type 'showevents' to see Event numbers.  Specify 'none' to clear it.");
		return;
	}

	num = atoi(cmdinfo.tok1);
	event = Event_event(num);
	if(event == NULL)
	{
		say("There is no such Event number \"%d\".",num);
		return;
	}

	if(!cmdinfo.istok2)
	{
		say("Specify a place for this Event, or specify 'none' to clear it.");
		if(event->place)
			sayf(0,0,"Current place: %s",event->place);
		else
			sayf(0,0,"Current place: none");
		return;
	}

	/* Extract place */
	strcpy(place,cmdinfo.param);
	split(NULL,place);

	/* Check for 'none' */
	if(isequal(place,"none"))
	{
		xfree(event->place);
		event->place = NULL;
		say("Event %d place erased.  (An Event *should* have a place!)",event->id);
		return;
	}

	/* Check for length */
	if(strlen(place) > 18)
	{
		say("That Event place is too long by %d character%s.  Try again.",strlen(place) - 18,(strlen(place) - 18)==1?"":"s");
		return;
	}

	evntrec_setplace(event,place);

	sayf(0,0,"Event %d place changed to \"%s\".",event->id,event->place);
	Event->flags |= EVENT_NEEDS_WRITE;
}

void cmd_seteventdates()
{
	evntrec	*event;
	int	num;
	char	dates[SERVERLEN];

	if(!cmdinfo.isparam)
	{
		say("Usage: seteventdates <#> <dates>");
		sayf(2,0,"Use this command to set the dates of an Event.  Type 'showevents' to see Event numbers.  Specify 'none' to clear it.");
		return;
	}

	if(!isanumber(cmdinfo.tok1))
	{
		say("Usage: seteventdates <#> <dates>");
		sayf(2,0,"Use this command to set the dates of an Event.  Type 'showevents' to see Event numbers.  Specify 'none' to clear it.");
		return;
	}

	num = atoi(cmdinfo.tok1);
	event = Event_event(num);
	if(event == NULL)
	{
		say("There is no such Event number \"%d\".",num);
		return;
	}

	if(!cmdinfo.istok2)
	{
		say("Specify dates for this Event, or specify 'none' to clear it.");
		if(event->when)
			sayf(0,0,"Current dates: %s",event->when);
		else
			sayf(0,0,"Current dates: none");
		return;
	}

	/* Extract dates */
	strcpy(dates,cmdinfo.param);
	split(NULL,dates);

	/* Check for 'none' */
	if(isequal(dates,"none"))
	{
		xfree(event->when);
		event->when = NULL;
		say("Event %d dates erased.  (An Event *should* have dates!)",event->id);
		return;
	}

	/* Check for length */
	if(strlen(dates) > 11)
	{
		say("That Event date is too long by %d character%s.  Try again.",strlen(dates) - 11,(strlen(dates) - 11)==1?"":"s");
		return;
	}

	evntrec_setwhen(event,dates);

	sayf(0,0,"Event %d dates changed to \"%s\".  Thanks!",event->id,event->when);
	Event->flags |= EVENT_NEEDS_WRITE;
}

void cmd_seteventabout()
{
	evntrec	*event;
	int	num;
	char	about[SERVERLEN];

	if(!cmdinfo.isparam)
	{
		say("Usage: seteventabout <#> <about>");
		sayf(2,0,"Use this command to set the about info of an Event.  Type 'showevents' to see Event numbers.  Specify 'none' to clear it.");
		return;
	}

	if(!isanumber(cmdinfo.tok1))
	{
		say("Usage: seteventabout <#> <about>");
		sayf(2,0,"Use this command to set the about info of an Event.  Type 'showevents' to see Event numbers.  Specify 'none' to clear it.");
		return;
	}

	num = atoi(cmdinfo.tok1);
	event = Event_event(num);
	if(event == NULL)
	{
		say("There is no such Event number \"%d\".",num);
		return;
	}

	if(!cmdinfo.istok2)
	{
		say("Specify about info for this Event, or specify 'none' to clear it.");
		if(event->about)
			sayf(0,0,"Current about info: %s",event->about);
		else
			sayf(0,0,"Current about info: none");
		return;
	}

	/* Extract about */
	strcpy(about,cmdinfo.param);
	split(NULL,about);

	/* Check for 'none' */
	if(isequal(about,"none"))
	{
		xfree(event->about);
		event->about = NULL;
		say("Event %d about info erased.",event->id);
		return;
	}

	/* Check for length */
	if(strlen(about) > 255)
	{
		say("That Event about info is too long by %d character%s.  Try again.",strlen(about) - 255,(strlen(about) - 255)==1?"":"s");
		return;
	}

	evntrec_setabout(event,about);

	sayf(0,0,"Event %d about info changed to \"%s\".",event->id,event->about);
	Event->flags |= EVENT_NEEDS_WRITE;
}

void cmd_seteventadmin()
{
	evntrec	*event;
	int	num;
	char	admin[SERVERLEN];

	if(!cmdinfo.isparam)
	{
		say("Usage: seteventadmin <#> <admin>");
		sayf(2,0,"Use this command to set the admin of an Event.  Type 'showevents' to see Event numbers.  Specify 'none' to clear it.");
		return;
	}

	if(!isanumber(cmdinfo.tok1))
	{
		say("Usage: seteventadmin <#> <admin>");
		sayf(2,0,"Use this command to set the admin of an Event.  Type 'showevents' to see Event numbers.  Specify 'none' to clear it.");
		return;
	}

	num = atoi(cmdinfo.tok1);
	event = Event_event(num);
	if(event == NULL)
	{
		say("There is no such Event number \"%d\".",num);
		return;
	}

	if(!cmdinfo.istok2)
	{
		say("Specify an admin for this Event, or specify 'none' to clear it.");
		if(event->admin)
			sayf(0,0,"Current admin: %s",event->admin);
		else
			sayf(0,0,"Current admin: none");
		return;
	}

	/* Extract admin */
	strcpy(admin,cmdinfo.param);
	split(NULL,admin);

	/* Check for 'none' */
	if(isequal(admin,"none"))
	{
		xfree(event->admin);
		event->admin = NULL;
		say("Event %d admin erased.",event->id);
		return;
	}

	/* Check for length */
	if(strlen(admin) > 255)
	{
		say("That Event admin is too long by %d character%s.  Try again.",strlen(admin) - 255,(strlen(admin) - 255)==1?"":"s");
		return;
	}

	evntrec_setadmin(event,admin);

	sayf(0,0,"Event %d admin changed to \"%s\".",event->id,event->admin);
	Event->flags |= EVENT_NEEDS_WRITE;
}

void cmd_attendevent()
{
	evntrec	*event;
	attnrec	*a;
	int	num;
	char	name[SERVERLEN];

	if(!cmdinfo.isparam)
	{
		say("Usage: attendevent <#>");
		sayf(2,0,"Use this command to sign yourself up for an Event.  Use 'showevents' to see all Events and Event numbers.");
		return;
	}

	num = atoi(cmdinfo.tok1);
	event = Event_event(num);
	if(event == NULL)
	{
		say("There is no such Event number \"%d\".",num);
		return;
	}

	/* See if they're already an attendee! */
	a = Event_attendee(event,cmdinfo.user->acctname);
	if(a != NULL)
	{
		say("You're already signed up for Event %d!",event->id);
		return;
	}

	a = Event_addnewattendee(event,cmdinfo.user->acctname);

	if(event->name)
		strcpy(name,event->name);
	else
		sprintf(name,"known as Event %d",event->id);

	sayf(0,0,"You have been added to the Event \"%s\"!",name);
	say("");
	sayf(0,0,"You can use the commands 'seteventwhen' and 'seteventcontact' to provide information about when you will be at the Event, and where you can be reached while there.  To remove yourself from an Event, use the 'unattendevent' command.");
	say("");
	sayf(0,0,"When you sign up for an Event, you are automatically subscribed to the Event's Note list for that Event (like a mailing list).  On it, you can send messages to other attendees using the 'eventnote' command.");

	Event->flags |= EVENT_NEEDS_WRITE;
}

void cmd_unattendevent()
{
	evntrec	*event;
	attnrec	*a;
	int	num;

	if(!cmdinfo.isparam)
	{
		say("Usage: unattendevent <#>");
		sayf(2,0,"Use this command to remove yourself from an Event.");
		return;
	}

	num = atoi(cmdinfo.tok1);
	event = Event_event(num);
	if(event == NULL)
	{
		say("There is no such Event number \"%d\".",num);
		return;
	}

	/* See if they're not already an attendee! */
	a = Event_attendee(event,cmdinfo.user->acctname);
	if(a == NULL)
	{
		say("You're not even signed up for Event %d!",event->id);
		return;
	}

	sayf(0,0,"You have been removed from Event %d.",event->id);
	Event_deleteattendee(event,a);

	Event->flags |= EVENT_NEEDS_WRITE;
}

void cmd_seteventwhen()
{
	evntrec	*event;
	attnrec	*a;
	int	num;
	char	when[SERVERLEN];

	if(!cmdinfo.isparam)
	{
		say("Usage: seteventwhen <#> <when>");
		sayf(2,0,"Use this command to set the dates for which you'll be at an Event.  Type 'showevents' to see Event numbers.  Specify 'none' to clear it.  (Note: If you're trying to change the dates for the Event itself, use the 'seteventdates' command...)");
		return;
	}

	if(!isanumber(cmdinfo.tok1))
	{
		say("Usage: seteventwhen <#> <when>");
		sayf(2,0,"Use this command to set the dates for which you'll be at an Event.  Type 'showevents' to see Event numbers.  Specify 'none' to clear it.  (Note: If you're trying to change the dates for the Event itself, use the 'seteventdates' command...)");
		return;
	}

	num = atoi(cmdinfo.tok1);
	event = Event_event(num);
	if(event == NULL)
	{
		say("There is no such Event number \"%d\".",num);
		return;
	}

	/* See if they're an attendee for this event. */
	a = Event_attendee(event,cmdinfo.user->acctname);
	if(a == NULL)
	{
		sayf(0,0,"You're not signed up for Event %d!  (Note: If you're trying to change the dates for the Event itself, use the 'seteventdates' command...)",event->id);
		return;
	}

	if(!cmdinfo.istok2)
	{
		say("You must specify the dates for which you plan to be at the Event, or specify 'none' to clear it.");
		if(a->when)
			sayf(0,0,"Current dates: %s",a->when);
		else
			sayf(0,0,"Current dates: none");
		return;
	}

	/* Extract dates */
	strcpy(when,cmdinfo.param);
	split(NULL,when);

	/* Check for 'none' */
	if(isequal(when,"none"))
	{
		xfree(a->when);
		a->when = NULL;
		say("Your dates for Event %d have been erased.",event->id);
		return;
	}

	/* Check for length */
	if(strlen(when) > 14)
	{
		say("That Event date is too long by %d character%s.  Try again.",strlen(when) - 14,(strlen(when) - 14)==1?"":"s");
		return;
	}

	attnrec_setwhen(a,when);

	sayf(0,0,"Your Event %d dates have been changed to \"%s\".",event->id,a->when);
	Event->flags |= EVENT_NEEDS_WRITE;
}

void cmd_seteventcontact()
{
	evntrec	*event;
	attnrec	*a;
	int	num;
	char	contact[SERVERLEN];

	if(!cmdinfo.isparam)
	{
		say("Usage: seteventcontact <#> <contact>");
		sayf(2,0,"Use this command to set how you can be contacted while at an Event or for other misc info.  Type 'showevents' to see Event numbers.  Specify 'none' to clear it.");
		return;
	}

	if(!isanumber(cmdinfo.tok1))
	{
		say("Usage: seteventcontact <#> <contact>");
		sayf(2,0,"Use this command to set how you can be contacted while at an Event or for other misc info.  Type 'showevents' to see Event numbers.  Specify 'none' to clear it.");
		return;
	}

	num = atoi(cmdinfo.tok1);
	event = Event_event(num);
	if(event == NULL)
	{
		say("There is no such Event number \"%d\".",num);
		return;
	}

	/* See if they're an attendee for this event. */
	a = Event_attendee(event,cmdinfo.user->acctname);
	if(a == NULL)
	{
		sayf(0,0,"You're not signed up for Event %d!",event->id);
		return;
	}

	if(!cmdinfo.istok2)
	{
		say("You must specify some contact info for when you plan to be at the Event, or specify 'none' to clear it.");
		if(a->contact)
			sayf(0,0,"Current contact: %s",a->contact);
		else
			sayf(0,0,"Current contact: none");
		return;
	}

	/* Extract contact */
	strcpy(contact,cmdinfo.param);
	split(NULL,contact);

	/* Check for 'none' */
	if(isequal(contact,"none"))
	{
		xfree(a->contact);
		a->contact = NULL;
		say("Your contact info for Event %d has been erased.",event->id);
		return;
	}

	/* Check for length */
	if(strlen(contact) > 40)
	{
		say("Your Event contact info is too long by %d character%s.  Try again.",strlen(contact) - 40,(strlen(contact) - 40)==1?"":"s");
		return;
	}

	attnrec_setcontact(a,contact);

	sayf(0,0,"Your Event %d contact info has been changed to \"%s\".",event->id,a->contact);
	Event->flags |= EVENT_NEEDS_WRITE;
}

void cmd_eventnote()
{
	if(!cmdinfo.isparam)
	{
		say("Usage: eventnote <#> <message...>");
		sayf(2,0,"Use this command to send a message to all other Event attendees for this Event.  You must be signed up for the Event in question in order to use this command.");
	}
	
	say("");
	say("Sorry, I'm not done with this command yet! :>");

	return;
}

void cmd_meminfo()
{
	char	display[200], n1[50], n2[50];
	time_t	counter = 1000;
	u_long	tot_notes = 0L, tot_accts = 0L, tot_nicks = 0L;
	u_long	tot_greets = 0L, tot = 0L;
	u_long	num_notes = 0L, num_accts = 0L, num_greets = 0L, num_nicks = 0L;
	userrec	*u;
	gretrec	*g;
	acctrec	*acct;
	generec	*generic;
	nickrec	*n;
	noterec	*note;
	chanrec	*chan;
	rtype	*rt;
	int	num_cols = 2;
	int	col, row, num_rows;
	char	line[180];
	

	context;
	/* Again we load up the 'genericlist' so we can sort the lists.
	 * It works well since we may have many channels or many response
	 * types.  By loading up the 'genericlist' and displaying them back
	 * out, we can balance the size of the columns
	 */
	/* Channel Object */
	sprintf(display,"%-15s %4s %10s","Channel:","","");
	Notify_addtogenericlist(display,counter);
	counter--;

	context;
	/* Each channel */
	for(chan=Channel->channels; chan!=NULL; chan=chan->next)
	{
		sprintf(n1, "%d", chan->num_members);
		comma_number(n2,chanrec_sizeof(chan));
		sprintf(display,"  - %-11s %4s %10s",chan->name,n1,n2);
		Notify_addtogenericlist(display,counter);
		counter--;
	}

	context;
	/* Codes */
	sprintf(n1, "%d", Codes->num_codes);
	comma_number(n2,Codes_sizeof());
	sprintf(display,"%-15s %4s %10s","Codes (server):",n1,n2);
	Notify_addtogenericlist(display,counter);
	counter--;

	/* Command */
#if 0
	comma_number(n1,listmgr_length(commands));
	comma_number(n2,command_sizeof());
	sprintf(display,"%-15s %4s %10s","Command:",n1,n2);
#endif
	sprintf(display,"%-15s %4s %10s","Command:","0","0");

	Notify_addtogenericlist(display,counter);
	counter--;

	/* DCC */
	sprintf(n1, "%d", DCC->num_dcc);
	comma_number(n2,DCC_sizeof());
	sprintf(display,"%-15s %4s %10s","DCC:",n1,n2);
	Notify_addtogenericlist(display,counter);
	counter--;

	/* Grufti */
	comma_number(n2,Grufti_sizeof());
	sprintf(display,"%-15s %4s %10s","Grufti (confs):","",n2);
	Notify_addtogenericlist(display,counter);
	counter--;

	/* Location */
	sprintf(n1, "%d", Location->num_locations);
	comma_number(n2,Location_sizeof());
	sprintf(display,"%-15s %4s %10s","Location:",n1,n2);
	Notify_addtogenericlist(display,counter);
	counter--;

	/* Logs */
	sprintf(n1, "%d", Log->num_logs);
	comma_number(n2,Log_sizeof());
	sprintf(display,"%-15s %4s %10s","Log:",n1,n2);
	Notify_addtogenericlist(display,counter);
	counter--;

	context;
	/* Notify */
	sprintf(n1, "%d", Notify->num_nicks + Notify->num_generics);
	comma_number(n2,Notify_sizeof());
	sprintf(display,"%-15s %4s %10s","Notify:",n1,n2);
	Notify_addtogenericlist(display,counter);
	counter--;

	/* Response */
	sprintf(display,"%-15s %4s %10s","Response:","","");
	Notify_addtogenericlist(display,counter);
	counter--;

	context;
	/* Each Response type */
	for(rt=Response->rtypes; rt!=NULL; rt=rt->next)
	{
		sprintf(n1, "%d", rt->num_responses);
		comma_number(n2,rtype_sizeof(rt));
		sprintf(display,"  - %-11s %4s %10s",rt->name,n1,n2);
		Notify_addtogenericlist(display,counter);
		counter--;
	}

	/* Server */
	sprintf(n1, "%d", Server->num_servers);
	comma_number(n2,Server_sizeof());
	sprintf(display,"%-15s %4s %10s","Server:",n1,n2);
	Notify_addtogenericlist(display,counter);
	counter--;

	/* URL */
	sprintf(n1, "%d", URL->num_urls);
	comma_number(n2,URL_sizeof());
	sprintf(display,"%-15s %4s %10s","URL (webcatch):",n1,n2);
	Notify_addtogenericlist(display,counter);
	counter--;

	/* User */
	sprintf(n1, "%d", User->num_users);
	comma_number(n2,User_sizeof());
	sprintf(display,"%-15s %4s %10s","User:",n1,n2);
	Notify_addtogenericlist(display,counter);
	counter--;

	context;
	/* Notes */
	for(u=User->userlist; u!=NULL; u=u->next)
	{
		if(!u->registered)
			continue;
		num_notes += u->num_notes;
		for(note=u->notes; note!=NULL; note=note->next)
			tot_notes += noterec_sizeof(note);
		num_accts += u->num_accounts;
		for(acct=u->accounts; acct!=NULL; acct=acct->next)
			tot_accts += acctrec_sizeof(acct);
		num_greets += u->num_greets;
		for(g=u->greets; g!=NULL; g=g->next)
			tot_greets += gretrec_sizeof(g);
		num_nicks += u->num_nicks;
		for(n=u->nicks; n!=NULL; n=n->next)
			tot_nicks += nickrec_sizeof(n);
	}

	context;
	/* Notes */
	sprintf(n1, "%lu", num_notes);
	comma_number(n2,tot_notes);
	sprintf(display,"  - %-11s %4s %10s","Notes",n1,n2);
	Notify_addtogenericlist(display,counter);
	counter--;

	context;
	/* Accounts */
	sprintf(n1, "%lu", num_accts);
	comma_number(n2,tot_accts);
	sprintf(display,"  - %-11s %4s %10s","Hosts",n1,n2);
	Notify_addtogenericlist(display,counter);
	counter--;

	/* Greets */
	sprintf(n1, "%lu", num_greets);
	comma_number(n2,tot_greets);
	sprintf(display,"  - %-11s %4s %10s","Greets",n1,n2);
	Notify_addtogenericlist(display,counter);
	counter--;

	/* Nicks */
	context;
	sprintf(n1, "%lu", num_nicks);
	comma_number(n2,tot_nicks);
	sprintf(display,"  - %-11s %4s %10s","Nicks",n1,n2);
	Notify_addtogenericlist(display,counter);
	counter--;

	/* unreg'd hosts */
	sprintf(n1,"%d",User->luser->num_accounts);
	tot_accts = 0L;
	for(acct=User->luser->accounts; acct!=NULL; acct=acct->next)
		tot_accts += acctrec_sizeof(acct);
	comma_number(n2,tot_accts);
	sprintf(display,"  - %-11s %4s %10s","Unreg Hosts",n1,n2);
	Notify_addtogenericlist(display,counter);
	counter--;

	context;
	/* Misc */
	sprintf(display,"%-15s %4s %10s","Misc","","");
	Notify_addtogenericlist(display,counter);
	counter--;

	/* net */
	comma_number(n2,net_sizeof());
	sprintf(display,"  - %-11s %4s %10s","net","",n2);
	Notify_addtogenericlist(display,counter);
	counter--;

	/* misc */
	comma_number(n2,misc_sizeof());
	sprintf(display,"  - %-11s %4s %10s","misc","",n2);
	Notify_addtogenericlist(display,counter);
	counter--;

	/* gruftiaux */
	context;
	comma_number(n2,gruftiaux_sizeof());
	sprintf(display,"  - %-11s %4s %10s","gruftiaux","",n2);
	Notify_addtogenericlist(display,counter);
	counter--;

	/* blowfish */
	comma_number(n2,blowfish_sizeof());
	sprintf(display,"  - %-11s %4s %10s","blowfish","",n2);
	Notify_addtogenericlist(display,counter);
	counter--;

	/* main */
	comma_number(n2,main_sizeof());
	sprintf(display,"  - %-11s %4s %10s","main","",n2);
	Notify_addtogenericlist(display,counter);
	counter--;

	/* fhelp */
	comma_number(n2,fhelp_sizeof());
	sprintf(display,"  - %-11s %4s %10s","fhelp","",n2);
	Notify_addtogenericlist(display,counter);
	counter--;

	context;
	/* ok now display */
	line[0] = 0;
	tot = Notify_orderbylogin();

	context;
	num_rows = tot / num_cols;
	if(tot % num_cols)
		num_rows++;

	say("Object          Recs   Size (b)   Object          Recs   Size (b)");
	say("--------------- ---- ----------   --------------- ---- ----------");
	context;
	for(row=1; row<=num_rows; row++)
	{
		for(col=1; col<=num_cols; col++)
		{
			generic = Notify_genericbyorder((num_rows * (col-1)) + row);
			if(generic)
			{
				sprintf(line,"%s%s   ",line,generic->display);
			}
		}
		line[strlen(line) - 3] = 0;
		say("%s",line);
		line[0] = 0;
	}
	context;
	say("-------------- ----- ----------   -------------- ----- ----------");
	comma_number(n1,net_sizeof() + misc_sizeof() + gruftiaux_sizeof() + blowfish_sizeof() + main_sizeof() + fhelp_sizeof() + User_sizeof() + URL_sizeof() + Server_sizeof() + Response_sizeof() + Notify_sizeof() + Log_sizeof() + Location_sizeof() + Grufti_sizeof() + DCC_sizeof() + command_sizeof() + Codes_sizeof() + Channel_sizeof());
	say("  Total: %s bytes",n1);

	Notify_freeallgenerics();
}

void cmd_die()
/*
 * a) We DON'T signoffallmembers().  Server_gotEOF() takes care of that
 *    when the server disconnects.
 * b) We DON'T disconnect the server.  We tell it to QUIT and let it close
 *    gracefully.  If it doesn't quit in a specified amount of time, it will 
 *    timeout and disconnect automatically.  Server_gotEOF() still takes care
 *    of the details...
 * c) We definitely DON'T exit() or fatal() from here.
 * d) Finally, we don't disconnect the caller's DCC connection.  We flag it
 *    and let the calling function take care of it.
 */
{
	dccrec *d, *d_next;

	context;
	if(cmdinfo.isparam)
	{
		sprintf(Server->sbuf,"DIE request by %s \"%s\"",cmdinfo.from_n,cmdinfo.param);
		if(Grufti->send_notice_die)
			Note_sendnotification("DIE request by %s \"%s\"",cmdinfo.from_n,cmdinfo.param);
	}
	else
		sprintf(Server->sbuf,"DIE request by %s",cmdinfo.from_n);

	Log_write(LOG_ALL,"*",Server->sbuf);

	Grufti->die = 1;

	/* If we're not even active, don't bother shutting down the server */
	if(Server_isactiveIRC())
	{
		Log_write(LOG_MAIN,"*","Sent QUIT, waiting for signoff...");
		Server_quit(Server->sbuf);
	}
	else if(Server_isconnected())
	{
		Server_disconnect();
		Log_write(LOG_MAIN,"*","Signing off...");
	}
	else
		Log_write(LOG_MAIN,"*","Signing off...");

	say("Bye!");

	if(cmdinfo.d)
	{
		/* This is how we close the connection */
		cmdinfo.d->flags |= STAT_DISCONNECT_ME;
	}

	/* Boot all users on DCC *except* for this one */
	d = DCC->dcclist;
	while(d != NULL)
	{
		d_next = d->next;
		if(d != cmdinfo.d)
			DCC_bootuser(d,Server->sbuf);

		d = d_next;
	}

	/* Wait for i/o to close server and cleanup if necessary. */
}

void cmd_boot()
{
	dccrec	*d;

	if(!cmdinfo.isparam)
	{
		say("Usage: boot <socket #>");
		sayf(2,0,"Use this command to boot the client from dcc.");
		return;
	}

	if(!isanumber(cmdinfo.tok1))
	{
		say("Usage: boot <socket #>");
		sayf(2,0,"You must specify a socket number.");
		return;
	}

	d = DCC_dccbysock(atoi(cmdinfo.tok1));
	if(d == NULL)
	{
		say("No dcc record exists with socket \"%d\".",atoi(cmdinfo.tok1));
		return;
	}

	DCC_bootuser(d,"You have been booted from the bot!");
	say("%s has been booted from the bot.",d->nick);
	return;
}

void cmd_raw()
{
	context;
	Server_write(PRIORITY_NORM,"%s",cmdinfo.param);
}

void cmd_chat()
{
	context;

	if(DCC->num_dcc >= Grufti->max_dcc_connections)
	{
		/* send to user "too full" */
		Log_write(LOG_MAIN | LOG_ERROR,"*","DCC connection limit reached. (%d)",DCC->num_dcc);
		return;
	}

	Log_write(LOG_DEBUG,"*","Sending chat request.");
	DCC_sendchat(cmdinfo.from_n,cmdinfo.from_uh);
	Log_write(LOG_DEBUG,"*","Finished sending chat request.");
}

void cmd_register()
{
	userrec *ucheck;
	char	cipher[PASSWORD_CIPHER_LEN], msg[80];

	context;
	/* No arguments */
	if(!cmdinfo.isparam)
	{
		say("Usage: register <username> <password>");
		sayf(2,0,"Use this command to create your new account.  You should probably choose your username to be your most frequently used IRC nick.  Remember your password!  You'll need it to later gain access to your account.");
		return;
	}

	context;
	/* First check if the user is already registered! */
	if(cmdinfo.account->registered)
	{
		sayf(0,0,"You are already registered under the username \"%s\".",cmdinfo.user->acctname);
		say("");
		sayf(0,0,"If you want to add a nick to your account, use the 'addnick' command.  If you want to create a new account, please delete this one first by typing 'unregister %s'.  Thanks :>",cmdinfo.user->acctname);
		say("");
		sayf(0,0,"If you want to add your current nick to your account, type 'addthisnick <acctname> <password>'");
		return;
	}
		
	context;
	/* Verify proper number of parameters */
	if(cmdinfo.num_elements != 2)
	{
		say("Usage: register <accountname> <password>");
		sayf(2,0,"Invalid number of parameters specified.");
		return;
	}

	context;
	/* Verify account name */
	if(invalidacctname(cmdinfo.tok1))
	{
		say("Usage: register <username> <password>");
		sayf(2,0,"The name \"%s\" is invalid.  Usernames are between 2 and 9 characters in length and must be valid IRC nicks.",cmdinfo.tok1);
		return;
	}

	context;
	/* Account name is valid.  Check against database. */
	ucheck = User_user(cmdinfo.tok1);
	if(ucheck->registered)
	{
		sayf(0,0,"Sorry, the name \"%s\" is already being used by %s.",cmdinfo.tok1,ucheck->acctname);
		return;
	}

	context;
	/* Verify password */
	if(invalidpassword(cmdinfo.tok2))
	{
		sayf(0,0,"\"%s\" is not a valid password.  Passwords are between 2 and 9 characters in length.",cmdinfo.tok2);
		return;
	}

	context;
	/*
	 * Account name is valid and password is good.  Create new record.
	 * After creation, we must reset all user and account pointers to
	 * reflect the new changes.  Also, if the user has set up an account
	 * name different to the nick he or she is currnently using, we MUST
	 * add that nick to their account, since the account record for the
	 * new account contains their current nick.  They can always delete	
	 * it later, which moves the accounts back to luser.
	 */
	context;
	cmdinfo.user = User_addnewuser(cmdinfo.tok1);
	cmdinfo.account->registered = 1;

	context;
	/* Add password to account */
	encrypt_pass(cmdinfo.tok2,cipher);
	userrec_setpass(cmdinfo.user,cipher);

	context;
	/* Move all accounts with that nick from Luser into the new record */
	User_moveaccounts_into_newhome(cmdinfo.user,cmdinfo.tok1);

	context;
	/* account name differs from nick.  Add nick to account... */
	if(!isequal(cmdinfo.tok1,cmdinfo.from_n))
	{
		User_addnewnick(cmdinfo.user,cmdinfo.from_n);
		User_moveaccounts_into_newhome(cmdinfo.user,cmdinfo.from_n);
		User_checkforsignon(cmdinfo.from_n);
	}

	context;
	/* Reset all user, account pointers in Channel, DCC, and Command. */
	User_resetallpointers();

	context;
	/* Possible signon */
	User_gotpossiblesignon(cmdinfo.user,cmdinfo.account);

	Log_write(LOG_MAIN,"*","New account \"%s\" created by %s (%s).",cmdinfo.tok1,cmdinfo.from_n,cmdinfo.from_uh);

	context;
	sayf(0,0,"Congratulations!");
	say("");
	sayf(0,0,"You have created a new account called \"%s\".  To complete registration, you should personalize your account.  Type 'finger %s' to view your account so far.  Type 'help account' to see what type of information you can set.",cmdinfo.tok1,cmdinfo.tok1);
	say("");
	sayf(0,0,"If the account has been created in error, please delete it now by typing 'unregister %s'.",cmdinfo.tok1);
	say("");
	say("Have fun, and welcome!");

	context;
	Note_sendnotice(cmdinfo.user,"Welcome to your new account %s!\n\nType 'help' for major topics and commands.  If you are new to %s or bots in general, check out a brief tutorial by typing 'help tutorial'.  Help can be found for every command by using the '-h' switch.  View a list of all commands with 'cmdinfo.'.\n\nFinally, see how you can set some information for your account by typing 'help account'.",cmdinfo.user->acctname,Grufti->botnick);

	if(Grufti->send_notice_user_created)
		Note_sendnotification("A new account called \"%s\" has been created by %s %s.",cmdinfo.user->acctname,cmdinfo.from_n,cmdinfo.from_uh);

	sprintf(msg,"Account created by %s.", cmdinfo.user->acctname);
	User_addhistory(cmdinfo.user, "system", time(NULL), msg);
}

void cmd_chusername()
{
	if(!cmdinfo.isparam)
	{
		say("Usage: chusername <newusername>");
		sayf(2,0,"Use this command to change your username.");
		return;
	}

	say("Not yet implemented.");
}


void cmd_addnick()
{
	userrec *ucheck;

	context;
	if(!cmdinfo.isparam)
	{
		say("Usage: addnick <nick>");
		sayf(2,0,"Use this command to specify which IRC nicks you are known by, and which are authorized to use this account.  (Passwords are still needed to gain access.)");
		return;
	}

	/* Check if they're over the limit. */
	context;
	if(cmdinfo.user->num_nicks >= Grufti->max_nicks)
	{
		sayf(0,0,"You are allowed %d nicks for this account and you've set up all %d!  Type 'delnick <nick>' to delete one.",Grufti->max_nicks,Grufti->max_nicks);
		return;
	}

	/* Verify nick name */
	context;
	if(invalidacctname(cmdinfo.tok1))
	{
		sayf(0,0,"Nicks are between 2 and 9 characters in length and must be valid IRC nicks.  Please try again.");
		return;
	}

	/* nick is valid.  Check against database. */
	context;
	ucheck = User_user(cmdinfo.tok1);
	context;
	if(ucheck->registered)
	{
		if(isequal(ucheck->acctname,cmdinfo.user->acctname))
			sayf(0,0,"The nick \"%s\" is already registered to you!",cmdinfo.tok1);
		else
			sayf(0,0,"Sorry, the nick \"%s\" is already registered to %s.",cmdinfo.tok1,ucheck->acctname);
		return;
	}

	/* Nick is ok and not in use.  Add it. */
	context;
	User_addnewnick(cmdinfo.user,cmdinfo.tok1);

	/* Update all accounts (move from luser to user) */
	context;
	User_moveaccounts_into_newhome(cmdinfo.user,cmdinfo.tok1);

	/* Reset all user, account pointers in Channel, DCC, and Command. */
	context;
	User_resetallpointers();

	context;
	User_checkforsignon(cmdinfo.tok1);
	context;
	say("The nick \"%s\" has been added to your account.",cmdinfo.tok1);
}
		

void cmd_delnick()
{
	context;
	if(!cmdinfo.isparam)
	{
		say("Usage: delnick <nick>");
		sayf(2,0,"Use this command to delete a nick from your account.");
		return;
	}

	context;
	/* Verify nick is in their account. */
	if(!User_isnick(cmdinfo.user,cmdinfo.tok1))
	{
		sayf(0,0,"Your account does not contain the nick \"%s\".  Doh!",cmdinfo.tok1);
		return;
	}

	context;
	/* Make sure nick isn't their acctname */
	if(isequal(cmdinfo.tok1,cmdinfo.user->acctname))
	{
		sayf(0,0,"You can't delete that nick!  One of your nicks MUST be your account name.  Sorry!");
		return;
	}

	context;
	/* See if nick is the account they're using. */
	if(isequal(cmdinfo.tok1,cmdinfo.from_n))
	{
		sayf(0,0,"You're deleting the nick you're currently using!  Ok... but you'll have to change to one of your other nicks in order to access your account after this.  Bye!");
		say("");
	}

	context;
	/* ok. delete nick, and release all accounts with that nick to Luser. */
	User_removenick(cmdinfo.user,cmdinfo.tok1);
	User_releaseaccounts(cmdinfo.user,cmdinfo.tok1);

	context;
	/* Reset all user, account pointers in Channel, DCC, and Command. */
	User_resetallpointers();

	say("The nick \"%s\" has been deleted from your account.",cmdinfo.tok1);
}

void cmd_setpass()
{
	char	newpass[40];

	context;
	if(!cmdinfo.isparam)
	{
		say("Usage: setpass <password>");
		sayf(2,0,"Use this command to set your password.  Passwords are needed to gain access to your account.");
		return;
	}

	context;
	if(invalidpassword(cmdinfo.tok1))
	{
		sayf(0,0,"\"%s\" is not a valid password.  Passwords are between 3 and 9 characters in length.",cmdinfo.tok1);
		return;
	}

	context;
	/* ok password is good.  update it. */
	encrypt_pass(cmdinfo.tok1,newpass);
	userrec_setpass(cmdinfo.user,newpass);

	say("Password set to \"%s\".",cmdinfo.tok1);

	context;
	if(!cmdinfo.account->registered)
	{
		context;
		sayf(0,0,"Adding \"%s\" to your account.",cmdinfo.account->uh);
		Log_write(LOG_MAIN,"*","Added \"%s\" to %s account.",cmdinfo.account->uh,cmdinfo.user->acctname);
		cmdinfo.account->registered = TRUE;

		context;
		/* Mark all accounts matching this uh as registered */
		User_updateregaccounts(cmdinfo.user,cmdinfo.account->uh);

		context;
		/* Possible signon */
		User_gotpossiblesignon(cmdinfo.user,cmdinfo.account);
	}
}

void cmd_chpass()
{
	userrec	*u;
	char	newpass[40];

	context;
	if(cmdinfo.num_elements != 2)
	{
		say("Usage: chpass <acctname> <newpassword>");
		sayf(2,0,"Use this command to change the password for a user's account.");
		return;
	}

	context;
	if(!User_isknown(cmdinfo.tok1))
	{
		sayf(0,0,"The nick \"%s\" is not known to me.",cmdinfo.tok1);
		return;
	}

	context;
	u = User_user(cmdinfo.tok1);
	if(!u->registered)
	{
		say("The nick \"%s\" is not a registered nick.",cmdinfo.tok1);
		return;
	}

	context;
	/* ok got user.  now check password */
	if(invalidpassword(cmdinfo.tok2))
	{
		sayf(0,0,"\"%s\" is not a valid password.  Passwords are between 3 and 9 characters in length.",cmdinfo.tok2);
		return;
	}

	context;
	/* ok password is good.  update it */
	encrypt_pass(cmdinfo.tok2,newpass);
	userrec_setpass(u,newpass);

	say("Password set to \"%s\" for %s.",cmdinfo.tok2,u->acctname);
}

void cmd_setfinger()
{
	if(!cmdinfo.isparam)
	{
		say("Usage: setfinger <finger info>");
		sayf(2,0,"Use this command to set your finger info.  Specify 'none' to erase it.");
		return;
	}

	context;
	/* Check for 'none' */
	if(isequal(cmdinfo.param,"none"))
	{
		xfree(cmdinfo.user->finger);
		cmdinfo.user->finger = NULL;
		say("Finger info erased.");
		return;
	}

	context;
	/* Check length */
	if(cmdinfo.paramlen > LENGTH_FINGERINFO)
	{
		say("Your finger info is too long by %d character%s.",cmdinfo.paramlen - LENGTH_FINGERINFO,(cmdinfo.paramlen - LENGTH_FINGERINFO)==1?"":"s");
		return;
	}

	context;
	userrec_setfinger(cmdinfo.user,cmdinfo.param);
	say("Finger info set.  Thanks!");
}

void cmd_setemail()
{
	context;
	if(!cmdinfo.isparam)
	{
		say("Usage: setemail <email address>");
		sayf(2,0,"Use this command to set your email address.  Specify 'none' to erase it.");
		return;
	}

	context;
	/* Check for 'none' */
	if(isequal(cmdinfo.param,"none"))
	{
		xfree(cmdinfo.user->email);
		cmdinfo.user->email = NULL;
		say("Email address erased.");
		return;
	}

	context;
	/* Check length */
	if(cmdinfo.paramlen > USERFIELDLEN)
	{
		say("Your email address is too long by %d character%s.",cmdinfo.paramlen - USERFIELDLEN,(cmdinfo.paramlen - USERFIELDLEN)==1?"":"s");
		return;
	}

	context;
	userrec_setemail(cmdinfo.user,cmdinfo.tok1);
	say("Email address set.  Thanks!");
}

void cmd_setwww()
{
	context;
	if(!cmdinfo.isparam)
	{
		say("Usage: setwww <web address>");
		sayf(2,0,"Use this command to set your www address.  Specify 'none' to erase it.");
		return;
	}

	context;
	/* Check for 'none' */
	if(isequal(cmdinfo.param,"none"))
	{
		xfree(cmdinfo.user->www);
		cmdinfo.user->www = NULL;
		say("Web address erased.");
		return;
	}

	context;
	/* Check for "http://" */
	if(strncmp(cmdinfo.param,"http://",7) != 0)
	{
		sayf(0,0,"\"%s\" is not a valid web address. It must begin with \"http://\".",cmdinfo.param);
		return;
	}

	context;
	/* Check length */
	if(cmdinfo.paramlen > USERFIELDLEN)
	{
		say("Your web address is too long by %d character%s.",cmdinfo.paramlen - USERFIELDLEN,(cmdinfo.paramlen - USERFIELDLEN)==1?"":"s");
		return;
	}

	context;
	userrec_setwww(cmdinfo.user,cmdinfo.param);
	say("Web address set.  Thanks!");
}

void cmd_setforward()
{
	context;
	if(!cmdinfo.isparam)
	{
		say("Usage: setforward <email address>");
		sayf(2,0,"Use this command to have your Notes forwarded automatically.  Specify 'none' to unset and receive Notes as normal.");
		say("");
		sayf(2,0,"Be careful!!  If your email address is not valid, your Notes will disappear without warning!");
		say("");
		if(cmdinfo.user->forward && cmdinfo.user->forward[0])
			sayf(2,0,"Current forwarding address: %s",cmdinfo.user->forward);
		else
			sayf(2,0,"Current forwarding address: none");
		return;
	}

	context;
	/* Check for 'none' */
	if(isequal(cmdinfo.param,"none"))
	{
		xfree(cmdinfo.user->forward);
		cmdinfo.user->forward = NULL;
		say("Forwarding address unset.  Your Notes will not be forwarded.");
		return;
	}

	/* Check length */
	if(cmdinfo.paramlen > USERFIELDLEN)
	{
		say("Your email address is too long by %d character%s.",cmdinfo.paramlen - USERFIELDLEN,(cmdinfo.paramlen - USERFIELDLEN)==1?"":"s");
		return;
	}

	if(!strchr(cmdinfo.tok1,'@'))
	{
		say("Usage: setforward <email address>");
		sayf(2,0,"The address \"%s\" is not a valid email address.",cmdinfo.tok1);
		return;
	}

	context;
	userrec_setforward(cmdinfo.user,cmdinfo.tok1);

	sayf(0,0,"Forward address set.  Your Notes will be forwarded to %s as soon as they are received.  Thanks!  (Specify 'none' to unset it and stop forwarding Notes)",cmdinfo.user->forward);
}

void cmd_forwardnotes()
{
	noterec	*n;
	int x;

	if(!cmdinfo.isparam)
	{
		say("Usage: forwardnotes <email address>");
		sayf(2,0,"Use this command to forward all the Notes in your mailbox to the specified email address.  To have your Notes forwarded automatically, use the 'setforward' command to set a forwarding email address.");
		say("");
		sayf(2,0,"Be careful!!  If your forwarding address is not valid, your Notes will disappear without warning!");
		return;
	}

	if(!strchr(cmdinfo.tok1,'@'))
	{
		say("Usage: forwardnotes <email address>");
		sayf(2,0,"The address \"%s\" is not a valid email address.",cmdinfo.tok1);
		return;
	}

	for(n=cmdinfo.user->notes; n!=NULL; n=n->next)
		n->flags |= NOTE_FORWARD;

	if(!Note_numforwarded(cmdinfo.user))
	{
		say("You have no Notes to forward!");
		return;
	}

	say("Forwarding all Notes to %s...  Thanks!",cmdinfo.tok1);
	x = DCC_forwardnotes(cmdinfo.user,cmdinfo.tok1);

	if(!x)
		sayf(0,0,"Encountered problem connecting to SMTP server on %s.  Unable to forward Notes.",Grufti->smtp_gateway);
}


void cmd_setother1()
{
	char *p;
	context;
	if(!cmdinfo.isparam)
	{
		say("Usage: setother1 [type] <info>");
		sayf(2,0,"Use this command to set a user-defined type. (ie 'setother3 phone 867-5309')  Specify 'none' to erase it.  Use 'showother' to display all info slots.");
		return;
	}

	context;
	/* Check for 'none' */
	if(isequal(cmdinfo.param,"none"))
	{
		xfree(cmdinfo.user->other1);
		cmdinfo.user->other1 = NULL;
		say("Other1 erased.");
		return;
	}

	context;
	/* Check length */
	p = strchr(cmdinfo.param, ' ');
	if(p)
		cmdinfo.paramlen = strlen(p);

	if(cmdinfo.paramlen > USERFIELDLEN)
	{
		say("Your other1 line is too long by %d character%s.",cmdinfo.paramlen - USERFIELDLEN,(cmdinfo.paramlen - USERFIELDLEN)==1?"":"s");
		return;
	}

	context;
	userrec_setother1(cmdinfo.user,cmdinfo.param);
	say("Other1 set.  Thanks!");
}

void cmd_setother2()
{
	char *p;
	context;
	if(!cmdinfo.isparam)
	{
		say("Usage: setother2 [type] <info>");
		sayf(2,0,"Use this command to set a user-defined type. (ie 'setother3 phone 867-5309')  Specify 'none' to erase it.  Use 'showother' to display all info slots.");
		return;
	}

	context;
	/* Check for 'none' */
	if(isequal(cmdinfo.param,"none"))
	{
		xfree(cmdinfo.user->other2);
		cmdinfo.user->other2 = NULL;
		say("Other2 erased.");
		return;
	}

	context;
	/* Check length */
	p = strchr(cmdinfo.param, ' ');
	if(p)
		cmdinfo.paramlen = strlen(p);

	if(cmdinfo.paramlen > USERFIELDLEN)
	{
		say("Your other2 line is too long by %d character%s.",cmdinfo.paramlen - USERFIELDLEN,(cmdinfo.paramlen - USERFIELDLEN)==1?"":"s");
		return;
	}

	context;
	userrec_setother2(cmdinfo.user,cmdinfo.param);
	say("Other2 set.  Thanks!");
}

void cmd_setother3()
{
	char *p;
	context;
	if(!cmdinfo.isparam)
	{
		say("Usage: setother3 [type] <info>");
		sayf(2,0,"Use this command to set a user-defined type. (ie 'setother3 phone 867-5309')  Specify 'none' to erase it.  Use 'showother' to display all info slots.");
		return;
	}

	context;
	/* Check for 'none' */
	if(isequal(cmdinfo.param,"none"))
	{
		xfree(cmdinfo.user->other3);
		cmdinfo.user->other3 = NULL;
		say("Other3 erased.");
		return;
	}

	context;
	/* Check length */
	p = strchr(cmdinfo.param, ' ');
	if(p)
		cmdinfo.paramlen = strlen(p);

	if(cmdinfo.paramlen > USERFIELDLEN)
	{
		say("Your other3 line is too long by %d character%s.",cmdinfo.paramlen - USERFIELDLEN,(cmdinfo.paramlen - USERFIELDLEN)==1?"":"s");
		return;
	}

	context;
	userrec_setother3(cmdinfo.user,cmdinfo.param);
	say("Other3 set.  Thanks!");
}

void cmd_setother4()
{
	char *p;
	context;
	if(!cmdinfo.isparam)
	{
		say("Usage: setother4 [type] <info>");
		sayf(2,0,"Use this command to set a user-defined type. (ie 'setother3 phone 867-5309')  Specify 'none' to erase it.  Use 'showother' to display all info slots.");
		return;
	}

	context;
	/* Check for 'none' */
	if(isequal(cmdinfo.param,"none"))
	{
		xfree(cmdinfo.user->other4);
		cmdinfo.user->other4 = NULL;
		say("Other4 erased.");
		return;
	}

	context;
	/* Check length */
	p = strchr(cmdinfo.param, ' ');
	if(p)
		cmdinfo.paramlen = strlen(p);

	if(cmdinfo.paramlen > USERFIELDLEN)
	{
		say("Your other4 line is too long by %d character%s.",cmdinfo.paramlen - USERFIELDLEN,(cmdinfo.paramlen - USERFIELDLEN)==1?"":"s");
		return;
	}

	context;
	userrec_setother4(cmdinfo.user,cmdinfo.param);
	say("Other4 set.  Thanks!");
}

void cmd_setother5()
{
	char *p;
	context;
	if(!cmdinfo.isparam)
	{
		say("Usage: setother5 [type] <info>");
		sayf(2,0,"Use this command to set a user-defined type. (ie 'setother3 phone 867-5309')  Specify 'none' to erase it.  Use 'showother' to display all info slots.");
		return;
	}

	context;
	/* Check for 'none' */
	if(isequal(cmdinfo.param,"none"))
	{
		xfree(cmdinfo.user->other5);
		cmdinfo.user->other5 = NULL;
		say("Other5 erased.");
		return;
	}

	context;
	/* Check length */
	p = strchr(cmdinfo.param, ' ');
	if(p)
		cmdinfo.paramlen = strlen(p);

	if(cmdinfo.paramlen > USERFIELDLEN)
	{
		say("Your other5 line is too long by %d character%s.",cmdinfo.paramlen - USERFIELDLEN,(cmdinfo.paramlen - USERFIELDLEN)==1?"":"s");
		return;
	}

	context;
	userrec_setother5(cmdinfo.user,cmdinfo.param);
	say("Other5 set.  Thanks!");
}

void cmd_setother6()
{
	char *p;
	context;
	if(!cmdinfo.isparam)
	{
		say("Usage: setother6 [type] <info>");
		sayf(2,0,"Use this command to set a user-defined type. (ie 'setother3 phone 867-5309')  Specify 'none' to erase it.  Use 'showother' to display all info slots.");
		return;
	}

	context;
	/* Check for 'none' */
	if(isequal(cmdinfo.param,"none"))
	{
		xfree(cmdinfo.user->other6);
		cmdinfo.user->other6 = NULL;
		say("Other6 erased.");
		return;
	}

	context;
	/* Check length */
	p = strchr(cmdinfo.param, ' ');
	if(p)
		cmdinfo.paramlen = strlen(p);

	if(cmdinfo.paramlen > USERFIELDLEN)
	{
		say("Your other6 line is too long by %d character%s.",cmdinfo.paramlen - USERFIELDLEN,(cmdinfo.paramlen - USERFIELDLEN)==1?"":"s");
		return;
	}

	context;
	userrec_setother6(cmdinfo.user,cmdinfo.param);
	say("Other6 set.  Thanks!");
}

void cmd_setother7()
{
	char *p;
	context;
	if(!cmdinfo.isparam)
	{
		say("Usage: setother7 [type] <info>");
		sayf(2,0,"Use this command to set a user-defined type. (ie 'setother3 phone 867-5309')  Specify 'none' to erase it.  Use 'showother' to display all info slots.");
		return;
	}

	context;
	/* Check for 'none' */
	if(isequal(cmdinfo.param,"none"))
	{
		xfree(cmdinfo.user->other7);
		cmdinfo.user->other7 = NULL;
		say("Other7 erased.");
		return;
	}

	context;
	/* Check length */
	p = strchr(cmdinfo.param, ' ');
	if(p)
		cmdinfo.paramlen = strlen(p);

	if(cmdinfo.paramlen > USERFIELDLEN)
	{
		say("Your other7 line is too long by %d character%s.",cmdinfo.paramlen - USERFIELDLEN,(cmdinfo.paramlen - USERFIELDLEN)==1?"":"s");
		return;
	}

	context;
	userrec_setother7(cmdinfo.user,cmdinfo.param);
	say("Other7 set.  Thanks!");
}

void cmd_setother8()
{
	char *p;
	context;
	if(!cmdinfo.isparam)
	{
		say("Usage: setother8 [type] <info>");
		sayf(2,0,"Use this command to set a user-defined type. (ie 'setother3 phone 867-5309')  Specify 'none' to erase it.  Use 'showother' to display all info slots.");
		return;
	}

	context;
	/* Check for 'none' */
	if(isequal(cmdinfo.param,"none"))
	{
		xfree(cmdinfo.user->other8);
		cmdinfo.user->other8 = NULL;
		say("Other8 erased.");
		return;
	}

	context;
	/* Check length */
	p = strchr(cmdinfo.param, ' ');
	if(p)
		cmdinfo.paramlen = strlen(p);

	if(cmdinfo.paramlen > USERFIELDLEN)
	{
		say("Your other8 line is too long by %d character%s.",cmdinfo.paramlen - USERFIELDLEN,(cmdinfo.paramlen - USERFIELDLEN)==1?"":"s");
		return;
	}

	context;
	userrec_setother8(cmdinfo.user,cmdinfo.param);
	say("Other8 set.  Thanks!");
}

void cmd_setother9()
{
	char *p;
	context;
	if(!cmdinfo.isparam)
	{
		say("Usage: setother9 [type] <info>");
		sayf(2,0,"Use this command to set a user-defined type. (ie 'setother3 phone 867-5309')  Specify 'none' to erase it.  Use 'showother' to display all info slots.");
		return;
	}

	context;
	/* Check for 'none' */
	if(isequal(cmdinfo.param,"none"))
	{
		xfree(cmdinfo.user->other9);
		cmdinfo.user->other9 = NULL;
		say("Other9 erased.");
		return;
	}

	context;
	/* Check length */
	p = strchr(cmdinfo.param, ' ');
	if(p)
		cmdinfo.paramlen = strlen(p);

	if(cmdinfo.paramlen > USERFIELDLEN)
	{
		say("Your other9 line is too long by %d character%s.",cmdinfo.paramlen - USERFIELDLEN,(cmdinfo.paramlen - USERFIELDLEN)==1?"":"s");
		return;
	}

	context;
	userrec_setother9(cmdinfo.user,cmdinfo.param);
	say("Other9 set.  Thanks!");
}

void cmd_setother10()
{
	char *p;
	context;
	if(!cmdinfo.isparam)
	{
		say("Usage: setother10 [type] <info>");
		sayf(2,0,"Use this command to set a user-defined type. (ie 'setother3 phone 867-5309')  Specify 'none' to erase it.  Use 'showother' to display all info slots.");
		return;
	}

	context;
	/* Check for 'none' */
	if(isequal(cmdinfo.param,"none"))
	{
		xfree(cmdinfo.user->other10);
		cmdinfo.user->other10 = NULL;
		say("Other10 erased.");
		return;
	}

	context;
	/* Check length */
	p = strchr(cmdinfo.param, ' ');
	if(p)
		cmdinfo.paramlen = strlen(p);

	if(cmdinfo.paramlen > USERFIELDLEN)
	{
		say("Your other9 line is too long by %d character%s.",cmdinfo.paramlen - USERFIELDLEN,(cmdinfo.paramlen - USERFIELDLEN)==1?"":"s");
		return;
	}

	context;
	userrec_setother10(cmdinfo.user,cmdinfo.param);
	say("Other10 set.  Thanks!");
}

void cmd_setplan1()
{
	context;
	if(!cmdinfo.isparam)
	{
		say("Usage: setplan1 <plan>");
		sayf(2,0,"Use this command to set plan line 1.  Specify 'none' to erase it.");
		return;
	}

	context;
	/* Check for none */
	if(isequal(cmdinfo.param,"none"))
	{
		xfree(cmdinfo.user->plan1);
		cmdinfo.user->plan1 = NULL;
		say("Plan line 1 erased.");
		return;
	}

	context;
	cmd_setplan_helper(1);
	say("Plan line 1 set.  Thanks!");
}

void cmd_setplan2()
{
	context;
	if(!cmdinfo.isparam)
	{
		say("Usage: setplan2 <plan>");
		sayf(2,0,"Use this command to set plan line 2.  Specify 'none' to erase it.");
		return;
	}

	context;
	/* Check for none */
	if(isequal(cmdinfo.param,"none"))
	{
		xfree(cmdinfo.user->plan2);
		cmdinfo.user->plan2 = NULL;
		say("Plan line 2 erased.");
		return;
	}

	context;
	cmd_setplan_helper(2);
	say("Plan line 2 set.  Thanks!");
}

void cmd_setplan3()
{
	if(!cmdinfo.isparam)
	{
		say("Usage: setplan3 <plan>");
		sayf(2,0,"Use this command to set plan line 3.  Specify 'none' to erase it.");
		return;
	}

	/* Check for none */
	if(isequal(cmdinfo.param,"none"))
	{
		xfree(cmdinfo.user->plan3);
		cmdinfo.user->plan3 = NULL;
		say("Plan line 3 erased.");
		return;
	}

	cmd_setplan_helper(3);
	say("Plan line 3 set.  Thanks!");
}

void cmd_setplan4()
{
	if(!cmdinfo.isparam)
	{
		say("Usage: setplan4 <plan>");
		sayf(2,0,"Use this command to set plan line 4.  Specify 'none' to erase it.");
		return;
	}

	/* Check for none */
	if(isequal(cmdinfo.param,"none"))
	{
		xfree(cmdinfo.user->plan4);
		cmdinfo.user->plan4 = NULL;
		say("Plan line 4 erased.");
		return;
	}

	cmd_setplan_helper(4);
	say("Plan line 4 set.  Thanks!");
}

void cmd_setplan5()
{
	if(!cmdinfo.isparam)
	{
		say("Usage: setplan5 <plan>");
		sayf(2,0,"Use this command to set plan line 5.  Specify 'none' to erase it.");
		return;
	}

	/* Check for none */
	if(isequal(cmdinfo.param,"none"))
	{
		xfree(cmdinfo.user->plan5);
		cmdinfo.user->plan5 = NULL;
		say("Plan line 5 erased.");
		return;
	}

	cmd_setplan_helper(5);
	say("Plan line 5 set.  Thanks!");
}

void cmd_setplan6()
{
	if(!cmdinfo.isparam)
	{
		say("Usage: setplan6 <plan>");
		sayf(2,0,"Use this command to set plan line 6.  Specify 'none' to erase it.");
		return;
	}

	/* Check for none */
	if(isequal(cmdinfo.param,"none"))
	{
		xfree(cmdinfo.user->plan6);
		cmdinfo.user->plan6 = NULL;
		say("Plan line 6 erased.");
		return;
	}

	cmd_setplan_helper(6);
	say("Plan line 6 set.  Thanks!");
}

void cmd_setplan_helper(int line)
{
	char *p, *q, c;

	p = cmdinfo.param;

	/* Skip past '.' if it's the first character */
	if(*p == '.')
		p++;

	while(strlen(p) > LENGTH_PLANLINE)
	{
		q = p + LENGTH_PLANLINE;

		/* Search backwards for last space */
		while((*q != ' ') && (q != p))
			q --;
		if(q == p)
			q = p + LENGTH_PLANLINE;
		c = *q;
		*q = 0;
		userrec_setplanx(cmdinfo.user,p,line);
		line++;
		if(line > NUMPLANLINES)
			return;
		*q = c;

		p = (c == ' ') ? q + 1 : q;
	}

	/* Left with string < LENGTH_PLANLINE */
	if(*p)
		userrec_setplanx(cmdinfo.user,p,line);
}

void cmd_setlocation()
{
	int	num = 0;
	locarec	*l;
	char	l_info[SERVERLEN];

	context;
	if(!cmdinfo.isparam)
	{
		say("Usage: setlocation <#>");
		sayf(2,0,"Use this command to set your location.  Type 'showlocations' to see which locations you can use.  Specify 'none' to clear it.");
		if(cmdinfo.user->location)
		{
			Location_idtostr(cmdinfo.user->location,l_info);
			sayf(2,0,"Currently: \"%s\"",l_info);
		}
		return;
	}

	context;
	/* Check for none */
	if(isequal(cmdinfo.tok1,"none"))
	{
		Location_decrement(cmdinfo.user);
		cmdinfo.user->location = 0;
		say("Location info erased.");
		return;
	}

	context;
	if(!isanumber(cmdinfo.tok1))
	{
		say("Usage: setlocation <#>");
		sayf(2,0,"You must use a predefined location number to set your location.  Type 'showlocations' to see which numbers you can use.");
		return;
	}

	context;
	num = atoi(cmdinfo.tok1);
	l = Location_location(num);

	context;
	/* Oops, number isn't valid */
	if(l == NULL)
	{
		say("Usage: setlocation <#>");
		sayf(2,0,"Location \"%d\" is not a valid location number.  Type 'showlocations' to see which location numbers you can use.",num);
		return;
	}

	context;
	/* ok good to go. */
	cmdinfo.user->location = num;
	
	Location_ltostr(l,l_info);
	sayf(0,0,"Location set to: \"%s\"",l_info);

	Location_increment(cmdinfo.user);
}
	
void cmd_unregister()
{
	context;
	if(!cmdinfo.isparam)
	{
		say("Usage: unregister <acctname>");
		sayf(2,0,"Use this command to delete your account.  Notes and user information are deleted as well.");
		return;
	}

	if(!isequal(cmdinfo.param,cmdinfo.user->acctname))
	{
		say("Usage: unregister <acctname>");
		sayf(2,0,"The acctname \"%s\" is not correct.  Please specify your acctname.",cmdinfo.param);
		return;
	}

	/* Don't let the owner unregister his or her own account! */
	if(cmdinfo.user->flags & USER_OWNER)
	{
		sayf(0,0,"You are trying to delete your account, which is the owner account!  I can't let you do this :>");
		return;
	}

	context;
	sayf(0,0,"The account \"%s\" has been deleted.",cmdinfo.user->acctname);
	if(Grufti->send_notice_user_deleted)
		Note_sendnotification("%s has unregistered his or her account.",cmdinfo.user->acctname);
	User_removeuser(cmdinfo.user);
	User_resetallpointers();
}

void cmd_motd()
{
	servrec *serv;
	char	motd[256];

	context;
	if(!cmdinfo.isparam)
	{
		if(cmdinfo.d == NULL)
			return;
		sprintf(motd,"%s/%s",Grufti->admindir,Grufti->motd_dcc);
		catfile(cmdinfo.d,motd);

		/* Update last seen motd */
		if(cmdinfo.user->registered)
			cmdinfo.user->seen_motd = time(NULL);
		return;
	}

	context;
	/* Arguments.  Check against servers. */
	for(serv=Server->serverlist; serv!=NULL; serv=serv->next)
	{
		if(isequal(serv->name,cmdinfo.tok1))
		{
			/* Haven't seen them yet. */
			if(serv->motd == NULL)
			{
				sayf(0,0,"No motd for server %s.",serv->name);
				say("I haven't connected with it yet.");
				return;
			}

			/* Display motd */
			say("***** MOTD for %s *****",serv->name);
			sayf(0,0,"%s",serv->motd);
			return;
		}
	}

	context;
	/* not found */
	sayf(0,0,"Sorry, I've never seen the server %s.",cmdinfo.tok1);
}

void cmd_resetmotd()
{
	Grufti->motd_modify_date = time(NULL);
	say("The MOTD has been reset.");
	if(!Grufti->show_motd_onlywhenchanged)
		say("'show-motd-onlywhenchanged' is set to OFF though...");
}

void cmd_pass()
/*
 * 1) No password?
 * 2) User already verified?
 * 3) Check password
 * 4)   If ok, update reg accounts and set the dcc pass bit (IF DCC).
 * 5)   If ok and not DCC, run pending command.  they'll need to send the
 *         password again to do another command.
 * 6)   If not ok, later.
 */
{
	char	cipher[PASSWORD_CIPHER_LEN];
	acctrec *account;

	context;
	if(!cmdinfo.isparam)
	{
		say("Usage: pass <password> [userhost]");
		sayf(2,0,"Use this command to identify yourself to the bot.  If a userhost is specified, the authentication will be performed on the corresponding hostmsk.  For example: pass password stever@*.home.com");
		return;
	}

	/* have to check registered status here */
	if(!cmdinfo.user->registered)
	{
		if(cmdinfo.d)
		{
			say("This command requires that you be registered.");
			say("");
			sayf(0,0,"The nick \"%s\" is not a registered nick.  To register this nick with the bot type 'register'.  By registering with the bot you will secure this nick as yours, have the ability to set personal info like your email and www address, and you will be able to receive Notes and acquire levels.",cmdinfo.from_n);
			say("");
			sayf(0,0,"If you are already registered and are using a nick which is not listed for your account, type 'addthisnick <acctname> <password>' to add it.");
			say("");
			sayf(0,0,"Thanks, have a nice day!");
		}
		else
			sayf(0,0,"This command requires that you be registered.  Type 'register' to create an account.  (Type 'addthisnick' if you are already registered...)");
		return;
	}

	if(cmdinfo.d == NULL && Grufti->require_dcc_for_pass_cmd)
	{
		/* User trying to send pass via msg.  no no */
		say("You must use a DCC connection to send your password, as passwords sent via normal message can easily be compromised.  Also, verify the user and host information to make sure the bot is not a fake.");
		return;
	}
		
	context;
	/* User has no password */
	if(cmdinfo.user->pass == NULL)
	{
		sayf(0,0,"You have no password set.  Use 'setpass <password>' to set one.");
		return;
	}

	context;
	/* Check password */
	encrypt_pass(cmdinfo.tok1,cipher);
	if(isequal(cmdinfo.user->pass,cipher))
	{
		/* password OK */
		cmdinfo.user->last_login = time(NULL);

		/* User is already identified */
		if(cmdinfo.d && (cmdinfo.d->flags & STAT_VERIFIED) && !cmdinfo.istok2)
		{
			say("You're already identified.  Doh!");
			if(cmdinfo.user->flags & USER_OP)
				Channel_trytogiveops(cmdinfo.from_n);
			return;
		}

		if(cmdinfo.istok2)
		{
			char nick[BUFSIZ], uh[BUFSIZ];
			if(strchr(cmdinfo.tok2, '!'))
				splitc(nick, cmdinfo.tok2, '!');
			else
				strcpy(nick, cmdinfo.from_n);
			strcpy(uh, cmdinfo.tok2);

			account = User_account_dontmakenew(cmdinfo.user, nick, uh);
			if(account == NULL)
			{
				say("That hostmask does not exist in your account.  Only hostmasks which already appear in your account can be registered.");
				return;
			}
		}
		else
			account = cmdinfo.account;

		context;
		/* Update other accounts and mark them R if any match */
		if(!account->registered)
		{
			sayf(0,0,"Adding \"%s\" to your account.",account->uh);
			Log_write(LOG_MAIN,"*","Added \"%s\" to %s account.",account->uh,cmdinfo.user->acctname);
			account->registered = TRUE;

			/* Mark all accounts matching this uh as registered */
			User_updateregaccounts(cmdinfo.user,account->uh);

			/* Possible signon only if account was not sent as argument */
			if(!cmdinfo.istok2)
				User_gotpossiblesignon(cmdinfo.user,account);
		}

		context;
		/* If DCC, set verified flag */
		if(cmdinfo.d)
			cmdinfo.d->flags |= STAT_VERIFIED;

		/* Op user if they have +o */
		if(cmdinfo.user->flags & USER_OP)
			Channel_trytogiveops(cmdinfo.from_n);

		context;
		/*
		 * Only execute pending command if
		 *  - the command was issued in the last n seconds
		 *  - the command was issued by the same nick sending the password
		 */
		if(cmdinfo.user->cmd_pending_pass != NULL &&
			(time(NULL) - cmdinfo.user->cmdtime_pending_pass) <
			Grufti->pending_command_timeout &&
			isequal(cmdinfo.user->cmdnick_pending_pass, cmdinfo.from_n))
		{
			/* cmdinfo.from parameters are ok. */
			strcpy(cmdinfo.buf, cmdinfo.user->cmd_pending_pass);

			/* Free pending command and set flag */
			xfree(cmdinfo.user->cmd_pending_pass);
			cmdinfo.user->cmd_pending_pass = NULL;
			xfree(cmdinfo.user->cmdnick_pending_pass);
			cmdinfo.user->cmdnick_pending_pass = NULL;

			/* Override verified */
			cmdinfo.flags |= COMMAND_SESSIONVERIFIED; 

			/* Execute it */
			command_do();

			/* Turn it off */
			cmdinfo.flags &= ~COMMAND_SESSIONVERIFIED; 
		}
		else
			say("It's good to see you again %s.",cmdinfo.user->acctname);

		return;
	}

	context;
	/* password incorrect.  discipline. */
	say("Wrong password for %s.  Go away.",cmdinfo.user->acctname);
	Log_write(LOG_CMD,"*","%s %s - <password was invalid>",cmdinfo.from_n,cmdinfo.from_uh);
}

void cmd_exit()
{
	context;
	if(cmdinfo.d)
	{
		say("Bye!");
		/* DON'T disconnect here!  If we do, we get messed up
		 * returning to dcc_gottelnetchatactivity.  It expects 'd'
		 * to be active.  Flag it and continue.  We'll check it in
		 * a few usecs.
		 */
		cmdinfo.d->flags |= STAT_DISCONNECT_ME;
		return;
	}

	context;
	say("That command is only valid if you are connected via dcc.  Doh!");
}

void cmd_addserver()
{
	int	port;
	char	*p;

	context;
	if(!cmdinfo.isparam)
	{
		say("Usage: addserver <servername> [port]");
		sayf(2,0,"Add a server to the serverlist.");
		return;
	}

	context;
	if(!cmdinfo.istok2)
	{
		port = DEFAULT_PORT;
		p = strchr(cmdinfo.tok1,':');
		if(p != NULL)
		{
			/* ':' embedded in servername. */
			*p = 0;
			port = atoi(p+1);
		}
	}
	else
		port = atoi(cmdinfo.tok2);

	context;
	Server_addserver(cmdinfo.tok1,port);

	sayf(0,0,"Server \"%s:%d\" added to serverlist.",cmdinfo.tok1,port);
}

void cmd_jump()
/*
 * To jump servers, all we need to do is set Server->tojump to the server
 * we want to jump to, then issue a QUIT.  Server_connect() will check for
 * pending jumps, and will take care of the rest.
 */
{
	servrec	*serv;
	char	oldname[HOSTLEN], *p;
	int	oldport, port = 0;

	context;
	/* Remember old values */
	strcpy(oldname,Server_name());
	oldport = Server_port();

	context;
	if(Server->flags & SERVER_QUIT_IN_PROGRESS)
	{
		sayf(0,0,"Sorry, I'm still waiting for %s to shut down...",Server_name());
		return;
	}

	context;
	if(!Grufti->irc)
	{
		say("IRC is turned OFF.");
		return;
	}

	context;
	/* No arguments.  Just QUIT, and let Server_connect() do its thing. */
	if(!cmdinfo.isparam)
	{
		context;
		if(Server->num_servers == 0)
		{
			say("No servers configured.");
			return;
		}

		context;
		if(!Server_isconnected())
		{
			/* Make next server unattempted, and try that one */
			Server->tojump = Server_nextserver();
			say("Trying %s:%d...",Server->tojump->name,Server->tojump->port);
			Server_makeunattempted(Server->tojump);
			Server->time_noserverpossible = 0L;
			return;
		}

		context;
		/* Where are we going */
		Server->tojump = Server_nextserver();
		sprintf(Server->sbuf,"changing servers %s:%d --> %s:%d",Server_name(),Server_port(),Server->tojump->name,Server->tojump->port);

		context;
		/* Tell server to shut things down */
		Server_quit(Server->sbuf);

		sprintf(Server->sbuf,"Jump request by %s",cmdinfo.from_n);

		context;
		/* set the reason for this shutdown */
		Server_setreason(Server->sbuf);

		sayf(0,0,"Jumping %s:%d --> %s:%d",Server_name(),Server_port(),Server->tojump->name,Server->tojump->port);
		return;
	}

	context;
	/* No port? */
	if(!cmdinfo.istok2)
	{
		/* no port specified. See if a ':' is embedded. */
		p = strchr(cmdinfo.tok1,':');
		if(p != NULL)
		{
			/* ok well.  search for name and embedded port */
			*p = 0;
			port = atoi(p+1);
			serv = Server_serverandport(cmdinfo.tok1,port);
		}
		else
			serv = Server_server(cmdinfo.tok1);
	}
	else
	{
		port = atoi(cmdinfo.tok2);
		serv = Server_serverandport(cmdinfo.tok1,port);
	}

	context;
	/* whew.  ok did we get one? */
	if(serv == NULL)
	{
		if(!port)
			port = DEFAULT_PORT;

		say("Server not found in my list.  Adding...");
		Server->tojump = Server_addserver(cmdinfo.tok1,port);
	}
	else
		Server->tojump = serv;

	context;
	if(!Server_isconnected())
	{
		/* Make next server unattempted, and try that one */
		Server_makeunattempted(Server->tojump);
		say("Trying %s:%d...",Server->tojump->name,Server->tojump->port);
		Server->time_noserverpossible = 0L;
		return;
	}

	context;
	sprintf(Server->sbuf,"changing servers %s:%d --> %s:%d",Server_name(),Server_port(),Server->tojump->name,Server->tojump->port);

	/* Tell server to shut down */
	Server_quit(Server->sbuf);

	sprintf(Server->sbuf,"Jump request by %s",cmdinfo.from_n);

	context;
	/* Set the reason for this shutdown */
	Server_setreason(Server->sbuf);

	sayf(0,0,"Jumping %s:%d --> %s:%d",Server_name(),Server_port(),Server->tojump->name,Server->tojump->port);
}

void cmd_delserver()
{
	servrec	*serv;
	int	port = 0;
	char	*p;

	if(!cmdinfo.isparam)
	{
		say("Usage: delserver <servername> [port]");
		sayf(2,0,"Delete a server from the serverlist.");
		return;
	}

	context;
	if(Server->flags & SERVER_QUIT_IN_PROGRESS)
	{
		/*
		 * Shit.  If we 'jump'ed a server and it's still waiting for
		 * it's QUIT signal, the next server it's planning to jump to
		 * (Server->tojump) may get deleted here, which would not be
		 * good.  Don't take any chances.
		 */
		sayf(0,0,"Sorry, I'm still waiting for %s to shut down...",Server_name());
		return;
	}

	context;
	if(!cmdinfo.istok2)
	{
		/* no port specified.  See if ':' is embedded. */
		p = strchr(cmdinfo.tok1,':');
		if(p != NULL)
		{
			/* ok well.  search for name and embedded port */
			*p = 0;
			port = atoi(p+1);
			serv = Server_serverandport(cmdinfo.tok1,port);
		}
		else
			serv = Server_server(cmdinfo.tok1);
	}
	else
	{
		port = atoi(cmdinfo.tok2);
		serv = Server_serverandport(cmdinfo.tok1,port);
	}

	context;
	/* ok did we get one? */
	if(serv == NULL)
	{
		if(port)
			sayf(0,0,"The server \"%s:%d\" is not known to me.",cmdinfo.tok1,port);
		else
			sayf(0,0,"The server \"%s\" is not known to me.",cmdinfo.tok1);
		return;
	}

	context;
	if(!Server_isconnected() || serv != Server->cur)
	{
#if 0
		Server->cur = serv;
		Server->tojump = Server_nextserver();
#endif
		if(Server->tojump == serv)
			Server->tojump = NULL;

		sayf(0,0,"Server %s:%d deleted.",serv->name,serv->port);
		Server_deleteserver(serv);
		return;
	}

	context;
	if(serv == Server->cur)
	{
		context;
		/*
		 * Don't delete server yet.  Wait for disconnect or all
		 * hell breaks loose.  (We actually have to wait until ->cur
		 * does not point to this server record anymore, which happens
		 * in Server_connect()...)
		 */
		/* Where are we going */
		Server->tojump = Server_nextserver();

		/* This server is gone once we jump */
		Server->flags |= SERVER_DELETE_ON_DISCON;

		/*
		 * If Server->tojump == serv, then the only server in the
		 * list is about to be deleted.
		 */
		if(Server->tojump == serv)
		{
			Server->tojump = NULL;
			sayf(0,0,"Server %s:%d deleted.",serv->name,serv->port);
		}
		else
		{
			sprintf(Server->sbuf,"changing servers %s:%d --> %s:%d",Server_name(),Server_port(),Server->tojump->name,Server->tojump->port);
			sayf(0,0,"Server deleted.  Jumping to \"%s:%d\".",Server->tojump->name,Server->tojump->port);
		}

		/* Tell server to shut things down */
		Server_quit(Server->sbuf);
	}
}

void cmd_setvar()
{
	int x;
	char var[STDBUF], param[STDBUF];

	context;
	if(!cmdinfo.isparam)
	{
		say("Usage: setvar <variable> [value]");
		sayf(2,0,"Use this command to set an internal variable.  Leave off the value argument to display the current value.  Use 'configinfo' to display all configuration variables.");
		return;
	}

	context;
	if(!cmdinfo.istok2)
	{
		char	ret[BUFSIZ];
		if(!Grufti_displayvar(cmdinfo.tok1,ret))
			say("No such variable \"%s\".  Use 'configinfo' to show all configuration variables.",cmdinfo.tok1);
		else
			say("%s is %s",cmdinfo.tok1,ret);
		return;
	}

	context;
	strcpy(param, cmdinfo.param);
	split(var, param);
	x = Grufti_setvar(var, param);
	if(x < 0)
	{
		if(x == -1)
			sayf(0,0,"Unable to set variable: %s.",var);
		else if(x == -2)
			sayf(0,0,"Argument not quoted.  (eg. setvar nick \"Grufti\")");
		else if(x == -3)
			sayf(0,0,"No such variable: %s", var);
		return;
	}

	sayf(0,0,"%s set to \"%s\".",var, param);

	context;
	if(isequal(var,"telnet-port"))
	{
		/* close old port. */
		dccrec *d;

		d = DCC_dccserverbyname("Server001");
		if(d != NULL)
			DCC_disconnect(d);

		DCC_addnewconnection("Server001",Grufti->botuh,0,0,Grufti->reqd_telnet_port,DCC_SERVER|SERVER_PUBLIC);
	}
	else if(isequal(var,"gateway-port"))
	{
		/* close old port */
		dccrec	*d;

		d = DCC_dccserverbyname("Server002");
		if(d != NULL)
			DCC_disconnect(d);

		DCC_addnewconnection("Server002",Grufti->botuh,0,0,Grufti->reqd_gateway_port,DCC_SERVER|SERVER_INTERNAL);
	}
	else if(isequal(var,"nick"))
		Server_nick(Grufti->botnick);
}

void cmd_servinfo()
{
	servrec	*serv;
	char	*p, dis_time[TIMELONGLEN];
	int	port = 0;

	if(!cmdinfo.isparam)
	{
		Server_display();
		return;
	}

	context;
	/* Check if argument is '-r' (for showreasons) */
	if(isequal(cmdinfo.tok1,"-r"))
	{
		Server->flags |= SERVER_SHOW_REASONS;
		Server_display();
		Server->flags &= ~SERVER_SHOW_REASONS;
		return;
	}

	context;
	if(!cmdinfo.istok2)
	{
		/* no port specified.  See if ':' is embedded. */
		p = strchr(cmdinfo.tok1,':');
		if(p != NULL)
		{
			/* ok well.  search for name and embedded port */
			*p = 0;
			port = atoi(p+1);
			serv = Server_serverandport(cmdinfo.tok1,port);
		}
		else
			serv = Server_server(cmdinfo.tok1);
	}
	else
	{
		port = atoi(cmdinfo.tok2);
		serv = Server_serverandport(cmdinfo.tok1,port);
	}

	context;
	/* ok did we get one? */
	if(serv == NULL)
	{
		if(port)
			sayf(0,0,"The server \"%s:%d\" is not known to me.",cmdinfo.tok1,port);
		else
			sayf(0,0,"The server \"%s\" is not known to me.",cmdinfo.tok1);
		return;
	}

	context;
	/* got one. */
	say("  Server                         Port Con'd AvLag    In   Out Net");
	say("------------------------------- ----- ----- ----- ----- ----- ---");
	servrec_display(serv);
	say("------------------------------- ----- ----- ----- ----- ----- ---");
	timet_to_ago_long(serv->disconnect_time,dis_time);
	if(serv != Server->cur)
	{
		if((serv->flags & SERV_CONNECT_ATTEMPTED) && !(serv->flags & SERV_WAS_ACTIVEIRC))
		{
			if(serv->reason)
			{
				say("- Unable to connect on attempt %s ago.",dis_time);
				say("- Reason: %s",serv->reason);
			}
			else
				say("- Unable to connect on attempt %s ago.",dis_time);
		}
		else if(!(serv->flags & SERV_NEVER_TRIED))
		{
			if(serv->reason)
			{
				say("- Disconnected %s ago.",dis_time);
				say("- Reason: %s",serv->reason);
			}
			else
				say("- Disconnected %s ago.",dis_time);
		}
		else
			say("- Server not yet tried.");
	}
	else
		say("Current server.");
}

void cmd_time()
{
	char	t[TIMELONGLEN];

	context;
	timet_to_date_long(time(NULL),t);
	say("%s",t);
}

void cmd_chaninfo()
{
	membrec	*m;
	char	flags[FLAGLONGLEN], modes[FLAGLONGLEN], chname[CHANLEN];
	char	join[TIMESHORTLEN], idle[TIMESHORTLEN], uh[UHOSTLEN];
	char	levels[FLAGSHORTLEN], mflags[FLAGSHORTLEN], spl[TIMESHORTLEN];
	int	summary = 0, tot, i;
	chanrec *chan;

	if(!cmdinfo.isparam || (isequal(cmdinfo.tok1,"-s") && !cmdinfo.istok2))
	{
		say("Usage: who [-s] <chname>");
		sayf(2,0,"Use this command to display who is on the specified channel and other information like the topic.  Use the '-s' switch for a summary listing.");
		return;
	}

	context;
	if(isequal(cmdinfo.tok1,"-s"))
	{
		summary = 1;
		strcpy(chname,cmdinfo.tok2);
	}
	else
		strcpy(chname,cmdinfo.tok1);

	chan = Channel_channel(chname);
	context;
	/* ok did we get one? */
	if(chan == NULL)
	{
		sayf(0,0,"The channel \"%s\" is not known to me.",chname);
		return;
	}

	context;
	/* got one. */
	if(!Grufti->irc)
		strcpy(flags,"IRC is set to OFF.");
	else if(chan->flags & CHAN_ONCHAN)
	{
		if(!(chan->flags & CHAN_ACTIVE))
			strcpy(flags,"Joined, reading list of who is on channel...");
		else
			strcpy(flags,"");
	}
	else
	{
		if(chan->nojoin_reason)
			sprintf(flags,"Couldn't join channel (%s).",chan->nojoin_reason);
		else
			strcpy(flags,"Waiting for JOIN to happen...");
	}

	context;
	Channel_modes2str(chan,modes);
	context;
	if(modes[0])
		say("Channel: %s (%s)",chan->name,modes);
	else
		say("Channel: %s",chan->name);
	context;
	if(flags[0])
		say("Status: %s",flags);
	else
	{
	context;
		if(chan->topic == NULL)
			say("Topic:");
		else if(chan->topic[0] == 0)
			say("Topic:");
		else
			say("Topic: \"%s\"",chan->topic);
	}

	if(summary)
	{
		if(chan->flags & CHAN_ACTIVE)
		{
			char	joins_reg[120], pub[120], pop[120], tmp[125];
			char	joins_nonreg[120];
			char	minpop_time[TIMELONGLEN];
			char	maxpop_time[TIMELONGLEN];
			int	thishr = thishour();
			char	yest[25];

			context;
			say("There %s %d %s on the channel.  (%d ban%s)",chan->num_members==1?"is":"are",chan->num_members,chan->num_members==1?"person":"people",chan->num_bans,chan->num_bans==1?"":"s");
			context;
			say("R/NRJoins = Joins by Reg'd/Non-Reg'd Users");

			/* Display 1st half of day */
			say("");
			context;
			strcpy(tmp,"Type  12am  1am  2am  3am  4am  5am  6am  7am  8am  9am 10am 11am");
			context;
			if(thishr < 12)
			{
				tmp[5 + (5*thishr) + 1] = ' ';
				tmp[5 + (5*thishr) + 2] = 'N';
				tmp[5 + (5*thishr) + 3] = 'O';
				tmp[5 + (5*thishr) + 4] = 'W';
			}
			context;
			say(tmp);
			context;
			say("----- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----");
			context;
			/* Display joins */
			joins_reg[0] = 0;
			context;
			for(i=0; i<12; i++)
				sprintf(joins_reg,"%s %4d",joins_reg,chan->joins_reg[i]);
			context;
			say("RJoin%s",joins_reg);

			/* Display joins */
			context;
			joins_nonreg[0] = 0;
			for(i=0; i<12; i++)
				sprintf(joins_nonreg,"%s %4d",joins_nonreg,chan->joins_nonreg[i]);
			context;
			say("NRJoi%s",joins_nonreg);

			/* Display public chatter */
			context;
			pub[0] = 0;
			for(i=0; i<12; i++)
				sprintf(pub,"%s %3luk",pub,chan->chatter[i] / 1024);
			context;
			say("Publc%s",pub);

			/* Display Avg Population */
			context;
			pop[0] = 0;
			context;
			for(i=0; i<12; i++)
			{
			context;
				if(i == thishr)
					sprintf(pop,"%s %4d",pop,(int)(chan->pop_prod / (time(NULL) - chan->joined + 1)));
				else
					sprintf(pop,"%s %4d",pop,chan->population[i]);
			}
			context;
			say("AvPop%s",pop);


			context;
			/* Display 2nd half of day */
			say("");
			strcpy(tmp,"Type  12pm  1pm  2pm  3pm  4pm  5pm  6pm  7pm  8pm  9pm 10pm 11pm");
			context;
			if(thishr >= 12)
			{
				tmp[5 + (5*(thishr-12)) + 1] = ' ';
				tmp[5 + (5*(thishr-12)) + 2] = 'N';
				tmp[5 + (5*(thishr-12)) + 3] = 'O';
				tmp[5 + (5*(thishr-12)) + 4] = 'W';
			}
			context;
			say(tmp);
			say("----- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ---- ----");
			context;
			/* Display joins */
			context;
			joins_reg[0] = 0;
			context;
			for(i=12; i<24; i++)
				sprintf(joins_reg,"%s %4d",joins_reg,chan->joins_reg[i]);
			say("RJoin%s",joins_reg);

			/* Display joins */
			joins_nonreg[0] = 0;
			for(i=12; i<24; i++)
				sprintf(joins_nonreg,"%s %4d",joins_nonreg,chan->joins_nonreg[i]);
			say("NRJoi%s",joins_nonreg);

			/* Display public chatter */
			context;
			pub[0] = 0;
			context;
			for(i=12; i<24; i++)
				sprintf(pub,"%s %3luk",pub,chan->chatter[i] / 1024);
			say("Publc%s",pub);

			/* Display Avg Population */
			context;
			pop[0] = 0;
			context;
			for(i=12; i<24; i++)
			{
				if(i == thishr)
					sprintf(pop,"%s %4d",pop,(int)(chan->pop_prod / (time(NULL) - chan->joined + 1)));
				else
					sprintf(pop,"%s %4d",pop,chan->population[i]);
			}
			say("AvPop%s",pop);

			say("");
			timet_to_time_short(chan->max_population_time,maxpop_time);
			timet_to_time_short(chan->min_population_time,minpop_time);
			if(chan->max_population_time < time_today_began(time(NULL)))
				strcpy(yest," yesterday");
			else
				strcpy(yest,"");
			say("Max population: %d people at %s%s.",chan->max_population,maxpop_time,yest);
			if(chan->min_population_time < time_today_began(time(NULL)))
				strcpy(yest," yesterday");
			else
				strcpy(yest,"");
			say("Min population: %d people at %s%s.",chan->min_population,minpop_time,yest);
		}

		return;
	}

	context;
	if(chan->memberlist != NULL)
	{
		say("");
		say("Join  Idle @Nick      Userhost                            Levels");
		say("----- ---- ---------- ----------------------------------- -------");

		tot = Channel_orderbyjoin_htol(chan);

		/* Loop through and pull each record in order */
		for(i=1; i<=tot; i++)
		{
			m = Channel_memberbyorder(chan,i);
			if(m == NULL)
				continue;

			if(m->joined == 0L)
				strcpy(join,"?????");
			else
				timet_to_time_short(m->joined,join);
			if(m->last == 0L)
				strcpy(idle,"????");
			else
				timet_to_ago_short(m->last,idle);
			User_flags2str(m->user->flags,levels);
		
			if(m->flags & MEMBER_VOICE)
				strcpy(mflags,"+");
			if(m->flags & MEMBER_OPER)
				strcpy(mflags,"@");
			else
				strcpy(mflags," ");

			if(m->split)
			{
				timet_to_ago_short(m->split,spl);
				sprintf(uh,"<-- netsplit for %s",spl);
			}
			else
				strcpy(uh,m->uh);
		
			if(strlen(uh) > 35)
				uh[35] = 0;
			if(strlen(levels) > 7)
				levels[7] = 0;
			say("%5s %4s %1s%-9s %-35s %-7s",join,idle,mflags,m->nick,uh,levels);
		}
		say("----- ---- ---------- ----------------------------------- -------");
	}
}

void cmd_help()
{
	FILE	*f;
	char	*s, helpfile[256];

	sprintf(helpfile,"%s/%s",Grufti->admindir,Grufti->helpfile);
	if((f = fopen(helpfile,"r")) == NULL)
	{
		say("No helpfile.");
		Log_write(LOG_MAIN | LOG_ERROR,"*","No helpfile was found. (req'd by %s %s)",cmdinfo.from_n,cmdinfo.from_uh);
		return;
	}

	context;
	if(!cmdinfo.isparam)
	{
		find_topic(f,"standard");
		while((s = get_ftext(f)))
			say(s);
	}
	else
	{
		if(!find_topic(f,cmdinfo.tok1))
			sayf(0,0,"No help available for \"%s\".",cmdinfo.tok1);
		else
			while((s = get_ftext(f)))
				say(s);
	}

	context;
	fclose(f);
}

void cmd_showlocations()
{
	locarec	*l;
	int	num_cols = 2;
	int	col, row, num_rows;
	char	line[160], l_info[BUFSIZ];

	context;
	line[0] = 0;
	say("ID# City, State  Country      #   ID# City, State  Country      #");
	say("--- ------------------------ --   --- ------------------------ --");
	num_rows = Location->num_locations / num_cols;
	if(Location->num_locations % num_cols)
		num_rows++;

	context;
	for(row=1; row<=num_rows; row++)
	{
		for(col=1; col<=num_cols; col++)
		{
			l = Location_locationbyindex((num_rows * (col-1)) + row);
			if(l)
			{
				Location_ltostr(l,l_info);
				if(strlen(l_info) > 24)
					l_info[24] = 0;
				sprintf(line,"%s%3d %-24s %2d   ",line,l->id,l_info,l->numusers);
			}
		}
		line[strlen(line) - 3] = 0;
		say("%s",line);
		line[0] = 0;
	}
	say("--- ------------------------ --   --- ------------------------ --");
	say("If your location is not listed you may add it with 'addlocation'.");
}

void cmd_chlocation()
{
	locarec	*l;
	char	city[SERVERLEN], state[SERVERLEN], country[SERVERLEN];
	char	l_info[SERVERLEN];
	char	*p, *q;
	int	num;

	context;
	if(!cmdinfo.isparam)
	{
		say("Usage: chlocation <#> <city>, <state>, <country>");
		sayf(2,0,"Use this command to change location information.  City, State, and Country arguments must be separated by commas.  Please use abbreviations for state.  If state does not apply, simply omit.");
		return;
	}

	context;
	if(!isanumber(cmdinfo.tok1))
	{
		say("Usage: chlocation <#>");
		sayf(2,0,"You must use a predefined location number to change a location.  Type 'showlocations' to see which numbers you can use.");
		return;
	}

	num = atoi(cmdinfo.tok1);
	l = Location_location(num);
	if(l == NULL)
	{
		say("Usage: chlocation <#> <city>, <state>, <country>");
		sayf(2,0,"Location \"%d\" is not a valid location number.  Type 'showlocations' to see which location numbers you can use.",num);
		return;
	}

	/*
	 * Ok got a location.  Now see if they're allowed to change it.
	 * Users with +m can change it anytime, everyone else may only change
	 * it within a certain time after it was added.
	 */
	if(!(cmdinfo.user->flags & USER_MASTER))
	{
		/* did they add it? */
		if(!isequal(cmdinfo.user->acctname,l->addedby))
		{
			sayf(0,0,"Sorry, but you didn't add that location.  Locations can only be changed by the user who added them.");
			return;
		}

		/* they added it.  is it too late? */
		if((time(NULL) - l->time_added) > Grufti->location_changetime)
		{
			char	changetime[TIMESHORTLEN];
			time_to_elap_short(Grufti->location_changetime,changetime);
			sayf(0,0,"It's too late to change or delete that location!  If changes are needed they must be made within %s of adding them.  Sorry!",changetime);
			return;
		}
	}
	
	context;
	/* user is ok to change it */
	split(NULL,cmdinfo.param);
	p = strchr(cmdinfo.param,',');
	if(p == NULL)
	{
		say("Usage: chlocation <#> <city>, <state>, <country>");
		sayf(2,0,"Use this command to change location information.  City, State, and Country arguments must be separated by commas.  Please use abbreviations for state.  If state does not apply, simply omit.");
		return;
	}
		
	context;
	/* Got one comma.  Must have city. */
	*p = 0;
	rmspace(cmdinfo.param);
	strcpy(city,cmdinfo.param);

	q = strchr(p+1,',');
	if(p == NULL)
	{
		/* only one comma.  Next argument must be country. */
		rmspace(p+1);
		strcpy(country,p+1);
		strcpy(state,"");
	}
	else
	{
		/* 2 commas. */
		*q = 0;	
		rmspace(p+1);
		strcpy(state,p+1);
		rmspace(q+1);
		strcpy(country,q+1);
	}

	context;
	/* ok.  everything's quoted, and all the arguments are there. */
	locarec_setcity(l,city);
	locarec_setstate(l,state);
	locarec_setcountry(l,country);

	Location_ltostr(l,l_info);

	sayf(0,0,"Location info changed to \"%s\".  Thanks!",l_info);
	Location_sort();
	Location->flags |= LOCATION_NEEDS_WRITE;
}

void cmd_addlocation()
{
	locarec	*l, *lcheck;
	char	city[SERVERLEN], state[SERVERLEN], country[SERVERLEN];
	char	l_info[SERVERLEN], changetime[TIMESHORTLEN];
	char	*p, *q;


	context;
	if(!cmdinfo.isparam)
	{
		say("Usage: addlocation <city>, <state>, <country>");
		sayf(2,0,"Use this command to add a location.  City, State, and Country arguments must be separated by commas.  Please use abbreviations for state.  If state does not apply, simply omit.");
		return;
	}

	context;
	p = strchr(cmdinfo.param,',');
	if(p == NULL)
	{
		say("Usage: addlocation <city>, <state>, <country>");
		sayf(2,0,"Use this command to add a location.  City, State, and Country arguments must be separated by commas.  Please use abbreviations for state.  If state does not apply, simply omit.");
		return;
	}
		
	context;
	/* Got one comma.  Must have city. */
	*p = 0;
	rmspace(cmdinfo.param);
	strcpy(city,cmdinfo.param);

	q = strchr(p+1,',');
	if(q == NULL)
	{
		say("Usage: addlocation <city>, <state>, <country>");
		sayf(2,0,"Use this command to add a location.  City, State, and Country arguments must be separated by commas.  Please use abbreviations for state.  If state does not apply, simply omit.");
		return;
	}
	else
	{
		/* 2 commas. */
		*q = 0;	
		rmspace(p+1);
		strcpy(state,p+1);
		rmspace(q+1);
		strcpy(country,q+1);
	}

	context;
	/* ok. Now make sure it's not a duplicate */
	lcheck = Location_locationbyarea(city,state,country);
	if(lcheck != NULL)
	{
		Location_ltostr(lcheck,l_info);
		sayf(0,0,"The location \"%s\" is already set as #%d!",l_info,lcheck->id);
		return;
	}

	context;
	/* OK now we can add it */
	l = Location_addnewlocation(city,state,country,cmdinfo.user,cmdinfo.from_n);
	Location_ltostr(l,l_info);

	time_to_elap_short(Grufti->location_changetime,changetime);
	sayf(0,0,"The location \"%s\" has been added as #%d.",l_info,l->id);

	if(Grufti->send_notice_location_created)
		Note_sendnotification("The location \"%s\" has been added by %s.",l_info,cmdinfo.user->acctname);

	say("");
	sayf(0,0,"You may change or delete this location with 'dellocation' or 'chlocation'.  Changes must be done so by the person who added the location, and must be done within %s of adding it.",changetime);
	Location_sort();
	Location->flags |= LOCATION_NEEDS_WRITE;
}

void cmd_dellocation()
{
	locarec	*l;
	userrec	*u;
	char	l_info[SERVERLEN];
	int	num;

	context;
	/* no arguments */
	if(!cmdinfo.isparam)
	{
		say("Usage: dellocation <#>");
		sayf(2,0,"Use this command to delete a location.  Anyone who has their location set to this number will have their location erased.");
		return;
	}

	context;
	if(!isanumber(cmdinfo.tok1))
	{
		say("Usage: dellocation <#>");
		sayf(2,0,"You must use a predefined location number to delete a location.  Type 'showlocations' to see which numbers you can use.");
		return;
	}

	num = atoi(cmdinfo.tok1);
	l = Location_location(num);
	if(l == NULL)
	{
		say("Usage: dellocation <#>");
		sayf(2,0,"Location \"%d\" is not a valid location number.  Type 'showlocations' to see which location numbers you can use.",num);
		return;
	}

	context;
	/*
	 * Ok got a location.  Now see if they're allowed to delete it.
	 * Users with +m can delete it anytime, everyone else may only delete
	 * it within a certain time after it was added.
	 */
	if(!(cmdinfo.user->flags & USER_MASTER))
	{
		/* did they add it? */
		if(!isequal(cmdinfo.user->acctname,l->addedby))
		{
			sayf(0,0,"Sorry, but you didn't add that location.  Locations can only be changed or deleted by the user who added them.");
			return;
		}

		/* they added it.  is it too late? */
		if((time(NULL) - l->time_added) > Grufti->location_changetime)
		{
			char	changetime[TIMESHORTLEN];
			time_to_elap_short(Grufti->location_changetime,changetime);
			sayf(0,0,"It's too late to change or delete that location!  If changes are needed they must be made within %s of adding it.  Sorry!",changetime);
			return;
		}
	}
	
	context;
	Location_ltostr(l,l_info);
	sayf(0,0,"The location \"%s\" has been deleted.",l_info);

	if(Grufti->send_notice_location_deleted)
		Note_sendnotification("The location \"%s\" has been deleted by %s.",l_info,cmdinfo.user->acctname);

	/* ok.  Delete the record. */
	Location_deletelocation(l);

	context;
	/* now go through the whole userlist, and erase anyone with that # */
	for(u=User->userlist; u!=NULL; u=u->next)
	{
		if(u->location == num)
		{
			u->location = 0;

			/* Tell that user their location was deleted */
			Note_sendnotice(u,"The location \"%s\" was deleted from the locations list.  As a result, your location info was cleared since it matched.  Sorry!",l_info);
		}
	}
		
	Location->flags |= LOCATION_NEEDS_WRITE;
}

void cmd_finger()
{
	userrec *u;

	context;
	if(!cmdinfo.isparam || isequal(cmdinfo.tok1,"-a"))
	{
		cmd_finger_showallonline();
		return;
	}

	if(strchr(cmdinfo.tok1,'@'))
	{
		cmd_seen_userhost();
		return;
	}

	context;
	u = User_user(cmdinfo.tok1);
	if(u->registered)
		cmd_finger_showregistered(u);
	else
		say("The nick \"%s\" does not belong to a registered user.",cmdinfo.tok1);
}

void cmd_finger_showallonline()
/*
 * This routine loads up the 'genericlist' in notify.c.  Since the entries which
 * are displayed here span across multiple lists (ie. nickrec, userlist,
 * dcclist), trying to find some way to order the lists would be a major hassle.
 * So 'genericlist' was written so that it could be loaded up with the
 * entries that would be displayed here and the login time.  Then we can use
 * this list to sort the entries.  Much easier.  Up to this point we have been
 * careful not to allocate any more memory than is needed.  This means for
 * all other sort routines, no further memory was allocated.  We make the
 * exception for this command just because of the complexity in trying to
 * order the entries across multiple lists.
 */
{
	userrec	*u;
	nickrec	*n;
	dccrec	*d;
	generec	*g;
	char	finger[100], via[10], on[10];
	char	login[TIMESHORTLEN], last[TIMESHORTLEN], display[160];
	int	showall = 0, tot, i;


	/* Show all DCC's and telnets as well if -a */
	if(isequal(cmdinfo.tok1,"-a"))
		showall = 1;

	context;
	for(u=User->userlist; u!=NULL; u=u->next)
	{
		if(u->registered)
		{
			for(n=u->nicks; n!=NULL; n=n->next)
			{
				if((n->flags & NICK_ONIRC) || (n->flags & NICK_VIADCC) || (n->flags & NICK_VIATELNET))

				{

/* Begin TAB - 4 */


	context;
	/* User is on somewhere.  Finger info is global.  Setup parameters
	 * for different ON types.  DCC will require iterating through dcc
	 * to find them all, they might be on via more than one time with
	 * their host.
	 */
	/* Set finger */
	if(u->finger)
		strcpy(finger,u->finger);
	else
		strcpy(finger,"");

	if(strlen(finger) > 34)
		strcpy(&finger[34 - 3],"...");

	context;
	/* check online */
	if(n->flags & NICK_ONIRC)
	{
		strcpy(via,"IRC");
		if(Channel_ismemberofanychan(n->nick))
		{
			strcpy(on,"chan");
			timet_to_ago_short(n->account->last_seen,last);
		}
		else
		{
			strcpy(on,"????");
			strcpy(last,"<on>");
		}
			
		timet_to_elap_short(n->joined,login);

		/* display (Don't say, add to 'genericlist')*/
		sprintf(display,"%3s %4s %-9s %-34s %6s %4s",via,on,n->nick,finger,login,last);
		Notify_addtogenericlist(display,n->joined);
	}


	context;
	if(!showall)
		continue;

	context;
	/* Check DCC records */
	for(d=DCC->dcclist; d!=NULL; d=d->next)
	{
		if((d->flags & CLIENT_IRC_CHAT) || (d->flags & CLIENT_TELNET_CHAT) || (d->flags & CLIENT_XFER))
		{
			if(isequal(d->nick,n->nick) && d->account->registered)
			{
				if(d->flags & STAT_WEBCLIENT)
					sprintf(on,"W%d",d->socket);
				else if(d->flags & STAT_GATEWAYCLIENT)
					sprintf(on,"G%d",d->socket);
				else if(d->flags & CLIENT_TELNET_CHAT)
					sprintf(on,"T%d",d->socket);
				else if(d->flags & CLIENT_IRC_CHAT)
					sprintf(on,"I%d",d->socket);
				else if(d->flags & CLIENT_XFER)
					sprintf(on,"X%d",d->socket);
				timet_to_elap_short(d->starttime,login);
				timet_to_ago_short(d->last,last);

				if(d->flags & CLIENT_TELNET_CHAT)
					strcpy(via,"TEL");
				else
					strcpy(via,"DCC");

				/* display (Don't say, add to 'genericlist') */
				sprintf(display,"%3s %4s %-9s %-34s %6s %4s",via,on,n->nick,finger,login,last);
				Notify_addtogenericlist(display,d->starttime);
			}
		}
	}

	


/* END TAB - 4 */
				}
			}
		}
	}

	context;
	/* Ok, now we display the genericlist, in its sorted order */
	tot = Notify_orderbylogin();

	if(tot == 0)
	{
		if(showall)
			say("No users are online or connected to the bot.");
		else
			say("No users are online.");
		return;
	}

	context;
	say("Via On   User      Real Name                          Login  Idle");
	say("--- ---- --------- ---------------------------------- ------ ----");
	for(i=1; i<=tot; i++)
	{
		g = Notify_genericbyorder(i);
		if(g == NULL)
			continue;

		say(g->display);
	}
	say("--- ---- --------- ---------------------------------- ------ ----");

	/*
	 * Now, clear the list, or it will continue to build each time this
	 * command is called!
	 */
	context;
	Notify_freeallgenerics();
}
			

	
void cmd_finger_showregistered(userrec *u)
{
	char	display[BUFSIZ], week[25], hrsperday[25], ircfreq[25];
	char	pre[BUFSIZ], tmp[BUFSIZ], created[TIMESHORTLEN], notes[25];
	char	levels[FLAGSHORTLEN], signoff[SERVERLEN];
	int	highest_plan, gotplan = 0, x, new;
	u_long	tot_time;
	float	hrs;

	say("=================================================================");
	if(u->finger)
		strcpy(display,u->finger);
	else if(u == cmdinfo.user)
		strcpy(display,"(Use 'setfinger' to set this line...)");
	else
		strcpy(display,"");

	/* Display acctname and finger */
	say("[%d] %s \"%s\"",u->registered,u->acctname,display);
	say("");

	context;
	/* Display nicks */
	context;
	User_makenicklist(display,u);
	context;
	say(" %-9s %s","(nicks)",display);

	context;
	/* Display email */
	if(u->email)
		strcpy(display,u->email);
	else
		strcpy(display,"<not set>");
	say(" %-9s %s","(email)",display);

	context;
	/* Display WWW */
	if(u->www)
	{
		strcpy(display,u->www);
		say(" %-9s %s","(www)",display);
	}

	context;
	/* Display bday */
	if(u->bday[0])
	{
		char hor[25];

		bday_plus_hor(display,hor,u->bday[0],u->bday[1],u->bday[2]);
		say(" %-9s %s (%s)","(bday)",display,hor);
	}

	context;
	/* Display other if we have them */
	if(u->other1)
	{
		if(strchr(u->other1,' '))
		{
			strcpy(tmp,u->other1);
			split(pre,tmp);
			if(strlen(pre) > 7)
				pre[7] = 0;
			strcat(pre,")");
			sprintf(display," (%-8s %s",pre,tmp);
		}
		else
			sprintf(display," (%-8s %s","other)",u->other1);

		say("%s",display);
	}

	context;
	if(u->other2)
	{
		if(strchr(u->other2,' '))
		{
			strcpy(tmp,u->other2);
			split(pre,tmp);
			if(strlen(pre) > 7)
				pre[7] = 0;
			strcat(pre,")");
			sprintf(display," (%-8s %s",pre,tmp);
		}
		else
			sprintf(display," (%-8s %s","other)",u->other2);

		say("%s",display);
	}
	if(u->other3)
	{
		if(strchr(u->other3,' '))
		{
			strcpy(tmp,u->other3);
			split(pre,tmp);
			if(strlen(pre) > 7)
				pre[7] = 0;
			strcat(pre,")");
			sprintf(display," (%-8s %s",pre,tmp);
		}
		else
			sprintf(display," (%-8s %s","other)",u->other3);

		say("%s",display);
	}

	if(u->other4)
	{
		if(strchr(u->other4,' '))
		{
			strcpy(tmp,u->other4);
			split(pre,tmp);
			if(strlen(pre) > 7)
				pre[7] = 0;
			strcat(pre,")");
			sprintf(display," (%-8s %s",pre,tmp);
		}
		else
			sprintf(display," (%-8s %s","other)",u->other4);

		say("%s",display);
	}

	if(u->other5)
	{
		if(strchr(u->other5,' '))
		{
			strcpy(tmp,u->other5);
			split(pre,tmp);
			if(strlen(pre) > 7)
				pre[7] = 0;
			strcat(pre,")");
			sprintf(display," (%-8s %s",pre,tmp);
		}
		else
			sprintf(display," (%-8s %s","other)",u->other5);

		say("%s",display);
	}

	if(u->other6)
	{
		if(strchr(u->other6,' '))
		{
			strcpy(tmp,u->other6);
			split(pre,tmp);
			if(strlen(pre) > 7)
				pre[7] = 0;
			strcat(pre,")");
			sprintf(display," (%-8s %s",pre,tmp);
		}
		else
			sprintf(display," (%-8s %s","other)",u->other6);

		say("%s",display);
	}

	if(u->other7)
	{
		if(strchr(u->other7,' '))
		{
			strcpy(tmp,u->other7);
			split(pre,tmp);
			if(strlen(pre) > 7)
				pre[7] = 0;
			strcat(pre,")");
			sprintf(display," (%-8s %s",pre,tmp);
		}
		else
			sprintf(display," (%-8s %s","other)",u->other7);

		say("%s",display);
	}

	if(u->other8)
	{
		if(strchr(u->other8,' '))
		{
			strcpy(tmp,u->other8);
			split(pre,tmp);
			if(strlen(pre) > 7)
				pre[7] = 0;
			strcat(pre,")");
			sprintf(display," (%-8s %s",pre,tmp);
		}
		else
			sprintf(display," (%-8s %s","other)",u->other8);

		say("%s",display);
	}

	if(u->other9)
	{
		if(strchr(u->other9,' '))
		{
			strcpy(tmp,u->other9);
			split(pre,tmp);
			if(strlen(pre) > 7)
				pre[7] = 0;
			strcat(pre,")");
			sprintf(display," (%-8s %s",pre,tmp);
		}
		else
			sprintf(display," (%-8s %s","other)",u->other9);

		say("%s",display);
	}

	if(u->other10)
	{
		if(strchr(u->other10,' '))
		{
			strcpy(tmp,u->other10);
			split(pre,tmp);
			if(strlen(pre) > 7)
				pre[7] = 0;
			strcat(pre,")");
			sprintf(display," (%-8s %s",pre,tmp);
		}
		else
			sprintf(display," (%-8s %s","other)",u->other10);

		say("%s",display);
	}



	context;
	/* Display location */
	Location_idtostr(u->location,display);
	if(display[0])
		say(" %-9s %s","(locat)",display);

	/* Display last login */
	timet_to_date_long(u->last_login, display);
	say(" %-9s %s","(login)", display);

	/* Display last seen */
	x = cmd_finger_helper_makelast(u,display);

	if(x == 11)
		sprintf(display,"Over %d days ago",Grufti->timeout_accounts);

	if(x == 9 || x == 10 || x == 11)
	{
		if(u->signoff)
			sprintf(signoff," (%s)",u->signoff);
		else
			strcpy(signoff,"");
		say(" %-9s %s%s","(seen)",display,signoff);
	}
	else
		say(" %-9s %s","(online)",display);

	say("");


	context;
	/* Display plan */
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
			say("%s",u->plan1);
		else
			say("");
	}

	if(u->plan2 || (gotplan && highest_plan >= 2))
	{
		gotplan = 2;
		if(u->plan2)
			say("%s",u->plan2);
		else
			say("");
	}

	if(u->plan3 || (gotplan && highest_plan >= 3))
	{
		gotplan = 3;
		if(u->plan3)
			say("%s",u->plan3);
		else
			say("");
	}

	if(u->plan4 || (gotplan && highest_plan >= 4))
	{
		gotplan = 4;
		if(u->plan4)
			say("%s",u->plan4);
		else
			say("");
	}

	if(u->plan5 || (gotplan && highest_plan >= 5))
	{
		gotplan = 5;
		if(u->plan5)
			say("%s",u->plan5);
		else
			say("");
	}

	if(u->plan6 || (gotplan && highest_plan >= 6))
	{
		gotplan = 6;
		if(u->plan6)
			say("%s",u->plan6);
		else
			say("");
	}

	if(!highest_plan)
	{
		say("");
		sayf(-1,-1,"-= No plan info set =-");
		say("");
	}
	else if(highest_plan < 6)
		say("");

	say("");

	context;
	/* Make dates */
	User_makeweekdates(week);

	/* Calculate avg hours / week */
	tot_time = User_timeperweek(u);
	if(tot_time < 2520 && tot_time > 0)
		tot_time = 2520;
	hrs = (float)tot_time / 25200.0;

	/* Make when registered */
	timet_to_dateonly_short(u->time_registered,created);

	/* Make totals for each day */
	User_makeweeksusage(hrsperday,u);

	/* Calculate ircfreq level */
	User_makeircfreq(ircfreq,u);

	/* Make levels string */
	User_flags2str(u->flags,levels);

	/* Make Notes string */
	new = Note_numnew(u);
	if(u->forward && u->forward[0])
		strcpy(notes,"Fwd");
	else if(new)
		sprintf(notes,"%d/%d",new,u->num_notes);
	else
		sprintf(notes,"%d",u->num_notes);
	say("%s H/Day Usage   Created    Notes Levels",week);
	say("%s %5.1f %-7s %-10s %5s +%s",hrsperday,hrs,ircfreq,created,notes,levels);

	say("=================================================================");
}

int cmd_finger_helper_makelast(userrec *u, char *out)
{
	char	when[TIMELONGLEN];
	acctrec	*a = NULL;
	int	x;

	x = Response_haveyouseen(u,u->acctname,&a,when);

	out[0] = 0;
	switch(x)
	{
		case 3 :
			sprintf(out,"on IRC as %s",a->nick);
			break;
		case 4 :
			sprintf(out,"on IRC as %s",a->nick);
			break;
		case 5 :
			sprintf(out,"via DCC as %s",a->nick);
			break;
		case 6 :
			sprintf(out,"via DCC as %s",a->nick);
			break;
		case 7 :
			sprintf(out,"via telnet as %s",a->nick);
			break;
		case 8 :
			sprintf(out,"via telnet as %s",a->nick);
			break;
		case 9 :
			sprintf(out,"%s ago",when);
			break;
		case 10 :
			sprintf(out,"%s ago as %s",when,a->nick);
			break;
	}

	return x;
}


void cmd_whois()
{
	userrec *u;
	acctrec	*a;
	char	display[BUFSIZ], uh[UHOSTLEN], reg[5], first[DATESHORTLEN];
	char	last[DATESHORTLEN], on[5];
	int	tot, i;

	if(!cmdinfo.isparam)
	{
		say("Usage: whois <nick | user@host>");
		return;
	}

	if(strchr(cmdinfo.tok1,'@'))
	{
		cmd_seen_userhost();
		return;
	}

	context;
	u = User_user(cmdinfo.tok1);
	
	/* Display acctname and nicks */
	if(u->registered)
	{
		User_makenicklist(display,u);
		say("[%d] %s (%s)",u->registered,u->acctname,display);
	}
	else
	{
		say("%s is not a registered nick.",cmdinfo.tok1);
	}
	say("");

	context;

	tot = User_orderaccountsbyfirstseen(u);

	/* Display accounts */
	say("   Nick      Userhost                          R First      Last");
	say("   --------- --------------------------------- - ---------- ----");
	if(tot == 0)
		say("No accounts.");
	for(i=1; i<=tot; i++)
	{
		a = User_accountbyorder(u,i);

		if(!u->registered && !isequal(cmdinfo.tok1,a->nick))
			continue;

		strcpy(uh,a->uh);
		if(strlen(uh) > 33)
			uh[33] = 0;
		if(a->registered)
			strcpy(reg,"R");
		else
			strcpy(reg,"-");
		timet_to_dateonly_short(a->first_seen,first);
		timet_to_ago_short(a->last_seen,last);
		on[0] = 0;
		if(a->flags & ACCT_VIADCC)
			strcat(on,"*");
		if(a->flags & ACCT_VIATELNET)
			strcat(on,"&");
		if(a->flags & ACCT_ONIRC)
			strcat(on,"+");
		say("%3s%-9s %-33s %s %-10s %4s",on,a->nick,uh,reg,first,last);
	}
	say("   --------- --------------------------------- - ---------- ----");
	if(u->registered)
		say("(Type 'finger %s' to display account information)",u->acctname);
}
	
void cmd_adduser()
{
	userrec	*ucheck, *newuser;
	char	extra[BUFSIZ], newpass[40], msg[80];

	context;
	/* No arguments */
	if(!cmdinfo.isparam)
	{
		say("Usage: adduser <acctname> <password>");
		sayf(0,0,"Use this command to create a new registered account.  Once the account is created, you can use 'chnicks', 'chfinger' etc to update the account.");
		return;
	}

	context;
	if(!cmdinfo.istok2)
	{
		say("Usage: adduser <acctname> <password>");
		sayf(0,0,"Please specify a password for the account.");
		return;
	}

	context;
	/* Verify account name */
	if(invalidacctname(cmdinfo.tok1))
	{
		sayf(0,0,"The name \"%s\" is invalid.  Account names are between 2 and 9 characters in length and must be valid IRC nicks.",cmdinfo.tok1);
		return;
	}

	context;
	/* Account name is valid.  Check against database. */
	ucheck = User_user(cmdinfo.tok1);
	if(ucheck->registered)
	{
		sayf(0,0,"The name \"%s\" is already being used by %s.",cmdinfo.tok1,ucheck->acctname);
		return;
	}

	context;
	/* Check password */
	if(invalidpassword(cmdinfo.tok2))
	{
		sayf(0,0,"Passwords are between 3 and 9 characters in length.");
		return;
	}

	/*
	 * Account name is valid and not in use.  Create new record.
	 * After creation, we must reset all user and account pointers to
	 * reflect the new changes.
	 */
	context;
	newuser = User_addnewuser(cmdinfo.tok1);

	/* Move all accounts with that nick from Luser into the new record */
	User_moveaccounts_into_newhome(newuser,cmdinfo.tok1);

	context;
	/* ok password is good.  update it. */
	encrypt_pass(cmdinfo.tok2,newpass);
	userrec_setpass(newuser,newpass);

	context;
	/* Reset all user, account pointers in Channel, DCC, and Command. */
	User_resetallpointers();

	Log_write(LOG_MAIN,"*","New account \"%s\" created by %s (%s).",cmdinfo.tok1,cmdinfo.from_n,cmdinfo.from_uh);

	/* First user registered is assigned +m */
	if(newuser->registered == 1)
	{
		newuser->flags |= USER_MASTER;
		sprintf(extra," (and was given +m)");
	}
	else
		extra[0] = 0;

	say("Account \"%s\" has been created.%s",newuser->acctname,extra);

	context;
	/* Introduce user to their new account */
	Note_sendnotice(newuser,"Welcome to your new account %s!\n\nType 'help' for major topics and commands.  If you are new to %s or bots in general, check out a brief tutorial by typing 'help tutorial'.  Help can be found for every command by using the '-h' switch.  View a list of all commands with 'cmdinfo.'.\n\nFinally, see how you can set some information for your account by typing 'help account'.",newuser->acctname,Grufti->botnick);

	if(Grufti->send_notice_user_created)
		Note_sendnotification("%s added the user \"%s\".",cmdinfo.user->acctname,newuser->acctname);

	sprintf(msg,"Account created by %s.", cmdinfo.user->acctname);
	User_addhistory(newuser, "system", time(NULL), msg);
}

void cmd_notify()
{
	/* Displays all the nicks which are online */
	context;
	cmd_finger_showallonline();
}

void cmd_deluser()
{
	userrec *u;
	context;
	if(!cmdinfo.isparam)
	{
		say("Usage: deluser <acctname>");
		return;	
	}

	context;
	u = User_user(cmdinfo.tok1);
	if(!u->registered)
	{
		sayf(0,0,"The nick \"%s\" is not registered.",cmdinfo.tok1);
		return;
	}

	if(u->flags & USER_OWNER)
	{
		sayf(0,0,"You are trying to delete the owner's account, which can not be permitted.");
		return;
	}

	context;
	say("The user \"%s\" has been deleted.",u->acctname);

	if(Grufti->send_notice_user_deleted)
		Note_sendnotification("%s has deleted the user \"%s\".",cmdinfo.user->acctname,u->acctname);

	User_removeuser(u);
	context;
	User_resetallpointers();
}

void cmd_seen()
{
	char	when[TIMELONGLEN], lookup[SERVERLEN], signoff[SERVERLEN];
	userrec	*u;
	acctrec	*a = NULL;
	nickrec	*n;
	int	x;

	context;
	if(!cmdinfo.isparam)
	{
		say("Usage: seen <nick | user@host>");
		sayf(2,0,"Use this command to determine when someone was last on IRC.");
		return;
	}

	context;

	if(strchr(cmdinfo.tok1,'@'))
	{
		cmd_seen_userhost();
		return;
	}

	/* Copy and truncate, if necessary */
	strcpy(lookup,cmdinfo.tok1);
	if(strlen(lookup) > NICKLEN)
		lookup[NICKLEN] = 0;

	context;
	/* Convert lookup case to actual nick case */
	u = User_user(lookup);
	if(u->registered)
	{
		n = User_nick(u,lookup);
		if(n != NULL)
			strcpy(lookup,n->nick);
	}

	context;
	x = Response_haveyouseen(u,lookup,&a,when);
	if(x == 2 || x == 9 || x == 10)
	{
		if(u->signoff)
			sprintf(signoff," (%s)",u->signoff);
		else
			strcpy(signoff,"");
	}

	switch(x)
	{
	case 0 :
		sayf(0,0,"No such user \"%s\".",lookup);
		break;

	case 1 :
		sayf(0,0,"I see %s (%s) on IRC.",a->nick,a->uh);
		break;

	case 2 :
		sayf(0,0,"Last saw %s (%s) %s ago.%s",a->nick,a->uh,when,signoff);
		break;

	case 3 :
		sayf(0,0,"I see %s (%s) on IRC.",a->nick,a->uh);
		break;

	case 4 :
		sayf(0,0,"I see %s on IRC as %s (%s).",lookup,a->nick,a->uh);
		break;

	case 5 :
		sayf(0,0,"I see %s (%s) connected via DCC.",a->nick,a->uh);
		break;

	case 6 :
		sayf(0,0,"I see %s connected via DCC as %s (%s).",lookup,a->nick,a->uh);
		break;

	case 7 :
		sayf(0,0,"I see %s (%s) connected via telnet.",a->nick,a->uh);
		break;

	case 8 :
		sayf(0,0,"I see %s connected via telnet as %s (%s).",a->nick,a->uh);
		break;

	case 9 :
		sayf(0,0,"Last saw %s (%s) %s ago.%s",a->nick,a->uh,when,signoff);
		break;

	case 10 :
		sayf(0,0,"Last saw %s as %s (%s) %s ago.%s",lookup,a->nick,a->uh,when,signoff);
		break;

	case 11 :
		sayf(0,0,"Last saw %s over %d days ago.",lookup,Grufti->timeout_accounts);
		break;
	}

}

#define MAX_SEENUSERHOST_HITS 50

void cmd_seen_userhost()
{
	userrec	*u;
	acctrec	*a;
	generec	*g;
	char	display[BUFSIZ], uh[UHOSTLEN], reg[5], first[DATESHORTLEN];
	char	last[DATESHORTLEN], on[5], lookup[UHOSTLEN], against[UHOSTLEN];
	int	tot, i, counter = 0;

	if(strchr(cmdinfo.tok1,'!'))
		sprintf(lookup,"%s",cmdinfo.tok1);
	else
		sprintf(lookup,"*!%s",cmdinfo.tok1);

	if(!Banlist_verifyformat(lookup))
	{
		sayf(0,0,"Invalid match format \"%s\".",lookup);
		say("Use the format 'nick!user@hostname'.");
		return;
	}

	for(u=User->userlist; u!=NULL && counter<=MAX_SEENUSERHOST_HITS ; u=u->next)
	{
		for(a=u->accounts; a!=NULL && counter<=MAX_SEENUSERHOST_HITS; a=a->next)
		{
			sprintf(against,"%s!%s",a->nick,a->uh);
			if(wild_match(lookup,against) || wild_match(against,lookup))
			{
				strcpy(uh,a->uh);
				if(strlen(uh) > 33)
					uh[33] = 0;
				if(a->registered)
					strcpy(reg,"R");
				else
					strcpy(reg," ");
				timet_to_dateonly_short(a->first_seen,first);
				timet_to_ago_short(a->last_seen,last);
				on[0] = 0;
				if(a->flags & ACCT_VIADCC)
					strcat(on,"*");
				if(a->flags & ACCT_VIATELNET)
					strcat(on,"&");
				if(a->flags & ACCT_ONIRC)
					strcat(on,"+");
				sprintf(display,"%3s%-9s %-33s %s %-10s %4s",on,a->nick,uh,reg,first,last);
				Notify_addtogenericlist(display,a->last_seen);

				counter++;
			}
		}
	}

	tot = Notify_orderbylogin();

	if(tot == 0)
	{
		say("No one matches the mask \"%s\".",lookup);
		return;
	}

	say("   Nick      Userhost                          R First      Last");
	say("   --------- --------------------------------- - ---------- ----");
	for(i=1; i<=tot; i++)
	{
		g = Notify_genericbyorder(i);
		if(g == NULL)
			continue;

		say(g->display);
	}
	say("   --------- --------------------------------- - ---------- ----");
	if(counter >= MAX_SEENUSERHOST_HITS)
		say("   (Output is limited to %d hits.)",MAX_SEENUSERHOST_HITS);

	/* Don't forget */
	Notify_freeallgenerics();
}
