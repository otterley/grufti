/******************************************************************************
 * Steve Rider - stever@area23.org
 * Grufti - an interactive, service-oriented bot for IRC 
 *
 * log.h - logging
 *
 *****************************************************************************/
/* 28 April 97 */

#ifndef LOG_H
#define LOG_H

#include <stdio.h>
#include "typedefs.h"
#include "grufti.h"

/* Let all files who need a Log have access to the Log object */
extern struct Log_object *Log;


/*****************************************************************************/
/* object Log declaration */
typedef struct Log_object {
	logrec	*logs;			/* -> list of logs */
	logrec	*last;			/* -> the last log record */

	int	num_logs;		/* number of logs */

	char 	send_buf[BUFSIZ];	/* send buffer */
	char 	send_buf1[BUFSIZ];	/* send buffer */

} Log_ob;

/* object Log low-level function declarations. */
void	Log_new();
u_long	Log_sizeof();
void	Log_freeall();

/* object Log data-specific function declarations. */
void	Log_display();

/* low-level Log structure function declarations. */
void	Log_appendlog(logrec *l);
void	Log_deletelog(logrec *l);
void	Log_freealllogs();

/* object Log high-level implementation-specific function declarations. */
int	Log_addnewlog(char *filename, char *days_str, char *type_str, char *chname);
int	Log_str2logtype(char *type);
void	Log_logtype2str(int type, char *s);
logrec	*Log_log(int type);
void	Log_write(int type, char *chname, char *format, ... );
void	Log_carryoveralllogs();


/* ASDF */
/*****************************************************************************/
/* record logrec declaration */

struct logrec_node {
	logrec	*next;			/* -> next */

	char	*filename;		/* Name of file */
	char	*chname;		/* Name of channel */
	int	type;			/* type of log */
	int days;			/* how many days of logs should be kept */
} ;

/* record logrec low-level function declarations. */
logrec	*logrec_new();
void	logrec_freeall(logrec **list, logrec **last);
void	logrec_append(logrec **list, logrec **last, logrec *entry);
void	logrec_delete(logrec **list, logrec **last, logrec *x);
void	logrec_movetofront(logrec **list, logrec **last, logrec *lastchecked, logrec *l);

/* record logrec low-level data-specific function declarations. */
void	logrec_init(logrec *l);
u_long	logrec_sizeof(logrec *l);
void	logrec_free(logrec *l);
void	logrec_setfilename(logrec *l, char *filename);
void	logrec_setchname(logrec *l, char *chname);
void	logrec_display(logrec *l);
logrec	*logrec_log(logrec **list, logrec **last, int type);




/*****************************************************************************/
/* logfile types */
#define LOG_PUBLIC 0x0001  /* p   public chatter, joins, parts, modes, kicks. */
#define LOG_CMD    0x0002  /* c   dcc connections, commands via dcc or msg. */
#define LOG_ERROR  0x0004  /* e   'oops' error messages.  inconsistencies. */
#define LOG_RAW    0x0008  /* r   raw server stuff coming in and going out. */
#define LOG_MAIN   0x0010  /* m   bot info and running status log. */
#define LOG_DEBUG  0x0020  /* d   debug and testing logging. */

#define LOG_ALL    0x003f  /* a   (dump to all logfiles) */

#endif /* LOG_H */
