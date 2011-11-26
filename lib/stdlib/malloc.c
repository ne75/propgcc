/*
 * @malloc.c
 * Implementation of memory allocation functions
 *
 * This is an extremely simple memory allocator, vaguely inspired
 * by the one in K&R.
 *
 * Copyright (c) 2011 Parallax, Inc.
 * Written by Eric R. Smith, Total Spectrum Software Inc.
 * MIT licensed (see terms at end of file)
 */

#include <stdlib.h>
#include <string.h>
#include <sys/thread.h>
#include "malloc.h"

static MemHeader *freelist;
static atomic_t malloc_lock;

void *
malloc(size_t n)
{
  MemHeader *p;
  MemHeader **prevp;
  size_t numunits;  /* size of block in MemHeader size chunks */

  numunits = (n + sizeof(MemHeader)-1)/sizeof(MemHeader) + 1;

  __lock(&malloc_lock);
  prevp = &freelist;

  for (p = (*prevp); p; prevp = &p->next, p = p->next)
    {
      if (p->len >= numunits)
	{
	  /* a big enough block */
	  if (p->len == numunits)
	    {
	      /* exactly */
	      *prevp = p->next;
	    }
	  else
	    {
	      /* allocate the tail */
	      p->len -= numunits;
	      p += p->len;
	      p->len = numunits;
	    }
	  break;
	}
    }

  if (!p)
    {
      p = _sbrk(numunits * sizeof(MemHeader));
    }
  __unlock(&malloc_lock);
  if (!p)
    return NULL;
  p->next = MAGIC;
  p->len = numunits;
  return (void *)(p+1);
}

void
free(void *ptr)
{
  struct MemHeader *thisp, *p, **prev;

  thisp = (MemHeader *)ptr;
  --thisp;
  if (thisp->next != MAGIC)
    {
      /* something is wrong, ignore the free request */
      return;
    }
  thisp->next = NULL;

  __lock(&malloc_lock);
  /* see if we can merge this into a block on the free list */
  if (!freelist)
    {
      /* no freelist, so just release this right away */
      freelist = thisp;
      __unlock(&malloc_lock);
      return;
    }

  prev = &freelist;
  p = *prev;
  while (p)
    {
      if (p + p->len == thisp)
	{
	  /* merge it in */
	  p->len += thisp->len;

	  /* see if we bump into the next block */
	  if (p + p->len == p->next)
	    {
	      MemHeader *nextp;
	      nextp = p->next;
	      p->len += nextp->len;
	      p->next = nextp->next;
	      nextp->next = NULL;
	    }
	  __unlock(&malloc_lock);
	  return;
	}

      if (thisp + thisp->len == p)
	{
	  *prev = thisp;
	  thisp->next = p->next;
	  thisp->len += p->len;
	  __unlock(&malloc_lock);
	  return;
	}

      if (thisp < p)
	break;

      prev = &p->next;
      p = *prev;
    }

  /* could not find a mergable block */
  /* just add it to the free list */
  thisp->next = *prev;
  *prev = thisp;
  __unlock(&malloc_lock);
}

/* +--------------------------------------------------------------------
 * ¦  TERMS OF USE: MIT License
 * +--------------------------------------------------------------------
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files
 * (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 * +--------------------------------------------------------------------
 */