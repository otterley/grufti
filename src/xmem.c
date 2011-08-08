/* -------------------------------------------------------------------------
 * Steve Rider - stever@area23.org
 * Grufti - an interactive, service-oriented bot for IRC 
 *
 * xmem.c - memory allocation and tracking
 *
 * Compile with -DTEST to test this module.
 * ------------------------------------------------------------------------- */
/* 23 June 98 */

#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "grufti.h" /* Declarations for xmem.c are in misc.h */

#ifdef TEST
#define STANDALONE
#endif

#ifdef STANDALONE

/* Normally defined in grufti.c */
#define fatal_error debug_to_stderr
#define debug debug_to_stderr
#define warning debug_to_stderr
#define internal debug_to_stderr
static void debug_to_stderr(char *fmt, ...)
{
	va_list args;

	va_start(args, fmt);
	vfprintf(stderr, fmt, args);
	fprintf(stderr, "\n");
	va_end(args);
}

#endif /* STANDALONE */


#ifdef MEMDEBUG
/* Prototypes for internal commands */
static void set_memtbl_file(char *file, int pos);

/* memory table */
static u_long mem_total = 0L;
static int mem_curpos = 0;

#define MEM_MEMTBLSIZE 200000 /* careful, memtbl struct is around 30 bytes */
#define MEM_FILELENGTH 15

static struct {
	void *ptr;
	int size;
	char file[MEM_FILELENGTH];
	int line;
} memtbl[MEM_MEMTBLSIZE];

#endif /* MEMDEBUG */


void init_mem()
/*
 * Function: Initialize memory table
 *
 *  This must be done before any calls to malloc().  A simple way is
 *  to execute the steps
 *
 *      #ifdef MEMDEBUG
 *          init_mem();
 *      #endif
 *
 *  in the beginning of your program.
 */
{
#ifdef MEMDEBUG
	int i;

	for(i = 0; i < MEM_MEMTBLSIZE; i++)
	{
		memtbl[i].ptr = NULL;
		memtbl[i].file[0] = 0;
		memtbl[i].size = 0;
		memtbl[i].line = 0;
	}
#endif
}


void *mem_malloc(int size, char *file, int line)
/*
 * Function: Override for standard malloc
 *  Returns: pointer to block of newly allocated memory
 *
 *  Internal command for xmem.c.  Actual usage is achieved by calling
 *  xmalloc() in place of malloc() in your program.  The #define in
 *  xmem.h overrides it and calls this routine with the appropriate
 *  file and line.
 */
{
	void *x;

	x = (void *)malloc(size);
	if(x == NULL)
	{
		fatal_error("mem: in malloc(), malloc failed. (%s:%d)", file, line);
		return NULL;
	}

#ifdef MEMDEBUG
	if(mem_curpos == MEM_MEMTBLSIZE)
	{
		fatal_error("mem: in malloc(), memory table full. (%s:%d)", file, line);
		return x;
	}

	memtbl[mem_curpos].ptr = x;
	memtbl[mem_curpos].size = size;
	set_memtbl_file(file, mem_curpos);
	memtbl[mem_curpos].line = line;
	mem_curpos++;
	mem_total += size;
#endif

	return x;
}


void *mem_realloc(void *ptr, int size, char *file, int line)
/*
 * Function: Override for standard realloc
 *  Returns: pointer to realloc'd block of memory
 *
 *  Internal command for xmem.c.  Actual usage is achieved by calling
 *  xrealloc() in place of realloc() in your program.  The #define in
 *  xmem.h overrides it and calls this routine with the appropriate
 *  file and line.
 */
{
#ifdef MEMDEBUG
	int i, pos = -1;
#endif
	void *x;

	if(ptr == NULL)
	{
		internal("mem: in realloc(), ptr is NULL. (%s:%d)", file, line);
		return NULL;
	}

	x = (void *)realloc(ptr, size);
	if(x == NULL)
	{
		fatal_error("mem: in realloc(), realloc failed. (%s:%d)", file, line);
		return NULL;
	}

#ifdef MEMDEBUG
	for(i = 0; i < mem_curpos; i++)
		if(memtbl[i].ptr == ptr)
			pos = i;

	if(pos == -1)
	{
		internal("mem: in realloc(), ptr not malloc'd. (%s:%d)", file, line);
		return NULL;
	}

	mem_total -= memtbl[pos].size;

	memtbl[pos].ptr = x;
	memtbl[pos].line = line;
	memtbl[pos].size = size;
	set_memtbl_file(file, pos);
	mem_total += size;
#endif

	return x;
}


void mem_strcpy(char **dest, char *src)
/*
 * Function: allocates memory and copies string
 *
 */
{
	/*
	 * xfree() will handle cases where *dest is NULL.
	 */
	xfree(*dest);
	*dest = (char *)xmalloc(strlen(src) + 1);
	strcpy(*dest, src);
}


#ifdef MEMDEBUG
/* Windows includes the whole path in the file.  We'll strip it off. */
static void set_memtbl_file(char *file, int pos)
{
	char *p, *q = NULL;

	q = file;
	p = strchr(file, '\\');
	while(p != NULL)
	{
		q = p+1;
		p = strchr(q, '\\');
	}

	strncpy(memtbl[pos].file, q, MEM_FILELENGTH-1);
	memtbl[pos].file[MEM_FILELENGTH-1] = 0;
}
#endif


int mem_free(void *ptr, char *file, int line)
/*
 * Function: Override for standard free
 *  Returns: 1 if successful
 *           0 if ptr is NULL, or if ptr has not been malloc'd (if MEMDEBUG)
 *
 *  Internal command for xmem.c.  Actual usage is achieved by calling
 *  xfree() in place of free() in your program.  The #define in xmem.h
 *  overrides it and calls this routine with the appropriate file and line.
 *
 *  Note that we don't consider passing a NULL ptr an error.
 */
{
#ifdef MEMDEBUG
	int i, pos = -1;
#endif

	if(ptr == NULL)
		return 0;

#ifdef MEMDEBUG
	for(i = 0; i < mem_curpos; i++)
		if(memtbl[i].ptr == ptr)
			pos = i;

	if(pos == -1)
	{
		internal("mem: in free(), ptr not malloc'd. (%s:%d)", file, line);
		return 0;
	}

	mem_total -= memtbl[pos].size;

	/* Move info from most recent entry into current slot */
	mem_curpos--;
	memtbl[pos].ptr = memtbl[mem_curpos].ptr;
	memtbl[pos].size = memtbl[mem_curpos].size;
	memtbl[pos].line = memtbl[mem_curpos].line;
	strcpy(memtbl[pos].file, memtbl[mem_curpos].file);
#endif

	free(ptr);

	return 1;
}


u_long mem_total_memory()
/*
 * Function: Determine how much memory is currently in use
 *  Returns: total memory in use (0 if MEMDEBUG not defined)
 *
 */
{
#ifndef MEMDEBUG
	warning("mem: in total_memory(), MEMDEBUG not defined.  No memory info available.");
	return 0L;
#else
	return mem_total;
#endif
}


int mem_total_alloc()
/*
 * Function: Determine how many calls to malloc have been performed
 *  Returns: Number of entries in the memtbl (0 if MEMDEBUG not defined)
 *
 */
{
#ifndef MEMDEBUG
	warning("mem: in total_alloc(), MEMDEBUG not defined.  No memory info available.");
	return 0L;
#else
	return mem_curpos;
#endif
}


u_long mem_file_memory(char *file)
/*
 * Function: Determine how much memory is in use by a particular file
 *  Returns: memory in use by file (0 if MEMDEBUG not defined, or not found)
 *
 */
{
#ifdef MEMDEBUG
	int i;
	u_long total = 0L;
#endif

#ifndef MEMDEBUG
	warning("mem: in file_memory(), MEMDEBUG not defined.  No memory info available.");
	return 0L;
#else

	if(file == NULL)
	{
		internal("mem: in file_memory(), file is NULL.");
		return 0L;
	}
	for(i = 0; i < mem_curpos; i++)
	{
		if(strcmp(memtbl[i].file, file) == 0)
			total += memtbl[i].size;
	}

	return total;
#endif
}


int mem_file_alloc(char *file)
/*
 * Function: Determine how many allocations made in a file
 *  Returns: allocations made by file (0 if MEMDEBUG not defined, or not found)
 *
 */
{
#ifdef MEMDEBUG
	int i, total = 0;
#endif

#ifndef MEMDEBUG
	warning("mem: in file_alloc(), MEMDEBUG not defined.  No memory info available.");
	return 0;
#else

	if(file == NULL)
	{
		internal("mem: in file_alloc(), file is NULL.");
		return 0;
	}
	for(i = 0; i < mem_curpos; i++)
	{
		if(strcmp(memtbl[i].file, file) == 0)
			total++;
	}

	return total;
#endif
}

void mem_dump_memtbl()
{
#ifndef MEMDEBUG
	warning("mem: in mem_dump_memtbl(), MEMDEBUG not defined.  No memory info available.");
#else
	int i;

	for(i = 0; i < mem_curpos; i++)
	{
		debug("%s:%d - %d", memtbl[i].file,
			memtbl[i].line, memtbl[i].size);
	}
#endif
}





#ifdef TEST
/* -------------------------------------------------------------------------
 * TESTING
 *
 * Test mem_malloc(), mem_free(), and tracking functions.
 *
 * These tests can be performed in any order.
 *
 * All tests should perform OK, and no warnings should occur.  Compile with
 *    gcc -o xmem xmem.c -DTEST -DMEMDEBUG
 * ------------------------------------------------------------------------- */

int verify_malloc_and_free()
{
	int ok = 0, ok_test = 0;
	char *p1, *p2, *p3;

	if((p1 = (char *)xmalloc(100)) != NULL)
		ok_test++;
	ok++;
	if((p2 = (char *)xmalloc(150)) != NULL)
		ok_test++;
	ok++;
	if((p3 = (char *)xmalloc(200)) != NULL)
		ok_test++;
	ok++;

	if(xfree(p1))
		ok_test++;
	ok++;
	if(xfree(p2))
		ok_test++;
	ok++;
	if(xfree(p3))
		ok_test++;
	ok++;

	return ok == ok_test;
}

int verify_realloc()
{
	int ok = 0, ok_test = 0;
	char *p1, *p2;

	if((p1 = (char *)xmalloc(100)) != NULL)
		ok_test++;
	ok++;

	if((p2 = (char *)xrealloc(p1, 200)) != NULL)
		ok_test++;
	ok++;

	xfree(p2);
	return ok == ok_test;
}
	
int verify_mem_tracking()
{
	int ok = 0, ok_test = 0;
	char *p1, *p2, *p3;

	p1 = (char *)xmalloc(10);
	p2 = (char *)xmalloc(20);
	p3 = (char *)xmalloc(30);

	ok_test += mem_total_alloc();
	ok += 3;
	ok_test += mem_file_alloc("xmem.c");
	ok += 3;

	ok_test += mem_total_memory();
	ok += 60;
	ok_test += mem_file_memory("xmem.c");
	ok += 60;

	/* Try a realloc */
	p1 = (char *)xrealloc(p1, 100);
	ok_test += mem_total_alloc();
	ok += 3;
	ok_test += mem_file_alloc("xmem.c");
	ok += 3;

	ok_test += mem_total_memory();
	ok += 150;
	ok_test += mem_file_memory("xmem.c");
	ok += 150;

	xfree(p1);
	xfree(p2);
	xfree(p3);

	if(mem_total_alloc() == 0)
		ok_test++;
	ok++;
	if(mem_file_alloc("xmem.c") == 0)
		ok_test++;
	ok++;

	if(mem_total_memory() == 0)
		ok_test++;
	ok++;
	if(mem_file_memory("xmem.c") == 0)
		ok_test++;
	ok++;

	return ok == ok_test;
}

int main()
{
	int ok1 = 0, ok2 = 0, ok3 = 0;

	printf("Testing module: xmem\n");
	printf("  %s\n", (ok1 = verify_malloc_and_free()) ? "malloc_and_free: ok" : "malloc_and_free: failed");
	printf("  %s\n", (ok2 = verify_realloc()) ? "realloc: ok" : "realloc: failed");
#ifdef MEMDEBUG
	printf("  %s\n", (ok3 = verify_mem_tracking()) ? "mem_tracking: ok" : "mem_tracking: failed");
#else
	printf("  %s\n","(MEMDEBUG not defined, mem_tracking test skipped)");
#endif

	if(ok1 && ok2 && ok3)
		printf("xmem: passed\n");
	else
		printf("xmem: failed\n");

	printf("\n");
	return 0;
}
#endif /* TEST */
