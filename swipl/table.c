/*  $Id$

    Part of XPCE --- The SWI-Prolog GUI toolkit

    Author:        Jan Wielemaker and Anjo Anjewierden
    E-mail:        wielemak@science.uva.nl
    WWW:           http://www.swi-prolog.org/projects/xpce/
    Copyright (C): 1985-2007, University of Amsterdam

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

typedef struct asymbol *ASymbol;
typedef struct table   *Table;

struct asymbol
{ atom_t	atom;
  PceName	name;
  ASymbol	next;
};

struct table
{ ASymbol      *symbols;
  int		allocated;
  int	        size;
  int		mask;
};

static struct table atom_to_name;
static struct table name_to_atom;

#define AtomKey(t, a) (int)(((a)>>5) & (t)->mask)
#define NameKey(t, a) (int)(((uintptr_t)(a)>>2) & (t)->mask)

static void
rehashTable(Table t, int aton)
{ ASymbol *old   = t->symbols;
  int oldentries = t->allocated;
  int n;

  t->allocated *= 2;
  t->mask = t->allocated - 1;
  t->symbols    = malloc(t->allocated * sizeof(ASymbol));
  memset(t->symbols, 0, t->allocated * sizeof(ASymbol));

  for(n=0; n<oldentries; n++)
  { ASymbol s = old[n];
    ASymbol n;

    for( ; s; s = n )
    { int k;

      n = s->next;

      if ( aton )
	k = AtomKey(t, s->atom);
      else
	k = NameKey(t, s->name);

      s->next = t->symbols[k];
      t->symbols[k] = s;
    }
  }

  free(old);
}

static PceName
atomToName(atom_t a)
{ int k = AtomKey(&atom_to_name, a);
  ASymbol s = atom_to_name.symbols[k];
  PceName name;
  size_t len;
  const char *textA;
  const wchar_t *textW;

  for( ; s; s = s->next )
  { if ( s->atom == a )
      return s->name;
  }

  PL_register_atom(a);
  if ( (textA = PL_atom_nchars(a, &len)) )
  { name = cToPceName_nA(textA, len);
  } else if ( (textW = PL_atom_wchars(a, &len)) )
  { name = cToPceName_nW(textW, len);
  } else
  { assert(0);
    return NULL;
  }

  s = pceAlloc(sizeof(struct asymbol));
  s->atom = a;
  s->name = name;
  s->next = atom_to_name.symbols[k];
  atom_to_name.symbols[k] = s;
  if ( ++atom_to_name.size > 2*atom_to_name.allocated )
    rehashTable(&atom_to_name, TRUE);

  return name;
}


static atom_t
CachedNameToAtom(PceName name)
{ int k = NameKey(&name_to_atom, name);
  ASymbol s = name_to_atom.symbols[k];
  atom_t a;
  size_t len;
  const char *textA;
  const wchar_t *textW;

  for( ; s; s = s->next )
  { if ( s->name == name )
      return s->atom;
  }

  if ( (textA = pceCharArrayToCA(name, &len)) )
  { a = PL_new_atom_nchars(len, textA);
  } else if ( (textW = pceCharArrayToCW(name, &len)) )
  { a = PL_new_atom_wchars(len, textW);
  } else
  { assert(0);
    return 0;
  }

  s = pceAlloc(sizeof(struct asymbol));
  s->atom = a;
  s->name = name;
  s->next = name_to_atom.symbols[k];
  name_to_atom.symbols[k] = s;
  if ( ++name_to_atom.size > 2*name_to_atom.allocated )
    rehashTable(&name_to_atom, FALSE);

  return a;
}


static void
initTable(Table t)
{ t->allocated = 1024;
  t->size      = 0;
  t->mask      = t->allocated-1;
  t->symbols   = malloc(t->allocated * sizeof(ASymbol));
  memset(t->symbols, 0, t->allocated * sizeof(ASymbol));
}


static void
initNameAtomTable()
{ initTable(&atom_to_name);
  initTable(&name_to_atom);
}
