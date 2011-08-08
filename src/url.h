/******************************************************************************
 * Steve Rider - stever@area23.org
 * Grufti - an interactive, service-oriented bot for IRC 
 *
 * url.h - catch URL's
 *
 *****************************************************************************/
/* 28 April 97 */

#ifndef URL_H
#define URL_H

#include <stdio.h>
#include "typedefs.h"
#include "grufti.h"

/* Let all files who need URL have access to the URL object */
extern struct URL_object *URL;


/*****************************************************************************/
/* object URL declaration */
typedef struct URL_object {
	urlrec	*urllist;		/* -> list of urls */
	urlrec	*last;			/* -> last url record */

        int	num_urls;		/* number of url records */
	u_long	flags;			/* flags (ACTIVE | NEEDSWRITE) */

	char	sbuf[BUFSIZ];		/* fast buffer */

} URL_ob;

/* object URL low-level function declarations. */
void	URL_new();
u_long	URL_sizeof();
void	URL_freeall();

/* object URL data-specific function declarations. */
void	URL_display();

/* low-level URL structure function declarations. */
void	URL_inserturlatfront(urlrec *u);
void	URL_appendurl(urlrec *u);
void	URL_deleteurl(urlrec *u);
void	URL_freeallurls();

/* object URL high-level implementation-specific function declarations. */
void	URL_checkforurl(char *nick, char *uh, char *text);
void	URL_addnewurl(char *nick, char *uh, char *url);
urlrec	*URL_url(char *url);
void	URL_writeurls();
void	URL_loadurls();

/* ASDF */
/*****************************************************************************/
/* record urlrec declaration */
struct urlrec_node {
	urlrec	*next;			/* -> next */

	char	*url;			/* url */
	char	*nick;			/* who said it */
	char	*uh;			/* who said it */
	time_t	time_caught;		/* when it was caught */
	char	*title;			/* title of page */
	u_long	flags;			/* url flags */
} ;

/* record urlrec low-level function declarations. */
urlrec	*urlrec_new();
void	urlrec_freeall(urlrec **list, urlrec **last);
void	urlrec_insertatfront(urlrec **list, urlrec **last, urlrec *x);
void	urlrec_append(urlrec **list, urlrec **last, urlrec *x);
void	urlrec_delete(urlrec **list, urlrec **last, urlrec *x);

/* record urlrec low-level data-specific function declarations. */
void	urlrec_init(urlrec *x);
u_long	urlrec_sizeof(urlrec *x);
void	urlrec_free(urlrec *x);
void	urlrec_seturl(urlrec *x, char *url);
void	urlrec_setnick(urlrec *x, char *nick);
void	urlrec_setuh(urlrec *x, char *uh);
void	urlrec_settitle(urlrec *x, char *title);
void	urlrec_display(urlrec *x);



/*****************************************************************************/
/* URL flags */

#define URL_ACTIVE		0x0001	/* If the list is active */
#define URL_NEEDS_WRITE		0x0002	/* so we don't write when unnecessary */
#define URL_FAILED_CONNECT	0x0004	/* couldn't connect to page */
#define URL_GOTTITLE		0x0008	/* connected and got title */
#define URL_NOSUCHSITE		0x0010	/* no such site */
#define URL_HTTPERROR		0x0020	/* error */
#define URL_NOTCHECKED		0x0040	/* URL title not checked yet */


#endif /* URL_H */
