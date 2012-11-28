/* Copyright 1996-97 Stuart A. HarveyStuart A. Harvey <sharvey@primenet.com> 
 *
 * Permission to use, copy, hack, and distribute this software and its
 * documentation for any purpose and without fee is hereby granted,
 * provided that the above copyright notice appear in all copies and
 * that both that copyright notice and this permission notice appear
 * in supporting documentation.  This application is presented as is
 * without any implied or written warranty.
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
/* browser.c: 
 *	callback procedure to launch a browser (tm) when a hypertext
 *	anchor is activated.
 * $Id: browser.c,v 1.3 1997/08/28 20:09:27 elf Exp $
 */

#ifdef HAVE_HTML

#include <stdio.h>
#include "maindefs.h"
#include "libhtmlw/HTML.h"
#include "main.h"

extern app_res_t app_res;

void __ExecWebBrowser ( char* href );

XtCallbackProc 
AnchorCallbackProc( Widget w, caddr_t call_data, caddr_t client_data)
{
  WbAnchorCallbackData	*anchor_data = ( WbAnchorCallbackData *) client_data;
  __ExecWebBrowser ( anchor_data->href );

}

void
__ExecWebBrowser ( char* href )
{
   static int browser_pid=0;
   char   *default_browser=BROWSER;
   char   *local_browser=BROWSER;

   if ( ! app_res.browser ) { 
     local_browser=default_browser;
   } else {
     local_browser=app_res.browser;
   }

   browser_pid = fork();

   if ( browser_pid == 0 ) { /* child process */
     execlp(local_browser,local_browser,href,NULL);
     fprintf(stderr,"xmotd: Couldn't exec %s\n", local_browser);
   } else {
     int status;
     (void) wait(&status);
   }
}
#endif
