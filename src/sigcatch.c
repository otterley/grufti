/******************************************************************************
 * Steve Rider - stever@area23.org
 * Grufti - an interactive, service-oriented bot for IRC 
 *
 * sigcatch.c - signal catching routines.
 *
 *****************************************************************************/
/* 28 April 97 */

#include "config.h"
#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include "misc.h"
#include "sigcatch.h"
#include "grufti.h"
#include "user.h"
#include "location.h"
#include "log.h"
#include "response.h"
#include "url.h"
#include "dcc.h"
#include "server.h"
#include "banlist.h"
#include "channel.h"
#include "event.h"

void sigcatch_init()
{
	struct sigaction sv;

	sv.sa_handler=got_bus;	sigemptyset(&sv.sa_mask);
	sv.sa_flags=0;		sigaction(SIGBUS,&sv,NULL);
	sv.sa_handler=got_segv;	sigaction(SIGSEGV,&sv,NULL);
	sv.sa_handler=got_fpe;	sigaction(SIGFPE,&sv,NULL);
	sv.sa_handler=got_term;	sigaction(SIGTERM,&sv,NULL);
	sv.sa_handler=got_hup;	sigaction(SIGHUP,&sv,NULL);
	sv.sa_handler=got_quit;	sigaction(SIGQUIT,&sv,NULL);
	sv.sa_handler=SIG_IGN;	sigaction(SIGPIPE,&sv,NULL);
	sv.sa_handler=got_usr1;	sigaction(SIGUSR1,&sv,NULL);
	sv.sa_handler=got_usr2;	sigaction(SIGUSR2,&sv,NULL);
	sv.sa_handler=got_ill;	sigaction(SIGILL,&sv,NULL);
	sv.sa_handler=got_alarm; sigaction(SIGALRM,&sv,NULL);
	sv.sa_handler=got_sigint; sigaction(SIGINT,&sv,NULL);
	/*  sv.sa_handler=got_pipe; sigaction(SIGPIPE,&sv,NULL); */
	/*  sv.sa_handler=got_child; sigaction(SIGCHLD,&sv,NULL);  */
}


void write_debug()
{
	mem_dump_memtbl();
	Log_write(LOG_ALL,"*","*** Last context: %s:%d",cx_file,cx_line);
}

void fatal(char *s, int x)
{
	Log_write(LOG_ALL,"*","FATAL: %s",s);
	printf("\n\nFATAL: %s\n",s);
	unlink(Grufti->pidfile);
	exit(x);
}

void depart()
{
	/* Write all files, we're almost ready to shut down */
	writeallfiles(0);

	fatal("*** SHUTDOWN COMPLETE (Soft Kill) ***\n\n",0);
}

void writeallfiles(int force)
{
	Log_write(LOG_MAIN,"*","Writing userfile...");
	User_writeuserfile();
	Log_write(LOG_MAIN,"*","Writing stats...");
	writestats();
	if((Banlist->flags & BANLIST_NEEDS_WRITE) || force)
	{
		Log_write(LOG_MAIN,"*","Writing banlist...");
		Banlist_writebans();
	}
	if((Location->flags & LOCATION_NEEDS_WRITE) || force)
	{
		Log_write(LOG_MAIN,"*","Writing locations...");
		Location_writelocations();
	}
	if((Event->flags & EVENT_NEEDS_WRITE) || force)
	{
		Log_write(LOG_MAIN,"*","Writing events...");
		Event_writeevents();
	}
	if((Response->flags & RESPONSE_NEEDS_WRITE) || force)
	{
		Log_write(LOG_MAIN,"*","Writing responses...");
		Response_writeresponses();
	}
	if((URL->flags & URL_NEEDS_WRITE) || force)
	{
		Log_write(LOG_MAIN,"*","Writing webcatches...");
		URL_writeurls();
	}

	Grufti->last_write = time(NULL);
}

/* Sigcatch routines */

void got_bus(int z)
{
	write_debug();
	fatal("BUS ERROR -- CRASHING!",1);
}

void got_segv(int z)
{
	write_debug();
	fatal("SEGMENT VIOLATION -- CRASHING!",1);
}

void got_fpe(int z)
{
	write_debug();
	fatal("FLOATING POINT ERROR -- CRASHING!",1);
}

/*
 * Normal kill:
 * Like soft kill but we don't wait for server to QUIT.  We need to get the
 * hell out of here fast.  This is what's called when we get a quit or term
 * signal like when the machine is going down.
 */
void got_term(int z)
{
	Log_write(LOG_ALL,"*","Received term signal.  Doing normal kill...");
	if(Server_isactiveIRC())
		User_signoffallnicks();

	writeallfiles(0);
	fatal("*** SHUTDOWN COMPLETE (Got TERM) ***\n\n",1);
}

void got_quit(int z)
{
	Log_write(LOG_ALL,"*","Received quit signal.  Doing normal kill...");
	got_term(z);
}

void got_hup(int z)
{
	Log_write(LOG_ALL,"*","Received HUP signal.  Doing normal kill...");
	got_term(z);
}

/* connection to a socket was ended prematurely */
void got_alarm(int z)
{
	Log_write(LOG_ERROR,"*","got alarm");
	/* dump dcc and server table */
	return;
}

void got_sigint(int z)
{
	/* do a soft kill */
	printf("Shutting down (soft kill detected)\n");
	Log_write(LOG_ALL,"*","Received interrupt signal.  Doing soft kill...");
	got_usr1(z);
}
	
	
/*
 * SOFT KILL: send QUIT, write datafiles, and exit (note we don't write the
 * datafiles here.  They're taken care of on depart().
 */
void got_usr1(int z)
{
	dccrec *d, *d_next;
	char	signoff[100];

	if(Grufti->die)
	{
		Log_write(LOG_ALL,"*","Received another soft kill while waiting for signoff.  Exiting via TERM.");
		got_term(z);
	}

	strcpy(signoff,"Soft kill requested, signing off.");
	Log_write(LOG_ALL,"*","Soft kill requested, signing off.");

	Grufti->die = 1;

	/* If we're not even active, don't bother shutting down the server */
	if(Server_isactiveIRC())
	{
		Log_write(LOG_MAIN,"*","Sent QUIT, waiting for signoff...");
		Server_quit(signoff);
	}
	else if(Server_isconnected())
	{
		Server_disconnect();
		Log_write(LOG_MAIN,"*","Signing off...");
	}
	else
		Log_write(LOG_MAIN,"*","Signing off...");

	/* Boot all users on DCC */
	d = DCC->dcclist;
	while(d != NULL)
	{
		d_next = d->next;
		DCC_bootuser(d,signoff);
		d = d_next;
	}

	/* Wait for i/o to close server and cleanup if necessary. */
}

/* HARD KILL:  No QUIT, and don't write data.  exit immediately. */
void got_usr2(int z)
{
	Log_write(LOG_ALL,"*","Hard kill requested.  *POW* we're dead.");
	fatal("*** SHUTDOWN COMPLETE (Hard KILL) ***\n\n",0);
}

/* got ILL signal -- log context and continue */
void got_ill(int z)
{
	Log_write(LOG_MAIN,"*","* Context: %s/%d",cx_file,cx_line);
}
