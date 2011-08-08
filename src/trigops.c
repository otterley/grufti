/* -------------------------------------------------------------------------
 * Steve Rider - stever@area23.org
 * Grufti - an interactive, service-oriented bot for IRC 
 *
 * trigops.c - complex trigger operations.  Used by trigcmd module and
 *             various other modules needing access to triggers.
 *
 * The trigger stack (in order of increasing dependency):
 *   trigcmd - command module for manipulaing triggers
 *   trigops - complex trigger operations
 *   triglist - direct operations on trigger list and record fields
 * ------------------------------------------------------------------------- */
/* 12 July 98 */

#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include "grufti.h"
#include "triglist.h"
#include "trigops.h"

