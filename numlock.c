/* Code adapted from NumLockX.
 *
 * This file licensed MIT/X11 license.
 */

#include <X11/Xlib.h>
#include <X11/XKBlib.h>
#include <X11/keysym.h>
#include <string.h>

static __inline__ int check_lib_compat(Display * dpy)
{
  int xkb_opcode, xkb_event, xkb_error;
  int xkb_lmaj = XkbMajorVersion;
  int xkb_lmin = XkbMinorVersion;

  return XkbLibraryVersion(&xkb_lmaj, &xkb_lmin)
    && XkbQueryExtension(dpy, &xkb_opcode, &xkb_event, &xkb_error,
			 &xkb_lmaj, &xkb_lmin);
}

static __inline__ unsigned int
get_xkb_modmask_by_name(Display *dpy, XkbDescPtr xkb, const char *name)
{
  int i;
  Atom *modAtoms;
  char *modStr;
  unsigned int mask;

  if (!xkb->names)
    return 0;

  modAtoms = xkb->names->vmods;
  for (i = 0; i < XkbNumVirtualMods; i++)
    if ((modStr = XGetAtomName(dpy, modAtoms[i])) != NULL
	&& !strcmp(modStr, name))
      {
	XkbVirtualModsToReal(xkb, 1 << i, &mask);
	return mask;
      }

  return 0;
}

static __inline__ unsigned int get_xkb_numlock_mask(Display * dpy)
{
  unsigned int mask = 0;
  XkbDescPtr xkb;

  if ((xkb = XkbGetKeyboard(dpy, XkbAllComponentsMask, XkbUseCoreKbd)))
    {
      mask = get_xkb_modmask_by_name(dpy, xkb, "NumLock");
      XkbFreeKeyboard(xkb, 0, True);
    }

  return mask;
}

void set_numlock_state(Display * dpy, int state)
{
  unsigned int mask;
  static int virgin = 1, is_compatible;

  if (virgin)
    {
      virgin = 0;
      is_compatible = check_lib_compat(dpy);
    }
  if (!is_compatible)
    return;

  if ((mask = get_xkb_numlock_mask(dpy)))
    XkbLockModifiers(dpy, XkbUseCoreKbd, mask, state ? mask : 0);
}

/* 
   Copyright (C) 2000-2001 Lubos Lunak        <l.lunak@kde.org>
   Copyright (C) 2001      Oswald Buddenhagen <ossi@kde.org>

   Permission is hereby granted, free of charge, to any person obtaining a
   copy of this software and associated documentation files (the "Software"),
   to deal in the Software without restriction, including without limitation
   the rights to use, copy, modify, merge, publish, distribute, sublicense,
   and/or sell copies of the Software, and to permit persons to whom the
   Software is furnished to do so, subject to the following conditions:

   The above copyright notice and this permission notice shall be included in
   all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
   THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
   FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
   DEALINGS IN THE SOFTWARE.

*/
