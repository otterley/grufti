/******************************************************************************
 * Steve Rider - stever@area23.org
 * Grufti - an interactive, service-oriented bot for IRC 
 *
 * gruftiaux.h - generic, miscellaneous routines specific to grufti and IRC
 *
 *****************************************************************************/
/* 28 April 97 */

#ifndef GRUFTIAUX_H
#define GRUFTIAUX_H

u_long	gruftiaux_sizeof();
void	mstrcpy(char **dest, char *src);
void	say(char *format, ...);
void	sayf(int lm, int rm, char *format, ...);
void	sayi(char *indent, char *format, ...);
void	sayframe(char *frame, char *format, ...);
void	sayframe_helper(char *frame, int len, char *ls, char *rs, char *p);
int	invalidpassword(char *password);
int	invalidnick(char *nick);
int	invalidacctname(char *acctname);
void	writeheader(FILE *f, char *fileinfo, char *filename);
void	fixfrom(char *from);
void	catfile(dccrec *d, char *file);
void	sendnotice(char *to, char *format, ...);
void	sendprivmsg(char *to, char *format, ...);
void	sendaction(char *to, char *format, ...);
void	dir_ls(char *path);
char	*my_strcasestr(char *haystack, char *needle);
void	verify_write(int x, char *srcfile, char *destfile);
time_t	timestr_to_secs(char *str);
void	killpercentchars(char *s);

#endif /* GRUFTIAUX_H */
