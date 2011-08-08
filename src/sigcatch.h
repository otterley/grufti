/******************************************************************************
 * Steve Rider - stever@area23.org
 * Grufti - an interactive, service-oriented bot for IRC 
 *
 * sigcatch.h - signal catching routines declarations
 *
 *****************************************************************************/
/* 28 April 97 */

#ifndef SIGCATCH_H
#define SIGCATCH_H
#include <signal.h>

void sigcatch_init();
void write_debug();
void fatal(char *s, int x);
void depart();
void writeallfiles(int force);
void got_bus(int z);
void got_segv(int z);
void got_fpe(int z);
void got_term(int z);
void got_quit(int z);
void got_hup(int z);
void got_alarm(int z);
void got_usr1(int z);
void got_usr2(int z);
void got_ill(int z);
void got_sigint(int z);

#endif /* SIGCATCH_H */
