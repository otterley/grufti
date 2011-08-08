/******************************************************************************
 * Steve Rider - stever@area23.org
 * Grufti - an interactive, service-oriented bot for IRC 
 *
 * response.c - Response execution and structures
 *
 *****************************************************************************/
/* 28 April 97 */

#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <ctype.h>
#include <fcntl.h>
#include "grufti.h"
#include "gruftiaux.h"
#include "server.h"
#include "response.h"
#include "log.h"
#include "notify.h"
#include "user.h"
#include "channel.h"
#include "listmgr.h"
#include "misc.h"

/***************************************************************************n
 *
 * Response object definitions.
 *
 ****************************************************************************/

static struct resprec_history *resprec_history_new()
{
	struct resprec_history *l;

	l = (struct resprec_history *)xmalloc(sizeof(struct resprec_history));

	l->info = NULL;
	l->date = 0L;
	l->who = NULL;

	return l;
}

static void resprec_history_free(struct resprec_history *l)
{
	xfree(l->info);
	xfree(l->who);

	xfree(l);
}

void Response_new()
{
	/* already in use? */
	if(Response != NULL)
		Response_freeall();

	/* allocate memory */
	Response = (Response_ob *)xmalloc(sizeof(Response_ob));

	/* initialize */
	Response->rtypes = NULL;
	Response->last = NULL;
	Response->num_rtypes = 0;
	Response->combos = NULL;
	Response->last_combo = NULL;
	Response->num_combos = 0;
	Response->time_sang_1015 = 0L;

	Response->flags = 0;
	Response->sbuf[0] = 0;
	Response->matches_buf[0] = 0;
	Response->except_buf[0] = 0;
	Response->elem_buf[0] = 0;
	Response->phrase_buf[0] = 0;
}

u_long Response_sizeof()
{
	u_long	tot = 0L;
	rtype	*rt;

	tot += sizeof(Response_ob);

	for(rt=Response->rtypes; rt!=NULL; rt=rt->next)
		tot += rtype_sizeof(rt);

	return tot;
}

void Response_freeall()
{
	/* Free all response type records */
	Response_freealltypes();

	/* Free the object */
	xfree(Response);
	Response = NULL;
}

/****************************************************************************/
/* low-level rtype structure function definitions. */

void Response_appendtype(rtype *rt)
{
	rtype_append(&Response->rtypes, &Response->last, rt);
	Response->num_rtypes++;
}

void Response_deletetype(rtype *rt)
{
	rtype_delete(&Response->rtypes, &Response->last, rt);
	Response->num_rtypes--;
}

void Response_freealltypes()
{
	rtype_freeall(&Response->rtypes, &Response->last);
	Response->num_rtypes = 0;
}

void Response_appendresponse(rtype *rt, resprec *r)
{
	resprec_append(&rt->responses, &rt->last_response, r);
	rt->num_responses++;
}

void Response_deleteresponse(rtype *rt, resprec *r)
{
	resprec_delete(&rt->responses, &rt->last_response, r);
	rt->num_responses--;
}

void Response_freeallresponses(rtype *rt)
{
	resprec_freeall(&rt->responses, &rt->last_response);
	rt->num_responses = 0;
}

void Response_appendline(resprec *r, rline *rl)
{
	rline_append(&r->lines, &r->last_line, rl);
	r->num_lines++;
}

void Response_deleteline(resprec *r, rline *rl)
{
	rline_delete(&r->lines, &r->last_line, rl);
	r->num_lines--;
}

void Response_freealllines(resprec *r)
{
	rline_freeall(&r->lines, &r->last_line);
	r->num_lines = 0;
}

void Response_appendrtypematch(rtype *rt, machrec *mr)
{
	machrec_append(&rt->matches, &rt->last_matches, mr);
	rt->num_matches++;
}

void Response_deletertypematch(rtype *rt, machrec *mr)
{
	machrec_delete(&rt->matches, &rt->last_matches, mr);
	rt->num_matches--;
}

void Response_freeallrtypematches(rtype *rt)
{
	machrec_freeall(&rt->matches, &rt->last_matches);
	rt->num_matches = 0;
}

void Response_appendrtypeexcept(rtype *rt, machrec *mr)
{
	machrec_append(&rt->except, &rt->last_except, mr);
	rt->num_excepts++;
}

void Response_deletertypeexcept(rtype *rt, machrec *mr)
{
	machrec_delete(&rt->except, &rt->last_except, mr);
	rt->num_excepts--;
}

void Response_freeallrtypeexcepts(rtype *rt)
{
	machrec_freeall(&rt->except, &rt->last_except);
	rt->num_excepts = 0;
}

void Response_appendmatch(resprec *r, machrec *mr)
{
	machrec_append(&r->matches, &r->last_matches, mr);
	r->num_matches++;
}

void Response_deletematch(resprec *r, machrec *mr)
{
	machrec_delete(&r->matches, &r->last_matches, mr);
	r->num_matches--;
}

void Response_freeallmatches(resprec *r)
{
	machrec_freeall(&r->matches, &r->last_matches);
	r->num_matches = 0;
}

void Response_appendexcept(resprec *r, machrec *mr)
{
	machrec_append(&r->except, &r->last_except, mr);
	r->num_excepts++;
}

void Response_deleteexcept(resprec *r, machrec *mr)
{
	machrec_delete(&r->except, &r->last_except, mr);
	r->num_excepts--;
}

void Response_freeallexcepts(resprec *r)
{
	machrec_freeall(&r->except, &r->last_except);
	r->num_excepts = 0;
}

void Response_appendcombo(combo *c)
{
	combo_append(&Response->combos, &Response->last_combo, c);
	Response->num_combos++;
}

void Response_deletecombo(combo *c)
{
	combo_delete(&Response->combos, &Response->last_combo, c);
	Response->num_combos--;
}

void Response_freeallcombos()
{
	combo_freeall(&Response->combos, &Response->last_combo);
	Response->num_combos = 0;
}

void Response_appendcombcol(combo *c, combcol *cc)
{
	combcol_append(&c->columns, &c->last, cc);
	c->num_columns++;
}

void Response_deletecombcol(combo *c, combcol *cc)
{
	combcol_delete(&c->columns, &c->last, cc);
	c->num_columns--;
}

void Response_freeallcombcols(combo *c)
{
	combcol_freeall(&c->columns, &c->last);
	c->num_columns = 0;
}

void Response_appendcombelm(combcol *cc, combelm *ce)
{
	combelm_append(&cc->elements, &cc->last, ce);
	cc->num_elements++;
}

void Response_deletecombelm(combcol *cc, combelm *ce)
{
	combelm_delete(&cc->elements, &cc->last, ce);
	cc->num_elements--;
}

void Response_freeallcombelms(combcol *cc)
{
	combelm_freeall(&cc->elements, &cc->last);
	cc->num_elements = 0;
}


/****************************************************************************/
/* object Response high-level implementation-specific function definitions. */

int Response_addnewtype(char *name, char *levels, char *matches, char type)
/*
 * Function: add new response type
 *  Returns: 0  if OK
 *           -1 if response name not quoted
 *           -2 if response matches not quoted
 */
{
	int	match_all = 0;
	u_long 	l = 0L;
	rtype	*rt;

	if(!rmquotes(name))
		return -1;

	if(isequal(matches,"*"))
		match_all = 1;
	else if(!rmquotes(matches))
		return -2;

	/* Check levels */
	User_str2flags(levels,&l);

	/* ok ready to add */
	rt = rtype_new();
	Response_appendtype(rt);

	/* fill in the data */
	rtype_setname(rt,name);
	if(match_all)
		Response_addrtypematches(rt,"*");
	else
		Response_addrtypematches(rt,matches);
	rt->type = type;
	rt->level = l;

	return 0;
}

void Response_translate_char(char *trans, char c, char *nick, char *uh, char *chname)
{
	switch(c)
	{
	   case 'h' : time_token(time(NULL),TIME_HOUR,trans); break;
	   case 'H' : time_token(time(NULL),TIME_HOUR_0,trans); break;
	   case 'N' : time_token(time(NULL),TIME_MIN_0,trans); break;
	   case 't' : time_token(time(NULL),TIME_SEC,trans); break;
	   case 'T' : time_token(time(NULL),TIME_SEC_0,trans); break;
	   case 'M' : date_token(time(NULL),DATE_MONTH,trans); break;
	   case 'D' : date_token(time(NULL),DATE_DAY_0,trans); break;
	   case 'd' : date_token(time(NULL),DATE_DAY,trans); break;
	   case 'y' : date_token(time(NULL),DATE_YEAR,trans); break;
	   case 'Y' : date_token(time(NULL),DATE_YEAR_19,trans); break;
	   case 'b' : strcpy(trans,"\002"); break;
	   case 'u' : strcpy(trans,"\031"); break;
	   case 'i' : strcpy(trans,"\022"); break;
	   case 'a' : strcpy(trans,uh); break;
	   case 'n' : strcpy(trans,nick); break;
	   case 'c' : strcpy(trans,chname); break;
	   case 'g' : strcpy(trans,Grufti->botnick); break;
	   case 'p' : sprintf(trans,"%d",Grufti->actual_telnet_port); break;
	   case 'e' : strcpy(trans,Grufti->bothostname); break;
	   case 'm' : strcpy(trans,Grufti->maintainer); break;
	   case 'w' : strcpy(trans,Grufti->homepage); break;
	   default  : strcpy(trans,""); break;
	}
}

void Response_parse_reply(char *out, char *s, char *nick, char *uh, char *chname, char *a)
{
	char	*p, trans[SERVERLEN], combo_ident[BUFSIZ], tmp[BUFSIZ];

	/* check for /m (action */
	if(s[0] == '/' && s[1] == 'm')
	{
		/* signify it was an action */
		*a = 1;

		/* bump pointer up by 2 ( / and m ) */
		s += 2;

		/* in case they did '/me' instead of '/m' */
		if(*s == 'e')
			s++;
	}
	else
		*a = 0;

	out[0] = 0;
	for(p=s; *p; p++)
	{
		if(*p == '\\')
		{
			p++;
			if(*p == 'r')
			{
				p++;
				strcpy(tmp,p);
				split(combo_ident,tmp);
				stripfromend_onlypunctuation(combo_ident);
				p += strlen(combo_ident);
				if(Response->flags & COMBO_ACTIVE)
					Response_buildcombo(trans,combo_ident);
				else
					strcpy(trans,"(COMBO_NOT_ACTIVE)");

			}
			else
				Response_translate_char(trans,*p,nick,uh,chname);
			strcat(out,trans);
		}
		else
			sprintf(out,"%s%c",out,*p);
	}
}

void Response_buildcombo(char *out, char *combo_ident)
{
	combo	*c;
	combcol	*cc;
	combelm	*ce;
	int	i, word;

	out[0] = 0;
	if(!combo_ident[0])
		return;

	c = Response_combo(combo_ident);
	if(c == NULL)
	{
		Log_write(LOG_ERROR,"*","Didn't find combo %s. ",combo_ident);
		return;
	}

	/* Now loop through each column and pick a phrase at random */
	for(cc=c->columns; cc!=NULL; cc=cc->next)
	{
		word = pdf_uniform(1,cc->num_elements);
		i = 0;
		for(ce=cc->elements; ce!=NULL; ce=ce->next)
		{
			i++;
			if(i == word)
				sprintf(out,"%s%s ",out,ce->elem);
		}
	}

	if(out[0])
		out[strlen(out)-1] = 0;
}
	

int Response_numresponses()
{
	rtype	*rt;
	resprec	*r;
	int	tot = 0;

	for(rt=Response->rtypes; rt!=NULL; rt=rt->next)
		for(r=rt->responses; r!=NULL; r=r->next)
			tot += r->num_lines;

	return tot;
}

int Response_numlines_intype(rtype *rt)
{
	resprec	*r;
	int	tot = 0;

	for(r=rt->responses; r!=NULL; r=r->next)
		tot += r->num_lines;

	return tot;
}

void Response_loadresponsefile()
{
	Response_readresponsefile();
	if(!(Response->flags & RESPONSE_ACTIVE))
	{
		Log_write(LOG_ERROR,"*","Response file not loaded due to errors.");
		Response->flags |= RESPONSE_ACTIVE;
		return;	
	}

	Response->flags &= ~RESPONSE_NEEDS_WRITE;
}

void Response_readresponsefile()
/*
 * %lyrics Coil
 * %match coil
 * %except this mortal coil
 * "...if you want to touch the sky just put a window in your eye..."
 * "...gold is the sky, in concentrate...power in it's purest state..."
 * "...see his hands....look at the fingers..."
 * "...oh rose, tho art sick...seduce...let loose, the vision and the void..."
 * "...sweet tortures fly on mystery wings...pure evil is when flowers sing..."
 * "...And the rapids of my heart will tear your ship of love apart..."
 * "...There's honey in the hollows and contours of the body..."
 * %end
 * 
 * %lyrics Everything But The Girl
 * %match everything but the girl, ebtg
 * "...You wondered why no one called, between you and me we scared them all..."
 * %end
 * 
 * %lyrics Skinny Puppy
 * %match skinny puppy, sp, s.p.
 * "...heavens thrash fixation turning...mass direction..."
 * "...mental shock...disturbed thoughts rotting..."
 * "...distorted moodswing smile...altered music shadow..."
 * "...Smothered hope fly from sorrow for a new divine tomorrow..."
 * "...Wasted views that's all they see blue, hot blood guilt optic nerve..."
 * "...wasted truth why call at all blue, hot lines eventual decline..."
 * "...with the right attitude, you will succeed blue..."
 * %end
 */
{
	resprec	*resp = NULL;
	struct resprec_history *l;
	rtype	*rt_check, *rt = NULL;
	char	s[BUFSIZ], ident[BUFSIZ], name[BUFSIZ], *p, respfile[256];
	char	elem[BUFSIZ];
	FILE	*f;
	int	inblock = 0, line = 0;

	/* open file for reading */
	sprintf(respfile,"%s/%s",Grufti->botdir,Grufti->respfile);
	f = fopen(respfile,"r");
	if(f == NULL)
	{
		Log_write(LOG_ERROR,"*","Error: Response file \"%s\" not found.",respfile);
		return;
	}

	/* Read each line */
	while(readline(f,s))
	{
		line++;

		rmspace(s);

		/* skip blank lines */
		if(!s[0])
			continue;

		/* skip comments */
		if(s[0] == '#')
			continue;

		/* Extract identifier code */
		if(inblock)
		{
			if(s[0] == '%')
			{
				p = s + 1;
				split(ident,p);
				if(isequal(ident,"match"))
					Response_addmatches(resp,p);
				else if(isequal(ident,"except"))
					Response_addexcept(resp,p);
				else if(isequal(ident,"last"))
					resp->last = atol(p);
				else if(isequal(ident,"history"))
				{
					l = resprec_history_new();
					split(elem, p);
					mem_strcpy(&l->who, elem);
					split(elem, p);
					l->date = (time_t)atol(elem);
					mem_strcpy(&l->info, s);

					listmgr_append(resp->history, l);
				}
				else if(isequal(ident,"end"))
				{
					Response_appendresponse(rt,resp);
					inblock = 0;
				}
				else
				{
					/* see if they forgot an END */
					rt_check = Response_type(ident);
					if(rt_check != NULL)
					{
						Log_write(LOG_ERROR,"*","[%d] Response structure \"%s\" still open (no %%end)",line,resp->name);
					}
					else
						Log_write(LOG_ERROR,"*","[%d] Invalid Response identifier \"%s\".",line,ident);
				}
			}
			else
			{
				Response_addline(resp,s);
			}
		}
		else
		{
			if(s[0] == '%')
			{
				p = s + 1;
				split(ident,p);
				/* Search for valid type */
				rt = Response_type(ident);
				if(rt == NULL)
					Log_write(LOG_ERROR,"*","[%d] Response type \"%s\" is invalid.",line,ident);
				else if(!p[0])
					Log_write(LOG_ERROR,"*","[%d] Response type \"%s\" has no name.",line,ident);
				else
				{
					inblock = 1;
					resp = resprec_new();
					resp->history = listmgr_new_list(LM_NOMOVETOFRONT);
					listmgr_set_free_func(resp->history, resprec_history_free);
					split(name,p);
					resprec_setname(resp,name);
				}
			}
			else
			{
				context;
				Log_write(LOG_ERROR,"*","[%d] Response line does not belong to any structure. (%s)",line,s);
				context;
			}
		}
	}

	if(inblock)
	{
		Log_write(LOG_ERROR,"*","[%d] No end statement after last response structure \"%s\"",line,resp->name);
		resprec_free(resp);
	}

	Response->flags |= RESPONSE_ACTIVE;
}
	
void Response_addhistory(resprec *r, char *who, time_t t, char *msg)
{
	struct resprec_history *l;

	l = resprec_history_new();

	mem_strcpy(&l->who, who);
	l->date = t;
	mem_strcpy(&l->info, msg);

	listmgr_append(r->history, l);
}

void Response_delhistory(resprec *r, int pos)
{
	struct resprec_history *l;
	int i;

	for(l = listmgr_begin(r->history), i = 0; l; l = listmgr_next(r->history), i++)
	{
		if(pos == i)
		{
			listmgr_delete(r->history, l);
			return;
		}
	}
}

int Response_numcombos()
{
	combo	*c;
	combcol	*cc;
	int	tot = 0;

	for(c=Response->combos; c!=NULL; c=c->next)
		for(cc=c->columns; cc!=NULL; cc=cc->next)
			tot += cc->num_elements;

	return tot;
}

void Response_readcombo()
{
	char	s[BUFSIZ], ident[BUFSIZ], name[BUFSIZ], e[BUFSIZ], *p;
	char	combofile[256];
	combo	*c = NULL;
	combcol	*cc = NULL;
	combelm	*ce = NULL;
	FILE	*f;
	int	inblock = 0, line = 0;

	/* open file for reading */
	sprintf(combofile,"%s/%s",Grufti->botdir,Grufti->combofile);
	f = fopen(combofile,"r");
	if(f == NULL)
	{
		Log_write(LOG_ERROR,"*","Combo file \"%s\" not found.",Grufti->combofile);
		return;
	}

	/* Read each line */
	while(readline(f,s))
	{
		line++;

		/* skip blank lines */
		rmspace(s);
		if(!s[0])
			continue;

		/* skip comments */
		if(s[0] == '#')
			continue;

		/* Extract identifier code */
		if(inblock)
		{
			if(s[0] == '%')
			{
				p = s + 1;
				split(ident,p);
				if(isequal(ident,"column"))
				{
					/* Create a new column for this combo */
					cc = combcol_new();
					Response_appendcombcol(c,cc);
				}
				else if(isequal(ident,"end"))
				{
					Response_appendcombo(c);
					inblock = 0;
				}
				else
				{
					Log_write(LOG_ERROR,"*","[%d] Combo structure \"%s\" still open (no %%end)",line,c->name);
				}
			}
			else
			{
				splitc(e,s,',');
				rmspace(e);
				while(e[0])
				{
					ce = combelm_new();
					Response_appendcombelm(cc,ce);
					combelm_setelem(ce,e);
					splitc(e,s,',');
					rmspace(e);
				}
			}
		}
		else
		{
			if(s[0] == '%')
			{
				p = s + 1;
				split(ident,p);
				if(!p[0])
					Log_write(LOG_ERROR,"*","[%d] Combo type has no name.",line);
				else
				{
					inblock = 1;
					c = combo_new();
					split(name,p);
					combo_setname(c,name);
				}
			}
			else
			{
				context;
				Log_write(LOG_ERROR,"*","[%d] Combo line does not belong to any structure. (%s)",line,s);
				context;
			}
		}
	}

	if(inblock)
	{
		Log_write(LOG_ERROR,"*","[%d] No end statement after last combo structure \"%s\"",line,c->name);
		combo_free(c);
	}

	fclose(f);

	Response->flags |= COMBO_ACTIVE;
}

void Response_addmatches(resprec *r, char *match)
{
	machrec	*mr;

	/* allocate memory for new node */
	mr = machrec_new();

	/* append to Response */
	Response_appendmatch(r,mr);

	/* Set data */
	machrec_setmatch(mr,match);
}

void Response_addrtypematches(rtype *rt, char *match)
{
	machrec	*mr;

	/* allocate memory for new node */
	mr = machrec_new();

	/* append to Response */
	Response_appendrtypematch(rt,mr);

	/* Set data */
	machrec_setmatch(mr,match);
}

void Response_addexcept(resprec *r, char *except)
{
	machrec	*mr;

	/* allocate memory for new node */
	mr = machrec_new();

	/* append to Response */
	Response_appendexcept(r,mr);

	/* Set data */
	machrec_setmatch(mr,except);
}

void Response_addline(resprec *r, char *line)
{
	rline	*rl;

	/* allocate memory for new node */
	rl = rline_new();

	/* append to Response */
	Response_appendline(r,rl);

	/* Set data */
	rline_settext(rl,line);
}

void Response_writeresponses()
{
	char	tmpfile[256], respfile[256];
	FILE	*f;
	resprec	*r;
	rtype	*rt;
	rline	*rl;
	machrec	*mr;
	struct resprec_history *l;
	int	numlines, x;

	if(!(Response->flags & RESPONSE_ACTIVE))
	{
		Log_write(LOG_MAIN,"*","Response is not active.  Ignoring write request.");
		return;
	}

	/* Open tmpfile for writing */
	sprintf(respfile,"%s/%s",Grufti->botdir,Grufti->respfile);
	sprintf(tmpfile,"%s.tmp",respfile);
	f = fopen(tmpfile,"w");
	if(f == NULL)
	{
		Log_write(LOG_MAIN|LOG_ERROR,"*","Couldn't write tmp responsefile!");
		return;
	}

	/* Write header */
	writeheader(f,"Response->responses - Response matches and responses",respfile);

	/* write summaries for each type */
	if(fprintf(f,"# Summary of responses\n") == EOF)
		return;
	for(rt=Response->rtypes; rt!=NULL; rt=rt->next)
	{
		numlines = Response_numlines_intype(rt);
		if(fprintf(f,"#   %s (%d record%s, %d response%s)\n",rt->name,rt->num_responses,rt->num_responses==1?"":"s",numlines,numlines==1?"":"s") == EOF)
	
			return;
	}
	if(fprintf(f,"\n") == EOF)
		return;

	/* Write each record in each type */
	for(rt=Response->rtypes; rt!=NULL; rt=rt->next)
	{
		if(fprintf(f,"\n\n") == EOF)
			return;
		if(fprintf(f,"################################################################################\n") == EOF)
			return;
		numlines = Response_numlines_intype(rt);
		if(fprintf(f,"# %s (%d record%s, %d response%s)\n",rt->name,rt->num_responses,rt->num_responses==1?"":"s",numlines,numlines==1?"":"s") == EOF)
			return;
		if(fprintf(f,"################################################################################\n") == EOF)
			return;
		for(r=rt->responses; r!=NULL; r=r->next)
		{
			/* Write response type and name */
			if(fprintf(f,"%%%s %s\n",rt->name,r->name) == EOF)
				return;

			/* Write response matches */
			for(mr=r->matches; mr!=NULL; mr=mr->next)
				if(fprintf(f,"%%match %s\n",mr->match) == EOF)
					return;

			/* Write response except */
			for(mr=r->except; mr!=NULL; mr=mr->next)
				if(fprintf(f,"%%except %s\n",mr->match) == EOF)
					return;

			/* Write response lines */
			for(rl=r->lines; rl!=NULL; rl=rl->next)
				if(fprintf(f,"%s\n",rl->text) == EOF)
					return;

			/* Write response time */
			if(fprintf(f,"%%last %lu\n",r->last) == EOF)
				return;

			/* Write log */
			if(r->history != NULL)
			{
				for(l = listmgr_begin(r->history); l; l = listmgr_next(r->history))
				{
					if(fprintf(f,"%%history %s %lu %s\n",l->who, l->date, l->info) == EOF)
						return;
				}
			}

			if(fprintf(f,"%%end\n\n") == EOF)
				return;
		}
	}
	fclose(f);

	/* Move tmpfile over to main response */
	if(Grufti->keep_tmpfiles)
		x = copyfile(tmpfile,respfile);
	else
		x = movefile(tmpfile,respfile);

	verify_write(x,tmpfile,respfile);
	if(x == 0)
	{
		Log_write(LOG_MAIN,"*","Wrote response file.");
		Response->flags &= ~RESPONSE_NEEDS_WRITE;
	}
}

rtype *Response_type(char *name)
{
	rtype	*rt;

	for(rt=Response->rtypes; rt!=NULL; rt=rt->next)
		if(isequal(rt->name,name))
			return rt;

	return NULL;
}

combo *Response_combo(char *name)
{
	combo	*c;

	for(c=Response->combos; c!=NULL; c=c->next)
		if(isequal(c->name,name))
			return c;

	return NULL;
}

resprec	*Response_response(rtype *rt, char *name)
{
	resprec	*r;

	for(r=rt->responses; r!=NULL; r=r->next)
		if(isequal(r->name,name))
			return r;

	return NULL;
}

resprec	*Response_response_checkall(char *name, rtype **_rt, resprec **_r)
{
	rtype	*rt;
	resprec	*r;

	for(rt=Response->rtypes; rt!=NULL; rt=rt->next)
		for(r=rt->responses; r!=NULL; r=r->next)
			if(isequal(r->name,name))
			{
				*_rt = rt;
				*_r = r;
				return r;
			}

	return NULL;
}

resprec *Response_addnewresponse(rtype *rt, char *name)
{
	resprec	*r;

	/* Allocate memory for new node */
	r = resprec_new();

	/* Append to the list */
	Response_appendresponse(rt,r);

	/* Set name */
	resprec_setname(r,name);

	r->history = listmgr_new_list(LM_NOMOVETOFRONT);
	listmgr_set_free_func(r->history, resprec_history_free);

	return r;
}

rline *Response_linebyindex(resprec *r, int num)
{
	int	i = 0;
	rline	*rl;

	if(num <= 0 || num > r->num_lines)
		return NULL;

	for(rl=r->lines; rl!=NULL; rl=rl->next)
	{
		i++;
		if(i == num)
			return rl;
	}

	return NULL;
}

machrec *Response_matchbyindex(resprec *r, int num)
{
	int	i = 0;
	machrec	*mr;

	if(num <= 0 || num > r->num_matches)
		return NULL;

	for(mr=r->matches; mr!=NULL; mr=mr->next)
	{
		i++;
		if(i == num)
			return mr;
	}

	return NULL;
}

machrec *Response_exceptbyindex(resprec *r, int num)
{
	int	i = 0;
	machrec	*mr;

	if(num <= 0 || num > r->num_excepts)
		return NULL;

	for(mr=r->except; mr!=NULL; mr=mr->next)
	{
		i++;
		if(i == num)
			return mr;
	}

	return NULL;
}

int Response_deletemarkedlines(resprec *r)
{
	rline	*rl, *rl_next;
	int	tot = 0;

	rl = r->lines;
	while(rl != NULL)
	{
		rl_next = rl->next;
		if(rl->flags & RLINE_DELETEME)
		{
			Response_deleteline(r,rl);
			tot++;
		}
		rl = rl_next;
	}

	return tot;
}

int Response_deletemarkedmatches(resprec *r)
{
	machrec	*mr, *mr_next;
	int	tot = 0;

	mr = r->matches;
	while(mr != NULL)
	{
		mr_next = mr->next;
		if(mr->flags & MACHREC_DELETEME)
		{
			Response_deletematch(r,mr);
			tot++;
		}
		mr = mr_next;
	}

	return tot;
}

int Response_deletemarkedexcepts(resprec *r)
{
	machrec	*mr, *mr_next;
	int	tot = 0;

	mr = r->except;
	while(mr != NULL)
	{
		mr_next = mr->next;
		if(mr->flags & MACHREC_DELETEME)
		{
			Response_deleteexcept(r,mr);
			tot++;
		}
		mr = mr_next;
	}

	return tot;
}

void Response_makertypelist(char *list)
{
	rtype	*rt;

	list[0] = 0;
	for(rt=Response->rtypes; rt!=NULL; rt=rt->next)
		sprintf(list,"%s%s, ",list,rt->name);

	if(strlen(list) >= 2)
		list[strlen(list) - 2] = 0;
}

void Response_checkformatch(char *nick, char *uh, char *to, char *msg)
/*
 * This routine must be very aware of resource usage, since it is called
 * on every public message we see.  This is why time has been invested in
 * configuring response types so that we can skip large chunks of responses
 * immediately if they don't match an initial type (ie lyrics matching "sing").
 */
{
	rtype	*rt;
	resprec	*r, *lastchecked = NULL;

	/* Check types.  Evalute the first one that matches.  (order counts) */
	for(rt=Response->rtypes; rt!=NULL; rt=rt->next)
	{
		if(Response_matchtype(rt,msg))
		{
			/*
			 * We have a response type which matches. Each response
			 * in that type must be checked for a match.  If no
			 * responses match, we return.  This means that if a
			 * fundamental type matches, like 'lyrics', yet no
			 * individual lyrics responses match, we don't attempt
			 * to see if anything else matches.
			 */
			for(r=rt->responses; r!=NULL; r=r->next)
			{
				if(Response_matchresponse(r,msg))
				{
					/*
					 * We have a response which matches.
					 * Execute it.
					 */
/*
					if(r->nick && isequal(r->nick,nick))
					{
						Response_response_checkall("internal_alreadysaidthat",&dummy,&rinternal);
						if(rinternal)
							Response_doresponse(rinternal,nick,uh,to);
						resprec_setnick(r,"");
					}
					else
					{
*/
						Response_doresponse(r,nick,uh,to);
/*
						resprec_setnick(r,nick);
					}
*/

					r->last = time(NULL);
					Response->flags |= RESPONSE_NEEDS_WRITE;

					/* move it up front */
					resprec_movetofront(&rt->responses,&rt->last_response,lastchecked,r);
					return;
				}
				lastchecked = r;
			}
			return;
		}
	}

	/* that's it! */
}

int Response_matchtype(rtype *rt, char *msg)
{
	int	result;

	/* 1st check PRIVATE */
	if(rt->type & RTYPE_PRIVATE)
	{
		/* No botnick, then it doesn't evaluate */
		if(!Response_checkforbotnick(msg))
			return 0;

		/* Have botnick, we can continue. */
	}

	/* Evaluate matches */
	result = Response_evaluatematch(rt->matches,msg);

	/* If match is FALSE, type doesn't match.  return FALSE */
	if(!result)
		return 0;

	/* Result is true.  Evaluate 'except' line */
	result = Response_evaluatematch(rt->except,msg);

	/* If except is TRUE, then type doesn't match.  return FALSE */
	if(result)
		return 0;

	/* matches was true, and except was FALSE, type matches. */
	return 1;
}

int Response_evaluatematch(machrec *mr, char *msg)
{
	int	result;
	machrec	*m;

	/* no matchlines definitely does not match */
	if(mr == NULL)
		return 0;

	for(m=mr; m!=NULL; m=m->next)
	{
		/* Wildcard matches anything */
		if(isequal(m->match,"*"))
			return 1;

		result = 0;

		/* Check each element of the match line */
		strcpy(Response->matches_buf,m->match);
		splitc(Response->elem_buf,Response->matches_buf,',');
		rmspace(Response->elem_buf);
		while(Response->elem_buf[0] && !result)
		{
			if(Response_enhanced_strstr(msg,Response->elem_buf))
				result = 1;
			else
			{
				splitc(Response->elem_buf,Response->matches_buf,',');
				rmspace(Response->elem_buf);
			}
		}

		/* If no results from OR, that match is false */
		if(!result)
			return 0;
	}

	/* If we're still here, it means each matchline got a TRUE somewhere */
	return 1;
}

int Response_matchresponse(resprec *r, char *msg)
{
	int	result;

	/* Evaulate 'matches' line */
	result = Response_evaluatematch(r->matches,msg);

	/* if result is FALSE, then we can return FALSE now */
	if(!result)
		return 0;

	/* Result is true.  Evaluate 'except' line */
	result = Response_evaluatematch(r->except,msg);

	/* If except is TRUE, then response doesn't match.  return FALSE */
	if(result)
		return 0;

	/* matches was true, and except was FALSE, response matches. */
	return 1;
}

int Response_checkforbotnick(char *msg)
{
	/* first check for botnick */
	if(Response_enhanced_strstr(msg,Grufti->botnick) || Response_enhanced_strstr(msg,Server->currentnick))
		return 1;

	/* Now check the alternate names */
	if(Grufti->alt_name1[0])
		if(Response_enhanced_strstr(msg,Grufti->alt_name1))
			return 1;

	if(Grufti->alt_name2[0])
		if(Response_enhanced_strstr(msg,Grufti->alt_name2))
			return 1;

	/* Nothing?  doesn't evaluate then */
	return 0;
}

void Response_doresponse(resprec *r, char *nick, char *uh, char *to)
{
	int	num_unhit, line_num, i = 0;
	char	action;
	rline	*rl;

	/* No response lines?  well then get the hell out of here! */
	if(r->lines == NULL)
		return;

	/* 1st, pick a response line */
	num_unhit = Response_num_unhit_lines(r);
	if(num_unhit == 0)
	{
		Response_unset_all_lines(r);
		num_unhit = Response_num_unhit_lines(r);
	}

	if(num_unhit == 1)
		line_num = 1;
	else
		line_num = pdf_uniform(1,num_unhit);

	for(rl=r->lines; rl!=NULL; rl=rl->next)
	{
		if(!(rl->flags & RLINE_HIT))
		{
			i++;
			if(i == line_num)
			{
				rl->flags |= RLINE_HIT;
				break;
			}
		}
	}

	if(rl == NULL)
		return;

	/* Ok we got one.  Now say it. */
	Response_parse_reply(Response->sbuf,rl->text,nick,uh,to,&action);
	rmspace(Response->sbuf);

	/*
	 * before saying, check if we're in a public flood.  checkforpublicflood
	 * returns TRUE if we've got permission to talk to channel.  FALSE
	 * if we're in a flood.  in that case, /msg it.
	 */
	if(Response_checkforpublicflood(to))
	{
		/* ok to send to channel */
		if(action)
			sendaction(to, "%s", Response->sbuf);
		else
			sendprivmsg(to, "%s", Response->sbuf);
	}
	else
	{
		Log_write(LOG_PUBLIC,to,"(Sending response to %s)",nick);
		/* not ok to send to channel.  send to nick instead */
		if(action)
			sendaction(nick, "%s", Response->sbuf);
		else
			sendprivmsg(nick, "%s", Response->sbuf);
	}
}
	
int Response_num_unhit_lines(resprec *r)
{
	rline	*rl;
	int	tot = 0;

	for(rl=r->lines; rl!=NULL; rl=rl->next)
		if(!(rl->flags & RLINE_HIT))
			tot++;

	return tot;
}

void Response_unset_all_lines(resprec *r)
{
	rline	*rl;

	for(rl=r->lines; rl!=NULL; rl=rl->next)
		rl->flags &= ~RLINE_HIT;
}

char *Response_enhanced_strstr(char *haystack, char *needle)
{
	char	*p, *q;

	/* First we check regular strstr.  If no match, then we're done */
	p = my_strcasestr(haystack,needle);
	if(p == NULL)
		return NULL;

	/* Next we look at the character just before the word */
	if(p != haystack)
	{
		/* If the character just before is alphanumeric, no match */
		if(isalnum((int)*(p-1)))
			return NULL;
	}

	/* Next, look at the character just after the word */
	q = p + strlen(needle);
	if(*q)
	{
		/* If the character just after is alphanumeric, no match */
		if(isalnum((int)*q))
			return NULL;
	}

	/* passed all our tests?  cool! */
	return p;
}
	
int Response_checkforpublicflood(char *to)
/*
 * 3 things happen:  if 'to' is not a channelname, then we return FALSE,
 * which means the caller should send the response to the nick.  Exactly
 * what we want if 'to' is the bot for example.
 *
 * If 'to' is a channel, we return FALSE if we're currently in a public
 * flood.  If not, we mark the time in one of the boxes, and return TRUE.
 *
 * If someone has asked us to HUSH, then we'll return FALSE regardless.
 *
 * Finally, we check and see if this next message will constitute in a flood,
 * if so, we turn PUBLICFLOOD on for this channel.
 */
{
	chanrec	*chan;
	time_t	now = time(NULL);
	int	i;

	if(to[0] == '#' || to[0] == '&' || to[0] == '+')
	{
		chan = Channel_channel(to);
		if(chan == NULL)
			return 0;

		if((chan->flags & CHAN_PUBLICFLOOD) || (chan->flags & CHAN_HUSH))
			return 0;

		/* bump pointer to next box */
		chan->resp_boxes_pos = (chan->resp_boxes_pos + 1) % (Grufti->public_flood_num - 1);

		/*
		 * if the time at this position is less than now - inteval,
		 * then we've got a flood.
		 */
		if((now - chan->resp_boxes[chan->resp_boxes_pos]) < Grufti->public_flood_interval)
		{
			/* flood */
			chan->flags |= CHAN_PUBLICFLOOD;
			chan->start_public_flood = now;
			Log_write(LOG_PUBLIC,chan->name,"Public flood detected.  Forwarding all responses back to user.");
			sendprivmsg(chan->name,"I'm feeling bombarded!  Forwarding all responses back to user...");

			/* reset all boxes to 0 */
			for(i=0; i<RESPRATE_NUM; i++)
				chan->resp_boxes[i] = 0L;
			return 0;
		}

		/* set box to time of this response */
		chan->resp_boxes[chan->resp_boxes_pos] = time(NULL);
		return 1;
	}

	return 0;
}

void Response_checkforstat(char *nick, char *to, char *msg)
{
	char	lookup[512], outbuf[512], *p, *q;
	char	target[256], life[25];
	userrec	*u;
	generec	*g;
	time_t	top_usage = 0L, top_chatter = 0L;
	int	i, tot;
	int	top_kicks = 0, top_bans = 0;

	/* private.  check for action line */
	p = Response_enhanced_strstr(msg,"stat");
	if(p != NULL)
		q = p + 4;
	else
	{
		/* Try 'stats' */
		p = Response_enhanced_strstr(msg,"stats");
		if(p != NULL)
			q = p + 5;
		else
			return;
	}

	rmspace(q);

	str_element(lookup,q,1);
	stripfromend_onlypunctuation(lookup);

	outbuf[0] = 0;
	if(!lookup[0])
		sprintf(outbuf,"(Use \"usage\", \"chatter\", \"kicks\", \"bans\", or <nick>)");
	else if(isequal(lookup,"usage"))
	{

		/* Determine top users based on usage */
		for(u=User->userlist; u!=NULL; u=u->next)
		{
			if(!u->registered || (u->flags & USER_BOT))
				continue;
	
			top_usage = User_timeperweek(u);
	
			Notify_addtogenericlist(u->acctname,top_usage);
		}

		tot = Notify_orderbylogin();
		if(tot)
		{
			strcpy(outbuf,"Top 10 users based on IRC usage this week (hrs/day): ");
			for(i=1; i<=10; i++)
			{
				g = Notify_genericbyorder(i);
				if(g == NULL)
					continue;
		
				sprintf(outbuf,"%s%s %.1f, ",outbuf,g->display,(float)(g->login) / (3600.0 * 7.0));
			}
			outbuf[strlen(outbuf) - 2] = 0;
		}

		Notify_freeallgenerics();
	}
	else if(isequal(lookup,"chatter"))
	{
		/* Determine top users based on usage */
		context;
		for(u=User->userlist; u!=NULL; u=u->next)
		{
			if(!u->registered || (u->flags & USER_BOT))
				continue;

		context;
			top_chatter = 0;
			for(i=0; i<7; i++)
				top_chatter += u->public_chatter[i];

		context;
			Notify_addtogenericlist(u->acctname,top_chatter);
		}

		context;
		tot = Notify_orderbylogin();
		if(tot)
		{
		context;
			strcpy(outbuf,"Top 10 users based on public chatter this week (kb/day): ");
		context;
			for(i=1; i<=10; i++)
			{
		context;
				g = Notify_genericbyorder(i);
				if(g == NULL)
					continue;
		
		context;
				sprintf(outbuf,"%s%s %luk, ",outbuf,g->display,g->login/(1024*7));
			}
		context;
			outbuf[strlen(outbuf) - 2] = 0;
		}

		Notify_freeallgenerics();
	}
	else if(isequal(lookup,"kicks"))
	{
		/* Determine top users based on usage */
		for(u=User->userlist; u!=NULL; u=u->next)
		{
			if(!u->registered || (u->flags & USER_BOT))
				continue;

			top_kicks = 0;
			for(i=0; i<7; i++)
				top_kicks += u->kicks[i];

			Notify_addtogenericlist(u->acctname,(time_t)top_kicks);
		}

		tot = Notify_orderbylogin();
		if(tot)
		{
			strcpy(outbuf,"Top 10 users based on kicks this week (kicks/day): ");
			for(i=1; i<=10; i++)
			{
				g = Notify_genericbyorder(i);
				if(g == NULL)
					continue;
		
				sprintf(outbuf,"%s%s %lu, ",outbuf,g->display,g->login/7);
			}
			outbuf[strlen(outbuf) - 2] = 0;
		}

		Notify_freeallgenerics();
	}
	else if(isequal(lookup,"bans"))
	{
		/* Determine top users based on usage */
		for(u=User->userlist; u!=NULL; u=u->next)
		{
			if(!u->registered || (u->flags & USER_BOT))
				continue;

			top_bans = 0;
			for(i=0; i<7; i++)
				top_bans += u->bans[i];

			Notify_addtogenericlist(u->acctname,(time_t)top_bans);
		}

		tot = Notify_orderbylogin();
		if(tot)
		{
			strcpy(outbuf,"Top 10 users based on bans this week (bans/day): ");
			for(i=1; i<=10; i++)
			{
				g = Notify_genericbyorder(i);
				if(g == NULL)
					continue;
		
				sprintf(outbuf,"%s%s %lu, ",outbuf,g->display,g->login/7);
			}
			outbuf[strlen(outbuf) - 2] = 0;
		}

		Notify_freeallgenerics();
	}
	else
	{
		u = User_user(lookup);
		if(!u->registered)
		{
			sprintf(outbuf,"No such stat type \"%s\".  (Use \"usage\", \"chatter\", \"kicks\", \"bans\", or <reg'd nick>)",lookup);
		}
		else
		{
			char	kicks_str[50], bans_str[50];

			kicks_str[0] = 0;
			bans_str[0] = 0;
			for(i=0; i<7; i++)
			{
				top_chatter += u->public_chatter[i];
				top_usage = User_timeperweek(u);
				top_kicks += u->kicks[i];
				top_bans += u->bans[i];
			}

			User_makeircfreq(life,u);

			if(top_kicks/7)
				sprintf(kicks_str,", %d kick%s/day",top_kicks/7,top_kicks/7==1?"":"s");
			if(top_bans/7)
				sprintf(bans_str,", %d ban%s/day",top_bans/7,top_bans/7==1?"":"s");
			sprintf(outbuf,"Stat for %s: %.1f hrs/day (%s), %luk chatter/day%s%s",u->acctname,(float)(top_usage)/(3600.0*7.0),life,top_chatter/(1024*7),kicks_str,bans_str);
		}
	}

	/* Check if target is nick or channel */
	if(Response_checkforpublicflood(to))
		strcpy(target,to);
	else
		strcpy(target,nick);

	sendprivmsg(target,"%s", outbuf);
}
	
void Response_checkforemail(char *nick, char *to, char *msg)
{
	char	lookup[512], outbuf[512], *p, *q;
	char	target[256];
	userrec	*u;
	nickrec	*n;

	/* private.  check for action line */
	p = Response_enhanced_strstr(msg,"email");
	if(p == NULL)
		return;

	q = p + 5;
	rmspace(q);

	/* Check for question format  (what is xxx's email?) */
	if((*q == '?' || strncmp(q,"address",7) == 0 || strncmp(q,"addy",4) == 0) && !Response_enhanced_strstr(q,"for"))
	{
		int x;
		/* Ok, now we need to extract the word just before bday */

		*q = 0;
		x = num_elements(msg);
		if(x > 1)
			str_element(lookup,msg,x-1);
		else
			lookup[0] = 0;

		if(isequal(lookup,"my"))
			strcpy(lookup,nick);
		if(isequal(lookup,"your"))
			strcpy(lookup,Grufti->botnick);

		if(strlen(lookup) > 2)
			if(lookup[strlen(lookup)-2] == '\'' && lookup[strlen(lookup)-1] == 's')
				lookup[strlen(lookup)-2] = 0;
	}
	else
	{
		rmspace(q);
		str_element(lookup,q,1);

		/* check for the word 'address' */
		if(isequal(lookup,"address"))
		{
			q = q + 7;
			rmspace(q);
			str_element(lookup,q,1);
		}

		/* check for the word 'addy' */
		if(isequal(lookup,"addy"))
		{
			q = q + 4;
			rmspace(q);
			str_element(lookup,q,1);
		}
			
		/* check for the word 'for' */
		if(isequal(lookup,"for"))
		{
			q = q + 3;
			rmspace(q);
			str_element(lookup,q,1);
		}
	}
		
	stripfromend_onlypunctuation(lookup);

	context;
	/* Verify lookup */
	if(!lookup[0])
		return;

	context;
	/* Truncate */
	if(strlen(lookup) > NICKLEN)
		lookup[NICKLEN] = 0;

	context;
	/* Convert lookup case to actual nick case */
	u = User_user(lookup);
	if(u->registered)
	{
		n = User_nick(u,lookup);
		if(n != NULL)
			strcpy(lookup,n->nick);

		if(u->email)
			sprintf(outbuf,"%s is %s",lookup,u->email);
		else
			sprintf(outbuf,"%s has no email set.",lookup);
	}
	else
		return;

	/* Check if target is nick or channel */
	if(Response_checkforpublicflood(to))
		strcpy(target,to);
	else
		strcpy(target,nick);

	sendprivmsg(target, "%s", outbuf);
}

void Response_checkforurl(char *nick, char *to, char *msg)
{
	char	lookup[512], outbuf[512], *p, *q;
	char	target[256];
	userrec	*u;
	nickrec	*n;

	/* private.  check for action line */
	p = Response_enhanced_strstr(msg,"url");
	if(p != NULL)
		q = p + 3;
	else
	{
		p = Response_enhanced_strstr(msg,"webpage");
		if(p != NULL)
			q = p + 7;
		else
		{
			p = Response_enhanced_strstr(msg,"homepage");
			if(p == NULL)
				return;

			q = p + 8;
		}
	}

	rmspace(q);
	/* Check for question format  (what is xxx's url?) */
	if(*q == '?')
	{
		int x;
		/* Ok, now we need to extract the word just before bday */
		*q = 0;
		x = num_elements(msg);
		if(x > 1)
			str_element(lookup,msg,x-1);
		else
			lookup[0] = 0;

		if(isequal(lookup,"my"))
			strcpy(lookup,nick);
		if(isequal(lookup,"your"))
			strcpy(lookup,Grufti->botnick);

		if(strlen(lookup) > 2)
			if(lookup[strlen(lookup)-2] == '\'' && lookup[strlen(lookup)-1] == 's')
				lookup[strlen(lookup)-2] = 0;
	}
	else
	{
		rmspace(q);
		str_element(lookup,q,1);

		/* check for the word 'address' */
		if(isequal(lookup,"address"))
		{
			q = q + 8;
			rmspace(q);
			str_element(lookup,q,1);
		}
		
		/* check for the word 'addy' */
		if(isequal(lookup,"addy"))
		{
			q = q + 4;
			rmspace(q);
			str_element(lookup,q,1);
		}
			
		/* check for the word 'for' */
		if(isequal(lookup,"for"))
		{
			q = q + 3;
			rmspace(q);
			str_element(lookup,q,1);
		}
	}
		
	stripfromend_onlypunctuation(lookup);

	context;
	/* Verify lookup */
	if(!lookup[0])
		return;

	if(isequal(lookup,"address"))
		return;

	context;
	/* Truncate */
	if(strlen(lookup) > NICKLEN)
		lookup[NICKLEN] = 0;

	context;
	/* Convert lookup case to actual nick case */
	u = User_user(lookup);
	if(u->registered)
	{
		n = User_nick(u,lookup);
		if(n != NULL)
			strcpy(lookup,n->nick);

		if(u->www)
			sprintf(outbuf,"Check out %s",u->www);
		else
			sprintf(outbuf,"%s has no URL set.",lookup);
	}
	else
		return;

	/* Check if target is nick or channel */
	if(Response_checkforpublicflood(to))
		strcpy(target,to);
	else
		strcpy(target,nick);

	sendprivmsg(target, "%s", outbuf);
}

void Response_checkforinfo(char *nick, char *to, char *msg)
{
	char	lookup[512], outbuf[512], info[512],*p, *q;
	char	gotother[512], gotinfo[512], pre[25], tmp[512];
	char	target[256];
	userrec	*u;
	nickrec	*n;

	/* private.  check for action line */
	p = Response_enhanced_strstr(msg,"info");
	if(p != NULL)
		q = p + 4;
	else
		return;

	rmspace(q);
	str_element(lookup,q,1);
	str_element(info,q,2);

	stripfromend_onlypunctuation(lookup);

	outbuf[0] = 0;

	/* Verify lookup */
	if(!lookup[0] || !info[0])
		sprintf(outbuf,"Usage: info <nick> <info>");
	else
	{

		context;
		/* Truncate */
		if(strlen(lookup) > NICKLEN)
			lookup[NICKLEN] = 0;
	
		context;
		/* Convert lookup case to actual nick case */
		u = User_user(lookup);
		if(u->registered)
		{
			n = User_nick(u,lookup);
			if(n != NULL)
				strcpy(lookup,n->nick);
	
			gotother[0] = 0;
			gotinfo[0] = 0;

			if(isequal(info,"finger") && u->finger)
			{
				strcpy(gotother,"finger");
				strcpy(gotinfo,u->finger);
			}
			if(isequal(info,"email") && u->email)
			{
				strcpy(gotother,"email");
				strcpy(gotinfo,u->email);
			}
			if(isequal(info,"www") && u->www)
			{
				strcpy(gotother,"www");
				strcpy(gotinfo,u->www);
			}
			if(isequal(info,"url") && u->www)
			{
				strcpy(gotother,"url");
				strcpy(gotinfo,u->www);
			}
			if(isequal(info,"bday") && u->bday)
			{
				char hor[25], excl[50];

				bday_plus_hor(tmp,hor,u->bday[0],u->bday[1],u->bday[2]);
				strcpy(gotother,"bday");
				sprintf(gotinfo,"%s%s (%s)",tmp,excl,hor);
			}
			if(u->other1)
			{
				if(strchr(u->other1,' '))
				{
					strcpy(tmp,u->other1);
					split(pre,tmp);
					if(isequal(pre,info))
					{
						strcpy(gotinfo,tmp);
						strcpy(gotother,pre);
					}
				}
			}
			if(u->other2)
			{
				if(strchr(u->other2,' '))
				{
					strcpy(tmp,u->other2);
					split(pre,tmp);
					if(isequal(pre,info))
					{
						strcpy(gotinfo,tmp);
						strcpy(gotother,pre);
					}
				}
			}

			if(u->other3)
			{
				if(strchr(u->other3,' '))
				{
					strcpy(tmp,u->other3);
					split(pre,tmp);
					if(isequal(pre,info))
					{
						strcpy(gotinfo,tmp);
						strcpy(gotother,pre);
					}
				}
			}

			if(u->other4)
			{
				if(strchr(u->other4,' '))
				{
					strcpy(tmp,u->other4);
					split(pre,tmp);
					if(isequal(pre,info))
					{
						strcpy(gotinfo,tmp);
						strcpy(gotother,pre);
					}
				}
			}

			if(u->other5)
			{
				if(strchr(u->other5,' '))
				{
					strcpy(tmp,u->other5);
					split(pre,tmp);
					if(isequal(pre,info))
					{
						strcpy(gotinfo,tmp);
						strcpy(gotother,pre);
					}
				}
			}

			if(u->other6)
			{
				if(strchr(u->other6,' '))
				{
					strcpy(tmp,u->other6);
					split(pre,tmp);
					if(isequal(pre,info))
					{
						strcpy(gotinfo,tmp);
						strcpy(gotother,pre);
					}
				}
			}

			if(u->other7)
			{
				if(strchr(u->other7,' '))
				{
					strcpy(tmp,u->other7);
					split(pre,tmp);
					if(isequal(pre,info))
					{
						strcpy(gotinfo,tmp);
						strcpy(gotother,pre);
					}
				}
			}

			if(u->other8)
			{
				if(strchr(u->other8,' '))
				{
					strcpy(tmp,u->other8);
					split(pre,tmp);
					if(isequal(pre,info))
					{
						strcpy(gotinfo,tmp);
						strcpy(gotother,pre);
					}
				}
			}

			if(u->other9)
			{
				if(strchr(u->other9,' '))
				{
					strcpy(tmp,u->other9);
					split(pre,tmp);
					if(isequal(pre,info))
					{
						strcpy(gotinfo,tmp);
						strcpy(gotother,pre);
					}
				}
			}

			if(gotother[0])
				sprintf(outbuf,"%s - %s: %s",lookup,gotother,gotinfo);
			else
				sprintf(outbuf,"Info \"%s\" not found for %s.",info,lookup);
		}
		else
			return;
	}

	/* Check if target is nick or channel */
	if(Response_checkforpublicflood(to))
		strcpy(target,to);
	else
		strcpy(target,nick);

	sendprivmsg(target, "%s", outbuf);
}
void Response_checkforshowbdays(char *nick, char *to, char *msg)
{
	char	outbuf[512], target[256], excl[50], *p;
	int	mnow, dnow, ynow, mnew, dnew, ynew, aM, aD, aY, i, tot;
	u_long	now, then, check;
	userrec	*u;
	generec	*g;

	/* private.  check for action line */
	p = Response_enhanced_strstr(msg,"showbdays");
	if(p == NULL)
	{
		/* no?  check for 'showbirthdays' then */
		p = Response_enhanced_strstr(msg,"showbirthdays");
		if(p == NULL)
			return;
	}
	/* OK, now list all birthdays within the next 2 weeks */
	mdy_today(&mnow, &dnow, &ynow);

	/* Determine day 14 days from now */
	mnew = mnow;
	dnew = dnow;
	ynew = ynow;
	calc_new_date(&mnew, &dnew, &ynew, 14);

	/* convert to # */
	now = calc_days(mnow,dnow,ynow);
	then = calc_days(mnew,dnew,ynew);

	/* Ok run through the userlist and get all users within limits */
	for(u=User->userlist; u!=NULL; u=u->next)
	{
		if(u->bday[0])
		{
			if(mnow == 12 && u->bday[0] == 1)
				check = calc_days(u->bday[0],u->bday[1],ynow+1);
			else
				check = calc_days(u->bday[0],u->bday[1],ynow);

			if(now <= check && check <= then)
				Notify_addtogenericlist(u->acctname,check);
		}
	}

	/* Ok now display */
	tot = Notify_orderbylogin();

	if(tot == 0)
		strcpy(outbuf,"There are no birthdays in the next 2 weeks.");
	else
	{
		strcpy(outbuf,"Birthdays in the next 2 weeks: ");
		for(i=tot; i>=1; i--)
		{
			g = Notify_genericbyorder(i);
			if(g != NULL)
			{
				uncalc_days(&aM, &aD, &aY, g->login);
				excl[0] = 0;
				if(aM == mnow && aD == dnow)
					strcpy(excl," (Happy Birthday!)");
				if(i == 1)
					sprintf(outbuf,"%s%s %d/%d%s",outbuf,g->display,aM,aD,excl);
				else
					sprintf(outbuf,"%s%s %d/%d%s, ",outbuf,g->display,aM,aD,excl);
			}
		}
	}

	Notify_freeallgenerics();

	/* Check if target is nick or channel */
	if(Response_checkforpublicflood(to))
		strcpy(target,to);
	else
		strcpy(target,nick);

	sendprivmsg(target, "%s", outbuf);
}

void Response_checkforbday(char *nick, char *to, char *msg)
{
	char	lookup[512], outbuf[512], *p, *q;
	char	target[256], display[150];
	userrec	*u;
	nickrec	*n;

	/* private.  check for action line */
	p = Response_enhanced_strstr(msg,"bday");
	if(p != NULL)
		q = p + 4;
	else
	{
		/* no?  check for 'birthday' then */
		p = Response_enhanced_strstr(msg,"birthday");
		if(p == NULL)
			return;

		/* that worked. */
		q = p + 8;
	}

	rmspace(q);

	/* Check for question format  (what is xxx's birthday?) */
	if(*q == '?')
	{
		int x;
		/* Ok, now we need to extract the word just before bday */
		*q = 0;
		x = num_elements(msg);
		if(x > 1)
			str_element(lookup,msg,x-1);
		else
			lookup[0] = 0;

		if(isequal(lookup,"my"))
			strcpy(lookup,nick);
		if(isequal(lookup,"your"))
			strcpy(lookup,Grufti->botnick);

		if(strlen(lookup) > 2)
			if(lookup[strlen(lookup)-2] == '\'' && lookup[strlen(lookup)-1] == 's')
				lookup[strlen(lookup)-2] = 0;
	}
	else
	{
		rmspace(q);
		str_element(lookup,q,1);

		/* check for the word 'for' */
		if(isequal(lookup,"for"))
		{
			q = q + 3;
			rmspace(q);
			str_element(lookup,q,1);
		}
	}
		
	stripfromend_onlypunctuation(lookup);

	context;
	/* Verify lookup */
	if(!lookup[0])
		return;

	context;
	/* Truncate */
	if(strlen(lookup) > NICKLEN)
		lookup[NICKLEN] = 0;

	context;
	/* Convert lookup case to actual nick case */
	u = User_user(lookup);
	if(u->registered)
	{
		n = User_nick(u,lookup);
		if(n != NULL)
			strcpy(lookup,n->nick);

		if(u->bday[0])
		{
			char hor[25], excl[50];
			int m, d, y;

			bday_plus_hor(display,hor,u->bday[0],u->bday[1],u->bday[2]);
			mdy_today(&m,&d,&y);
			if(m == u->bday[0] && d == u->bday[1])
				strcpy(excl,", today!  Happy Birthday!");
			else
				strcpy(excl,".");

			if(u->bday[2] == -1)
				sprintf(outbuf,"%s's birthday is on %s%s (%s)",lookup,display,excl,hor);
			else
				sprintf(outbuf,"%s's birthday is %s%s (%s)",lookup,display,excl,hor);
		}
		else
			sprintf(outbuf,"%s has no bday set.",lookup);
	}
	else
		return;

	/* Check if target is nick or channel */
	if(Response_checkforpublicflood(to))
		strcpy(target,to);
	else
		strcpy(target,nick);

	sendprivmsg(target, "%s", outbuf);
}


void Response_checkforhaveyouseen(char *nick, char *to, char *msg)
{
	char	lookup[512], when[TIMELONGLEN], outbuf[512], *p, *q;
	char	signoff[SERVERLEN], target[256];
	userrec	*u;
	acctrec	*a = NULL;
	nickrec	*n;
	int	x;

	context;
	/* private.  check for action line */
	p = Response_enhanced_strstr(msg,"have you seen");
	if(p == NULL)
		return;

	context;
	q = strchr(p,'n');
	if(q == NULL)
	{
		q = strchr(p,'N');
		if(q == NULL)
			return;
	}
	q++;

	str_element(lookup,q,1);
	stripfromend_onlypunctuation(lookup);

	context;
	/* Verify lookup */
	if(!lookup[0])
		return;

	context;
	/* Truncate */
	if(strlen(lookup) > NICKLEN)
		lookup[NICKLEN] = 0;

	context;
	/* check for 'my' */
	if(isequal(lookup,"my"))
	{
		char	tmp_my[BUFSIZ];
		strcpy(tmp_my,q+4);

		if(!tmp_my[0])
			return;
		if(strlen(tmp_my) > 60)
			tmp_my[60] = 0;
		stripfromend_onlypunctuation(tmp_my);
		sprintf(outbuf,"I haven't seen your damn %s, now shut up.",tmp_my);

		/* Check if target is nick or channel */
		if(Response_checkforpublicflood(to))
			strcpy(target,to);
		else
			strcpy(target,nick);
	
		sendprivmsg(target, "%s", outbuf);

		return;
	}
		
	context;
	/* Convert lookup case to actual nick case */
	u = User_user(lookup);
	if(u->registered)
	{
		n = User_nick(u,lookup);
		if(n != NULL)
			strcpy(lookup,n->nick);
	}

	context;
	x = Response_haveyouseen(u,lookup,&a,when);

	if(x == 2 || x == 9 || x == 10)
	{
		if(u->signoff)
			sprintf(signoff," (%s)",u->signoff);
		else
			strcpy(signoff,"");
	}

	context;
	switch(x)
	{
	case 0 : sprintf(outbuf,"No such user \"%s\".",lookup); break;
	case 1 : sprintf(outbuf,"sees %s (%s) on IRC.",a->nick,a->uh); break;
	case 2 : sprintf(outbuf,"last saw %s (%s) %s ago.%s",a->nick,a->uh,when,signoff); break;
	case 3 : sprintf(outbuf,"sees %s (%s) on IRC.",a->nick,a->uh); break;
	case 4 : sprintf(outbuf,"sees %s on IRC as %s (%s).",lookup,a->nick,a->uh); break;
	case 5 : sprintf(outbuf,"sees %s (%s) connected via DCC.",a->nick,a->uh); break;
	case 6 : sprintf(outbuf,"sees %s connected via DCC as %s (%s).",lookup,a->nick,a->uh); break;
	case 7 : sprintf(outbuf,"sees %s (%s) connected via telnet.",a->nick,a->uh); break;
	case 8 : sprintf(outbuf,"sees %s connected via telnet as %s (%s).",lookup,a->nick,a->uh); break;
	case 9 : sprintf(outbuf,"last saw %s (%s) %s ago.%s",a->nick,a->uh,when,signoff); break;
	case 10 : sprintf(outbuf,"last saw %s as %s (%s) %s ago.%s",lookup,a->nick,a->uh,when,signoff); break;
	case 11 : sprintf(outbuf,"Never seen %s.  (but they do have an account.)",lookup); break;
	}

	context;
	/* Check if target is nick or channel */
	if(Response_checkforpublicflood(to))
		strcpy(target,to);
	else
		strcpy(target,nick);

	context;
	/* Send action for all types but 0 and 11 */
	if(x == 0 || x == 11)
		sendprivmsg(target, "%s", outbuf);
	else
		sendaction(target, "%s", outbuf);
}
	
int Response_haveyouseen(userrec *u, char *lookup, acctrec **a_seen, char *when)
/*
 * Return types are:
 *    0 - "No such user."
 *    1 - "sees <seenas> on IRC."		(not registered)
 *    2 - "last saw <seenas> <when> ago."	(not registered)
 *    3 - "sees <seenas> on IRC."
 *    4 - "sees <lookup> on IRC as <seenas>."
 *    5 - "sees <seenas> connected via DCC."
 *    6 - "sees <lookup> connected via DCC as <seenas>."
 *    7 - "sees <seenas> connected via telnet."
 *    8 - "sees <lookup> connected via telnet as <seenas>."
 *    9 - "last saw <seenas> <when> ago."
 *   10 - "last saw <lookup> as <seenas> <when> ago."
 *   11 - "never seen <lookup>.  But they do have an account."
 *
 * Being on IRC precedes telnet which precedes DCC.
 */
{
	acctrec	*a;
	membrec	*m;

	*a_seen = NULL;
	when[0] = 0;
	if(!u->registered)
	{
		/* not registered.  First see if they're on a channel. */
		m = Channel_memberofanychan(lookup);
		if(m != NULL)
		{
			*a_seen = m->account;
			return 1;
		}

		/* not on channel.  Find latest account in luser */
		a = User_lastseenunregisterednick(u,lookup);
		if(a != NULL)
		{
			timet_to_ago_long(a->last_seen,when);
			*a_seen = a;
			return 2;
		}

		/* no such user */
		return 0;
	}

	/* user is registered.  First check if any nicks are on IRC. */
	a = User_lastseentype(u,ACCT_ONIRC);
	if(a != NULL)
	{
		*a_seen = a;
		if(isequal(lookup,a->nick))
			return 3;
		else
			return 4;
	}

	/* not on IRC.  via DCC? */
	a = User_lastseentype(u,ACCT_VIADCC);
	if(a != NULL)
	{
		/* is connected via DCC */
		*a_seen = a;
		if(isequal(lookup,a->nick))
			return 5;
		else
			return 6;
	}

	/* not on IRC or via DCC.  via telnet? */
	a = User_lastseentype(u,ACCT_VIATELNET);
	if(a != NULL)
	{
		*a_seen = a;
		if(isequal(lookup,a->nick))
			return 7;
		else
			return 8;
	}

	/* not on IRC, DCC, or telnet.  Find their last record. */
	a = User_lastseen(u);
	if(a != NULL)
	{
		timet_to_ago_long(a->last_seen,when);
		*a_seen = a;
		if(isequal(lookup,a->nick))
			return 9;
		else
			return 10;
	}

	/* we've never seen them! */
	return 11; 
}

int Response_orderbyresponse(rtype *rt)
{
	resprec	*r, *r_save = NULL;
	int	counter = 0, done = 0;
	char	h[256], highest[256];

	/* Put the highest ascii character in 'highest'. */
	sprintf(highest,"%c",HIGHESTASCII);

	/* We need to clear all ordering before we start */
	Response_clearorder(rt);

	/* Continue until all entries have an order number */
	while(!done)
	{
		done = 1;
		strcpy(h,highest);
		for(r=rt->responses; r!=NULL; r=r->next)
		{
			if(!r->order && strcasecmp(r->name,h) <= 0)
			{
				done = 0;
				strcpy(h,r->name);
				r_save = r;
			}
		}

		/* Not done means we got u_save and now we need to order it */
		if(!done)
		{
			counter++;
			r_save->order = counter;
		}
	}

	return counter;
}

void Response_clearorder(rtype *rt)
{
	resprec	*r;

	for(r=rt->responses; r!=NULL; r=r->next)
		r->order = 0;
}

resprec	*Response_responsebyorder(rtype *rt, int ordernum)
{
	resprec	*r;

	for(r=rt->responses; r!=NULL; r=r->next)
		if(r->order == ordernum)
			return r;

	return NULL;
}

void Response_checkresponses(char *nick, char *to, char *msg)
{
	/* If botnick isn't embedded in response, return now */
	if(!Response_checkforbotnick(msg))
		return;

	/* Check each type */
	Response_checkforinfo(nick,to,msg);
	if(Grufti->respond_to_stat)
		Response_checkforstat(nick,to,msg);
	if(Grufti->respond_to_email)
		Response_checkforemail(nick,to,msg);
	if(Grufti->respond_to_url)
		Response_checkforurl(nick,to,msg);
	if(Grufti->respond_to_bday)
		Response_checkforbday(nick,to,msg);
	if(Grufti->respond_to_showbdays)
		Response_checkforshowbdays(nick,to,msg);
	if(Grufti->respond_to_haveyouseen)
		Response_checkforhaveyouseen(nick,to,msg);
}

void Response_checkforcron()
{
	struct tm *btime;
	time_t	now = time(NULL);
	char	timezone[10];
	chanrec	*chan;

	if(!Grufti->sing_1015)
		return;

	timezone[0] = 0;
	btime = localtime(&now);
	if(btime->tm_wday == 6)
	{
		if(btime->tm_hour == 22 && btime->tm_min == 15)
			sprintf(timezone,"PST");
		else if(btime->tm_hour == 21 && btime->tm_min == 15)
			sprintf(timezone,"MST");
		else if(btime->tm_hour == 20 && btime->tm_min == 15)
			sprintf(timezone,"CST");
		else if(btime->tm_hour == 19 && btime->tm_min == 15)
			sprintf(timezone,"EST");
	}

	if(timezone[0])
	{
		for(chan=Channel->channels; chan!=NULL; chan=chan->next)
		{
			sendprivmsg(chan->name,"\"...10:15 on a Saturday night.. (%s) ... and the tap drips, under the strip light... And i'm sitting in the kitchen sink... and the tap drips drip drip drip drip drip...\"",timezone);
		}

		Response->time_sang_1015 = time(NULL);
	}
}


/* ASDF */
/****************************************************************************
 *
 * rtype record definitions.  No changes should be made to new(), freeall(),
 * append(), delete(), or movetofront().
 *
 ****************************************************************************/

rtype *rtype_new()
{
	rtype *x;

	/* allocate memory */
	x = (rtype *)xmalloc(sizeof(rtype));

	/* initialize */
	x->next = NULL;

	rtype_init(x);

	return x;
}

void rtype_freeall(rtype **list, rtype **last)
{
	rtype *entry = *list, *v;

	while(entry != NULL)
	{
		v = entry->next;
		rtype_free(entry);
		entry = v;
	}

	*list = NULL;
	*last = NULL;
}

void rtype_append(rtype **list, rtype **last, rtype *x)
{
	rtype *lastentry = *last;

	if(*list == NULL)
		*list = x;
	else
		lastentry->next = x;

	*last = x;
	x->next = NULL;
}

void rtype_delete(rtype **list, rtype **last, rtype *x)
{
	rtype *entry, *lastchecked = NULL;

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

			rtype_free(entry);
			return;
		}

		lastchecked = entry;
		entry = entry->next;
	}
}

/****************************************************************************/
/* record rtype low-level data-specific function definitions. */

void rtype_init(rtype *rt)
{
	/* initialize */
	rt->name = NULL;
	rt->type = 0;
	rt->level = 0L;
	rt->matches = NULL;
	rt->last_matches = NULL;
	rt->num_matches = 0;
	rt->except = NULL;
	rt->last_except = NULL;
	rt->num_excepts = 0;
	rt->responses = NULL;
	rt->last_response = NULL;
	rt->num_responses = 0;
}

u_long rtype_sizeof(rtype *rt)
{
	u_long	tot = 0L;
	machrec	*m;
	resprec	*r;

	tot += sizeof(rtype);
	tot += rt->name ? strlen(rt->name)+1 : 0L;
	for(m=rt->matches; m!=NULL; m=m->next)
		tot += machrec_sizeof(m);
	for(m=rt->except; m!=NULL; m=m->next)
		tot += machrec_sizeof(m);
	for(r=rt->responses; r!=NULL; r=r->next)
		tot += resprec_sizeof(r);

	return tot;
}

void rtype_free(rtype *rt)
{
	/* Free the elements. */
	xfree(rt->name);
	Response_freeallrtypematches(rt);
	Response_freeallrtypeexcepts(rt);
	Response_freeallresponses(rt);

	/* Free this record. */
	xfree(rt);
}

void rtype_setname(rtype *rt, char *name)
{
	mstrcpy(&rt->name,name);
}

rtype *rtype_rtype(rtype **list, rtype **last, char *name)
/*
 * Make this check MATCHES, not name
 */
{
	rtype *rt;

	for(rt=*list; rt!=NULL; rt=rt->next)
	{
		if(isequal(rt->name,name))
			return rt;
	}

	return NULL;
}


/***************************************************************************n
 *
 * resprec record definitions.  No changes should be made to new(), freeall(),
 * append(), delete(), or movetofront().
 *
 ****************************************************************************/

resprec *resprec_new()
{
	resprec *x;

	/* allocate memory */
	x = (resprec *)xmalloc(sizeof(resprec));

	/* initialize */
	x->next = NULL;

	resprec_init(x);

	return x;
}

void resprec_freeall(resprec **list, resprec **last)
{
	resprec *entry = *list, *v;

	while(entry != NULL)
	{
		v = entry->next;
		resprec_free(entry);
		entry = v;
	}

	*list = NULL;
	*last = NULL;
}

void resprec_append(resprec **list, resprec **last, resprec *x)
{
	resprec *lastentry = *last;

	if(*list == NULL)
		*list = x;
	else
		lastentry->next = x;

	*last = x;
	x->next = NULL;
}

void resprec_delete(resprec **list, resprec **last, resprec *x)
{
	resprec *entry, *lastchecked = NULL;

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

			resprec_free(entry);
			return;
		}

		lastchecked = entry;
		entry = entry->next;
	}
}

void resprec_movetofront(resprec **list, resprec **last, resprec *lastchecked, resprec *x)
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
/* record resprec low-level data-specific function definitions. */

void resprec_init(resprec *r)
{
	/* initialize */
	r->name = NULL;
	r->matches = NULL;
	r->last_matches = NULL;
	r->num_matches = 0;
	r->except = NULL;
	r->last_except = NULL;
	r->num_excepts = 0;
	r->lines = NULL;
	r->last_line = NULL;
	r->num_lines = 0;
	r->flags = 0;
	r->order = 0;
	r->nick = NULL;
	r->last = 0L;
	r->history = NULL;
}

u_long resprec_sizeof(resprec *r)
{
	u_long	tot = 0L;
	machrec	*m;
	rline	*l;

	tot += sizeof(resprec);
	tot += r->name ? strlen(r->name)+1 : 0L;
	for(m=r->matches; m!=NULL; m=m->next)
		tot += machrec_sizeof(m);
	for(m=r->except; m!=NULL; m=m->next)
		tot += machrec_sizeof(m);
	for(l=r->lines; l!=NULL; l=l->next)
		tot += rline_sizeof(l);

	return tot;
}

void resprec_free(resprec *r)
{
	/* Free the elements. */
	xfree(r->name);
	xfree(r->matches);
	xfree(r->except);
	Response_freealllines(r);
	listmgr_destroy_list(r->history);

	/* Free this record. */
	xfree(r);
}

void resprec_setname(resprec *r, char *name)
{
	mstrcpy(&r->name,name);
}

void resprec_setnick(resprec *r, char *nick)
{
	mstrcpy(&r->nick,nick);
}

resprec *resprec_resp(resprec **list, resprec **last, char *name)
/*
 * Again, make this match MATCHES, not name
 */
{
	resprec *r, *lastchecked = NULL;

	for(r=*list; r!=NULL; r=r->next)
	{
		if(isequal(r->name,name))
		{
			/* found it.  do a movetofront. */
			resprec_movetofront(list,last,lastchecked,r);

			return r;
		}
		lastchecked = r;
	}

	return NULL;
}



/***************************************************************************n
 *
 * rline record definitions.  No changes should be made to new(), freeall(),
 * append(), delete(), or movetofront().
 *
 ****************************************************************************/

rline *rline_new()
{
	rline *x;

	/* allocate memory */
	x = (rline *)xmalloc(sizeof(rline));

	/* initialize */
	x->next = NULL;

	rline_init(x);

	return x;
}

void rline_freeall(rline **list, rline **last)
{
	rline *entry = *list, *v;

	while(entry != NULL)
	{
		v = entry->next;
		rline_free(entry);
		entry = v;
	}

	*list = NULL;
	*last = NULL;
}

void rline_append(rline **list, rline **last, rline *x)
{
	rline *lastentry = *last;

	if(*list == NULL)
		*list = x;
	else
		lastentry->next = x;

	*last = x;
	x->next = NULL;
}

void rline_delete(rline **list, rline **last, rline *x)
{
	rline *entry, *lastchecked = NULL;

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

			rline_free(entry);
			return;
		}

		lastchecked = entry;
		entry = entry->next;
	}
}

/****************************************************************************/
/* record rline low-level data-specific function definitions. */

void rline_init(rline *rl)
{
	/* initialize */
	rl->text = NULL;
	rl->flags = 0;
}

u_long rline_sizeof(rline *l)
{
	u_long	tot = 0L;

	tot += sizeof(rline);
	tot += l->text ? strlen(l->text)+1 : 0L;

	return tot;
}

void rline_free(rline *rl)
{
	/* Free the elements. */
	xfree(rl->text);

	/* Free this record. */
	xfree(rl);
}

void rline_settext(rline *rl, char *text)
{
	mstrcpy(&rl->text,text);
}



/***************************************************************************n
 *
 * machrec record definitions.  No changes should be made to new(), freeall(),
 * append(), delete(), or movetofront().
 *
 ****************************************************************************/

machrec *machrec_new()
{
	machrec *x;

	/* allocate memory */
	x = (machrec *)xmalloc(sizeof(machrec));

	/* initialize */
	x->next = NULL;

	machrec_init(x);

	return x;
}

void machrec_freeall(machrec **list, machrec **last)
{
	machrec *entry = *list, *v;

	while(entry != NULL)
	{
		v = entry->next;
		machrec_free(entry);
		entry = v;
	}

	*list = NULL;
	*last = NULL;
}

void machrec_append(machrec **list, machrec **last, machrec *x)
{
	machrec *lastentry = *last;

	if(*list == NULL)
		*list = x;
	else
		lastentry->next = x;

	*last = x;
	x->next = NULL;
}

void machrec_delete(machrec **list, machrec **last, machrec *x)
{
	machrec *entry, *lastchecked = NULL;

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

			machrec_free(entry);
			return;
		}

		lastchecked = entry;
		entry = entry->next;
	}
}

/****************************************************************************/
/* record machrec low-level data-specific function definitions. */

void machrec_init(machrec *mr)
{
	/* initialize */
	mr->match = NULL;
	mr->flags = 0;
}

u_long machrec_sizeof(machrec *m)
{
	u_long	tot = 0L;

	tot += sizeof(machrec);
	tot += m->match ? strlen(m->match)+1 : 0L;

	return tot;
}

void machrec_free(machrec *mr)
{
	/* Free the elements. */
	xfree(mr->match);

	/* Free this record. */
	xfree(mr);
}

void machrec_setmatch(machrec *mr, char *match)
{
	mstrcpy(&mr->match,match);
}




/***************************************************************************n
 *
 * combo record definitions.  No changes should be made to new(), freeall(),
 * append(), delete(), or movetofront().
 *
 ****************************************************************************/

combo *combo_new()
{
	combo *x;

	/* allocate memory */
	x = (combo *)xmalloc(sizeof(combo));

	/* initialize */
	x->next = NULL;

	combo_init(x);

	return x;
}

void combo_freeall(combo **list, combo **last)
{
	combo *entry = *list, *v;

	while(entry != NULL)
	{
		v = entry->next;
		combo_free(entry);
		entry = v;
	}

	*list = NULL;
	*last = NULL;
}

void combo_append(combo **list, combo **last, combo *x)
{
	combo *lastentry = *last;

	if(*list == NULL)
		*list = x;
	else
		lastentry->next = x;

	*last = x;
	x->next = NULL;
}

void combo_delete(combo **list, combo **last, combo *x)
{
	combo *entry, *lastchecked = NULL;

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

			combo_free(entry);
			return;
		}

		lastchecked = entry;
		entry = entry->next;
	}
}

/****************************************************************************/
/* record combo low-level data-specific function definitions. */

void combo_init(combo *c)
{
	/* initialize */
	c->name = NULL;
	c->columns = NULL;
	c->last = NULL;
	c->num_columns = 0;
}

u_long combo_sizeof(combo *c)
{
	u_long	tot = 0L;
	combcol	*cc;

	tot += sizeof(combo);
	tot += c->name ? strlen(c->name)+1 : 0L;
	for(cc=c->columns; cc!=NULL; cc=cc->next)
		tot += combcol_sizeof(cc);

	return tot;
}

void combo_free(combo *c)
{
	/* Free the elements. */

	/* Free this record. */
	xfree(c);
}

void combo_setname(combo *c, char *name)
{
	mstrcpy(&c->name,name);
}




/***************************************************************************n
 *
 * combcol record definitions.  No changes should be made to new(), freeall(),
 * append(), delete(), or movetofront().
 *
 ****************************************************************************/

combcol *combcol_new()
{
	combcol *x;

	/* allocate memory */
	x = (combcol *)xmalloc(sizeof(combcol));

	/* initialize */
	x->next = NULL;

	combcol_init(x);

	return x;
}

void combcol_freeall(combcol **list, combcol **last)
{
	combcol *entry = *list, *v;

	while(entry != NULL)
	{
		v = entry->next;
		combcol_free(entry);
		entry = v;
	}

	*list = NULL;
	*last = NULL;
}

void combcol_append(combcol **list, combcol **last, combcol *x)
{
	combcol *lastentry = *last;

	if(*list == NULL)
		*list = x;
	else
		lastentry->next = x;

	*last = x;
	x->next = NULL;
}

void combcol_delete(combcol **list, combcol **last, combcol *x)
{
	combcol *entry, *lastchecked = NULL;

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

			combcol_free(entry);
			return;
		}

		lastchecked = entry;
		entry = entry->next;
	}
}

/****************************************************************************/
/* record combcol low-level data-specific function definitions. */

void combcol_init(combcol *cc)
{
	/* initialize */
	cc->elements = NULL;
	cc->last = NULL;
	cc->num_elements = 0;
}

u_long combcol_sizeof(combcol *cc)
{
	u_long	tot = 0L;
	combelm	*ce;

	tot += sizeof(combcol);
	for(ce=cc->elements; ce!=NULL; ce=ce->next)
		tot += combelm_sizeof(ce);

	return tot;
}

void combcol_free(combcol *ce)
{
	/* Free the elements. */

	/* Free this record. */
	xfree(ce);
}




/***************************************************************************n
 *
 * combelm record definitions.  No changes should be made to new(), freeall(),
 * append(), delete(), or movetofront().
 *
 ****************************************************************************/

combelm *combelm_new()
{
	combelm *x;

	/* allocate memory */
	x = (combelm *)xmalloc(sizeof(combelm));

	/* initialize */
	x->next = NULL;

	combelm_init(x);

	return x;
}

void combelm_freeall(combelm **list, combelm **last)
{
	combelm *entry = *list, *v;

	while(entry != NULL)
	{
		v = entry->next;
		combelm_free(entry);
		entry = v;
	}

	*list = NULL;
	*last = NULL;
}

void combelm_append(combelm **list, combelm **last, combelm *x)
{
	combelm *lastentry = *last;

	if(*list == NULL)
		*list = x;
	else
		lastentry->next = x;

	*last = x;
	x->next = NULL;
}

void combelm_delete(combelm **list, combelm **last, combelm *x)
{
	combelm *entry, *lastchecked = NULL;

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

			combelm_free(entry);
			return;
		}

		lastchecked = entry;
		entry = entry->next;
	}
}

/****************************************************************************/
/* record combelm low-level data-specific function definitions. */

void combelm_init(combelm *ce)
{
	/* initialize */
	ce->elem = NULL;
}

u_long combelm_sizeof(combelm *ce)
{
	u_long	tot = 0L;

	tot += sizeof(combelm);
	tot += ce->elem ? strlen(ce->elem)+1 : 0L;

	return tot;
}

void combelm_free(combelm *ce)
{
	/* Free the elements. */
	xfree(ce->elem);

	/* Free this record. */
	xfree(ce);
}

void combelm_setelem(combelm *ce, char *elem)
{
	mstrcpy(&ce->elem,elem);
}

