/******************************************************************************
 * Steve Rider - stever@area23.org
 * Grufti - an interactive, service-oriented bot for IRC 
 *
 * response.h - Response execution and structures
 *
 *****************************************************************************/
/* 28 April 97 */

#ifndef RESPONSE_H
#define RESPONSE_H

#include <stdio.h>
#include "typedefs.h"
#include "grufti.h"

/* Let all files who need Response have access to the Response object */
extern struct Response_object *Response;


/*****************************************************************************/
/* object Response declaration */
typedef struct Response_object {
	rtype	*rtypes;		/* -> list of response types */
	rtype	*last;			/* -> last response type */
        int	num_rtypes;		/* number of response types */
	combo	*combos;		/* -> list of combos */
	combo	*last_combo;		/* -> last combo */
	int	num_combos;
	time_t	time_sang_1015;		/* 10:15 on a saturday night :> */

	char	flags;
	char	sbuf[BUFSIZ];		/* buffer for displaying responses */

	/* Fast buffers for holding matchlines and elements */
	char	matches_buf[BUFSIZ];
	char	except_buf[BUFSIZ];
	char	elem_buf[BUFSIZ];
	char	phrase_buf[BUFSIZ];

} Response_ob;

/* object Response low-level function declarations. */
void	Response_new();
u_long	Response_sizeof();
void	Response_freeall();

/* low-level ID structure function declarations. */
void	Response_appendtype(rtype *rt);
void	Response_deletetype(rtype *rt);
void	Response_freealltypes();
void	Response_appendresponse(rtype *rt, resprec *r);
void	Response_deleteresponse(rtype *rt, resprec *r);
void	Response_freeallresponses(rtype *rt);
void	Response_appendline(resprec *r, rline *rl);
void	Response_deleteline(resprec *r, rline *rl);
void	Response_freealllines(resprec *r);
void	Response_appendrtypematch(rtype *rt, machrec *rm);
void	Response_deletertypematch(rtype *rt, machrec *rm);
void	Response_freeallrtypematches(rtype *rt);
void	Response_appendrtypeexcept(rtype *rt, machrec *rm);
void	Response_deletertypeexcept(rtype *rt, machrec *rm);
void	Response_freeallrtypeexcepts(rtype *rt);
void	Response_appendmatch(resprec *r, machrec *rm);
void	Response_deletematch(resprec *r, machrec *rm);
void	Response_freeallmatches(resprec *r);
void	Response_appendexcept(resprec *r, machrec *rm);
void	Response_deleteexcept(resprec *r, machrec *rm);
void	Response_freeallexcepts(resprec *r);
void	Response_appendcombo(combo *c);
void	Response_deletecombo(combo *c);
void	Response_freeallcombos();
void	Response_appendcombcol(combo *c, combcol *cc);
void	Response_deletecombcol(combo *c, combcol *cc);
void	Response_freeallcombcols(combo *c);

/* object Response high-level implementation-specific function declarations. */
int	Response_addnewtype(char *name, char *level, char *matches, char type);
void	Response_translate_char(char *trans, char c, char *nick, char *uh, char *chname);
void	Response_parse_reply(char *out, char *s, char *nick, char *uh, char *chname, char *a);
void	Response_buildcombo(char *out, char *combo_ident);
int	Response_numresponses();
int	Response_numlines_intype(rtype *rt);
void	Response_loadresponsefile();
void	Response_readresponsefile();
void	Response_readcombo();
int	Response_numcombos();
void	Response_addmatches(resprec *r, char *matches);
void	Response_addrtypematches(rtype *rt, char *matches);
void	Response_addexcept(resprec *r, char *except);
void	Response_addline(resprec *r, char *line);
void	Response_writeresponses();
rtype	*Response_type(char *name);
combo	*Response_combo(char *name);
resprec	*Response_response(rtype *rt, char *name);
resprec	*Response_response_checkall(char *name, rtype **rt, resprec **r);
resprec *Response_addnewresponse(rtype *rt, char *name);
rline	*Response_linebyindex(resprec *r, int num);
machrec	*Response_matchbyindex(resprec *r, int num);
machrec	*Response_exceptbyindex(resprec *r, int num);
int	Response_deletemarkedlines(resprec *r);
int	Response_deletemarkedmatches(resprec *r);
int	Response_deletemarkedexcepts(resprec *r);
void	Response_makertypelist(char *list);
void	Response_checkformatch(char *nick, char *uh, char *to, char *msg);
int	Response_matchtype(rtype *rt, char *msg);
int	Response_evaluatematch(machrec *mr, char *msg);
int	Response_matchresponse(resprec *r, char *msg);
int	Response_checkforbotnick(char *msg);
void	Response_doresponse(resprec *r, char *nick, char *uh, char *to);
void	Response_unset_all_lines(resprec *r);
char	*Response_enhanced_strstr(char *haystack, char *needle);
int	Response_checkforpublicflood(char *to);
int	Response_haveyouseen(userrec *u, char *lookup, acctrec **a, char *when);
void	Response_checkforstat(char *nick, char *to, char *lookup);
void	Response_checkforemail(char *nick, char *to, char *lookup);
void	Response_checkforurl(char *nick, char *to, char *lookup);
void	Response_checkforhaveyouseen(char *nick, char *to, char *lookup);
int	Response_orderbyresponse(rtype *rt);
void	Response_clearorder(rtype *rt);
resprec	*Response_responsebyorder(rtype *rt, int ordernum);
void	Response_checkresponses(char *nick, char *to, char *msg);
void	Response_checkforcron();
void	Response_addhistory(resprec *r, char *who, time_t t, char *msg);
void    Response_delhistory(resprec *r, int pos);


/* ASDF */
/*****************************************************************************/
/* record rtype declaration */
struct rtype_node {
	rtype	*next;			/* -> next */

	char	*name;			/* name of response type (lyrics... )*/
	char	type;			/* PUBLIC, PRIVATE, INTERNAL */
	u_long	level;			/* USER_RESPONSE, USER_LYRICS */
	machrec	*matches;
	machrec	*last_matches;
	int	num_matches;
	machrec	*except;
	machrec	*last_except;
	int	num_excepts;
	resprec	*responses;		/* -> list of responses for this type */
	resprec	*last_response;		/* -> last response record for type */
	int	num_responses;		/* number of responses */
} ;

/* record rtype low-level function declarations. */
rtype	*rtype_new();
void	rtype_freeall(rtype **list, rtype **last);
void	rtype_append(rtype **list, rtype **last, rtype *entry);
void	rtype_delete(rtype **list, rtype **last, rtype *x);

/* record rtype low-level data-specific function declarations. */
void	rtype_init(rtype *rt);
u_long	rtype_sizeof(rtype *rt);
void	rtype_free(rtype *rt);
void	rtype_setname(rtype *rt, char *name);
rtype	*rtype_rtype(rtype **list, rtype **last, char *name);



/*****************************************************************************/
/* record resprec declaration */
struct resprec_node {
	resprec	*next;			/* -> next */

	char	*name;			/* name */
	machrec	*matches;		/* matches */
	machrec	*last_matches;		/* matches */
	int	num_matches;
	machrec	*except;		/* "match except for" */
	machrec	*last_except;		/* "match except for" */
	int	num_excepts;
	rline	*lines;			/* -> list of response lines */
	rline	*last_line;		/* -> last line */
	int	num_lines;		/* num display lines */
	char	flags;			/* flags */
	int	order;			/* used for ordering lists */
	char	*nick;			/* nick of person who last asked */
	time_t	last;			/* last time response was asked */

	/* response history */
	struct listmgr_t *history;
} ;

struct resprec_history {
	char *info;
	char *who;
	time_t date;
} ;

/* record resprec low-level function declarations. */
resprec	*resprec_new();
void	resprec_freeall(resprec **list, resprec **last);
void	resprec_append(resprec **list, resprec **last, resprec *x);
void	resprec_delete(resprec **list, resprec **last, resprec *x);
void	resprec_movetofront(resprec **list, resprec **last, resprec *lastchecked, resprec *x);

/* record resprec low-level data-specific function declarations. */
void	resprec_init(resprec *r);
u_long	resprec_sizeof(resprec *r);
void	resprec_free(resprec *r);
void	resprec_setname(resprec *r, char *name);
void	resprec_setnick(resprec *r, char *nick);
resprec	*resprec_resp(resprec **list, resprec **last, char *match);



/*****************************************************************************/
/* record rline declaration */
struct rline_node {
	rline	*next;			/* -> next */

	char	*text;			/* response message */
	char	flags;			/* flags */
} ;

/* record rline low-level function declarations. */
rline	*rline_new();
void	rline_freeall(rline **list, rline **last);
void	rline_append(rline **list, rline **last, rline *entry);
void	rline_delete(rline **list, rline **last, rline *x);

/* record rline low-level data-specific function declarations. */
void	rline_init(rline *rl);
u_long	rline_sizeof(rline *rl);
void	rline_free(rline *rl);
void	rline_settext(rline *rl, char *text);



/*****************************************************************************/
/* record machrec declaration */
struct machrec_node {
	machrec	*next;			/* -> next */

	char	*match;			/* response matches */
	char	flags;			/* flags */
} ;

/* record machrec low-level function declarations. */
machrec	*machrec_new();
void	machrec_freeall(machrec **list, machrec **last);
void	machrec_append(machrec **list, machrec **last, machrec *entry);
void	machrec_delete(machrec **list, machrec **last, machrec *x);

/* record machrec low-level data-specific function declarations. */
void	machrec_init(machrec *m);
u_long	machrec_sizeof(machrec *m);
void	machrec_free(machrec *m);
void	machrec_setmatch(machrec *m, char *match);



/*****************************************************************************/
/* record combo declaration */
struct combo_node {
	combo	*next;			/* -> next */

	char	*name;			/* Name of combo (ie. insult) */
	combcol	*columns;
	combcol	*last;
	int	num_columns;
} ;

/* record combo low-level function declarations. */
combo	*combo_new();
void	combo_freeall(combo **list, combo **last);
void	combo_append(combo **list, combo **last, combo *entry);
void	combo_delete(combo **list, combo **last, combo *x);

/* record combo low-level data-specific function declarations. */
void	combo_init(combo *c);
u_long	combo_sizeof(combo *c);
void	combo_free(combo *c);
void	combo_setname(combo *c, char *name);



/*****************************************************************************/
/* record combcol declaration */
struct combcol_node {
	combcol	*next;			/* -> next */

	combelm	*elements;
	combelm	*last;
	int	num_elements;
} ;

/* record combcol low-level function declarations. */
combcol	*combcol_new();
void	combcol_freeall(combcol **list, combcol **last);
void	combcol_append(combcol **list, combcol **last, combcol *entry);
void	combcol_delete(combcol **list, combcol **last, combcol *x);

/* record combcol low-level data-specific function declarations. */
void	combcol_init(combcol *c);
u_long	combcol_sizeof(combcol *c);
void	combcol_free(combcol *c);


/*****************************************************************************/
/* record combelm declaration */
struct combelm_node {
	combelm	*next;			/* -> next */

	char	*elem;
} ;

/* record combelm low-level function declarations. */
combelm	*combelm_new();
void	combelm_freeall(combelm **list, combelm **last);
void	combelm_append(combelm **list, combelm **last, combelm *entry);
void	combelm_delete(combelm **list, combelm **last, combelm *x);

/* record combelm low-level data-specific function declarations. */
void	combelm_init(combelm *c);
u_long	combelm_sizeof(combelm *c);
void	combelm_free(combelm *c);
void	combelm_setelem(combelm *c, char *elem);



/*****************************************************************************/
/* Response flags */
#define RESPONSE_ACTIVE		0x01	/* marked active */
#define RESPONSE_NEEDS_WRITE	0x02	/* needs write */
#define COMBO_ACTIVE		0x04	/* combo list exists */

/*****************************************************************************/
/* rtype flags */
#define RTYPE_PUBLIC		0x01	/* public match */
#define RTYPE_PRIVATE		0x02	/* private match */
#define RTYPE_INTERNAL		0x04	/* internal match */
#define RTYPE_DELETEME		0x08	/* marked for delete */

/*****************************************************************************/
/* resprec flags */
#define RESP_DELETEME		0x01	/* marked for delete */

/*****************************************************************************/
/* rline flags */
#define RLINE_HIT		0x01	/* marked as hit */
#define RLINE_DELETEME		0x02	/* marked for delete */

/*****************************************************************************/
/* machrec flags */
#define MACHREC_DELETEME	0x01	/* marked for delete */

#endif /* RESPONSE_H */
