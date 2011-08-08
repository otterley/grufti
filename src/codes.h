/******************************************************************************
 * Steve Rider - stever@area23.org
 * Grufti - an interactive, service-oriented bot for IRC 
 *
 * codes.h - Server codes
 *
 *****************************************************************************/
/* 28 April 97 */

#ifndef CODES_H
#define CODES_H

#include <stdio.h>
#include "typedefs.h"
#include "grufti.h"

/* Let all files who need Codes have access to the Codes object */
extern struct Codes_object *Codes;


/*****************************************************************************/
/* object Codes declaration */
typedef struct Codes_object {
	coderec	*codelist;		/* -> list of server codes */
	coderec	*last;			/* -> last code record */

        int	num_codes;		/* number of code records */

} Codes_ob;

/* object Codes low-level function declarations. */
void	Codes_new();
u_long	Codes_sizeof();
void	Codes_freeall();

/* object Codes data-specific function declarations. */
void	Codes_display();

/* low-level Codes structure function declarations. */
void	Codes_appendcode(coderec *code);
void	Codes_deletecode(coderec *code);
void	Codes_freeallcodes();

/* object Codes high-level implementation-specific function declarations. */
coderec	*Codes_code(char *codename);
void	Codes_loadcodes();
coderec	*Codes_codebyindex(int item);
coderec	*Codes_codebyorder(int ordernum);
int	Codes_orderbycode();
void	Codes_clearorder();


/* ASDF */
/*****************************************************************************/
/* record coderec declaration */
struct coderec_node {
	coderec	*next;			/* -> next */

	char	*name;			/* name */
	void	(*function)();		/* function */
	u_long	accessed;		/* times accessed */
	int	order;			/* used for ordering lists */
} ;

/* record coderec low-level function declarations. */
coderec	*coderec_new();
void	coderec_freeall(coderec **list, coderec **last);
void	coderec_append(coderec **list, coderec **last, coderec *x);
void	coderec_delete(coderec **list, coderec **last, coderec *x);
void	coderec_movetofront(coderec **list, coderec **last, coderec *lastchecked, coderec *x);

/* record coderec low-level data-specific function declarations. */
void	coderec_init(coderec *code);
u_long	coderec_sizeof(coderec *code);
void	coderec_free(coderec *code);
void	coderec_setname(coderec *code, char *name);
void	coderec_setfunction(coderec *code, void (*function)());
void	coderec_display(coderec *code);
coderec	*coderec_code(coderec **list, coderec **last, char *name);


/*
 * The fixed code structure is located in codes.c.  It is loaded on startup
 * into the structure above.  We don't use the fixed-list for operations
 * because we want to provide efficient caching using the movetofront routines,
 * and we need a linked-list to do that.
 */

/*****************************************************************************/
/* record coderec_node_fixed declaration */
struct coderec_node_fixed {
	char	*name;			/* Name of server code */
	void	(*function)();		/* pointer to function */
} ;



#endif /* CODES_H */
