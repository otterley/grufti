/******************************************************************************
 * Steve Rider - stever@area23.org
 * Grufti - an interactive, service-oriented bot for IRC 
 *
 * misc.c - generic, miscellanous routines
 *
 *****************************************************************************/
/* 28 April 97 */

#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <fcntl.h>
#include <ctype.h>
#include "grufti.h"
#include "misc.h"

static char *months_long[] =
{
	"January","February","March","April","May","June","July",
	"August","September","October","November","December"
};

static char *months[] =
{
	"Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec"
};

static char *week_days[] =
{
	"Sun","Mon","Tue","Wed","Thu","Fri","Sat"
};

static char *week_days_long[] =
{
	"Sunday","Monday","Tuesday","Wednesday","Thursday","Friday","Saturday"
};

static char *day_suffix[] =
{
	"th","st","nd","rd","th","th","th","th","th","th","th","th","th","th",
	"th","th","th","th","th","th","th","st","nd","rd","th","th","th","th",
	"th","th","th","st"
};

static int month_length[2][13] =
{
	{ 0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 },
	{ 0, 31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 }
};

static int days_in_months[2][14] =
{
	{ 0, 0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334, 365 },
	{ 0, 0, 31, 60, 91, 121, 152, 182, 213, 244, 274, 305, 335, 366 }
};

static char *horoscope_signs[] =
{
	"","Capricorn","Aquarius","Pisces","Aries","Taurus","Gemini",
	"Cancer","Leo","Virgo","Libra","Scorpio","Sagittarius",
	"Capricorn","Aquarius"
};

static int horoscope_dates[] =
{
	0, 19, 18, 20, 19, 20, 20, 22, 22, 22, 22, 21, 21, 19
};

u_long misc_sizeof()
{
	u_long	tot = 0L;

	tot += sizeof(months_long);
	tot += sizeof(months);
	tot += sizeof(week_days);
	tot += sizeof(week_days_long);
	tot += sizeof(day_suffix);
	tot += sizeof(month_length);
	tot += sizeof(horoscope_signs);
	tot += sizeof(horoscope_dates);
	tot += sizeof(days_in_months);

	return tot;
}

int horoscope(char *s, int m, int d, int y)
{
	/* Take m,d,y, convert to horoscope sign */
	s[0] = 0;

	if(!check_date(m,d,y))
		return 0;

	if(d > horoscope_dates[m])
		strcpy(s,horoscope_signs[m+1]);
	else
		strcpy(s,horoscope_signs[m]);

	return 1;
}

int horoscope_index(int m, int d, int y)
{
	if(!check_date(m,d,y))
		return 0;

	if(d > horoscope_dates[m])
		return m+1;
	else
		return m;
}

int leap(int year)
{
	/* If year is -1, then no year is set, so we assume the year could
	 * be a leap year.
	 */
	if(year == -1)
		return 1;
	else
		return((((year % 4) == 0) && ((year % 100) != 0)) || ((year % 400) == 0));
}

int check_date(int mm, int dd, int year)
{
	if((mm < 1) || (mm > 12))
		return 0;
	if((dd < 1) || (dd > month_length[leap(year)][mm]))
		return 0;
	return 1;
}

int which_month(char *s)
{
	int i;

	for(i=0; i<12; i++)
	{
		if(isequal(s,months[i]) || isequal(s,months_long[i]))
			return i + 1;
	}

	return 0;
}

void month_str_long(char *s, int m)
{
	strcpy(s,months_long[m]);
}

void month_str_short(char *s, int m)
{
	strcpy(s,months[m]);
}

int daysuffix(char *s, int d)
{
	s[0] = 0;

	if(d < 0 || d > 31)
		return 0;

	sprintf(s,"%s",day_suffix[d]);

	return 1;
}

int bday_plus_hor(char *s, char *hor, int m, int d, int y)
{
	int	x;

	hor[0] = 0;

	x = bday(s,m,d,y);
	if(x)
		strcpy(hor,horoscope_signs[horoscope_index(m,d,y)]);

	return x;
}

int bday(char *s, int m, int d, int y)
{
	s[0] = 0;
	if(!check_date(m,d,y))
		return 0;

	if(y == -1)
		sprintf(s,"%s %d%s",months_long[m-1],d,day_suffix[d]);
	else
		sprintf(s,"%s %d%s, %4.4d",months_long[m-1],d,day_suffix[d],y);

	return 1;
}

/* copy a file from one place to another (possibly erasing old copy) */
/* returns 0 if OK, 1 if can't open original file, 2 if can't open new */
/* file, 3 if original file isn't normal, 4 if ran out of disk space */
int copyfile(char *oldpath, char *newpath)
{
	int	fi,fo,x;
	char	buf[512];
	struct	stat st;

	fi = open(oldpath,O_RDONLY,0);
	if(fi < 0)
		return 1;

	fstat(fi,&st);
	if(!(st.st_mode & S_IFREG))
		return 3;

	fo = creat(newpath,(int)(st.st_mode & 0777));
	if(fo < 0)
	{
		close(fi);
		return 2;
	}
	for(x=1; x>0; )
	{
		x = read(fi,buf,512);
		if(x > 0)
		{
			if(write(fo,buf,x) < x)
			{
				close(fo);
				close(fi);
				unlink(newpath);
				return 4;
			}
		}
	}
	close(fo);
	close(fi);

	return 0;
}


int movefile(char *oldpath, char *newpath)
{
	int x = copyfile(oldpath,newpath);
	if (x == 0)
		unlink(oldpath);

	return x;
}


int isfile(char *filename)
{
	struct	stat ss;
	int	i = stat(filename,&ss);

	if(i < 0)
		return 0;

	if((ss.st_mode & S_IFREG) || (ss.st_mode & S_IFLNK))
		return 1;

	return 0;
}

char *readline(FILE *f, char *s)
{
	if(fgets(s,BUFSIZ,f))
	{
		s[strlen(s)-1] = '\0';
		return s;
	}
	return NULL;
}

void bytes_to_kb_short(u_long bytes, char *kb)
{
	if(bytes < 1024)
		sprintf(kb,"%.1fk",(float)(bytes) / 1024.0);
	else if(bytes < 1048576)
		sprintf(kb,"%luk",bytes / 1024);
	else
		sprintf(kb,"%.1fM",(float)(bytes) / 1048576.0);
}

void bytes_to_kb_long(u_long bytes, char *kb)
{
	if(bytes < 1048576)
		sprintf(kb,"%.3fk",(float)(bytes) / 1024.0);
	else
		sprintf(kb,"%.3fM",(float)(bytes) / 1048576.0);
}

void timet_to_time_short(time_t tt, char *time)
{
	strcpy(time,ctime(&tt));
	strcpy(time,&time[11]);
	time[5] = 0;
}
	
void timet_to_date_short(time_t time, char *date)
{
	struct tm *btime;

	btime = localtime(&time);
	sprintf(date,"%-2.2d/%-2.2d/%-4.4d %-2.2d:%-2.2d",btime->tm_mon+1,btime->tm_mday,btime->tm_year+1900,btime->tm_hour,btime->tm_min);
}

void timet_to_dateonly_short(time_t time, char *date)
{
	struct tm *btime;

	btime = localtime(&time);
	sprintf(date,"%-2.2d/%-2.2d/%-4.4d",btime->tm_mon+1,btime->tm_mday,btime->tm_year+1900);
}

void timet_to_date_long(time_t time, char *date)
{
	struct tm *btime;

	btime=localtime(&time);
	sprintf(date,"%s %s %d %-2.2d:%-2.2d:%-2.2d %d",week_days[btime->tm_wday],months[btime->tm_mon],btime->tm_mday,btime->tm_hour,btime->tm_min,btime->tm_sec,btime->tm_year + 1900);
	return;
}

void timet_to_date_formal(time_t time, char *date)
{
	struct tm *btime;

	btime = localtime(&time);
	sprintf(date,"%s, %d %s %d %-2.2d:%-2.2d:%-2.2d",week_days[btime->tm_wday], btime->tm_mday, months[btime->tm_mon], btime->tm_year + 1900, btime->tm_hour, btime->tm_min, btime->tm_sec);
}

void mdy_today(int *m, int *d, int *y)
{
	struct tm *btime;
	time_t now = time(NULL);

	btime = localtime(&now);
	*m = btime->tm_mon+1;
	*d = btime->tm_mday;
	*y = btime->tm_year + 1900;
}

time_t time_today_began(time_t now)
{
	struct tm *btime;

	btime = localtime(&now);
	return now - ((3600 * btime->tm_hour) + (60 * btime->tm_min) + btime->tm_sec);
}

void date_token(time_t time, int type, char *timebuf)
{
	struct tm *btime;

	btime = localtime(&time);
	strcpy(timebuf,"");
	switch(type)
	{
		case (1) : sprintf(timebuf,"%s",months_long[btime->tm_mon]); break;
		case (2) : sprintf(timebuf,"%-2.2d",btime->tm_mday); break;
		case (3) : sprintf(timebuf,"%-2.2d",btime->tm_year + 1900); break;
		case (4) : sprintf(timebuf,"%d",btime->tm_mday); break;
		case (5) : sprintf(timebuf,"%-2.2d",btime->tm_year + 1900);break;
		case (6) : sprintf(timebuf,"%-2.2d",(btime->tm_mon+1)); break;
		case (7) : sprintf(timebuf,"%s",week_days[btime->tm_wday]); break;
		default : return; break;
	}
}

void time_token(time_t time, int type, char *timebuf)
{
	struct tm *btime;

	btime=localtime(&time);
	strcpy(timebuf,"");
	switch(type)
	{
		case (1): sprintf(timebuf,"%d",btime->tm_hour); break;
		case (2): sprintf(timebuf,"%d",btime->tm_min); break;
		case (3): sprintf(timebuf,"%d",btime->tm_sec); break;
		case (4): sprintf(timebuf,"%-2.2d",btime->tm_hour); break;
		case (5): sprintf(timebuf,"%-2.2d",btime->tm_min); break;
		case (6): sprintf(timebuf,"%-2.2d",btime->tm_sec); break;
		default: return ; break;
	}
}

void timet_to_ago_short(time_t when, char *ago)
{
	time_to_elap_short(time(NULL) - when,ago);
}

void time_to_elap_short(time_t dur, char *elap)
{
	int days=0,hrs=0,mins=0,secs=0;

	elap[0] = 0;

	days = (int)(dur / 86400);
	dur -= (int)(dur / 86400) * 86400;

	hrs = (int)(dur / 3600);
	dur -= (int)(dur / 3600) * 3600;

	mins = (int)(dur / 60);
	secs = dur - (int)(dur / 60) * 60;

	if(days)
		sprintf(elap,"%dd",days);
	else if(hrs)
		sprintf(elap,"%dh",hrs);
	else if(mins)
		sprintf(elap,"%dm",mins);
	else
		sprintf(elap,"%ds",secs);
}

void timet_to_ago_long(time_t when, char *ago)
{
	time_t dur;
	int days=0,hrs=0,mins=0,secs=0;

	dur=time(NULL)-when;
		
	days=(int)(dur/86400);
	dur-=(((int)(dur/86400))*86400);

	hrs=((int)(dur/3600));
	dur-=(((int)(dur/3600))*3600);

	mins=((int)(dur/60));
	secs=dur-(((int)(dur/60))*60);

	if(days)
		sprintf(ago,"%d day%s, %d hr%s",days,EXTRA_CHAR(days),hrs,EXTRA_CHAR(hrs));
	else if(hrs)
		sprintf(ago,"%d hr%s, %d min%s",hrs,EXTRA_CHAR(hrs),mins,EXTRA_CHAR(mins));
	else if(mins)
		sprintf(ago,"%d min%s, %d sec%s",mins,EXTRA_CHAR(mins),secs,EXTRA_CHAR(secs));
	else
		sprintf(ago,"%d second%s",secs,EXTRA_CHAR(secs));

	return;
}

void timet_to_elap_short(time_t t, char *elap)
{
	char s1[TIMESHORTLEN];

	if(t > 0)
	{
		strcpy(elap,ctime(&(t)));

		if((time(NULL)-t) > 86400)
		{
			strcpy(s1,&elap[4]);
			strcpy(elap,&elap[8]);
			strcpy(&elap[2],s1);
			elap[5] = 0;
		}
		else
		{
			strcpy(elap,&elap[11]);
			elap[5] = 0;
		}
	}
	else
		strcpy(elap,"?????");
}

void comma_number(char *string, u_long sum)
{
	u_long mod = 0;
	char o_bytes[64];

	strcpy(o_bytes,"");
	while(sum>999)
	{
		mod = sum%1000;
		sum = sum/1000;
		sprintf(string,",%-3.3lu%s",mod,o_bytes);
		strcpy(o_bytes,string);
	}
	sprintf(string,"%lu%s",sum,o_bytes);
	return;
}

/* split first word off of rest and put it in first */
void splitc(char *first, char *rest, char divider)
{
       char *p;

       p = strchr(rest,divider);
       if(p == NULL)
       {
                if((first != rest) && (first != NULL))
		{
			strcpy(first,rest);
                        rest[0] = 0;
		}
                return;
        }
        *p = 0;
        if(first != NULL)
                strcpy(first,rest);
        if(first != rest)
                strcpy(rest,p+1);
}

void split(char *first, char *rest)
{
        splitc(first,rest,' ');
/*
	if(first != NULL)
	{
		if(!first[0])
		{
			strcpy(first,rest);
			rest[0] = 0;
		}
	}
*/
}

void rmspace(char *s)
{
#define whitespace(c) ( ((c)==32) || ((c)==9) || ((c)==13) || ((c)==10) )
	char *p;

	/* wipe end of string */
	for(p=s+strlen(s)-1; ((whitespace(*p)) && (p>=s)); p--);

	if(p != s+strlen(s)-1)
		*(p+1) = 0;
	for(p=s; ((whitespace(*p)) && (*p)); p++);

	if(p != s)
		strcpy(s,p);
}

int rmquotes(char *s)
{
	char *p, last = '\0';
	int numquotes = 0;

	rmspace(s);

	/* count number of quotes */
	for(p=s; *p!='\0'; p++)
	{
		if(*p == '"' && last != '\\')
			numquotes++;
		last = *p;
	}

	if(numquotes != 2)
		return 0;

	/* if last char is not the quote, and first char is not the quote */
	if(s[0] != '"' && s[strlen(s)-1] != '"')
		return 0;

	s[strlen(s)-1] = '\0';
	strcpy(s,s+1);

	return 1;
}

int isanumber(char *s)
{
	char *p;

	for(p=s; *p!='\0'; p++)
		if(!isdigit((int)*p))
			return 0;

	return 1;
}

void fixcolon(char *s)
{
	if (s[0]==':')
		strcpy(s,&s[1]);
	else
		split(s,s);
}

void str_element(char *out, char *in, int item)
{
	char *p, *p1, *pbuf, buf[BUFSIZ];
	int on_token_last = 0, num_tokens = 0, done = 0;

	for(p=in; *p && !done; p++)
	{
		if(whitespace(*p))
			on_token_last = 0;
		else
		{
			if(!on_token_last)
				num_tokens++;
			on_token_last = 1;
			if(num_tokens == item)
			{
				pbuf = buf;
				for(p1=p; !(whitespace(*p1)) && *p1; p1++)
					*pbuf++ = *p1;
				*pbuf = 0;
				done = 1;
			}
		}
	}

	if(done)
		strcpy(out,buf);
	else
		strcpy(out,"");
}

void quoted_str_element(char *out, char *in, int item)
{
	char *p, *p1, *pbuf, buf[BUFSIZ];
	int on_token_last = 0, num_tokens = 0, done = 0;
	int inquote = 0;

	for(p=in; *p && !done; p++)
	{
		if(*p == '"')
			inquote = inquote?0:1;
		if(whitespace(*p) && !inquote)
			on_token_last = 0;
		else
		{
			if(!on_token_last)
				num_tokens++;
			on_token_last = 1;
			if(num_tokens == item)
			{
				pbuf = buf;
				inquote = 0;
				for(p1=p; (!(whitespace(*p1)) || inquote) && *p1; p1++)
				{
					*pbuf++ = *p1;
					if(*p1 == '"')
						inquote = inquote?0:1;
				}
				*pbuf = 0;
				done = 1;
			}
		}
	}

	if(done)
		strcpy(out,buf);
	else
		strcpy(out,"");
}

int num_elements(char *s)
{
	char *p;
	int on_token_last = 0, num_tokens = 0;

	for(p=s; *p; p++)
	{
		if(whitespace(*p))
			on_token_last = 0;
		else
		{
			if(!on_token_last)
				num_tokens++;
			on_token_last = 1;
		}
	}
	return num_tokens;
}

int today()
{
	struct tm *nowtm;
	time_t now = time(NULL);

	nowtm = localtime(&now);
	return nowtm->tm_wday;
}

int yesterday()
{
	int yest;

	yest = today() - 1;
	if(yest < 0)
		yest = 6;

	return yest;
}

int tomorrow()
{
	struct tm *tomorrowtm;
	time_t tomorrow = time(NULL) + 86400;	/* This is ok to use? */

	tomorrowtm = localtime(&tomorrow);
	return tomorrowtm->tm_wday;
}

int thishour()
{
	struct tm *btime;
	time_t now = time(NULL);

	btime = localtime(&now);
	return btime->tm_hour;
}

void splituh(char *u, char *h, char *uh)
{
	char *p;

	strcpy(u,uh);
	p = strchr(u,'@');
	if(p == NULL)
	{
		/* no '@'?  uh is not uh.  u = 0, uh must be h */
		u[0] = 0;
		strcpy(h,uh);
		return;
	}
	else
	{
		/* set u at '@' to 0.  it's left with user. */
		*p = 0;
		strcpy(h,strchr(uh,'@')+1);
		return;
	}
}
	
/* "user@a.b.host" --> "user@*.b.host" or "user@1.2.3.4" --> "user@1.2.3.*" */
void makehostmask(char *m_uh, char *uh)
{
	char *p,*q,xx[UHOSTLEN];

	strcpy(xx,uh);
	p = strchr(xx,'@');
	if(p != NULL)
	{
		q = strchr(p,'.');


		/* form xx@yy -> very bizarre */
		if(q == NULL)
		{
			sprintf(m_uh,"%s",xx);
			return;
		}

		/* form xx@yy.com -> don't truncate */
		if(strchr(q+1,'.') == NULL)
		{
			sprintf(m_uh,"%s",xx);
			return;
		}

		/* ip number -> xx@#.#.#.* */
		if((xx[strlen(xx)-1] >= '0') && (xx[strlen(xx)-1] <= '9'))
		{
			q = strrchr(p,'.');
			if(q != NULL)
				strcpy(q,".*");
			sprintf(m_uh,"%s",xx);
			return;
		}

		{ /* cph - Improved code to deal with 'dynamic hostnames' */

			/* Find last digit in the hostname */
			char *v = NULL, *w = p;
			while (*++w) if (isdigit(*w)) v = w;

			if (v) {
				v = strchr(v, '.');
				if (v && strchr(v+1, '.')) q = v;
			}
		}

		/* form xx@yy.zz.etc.edu or whatever -> xx@*.zz.etc.edu */
		if(q != NULL)
		{
			*(p+1) = '*';
			strcpy(p+2,q);
		}
		sprintf(m_uh,"%s",xx);
	}
	else
		strcpy(m_uh,"*@*");
}

void weekdaytoday_to_str(char *day_today)
{
	strcpy(day_today,week_days_long[today()]);
}


int pdf_uniform(int a, int b)
{
	char	usecs[7];
	struct timeval tv;
	struct timezone tz;
	long	random;
	int	i;

	/*
	 * seed rand.
	 * Strip lower 3rd and 4th digit (ASCII) from usecs component and
	 * multiply by entire usecs component to use as seed.
	 */
	gettimeofday(&tv,&tz);
	for(i=0; i<7; i++)
		usecs[i] = 0;

	sprintf(usecs,"%lu",(u_long)tv.tv_usec);
	srand(tv.tv_usec * (usecs[2] * 10 + usecs[3]));

	/* pick a number */
	random = (int)(rand());

	return (int)(random - ((random / (b - a + 1)) * (b - a + 1))) + a;
}

void stripfromend(char *s)
{
	char	*p;
	int	done = 0;

	p = s + strlen(s);
	while(p != s && !done)
	{
		if(!isalnum((int)*p))
			*p = 0;
		else
			done = 1;
		p--;
	}
}

void stripfromend_onlypunctuation(char *s)
{
	char	*p;
	int	done = 0;

	p = s + strlen(s);
	while(p != s && !done)
	{
		/* if the last character is one of these illegal characters */
		if(strchr("?!.~@$#%&*()=+\"':;,.<>/",*p) != NULL)
			*p = 0;
		else
			done = 1;
		p--;
	}
}

u_long year_to_days(int year)
{
	return year * 365L + (year / 4) - (year / 100) + (year / 400);
}

void uncalc_days(int *m, int *d, int *y, u_long date)
{
	int lp;

	*y = (int)(date / 365.2425);
	*d = (int)(date - year_to_days(*y));
	if(*d < 1)
		*d = (int)(date - year_to_days(*y-1));
	else
		(*y)++;

	lp = leap(*y);
	if(*d > days_in_months[lp][13])
	{
		*d -= days_in_months[lp][13];
		lp = leap(++(*y));
	}
	for(*m = 12; *m > 0; (*m)--)
	{
		if(*d > days_in_months[lp][*m])
		{
			*d -= days_in_months[lp][*m];
			break;
		}
	}
}

u_long calc_days(int m, int d, int year)
{
	int lp;

	lp = leap(year);
	if(year < 1)
		return 0L;
	if((m < 1) | (m > 12))
		return 0L;
	if((d < 1) | (d > month_length[lp][m]))
		return 0L;
	return year_to_days(--year) + days_in_months[lp][m] + d;
}

int day_of_week(int m, int d, int year)
{
	unsigned long  days;

	days = calc_days(m, d, year);
	if(days > 0L)
	{
		days--;
		days %= 7L;
		days++;
	}
	return (int)days;
}

u_long dates_difference(int m1, int d1, int y1, int m2, int d2, int y2)
{
    return calc_days(m2, d2, y2) - calc_days(m1, d1, y1);
}

int calc_new_date(int *mm, int *dd, int *year, u_long offset)
{
	u_long  days;
	int lp;

	lp = leap(*year);
	if(((days = calc_days(*mm,*dd,*year)) > 0L) && ((days += offset) > 0L))
	{
		*year = (int)(days / 365.2425);
		*dd = (int)(days - year_to_days(*year));
		if(*dd < 1)
			*dd = (int)(days - year_to_days(*year-1));
		else
			(*year)++;
		if(*dd > days_in_months[lp][13])
		{
			*dd -= days_in_months[lp][13];
			lp = leap(++(*year));
		}
		for(*mm = 12; *mm > 0; (*mm)--)
		{
			if(*dd > days_in_months[lp][*mm])
			{
				*dd -= days_in_months[lp][*mm];
				break;
			}
		}
		return 1;
	}
	return 0;
}

int week_number(int mm, int dd, int year)
{
	int first;

	first = day_of_week(1,1,year) - 1;
	return (int)((dates_difference(1,1,year, mm,dd,year) + first) / 7L ) + (first < 4);
}

/* NEW as of july 1 98 */
char *xmemcpy(char *dest, const char *src, size_t len)
{
	char *x = dest;

	while(len--)
	{
		*dest = *src;
		dest++;
		src++;
	}

	return x;
}

static int filecompare(const void *x,const void *y)
{
	register char **p, **q;

	p = (char **) x;
	q = (char **) y;
	return strcmp(*p,*q);
}

char **getfiles(const char *dir_name, int *number_entries)
{
	char **filelist, dirnamebuffer[256];
	char cwd[256];
	DIR	*current_directory;
	int	status;
	struct dirent *entry;
	unsigned int max_entries = 5096;

	*number_entries = 0;

	/*
	 * Use our own buffer for dir_name since getcwd reassigns it.
	 */
	strcpy(dirnamebuffer,dir_name);

	/* Save current working directory and change to specified dir.*/
	getcwd(cwd,256-1);
	status = chdir(dirnamebuffer);
	if(status != 0)
		return NULL;

	/* Get full pathname of new cwd and open for reading.*/
	getcwd(dirnamebuffer,256-1);
	current_directory = opendir(dirnamebuffer);
	if(current_directory == NULL)
	{
		/* Change back to start directory */
		chdir(cwd);

		return NULL;
	}


	/* Directory is legitimate.  Allocate filelist. */
	filelist = (char **)xmalloc(max_entries*sizeof(char *));
	if(filelist == NULL)
	{
		(void) closedir(current_directory);

		/* Change back to start directory */
		chdir(cwd);

		return NULL;
	}

	/* Read directory */
	entry = readdir(current_directory);
	while(entry != NULL)
	{
		if(*entry->d_name == '.')
		{
			entry = readdir(current_directory);
			continue;
		}

		if(*number_entries >= max_entries)
		{
			/* Extend the file list. */
			max_entries <<= 1;
			filelist = (char **)xrealloc((char **) filelist,max_entries*sizeof(char *));
			if(filelist == NULL)
			{
				(void) closedir(current_directory);

				printf("Max entries exceeded.\n");
				/* Change back to start directory */
				chdir(cwd);

				return NULL;
			}
		}
		filelist[*number_entries] = (char *)xmalloc(strlen(entry->d_name)+2);
		if(filelist[*number_entries] == NULL)
			break;
		(void) strcpy(filelist[*number_entries],entry->d_name);

		(*number_entries)++;

		entry = readdir(current_directory);
	}
	(void) closedir(current_directory);

	/* Change back to start directory */
	chdir(cwd);

	/* Sort filelist in ascending order. */
	qsort((void *) filelist,*number_entries,sizeof(char **), (int (*)(const void *, const void *)) filecompare);

	return(filelist);
}

