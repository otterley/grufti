/******************************************************************************
 * Steve Rider - stever@area23.org
 * Grufti - an interactive, service-oriented bot for IRC 
 *
 * log.c - grufti-specific variables and main Grufti object
 *
 *****************************************************************************/
/* 28 April 97 */

#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <sys/types.h>
#include <fcntl.h>
#include "grufti.h"
#include "gruftiaux.h"
#include "misc.h"
#include "log.h"
#include "dcc.h"

/***************************************************************************n
 *
 * Log object definitions
 *
 ****************************************************************************/

void Log_new()
{
	/* already in use? */
	if(Log != NULL)
		Log_freeall();

	/* allocate memory */
	Log = (Log_ob *)xmalloc(sizeof(Log_ob));

	/* initialize */
	Log->logs = NULL;
	Log->last = NULL;

	Log->num_logs = 0;

	Log->send_buf[0] = 0;
	Log->send_buf1[0] = 0;
}

u_long Log_sizeof()
{
	u_long	tot = 0L;
	logrec	*l;

	tot += sizeof(Log_ob);
	for(l=Log->logs; l!=NULL; l=l->next)
		tot += logrec_sizeof(l);

	return tot;
}

void Log_freeall()
{
	/* Free all log records */
	Log_freealllogs();

	/* Free this object */
	xfree(Log);
	Log = NULL;
}

/****************************************************************************/
/* object Log data-specific function definitions. */

void Log_display()
{
	logrec *x;

	Log_write(LOG_MAIN,"*","%d logs.",Log->num_logs);
	for(x=Log->logs; x!=NULL; x=x->next)
		logrec_display(x);
}

/****************************************************************************/
/* low-level Log structure function definitions. */

void Log_appendlog(logrec *l)
{
	logrec_append(&Log->logs, &Log->last, l);
	Log->num_logs++;
}

void Log_delete(logrec *l)
{
	logrec_delete(&Log->logs, &Log->last, l);
	Log->num_logs--;
}

void Log_freealllogs()
{
	logrec_freeall(&Log->logs, &Log->last);
	Log->num_logs = 0;
}

/****************************************************************************/
/* object Log high-level implementation-specific function definitions. */

int Log_addnewlog(char *filename, char *days_str, char *type_str, char *chname)
/*
 * Function: add new log type
 *  Returns: 0  if OK
 *           -1 if type RAW not defined
 *           -2 log filename not quoted
 */
{
	logrec *x;
	int type;

	type = Log_str2logtype(type_str);

#ifndef RAW_LOGGING
	if(type & LOG_RAW)
		return -1;
#endif

	if(!rmquotes(filename))
		return -2;

	/* Create new log */
	x = logrec_new();

	/* Assign it some data */
	if(chname[0])
		rmquotes(chname);
	else
		strcpy(chname,"*");
	logrec_setfilename(x,filename);
	logrec_setchname(x,chname);
	x->days = atoi(days_str);
	x->type = type;

	/* Add it to the list */
	Log_appendlog(x);

	return 0;
}
	
void Log_logtype2str(int t, char *s)
{
	s[0] = 0;
	if(t & LOG_CMD)		strcat(s,"c");
	if(t & LOG_DEBUG)	strcat(s,"d");
	if(t & LOG_ERROR)	strcat(s,"e");
	if(t & LOG_MAIN)	strcat(s,"m");
	if(t & LOG_PUBLIC)	strcat(s,"p");
#ifdef RAW_LOGGING
	if(t & LOG_RAW)		strcat(s,"r");
#endif
	if(t & LOG_ALL)		strcat(s,"a");
	if(s[0] == 0)
	{
		Log_write(LOG_ERROR,"*","Config file error: logfile has no log type %d.",t);
	}
}

int Log_str2logtype(char *type)
{
	char *p;
	int res = 0;

	for(p=type; *p; p++)
	{
		switch(*p)
		{
			case 'm' : case 'M' : res |= LOG_MAIN;	break;
			case 'c' : case 'C' : res |= LOG_CMD;	break;
			case 'p' : case 'P' : res |= LOG_PUBLIC;break;
#ifdef RAW_LOGGING
			case 'r' : case 'R' : res |= LOG_RAW;	break;
#endif
			case 'd' : case 'D' : res |= LOG_DEBUG;	break;
			case 'e' : case 'E' : res |= LOG_ERROR;	break;
			case 'a' : case 'A' : res |= LOG_ALL;	break;
			default : 
				Log_write(LOG_ERROR,"*","Config file error: Unknown logfile type \"%c\"",*p);
		}
	}

	return res;
}

logrec *Log_log(int type)
{
	return(logrec_log(&Log->logs, &Log->last, type));
}

void Log_write(int type, char *chname, char *format, ... )
{
	va_list	va;
	logrec	*l;
	dccrec	*d;
	FILE	*f;
	char	logtime[TIMESHORTLEN], logfile[256];

	if(type == 0)
	{
		Log_write(LOG_ERROR,"*","Invalid logfile type.  Log_write() probably incorrectly formatted.");
		return;
	}
	va_start(va,format);
	vsprintf(Log->send_buf1,format,va);
	va_end(va);

	/* Write logtime */
	timet_to_time_short(time(NULL),logtime);
	sprintf(Log->send_buf,"[%s] %s",logtime,Log->send_buf1);

	/*
	 * Find logfiles with matching type.  We can't use our builtin
	 * log_log() routine since more than one type of log may be specified
	 * in 'type'.  So we do our own search.  Don't movetofront though...
	 */
	for(l=Log->logs; l!=NULL; l=l->next)
	{
		if(l->type & type)
		{
			if((chname[0] == '*') || (l->chname[0] == '*') || (isequal(chname,l->chname)))
			{
				/* open file and write to it. */
				sprintf(logfile,"%s/%s",Grufti->logdir,l->filename);
				f = fopen(logfile,"a+");
				if(f != NULL)
				{
					fprintf(f,"%s\n",Log->send_buf);
					fclose(f);
				}
			}
		}
	}

	/*
	 * Anyone out there in dccland listening?
	 * We need to be really careful we don't get into a recursive loop
	 * here.  Don't forget, DCC_write() calls Log_write!  So don't use
	 * it.  We'll use DCC_write_nolog().
	 */
	for(d=DCC->dcclist; d!=NULL; d=d->next)
	{
		if(d->logtype & type)
		{
			if((chname[0] == '*') || (d->logchname[0] == '*') || (isequal(chname,d->logchname)))
			{
				DCC_write_nolog(d,"%s",Log->send_buf);
			}
		}
	}
}

void Log_carryoveralllogs()
{
	logrec	*l;
	int	x, i, num_files, m, d, y;
	char log_yesterday[256], logfile[256], logfile_delete[256];
	char dummy[25], buffer[256];
	char **files, *p;
	u_long date, expiration;
	time_t t;
	struct tm *btime;

	Log_write(LOG_MAIN,"*","Switching logfiles...");
	for(l=Log->logs; l!=NULL; l=l->next)
	{
		sprintf(logfile,"%s/%s",Grufti->logdir,l->filename);
		if(isfile(logfile))
		{
			/*
			 * Move logfile to logfile.yyyy_mm_dd where "dd" is yesterday
			 */
			t = time(NULL) - 86400;
			btime = localtime(&t);
			sprintf(log_yesterday,"%s.%4d_%02d_%02d", logfile,
				btime->tm_year + 1900, btime->tm_mon + 1, btime->tm_mday);
			x = movefile(logfile,log_yesterday);
			verify_write(x,logfile,log_yesterday);

			/*
			 * Write log files indefinitely when days = 0
			 */
			if(l->days == 0)
				continue;

			t = time(NULL) - (86400 * l->days);
			btime = localtime(&t);
			sprintf(dummy, "%4d%02d%02d", btime->tm_year + 1900, btime->tm_mon + 1, btime->tm_mday);
			expiration = atol(dummy);
			
			/*
			 * Remove logfiles older than expiration date
			 */
			files = getfiles(Grufti->logdir, &num_files);
			if(files)
			{
				for(i = 0; i < num_files; i++)
				{
					if(strncmp(files[i], l->filename, strlen(l->filename)) == 0)
					{
						/*
						 * Get date of file, where date is in yyyy_mm_dd form
						 */
						strcpy(buffer, files[i] + strlen(l->filename));
						p = strchr(buffer, '.');
						if(p && strlen(p) == 11)
						{
							p++;
							d = atoi(p + 8);
							p[7] = 0;
							m = atoi(p + 5);
							p[4] = 0;
							y = atoi(p);
							sprintf(dummy, "%4d%02d%02d", y, m, d);
							date = atol(dummy);

							if(date < expiration)
							{
								sprintf(logfile_delete, "%s/%s", Grufti->logdir, files[i]);
								unlink(logfile_delete);
							}
						}
					}
				}

				for(i = 0; i < num_files; i++)
					xfree(files[i]);
				xfree(files);
			}
		}
	}
}
	
/* ASDF */
/****************************************************************************
 *
 * logrec record definitions.  No changes should be made to new(), freeall(),
 * append(), delete(), or movetofront().
 *
 ****************************************************************************/

logrec *logrec_new()
{
	logrec *x;

	/* allocate memory */
	x = (logrec *)xmalloc(sizeof(logrec));

	/* initialize */
	x->next = NULL;

	logrec_init(x);

	return x;
}

void logrec_freeall(logrec **list, logrec **last)
{
	logrec *l = *list, *v;

	while(l != NULL)
	{
		v = l->next;
		logrec_free(l);
		l = v;
	}

	*list = NULL;
	*last = NULL;
}

void logrec_append(logrec **list, logrec **last, logrec *entry)
{
	logrec *lastentry = *last;

	if(*list == NULL)
		*list = entry;
	else
		lastentry->next = entry;

	*last = entry;
	entry->next = NULL;
}

void logrec_delete(logrec **list, logrec **last, logrec *x)
{
	logrec *l, *lastchecked = NULL;

	l = *list;
	while(l != NULL)
	{
		if(l == x)
		{
			if(lastchecked == NULL)
			{
				*list = l->next;
				if(l == *last)
					*last = NULL;
			}
			else
			{
				lastchecked->next = l->next;
				if(l == *last)
					*last = lastchecked;
			}

			logrec_free(l);
			return;
		}

		lastchecked = l;
		l = l->next;
	}
}

void logrec_movetofront(logrec **list, logrec **last, logrec *lastchecked, logrec *l)
{
	if(lastchecked != NULL)
	{
		if(*last == l)
			*last = lastchecked;

		lastchecked->next = l->next;
		l->next = *list;
		*list = l;
	}
}

/****************************************************************************/
/* record cmdrec low-level data-specific function definitions. */

void logrec_init(logrec *l)
{
	/* initialize */
	l->filename = NULL;
	l->chname = NULL;
	l->type = 0;
	l->days = 0;
}

u_long logrec_sizeof(logrec *l)
{
	u_long	tot = 0L;

	tot += sizeof(logrec);
	tot += l->filename ? strlen(l->filename)+1 : 0L;
	tot += l->chname ? strlen(l->chname)+1 : 0L;

	return tot;
}

void logrec_free(logrec *l)
{
	/* Free the elements */
	xfree(l->filename);
	xfree(l->chname);

	/* Free this record. */
	xfree(l);
}

void logrec_setfilename(logrec *l, char *filename)
{
	mstrcpy(&l->filename,filename);
}

void logrec_setchname(logrec *l, char *chname)
{
	mstrcpy(&l->chname,chname);
}

void logrec_display(logrec *l)
{
	char s[LOGFLAGLEN];

	Log_logtype2str(l->type,s);
	Log_write(LOG_MAIN,"*","%s %s %s",l->filename,s,l->chname);
}

logrec *logrec_log(logrec **list, logrec **last, int type)
{
	logrec *l, *lastchecked = NULL;

	for(l=*list; l!=NULL; l=l->next)
	{
		if(l->type == type)
		{
			/* found it.  do a movetofront. */
			logrec_movetofront(list,last,lastchecked,l);

			return l;
		}
		lastchecked = l;
	}

	return NULL;
}
