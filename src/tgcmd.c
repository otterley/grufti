/******************************************************************************
 * Steve Rider - stever@area23.org
 * Grufti - an interactive, service-oriented bot for IRC 
 *
 * tgcmd.c - TinyGrufti commands
 *
 *****************************************************************************/
/* 11 April 98 */

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
#include "location.h"
#include "note.h"
#include "response.h"
#include "url.h"
#include "server.h"
#include "tgrufti.h"
#include "tgcmd.h"
#include "event.h"


cmdrec_fixed_t tgcommand_tbl[] =
{
/*	Command			function		levels	DCC?	*/
/*	=====================	=====================	======	======	*/
	{ "channels",		NULL, tgcmd_channels,		"-",	FALSE	},
	{ "who",		NULL, tgcmd_who,		"-",	FALSE	},
	/*
	 :
	 */
	{ NULL,			NULL, null(void(*)()),	NULL,	FALSE	},
};


void tgcmd_channels()
{
	chanrec	*chan;
	char	channels[BUFSIZ];

	channels[0] = 0;

	for(chan=Channel->channels; chan; chan=chan->next)
		sprintf(channels,"%s %s",channels,chan->name);

	tgsay("%s CHANNELS %s",TinyGrufti->header,channels); 
}

void tgcmd_who()
{
	chanrec	*chan;
	membrec	*m;
	char	mflags[10];

	if(!TinyGrufti->istok1)
	{
		tgsay("ERROR Invalid arguments to WHO");
		return;
	}

	chan = Channel_channel(TinyGrufti->tok1);
	if(chan == NULL)
	{
		tgsay("ERROR No such channel \"%s\"",TinyGrufti->tok1);
		return;
	}

	for(m=chan->memberlist; m; m=m->next)
	{
		mflags[0] = 0;
		if(m->flags & MEMBER_VOICE)
			strcat(mflags,"+");
		if(m->flags & MEMBER_OPER)
			strcat(mflags,"@");
		
		tgsay("%s WHO %s %s %lu %s %lu",TinyGrufti->header, m->nick,
			m->uh, m->joined, mflags, m->split);
	}
}

/* ASDF */
