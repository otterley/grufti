/******************************************************************************
 * Steve Rider - stever@area23.org
 * Grufti - an interactive, service-oriented bot for IRC 
 *
 * TinyGrufti.c - TinyGrufti interface commands
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
#include "command.h"
#include "dcc.h"
#include "misc.h"
#include "tgrufti.h"

extern cmdrec_fixed tgcommand_tbl[];

/***************************************************************************n
 *
 * TinyGrufti object definitions.
 *
 ****************************************************************************/

void TinyGrufti_new()
{
	/* already in use? */
	if(TinyGrufti != NULL)
		TinyGrufti_freeall();

	/* allocate memory */
	TinyGrufti = (TinyGrufti_ob *)xmalloc(sizeof(TinyGrufti_ob));

	/* initialize */
	TinyGrufti->commands = NULL;
	TinyGrufti->last = NULL;
	TinyGrufti->num_commands = 0;
	TinyGrufti->cmdinuse = 0;
	TinyGrufti->from_n[0] = 0;
	TinyGrufti->from_uh[0] = 0;
	TinyGrufti->cmdname[0] = 0;
	TinyGrufti->cmdparam[0] = 0;
	TinyGrufti->user = NULL;
	TinyGrufti->account = NULL;
	TinyGrufti->d = NULL;
	TinyGrufti->tok1[0] = 0;
	TinyGrufti->tok2[0] = 0;
	TinyGrufti->tok3[0] = 0;
	TinyGrufti->istok1 = 0;
	TinyGrufti->istok2 = 0;
	TinyGrufti->istok3 = 0;
	TinyGrufti->num_elements = 0;
	TinyGrufti->paramlen = 0;
	TinyGrufti->isparam = 0;
	TinyGrufti->flags = 0L;
	TinyGrufti->header[0] = 0;

	TinyGrufti->say_buf[0] = 0;
	TinyGrufti->display[0] = 0;

}

u_long TinyGrufti_sizeof()
{
	u_long	tot = 0L;

	return tot;
}

void TinyGrufti_freeall()
{
	/* Free the object */
	xfree(TinyGrufti);
	TinyGrufti = NULL;
}

/****************************************************************************/
/* low-level TinyGrufti structure function declarations. */

void TinyGrufti_appendcmd(cmdrec *c)
{
	cmdrec_append(&TinyGrufti->commands, &TinyGrufti->last, c);
	TinyGrufti->num_commands++;
}

void TinyGrufti_deletecmd(cmdrec *c)
{
	cmdrec_delete(&TinyGrufti->commands, &TinyGrufti->last, c);
	TinyGrufti->num_commands--;
}

void TinyGrufti_freeallcmds()
{
	cmdrec_freeall(&TinyGrufti->commands, &TinyGrufti->last);
	TinyGrufti->num_commands = 0;
}


/****************************************************************************/
/* object TinyGrufti high-level implementation-specific function definitions. */

void TinyGrufti_loadcommands()
{
	int	i;
	cmdrec_t	*newcmd;

	for(i=0; tgcommand_tbl[i].name != NULL; i++)
	{
		/* create a new node */
		newcmd = cmdrec_new();

		/* copy data */
		cmdrec_setname(newcmd,tgcommand_tbl[i].name);
		cmdrec_setfunction(newcmd,tgcommand_tbl[i].function);

		User_str2flags(tgcommand_tbl[i].levels,&newcmd->levels);
		if(newcmd->levels & USER_INVALID)
			Log_write(LOG_ERROR,"*","Warning: TinyGrufti command \"%s\" has invalid levels \"%s\".",tgcommand_tbl[i].name,tgcommand_tbl[i].levels);

		/* we don't care about dcc requirements here */
		newcmd->needsdcc = FALSE;

		/* Append it to the TinyGrufti commands list */
		TinyGrufti_appendcmd(newcmd);
	}
}

void TinyGrufti_docommand(dccrec *d, char *buf)
{
	cmdrec	*tgc;

	/* fix buf so we don't get empty fields */
	rmspace(buf);

	/* TinyGrufti command variables must be preset */
	strcpy(TinyGrufti->from_n,d->nick);
	strcpy(TinyGrufti->from_uh,d->uh);
	split(TinyGrufti->cmdname,buf);
	strcpy(TinyGrufti->cmdparam,buf);

	/* Set DCC connection record */
	TinyGrufti->d = d;

	/* Set user and account pointers */
	TinyGrufti->user = d->user;
	TinyGrufti->account = d->account;

	/* Now we're ready */
	if(TinyGrufti->cmdname[0] == 0)
	{
		Log_write(LOG_DEBUG,"*","TinyGrufti_docommand() invoked with empty cmdname.");
		return;
	}

	/* Mark command as active */
	TinyGrufti->cmdinuse = 1;

	/* Find the command */
	tgc = TinyGrufti_cmd(TinyGrufti->cmdname);
	if(tgc == NULL)
	{
		/* Not found. */
		tgsay("ERROR No such command \"%s\".",TinyGrufti->cmdname);
		TinyGrufti->cmdinuse = 0;
		TinyGrufti_reset();
		return;
	}

	/* Check levels */
	if(tgc->levels != 0L)
	{
		if(!(TinyGrufti->user->flags & tgc->levels))
		{
			char	flags[USERFLAGLEN];
	
			User_flags2str(tgc->levels,flags);
			tgsay("ERROR That command requires +%s and you don't have it.  Go away.",flags);
			TinyGrufti->cmdinuse = 0;
			TinyGrufti_reset();
			return;
		}
	}

	/* Ok here we go */
	str_element(TinyGrufti->tok1,TinyGrufti->cmdparam,1);
	str_element(TinyGrufti->tok2,TinyGrufti->cmdparam,2);
	str_element(TinyGrufti->tok3,TinyGrufti->cmdparam,3);
	TinyGrufti->istok1 = TinyGrufti->tok1[0] ? 1 : 0;
	TinyGrufti->istok2 = TinyGrufti->tok2[0] ? 1 : 0;
	TinyGrufti->istok3 = TinyGrufti->tok3[0] ? 1 : 0;
	TinyGrufti->num_elements = num_elements(TinyGrufti->cmdparam);
	TinyGrufti->paramlen = strlen(TinyGrufti->cmdparam);
	TinyGrufti->isparam = TinyGrufti->cmdparam[0] ? 1 : 0;
	strcpy(TinyGrufti->header,d->tinygrufti_header);

	/* Execute command */
	tgc->accessed++;
	tgc->function();

	/* Reset */
	TinyGrufti_reset();
	TinyGrufti->cmdinuse = 0;
}

void TinyGrufti_reset()
{
	TinyGrufti->from_n[0] = 0;
	TinyGrufti->from_uh[0] = 0;
	TinyGrufti->cmdname[0] = 0;
	TinyGrufti->cmdparam[0] = 0;
	TinyGrufti->user = NULL;
	TinyGrufti->account = NULL;
	TinyGrufti->d = NULL;
	TinyGrufti->tok1[0] = 0;
	TinyGrufti->tok2[0] = 0;
	TinyGrufti->tok3[0] = 0;
	TinyGrufti->istok1 = 0;
	TinyGrufti->istok2 = 0;
	TinyGrufti->istok3 = 0;
	TinyGrufti->num_elements = 0;
	TinyGrufti->paramlen = 0;
	TinyGrufti->isparam = 0;
}

cmdrec *TinyGrufti_cmd(char *name)
{
	return cmdrec_cmd(&TinyGrufti->commands, &TinyGrufti->last, name);
}

void tgsay(char *format, ...)
{
	va_list	va;

	TinyGrufti->say_buf[0] = 0;
	va_start(va,format);
	vsprintf(TinyGrufti->say_buf,format,va);
	va_end(va);

	/* TinyGrufti say is used only from inside command loop */
	if(!TinyGrufti->cmdinuse)
	{
		Log_write(LOG_DEBUG,"*","Attempted to 'tgsay' outside of command loop. \"%s\"",TinyGrufti->say_buf);
		return;
	}

	if(TinyGrufti->d == NULL)
	{
		Log_write(LOG_ERROR,"*","Recipient has no DCC record! \"%s\"",TinyGrufti->say_buf);
		return;
	}

	DCC_write(TinyGrufti->d, "%s", TinyGrufti->say_buf);
}

/* ASDF */
