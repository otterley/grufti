/* -------------------------------------------------------------------------
 * Steve Rider - stever@area23.org
 * Grufti - an interactive, service-oriented bot for IRC 
 *
 * state.h - general system state tracking
 *
 * ------------------------------------------------------------------------- */
/* 9 July 98 */

#ifndef STATE_H
#define STATE_H

/* state types */
#define SP_I1           0x01
#define SP_I2           0x02
#define SP_I3           0x04
#define SP_I4           0x08
#define SP_C1           0x00
#define SP_C2           0x00
#define SP_C3           0x00
#define SP_C4           0x00

/* state owner */
#define SO_NET          1
#define SO_SUPER        2
#define SO_SERVER       3
#define SO_TRIGGER      4
#define SO_CHANNEL      5

/* States */
enum st_state {
	S_IRC = 0,
	S_ONCHAN,
	S_ONIRC,
	S_MEONCHAN,
	S_NUMONCHAN
} ;


/*
 * State node.  Only dynamic information needs to be stored here (except
 * for name, which just points to state table name), since we can lookup
 * a state's property in the state table.
 */
typedef struct state_t {
	enum st_state id;      /* numeric id (enum'd above) */
	char *name;            /* pointer to name buffer in state table */
	char set;              /* Whether this state is set or not */
	struct {
		char *str;
		int val;
	} p1, p2, p3;          /* parameters (could be int or char *) */
	
} state_t ;

#endif /* STATE_H */
