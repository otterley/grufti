/******************************************************************************
 * Steve Rider - stever@area23.org
 * Grufti - an interactive, service-oriented bot for IRC 
 *
 * tgrufti.h - TinyGrufti interface
 *
 *****************************************************************************/
/* 28 April 97 */

#ifndef TGRUFTI_H
#define TGRUFTI_H

#include <stdio.h>
#include "typedefs.h"
#include "grufti.h"

/* Let all files who need TinyGrufti have access to the TinyGrufti object */
extern struct TinyGrufti_object *TinyGrufti;


/*****************************************************************************/
/* object TinyGrufti declaration */
typedef struct TinyGrufti_object {
	cmdrec	*commands;		/* -> list of web commands */
	cmdrec	*last;			/* -> last web command */
	int	num_commands;		/* number of web commands */
	int	cmdinuse;		/* marked when currently in use */

	/* preset fields for web commands */
	char	from_n[UHOSTLEN];	/* nick of person */
	char	from_uh[UHOSTLEN];	/* uh of person */
	char	cmdname[BUFSIZ];	/* command name */
	char	cmdparam[BUFSIZ];	/* command parameters */
	userrec	*user;			/* -> user record */
	acctrec	*account;		/* -> account record */
	dccrec	*d;			/* -> dcc record */

	/* Info on current command */
	char	tok1[BUFSIZ];
	char	tok2[BUFSIZ];
	char	tok3[BUFSIZ];
	int	istok1;
	int	istok2;
	int	istok3;
	int	num_elements;
	int	paramlen;
	int	isparam;
	u_long	flags;
	char	header[40];

	/* buffers */
	char	say_buf[BUFSIZ];
	char	display[BUFSIZ];

} TinyGrufti_ob;

/* object Web low-level function declarations. */
void	TinyGrufti_new();
u_long	TinyGrufti_sizeof();
void	TinyGrufti_freeall();

/* low-level TinyGrufti structure function declarations. */
void	TinyGrufti_appendcmd(cmdrec *c);
void	TinyGrufti_deletecmd(cmdrec *c);
void	TinyGrufti_freeallcmds();

/* object Web high-level implementation-specific function declarations. */
void	TinyGrufti_loadcommands();
void	TinyGrufti_docommand(dccrec *d, char *buf);
void	TinyGrufti_reset();
cmdrec	*TinyGrufti_cmd(char *name);
void	tgsay(char *format, ...);

/* ASDF */

#endif /* TGRUFTI_H */
