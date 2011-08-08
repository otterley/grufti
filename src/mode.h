/******************************************************************************
 * Steve Rider - stever@area23.org
 * Grufti - an interactive, service-oriented bot for IRC 
 *
 * mode.h - perform mode operations
 *
 *****************************************************************************/
/* 28 April 97 */

#ifndef MODE_H
#define MODE_H

void	mode_gotmode();
void	mode_gotusermode();
void	mode_gotchanmode();
void	mode_parsemode(chanrec *chan, char *nick, char *uh, char *mode, char *arg, int server);
void	mode_changemode(chanrec *chan, int addmode, u_long type);
void	mode_changeumode(chanrec *chan, int addmode, u_long type, char *who);
void	mode_onban(chanrec *chan, char *arg, char *nick, char *uh);
void	mode_pushchannelmodes(chanrec *chan);
void	mode_setlimit(int addmode, chanrec *chan, char *nick, int server, int limit);
void	mode_setkey(int addmode, chanrec *chan, char *nick, int server, char *key);
void	mode_reversemode(int addmode, chanrec *chan, char *nick, char mode, int server);

#endif /* MODE_H */
