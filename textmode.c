/* $Id: textmode.c,v 1.7 1998/11/03 22:24:31 elf Exp $ */

/*
 * Copyright 1993-97 Luis Fernandes <elf@ee.ryerson.ca> 
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
#include <string.h>
#include <malloc.h>
#include <sys/stat.h>
#include <memory.h>
#include <fcntl.h>

#include "maindefs.h"

void
runInTextMode(argc, argv)
int argc;
char **argv;
{

  register int i, displayed=0;
  float sleepPeriod=0.0;
  
  static int onceAlready;
  
  static char buf[256], stampfile[256];
  memset(buf, 0, 256);
  memset(stampfile, 0, 256);

  strcpy(stampfile, TIMESTAMP);	/* default stampfile name */
  
  while(1)
	{
	  sprintf(buf, "%s/%s", getenv("HOME"), stampfile);
	  
	  for(i=1; i<argc; i++)
		{

		  /* if it doesn't begin with a '-', its a file */
		  if(argv[i][0]!='-')	
			{

			  if(motdChanged(argv[i], buf))
				{
				  char *txtbuf;
				  struct stat motdstat;
				  FILE *fp;
			  
				  if((fp=fopen(argv[i],"r"))==NULL)
					{
					  perror(argv[i]);
					  continue;	/* get next file */
					}

				  stat(argv[i],&motdstat);
				  if(!motdstat.st_size) continue; /* ignore zero-length files */
			  
				  txtbuf=(char *)calloc(1, (motdstat.st_size+1)*sizeof(char));
				  if(!txtbuf)
					{
					  perror("xmotd");
					  exit(2);
					}

				  fread(txtbuf,(int)motdstat.st_size,1,fp);
				  fclose(fp);
			  
				  fprintf(stdout, "%s", txtbuf);
				  free(txtbuf);
				  displayed++;
			  
				}
			}
		  else
			/* text mode only understands "-stampfile" and "-wakeup"
               options apr/15/96*/
			{
			  /* this get done everytime we wake-up, maybe we need
                 some check here to the options are parsed only
                 once...*/

			  if(!strcmp((argv[i]), "-stampfile"))
				{
				  strcpy(stampfile, (argv[i+1])); /* next param is the filename */
				  i++;

				  sprintf(buf, "%s/%s", getenv("HOME"), stampfile);
/*				  fprintf(stderr, "stampfile is %s", buf);*/

				}
			  else if(!strcmp((argv[i]), "-wakeup"))
				{
				  /* next param is the period in hrs==> convert to
					 seconds*/
				  sleepPeriod=(atof(argv[i+1])*3600.0);	
				  i++;
				}
			  else
				{
				  fprintf(stdout, "%s: WARNING, ignoring %s\n", argv[0], argv[i]);
				}
			  
			}
	  
		}
	  
	  if(displayed)
		{
/*		  fprintf(stderr, "Displayed file(s)\n");*/
		  updateTimeStamp(buf);		/* reset the timestamp after all files 
									   have been read*/ 
		}
	  
	  if(sleepPeriod)
		{
		  int fd;
		  
		  if(fork()) exit(0);
		  sleep((unsigned)sleepPeriod);
		  
		  /* Check if user is still logged-in by trying to open
			 "/dev/tty". If we can't open the controlling terminal
			 then the user has logged-out (W. Richard Stevens
			 _Advanced Programming in the Unix Environment_) and xmotd
			 can exit. */
		  
		  if((fd=open("/dev/tty", O_RDONLY, O_RDONLY)<0))
			{
			exit(0);
		  }
		  
		  close(fd);
		  displayed=0;			/* reset the flag  */
		}
	  else
		{
		  exit(0);
		}
	  
	} /* while forever */
  
}
