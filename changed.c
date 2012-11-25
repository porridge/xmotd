/* Copyright 1993, 1994  Luis Fernandes <elf@ee.ryerson.ca> 
 *
 * Permission to use, copy, hack, and distribute this software and its
 * documentation for any purpose and without fee is hereby granted,
 * provided that the above copyright notice appear in all copies and
 * that both that copyright notice and this permission notice appear
 * in supporting documentation.  This application is presented as is
 * without any implied or written warranty.
 */

/* changed.c: utility function for checking the times of 2 files
 * $Id: changed.c,v 1.3 1996/08/14 19:35:01 elf Exp $ 
 */
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <X11/Intrinsic.h>

/*#include <X11/StringDefs.h>*/
#include "main.h"

extern app_res_t app_res;

/* motdChanged() returns time_t of the motd file: if modification time
 * of motd is greater than the modification time of time-stamp file;
 * else it returns 0.
 * */

time_t 
motdChanged(char *motd, char *stamp)
{
  struct stat motdstat, tsstat;

  stat(motd, &motdstat);

  if(stat(stamp, &tsstat))
    {
     extern int errno;

      if(errno==ENOENT)			/* file does not exist if 1st time */
		return(motdstat.st_mtime);
    }

  if(motdstat.st_mtime <= tsstat.st_mtime) /*Butch Deal*/
    return((time_t) 0);

  if(!motdstat.st_size && !app_res.always)
    return((time_t) 0);

  return(motdstat.st_mtime);
}

