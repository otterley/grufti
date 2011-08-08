/******************************************************************************
 * Steve Rider - stever@area23.org
 * Grufti - an interactive, service-oriented bot for IRC 
 *
 * dummycgi - basic CGI interface for Grufti, testing purposes only
 *
 *****************************************************************************/
/* 28 April 97 */

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include "cgic.h"

/* prototype */
void putpage(char *format, ...);

/* context logging */
char	cx_file[30];
int	cx_line;


int cgiMain()
{
	/* Keep this first! */
	cgiHeaderContentType("text/html");

	putpage("<html><head>");
	putpage("<title>dummy page entry</title></head>");
	putpage("<body bgcolor=\"#000000\" text=\"#abaca4\">");
	putpage("<br><br>");
	putpage("Here's some test stuff.");
	putpage("<center>");
	putpage("</td></tr></table>");
	putpage("</center></body></html>");

	return 0;
}

void putpage(char *format, ...)
{
	va_list va;
	static char websay_buf[4096];

	websay_buf[0] = 0;
	va_start(va,format);
	vsprintf(websay_buf,format,va);
	va_end(va);

	fprintf(cgiOut,"%s\n",websay_buf);
}

