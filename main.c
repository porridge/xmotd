/* xmotd is a  message of the day displayer for X11 and dumb-terminals
 *
 * It displays a logo in the top-left corner, a 3-line title to its
 * right and a text-widget (optionally a HTML widget) for displaying
 * the message, just below. The motd filename(s) are supplied on the
 * command-line. A single button is used to sequentailly access the
 * motd(s) and to dismiss the browser when all the messages have been
 * viewed. A label displays the time of the file being viewed and
 * (optionally) the filename. It has options for automatically popping
 * down w/o user intervention and other features which I can't be
 * bothered to type-in here and would rather have you look at the
 * man-page or http://www.ee.ryerson.ca:8080/~elf/xmotd.html
 * */

/* $Id: main.c,v 1.19 2003/02/14 00:35:03 elf Exp elf $ */

/*
 * Copyright 1993-97, 1999, 2003 Luis Fernandes <elf@ee.ryerson.ca> 
 *
 * Permission to use, copy, hack, and distribute this software and its
 * documentation for any purpose and without fee is hereby granted,
 * provided that the above copyright notice appear in all copies and
 * that both that copyright notice and this permission notice appear
 * in supporting documentation.  This application is presented as is
 * without any implied or written warranty.
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
 * libhtmlw is copyright (C) 1993, Board of Trustees of the University
 * of Illinois. See the file libhtmlw/HTML.c for the complete text of
 * the NCSA copyright.
 */

#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <time.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <utime.h>
#include <errno.h>
#include <fcntl.h>

#if defined(HPUX)

#include <ndir.h>
#define dirent  direct

#else

#include <dirent.h>

#endif

#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <X11/Xmu/Editres.h>

#ifdef HAVE_XPM
#include <X11/xpm.h>
#endif

#ifdef MOTIF

#include <Xm/Text.h>
#include <Xm/PushB.h>
#include <Xm/Form.h>
#include <Xm/Label.h>
#include <Xm/Separator.h>

#else

#include <X11/Shell.h>
#include <X11/Xaw/AsciiText.h>
#include <X11/Xaw/Command.h>
#include <X11/Xaw/Form.h>
#include <X11/Xaw/Label.h>

#endif

#ifdef HAVE_HTML
#include <HTML.h>
#endif

#define MIN_SLEEP_PERIOD 60		/* in seconds */
#define HOURS_TO_SECS 3600.0

#include "maindefs.h"
#include "appdefs.h"
#include "main.h"

extern time_t motdChanged();
extern void nextMessage(Widget w, caddr_t call_data, caddr_t client_data);
extern void AnchorCallbackProc(Widget w, caddr_t call_data, caddr_t client_data); /* browser.c */

extern void loadLogo(char *logo, Pixmap *icon_pixmap, Pixel fg, Pixel bg); /* in logo.c */

/* fwd decls*/
XtActionProc reallyQuit(Widget w, XButtonEvent *ev);
void Quit(Widget w, caddr_t call_data, caddr_t client_data);
void printUsage(char *str);		/* usage.c */
int runSilentRunDeep(unsigned slumber);
unsigned getAlarmTime(float period);
messageptr freeMessage(messageptr msglist);
messageptr newMessage(char *file);

XtAppContext app_con;
Widget topLevel;				/* the application widget*/

Widget text, quit;
Widget info;

XtIntervalId timer;             /* pop-down time-out ID */

Pixmap icon_pixmap;

app_res_t app_res;				/* application resources, in main.h */

/* list of pointers to the messages that will actually be displayed */
messageptr msgslist;

int gargc;						/* globally accessible argc */
char **gargv;					/* globally accessible argv*/

char timeStamp[256];

static int alreadyForked;		/* flag to remember if we are already
								   running in the background */

#define offset(field)   XtOffset (struct _resources *, field)

static XtResource resources[] = {
  { "always","Always",XtRInt, sizeof(int),
	  offset(always),XtRString, "0"},

  { "popdown","Popdown",XtRInt, sizeof(int),
	  offset(pto),XtRString, "0"},
  { "usedomains","UseDomains",XtRInt, sizeof(int),
	  offset(usedomains),XtRString, "0"},
  { "showfilename","ShowFilename",XtRInt, sizeof(int),
	  offset(showfilename),XtRString, "0"},
  { "paranoid","Paranoid",XtRInt, sizeof(int),
	  offset(paranoid),XtRString, "0"},
  { "tail","Tail",XtRInt, sizeof(int),
	offset(tail),XtRString, "0"},
  { "bitmaplogo","BitmapLogo",XtRString, sizeof(String),
	  offset(logo),XtRString, NULL},
  { "warnfile","Warnfile",XtRString, sizeof(String),
	  offset(warnfile),XtRString, NULL},
  { "wakeup","Wakeup",XtRFloat, sizeof(float),
	  offset(periodic),XtRString, "0"},
  { "stampfile","Stampfile",XtRString, sizeof(String),
	  offset(stampfile),XtRString, TIMESTAMP},
  { "atom","Atom",XtRString, sizeof(String),
	  offset(atomname),XtRString, ATOM},
#ifdef HAVE_HTML
  { "browser","Browser",XtRString, sizeof(String),
	  offset(browser),XtRString, BROWSER},
#endif
};

static XrmOptionDescRec options[] = {
  { "-always",       "always",       XrmoptionNoArg,  "1"},
  { "-showfilename", "showfilename", XrmoptionNoArg,  "1"},
  { "-usedomains",   "usedomains",   XrmoptionNoArg,  "1"},
  { "-paranoid",     "paranoid",     XrmoptionNoArg,  "1"},
  { "-tail",         "tail",         XrmoptionNoArg,  "1"},
  { "-popdown",      "popdown",      XrmoptionSepArg, (caddr_t) NULL},
  { "-bitmaplogo",   "bitmaplogo",   XrmoptionSepArg, (caddr_t) NULL},
  { "-warnfile",     "warnfile",     XrmoptionSepArg, (caddr_t) NULL},
  { "-wakeup",       "wakeup",       XrmoptionSepArg, (caddr_t) NULL},
  { "-stampfile",    "stampfile",    XrmoptionSepArg, (caddr_t) NULL},
  { "-atom",         "atom",         XrmoptionSepArg, (caddr_t) NULL},
#ifdef HAVE_HTML
  { "-browser",      "browser",      XrmoptionSepArg, (caddr_t) NULL},
#endif
};

/* You can use shift + btn1 to quit xmotd (when run with -wakeup)*/
static char shift1Trans[]="#override\n\
        Shift<Btn1Down>,Shift<Btn1Up>: reallyquit()";

static XtActionsRec xlations[]={
        {"reallyquit", (XtActionProc) reallyQuit},
};

/* when the text widget is mapped and we are in "tail" mode, scroll to
   the end of the file*/
static char tailTrans[]="#override\n\
        <Expose>: end-of-file()";

char *
getTimeStampName()
{
  static char buf[256];
  
  sprintf(buf, "%s/%s", getenv("HOME"), app_res.stampfile);

  if(app_res.usedomains)
	{
	  char domainame[256];

	  getdomainname(domainame, 256);
  
	  strcat(buf, "."); 
	  strcat(buf, domainame);
	}

  return(buf);
  
}

/* convert user-specified wakeup time to seconds (argument to sleep)*/
unsigned 
getAlarmTime(float period)
{
  unsigned slumber=(period*HOURS_TO_SECS); 

  /*  fprintf(stderr,"slumber=%ld\n", slumber);*/

  /* limit sleep to 60sec*/
  return((slumber<MIN_SLEEP_PERIOD)?MIN_SLEEP_PERIOD:slumber);

}

/* NOTE: Jul 17 1996: This code doesn't work accurately for some
   reason; the mod-time fo a file doesn't matchup with the time
   returned by 'date' (this is on 4.1.3_U1). This is only a problem if
   the wakeup period is something like 1 minute (which I use for
   testing) -elf*/

void
updateTimeStamp(char *motdfile)
{
  struct utimbuf ut;
  time_t now;

/*   fprintf(stderr, "Updating timestamp %s now.\n", getTimeStampName() );  */

  now = time((time_t *)NULL);

  ut.actime = now;
  ut.modtime = now;

  if(utime(motdfile,&ut))
	{
	  extern int errno;

	  if(errno==ENOENT)			/* if the file does not (1st time), 
								   create it */
		{
		  FILE *fp;
		  
		  if((fp=fopen(motdfile,"w"))==NULL){
			perror("xmotd");
		  }
		  
		  fclose(fp);

		}
	  else						/* some other problem */
		{
		  perror("xmotd");
		  exit(1);
		}
	}
  
}



XtActionProc 
reallyQuit(Widget w, XButtonEvent *ev )
{
  extern char *txtbuf;

  if(txtbuf) free(txtbuf);

  if(icon_pixmap)
	{
	  XFreePixmap(XtDisplay(topLevel), icon_pixmap);
	}

  XtDestroyApplicationContext(app_con);

/*  fprintf(stderr,"Bye.\n");*/
  exit(0);
  
}


void 
/*ARGSUSED*/
Quit(Widget w, caddr_t call_data, caddr_t client_data)
{
  extern char *txtbuf;
  
  if(!app_res.periodic)
	{
	  /* we can exit because -wakeup is not specified*/
	  reallyQuit((Widget)NULL, (XButtonEvent *)NULL );
	}
  else  
	{
	  extern void revisitMessagesAndDisplay(int);

	  XUnmapWindow(XtDisplay(topLevel), XtWindow(topLevel));
	  XFlush(XtDisplay(topLevel));

	  if(!alreadyForked)
		{
		  if(fork()) exit(0);
		  
		  alreadyForked=1;
		}
	  
	  revisitMessagesAndDisplay(runSilentRunDeep(getAlarmTime(app_res.periodic)));
	  
	}

}

messageptr
freeMessage(messageptr msglist)
{
  messageptr oldmsg;
  oldmsg = msglist;
  msglist = msglist->next;
  
  free(oldmsg->file);
  free(oldmsg);
  return(msglist);

} /*freeMessage*/

messageptr
newMessage(char *file)
{
  messageptr newmsg;

  if (!(newmsg = (messageptr)malloc(sizeof(struct messagenode)))) 
	{
	  perror("xmotd");
	  exit (2);					/*problems, probably no ram HA  */
	} /*if*/

  newmsg->file=(char *)calloc(1,(strlen(file)+1)*sizeof(char));

  if(!newmsg->file) 
	{
	  perror("xmotd");
	  exit(2);
	}
 
  strcpy(newmsg->file, file);
  newmsg->next = NULL;
  return(newmsg);

} /*newMessage*/


int 
numFilesToDisplay(int argc, char **argv)
{
  register int numsg=0, i;
  struct stat motdstat;
  messageptr newmsg, currentmsg = msgslist;
  
  /* i starts at 1 since argv[0] is the program name*/
  for(i=1; i<argc; i++) 
	{
      stat(argv[i], &motdstat);
      
	  if(motdstat.st_mode & S_IFDIR) /* it's a directory */
		{
		  DIR *dir;
		  struct dirent *dp;
		  char name[BUFSIZ];

		  if ((dir = opendir(argv[i]))) 
			{
			  while (dp = readdir(dir)) 
				{
				  if (dp->d_ino == 0)
					continue;
				  if (!strcmp(dp->d_name, ".") || !strcmp(dp->d_name, ".."))
					continue;
				  strcpy(name, argv[i]);

				  if (name[strlen(name) - 1] != '/')
					strcat(name, "/");
				  strcat(name, dp->d_name);

				  if (access(name, 0) < 0)
					continue;

				  if(motdChanged(name, timeStamp) || app_res.always) 
					{
					  newmsg = newMessage(name);

					  if (!currentmsg) /* first in list */
						msgslist = newmsg;
					  else
						currentmsg->next = newmsg;

					  currentmsg = newmsg;
					  numsg++;
					} /*if motdchanged*/

                } /*while*/

              closedir(dir);
            } /*if opendir*/
        }
      else
        if(motdChanged(argv[i], timeStamp) || app_res.always) 
		  {
            newmsg = newMessage(argv[i]);

            if (!currentmsg)	/* first in list */
              msgslist = newmsg;
            else
              currentmsg->next = newmsg;

            currentmsg = newmsg;
            numsg++;
          }
    }
  
  if((numsg || app_res.paranoid || app_res.always) && app_res.warnfile) 
	{
      newmsg = newMessage(app_res.warnfile);
      newmsg->next = msgslist;
      msgslist = newmsg;
      numsg++;
    } /*if*/
  
  return numsg;
  
}/* numFilesToDisplay*/

int
runSilentRunDeep(unsigned slumber)
{
  int numsg=0;
  
  while(1)
	{
	  int fd;
	  
	  /*	  fprintf(stderr, "Going to sleep! (Zzzzzzzz...)\n");*/
	  sleep(slumber);
	  /*	  fprintf(stderr, "Waking-up! (Yawn...)\n");*/

	  /* first thing we do after waking up is to see if the user is
         still logged-in*/
	  if((fd=open("/dev/console", O_RDONLY, O_RDONLY)<0))
		{
		  /* since xmotd can't read from the console we assume user
             has logged-out, so we exit*/
		  exit(0);
		}

	  close(fd);
	  
	  /* next check if any messages need to be displayed, if there
         aren't any, go back to sleep; otherwise return to display
         messages*/
	  if(numsg=numFilesToDisplay(gargc, gargv)) return(numsg);
	}

}


main(argc, argv)
int argc;
char **argv;
{
  extern Boolean atomExists(String);
  Display *display;
  register int i, start=0;
  int numsg;

  
  if ((argc > 1) && !(strcmp(argv[1],"-help")))
	{
	  printUsage(argv[0]);		/* and exit */
	}

  /* Test to see whether we are connected to an X display. If we
	 aren't, we proceed in text-only mode: bare-bones functionality;
	 output to stdout.  Why bare-bones, I hear you asking? Well, X
	 does all the command-line options parsing for me and I don't feel
	 like duplicating all that code. So there.*/

  if((display=XOpenDisplay((char *)NULL))==NULL) 
	{

	  if(argc<2)
		{
		  fprintf(stderr, "xmotd: ERROR, missing file.\n");
		  printUsage(argv[0]);	/* and exit */
		}
	  else
		{
		  extern void runInTextMode();
		  runInTextMode(argc, argv); /* ...and exit... */
		} 

	  fprintf(stderr,"Never gets here!\n");
	  exit(0);				/* just in case */
	  
	} 
  else 
	{
	  XCloseDisplay(display);
	}
  
  /* we have to init the toolkit *before* we check the command-line so
     we can use X's parsing routines, since -geom options, etc. may be
     specified, in which case, the motd-filename is *not* the 2nd
     argument*/
  topLevel = XtVaAppInitialize(&app_con, "XMotd", options, 
							   XtNumber(options),
							   &argc, argv, fallback_resources, 
							   NULL);

  XtGetApplicationResources(topLevel, (caddr_t) &app_res,
							resources, XtNumber(resources),
							(ArgList) NULL, (Cardinal) 0);

  if(argc<2)
	{
	  fprintf(stderr,"xmotd: ERROR, missing file\n");
	  printUsage(argv[0]);	/* and exit */
	}
  
  if(app_res.paranoid && !app_res.warnfile)
	{
	  fprintf(stderr,"xmotd: ERROR, specified \"-paranoid\" without \"-warnfile\"\n");
	  printUsage(argv[0]);	/* and exit */
	}

  strcpy(timeStamp, getTimeStampName());

  gargc=argc;
  gargv=argv;
  
  /* first figure out how many of the files supplied on the
     command-line we will be actually displaying; i.e. we only show
     the new ones (unless -always has been specified, in which case we
     show all of them)*/
  numsg=numFilesToDisplay(argc, argv);

  if(!app_res.periodic && !numsg)
	{
	  /* if none of the messages need to be displayed and -wakeup not
	  specified */

	  XtDestroyApplicationContext(app_con);		
	  exit(0);
	}  

  if(app_res.periodic)			/*-wakeup or -timeout specified*/
	{

	  /*ensure no other copies of xmotd are running*/
	  if(atomExists(app_res.atomname)){
		XtDestroyApplicationContext(app_con);		
		exit(0);
	  }

	  if(fork()) exit(0);		/*we have to daemonize ourselves*/
	  alreadyForked=1;			/* make a note of it */

	  if(!numsg)
		{
		  /* if no messages to be displayed, we sleep */
		  numsg=runSilentRunDeep(getAlarmTime(app_res.periodic));
		}

	}

  createWidgets(numsg);
  nextMessage((Widget)NULL, (caddr_t)NULL, (caddr_t)NULL);  

  XtAddEventHandler(topLevel, (EventMask)0, True,
					(XtEventHandler)_XEditResCheckMessages, 0);

  XtRealizeWidget(topLevel);  
  XtAppMainLoop(app_con);
}


createWidgets(int anymsg)
{
  Widget form, paned, logo, mlabel, hline;
  XtTranslations shift1TransTable, tailTransTable;
  Pixel fg, bg;
  Arg args[8];
  int n;
  
#ifdef MOTIF
  XmString xmstr;

  form=XtVaCreateManagedWidget("form", xmFormWidgetClass,topLevel, 
							   NULL);
#else

  form=XtVaCreateManagedWidget("form", formWidgetClass,topLevel, 
							   XtNresizable, True,
							   NULL);
#endif /* ifdef MOTIF */

  XtVaGetValues(form, 
				XtNbackground, &bg,
				XtNforeground, &fg, 
				NULL);

  loadLogo(app_res.logo, &icon_pixmap, fg, bg);
  
#ifdef MOTIF
  logo=XtVaCreateManagedWidget("logo", xmLabelWidgetClass, form, 
							   XmNleftAttachment, XmATTACH_FORM,
							   XmNtopAttachment, XmATTACH_FORM,
							   XmNlabelType, XmPIXMAP,
							   XmNlabelPixmap, icon_pixmap,
							   XmNborderWidth, 0,
							   NULL);

  mlabel=XtVaCreateManagedWidget("title", xmLabelWidgetClass, form, 
								 XmNleftAttachment, XmATTACH_WIDGET,
 								 XmNleftWidget, logo,
								 XmNrightAttachment, XmATTACH_FORM,
								 NULL);

  hline=XtVaCreateManagedWidget("hline", xmSeparatorWidgetClass, form, 
								 XmNtopAttachment, XmATTACH_WIDGET,
								 XmNtopWidget, logo,
								 NULL);

  quit = XtVaCreateManagedWidget("quit", xmPushButtonWidgetClass, form, 
								 XmNtopAttachment, XmATTACH_WIDGET,
								 XmNtopWidget, logo,
								 XmNlabelType, XmSTRING,

								 XmNtraversalOn, False, /* remove
								 Dismiss button from the Tab group;
								 comment this line out if YOU WANT
								 "Space-bar" to activate the dismiss
								 button*/
								 NULL);

  info=XtVaCreateManagedWidget("info", xmLabelWidgetClass, form, 
							   XmNleftAttachment, XmATTACH_FORM,
							   XmNtopAttachment, XmATTACH_WIDGET,
							   XmNtopWidget, quit,
							   XmNlabelType, XmSTRING,
							   NULL);

#ifdef HAVE_HTML

   n = 0;
  XtSetArg(args[n],XmNtopAttachment, XmATTACH_WIDGET); n++;
  XtSetArg(args[n],XmNtopWidget, info); n++;
  XtSetArg(args[n],XmNbottomAttachment, XmATTACH_FORM); n++;

  text = XtVaCreateManagedWidget("text", htmlWidgetClass, form,NULL);

  XtSetValues(text,args,n);

  XtManageChild(text);

#else

  n=0;
  XtSetArg(args[n], XmNeditMode, XmMULTI_LINE_EDIT); n++;
  XtSetArg(args[n], XmNeditable, False); n++;
  XtSetArg(args[n], XmNscrollHorizontal, False); n++;
  XtSetArg(args[n], XmNscrollLeftSide, True); n++;
  XtSetArg(args[n], XmNcursorPositionVisible, False); n++;

  text=XmCreateScrolledText(form, "text", args, n);
  n=0;
  
  XtVaSetValues(XtParent(text), 
				XmNtopAttachment, XmATTACH_WIDGET,
				XmNtopWidget, info,
				XmNbottomAttachment, XmATTACH_FORM,
				NULL);
  
  XtManageChild(text);
#endif /* ifdef HAVE_HTML & ifdef MOTIF */

#else

  logo=XtVaCreateManagedWidget("logo", labelWidgetClass, form, 
								XtNleft, XtChainLeft,
								XtNright, XtChainLeft,
								XtNbitmap, icon_pixmap,
								NULL);
  
  mlabel=XtVaCreateManagedWidget("title", labelWidgetClass, form, 
								 XtNfromHoriz, logo,
								 NULL);

  hline=XtVaCreateManagedWidget("hline", labelWidgetClass, form, 
								 XtNfromVert, logo,
								 XtNfromVert, mlabel,
								 XtNlabel, (char *)NULL,
								 NULL);

  quit = XtVaCreateManagedWidget("quit", commandWidgetClass, form, 
								  XtNleft, XtChainLeft,
								  XtNright, XtChainLeft,
								  XtNfromVert, logo,
								  NULL);

  info=XtVaCreateManagedWidget("info", labelWidgetClass, form, 
							   XtNright, XtChainLeft,
							   XtNfromVert, quit,
							   XtNresizable, True,
							   NULL);

#ifdef HAVE_HTML
  text = XtVaCreateManagedWidget("text",
								 htmlWidgetClass, form,
								 XtNfromVert, info,
								 XtNwidth, 680,
								 XtNheight, 500,
								 NULL);

#else
  text = XtVaCreateManagedWidget("text",
								 asciiTextWidgetClass, form, 
								 XtNfromVert, info,
								 NULL);

#endif /* ifdef HAVE_HTML */

  if(app_res.tail)
	{
	  tailTransTable=XtParseTranslationTable(tailTrans);
	  XtOverrideTranslations(text, tailTransTable);
	}
  
#endif /* ifdef MOTIF */
	
  if((anymsg>1))
	{
#ifdef MOTIF 
	  xmstr=XmStringCreateLocalized(NEXT_MESSAGE_LABEL);
	  XtVaSetValues(quit, XmNlabelString, xmstr, NULL);
	  XmStringFree(xmstr);

	  XtAddCallback(quit, XmNactivateCallback, (XtCallbackProc)nextMessage, 0);
#else
	  XtAddCallback(quit, XtNcallback, (XtCallbackProc)nextMessage,(caddr_t)0);
	  XtVaSetValues(quit, XtNlabel, NEXT_MESSAGE_LABEL, NULL);
#endif
  	  if(app_res.pto)
	    timer=XtAppAddTimeOut(app_con, (unsigned long)(app_res.pto*1000),
			(XtTimerCallbackProc)nextMessage, (caddr_t) NULL);
	}
  else
	{
#ifdef MOTIF 
	  xmstr=XmStringCreateLocalized(LAST_MESSAGE_LABEL);
	  XtVaSetValues(quit, XmNlabelString, xmstr, NULL);
	  XmStringFree(xmstr);

	  XtAddCallback(quit, XmNactivateCallback, (XtCallbackProc)Quit, 0);
#else
	  XtAddCallback(quit, XtNcallback, (XtCallbackProc)Quit, 0);
	  XtVaSetValues(quit, XtNlabel, LAST_MESSAGE_LABEL, NULL);
#endif
  	  if(app_res.pto)
	    timer=XtAppAddTimeOut(app_con, (unsigned long)(app_res.pto*1000),
			(XtTimerCallbackProc)Quit, (caddr_t) NULL);
	}

#ifdef HAVE_HTML
	  XtAddCallback(text, WbNanchorCallback, 
					(XtCallbackProc)AnchorCallbackProc,(caddr_t)0);
#endif
  
  XtAppAddActions(app_con, xlations, XtNumber(xlations));
  shift1TransTable=XtParseTranslationTable(shift1Trans);
  XtOverrideTranslations(quit, shift1TransTable);
  
}/* createWidgets*/

