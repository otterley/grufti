/******************************************************************************
 * Steve Rider - stever@area23.org
 * Grufti - an interactive, service-oriented bot for IRC 
 *
 * typedefs.h - all typedefs
 *
 *****************************************************************************/
/* 28 April 97 */

#ifndef TYPEDEF_H
#define TYPEDEF_H

/* Object Banlist */
typedef struct banlrec_node banlrec;

/* Object Channel */
typedef struct chanrec_node chanrec;
typedef struct membrec_node membrec;
typedef struct banrec_node banrec;


/* Object Codes */
typedef struct coderec_node coderec;
typedef struct coderec_node_fixed coderec_fixed;

/* Object Command */
typedef struct cmdrec_t cmdrec_t;

/* Object DCC */
typedef struct dccrec_node dccrec;

/* Object Event */
typedef struct evntrec_node evntrec;
typedef struct attnrec_node attnrec;

/* Object ID */
typedef struct idrec_node idrec;

/* Object Location */
typedef struct locarec_node locarec;

/* Object Log */
typedef struct logrec_node logrec;

/* Object Note */
typedef struct noterec_node noterec;
typedef struct nbody_node nbody;

/* Object Notify */
typedef struct radirec_node radirec;
typedef struct generec_node generec;

/* Object Response */
typedef struct rtype_node rtype;
typedef struct resprec_node resprec;
typedef struct rline_node rline;
typedef struct machrec_node machrec;
typedef struct combo_node combo;
typedef struct combcol_node combcol;
typedef struct combelm_node combelm;

/* Object Server */
typedef struct servrec_node servrec;
typedef struct squeue_node squeue;

/* Object User */
typedef struct userrec_node userrec;
typedef struct nickrec_node nickrec;
typedef struct acctrec_node acctrec;
typedef struct gretrec_node gretrec;

/* Object URL */
typedef struct urlrec_node urlrec;

#endif /* TYPEDEF_H */
