#ifdef WEB_MODULE
/******************************************************************************
 * Steve Rider - stever@area23.org
 * Grufti - an interactive, service-oriented bot for IRC 
 *
 * web.h - module-like web interface
 *
 *****************************************************************************/
/* 28 April 97 */

#ifndef WEB_H
#define WEB_H

#include <stdio.h>
#include "typedefs.h"
#include "grufti.h"
#include "command.h"

/* Let all files who need Web have access to the Web object */
extern struct Web_object *Web;


/*
 * This Web object is shared between Grufti and cgi_interface.cgi.
 * Some of the variables here may be used in only one of the above
 * programs.  Don't get too confused...
 */

/*****************************************************************************/
/* object Web declaration */
typedef struct Web_object {
	cmdrec_t	*commands;		/* -> list of web commands */
	cmdrec_t	*last;			/* -> last web command */
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

	/* buffers */
	char	websay_buf[BUFSIZ];
	char	display[BUFSIZ];
	
	/* Web page stuff */
	char	title[256];		/* title of page */
	char	main_url[256];		/* URL for main page */
	char	welcome_image[256];	/* Image to use for welcome */
	int	welcome_image_width;
	int	welcome_image_height;
	char	cgi_url[256];		/* URL for cgi script */
	char	bgcolor[25];		/* page color for all pages */
	char	text_color[25];		/* text color for all pages */
	char	link_color[25];		/* link color for all pages */
	char	vlink_color[25];	/* vlink color for all pages */
	char	alink_color[25];	/* alink color for all pages */
	char	bg_file[256];		/* background file, if any */
	char	color1[25];		/* color 1 */
	char	color2[25];		/* color 2 */
	char	color3[25];		/* color 3 (bold) */
	char	color4[25];		/* color 4 titles */

	char	html_body[256];

} Web_ob;

/* object Web low-level function declarations. */
void	Web_new();
u_long	Web_sizeof();
void	Web_freeall();

/* low-level Web structure function declarations. */
void	Web_appendcmd(cmdrec_t *c);
void	Web_deletecmd(cmdrec_t *c);
void	Web_freeallcmds();

/* object Web high-level implementation-specific function declarations. */
void	Web_loadcommands();
void	Web_docommand(dccrec *d, char *buf);
void	Web_reset();
cmdrec_t	*Web_cmd(char *name);
void	websay(char *format, ...);
void	Web_saynotregistered(dccrec *d, char *nick);
void	Web_sayinvalidpassword(dccrec *d, char *nick);
void	Web_fixforhtml(char *s);
void	Web_fixforinput(char *in, char *out);
void	Web_catfile(dccrec *d, char *filename);

#endif /* WEB_H */

#endif /* WEB_MODULE */
