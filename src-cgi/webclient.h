/******************************************************************************
 * Steve Rider - stever@area23.org
 * Grufti - an interactive, service-oriented bot for IRC 
 *
 * webclient.h - web interface
 *
 *****************************************************************************/
/* 28 April 97 */

#ifndef WEBCLIENT_H
#define WEBCLIENT_H

#include <stdio.h>

/* Let all files who need Web have access to the Web object */
extern struct Web_object *Web;


/*****************************************************************************/
/* object Web declaration */
typedef struct Web_object {

	/* buffers */
	char	websay_buf[BUFSIZ];
	char	logfile[BUFSIZ];
	
	/* Web page stuff */
	char	main_url[256];		/* URL for main page */
	char	cgi_url[256];		/* URL for cgi script */
	char	bg_color[25];		/* page color for all pages */
	char	text_color[25];		/* text color for all pages */
	char	link_color[25];		/* link color for all pages */
	char	vlink_color[25];	/* vlink color for all pages */
	char	alink_color[25];	/* alink color for all pages */
	char	bg_file[256];		/* background file, if any */
	char	color1[25];		/* color 1 */
	char	color2[25];		/* color 2 */
	char	color3[25];		/* color 3 (bold) */
	char	color4[25];		/* color 4 (titles) */
	char	html_body[256];

	/* These are used by web_client.c for grufti_interface.cgi */
	char	grufti_host[256];	/* Grufti's host */
	int	grufti_port;		/* Grufti's port */
	int	sock;			/* Socket we'll be using */

	char	referer[1024];		/* web page referer */
	char	username[1024];		/* web page client's login */
	char	password[1024];		/* web page client's password */


	time_t	last_gotanything;
	int	timeout;
	char	flags;

} Web_ob;

/* object Web low-level function declarations. */
void	Web_new();
void	Web_freeall();

/* object Web high-level implementation-specific function declarations. */
void	Client_gotEOF();
void	Client_quit();
void	Client_disconnect();
void	Client_gotsocketerror();
void	Client_gotactivity(int sock, char *buf, int i);
void	Client_processform();
void	Client_NotePost();
void	Client_UpdateNotes();
void	Client_CmdlinePost();
void	Client_SetupPost();
void	Client_ResponsePost();
void	Client_NewResponsePost();
void	Client_EventPost();
void	Client_FingerLookup();
void	Client_accesserror();
void	Client_gruftidown();
void	Client_error();
void	Client_blankentry();
void	Client_blanknote();
void	fatal(char *s, int x);
void	putgrufti(char *format, ...);
void	putpage(char *format, ...);
void	putlog(char *format, ...);

/******************************************************************************/
/* flags */
#define WAITINGFOROK	0x04
#define SENTCMD		0x08
#define CONNECTED	0x10

#define LINELENGTH	65

#endif /* WEBCLIENT_H */
