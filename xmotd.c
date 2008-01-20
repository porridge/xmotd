/*$Id: xmotd.c,v 1.20 2003/02/14 00:37:38 elf Exp $*/

/*
 * Copyright 1994-97, 2003 Luis Fernandes <elf@ee.ryerson.ca> 
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
#include <stdlib.h>
#include <malloc.h>
#include <time.h>
#include <memory.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <utime.h>
#include <errno.h>

#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>

#ifdef MOTIF

#include <Xm/Text.h>
#include <Xm/PushB.h>
#include <Xm/Form.h>
#include <Xm/Label.h>
#include <Xm/Separator.h>

#ifdef HAVE_HTML
#include <HTML.h>
#endif

#else

#include <X11/Shell.h>
#include <X11/Xaw/AsciiText.h>
#include <X11/Xaw/Command.h>
#include <X11/Xaw/Form.h>
#include <X11/Xaw/Label.h>

#ifdef HAVE_HTML
#include <HTML.h>
#endif

#endif

#include "maindefs.h"
#include "main.h"

extern time_t motdChanged();
extern messageptr freeMessage();
extern Pixmap icon_pixmap;
extern char timeStamp[256];

extern messageptr msgslist;/* list of pointers to the motds requested
							  to be displayed */
extern app_res_t app_res;
extern XtAppContext app_con;
extern XtIntervalId timer;

char *txtbuf;					/* file is loaded into this malloc'd pointer */


void 
/*ARGSUSED*/
nextMessage(Widget w, caddr_t call_data, caddr_t client_data)
{

  char buffer[256];
  time_t ftime= time((time_t *)NULL);
  extern Widget info, quit;
  
#ifdef MOTIF
  XmString xmstr;
#endif

/*   fprintf(stderr,"nextMessage()\n"); */

  if (!msgslist)
    return;

  memset(buffer, 0, 256);
  displayMessage(msgslist->file);

  updateTimeStamp(getTimeStampName()); /* update the timestamp
										  whenever we pop-up to
										  display a message */

  if(app_res.showfilename)		/*show the filename */
	{
	  sprintf(buffer, "%s (%s)", ctime(&ftime), msgslist->file);
	}
  else 
	{
	  sprintf(buffer, "%s", ctime(&ftime));
	}

  *strchr(buffer, '\n')=' '; /* junk the \n at the end*/
  
#ifdef MOTIF
  xmstr=XmStringCreateLocalized(buffer);
  XtVaSetValues(info, XmNlabelString, xmstr, NULL);
  XmStringFree(xmstr);
#else
  XtVaSetValues(info, XtNlabel, buffer, NULL);
#endif
  
  /* next time through the loop, we continue at the next message*/
  msgslist = freeMessage(msgslist);

  /*figure out if we are displaying the last message;
	if we are, then change the button-label to
	"Dismiss" and re-direct the callback to Quit()*/
  if(!msgslist)
	{
	  extern void Quit(Widget w, caddr_t call_data, caddr_t client_data) ;
	  
/*	  fprintf(stderr, "msgindex=%d, displayed %s (LAST MESSAGE)\n", msgindex, buffer);*/
	  
#ifdef MOTIF
	  xmstr=XmStringCreateLocalized(LAST_MESSAGE_LABEL);
	  XtVaSetValues(quit, XmNlabelString, xmstr, NULL);
	  XmStringFree(xmstr);

	  XtRemoveAllCallbacks(quit, XmNactivateCallback);
	  
	  XtAddCallback(quit, XmNactivateCallback,
					(XtCallbackProc)Quit, 0);
#else
	  XtVaSetValues(quit, XtNlabel, LAST_MESSAGE_LABEL, NULL);
	  XtRemoveAllCallbacks(quit, XtNcallback);
	  
	  XtAddCallback(quit, XtNcallback,
					(XtCallbackProc)Quit, 0);
#endif
	  if(app_res.pto)
            timer=XtAppAddTimeOut(app_con, (unsigned long)(app_res.pto*1000),
                                (XtTimerCallbackProc)Quit, (caddr_t) NULL);
	  return;
	}
  if(app_res.pto)
    timer=XtAppAddTimeOut(app_con, (unsigned long)(app_res.pto*1000),
                   (XtTimerCallbackProc)nextMessage, (caddr_t) NULL);
}

void 
revisitMessagesAndDisplay(int numsg)
{
  extern int gargc;
  extern char **gargv;
  extern Widget topLevel, quit;
  extern void Quit(Widget w, caddr_t call_data, caddr_t client_data);
  
  /*  fprintf(stderr,"revisitMessagesAndDisplay()\n");*/

  if(numsg>1)
	{
#ifdef MOTIF
	  XmString xmstr;
	  xmstr=XmStringCreateLocalized(NEXT_MESSAGE_LABEL);
	  XtVaSetValues(quit, XmNlabelString, xmstr, NULL);
	  XmStringFree(xmstr);
	  
	  XtRemoveAllCallbacks(quit,XmNactivateCallback);

	  XtAddCallback(quit, XmNactivateCallback,
					(XtCallbackProc)nextMessage, 0);
#else	  
	  XtVaSetValues(quit, XtNlabel, NEXT_MESSAGE_LABEL, NULL);
	  XtRemoveAllCallbacks(quit,XtNcallback);
	  XtAddCallback(quit, XtNcallback,
					(XtCallbackProc)nextMessage, 0);
	  
#endif
	}
  /* find and display 1st msg*/
  nextMessage((Widget)NULL, (caddr_t)NULL, (caddr_t)NULL);
  XMapRaised(XtDisplay(topLevel), XtWindow(topLevel));
  XFlush(XtDisplay(topLevel));
  
}


int
displayMessage(char *filename)
{
  FILE *fp;
  struct stat motdstat;
  extern Widget text;

/*   fprintf(stderr,"displayMessage()\n"); */

#ifdef MOTIF
  XmString xmstr;
#endif

/*  memset((char *)(&motdstat), 0, sizeof(struct stat));*/
  
  if((fp=fopen(filename,"r"))==NULL)  /* Read it in...*/
	{
	  perror(filename);
	  return(0);				/* no file by this name*/
	}

  stat(filename,&motdstat);
  if(txtbuf) free(txtbuf);
  
  txtbuf=(char *)calloc(1, (motdstat.st_size+1)*sizeof(char));
  
  if(!txtbuf)
	{
	  extern XtAppContext app_con;

	  perror("xmotd");
	  XtDestroyApplicationContext(app_con);
	  exit(2);
	}

  fread(txtbuf,(int)motdstat.st_size,1,fp);

  fclose(fp);

#ifdef MOTIF

#ifdef HAVE_HTML

   HTMLSetText(text,txtbuf,NULL,NULL,0,NULL,NULL);

#else
  XtVaSetValues(text, XmNvalue, txtbuf, NULL);
#endif

#else

#ifdef HAVE_HTML

   HTMLSetText(text,txtbuf,NULL,NULL,0,NULL,NULL);

#else
  XtVaSetValues(text, XtNstring, txtbuf, NULL);
#endif

#endif

  return(1);
  
}/* displayMessage*/

