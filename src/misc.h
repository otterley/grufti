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

#include "grufti.h"
#include <time.h>

#define EXTRA_CHAR(q)  q == 1 ? "" : "s"
#define EXTRA_SPC(q)  q < 10 ? " " : ""
#define isequal(s1,s2)  strcasecmp(s1,s2) == 0
#define isequalcase(s1,s2)  strcmp(s1,s2) == 0
#define upcase(c) (((c)>='a' && (c)<='z') ? (c)-'a'+'A' : (c))

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
int	leap(int year);
int	horoscope(char *s, int m, int d, int y);
int	horoscope_index(int m, int d, int y);
int	check_date(int year, int mm, int dd);
int	bday_plus_hor(char *s, char *hor, int m, int d, int y);
int	bday(char *s, int m, int d, int y);
int	which_month(char *s);
void	month_str_long(char *s, int m);
void	month_str_short(char *s, int m);
int	daysuffix(char *s, int d);
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
void	mdy_today(int *m, int *d, int *y);
void	date_token(time_t time, int type, char *timebuf);
void	time_token(time_t time, int type, char *timebuf);
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
int	thishour();
void	splituh(char *u, char *h, char *uh);
void	makehostmask(char *m_uh, char *uh);
void	weekdaytoday_to_str(char *day_today);
int	pdf_uniform(int a, int b);
void	stripfromend(char *s);
void	stripfromend_onlypunctuation(char *s);
u_long	year_to_days(int year);
void	uncalc_days(int *m, int *d, int *year, u_long date);
u_long	calc_days(int m, int d, int year);
int	day_of_week(int m, int d, int year);
u_long	dates_difference(int m1, int d1, int y1, int m2, int d, int y2);
int	calc_new_date(int *mm, int *dd, int *year, u_long offset);
int	week_number(int mm, int dd, int year);

/* misc.c */
char *xmemcpy(char *dest, const char *src, size_t len);

/* strcasecmp.c */
int strcasecmp(const char *s1, const char *s2);
int strncasecmp(const char *s1, const char *s2, size_t n);

/* xmem.c */
#define xmalloc(x) mem_malloc((x), __FILE__, __LINE__)
#define xrealloc(x,y) mem_realloc((x), (y), __FILE__, __LINE__)
#define xfree(x) mem_free((x), __FILE__, __LINE__)

void mem_init();
void *mem_malloc(int size, char *file, int line);
void *mem_realloc(void *ptr, int size, char *file, int line);
void mem_strcpy(char **dest, char *src);
int mem_free(void *ptr, char *file, int line);
u_long mem_total_memory();
int mem_total_alloc();
u_long mem_file_memory(char *file);
int mem_file_alloc(char *file);
void mem_dump_memtbl();

char **getfiles(const char *dir_name, int *number_entries);

#endif /* MISC_H */
