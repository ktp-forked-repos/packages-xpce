/*  $Id$

    Part of XPCE

    Author:  Jan Wielemaker and Anjo Anjewierden
    E-mail:  jan@swi.psy.uva.nl
    WWW:     http://www.swi.psy.uva.nl/projects/xpce/
    Copying: GPL-2.  See the file COPYING or http://www.gnu.org

    Copyright (C) 1990-2001 SWI, University of Amsterdam. All rights reserved.
*/

#include <h/kernel.h>
#include <h/graphics.h>

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
TBD:	handle destruction of these objects.  Not that important as they
	are generally not destroyed and only a bit of memory is wasted
	if they are.
- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#define XREF_TABLESIZE 		256
#define HashValue(obj)		(((unsigned long)(obj) & (XREF_TABLESIZE-1)))

static Xref XrefTable[XREF_TABLESIZE];


WsRef
getXrefObject(Any obj, DisplayObj d)
{ int v = HashValue(obj);
  Xref r;

  XrefsResolved++;

  for( r = XrefTable[v]; r != NULL; r = r->next)
    if ( r->object == obj && r->display == d )
    { DEBUG(NAME_getXref, Cprintf("getXrefObject(%s, %s) --> 0x%lx\n",
				  pp(obj), pp(d), (unsigned long) r->xref));
      return r->xref;
    }

  if ( openDisplay(d) == SUCCEED )
  { if ( send(obj, NAME_Xopen, d, EAV) == SUCCEED )
    { for( r = XrefTable[v]; r != NULL; r = r->next)
	if ( r->object == obj && r->display == d )
	{ DEBUG(NAME_getXref, Cprintf("getXrefObject(%s, %s) --> 0x%lx\n",
				      pp(obj), pp(d), (unsigned long) r->xref));
	  return r->xref;
	}
    }
  }

  XrefsResolved--;

  errorPce(obj, NAME_xOpen, d);

  return NULL;
}


WsRef
getExistingXrefObject(Any obj, DisplayObj d)
{ int v = HashValue(obj);
  Xref r;


  for( r = XrefTable[v]; r != NULL; r = r->next)
    if ( r->object == obj && r->display == d )
    { XrefsResolved++;
      return r->xref;
    }

  return NULL;
}


status
registerXrefObject(Any obj, DisplayObj d, WsRef xref)
{ Xref *R = &XrefTable[HashValue(obj)];
  Xref r, new;

  DEBUG(NAME_xref, Cprintf("registerXrefObject(%s, %s, 0x%lx)\n",
			   pp(obj), pp(d), (unsigned long) xref));

  for( r = *R; r != NULL; r = r->next)
    if ( r->object == obj && r->display == d )
    { r->xref = xref;
      succeed;
    }

  new = alloc(sizeof(struct xref));
  new->object = obj;
  new->display = d;
  new->xref = xref;
  new->next = *R;
  *R = new;

  succeed;
}


Xref
unregisterXrefObject(Any obj, DisplayObj d)
{ Xref *R = &XrefTable[HashValue(obj)];
  Xref r = *R;
  static struct xref old;

  for( ; r != NULL; R = &r->next, r = *R )
  { if ( r->object == obj && (r->display == d || isDefault(d)) )
    { *R = r->next;

      DEBUG(NAME_xref, Cprintf("unregisterXrefObject(%s, %s)\n",
			       pp(obj), pp(r->display)));
      old = *r;
      unalloc(sizeof(struct xref), r);
      return &old;
    }
  }

  fail;
}


void
closeAllXrefs()
{ int i;  

  for(i=0; i<XREF_TABLESIZE; i++)
  { Xref r = XrefTable[i];
    Xref nr;

    for(; r; r = nr)
    { nr = r->next;

      send(r->object, NAME_Xclose, r->display, EAV);
    }
  }
}


#if KEEP
static void
unregisterAllXrefsObject(Any obj)
{ Xref *R = &XrefTable[HashValue(obj)];
  Xref r = *R;
  WsRef old;

  for( ; r != NULL; R = &r->next, r = *R )
  { if ( r->object == obj )
    { *R = r->next;
      R = &r->next;

      DEBUG(NAME_xref, Cprintf("unregisterXrefObject(%s, %s)\n",
			       pp(obj), pp(r->display)));
      old = r->xref;
      unalloc(sizeof(struct xref), r);

    } else
      R = &r->next;
  }
}
#endif

