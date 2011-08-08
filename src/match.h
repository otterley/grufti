/******************************************************************************
 * Steve Rider - stever@area23.org
 * Grufti - an interactive, service-oriented bot for IRC 
 *
 * match.h - wildcard matching routines
 *
 *****************************************************************************/
/* 28 April 97 */

#ifndef MATCH_H
#define MATCH_H

int wild_match_per(register u_char *m, register u_char *n);
int wild_match(register u_char *m, register u_char *n);
int wild_match_file(register u_char *m, register u_char *n);

#endif /* MATCH_H */
