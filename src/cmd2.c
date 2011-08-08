/******************************************************************************
 * Steve Rider - stever@area23.org
 * Grufti - an interactive, service-oriented bot for IRC 
 *
 * cmd2.c - the command declarations
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

void cmd_write()
{
	int	all = 0, wrotesomething = 0;

	context;
	if(!cmdinfo.isparam)
	{
		say("Usage: write <file | all>");
		sayf(2,0,"Use this command to write bot files to disk.  Specify 'all' to write all files.  Valid files are: userlist, response, locations, events, webcatch, banlist, or stats.");
		return;
	}

	if(isequal(cmdinfo.tok1,"all"))
		all = 1;

	context;
	if(isequal(cmdinfo.tok1,"userlist") || all)
	{
		wrotesomething ++;

		if(User->userlist_active)
		{
			say("Writing \"%s\"...",Grufti->userfile);
			User_writeuserfile();
			say("Wrote \"%s\". (%d users)",Grufti->userfile,User->num_users);
		}
		else
			say("Userlist is not active.  Ignoring write.");
	}

	context;
	if(isequal(cmdinfo.tok1,"locations") || all)
	{
		say("Writing \"%s\"...",Grufti->locafile);
		Location_writelocations();
		say("Wrote \"%s\". (%d locations)",Grufti->locafile,Location->num_locations);
		Location->flags &= ~LOCATION_NEEDS_WRITE;
		wrotesomething ++;
	}

	if(isequal(cmdinfo.tok1,"events") || all)
	{
		say("Writing \"%s\"...",Grufti->eventfile);
		Event_writeevents();
		say("Wrote \"%s\". (%d events)",Grufti->eventfile,Event->num_events);
		Event->flags &= ~EVENT_NEEDS_WRITE;
		wrotesomething++;
	}

	if(isequal(cmdinfo.tok1,"webcatch") || all)
	{
		say("Writing \"%s\"...",Grufti->webfile);
		URL_writeurls();
		say("Wrote \"%s\". (%d urls)",Grufti->webfile,URL->num_urls);
		URL->flags &= ~URL_NEEDS_WRITE;
		wrotesomething ++;
	}

	context;
	if(isequal(cmdinfo.tok1,"response") || all)
	{
		wrotesomething ++;

		if(Response->flags & RESPONSE_ACTIVE)
		{
			say("Writing \"%s\"...",Grufti->respfile);
			Response_writeresponses();
			say("Wrote \"%s\". (%d responses)",Grufti->respfile,Response_numresponses());
			Response->flags &= ~RESPONSE_NEEDS_WRITE;
		}
		else
			say("Response list is not active.  Ignoring write request.");
		
	}

	if(isequal(cmdinfo.tok1,"banlist") || all)
	{
		wrotesomething ++;

		if(Banlist->flags & BANLIST_ACTIVE)
		{
			say("Writing \"%s\"...",Grufti->banlist);
			Banlist_writebans();
			say("Wrote \"%s\". (%d bans)",Grufti->banlist,Banlist->num_bans);
			Banlist->flags &= ~BANLIST_NEEDS_WRITE;
		}
		else
			say("Ban list is not active.  Ignoring write request.");
		
	}

	if(isequal(cmdinfo.tok1,"stats") || all)
	{
		wrotesomething ++;

		if(Grufti->stats_active)
		{
			say("Writing \"%s\"...",Grufti->statsfile);
			writestats();
			say("Wrote \"%s\". (%d channels)",Grufti->statsfile,Channel->num_channels);
		}
		else
			say("Stats list is not active.  Ignoring write request.");
		
	}

	if(!wrotesomething)
	{
		sayf(0,0,"Don't know how to write \"%s\".",cmdinfo.tok1);
		sayf(0,0,"Valid files are: userlist, response, locations, events, webcatch, banlist, or stats.");
		return;
	}
}

void cmd_addthisnick()
{
	userrec	*ucheck, *unickcheck;
	char	cipher[PASSWORD_CIPHER_LEN];

	context;
	if(!cmdinfo.isparam)
	{
		say("Usage: addthisnick <acctname> <password>");
		sayf(2,0,"Use this command to add the nick you're currently using to the specified account.  Good for when you need access to your account and you can't use any of your nicks.");
		return;
	}

	context;
	/* Check if proper # parameters */
	if(cmdinfo.num_elements != 2)
	{
		say("Usage: addthisnick <acctname> <password>");
		sayf(2,0,"Invalid number of parameters specified.");
		return;
	}

	context;
	/* Verify account is legit */
	ucheck = User_user(cmdinfo.tok1);
	if(!ucheck->registered)
	{
		sayf(0,0,"No such account \"%s\".",cmdinfo.tok1);
		return;
	}

	context;
	/* Accountname is valid.  See if the password is correct */
	if(ucheck->pass == NULL)
	{
		sayf(0,0,"That account has no password set.  It must be set by the connection which created it.  Sorry!");
		return;
	}

	context;
	/* Check if they're over the limit. */
	if(ucheck->num_nicks >= Grufti->max_nicks)
	{
		sayf(0,0,"You are allowed %d nicks for this account and you've set up all %d!  The best way for you to gain access to your account at this point would be to telnet to the bot.  Type 'telnet' to view available telnet ports.",Grufti->max_nicks,Grufti->max_nicks);
		return;
	}

	context;
	/* Check password */
	encrypt_pass(cmdinfo.tok2,cipher);
	if(!isequal(ucheck->pass,cipher))
	{
		say("Wrong password for %s.  Go away.",ucheck->acctname);
		return;
	}

	context;
	/* nick is valid.  Check against database. */
	unickcheck = User_user(cmdinfo.from_n);
	if(unickcheck->registered)
	{
		sayf(0,0,"Sorry, the nick \"%s\" is already registered to %s.",cmdinfo.from_n,ucheck->acctname);
		return;
	}

	/*
	 * At this point, the user and password are good.  We need to treat
	 * this sort of like a new user registering, since we need to adjust
	 * the cmdinfo.user to reflect the new changes, etc.
	 */

	context;
	/* Nick is ok and not in use.  Add it. */
	User_addnewnick(ucheck,cmdinfo.from_n);

	say("The nick \"%s\" has been added to your account.  Thanks!",cmdinfo.from_n);

	context;
	/* Update all accounts (move from luser to user) */
	User_moveaccounts_into_newhome(ucheck,cmdinfo.from_n);

	/*
	 * Reset all user, account pointers in Channel, DCC, and Command.
	 * The cmdinfo.user that we're using right now should reflect the
	 * new changes.
	 */
	context;
	User_resetallpointers();

	Log_write(LOG_MAIN,"*","Nick %s added to user %s.",cmdinfo.from_n,cmdinfo.user->acctname);

	context;
	/* Update other accounts and mark them R if any match */
	if(!cmdinfo.account->registered)
	{
		sayf(0,0,"Adding \"%s\" to your account.",cmdinfo.account->uh);
		Log_write(LOG_MAIN,"*","Added \"%s\" to %s account.",cmdinfo.account->uh,ucheck->acctname);
		cmdinfo.account->registered = TRUE;

		/* Mark all accounts matching this uh as registered */
		User_updateregaccounts(ucheck,cmdinfo.account->uh);
	}

	context;
	User_checkforsignon(cmdinfo.from_n);

	User_gotpossiblesignon(cmdinfo.user,cmdinfo.account);
}

void cmd_wallnote()
{
	int i, tot;
	u_long flags;
	char levels[80], newmsg[STDBUF];
	userrec	*u_recp;

	if(!cmdinfo.isparam)
	{
		say("Usage: wallnote <levels> <msg>");
		sayf(2,0, "Use this command to send a note to multiple recipients, as specified by the levels.  For example, to send a note to all users with +o, but not those with +3, you would type: wallnote +o-3 Hey folks, bla bla bla.  To see which users match certain levels, use the 'user' command.");
		return;
	}

	if(cmdinfo.num_elements < 2)
	{
		say("Usage: wallnote <levels> <msg>");
		sayf(2,0,"No msg was specified!");
		return;
	}

	User_str2flags(cmdinfo.tok1,&flags);
	tot = User_orderbyacctname_plusflags(flags);
	if(tot == 0)
	{
		say("There are no recipients which match the flags: %s", cmdinfo.tok1);
		return;
	}

	User_flags2str(flags, levels);
	split(NULL,cmdinfo.param);
	sprintf(newmsg, "%s %s", levels, cmdinfo.param);

	for(i = 1; i <= tot; i++)
	{
		u_recp = User_userbyorder(i);
		User_addnewnote(u_recp,cmdinfo.user,cmdinfo.account,newmsg,NOTE_NEW | NOTE_WALL);
		User_informuserofnote(u_recp, cmdinfo.account);
	}

	say("Sent Note to %d user%s.", tot, tot == 1 ? "" : "s");
}
	


void cmd_note()
{
	userrec	*u_recp;

	context;
	if(!cmdinfo.isparam)
	{
		say("Usage: note <nick> <msg>");
		sayf(2,0,"Use this command to send a quick Note to another user.  Recipients must be registered users.");
		return;
	}

	context;
	if(cmdinfo.num_elements < 2)
	{
		say("Usage: note <nick> <msg>");
		sayf(2,0,"No msg was specified!");
		return;
	}

	context;
	if(!User_isknown(cmdinfo.tok1))
	{
		sayf(0,0,"The nick \"%s\" is not known to me.",cmdinfo.tok1);
		return;
	}

	context;
	u_recp = User_user(cmdinfo.tok1);
	if(!u_recp->registered)
	{
		sayf(0,0,"The user \"%s\" is not registered.  Notes can only be sent to registered users.  Sorry!",cmdinfo.tok1);
		return;
	}

	context;
	/* Recipient is ok.  Send note. */
	split(NULL,cmdinfo.param);
	User_addnewnote(u_recp,cmdinfo.user,cmdinfo.account,cmdinfo.param,NOTE_NEW);

	context;
	say("Note sent to %s.  Thanks!  (%d chars)",u_recp->acctname,strlen(cmdinfo.param));
	if(User_informuserofnote(u_recp,cmdinfo.account))
		say("(%s is online...)",u_recp->acctname);
}

void cmd_addnote()
{
	userrec	*u_recp;
	noterec	*n;

	context;
	if(!cmdinfo.isparam)
	{
		say("Usage: addnote <nick> <msg>");
		sayf(2,0,"Use this command to add more text to a previous Note.  If the Note has been read or deleted, the msg will be sent as a new Note.");
		return;
	}

	context;
	if(cmdinfo.num_elements < 2)
	{
		say("Usage: addnote <nick> <msg>");
		sayf(2,0,"No msg was specified!");
		return;
	}

	context;
	if(!User_isknown(cmdinfo.tok1))
	{
		sayf(0,0,"The nick \"%s\" is not known to me.",cmdinfo.tok1);
		return;
	}

	context;
	u_recp = User_user(cmdinfo.tok1);
	if(!u_recp->registered)
	{
		sayf(0,0,"The user \"%s\" is not registered.  Notes can only be sent to registered users.  Sorry!",cmdinfo.tok1);
		return;
	}

	split(NULL,cmdinfo.param);

	context;
	/* Recipient is ok.  Find their last Note from this user */
	n = Note_lastnotefromnick(u_recp,cmdinfo.user,cmdinfo.from_n);

	/* If NULL, no Notes from this user could be found (or it was read) */
	if(n == NULL)
	{
		say("No Note from you exists for \"%s\"!  Sending as new...",u_recp->acctname);
		User_addnewnote(u_recp,cmdinfo.user,cmdinfo.account,cmdinfo.param,NOTE_NEW);
		User_informuserofnote(u_recp,cmdinfo.account);
		say("Note sent to %s.  Thanks!  (%d chars)",u_recp->acctname,strlen(cmdinfo.param));
		return;
	}

	context;
	/* Found a Note from this user and it is still new.  Append text. */
	Note_addbody(n,cmdinfo.param);
	say("Message added to your last Note to %s. (%d chars)",u_recp->acctname,strlen(cmdinfo.param));
}

void cmd_showmynote()
{
	userrec	*u_recp;
	nbody	*body;
	noterec	*n;
	char	sent[TIMESHORTLEN];

	context;
	if(!cmdinfo.isparam)
	{
		say("Usage: showmynote <user>");
		sayf(2,0,"Use this command to display your last Note sent to the specified User.");
		return;
	}

	context;
	if(!User_isknown(cmdinfo.tok1))
	{
		sayf(0,0,"The nick \"%s\" is not known to me.",cmdinfo.tok1);
		return;
	}

	context;
	u_recp = User_user(cmdinfo.tok1);
	if(!u_recp->registered)
	{
		sayf(0,0,"The user \"%s\" is not registered.  Since Notes can only be sent to registered users, they aren't going to have any Notes from you.  Sorry!",cmdinfo.tok1);
		return;
	}

	context;
	/* Recipient is ok.  Find their last Note from this user */
	n = Note_lastnotefromnick(u_recp,cmdinfo.user,cmdinfo.from_n);

	context;
	/* If NULL, no Notes from this user could be found (or it was read) */
	if(n == NULL)
	{
		char fwd[25];

		fwd[0] = 0;
		if(u_recp->forward && u_recp->forward[0])
			sprintf(fwd," or forwarded");

		sayf(0,0,"No Notes could be found to %s from you!  (Or your last Note to %s has been read%s...)",u_recp->acctname,u_recp->acctname,fwd);

		return;
	}

	context;
	say("Your last Note to %s reads:",u_recp->acctname);
	timet_to_date_long(n->sent,sent);
	say("-----------------------------------------------------------------");
	if(isequal(n->finger,n->uh))
		sayf(0,0,"From: %s (%s)",n->nick,n->uh);
	else
	{
		say("From: %s \"%s\"",n->nick,n->finger);
		say("Host: %s",n->uh);
	}
	say("Sent: %s",sent);
	say("-");
	for(body=n->body; body!=NULL; body=body->next)
		say("%s",body->text);
	say("-----------------------------------------------------------------");
}


void cmd_memo()
{
	context;
	if(!cmdinfo.isparam)
	{
		say("Usage: memo <text>");
		sayf(2,0,"Use this command to send a quick memo to yourself.");
		return;
	}

	User_addnewnote(cmdinfo.user,cmdinfo.user,cmdinfo.account,cmdinfo.param,NOTE_MEMO);

	context;
	say("Memo saved.  (%d chars)",strlen(cmdinfo.param));
}

void cmd_rn()
{
	int	note_num = 0, num = 0, i;
	nbody	*body;
	noterec	*n = NULL;
	char	sent[TIMESHORTLEN], buf[BUFSIZ], buf1[BUFSIZ], tail[LINELENGTH];

	context;
	if(!cmdinfo.isparam)
	{
		if(!cmdinfo.user->num_notes)
		{
			say("You have no Notes.");
			return;
		}

		cmd_rn_display(NOTE_NEW|NOTE_READ|NOTE_MEMO|NOTE_NOTICE|NOTE_SAVE);
		return;
	}

	context;
	if(!cmdinfo.user->num_notes)
	{
		say("You have no Notes.");
		return;
	}

	context;
	/* Display index of type */
	if(isequal(cmdinfo.tok1,"new"))
	{
		if(!Note_numnew(cmdinfo.user))
			say("You have no new Notes.");
		else
		{
			say("Displaying new Notes only. (%d new)",Note_numnew(cmdinfo.user));
			say("");
			cmd_rn_display(NOTE_NEW);
		}
		return;
	}

	context;
	if(isequal(cmdinfo.tok1,"saved"))
	{
		if(!Note_numsaved(cmdinfo.user))
			say("You have no saved Notes.");
		else
		{
			say("Displaying saved Notes only. (%d saved)",Note_numsaved(cmdinfo.user));
			say("");
			cmd_rn_display(NOTE_SAVE);
		}
		return;
	}

	context;
	if(isequal(cmdinfo.tok1,"memo"))
	{
		if(!Note_nummemo(cmdinfo.user))
			say("You have no memo Notes.");
		else
		{
			int	num;

			num = Note_nummemo(cmdinfo.user);
			say("Displaying memo Notes only. (%d memo%s)",num,num==1?"":"s");
			say("");
			cmd_rn_display(NOTE_MEMO);
		}
		return;
	}

	context;
	if(isequal(cmdinfo.tok1,"read"))
	{
		if(!Note_numread(cmdinfo.user))
			say("You have no read Notes.");
		else
		{
			say("Displaying read Notes only. (%d read)",Note_numread(cmdinfo.user));
			say("");
			cmd_rn_display(NOTE_READ);
		}
		return;
	}

	if(isequal(cmdinfo.tok1,"notice"))
	{
		if(!Note_numnotice(cmdinfo.user))
			say("You have no notice Notes.");
		else
		{
			int	num;

			num = Note_numnotice(cmdinfo.user);
			say("Displaying notice Notes only. (%d notice%s)",num,num==1?"":"s");
			say("");
			cmd_rn_display(NOTE_NOTICE);
		}
		return;
	}

	if(isequal(cmdinfo.tok1,"all"))
	{
		say("Displaying all Notes. (%d Notes)",cmdinfo.user->num_notes);
		say("");
		cmd_rn_display(NOTE_NEW|NOTE_READ|NOTE_MEMO|NOTE_NOTICE|NOTE_SAVE);
		return;
	}

	context;
	/* User wants next unread.  Find it */
	if(isequal(cmdinfo.tok1,"n"))
	{
		for(n=cmdinfo.user->notes; n!=NULL; n=n->next)
		{
			note_num++;
			if(n->flags & NOTE_NEW)
			{
				num = note_num;
				break;
			}
		}

		if(num == 0)
		{
			say("No more unread Notes.");
			return;
		}
	}
	else if(!isanumber(cmdinfo.tok1))
	{
		userrec	*u;
		noterec	*got_n = NULL;
		char	lookup[256];

		strcpy(lookup,cmdinfo.tok1);
		if(strlen(lookup) > 9)
			lookup[9] = 0;

		/* Lookup nick */
		u = User_user(lookup);
		if(!u->registered)
		{
			say("No such nick \"%s\".",lookup);
			say("Type 'rn -h' to see help on this command.");
			return;
		}

		for(n=cmdinfo.user->notes; n!=NULL; n=n->next)
		{
			if(isequal(n->nick,u->acctname))
				got_n = n;
		}

		n = got_n;
		if(n == NULL)
		{
			sayf(0,0,"No Notes by %s could be found!",lookup);
			return;
		}
	}
	else
		num = atoi(cmdinfo.tok1);

	context;
	if(!n && (num <= 0 || num > cmdinfo.user->num_notes))
	{
		say("Invalid number!  (Use numbers between 1 and %d)",cmdinfo.user->num_notes);
		return;
	}

	context;
	/* Ok the note number is valid.  Find and display it */
	if(n == NULL)
	{
		n = Note_notebynumber(cmdinfo.user,num);
		if(n == NULL)
			return;
	}

	context;
	timet_to_date_long(n->sent,sent);

	context;
	n->flags &= ~NOTE_NEW;
	n->flags |= NOTE_READ;

	/* Create new notes string */
	buf1[0] = 0;
	if(Note_numnew(cmdinfo.user))
		sprintf(buf1, "(%d New) ", Note_numnew(cmdinfo.user));

	/* Create first part of string and Notes string */
	sprintf(buf, "-------------------- [ Note: %d of %d %s] ", num,
		cmdinfo.user->num_notes, buf1);

	/* Add as many '-' characters as needed */
	tail[0] = 0;
	for(i = 0; i < LINELENGTH - strlen(buf); i++)
		strcat(tail, "-");

	say("%s%s", buf, tail);
	say("From: %s \"%s\"",n->nick,n->finger);
	say("Host: %s",n->uh);
	say("Sent: %s",sent);
	say("-");
	for(body=n->body; body!=NULL; body=body->next)
		say("%s",body->text);
	say("-----------------------------------------------------------------");

	context;
#if 0
	/* Delete notices as soon as they are read */
	if(n->flags & NOTE_NOTICE)
	{
		n->flags &= ~NOTE_NEW;
		n->flags |= NOTE_READ;
		n->flags |= NOTE_DELETEME;
		Note_deletemarkednotes(cmdinfo.user);
		context;
		return;
	}
#endif
}

void cmd_rn_display(u_long type)
{
	noterec	*n;
	char	save[20], read[20], subject[LINELENGTH], received[TIMESHORTLEN];
	char	finger[UHOSTLEN];
	int	num = 0;

	context;
	if(!cmdinfo.user->registered)
	{
		Log_write(LOG_ERROR,"*","User must be registered in order to read Notes.");
		return;
	}

	say(" #    Sent  From      Real Name               Subject");
	say("-- -- ----- --------- ----------------------- -------------------");
	context;
	for(n=cmdinfo.user->notes; n!=NULL; n=n->next)
	{
		num++;

		if(!(n->flags & type))
			continue;

		save[0] = 0;
		read[0] = 0;
		if(n->flags & NOTE_SAVE)
			strcpy(save,"*");
		if(n->flags & NOTE_NOTICE)
			strcpy(save,"!");
		if(n->flags & NOTE_NEW)
			strcpy(read,"N");
		else if(n->flags & NOTE_MEMO)
			strcpy(read,"m");
		else if(n->flags & NOTE_READ)
			strcpy(read," ");

		strcpy(finger,n->finger);
		if(strlen(finger) > 23)
			strcpy(&finger[23 - 3],"...");

		if(n->body != NULL && n->body->text != NULL)
			strcpy(subject,n->body->text);
		else
			strcpy(subject,"");
		if(strlen(subject) > 19)
			strcpy(&subject[19 - 3],"...");
		timet_to_dateonly_short(n->sent,received);
		received[5] = 0;

		say("%2d %1s%1s %5s %-9s %-23s %-19s",num,save,read,received,n->nick,finger,subject);
	}
	say("-- -- ----- --------- ----------------------- -------------------");
}

void cmd_dn()
{
	int	num = 0, num_deleted, req = 0;
	char	eachnum[BUFSIZ];
	noterec	*n;

	context;
	if(!cmdinfo.isparam)
	{
		say("Usage: dn <# | *> [# ... ]");
		sayf(2,0,"Use this command to delete one or more Notes.  Use '*' to delete all Notes.");
		return;
	}

	context;
	if(isequal(cmdinfo.tok1,"*"))
	{
		User_freeallnotes(cmdinfo.user);
		say("All Notes deleted!");
		return;
	}

	context;
	split(eachnum,cmdinfo.param);
	while(eachnum[0])
	{
		if(!isanumber(eachnum))
		{
			split(eachnum,cmdinfo.param);
			continue;
		}
		else
			num = atoi(eachnum);
	
		if(num <= 0 || num > cmdinfo.user->num_notes)
		{
			split(eachnum,cmdinfo.param);
			continue;
		}
	
		/* Ok the note number is valid.  Delete it. */
		n = Note_notebynumber(cmdinfo.user,num);
		if(n == NULL)
		{
			split(eachnum,cmdinfo.param);
			continue;
		}
	
		/* Mark for delete */
		if((n->flags & NOTE_NEW) && !(n->flags & NOTE_REQDEL))
		{
			req++;
			say("Note %d has not been read!  Type 'dn %d' to confirm delete.",num,num);
			n->flags |= NOTE_REQDEL;
		}
		else
			n->flags |= NOTE_DELETEME;

		split(eachnum,cmdinfo.param);
	}

	context;
	num_deleted = Note_deletemarkednotes(cmdinfo.user);
	if(req)
	{
		if(req > 1)
			say("Some Notes were not deleted because they have not been read.");
	}
	else
	{
		if(!num_deleted)
			say("No Notes deleted!");
		else if(num_deleted == 1)
			say("Note %d deleted!",num);
		else
			say("%d Notes deleted!",num_deleted);
	}
}

void cmd_dccinfo()
{
	dccrec	*d;
	char	estab[TIMESHORTLEN], last[TIMESHORTLEN], type[50], flags[50];
	char	uh[UHOSTLEN], in[20], out[20], id[5];
	int	tot, i;

	context;

	/* Display header */
	if(isequal(cmdinfo.tok1,"-t"))
	{
		say(" ID Estab Idl Name        Type (or filename) Flags");
		say("--- ----- --- ----------- ------------------ --------------------");
	}
	else
	{
		say(" ID Estab Idl Name      Host                            In   Out");
		say("--- ----- --- --------- ------------------------------- ---- ----");
	}

	/* Display each record */
	tot = DCC_orderbystarttime();
	for(i=1; i<=tot; i++)
	{
		d = DCC_dccbyordernum(i);
		if(d == NULL)
			continue;

		timet_to_time_short(d->starttime,estab);
		timet_to_ago_short(d->last,last);

		id[0] = 0;
		if(d->flags & STAT_WEBCLIENT)
			sprintf(id,"W%d",d->socket);
		else if(d->flags & STAT_GATEWAYCLIENT)
			sprintf(id,"G%d",d->socket);
		else if(d->flags & CLIENT_TELNET_CHAT)
			sprintf(id,"T%d",d->socket);
		else if(d->flags & CLIENT_IRC_CHAT)
			sprintf(id,"I%d",d->socket);
		else if(d->flags & CLIENT_XFER)
			sprintf(id,"X%d",d->socket);
		else if(d->flags & (SERVER_PUBLIC|SERVER_INTERNAL|SERVER_IRC_CHAT|SERVER_XFER))
			sprintf(id,"S%d",d->socket);

		if(isequal(cmdinfo.tok1,"-t"))
		{
			/* Show type information */
			type[0] = 0;
			flags[0] = 0;
			if(d->flags & SERVER_PUBLIC)
			{
				strcpy(type,"telnet server");
				sprintf(flags,"on port %d",d->port);
			}
			else if(d->flags & SERVER_INTERNAL)
			{
				strcpy(type,"internal server");
				if(Grufti->show_internal_ports)
					sprintf(flags,"on port %d",d->port);
				else
					sprintf(flags,"on port ****");
			}
			else if(d->flags & SERVER_IRC_CHAT)
			{
				strcpy(type,"dcc chat server");
				strcpy(flags,"waiting for chat");
			}
			else if(d->flags & SERVER_XFER)
			{
				strcpy(type,"dcc xfer server");
				strcpy(flags,"waiting for xfer");
			}
			else if(d->flags & CLIENT_IDENT)
			{
				strcpy(type,"ident client");
				strcpy(flags,"waiting for ident");
			}
			else if(d->flags & CLIENT_IRC_CHAT)
				strcpy(type,"dcc chat client");
			else if(d->flags & CLIENT_TELNET_CHAT)
				strcpy(type,"telnet client");
			else if(d->flags & CLIENT_XFER)
			{
				char	n1[50], n2[50], whichway[10];

				sprintf(type,"\"%s\"",d->filename);
				if(strlen(type) > 18)
					type[18] = 0;
				bytes_to_kb_short(d->sent,n1);
				bytes_to_kb_short(d->length,n2);

				whichway[0] = 0;
				if(d->flags & STAT_XFER_IN)
					strcpy(whichway,"recv");
				if(d->flags & STAT_XFER_OUT)
					strcpy(whichway,"sent");
				sprintf(flags,"%s of %s %s",n1,n2,whichway);
			}

			if(d->flags & STAT_WEBCLIENT)
				strcpy(type,"web client");
			else if(d->flags & STAT_GATEWAYCLIENT)
				strcpy(type,"gateway client");
			if(d->flags & STAT_LOGIN)
				strcpy(flags,"doing login");
			if(d->flags & STAT_PASSWORD)
				strcpy(flags,"doing password");
			if(d->flags & STAT_PENDING_IDENT)
				strcpy(flags,"pending ident");
			if(d->flags & STAT_VERIFIED)
				strcpy(flags,"user is verified");
	
			say("%3s %5s %3s %-11s %-18s %-20s",id,estab,last,d->nick,type,flags);
		}
		else
		{
			/* Show normal information */
			bytes_to_kb_short(d->bytes_in,in);
			bytes_to_kb_short(d->bytes_out,out);
			strcpy(uh,d->uh);
			if(strlen(uh) > 31)
				uh[31] = 0;
	
			say("%3s %5s %3s %-9s %-31s %4s %4s",id,estab,last,d->nick,uh,in,out);
		}
	}

	/* Display footer */
	if(isequal(cmdinfo.tok1,"-t"))
		say("--- ----- --- ----------- ------------------ --------------------");
	else
		say("--- ----- --- --------- ------------------------------- ---- ----");

}

void cmd_savenote()
{
	int	num = 0;
	noterec	*n;

	context;
	if(!cmdinfo.isparam)
	{
		say("Usage: savenote <#>");
		sayf(2,0,"Use this command to mark a Note as 'saved'.  'Saved' Notes do not timeout and remain in your account until you delete them.");
		return;
	}

	context;
	if(!isanumber(cmdinfo.tok1))
	{
		say("Usage: savenote <#>");
		sayf(2,0,"Use this command to mark a Note as 'saved'.  'Saved' Notes do not timeout and remain in your account until you delete them.");
		return;
	}
	else
		num = atoi(cmdinfo.tok1);

	context;
	if(num <= 0 || num > cmdinfo.user->num_notes)
	{
		say("Invalid number!  (Use numbers between 1 and %d)",cmdinfo.user->num_notes);
		return;
	}

	context;
	/* Ok the note number is valid.  Mark note as saved */
	n = Note_notebynumber(cmdinfo.user,num);
	if(n == NULL)
		return;

	if(n->flags & NOTE_SAVE)
	{
		say("Note %d was already marked 'saved'!",num);
		return;
	}

	context;
	if(Note_numsaved(cmdinfo.user) >= Grufti->max_saved_notes)
	{
		sayf(0,0,"You are allowed %d saved Notes and you have %d saved already!  Type 'unsavenote <#>' or 'dn <#>' to unsave or delete one.",Grufti->max_saved_notes,Grufti->max_saved_notes);
		return;
	}

	context;
	n->flags |= NOTE_SAVE;
	say("Note %d will be saved.  Type 'rn saved' to view.",num);
}

void cmd_unsavenote()
{
	int	num = 0;
	noterec	*n;

	context;
	if(!cmdinfo.isparam)
	{
		say("Usage: unsavenote <#>");
		sayf(2,0,"Use this command to unmark a Note as 'saved'.");
		return;
	}

	context;
	if(!isanumber(cmdinfo.tok1))
	{
		say("Usage: unsavenote <#>");
		sayf(2,0,"Use this command to unmark a Note as 'saved'.");
		return;
	}
	else
		num = atoi(cmdinfo.tok1);

	context;
	if(num <= 0 || num > cmdinfo.user->num_notes)
	{
		say("Invalid number!  (Use numbers between 1 and %d)",cmdinfo.user->num_notes);
		return;
	}

	context;
	/* Ok the note number is valid.  Unmark note as saved */
	n = Note_notebynumber(cmdinfo.user,num);
	if(n == NULL)
		return;

	context;
	if(!(n->flags & NOTE_SAVE))
	{
		say("Note %d was not marked 'saved'!",num);
		return;
	}

	n->flags &= ~NOTE_SAVE;
	say("Save flag on Note %d removed.",num);
}

void cmd_cmdinfo()
{
	context;
	if(cmdinfo.istok1)
		cmd_cmdinfo_long();
	else
		cmd_cmdinfo_short();
}

extern listmgr_t *commands;
void cmd_cmdinfo_long()
{
	cmdrec_t	*c, *c_next;
	int	num_cols = 2;
	int	col, row, num_rows, tot, have_flags = 0;
	u_long	flags = 0L;
	char	line[160], levels[FLAGSHORTLEN];

	/* First token is '-l'.  If 2nd token exists, check for levels */
	if(isequal(cmdinfo.tok1,"-l"))
	{
		if(cmdinfo.istok2)
		{
			User_str2flags(cmdinfo.tok2,&flags);
			have_flags = 1;
		}
	}
	else
	{
		User_str2flags(cmdinfo.tok1,&flags);
		have_flags = 1;
	}
	
	/*
	 * THIS IS A QUICK HACK TO MAKE THIS COMMAND WORK.  DOES NOT
	 * SUPPORT NESTED COMMANDS.
	 */

	listmgr_sort_ascending(commands);
	/*
	 * if we have flags, remove (temporarily) all commands which don't
	 * have those flags.
	 */
	if(have_flags)
	{
		c = listmgr_begin_sorted(commands);
		while(c)
		{
			c_next = listmgr_next_sorted(commands);
			if(!(c->levels & flags))
				listmgr_remove_from_sorted(commands, c);
	
			c = c_next;
		}
	}

	tot = listmgr_length_sorted(commands);

	line[0] = 0;

	/* Display header */
	if(have_flags)
	{
		User_flags2str(flags,levels);
		say("%d command%s match%s levels \"%s\".",tot,tot==1?"":"s",tot==1?"es":"",levels);
	}
	else
		say("%d command%s.",tot,tot==1?"":"s");

	say("");

	/* Display 1 column if less than 20 entries */
	if(tot <= ENTRIES_FOR_ONE_COL)
	{
		say("Hits Command        DCC  Levels");
		say("---- -------------- ---- ------");
		num_cols = 1;
	}
	else
	{
		say("Hits Command        DCC  Levels   Hits Command        DCC  Levels");
		say("---- -------------- ---- ------   ---- -------------- ---- ------"); 
	}

	num_rows = tot / num_cols;
	if(tot % num_cols)
		num_rows++;

	for(row=1; row<=num_rows; row++)
	{
		for(col=1; col<=num_cols; col++)
		{
			/*
			 * TODO:
			 *  We need a pointer array so that we can grab an entry at
			 *  any offset without iterating the list over and over again.
			 */
			c = Command_cmdbyorder((num_rows * (col-1)) + row);
			if(c)
			{
				User_flags2str(c->levels,levels);
				sprintf(line,"%s%4d %-14s %-4s %-6s   ",line,c->accessed,c->name,c->needsdcc==1?"YES":"NO",levels);
			}
		}
		line[strlen(line) - 3] = 0;
		say("%s",line);
		line[0] = 0;
	}

	if(tot <= ENTRIES_FOR_ONE_COL)
		say("---- -------------- ---- ------");
	else
		say("---- -------------- ---- ------   ---- -------------- ---- ------"); 
}

void cmd_cmdinfo_short()
{
	cmdrec_t *c, *c_next;
	int	num_cols = 4;
	int	col, row, num_rows, tot, have_flags = 0;
	u_long	flags = 0L;
	char	line[160], levels[FLAGSHORTLEN];

	if(cmdinfo.istok1)
	{
		User_str2flags(cmdinfo.tok1,&flags);
		have_flags = 1;
	}

	listmgr_sort_ascending(commands);
	c = listmgr_begin_sorted(commands);
	/*
	 * if we have flags, remove (temporarily) all commands which don't
	 * have those flags.
	 */
	if(have_flags)
	{
		c = listmgr_begin_sorted(commands);
		while(c)
		{
			c_next = listmgr_next_sorted(commands);
			if(!(c->levels & flags))
				listmgr_remove_from_sorted(commands, c);
	
			c = c_next;
		}
	}

	tot = listmgr_length_sorted(commands);

	line[0] = 0;

	/* Display header */
	if(have_flags)
	{
		User_flags2str(flags,levels);
		say("%d command%s match%s levels \"%s\".",tot,tot==1?"":"s",tot==1?"es":"",levels);
	}
	else
		say("%d command%s.",tot,tot==1?"":"s");

	say("");

	/* Display 1 column if less than 20 entries */
	if(tot <= ENTRIES_FOR_ONE_COL)
	{
		say("Command");
		say("--------------");
		num_cols = 1;
	}
	else
	{
		say("Command         Command         Command         Command");
		say("--------------- --------------- --------------- ---------------");
	}

	num_rows = tot / num_cols;
	if(tot % num_cols)
		num_rows++;

	for(row=1; row<=num_rows; row++)
	{
		for(col=1; col<=num_cols; col++)
		{
			c = Command_cmdbyorder((num_rows * (col-1)) + row);
			if(c)
				sprintf(line,"%s%-15s ",line,c->name);
		}
		line[strlen(line) - 1] = 0;
		say("%s",line);
		line[0] = 0;
	}

	if(tot <= ENTRIES_FOR_ONE_COL)
		say("--------------");
	else
		say("--------------- --------------- --------------- ---------------");
}

void cmd_codesinfo()
{
	coderec	*code;
	int	num_cols = 4;
	int	col, row, num_rows, tot;
	char	line[160];

	context;
	line[0] = 0;
	say("%d server codes.",Codes->num_codes);

	say("");
	say("Hit    Code      Hit    Code      Hit    Code      Hit    Code");
	say("------ -------   ------ -------   ------ -------   ------ -------");
	tot = Codes_orderbycode();

	num_rows = tot / num_cols;
	if(tot % num_cols)
		num_rows++;

	for(row=1; row<=num_rows; row++)
	{
		for(col=1; col<=num_cols; col++)
		{
			code = Codes_codebyorder((num_rows * (col-1)) + row);
			if(code)
				sprintf(line,"%s%6lu %-7s   ",line,code->accessed,code->name);
		}
		line[strlen(line) - 3] = 0;
		say("%s",line);
		line[0] = 0;
	}
	say("------ -------   ------ -------   ------ -------   ------ -------");
}

void cmd_whoisin()
{
	locarec	*location;
	userrec	*u;
	char	l_string[BUFSIZ], u_location[20], l_info[SERVERLEN];
	int	num = 0;

	context;
	if(!cmdinfo.isparam)
	{
		say("Usage: whoisin <# | city | state | country>");
		sayf(2,0,"Use this command to display a listing of all registered users located in a particular region.  Type 'showlocations' to view all location possibilities.");
		return;
	}

	/* l_string looks like: " # # # # " (note the spaces) */

	/* if it's a number, get locations by number. */
	if(isanumber(cmdinfo.param))
	{
		num = atoi(cmdinfo.tok1);
		location = Location_location(num);
		if(location == NULL)
		{
			sayf(0,0,"Location \"%d\" is not a valid location number.  Type 'showlocations' to see which location numbers you can use.  Or, use a region like 'CA' or 'Boston'.",num);
			return;
		}

		sprintf(l_string," %d ",location->id);
		Location_ltostr(location,l_info);
		say("Users who live in area: \"%s\"",l_info);
	}
	else
	{
		location = Location_locationbytok(cmdinfo.param);
		if(location == NULL)
		{
			sayf(0,0,"The location \"%s\" is not among my list of locations.  Type 'showlocations' to view all location possibilities.  You can use either location numbers, or regions like 'CA' and 'Boston'.",cmdinfo.param);
			return;
		}

		/* Got it.  Write a string with #'s matching that location */
		Location_locationidsforarea(cmdinfo.param,l_string);
		say("Users who live in area: \"%s\"",cmdinfo.param);
	}
	
	say("");

	say("User      Location");
	say("--------- -------------------------------------------------------");
	for(u=User->userlist; u!=NULL; u=u->next)
	{
		sprintf(u_location," %d ",u->location);
		if(strstr(l_string,u_location))
		{
			Location_idtostr(u->location,l_info);
			if(strlen(l_info) > 55)
				l_info[52] = 0;
			say("%-9s %-55s",u->acctname,l_info);
		}
	}
	say("--------- -------------------------------------------------------");
}

void cmd_webcatch()
{
	urlrec	*u;
	int	count = 0, num = 0;
	char	when[TIMESHORTLEN], url[SERVERLEN], pre[20];

	context;
	if(!cmdinfo.isparam)
		num = 20;

	context;
	if(!URL->num_urls)
	{
		say("No URL's caught yet.  Try again later!");
		return;
	}

	context;
	if(!isanumber(cmdinfo.tok1))
	{
		say("Usage: webcatch [# hits]");
		sayf(2,0,"Use this command to display the last n web catches made.");
		return;
	}

	context;
	if(!num)
	{
		num = atoi(cmdinfo.tok1);
		if(num <= 0)
		{
			say("Invalid number.  Please use POSITIVE numbers only, thanks.");
			return;
		}
	}

	say("When  Nick      URL (%d URL%s caught)",URL->num_urls,URL->num_urls==1?"":"'s");
	say("----- --------- -------------------------------------------------");
	context;
	for(u=URL->urllist; u!=NULL; u=u->next)
	{
		count++;
		timet_to_time_short(u->time_caught,when);
		strcpy(url,u->url);
		if(strlen(url) > 145)
			url[145] = 0;

		sprintf(pre,"%-5s %-9s ",when,u->nick);
		sayi(pre,"%s",url);

		if(count == num)
			break;
	}
	say("----- --------- -------------------------------------------------");
}

void cmd_clearwebcatch()
{
	context;
	URL_freeallurls();
	say("Webcatch cleared.");
}

void cmd_addgreet()
{
	context;
	if(!cmdinfo.isparam)
	{
		say("Usage: addgreet <greeting>",cmdinfo.cmdname);
		sayf(2,0,"Use this command to set a greet.  The bot will not greet you however unless your levels include +g.");
		return;
	}

	context;
	/* Check if they're over the limit. */
	if(cmdinfo.user->num_greets >= Grufti->max_greets)
	{
		sayf(0,0,"You are allowed %d greetings for this account and you've set up all %d!  Type 'delgreet <#>' to delete one.",Grufti->max_greets,Grufti->max_greets);
		return;
	}

	context;
	/* Verify greet */
	if(!strstr(cmdinfo.param,"\\n"))
	{
		sayf(0,0,"You must have a \\n declaration somewhere in your greeting, which translates into the nick you're using when you join the channel.");
		return;
	}

	/* Fuck. */
	if(strchr(cmdinfo.param,'%'))
	{
		sayf(0,0,"Sorry, I can't allow the percent character in your greets for now.  It gives me the runs.");
		return;
	}

	context;
	/* Check length */
	if(cmdinfo.paramlen > GREETLENGTH)
	{
		say("Your greeting is too long by %d character%s.",cmdinfo.paramlen - GREETLENGTH,(cmdinfo.paramlen - GREETLENGTH)==1?"":"s");
		return;
	}

	context;
	User_addnewgreet(cmdinfo.user,cmdinfo.param);

	if(!(cmdinfo.user->flags & USER_GREET))
		sayf(0,0,"Greet line set.  (But your levels don't include +g!)");
	else
		sayf(0,0,"Greet line set.");
}

void cmd_showgreets()
{
	gretrec	*g;
	char	indent[80], line[SERVERLEN];
	int	i = 0, translate = 0;
	char	action;

	context;
	/* Show a listing of translated greetings */
	if(isequal(cmdinfo.tok1,"-t"))
		translate = 1;

	context;
	/* If an argument exists, show greetings for that user */
	if(cmdinfo.istok1 && (cmdinfo.user->flags & USER_MASTER) && !translate)
	{
		cmd_showgreets_for_user();
		return;
	}

	context;
	if(!cmdinfo.user->num_greets)
	{
		say("You have no greetings set.  Type 'addgreet'.");
		return;
	}

	context;
	if(translate)
		say(" # Greeting (in translated form)");
	else
		say(" # Greeting (use '-t' to display in translated form)");
	say("-- --------------------------------------------------------------");
	context;
	for(g=cmdinfo.user->greets; g!=NULL; g=g->next)
	{
		i++;
		sprintf(indent,"%2d ",i);
		if(translate)
		{
			Response_parse_reply(line,g->text,cmdinfo.from_n,cmdinfo.from_uh,"#???",&action);
			rmspace(line);
			if(action)
				sayi(indent,"* %s %s",Grufti->botnick,line);
			else
				sayi(indent,"<%s> %s",Grufti->botnick,line);
		}
		else
			sayi(indent,"%s",g->text);
	}
	say("-- --------------------------------------------------------------");
}

void cmd_showother()
{
	char tmp[BUFSIZ], pre[BUFSIZ];

	say(" # Field   Info");
	say("-- ------- ------------------------------------------------------");
	if(cmdinfo.user->other1)
	{
		if(strchr(cmdinfo.user->other1, ' '))
		{
			strcpy(tmp, cmdinfo.user->other1);
			split(pre, tmp);
			if(strlen(pre) > 7)
				pre[7] = 0;
			say("%2d %-7s %s", 1, pre, tmp);
		}
		else
			say("%2d %-7s %s", 1, "other", cmdinfo.user->other1);
	}
	else
		say("%2d", 1);

	if(cmdinfo.user->other2)
	{
		if(strchr(cmdinfo.user->other2, ' '))
		{
			strcpy(tmp, cmdinfo.user->other2);
			split(pre, tmp);
			if(strlen(pre) > 7)
				pre[7] = 0;
			say("%2d %-7s %s", 2, pre, tmp);
		}
		else
			say("%2d %-7s %s", 2, "other", cmdinfo.user->other2);
	}
	else
		say("%2d", 2);

	if(cmdinfo.user->other3)
	{

		if(strchr(cmdinfo.user->other3, ' '))
		{
			strcpy(tmp, cmdinfo.user->other3);
			split(pre, tmp);
			if(strlen(pre) > 7)
				pre[7] = 0;
			say("%2d %-7s %s", 3, pre, tmp);
		}
		else
			say("%2d %-7s %s", 3, "other", cmdinfo.user->other3);
	}
	else
		say("%2d", 3);

	if(cmdinfo.user->other4)
	{

		if(strchr(cmdinfo.user->other4, ' '))
		{
			strcpy(tmp, cmdinfo.user->other4);
			split(pre, tmp);
			if(strlen(pre) > 7)
				pre[7] = 0;
			say("%2d %-7s %s", 4, pre, tmp);
		}
		else
			say("%2d %-7s %s", 4, "other", cmdinfo.user->other4);
	}
	else
		say("%2d", 4);

	if(cmdinfo.user->other5)
	{

		if(strchr(cmdinfo.user->other5, ' '))
		{
			strcpy(tmp, cmdinfo.user->other5);
			split(pre, tmp);
			if(strlen(pre) > 7)
				pre[7] = 0;
			say("%2d %-7s %s", 5, pre, tmp);
		}
		else
			say("%2d %-7s %s", 5, "other", cmdinfo.user->other5);
	}
	else
		say("%2d", 5);

	if(cmdinfo.user->other6)
	{

		if(strchr(cmdinfo.user->other6, ' '))
		{
			strcpy(tmp, cmdinfo.user->other6);
			split(pre, tmp);
			if(strlen(pre) > 7)
				pre[7] = 0;
			say("%2d %-7s %s", 6, pre, tmp);
		}
		else
			say("%2d %-7s %s", 6, "other", cmdinfo.user->other6);
	}
	else
		say("%2d", 6);

	if(cmdinfo.user->other7)
	{

		if(strchr(cmdinfo.user->other7, ' '))
		{
			strcpy(tmp, cmdinfo.user->other7);
			split(pre, tmp);
			if(strlen(pre) > 7)
				pre[7] = 0;
			say("%2d %-7s %s", 7, pre, tmp);
		}
		else
			say("%2d %-7s %s", 7, "other", cmdinfo.user->other7);
	}
	else
		say("%2d", 7);

	if(cmdinfo.user->other8)
	{

		if(strchr(cmdinfo.user->other8, ' '))
		{
			strcpy(tmp, cmdinfo.user->other8);
			split(pre, tmp);
			if(strlen(pre) > 7)
				pre[7] = 0;
			say("%2d %-7s %s", 8, pre, tmp);
		}
		else
			say("%2d %-7s %s", 8, "other", cmdinfo.user->other8);
	}
	else
		say("%2d", 8);

	if(cmdinfo.user->other9)
	{

		if(strchr(cmdinfo.user->other9, ' '))
		{
			strcpy(tmp, cmdinfo.user->other9);
			split(pre, tmp);
			if(strlen(pre) > 7)
				pre[7] = 0;
			say("%2d %-7s %s", 9, pre, tmp);
		}
		else
			say("%2d %-7s %s", 9, "other", cmdinfo.user->other9);
	}
	else
		say("%2d", 9);

	if(cmdinfo.user->other10)
	{

		if(strchr(cmdinfo.user->other10, ' '))
		{
			strcpy(tmp, cmdinfo.user->other10);
			split(pre, tmp);
			if(strlen(pre) > 7)
				pre[7] = 0;
			say("%2d %-7s %s", 10, pre, tmp);
		}
		else
			say("%2d %-7s %s", 10, "other", cmdinfo.user->other10);
	}
	else
		say("%2d", 10);

	say("-- ------- ------------------------------------------------------");
	say("Use 'setother' and 'delother' to manage your other info slots.");
}

void cmd_delother()
{
	int n;
	if(!cmdinfo.isparam)
	{
		say("Usage: delother <#>");
		sayf(2,0,"Use this command to delete an info slot.  Use 'showother' to display all info slots.");
		return;
	}

	n = atoi(cmdinfo.tok1);
	if(n < 1 || n > 10)
	{
		say("No such info slot %d.  Use a number between 1 and 10.", n);
		return;
	}

	switch(n)
	{
		case 1:
			xfree(cmdinfo.user->other1);
			cmdinfo.user->other1 = NULL;
			break;

		case 2:
			xfree(cmdinfo.user->other2);
			cmdinfo.user->other2 = NULL;
			break;

		case 3:
			xfree(cmdinfo.user->other3);
			cmdinfo.user->other3 = NULL;
			break;

		case 4:
			xfree(cmdinfo.user->other4);
			cmdinfo.user->other4 = NULL;
			break;

		case 5:
			xfree(cmdinfo.user->other5);
			cmdinfo.user->other5 = NULL;
			break;

		case 6:
			xfree(cmdinfo.user->other6);
			cmdinfo.user->other6 = NULL;
			break;

		case 7:
			xfree(cmdinfo.user->other7);
			cmdinfo.user->other7 = NULL;
			break;

		case 8:
			xfree(cmdinfo.user->other8);
			cmdinfo.user->other8 = NULL;
			break;

		case 9:
			xfree(cmdinfo.user->other9);
			cmdinfo.user->other9 = NULL;
			break;

		case 10:
			xfree(cmdinfo.user->other10);
			cmdinfo.user->other10 = NULL;
			break;

	}

	say("Info slot %d erased.", n);
}


void cmd_showgreets_for_user()
{
	userrec	*u;
	gretrec	*g;
	char	indent[80];
	int	i = 0;

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
		sayf(0,0,"The nick \"%s\" is not registered.",cmdinfo.tok1);
		return;
	}

	context;
	if(!u->num_greets)
	{
		say("The user \"%s\" has no greetings set.",u->acctname);
		return;
	}

	say(" # Greeting");
	say("-- --------------------------------------------------------------");
	context;
	for(g=u->greets; g!=NULL; g=g->next)
	{
		i++;
		sprintf(indent,"%2d ",i);
		sayi(indent,"%s",g->text);
	}
	say("-- --------------------------------------------------------------");
}

void cmd_delgreet()
{
	int	num = 0, num_deleted;
	char	eachnum[BUFSIZ];
	gretrec	*g;

	context;
	if(!cmdinfo.isparam)
	{
		say("Usage: delgreet <# | *> [# ... ]");
		sayf(2,0,"Use this command to delete one or more greetings.  Use '*' to delete all greetings.");
		return;
	}

	context;
	if(isequal(cmdinfo.tok1,"*"))
	{
		User_freeallgreets(cmdinfo.user);
		say("All greetings deleted!");
		return;
	}

	context;
	split(eachnum,cmdinfo.param);
	while(eachnum[0])
	{
		if(!isanumber(eachnum))
		{
			split(eachnum,cmdinfo.param);
			continue;
		}
		else
			num = atoi(eachnum);
	
		if(num <= 0 || num > cmdinfo.user->num_greets)
		{
			split(eachnum,cmdinfo.param);
			continue;
		}
	
		/* Ok the greet number is valid.  Delete it. */
		g = User_greetbyindex(cmdinfo.user,num);
		if(g == NULL)
		{
			split(eachnum,cmdinfo.param);
			continue;
		}
	
		/* Mark for delete */
		g->flags |= GREET_DELETEME;

		split(eachnum,cmdinfo.param);
	}

	context;
	num_deleted = User_deletemarkedgreets(cmdinfo.user);
	if(!num_deleted)
		say("No greetings deleted!");
	else if(num_deleted == 1)
		say("Greet %d deleted!",num);
	else
		say("%d greetings deleted!",num_deleted);
}

void cmd_mode()
{
	userrec	*u;
	int	pos = 1;
	u_long	f, f_before;
	char	flags[FLAGSHORTLEN], mode[10], notice[BUFSIZ], msg[BUFSIZ], *p;

	if(!cmdinfo.isparam)
	{
		say("Usage: mode <user> <+|-><modes>");
		sayf(2,0,"Use this command to change the levels for a user.");
		return;
	}

	if(!User_isknown(cmdinfo.tok1))
	{
		sayf(0,0,"The nick \"%s\" is not known to me.",cmdinfo.tok1);
		return;
	}

	u = User_user(cmdinfo.tok1);
	if(!u->registered)
	{
		sayf(0,0,"The nick \"%s\" is not a registered user.",cmdinfo.tok1);
		return;
	}

	/* If only the user is given, display their modes. */
	if(cmdinfo.num_elements == 1)
	{
		User_flags2str(u->flags,flags);
		say("Levels for \"%s\" are: +%s",u->acctname,flags);
		return;
	}

	/* If a non-owner is trying to change the mode of an owner... */
	if(!(cmdinfo.user->flags & USER_OWNER) && (u->flags & USER_OWNER))
	{
		sayf(0,0,"You are trying to change the levels for the owner account, which is something I cannot permit you to do.");
		return;
	}

	/* Remember flags before we start */
	f_before = u->flags;

	/* we got flags.  iterate through each one */
	notice[0] = 0;
	for(p=cmdinfo.tok2; *p; p++)
	{
		if(*p == '+')
			pos = 1;
		else if(*p == '-')
			pos = 0;
		else
		{
			sprintf(mode,"%c",*p);
			User_str2flags(mode,&f);

			if(!(f & (USER_INVALID | USER_REGD | USER_NOLEVEL | USER_ANYONE | USER_OWNER)))
			{
				if(pos)
				{
					if(!(u->flags & f))
					{
						u->flags |= f;
						Note_informuseroflevelplus(u,f);
						sprintf(msg, "%s has given %s the level +%s.",
							cmdinfo.user->acctname, u->acctname, mode);

						/* Add to user history */
						User_addhistory(u, "system", time(NULL), msg);

						if(Grufti->send_notice_level_plus)
						{
							if(notice[0])
								sprintf(notice,"%s\n%s", notice, msg);
							else
								sprintf(notice,"%s", msg);
						}
					}
				}
				else
				{
					if(u->flags & f)
					{
						u->flags &= ~f;
						Note_informuseroflevelmin(u,f);
						sprintf(msg, "%s has taken the level +%s from %s.",
							cmdinfo.user->acctname, mode, u->acctname);

						/* Add to user history */
						User_addhistory(u, "system", time(NULL), msg);

						if(Grufti->send_notice_level_minus)
						{
							if(notice[0])
								sprintf(notice,"%s\n%s", notice, msg);
							else
								sprintf(notice,"%s", msg);
						}
					}
				}
			}
		}
	}

	context;
	User_flags2str(u->flags,flags);
	say("Levels for \"%s\" are: +%s",u->acctname,flags);

	/* Send one big Grufti Notice for all mode changes */
	if(notice[0])
		Note_sendnotification(notice);
}

void cmd_telnet()
{
	context;
	if(!Grufti->reqd_telnet_port)
	{
		say("No telnet access is available.");
		return;
	}

	say("Telnet access is available at: %s %d",Grufti->bothostname,Grufti->actual_telnet_port);
}

void cmd_maint()
{
	context;
	if(Grufti->maintainer[0])
		say("Maintainer: %s",Grufti->maintainer);
	else
		say("No maintainer info available.");
}

void cmd_version()
{
	context;
	if(Grufti->copyright[0])
	{
		say("%s",Grufti->copyright);
		say("Version: %s",Grufti->build);
	}
	else
		say("No version info available.");
}

void cmd_homepage()
{
	context;
	if(Grufti->homepage[0])
		say("homepage: %s",Grufti->homepage);	
	else
		say("No homepage info available.");
}

void cmd_uptime()
{
	int	hr, min;
	char	since[TIMELONGLEN], cpu[50];
#if HAVE_GETRUSAGE
	struct	rusage ru;
#else
#if HAVE_CLOCK
	clocl_t cl;
#endif
#endif

	/* online */
	context;
	timet_to_ago_long(Grufti->online_since,since);

#if HAVE_GETRUSAGE
	getrusage(RUSAGE_SELF,&ru);
	hr = (int)((ru.ru_utime.tv_sec + ru.ru_stime.tv_sec) / 60);
	min = (int)((ru.ru_utime.tv_sec + ru.ru_stime.tv_sec) - (hr * 60));
	sprintf(cpu," (CPU %02d:%02d)",hr,min);
#else
#if HAVE_CLOCK
	cl = (clock() / CLOCKS_PER_SEC);
	hr = (int)(cl / 60);
	min = (int)(cl - (hr * 60));
	sprintf(cpu," (CPU %02d:%02d)",hr,min);
#else
	sprintf(cpu,"");
#endif
#endif

	say("Online for %s.%s",since,cpu);
}

void cmd_showresponse()
{
	rtype	*rt;
	resprec	*r;
	char	list[BUFSIZ];

	context;
	if(!cmdinfo.isparam)
	{
		Response_makertypelist(list);

		say("Usage: %s <response type>",cmdinfo.cmdname);
		sayf(2,0,"Use a response type from the following list: %s",list);
		return;
	}

	context;
	rt = Response_type(cmdinfo.tok1);
	if(rt == NULL)
	{
		r = NULL;
		for(rt=Response->rtypes; rt!=NULL; rt=rt->next)
		{
			r = Response_response(rt,cmdinfo.tok1);
			if(r != NULL)
				break;
		}
		if(r == NULL)
		{
			sayf(0,0,"There is no response name or type called \"%s\".",cmdinfo.tok1);
			return;
		}
		else
			cmd_showresponses_byname(rt,r);
	}
	else
		cmd_showresponses_bytype(rt);
}

void cmd_showresponses_bytype(rtype *rt)
{
	resprec	*r;
	int	num_cols = 2;
	int	col, row, num_rows, tot;
	char	line[160];

	context;
	line[0] = 0;

	tot = Response_orderbyresponse(rt);

	if(tot == 0)
	{
		say("There are no %s responses.",rt->name);
		return;
	}

	say("Responses for type \"%s\" (%d)",rt->name,tot);

	say("");

	/* Display 1 column if less than 20 entries */
	if(tot <= ENTRIES_FOR_ONE_COL)
	{
		say("  # Response name");
		say("--- ---------------------------");
		num_cols = 1;
	}
	else
	{
		say("  # Response name                   # Response name");
		say("--- ---------------------------   --- ---------------------------");
	}

	num_rows = tot / num_cols;
	if(tot % num_cols)
		num_rows++;

	for(row=1; row<=num_rows; row++)
	{
		for(col=1; col<=num_cols; col++)
		{
			r = Response_responsebyorder(rt,(num_rows * (col-1)) + row);
			if(r)
				sprintf(line,"%s%3d %-27s   ",line,r->num_lines,r->name);
		}
		line[strlen(line) - 3] = 0;
		say("%s",line);
		line[0] = 0;
	}

	if(tot <= ENTRIES_FOR_ONE_COL)
		say("--- ---------------------------");
	else
		say("--- ---------------------------   --- ---------------------------");
}

void cmd_showresponses_byname(rtype *rt, resprec *r)
{
	rline	*rl;
	machrec	*mr;
	char	pre[100], date[TIMESHORTLEN];
	int	i;

	context;
	say("-----------------------------------------------------------------");
	say("Response: %s  (Type: %s)",r->name,rt->name);

	context;
	/* Display matches line */
	if(r->matches == NULL)
	{
		sayf(0,0,"Match  : No match lines set!!!  Use 'addrespmatch'");
	}
	else
	{
		i = 0;
		for(mr=r->matches; mr!=NULL; mr=mr->next)
		{
			i++;
			sprintf(pre,"Match %d: ",i);
			sayi(pre,"%s",mr->match);
		}
	}

	context;
	/* Display except lines */
	if(r->except != NULL)
	{
		i = 0;
		for(mr=r->except; mr!=NULL; mr=mr->next)
		{
			i++;
			sprintf(pre,"Excpt %d: ",i);
			sayi(pre,"%s",mr->match);
		}
	}

	context;
	say("-");
	i = 0;
	for(rl=r->lines; rl!=NULL; rl=rl->next)
	{
		i++;
		sprintf(pre,"%3d: ",i);
		sayi(pre,"%s",rl->text);
	}
	timet_to_date_short(r->last, date);
	say("");
	say("Last: %s", date);
	say("-----------------------------------------------------------------");
	context;
}

void cmd_addrespmatch()
{
	rtype	*rt;
	resprec	*r;
	char	newmatches[BUFSIZ];

	context;
	if(!cmdinfo.isparam)
	{
		say("Usage: addrespmatch <response name> <match> [#]");
		sayf(2,0,"Use this command to add matches to the specified response.  The match is added after the last element, so if the matchline looked like: 'foo, bar', this command would result in: 'foo, bar, newmatch'.  To create a new matchline, which evaluates on an AND basis, use the 'newrespmatch' command.  See help on 'response' and 'response-examples'.");
		return;
	}

	context;
	if(Response_response_checkall(cmdinfo.tok1,&rt,&r) == NULL)
	{
		sayf(0,0,"No such response name \"%s\".  Type 'showresponses' to see a listing of all response names.",cmdinfo.tok1);
		return;
	}

	/* 
	 * Levels of user must match this response type.  Command rules state
	 * that masters (+m) must have access to any command.
	 */
	if(!(cmdinfo.user->flags & USER_MASTER) &&
		!(cmdinfo.user->flags & rt->level))
	{
		say("You do not have the levels required to make changes to responses of this type. (type: %s)",rt->name);
		return;
	}

	context;
	if(!cmdinfo.istok2)
	{
		say("Usage: addrespmatch <response name> <matchline> [#]");
		sayf(2,0,"Use this command to add matches to the specified response.  The match is added after the last element, so if the matchline looked like: 'foo, bar', this command would result in: 'foo, bar, newmatch'.  To create a new matchline, which evaluates on an AND basis, use the 'newrespmatch' command.  See help on 'response' and 'response-examples'.");
		return;
	}	

	context;
	/* add it to the first matchline */
	split(NULL,cmdinfo.param);
	if(r->num_matches == 0)
		Response_addmatches(r,cmdinfo.param);
	else if(r->matches != NULL)
	{
		context;
		sprintf(newmatches,"%s, %s",r->matches->match,cmdinfo.param);
		machrec_setmatch(r->matches,newmatches);
	}
	else
	{
		say("Couldn't process instruction.");
		Log_write(LOG_ERROR,"*","num_matches says 0 but r->matches != NULL! (%s)",r->name);
		return;
	}

	context;
	sayf(0,0,"Response matchline added to the \"%s\" response.  (%d chars).",r->name,strlen(cmdinfo.param));

	context;
	Response->flags |= RESPONSE_NEEDS_WRITE;
}
	
void cmd_newrespmatch()
{
	rtype	*rt;
	resprec	*r;

	context;
	if(!cmdinfo.isparam)
	{
		say("Usage: newrespmatch <response name> <match>");
		sayf(2,0,"Use this command to add another matchline to the specified response.  If you want to add another match word to the matchline already in place, use the 'addrespmatch' command.");
		return;
	}

	context;
	if(Response_response_checkall(cmdinfo.tok1,&rt,&r) == NULL)
	{
		sayf(0,0,"No such response name \"%s\".  Type 'showresponses' to see a listing of all response names.",cmdinfo.tok1);
		return;
	}

	/* 
	 * Levels of user must match this response type.  Command rules state
	 * that masters (+m) must have access to any command.
	 */
	if(!(cmdinfo.user->flags & USER_MASTER) &&
		!(cmdinfo.user->flags & rt->level))
	{
		say("You do not have the levels required to make changes to responses of this type. (type: %s)",rt->name);
		return;
	}

	context;
	if(!cmdinfo.istok2)
	{
		say("Usage: newrespmatch <response name> <match>");
		sayf(2,0,"Use this command to add another matchline to the specified response.  If you want to add another match word to the matchline already in place, use the 'addrespmatch' command.");
		return;
	}	

	context;
	/* add this matchline */
	split(NULL,cmdinfo.param);
	Response_addmatches(r,cmdinfo.param);

	context;
	say("Response matchline added to the \"%s\" response.  (%d chars).",r->name,strlen(cmdinfo.param));

	context;
	Response->flags |= RESPONSE_NEEDS_WRITE;
}
	
void cmd_addrespexcept()
{
	rtype	*rt;
	resprec	*r;

	context;
	if(!cmdinfo.isparam)
	{
		say("Usage: addrespexcept <response name> <exceptline>");
		sayf(2,0,"Use this command to add an except line to the specified response.  An except line looks like \"foo, bar, foobar\" which evaluates to \"foo\" or \"bar\" or \"foobar\", so if any of these terms exist in the text to be evaluated, this response will NOT be executed.  Multiple exceptlines evalute on an AND basis.  See help on 'addrespexcept' for more details.");
		return;
	}

	context;
	if(Response_response_checkall(cmdinfo.tok1,&rt,&r) == NULL)
	{
		sayf(0,0,"No such response name \"%s\".  Type 'showresponses' to see a listing of all response names.",cmdinfo.tok1);
		return;
	}

	/* 
	 * Levels of user must match this response type.  Command rules state
	 * that masters (+m) must have access to any command.
	 */
	if(!(cmdinfo.user->flags & USER_MASTER) &&
		!(cmdinfo.user->flags & rt->level))
	{
		say("You do not have the levels required to make changes to responses of this type. (type: %s)",rt->name);
		return;
	}

	context;
	if(!cmdinfo.istok2)
	{
		say("Usage: addrespexcept <response name> <exceptline>");
		sayf(2,0,"Use this command to add an except line to the specified response.  An except line looks like \"foo, bar, foobar\" which evaluates to \"foo\" or \"bar\" or \"foobar\", so if any of these terms exist in the text to be evaluated, this response will NOT be executed.  Multiple exceptlines evalute on an AND basis.  See help on 'addrespexcept' for more details.");
		return;
	}	

	context;
	split(NULL,cmdinfo.param);
	Response_addexcept(r,cmdinfo.param);

	say("Response exceptline added to the \"%s\" response.  (%d chars).",r->name,strlen(cmdinfo.param));

	Response->flags |= RESPONSE_NEEDS_WRITE;
}
	
void cmd_addrespline()
{
	rtype	*rt;
	resprec	*r;

	context;
	if(!cmdinfo.isparam)
	{
		say("Usage: addrespline <response name> <response line>");
		sayf(2,0,"Use this command to add some lines to a response.  (Note that a 'response' refers to a collection of response lines - ie Skinny_Puppy is a 'response', and the text which will be displayed are the 'response lines'.)  To create a new response, type 'newresponse'.  Please doublecheck response lines to make sure you're not duplicating an existing line, and try to keep the format consistent.");
		return;
	}

	context;
	if(Response_response_checkall(cmdinfo.tok1,&rt,&r) == NULL)
	{
		sayf(0,0,"No such response name \"%s\".  Type 'showresponses' to see a listing of all response names.",cmdinfo.tok1);
		return;
	}

	/* 
	 * Levels of user must match this response type.  Command rules state
	 * that masters (+m) must have access to any command.
	 */
	if(!(cmdinfo.user->flags & USER_MASTER) &&
		!(cmdinfo.user->flags & rt->level))
	{
		say("You do not have the levels required to make changes to responses of this type. (type: %s)",rt->name);
		return;
	}

	context;
	if(!cmdinfo.istok2)
	{
		say("Usage: addrespline <response name> <response line>");
		sayf(2,0,"Use this command to add some lines to a response.  (Note that a 'response' refers to a collection of response lines - ie Skinny_Puppy is a 'response', and the text which will be displayed are the 'response lines'.)  To create a new response, type 'newresponse'.  Please doublecheck response lines to make sure you're not duplicating an existing line, and try to keep the format consistent.");
		return;
	}

	context;
	split(NULL,cmdinfo.param);
	Response_addline(r,cmdinfo.param);

	sayf(0,0,"Response line added to the \"%s\" response.  (%d chars).",r->name,strlen(cmdinfo.param));

	Response->flags |= RESPONSE_NEEDS_WRITE;
}

void cmd_newresponse()
{
	resprec	*r_dummy, *r;
	rtype	*rt, *rt_dummy;
	char	list[BUFSIZ];

	context;
	if(!cmdinfo.isparam)
	{
		say("Usage: newresponse <response type> <response name>");
		sayf(2,0,"Use this command to create a new response.  Note that a 'response' refers to a collection of response lines - ie Skinny_Puppy is a 'response', and the text which will be displayed are the 'response lines'.  To add lines to an existing response, use the command 'addresplines'.");
		say("");
		say("Note that response names must be a single word with no spaces.");
		return;
	}

	context;
	rt = Response_type(cmdinfo.tok1);
	if(rt == NULL)
	{
		Response_makertypelist(list);
		sayf(0,0,"The response type \"%s\" does not exist.  Valid response types are: %s.",cmdinfo.tok1,list);
		return;
	}

	/* 
	 * Levels of user must match this response type.  Command rules state
	 * that masters (+m) must have access to any command.
	 */
	if(!(cmdinfo.user->flags & USER_MASTER) &&
		!(cmdinfo.user->flags & rt->level))
	{
		say("You do not have the levels required to make changes to responses of this type. (type: %s)",rt->name);
		return;
	}

	context;
	if(!cmdinfo.istok2)
	{
		say("Usage: newresponse <response type> <response name>");
		sayf(2,0,"Use this command to create a new response.  Note that a 'response' refers to a collection of response lines - ie Skinny_Puppy is a 'response', and the text which will be displayed are the 'response lines'.  To add lines to an existing response, use the command 'addresplines'.");
		return;
	}

	context;
	if(Response_response_checkall(cmdinfo.tok2,&rt_dummy,&r_dummy) != NULL)
	{
		sayf(0,0,"The response \"%s\" already exists!",cmdinfo.tok2);
		return;
	}

	/* OK.  type is valid, and response name is unique. */
	r = Response_addnewresponse(rt,cmdinfo.tok2);

	context;
	sayf(0,0,"The response \"%s\" has been added under the response type \"%s\".",cmdinfo.tok2,rt->name);
	say("");
	sayf(0,0,"You'll need to add matches and response lines to that response.  Use the commands 'addrespmatch' and 'addresplines' to do this.  See help on 'response' for more details.");
	Response->flags |= RESPONSE_NEEDS_WRITE;

	if(Grufti->send_notice_response_created)
		Note_sendnotification("%s has created the %s response \"%s\".",cmdinfo.user->acctname,rt->name,cmdinfo.tok2);

#if 0
	Response_addhistory(r, cmdinfo.user->acctname, time(NULL), "Response created");
#endif
}

void cmd_delresponse()
{
	rtype	*rt;
	resprec	*r;

	if(!cmdinfo.isparam)
	{
		say("Usage: delresponse <response name>");
		sayf(2,0,"Use this command to delete an entire response.");
		return;
	}

	if(Response_response_checkall(cmdinfo.tok1,&rt,&r) == NULL)
	{
		sayf(0,0,"No such response name \"%s\".  Type 'showresponses' to view all possibilities.",cmdinfo.tok1);
		return;
	}

	/* 
	 * Levels of user must match this response type.  Command rules state
	 * that masters (+m) must have access to any command.
	 */
	if(!(cmdinfo.user->flags & USER_MASTER) &&
		!(cmdinfo.user->flags & rt->level))
	{
		say("You do not have the levels required to make changes to responses of this type. (type: %s)",rt->name);
		return;
	}

	if(cmdinfo.istok2)
	{
		say("Usage: delresponse <response name>");
		sayf(2,0,"Use this command to delete an entire response.");
		return;
	}

	if(Grufti->send_notice_response_deleted)
		Note_sendnotification("%s has deleted the %s response \"%s\".",cmdinfo.user->acctname,rt->name,r->name);

	say("The %s response \"%s\" has been deleted.",rt->name,r->name);

	Response_deleteresponse(rt,r);
	Response->flags |= RESPONSE_NEEDS_WRITE;
}

void cmd_delrespmatch()
{
	int	num = 0, num_deleted;
	char	eachnum[BUFSIZ];
	rtype	*rt;
	resprec	*r;
	machrec	*mr;

	if(!cmdinfo.isparam)
	{
		say("Usage: delrespmatch <response name> <match #>");
		sayf(2,0,"Use this command to delete response matches from a response.");
		return;
	}

	if(Response_response_checkall(cmdinfo.tok1,&rt,&r) == NULL)
	{
		sayf(0,0,"No such response name \"%s\".  Type 'showresponses' to view all possibilities.",cmdinfo.tok1);
		return;
	}

	/* 
	 * Levels of user must match this response type.  Command rules state
	 * that masters (+m) must have access to any command.
	 */
	if(!(cmdinfo.user->flags & USER_MASTER) &&
		!(cmdinfo.user->flags & rt->level))
	{
		say("You do not have the levels required to make changes to responses of this type. (type: %s)",rt->name);
		return;
	}

	context;
	if(!cmdinfo.istok2)
	{
		say("Usage: delrespmatch <response name> <match #>");
		sayf(2,0,"Use this command to delete response matches from a response.");
		return;
	}

	context;
	split(NULL,cmdinfo.param);
	split(eachnum,cmdinfo.param);
	while(eachnum[0])
	{
		if(!isanumber(eachnum))
		{
			split(eachnum,cmdinfo.param);
			continue;
		}
		else
			num = atoi(eachnum);

		if(num <= 0 || num > r->num_matches)
		{
			split(eachnum,cmdinfo.param);
			continue;
		}

		/* Ok, got response, and number works.  Delete it. */
		mr = Response_matchbyindex(r,num);
		if(mr == NULL)
		{
			Log_write(LOG_ERROR,"*","Match returned by Response_linebyindex is NULL.  (number is %d)",num);
			split(eachnum,cmdinfo.param);
			continue;
		}

		/* mark for delete */
		mr->flags |= MACHREC_DELETEME;

		split(eachnum,cmdinfo.param);
	}

	context;
	num_deleted = Response_deletemarkedmatches(r);
	if(!num_deleted)
		say("No response matches deleted!");
	else if(num_deleted == 1)
		say("Response match %d deleted!",num);
	else
		say("%d Response matches deleted!",num_deleted);

	Response->flags |= RESPONSE_NEEDS_WRITE;
}

void cmd_delrespexcept()
{
	int	num = 0, num_deleted;
	char	eachnum[BUFSIZ];
	rtype	*rt;
	resprec	*r;
	machrec	*mr;

	if(!cmdinfo.isparam)
	{
		say("Usage: delrespexcept <response name> <except #>");
		sayf(2,0,"Use this command to delete response excepts from a response.");
		return;
	}

	if(Response_response_checkall(cmdinfo.tok1,&rt,&r) == NULL)
	{
		sayf(0,0,"No such response name \"%s\".  Type 'showresponses' to view all possibilities.",cmdinfo.tok1);
		return;
	}

	/* 
	 * Levels of user must match this response type.  Command rules state
	 * that masters (+m) must have access to any command.
	 */
	if(!(cmdinfo.user->flags & USER_MASTER) &&
		!(cmdinfo.user->flags & rt->level))
	{
		say("You do not have the levels required to make changes to responses of this type. (type: %s)",rt->name);
		return;
	}

	context;
	if(!cmdinfo.istok2)
	{
		say("Usage: delrespexcept <response name> <except #>");
		sayf(2,0,"Use this command to delete response excepts from a response.");
		return;
	}

	context;
	split(NULL,cmdinfo.param);
	split(eachnum,cmdinfo.param);
	while(eachnum[0])
	{
		if(!isanumber(eachnum))
		{
			split(eachnum,cmdinfo.param);
			continue;
		}
		else
			num = atoi(eachnum);

		if(num <= 0 || num > r->num_excepts)
		{
			split(eachnum,cmdinfo.param);
			continue;
		}

		/* Ok, got response, and number works.  Delete it. */
		mr = Response_exceptbyindex(r,num);
		if(mr == NULL)
		{
			Log_write(LOG_ERROR,"*","Except returned by Response_linebyindex is NULL.  (number is %d)",num);
			split(eachnum,cmdinfo.param);
			continue;
		}

		/* mark for delete */
		mr->flags |= MACHREC_DELETEME;

		split(eachnum,cmdinfo.param);
	}

	context;
	num_deleted = Response_deletemarkedexcepts(r);
	if(!num_deleted)
		say("No response excepts deleted!");
	else if(num_deleted == 1)
		say("Response except %d deleted!",num);
	else
		say("%d Response excepts deleted!",num_deleted);

	Response->flags |= RESPONSE_NEEDS_WRITE;
}

void cmd_delrespline()
{
	int	num = 0, num_deleted;
	char	eachnum[BUFSIZ];
	rtype	*rt;
	resprec	*r;
	rline	*rl;

	context;
	if(!cmdinfo.isparam)
	{
		say("Usage: delrespline <response name> <line #>");
		sayf(2,0,"Use this command to delete responses lines from a response.");
		return;
	}

	context;
	if(Response_response_checkall(cmdinfo.tok1,&rt,&r) == NULL)
	{
		sayf(0,0,"No such response name \"%s\".  Type 'showresponses' to view all possibilities.",cmdinfo.tok1);
		return;
	}

	/* 
	 * Levels of user must match this response type.  Command rules state
	 * that masters (+m) must have access to any command.
	 */
	if(!(cmdinfo.user->flags & USER_MASTER) &&
		!(cmdinfo.user->flags & rt->level))
	{
		say("You do not have the levels required to make changes to responses of this type. (type: %s)",rt->name);
		return;
	}

	context;
	if(!cmdinfo.istok2)
	{
		say("Usage: delrespline <response name> <line #>");
		sayf(2,0,"Use this command to delete responses lines from a response.");
		return;
	}

	context;
	split(NULL,cmdinfo.param);
	split(eachnum,cmdinfo.param);
	while(eachnum[0])
	{
		if(!isanumber(eachnum))
		{
			split(eachnum,cmdinfo.param);
			continue;
		}
		else
			num = atoi(eachnum);

		if(num <= 0 || num > r->num_lines)
		{
			split(eachnum,cmdinfo.param);
			continue;
		}

		/* Ok, got response, and number works.  Delete it. */
		rl = Response_linebyindex(r,num);
		if(rl == NULL)
		{
			Log_write(LOG_ERROR,"*","Line returned by Response_linebyindex is NULL.  (number is %d)",num);
			split(eachnum,cmdinfo.param);
			continue;
		}

		/* mark for delete */
		rl->flags |= RLINE_DELETEME;

		split(eachnum,cmdinfo.param);
	}

	context;
	num_deleted = Response_deletemarkedlines(r);
	if(!num_deleted)
		say("No response lines deleted!");
	else if(num_deleted == 1)
		say("Response line %d deleted from %s!",num,r->name);
	else
		say("%d Response lines deleted from %s!",num_deleted,r->name);

	Response->flags |= RESPONSE_NEEDS_WRITE;
}

void cmd_op()
{
	char	chname[BUFSIZ], nick[BUFSIZ];
	chanrec	*chan;
	membrec	*m;

	if(cmdinfo.tok1[0] == '#' || cmdinfo.tok1[0] == '&' || cmdinfo.tok1[0] == '+')
	{
		/* Channel is specified. */
		strcpy(chname,cmdinfo.tok1);
	
		/* check if nick and channel are given */
		if(cmdinfo.istok2)
			strcpy(nick,cmdinfo.tok2);
		else
			strcpy(nick,cmdinfo.from_n);
	
		chan = Channel_channel(chname);
		if(chan == NULL)
		{
			sayf(0,0,"The channel \"%s\" is not known to me.",chname);
			return;
		}
	
		/* got it.  check if i have ops */
		if(!Channel_ihaveops(chan))
		{
			say("Unable to op %s on %s.  I don't have ops there.",nick,chan->name);
			return;
		}
	
		/* ok. have ops.  if no nick, see if this user is on that channel */
		m = Channel_member(chan,nick);
		if(m == NULL)
		{
			say("I don't see %s on channel %s!",nick,chan->name);
			return;
		}
	
		/* Check if they're split from us */
		if(m->split)
		{
			char	tsplit[TIMESHORTLEN];
	
			timet_to_ago_short(m->split,tsplit);
			say("%s has been netsplit from me for %s!  Sorry.",m->nick,tsplit);
			return;
		}
	
		/* ok.  user is on channel.  op them */
		if(m->flags & MEMBER_OPER)
		{
			say("%s already has ops on %s!",m->nick,chan->name);
			return;
		}
		if(m->flags & MEMBER_PENDINGOP)
		{
			say("%s is pending ops from the server...",m->nick);
			return;
		}

		Channel_giveops(chan,m);
		say("Sending mode change +o for %s on %s.",m->nick,chan->name);

		return;
	}

	/* No channel is specified.  Op user on all channels */
	for(chan=Channel->channels; chan!=NULL; chan=chan->next)
	{
		context;
		/* check if nick and channel are given */
		if(cmdinfo.istok1)
			strcpy(nick,cmdinfo.tok1);
		else
			strcpy(nick,cmdinfo.from_n);
	
		context;
		/* is member on it? */
		m = Channel_member(chan,nick);
		if(m != NULL)
		{
			if(!(m->flags & MEMBER_OPER) && !(m->flags & MEMBER_PENDINGOP) && !m->split)
			{
				/* if i don't have ops, don't bother */
				if(!Channel_ihaveops(chan))
					say("Unable to op %s on %s.  I don't have ops there.",nick,chan->name);
				else
				{
					Channel_giveops(chan,m);
					say("Sending mode change +o for %s on %s.",m->nick,chan->name);
				}
			}
		}
	}
}

void cmd_whoami()
{
	sprintf(cmdinfo.tok1,"%s",cmdinfo.user->acctname);
	strcpy(cmdinfo.tok2,"");
	strcpy(cmdinfo.tok3,"");
	sprintf(cmdinfo.param,"%s",cmdinfo.user->acctname);
	cmdinfo.isparam = 1;
	cmdinfo.istok1 = 1;
	cmdinfo.istok2 = 0;
	cmdinfo.istok3 = 0;
	cmd_finger();
}

void cmd_nn()
{
	int	new;
	userrec	*u;

	/* No arguments.  Show num_notes for user */
	if(!cmdinfo.isparam && !isequal(cmdinfo.tok1,"-h"))
	{
		if(!cmdinfo.user->registered)
		{
			say("You must be registered to be able to receive Notes.");
			return;
		}
		
		/* Display number of Notes */
		new = Note_numnew(cmdinfo.user);
		if(!cmdinfo.user->num_notes)
			say("You have no new Notes.");
		else if(new)
			say("You have %d new Note%s. (%d total)",new,new==1?"":"s",cmdinfo.user->num_notes);
		else
			say("You have no new Notes. (%d total)",cmdinfo.user->num_notes);
		return;
	}

	if(!User_isknown(cmdinfo.tok1))
	{
		say("No such user \"%s\".",cmdinfo.tok1);
		return;
	}

	u = User_user(cmdinfo.tok1);
	if(!u->registered)
	{
		say("The nick \"%s\" is not registered.",cmdinfo.tok1);
		return;
	}

	/* Display number of Notes */
	new = Note_numnew(u);
	if(!u->num_notes)
		say("%s has no new Notes.",u->acctname);
	else if(new)
		say("%s has %d new Note%s. (%d total)",u->acctname,new,new==1?"":"s",u->num_notes);
	else
		say("%s has no new Notes. (%d total)",u->acctname,u->num_notes);
}

void cmd_users()
{
	userrec	*u;
	int	num_cols = 4;
	int	col, row, num_rows, tot, index, have_flags = 0, byacct = 0;
	u_long	flags;
	char	line[160], levels[FLAGSHORTLEN];

	context;
	line[0] = 0;
	if(cmdinfo.istok1)
	{
		if(isequal(cmdinfo.tok1,"-a"))
		{
			byacct = 1;
			if(cmdinfo.istok2)
			{
				have_flags = 1;
				User_str2flags(cmdinfo.tok2,&flags);
			}
		}
		else
		{
			have_flags = 1;
			User_str2flags(cmdinfo.tok1,&flags);
		}
	}

	if(byacct)
	{
		if(have_flags)
			tot = User_orderbyacctname_plusflags(flags);
		else
			tot = User_orderbyacctname();
	}
	else
	{
		if(have_flags)
			tot = User_orderbyreg_plusflags(flags);
		else
			tot = User_orderbyreg();
	}

	if(tot == 0)
	{
		if(have_flags)
		{
			User_flags2str(flags,levels);
			say("No users match levels \"%s\"",levels);
		}
		else
			say("No users.");
		return;
	}

	if(have_flags)
	{
		User_flags2str(flags,levels);
		say("%d user%s match%s levels \"%s\".",tot,tot==1?"":"s",tot==1?"es":"",levels);
	}
	else
		say("%d user%s.",tot,tot==1?"":"s");

	say("");

	/* Display 1 column if less than 20 entries */
	if(tot <= ENTRIES_FOR_ONE_COL)
	{
		say("   # Username");
		say("---- ---------");
		num_cols = 1;
	}
	else
	{
		say("   # Username       # Username       # Username       # Username");
		say("---- ---------   ---- ---------   ---- ---------   ---- ---------");
	}

	num_rows = tot / num_cols;
	if(tot % num_cols)
		num_rows++;

	for(row=1; row<=num_rows; row++)
	{
		for(col=1; col<=num_cols; col++)
		{
			index = (num_rows * (col-1)) +row;
			u = User_userbyorder(index);
			if(u)
				sprintf(line,"%s%4d %-9s   ",line,u->registered,u->acctname);
		}
		line[strlen(line) - 3] = 0;
		say("%s",line);
		line[0] = 0;
	}

	if(tot <= ENTRIES_FOR_ONE_COL)
		say("---- ---------");
	else
		say("---- ---------   ---- ---------   ---- ---------   ---- ---------");
}

void cmd_say()
{
	chanrec	*chan;

	if(!cmdinfo.isparam)
	{
		say("Usage: say <channel> <message>");
		sayf(2,0,"Use this command to have the bot say something to a channel.  Use 'act' to have the bot say an action.");
		return;
	}

	chan = Channel_channel(cmdinfo.tok1);
	if(chan == NULL)
	{
		sayf(0,0,"The channel \"%s\" is not known to me.",cmdinfo.tok1);
		return;
	}

	split(NULL,cmdinfo.param);
	Log_write(LOG_PUBLIC,chan->name,"<%s has ordered me to say the next message>",cmdinfo.user->acctname);
	sendprivmsg(chan->name,cmdinfo.param);
}

void cmd_act()
{
	chanrec	*chan;

	if(!cmdinfo.isparam)
	{
		say("Usage: act <channel> <message>");
		sayf(2,0,"Use this command to have the bot say an action to a channel.  Use 'say' to have the bot say a message normally.");
		return;
	}

	chan = Channel_channel(cmdinfo.tok1);
	if(chan == NULL)
	{
		sayf(0,0,"The channel \"%s\" is not known to me.",cmdinfo.tok1);
		return;
	}

	split(NULL,cmdinfo.param);
	Log_write(LOG_PUBLIC,chan->name,"<%s has ordered me to say the next message>",cmdinfo.user->acctname);
	sendaction(chan->name,cmdinfo.param);
}

void cmd_configinfo()
{
	Grufti_showconfig();
}

void cmd_watchlog()
{
	int	pos, type;
	chanrec	*chan;
	dccrec	*d;
	char	s[BUFSIZ];
	
	context;
	if(!cmdinfo.isparam)
	{
		int	watching = 0;

		/* Display which logs are being watched and by whom. */
		for(d=DCC->dcclist; d!=NULL; d=d->next)
		{
			if(!d->logtype)
				continue;

			s[0] = 0;
#ifdef RAW_LOGGING
			if(d->logtype & LOG_RAW)
				sprintf(s,"%sraw, ",s);
#else
			say("Raw logging is not defined for this build.");
#endif
			if(d->logtype & LOG_MAIN)
				sprintf(s,"%smain, ",s);
			if(d->logtype & LOG_PUBLIC)
				sprintf(s,"%spublic (%s), ",s,d->logchname);
			if(d->logtype & LOG_CMD)
				sprintf(s,"%scmd, ",s);
			if(d->logtype & LOG_ERROR)
				sprintf(s,"%serror, ",s);
			if(d->logtype & LOG_DEBUG)
				sprintf(s,"%sdebug, ",s);
	
			if(s[0])
			{
				s[strlen(s) - 2] = 0;
				sayf(0,0,"%s is watching log%s: %s",d->nick,strchr(s,',') != NULL?"s":"",s);
				watching = 1;
			}
		}
		if(!watching)
			say("No logs are being watched at this time.");
		return;
	}

	context;
	/* Check for + or - */
	if(cmdinfo.tok1[0] == '+')
		pos = 1;
	else if(cmdinfo.tok1[0] == '-')
		pos = 0;
	else
	{
		say("Usage: watchlog <+|-><type> [channel]");
		sayf(2,0,"The first argument must begin with + or - to signify whether the log should be turned on or off for watching.");
		return;
	}

	context;
	/* ok + or -.  now get type */
	if(isequal(cmdinfo.tok1+1,"public"))
		type = LOG_PUBLIC;
#ifdef RAW_LOGGING
	else if(isequal(cmdinfo.tok1+1,"raw"))
		type = LOG_RAW;
#endif
	else if(isequal(cmdinfo.tok1+1,"main"))
		type = LOG_MAIN;
	else if(isequal(cmdinfo.tok1+1,"cmd"))
		type = LOG_CMD;
	else if(isequal(cmdinfo.tok1+1,"error"))
		type = LOG_ERROR;
	else if(isequal(cmdinfo.tok1+1,"debug"))
		type = LOG_DEBUG;
	else
	{
		sayf(0,0,"The type \"%s\" is not a valid logtype.  Choose from raw, public, main, cmd, error, or debug.",cmdinfo.tok1+1);
		return;	
	}

	context;
	/* Make sure we got a channel on public logtype */
	if(type & LOG_PUBLIC)
	{
		if(!cmdinfo.istok2)
		{
			sayf(0,0,"You need to specify a channel for that type.");
			return;
		}

		chan = Channel_channel(cmdinfo.tok2);
		if(chan == NULL)
		{
			sayf(0,0,"The channel \"%s\" is not known to me.",cmdinfo.tok2);
			return;
		}

		/* ok mark channel */
		dccrec_setlogchname(cmdinfo.d,chan->name);
	}
	else
		dccrec_setlogchname(cmdinfo.d,"*");

	context;
	/* ok mark it. */
	if(pos && (cmdinfo.d->logtype & type))
	{
		say("You're already watching that log!");
		return;
	}
	if(!pos && !(cmdinfo.d->logtype & type))
	{
		say("You're not watching that log!");
		return;
	}

	context;
	if(pos)
	{
		cmdinfo.d->logtype |= type;
		if(type & LOG_PUBLIC)
			say("Now watching log type \"%s\" (%s).",cmdinfo.tok1+1,cmdinfo.d->logchname);
		else
			say("Now watching log type \"%s\".",cmdinfo.tok1+1);
	}
	else
	{
		say("No longer watching log %s.",cmdinfo.tok1+1);
		cmdinfo.d->logtype &= ~type;
	}
}

void cmd_files()
{
	struct dirent *dd;
	DIR	*dir;
	struct stat st;
	char	filename[256], n1[50];

	/* Need dcc for this */
	if(cmdinfo.d == NULL)
		return;

	dir = opendir(Grufti->xfer_files);
	if(dir == NULL)
	{
		Log_write(LOG_ERROR,"*","Unable to open directory \"%s\".",Grufti->xfer_files);
		say("Unable to access file transfer area.  Sorry.");
		return;
	}

	say("Filename                       Size");
	say("------------------------------ ----------");
	dd = readdir(dir);
	while(dd != NULL)
	{
		if(dd->d_name[0] != '.')
		{
			sprintf(filename,"%s/%s",Grufti->xfer_files,dd->d_name);
			stat(filename,&st);
			comma_number(n1,st.st_size);
			say("%-30s %10s",dd->d_name,n1);
		}
		dd = readdir(dir);
	}
	closedir(dir);
	say("------------------------------ ----------");
}

void cmd_send()
{
	char	filepath[256];
	FILE	*f;

	if(!cmdinfo.isparam)
	{
		say("Usage: send <filename>");
		sayf(2,0,"Use this command to have the bot send you a file.  To view which files are available, use the 'files' command.");
		return;
	}

	/* can't do send via telnet! */
	if(cmdinfo.d && cmdinfo.d->flags & CLIENT_TELNET_CHAT)
	{
		say("You can't receive a file over telnet!");
		return;
	}

	if(DCC->num_dcc >= Grufti->max_dcc_connections)
	{
		/* send to user "too full" */
		Log_write(LOG_MAIN | LOG_ERROR,"*","DCC connection limit reached. (%d)",DCC->num_dcc);
		return;
	}

	if(strchr(cmdinfo.tok1,'/'))
	{
		say("Filenames cannot contain '/'.");
		return;
	}

	if(cmdinfo.tok1[0] == '.')
	{
		say("Invalid filename.");
		return;
	}

	sprintf(filepath,"%s/%s",Grufti->xfer_files,cmdinfo.tok1);
	f = fopen(filepath,"r");
	if(f == NULL)
	{
		sayf(0,0,"No such file \"%s\".  Use 'files' to view which are available.",cmdinfo.tok1);
		return;
	}

	fclose(f);

	if(cmdinfo.d)
	{
		sayf(0,0,"Please type \"/msg %s send %s\".  I need to know the IRC nick to which I'm going to send the file.  I can't tell if it's the same nick which you connected with.",Server->currentnick,cmdinfo.tok1);
		return;
	}

	DCC_sendfile(cmdinfo.d,cmdinfo.from_n,cmdinfo.from_uh,cmdinfo.tok1);
}

void cmd_topic()
{
	chanrec	*chan;

	if(!cmdinfo.isparam)
	{
		say("Usage: topic <channel> [topic]");
		sayf(2,0,"Use this command to set the topic on a channel.  If no topic is given, the current topic is displayed.");
		return;
	}

	chan = Channel_channel(cmdinfo.tok1);
	if(chan == NULL)
	{
		sayf(0,0,"The channel \"%s\" is not known to me.",cmdinfo.tok1);
		return;
	}

	if(!cmdinfo.istok2)
	{
		if(chan->topic && chan->topic[0])
			say("Topic for %s: \"%s\"",chan->name,chan->topic);
		else
			say("No topic on %s.",chan->name);
		return;

	}

	split(NULL,cmdinfo.param);
	Server_write(PRIORITY_NORM,"TOPIC %s :%s",chan->name,cmdinfo.param);

	say("The topic on %s has been changed to \"%s\".",chan->name,cmdinfo.param);
}

void cmd_kick()
{
	chanrec	*chan;
	membrec	*m;
	char	nick[256], nicks[256], message[256];
	
	if(!cmdinfo.isparam)
	{
		say("Usage: kick [channel] <nick[,nick,...]> [message]");
		sayf(2,0,"Use this command to kick a user from the channel.  You can specify that multiple nicks be kicked in one blow, but make sure you don't put any spaces between the commas that separate the nicks.  If no channel is specified, user is kicked from all botted channels they are on.");
		return;
	}

	/* Channel is given */
	if(cmdinfo.tok1[0] == '#' || cmdinfo.tok1[0] == '&' || cmdinfo.tok1[0] == '+')
	{
		chan = Channel_channel(cmdinfo.tok1);
		if(chan == NULL)
		{
			sayf(0,0,"The channel \"%s\" is not known to me.",cmdinfo.tok1);
			return;
		}
	
		/* Check for ops */
		if(!Channel_ihaveops(chan))
		{
			say("Sorry, I don't have ops on channel %s.",chan->name);
			return;
		}
	
		if(!cmdinfo.istok2)
		{
			say("Usage: kick [channel] <nick[,nick,...]> [message]");
			sayf(2,0,"Use this command to kick a user from the channel.  You can specify that multiple nicks be kicked in one blow, but make sure you don't put any spaces between the commas that separate the nicks.  If no channel is specified, user is kicked from all botted channels they are on.");
			return;
		}
	
		if(!cmdinfo.istok3)
			strcpy(message,"Bye!");
		else
		{
			split(NULL,cmdinfo.param);
			split(NULL,cmdinfo.param);
			strcpy(message,cmdinfo.param);
		}
	
		/* Start kicking nicks */
		splitc(nick,cmdinfo.tok2,',');
		while(nick[0])
		{
			m = Channel_member(chan,nick);
			if(m == NULL)
				say("No such nick \"%s\" on %s...",nick,chan->name);
			else if(!isequal(m->nick,Server->currentnick))
			{
				Server_write(PRIORITY_HIGH,"KICK %s %s :%s",chan->name,m->nick,message);
				say("Kicked %s from %s...",nick,chan->name);
			}
	
			splitc(nick,cmdinfo.tok2,',');
		}

		return;
	}

	/* Extract message */
	if(!cmdinfo.istok2)
		strcpy(message,"Bye!");
	else
	{
		split(NULL,cmdinfo.param);
		strcpy(message,cmdinfo.param);
	}
	
	/* Channel is not given.  Check them all */
	for(chan=Channel->channels; chan!=NULL; chan=chan->next)
	{
		if(!Channel_ihaveops(chan))
			continue;
		
		/* Start kicking nicks */
		strcpy(nicks,cmdinfo.tok1);
		splitc(nick,nicks,',');
		while(nick[0])
		{
			m = Channel_member(chan,nick);
			if(m != NULL && !isequal(m->nick,Server->currentnick))
			{
				Server_write(PRIORITY_HIGH,"KICK %s %s :%s",chan->name,m->nick,message);
				say("Kicked %s from %s...",nick,chan->name);
			}
	
			splitc(nick,cmdinfo.tok2,',');
		}
	}
}

void cmd_invite()
{
	chanrec	*chan;
	membrec	*m;
	char	nick[256];
	
	if(!cmdinfo.isparam)
	{
		say("Usage: invite <channel> [nick]");
		sayf(2,0,"Use this command to invite yourself to a botted channel.  If a nick is given, the bot invites that nick.");
		return;
	}

	
	chan = Channel_channel(cmdinfo.tok1);
	if(chan == NULL)
	{
		sayf(0,0,"The channel \"%s\" is not known to me.",cmdinfo.tok1);
		return;
	}

	if(!(chan->flags & CHAN_ACTIVE))
	{
		if(chan->nojoin_reason)
			say("Couldn't join channel (%s).",chan->nojoin_reason);
		else
			say("Sorry, I'm not active yet on %s.",chan->name);
		return;
	}

	if(cmdinfo.istok2)
		strcpy(nick,cmdinfo.tok2);
	else
		strcpy(nick,cmdinfo.from_n);

	m = Channel_member(chan,nick);

	if(m != NULL)
	{
		if(isequal(nick,cmdinfo.from_n))
			say("You are already on %s!",chan->name);
		else
			say("%s is already on %s!",nick,chan->name);
		return;
	}

	if(!isequal(nick,cmdinfo.from_n))
		say("Invited %s to channel %s.",nick,chan->name);
	Server_write(PRIORITY_NORM,"INVITE %s %s",nick,chan->name);
}
	
void cmd_ban()
{
	chanrec	*chan;
	membrec	*m;
	char	nick[256], nicks[256], hm[UHOSTLEN];
	
	if(!cmdinfo.isparam)
	{
		say("Usage: ban [channel] <nick[,nick,...]>");
		sayf(2,0,"Use this command to institute a ban for someone on the specified channel.  Multiple nicks can be specified by using commas.");
		return;
	}

	if(cmdinfo.tok1[0] == '#' || cmdinfo.tok1[0] == '&' || cmdinfo.tok1[0] == '+')
	{
		chan = Channel_channel(cmdinfo.tok1);
		if(chan == NULL)
		{
			sayf(0,0,"The channel \"%s\" is not known to me.",cmdinfo.tok1);
			return;
		}
	
		if(!(chan->flags & CHAN_ACTIVE))
		{
			if(chan->nojoin_reason)
				say("Couldn't join channel (%s).",chan->nojoin_reason);
			else
				say("Sorry, I'm not active yet on %s.",chan->name);
			return;
		}
	
		if(!cmdinfo.istok2)
		{
			say("Usage: ban <channel> <nick>");
			sayf(2,0,"Use this command to ban someone from the specified channel.");
			return;
		}
	
		m = Channel_member(chan,cmdinfo.tok2);
		if(m == NULL)
		{
			say("No such nick \"%s\" on %s!",cmdinfo.tok2,chan->name);
			return;
		}
	
		/* check if we've already initiated a mode change */
		if(m->flags & MEMBER_PENDINGBAN)
		{
			say("Still waiting for the server to process original ban request...");
			return;
		}
	
		/* Ok start banning nicks */
		splitc(nick,cmdinfo.tok2,',');
		while(nick[0])
		{
			m = Channel_member(chan,nick);
			if(m == NULL)
				say("No such nick \"%s\" on channel %s...",nick,chan->name);
			else
			{
				makehostmask(hm,m->uh);
				Server_write(PRIORITY_HIGH,"MODE %s +b *!*%s",chan->name,hm+1);
				say("Instituted ban against %s (*!*%s) on %s...",nick,hm+1,chan->name);
				m->flags |= MEMBER_PENDINGBAN;
			}
	
			splitc(nick,cmdinfo.tok2,',');

		return;
		}
	}

	/* Channel is not given.  Check them all */
	for(chan=Channel->channels; chan!=NULL; chan=chan->next)
	{
		if(!Channel_ihaveops(chan))
			continue;
		
		/* Start nicks */
		strcpy(nicks,cmdinfo.tok1);
		splitc(nick,nicks,',');
		while(nick[0])
		{
			m = Channel_member(chan,nick);
			if(m != NULL)
			{
				makehostmask(hm,m->uh);
				Server_write(PRIORITY_HIGH,"MODE %s +b *!*%s",chan->name,hm+1);
				say("Banned %s (*!*%s) from %s...",nick,hm+1,chan->name);
			}
	
			splitc(nick,cmdinfo.tok2,',');
		}
	}
}

void cmd_kickban()
{
	chanrec	*chan;
	membrec	*m;
	char	nick[256], message[256], hm[UHOSTLEN], nicks[256];
	char 	q_nick[256], q_mode[UHOSTLEN * 3], n[NICKLEN], bbb[10];
	char	ooo[10];
	int	counter, i;
	
	if(!cmdinfo.isparam)
	{
		say("Usage: kickban [channel] <nick[,nick,...]> [message]");
		sayf(2,0,"Use this command to kickban a user from the channel.  You can specify that multiple nicks be kickbanned in one blow, but make sure you don't put any spaces between the commas that separate the nicks.  Also, if a channel is not specified, the user is kickbanned from all botted channels they are on.");
		return;
	}

	if(cmdinfo.tok1[0] == '#' || cmdinfo.tok1[0] == '&' || cmdinfo.tok1[0] == '+')
	{
		/* Channel is given */
		chan = Channel_channel(cmdinfo.tok1);
		if(chan == NULL)
		{
			sayf(0,0,"The channel \"%s\" is not known to me.",cmdinfo.tok1);
			return;
		}
	
		/* Check for ops */
		if(!Channel_ihaveops(chan))
		{
			say("Sorry, I don't have ops on channel %s.",chan->name);
			return;
		}
	
		if(!cmdinfo.istok2)
		{
			say("Usage: kickban [channel] <nick[,nick,...]> [message]");
			sayf(2,0,"Use this command to kickban a user from the channel.  You can specify that multiple nicks be kickbanned in one blow, but make sure you don't put any spaces between the commas that separate the nicks.  Also, if a channel is not specified, the user is kickbanned from all botted channels they are on.");
			return;
		}
	
		if(!cmdinfo.istok3)
			strcpy(message,"Bye!");
		else
		{
			split(NULL,cmdinfo.param);
			split(NULL,cmdinfo.param);
			strcpy(message,cmdinfo.param);
		}
	
		/* Start kickbanning nicks */
		splitc(nick,cmdinfo.tok2,',');
		while(nick[0])
		{
			m = Channel_member(chan,nick);
			if(m == NULL)
				say("No such nick \"%s\" on %s...",nick,chan->name);
			else if(!isequal(nick,Server->currentnick))
			{
				makehostmask(hm,m->uh);
				Server_write(PRIORITY_HIGH,"MODE %s -o+b %s *!*%s",chan->name,m->nick,hm+1);
				Server_write(PRIORITY_HIGH,"KICK %s %s :%s",chan->name,m->nick,message);
				say("Kickbanned %s (*!*%s) from %s...",nick,hm+1,chan->name);
			}
	
			splitc(nick,cmdinfo.tok2,',');
		}
		return;
	}

	/* Extract message */
	if(!cmdinfo.istok2)
		strcpy(message,"Bye!");
	else
	{
		split(NULL,cmdinfo.param);
		strcpy(message,cmdinfo.param);
	}
	
	/* Channel is not given.  Check them all */
	for(chan=Channel->channels; chan!=NULL; chan=chan->next)
	{
		if(!Channel_ihaveops(chan))
			continue;
		
		/* Start kickbanning nicks */
		counter = 0;
		q_nick[0] = 0;
		q_mode[0] = 0;
		strcpy(nicks,cmdinfo.tok1);
		splitc(nick,nicks,',');
		while(nick[0])
		{
			m = Channel_member(chan,nick);
			if(m != NULL && !isequal(m->nick,Server->currentnick))
			{
				counter++;
				makehostmask(hm,m->uh);
				sprintf(q_nick,"%s%s ",q_nick,m->nick);
				sprintf(q_mode,"%s*!*%s ",q_mode,hm+1);

				if(counter >= 3)
				{
					Server_write(PRIORITY_HIGH,"MODE %s -ooo+bbb %s %s",chan->name,q_nick,q_mode);
					q_mode[0] = 0;

					split(n,q_nick);
					while(n[0])
					{
						Server_write(PRIORITY_HIGH,"KICK %s %s :%s",chan->name,n,message);
						say("Kickbanned %s from %s...",n,chan->name);
						split(n,q_nick);
					}
					counter = 0;
					q_nick[0] = 0;
				}
			}
	
			splitc(nick,nicks,',');
		}

		if(counter)
		{
			bbb[0] = 0;
			ooo[0] = 0;
			for(i=1; i<=counter; i++)
			{
				sprintf(bbb,"%sb",bbb);
				sprintf(ooo,"%so",ooo);
			}

			Server_write(PRIORITY_HIGH,"MODE %s -%s+%s %s %s",chan->name,ooo,bbb,q_nick,q_mode);
			split(n,q_nick);
			while(n[0])
			{
				Server_write(PRIORITY_HIGH,"KICK %s %s :%s",chan->name,n,message);
				say("Kickbanned %s from %s...",n,chan->name);
				split(n,q_nick);
			}
		}
	}
}

void cmd_stat()
{
	int	seen_people = 0, cmds_people = 0, cmds = 0, via_web = 0;
	int	avg_counter = 0, num_hostmasks = 0, num_uhostmasks = 0;
	int	num_notes = 0, num_note_users = 0, num_notes_sent = 0;
	int	joins = 0, joins_reg = 0, chatter = 0, i;
	int	resp_counter = 0, tot, usage_people = 0;
	int	top_kicks = 0, top_bans = 0;
	char	date[TIMELONGLEN], online_time[TIMELONGLEN], memory_used[25];
	char	when_cleaned[TIMELONGLEN], out[512];
	char	life[25], b_in[25], b_out[25], avg_lag[TIMESHORTLEN];
	generec	*g;
	userrec	*u;
	servrec	*serv;
	noterec	*note;
	chanrec	*chan;
	resprec	*r;
	rtype	*rt;
	time_t	t, now = time(NULL), top_usage = 0L, top_chatter = 0L;
	int	serv_connected = 0;
	float	avg_note_age = 0, avg_user_age = 0;
	float	avg_last_seen = 0, avg_usage = 0, avg_usage_partial;
	float	avg_last_login = 0;
	int	login_people = 0;

	/* If arguments, stat argument type */
	if(cmdinfo.isparam)
	{
		if(isequal(cmdinfo.tok1,"usage"))
		{
			/* Determine top users based on usage */
			for(u=User->userlist; u!=NULL; u=u->next)
			{
				if(!u->registered || (u->flags & USER_BOT))
					continue;
		
				top_usage = User_timeperweek(u);
		
				Notify_addtogenericlist(u->acctname,top_usage);
			}

			tot = Notify_orderbylogin();
			if(tot)
			{
				say("Top 10 users based on IRC usage this week (hrs/day):");
				out[0] = 0;
				for(i=1; i<=10; i++)
				{
					g = Notify_genericbyorder(i);
					if(g == NULL)
						continue;
			
					sprintf(out,"%s%s %2.1f, ",out,g->display,(float)(g->login) / (3600.0 * 7.0));
				}
				out[strlen(out) - 2] = 0;
			}

			Notify_freeallgenerics();

			sayf(2,0,out);
		}
		else if(isequal(cmdinfo.tok1,"chatter"))
		{
			/* Determine top users based on usage */
			for(u=User->userlist; u!=NULL; u=u->next)
			{
				if(!u->registered || (u->flags & USER_BOT))
					continue;
	
				top_chatter = 0L;
				for(i=0; i<7; i++)
					top_chatter += u->public_chatter[i];
	
				Notify_addtogenericlist(u->acctname,top_chatter);
			}
	
			tot = Notify_orderbylogin();
			if(tot)
			{
				say("Top 10 users based on public chatter this week (kb/day):");
				out[0] = 0;
				for(i=1; i<=10; i++)
				{
					g = Notify_genericbyorder(i);
					if(g == NULL)
						continue;
			
					sprintf(out,"%s%s %luk, ",out,g->display,g->login/(1024*7));
				}
				out[strlen(out) - 2] = 0;
			}
	
			Notify_freeallgenerics();

			sayf(2,0,out);
		}
		else if(isequal(cmdinfo.tok1,"kicks"))
		{
			/* Determine top users based on usage */
			for(u=User->userlist; u!=NULL; u=u->next)
			{
				if(!u->registered || (u->flags & USER_BOT))
					continue;
	
				top_kicks = 0;
				for(i=0; i<7; i++)
					top_kicks += u->kicks[i];
	
				Notify_addtogenericlist(u->acctname,(time_t)top_kicks);
			}
	
			tot = Notify_orderbylogin();
			if(tot)
			{
				say("Top 10 users based on kicks this week (kicks/day):");
				out[0] = 0;
				for(i=1; i<=10; i++)
				{
					g = Notify_genericbyorder(i);
					if(g == NULL)
						continue;
			
					sprintf(out,"%s%s %lu, ",out,g->display,g->login/7);
				}
				out[strlen(out) - 2] = 0;
			}
	
			Notify_freeallgenerics();

			sayf(2,0,out);
		}
		else if(isequal(cmdinfo.tok1,"bans"))
		{
			/* Determine top users based on usage */
			for(u=User->userlist; u!=NULL; u=u->next)
			{
				if(!u->registered || (u->flags & USER_BOT))
					continue;
	
				top_bans = 0;
				for(i=0; i<7; i++)
					top_bans += u->bans[i];
	
				Notify_addtogenericlist(u->acctname,(time_t)top_bans);
			}
	
			tot = Notify_orderbylogin();
			if(tot)
			{
				say("Top 10 users based on bans this week (bans/day):");
				out[0] = 0;
				for(i=1; i<=10; i++)
				{
					g = Notify_genericbyorder(i);
					if(g == NULL)
						continue;
			
					sprintf(out,"%s%s %lu, ",out,g->display,g->login/7);
				}
				out[strlen(out) - 2] = 0;
			}
	
			Notify_freeallgenerics();

			sayf(2,0,out);
		}
		else
		{
			u = User_user(cmdinfo.tok1);
			if(!u->registered)
			{
				say("No such stat type \"%s\".  (Use \"usage\", \"chatter\", \"kicks\", \"bans\", or <nick>)",cmdinfo.tok1);
			}
			else
			{
				struct tm *btime;
				time_t	now, t;

				now = time(NULL);
				btime = localtime(&now);

				for(i=0; i<7; i++)
				{
					top_chatter += u->public_chatter[i];
					top_usage = User_timeperweek(u);
					top_kicks += u->kicks[i];
					top_bans += u->bans[i];
				}
	
				User_makeircfreq(life,u);
	
				say("Stat for %s",u->acctname);
				say("%2.1f hrs/day (%s), %luk chatter/day, %d kick%s/day, %d ban%s/day",(float)(top_usage)/(3600.0*7.0),life,top_chatter/(1024*7),top_kicks/7,top_kicks/7==1?"":"s",top_bans/7,top_bans/7==1?"":"s");
				say("");
				strcpy(out,"             Sun  Mon  Tue  Wed  Thu  Fri  Sat     AVERAGE");
				if(btime->tm_wday >= 0 && btime->tm_wday <= 6 && strlen(out) >= 42)
				{
					out[(btime->tm_wday * 5) + 12] = '(';
					out[(btime->tm_wday * 5) + 16] = ')';
				}

				say("%s",out);
				strcpy(out,"Usage (hrs) ");
				for(i=0; i<7; i++)
				{
					t = u->time_seen[i];
					if(btime->tm_wday == i)
					{
						if(u->first_activity)
							t += (now - u->first_activity);
					}

					sprintf(out,"%s%4.1f ",out,(float)t / 3600.0);
				}
				sprintf(out,"%s    %4.1f hrs/day",out,(float)(top_usage)/(3600.0*7.0));
				say("%s",out);

				strcpy(out,"Chatter (k) ");
				for(i=0; i<7; i++)
					sprintf(out,"%s%4.1f ",out,(float)u->public_chatter[i] / 1024.0);
				sprintf(out,"%s    %4.1f kb/day",out,(float)top_chatter / (1024.0 * 7.0));
				say("%s",out);

				strcpy(out,"Kicks       ");
				for(i=0; i<7; i++)
					sprintf(out,"%s%4d ",out,u->kicks[i]);
				sprintf(out,"%s    %4.1f kicks/day",out,(float)top_kicks / 7.0);
				say("%s",out);

				strcpy(out,"Bans        ");
				for(i=0; i<7; i++)
					sprintf(out,"%s%4d ",out,u->bans[i]);
				sprintf(out,"%s    %4.1f bans/day",out,(float)top_bans / 7.0);
				say("%s",out);
			}

		}
		return;
	}

	/* List all the statistics for the day */
	timet_to_dateonly_short(now,date);
	say("Summary for %s:",date);
	timet_to_ago_long(Grufti->online_since,online_time);
	comma_number(memory_used,net_sizeof() + misc_sizeof() + gruftiaux_sizeof() + blowfish_sizeof() + main_sizeof() + fhelp_sizeof() + User_sizeof() + URL_sizeof() + Server_sizeof() + Response_sizeof() + Notify_sizeof() + Log_sizeof() + Location_sizeof() + Grufti_sizeof() + DCC_sizeof() + command_sizeof() + Codes_sizeof() + Channel_sizeof());
	say("Online: %s      Memory: %s bytes",online_time,memory_used);
	say("");
	if(Grufti->cleanup_time == 0L)
		say("  No cleanup information is available");
	else
	{
		timet_to_date_short(Grufti->cleanup_time,when_cleaned);
		say("  Auto Cleanup Summary (%s)",when_cleaned);
		say("    %d user%s deleted (timeout: %d days)",Grufti->users_deleted,Grufti->users_deleted==1 ? " was":"s were",Grufti->timeout_users);
		say("    %d hostmask%s deleted (timeout: %d days)",Grufti->accts_deleted,Grufti->accts_deleted==1 ? " was":"s were",Grufti->timeout_accounts);
		say("    %d note%s deleted (timeout: %d days)",Grufti->notes_deleted,Grufti->notes_deleted==1 ? " was":"s were",Grufti->timeout_notes);
	}
	say("");

	context;
	t = time_today_began(time(NULL));
	for(u=User->userlist; u!=NULL; u=u->next)
	{
		context;
		if(u->registered)
			num_hostmasks += u->num_accounts;
		else
			num_uhostmasks += u->num_accounts;

		context;
		if(!u->registered)
			continue;

		context;
		if(u->bot_commands)
		{
			cmds_people++;
			cmds += u->bot_commands;
		}

		context;
		if(u->via_web)
			via_web++;

		context;
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

		context;
		avg_last_seen += ((float)(now - u->last_seen) / 86400.0);
		avg_last_login += ((float)(now - u->last_login) / 86400.0);
		avg_user_age += ((float)(now - u->time_registered) / 86400.0);

		context;

		context;
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

		context;
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

	say("  Overall User Statistics");
	say("    %d user%s registered (%d hostmask%s)",User->num_users - 1,User->num_users-1 == 1?" is":"s are",num_hostmasks,num_hostmasks==1?"":"s");
	say("    %d unregistered hostmasks are being tracked",num_uhostmasks);
	if(num_note_users)
		say("    %d Note%s being stored by %d user%s (%2.1f notes/user)",num_notes,num_notes==1?" is":"s are",num_note_users,num_note_users==1?"":"s",(float)(num_notes)/(float)(num_note_users));
	else
		say("    %d Note%s being stored by %d user%s",num_notes,num_notes==1?" is":"s are",num_note_users,num_note_users==1?"":"s");
	say("    %d Note%s been sent today, %d %s forwarded",num_notes_sent,num_notes_sent==1?" has":"s have", Grufti->notes_forwarded,Grufti->notes_forwarded==1?"was":"were");
	say("    Average %s note is %2.1f days old",Grufti->botnick,avg_note_age);
	say("    Average %s user account is %2.1f days old",Grufti->botnick,avg_user_age);
	say("    Average %s user was seen %2.1f days ago",Grufti->botnick,avg_last_seen);
	say("    Average %s user last logged in %2.1f days ago",Grufti->botnick,avg_last_login);
	say("    Average time a %s user is on IRC is %2.1f hrs/day",Grufti->botnick,avg_usage);
	say("");

	say("  Of the %d registered user%s:",User->num_users - 1, (User->num_users - 1) == 1? "":"s");
	say("    %d %s queried the bot via IRC today (%d command%s)",cmds_people,cmds_people==1?"has":"have",cmds,cmds==1?"":"s");
	say("    %d %s logged in from the web or via telnet",via_web,via_web==1?"has":"have");
	say("    %d %s logged in to their account in general",login_people,login_people==1?"has":"have");
	say("    %d %s been seen online",seen_people,seen_people==1?"has":"have");
	say("");

	/* Determine top users based on usage */
	for(u=User->userlist; u!=NULL; u=u->next)
	{
		if(!u->registered || (u->flags & USER_BOT))
			continue;

		top_usage = User_timeperweek(u);

		Notify_addtogenericlist(u->acctname,top_usage);
	}

	tot = Notify_orderbylogin();
	if(tot)
	{
		say("  Top users based on IRC usage this week (hrs/day):");
		strcpy(out,"    ");
		for(i=1; i<=4; i++)
		{
			g = Notify_genericbyorder(i);
			if(g == NULL)
				continue;

			sprintf(out,"%s%s %2.1f, ",out,g->display,(float)(g->login) / (3600.0 * 7.0));
		}
		out[strlen(out) - 2] = 0;
		if(strlen(out) > 2)
			say("%s",out);

		strcpy(out,"    ");
		for(i=5; i<=8; i++)
		{
			g = Notify_genericbyorder(i);
			if(g == NULL)
				continue;

			sprintf(out,"%s%s %2.1f, ",out,g->display,(float)(g->login) / (3600.0 * 7.0));
		}
		out[strlen(out) - 2] = 0;
		if(strlen(out) > 2)
			say("%s",out);

		strcpy(out,"    ");
		for(i=9; i<=12; i++)
		{
			g = Notify_genericbyorder(i);
			if(g == NULL)
				continue;

			sprintf(out,"%s%s %2.1f, ",out,g->display,(float)(g->login) / (3600.0 * 7.0));
		}
		out[strlen(out) - 2] = 0;
		if(strlen(out) > 2)
			say("%s",out);
	}

	Notify_freeallgenerics();

	say("");
	/* Determine top users based on public chatter */
	for(u=User->userlist; u!=NULL; u=u->next)
	{
		if(!u->registered || (u->flags & USER_BOT))
			continue;

		top_chatter = 0;
		for(i=0; i<7; i++)
			top_chatter += u->public_chatter[i];

		Notify_addtogenericlist(u->acctname,top_chatter);
	}

	tot = Notify_orderbylogin();
	if(tot)
	{
		say("  Top users based on public chatter this week:");
		strcpy(out,"    ");
		for(i=1; i<=4; i++)
		{
			g = Notify_genericbyorder(i);
			if(g == NULL)
				continue;

			sprintf(out,"%s%s %luk, ",out,g->display,g->login/(1024 * 7));
		}
		out[strlen(out) - 2] = 0;
		if(strlen(out) > 2)
			say("%s",out);

		strcpy(out,"    ");
		for(i=5; i<=8; i++)
		{
			g = Notify_genericbyorder(i);
			if(g == NULL)
				continue;

			sprintf(out,"%s%s %luk, ",out,g->display,g->login/(1024 * 7));
		}
		out[strlen(out) - 2] = 0;
		if(strlen(out) > 2)
			say("%s",out);

		strcpy(out,"    ");
		for(i=9; i<=12; i++)
		{
			g = Notify_genericbyorder(i);
			if(g == NULL)
				continue;

			sprintf(out,"%s%s %luk, ",out,g->display,g->login/(1024 * 7));
		}
		out[strlen(out) - 2] = 0;
		if(strlen(out) > 2)
			say("%s",out);
	}

	Notify_freeallgenerics();
	say("");

	say("  Response Statistics");
	for(rt=Response->rtypes; rt!=NULL; rt=rt->next)
	{
		resp_counter = 0;
		for(r=rt->responses; r!=NULL; r=r->next)
		{
			if(r->last >= t)
				resp_counter++;
		}

		say("    %d of my %d %s response%s matched",resp_counter,rt->num_responses,rt->name,rt->num_responses==1?" was":"s were");
	}
	say("");

	say("  Channel Statistics");
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

		say("    %s had %d join%s (%d by reg'd user%s), %dk chatter",chan->name,joins,joins==1?"":"s",joins_reg,joins_reg==1?"":"s",chatter/1024);
	}
	say("");

	/* Display server statistics */
	for(serv=Server->serverlist; serv!=NULL; serv=serv->next)
		serv_connected += Server_timeconnected(serv);

	say("  Server Statistics (Online %2.1f hrs today)",(float)serv_connected / 3600.0);
	for(serv=Server->serverlist; serv!=NULL; serv=serv->next)
	{
		if(Server_timeconnected(serv))
		{
			bytes_to_kb_short(serv->bytes_in,b_in);
			bytes_to_kb_short(serv->bytes_out,b_out);
			time_to_elap_short(Server_avglag(serv),avg_lag);
			say("    %s - %2.1f hrs, %s in, %s out, %s lag",serv->name,(float)Server_timeconnected(serv)/3600.0,b_in,b_out,avg_lag);
		}
	}
	say("");
}

void cmd_banlist()
{
	banlrec	*b;
	char	ban[UHOSTLEN], date[DATESHORTLEN], type[25], chname[256];
	int	i, tot;
	time_t	until;

	tot = Banlist_orderbysettime();
	if(tot == 0)
	{
		say("Banlist is empty.");
		return;
	}

	say(" # Ban                                                 Date Set");
	say("-- --------------------------------------------------- ----------");

	for(i=1; i<=tot; i++)
	{
		b = Banlist_banbyordernum(i);
		if(b == NULL)
			continue;

		timet_to_dateonly_short(b->time_set,date);

		type[0] = 0;
		if(b->expires == 0L)
			strcpy(type,"perm");
		else
		{
			until = (b->time_set + b->expires) - time(NULL);
			if(until < 60)
				sprintf(type,"%lus",until);
			else if(until < 3600)
				sprintf(type,"%lum",until/60);
			else if(until < 86400)
				sprintf(type,"%luh",until/3600);
			else
				sprintf(type,"%lud",until/86400);
		}
		
		strcpy(ban,b->ban);
		if(strlen(ban) > 51)
			ban[51] = 0;

		if(b->chan == NULL)
			strcpy(chname,"(global)");
		else
			strcpy(chname,b->chan->name);

		if(strlen(chname) > 8)
			chname[8] = 0;
	
		say("%2d %-51s %-10s", i, ban, date);
		say("    Set by: %-9s  Channel: %-10s  Type: %-4s  Hits: %-3d",
			b->nick, chname, type, b->used);
	}
	say("-- --------------------------------------------------- ----------");
}

void cmd_addban()
{
	char	expires[512], tmp_uh[UHOSTLEN];
	time_t	expire_time = 0L;
	membrec	*m;
	chanrec	*chan = NULL;
	banlrec	*bl;

	if(!cmdinfo.isparam)
	{
		say("Usage: addban <ban> [channel] [expires]");
		sayf(2,0,"Use this command to add a ban to the banlist.  Ban format is nick!user@host.  If no channel is specified, the ban will be GLOBAL.  If no expire time is specified, the ban will be PERMANENT.  Expire times are in the format 30m, 2h, 10d, etc.");
		return;
	}

	expires[0] = 0;

	/* Verify ban is in correct format */
	if(!Banlist_verifyformat(cmdinfo.tok1))
	{
		sayf(0,0,"Incorrect ban format \"%s\".",cmdinfo.tok1);
		say("Use the format 'nick!user@hostname'.");
		return;
	}

	/* Make sure we're not duplicating bans */
	if(Banlist_ban(cmdinfo.tok1) != NULL)
	{
		say("Already have that ban in the banlist.");
		return;
	}

	/* Check for 2nd argument */
	if(cmdinfo.istok2)
	{
		if(cmdinfo.tok2[0] == '#' || cmdinfo.tok2[0] == '&' || cmdinfo.tok2[0] == '+')
		{
			/* Got channel.  check if it's valid */
			chan = Channel_channel(cmdinfo.tok2);
			if(chan == NULL)
			{
				say("Channel \"%s\" is not known to me.",cmdinfo.tok2);
				return;
			}

			/* If THIRD argument, set expires to it */
			if(cmdinfo.istok3)
				strcpy(expires,cmdinfo.tok3);
		}
		else
			strcpy(expires,cmdinfo.tok2);
	}

	if(expires[0])
	{
		expire_time = timestr_to_secs(expires);
		if(expire_time == 0L)
		{
			sayf(0,0,"Incorrect expire time \"%s\".  Specify a number followed by d,h,m,s (days, hrs, mins, secs)",expires);
			return;
		}
	}

	/* Ok we have the ban, and the expire time.  */
	Banlist_addban(cmdinfo.user->acctname,cmdinfo.tok1,chan,expire_time,time(NULL));

	say("Added \"%s\" to the banlist.",cmdinfo.tok1);

	Banlist->flags |= BANLIST_NEEDS_WRITE;

	/* See if we need to exercise ban */
	for(chan=Channel->channels; chan!=NULL; chan=chan->next)
	{
		if(!Channel_ihaveops(chan))
			continue;

		for(m=chan->memberlist; m!=NULL; m=m->next)
		{
			sprintf(tmp_uh,"%s!%s",m->nick,m->uh);
			bl = Banlist_match(tmp_uh,chan);
			if(bl != NULL)
			{
				say("(Exercised ban against %s %s...)",m->nick,m->uh);
				Log_write(LOG_PUBLIC,chan->name,"Kickbanned %s %s from %s (on banlist)",m->nick,m->uh,chan->name);
				Server_write(PRIORITY_HIGH,"MODE %s -o+b %s %s",chan->name,m->nick,bl->ban);
				Server_write(PRIORITY_HIGH,"KICK %s %s :%s",chan->name,m->nick,Grufti->shitlist_message);
				bl->used++;
			}
		}
	}
}

void cmd_delban()
{
	banlrec	*b;
	int num, tot;

	if(!cmdinfo.isparam)
	{
		say("Usage: delban <ban | #>");
		sayf(2,0,"Use this command to delete a ban from the banlist.  Specify the name of the ban or the ban number.");
		return;
	}

	if(isanumber(cmdinfo.tok1))
	{
		num = atoi(cmdinfo.tok1);
		tot = Banlist_orderbysettime();
		if(num < 1 && num > tot)
		{
			say("Invalid ban number %d.", num);
			return;
		}

		b = Banlist_banbyordernum(num);
	}
	else
		b = Banlist_ban(cmdinfo.tok1);

	if(b == NULL)
	{
		say("No such ban \"%s\" in the banlist.",cmdinfo.tok1);
		return;
	}

	say("Deleted \"%s\" from the banlist.",b->ban);
	Banlist_deleteban(b);

	Banlist->flags |= BANLIST_NEEDS_WRITE;
}

void cmd_voice()
{
	char	chname[BUFSIZ], nick[BUFSIZ];
	chanrec	*chan;
	membrec	*m;

	if(cmdinfo.tok1[0] == '#' || cmdinfo.tok1[0] == '&' || cmdinfo.tok1[0] == '+')
	{
		/* Channel is specified. */
		strcpy(chname,cmdinfo.tok1);
	
		/* check if nick and channel are given */
		if(cmdinfo.istok2)
			strcpy(nick,cmdinfo.tok2);
		else
			strcpy(nick,cmdinfo.from_n);
	
		chan = Channel_channel(chname);
		if(chan == NULL)
		{
			sayf(0,0,"The channel \"%s\" is not known to me.",chname);
			return;
		}
	
		/* got it.  check if i have ops */
		if(!Channel_ihaveops(chan))
		{
			say("Unable to voice %s on %s.  I don't have ops there.",nick,chan->name);
			return;
		}
	
		/* ok. have ops.  if no nick, see if this user is on that channel */
		m = Channel_member(chan,nick);
		if(m == NULL)
		{
			say("I don't see %s on channel %s!",nick,chan->name);
			return;
		}
	
		/* Check if they're split from us */
		if(m->split)
		{
			char	tsplit[TIMESHORTLEN];
	
			timet_to_ago_short(m->split,tsplit);
			say("%s has been netsplit from me for %s!  Sorry.",m->nick,tsplit);
			return;
		}
	
		/* ok.  user is on channel.  +v them */
		if(m->flags & MEMBER_VOICE)
		{
			say("%s already has +v on %s!",m->nick,chan->name);
			return;
		}
		if(m->flags & MEMBER_OPER)
		{
			say("%s already has +o on %s!",m->nick,chan->name);
			return;
		}
		if(m->flags & MEMBER_PENDINGVOICE)
		{
			say("%s is pending +v from the server...",m->nick);
			return;
		}

		Channel_givevoice(chan,m);
		say("Sending mode change +v for %s on %s.",m->nick,chan->name);

		return;
	}

	/* No channel is specified.  +v user on all channels */
	for(chan=Channel->channels; chan!=NULL; chan=chan->next)
	{
		context;
		/* check if nick and channel are given */
		if(cmdinfo.istok1)
			strcpy(nick,cmdinfo.tok1);
		else
			strcpy(nick,cmdinfo.from_n);
	
		context;
		/* is member on it? */
		m = Channel_member(chan,nick);
		if(m != NULL)
		{
			if(!(m->flags & MEMBER_VOICE) && !(m->flags & MEMBER_PENDINGVOICE) && !m->split)
			{
				/* if i don't have ops, don't bother */
				if(!Channel_ihaveops(chan))
					say("Unable to voice %s on %s.  I don't have ops there.",nick,chan->name);
				else
				{
					Channel_givevoice(chan,m);
					say("Sending mode change +v for %s on %s.",m->nick,chan->name);
				}
			}
		}
	}
}

void cmd_leave()
{
	chanrec	*chan;
	char	timeout[25];
	time_t	timeout_time;

	if(!cmdinfo.isparam)
	{
		sayf(0,0,"Usage: leave <channel> [timeout]");
		sayf(2,0,"Use this command to make the bot leave the specified channel.  If a timeout period is given, the bot will leave the channel for that amount of time.  If no timeout is specified, the bot will rejoin in 5 minutes.");
		return;
	}

	chan = Channel_channel(cmdinfo.tok1);
	if(chan == NULL)
	{
		sayf(0,0,"Channel \"%s\" is not known to me.",cmdinfo.tok1);
		return;
	}

	/* Have we already been asked to leave? */
	if(chan->flags & CHAN_LEAVE)
	{
		time_to_elap_short((chan->leave_time + chan->leave_timeout) - time(NULL),timeout);
		sayf(0,0,"We've already been asked to leave %s.  (Rejoin in %s)",chan->name,timeout);
		return;
	}

	if(!(chan->flags & CHAN_ONCHAN))
	{
		if(chan->nojoin_reason)
			sayf(0,0,"I couldn't join %s (%s).  Ignoring leave request.",chan->name,chan->nojoin_reason);
		else
			sayf(0,0,"Not on channel yet, waiting for JOIN to happen.  Ignoring leave request.");
		return;
	}

	if(cmdinfo.istok2)
	{
		timeout_time = timestr_to_secs(cmdinfo.tok2);
		if(timeout_time == 0L)
		{
			sayf(0,0,"Incorrect timeout format \"%s\".  Specify a number followed by d,h,m,s (days, hrs, mins, secs)",cmdinfo.tok2);
			return;
		}
	}
	else
		timeout_time = Grufti->default_leave_timeout;

	/* Don't leave for more than max */
	if(timeout_time > Grufti->max_leave_timeout)
	{
		time_to_elap_short(Grufti->max_leave_timeout,timeout);
		say("(Not leaving any longer than %s, sorry.)",timeout);
		timeout_time = Grufti->max_leave_timeout;
	}
		
	chan->leave_time = time(NULL);
	chan->leave_timeout = timeout_time;
	chan->flags |= CHAN_LEAVE;

	Server_write(PRIORITY_HIGH,"PART :%s",chan->name);
	time_to_elap_short(timeout_time,timeout);
	if(!(chan->flags & CHAN_ACTIVE))
		say("Not even active on %s yet, but ok.  Leaving for %s.",chan->name,timeout);
	else
		say("Leaving channel %s for %s.",chan->name,timeout);
}

void cmd_join()
{
	chanrec	*chan;

	if(!cmdinfo.isparam)
	{
		sayf(0,0,"Usage: join <channel>");
		sayf(2,0,"Use this command to make the bot join the specified channel if it has been asked to leave.");
/*
		sayf(2,0,"Use this command to make the bot join the specified channel if it has been asked to leave.  If the bot is asked to join a new channel, it will create a new entry for that channel.  The new channel will not be written to the configfile, and when asked to leave, the bot will not try rejoining the channel.");
*/
		return;
	}

	chan = Channel_channel(cmdinfo.tok1);
	if(chan == NULL)
	{
		sayf(0,0,"Channel \"%s\" is not known to me.",cmdinfo.tok1);
		return;
/*
		chan = Channel_addchannel(cmdinfo.tok1);
		if(chan == NULL)
			return;

		sayf(0,0,"Created the channel entry \"%s\".",chan->name);
		chan->flags &= ~CHAN_PERMANENT;
*/
	}

	if(chan->flags & CHAN_ONCHAN)
		say("Already on %s!",chan->name);

	/* Turn off request to leave */
	chan->leave_time = 0L;
	chan->flags &= ~CHAN_LEAVE;

	/* Mark that we haven't tried a rejoin so we'll join right away */
	Channel->time_tried_join = 0L;

	say("Attempting to join %s...",chan->name);

	/* Don't need to do anything else here, we'll join in a millisecond */
}

void cmd_hush()
{
	chanrec	*chan;
	char	timeout[25];
	time_t	timeout_time;

	if(!cmdinfo.isparam)
	{
		sayf(0,0,"Usage: hush <channel> [timeout]");
		sayf(2,0,"Use this command to make the bot stop chattering on the specified channel.  If no channel is specified, the hush will take place on all channels.  If a timeout period is given, the bot will remain in this state for that amount of time.  If no timeout is specified, the bot will resume normal operation in 5 minutes.");
		return;
	}

	chan = Channel_channel(cmdinfo.tok1);
	if(chan == NULL)
	{
		sayf(0,0,"Channel \"%s\" is not known to me.",cmdinfo.tok1);
		return;
	}

	/* Have we already been asked to hush? */
	if(chan->flags & CHAN_HUSH)
	{
		time_to_elap_short((chan->hush_time + chan->hush_timeout) - time(NULL),timeout);
		sayf(0,0,"We've already been asked to hush %s.  (back to normal in %s)",chan->name,timeout);
		return;
	}

	if(!(chan->flags & CHAN_ONCHAN))
	{
		if(chan->nojoin_reason)
			sayf(0,0,"I couldn't join %s (%s).  Ignoring hush request.",chan->name,chan->nojoin_reason);
		else
			sayf(0,0,"Not on channel yet, waiting for JOIN to happen.  Ignoring hush request.");
		return;
	}

	if(cmdinfo.istok2)
	{
		timeout_time = timestr_to_secs(cmdinfo.tok2);
		if(timeout_time == 0L)
		{
			sayf(0,0,"Incorrect timeout format \"%s\".  Specify a number followed by d,h,m,s (days, hrs, mins, secs)",cmdinfo.tok2);
			return;
		}
	}
	else
		timeout_time = Grufti->default_hush_timeout;

	/* Don't hush for more than max */
	if(timeout_time > Grufti->max_hush_timeout)
	{
		time_to_elap_short(Grufti->max_hush_timeout,timeout);
		say("(Not hushing any longer than %s, sorry.)",timeout);
		timeout_time = Grufti->max_hush_timeout;
	}
		
	chan->hush_time = time(NULL);
	chan->hush_timeout = timeout_time;
	chan->flags |= CHAN_HUSH;

	time_to_elap_short(timeout_time,timeout);
	if(!(chan->flags & CHAN_ACTIVE))
		say("Not even active on %s yet, but ok.  Hushing for %s.",chan->name,timeout);
	else
		say("Hushing on channel %s for %s.",chan->name,timeout);
}

void cmd_unhush()
{
	chanrec	*chan;

	if(!cmdinfo.isparam)
	{
		sayf(0,0,"Usage: unhush <channel>");
		sayf(2,0,"Use this command to have the bot resume normal operations on the channel after it has been told to hush.");
		return;
	}

	chan = Channel_channel(cmdinfo.tok1);
	if(chan == NULL)
	{
		sayf(0,0,"Channel \"%s\" is not known to me.",cmdinfo.tok1);
		return;
	}

	if(!(chan->flags & CHAN_HUSH))
		say("I'm not being asked to hush on %s!",chan->name);

	/* Turn off request to hush */
	chan->hush_time = 0L;
	chan->flags &= ~CHAN_HUSH;

	say("Resuming normal operations on %s...",chan->name);
}

void cmd_backup()
{ 
	int	x;
	char	backup[256], src[256];

	/* Write all files */
	writeallfiles(1);

	/* Backup userlist */
	sprintf(backup,"%s/%s",Grufti->backupdir,Grufti->userfile);
	sprintf(src,"%s/%s",Grufti->botdir,Grufti->userfile);
	x = copyfile(src,backup);
	verify_write(x,src,backup);
	if(x == 0)
		say("Wrote backup userlist file.");
	else
		say("Had problems backing up userlist file. (check log)");

	/* Backup response */
	sprintf(backup,"%s/%s",Grufti->backupdir,Grufti->respfile);
	sprintf(src,"%s/%s",Grufti->botdir,Grufti->respfile);
	x = copyfile(src,backup);
	verify_write(x,src,backup);
	if(x == 0)
		say("Wrote backup response file.");
	else
		say("Had problems backing up response file. (check log)");

	/* Backup locations */
	sprintf(backup,"%s/%s",Grufti->backupdir,Grufti->locafile);
	sprintf(src,"%s/%s",Grufti->botdir,Grufti->locafile);
	x = copyfile(src,backup);
	verify_write(x,src,backup);
	if(x == 0)
		say("Wrote backup locations file.");
	else
		say("Had problems backing up locations file. (check log)");
				
	/* Backup banlist */
	sprintf(backup,"%s/%s",Grufti->backupdir,Grufti->banlist);
	sprintf(src,"%s/%s",Grufti->botdir,Grufti->banlist);
	x = copyfile(src,backup);
	verify_write(x,src,backup);
	if(x == 0)
		say("Wrote backup banlist file.");
	else
		say("Had problems backing up banlist file. (check log)");
}

void cmd_showbans()
{
	banrec	*b;
	char	flags[FLAGLONGLEN], modes[FLAGLONGLEN];
	char	ban[UHOSTLEN], date[DATESHORTLEN], who_set[UHOSTLEN];
	chanrec	*chan;

	if(!cmdinfo.isparam)
	{
		sayf(0,0,"Usage: showbans <chname>");
		sayf(2,0,"Use this command to display the bans set on the specified channel.");
		return;
	}

	chan = Channel_channel(cmdinfo.tok1);
	if(chan == NULL)
	{
		sayf(0,0,"Channel \"%s\" is not known to me.",cmdinfo.tok1);
		return;
	}

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

	Channel_modes2str(chan,modes);
	context;
	if(modes[0])
		say("Channel: %s (%s)",chan->name,modes);
	else
		say("Channel: %s",chan->name);
	context;
	if(flags[0])
		say("Status: %s",flags);

	/* Any bans on channel? */
	if(!chan->num_bans)
	{
		say("No channel bans on %s.",chan->name);
		return;
	}

	say("Date       Set by                       Ban");
	say("---------- ---------------------------- -------------------------");

	for(b=chan->banlist; b!=NULL; b=b->next)
	{
		timet_to_dateonly_short(b->time_set,date);

		strcpy(ban,b->ban);
		if(strlen(ban) > 25)
			ban[25] = 0;

		sprintf(who_set,"%s %s",b->who_n,b->who_uh);
		if(strlen(who_set) > 28)
			who_set[28] = 0;

		say("%10s %-28s %-25s",date,who_set,ban);
	}
	say("---------- ---------------------------- -------------------------");
}
	
void cmd_email()
{
	userrec	*u;
	nickrec	*n;
	char	lookup[NICKLEN];

	if(!cmdinfo.isparam)
	{
		say("Usage: email <nick>");
		sayf(2,0,"Use this command to display the email address for the specified nick.");
		return;
	}

	u = User_user(cmdinfo.tok1);
	if(!u->registered)
	{
		sayf(0,0,"The nick \"%s\" is not registered.",cmdinfo.tok1);
		return;
	}

	n = User_nick(u,cmdinfo.tok1);
	if(n != NULL)
		strcpy(lookup,n->nick);
	else
		strcpy(lookup,cmdinfo.tok1);

	if(u->email && u->email[0])
		say("%s is %s",lookup,u->email);
	else
		say("%s has no email set.",lookup);
}
		
void cmd_url()
{
	userrec	*u;
	nickrec	*n;
	char	lookup[NICKLEN];

	if(!cmdinfo.isparam)
	{
		say("Usage: url <nick>");
		sayf(2,0,"Use this command to display the url address for the specified nick.");
		return;
	}

	u = User_user(cmdinfo.tok1);
	if(!u->registered)
	{
		sayf(0,0,"The nick \"%s\" is not registered.",cmdinfo.tok1);
		return;
	}

	n = User_nick(u,cmdinfo.tok1);
	if(n != NULL)
		strcpy(lookup,n->nick);
	else
		strcpy(lookup,cmdinfo.tok1);

	if(u->www && u->www[0])
		say("Check out %s",u->www);
	else
		say("%s has no URL set.",lookup);
}

void cmd_bday()
{
	userrec	*u;
	nickrec	*n;
	char	lookup[NICKLEN], display[150];

	if(!cmdinfo.isparam)
	{
		say("Usage: bday <nick>");
		sayf(2,0,"Use this command to display the birthday for the specified nick.");
		return;
	}

	u = User_user(cmdinfo.tok1);
	if(!u->registered)
	{
		sayf(0,0,"The nick \"%s\" is not registered.",cmdinfo.tok1);
		return;
	}

	n = User_nick(u,cmdinfo.tok1);
	if(n != NULL)
		strcpy(lookup,n->nick);
	else
		strcpy(lookup,cmdinfo.tok1);

	if(u->bday[0])
	{
		char hor[25];

		bday_plus_hor(display,hor,u->bday[0],u->bday[1],u->bday[2]);
		if(u->bday[2] == -1)
			say("%s's birthday is %s. (%s)",lookup,display,hor);
		else
			say("%s's birthday is on %s. (%s)",lookup,display,hor);
	}
	else
		say("%s has no bday set.",lookup);
}

void cmd_showhistory()
{
	userrec *u;
	struct userrec_history *l;
	char msg[STDBUF], next_msg[STDBUF], old_msg[STDBUF], date[DATESHORTLEN];
	char *p;

	if(!cmdinfo.isparam)
	{
		say("Usage: showhistory <nick>");
		sayf(2,0,"Use this command to view a user's account history");
		return;
	}

	u = User_user(cmdinfo.tok1);
	if(!u->registered)
	{
		sayf(0,0,"The nick \"%s\" is not registered.", cmdinfo.tok1);
		return;
	}

	if(u->history == NULL)
	{
		internal("cmd: in showhistory(), log is NULL.");
		return;
	}

	say("Account history: %s", u->acctname);
	say("");
	say("Date       Added By  Entry");
	say("---------- --------- --------------------------------------------");
	for(l = listmgr_begin(u->history); l; l = listmgr_next(u->history))
	{
		timet_to_dateonly_short(l->date, date);

		next_msg[0] = 0;
		strcpy(msg, l->info);
		p = strchr(msg, '\n');
		if(p != NULL)
		{
			*p = 0;
			strcpy(next_msg, p+1);
		}
		if(strlen(msg) > 44)
		{
			strcpy(old_msg, next_msg);
			strcpy(next_msg, &msg[44]);
			strcat(next_msg, old_msg);
			msg[44] = 0;
		}
		say("%10s %-9s %s", date, l->who, msg);

		strcpy(msg, next_msg);
		next_msg[0] = 0;

		while(msg[0])
		{
			p = strchr(msg, '\n');
			if(p != NULL)
			{
				*p = 0;
				strcpy(next_msg, p+1);
			}
			if(strlen(msg) > 44)
			{
				strcpy(old_msg, next_msg);
				strcpy(next_msg, &msg[44]);
				strcat(next_msg, old_msg);
				msg[44] = 0;
			}
			say("%18s %s", "", msg);

			strcpy(msg, next_msg);
			next_msg[0] = 0;
		}
	}
	say("---------- --------- --------------------------------------------");
}

void cmd_addhistory()
{
	userrec *u;

	if(!cmdinfo.istok2)
	{
		say("Usage: addhistory <nick> <entry...>");
		sayf(2,0,"Use this command to add an entry to a user's account history.  Useful for making notes to yourself, or other masters.");
		return;
	}

	u = User_user(cmdinfo.tok1);
	if(!u->registered)
	{
		sayf(0,0,"The nick \"%s\" is not registered.", cmdinfo.tok1);
		return;
	}

	if(u->history == NULL)
	{
		internal("cmd: in showhistory(), history is NULL.");
		return;
	}

	split(NULL, cmdinfo.param);
	User_addhistory(u, cmdinfo.user->acctname, time(NULL), cmdinfo.param);

	say("Added history entry.");
}

void cmd_setusernumber()
{
	userrec *u;
	int num;

	if(!cmdinfo.istok2)
	{
		say("Usage: setusernumber <nick> <number>");
		sayf(2,0,"Use this command to reset the registration number for a user.");
		return;
	}

	u = User_user(cmdinfo.tok1);
	if(!u->registered)
	{
		sayf(0,0,"The nick \"%s\" is not registered.", cmdinfo.tok1);
		return;
	}


	num = atoi(cmdinfo.tok2);
	if(num < 0 || num > 999)
	{
		sayf(0,0,"Registration numbers must be between 1 and 999.");
		return;
	}

	u->registered = num;
	say("User registration number for %s set to: %d", u->acctname, num);
}

void cmd_setusercreation()
{
	userrec *u;
	char *p;
	char date[BUFSIZ], elem[BUFSIZ], result[BUFSIZ];
	int count = 0;
	int m,d,y;
	struct tm t;
	time_t time_spec;

	if(!cmdinfo.istok2)
	{
		say("Usage: setusercreation <nick> <mm/dd/yyyy>");
		sayf(2,0,"Use this command to reset the account creation date for a user.");
		return;
	}

	u = User_user(cmdinfo.tok1);
	if(!u->registered)
	{
		sayf(0,0,"The nick \"%s\" is not registered.", cmdinfo.tok1);
		return;
	}

	strcpy(date, cmdinfo.tok2);
	/* Count number of '/' chars */
	for(p=date; *p; p++)
	{
		if(*p == '/')
			count++;
	}

	if(count < 2 || count > 2)
	{
		sayf(0,0,"Sorry, that date is not in the correct format.  Use mm/dd/yyyy please.");
		return;
	}

	splitc(elem,date,'/');
	m = elem[0] ? atoi(elem) : 0;

	splitc(elem,date,'/');
	d = elem[0] ? atoi(elem) : 0;
	y = date[0] ? atoi(date) : -1;

	if(!check_date(m,d,y))
	{
		sayf(0,0,"Sorry, that date is not in the correct format.  Use mm/dd/yyyy please.");
		return;
	}
		
	cmdinfo.user->bday[0] = m;
	cmdinfo.user->bday[1] = d;

	/* convert year */
	y -= 1900;

	t.tm_sec = 0;
	t.tm_min = 0;
	t.tm_hour = 0;
	t.tm_mday = d;
	t.tm_mon = m - 1;
	t.tm_year = y;
	t.tm_isdst = 0;

	time_spec = mktime(&t);
	if(time_spec == -1)
	{
		say("Unable to represent that date.");
		return;
	}

	timet_to_date_long(time_spec, result);
	say("Account creation date for %s set to: %s", u->acctname, result);

	u->time_registered = time_spec;
}

void cmd_delhostmask()
{
	acctrec *a;

	if(!cmdinfo.istok2)
	{
		say("Usage: delhostmask <nick> <hostmask>");
		sayf(2,0,"Use this command to delete hostmasks from your account.  Hostmasks allow the bot to track your online usage, and to determine whether or not you should be greeted, if you have +g. Type 'whois %s' to see the hostmasks listed for your account.", cmdinfo.from_n);
		return;
	}

	/*
	 * If hostmask matches, move account record to the luser record
	 */
	a = User_account_dontmakenew(cmdinfo.user, cmdinfo.tok1, cmdinfo.tok2);
	if(a)
	{
		User_moveaccount(cmdinfo.user, User->luser, a);
		say("Hostmask %s!%s deleted from your account.", cmdinfo.tok1, cmdinfo.tok2);
	}
	else
		say("Unable to delete hostmask: %s!%s", cmdinfo.tok1, cmdinfo.tok2);
}
