/******************************************************************************
 * Steve Rider - stever@area23.org
 * Grufti - an interactive, service-oriented bot for IRC 
 *
 * location.h - location records
 *
 *****************************************************************************/
/* 28 April 97 */

#ifndef LOCATION_H
#define LOCATION_H

#include <stdio.h>
#include "typedefs.h"
#include "grufti.h"

/* Let all files who need Location have access to the Location object */
extern struct Location_object *Location;


/*****************************************************************************/
/* object Location declaration */
typedef struct Location_object {
	locarec	*localist;		/* -> list of locations */
	locarec	*last;			/* -> last location record */

        int	num_locations;		/* number of location records */
	u_long	flags;			/* flags (ACTIVE | NEEDSWRITE) */

} Location_ob;

/* object Location low-level function declarations. */
void	Location_new();
u_long	Location_sizeof();
void	Location_freeall();

/* object Location data-specific function declarations. */
void	Location_display();

/* low-level Location structure function declarations. */
void	Location_appendlocation(locarec *loca);
void	Location_deletelocation(locarec *loca);
void	Location_freealllocations();

/* object Location high-level implementation-specific function declarations. */
locarec	*Location_location(int id_num);
void	Location_loadlocations();
void	Location_writelocations();
int	Location_newidnumber();
void	Location_idtostr(int id_num, char *l_info);
void	Location_ltostr(locarec *l, char *l_info);
void	Location_sort();
void	Location_sortby(int which);
locarec	*Location_addnewlocation(char *city, char *state, char *country, userrec *u, char *nick);
locarec	*Location_locationbyarea(char *city, char *state, char *country);
void	Location_locationsforarea(char *loc, char *str);
locarec	*Location_locationbytok(char *tok);
locarec	*Location_locationbyindex(int item);
void	Location_increment(userrec *u);
void	Location_decrement(userrec *u);


/* ASDF */
/*****************************************************************************/
/* record locarec declaration */
struct locarec_node {
	locarec	*next;			/* -> next */

	char	*city;			/* city */
	char	*state;			/* state or province */
	char	*country;		/* country */
	char	*addedby;		/* who added it */
	time_t	time_added;		/* when it was added */
	char	*description;		/* Short description of place */
	int	id;			/* id # */
	u_long	flags;			/* location flags */
	int	numusers;		/* how many users are in this loc */
} ;

/* record locarec low-level function declarations. */
locarec	*locarec_new();
void	locarec_freeall(locarec **list, locarec **last);
void	locarec_append(locarec **list, locarec **last, locarec *x);
void	locarec_delete(locarec **list, locarec **last, locarec *x);
void	locarec_movetofront(locarec **list, locarec **last, locarec *lastchecked, locarec *x);

/* record locarec low-level data-specific function declarations. */
void	locarec_init(locarec *loca);
u_long	locarec_sizeof(locarec *loca);
void	locarec_free(locarec *loca);
void	locarec_setcity(locarec *loca, char *city);
void	locarec_setstate(locarec *loca, char *state);
void	locarec_setcountry(locarec *loca, char *country);
void	locarec_setaddedby(locarec *loca, char *n);
void	locarec_setdescription(locarec *loca, char *description);
void	locarec_display(locarec *loca);
locarec	*locarec_location(locarec **list, locarec **last, int id_num);



/*****************************************************************************/
/* Location flags */
#define LOCATION_ACTIVE		0x0001	/* If the list is active */
#define LOCATION_NEEDS_WRITE	0x0002	/* so we don't write when unnecessary */

#endif /* LOCATION_H */
