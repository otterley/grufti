/******************************************************************************
 * Steve Rider - stever@area23.org
 * Grufti - an interactive, service-oriented bot for IRC 
 *
 * fhelp.c - format and read help files
 *
 *****************************************************************************/
/* 28 April 97 */

#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/time.h>
#include <time.h>
#include "grufti.h"
#include "gruftiaux.h"
#include "misc.h"
#include "log.h"

/* global buffers for i/o */
static	char	ftext[BUFSIZ];
static	char	fword[BUFSIZ];


u_long fhelp_sizeof()
{
	u_long	tot = 0L;

	tot += sizeof(ftext);
	tot += sizeof(fword);

	return tot;
}

char *read_word(FILE *ffile)
/*
 * Pre: ffile is opened, word points to allocated memory (string)
 * Post: read_word returns word
 *
 * reads from file until ' ' or '\n' and returns word.
 * assumes no eof() is encountered
 */
{
	char	c;

	fword[0] = 0;
	while(1)
	{	
		c = fgetc(ffile);
		switch(c)
		{
		case ' ': 
			return fword;
		case '\n': 
			return fword;
		default:
			sprintf(fword,"%s%c",fword,c);
			break;
		}	
	}
}

/******************************************************************************/

void translate_escape(char *s, char c)
/*
 * Translates an escaped character c using to following rules:
 *	b	^B
 *	u	^_
 *	i	^V
 *	n	bot's original nick
 *      h       host
 *      w       web page
 *      t       port
 *	v	version
 *	m	maintainer
 *	c	copyright
 *      \	\
 *	#	#
 *	r	return
 */
{
	switch(c)
	{
	case 'b':	/* bold */
		strcat(s,"\002");
		break;
	case 'u':	/* underline */
		strcat(s,"\031");
		break;
	case 'i':	/* inverse */
		strcat(s,"\022");
		break;
	case 'n':	/* nick */
		strcat(s,Grufti->botnick);
		break;
	case 'w':
		strcat(s,Grufti->homepage);
		break;
	case 'h':
		strcat(s,Grufti->bothostname);
		break;
	case 't':
		sprintf(s,"%s%d",s,Grufti->actual_telnet_port);
		break;
	case 'v':
		strcat(s,Grufti->build);
		break;
	case 'm':
		strcat(s,Grufti->maintainer);
		break;
	case 'c':
		strcat(s,Grufti->copyright);
		break;
	case '\\':
		strcat(s,"\\");
		break;
	case '#':
		strcat(s,"#");
                break;
	default:
		Log_write(LOG_ERROR,"*","Unknown help file escape sequence \'%c\'\n",c);
		break;
	}
}
	

void skipcomment(FILE *ffile)
/*
 * Read characters from the file until eoln.
 */
{
	while(fgetc(ffile) != '\n')
		;
}


int find_topic(FILE *ffile, char *topic)
/*
 * moves the filepointer until the beginning of topic "topic".
 * returns 1 if topic found, else 0
 */
{
	char	c;
	int	esc = 0;
	char	command[BUFSIZ];

	while(!feof(ffile))
	{
		c = fgetc(ffile);
		switch(c)
		{

		case '\\':
			esc = 1;
			break;			

		case '#':
			if(!esc)
				skipcomment(ffile);
			else
				esc = 0;
			break;

		case '%':
			if(!esc)
			{
				strcpy(command,read_word(ffile));
				if(isequal(command,"subject"))
					if(isequal(read_word(ffile),topic))
						return 1;
			}
			else
				esc = 0;
			break;

		default:
			esc = 0;
			break;
		}
	}
	return 0;
}


char *get_ftext(FILE *ffile)
/*
 * Pre: ffile has been opened
 * Post: get_ftext returns NULL on eof, or else a parsed string
 */
{
	char	command[BUFSIZ];
	char	c;
	
	ftext[0] = 0;

	while(!feof(ffile))
	{
		c = fgetc(ffile);
		switch(c)
		{
		case '%':
			/* we read a command */
			strcpy(command,read_word(ffile));
			if(isequal(command,"end"))
				return(NULL);
			if(isequal(command,"line"))
			{
				strcat(ftext,"   ");
				return(ftext);
			}
			break;

		case '\\':
			c = fgetc(ffile);
			translate_escape(ftext,c);
			break;	

		case '\n':
			return(ftext);
			break;

		default:
			sprintf(ftext,"%s%c",ftext,c);
			break;
		}	
	}
	return NULL;
}
