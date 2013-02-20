/* $Id: usage.c,v 1.8 2003/02/14 00:34:14 elf Exp $ */
/*
 * Copyright 1994-97, 1999, 2003 Luis Fernandes <elf@ee.ryerson.ca> 
 *
 * Permission to use, copy, hack, and distribute this software and its
 * documentation for any purpose and without fee is hereby granted,
 * provided that the above copyright notice appear in all copies and
 * that both that copyright notice and this permission notice appear
 * in supporting documentation.  This application is presented as is
 * without any implied or written warranty.
 *
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 * 
 */

#include <stdio.h>
#include <stdlib.h>

#include "maindefs.h"
#include "patchlevel.h"

void
printUsage(str)
char *str;
{

  fprintf(stderr, "This is xmotd %d.%d %s\n", RELEASE, PATCH, STATUS);

  fprintf(stderr, USAGESTRING, str);

  fprintf(stderr, "\nValid options are:\n");

  fprintf(stderr, "   -help             display this message\n");
  fprintf(stderr, "   -always           ignore time-stamp and always display motd(s)\n");

  fprintf(stderr, "   -bitmaplogo file  show the bitmap");
#ifdef HAVE_XPM
  fprintf(stderr, "/pixmap");
#endif
  fprintf(stderr," in \"file\" instead of X logo\n");

#ifdef HAVE_HTML
  fprintf(stderr, "   -browser program  invoke \"program\" when URL is clicked\n");
#endif

  fprintf(stderr, "   -paranoid         (used with -warnfile) always show the warning message\n");


  fprintf(stderr, "   -popdown #        automatically pop-down xmotd after waiting # seconds\n");
  fprintf(stderr, "   -stampfile file   use \"file\" as timestamp, instead of \"%s\"\n", TIMESTAMP);
  fprintf(stderr, "   -showfilename     show name of the file currently being viewed\n");
  
  fprintf(stderr, "   -usedomains       append domain-names to timestamp\n");
  fprintf(stderr, "   -wakeup #.#       every # hours, check motd(s) for changes\n");
  fprintf(stderr, "   -warnfile file    show the warning message in \"file\" before motd(s)\n");
  fprintf(stderr, "   -atom name        register xmotd with \"name\"\n");
  fprintf(stderr, "   -tail             scroll & display end of motd file(s)\n");

  fprintf(stderr, "\n");
  exit(1);
		  

}
