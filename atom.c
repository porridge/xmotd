/*$Id: atom.c,v 1.2 1999/11/23 00:50:14 elf Exp $*/

/*
 * Copyright 1999 Luis Fernandes <elf@ee.ryerson.ca> 
 *
 * Permission to use, copy, hack, and distribute this software and its
 * documentation for any purpose and without fee is hereby granted,
 * provided that the above copyright notice appear in all copies and
 * that both that copyright notice and this permission notice appear
 * in supporting documentation.  This application is presented as is
 * without any implied or written warranty.
 *
 */
/*
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

#include <sys/types.h>
#include <errno.h>
#include <pwd.h>

#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>

extern XtAppContext app_con;
extern Widget topLevel;				/* the application widget*/


/* To prevent multiple copies of xmotd from running (when run with
   -wakeup) for the same user (this usually happens when a user logs
   in and out multiple times within the timeout period), we create an
   Atom in the server so other copies of xmotd can check it

   returns: True if the atom already exists
            False if the atom didn't exist (and was created)
*/

Boolean 
atomExists(String atomname)
{
  Atom xmotd;
  struct passwd *pw;

  static char buffer[256];
  pw = getpwuid(getuid());

  /* create an unique atom */
  sprintf(buffer, "xmotd-%s.%s", atomname, pw->pw_name); 
  
  /*check if the atom exists*/
  xmotd=XInternAtom(XtDisplay(topLevel), buffer, (Boolean)True);

  /*if the atom doesn't exist*/
  if(xmotd==None){
	/*create it*/
	xmotd=XInternAtom(XtDisplay(topLevel), buffer, (Boolean)False);
/* 	fprintf(stderr,"Created atom: %s\n", buffer); */

	return(False);
  }
  else{
	fprintf(stderr,"xmotd (atomized as %s) is already running-- aborting.\n",
			atomname);
	return(True);
  }

} /* atomExists */
