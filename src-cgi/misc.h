/******************************************************************************
 * Steve Rider - stever@area23.org
 * Grufti - an interactive, service-oriented bot for IRC 
 *
 * misc.h - declarations for generic, miscellaneous routines
 *
 *****************************************************************************/
/* 28 April 97 */

#ifndef MISC_H
#define MISC_H

#include <sys/time.h>
#include <time.h>
/* jeez */
#include <sys/types.h>
#ifndef u_long
#define u_long unsigned long
#endif

#ifndef u_short
#define u_short unsigned short
#endif

#ifndef u_char
#define u_char unsigned char
#endif

#ifndef u_int
#define u_int unsigned int
#endif

#define EXTRA_CHAR(q)  q == 1 ? "" : "s"
#define EXTRA_SPC(q)  q < 10 ? " " : ""
#define isequal(s1,s2)  strcasecmp(s1,s2) == 0
#define isequalcase(s1,s2)  strcmp(s1,s2) == 0
#define upcase(c) (((c)>='a' && (c)<='z') ? (c)-'a'+'A' : (c))
#define nmalloc(x)	n_malloc((x),__FILE__,__LINE__)
#define nfree(x)	n_free((x),__FILE__,__LINE__)

#define TIMESHORTLEN	50
#define UHOSTLEN	256

#define TIME_HOUR 1
#define TIME_MIN 2
#define TIME_SEC 3
#define TIME_HOUR_0 4
#define TIME_MIN_0 5
#define TIME_SEC_0 6
#define DATE_MONTH 1
#define DATE_DAY_0 2
#define DATE_YEAR 3
#define DATE_DAY 4
#define DATE_YEAR_19 5
#define DATE_MONTH_0 6

u_long	misc_sizeof();
int	copyfile(char *oldpath, char *newpath);
int	movefile(char *oldpath, char *newpath);
int	isfile(char *filename);
void	dumplots(int idx, char *prefix, char *data);
char	*readline(FILE *f, char *s);
void	bytes_to_kb_short(u_long bytes, char *kb);
void	bytes_to_kb_long(u_long bytes, char *kb);
void	timet_to_date_short(time_t time, char *date);
void	timet_to_dateonly_short(time_t time, char *date);
void	timet_to_date_long(time_t time, char *date);
void	timet_to_time_short(time_t tt, char *time);
time_t	time_today_began(time_t now);
void	date_token(long time, int type, char *timebuf);
void	time_token(long time, int type, char *timebuf);
void	timet_to_ago_short(time_t when, char *ago);
void	time_to_elap_short(time_t dur, char *elap);
void	timet_to_ago_long(time_t when, char *ago);
void	timet_to_elap_short(time_t t, char *elap);
void	comma_number(char *string, u_long sum);
void	*n_malloc(int size, char *file, int line);
void	n_free(void *ptr, char *file, int line);
void	splitc(char *first, char *rest, char divider);
void	split(char *first, char *rest);
int	num_elements(char *s);
void	rmspace(char *s);
int	rmquotes(char *s);
int	isanumber(char *s);
void	fixcolon(char *s);
void	str_element(char *out, char *in, int item);
int	today();
int	yesterday();
int	tomorrow();
void	splituh(char *u, char *h, char *uh);
void	makehostmask(char *m_uh, char *uh);
void	weekdaytoday_to_str(char *day_today);
int	pdf_uniform(int a, int b);
void	stripfromend(char *s);
void	stripfromend_onlypunctuation(char *s);

void	mstrcpy(char **dest, char *src);
void	mfree(void *ptr);
void	*n_malloc(int size, char *file, int line);
void	n_free(void *ptr, char *file, int line);

#endif /* MISC_H */
