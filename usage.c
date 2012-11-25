/* $Id: usage.c,v 1.2 1996/08/14 16:42:33 elf Exp $ */
/*
 * Copyright 1993, 1994, 1995, 1996 Luis Fernandes <elf@ee.ryerson.ca> 
 *
 * Permission to use, copy, hack, and distribute this software and its
 * documentation for any purpose and without fee is hereby granted,
 * provided that the above copyright notice appear in all copies and
 * that both that copyright notice and this permission notice appear
 * in supporting documentation.  This application is presented as is
 * without any implied or written warranty.
 *
 */

#include <stdio.h>

#include "maindefs.h"

void
printUsage(str)
char *str;
{

  fprintf(stderr, USAGESTRING, str); /* missing motd filename */
  fprintf(stderr, "\nOptions are:\n");

  fprintf(stderr, "   -help             display this message\n");
  fprintf(stderr, "   -always           ignore time-stamp and always display motd(s)\n");

  fprintf(stderr, "   -bitmaplogo file  show the bitmap in \"file\" instead of X logo\n");
  fprintf(stderr, "   -paranoid         (used with -warnfile) always show the warning message\n");
  fprintf(stderr, "   -popdown #        automatically pop-down xmotd after waiting # seconds\n");
  fprintf(stderr, "   -stampfile file   use \"file\" as timestamp, instead of \"%s\"\n", TIMESTAMP);
  fprintf(stderr, "   -showfilename     show filename, currently being viewed, alongside date\n");
  
  fprintf(stderr, "   -usedomains       append domain-names to timestamp\n");
  fprintf(stderr, "   -wakeup #.#       every # hours, check motd(s) for changes\n");
  fprintf(stderr, "   -warnfile file    show the warning message in \"file\" before motd(s)\n");


  fprintf(stderr, "\n");
  exit(1);
		  

}
