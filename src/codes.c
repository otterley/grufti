/******************************************************************************
 * Steve Rider - stever@area23.org
 * Grufti - an interactive, service-oriented bot for IRC 
 *
 * codes.c - Server codes.
 *
 *****************************************************************************/
/* 28 April 97 */

#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include "grufti.h"
#include "gruftiaux.h"
#include "codes.h"
#include "log.h"
#include "misc.h"

extern coderec_fixed code_tbl[];

/***************************************************************************n
 *
 * Codes object definitions.
 *
 ****************************************************************************/

void Codes_new()
{
	/* already in use? */
	if(Codes != NULL)
		Codes_freeall();

	/* allocate memory */
	Codes = (Codes_ob *)xmalloc(sizeof(Codes_ob));

	/* initialize */
	Codes->codelist = NULL;
	Codes->last = NULL;

	Codes->num_codes = 0;
}

u_long Codes_sizeof()
{
	u_long	tot = 0L;
	coderec	*c;

	tot += sizeof(Codes_ob);
	for(c=Codes->codelist; c!=NULL; c=c->next)
		tot += coderec_sizeof(c);

	return tot;
}

void Codes_freeall()
{
	/* Free all code records */
	Codes_freeallcodes();

	/* Free the object */
	xfree(Codes);
	Codes = NULL;
}

/****************************************************************************/
/* object Codes data-specific function definitions. */

void Codes_display()
{
	coderec	*code;
	int	count = 0;
	char	line[80];

	line[0] = 0;
	say("%d total codes.",Codes->num_codes);

	say("Code    Access   Code    Access   Code    Access   Code    Access");
	say("------- ------   ------- ------   ------- ------   ------- ------");
	for(code=Codes->codelist; code!=NULL; code=code->next)
	{
		sprintf(line,"%s%-7s %6lu   ",line,code->name,code->accessed);
		count++;
		if(count >= 4)
		{
			line[strlen(line) - 3] = 0;
			say("%s",line);
			count = 0;
			line[0] = 0;
		}
	}
	if(count)
		say("%s",line);
	say("------- ------   ------- ------   ------- ------   ------- ------");
}

/****************************************************************************/
/* low-level Codes structure function definitions. */

void Codes_appendcode(coderec *code)
{
	coderec_append(&Codes->codelist, &Codes->last, code);
	Codes->num_codes++;
}

void Codes_deletecode(coderec *code)
{
	coderec_delete(&Codes->codelist, &Codes->last, code);
	Codes->num_codes--;
}

void Codes_freeallcodes()
{
	coderec_freeall(&Codes->codelist, &Codes->last);
	Codes->num_codes = 0;
}

/****************************************************************************/
/* object Codes high-level implementation-specific function definitions. */

coderec *Codes_code(char *codename)
{
	return coderec_code(&Codes->codelist, &Codes->last, codename);
}


void Codes_loadcodes()
{
	int i;
	coderec *newcode;

	for(i=0; code_tbl[i].name != NULL; i++)
	{
		/* create a new node */
		newcode = coderec_new();

		/* assign data */
		coderec_setname(newcode,code_tbl[i].name);
		coderec_setfunction(newcode,code_tbl[i].function);

		/* Append it to code list */
		Codes_appendcode(newcode);
	}
}

coderec *Codes_codebyindex(int item)
{
	coderec	*c;
	int	i = 0;

	if(item > Codes->num_codes)
		return NULL;

	for(c=Codes->codelist; c!=NULL; c=c->next)
	{
		i++;
		if(i == item)
			return c;
	}

	return NULL;
}

coderec *Codes_codebyorder(int ordernum)
{
	coderec	*c;

	for(c=Codes->codelist; c!=NULL; c=c->next)
		if(c->order == ordernum)
			return c;

	return NULL;
}

int Codes_orderbycode()
{
	coderec	*c, *c_save = NULL;
	int	counter = 0, done = 0;
	char	h[256], highest[256];

	/* Put the highest ascii character in 'highest'. */
	sprintf(highest,"%c",HIGHESTASCII);

	/* We need to clear all ordering before we start */
	Codes_clearorder();

	/* Continue until all entries have an order number */
	while(!done)
	{
		done = 1;
		strcpy(h,highest);
		for(c=Codes->codelist; c!=NULL; c=c->next)
		{
			if(!c->order && strcasecmp(c->name,h) <= 0)
			{
				done = 0;
				strcpy(h,c->name);
				c_save = c;
			}
		}

		/* Not done means we got u_save and now we need to order it */
		if(!done)
		{
			counter++;
			c_save->order = counter;
		}
	}

	return counter;
}

void Codes_clearorder()
{
	coderec	*c;

	for(c=Codes->codelist; c!=NULL; c=c->next)
		c->order = 0;
}

/* ASDF */
/***************************************************************************n
 *
 * coderec record definitions.  No changes should be made to new(), freeall(),
 * append(), delete(), or movetofront().
 *
 ****************************************************************************/

coderec *coderec_new()
{
	coderec *x;

	/* allocate memory */
	x = (coderec *)xmalloc(sizeof(coderec));

	/* initialize */
	x->next = NULL;

	coderec_init(x);

	return x;
}

void coderec_freeall(coderec **list, coderec **last)
{
	coderec *entry = *list, *v;

	while(entry != NULL)
	{
		v = entry->next;
		coderec_free(entry);
		entry = v;
	}

	*list = NULL;
	*last = NULL;
}

void coderec_append(coderec **list, coderec **last, coderec *x)
{
	coderec *lastentry = *last;

	if(*list == NULL)
		*list = x;
	else
		lastentry->next = x;

	*last = x;
	x->next = NULL;
}

void coderec_delete(coderec **list, coderec **last, coderec *x)
{
	coderec *entry, *lastchecked = NULL;

	entry = *list;
	while(entry != NULL)
	{
		if(entry == x)
		{
			if(lastchecked == NULL)
			{
				*list = entry->next;
				if(entry == *last)
					*last = NULL;
			}
			else
			{
				lastchecked->next = entry->next;
				if(entry == *last)
					*last = lastchecked;
			}

			coderec_free(entry);
			return;
		}

		lastchecked = entry;
		entry = entry->next;
	}
}

void coderec_movetofront(coderec **list, coderec **last, coderec *lastchecked, coderec *x)
{
	if(lastchecked != NULL)
	{
		if(*last == x)
			*last = lastchecked;

		lastchecked->next = x->next;
		x->next = *list;
		*list = x;
	}
}

/****************************************************************************/
/* record coderec low-level data-specific function definitions. */

void coderec_init(coderec *code)
{
	/* initialize */
	code->name = NULL;
	code->function = null(void(*)());
	code->accessed = 0;
	code->order = 0;
}

u_long coderec_sizeof(coderec *code)
{
	u_long	tot = 0L;

	tot += sizeof(coderec);
	tot += code->name ? strlen(code->name)+1 : 0L;

	return tot;
}

void coderec_free(coderec *code)
{
	/* Free the elements. */
	xfree(code->name);

	/* Free this record. */
	xfree(code);
}

void coderec_setname(coderec *code, char *name)
{
	mstrcpy(&code->name,name);
}

void coderec_setfunction(coderec *code, void (*function)())
{
	code->function = function;
}

void coderec_display(coderec *code)
{
	say("%-9s %6lu",code->name,code->accessed);
}

coderec *coderec_code(coderec **list, coderec **last, char *name)
{
	coderec *code, *lastchecked = NULL;

	for(code=*list; code!=NULL; code=code->next)
	{
		if(isequal(code->name,name))
		{
			/* found it.  do a movetofront. */
			coderec_movetofront(list,last,lastchecked,code);

			return code;
		}
		lastchecked = code;
	}

	return NULL;
}
