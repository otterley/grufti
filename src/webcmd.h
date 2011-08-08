/******************************************************************************
 * Steve Rider - stever@area23.org
 * Grufti - an interactive, service-oriented bot for IRC 
 *
 * webcmd.h - module-like web interface - the commands
 *
 *****************************************************************************/
/* 28 April 97 */

#ifndef WEBCMD_H
#define WEBCMD_H

#include <stdio.h>

void	openhtml(char *title);
void	closehtml();
void	websay_statusline();
void	websay_menuline(char *refresh);
void	websay_bar(char *str);
void	websay_nicksearchinput();
void	webcmd_intro();
void	webcmd_login();
void	webcmd_reply();
void	webcmd_note();
void	webcmd_putnote();
void	webcmd_putsavdel();
void	webcmd_rn();
void	webcmd_dn();
void	webcmd_read();
void	webcmd_putsearch();
void	webcmd_users();
void	webcmd_finger();
void	webcmd_channel();
void	webcmd_cmdline();
void	webcmd_setup();
void	webcmd_putcmdline();
void	webcmd_putsetup();
void	webcmd_response();
void	webcmd_putnewresponse();
void	webcmd_vresponse();
void	webcmd_dresponse();
void	webcmd_putresponse();
void	webcmd_webcatch();
void	webcmd_stat();
void	webcmd_showevents();
void	webcmd_showevent();
void	webcmd_putevent();
void	webcmd_unattendevent();

#endif /* WEBCMD_H */
