/******************************************************************************
 * Steve Rider - stever@area23.org
 * Grufti - an interactive, service-oriented bot for IRC 
 *
 * fhelp.h - format and read help files
 *
 *****************************************************************************/
/* 28 April 97 */

#ifndef FHELP_H
#define FHELP_H

u_long	fhelp_sizeof();
char	*read_word(FILE *ffile);
void	translate_escape(char *s, char c);
void	skipcomment(FILE *ffile);
int	find_topic(FILE *ffile, char *topic);
char	*get_ftext(FILE *ffile);

#endif /* FHELP_H */
