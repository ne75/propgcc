/*
 * Copyright (c) 2009, Sun Microsystems, Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * - Redistributions of source code must retain the above copyright notice,
 *   this list of conditions and the following disclaimer.
 * - Redistributions in binary form must reproduce the above copyright notice,
 *   this list of conditions and the following disclaimer in the documentation
 *   and/or other materials provided with the distribution.
 * - Neither the name of Sun Microsystems, Inc. nor the names of its
 *   contributors may be used to endorse or promote products derived
 *   from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * xdr_reference.c, Generic XDR routines impelmentation.
 *
 * Copyright (C) 1987, Sun Microsystems, Inc.
 *
 * These are the "non-trivial" xdr primitives used to serialize and de-serialize
 * "pointers".  See xdr.h for more info on the interface to xdr.
 */

#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <rpc/types.h>
#include <rpc/xdr.h>

#include "xdr_private.h"

#define LASTUNSIGNED    ((u_int)0-1)

/*
 * XDR an indirect pointer
 * xdr_reference is for recursively translating a structure that is
 * referenced by a pointer inside the structure that is currently being
 * translated.  pp references a pointer to storage. If *pp is null
 * the  necessary storage is allocated.
 * size is the sizeof the referneced structure.
 * proc is the routine to handle the referenced structure.
 */
bool_t
_DEFUN (xdr_reference, (xdrs, pp, size, proc),
        XDR * xdrs _AND
	caddr_t * pp _AND
	u_int size _AND
	xdrproc_t proc)
{
  caddr_t loc = *pp;
  bool_t stat;

  if (loc == NULL)
    switch (xdrs->x_op)
      {
      case XDR_FREE:
        return TRUE;

      case XDR_DECODE:
        *pp = loc = (caddr_t) mem_alloc (size);
        if (loc == NULL)
          {
            xdr_warnx ("xdr_reference: out of memory");
            errno = ENOMEM;
            return FALSE;
          }
        memset (loc, 0, size);
        break;

      case XDR_ENCODE:
        break;
      }

  stat = (*proc) (xdrs, loc, LASTUNSIGNED);

  if (xdrs->x_op == XDR_FREE)
    {
      mem_free (loc, size);
      *pp = NULL;
    }
  return stat;
}


/*
 * xdr_pointer():
 *
 * XDR a pointer to a possibly recursive data structure. This
 * differs with xdr_reference in that it can serialize/deserialiaze
 * trees correctly.
 *
 *  What's sent is actually a union:
 *
 *  union object_pointer switch (boolean b) {
 *  case TRUE: object_data data;
 *  case FALSE: void nothing;
 *  }
 *
 * > objpp: Pointer to the pointer to the object.
 * > obj_size: size of the object.
 * > xdr_obj: routine to XDR an object.
 *
 */
bool_t
_DEFUN (xdr_pointer, (xdrs, objpp, obj_size, xdr_obj),
        XDR * xdrs _AND
	char **objpp _AND
	u_int obj_size _AND
	xdrproc_t xdr_obj)
{
  bool_t more_data;

  more_data = (*objpp != NULL);
  if (!xdr_bool (xdrs, &more_data))
    {
      return FALSE;
    }
  if (!more_data)
    {
      *objpp = NULL;
      return TRUE;
    }
  return (xdr_reference (xdrs, objpp, obj_size, xdr_obj));
}
