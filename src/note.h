/******************************************************************************
 * Steve Rider - stever@area23.org
 * Grufti - an interactive, service-oriented bot for IRC 
 *
 * note.h - Notes
 *
 *****************************************************************************/
/* 28 April 97 */

#ifndef NOTE_H
#define NOTE_H

#include <stdio.h>
#include "typedefs.h"
#include "grufti.h"

/*
 * Note records are a subset of the USER record.  They are saved in a
 * separate file.
 */

/* Let all files who need Note have access to the Note object */

extern struct Note_object *Note;


/*****************************************************************************/
/* object Note declaration */
typedef struct Note_object {
	u_long	flags;			/* flags */

	char	send_buf[4096];
	char	send_buf1[4096];

	int	numNotesQueued;
	time_t	time_dequeued;

} Note_ob;

/* object Note low-level function declarations. */
void	Note_new();
void	Note_freeall();

/* low-level Note structure function declarations. */
void	Note_appendbody(noterec *n, nbody *body);
void	Note_deletebody(noterec *n, nbody *body);
void	Note_freeallbodys(noterec *n);

/* object Note high-level implementation-specific function declarations. */
void	Note_addbody(noterec *n, char *text);
void	Note_addbodyline(noterec *n, char *line);
void	Note_str2flags(char *s, char *f);
void	Note_flags2str(char flags, char *s);
noterec	*Note_notebynumber(userrec *u, int num);
int	Note_numforwarded(userrec *u);
int	Note_numsaved(userrec *u);
int	Note_numnew(userrec *u);
int	Note_numread(userrec *u);
int	Note_numnotice(userrec *u);
int	Note_nummemo(userrec *u);
int	Note_deletemarkednotes(userrec *u);
void	Note_sendnotice(userrec *u, char *format, ...);
noterec	*Note_lastnotefromnick(userrec *u_recp, userrec *u_from, char *nick);
void	Note_informuseroflevelplus(userrec *u, u_long flag);
void	Note_informuseroflevelmin(userrec *u, u_long flag);
void	Note_sendnotification(char *format, ...);
int	Note_numnewnote(userrec *u);
void	Note_dequeueforwardednotes();


/* ASDF */
/*****************************************************************************/
/* record noterec declaration - used by userrec object in user.h */
struct noterec_node {
	noterec	*next;			/* -> next */

	char	*nick;			/* nick from */
	char	*uh;			/* uh from */
	char	*finger;		/* finger from user (or uh if !reg) */
	time_t	sent;			/* when note was sent */
	nbody	*body;			/* note text */
	nbody	*body_last;		/* last line */
	int	num_lines;		/* number of lines in body */
	char	flags;			/* flags */
} ;

/* record noterec low-level function declarations. */
noterec	*noterec_new();
void	noterec_freeall(noterec **list, noterec **last);
void	noterec_append(noterec **list, noterec **last, noterec *x);
void	noterec_delete(noterec **list, noterec **last, noterec *x);
void	noterec_movetofront(noterec **list, noterec **last, noterec *lastchecked, noterec *x);

/* record noterec low-level data-specific function declarations. */
void	noterec_init(noterec *n);
u_long	noterec_sizeof(noterec *n);
void	noterec_free(noterec *n);
void	noterec_setnick(noterec *n, char *nick);
void	noterec_setuh(noterec *n, char *uh);
void	noterec_setfinger(noterec *n, char *finger);
void	noterec_display(noterec *n);



/*****************************************************************************/
/* record nbody declaration */
struct nbody_node {
	nbody	*next;			/* -> next */

	char	*text;			/* body of message */
} ;

/* record nbody low-level function declarations. */
nbody	*nbody_new();
void	nbody_freeall(nbody **list, nbody **last);
void	nbody_append(nbody **list, nbody **last, nbody *x);
void	nbody_delete(nbody **list, nbody **last, nbody *x);
void	nbody_movetofront(nbody **list, nbody **last, nbody *lastchecked, nbody *x);

/* record nbody low-level data-specific function declarations. */
void	nbody_init(nbody *n);
u_long	nbody_sizeof(nbody *n);
void	nbody_free(nbody *n);
void	nbody_settext(nbody *n, char *text);
void	nbody_display(nbody *w);



/*****************************************************************************/
/* Note flags */
#define NOTE_READ	0x0001		/* Note has been read */
#define NOTE_NEW	0x0002		/* Note is a new note */
#define NOTE_MEMO	0x0004		/* Note is of type memo */
#define NOTE_SAVE	0x0008		/* Note has been marked 'save' */
#define NOTE_DELETEME	0x0010		/* Note has been marked for delete */
#define NOTE_NOTICE	0x0020		/* Grufti Notice */
#define NOTE_REQDEL	0x0040		/* New Note was requested delete */
#define NOTE_FORWARD	0x0080		/* Note needs to be forwarded */
#define NOTE_WALL	0x0100		/* Note has been wall'd */

#endif /* NOTIFY_H */
